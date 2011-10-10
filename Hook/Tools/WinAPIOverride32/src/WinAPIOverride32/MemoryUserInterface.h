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
// Object: manages the memory dialog
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <tlhelp32.h>
#include "resource.h"
#include "../tools/String/StrToHex.h"
#include "../tools/APIError/ApiError.h"
#include "../tools/Process/memory/ProcessMemory.h"
#include "../tools/Process/ProcessHelper/ProcessHelper.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../Tools/String/StringConverter.h"

// number of bytes visible in the read text box
#define NB_BYTES_PER_LINE 16 // 4 DWORD

#pragma once

class CMemoryUserInterface
{
private:
    HWND hWndDialog;
    HFONT MemoryhFont;
    DWORD dwSystemPageSize;

    void Init();
    void Close();
    static DWORD WINAPI ModelessDialogThread(PVOID lParam);
    static CMemoryUserInterface* GetAssociatedObject(HWND hWndDialog);
    static LRESULT CALLBACK UserInterfaceWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
    CMemoryUserInterface(void);
    ~CMemoryUserInterface(void);
    static void Show(HINSTANCE hInstance,HWND ParentHandle);
    void MemoryAllocate();
    void MemoryFree();
    void MemoryRead();
    void MemoryWrite();
    void RefreshProcessList();
    BOOL CheckIfProcessIsAlive(DWORD dwProcessId);
    DWORD GetProcessId();
};
