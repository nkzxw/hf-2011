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

#ifndef _HOOK_H
#define _HOOK_H

#include <ntddk.h>

#define MAX_STUB_SIZE	20
#define MAX_HEAD_SIZE	20

typedef struct {
  PBYTE pTargetFunc ;
  PBYTE pHookFunc ;
  BYTE	pStub[MAX_STUB_SIZE] ;
  UINT	nStubSize ;
  BYTE	pHead[MAX_HEAD_SIZE] ;
  UINT	nHeadSize ;
} HOOKSTRUCT ;

NTSTATUS Hook_InstallHook (HOOKSTRUCT*) ;

NTSTATUS Hook_UninstallHook (HOOKSTRUCT*)  ;
     

#endif
