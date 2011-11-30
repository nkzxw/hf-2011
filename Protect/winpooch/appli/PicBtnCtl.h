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

#ifndef _PICBTNCTL_H
#define _PICBTNCTL_H

#include <windows.h>

#define WC_PICBTNCTL	TEXT("PicButton")

#define PBM_SETIMAGE		(WM_USER+1)
#define PBM_SETSELETED		(WM_USER+2)
#define PBM_SETBACKGROUND	(WM_USER+3)
#define PBM_INTERNAL_1		(WM_USER+4)


BOOL PicBtnCtl_RegisterClass	(HINSTANCE) ;

#define PicBtnCtl_SetImage(hwnd,szFile) \
  SendMessage ((HWND)hwnd, PBM_SETIMAGE, 0, (LPARAM)(LPCTSTR)szFile)

#define PicBtnCtl_SetSelected(hwnd,bSelected) \
  SendMessage ((HWND)hwnd, PBM_SETSELETED, 0, (LPARAM)(BOOL)bSelected)

#define PicBtnCtl_SetBackground(hwnd,hbrush) \
  SendMessage ((HWND)hwnd, PBM_SETBACKGROUND, 0, (LPARAM)(HBRUSH)hbrush)

#endif
