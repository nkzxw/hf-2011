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
// Object: manages the call stack dialog
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "resource.h"
#include "../../../LinkList/LinkListSimple.h"
#include "DynamicLoadedFuncs.h"
#include "APIOverrideKernel.h"
#include "Struct.h"
#include "../../../pe/PE.h"
#include "../../../LinkList/SingleThreaded/LinkListSingleThreaded.h"

#define MAX_SIZEOF_ONE_STACK_ITEM_REPRESENTATION 512

class CCallStackUserInterface
{

private:
    HWND hWndDialog;
    PBYTE CallerEbp;
    PBYTE ReturnAddress;
    CLinkListSingleThreaded* pLinkListTlsData;

    void Init(HWND hWnd);
    void Close();
    static CCallStackUserInterface* GetAssociatedObject(HWND hWndDialog);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL TryToGetDllFunctionName(CPE* pPe,PBYTE RelativeAddress, OUT TCHAR* pszFuncName,DWORD MaxpszFuncNameSize);
    BOOL TryToGetDllFunctionName(TCHAR* DllName,PBYTE RelativeAddress, OUT TCHAR* pszFuncName,DWORD MaxpszFuncNameSize);
public:
    CCallStackUserInterface(PBYTE CallerEbp,PBYTE ReturnAddress,CLinkListSingleThreaded* pLinkListTlsData);
    ~CCallStackUserInterface(void);
    INT_PTR ShowDialog(HWND ParentHandle);
};
