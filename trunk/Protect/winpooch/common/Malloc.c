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
#include "Malloc.h"

// Windows' headers
#include <ntddk.h>

// project's headers
#include "Strlcpy.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define MAX_FUNCTION_LENGTH		64
#define MAX_FUNCTION_COUNT		128


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  CHAR		szFunction[MAX_FUNCTION_LENGTH] ;
  ULONG		nGoodMallocCount ;
  ULONG		nBadMallocCount ;
  ULONG		nFreeCount ;
} FUNCTION ;

typedef struct {
#ifdef __NTDDK__
  KMUTEX	mutex ;
#else
  HANDLE	hMutex ;
#endif
  FUNCTION	aFunctions[MAX_FUNCTION_COUNT] ;
  UINT		nFunctions ;
} INTERNAL_DATA ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static INTERNAL_DATA g_data ;


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

void Malloc_Init () 
{
  TRACE ;

#ifdef __NTDDK__
  KeInitializeMutex (&g_data.mutex, 0) ;
#else
  g_data.hMutex = CreateMutex (NULL, FALSE, NULL) ;
#endif
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

void Malloc_Uninit () 
{
  TRACE ;

#ifndef __NTDDK__
  CloseHandle (g_data.hMutex) ;
#endif
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

void Malloc_PrintStats () 
{
#ifdef MALLOC_STATISTICS

  FUNCTION * p ;
  UINT i ;

  DbgPrint ("Memory allocation statistics\n") ;
  DbgPrint ("----------------------------\n") ;

  DbgPrint ("Function\tSuccessfull MALLOC\tUnsuccessful MALLOC\tFREE on non-null pointers\n") ;

  for( i=0 ; i<g_data.nFunctions ; i++ )
    {
      p = &g_data.aFunctions[i] ;

      DbgPrint (TEXT("%hs\t%lu\t%lu\t%lu\n"), 
		p->szFunction, p->nGoodMallocCount, 
		p->nBadMallocCount, p->nFreeCount) ;
    }
#endif
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

FUNCTION * _Malloc_FindFunctionStruct (LPCSTR szFunction)
{
  FUNCTION * p ;
  UINT i ;

  //
  // Look in the table
  //
  for( i=0 ; i<g_data.nFunctions ; i++ )
    {
      p = &g_data.aFunctions[i] ;

      if( ! strcmp(p->szFunction, szFunction) )
	return p ;
    }

  //
  // Add in the table
  //

  if( g_data.nFunctions >= MAX_FUNCTION_COUNT ) return NULL ;

  p = &g_data.aFunctions[g_data.nFunctions++] ;

  strlcpy (p->szFunction, szFunction, MAX_FUNCTION_LENGTH) ;

  return p ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

PVOID Malloc_Alloc (const char * szFunction, size_t nSize) 
{
  VOID * p ;
  FUNCTION * f ;

#ifdef __NTDDK__
  KeWaitForMutexObject (&g_data.mutex, Executive, KernelMode, FALSE, NULL) ;
#else
  WaitForSingleObject (g_fifo.hMutex, INFINITE) ;
#endif
  
  p = ExAllocatePool (PagedPool,nSize) ;
  f = _Malloc_FindFunctionStruct (szFunction) ;

  if( f )
    {
      if( p ) f->nGoodMallocCount++ ;
      else f->nBadMallocCount++ ;
    }

#ifdef __NTDDK__
  KeReleaseMutex (&g_data.mutex, FALSE) ;
#else
  ReleaseMutex (g_data.hMutex) ;
#endif

  return p ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

void Malloc_Free (const char * szFunction, void* p) 
{
  FUNCTION * f ;

  KeWaitForMutexObject (&g_data.mutex, Executive, KernelMode, FALSE, NULL) ;

  f = _Malloc_FindFunctionStruct (szFunction) ;
  
  if( p != NULL )
    {
      if( f ) f->nFreeCount++ ;
      ExFreePool(p) ;
    }

  KeReleaseMutex (&g_data.mutex, FALSE) ;
}
