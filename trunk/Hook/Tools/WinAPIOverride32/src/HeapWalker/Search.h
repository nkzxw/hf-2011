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
// Object: manages the search dialog
//-----------------------------------------------------------------------------


#pragma once


#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "resource.h"
#include "Options.h"
#include "defines.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../tools/GUI/ListView/ListView.h"
#include "../Tools/String/StrToHex.h"

class CSearch
{
private:
    HWND hWndDialog;
    HINSTANCE hInstance;
    COptions* pOptions;
    CListview* pListview;
    BOOL SearchMatchCase;
    BOOL SearchUnicode;
    BOOL SearchAscii;
    BOOL SearchHex;
    TCHAR SearchContent[MAX_PATH];
    char AsciiSearchContent[MAX_PATH];
    DWORD AsciiSearchContentSize;
    wchar_t UnicodeSearchContent[MAX_PATH];
    DWORD UnicodeSearchContentSize;
    PBYTE HexData;
    DWORD HexDataSize;

    BOOL GetValue(int ControlID,DWORD* pValue);
    BOOL GetValue(int ControlID,TCHAR* pValue,DWORD MaxChar);
    void LoadOptions();
    void SaveOptions();
    void Find();
    void FindNext(int StartItemIndex);
    void FindPrevious(int StartItemIndex);
    BOOL UpdateSearchFields();
    void NoItemFoundMessage();
    BOOL DoesItemMatch(int ItemIndex);
    BOOL FindBufferInBuffer(PBYTE SearchedBuffer, DWORD SearchedBufferSize, PBYTE Buffer, DWORD BufferSize);
    void Close();
    void Init();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static DWORD WINAPI ModelessDialogThread(PVOID lParam);

public:
    CSearch(void);
    ~CSearch(void);
    static void Show(HINSTANCE hInstance,HWND hWndDialog,CListview* pListview,COptions* pOptions);
};
