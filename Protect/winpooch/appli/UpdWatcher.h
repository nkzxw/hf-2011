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

#ifndef _UPDWATCHER_H
#define _UPDWATCHER_H

#include <windows.h>


#define WM_UPDATE_FOUND	(WM_USER+1)
// lParam = HUPDWATCHER


/**
 * Handle type.
 */
typedef LPVOID HUPDWATCHER ;

/**
 * Create a new update watcher.
 */
HUPDWATCHER UpdWatcher_New (HWND hwnd) ;

/**
 * Delete an existing update watcher.
 */
VOID UpdWatcher_Delete (HUPDWATCHER) ;

LPCTSTR UpdWatcher_GetNewVersion (HUPDWATCHER) ;

LPCTSTR UpdWatcher_GetDownloadPage (HUPDWATCHER) ;


#endif
