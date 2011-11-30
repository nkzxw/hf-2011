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
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "Strlcpy.h"

// standard headers
#include <string.h>

// project's headers
#include "Assert.h"
#include "Trace.h"


/******************************************************************/
/* Exported function : strlcpy                                    */
/******************************************************************/

size_t strlcpy (char *dst, const char *src, size_t size) 
{
  size_t n ;

  ASSERT (dst!=NULL) ;
  ASSERT (src!=NULL) ;

  n = strlen (src) ;

  if( n > size-1 ) {
    TRACE_WARNING (TEXT("String %hs will be truncated to %u chars\n"), src, size) ;
    n = size-1 ;
  }

  memcpy (dst, src, n*sizeof(char)) ;
  dst[n] = 0 ;

  return n ;
}


/******************************************************************/
/* Exported function : strlcpy                                    */
/******************************************************************/

size_t wcslcpy (wchar_t *dst, const wchar_t *src, size_t size) 
{
  size_t n ;

  ASSERT (dst!=NULL) ;
  ASSERT (src!=NULL) ;

  n = wcslen (src) ;

  if( n > size-1 ) {
    TRACE_WARNING (TEXT("String %ls will be truncated %u chars\n"), src, size) ;
    n = size-1 ;
  }

  memcpy (dst, src, n*sizeof(wchar_t)) ;
  dst[n] = 0 ;

  return n ;
}

