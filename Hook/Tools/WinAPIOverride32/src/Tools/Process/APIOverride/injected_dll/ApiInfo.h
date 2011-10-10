/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//-----------------------------------------------------------------------------
// Object: API_Info associated functions
//-----------------------------------------------------------------------------
#pragma once

#include "../SupportedParameters.h"

#include <windows.h>
#include "defines.h"
#include "struct.h"
#include "DynamicLoadedFuncs.h"
#include "APIOverrideKernel.h"
#include "modulefilters.h"

BOOL __stdcall InitializeApiInfo(API_INFO *pAPIInfo,TCHAR* ModuleName,TCHAR*FunctionName);

BOOL __stdcall FreeApiInfoItem(CLinkListItem* pItemAPIInfo);
void __stdcall ReleaseAndFreeApiInfo(CLinkListItem* pItemAPIInfo);
BOOL __stdcall FreeApiInfo(API_INFO* pAPIInfo);
void __stdcall FreeOptionalParametersMemory(API_INFO *pAPIInfo);
void __stdcall FreeOptionalParametersMemory(API_INFO *pAPIInfo,BYTE FromIndex,BYTE ToIndex);
BOOL __stdcall UnHookIfPossible(CLinkListItem* pItemAPIInfo,BOOL bRestoreOriginalBytes);
DWORD __stdcall GetStackSizeToRemove(API_INFO *pAPIInfo);