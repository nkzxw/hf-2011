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

#define TRACE_LEVEL	2 /* warning */


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "FilterWnd.h"

// standard headers
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>

// project's headers
#include "Assert.h"
#include "FiltCond.h"
#include "Filter.h"
#include "FilterSet.h"
#include "FilterTools.h"
#include "Language.h"
#include "ProgPathDlg.h"
#include "ProjectInfo.h"
#include "Resources.h"
#include "RuleDlg.h"
#include "SpyServer.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

// column identifiers
#define COL_PROGRAM	0
#define COL_REASON	0
#define COL_PARAM1	1
#define COL_PARAM2	2
#define COL_REACTION	3
#define COL_VERBOSITY	4
#define COL_OPTIONS	5

// radio button height
#define CY_RADIOBTN		20

#define CX_TOOLBAR		40

// window class name
#define WC_FILTERWND TEXT("FilterWnd")

// window messages
#define WM_UPDATEPROGRAMLIST	(WM_UPDATEFILTERS+1)
#define WM_UPDATERULELIST	(WM_UPDATEFILTERS+2)


LPCTSTR	g_szAddProgram = TEXT("Add program") ;
LPCTSTR	g_szEditProgram = TEXT("Edit program") ;
LPCTSTR g_szRemoveProgram = TEXT("Remove program") ;
LPCTSTR g_szAddRule = TEXT("Add rule") ;
LPCTSTR g_szEditRule = TEXT("Edit rule") ;
LPCTSTR g_szRemoveRule = TEXT("Remove rule") ;
LPCTSTR g_szMoveUpRule = TEXT("Move up rule") ;
LPCTSTR g_szMoveDownRule = TEXT("Move down rule") ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

LRESULT CALLBACK _FilterWnd_WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) ;

DWORD WINAPI _FilterWnd_UpdateIconThread (VOID*) ;

VOID _FilterWnd_UpdateRuleItem (HWND hwndList, PFILTRULE pRule)  ;


/******************************************************************/
/* Exported function : RegisterClass                              */
/******************************************************************/

BOOL FilterWnd_RegisterClass (HINSTANCE hInstance) 
{
  WNDCLASS wndclass ;

  wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
  wndclass.lpfnWndProc   = _FilterWnd_WndProc ;
  wndclass.cbClsExtra    = 0 ;
  wndclass.cbWndExtra    = 0 ;
  wndclass.hInstance     = hInstance ;
  wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION) ;
  wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
  wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1) ;
  wndclass.lpszMenuName  = NULL ;
  wndclass.lpszClassName = WC_FILTERWND ;

  return 0!=RegisterClass (&wndclass) ;
}


/******************************************************************/
/* Exported function : CreateWindow                               */
/******************************************************************/

HWND FilterWnd_CreateWindow (HINSTANCE hInstance, HWND hwndParent)
{  
  return CreateWindow (WC_FILTERWND, NULL,
		       WS_CHILD,
		       CW_USEDEFAULT, CW_USEDEFAULT, 
		       CW_USEDEFAULT, CW_USEDEFAULT, 
		       hwndParent, NULL, hInstance, NULL) ;
}


/******************************************************************/
/* Internal function : EnumRulesCallback                          */
/******************************************************************/

VOID _FilterWnd_EnumRulesCallback (LPVOID pContext, PFILTRULE pRule) 
{
  LVITEM lvi = { 0 } ;
  HWND hwndList = (HWND)pContext ;
  //TCHAR szBuffer[1024] ;

  ASSERT (pRule!=NULL) ;

  lvi.mask = LVIF_PARAM ;
  lvi.iItem = ListView_GetItemCount (hwndList) ;
  lvi.iSubItem = 0 ;
  lvi.lParam = (LPARAM)pRule ;
  ListView_InsertItem (hwndList, &lvi) ;

  _FilterWnd_UpdateRuleItem (hwndList, pRule) ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

VOID _FilterWnd_UpdateRuleItem (HWND hwndList, PFILTRULE pRule) 
{
  int i ;

  LVFINDINFO lvfi = { 0 } ;
  LVITEM lvi = { 0 } ;
  TCHAR szBuffer[1024] ;

 
  lvfi.flags = LVFI_PARAM ;
  lvfi.lParam = (LPARAM)pRule ;
  
  i = ListView_FindItem (hwndList,-1,&lvfi) ;

  if( i < 0 ) return ; 
  
  FiltCond_GetReasonAsString (&pRule->condition, szBuffer, 64) ;
  lvi.mask = LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE ;
  lvi.iItem = i ;
  lvi.iSubItem = COL_REASON ;
  lvi.pszText = szBuffer ;
  lvi.iImage = pRule->condition.nReason ;
  lvi.lParam = (LPARAM)pRule ;
  ListView_SetItem (hwndList, &lvi) ;

  FiltCond_GetParamAsString (&pRule->condition, 0, szBuffer, 128) ;
  lvi.mask = LVIF_TEXT ;
  lvi.iSubItem = COL_PARAM1 ;
  lvi.pszText = szBuffer ;
  ListView_SetItem (hwndList, &lvi) ;

  FiltCond_GetParamAsString (&pRule->condition, 1, szBuffer, 128) ;
  lvi.mask = LVIF_TEXT ;
  lvi.iSubItem = COL_PARAM2 ;
  lvi.pszText = szBuffer ;
  ListView_SetItem (hwndList, &lvi) ;

  // reaction
  FiltRule_GetReactionString (pRule, szBuffer, 64) ;
  lvi.mask = LVIF_TEXT ;
  lvi.iSubItem = COL_REACTION ;
  lvi.pszText = szBuffer ;
  ListView_SetItem (hwndList, &lvi) ;
  
  // verbosity
  FiltRule_GetVerbosityString (pRule, szBuffer, 64) ;
  lvi.mask = LVIF_TEXT ;
  lvi.iSubItem = COL_VERBOSITY ;
  lvi.pszText = szBuffer ;
  ListView_SetItem (hwndList, &lvi) ;

  // options
  FiltRule_GetOptionsString (pRule, szBuffer, 64) ;
  lvi.mask = LVIF_TEXT ;
  lvi.iSubItem = COL_OPTIONS ;
  lvi.pszText = szBuffer ;
  ListView_SetItem (hwndList, &lvi) ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

VOID _FilterWnd_RemoveRuleItem (HWND hwndList, PFILTRULE pRule) 
{
  int i ;

  LVFINDINFO lvfi = { 0 } ;
 
  lvfi.flags = LVFI_PARAM ;
  lvfi.lParam = (LPARAM)pRule ;
  
  i = ListView_FindItem (hwndList,-1,&lvfi) ;

  if( i < 0 ) return ; 

  ListView_DeleteItem (hwndList, i) ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

VOID _FilterWnd_UpdateProgramItem (HWND hwndList, HFILTER hFilter)
{
  LVFINDINFO lvfi = { 0 } ;
  LVITEM lvi ;
  HANDLE hThread ;
  DWORD dwThreadId ;
  int i ;

  lvfi.flags = LVFI_PARAM ;
  lvfi.lParam = (LPARAM)hFilter ;
  
  i = ListView_FindItem (hwndList,-1,&lvfi) ;
  
  if( i < 0 ) return ; 

  ZeroMemory (&lvi, sizeof(lvi)) ;
  lvi.mask = LVIF_TEXT|LVIF_IMAGE ;
  lvi.iItem = i ;
  lvi.iImage = -1 ;
  lvi.pszText = (TCHAR*)Filter_GetProgram(hFilter) ;
  ListView_SetItem (hwndList, &lvi) ;
  
  hThread = CreateThread (NULL, 0, _FilterWnd_UpdateIconThread,
			  hwndList, 0, &dwThreadId) ;
  
  CloseHandle (hThread) ;
}

/******************************************************************/
/* Internal function                                              */
/******************************************************************/

VOID _FilterWnd_RemoveProgramItem (HWND hwndList, HFILTER hFilter)
{
  LVFINDINFO lvfi = { 0 } ;
  int i ;

  lvfi.flags = LVFI_PARAM ;
  lvfi.lParam = (LPARAM)hFilter ;
  
  i = ListView_FindItem (hwndList,-1,&lvfi) ;
  
  if( i < 0 ) return ; 

  ListView_DeleteItem (hwndList, i) ;
}


/******************************************************************/
/* Internal function : WndProc                                    */
/******************************************************************/

LRESULT CALLBACK _FilterWnd_WndProc (HWND hwnd, UINT message, 
				    WPARAM wParam, LPARAM lParam)
{  
  static HWND		g_hwndPrograms ;
  static HWND		g_hwndRules ;
  static HWND		g_hwndEnableHook ;
  static HWND		g_hwndDisableHook ;
  static HWND		g_hwndProgTools ;
  static HWND		g_hwndRulesTools ;
  static HINSTANCE	g_hInstance ;
  static HMENU		g_hmenuPrograms ;
  static HMENU		g_hmenuRules ;
  static HFILTER	g_hfltCurrent ;
  static FILTRULE	*g_pCurRule ;

  TCHAR			szBuffer[1024] ;
  int			nWidth, nHeight ;
  POINT			point ;
  HFILTERSET		hFilterSet ;
  //HFILTER		hFilter ;
  LVCOLUMN		lvc ;
  HIMAGELIST		hImageList ;
  TBBUTTON		tbb ;
  
  union {    
    NMHDR		*header ;
    NMITEMACTIVATE	*itemactivate ;
    NMLISTVIEW		*listview ; 
    NMTTDISPINFO	*getdispinfo ; 
    NMLVKEYDOWN		*keydown ;
  } pnm ;

  switch (message)
    {
    case WM_CREATE:
      
      g_hfltCurrent = NULL ;
      g_pCurRule = NULL ;
	
      g_hInstance = ((CREATESTRUCT*)lParam)->hInstance ;
            
      g_hwndPrograms = CreateWindowEx (WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL,
				       WS_CHILD|WS_VISIBLE|WS_VSCROLL|LVS_REPORT|LVS_SINGLESEL|
				       LVS_SHOWSELALWAYS|LVS_NOSORTHEADER,
				       0,0,0,0, hwnd, (HMENU)IDC_PROGRAMLIST, g_hInstance, NULL) ;
      ListView_SetExtendedListViewStyle (g_hwndPrograms, LVS_EX_FULLROWSELECT) ;

      g_hwndProgTools = CreateWindow (TOOLBARCLASSNAME, NULL, 
				      WS_CHILD|WS_VISIBLE|CCS_NORESIZE|CCS_NOPARENTALIGN|
				      TBSTYLE_WRAPABLE|TBSTYLE_TOOLTIPS|TBSTYLE_FLAT,
				      0,0,0,0, hwnd, NULL, g_hInstance, NULL) ;
      SendMessage (g_hwndProgTools, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0); 
	
      g_hwndRules = CreateWindowEx (WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL,
				    WS_CHILD|WS_VISIBLE|WS_VSCROLL|LVS_REPORT|
				    LVS_SINGLESEL|LVS_SHOWSELALWAYS|LVS_NOSORTHEADER,
				    0,0,0,0, hwnd, (HMENU)IDC_RULELIST, g_hInstance, NULL) ;
      ListView_SetExtendedListViewStyle (g_hwndRules, LVS_EX_FULLROWSELECT) ;

      g_hwndRulesTools = CreateWindow (TOOLBARCLASSNAME, NULL, 
				       WS_CHILD|WS_VISIBLE|CCS_NORESIZE|CCS_NOPARENTALIGN|
				       TBSTYLE_WRAPABLE|TBSTYLE_TOOLTIPS|TBSTYLE_FLAT,
				       0,0,0,0, hwnd, NULL, g_hInstance, NULL) ;
      SendMessage (g_hwndRulesTools, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0); 

      g_hwndEnableHook = CreateWindow (WC_BUTTON, NULL, WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
				       0,0,0,0, hwnd, (HMENU)IDC_ENABLE_HOOK, g_hInstance, NULL) ;

      SendMessage (g_hwndEnableHook, WM_SETFONT, 
		   (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0) ;
      
      g_hwndDisableHook = CreateWindow (WC_BUTTON, NULL, WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
					0,0,0,0, hwnd, (HMENU)IDC_DISABLE_HOOK, g_hInstance, NULL) ;
      
      SendMessage (g_hwndDisableHook, WM_SETFONT, 
		   (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0) ;

      //
      // Init image list for filter list
      //
      hImageList = ImageList_Create (16,16,ILC_COLOR32,16,8) ;
      ListView_SetImageList (g_hwndPrograms, hImageList, LVSIL_SMALL) ;
      
      //
      // Init image list for rules window
      //
      hImageList = ImageList_Create (16,16,ILC_COLOR32|ILC_MASK,6,4) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_UNKNOWN))) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_REASON_FILE_READ))) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_REASON_FILE_WRITE))) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_REASON_NET_CONNECT))) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_REASON_NET_LISTEN))) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_REASON_NET_CONNECT))) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_REASON_REG_SETVALUE))) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_REASON_REG_QUERYVALUE))) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_REASON_SYS_EXECUTE))) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_REASON_SYS_KILLPROCESS))) ;
      ListView_SetImageList (g_hwndRules, hImageList, LVSIL_SMALL) ;

      // menus will be created by WM_LANGUAGECHANGED
      g_hmenuRules = NULL ;
      g_hmenuPrograms = NULL ;

      //
      // create columns in list view controls
      //

      ZeroMemory (&lvc, sizeof(lvc)) ;
      lvc.mask		= LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM ;
      lvc.pszText	= TEXT("") ;

      lvc.cx		= 800 ;
      lvc.iSubItem	= COL_PROGRAM ;
      ListView_InsertColumn (g_hwndPrograms, lvc.iSubItem, &lvc) ; 
      
      lvc.cx		= 80 ;
      lvc.iSubItem	= COL_REASON   ;
      ListView_InsertColumn (g_hwndRules, lvc.iSubItem, &lvc) ; 
      
      lvc.cx		= 200 ;
      lvc.iSubItem	= COL_PARAM1 ;
      ListView_InsertColumn (g_hwndRules, lvc.iSubItem, &lvc) ; 

      lvc.cx		= 100 ;
      lvc.iSubItem	= COL_PARAM2 ;
      ListView_InsertColumn (g_hwndRules, lvc.iSubItem, &lvc) ; 
      
      lvc.cx		= 180 ;
      lvc.iSubItem	= COL_REACTION ;
      ListView_InsertColumn (g_hwndRules, lvc.iSubItem, &lvc) ; 

      lvc.cx		= 150 ;
      lvc.iSubItem	= COL_VERBOSITY ;
      ListView_InsertColumn (g_hwndRules, lvc.iSubItem, &lvc) ; 

      lvc.cx		= 100 ;
      lvc.iSubItem	= COL_OPTIONS ;      
      ListView_InsertColumn (g_hwndRules, lvc.iSubItem, &lvc) ; 
        
      g_hfltCurrent = FilterSet_GetDefaultFilter (SpySrv_GetFilterSet()) ;
      g_pCurRule = NULL ;

      //
      // Create imagelist for tool bars
      //

      hImageList = ImageList_Create (32,32,ILC_COLOR32|ILC_MASK,6,4) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_TOOL_ADD))) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_TOOL_EDIT))) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_TOOL_REMOVE))) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_TOOL_UP))) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_TOOL_DOWN))) ;

      //
      // Add buttons to program tool-bar
      //

      // set image list
      SendMessage (g_hwndProgTools, TB_SETIMAGELIST, 0, (LPARAM)hImageList) ;
      
      tbb.fsState	= TBSTATE_ENABLED ;
      tbb.fsStyle	= TBSTYLE_BUTTON ;
      tbb.iString	= 0 ;

      tbb.iBitmap	= 0 ;
      tbb.idCommand	= IDM_PROGRAM_ADD ;
      SendMessage (g_hwndProgTools, TB_ADDBUTTONS, 1, (LPARAM)&tbb) ;  

      tbb.iBitmap	= 1 ;
      tbb.idCommand	= IDM_PROGRAM_EDIT ;
      SendMessage (g_hwndProgTools, TB_ADDBUTTONS, 1, (LPARAM)&tbb) ;          

      tbb.iBitmap	= 2 ;
      tbb.idCommand	= IDM_PROGRAM_REMOVE ;
      SendMessage (g_hwndProgTools, TB_ADDBUTTONS, 1, (LPARAM)&tbb) ;   

      //
      // Add buttons to program tool-bar
      //

      // set image list
      SendMessage (g_hwndRulesTools, TB_SETIMAGELIST, 0, (LPARAM)hImageList) ;
      
      tbb.fsState	= TBSTATE_ENABLED ;
      tbb.fsStyle	= TBSTYLE_BUTTON ;
      tbb.iString	= 0 ;

      tbb.iBitmap	= 0 ;
      tbb.idCommand	= IDM_RULE_ADD ;
      SendMessage (g_hwndRulesTools, TB_ADDBUTTONS, 1, (LPARAM)&tbb) ;  

      tbb.iBitmap	= 1 ;
      tbb.idCommand	= IDM_RULE_EDIT ;
      SendMessage (g_hwndRulesTools, TB_ADDBUTTONS, 1, (LPARAM)&tbb) ;          

      tbb.iBitmap	= 2 ;
      tbb.idCommand	= IDM_RULE_REMOVE ;
      SendMessage (g_hwndRulesTools, TB_ADDBUTTONS, 1, (LPARAM)&tbb) ;     

      tbb.iBitmap	= 3 ;
      tbb.idCommand	= IDM_RULE_UP ;
      SendMessage (g_hwndRulesTools, TB_ADDBUTTONS, 1, (LPARAM)&tbb) ;    

      tbb.iBitmap	= 4 ;
      tbb.idCommand	= IDM_RULE_DOWN ;
      SendMessage (g_hwndRulesTools, TB_ADDBUTTONS, 1, (LPARAM)&tbb) ;  


      SendMessage (hwnd, WM_LANGUAGECHANGED, 0, 0) ;

      return 0 ; // case WM_CREATE:

    case WM_DESTROY: 

      // destroy toolbar's image list
      hImageList = (HIMAGELIST) SendMessage (g_hwndProgTools, TB_GETIMAGELIST, 0, 0) ;
      ImageList_Destroy (hImageList) ;

      // destroy menus
      DestroyMenu (g_hmenuPrograms) ;
      DestroyMenu (g_hmenuRules) ;

      return 0 ; // case WM_DESTROY:   
      
    case WM_SIZE:
      nWidth = LOWORD (lParam) ;
      nHeight = HIWORD (lParam) ;
      MoveWindow (g_hwndProgTools, nWidth-CX_TOOLBAR, 0, CX_TOOLBAR, nHeight/3-CY_RADIOBTN, TRUE) ;
      MoveWindow (g_hwndPrograms, 0, 0, nWidth-CX_TOOLBAR, nHeight/3-CY_RADIOBTN, TRUE) ;
      MoveWindow (g_hwndEnableHook, 0, nHeight/3-CY_RADIOBTN, nWidth, CY_RADIOBTN, TRUE) ;
      MoveWindow (g_hwndDisableHook, 0, nHeight/3, nWidth, CY_RADIOBTN, TRUE) ;
      MoveWindow (g_hwndRulesTools, nWidth-CX_TOOLBAR, nHeight/3+CY_RADIOBTN, CX_TOOLBAR, 2*nHeight/3-CY_RADIOBTN, TRUE) ;
      MoveWindow (g_hwndRules, 0, nHeight/3+CY_RADIOBTN, nWidth-CX_TOOLBAR, 2*nHeight/3-CY_RADIOBTN, TRUE) ;
      return 0 ;

    case WM_LANGUAGECHANGED:
        
      // 
      // Set text of columns
      //
      ZeroMemory (&lvc, sizeof(lvc)) ;
      lvc.mask		= LVCF_TEXT ;      

      lvc.pszText	= (LPTSTR) STR_DEF(_PROGRAM,TEXT("Program")) ;
      ListView_SetColumn (g_hwndPrograms, COL_PROGRAM, &lvc) ; 

      lvc.pszText	= (LPTSTR) STR_DEF (_REASON,TEXT("Function")) ;
      ListView_SetColumn (g_hwndRules, COL_REASON, &lvc) ; 

      lvc.pszText	= szBuffer ;
      wsprintf (szBuffer, TEXT("%s 1"), STR_DEF(_PARAM,TEXT("Param"))) ; ;
      ListView_SetColumn (g_hwndRules, COL_PARAM1, &lvc) ;     

      lvc.pszText	= szBuffer ;
      wsprintf (szBuffer, TEXT("%s 2"), STR_DEF(_PARAM,TEXT("Param"))) ; ;
      ListView_SetColumn (g_hwndRules, COL_PARAM2, &lvc) ;  

      lvc.pszText	= (LPTSTR) STR_DEF (_REACTION, TEXT("Reaction")) ;
      ListView_SetColumn (g_hwndRules, COL_REACTION, &lvc) ;

      lvc.pszText	= (LPTSTR) STR_DEF (_VERBOSITY,TEXT("Verbosity")) ;
      ListView_SetColumn (g_hwndRules, COL_VERBOSITY, &lvc) ;
      
      lvc.pszText	= (LPTSTR) STR_DEF (_OPTIONS,TEXT("Options")) ;
      ListView_SetColumn (g_hwndRules, COL_OPTIONS, &lvc) ;      

      // create process menu
      DestroyMenu (g_hmenuPrograms) ;
      g_hmenuPrograms = CreatePopupMenu () ;
      AppendMenu (g_hmenuPrograms, MF_STRING|MF_GRAYED, 0, 
		  STR_DEF(_PROGRAM_MENU,TEXT("Program menu"))) ;
      AppendMenu (g_hmenuPrograms, MF_SEPARATOR, 0, NULL) ;
      AppendMenu (g_hmenuPrograms, MF_STRING, IDM_PROGRAM_ADD, 
		  STR_DEF(_ADD_PROGRAM,g_szAddProgram)) ;
      AppendMenu (g_hmenuPrograms, MF_STRING, IDM_PROGRAM_EDIT, 
		  STR_DEF(_EDIT_PROGRAM,g_szEditProgram)) ;
      AppendMenu (g_hmenuPrograms, MF_STRING, IDM_PROGRAM_REMOVE,
		  STR_DEF(_REMOVE_PROGRAM,g_szRemoveProgram)) ;
    
      // create rule menu
      DestroyMenu (g_hmenuRules) ;
      g_hmenuRules = CreatePopupMenu () ;
      AppendMenu (g_hmenuRules, MF_STRING|MF_GRAYED, 0, 
		  STR_DEF(_FILTER_MENU, TEXT ("Filter menu"))) ;
      AppendMenu (g_hmenuRules, MF_SEPARATOR, 0, NULL) ;
      AppendMenu (g_hmenuRules, MF_STRING, IDM_RULE_ADD, 
		  STR_DEF(_ADD_RULE,g_szAddRule)) ;
      AppendMenu (g_hmenuRules, MF_STRING, IDM_RULE_EDIT, 
		  STR_DEF(_EDIT_RULE, g_szEditRule)) ;
      AppendMenu (g_hmenuRules, MF_STRING, IDM_RULE_REMOVE, 
		  STR_DEF(_REMOVE_RULE, g_szRemoveProgram)) ;
      AppendMenu (g_hmenuRules, MF_SEPARATOR, 0, NULL) ;
      AppendMenu (g_hmenuRules, MF_STRING, IDM_RULE_UP, 
		  STR_DEF(_MOVE_UP_RULE, g_szMoveUpRule)) ;
      AppendMenu (g_hmenuRules, MF_STRING, IDM_RULE_DOWN, 
		  STR_DEF(_MOVE_DOWN_RULE, g_szMoveDownRule)) ;

      // check boxes
      SetWindowText (g_hwndEnableHook, STR_DEF(_HOOK_THIS_PROGRAM,
					       TEXT("Hook this program"))) ;
      SetWindowText (g_hwndDisableHook, STR_DEF(_DONT_HOOK_THIS_PROGRAM,
						TEXT("Don't hook this program"))) ;
      
      PostMessage (hwnd, WM_UPDATEFILTERS, 0, 0) ;

      return 0 ;

    case WM_UPDATEFILTERS :
      
    case WM_UPDATEPROGRAMLIST:
      {
	UINT		i ;
	HFILTERSET	hFilterSet ;
	LV_ITEM		lvi ;	
	DWORD		dwThreadId ;
	HANDLE		hThread ;
	
	ListView_DeleteAllItems (g_hwndPrograms) ;

	hFilterSet = SpySrv_GetFilterSet () ;
	
	for( i=0 ; i<FilterSet_GetFilterCount(hFilterSet) ; i++ )
	  {
	    HFILTER hFilter = FilterSet_GetFilterByNum(hFilterSet,i) ;
	    ZeroMemory (&lvi, sizeof(lvi)) ;
	    lvi.mask = LVIF_TEXT|LVIF_STATE|LVIF_IMAGE|LVIF_PARAM ;
	    lvi.lParam = (LPARAM)hFilter ;
	    lvi.iItem = i ;
	    lvi.iImage = -1 ;
	    lvi.state = hFilter==g_hfltCurrent ? LVIS_SELECTED|LVIS_FOCUSED : 0 ;
	    lvi.stateMask = LVIS_SELECTED|LVIS_FOCUSED ;
	    lvi.pszText = (TCHAR*)Filter_GetProgram(hFilter) ;
	    ListView_InsertItem (g_hwndPrograms, &lvi) ;

	    if( hFilter==g_hfltCurrent )
	      ListView_EnsureVisible (g_hwndPrograms, i, FALSE) ;
	  }

	hThread = CreateThread (NULL, 0, _FilterWnd_UpdateIconThread,
					   g_hwndPrograms, 0, &dwThreadId) ;
	
	CloseHandle (hThread) ;

	SendMessage (hwnd, WM_UPDATERULELIST, 0, 0) ;
      }
      return 0 ;      

    case WM_UPDATERULELIST: // ======== Update the "rule" list ========
      {
	LVFINDINFO	lvfi ;
	LVITEM		lvi ;
	int		iItem ;
      
	ListView_DeleteAllItems (g_hwndRules) ;
       	
	if( ! g_hfltCurrent ) 
	  {
	    EnableWindow (g_hwndEnableHook, FALSE) ;
	    EnableWindow (g_hwndDisableHook, FALSE) ;
	    EnableWindow (g_hwndRules, FALSE) ;

	    // disable rule buttons
	    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_ADD, MAKELONG(FALSE,0)) ;
	    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_EDIT, MAKELONG(FALSE,0)) ;
	    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_REMOVE, MAKELONG(FALSE,0)) ;
	    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_UP, MAKELONG(FALSE,0)) ;
	    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_DOWN, MAKELONG(FALSE,0)) ;
	  }
	else if( Filter_IsHookEnabled(g_hfltCurrent) ) 
	  {
	    EnableWindow (g_hwndEnableHook, FALSE) ;
	    EnableWindow (g_hwndDisableHook, FALSE) ;
	    EnableWindow (g_hwndRules, TRUE) ;
	    
	    // enable rule buttons
	    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_ADD, MAKELONG(TRUE,0)) ;

	    if( g_pCurRule ) {
	      SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_EDIT, MAKELONG(TRUE,0)) ;
	      SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_REMOVE, MAKELONG(TRUE,0)) ;
	      SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_UP, 
			   MAKELONG(Filter_CanMoveUpRule(g_hfltCurrent,g_pCurRule),0)) ;
	      SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_DOWN, 
			   MAKELONG(Filter_CanMoveDownRule(g_hfltCurrent,g_pCurRule),0)) ;
	    }
	    else {
	    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_EDIT, MAKELONG(FALSE,0)) ;
	    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_REMOVE, MAKELONG(FALSE,0)) ;
	    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_UP, MAKELONG(FALSE,0)) ;
	    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_DOWN, MAKELONG(FALSE,0)) ;	      
	    }
	    
	    SendMessage (g_hwndEnableHook, BM_SETCHECK, BST_CHECKED, 0) ;
	    SendMessage (g_hwndDisableHook, BM_SETCHECK, BST_UNCHECKED, 0) ;
	    
	    Filter_EnumRules (g_hfltCurrent, _FilterWnd_EnumRulesCallback, g_hwndRules) ;
	  }
	else
	  {
	    EnableWindow (g_hwndEnableHook, FALSE) ;
	    EnableWindow (g_hwndDisableHook, FALSE) ;
	    EnableWindow (g_hwndRules, FALSE) ;
	    
	    // disable rule buttons
	    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_ADD, MAKELONG(FALSE,0)) ;
	    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_EDIT, MAKELONG(FALSE,0)) ;
	    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_REMOVE, MAKELONG(FALSE,0)) ;
	    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_UP, MAKELONG(FALSE,0)) ;
	    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_DOWN, MAKELONG(FALSE,0)) ;
	    
	    SendMessage (g_hwndEnableHook, BM_SETCHECK, BST_UNCHECKED, 0) ;
	    SendMessage (g_hwndDisableHook, BM_SETCHECK, BST_CHECKED, 0) ;
	  }

	// search selected item
	lvfi.flags = LVFI_PARAM ;
	lvfi.lParam = (LPARAM) g_pCurRule ;
	iItem = ListView_FindItem (g_hwndRules, -1, &lvfi) ;
	
	// set selection
	ZeroMemory (&lvi, sizeof(lvi)) ;  // <- useful ?
	lvi.mask	= LVIF_STATE ;
	lvi.iItem	= iItem ;
	lvi.iSubItem	= 0 ;
	lvi.stateMask	= LVIS_SELECTED ;
	lvi.state	= LVIS_SELECTED ;
	ListView_SetItem (g_hwndRules, &lvi) ;

	ListView_EnsureVisible (g_hwndRules, iItem, FALSE) ;
      }
      return 0 ; // ======== Update the "rule" list ========

    case WM_COMMAND:

      switch( LOWORD(wParam))
	{
	case IDM_PROGRAM_ADD:

	  szBuffer[0] = 0 ;

	  if( IDOK==DialogBoxParam(g_hInstance, MAKEINTRESOURCE(DLG_PROGPATH), 
				   hwnd, ProgPathDlg_DlgProc, (LPARAM)szBuffer) )
	    {
	      hFilterSet = SpySrv_GetFilterSet () ;
	      g_hfltCurrent = Filter_Create (szBuffer) ;
	      SpySrv_LockFilterSet () ;
	      FilterSet_AddFilter (hFilterSet, g_hfltCurrent) ;
	      SpySrv_SendFilterSetToDriver () ;
	      SpySrv_UnlockFilterSet () ;
	      SendMessage (hwnd, WM_UPDATEPROGRAMLIST, 0, 0) ;
	      SendMessage (hwnd, WM_UPDATERULELIST, 0, 0) ;
	    }

	  return 0 ; // case IDM_PROGRAM_ADD:

	case IDM_PROGRAM_EDIT: //  ======== "Edit" command on program list ========
	  {
	    // is command allowed ?
	    if( !g_hfltCurrent || Filter_GetProtected(g_hfltCurrent) ) 
	      return 0 ;
	    
	    szBuffer[0] = 0 ;	    
	    _tcscpy (szBuffer, Filter_GetProgram (g_hfltCurrent)) ;
	    
	    if( IDOK==DialogBoxParam(g_hInstance, MAKEINTRESOURCE(DLG_PROGPATH), 
				     hwnd, ProgPathDlg_DlgProc, (LPARAM)szBuffer) )
	      {
		SpySrv_LockFilterSet () ;
		Filter_SetProgram (g_hfltCurrent, szBuffer) ;
		SpySrv_SendFilterSetToDriver () ;
		SpySrv_UnlockFilterSet () ;
		_FilterWnd_UpdateProgramItem (g_hwndPrograms, g_hfltCurrent) ;
	      }
	  }	    
	  return 0 ;  //  ======== "Edit" command on program list ========


	case IDM_PROGRAM_REMOVE: //  ======== "Remove" command on program list ========
	  {
	    // is command allowed ?
	    if( ! g_hfltCurrent || Filter_GetProtected(g_hfltCurrent) ) 
	      return 0 ;
	    
	    wsprintf (szBuffer,
		      STR_DEF(_SURE_REMOVE_FILTER_FOR_S,
			      TEXT("Are you sure you want to remove filter for \"%s\" ?")),
		      Filter_GetProgram(g_hfltCurrent)) ;
	    
	    if( IDYES==MessageBox(hwnd,szBuffer,TEXT(APPLICATION_NAME),
				  MB_ICONQUESTION|MB_YESNO) )
	      {
		_FilterWnd_RemoveProgramItem (g_hwndPrograms, g_hfltCurrent) ;

		hFilterSet = SpySrv_GetFilterSet () ;
		
		SpySrv_LockFilterSet () ;
		FilterSet_Remove (hFilterSet, g_hfltCurrent) ;
		SpySrv_SendFilterSetToDriver () ;
		SpySrv_UnlockFilterSet () ;
		
		Filter_Destroy (g_hfltCurrent) ;
		g_hfltCurrent = NULL ;
		
		SendMessage (hwnd, WM_UPDATERULELIST, 0, 0) ;
	      }
	  }
	  return 0 ; //  ======== "Remove" command on program list ========



	case IDM_RULE_ADD: //  ======== "Add" command on rule list ========
	  {
	    FILTRULE * pRule ;
	    
	    // is the filter selected ?
	    if( ! g_hfltCurrent )  return 0 ;
	    
	    // alloc new rule
	    pRule = (FILTRULE*) calloc (1, sizeof(FILTRULE)) ;
	    
	    // change rule
	    if( IDOK==RuleDlg_DialogBox (g_hInstance, hwnd, Filter_GetProgram(g_hfltCurrent), pRule, TRUE) )
	      {
		SpySrv_LockFilterSet () ;
		Filter_AddRule (g_hfltCurrent, pRule) ;
		SpySrv_SendFilterSetToDriver () ;
		SpySrv_UnlockFilterSet () ;

		g_pCurRule = pRule ;

		SendMessage (hwnd, WM_UPDATERULELIST, 0, 0) ;
	      }
	    else free (pRule) ;	      
	  }	  
	  return 0 ;  //  ======== "Add" command on rule list ========

	case IDM_RULE_EDIT: // ======== 'Edit rule' command on 'rules' list-view ========
	  {
	    if( !g_hfltCurrent || !g_pCurRule ) return 0 ;
	    
	    if( IDOK==RuleDlg_DialogBox (g_hInstance, hwnd, Filter_GetProgram(g_hfltCurrent), g_pCurRule, FALSE) )
	      {
		SpySrv_LockFilterSet () ;		
		SpySrv_SendFilterSetToDriver () ;
		SpySrv_UnlockFilterSet () ;
		_FilterWnd_UpdateRuleItem (g_hwndRules, g_pCurRule) ;
	      }
	  }	  
	  return 0 ; // ======== 'Edit rule' command on 'rules' list-view ========
      
	case IDM_RULE_REMOVE: // ======== 'Remove rule' command on 'rules' list-view ========
	  {
	    LPCTSTR szMessage  = STR_DEF(_SURE_REMOVE_RULE,
					 TEXT("Are you sure you want to remove this rule ?")) ;
	    
	    if( !g_hfltCurrent || !g_pCurRule ) return 0 ;
	    
	    if( IDYES==MessageBox(hwnd,szMessage,TEXT(APPLICATION_NAME),
				  MB_ICONQUESTION|MB_YESNO) )
	      {		
		_FilterWnd_RemoveRuleItem (g_hwndRules, g_pCurRule) ;
		SpySrv_LockFilterSet () ;
		Filter_DeleteRule (g_hfltCurrent, g_pCurRule) ;
		SpySrv_SendFilterSetToDriver () ;
		SpySrv_UnlockFilterSet () ;
		g_pCurRule = NULL ;
	      }		 	
	  } 	  
	  return 0 ; // ======== 'Remove rule' command on 'rules' list-view ========

	case IDM_RULE_UP: // ======== 'Move rule up' command on 'rules' list-view ========
	  {
	    if( g_hfltCurrent && g_pCurRule )
	      {
		SpySrv_LockFilterSet () ;
		Filter_MoveRuleUp (g_hfltCurrent, g_pCurRule) ;
		SpySrv_SendFilterSetToDriver () ;
		SpySrv_UnlockFilterSet () ;
		SendMessage (hwnd, WM_UPDATERULELIST, 0, 0) ;		 
	      }
	  }	  
	  return 0 ; // ======== 'Move rule up' command on 'rules' list-view ========

	case IDM_RULE_DOWN: // ======== 'Move rule down' command on 'rules' list-view ========
	  {
	    if( g_hfltCurrent && g_pCurRule )
	      {
		SpySrv_LockFilterSet () ;
		Filter_MoveRuleDown (g_hfltCurrent, g_pCurRule) ;
		SpySrv_SendFilterSetToDriver () ;
		SpySrv_UnlockFilterSet () ;
		SendMessage (hwnd, WM_UPDATERULELIST, 0, 0) ;		 
	      }
	  }
	  return 0 ; // ======== 'Move rule down' command on 'rules' list-view ========
/*
	case IDC_ENABLE_HOOK:
	  
	  if( ! g_hfltCurrent ) return 0 ;

	  if( HIWORD(wParam)==BN_CLICKED )
	    {
	      Filter_EnableHook (g_hfltCurrent, TRUE) ;
	      SendMessage (hwnd, WM_UPDATERULELIST, 0, 0) ;
	    }

	  return 0 ; // case IDC_ENABLE_HOOK:

	case IDC_DISABLE_HOOK:

	  if( ! g_hfltCurrent ) return 0 ;

	  if( HIWORD(wParam)==BN_CLICKED ) 
	    {
	      Filter_EnableHook (g_hfltCurrent, FALSE) ;
	      SendMessage (hwnd, WM_UPDATERULELIST, 0, 0) ;
	    }

	  return 0 ; // case IDC_DISABLE_HOOK:
*/
	} // switch( LOWORD(wParam) )
      
      break ; // case WM_COMMAND:
  
    case WM_NOTIFY:
      
      pnm.header = (NMHDR*)lParam ;
      
      switch( pnm.header->idFrom )
	{
	case IDC_PROGRAMLIST:
	  
	  switch( pnm.header->code )
	    {
	    case LVN_ITEMCHANGED:
	      
	      if( pnm.listview->uNewState & LVIS_SELECTED )
		{
		  hFilterSet = SpySrv_GetFilterSet () ;

		  g_hfltCurrent = FilterSet_GetFilterByNum (hFilterSet, pnm.listview->iItem) ;
		  g_pCurRule = NULL ;

		  if( pnm.listview->iItem>0 && ListView_GetSelectedCount(g_hwndPrograms)>0 ) {
		    SendMessage (g_hwndProgTools, TB_ENABLEBUTTON, IDM_PROGRAM_EDIT, MAKELONG(TRUE,0)) ;
		    SendMessage (g_hwndProgTools, TB_ENABLEBUTTON, IDM_PROGRAM_REMOVE, MAKELONG(TRUE,0)) ;
		  } 
		  else {
		    SendMessage (g_hwndProgTools, TB_ENABLEBUTTON, IDM_PROGRAM_EDIT, MAKELONG(FALSE,0)) ;
 		    SendMessage (g_hwndProgTools, TB_ENABLEBUTTON, IDM_PROGRAM_REMOVE, MAKELONG(FALSE,0)) ; 
		  } 		  
		  
		  SendMessage (hwnd, WM_UPDATERULELIST, 0, 0) ;
		}	
	      
	      return 0 ; // case LVN_ITEMCHANGED:

	    case LVN_ITEMACTIVATE:
	      
	      // simulate a click on the menu
	      if( pnm.listview->iItem>0 )
		SendMessage (hwnd, WM_COMMAND, IDM_PROGRAM_EDIT, 0) ;
	      
	      return 0 ; // case NM_DBLCLICK:
	      
	    case NM_RCLICK: //  ======== Right click on "programs" list-view ======== 
	      {
		BOOL		bEnable ;
		POINT		pt ;
		
		// enable/disable "edit" and "remove" menu item
		bEnable = g_hfltCurrent!=NULL && !Filter_GetProtected(g_hfltCurrent) ;
		EnableMenuItem (g_hmenuPrograms, IDM_PROGRAM_EDIT, MF_BYCOMMAND|(bEnable?MF_ENABLED:MF_GRAYED)) ;
		EnableMenuItem (g_hmenuPrograms, IDM_PROGRAM_REMOVE, MF_BYCOMMAND|(bEnable?MF_ENABLED:MF_GRAYED)) ;
		
		// get mouse position
		GetCursorPos (&pt) ;

		// display menu
		TrackPopupMenu (g_hmenuPrograms, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL) ;		
	      }
	      return 0 ; //  ======== Right click on "programs" list-view ======== 

	    case LVN_KEYDOWN:

	      switch( pnm.keydown->wVKey )
		{
		case VK_DELETE:
		  PostMessage (hwnd, WM_COMMAND, IDM_PROGRAM_REMOVE, 0) ;
		  break ;
		}

	      return 0 ;
	      
	    } //  switch( pnmh->code )
	  
	  break ; // case IDC_PROGRAMLIST:
	  
	case IDC_RULELIST:
	  
	  switch( pnm.header->code )
	    {
	    case LVN_ITEMACTIVATE:

	      // simulate a click on the menu
	      SendMessage (hwnd, WM_COMMAND, IDM_RULE_EDIT, 0) ;

	      return 0 ; // case LVN_ITEMACTIVATE:

	    case LVN_ITEMCHANGED:

	      if( pnm.listview->uNewState & LVIS_SELECTED )
		{
		  ASSERT (g_hfltCurrent) ;
		  ASSERT (pnm.listview->lParam!=0) ;

		  g_pCurRule = (FILTRULE*)pnm.listview->lParam ;

		  if( Filter_CanMoveUpRule(g_hfltCurrent,g_pCurRule) )
		    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_UP, MAKELONG(TRUE,0)) ;
		  else
		    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_UP, MAKELONG(FALSE,0)) ;
		  if( Filter_CanMoveDownRule(g_hfltCurrent,g_pCurRule) )
		    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_DOWN, MAKELONG(TRUE,0)) ;
		  else
		    SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_DOWN, MAKELONG(FALSE,0)) ;
		}
	      
	      if( ListView_GetSelectedCount(g_hwndRules)>0 ) {
		SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_EDIT, MAKELONG(TRUE,0)) ;
		SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_REMOVE, MAKELONG(TRUE,0)) ;
	      } 
	      else {
		SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_EDIT, MAKELONG(FALSE,0)) ;
		SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_REMOVE, MAKELONG(FALSE,0)) ;
		SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_UP, MAKELONG(FALSE,0)) ;
		SendMessage (g_hwndRulesTools, TB_ENABLEBUTTON, IDM_RULE_DOWN, MAKELONG(FALSE,0)) ;
	      } 		  
	      
	      return 0 ; // case LVN_ITEMCHANGED:


	    case NM_RCLICK:
	      
	      if( ! g_hfltCurrent ) return 0 ;
	      
	      GetCursorPos (&point) ;
	      
	      if( ListView_GetSelectedCount(g_hwndRules)>0 ) {
		EnableMenuItem (g_hmenuRules, IDM_RULE_EDIT, MF_BYCOMMAND|MF_ENABLED) ;
		EnableMenuItem (g_hmenuRules, IDM_RULE_REMOVE, MF_BYCOMMAND|MF_ENABLED) ;		  
		} 
	      else {
		EnableMenuItem (g_hmenuRules, IDM_RULE_EDIT, MF_BYCOMMAND|MF_GRAYED) ;
		EnableMenuItem (g_hmenuRules, IDM_RULE_REMOVE, MF_BYCOMMAND|MF_GRAYED) ;		  
	      } 
	      
	      TrackPopupMenu (g_hmenuRules, TPM_RIGHTBUTTON, 
			      point.x, point.y, 0, hwnd, NULL) ;
	      
	      return 0 ; // case NM_RCLICK:

	    case LVN_KEYDOWN:

	      switch( pnm.keydown->wVKey )
		{
		case VK_DELETE:
		  PostMessage (hwnd, WM_COMMAND, IDM_RULE_REMOVE, 0) ;
		  break ;
		}

	      return 0 ;
	    }
	  
	  break ; // case IDC_RULELIST:	   

	case IDM_PROGRAM_ADD:
	  if( pnm.header->code == TTN_GETDISPINFO )
	    pnm.getdispinfo->lpszText = (LPTSTR) STR_DEF (_ADD_PROGRAM,g_szAddProgram) ;  
	  return 0 ; // IDM_PROGRAM_ADD:

	case IDM_PROGRAM_EDIT:
	  if( pnm.header->code == TTN_GETDISPINFO )
	    pnm.getdispinfo->lpszText = (LPTSTR) STR_DEF (_EDIT_PROGRAM,g_szEditProgram) ;  
	  return 0 ; // IDM_PROGRAM_EDIT:	  

	case IDM_PROGRAM_REMOVE:
	  if( pnm.header->code == TTN_GETDISPINFO )
	    pnm.getdispinfo->lpszText = (LPTSTR) STR_DEF (_REMOVE_PROGRAM,g_szRemoveProgram) ;  
	  return 0 ; // IDM_PROGRAM_ADD:

	case IDM_RULE_ADD:
	  if( pnm.header->code == TTN_GETDISPINFO )
	    pnm.getdispinfo->lpszText = (LPTSTR) STR_DEF (_ADD_RULE,g_szAddRule) ;  
	  return 0 ; // IDM_RULE_ADD:

	case IDM_RULE_EDIT:
	  if( pnm.header->code == TTN_GETDISPINFO )
	    pnm.getdispinfo->lpszText = (LPTSTR) STR_DEF (_EDIT_RULE,g_szEditRule) ;  
	  return 0 ; // IDM_RULE_EDIT:	  

	case IDM_RULE_REMOVE:
	  if( pnm.header->code == TTN_GETDISPINFO )
	    pnm.getdispinfo->lpszText = (LPTSTR) STR_DEF (_REMOVE_RULE,g_szRemoveRule) ;  
	  return 0 ; // IDM_RULE_ADD:

	case IDM_RULE_UP:
	  if( pnm.header->code == TTN_GETDISPINFO )
	    pnm.getdispinfo->lpszText = (LPTSTR) STR_DEF (_MOVE_UP_RULE,g_szMoveUpRule) ;  
	  return 0 ; // IDM_RULE_UP:

	case IDM_RULE_DOWN:
	  if( pnm.header->code == TTN_GETDISPINFO )
	    pnm.getdispinfo->lpszText = (LPTSTR) STR_DEF (_MOVE_DOWN_RULE,g_szMoveDownRule) ;  
	  return 0 ; // IDM_RULE_DOWN:
	  
	} // switch( pnm.header->idFrom )

      return 0 ;

    }

  return DefWindowProc (hwnd, message, wParam, lParam) ;
}



/******************************************************************/
/* Internal function : WndProc                                    */
/******************************************************************/

DWORD WINAPI _FilterWnd_UpdateIconThread (VOID * pContext) 
{
  HWND		hwndList = (HWND)pContext ; 
  HIMAGELIST	hImageList = ListView_GetImageList (hwndList, LVSIL_SMALL) ;

  TCHAR		szBuffer[MAX_PATH] ;
  int		i ;

  ImageList_RemoveAll (hImageList) ;

  for( i=0 ; i<ListView_GetItemCount(hwndList) ; i++ )
    {
      LVITEM	lvi ;

      ZeroMemory (&lvi, sizeof(lvi)) ;
      lvi.mask		= LVIF_TEXT| LVIF_STATE ;
      lvi.iItem		= i ;
      lvi.iSubItem	= COL_PROGRAM ;
      lvi.pszText	= szBuffer ;
      lvi.cchTextMax	= MAX_PATH ;

      if( ListView_GetItem (hwndList, &lvi) )
	{
	  HICON	hIcon ;
	  
	  if( i==0 ) // default filter
	    {
	      hIcon = LoadIcon (GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DEFAULT)) ;
	    }
	  else
	    {
	      SHFILEINFO sfi ;
	      
	      if( SHGetFileInfo (szBuffer, 0, &sfi, sizeof(sfi), SHGFI_ICON|SHGFI_SMALLICON) )
		{
		  hIcon = sfi.hIcon ;
		}
	      else
		{
		  hIcon = LoadIcon (GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_UNKNOWN)) ;
		}	      
	    }
	  
	  ImageList_AddIcon (hImageList, hIcon) ;
	  DestroyIcon (hIcon) ;
	  
	  lvi.mask	= LVIF_IMAGE;
	  lvi.iImage	= i ;
	  
	  ListView_SetItem (hwndList, &lvi) ;
	}     
    }

  return 0 ;
}
