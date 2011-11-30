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

#ifndef _HOOKS_H
#define _HOOKS_H


#include <ntddk.h>

typedef enum {
  HOOKS_NTSETVALUEKEY,
  HOOKS_NTQUERYVALUEKEY,
  HOOKS_IOCREATEFILE,
  HOOKS_NTDELETEFILE,
  HOOKS_NTSETINFORMATIONFILE,
  HOOKS_NTDEVICEIOCONTROLFILE,
  HOOKS_NTCREATEPROCESS,
  HOOKS_NTCREATEPROCESSEX,
  HOOKS_NTCREATESECTION,
  HOOKS_NTCREATEUSERPROCESS,
  HOOKS_NTTERMINATEPROCESS,
  //HOOKS_OBFASTDEREFERENCEOBJECT,
  //HOOKS_OBFDEREFERENCEOBJECT,
  HOOKS_PSPTERMINATEPROCESS, 
  HOOKS_OBPFREEOBJECT,
  _HOOKS_FUNCTION_COUNT
} HOOKS_FUNCTION ;

NTSTATUS Hooks_Init () ;

NTSTATUS Hooks_Uninit () ;

NTSTATUS Hooks_InstallHooks () ;

NTSTATUS Hooks_UninstallHooks () ;

PROC	Hooks_GetStubAddress	(HOOKS_FUNCTION id) ;

LPVOID	Hooks_GetStubAddForJmp	(HOOKS_FUNCTION id) ;

#define JUMP_TO_STUB(iFunc)				\
  asm (/*"int $3\n\t"*/					\
       "movl %%ebp, %%esp\n\t"				\
       "pop %%ebp\n\t"					\
       "jmp *(%%eax)"					\
       :: "a" (Hooks_GetStubAddForJmp(iFunc))) ;


#endif
