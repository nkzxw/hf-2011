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

#ifndef _FILTERSET_H
#define _FILTERSET_H

#include "Filter.h"

typedef void* HFILTERSET ;

typedef BOOL (*ENUMFILTERSCALLBACK)(LPVOID pContext, HFILTER hFilter) ;


HFILTERSET	FilterSet_Create (int nFiltersMax) ;

VOID		FilterSet_Destroy (HFILTERSET) ;

BOOL		FilterSet_AddFilter (HFILTERSET hFilterSet, HFILTER hNewFilter) ;

// DEPRECATED
HFILTER		FilterSet_GetFilter (HFILTERSET hFilterSet, LPCWSTR szProcess) ;

// DEPRECATED
HFILTER		FilterSet_GetFilterStrict (HFILTERSET hFilterSet, LPCWSTR szProcess) ;

HFILTER		FilterSet_GetFilterByNum (HFILTERSET hFilterSet, UINT i) ;

HFILTER		FilterSet_GetDefaultFilter (HFILTERSET hFilterSet) ;

UINT		FilterSet_GetFilterCount (HFILTERSET hFilterSet) ;

VOID		FilterSet_Remove (HFILTERSET hFilterSet, HFILTER hFilter) ;

VOID		FilterSet_RemoveByNum (HFILTERSET hFilterSet, UINT iFilter) ;

BOOL		FilterSet_EnumFilters (HFILTERSET hFilterSet, ENUMFILTERSCALLBACK, LPVOID pContext) ;

// DEPRECATED
BOOL		FilterSet_TestPartialKey (HFILTERSET hFilterSet, LPCWSTR szPartialKey)  ;

UINT		FilterSet_Serialize	(HFILTERSET, PVOID pBuffer, UINT nMaxSize) ;

UINT		FilterSet_Unserialize	(HFILTERSET, PCVOID pBuffer, UINT nMaxSize) ;

#endif
