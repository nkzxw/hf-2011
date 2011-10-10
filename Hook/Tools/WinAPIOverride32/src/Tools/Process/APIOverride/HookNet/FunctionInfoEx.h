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
#pragma once

#include "FunctionInfo.h"
#include <corprof.h>

namespace NET
{

class CFunctionInfoEx: public CFunctionInfo
{
private:
    ICorProfilerInfo* pICorProfilerInfo;
public:
    CFunctionInfoEx(void);
    ~CFunctionInfoEx(void);

    TCHAR szModule[MAX_LENGTH];// could be private, but putting it public allows fast access
    PBYTE BaseAddress;// could be private, but putting it public allows fast access

    FunctionID FunctionId;
    ClassID ClassId;
    ModuleID ModuleId;
    LPBYTE AsmCodeStart;
    ULONG AsmCodeSize;
    BOOL GetModuleHandle(OUT PBYTE* pBaseAddress);
    BOOL GetModuleName(OUT TCHAR* pszModuleName,DWORD ModuleNameMaxLen);
    BOOL GetModuleNameAndHandle(OUT TCHAR* pszModuleName,DWORD ModuleNameMaxLen,OUT PBYTE* pBaseAddress);
    FunctionID GetFunctionId();
    ClassID GetClassId();
    BOOL GetILFunctionBody(OUT PBYTE* ppMethodHeader,OUT ULONG* pcbMethodSize);

    static CFunctionInfoEx* Parse(ICorProfilerInfo* pICorProfilerInfo,FunctionID functionID,BOOL bGetGeneratedCodeInformations);

    // pItemApiInfo : Hook informations
    // NULL if function is not hooked
    // CLinkListItem pointer of API_INFO struct associated to hook
    CLinkListItem* pItemApiInfo;
};

}