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
#include "AskDlg.h"

// project's headers
#include "Assert.h"
#include "Language.h"
#include "PicBtnCtl.h"
#include "Resources.h"
#include "FiltRule.h"
#include "FilterTools.h"
#include "Trace.h"

// standard headers
#include <tchar.h>


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

// time-out timer id
#define TIMER_TIMEOUT	1

#define TIMEOUT_PERIOD	40 /*seconds*/

#define CX_IMAGE	136
#define CY_IMAGE	184
#define CX_SPACE	12



/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  HANDLE	hMutex ;
} INTERNALDATA ;

typedef struct {
  LPCTSTR	szProgram ;
  DWORD		nProcessId ;
  UINT		nDefReaction ;
  PFILTCOND	pCondition ;
} DLGPARAMS ;

typedef struct {
  HWND	hwndImage ;
  UINT	nTimeOut ;
  UINT	nDefReaction ;
} WNDDATA ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static INTERNALDATA g_data ;

/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

BOOL CALLBACK _AskDlg_DlgProc (HWND, UINT, WPARAM, LPARAM) ;

UINT _AskDlg_CondToString (PFILTCOND pCond, LPTSTR szBuffer, UINT nSize) ;



/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL AskDlg_Init ()
{
  g_data.hMutex = CreateMutex (NULL, FALSE, NULL) ;

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL AskDlg_Uninit () 
{
  CloseHandle (g_data.hMutex) ;
  g_data.hMutex = NULL ;

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

UINT AskDlg_DialogBox (HINSTANCE	hInstance,
		       HWND		hwndParent,
		       LPCTSTR		szProgram,
		       UINT		nProcessId,
		       UINT		nDefReaction,
		       PFILTCOND	pCondition) 
{
  DLGPARAMS params ;
  UINT nResult ;

  params.szProgram = szProgram ;
  params.nProcessId = nProcessId ;
  params.nDefReaction = nDefReaction ;
  params.pCondition = pCondition ;

  WaitForSingleObject (g_data.hMutex, INFINITE) ;
  
  nResult = DialogBoxParam (hInstance, MAKEINTRESOURCE(DLG_ASK), hwndParent, 
			    _AskDlg_DlgProc, (LPARAM)&params) ;

  ReleaseMutex (g_data.hMutex) ;

  return nResult ;
}

/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL CALLBACK _AskDlg_DlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{
  static	HINSTANCE	g_hInstance ;

  WNDDATA	* pData ;
    
  TCHAR		szBuffer[256] ;
  int		w, h ;
  RECT		rect ;
  
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

      PicBtnCtl_SetImage (pData->hwndImage, MAKEINTRESOURCE(IDB_ASK)) ;
      PicBtnCtl_SetSelected (pData->hwndImage, TRUE) ;

      pData->nTimeOut = TIMEOUT_PERIOD ;
      SendMessage (hDlg, WM_TIMER, TIMER_TIMEOUT, 0) ;
      SetTimer (hDlg, TIMER_TIMEOUT, 1000, NULL) ;     

      pData->nDefReaction = ((DLGPARAMS*)lParam)->nDefReaction ;      
      
      wsprintf (szBuffer, TEXT("%s (%u)"), 
		((DLGPARAMS*)lParam)->szProgram,
		((DLGPARAMS*)lParam)->nProcessId) ;
      SetDlgItemText (hDlg, IDC_PROCESS, szBuffer) ;
      SendDlgItemMessage (hDlg, IDC_PROCESS, EM_SETSEL, 0, 0) ;      

      _AskDlg_CondToString (((DLGPARAMS*)lParam)->pCondition, szBuffer, 256) ;
      SetDlgItemText (hDlg, IDC_REASON, szBuffer) ;
      SendDlgItemMessage (hDlg, IDC_REASON, EM_SETSEL, 0, 0) ;    

      if( Language_IsLoaded() ) {
	SetDlgItemText (hDlg, IDT_PROCESS, STR(_THE_FOLLOWING_PROCESS)) ;
	SetDlgItemText (hDlg, IDT_REASON, STR(_IS_TRYING_TO)) ;
	SetDlgItemText (hDlg, IDT_OPTIONS, STR(_OTHER_OPTIONS)) ;
	SetDlgItemText (hDlg, IDC_NEW_FILTER, STR(_NEW_FILTER)) ;
	SetDlgItemText (hDlg, IDC_ACCEPT, STR(_ACCEPT)) ;
	SetDlgItemText (hDlg, IDC_FEIGN, STR(_FEIGN)) ;
	SetDlgItemText (hDlg, IDC_REJECT, STR(_REJECT)) ;
	SetDlgItemText (hDlg, IDC_UNHOOK, STR(_UNHOOK_PROCESS)) ;
	SetDlgItemText (hDlg, IDC_KILL_PROCESS, STR(_KILL_PROCESS)) ;
      }	  
      
      SetFocus (hDlg) ;
      SetForegroundWindow (hDlg) ;

      return TRUE ;

    case WM_DESTROY:

      KillTimer (hDlg, TIMER_TIMEOUT) ;
      free (pData) ;

      return TRUE ;
 
    case WM_TIMER:
      {
	LPCTSTR szDefReaction ;

	switch( pData->nDefReaction )
	  {
	  case RULE_REJECT:
	    szDefReaction = STR_DEF(_REJECT,TEXT("Reject")) ;
	    break ;
	  case RULE_FEIGN:
	    szDefReaction = STR_DEF(_FEIGN,TEXT("Feign")) ;
	    break ;
	  case RULE_ACCEPT:
	    szDefReaction = STR_DEF(_ACCEPT,TEXT("Accept")) ;
	    break ;
	  case RULE_KILLPROCESS:
	    szDefReaction = STR_DEF(_KILL_PROCESS,TEXT("Kill")) ;
	    break ;
	  default:
	    szDefReaction = TEXT("???") ;
	  }

	_tcscpy (szBuffer, STR_DEF(_WHAT_DO_YOU_WANT,TEXT("What do you want ?"))) ;
	_tcscat (szBuffer, TEXT(" ")) ;
	wsprintf (szBuffer+_tcslen(szBuffer), 
		  STR_DEF(_S_IN_D_SECONDS,TEXT("(%s in %d seconds)")), 
		  szDefReaction, pData->nTimeOut) ;
	SetDlgItemText (hDlg, IDT_REACTION, szBuffer) ;
		
	if( ! --pData->nTimeOut )
	  {	    
	    switch( pData->nDefReaction )
	      {
	      case RULE_REJECT:
		EndDialog (hDlg, ASKDLG_REJECT) ;
		break ;
	      case RULE_FEIGN:
		EndDialog (hDlg, ASKDLG_FEIGN) ;
		break ;
	      case RULE_ACCEPT:
		EndDialog (hDlg, ASKDLG_ACCEPT) ;
		break ;
	      case RULE_KILLPROCESS:
		EndDialog (hDlg, ASKDLG_KILLPROCESS) ;
		break ;
	      }
	  }
      }
      return TRUE ;


    case WM_COMMAND:

      switch( LOWORD(wParam) )
	{
	case IDC_ACCEPT:
	  EndDialog (hDlg, ASKDLG_ACCEPT) ;	  
	  return TRUE ;

	case IDC_FEIGN:
	  EndDialog (hDlg, ASKDLG_FEIGN) ;
	  return TRUE ;

	case IDC_REJECT:
	  EndDialog (hDlg, ASKDLG_REJECT) ;
	  return TRUE ;

	case IDC_NEW_FILTER:
	  EndDialog (hDlg, ASKDLG_CREATEFILTER) ;	  
	  return TRUE ;	 

	case IDC_UNHOOK:
	  EndDialog (hDlg, ASKDLG_UNHOOK) ;	  
	  return TRUE ;	   

	case IDC_KILL_PROCESS:	 
	  EndDialog (hDlg, ASKDLG_KILLPROCESS) ;	  
	  return TRUE ;	   
	}

      return FALSE ; // case WM_COMMAND:
    }
 
  return FALSE ;
}

/******************************************************************/
/* Internal function                                              */
/******************************************************************/

UINT _AskDlg_CondToString (PFILTCOND pCond, LPTSTR szBuffer, UINT nSize)
{
  UINT i, n ;

  LPCTSTR szReason = STR_DEF(_REASON,TEXT("Reason")) ;
  LPCTSTR szParam = STR_DEF(_PARAM,TEXT("Param")) ;
  
  n = wsprintf (szBuffer, TEXT("%s = "), szReason) ;
  
  n += FiltCond_GetReasonAsString (pCond, szBuffer+n, nSize-n) ;
  
  for( i=0 ; i<pCond->nParams ; i++ )
    {
      n += wsprintf (szBuffer+n, TEXT("\r\n%s %d = "), szParam, i+1) ;
      
      n += FiltCond_GetParamAsString (pCond, i, szBuffer+n, nSize-n) ;	 
    }
  
  n += wsprintf (szBuffer+n, TEXT("\r\n")) ;

  return n ;
}
