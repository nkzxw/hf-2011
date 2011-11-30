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


#ifndef _ASKDLG_H
#define _ASKDLG_H

#include <windows.h>

#include "FiltCond.h"


#define ASKDLG_ACCEPT		0 // = RULE_ACCEPT
#define ASKDLG_FEIGN		1 // = RULE_FEIGN
#define ASKDLG_REJECT		2 // = RULE_REJECT
#define ASKDLG_KILLPROCESS	3 // = RULE_KILLPROCESS
#define ASKDLG_CREATEFILTER	4
#define ASKDLG_UNHOOK		5

BOOL AskDlg_Init () ;

BOOL AskDlg_Uninit () ;

UINT AskDlg_DialogBox (HINSTANCE	hInstance,
		       HWND		hwndParent,
		       LPCTSTR		szProgram,
		       UINT		nProcessId,
		       UINT		nDefReaction,
		       PFILTCOND	pCondition) ;

#endif
