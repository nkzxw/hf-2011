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
/* Build configuration                                            */
/******************************************************************/

#define	TRACE_LEVEL	2


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface 
#include "Hooks.h"

// project's headers
#include "Hook.h"
#include "HookReg.h"
#include "HookFile.h"
#include "HookSys.h"
#include "Link.h"
#include "Trace.h"
#include "NtUndoc.h"
#include "SystInfo.h"


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static HOOKSTRUCT g_aHooks[_HOOKS_FUNCTION_COUNT] ;
static BOOL g_aMask[_HOOKS_FUNCTION_COUNT] ;

VOID	* g_aStubs[_HOOKS_FUNCTION_COUNT] ;


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

NTSTATUS Hooks_Init ()
{
  int	i ;

  TRACE ;

  // set indirect pointer to stubs
  // this is used by JUMP_TO_STUB macro
  for( i=0 ; i<_HOOKS_FUNCTION_COUNT ; i++ )
    g_aStubs[i] = g_aHooks[i].pStub ;

  RtlZeroMemory (g_aHooks, sizeof(g_aHooks)) ;
  
  g_aMask[HOOKS_NTSETVALUEKEY] = TRUE ;
  g_aHooks[HOOKS_NTSETVALUEKEY].pTargetFunc = (VOID*)ntundoc.NtSetValueKey ;
  g_aHooks[HOOKS_NTSETVALUEKEY].pHookFunc = (VOID*)Hook_NtSetValueKey ;    

  g_aMask[HOOKS_NTQUERYVALUEKEY] = TRUE ;
  g_aHooks[HOOKS_NTQUERYVALUEKEY].pTargetFunc = (VOID*)ntundoc.NtQueryValueKey ;
  g_aHooks[HOOKS_NTQUERYVALUEKEY].pHookFunc = (VOID*)Hook_NtQueryValueKey ;    

  g_aMask[HOOKS_IOCREATEFILE] = TRUE ;
  g_aHooks[HOOKS_IOCREATEFILE].pTargetFunc = (VOID*)IoCreateFile ;
  g_aHooks[HOOKS_IOCREATEFILE].pHookFunc = (VOID*)Hook_IoCreateFile ;

  g_aMask[HOOKS_NTDELETEFILE] = TRUE ;
  g_aHooks[HOOKS_NTDELETEFILE].pTargetFunc = (VOID*)NtDeleteFile ;
  g_aHooks[HOOKS_NTDELETEFILE].pHookFunc = (VOID*)Hook_NtDeleteFile ;

  g_aMask[HOOKS_NTSETINFORMATIONFILE] = TRUE ;
  g_aHooks[HOOKS_NTSETINFORMATIONFILE].pTargetFunc = (VOID*)ntundoc.NtSetInformationFile ;
  g_aHooks[HOOKS_NTSETINFORMATIONFILE].pHookFunc = (VOID*)Hook_NtSetInformationFile ;

  g_aMask[HOOKS_NTDEVICEIOCONTROLFILE] = TRUE ; 
  g_aHooks[HOOKS_NTDEVICEIOCONTROLFILE].pTargetFunc = (VOID*)NtDeviceIoControlFile ;
  g_aHooks[HOOKS_NTDEVICEIOCONTROLFILE].pHookFunc = (VOID*)Hook_NtDeviceIoControlFile ;

  g_aMask[HOOKS_NTCREATESECTION] = TRUE ;
  g_aHooks[HOOKS_NTCREATESECTION].pTargetFunc = (VOID*)ntundoc.NtCreateSection ;
  g_aHooks[HOOKS_NTCREATESECTION].pHookFunc = (VOID*)Hook_NtCreateSection ;

  g_aMask[HOOKS_NTCREATEPROCESS] = ntundoc.NtCreateProcessEx==NULL ;
  g_aHooks[HOOKS_NTCREATEPROCESS].pTargetFunc = (VOID*)ntundoc.NtCreateProcess ;
  g_aHooks[HOOKS_NTCREATEPROCESS].pHookFunc = (VOID*)Hook_NtCreateProcess ;

  g_aMask[HOOKS_NTCREATEPROCESSEX] = ntundoc.NtCreateProcessEx!=NULL ;
  g_aHooks[HOOKS_NTCREATEPROCESSEX].pTargetFunc = (VOID*)ntundoc.NtCreateProcessEx ;
  g_aHooks[HOOKS_NTCREATEPROCESSEX].pHookFunc = (VOID*)Hook_NtCreateProcessEx ;

  g_aMask[HOOKS_NTCREATEUSERPROCESS] = ntundoc.NtCreateUserProcess!=NULL ;
  g_aHooks[HOOKS_NTCREATEUSERPROCESS].pTargetFunc = (VOID*)ntundoc.NtCreateUserProcess ;
  g_aHooks[HOOKS_NTCREATEUSERPROCESS].pHookFunc = (VOID*)Hook_NtCreateUserProcess ;

  g_aMask[HOOKS_NTTERMINATEPROCESS] = TRUE ;
  g_aHooks[HOOKS_NTTERMINATEPROCESS].pTargetFunc = (VOID*)ntundoc.NtTerminateProcess ;
  g_aHooks[HOOKS_NTTERMINATEPROCESS].pHookFunc = (VOID*)Hook_NtTerminateProcess ;

  g_aMask[HOOKS_PSPTERMINATEPROCESS] = ntundoc.PspTerminateProcess != NULL ;
  g_aHooks[HOOKS_PSPTERMINATEPROCESS].pTargetFunc = (VOID*)ntundoc.PspTerminateProcess ;
  g_aHooks[HOOKS_PSPTERMINATEPROCESS].pHookFunc = (VOID*)Hook_PspTerminateProcess ; 

  g_aMask[HOOKS_OBPFREEOBJECT] = TRUE ;
  g_aHooks[HOOKS_OBPFREEOBJECT].pTargetFunc = (VOID*)ntundoc.ObpFreeObject ;
  if( SystInfo_GetWindowsVersion() >= 0x0600 )
    g_aHooks[HOOKS_OBPFREEOBJECT].pHookFunc = (VOID*)Hook_ObpFreeObject_Vista ;
  else
    g_aHooks[HOOKS_OBPFREEOBJECT].pHookFunc = (VOID*)Hook_ObpFreeObject_Win2K ;
    
/*
  g_aHooks[HOOKS_OBFDEREFERENCEOBJECT].bEnabledHook = TRUE ;
  g_aHooks[HOOKS_OBFDEREFERENCEOBJECT].pTargetFunc = ntundoc.ObfDereferenceObject ;
  g_aHooks[HOOKS_OBFDEREFERENCEOBJECT].pHookFunc = Hook_ObfDereferenceObject ;

  g_aHooks[HOOKS_OBFASTDEREFERENCEOBJECT].bEnabledHook = TRUE ;
  g_aHooks[HOOKS_OBFASTDEREFERENCEOBJECT].pTargetFunc = ntundoc.ObFastDereferenceObject ;
  g_aHooks[HOOKS_OBFASTDEREFERENCEOBJECT].pHookFunc = Hook_ObFastDereferenceObject ;
*/
  
  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

NTSTATUS Hooks_Uninit ()
{
  TRACE ;
  
  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

NTSTATUS Hooks_InstallHooks () 
{
  int i ;

  TRACE ;
  
  for( i=0 ; i<_HOOKS_FUNCTION_COUNT ; i++ )
    if( g_aMask[i] &&  g_aHooks[i].pTargetFunc==NULL )
      {
	g_aMask[i] = FALSE ;
	TRACE_WARNING (TEXT("Hook %d has been disabled, target is NULL\n"), i) ;
      }

  for( i=0 ; i<_HOOKS_FUNCTION_COUNT ; i++ )
    if( g_aMask[i] )
      Hook_InstallHook (&g_aHooks[i]) ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

NTSTATUS Hooks_UninstallHooks () 
{
  LARGE_INTEGER	liSleepTime ;
  int i ;

  TRACE ;

  for( i=0 ; i<_HOOKS_FUNCTION_COUNT ; i++ )
    if( g_aMask[i] )
      Hook_UninstallHook (&g_aHooks[i]) ;
  
  // sleep so that hook will not be destroy while in use
  liSleepTime.QuadPart = - 2/*seconds*/ *1000*1000*10 ;
  KeDelayExecutionThread (KernelMode, TRUE, &liSleepTime) ;
  
  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

PROC Hooks_GetStubAddress (HOOKS_FUNCTION id) 
{
  ASSERT (id<_HOOKS_FUNCTION_COUNT) ;

  return (PROC)g_aHooks[id].pStub ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID* Hooks_GetStubAddForJmp (HOOKS_FUNCTION id) 
{
  ASSERT (id<_HOOKS_FUNCTION_COUNT) ;

  return &g_aStubs[id] ;
}
