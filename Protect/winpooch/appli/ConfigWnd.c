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

#define	TRACE_LEVEL		2


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "ConfigWnd.h"

// standard headers
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>
#include <stdio.h>
#include <shlwapi.h>

// project's headers
#include "Assert.h"
#include "BkgndScan.h"
#include "Config.h"
#include "FilterDefault.h"
#include "FilterFile.h"
#include "Language.h"
#include "LogFile.h"
#include "ProjectInfo.h"
#include "Resources.h"
#include "Scanner.h"
#include "Sounds.h"
#include "SpyServer.h"
#include "Strlcpy.h"
#include "Trace.h"
#include "TrayIcon.h"


/******************************************************************/
/* Internal macros                                                */
/******************************************************************/

#define arraysize(a) (sizeof(a)/sizeof((a)[0]))

#define clear(a) memset(&(a),0,sizeof(a))

#define EnableDlgItem(hDlg,nItem,bEnable) \
  EnableWindow(GetDlgItem(hDlg,nItem),bEnable)

/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  UINT		idCtrl ;
  UINT		idCfgInt ;
  UINT		idStr ;
} CONFIG_CHECKBOX ;


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define MAX_FILTER	128

#define WM_UPDATECONFIG			(WM_USER+10)
#define WM_FOLDERS_LIST_CHANGED		(WM_USER+11)
#define WM_PATTERN_LIST_CHANGED		(WM_USER+12)

static CONFIG_CHECKBOX aCheckBoxes[] = {
  { IDC_SPLASH_SCREEN,		CFGINT_SPLASH_SCREEN },
  { IDC_CHECK_UPDATES,		CFGINT_CHECK_FOR_UPDATES },
  { IDC_TRAY_ICON_ANIMATION,	CFGINT_TRAY_ICON_ANIMATION } 
} ;

LPCTSTR g_szConfirmEraseFilters	= TEXT("You will lost all existing filters.\nAre-you sure ?") ;
LPCTSTR g_szAddFolder		= TEXT("Add folder") ;
LPCTSTR g_szRemoveFolder	= TEXT("Remove folder") ;
LPCTSTR g_szAddPattern		= TEXT("Add pattern") ;
LPCTSTR g_szRemovePattern	= TEXT("Remove pattern") ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

BOOL CALLBACK _ConfigWnd_DlgProc (HWND, UINT, WPARAM, LPARAM) ;

BOOL _ConfigWnd_Browse (HWND hDlg, BOOL bExport, LPTSTR szPath, UINT nMax) ;

BOOL _ConfigWnd_BrowseSound (HWND hDlg, LPTSTR szPath, UINT nMax) ;

VOID _ConfigWnd_ListToConfig (HWND hwndList, UINT nArrayId) ;

VOID _ConfigWnd_ConfigToList (HWND hwndList, UINT nArrayId) ;


/******************************************************************/
/* Exported function : RegisterClass                              */
/******************************************************************/

BOOL ConfigWnd_RegisterClass (HINSTANCE hInstance) 
{

  return TRUE ;
}


/******************************************************************/
/* Exported function : CreateWindow                               */
/******************************************************************/

HWND ConfigWnd_CreateWindow (HINSTANCE hInstance, HWND hwndParent)
{  
  return CreateDialog (hInstance, MAKEINTRESOURCE(DLG_CONFIG),
		       hwndParent, _ConfigWnd_DlgProc) ;
}


/******************************************************************/
/* Internal function : WndProc                                    */
/******************************************************************/

BOOL CALLBACK _ConfigWnd_DlgProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  int		i ;
  TCHAR		szBuffer[1024] ;
 
  static HMENU	g_hmenuFolders ;
  static HMENU	g_hmenuPatterns ;

  switch( message )
    {
    case WM_INITDIALOG:

      // set language list
      for( i=0 ; i<Language_GetLanguageCount() ; i++ )
	SendDlgItemMessage (hDlg, IDC_LANGUAGE, CB_ADDSTRING, 0,
			    (LPARAM)Language_GetLanguage(i)) ;

    case WM_LANGUAGECHANGED:

      // add antivirus
      SendDlgItemMessage (hDlg, IDC_ANTIVIRUS, CB_RESETCONTENT, 0, 0) ; 
      SendDlgItemMessage (hDlg, IDC_ANTIVIRUS, CB_ADDSTRING, 0,
			  (LPARAM) STR_DEF(_ANTIVIRUS_NONE,TEXT("None"))) ;
      SendDlgItemMessage (hDlg, IDC_ANTIVIRUS, CB_ADDSTRING, 0,
			  (LPARAM) STR_DEF(_ANTIVIRUS_CLAMWIN,TEXT("ClamWin"))) ;
      SendDlgItemMessage (hDlg, IDC_ANTIVIRUS, CB_ADDSTRING, 0,
			  (LPARAM) STR_DEF(_ANTIVIRUS_KASPERSKY_WS,
					   TEXT("Kaspersky antivirus for Workstation"))) ;
      SendDlgItemMessage (hDlg, IDC_ANTIVIRUS, CB_ADDSTRING, 0,
			  (LPARAM) STR_DEF(_ANTIVIRUS_BITDEFENDER,
					   TEXT("BitDefender"))) ;
      SendDlgItemMessage (hDlg, IDC_ANTIVIRUS, CB_ADDSTRING, 0,
			  (LPARAM) STR_DEF(_ANTIVIRUS_LIBCLAMAV,
					   TEXT("Libclamav"))) ;

      // create folder menu
      DestroyMenu (g_hmenuFolders) ;
      g_hmenuFolders = CreatePopupMenu () ;
      AppendMenu (g_hmenuFolders, MF_STRING, IDM_FOLDER_ADD, 
		  STR_DEF(_ADD_FOLDER,g_szAddFolder)) ;
      AppendMenu (g_hmenuFolders, MF_STRING, IDM_FOLDER_REMOVE,
		  STR_DEF(_REMOVE_FOLDER,g_szRemoveFolder)) ;

      // create pattern menu
      DestroyMenu (g_hmenuPatterns) ;
      g_hmenuPatterns = CreatePopupMenu () ;
      AppendMenu (g_hmenuPatterns, MF_STRING, IDM_PATTERN_ADD, 
		  STR_DEF(_ADD_PATTERN,g_szAddPattern)) ;
      AppendMenu (g_hmenuPatterns, MF_STRING, IDM_PATTERN_REMOVE,
		  STR_DEF(_REMOVE_PATTERN,g_szRemovePattern)) ;

      // set language dependant strings
      if( Language_IsLoaded() )
	{
	  SendDlgItemMessage (hDlg, IDC_LANGUAGE, CB_SELECTSTRING, 0,
			      (LPARAM)STR(_LANGUAGE_NAME_IN_ENGLISH)) ;   
	  SetDlgItemText (hDlg, IDT_CONFIGURATION, STR(_CONFIGURATION)) ;   
	  SetDlgItemText (hDlg, IDT_ANTIVIRUS, STR(_ANTIVIRUS)) ;
	  SetDlgItemText (hDlg, IDT_LANGUAGE, STR(_LANGUAGE)) ;
	  SetDlgItemText (hDlg, IDC_SPLASH_SCREEN, STR(_ENABLE_SPLASH_SCREEN)) ;
	  SetDlgItemText (hDlg, IDC_CHECK_UPDATES, STR(_CHECK_FOR_UPDATES)) ;
	  SetDlgItemText (hDlg, IDT_MAX_LOG_SIZE, STR(_MAX_LOG_FILE_SIZE)) ;
	  SetDlgItemText (hDlg, IDT_KILOBYTES, STR(_KILOBYTES)) ;
	  SetDlgItemText (hDlg, IDT_FILTERS, STR(_FILTERS)) ;
	  SetDlgItemText (hDlg, IDC_IMPORT, STR(_IMPORT)) ;
	  SetDlgItemText (hDlg, IDC_EXPORT, STR(_EXPORT)) ;
	  SetDlgItemText (hDlg, IDC_RESET, STR(_RESET)) ;
	  SetDlgItemText (hDlg, IDT_SOUNDS, STR(_SOUNDS)) ;
	  SetDlgItemText (hDlg, IDT_ALERT, STR(_ALERTING)) ;
	  SetDlgItemText (hDlg, IDT_ASK, STR(_ASKING)) ;
	  SetDlgItemText (hDlg, IDT_VIRUS, STR(_VIRUS)) ;
	  SetDlgItemText (hDlg, IDC_TRAY_ICON_ANIMATION, STR(_TRAY_ICON_ANIMATION)) ;
	  SetDlgItemText (hDlg, IDC_NO_SOUND, STR(_NO_SOUND)) ;
	  SetDlgItemText (hDlg, IDC_DEFAULT_SOUNDS, STR(_DEFAULT_SOUNDS)) ;
	  SetDlgItemText (hDlg, IDC_CUSTOM_SOUNDS, STR(_CUSTOM_SOUNDS)) ;
	  SetDlgItemText (hDlg, IDT_SCAN_PATTERNS, STR(_SCAN_ONLY_MATCHING_FILES)) ;
	  SetDlgItemText (hDlg, IDT_SCAN_FOLDERS, STR(_BACKGROUND_SCAN_THESE_FOLDERS)) ;
	}

    case WM_UPDATECONFIG:

      // antivirus
      SendDlgItemMessage (hDlg, IDC_ANTIVIRUS, CB_SETCURSEL, 
			  Scanner_GetScanner(), 0) ;

      // background scan folders
      _ConfigWnd_ConfigToList (GetDlgItem(hDlg,IDC_SCAN_FOLDERS), CFGSAR_SCAN_FOLDERS) ;

      // scan filters
      _ConfigWnd_ConfigToList (GetDlgItem(hDlg,IDC_SCAN_PATTERNS), CFGSAR_SCAN_PATTERNS) ;     

      // check boxes
      for( i=0 ; i<arraysize(aCheckBoxes) ; i++ )
	CheckDlgButton (hDlg, aCheckBoxes[i].idCtrl, 
			Config_GetInteger(aCheckBoxes[i].idCfgInt)
			? BST_CHECKED : BST_UNCHECKED) ; 

      CheckDlgButton (hDlg, IDC_NO_SOUND, Config_GetInteger(CFGINT_SOUND)==0 ? BST_CHECKED : BST_UNCHECKED) ;
      CheckDlgButton (hDlg, IDC_DEFAULT_SOUNDS, Config_GetInteger(CFGINT_SOUND)==1 ? BST_CHECKED : BST_UNCHECKED) ;
      CheckDlgButton (hDlg, IDC_CUSTOM_SOUNDS, Config_GetInteger(CFGINT_SOUND)==2 ? BST_CHECKED : BST_UNCHECKED) ;

      EnableDlgItem (hDlg, IDC_ALERT_SOUND, Config_GetInteger(CFGINT_SOUND)==2) ;
      EnableDlgItem (hDlg, IDC_ASK_SOUND, Config_GetInteger(CFGINT_SOUND)==2) ;
      EnableDlgItem (hDlg, IDC_VIRUS_SOUND, Config_GetInteger(CFGINT_SOUND)==2) ;
      EnableDlgItem (hDlg, IDC_BROWSE_ALERT_SOUND, Config_GetInteger(CFGINT_SOUND)==2) ;
      EnableDlgItem (hDlg, IDC_BROWSE_ASK_SOUND, Config_GetInteger(CFGINT_SOUND)==2) ;
      EnableDlgItem (hDlg, IDC_BROWSE_VIRUS_SOUND, Config_GetInteger(CFGINT_SOUND)==2) ;

      SetDlgItemText (hDlg, IDC_ALERT_SOUND, Config_GetString(CFGSTR_ALERT_SOUND)) ;
      SetDlgItemText (hDlg, IDC_ASK_SOUND, Config_GetString(CFGSTR_ASK_SOUND)) ;
      SetDlgItemText (hDlg, IDC_VIRUS_SOUND, Config_GetString(CFGSTR_VIRUS_SOUND)) ;     

      // max log size
      SetDlgItemInt (hDlg, IDC_MAX_LOG_SIZE,
		     Config_GetInteger(CFGINT_MAX_LOG_SIZE)>>10,
		     FALSE) ;
      
      return TRUE ;

    case WM_COMMAND:

     switch( wParam )
	{
	case MAKELONG(IDC_LANGUAGE,CBN_SELCHANGE):  //    ======== 'Language' selection changed ========
	  {
	    TCHAR szBuffer[64] ;

	    // language
	    GetDlgItemText (hDlg, IDC_LANGUAGE, szBuffer, 64) ;
	    Language_LoadByName (szBuffer) ;
	  }
	  return TRUE ; //    ======== 'Language' selection changed ========

	case MAKELONG(IDC_ANTIVIRUS,CBN_SELCHANGE): //    ======== 'Antivirus' selection changed ========
	  {
	    int nScanner = SendDlgItemMessage(hDlg,IDC_ANTIVIRUS,CB_GETCURSEL,0,0) ;

	    BkgndScan_Stop () ;

	    if( Scanner_SetScanner(nScanner) )
	      {
		BkgndScan_Start () ;
	      }
	    else
	      {
		
		MessageBox (hDlg, STR_DEF(_ANTIVIRUS_NOT_INSTALLED,
					  TEXT("The antivirus you selected isn't installed")),
			    TEXT(APPLICATION_NAME), MB_ICONERROR) ;
	      }
	  }	  
	  return TRUE ; //    ======== 'Antivirus' selection changed ========

	case MAKELONG(IDC_MAX_LOG_SIZE,EN_KILLFOCUS): //    ======== 'Max log size' changed ========
	  {
	    // max log size
	    UINT nSize = 1024 * GetDlgItemInt (hDlg, IDC_MAX_LOG_SIZE, NULL, FALSE) ;

	    Config_SetInteger (CFGINT_MAX_LOG_SIZE, nSize) ;
	    Config_SaveInteger (CFGINT_MAX_LOG_SIZE) ;

	    LogFile_ReloadConfig () ;	
	  }
	  return TRUE ; //    ======== 'Max log size' changed ========

	case MAKELONG(IDC_SPLASH_SCREEN,BN_CLICKED):
	  {
	    BOOL bEnable = IsDlgButtonChecked (hDlg, IDC_SPLASH_SCREEN) == BST_CHECKED ;

	    Config_SetInteger (CFGINT_SPLASH_SCREEN, bEnable) ;
	    Config_SaveInteger (CFGINT_SPLASH_SCREEN) ;
	  }
	  return TRUE ;

	case MAKELONG(IDC_CHECK_UPDATES,BN_CLICKED):
	  {
	    BOOL bEnable = IsDlgButtonChecked (hDlg, IDC_CHECK_UPDATES) == BST_CHECKED ;

	    Config_SetInteger (CFGINT_CHECK_FOR_UPDATES, bEnable) ;
	    Config_SaveInteger (CFGINT_CHECK_FOR_UPDATES) ;
	  }
	  return TRUE ;

	case MAKELONG(IDC_TRAY_ICON_ANIMATION,BN_CLICKED):
	  {
	    BOOL bEnable = IsDlgButtonChecked (hDlg, IDC_TRAY_ICON_ANIMATION) == BST_CHECKED ;

	    Config_SetInteger (CFGINT_TRAY_ICON_ANIMATION, bEnable) ;
	    Config_SaveInteger (CFGINT_TRAY_ICON_ANIMATION) ;

	    TrayIcon_ReloadConfig () ;
	  }
	  return TRUE ;

	case MAKELONG(IDC_ALERT_SOUND,EN_KILLFOCUS):
	  {
	    TCHAR szBuffer[MAX_PATH] ;

	    GetDlgItemText (hDlg, IDC_ALERT_SOUND, szBuffer, MAX_PATH) ;

	    Config_SetString (CFGSTR_ALERT_SOUND, szBuffer) ;
	    Config_SaveString (CFGSTR_ALERT_SOUND) ;

	    Sounds_ReloadConfig () ;
	  }
	  return TRUE ;

	case MAKELONG(IDC_ASK_SOUND,EN_KILLFOCUS):
	  {
	    TCHAR szBuffer[MAX_PATH] ;

	    GetDlgItemText (hDlg, IDC_ASK_SOUND, szBuffer, MAX_PATH) ;
	    Config_SetString (CFGSTR_ASK_SOUND, szBuffer) ;
	    Config_SaveString (CFGSTR_ASK_SOUND) ;

	    Sounds_ReloadConfig () ;
	  }
	  return TRUE ;

	case MAKELONG(IDC_VIRUS_SOUND,EN_KILLFOCUS):
	  {
	    TCHAR szBuffer[MAX_PATH] ;

	    GetDlgItemText (hDlg, IDC_VIRUS_SOUND, szBuffer, MAX_PATH) ;

	    Config_SetString (CFGSTR_VIRUS_SOUND, szBuffer) ;
	    Config_SaveString (CFGSTR_VIRUS_SOUND) ;

	    Sounds_ReloadConfig () ;
	  }
	  return TRUE ;

	case MAKELONG(IDC_BROWSE_ALERT_SOUND,BN_CLICKED):
	  {
	    TCHAR szBuffer[MAX_PATH] ;

	    _ConfigWnd_BrowseSound (hDlg, szBuffer, MAX_PATH) ;

	    SetDlgItemText (hDlg, IDC_ALERT_SOUND, szBuffer) ;

	    Config_SetString (CFGSTR_ALERT_SOUND, szBuffer) ;
	    Config_SaveString (CFGSTR_ALERT_SOUND) ;

	    Sounds_ReloadConfig () ;
	  }
	  return TRUE ;

	case MAKELONG(IDC_BROWSE_ASK_SOUND,BN_CLICKED):
	  {
	    TCHAR szBuffer[MAX_PATH] ;

	    _ConfigWnd_BrowseSound (hDlg, szBuffer, MAX_PATH) ;

	    SetDlgItemText (hDlg, IDC_ASK_SOUND, szBuffer) ;

	    Config_SetString (CFGSTR_ASK_SOUND, szBuffer) ;
	    Config_SaveString (CFGSTR_ASK_SOUND) ;

	    Sounds_ReloadConfig () ;
	  }
	  return TRUE ;

	case MAKELONG(IDC_BROWSE_VIRUS_SOUND,BN_CLICKED):
	  {
	    TCHAR szBuffer[MAX_PATH] ;

	    _ConfigWnd_BrowseSound (hDlg, szBuffer, MAX_PATH) ;

	    SetDlgItemText (hDlg, IDC_VIRUS_SOUND, szBuffer) ;

	    Config_SetString (CFGSTR_VIRUS_SOUND, szBuffer) ;
	    Config_SaveString (CFGSTR_VIRUS_SOUND) ;

	    Sounds_ReloadConfig () ;
	  }
	  return TRUE ;

	case MAKELONG(IDC_NO_SOUND,BN_CLICKED):
	  {
	    EnableDlgItem (hDlg, IDC_ALERT_SOUND, FALSE) ;
	    EnableDlgItem (hDlg, IDC_ASK_SOUND, FALSE) ;
	    EnableDlgItem (hDlg, IDC_VIRUS_SOUND, FALSE) ;
	    EnableDlgItem (hDlg, IDC_BROWSE_ALERT_SOUND, FALSE) ;
	    EnableDlgItem (hDlg, IDC_BROWSE_ASK_SOUND, FALSE) ;
	    EnableDlgItem (hDlg, IDC_BROWSE_VIRUS_SOUND, FALSE) ;

	    Config_SetInteger(CFGINT_SOUND, 0) ;
	    Config_SaveInteger(CFGINT_SOUND) ;

	    Sounds_ReloadConfig () ;
	  }
	  return TRUE ;

	case MAKELONG(IDC_DEFAULT_SOUNDS,BN_CLICKED):
	  {
	    EnableDlgItem (hDlg, IDC_ALERT_SOUND, FALSE) ;
	    EnableDlgItem (hDlg, IDC_ASK_SOUND, FALSE) ;
	    EnableDlgItem (hDlg, IDC_VIRUS_SOUND, FALSE) ;
	    EnableDlgItem (hDlg, IDC_BROWSE_ALERT_SOUND, FALSE) ;
	    EnableDlgItem (hDlg, IDC_BROWSE_ASK_SOUND, FALSE) ;
	    EnableDlgItem (hDlg, IDC_BROWSE_VIRUS_SOUND, FALSE) ;

	    Config_SetInteger(CFGINT_SOUND, 1) ;
	    Config_SaveInteger(CFGINT_SOUND) ;

	    Sounds_ReloadConfig () ;
	  }
	  return TRUE ;

	case MAKELONG(IDC_CUSTOM_SOUNDS,BN_CLICKED):
	  {
	    EnableDlgItem (hDlg, IDC_ALERT_SOUND, TRUE) ;
	    EnableDlgItem (hDlg, IDC_ASK_SOUND, TRUE) ;
	    EnableDlgItem (hDlg, IDC_VIRUS_SOUND, TRUE) ;
	    EnableDlgItem (hDlg, IDC_BROWSE_ALERT_SOUND, TRUE) ;
	    EnableDlgItem (hDlg, IDC_BROWSE_ASK_SOUND, TRUE) ;
	    EnableDlgItem (hDlg, IDC_BROWSE_VIRUS_SOUND, TRUE) ;

	    Config_SetInteger(CFGINT_SOUND, 2) ;
	    Config_SaveInteger(CFGINT_SOUND) ;

	    Sounds_ReloadConfig () ;
	  }
	  return TRUE ;

	case MAKELONG(IDC_EXPORT,BN_CLICKED): //    ======== 'Export' button clicked ========
	  {
	    TCHAR	szFileName[64] ;
	    int		i ;
	    BOOL	bSuccess ;
	    
	    if( _ConfigWnd_Browse (hDlg, TRUE, szBuffer, MAX_PATH) )
	      {
		SpySrv_LockFilterSet () ;
		bSuccess = FilterFile_Write (szBuffer, SpySrv_GetFilterSet()) ;
		SpySrv_UnlockFilterSet () ;

		if( ! bSuccess )
		  {		    
		    _tcslcpy (szFileName, PathFindFileName(szBuffer), 64) ;
		    
		    i = wsprintf (szBuffer, STR_DEF(_ERROR_WRITING_FILE_S,
						    TEXT("Error writing file \"%s\"")), szFileName) ;
		    
		    wsprintf (szBuffer+i, TEXT("\r\n%s"), FilterFile_GetErrorString()) ;
		    
		    MessageBox (hDlg, szBuffer, NULL, MB_ICONERROR) ;
		  }
	      }
	  }
	  return TRUE ;  //    ======== 'Export' button clicked ========

	case MAKELONG(IDC_IMPORT,BN_CLICKED): //    ======== 'Import' button clicked ========
	  {
	    HFILTERSET	hFilterSet ;
	    TCHAR	szFileName[64] ;
	    int		i ;
	    
	    if( _ConfigWnd_Browse (hDlg, FALSE, szBuffer, MAX_PATH) )
	      {		
		hFilterSet = FilterFile_Read (szBuffer) ;
		
		if( ! hFilterSet )
		  {		    
		    _tcslcpy (szFileName, PathFindFileName(szBuffer), 64) ;
		    
		    i = wsprintf (szBuffer, STR_DEF(_ERROR_READING_FILE_S,
						    TEXT("Error reading file \"%s\"")), szFileName) ;
		    
		    wsprintf (szBuffer+i, TEXT("\r\n%s"), FilterFile_GetErrorString()) ;
		    
		    MessageBox (hDlg, szBuffer, NULL, MB_ICONERROR) ;
		  }
		else
		  {
		    i = MessageBox(hDlg, 
				   STR_DEF(_CONFIRM_ERASE_FILTERS, g_szConfirmEraseFilters),
				   TEXT(APPLICATION_NAME),
				   MB_ICONWARNING|MB_YESNO) ;

		    if( i==IDYES )
		      {
			SpySrv_LockFilterSet () ;
			SpySrv_SetFilterSet (hFilterSet) ;
			SpySrv_SendFilterSetToDriver () ;
			SpySrv_UnlockFilterSet () ;
		      }
		    else
		      FilterSet_Destroy (hFilterSet) ;
		  }
	      }	    
	  }
	  return TRUE ; //    ======== 'Import' button clicked ========

	case MAKELONG(IDC_RESET,BN_CLICKED): //    ======== 'Reset' button clicked ========
	  {
	    HFILTERSET	hFilterSet ;
	    int		i ;
	    
	    hFilterSet = FilterSet_Create (64) ;
	    
	    if( hFilterSet )
	      {	
		FilterSet_InitDefaultFilter (hFilterSet) ;
	    
		i = MessageBox(hDlg, 
			       STR_DEF(_CONFIRM_ERASE_FILTERS, g_szConfirmEraseFilters),
			       TEXT(APPLICATION_NAME),
			       MB_ICONWARNING|MB_YESNO) ;
		
		if( i==IDYES )
		  {
		    SpySrv_LockFilterSet () ;
		    SpySrv_SetFilterSet (hFilterSet) ;
		    SpySrv_SendFilterSetToDriver () ;
		    SpySrv_UnlockFilterSet () ;
		  }
		else
		  FilterSet_Destroy (hFilterSet) ;
	      }	  	    
	  }
	  return TRUE ; //    ======== 'Reset' button clicked ========

	case IDM_FOLDER_ADD:
	  {
	    TCHAR szFolder[MAX_PATH] ;

	    ITEMIDLIST * pidl ; 

	    BROWSEINFO bi =
	      {
		.hwndOwner = hDlg,
	      } ;

	    pidl = SHBrowseForFolder (&bi) ;
	    
	    if( pidl && SHGetPathFromIDList(pidl, szFolder) )
	      {
		LVITEM	lvi = {0} ;
		HWND	hwndList = GetDlgItem (hDlg, IDC_SCAN_FOLDERS) ;

		lvi.iItem = ListView_GetItemCount(hwndList) ;
		lvi.mask = LVIF_TEXT ;
		lvi.pszText = szFolder ;
		ListView_InsertItem (hwndList, &lvi) ;
	      }
	    
	    PostMessage (hDlg, WM_FOLDERS_LIST_CHANGED, 0, 0) ;
	  }
	  return TRUE ; // IDM_FOLDER_ADD

	case IDM_FOLDER_REMOVE:
	  {
	    HWND hwndList = GetDlgItem (hDlg, IDC_SCAN_FOLDERS) ;

	    while( 1 )
	      {
		int i = ListView_GetNextItem (hwndList, -1, LVNI_SELECTED) ;
		  
		if( i<0 ) break ;

		ListView_DeleteItem (hwndList, i) ;
	      }

	    PostMessage (hDlg, WM_FOLDERS_LIST_CHANGED, 0, 0) ;
	  }
	  return TRUE ; // IDM_FOLDER_REMOVE

	case IDM_PATTERN_ADD:
	  {
	    LPTSTR	szPattern = TEXT("<pattern>") ;
	    LVITEM	lvi = {0} ;
	    HWND	hwndList = GetDlgItem (hDlg, IDC_SCAN_PATTERNS) ;
	    int		iItem = ListView_GetItemCount(hwndList) ;

	    lvi.iItem = iItem ;
	    lvi.mask = LVIF_TEXT ;
	    lvi.pszText = szPattern ;
	    ListView_InsertItem (hwndList, &lvi) ;

	    ListView_EditLabel (hwndList, iItem) ;
	  }
	  return TRUE ; // IDM_FOLDER_ADD

	case IDM_PATTERN_REMOVE:
	  {
	    HWND hwndList = GetDlgItem (hDlg, IDC_SCAN_PATTERNS) ;

	    while( 1 )
	      {
		int i = ListView_GetNextItem (hwndList, -1, LVNI_SELECTED) ;
		  
		if( i<0 ) break ;

		ListView_DeleteItem (hwndList, i) ;
	      }

	    PostMessage (hDlg, WM_PATTERN_LIST_CHANGED, 0, 0) ;
	  }
	  return TRUE ; // IDM_FOLDER_REMOVE
	}

     return FALSE ; // WM_COMMAND

    case WM_NOTIFY:
      {
	union
	{
	  NMHDR * hdr ;
	} pnm ;

	pnm.hdr = (NMHDR*) lParam ;

	switch( pnm.hdr->idFrom )
	  {
	  case IDC_SCAN_FOLDERS:
	    {
	      switch( pnm.hdr->code )
		{
		case NM_RCLICK:
		  {
		    POINT pt ;

		    if( ListView_GetSelectedCount(GetDlgItem(hDlg,IDC_SCAN_FOLDERS)) > 0 )
		      EnableMenuItem (g_hmenuFolders, IDM_FOLDER_REMOVE, MF_BYCOMMAND|MF_ENABLED) ;
		    else
		      EnableMenuItem (g_hmenuFolders, IDM_FOLDER_REMOVE, MF_BYCOMMAND|MF_GRAYED) ;

		    // get mouse position
		    GetCursorPos (&pt) ;
		    
		    // display menu
		    TrackPopupMenu (g_hmenuFolders, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hDlg, NULL) ;
		  }
		  return TRUE ; // NM_RCLICK

		case LVN_ENDLABELEDIT:
		  {
		    SetWindowLong (hDlg, DWL_MSGRESULT, TRUE) ;
		    
		    PostMessage (hDlg, WM_FOLDERS_LIST_CHANGED, 0, 0) ;
		  }
		  return TRUE ; // LVN_ENDLABELEDIT
		}
	    }
	    return TRUE ; // IDC_SCAN_FOLDERS

	  case IDC_SCAN_PATTERNS:
	    {
	      switch( pnm.hdr->code )
		{
		case NM_RCLICK:
		  {
		    POINT pt ;

		    if( ListView_GetSelectedCount(GetDlgItem(hDlg,IDC_SCAN_PATTERNS)) > 0 )
		      EnableMenuItem (g_hmenuPatterns, IDM_FOLDER_REMOVE, MF_BYCOMMAND|MF_ENABLED) ;
		    else
		      EnableMenuItem (g_hmenuPatterns, IDM_FOLDER_REMOVE, MF_BYCOMMAND|MF_GRAYED) ;

		    // get mouse position
		    GetCursorPos (&pt) ;
		    
		    // display menu
		    TrackPopupMenu (g_hmenuPatterns, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hDlg, NULL) ;
		  }
		  return TRUE ; // NM_RCLICK

		case LVN_ENDLABELEDIT:
		  {
		    SetWindowLong (hDlg, DWL_MSGRESULT, TRUE) ;
		    
		    PostMessage (hDlg, WM_PATTERN_LIST_CHANGED, 0, 0) ;
		  }
		  return TRUE ; // LVN_ENDLABELEDIT
		}
	    }
	    return TRUE ; // IDC_SCAN_PATTERNS
	  } 
      }
      return FALSE ; // WM_NOTIFY


    case WM_FOLDERS_LIST_CHANGED:
      {
	HWND hwndList = GetDlgItem (hDlg, IDC_SCAN_FOLDERS) ;

	_ConfigWnd_ListToConfig (hwndList, CFGSAR_SCAN_FOLDERS) ;
	
	BkgndScan_ReloadConfig () ;
      }
      return TRUE ;      

    case WM_PATTERN_LIST_CHANGED:
      {
	HWND hwndList = GetDlgItem (hDlg, IDC_SCAN_PATTERNS) ;
	LPCTSTR * pszFilters ;
	UINT nFilters ;

	_ConfigWnd_ListToConfig (hwndList, CFGSAR_SCAN_PATTERNS) ;
	pszFilters = Config_GetStringArray (CFGSAR_SCAN_PATTERNS, &nFilters) ;
	
	SpySrv_SetScanFilters (pszFilters, nFilters) ;
	BkgndScan_ReloadConfig () ;
	
      }
      return TRUE ;      
    }

  return FALSE ;
}


/******************************************************************/
/* Internal function : Browse                                     */
/******************************************************************/

BOOL _ConfigWnd_Browse (HWND hDlg, BOOL bExport, LPTSTR szPath, UINT nMax)
{
  TCHAR szFilter[MAX_FILTER] ;
  INT   iPos ;  
  
  OPENFILENAME ofn = {
    .lStructSize = sizeof(OPENFILENAME),
    .hwndOwner   = hDlg,
    .nMaxFile    = nMax,
    .lpstrFile   = szPath,
    .lpstrFilter = szFilter,
    .lpstrDefExt = TEXT("wpf"),
  } ;
  
  szPath[0] = 0 ;

  if( bExport )
    ofn.Flags = OFN_PATHMUSTEXIST ;
  else
    ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY ;
  
  iPos = 0 ;

  iPos += 1 + _sntprintf (szFilter+iPos, MAX_FILTER-iPos, TEXT("%s (*.wpf)"),
			  STR_DEF(_FILTER_FILES,TEXT("Winpooch filters"))) ;
  iPos += 1 + _sntprintf (szFilter+iPos, MAX_FILTER-iPos, TEXT("*.wpf")) ;

  iPos += 1 + _sntprintf (szFilter+iPos, MAX_FILTER-iPos, TEXT("%s (*.*)"),
			  STR_DEF(_ALL_FILES,TEXT("All files"))) ;
  iPos += 1 + _sntprintf (szFilter+iPos, MAX_FILTER-iPos, TEXT("*.*")) ;

  szFilter[iPos] = 0 ;
  
  if( bExport ) return GetSaveFileName (&ofn) ;
  return GetOpenFileName (&ofn) ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _ConfigWnd_BrowseSound (HWND hDlg, LPTSTR szPath, UINT nMax)
{
  TCHAR szFilter[MAX_FILTER] ;
  INT   iPos ;  
  
  OPENFILENAME ofn = {
    .lStructSize	= sizeof(OPENFILENAME),
    .hwndOwner		= hDlg,
    .nMaxFile		= nMax,
    .lpstrFile		= szPath,
    .lpstrFilter	= szFilter,
    .lpstrDefExt	= TEXT("wav"),
    .Flags		= OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY,
  } ;
  
  szPath[0] = 0 ;
  
  iPos = 0 ;

  iPos += 1 + _sntprintf (szFilter+iPos, MAX_FILTER-iPos, TEXT("%s (*.wav)"),
			  STR_DEF(_WAVE_FILES,TEXT("Wave files"))) ;
  iPos += 1 + _sntprintf (szFilter+iPos, MAX_FILTER-iPos, TEXT("*.wav")) ;

  iPos += 1 + _sntprintf (szFilter+iPos, MAX_FILTER-iPos, TEXT("%s (*.*)"),
			  STR_DEF(_ALL_FILES,TEXT("All files"))) ;
  iPos += 1 + _sntprintf (szFilter+iPos, MAX_FILTER-iPos, TEXT("*.*")) ;

  szFilter[iPos] = 0 ;
  
  return GetOpenFileName (&ofn) ;
}

/******************************************************************/
/* Internal function                                              */
/******************************************************************/

VOID _ConfigWnd_ConfigToList (HWND hwndList, UINT nArrayId) 
{
  UINT		i, nLength ;
  LPCTSTR	*pszArray ;
  LVITEM	lvi = {0} ;
  
  pszArray = Config_GetStringArray (nArrayId, &nLength) ;
  
  ASSERT (hwndList!=NULL) ;
  TRACE_INFO (TEXT("nLength = %d\n"), nLength) ;
  
  ListView_DeleteAllItems (hwndList) ;
  for( i=0 ; i<nLength ; i++ )
    {
      TRACE_INFO (TEXT("%d = %s\n"), i, pszArray[i]) ;
      lvi.iItem = i ;
      lvi.mask = LVIF_TEXT ;
      lvi.pszText = (LPTSTR)pszArray[i] ;
      ListView_InsertItem (hwndList, &lvi) ;
    }
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

VOID _ConfigWnd_ListToConfig (HWND hwndList, UINT nArrayId) 
{
  LPTSTR* pszArray ;
  UINT nLength ;
  UINT i ;

  nLength = ListView_GetItemCount(hwndList) ;

  pszArray = malloc (nLength*sizeof(LPCTSTR)) ;

  for( i=0 ; i<nLength ; i++ )
    {
      pszArray[i] = malloc (MAX_PATH*sizeof(TCHAR)) ; 
      
      ListView_GetItemText (hwndList, i, 0, pszArray[i], MAX_PATH) ;

      TRACE_INFO (TEXT("Item %d : %s\n"), i, pszArray[i]) ;
    }
  
  Config_SetStringArray (nArrayId, (LPCTSTR*)pszArray, nLength) ;
  Config_SaveStringArray (nArrayId) ;

  for( i=0 ; i<nLength ; i++ )
    free (pszArray[i]) ;
  free (pszArray) ;
}
