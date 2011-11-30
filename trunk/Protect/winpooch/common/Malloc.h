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

#ifndef _MALLOC_H
#define _MALLOC_H

#ifdef __NTDDK__
  #include <ntddk.h>
#else
  #include <malloc.h>
#endif

#ifndef MALLOC_STATISTICS

#ifdef __NTDDK__
  #define MALLOC(n)	ExAllocatePool(PagedPool,n)
  #define FREE(p)	if(p)ExFreePool(p)
#else
  #define MALLOC(n)	malloc(n)
  #define FREE(p)	free(p)
#endif

#else

#define MALLOC(n)	Malloc_Alloc(__func__,n)
#define FREE(p)		Malloc_Free(__func__,p)

#endif


void Malloc_Init () ;

void Malloc_Uninit () ;

void Malloc_PrintStats () ;

void* Malloc_Alloc (const char * szFunction, size_t nSize) ;

void Malloc_Free (const char * szFunction, void * p) ;


#endif
