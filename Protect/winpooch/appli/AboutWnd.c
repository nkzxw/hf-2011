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
#include "AboutWnd.h"

// standard headers
#include <windows.h>
#include <tchar.h>
#include <richedit.h>
#include <shlwapi.h>

// project's headers
#include "Language.h"
#include "ProjectInfo.h"
#include "Resources.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define WC_ABOUTWND	TEXT("AboutWnd")

#define CX_BUTTON	150
#define CY_BUTTON	40
#define CX_SPACE	5
#define CY_SPACE	10

static TCHAR g_szVersion[] = 
  TEXT("Winpooch version %s, Copyright (C) 2004-2006 Benoit Blanchon") ;

static TCHAR g_szDrawings[] = 
  TEXT("All drawings has been realized by Sylvain Fajon") ;

static TCHAR g_szLicense[] =
  TEXT ("Winpooch comes with ABSOLUTELY NO WARRANTY.\n"
	"This is free software, and you are welcome to redistribute "
	"it under certain conditions.\nFor details, please read "
	"LICENCE text file provided with this software.") ;

static TCHAR g_szFreeImageLicense[] =
  TEXT("This software uses the FreeImage open source image library.\n"
       "See http://freeimage.sourceforge.net/ for details.\n"
       "FreeImage is used under the GNU GPL, version 2.") ;

static TCHAR g_szLibclamavLicense[] =
  TEXT("This software uses the Libclamav antivirus library.\n"
       "See http://www.clamav.net/ for details.\n"
       "Libclamav is used under the GNU GPL, version 2.") ;

static TCHAR g_szViewReadme[] = TEXT("View README file") ;

static TCHAR g_szViewChangelog[] = TEXT("View CHANGELOG file") ;

static TCHAR g_szViewFaq[] = TEXT("View FAQ file") ;

static TCHAR g_szViewLicense[] = TEXT("View LICENSE file") ;

static TCHAR g_szMakeDonation[] = TEXT("Make a donation") ;

static TCHAR g_szNotepadExe[] = TEXT("notepad.exe") ;

static TCHAR g_szLanguageDir[] = TEXT("languages") ;

static TCHAR g_szReadmeFileName[] = TEXT("README") ;

static TCHAR g_szChangeLogFileName[] = TEXT("CHANGELOG") ;

static TCHAR g_szFaqFileName[] = TEXT("FAQ") ;

static TCHAR g_szLicenseFileName[] = TEXT("LICENSE") ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

LRESULT CALLBACK _AboutWnd_WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) ;

void _AboutWnd_GetAbsolutePath (LPTSTR szPath, LPCTSTR szSubDir, LPCTSTR szFileName) ;


/******************************************************************/
/* Exported function : RegisterClass                              */
/******************************************************************/

BOOL AboutWnd_RegisterClass (HINSTANCE hInstance) 
{
  WNDCLASS wndclass ;

  LoadLibrary (TEXT("Riched20.dll")) ;

  wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
  wndclass.lpfnWndProc   = _AboutWnd_WndProc ;
  wndclass.cbClsExtra    = 0 ;
  wndclass.cbWndExtra    = 0 ;
  wndclass.hInstance     = hInstance ;
  wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION) ;
  wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
  wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1) ;
  wndclass.lpszMenuName  = NULL ;
  wndclass.lpszClassName = WC_ABOUTWND ;

  return 0!=RegisterClass (&wndclass) ;
}


/******************************************************************/
/* Exported function : CreateWindow                               */
/******************************************************************/

HWND AboutWnd_CreateWindow (HINSTANCE hInstance, HWND hwndParent)
{  
  return CreateWindowEx (WS_EX_CLIENTEDGE,
			 WC_ABOUTWND, NULL,
			 WS_CHILD,
			 CW_USEDEFAULT, CW_USEDEFAULT,
			 CW_USEDEFAULT, CW_USEDEFAULT, 
			 hwndParent, NULL, hInstance, NULL) ;
}


/******************************************************************/
/* Internal function : WndProc                                    */
/******************************************************************/

LRESULT CALLBACK _AboutWnd_WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  static HINSTANCE	g_hInstance ;
  static HWND		g_hwndRichEdit ;
  static HWND		g_hbtnReadme ;
  static HWND		g_hbtnChangelog ;
  static HWND		g_hbtnFaq ;
  static HWND		g_hbtnLicense ;
  static HWND		g_hbtnDonation ;
  TCHAR			szBuffer[1024] ;

  switch (message)
    {
    case WM_CREATE:

      g_hInstance = ((CREATESTRUCT*)lParam)->hInstance ;

      g_hbtnReadme = CreateWindow (WC_BUTTON, NULL,
				   WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_FLAT,
				   0, 0, 0, 0, hwnd, 
				   (HMENU)IDC_VIEW_README, 
				   g_hInstance, NULL) ;

      g_hbtnChangelog = CreateWindow (WC_BUTTON, NULL,
				      WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_FLAT,
				      0, 0, 0, 0, hwnd, 
				      (HMENU)IDC_VIEW_CHANGELOG, 
				      g_hInstance, NULL) ;

      g_hbtnFaq = CreateWindow (WC_BUTTON, NULL,
				WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_FLAT,
				0, 0, 0, 0, hwnd, 
				(HMENU)IDC_VIEW_FAQ, 
				g_hInstance, NULL) ;

      g_hbtnLicense = CreateWindow (WC_BUTTON, NULL,
				    WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_FLAT,
				    0, 0, 0, 0, hwnd, 
				    (HMENU)IDC_VIEW_LICENSE, 
				    g_hInstance, NULL) ;

      g_hbtnDonation = CreateWindow (WC_BUTTON, NULL,
				     WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_FLAT,
				     0, 0, 0, 0, hwnd, 
				     (HMENU)IDC_MAKE_DONATION, 
				     g_hInstance, NULL) ;
      
      g_hwndRichEdit = CreateWindow (RICHEDIT_CLASS, NULL,
				     WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_READONLY,
				     0, 0, 0, 0, hwnd, NULL, g_hInstance, NULL) ;

      SendMessage (g_hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0) ;
      SendMessage (g_hwndRichEdit, EM_SETEVENTMASK, 0, ENM_LINK) ;
      
    case WM_LANGUAGECHANGED:

      SetWindowText (g_hbtnReadme, STR_DEF(_VIEW_README,g_szViewReadme)) ;
      SetWindowText (g_hbtnChangelog, STR_DEF(_VIEW_CHANGELOG,g_szViewChangelog)) ;
      SetWindowText (g_hbtnFaq, STR_DEF(_VIEW_FAQ,g_szViewFaq)) ;
      SetWindowText (g_hbtnLicense, STR_DEF(_VIEW_LICENSE,g_szViewLicense)) ;
      SetWindowText (g_hbtnDonation, STR_DEF(_MAKE_DONATION,g_szMakeDonation)) ;

      SetWindowText (g_hwndRichEdit, TEXT("")) ;
      
      if( Language_IsLoaded() )
	wsprintf (szBuffer, STR(_VERSION_S), TEXT(APPLICATION_VERSION_STRING)) ;
      else
	wsprintf (szBuffer, g_szVersion, TEXT(APPLICATION_VERSION_STRING)) ;
      SendMessage (g_hwndRichEdit, EM_REPLACESEL, FALSE, (LPARAM)szBuffer) ;

      SendMessage (g_hwndRichEdit, EM_REPLACESEL, FALSE, (LPARAM)TEXT("\n\n")) ;
      _tcscpy (szBuffer, STR_DEF(_ABOUT_DRAWING,g_szDrawings)) ;
      SendMessage (g_hwndRichEdit, EM_REPLACESEL, FALSE, (LPARAM)szBuffer) ;
     
      if( Language_IsLoaded() ) {
	SendMessage (g_hwndRichEdit, EM_REPLACESEL, FALSE, (LPARAM)TEXT("\n\n")) ;
	_tcscpy (szBuffer, STR(_TRANSLATION_BY)) ;
	SendMessage (g_hwndRichEdit, EM_REPLACESEL, FALSE, (LPARAM)szBuffer) ;
      }

      _tcscpy (szBuffer, STR_DEF(_LICENSE, g_szLicense)) ;
      SendMessage (g_hwndRichEdit, EM_REPLACESEL, FALSE, (LPARAM)TEXT("\n\n")) ;
      SendMessage (g_hwndRichEdit, EM_REPLACESEL, FALSE, (LPARAM)szBuffer) ;

       _tcscpy (szBuffer, STR_DEF(_ABOUT_FREEIMAGE, g_szFreeImageLicense)) ;
      SendMessage (g_hwndRichEdit, EM_REPLACESEL, FALSE, (LPARAM)TEXT("\n\n")) ;
      SendMessage (g_hwndRichEdit, EM_REPLACESEL, FALSE, (LPARAM)szBuffer) ;   

       _tcscpy (szBuffer, STR_DEF(_ABOUT_LIBCLAMAV, g_szLibclamavLicense)) ;
      SendMessage (g_hwndRichEdit, EM_REPLACESEL, FALSE, (LPARAM)TEXT("\n\n")) ;
      SendMessage (g_hwndRichEdit, EM_REPLACESEL, FALSE, (LPARAM)szBuffer) ;     
 
      if( Language_IsLoaded() ) {
	_tcscpy (szBuffer, STR(_ABOUT_SITE)) ;
	SendMessage (g_hwndRichEdit, EM_REPLACESEL, FALSE, (LPARAM)TEXT("\n\n")) ;
	SendMessage (g_hwndRichEdit, EM_REPLACESEL, FALSE, (LPARAM)szBuffer) ;
      }

      return 0 ;


    case WM_SIZE:

      MoveWindow (g_hwndRichEdit, 
		  CX_SPACE, CY_SPACE, 
		  LOWORD(lParam)-3*CX_SPACE-CX_BUTTON, 
		  HIWORD(lParam)-2*CY_SPACE, TRUE) ;

      MoveWindow (g_hbtnReadme, 
		  LOWORD(lParam)-2*CX_SPACE-CX_BUTTON, CY_SPACE, 
		  CX_BUTTON, CY_BUTTON, TRUE) ;

      MoveWindow (g_hbtnChangelog, 
		  LOWORD(lParam)-2*CX_SPACE-CX_BUTTON, 
		  2*CY_SPACE+CY_BUTTON, 
		  CX_BUTTON, CY_BUTTON, TRUE) ;

      MoveWindow (g_hbtnFaq, 
		  LOWORD(lParam)-2*CX_SPACE-CX_BUTTON, 
		  3*CY_SPACE+2*CY_BUTTON, 
		  CX_BUTTON, CY_BUTTON, TRUE) ;

      MoveWindow (g_hbtnLicense, 
		  LOWORD(lParam)-2*CX_SPACE-CX_BUTTON, 
		  4*CY_SPACE+3*CY_BUTTON, 
		  CX_BUTTON, CY_BUTTON, TRUE) ;

      MoveWindow (g_hbtnDonation, 
		  LOWORD(lParam)-2*CX_SPACE-CX_BUTTON, 
		  6*CY_SPACE+5*CY_BUTTON, 
		  CX_BUTTON, CY_BUTTON, TRUE) ;
      
      return 0 ;

    case WM_NOTIFY:
      {
	union {
	  NMHDR		*pnmh ;
	  ENLINK	*plink ;
	} nmu ;

	nmu.pnmh = (NMHDR*) lParam ;
	
	switch( nmu.pnmh->code )
	  {
	  case EN_LINK: 
	    {
	      TCHAR	szBuffer[256] ;
	      TEXTRANGE tr ;

	      if( nmu.plink->msg!=WM_LBUTTONDOWN ) return 0 ;

	      tr.lpstrText = szBuffer ;
	      tr.chrg = nmu.plink->chrg ;

	      SendMessage (g_hwndRichEdit, EM_GETTEXTRANGE, 0, (LPARAM)&tr) ;
	      
	      ShellExecute (hwnd, NULL, szBuffer, NULL, NULL, SW_SHOW) ;
	    }
	    return 0 ;
	  }
      }
      return 0 ; // case WM_NOTIFY:

    case WM_COMMAND:

      switch( LOWORD(wParam) )
	{
	case IDC_VIEW_README:
	  _AboutWnd_GetAbsolutePath (szBuffer, NULL, g_szReadmeFileName) ;	  
	  ShellExecute (hwnd, NULL, g_szNotepadExe, szBuffer, NULL, SW_SHOW) ; 
	  return 0 ;

	case IDC_VIEW_CHANGELOG:
	  _AboutWnd_GetAbsolutePath (szBuffer, NULL, g_szChangeLogFileName) ;	  
	  ShellExecute (hwnd, NULL, g_szNotepadExe, szBuffer, NULL, SW_SHOW) ; 
	  return 0 ;

	case IDC_VIEW_FAQ:
	  _AboutWnd_GetAbsolutePath (szBuffer, NULL, g_szFaqFileName) ;	  
	  ShellExecute (hwnd, NULL, g_szNotepadExe, szBuffer, NULL, SW_SHOW) ; 
	  return 0 ;

	case IDC_VIEW_LICENSE:
	  if( Language_IsLoaded() )
	    _AboutWnd_GetAbsolutePath (szBuffer, g_szLanguageDir, STR(_LICENSE_FILE)) ;	  
	  else
	    _AboutWnd_GetAbsolutePath (szBuffer, NULL, g_szLicenseFileName) ;	  	    
	  ShellExecute (hwnd, NULL, g_szNotepadExe, szBuffer, NULL, SW_SHOW) ; 
	  return 0 ;

	case IDC_MAKE_DONATION:
	  ShellExecute (hwnd, NULL, TEXT(DONATION_PAGE), NULL, NULL, SW_SHOW) ; 
	  return 0 ;
	}

      break ; // case WM_COMMAND:
    }

  return DefWindowProc (hwnd, message, wParam, lParam) ;
}


/******************************************************************/
/* Internal function : GetAbsolutePath                            */
/******************************************************************/

void _AboutWnd_GetAbsolutePath (LPTSTR szPath, LPCTSTR szSubDir, LPCTSTR szFileName)
{
  GetModuleFileName (NULL, szPath, MAX_PATH) ;

  PathRemoveFileSpec (szPath) ;

  if( szSubDir )
    {
      PathAppend (szPath, szSubDir) ; 

      if( 0xFFFFFFFF==GetFileAttributes(szPath) )
	{
	  PathRemoveFileSpec (szPath) ; 
	  PathRemoveFileSpec (szPath) ; 
	  PathAppend (szPath, szSubDir) ; 
	}
    }
  
  PathAppend (szPath, szFileName) ;
}
