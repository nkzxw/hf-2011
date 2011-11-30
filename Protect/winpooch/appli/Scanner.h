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

#ifndef _SCANNER_H
#define _SCANNER_H

#include <windows.h>
#include "ScanResult.h"

#define SCANNER_NONE		0
#define SCANNER_CLAMWIN		1
#define SCANNER_KASPERSKY_WS	2
#define SCANNER_BITDEFENDER	3
#define SCANNER_LIBCLAMAV	4

BOOL Scanner_Init () ;

VOID Scanner_Uninit () ;

BOOL Scanner_SetScanner (UINT) ;

UINT Scanner_GetScanner () ;

SCANRESULT Scanner_ScanFile (LPCTSTR szFile, LPTSTR szOutput, UINT nOutputMax, BOOL bBackground) ;

BOOL Scanner_IsConfigured () ;

BOOL Scanner_IsScanner (LPCTSTR szPath) ;

LPCTSTR Scanner_GetScannerExe () ;

BOOL Scanner_ReloadConfig () ;

#endif
