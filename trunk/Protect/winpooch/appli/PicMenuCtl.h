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

#ifndef _PICMENUCTL_H
#define _PICMENUCTL_H

#include <windows.h>

BOOL PicMenuCtl_RegisterClass	(HINSTANCE) ;

HWND PicMenuCtl_CreateWindow (HINSTANCE, HWND, UINT nStyle) ;

BOOL PicMenuCtl_AddItem (HWND hwndMenu, UINT nId, LPCTSTR szLabel, LPCTSTR szPicRsrc) ;

BOOL PicMenuCtl_SelectItem (HWND hwndMenu, UINT nId) ;

BOOL PicMenuCtl_SetItemText (HWND hwndMenu, UINT nId, LPCTSTR szLabel) ;

#endif
