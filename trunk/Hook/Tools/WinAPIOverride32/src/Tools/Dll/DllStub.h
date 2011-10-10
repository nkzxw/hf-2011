/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2010 Jacquelin POTIER <jacquelin.potier@free.fr>

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

#pragma once
#include <Windows.h>
#include "../LinkList/LinkList.h"

class CDllStub
{
private:
    BOOL bOsHasNoStub;
    BOOL bError;
    BOOL Parse(TCHAR* ApiSetSchemaPath);
    BOOL FreeMemory();

public:

    typedef struct tagApiSetHeader
    {
        DWORD Unknown; // perhaps intended as a version number 
        DWORD NumberOfApiSetModules; // number of API Set modules
        // array of API_SET_MODULE_ENTRY --> API_SET_MODULE_ENTRY[NumberOfApiSetModules]
    }API_SET_HEADER,*PAPI_SET_HEADER;
    typedef struct tagApiSetModuleEntry
    {
        DWORD NameOffset;// offset from start of map to name of API Set module
        WORD NameSize; // size, in bytes, of name of API Set module
        DWORD HostModulesDescriptionOffset;// offset from start of map to description of host modules
    }API_SET_MODULE_ENTRY,*PAPI_SET_MODULE_ENTRY;

    typedef struct tagHostModulesDescription
    {
        DWORD NumberOfHosts; // number of hosts
        // array of entries for hosts, default host first, then exceptional hosts
    }HOST_MODULES_DESCRIPTION,*PHOST_MODULES_DESCRIPTION;

    typedef struct tagHostModuleEntry
    {
        DWORD ImportingModuleNameOffset;// offset from start of map to name of importing module, in Unicode 
        WORD ImportingModuleNameSize;// size, in bytes, of name of importing module
        DWORD HostModuleNameOffset;// offset from start of map to name of host module, in Unicode
        DWORD HostModuleNameSize;// size, in bytes, of name of host module 
    }HOST_MODULE_ENTRY,*PHOST_MODULE_ENTRY;

    typedef struct tagApiSetModuleEntryEx
    {
        API_SET_MODULE_ENTRY ModuleEntry;
        TCHAR Name[MAX_PATH];
        CLinkList* pHostModuleEntry; // list of HOST_MODULE_ENTRY_EX
    }API_SET_MODULE_ENTRY_EX,*PAPI_SET_MODULE_ENTRY_EX;

    typedef struct tagHostModuleEntryEx
    {
        HOST_MODULE_ENTRY HostModuleEntry;
        TCHAR ImportingModuleName[MAX_PATH];
        TCHAR HostModuleName[MAX_PATH];
    }HOST_MODULE_ENTRY_EX,*PHOST_MODULE_ENTRY_EX;

    CLinkList* pApiSetModule; // list of API_SET_MODULE_ENTRY_EX

    CDllStub(void);
    ~CDllStub(void);
    BOOL IsStubDll(IN TCHAR* DllName);
    API_SET_MODULE_ENTRY_EX* GetStubDllInfos(IN TCHAR* DllName);
    BOOL GetRealModuleNameFromStubName(IN TCHAR* CurrentFileName,IN TCHAR* StubDllName,OUT TCHAR* RealDllName);
};

