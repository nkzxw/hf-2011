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
#include "TrayIcon.h"

// standard headers
#include <tchar.h>

// project's headers
#include "Config.h"
#include "Language.h"
#include "ProjectInfo.h"
#include "Resources.h"
#include "Strlcpy.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

// timer intervals
#define IDLE_ANIM_LENGTH	5
#define ALERT_ANIM_LENGTH	2
//#define SCAN_ANIM_LENGTH	3
#define ANIM_INTERVAL		750
#define BALLOON_TIMEOUT		10000

// tray icon identifier
#define ID_ICON		1

#define STATE_STACK_SIZE	8


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  HWND		hwnd ;
  HMENU		hPopupMenu ;
  HICON		aIdleIcons[IDLE_ANIM_LENGTH] ;
  HICON		aAlertIcons[ALERT_ANIM_LENGTH] ;
  //  HICON		aScanIcons[SCAN_ANIM_LENGTH] ;
  int		iAnimTime ;
  UINT		nState ;
  UINT		nAnimTimer ;
  UINT		aStack[STATE_STACK_SIZE] ;
  UINT		nStackLevel ;
} INTERNALDATA ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static INTERNALDATA g_data ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

VOID CALLBACK _TrayIcon_AnimTimerProc (HWND, UINT, UINT, DWORD) ;

HICON _TrayIcon_GetCurIcon () ;
 

/******************************************************************/
/* Exported function : Create                                     */
/******************************************************************/

BOOL TrayIcon_Create (HINSTANCE hInstance, HWND hwnd) 
{
  NOTIFYICONDATA	nid ;
  int			i ;

  // save hwnd
  g_data.hwnd	= hwnd ;  

  // initial state : enabled
  g_data.nState = TIS_NORMAL ;   

  // clear state stack
  g_data.nStackLevel = 0 ;
  
  // create menu
  TrayIcon_LanguageChanged () ;

  // load icons
  for( i=0 ; i<IDLE_ANIM_LENGTH ; i++ )
    g_data.aIdleIcons[i] = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_TRAY_IDLE0+i)) ;
  for( i=0 ; i<ALERT_ANIM_LENGTH ; i++ )
    g_data.aAlertIcons[i] = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_TRAY_ALERT0+i)) ;   
  //for( i=0 ; i<SCAN_ANIM_LENGTH ; i++ )
  //g_data.aScanIcons[i] = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_TRAY_SCAN0+i)) ; 

  // add to tray
  nid.cbSize		= sizeof(nid) ;
  nid.hWnd		= g_data.hwnd ;
  nid.uID		= ID_ICON ;
  nid.uFlags		= NIF_ICON | NIF_MESSAGE | NIF_TIP ;
  nid.uCallbackMessage	= WM_TRAYICON ;
  nid.hIcon		= g_data.aIdleIcons[0] ;
  _tcscpy (nid.szTip, TEXT(APPLICATION_NAME " " APPLICATION_VERSION_STRING)) ;
  Shell_NotifyIcon (NIM_ADD, &nid) ;

  TrayIcon_ReloadConfig () ;

  return FALSE ;
}


/******************************************************************/
/* Exported function : Destroy                                    */
/******************************************************************/

VOID	TrayIcon_Destroy () 
{
  NOTIFYICONDATA	nid ;
  int			i ;

  // destroy anim timer
  if( g_data.nAnimTimer ) {
    KillTimer (NULL, g_data.nAnimTimer) ;
    g_data.nAnimTimer = 0 ;
  }

  // remove tray icon
  nid.cbSize	= sizeof(nid) ;
  nid.hWnd	= g_data.hwnd ;
  nid.uID	= ID_ICON ;
  nid.uFlags	= 0 ;
  Shell_NotifyIcon (NIM_DELETE, &nid) ;

  // destroy all icons
  for( i=0 ; i<IDLE_ANIM_LENGTH ; i++ )
    CloseHandle (g_data.aIdleIcons[i]) ;
  for( i=0 ; i<ALERT_ANIM_LENGTH ; i++ )
    CloseHandle (g_data.aAlertIcons[i]) ;
  //  for( i=0 ; i<SCAN_ANIM_LENGTH ; i++ )
  //    CloseHandle (g_data.aScanIcons[i]) ;

  // destroy context manu
  DestroyMenu (g_data.hPopupMenu) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL TrayIcon_ReloadConfig ()
{
  BOOL bAnimated = Config_GetInteger(CFGINT_TRAY_ICON_ANIMATION) ;

  if( bAnimated )
    {
      // create icon animation timer
      if( ! g_data.nAnimTimer )	
	g_data.nAnimTimer = SetTimer (NULL, 0, ANIM_INTERVAL, _TrayIcon_AnimTimerProc) ;  	
    }
  else
    {
      // destroy anim timer
      if( g_data.nAnimTimer )
	KillTimer (NULL, g_data.nAnimTimer) ;

      g_data.nAnimTimer = 0 ;
      g_data.iAnimTime = 0 ;
    }

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID TrayIcon_LanguageChanged ()
{
  if( ! g_data.hPopupMenu )
    {
      g_data.hPopupMenu = CreatePopupMenu () ;
      AppendMenu (g_data.hPopupMenu, MF_GRAYED|MF_STRING, IDC_STATIC, TEXT(APPLICATION_NAME)) ;
      AppendMenu (g_data.hPopupMenu, MF_SEPARATOR, 0, NULL) ;
      AppendMenu (g_data.hPopupMenu, MF_GRAYED|MF_STRING, IDM_OPEN, 
		  STR_DEF (_OPEN,TEXT("Open"))) ;
      AppendMenu (g_data.hPopupMenu, MF_GRAYED|MF_STRING, IDM_SHUTDOWN, 
		  STR_DEF (_SHUTDOWN,TEXT("Shutdown"))) ;
    }
  else
    {
      ModifyMenu (g_data.hPopupMenu, IDM_OPEN, MF_STRING|MF_BYCOMMAND, 
		  IDM_OPEN, STR_DEF(_OPEN,TEXT("Open"))) ;
      ModifyMenu (g_data.hPopupMenu, IDM_SHUTDOWN, MF_STRING|MF_BYCOMMAND, 
		  IDM_SHUTDOWN, STR_DEF(_SHUTDOWN,TEXT("Shutdown"))) ;
    }
}


VOID	TrayIcon_SetState (UINT nNewState) 
{
  NOTIFYICONDATA	nid ;

  g_data.nState = nNewState ;
  
  nid.cbSize		= sizeof(nid) ;
  nid.hWnd		= g_data.hwnd ;
  nid.uID		= ID_ICON ;
  nid.uFlags		= NIF_ICON ;
  nid.hIcon		= _TrayIcon_GetCurIcon () ;
  
  if( ! Shell_NotifyIcon (NIM_MODIFY, &nid) )
    {
      TrayIcon_RestoreIcon () ;
      Shell_NotifyIcon (NIM_MODIFY, &nid) ;
    }
}

VOID CALLBACK _TrayIcon_AnimTimerProc (HWND hwnd,UINT uMsg, UINT idEvent,DWORD dwTime) 
{
  NOTIFYICONDATA	nid ;

  g_data.iAnimTime++ ;

  nid.cbSize		= sizeof(nid) ;
  nid.hWnd		= g_data.hwnd ;
  nid.uID		= ID_ICON ;
  nid.uFlags		= NIF_ICON ;
  nid.hIcon		= _TrayIcon_GetCurIcon () ;
  
  if( ! Shell_NotifyIcon (NIM_MODIFY, &nid) )
    {
      TrayIcon_RestoreIcon () ;
      Shell_NotifyIcon (NIM_MODIFY, &nid) ;
    }
}

int _TrayIcon_GetAnimTime (int nAnimLen)
{
  int i ;
  
  i = g_data.iAnimTime % (nAnimLen*2-2) ;
  if( i>=nAnimLen ) i = nAnimLen-2 - i%nAnimLen ;

  return i ;
}

HICON _TrayIcon_GetCurIcon () 
{
  switch( g_data.nState )
    {
    case TIS_NORMAL:
      return g_data.aIdleIcons[_TrayIcon_GetAnimTime(IDLE_ANIM_LENGTH)] ;
    case TIS_ALERT:
    case TIS_ALERTED:
      return g_data.aAlertIcons[_TrayIcon_GetAnimTime(ALERT_ANIM_LENGTH)] ;
      //    case TIS_SCANNING:
      //      return g_data.aScanIcons[_TrayIcon_GetAnimTime(SCAN_ANIM_LENGTH)] ;
    }
  
  return NULL ;
}


VOID TrayIcon_RestoreIcon ()  
{
  NOTIFYICONDATA	nid ;

  nid.cbSize		= sizeof(NOTIFYICONDATA) ;
  nid.hWnd		= g_data.hwnd ;
  nid.uID		= ID_ICON ;
  nid.uFlags		= NIF_ICON | NIF_MESSAGE | NIF_TIP ;
  nid.uCallbackMessage	= WM_TRAYICON ;
  nid.hIcon		= _TrayIcon_GetCurIcon () ;
  _tcscpy (nid.szTip, TEXT(APPLICATION_NAME " " APPLICATION_VERSION_STRING)) ;

  Shell_NotifyIcon (NIM_ADD, &nid) ;    
}


VOID	TrayIcon_Alert (LPCTSTR szText) 
{
  NOTIFYICONDATA	nid ;
  
  g_data.nState = TIS_ALERT ;

  nid.cbSize		= sizeof(NOTIFYICONDATA) ;
  nid.hWnd		= g_data.hwnd ;
  nid.uID		= ID_ICON ;
  nid.uFlags		= NIF_INFO | NIF_ICON ;
  nid.dwInfoFlags	= NIIF_WARNING ;
  nid.uTimeout		= BALLOON_TIMEOUT ;
  nid.hIcon		= g_data.aAlertIcons[0] ;  
  
  // fill info text
  _tcslcpy (nid.szInfo, szText, 255) ;
  nid.szInfo[255] = 0 ;

  // fill info title
  wsprintf (nid.szInfoTitle, TEXT(APPLICATION_NAME " " APPLICATION_VERSION_STRING), 64) ;
	    
  if( ! Shell_NotifyIcon (NIM_MODIFY, &nid) )
    {
      TrayIcon_RestoreIcon () ;
      Shell_NotifyIcon (NIM_MODIFY, &nid) ;
    }
}

VOID TrayIcon_TrackPopupMenu ()
{
  POINT	pt ;

  GetCursorPos (&pt) ;

  SetForegroundWindow (g_data.hwnd) ;

  TrackPopupMenu (g_data.hPopupMenu, TPM_RIGHTALIGN, 
		  pt.x, pt.y, 0, g_data.hwnd, NULL) ;
}


VOID	TrayIcon_PushState () 
{
  if( g_data.nStackLevel < STATE_STACK_SIZE )
    g_data.aStack[g_data.nStackLevel++] = g_data.nState ;
}

VOID	TrayIcon_PopState () 
{
  if( g_data.nStackLevel > 0 )
    TrayIcon_SetState (g_data.aStack[--g_data.nStackLevel]) ;
}

VOID	TrayIcon_EnableMenuItem (UINT nItemId, BOOL bEnable) 
{
  EnableMenuItem (g_data.hPopupMenu, nItemId, bEnable ? MF_ENABLED : MF_GRAYED) ;
}
