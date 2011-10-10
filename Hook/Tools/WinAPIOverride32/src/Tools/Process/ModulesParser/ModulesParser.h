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
// Object: Module Searching helper
//-----------------------------------------------------------------------------

#pragma once

#include <Windows.h>
#include <Tlhelp32.h>

typedef MODULEENTRY32 MODULEENTRY;
#define CreateToolhelpSnapshot CreateToolhelp32Snapshot
#define THCS_SNAPMODULE TH32CS_SNAPMODULE
#define ModuleFirst Module32First
#define ModuleNext Module32Next

// return TRUE to continue parsing, FALSE to stop it
typedef BOOL (pfModuleFoundCallback)(MODULEENTRY* pModuleEntry,PVOID UserParam);

class CModulesParser
{
public:
    CModulesParser(void);
    ~CModulesParser(void);
    static BOOL Parse(DWORD ProcessID,pfModuleFoundCallback ModuleFoundCallBack,PVOID UserParam);
    static BOOL Parse(DWORD ProcessID,HANDLE hCancelEvent,pfModuleFoundCallback ModuleFoundCallBack,PVOID UserParam);
};

