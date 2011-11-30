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
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "HistoryWnd.h"

// standard headers
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>

// project's headers
#include "Assert.h"
#include "EventLog.h"
#include "Language.h"
#include "LogFile.h"
#include "ProjectInfo.h"
#include "Resources.h"
#include "FiltRule.h"
#include "FilterTools.h"
#include "RuleDlg.h"
#include "SpyServer.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define COL_TIME	0
#define COL_PROCESS	1
#define COL_PID		2
#define COL_REACTION	3
#define COL_CONDITION	4

#define WC_HISTORYWND TEXT("HistoryWnd")

LPCTSTR	g_szClearHistory = TEXT("Clear history") ;
LPCTSTR g_szCreateRuleFromEvent = TEXT("Create a rule from selected event") ;
LPCTSTR g_szViewLogFile = TEXT("View log file") ;

#define IM_CLOCK	0
#define IM_REASON(n)	(n+1)


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

LRESULT CALLBACK _HistoryWnd_WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) ;


/******************************************************************/
/* Exported function : RegisterClass                              */
/******************************************************************/

BOOL HistoryWnd_RegisterClass (HINSTANCE hInstance) 
{
  WNDCLASS wndclass ;

  wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
  wndclass.lpfnWndProc   = _HistoryWnd_WndProc ;
  wndclass.cbClsExtra    = 0 ;
  wndclass.cbWndExtra    = 0 ;
  wndclass.hInstance     = hInstance ;
  wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION) ;
  wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
  wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1) ;
  wndclass.lpszMenuName  = NULL ;
  wndclass.lpszClassName = WC_HISTORYWND ;

  return 0!=RegisterClass (&wndclass) ;
}


/******************************************************************/
/* Exported function : CreateWindow                               */
/******************************************************************/

HWND HistoryWnd_CreateWindow (HINSTANCE hInstance, HWND hwndParent)
{  
  return CreateWindow (WC_HISTORYWND, NULL,
		       WS_CHILD,
		       CW_USEDEFAULT, CW_USEDEFAULT,
		       CW_USEDEFAULT, CW_USEDEFAULT, 
		       hwndParent, NULL, hInstance, NULL) ;
}


/******************************************************************/
/* Internal function : WndProc                                    */
/******************************************************************/

LRESULT CALLBACK _HistoryWnd_WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  static HWND		g_hwndList ;
  static HINSTANCE	g_hInstance ;
  static DWORD		g_nNextEventId ;
  static DWORD		g_nSelectedEventId ;
  static HMENU		g_hmenuHistory ;

  EVENTSTRUCT	*pEvent ;
  int		nWidth ;
  int		nHeight ;
  TCHAR		szBuffer[1024] ;
  LV_COLUMN	lvc ;
  LVITEM	lvi ;
  HIMAGELIST	hImageList ;

  union {    
    NMHDR		*header ;
    NMITEMACTIVATE	*itemactivate ;
    NMLISTVIEW		*listview ; 
    NMTTDISPINFO	*getdispinfo ; 
  } pnm ;

  switch (message)
    {
    case WM_CREATE:
      
      g_hInstance = ((CREATESTRUCT*)lParam)->hInstance ;

      g_hwndList = CreateWindowEx (WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL,
				  WS_CHILD|WS_VISIBLE|WS_VSCROLL|LVS_REPORT|
				   LVS_SINGLESEL|LVS_SHOWSELALWAYS|LVS_NOSORTHEADER,
				  0,0,0,0, hwnd, (HMENU)IDC_HISTORYLIST, g_hInstance, NULL) ;
      ListView_SetExtendedListViewStyle (g_hwndList, LVS_EX_FULLROWSELECT|LVS_EX_SUBITEMIMAGES) ;

      // menu will be created by WM_LANGUAGECHANGED
      g_hmenuHistory = NULL ;

      // create columns
      ZeroMemory (&lvc, sizeof(lvc)) ;
      lvc.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM/* | LVCF_FMT*/ ;
      lvc.pszText = TEXT("") ;
      //  lvc.fmt = LVCFMT_BITMAP_ON_RIGHT ;

      lvc.cx		= 80 ;
      lvc.iSubItem	= COL_TIME ;
      ListView_InsertColumn (g_hwndList, lvc.iSubItem, &lvc) ; 
      
      lvc.cx		= 90 ;
      lvc.iSubItem	= COL_PROCESS ;
      ListView_InsertColumn (g_hwndList, lvc.iSubItem, &lvc) ; 
      
      lvc.cx		= 40 ;
      lvc.iSubItem	= COL_PID ;
      ListView_InsertColumn (g_hwndList, lvc.iSubItem, &lvc) ; 
      
      lvc.cx		= 60 ;
      lvc.iSubItem	= COL_REACTION ;
      ListView_InsertColumn (g_hwndList, lvc.iSubItem, &lvc) ; 

      lvc.cx		= 480 ;
      lvc.iSubItem	= COL_CONDITION ;
      ListView_InsertColumn (g_hwndList, lvc.iSubItem, &lvc) ;
      
      //
      // Init image list
      //
      hImageList = ImageList_Create (16,16,ILC_COLOR32|ILC_MASK,6,4) ;
      ImageList_AddIcon (hImageList, LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_CLOCK))) ;
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
      ListView_SetImageList (g_hwndList, hImageList, LVSIL_SMALL) ; 
      
      g_nNextEventId = 0 ;
      g_nSelectedEventId = (DWORD)-1 ;

      SendMessage (hwnd, WM_LANGUAGECHANGED, 0, 0) ;
	
      return 0 ; // case WM_CREATE:

    case WM_LANGUAGECHANGED:

      ZeroMemory (&lvc, sizeof(lvc)) ;
      lvc.mask = LVCF_TEXT ;

      lvc.pszText = (LPTSTR) STR_DEF (_TIME, TEXT("Time")) ;
      ListView_SetColumn (g_hwndList, COL_TIME, &lvc) ; 
      
      lvc.pszText = (LPTSTR) STR_DEF (_PROCESS, TEXT("Process")) ;
      ListView_SetColumn (g_hwndList, COL_PROCESS, &lvc) ; 
      
      lvc.pszText = (LPTSTR) STR_DEF (_PID, TEXT("PID")) ;
      ListView_SetColumn (g_hwndList, COL_PID, &lvc) ; 
      
      lvc.pszText = (LPTSTR) STR_DEF (_REACTION, TEXT("Reaction")) ;
      ListView_SetColumn (g_hwndList, COL_REACTION, &lvc) ; 
      
      lvc.pszText = (LPTSTR) STR_DEF (_REASON, TEXT("Reason")) ;
      ListView_SetColumn (g_hwndList, COL_CONDITION, &lvc) ; 

      // create history menu
      DestroyMenu (g_hmenuHistory) ;
      g_hmenuHistory = CreatePopupMenu () ;
      AppendMenu (g_hmenuHistory, MF_STRING|MF_GRAYED, 0, 
		  STR_DEF(_HISTORY_MENU,TEXT("History menu"))) ;
      AppendMenu (g_hmenuHistory, MF_SEPARATOR, 0, NULL) ;
      AppendMenu (g_hmenuHistory, MF_STRING, IDM_HISTORY_CLEAR,
		  STR_DEF(_CLEAN_HISTORY,g_szClearHistory)) ;
      AppendMenu (g_hmenuHistory, MF_STRING, IDM_HISTORY_CREATE_RULE, 
		  STR_DEF(_CREATE_RULE_FROM_EVENT,g_szCreateRuleFromEvent)) ;
      AppendMenu (g_hmenuHistory, MF_STRING, IDM_HISTORY_VIEWLOG, 
		  STR_DEF (_VIEW_LOG_FILE,g_szViewLogFile)) ;

      PostMessage (hwnd, WM_UPDATEHISTORY, 0, 0) ;   

      return 0 ; // case WM_LANGUAGECHANGED:
      
    case WM_DESTROY:

      DestroyMenu (g_hmenuHistory) ;
   
      return 0 ;

    case WM_SIZE:
      nWidth = LOWORD (lParam) ;
      nHeight = HIWORD (lParam) ;
      MoveWindow (g_hwndList, 0, 0, nWidth, nHeight, TRUE) ;

      return 0 ;

    case WM_UPDATEHISTORY:
      
      if( g_nNextEventId < EventLog_GetBeginId() )
	g_nNextEventId = EventLog_GetBeginId() ;

      while( g_nNextEventId < EventLog_GetEndId() )
	{
	  pEvent = EventLog_MapEvent (g_nNextEventId) ;

	  if( pEvent )
	    {    
	      ZeroMemory (&lvi, sizeof(lvi)) ;	      
	      lvi.mask = LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE ;
	      lvi.iImage = IM_CLOCK ;
	      lvi.iItem = ListView_GetItemCount (g_hwndList) ;

	      wsprintf (szBuffer, TEXT("%02u:%02u:%02u"),
			pEvent->time.wHour, pEvent->time.wMinute, pEvent->time.wSecond) ;

	      // TIME
	      lvi.iSubItem = COL_TIME ;
	      lvi.pszText = szBuffer ;
	      lvi.lParam = g_nNextEventId ;
	      ListView_InsertItem (g_hwndList, &lvi) ;

	      // PROCESS NAME
	      lvi.mask = LVIF_TEXT ;
	      lvi.iSubItem = COL_PROCESS ;
	      lvi.pszText = pEvent->szExeName ;
	      ListView_SetItem (g_hwndList, &lvi) ;

	      // PROCESS ID
	      wsprintf (szBuffer, TEXT("%u"), pEvent->dwProcessId) ;
	      lvi.iSubItem = COL_PID ;
	      lvi.pszText = szBuffer ;
	      ListView_SetItem (g_hwndList, &lvi) ;

	      // REACTION
	      wsprintf (szBuffer, TEXT("%s"), 
			pEvent->nReaction & RULE_REJECT ? STR_DEF(_REJECTED,TEXT("rejected")) : 
			pEvent->nReaction & RULE_FEIGN ? STR_DEF(_FEIGNED,TEXT("feigned")) : 
			STR_DEF(_ACCEPTED,TEXT("accepted"))) ;
	      lvi.iSubItem = COL_REACTION ;
	      lvi.pszText = szBuffer ;
	      ListView_SetItem (g_hwndList, &lvi) ;
	      
	      // REASION
	      FiltCond_ToString (&pEvent->condition, szBuffer, 1024) ;
	      lvi.mask		= LVIF_TEXT | LVIF_IMAGE ;
	      lvi.iImage	= IM_REASON(pEvent->condition.nReason) ;
	      lvi.iSubItem	= COL_CONDITION ;
	      lvi.pszText	= szBuffer ;
	      ListView_SetItem (g_hwndList, &lvi) ;	
	    }
	  else
	    {
	      TRACE_ERROR (TEXT("EventLog_MapEvent failed\n")) ;
	    }

	  EventLog_UnmapEvent (g_nNextEventId) ;
	  g_nNextEventId++ ;
	}

      return 0 ; // case WM_UPDATEHISTORY:

   case WM_NOTIFY:
      
      pnm.header = (NMHDR*)lParam ;
      
      switch( pnm.header->idFrom )
	{
	case IDC_HISTORYLIST:
	  
	  switch( pnm.header->code )
	    {
	    case LVN_ITEMCHANGED:
	      
	      if( pnm.listview->uNewState & LVIS_SELECTED )
		{
		  g_nSelectedEventId = pnm.listview->lParam ;
		}	
	      
	      return 0 ; // case LVN_ITEMCHANGED:

	    case NM_RCLICK: //  ======== Right click on "history" list-view ======== 
	      {
		BOOL	bEnable ;
		POINT	pt ;
		
      		// get mouse position
		GetCursorPos (&pt) ;

		bEnable = g_nSelectedEventId!=(DWORD)-1 ;
		EnableMenuItem (g_hmenuHistory, IDM_HISTORY_CREATE_RULE, 
				MF_BYCOMMAND|(bEnable?MF_ENABLED:MF_GRAYED)) ;	  

		// display menu
		TrackPopupMenu (g_hmenuHistory, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL) ;		
	      }
	      return 0 ; //  ======== Right click on "history" list-view ======== 	      
	    }

	}
         
      return 0 ; //  case WM_NOTIFY:

    case WM_COMMAND:
      
      switch( LOWORD(wParam))
	{
	case IDM_HISTORY_CREATE_RULE: //  ======== 'Create rule' command ========
	  {
	    FILTRULE	* pRule ;
	    EVENTSTRUCT	* pEvent ;

	    pEvent = EventLog_MapEvent (g_nSelectedEventId) ;
	    if( ! pEvent ) return 0 ;
	    
	    pRule = (FILTRULE*) malloc (sizeof(FILTRULE)) ;
	    pRule->nReaction	= pEvent->nReaction ;
	    pRule->nVerbosity	= pEvent->nVerbosity ;
	    pRule->nOptions	= 0 ;
	    FiltCond_Dup (&pRule->condition, &pEvent->condition) ;
	    
	    if( IDOK==RuleDlg_DialogBox (g_hInstance, hwnd, pEvent->szPath, pRule, TRUE) )
	      {
		SpySrv_AddRuleForProgram (pRule, pEvent->szPath) ;
	      }
	    else
	      {
		FiltRule_Clear (pRule) ;
		free (pRule) ;
	      }

	    EventLog_UnmapEvent (g_nSelectedEventId) ;
	  } 

	  return 0 ;  //  ======== 'Create rule' command ========

	case IDM_HISTORY_CLEAR: //  ======== 'Clear history' command ========
	  {
	    EventLog_Clear () ;
	    ListView_DeleteAllItems (g_hwndList) ;
	  }
	  return 0 ; //  ======== 'Clear history' command ========
	  
	case IDM_HISTORY_VIEWLOG:
	  ShellExecute (hwnd, NULL, TEXT("notepad.exe"), 
			LogFile_GetPath(), NULL, SW_SHOW) ;
	  return 0 ;
	}

      break ; // case WM_COMMAND:
    }

  return DefWindowProc (hwnd, message, wParam, lParam) ;
}
