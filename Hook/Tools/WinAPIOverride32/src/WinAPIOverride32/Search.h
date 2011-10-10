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
#include "StructsAndDefines.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../Tools/LinkList/LinkList.h"
#include "../tools/Process/APIOverride/ApiOverride.h"
#include "../tools/Process/APIOverride/InterProcessCommunication.h"
#include "../tools/GUI/ListView/ListView.h"
#include "../Tools/String/WildCharCompare.h"
#include "../Tools/String/TrimString.h"
#include "../Tools/String/StrToHex.h"
#include "../Tools/String/StringConverter.h"

#define CSEARCH_PARAM_SPLITTER _T("\r\n")

class CSearch
{
    friend class CLogsStatsUI;
private:
    typedef struct tagParameterSearchOption
    {
        DWORD ParamNumber;
        PBYTE ParamValue;
        DWORD ParamValueSize;
        DWORD PointedValueSize;
        PBYTE PointedValue;
    }PARAMETER_SEARCH_OPTION,*PPARAMETER_SEARCH_OPTION;

    HWND hWndDialog;
    HWND hWndParent;
    HINSTANCE hInstance;

    TCHAR ApiName[MAX_PATH];
    BOOL bCheckApiName;
    TCHAR ModuleName[MAX_PATH];
    BOOL bCheckModuleName;
    TCHAR CallingModuleName[MAX_PATH];
    BOOL bCheckCallingModuleName;

    PBYTE CallerAddress;
    BOOL bCheckCallerAddress;
    DWORD ProcessID;
    BOOL bCheckProcessID;
    DWORD ThreadID;
    BOOL bCheckThreadID;
    PBYTE Result;
    BOOL bCheckResult;

    double FloatingResult;
    BOOL bCheckFloatingResult;

    FILETIME StartMin;
    BOOL bCheckStartMin;
    FILETIME StartMax;
    BOOL bCheckStartMax;
    DWORD DurationMin;
    BOOL bCheckDurationMin;
    DWORD DurationMax;
    BOOL bCheckDurationMax;

    DWORD EaxBefore;
    BOOL bCheckEaxBefore;
    DWORD EbxBefore;
    BOOL bCheckEbxBefore;
    DWORD EcxBefore;
    BOOL bCheckEcxBefore;
    DWORD EdxBefore;
    BOOL bCheckEdxBefore;
    DWORD EsiBefore;
    BOOL bCheckEsiBefore;
    DWORD EdiBefore;
    BOOL bCheckEdiBefore;
    DWORD EflBefore;
    BOOL bCheckEflBefore;

    DWORD EaxAfter;
    BOOL bCheckEaxAfter;
    DWORD EbxAfter;
    BOOL bCheckEbxAfter;
    DWORD EcxAfter;
    BOOL bCheckEcxAfter;
    DWORD EdxAfter;
    BOOL bCheckEdxAfter;
    DWORD EsiAfter;
    BOOL bCheckEsiAfter;
    DWORD EdiAfter;
    BOOL bCheckEdiAfter;
    DWORD EflAfter;
    BOOL bCheckEflAfter;

    CLinkList* pParameters;
    BOOL bCheckParameters;
    BOOL ShouldMatchAllParamValues;

    BOOL UserInputFieldError;

    BOOL EncodeTime(TCHAR* String,FILETIME* pValue);
    BOOL GetValue(int ControlID,DWORD* pValue);
    BOOL GetValue(int ControlID,PBYTE* pValue);
    BOOL GetValue(int ControlID,double* pValue);
    BOOL GetValue(int ControlID,TCHAR* pValue,DWORD MaxChar);
    void Find();
    void FindNext(int StartItemIndex);
    void FindPrevious(int StartItemIndex);
    void SelectAll();

    void FindFromGUI();
    void FindNextFromGUI(int StartItemIndex);
    void FindPreviousFromGUI(int StartItemIndex);
    void SelectAllFromGUI();

    BOOL UpdateSearchFields();
    void NoItemFoundMessage();
    BOOL DoesItemMatch(int ItemIndex);
    void UnselectAll();
    void FreepParameters();
    BOOL ParseParamCondition(TCHAR* StringCondition,PARAMETER_SEARCH_OPTION* pParamOption);

    void Close();
    void Init();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static DWORD WINAPI ModelessDialogThread(PVOID lParam);
public:
    CSearch(void);
    ~CSearch(void);
    static void ShowDialog(HINSTANCE hInstance,HWND hWndDialog);
};
