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

#define TRACE_LEVEL	2	// warning level


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// Windows' headers
#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>

// standard headers
#include <tchar.h>

// module's interface
#include "RuleDlg.h"

// project's headers
#include "Assert.h"
#include "FiltCond.h"
#include "Language.h"
#include "Reason.h"
#include "Resources.h"
#include "FiltRule.h"
#include "FilterTools.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define WM_UPDATE_TAB	(WM_USER+11)
#define WM_SETPARAMPTR	(WM_USER+12)
#define WM_SAVEPARAM	(WM_USER+13)


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct 
{
  UINT nType ;
  TCHAR szValue[1024] ;
} PARAM ;

typedef struct 
{
  LPCTSTR	szProgram ;
  FILTRULE	*pRule ;
  BOOL		bAllowChangeReason ;
} DLGPARAMS ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

BOOL _RuleDlg_SetCond (FILTCOND *, UINT nReason, UINT nParams, PARAM * pParams) ;

BOOL CALLBACK _RuleDlg_DlgProc (HWND, UINT, WPARAM, LPARAM) ;


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

INT_PTR RuleDlg_DialogBox (HINSTANCE hInstance, HWND hwndParent, 
			   LPCTSTR szProgram, FILTRULE* pRule, 
			   BOOL bAllowChangeReason) 
{
  DLGPARAMS params =
    {
      .szProgram = szProgram,
      .pRule = pRule,
      .bAllowChangeReason = bAllowChangeReason
    } ;

  ASSERT (szProgram!=NULL) ;
  ASSERT (pRule!=NULL) ;

  return DialogBoxParam (hInstance, MAKEINTRESOURCE(DLG_RULE), hwndParent, 
			 _RuleDlg_DlgProc, (LPARAM)&params) ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL CALLBACK _RuleDlg_ParamDlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{
  typedef struct 
  {
    PARAM	* pParam ;
    HWND hwndType ;
  } WNDDATA ;
  
  WNDDATA	*pData = (WNDDATA*) GetWindowLong (hDlg, GWL_USERDATA) ;

  switch( message )
    {
    case WM_INITDIALOG:

      // alloc window data
      pData = calloc (1, sizeof(WNDDATA)) ;
      SetWindowLong (hDlg, GWL_USERDATA, (LONG)pData) ;

      // init static data
      pData->hwndType = GetDlgItem (hDlg, IDC_TYPE) ;
    
      SendMessage (hDlg, WM_LANGUAGECHANGED, 0, 0) ;

      return TRUE ;

    case WM_DESTROY:
      
      free (pData) ;
      
      return TRUE ;

    case WM_LANGUAGECHANGED:

      if( Language_IsLoaded () ) {
	SetDlgItemText (hDlg, IDT_TYPE, STR(_TYPE)) ;
	SetDlgItemText (hDlg, IDT_VALUE, STR(_VALUE)) ;
      }
      
      // set strings
      ComboBox_AddString (pData->hwndType, STR_DEF(_ANY_VALUE,TEXT("Any value"))) ;
      ComboBox_AddString (pData->hwndType, STR_DEF(_INTEGER,TEXT("Integer"))) ;
      ComboBox_AddString (pData->hwndType, STR_DEF(_STRING,TEXT("String"))) ;
      ComboBox_AddString (pData->hwndType, STR_DEF(_WILDCARDS,TEXT("Wildcards"))) ;
      ComboBox_AddString (pData->hwndType, STR_DEF(_PATH_SPEC,TEXT("Path (with wildcards)"))) ;
      
      return TRUE ;

    case WM_SETPARAMPTR:

      pData->pParam = (PARAM*) lParam ;

      ComboBox_SetCurSel (pData->hwndType, pData->pParam->nType) ;
      SetDlgItemText (hDlg, IDC_VALUE, pData->pParam->szValue) ;

      return TRUE ;

    case WM_SAVEPARAM:

      if( pData->pParam ) {
	pData->pParam->nType = ComboBox_GetCurSel (pData->hwndType) ;
	GetDlgItemText (hDlg, IDC_VALUE, pData->pParam->szValue, 1024) ;
      }

      return TRUE ;
    }

  return FALSE ; 
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL CALLBACK _RuleDlg_DlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{
  typedef struct 
  {
    HWND	hwndTabCtl ;
    HWND	hwndReason ;
    HWND	hwndParam ;
    PARAM	aParams[MAX_PARAMS] ;
    FILTRULE	*pRule ;     
  } WNDDATA ;

  static HINSTANCE	g_hInstance ;

  UINT		nParam, nParams, nReason ;
  TC_ITEM tie; 
  NMHDR * pnmh ;
  TCHAR	szBuffer[256] ;

  WNDDATA	*pData = (WNDDATA*) GetWindowLong (hDlg, GWL_USERDATA) ;

  switch( message )
    {
    case WM_INITDIALOG: // ======== Creating 'rule' dialog box ========
      {
	int nReaction, nVerbosity, nOptions ;
	
	// alloc window data
	pData = calloc (1, sizeof(WNDDATA)) ;
	SetWindowLong (hDlg, GWL_USERDATA, (LONG)pData) ;
	
	g_hInstance = (HINSTANCE) GetWindowLong (hDlg, GWL_HINSTANCE) ;
	
	pData->pRule = ((DLGPARAMS*)lParam)->pRule ;
	
	pData->hwndTabCtl = GetDlgItem (hDlg, IDC_PARAMS) ;
	pData->hwndReason = GetDlgItem (hDlg, IDC_REASON) ;
     
	pData->hwndParam = CreateDialog (g_hInstance, MAKEINTRESOURCE(DLG_PARAM), 
					 hDlg, _RuleDlg_ParamDlgProc) ;
	
	for( nReason=0 ; nReason<_FILTREASON_COUNT ; nReason++ )
	  ComboBox_AddString (pData->hwndReason, FiltReason_GetName(nReason)) ;

	// if condition already initialized
	if( /*pData->pRule->pCondition*/TRUE )
	  {
	    ASSERT (pData->pRule->condition.nReason<_FILTREASON_COUNT) ;
	    
	    for( nParam=0 ; nParam<FiltCond_GetParamCount(&pData->pRule->condition) ; nParam++ )
	      {
		pData->aParams[nParam].nType = FiltCond_GetParamType (&pData->pRule->condition, nParam) ;
		FiltCond_GetParamAsString (&pData->pRule->condition, nParam, pData->aParams[nParam].szValue, 1024) ;
	      }
	    
	    ComboBox_SetCurSel (pData->hwndReason, pData->pRule->condition.nReason) ; 
	  }
	// else no existing condition
	else
	  {
	    ZeroMemory (pData->aParams, sizeof(pData->aParams)) ; 
	    
	    ComboBox_SetCurSel (pData->hwndReason, 0) ; 
	  }
	
	// enable/disable changing reason
	EnableWindow (pData->hwndReason, ((DLGPARAMS*)lParam)->bAllowChangeReason); 

	// program
	SetDlgItemText (hDlg, IDC_PROGRAM, ((DLGPARAMS*)lParam)->szProgram) ;	
	
	// reaction
	nReaction = pData->pRule->nReaction ;
	CheckDlgButton (hDlg, IDC_ACCEPT,	nReaction==RULE_ACCEPT	? BST_CHECKED : BST_UNCHECKED) ;
	CheckDlgButton (hDlg, IDC_FEIGN,	nReaction==RULE_FEIGN	? BST_CHECKED : BST_UNCHECKED) ;
	CheckDlgButton (hDlg, IDC_REJECT,	nReaction==RULE_REJECT	? BST_CHECKED : BST_UNCHECKED) ;
	
	// verbosity
	nVerbosity = pData->pRule->nVerbosity ;
	CheckDlgButton (hDlg, IDC_SILENT,	nVerbosity==RULE_SILENT	? BST_CHECKED : BST_UNCHECKED) ;
	CheckDlgButton (hDlg, IDC_LOG,		nVerbosity==RULE_LOG	? BST_CHECKED : BST_UNCHECKED) ;
	CheckDlgButton (hDlg, IDC_ALERT,	nVerbosity==RULE_ALERT	? BST_CHECKED : BST_UNCHECKED) ;
	
	// options
	nOptions = pData->pRule->nOptions ;
	CheckDlgButton (hDlg, IDC_ASK,	nOptions & RULE_ASK	? BST_CHECKED : BST_UNCHECKED) ;
	CheckDlgButton (hDlg, IDC_SCAN,	nOptions & RULE_SCAN	? BST_CHECKED : BST_UNCHECKED) ;
	
	SendMessage (hDlg, WM_LANGUAGECHANGED, 0, 0) ;
	SendMessage (hDlg, WM_UPDATE_TAB, 0, 0) ;
      }	
      return TRUE ; // ======== Creating 'rule' dialog box ========

    case WM_LANGUAGECHANGED:

      if( Language_IsLoaded() )
	{
	  SetWindowText (hDlg, STR(_RULE)) ;
	  SetDlgItemText (hDlg, IDT_PROGRAM, STR(_PROGRAM)) ;
	  SetDlgItemText (hDlg, IDT_REASON, STR(_REASON)) ;
	  SetDlgItemText (hDlg, IDT_REACTION, STR(_REACTION)) ;
	  SetDlgItemText (hDlg, IDC_ASK, STR(_ASK)) ;
	  SetDlgItemText (hDlg, IDC_ACCEPT, STR(_ACCEPT)) ;
	  SetDlgItemText (hDlg, IDC_FEIGN, STR(_FEIGN)) ;
	  SetDlgItemText (hDlg, IDC_REJECT, STR(_REJECT)) ;
	  SetDlgItemText (hDlg, IDT_VERBOSITY, STR(_VERBOSITY)) ;
	  SetDlgItemText (hDlg, IDC_SILENT, STR(_SILENT)) ;
	  SetDlgItemText (hDlg, IDC_LOG, STR(_LOG)) ;
	  SetDlgItemText (hDlg, IDC_ALERT, STR(_ALERT)) ;
	  SetDlgItemText (hDlg, IDT_OPTIONS, STR(_OPTIONS)) ;
	  SetDlgItemText (hDlg, IDC_SCAN, STR(_VIRUS_SCAN)) ;
	  SetDlgItemText (hDlg, IDOK, STR(_OK)) ;
	  SetDlgItemText (hDlg, IDCANCEL, STR(_CANCEL)) ;
	  SetDlgItemText (hDlg, IDHELP, STR(_HELP)) ;
	}

      return TRUE ;

    case WM_UPDATE_TAB:

      TabCtrl_DeleteAllItems (pData->hwndTabCtl) ;

      nReason = ComboBox_GetCurSel (pData->hwndReason) ;
      nParams = FiltReason_GetParamCount(nReason) ;

      for( nParam=0 ; nParam<nParams ; nParam++)
	{
	  tie.mask	= TCIF_TEXT | TCIF_IMAGE ; 
	  tie.iImage	= -1; 
	  tie.pszText	= szBuffer ;

	  wsprintf (szBuffer, TEXT("%s %d : \"%s\""),
		    STR_DEF(_PARAM,TEXT("Param")),
		    nParam+1, FiltReason_GetParamName(nReason, nParam)) ; 

	  TabCtrl_InsertItem (pData->hwndTabCtl, nParam, &tie); 
	}

      if( FiltReason_GetParamCount(nReason)>0 )
	{
	  SetWindowPos (pData->hwndParam, HWND_TOP, 
		       0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
	  ShowWindow (pData->hwndParam, SW_SHOW) ;
	  nParam = TabCtrl_GetCurSel(pData->hwndTabCtl) ;
	  SendMessage (pData->hwndParam, WM_SETPARAMPTR, 0, (LPARAM)&pData->aParams[nParam]) ;
	}
      else
	ShowWindow (pData->hwndParam, SW_HIDE) ;

      if( FiltReason_GetOptionMask(nReason) & RULE_SCAN )
	{
	  EnableWindow (GetDlgItem(hDlg,IDC_SCAN), TRUE) ;
	  CheckDlgButton (hDlg, IDC_SCAN,
			  pData->pRule->nOptions&RULE_SCAN?
			  BST_CHECKED : BST_UNCHECKED) ;
	}
      else
	{
	  CheckDlgButton (hDlg, IDC_SCAN, BST_UNCHECKED) ;
	  EnableWindow (GetDlgItem(hDlg,IDC_SCAN), FALSE) ;
	}
     
      return TRUE ;
      

    case WM_NOTIFY:
      
      pnmh = (NMHDR*)lParam ;
      
      switch( pnmh->code )
	{
	case TCN_SELCHANGE:
	  
	  nParam = TabCtrl_GetCurSel(pData->hwndTabCtl) ;
	  SendMessage (pData->hwndParam, WM_SAVEPARAM, 0, 0) ;
	  SendMessage (pData->hwndParam, WM_SETPARAMPTR, 0, (LPARAM)&pData->aParams[nParam]) ;
	 
	  return TRUE ;
	}
      
      return FALSE ;
      
    case WM_COMMAND:
    
      switch( LOWORD(wParam) )
	{
	case IDC_REASON:
	  	  
	  if( HIWORD(wParam)==CBN_SELCHANGE )
	    SendMessage (hDlg, WM_UPDATE_TAB, 0, 0) ;	      

	  return TRUE ;

	case IDOK:

	  SendMessage (pData->hwndParam, WM_SAVEPARAM, 0, 0) ;
	  nReason = ComboBox_GetCurSel (pData->hwndReason) ;
	  nParams = FiltReason_GetParamCount(nReason) ;
	  FiltCond_Clear (&pData->pRule->condition) ;

	  _RuleDlg_SetCond (&pData->pRule->condition, nReason, nParams, pData->aParams) ;

	  // reaction
	  if( IsDlgButtonChecked(hDlg, IDC_ACCEPT)==BST_CHECKED )	pData->pRule->nReaction = RULE_ACCEPT ; 
	  if( IsDlgButtonChecked(hDlg, IDC_FEIGN)==BST_CHECKED )	pData->pRule->nReaction = RULE_FEIGN ;
	  if( IsDlgButtonChecked(hDlg, IDC_REJECT)==BST_CHECKED )	pData->pRule->nReaction = RULE_REJECT ;	  
	  // verbosity
	  if( IsDlgButtonChecked(hDlg, IDC_SILENT)==BST_CHECKED )	pData->pRule->nVerbosity = RULE_SILENT ;
	  if( IsDlgButtonChecked(hDlg, IDC_LOG)==BST_CHECKED )		pData->pRule->nVerbosity = RULE_LOG ;
	  if( IsDlgButtonChecked(hDlg, IDC_ALERT)==BST_CHECKED )	pData->pRule->nVerbosity = RULE_ALERT ;
	  // options
	  pData->pRule->nOptions = 0 ;
	  if( IsDlgButtonChecked(hDlg, IDC_ASK)==BST_CHECKED )		pData->pRule->nOptions |= RULE_ASK ;
	  if( IsDlgButtonChecked(hDlg, IDC_SCAN)==BST_CHECKED )		pData->pRule->nOptions |= RULE_SCAN ;     

	case IDCANCEL:

	  EndDialog (hDlg, LOWORD(wParam)) ;
	  return TRUE ;

	} // switch( LOWORD(wParam) )
      
      return FALSE ; // case WM_COMMAND:
    }

  return FALSE ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _RuleDlg_SetCond (FILTCOND * pCond, UINT nReason, UINT nParams, PARAM * pParams) 
{
  PARAM	* pInParam ;
  FILTPARAM * pOutParam ;
  UINT iParam ;
  TCHAR * szOutValue ;
  UINT	nOutValue ;
   
  // fill size field
  pCond->nReason = nReason ;
  pCond->nParams = nParams ;

  // for each param (begin)
  for( iParam=0 ; iParam<nParams ; iParam++ )
    {
      pInParam = &pParams[iParam] ;
      
      switch( pInParam->nType )
	{
	case FILTPARAM_ANY:

	  // get param struct address
	  pOutParam = &pCond->aParams[iParam] ;

	  // fill PARAM
	  pOutParam->nType	= FILTPARAM_ANY ;

	  break ;
	  

	case FILTPARAM_UINT:
	  
	  // read value
	  nOutValue = _ttoi (pInParam->szValue) ;

	  // get param struct address
	  pOutParam = &pCond->aParams[iParam] ;
	 
	  // fill PARAM
	  pOutParam->nType	= FILTPARAM_UINT ;
	  pOutParam->nValue	= nOutValue ;

	  break ;

	case FILTPARAM_STRING:
	case FILTPARAM_WILDCARDS:
	case FILTPARAM_PATH:

	  // get value
	  szOutValue = pInParam->szValue ;

	  // get param struct address
	  pOutParam = &pCond->aParams[iParam] ;

	  // fill PARAM
	  pOutParam->nType	= pInParam->nType ;
	  TRACE;
	  if( szOutValue ) {
	    pOutParam->szValue = malloc((_tcslen(szOutValue)+1)*sizeof(TCHAR)) ;
	    _tcscpy (pOutParam->szValue, szOutValue) ; 	  
	  }
	  else {
	    pOutParam->szValue = NULL ;	  
	  }

	  break ;
	  
	default:
	  ASSERT (!"[Invalid condition format]") ;
	}
    }
  
  return TRUE ;
}


