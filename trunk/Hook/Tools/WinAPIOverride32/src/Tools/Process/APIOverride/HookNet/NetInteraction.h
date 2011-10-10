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
// Object: display .Net jitted function and allow to show generated asm opcodes
//-----------------------------------------------------------------------------

#pragma once

#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0501 // for xp os
#endif
#include <Windows.h>
#include "../../../Gui/ListView/Listview.h"
#include "../../../Gui/Dialog/DialogHelper.h"
#include "../../../Gui/Toolbar/Toolbar.h"
#include "../../../Disasm/Disasm.h"
#include "HookNet.h"
#include "DisplayAsm.h"
#include "resource.h"

#ifdef _WIN64
#ifdef _DEBUG
#pragma comment(lib, "../../../Disasm/distorm64/distorm_debug64.lib")
#else
#pragma comment(lib, "../../../Disasm/distorm64/distorm64.lib")
#endif
#else
#ifdef _DEBUG
#pragma comment(lib, "../../../Disasm/distorm64/distorm_debug.lib")
#else
#pragma comment(lib, "../../../Disasm/distorm64/distorm.lib")
#endif
#endif

#define CNetInteraction_DIALOG_MIN_WIDTH 200
#define CNetInteraction_DIALOG_MIN_HEIGHT 100

class CNetInteraction
{
public:
    CNetInteraction(void);
    ~CNetInteraction(void);
    static void Show();
private:
    enum tagListViewColumnIndex
    {
        ListViewColumnIndex_NAME,
        ListViewColumnIndex_TOKEN,
        ListViewColumnIndex_MODULE_NAME,
        ListViewColumnIndex_ASM_CODE_START,
        ListViewColumnIndex_ASM_CODE_LENGTH,
        ListViewColumnIndex_HOOKED
    };
    typedef struct tagDisasmInstructionCallBackParam
    {
        TCHAR* Buffer;
        DWORD BufferSize;
    }DISASM_INSTRUCTION_CALLBACK_PARAM,*PDISASM_INSTRUCTION_CALLBACK_PARAM;
    HINSTANCE hInstance;
    INT_PTR DialogResult;
    HWND hWndDialog;
    CListview* pListview;
    UINT MenuShowGeneratedAsmId;
    UINT MenuHookSelectedId;
    UINT MenuUnhookSelectedId;
    CLinkListItem* pItemDialog;
    CToolbar* pToolbar;

    BOOL UnhookSelected();
    BOOL HookSelected();
    void ShowGeneratedAssembly();
    void RefreshNetHookList();
    static void CallbackListViewMenuItemClickStatic(UINT MenuID,LPVOID UserParam);
    static BOOL DisasmInstructionCallBack(ULONGLONG InstructionAddress,DWORD InstructionsSize,TCHAR* InstructionHex,TCHAR* Mnemonic,TCHAR* Operands,PVOID UserParam);
    static DWORD WINAPI ShowGeneratedAsmThread(LPVOID UserParam);
    void CallbackListViewMenuItemClick(UINT MenuID);
    static DWORD WINAPI ModelessDialogThread2(PVOID lParam);
    static DWORD WINAPI ModelessDialogThread(PVOID lParam);

    static CNetInteraction* GetAssociatedDialogObject(HWND hWndDialog);
    void Init();
    void Close();
    void OnSize();
    void OnSizing(RECT* pRect);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
