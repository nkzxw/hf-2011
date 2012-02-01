//////////////////////////////////////////////////////////////////////////////
//
//  Presence of this DLL (detoured.dll) marks a process as detoured.
//
//  Microsoft Research Detours Package.
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//

#include <windows.h>
#include "detoured.h"

static HMODULE s_hDll;

HMODULE WINAPI Detoured()
{
	if(s_hDll == 0)
	{
		MEMORY_BASIC_INFORMATION MemInfo;
		VirtualQuery(Detoured, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION));
		s_hDll = (HMODULE)MemInfo.BaseAddress;
	}
    return s_hDll;
}

//
///////////////////////////////////////////////////////////////// End of File.
