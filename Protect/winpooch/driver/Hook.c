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

#define TRACE_LEVEL	2	// warning level


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "Hook.h"

// Windows' headers
#include <ntddk.h>
#include <ddk/ntapi.h>

// project's headers
#include "Disasm.h"
#include "NtUndoc.h"
#include "Trace.h"


#define GET_CR0(val)						\
  __asm__ __volatile__ ("movl %%cr0,%0" : "=r"(val)) ;

#define SET_CR0(val)						\
  __asm__ __volatile__ ("movl %0,%%cr0" : : "r"(val)) ;


NTSTATUS Hook_InstallHook (HOOKSTRUCT * pHook) 
{
  DWORD		dwJmpDst ;
  int		nInstSize, nInstSizeIfMoved ;
  int		i ;
  DWORD		nOldCr0, nNewCr0 ;
  BOOL		bSuccess ;
  
  ASSERT (pHook!=NULL) ;
  ASSERT (pHook->pTargetFunc!=NULL) ;

  //
  // Mesure head size and stub size
  // ------------------------------

  TRACE_INFO (TEXT("Measure head size (target=0x%08X)\n"), pHook->pTargetFunc) ;

  pHook->nHeadSize = 0 ;
  pHook->nStubSize = 5 ; // == sizeof(JMP rel32)

  while( pHook->nHeadSize < 5 ) // need at least 5 char to path
    {
      bSuccess = Dasm_GetInstructionSize (pHook->pTargetFunc+pHook->nHeadSize, 
					  &nInstSize, 
					  &nInstSizeIfMoved) ;
      
      if( ! bSuccess ) {
	TRACE_ERROR (TEXT("GetInstructionSize failed (target=0x%08X)\n"), pHook->pTargetFunc) ;
	return STATUS_UNSUCCESSFUL ;
      }
      
      pHook->nHeadSize += nInstSize ;
      pHook->nStubSize += nInstSizeIfMoved ;
      
      if( pHook->nStubSize >= MAX_STUB_SIZE ) {
	TRACE_ERROR (TEXT("Stub too big (target=0x%08X)\n"), pHook->pTargetFunc) ;
	return STATUS_UNSUCCESSFUL ;
      }      
    }

  TRACE_INFO (TEXT("Head size = %d\n"), pHook->nHeadSize) ;
  TRACE_INFO (TEXT("Stub size = %d\n"), pHook->nStubSize) ;


  //
  // Backup the function head
  // ------------------------

  TRACE_INFO (TEXT("Backup function head\n")) ;

  memcpy (pHook->pHead, pHook->pTargetFunc, pHook->nHeadSize) ;
  
 
  //
  // Create stub
  // -----------

  TRACE_INFO (TEXT("Create stub (stub=0x%08X)\n"), pHook->pStub) ;
  
  // create stub
  bSuccess = Dasm_MoveProgram (pHook->pTargetFunc, pHook->nHeadSize,
			       pHook->pStub, &pHook->nStubSize, MAX_STUB_SIZE) ;
  if( ! bSuccess ) 
    {
      TRACE_ERROR (TEXT("MoveProgram failed (target=0x%08X)\n"), pHook->pTargetFunc) ;
      return STATUS_UNSUCCESSFUL ;
    }
  
  // add jmp at the end of the stub
  dwJmpDst = (DWORD)pHook->pTargetFunc - (DWORD)pHook->pStub - pHook->nStubSize ;
  pHook->pStub[pHook->nStubSize] = 0xE9 ; // JMP rel32
  memcpy (&pHook->pStub[pHook->nStubSize+1], &dwJmpDst, 4) ; 
  pHook->nStubSize += 5 ;

  TRACE_INFO (TEXT("Stub created (stub=0x%08X, size=%u)\n"),
	      pHook->pStub, pHook->nStubSize) ;
  
  
  //
  // Remove target protection
  // ------------------------

  TRACE_INFO (TEXT("Removing write protection\n")) ;
  
  GET_CR0 (nOldCr0) ;
  nNewCr0 = nOldCr0 & ~0x10000 ;
  SET_CR0 (nNewCr0) ;

  //
  // Patch target
  // ------------

  TRACE_INFO (TEXT("Patching target\n")) ;

  // overwrite head with a jmp to spy function
  pHook->pTargetFunc[0] = 0xE9 ; // JMP rel32
  dwJmpDst = (DWORD)pHook->pHookFunc - (DWORD)pHook->pTargetFunc - 5 ; // 5==sizeof(JMP rel32)
  memcpy (pHook->pTargetFunc+1, &dwJmpDst, 4) ;
  
  // complete head with NOPs
  for( i=5 ; i<pHook->nHeadSize ; i++ )
    pHook->pTargetFunc[i] = 0x90 ; // NOP

  //
  // Restore target protection
  // -------------------------

  TRACE_INFO (TEXT("Restoring write protection\n")) ;
  SET_CR0 (nOldCr0) ;
   
  return STATUS_SUCCESS ;  
}


NTSTATUS Hook_UninstallHook (HOOKSTRUCT * pHook) 
{
  DWORD		nOldCr0, nNewCr0 ;

  TRACE ;

  ASSERT (pHook!=NULL) ;

  // verify that target is hooked
  if( pHook->nHeadSize<=0 || pHook->pTargetFunc[0]!=0xE9 ) 
    {
      TRACE_WARNING (TEXT("Function not hooked (pfnTarget=0x%08X)\n"), pHook->pTargetFunc) ;
      return STATUS_UNSUCCESSFUL ;
    }

  //
  // Remove target protection
  // ------------------------

  TRACE_INFO (TEXT("Removing write protection\n")) ;
  
  GET_CR0 (nOldCr0) ;
  nNewCr0 = nOldCr0 & ~0x10000 ;
  SET_CR0 (nNewCr0) ;

  //
  // Restoring original code
  // -----------------------

  TRACE_INFO (TEXT("Restoring original code (head=0x%08X, headsize=%u\n"), pHook->pHead, pHook->nHeadSize) ;
  
  // copy head from stub
  memcpy (pHook->pTargetFunc, pHook->pHead, pHook->nHeadSize) ;

  //
  // Restore target protection
  // -------------------------

  TRACE_INFO (TEXT("Restoring write protection\n")) ;
  SET_CR0 (nOldCr0) ;

  return STATUS_SUCCESS ;
}
