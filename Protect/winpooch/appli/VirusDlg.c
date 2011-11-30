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
#include "VirusDlg.h"

// project's headers
#include "Language.h"
#include "PicBtnCtl.h"
#include "ProjectInfo.h"
#include "Resources.h"
#include "FiltRule.h"

// standard headers
#include <tchar.h>
#include <strings.h>


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

// time-out timer id
#define TIMER_TIMEOUT	1

#define TIMEOUT_PERIOD	40 /*seconds*/

#define CX_IMAGE	136
#define CY_IMAGE	184
#define CX_SPACE	20


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  HWND	hwndImage ;
  UINT	nTimeOut ;
} WNDDATA ;


/******************************************************************/
/* Exported function : DlgProc                                    */
/******************************************************************/

BOOL CALLBACK VirusDlg_DlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{
  static	HINSTANCE	g_hInstance ;

  WNDDATA	* pData ;
    
  TCHAR		szBuffer[1024] ;
  LPCTSTR	szBaseName ;
  RECT		rect ;
  int		w, h, i ;
  int		nResult ;


  pData = (WNDDATA*) GetWindowLong (hDlg, GWL_USERDATA) ;

  switch( message )
    {
    case WM_INITDIALOG:

      // get instance handle
      g_hInstance = (HINSTANCE) GetWindowLong (hDlg, GWL_HINSTANCE) ;

      pData = (WNDDATA*) malloc (sizeof(WNDDATA)) ;
      SetWindowLong (hDlg, GWL_USERDATA, (LONG)pData) ;

      GetClientRect (hDlg, &rect) ;
      w = rect.right ;
      h = rect.bottom ;

      pData->hwndImage = CreateWindow (WC_PICBTNCTL, NULL,
				       WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE, 
				       w-CX_IMAGE-CX_SPACE, (h-CY_IMAGE)/2, CX_IMAGE, CY_IMAGE,
				       hDlg, NULL, g_hInstance, NULL) ; 

      PicBtnCtl_SetImage (pData->hwndImage, MAKEINTRESOURCE(IDB_VIRUS)) ;
      PicBtnCtl_SetSelected (pData->hwndImage, TRUE) ;

      pData->nTimeOut = TIMEOUT_PERIOD ;
      SendMessage (hDlg, WM_TIMER, TIMER_TIMEOUT, 0) ;
      SetTimer (hDlg, TIMER_TIMEOUT, 1000, NULL) ;     
       
      // find basename
      szBaseName = _tcsrchr (((VIRUSDLGPARAMS*)lParam)->szFile, TEXT('\\')) ;
      if( szBaseName ) szBaseName++ ;
      else szBaseName = ((VIRUSDLGPARAMS*)lParam)->szFile ;

      wsprintf (szBuffer, 
		STR_DEF (_FILE_S_IS_INFECTED,TEXT("The file %s may be infected !")), 
		szBaseName) ;      
      SetDlgItemText (hDlg, IDT_FILE, szBuffer) ;
      
      SetDlgItemText (hDlg, IDC_REPORT, ((VIRUSDLGPARAMS*)lParam)->szReport) ;
      SendDlgItemMessage (hDlg, IDC_REPORT, EM_SETSEL, 0, 0) ;      

      if( Language_IsLoaded() ) {
	SetDlgItemText (hDlg, IDT_REPORT, STR(_ANTIVIRUS_REPORT)) ;
	SetDlgItemText (hDlg, IDC_ACCEPT, STR(_ACCEPT)) ;
	SetDlgItemText (hDlg, IDC_FEIGN, STR(_FEIGN)) ;
	SetDlgItemText (hDlg, IDC_REJECT, STR(_REJECT)) ;
      }	
      
      SetFocus (hDlg) ;
      SetForegroundWindow (hDlg) ;

      return TRUE ;

    case WM_DESTROY:

      KillTimer (hDlg, TIMER_TIMEOUT) ;
      free (pData) ;

      return TRUE ;

  
    case WM_TIMER:
      
      i = wsprintf (szBuffer, TEXT("%s "),
		    STR_DEF(_WHAT_DO_YOU_WANT,TEXT("What do you want ?"))) ;
      wsprintf (szBuffer+i, 
		STR_DEF (_S_IN_D_SECONDS,TEXT("(%s in %d seconds)")), 
		STR_DEF (_REJECT, TEXT("Reject")), pData->nTimeOut) ;
      SetDlgItemText (hDlg, IDT_REACTION, szBuffer) ;
		
      if( ! --pData->nTimeOut )
	PostMessage (hDlg, WM_COMMAND, MAKELONG(IDC_REJECT,0), 0) ;
	
      return TRUE ;


    case WM_COMMAND:

      switch( LOWORD(wParam) )
	{
	case IDC_ACCEPT:
	  
	  if( IDYES!=MessageBox(hDlg,
				STR_DEF(_ARE_YOU_SURE,TEXT("Are you sure ?")),
				TEXT(APPLICATION_NAME), 
				MB_ICONQUESTION|MB_YESNO) )
	    return 0 ;
	  
	case IDC_FEIGN:
	case IDC_REJECT:
	  
	  nResult = RULE_ACCEPT ;
	  if( LOWORD(wParam)==IDC_FEIGN ) nResult = RULE_FEIGN ;
	  if( LOWORD(wParam)==IDC_REJECT ) nResult = RULE_REJECT ; 
       	  	  
	  EndDialog (hDlg, nResult) ;
	  return TRUE ;
	}

      return FALSE ; // case WM_COMMAND:
    }
 
  return FALSE ;
}
