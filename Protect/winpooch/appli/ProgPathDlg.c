/******************************************************************/
/*                                                                */
/*  Winpooch : Windows Watchdog                                   */
/*  Copyright (C) 2004-2005  Benoit Blanchon                      */
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
#include "ProgPathDlg.h"

// standard headers
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <tchar.h>

// project's headers
#include "Assert.h"
#include "Language.h"
#include "ProcList.h"
#include "ProjectInfo.h"
#include "Resources.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define MAX_FILTER	128


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

BOOL _ProgPathDlg_Browse (HWND hDlg, LPTSTR szPath, UINT nMax) ;

BOOL _ProgPathDlg_EnumCallback (void *, PROCSTRUCT * ) ;


/******************************************************************/
/* Exported function : DlgProc                                    */
/******************************************************************/

BOOL CALLBACK ProgPathDlg_DlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  static LPTSTR		g_szDst ;
  static HWND		g_hwndList ;

  TCHAR		szPath[MAX_PATH] ;
  LV_COLUMN	lvc ;

  union {    
    NMHDR		*header ;
    NMITEMACTIVATE	*itemactivate ;
    NMLISTVIEW		*listview ; 
  } pnm ;

  switch( message )
    {
    case WM_INITDIALOG:

      // verify params
      ASSERT (lParam!=0) ;
      
      // set static variables
      g_szDst = (LPTSTR) lParam ;
      g_hwndList = GetDlgItem (hDlg, IDC_PROCESSLIST) ;

      // set path edit box
      SetDlgItemText (hDlg, IDC_PATH, g_szDst) ;

      // create the only column
      ZeroMemory (&lvc, sizeof(lvc)) ;
      lvc.mask = LVCF_TEXT|LVCF_WIDTH ;
      lvc.cx = 500 ;
      lvc.pszText = (LPTSTR) STR_DEF(_HOOKED_PROCESSES,TEXT("Hooked processes")) ;
      ListView_InsertColumn (g_hwndList, lvc.iSubItem, &lvc) ;   

      // fill program list
      ProcList_Lock () ;
      ProcList_Enum (_ProgPathDlg_EnumCallback, g_hwndList) ;
      ProcList_Unlock () ;

      if( Language_IsLoaded() )
	{
	  SetWindowText (hDlg, STR(_SET_PROGRAM_PATH)) ;
	  SetDlgItemText (hDlg, IDT_PATH, STR(_PROGRAM_PATH)) ;
	  SetDlgItemText (hDlg, IDOK, STR(_OK)) ;
	  SetDlgItemText (hDlg, IDCANCEL, STR(_CANCEL)) ;
	}

      return TRUE ;

    case WM_COMMAND:

      switch( wParam )
	{
	  
	case MAKELONG(IDC_BROWSE,BN_CLICKED) :

	  GetDlgItemText (hDlg, IDC_PATH, szPath, MAX_PATH) ;
	  if( _ProgPathDlg_Browse (hDlg, szPath, MAX_PATH) )
	    SetDlgItemText (hDlg, IDC_PATH, szPath) ;
	  
	  return TRUE ;	// case IDC_BROWSE:

	case MAKELONG(IDOK,BN_CLICKED):
	  
	  GetDlgItemText (hDlg, IDC_PATH, g_szDst, MAX_PATH) ;
	/*  
	  if( 0xFFFFFFFF==GetFileAttributes(g_szDst) )
	    {
	      MessageBox (hDlg, TEXT("File not found.\nPlease set a valid file path."),
			  TEXT(APPLICATION_NAME), MB_ICONERROR) ;
	      return TRUE ;
	    }
	*/
	  
	case MAKELONG(IDCANCEL,BN_CLICKED):
	  
	  EndDialog (hDlg, LOWORD(wParam)) ;

	  return TRUE ;

	} // switch( LOWORD(wParam) )

      return FALSE ; // case WM_COMMAND:


    case WM_NOTIFY:

      pnm.header = (NMHDR*) lParam ;

      switch( pnm.header->code )
	{
	case LVN_ITEMCHANGED:
	  
	  if( pnm.listview->uNewState & LVIS_SELECTED )
	    {
	      ListView_GetItemText (g_hwndList, pnm.listview->iItem, 0, 
				    szPath, MAX_PATH*sizeof(TCHAR)) ;
	      SetDlgItemText (hDlg, IDC_PATH, szPath) ;
	    }

	  return TRUE ;
	}

      return FALSE ;
    }

  
  return FALSE ;
}


/******************************************************************/
/* Internal function : Browse                                     */
/******************************************************************/

BOOL _ProgPathDlg_EnumCallback (void * pContext, PROCSTRUCT * pProc) 
{
  HWND		hwndList = (HWND)pContext ;
  LVITEM	lvi = { 0 } ;
  LVFINDINFO	lvfi = { 0 } ;
  

  // verify params
  ASSERT (pContext!=NULL) ;
  ASSERT (pProc!=NULL) ;

  if( pProc->nState==PS_HOOKED_SINCE_BIRTH ||
      pProc->nState==PS_HOOKED_WHILE_RUNNING ) 
    {
      lvfi.flags	= LVFI_STRING ;
      lvfi.psz		= pProc->szPath ;

      if( ListView_FindItem(hwndList,-1,&lvfi) < 0 )
	{	  
	  lvi.mask = LVIF_TEXT ;
	  lvi.pszText = pProc->szPath ;   
	  lvi.iItem = ListView_GetItemCount (hwndList) ;
	  ListView_InsertItem (hwndList, &lvi) ;
	}
    }

  return TRUE ;
}



/******************************************************************/
/* Internal function : Browse                                     */
/******************************************************************/

BOOL _ProgPathDlg_Browse (HWND hDlg, LPTSTR szPath, UINT nMax)
{
  TCHAR szFilter[MAX_FILTER] ;
  INT   iPos ;  
  
  OPENFILENAME ofn = {
    .lStructSize = sizeof(OPENFILENAME),
    .hwndOwner   = hDlg,
    .nMaxFile    = nMax,
    .lpstrFile   = szPath,
    .lpstrFilter = szFilter,
    .Flags       = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY,
  } ;
  
  szPath[0] = 0 ;
  
  iPos = 0 ;
  iPos += 1 + _sntprintf (szFilter+iPos, MAX_FILTER-iPos, TEXT("%s (*.exe)"),
			  STR_DEF(_EXECUTABLE_FILES,TEXT("Executable files"))) ;
  iPos += 1 + _sntprintf (szFilter+iPos, MAX_FILTER-iPos, 
			  TEXT("*.exe")) ;
  iPos += 1 + _sntprintf (szFilter+iPos, MAX_FILTER-iPos, TEXT("%s (*.*)"),
			  STR_DEF(_ALL_FILES,TEXT("All files"))) ;
  iPos += 1 + _sntprintf (szFilter+iPos, MAX_FILTER-iPos, 
			  TEXT("*.*")) ;
  szFilter[iPos] = 0 ;
  
  return GetOpenFileName (&ofn) ;
}
