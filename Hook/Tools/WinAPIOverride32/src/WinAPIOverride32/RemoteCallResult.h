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
// Object: manages the Remote Call result dialog
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

// select "resource.h" according to project
#ifdef HOOKCOM_EXPORTS
    #include "../Tools/Process/APIOverride/HookCom/resource.h"
#else
    #include "resource.h"
#endif 

#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../tools/Process/APIOverride/ApiOverride.h"

#define CRemoteCallResult_DIALOG_MIN_WIDTH 300
#define CRemoteCallResult_DIALOG_MIN_HEIGHT 200
#define CRemoteCallResult_SPACE_BETWEEN_CONTROLS 5

class CRemoteCallResult
{
private:
#ifdef HOOKCOM_EXPORTS // if inside HookCom dll
    CLinkListItem* pItemDialog;
#endif

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void Close();
    void Init(HWND hwnd);
    void OnSize();
    void OnSizing(RECT* pWinRect);
    TCHAR* _tcsIncreaseIfNecessaryAndCat(TCHAR** ppsz1,TCHAR* psz2,size_t* p_psz1MaxLength);
    HFONT hFont;
    HWND hWndDialog;
    STRUCT_FUNC_PARAM* pParam;
    DWORD NbParams;
    TCHAR* pszDllName;
    TCHAR* pszFuncName;
    PBYTE RetValue;
    REGISTERS Registers;
    BOOL ShowRegisters;
    double FloatingResult;
public:
    void Show(HINSTANCE hInstance,HWND hWndParentDialog);
    CRemoteCallResult(TCHAR* pszDllName,TCHAR* pszFuncName,DWORD NbParams,STRUCT_FUNC_PARAM* pParam,REGISTERS* pRegisters,PBYTE RetValue,BOOL ShowRegisters,double FloatingResult);
    ~CRemoteCallResult();
};
