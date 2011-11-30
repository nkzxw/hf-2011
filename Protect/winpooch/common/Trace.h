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

/*
  Log levels :
  0 : nothing
  1 : errors (TRACE_ERROR)
  2 : warnings (TRACE_WARNING)
  3 : infos (TRACE_INFO)
  4 : traces (TRACE)

  If TRACE_LEVEL isn't defined, it's default to 4
*/

#include "Types.h"

#ifdef __NTDDK__

#define TRACE_HEADER TEXT("Winpooch.sys - ")

#define _BREAK DbgBreakPoint()
#else

#define TRACE_HEADER TEXT("Winpooch.exe + ")

VOID DbgPrint (LPCTSTR szFormat, ...) ;

#define _BREAK DebugBreak()

#endif

#ifndef TRACE_LEVEL
#define TRACE_LEVEL	4
#endif

#ifdef TRACE_IGNORE_LEVEL
#undef TRACE_LEVEL
#define TRACE_LEVEL	4
#endif	

#if TRACE_ALLOW_BREAK

   #define TRACE_BREAK(desc,...) 						\
    { DbgPrint (TRACE_HEADER TEXT("BREAK %hs:%d (in %hs) -> ") desc,__FILE__,__LINE__,__func__, ##__VA_ARGS__) ; _BREAK ; }

#else

  #define TRACE_BREAK TRACE_ERROR

#endif

#if TRACE_LEVEL>=1

  #define TRACE_ERROR(desc,...) 						\
    DbgPrint (TRACE_HEADER TEXT("ERROR %hs:%d (in %hs) -> ") desc,__FILE__,__LINE__,__func__, ##__VA_ARGS__)

  #define TRACE_ALWAYS(desc,...) 						\
    DbgPrint (TRACE_HEADER TEXT("%hs:%d (in %hs) -> ") desc,__FILE__,__LINE__,__func__, ##__VA_ARGS__)

#else

  #define TRACE_ERROR(...)	((void)0)

  #define TRACE_ALWAYS(...)	((void)0)

#endif


#if TRACE_LEVEL>=2

  #define TRACE_WARNING(desc,...)						\
    DbgPrint (TRACE_HEADER TEXT("WARNING %hs:%d (in %hs) -> ") desc,__FILE__,__LINE__,__func__, ##__VA_ARGS__)

#else

  #define TRACE_WARNING(...)	((void)0)

#endif
 

#if TRACE_LEVEL>=3

  #define TRACE_INFO(desc,...)						\
    DbgPrint (TRACE_HEADER TEXT("%hs:%d (in %hs) -> ") desc,__FILE__,__LINE__,__func__, ##__VA_ARGS__)

#else

  #define TRACE_INFO(...)	((void)0)

#endif 


#if TRACE_LEVEL>=4

  #define TRACE						\
    DbgPrint (TRACE_HEADER TEXT("%hs:%d (in %hs)\n"),__FILE__,__LINE__,__func__)

#else

  #define TRACE			((void)0)

#endif
