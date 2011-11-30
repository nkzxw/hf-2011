/******************************************************************/
/*                                                                */
/*  Winpooch : Windows Watchdog                                   */
/*  Copyright (C) 2004-2005  Benoit Blanchon                      */
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

#ifndef _WILDCARDS_H
#define _WILDCARDS_H

#include "Types.h"


BOOL Wildcards_CompleteStringCmp (LPCWSTR szPattern, LPCWSTR szString) ;

BOOL Wildcards_PartialStringCmp (LPCWSTR szPattern, LPCWSTR szString) ;

BOOL Wildcards_CompletePathCmp (LPCWSTR szPattern, LPCWSTR szString) ;

BOOL Wildcards_PartialPathCmp (LPCWSTR szPattern, LPCWSTR szString) ;


BOOL Wildcards_GenericCmp (LPCWSTR szPattern, LPCWSTR szString, BOOL bPartial, BOOL bStopAtSlash) ;


#endif
