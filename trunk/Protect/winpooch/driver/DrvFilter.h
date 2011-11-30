#ifndef _DRVFILTER_H
#define _DRVFILTER_H


#include "FilterSet.h"


NTSTATUS	DrvFilter_Init () ;

NTSTATUS	DrvFilter_Uninit () ;

HFILTERSET	DrvFilter_GetFilterSet () ;

VOID		DrvFilter_SetFilterSet (HFILTERSET) ;

NTSTATUS	DrvFilter_SetSerializedFilterSet (PCVOID, UINT) ;

NTSTATUS	DrvFilter_GetFiltersForProgram (IN LPCWSTR	wszPath,
						OUT HFILTER	*pFilters,
						OUT ULONG	*pnFilters,
						IN ULONG	nMaxFilters) ;

NTSTATUS	DrvFilter_LockMutex () ;

VOID		DrvFilter_UnlockMutex () ;

BOOL		DrvFilter_IsLocked () ;

#endif
