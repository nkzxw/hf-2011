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

#ifndef _TRAYICON_H
#define _TRAYICON_H

#include <windows.h>

#define WM_TRAYICON	WM_USER+20

#define TIS_NORMAL	0
#define TIS_ALERT	1
#define TIS_ALERTED	2
#define TIS_DISABLED	3
//#define TIS_SCANNING	4


BOOL	TrayIcon_Create (HINSTANCE, HWND) ;

VOID	TrayIcon_Destroy () ;

BOOL	TrayIcon_ReloadConfig () ;

VOID	TrayIcon_LanguageChanged () ;

VOID	TrayIcon_RestoreIcon () ;

VOID	TrayIcon_SetState (UINT) ;

UINT	TrayIcon_GetState () ;

VOID	TrayIcon_Alert (LPCTSTR) ;

VOID	TrayIcon_TrackPopupMenu () ;

VOID	TrayIcon_PushState () ;

VOID	TrayIcon_PopState () ;

VOID	TrayIcon_EnableMenuItem (UINT, BOOL) ;

#endif
