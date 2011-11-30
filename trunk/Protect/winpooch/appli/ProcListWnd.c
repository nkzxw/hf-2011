/******************************************************************/
/*                                                                */
/*  Winpooch : Windows Watchdog                                   */
/*  Copyright (C) 2004-2006  Benoit Blanchon                      */
/*                                                                */
/*  This program is free software; you can redistribute it        */
/*  and/or modify it under the terms of the GNU General Public    */
/*  License as published by the Free Software Foundation; either  */
/*  version 2 of the License, or (at your option) any later       */
/*  version.                                                      */
/*                                                                */
/*  This program is distributed in the hope that it will be       */
/*  useful, but WITHOUT ANY WARRANTY; without even the implied    */
/*  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       */
/*  PURPOSE.  See the GNU General Public License for more         */
/*  details.                                                      */
/*                                                                */
/*  You should have received a copy of the GNU General Public     */
/*  License along with this program; if not, write to the Free    */
/*  Software Foundation, Inc.,                                    */
/*  675 Mass Ave, Cambridge, MA 02139, USA.                       */
/*                                                                */
/******************************************************************/


/******************************************************************/
/* Build configuration                                            */
/******************************************************************/

#define	TRACE_LEVEL	2


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "ProcListWnd.h"

// standard headers
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>

// project's headers
#include "Assert.h"
#include "Language.h"
#include "ProcList.h"
#include "ProjectInfo.h"
#include "Resources.h"
#include "SpyServer.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

// column identifiers
#define COL_NAME	0
#define COL_PID		1
#define COL_STATE	2
#define COL_PATH	3

// window class name
#define WC_PROCWND TEXT("ProcListWnd")

// messages
#define WM_REFRESH	(WM_USER+10)

LPCTSTR g_szProcessMenu = TEXT("Process menu") ;
LPCTSTR g_szKillProcess = TEXT("Kill selected process") ;
LPCTSTR g_szHookProcess = TEXT("Hook selected process") ;
LPCTSTR g_szUnhookProcess = TEXT("Unhook selected process") ;
LPCTSTR g_szConfirmKillProcess = TEXT("Are you sure you do want to kill this process ?") ;
LPCTSTR g_szConfirmKillProtectedProcess = TEXT("This process is protected, are you sure you do want to kill it ?") ;
LPCTSTR g_szFailedToKillProcess = TEXT("Failed to kill process") ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

LRESULT CALLBACK _ProcListWnd_WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) ;

BOOL _ProcListWnd_AddProcess (void *, PROCSTRUCT *) ;

VOID _ProcListWnd_UpdateProcessId (HWND hwndList, PROCADDR) ;

VOID _ProcListWnd_UpdateProcess (HWND hwndList, PROCSTRUCT*) ;

VOID _ProcListWnd_RemProcess (HWND hwndList, PROCADDR) ;

int CALLBACK _ProcListWnd_ItemCompare (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) ;


/******************************************************************/
/* Exported function : RegisterClass                              */
/******************************************************************/

BOOL ProcListWnd_RegisterClass (HINSTANCE hInstance) 
{
  WNDCLASS wndclass ;

  wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
  wndclass.lpfnWndProc   = _ProcListWnd_WndProc ;
  wndclass.cbClsExtra    = 0 ;
  wndclass.cbWndExtra    = 0 ;
  wndclass.hInstance     = hInstance ;
  wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION) ;
  wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
  wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1) ;
  wndclass.lpszMenuName  = NULL ;
  wndclass.lpszClassName = WC_PROCWND ;

  return 0!=RegisterClass (&wndclass) ;
}


/******************************************************************/
/* Exported function : CreateWindow                               */
/******************************************************************/

HWND ProcListWnd_CreateWindow (HINSTANCE hInstance, HWND hwndParent)
{  
  return  CreateWindow (WC_PROCWND, NULL,
			WS_CHILD,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, 
			hwndParent, NULL, hInstance, NULL) ;
}


/******************************************************************/
/* Internal function : WndProc                                    */
/******************************************************************/

LRESULT CALLBACK _ProcListWnd_WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  static HWND		g_hwndList ;
  static HINSTANCE	g_hInstance ;
  static HMENU		g_hmenuProcess ;
  static PROCADDR	g_nSelectedProcessAddr ;
  static int		g_nSortParam ;

  int		nWidth ;
  int		nHeight ;
  LV_COLUMN	lvc ;
  HIMAGELIST	hImList ;


  switch (message)
    {
    case WM_CREATE:
      {
	g_hInstance = ((CREATESTRUCT*)lParam)->hInstance ;
	
	g_hwndList = CreateWindowEx (WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL,
				     WS_CHILD|WS_VISIBLE|WS_VSCROLL|LVS_REPORT|LVS_SINGLESEL,
				     0,0,0,0, hwnd, (HMENU)IDC_PROCESSLIST, g_hInstance, NULL) ;
	ListView_SetExtendedListViewStyle (g_hwndList, LVS_EX_FULLROWSELECT|LVS_EX_SUBITEMIMAGES) ;
	
	//
	// Init image list for process list
	//
	hImList = ImageList_Create (16,16,ILC_COLOR32,16,8) ;
	ListView_SetImageList (g_hwndList, hImList, LVSIL_SMALL) ;
	
	g_nSelectedProcessAddr = 0 ;
	g_nSortParam = -1 ;

	// menu will be created by WM_LANGUAGECHANGED
	g_hmenuProcess = NULL ;

	hImList = ImageList_Create (8, 8, ILC_COLOR|ILC_MASK, 2, 16) ;
	ImageList_AddIcon (hImList, LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_COLUMN_UP))) ;
	ImageList_AddIcon (hImList, LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_COLUMN_DOWN))) ;

	Header_SetImageList (ListView_GetHeader(g_hwndList), hImList) ;
      
	// clear LVC struct
	ZeroMemory (&lvc, sizeof(lvc)) ;
	lvc.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM | LVCF_IMAGE | LVCF_FMT;
	lvc.fmt = LVCFMT_BITMAP_ON_RIGHT ;
	lvc.pszText = TEXT("") ;
	lvc.iImage = -1 ;
      
	// add process name column
	lvc.cx		= 100 ;
	lvc.iSubItem	= COL_NAME ;
	ListView_InsertColumn (g_hwndList, lvc.iSubItem+1, &lvc) ; 
      
	// add PID column
	lvc.cx		= 60 ;
	lvc.iSubItem	= COL_PID ;
	ListView_InsertColumn (g_hwndList, lvc.iSubItem+1, &lvc) ; 

	// add state column
	lvc.cx		= 150 ;
	lvc.iSubItem	= COL_STATE ;
	ListView_InsertColumn (g_hwndList, lvc.iSubItem+1, &lvc) ; 
      
	// add path
	lvc.cx		= 500 ;
	lvc.iSubItem	= COL_PATH ;
	ListView_InsertColumn (g_hwndList, lvc.iSubItem+1, &lvc) ; 

	SendMessage (hwnd, WM_LANGUAGECHANGED, 0, 0) ;
      }
	
      return 0 ; // case WM_CREATE:

    case WM_LANGUAGECHANGED:

      // clear LVC struct
      ZeroMemory (&lvc, sizeof(lvc)) ;
      lvc.mask = LVCF_TEXT ;
      
      // add process name column
      lvc.pszText = (LPTSTR) STR_DEF (_PROCESS, TEXT("Process")) ;
      ListView_SetColumn (g_hwndList, COL_NAME, &lvc) ; 
      
      // add PID column
      lvc.pszText = (LPTSTR) STR_DEF (_PID, TEXT("PID")) ;
      ListView_SetColumn (g_hwndList, COL_PID, &lvc) ; 

      // add state column
      lvc.pszText = (LPTSTR) STR_DEF (_STATE, TEXT("State")) ;
      ListView_SetColumn (g_hwndList, COL_STATE, &lvc) ; 
      
      // add path
      lvc.pszText = (LPTSTR) STR_DEF (_PATH, TEXT("Path")) ;
      ListView_SetColumn (g_hwndList, COL_PATH, &lvc) ; 

      // create processes menu
      DestroyMenu (g_hmenuProcess) ;
      g_hmenuProcess = CreatePopupMenu () ;
      AppendMenu (g_hmenuProcess, MF_STRING|MF_GRAYED, 0, 
		  STR_DEF(_PROCESS_MENU,g_szProcessMenu)) ;
      AppendMenu (g_hmenuProcess, MF_SEPARATOR, 0, NULL) ;
      AppendMenu (g_hmenuProcess, MF_STRING, IDM_PROCESS_HOOK, 
		  STR_DEF(_HOOK_SELECTED_PROCESS,g_szHookProcess)) ;
      AppendMenu (g_hmenuProcess, MF_STRING, IDM_PROCESS_UNHOOK, 
		  STR_DEF(_UNHOOK_SELECTED_PROCESS,g_szUnhookProcess)) ;
      AppendMenu (g_hmenuProcess, MF_STRING, IDM_PROCESS_KILL, 
		  STR_DEF(_KILL_SELECTED_PROCESS,g_szKillProcess)) ;
      
      
      PostMessage (hwnd, WM_REFRESH, 0, 0) ;

      return 0 ;
      
    case WM_DESTROY:

      return 0 ;

    case WM_SIZE:
      nWidth = LOWORD (lParam) ;
      nHeight = HIWORD (lParam) ;
      MoveWindow (g_hwndList, 0, 0, nWidth, nHeight, TRUE) ;

      return 0 ;

    case WM_REFRESH: // ======== Refresh process list ========
      {
	LVFINDINFO	lvfi ;
	int		iItem ;
	HIMAGELIST	hImageList ;

	hImageList = ListView_GetImageList (g_hwndList, LVSIL_SMALL) ;

	ListView_DeleteAllItems (g_hwndList) ;	

       	ImageList_RemoveAll (hImageList) ;
	ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_STATE_UNKNOWN))) ;
	ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_STATE_DISABLED))) ;
	ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_STATE_HOOKED))) ;

	ProcList_Lock () ;
	ProcList_Enum (_ProcListWnd_AddProcess, g_hwndList) ;
	ProcList_Unlock () ;

	if( g_nSortParam >= 0 )
	  ListView_SortItems (g_hwndList, _ProcListWnd_ItemCompare, g_nSortParam) ;

	if( g_nSelectedProcessAddr )
	  {
	    lvfi.flags = LVFI_PARAM ;
	    lvfi.lParam = g_nSelectedProcessAddr ;
	    
	    iItem = ListView_FindItem (g_hwndList, -1, &lvfi) ;
	    	    
	    if( iItem>=0 )
	      ListView_SetItemState (g_hwndList, iItem, LVIS_SELECTED, LVIS_SELECTED) ;
	  }
      }      
      return 0 ; // ======== Refresh process list ========

    case WM_SPYNOTIFY:
      {
	switch( wParam )
	  {

	  case SN_PROCESSCREATED: // ==== Process Created ====
	    {
	      PROCSTRUCT	*pProc ;

	      ProcList_Lock () ;
	      pProc = ProcList_Get (lParam) ;

	      if( pProc )
		_ProcListWnd_AddProcess (g_hwndList, pProc) ;
	      else
		TRACE_ERROR (TEXT("ProcList_Get failed (PID=%u)\n"), lParam) ;
	      
	      ProcList_Unlock () ;
	    }
	    return 0 ; // ==== Process Created ====

	  case SN_PIDCHANGED: // ==== Process ID changed ====
	    {
	      _ProcListWnd_UpdateProcessId (g_hwndList, lParam) ;	      
	    }
	    return 0 ; // ==== Process ID chanegd ====

	  case SN_PROCESSCHANGED: // ==== Process info changed ====
	    {
	      PROCSTRUCT * pProc ;

	      ProcList_Lock () ;
	      pProc = ProcList_Get (lParam) ;
	      if( pProc ) _ProcListWnd_UpdateProcess (g_hwndList, pProc) ;	      
	      ProcList_Unlock () ;
	    }
	    return 0 ; // ==== Process info chanegd ====

	  case SN_PROCESSTERMINATED: // ==== Process terminated ====
	    {
	      _ProcListWnd_RemProcess (g_hwndList, lParam) ;	      
	    }
	    return 0 ; // ==== Process terminated ====
	  }
      }
      return 0 ;

    case WM_COMMAND:
      
      switch( LOWORD(wParam))
	{
	case IDM_PROCESS_HOOK: //  ======== 'Hook selected process' command ========
	  {
	    SpySrv_IgnoreProcess (g_nSelectedProcessAddr, FALSE) ;
	  }
	  return 0 ; //  ======== 'Hook selected process' command ========

	case IDM_PROCESS_UNHOOK: //  ======== 'Unhook selected process' command ========
	  {
	    SpySrv_IgnoreProcess (g_nSelectedProcessAddr, TRUE) ;
	  }
	  return 0 ; //  ======== 'Unhook selected process' command ========

	case IDM_PROCESS_KILL: //  ======== 'Kill selected process' command ========
	  {
	    BOOL	bSuccess ;
	    UINT	n ;
	    PROCADDR	nProcessAddress = g_nSelectedProcessAddr ;
	    g_nSelectedProcessAddr = 0 ;

	    // no process selected ? no need to go on.
	    if( ! nProcessAddress ) return 0 ;
	      
	    // I need confirmation before killing a process
	    n = MessageBox (hwnd, 
			    STR_DEF(_CONFIRM_KILL_PROCESS,g_szConfirmKillProcess),
			    TEXT(APPLICATION_NAME),
			    MB_ICONWARNING|MB_YESNO) ;	    
	    if( n!=IDYES ) return 0 ;
	      
	    // first try a user mode kill
	    bSuccess = SpySrv_KillProcess (nProcessAddress, FALSE) ;		
	    if( bSuccess ) return 0 ;
	    
	    // I need a second confirmation to kill a protected process
	    n = MessageBox (hwnd, 
			    STR_DEF(_CONFIRM_KILL_PROTECTED_PROCESS,g_szConfirmKillProtectedProcess),
			    TEXT(APPLICATION_NAME),
			    MB_ICONWARNING|MB_YESNO) ;
	    
	    if( n!=IDYES ) return 0 ;

	    // now try a kernel mode kill
	    bSuccess = SpySrv_KillProcess (nProcessAddress, TRUE) ;

	    if( !bSuccess )
	      MessageBox (hwnd,
			  STR_DEF(_FAILED_TO_KILL_PROCESS,g_szFailedToKillProcess),
			  TEXT(APPLICATION_NAME), MB_ICONERROR) ;	  
	  }
	  return 0 ; //  ======== 'Kill selected process' command ========
	}

      break ; // case WM_COMMAND:

    case WM_NOTIFY: // ============ 'notify' message on 'processes' window ==============
      {
        union {    
	  NMHDR			*header ;
	  NMITEMACTIVATE	*itemactivate ;
	  NMLISTVIEW		*listview ; 
	  NMTTDISPINFO		*getdispinfo ; 
	} pnm ;

	pnm.header = (NMHDR*)lParam ;
	
	switch( pnm.header->idFrom )
	  {
	  case IDC_PROCESSLIST:
	    
	    switch( pnm.header->code )
	      {
	      case LVN_ITEMCHANGED: // ======== Item changed on 'processes' list-view ========
		{
		  // selection changed
		  if( pnm.listview->uNewState & LVIS_SELECTED )
		    {
		      // get pid of selected process
		      g_nSelectedProcessAddr = pnm.listview->lParam ;
		    }			  
		}			      
		return 0 ; //  ======== Item changed on 'processes' list-view ========

	      case NM_RCLICK: //  ======== Right click on 'processes' list-view ======== 
		{
		  POINT		pt ;		  
		  BOOL		bCanKill = FALSE ;
		  BOOL		bCanHook = FALSE ;
		  BOOL		bCanUnhook = FALSE ;

		  if( g_nSelectedProcessAddr )
		    {
		      PROCSTRUCT	*pProc ;

		      ProcList_Lock () ;
		      
		      pProc = ProcList_Get (g_nSelectedProcessAddr) ;
		      
		      if( pProc ) 
			{
			  BOOL bIsCurProcess = pProc->nProcessId==GetCurrentProcessId() ;
		  
			  bCanKill = !bIsCurProcess ;
			  bCanHook = !bIsCurProcess && pProc->nState==PS_HOOK_DISABLED ;
			  bCanUnhook = !bIsCurProcess && pProc->nState!=PS_HOOK_DISABLED ;
			}
		      else
			TRACE_ERROR (TEXT("Process 0x%08X not in process list\n"), g_nSelectedProcessAddr) ;
		      
		      ProcList_Unlock () ;
		    }

		  // enable/disable 'hook' menu item
		  EnableMenuItem (g_hmenuProcess, IDM_PROCESS_HOOK, 
				  MF_BYCOMMAND|(bCanHook?MF_ENABLED:MF_GRAYED)) ;

		  // enable/disable 'unhook' menu item
		  EnableMenuItem (g_hmenuProcess, IDM_PROCESS_UNHOOK, 
				  MF_BYCOMMAND|(bCanUnhook?MF_ENABLED:MF_GRAYED)) ;

		  // enable/disable 'kill' menu item		  
		  EnableMenuItem (g_hmenuProcess, IDM_PROCESS_KILL, 
				  MF_BYCOMMAND|(bCanKill?MF_ENABLED:MF_GRAYED)) ;

		  // get mouse position
		  GetCursorPos (&pt) ;	
		  
		  // display menu
		  TrackPopupMenu (g_hmenuProcess, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL) ;		
		}
		return 0 ; //  ======== Right click on 'processes' list-view ======== 

	      case LVN_COLUMNCLICK: //  ======== Click on column header ======== 
		{
		  LVCOLUMN	lvc ;
		  int		i ;

		  if( g_nSortParam == pnm.listview->iSubItem )
		    g_nSortParam |= 0x80 ;
		  else
		    g_nSortParam = pnm.listview->iSubItem ;	
		  
		  ListView_SortItems (g_hwndList, _ProcListWnd_ItemCompare, g_nSortParam) ;
		  
		  for( i=0 ; i<4 ; i++ )
		    {
		      ZeroMemory (&lvc, sizeof(lvc)) ;

		      lvc.mask = LVCF_IMAGE ;
		      lvc.iImage = i!=pnm.listview->iSubItem ? -1 :
			g_nSortParam&0x80 ? 1 : 0 ;		   
		      
		      ListView_SetColumn (g_hwndList, i, &lvc) ;
		    }
		}
		return 0 ; //  ======== Click on column header ======== 
	      }
	  }
	}
         
      return 0 ; // ============ 'notify' message on 'processes' window ==============
    }

  return DefWindowProc (hwnd, message, wParam, lParam) ;
}


/******************************************************************/
/* Internal function :                                            */
/******************************************************************/

BOOL _ProcListWnd_AddProcess (void * pContext, PROCSTRUCT * pProc) 
{
  LVITEM lvi = { 0 } ;
  HWND hwndList = (HWND)pContext ;

  ASSERT (pProc!=NULL) ;

  lvi.mask = LVIF_PARAM ;
  lvi.iItem = ListView_GetItemCount (hwndList) ;
  lvi.iSubItem = 0 ;
  lvi.lParam = (LPARAM)pProc->nProcessAddress ;
  ListView_InsertItem (hwndList, &lvi) ;

  _ProcListWnd_UpdateProcess (hwndList, pProc) ;

  return TRUE ;
}

VOID _ProcListWnd_UpdateProcess (HWND hwndList, PROCSTRUCT *pProc) 
{
  LVFINDINFO	lvfi = { 0 } ;
  LVITEM	lvi = { 0 } ;
  TCHAR		szBuffer[16] ;
  int		iImage, i ;
  HICON		hIcon ;
  SHFILEINFO	sfi ;
  HIMAGELIST	hImageList = ListView_GetImageList (hwndList, LVSIL_SMALL) ;

  lvfi.flags = LVFI_PARAM ;
  lvfi.lParam = (LPARAM)pProc->nProcessAddress ;
  
  i = ListView_FindItem (hwndList,-1,&lvfi) ;

  if( i < 0 ) 
    {
      TRACE_ERROR (TEXT("Process 0x%08X not in list\n"), pProc->nProcessAddress) ;
      return ;
    }

  // get icon
  if( ! pProc->szPath[0] )
    hIcon = LoadIcon (NULL, IDI_WINLOGO) ;
  else if( SHGetFileInfo (pProc->szPath, 0, &sfi, sizeof(sfi), SHGFI_ICON|SHGFI_SMALLICON) )
    hIcon = sfi.hIcon ;  
  else if( pProc->szPath[0] )
    hIcon = LoadIcon (NULL, IDI_APPLICATION) ;
  iImage = ImageList_AddIcon (hImageList, hIcon) ;
  DestroyIcon (hIcon) ; 
    
  // set process name
  lvi.mask	= LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE ;
  lvi.iItem	= i ;
  lvi.iSubItem	= COL_NAME ; 
  lvi.pszText	= pProc->szName ;
  lvi.lParam	= pProc->nProcessAddress ;
  lvi.iImage	= iImage ;
  ListView_SetItem (hwndList, &lvi) ;

  // set PID
  wsprintf (szBuffer, TEXT("%u"), pProc->nProcessId) ;
  lvi.mask	= LVIF_TEXT ;
  lvi.iSubItem	= COL_PID ;
  lvi.pszText	= szBuffer ;
  ListView_SetItem (hwndList, &lvi) ;

  // set state
  lvi.mask	= LVIF_TEXT | LVIF_IMAGE ;
  lvi.iSubItem	= COL_STATE ;
  if( pProc->nState==PS_HOOK_DISABLED ) {
    lvi.pszText = (LPTSTR) STR_DEF (_HOOK_DISABLED, TEXT("Not hooked (disabled)")) ;
    lvi.iImage	= 1 ;
  }
  else if( pProc->nState==PS_HOOKED_SINCE_BIRTH ) {
    lvi.pszText = (LPTSTR) STR_DEF (_HOOKED_SINCE_BIRTH, TEXT("Hooked (since birth)")) ;
    lvi.iImage	= 2 ;
  }
  else if( pProc->nState==PS_HOOKED_WHILE_RUNNING ) {
    lvi.pszText = (LPTSTR) STR_DEF (_HOOKED_WHILE_RUNNING, TEXT("Hooked (while running)")) ;
    lvi.iImage	= 2 ;
  }
  else {
    lvi.pszText = (LPTSTR) STR_DEF (_UNKNOWN_STATE, TEXT("Unknown")) ;
    lvi.iImage	= 0 ;
  }
  ListView_SetItem (hwndList, &lvi) ;

  // set path
  lvi.mask	= LVIF_TEXT ;
  lvi.iSubItem	= COL_PATH ;
  lvi.pszText	= pProc->szPath ;
  ListView_SetItem (hwndList, &lvi) ;
  
  return ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

VOID _ProcListWnd_UpdateProcessId (HWND hwndList, PROCADDR nProcessAddress)
{
  PROCSTRUCT	*pProc ;
  TCHAR		szBuffer[16] ;
  LVFINDINFO	lvfi ;
  int		iCurrent ;
  LVITEM	lvi ;
  BOOL		bSuccess ;

  ProcList_Lock () ;
  pProc = ProcList_Get (nProcessAddress) ;		
  wsprintf (szBuffer, TEXT("%u"), pProc->nProcessId) ;
  ProcList_Unlock () ;

  memset (&lvfi, 0, sizeof(lvfi)) ;
  lvfi.flags	= LVFI_PARAM ;
  lvfi.lParam	= nProcessAddress ;
   
  iCurrent = ListView_FindItem (hwndList, -1, &lvfi) ;
  if( iCurrent==-1 ) {
    TRACE_ERROR (TEXT("Process 0x%08X not found in process window\n"), nProcessAddress) ;
    return ;
  }
  
  lvi.mask	= 0 ;
  lvi.iItem	= iCurrent ;
  lvi.iSubItem	= COL_PID ;      
  bSuccess = ListView_GetItem (hwndList, &lvi) ; 
  if( ! bSuccess ) {
    TRACE_ERROR (TEXT("ListView_GetItem failed\n")) ;
    return ;
  }
    
  lvi.mask = LVIF_TEXT ;
  lvi.pszText = szBuffer ;
  bSuccess = ListView_SetItem (hwndList, &lvi) ;
  if( ! bSuccess ) {
    TRACE_ERROR (TEXT("ListView_GetItem failed\n")) ;
    return ;
  }    
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

VOID _ProcListWnd_RemProcess (HWND hwndList, PROCADDR nProcessAddress)
{
  LVFINDINFO	lvfi ;
  int iCurrent = -1 ;

  memset (&lvfi, 0, sizeof(lvfi)) ;
  lvfi.flags	= LVFI_PARAM ;
  lvfi.lParam	= nProcessAddress ;

  while(1) 
    {      
      iCurrent = ListView_FindItem (hwndList, iCurrent, &lvfi) ;

      if( iCurrent==-1 ) break ;

      if( ! ListView_DeleteItem (hwndList, iCurrent) )
	{
	  TRACE_ERROR (TEXT("ListView_DeleteItem failed (error=%u)\n"),
		       GetLastError()) ;
	  break ;
	}
    }

  TRACE_INFO (TEXT("Finished\n")) ;
}

/******************************************************************/
/* Internal function :                                            */
/******************************************************************/

int CALLBACK _ProcListWnd_ItemCompare (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) 
{
  int		iResult ;
  PROCSTRUCT	*pProc1, *pProc2 ;

  ProcList_Lock () ;

  pProc1 = ProcList_Get (lParam1) ;
  pProc2 = ProcList_Get (lParam2) ;

  if( !pProc1 || !pProc2 ) {
    ProcList_Unlock () ;
    return 0 ;
  }
    
  switch( lParamSort & 0x7F )
    {
    case COL_NAME:
      iResult = _tcsicmp (pProc1->szName, pProc2->szName) ;
      break ;

    case COL_PID:            
      iResult = (int)pProc1->nProcessId - (int)pProc2->nProcessId ;
      break ;
      
    case COL_STATE:
      iResult = (int)pProc1->nState - (int)pProc2->nState ;
      break ;

    case COL_PATH:
      iResult = _tcsicmp (pProc1->szPath, pProc2->szPath) ;
      break ;

    default:
      iResult = 0 ;
    }

  ProcList_Unlock () ;

  if( lParamSort & 0x80 )
    iResult = -iResult ;

  return iResult ;
}
