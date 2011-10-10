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
// Object: manages the break dialog
//-----------------------------------------------------------------------------

#pragma once

#include "../SupportedParameters.h"

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "resource.h"
#include "struct.h"
#include "../../../LinkList/SingleThreaded/LinkListSingleThreaded.h"

#define CBREAKUSERINTERFACE_RETURN_STRING_MAX_SIZE 200
class CBreakUserInterface
{
private:
    HWND hWndDialog;
    HFONT hFont;
    PAPI_INFO pAPIInfo;
    LOG_INFOS* pLogInfo;
    BOOL BeforeCall;
    PBYTE StackParametersPointer;
    PREGISTERS pRegisters;
    PBYTE ReturnAddress;
    PBYTE CallerEbp;
    double* pDoubleResult;
    INT_PTR Dlg_Res;
    CLinkListSingleThreaded* pLinkListTlsData;

    void Init(HWND hWnd);
    void Close();
    static CBreakUserInterface* GetAssociatedObject(HWND hWndDialog);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    static DWORD WINAPI ModelessDialogThread(PVOID lParam);
    BOOL IsThreadApiOverrideOne(DWORD ThreadId);

    
public:
    CBreakUserInterface(PAPI_INFO pAPIInfo,LOG_INFOS* pLogInfo,PBYTE StackParametersPointer,PREGISTERS pRegisters,double* pDoubleResult,PBYTE ReturnAddress,PBYTE EbpAtAPIHandler,BOOL BeforeCall,CLinkListSingleThreaded* pLinkListTlsData);
    ~CBreakUserInterface(void);
    INT_PTR ShowDialog();
};
