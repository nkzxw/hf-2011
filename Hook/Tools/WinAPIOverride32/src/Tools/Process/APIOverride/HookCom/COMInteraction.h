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
// Object: manages the Com Interaction dialog
//-----------------------------------------------------------------------------

#pragma once

#include "HookedClass.h"

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "resource.h"
#include "../../../Exception/HardwareException.h"
#include "../../../GUI/Dialog/DialogHelper.h"
#include "../../../GUI/TreeView/TreeView.h"
#include "../../../Com/GUIDStringConvert.h"
#include "../../../String/TrimString.h"
#include "../../../LinkList/LinkListSimple.h"
#include "../../../../WinAPIOverride32/ParseParametersForRemoteCall.h"
#include "../../../../WinAPIOverride32/RemoteCallResult.h"
#include "ShowPropertyPage.h"
#include "GetIDispatchWinApiOverrideFunctionsRepresentation.h"


#define CComInteraction_DISPLAYED_FUNCTION_NAME _T("%s 0x%p Index: %u")

class CComInteraction
{
private:
    HTREEITEM LastSelectedHookedObject;
    HTREEITEM LastSelectedObjectInterface;
    enum tagTreeviewDepth
    {
        DEPTH_CLASS_NAME,
        DEPTH_OBJECTS,
        DEPTH_OBJECTS_INTERFACE,
    };

    // parameter provide to ParseCOMMonitoringFileLine function
    typedef struct tagParseComInteractionCOMMonitoringFileLineParam
    {
        CComInteraction* pComInteraction;
        IUnknown* pInterfaceAssociatedToIID;
        IID*          pIid;
    }COMINTERACTION_PARSE_COM_MONITORING_FILE_LINE_PARAM;

    typedef struct tagFunctionInfoForCall
    {
        TCHAR* pszContent;
        BOOL bIDispatchStatic;// TRUE if function is from IDispatch parsed interface and declared as static
        PBYTE FunctionAddress;
    }FUNCTION_INFO_FOR_CALL,*PFUNCTION_INFO_FOR_CALL;

    CLinkList* pLinkListFunctionsInfosForCall; // list of FUNCTION_INFO_FOR_CALL struct. Associated to combo for selecting methods
    
    HWND hWndDialog;
    CTreeview* pTreeview;

    HWND hWndComboMethods;
    HWND hWndComboInterfaces;

    BOOL ParseInterfaceDefinitionFile(IID* pIid,
                                      IUnknown* pInterfaceAssociatedToIID,
                                      IID* pFileIid
                                      );
    static BOOL ParseCOMMonitoringFileLine(TCHAR* pszFileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam);

    CHookedObject::OBJECT_INFO_INTERFACE_POINTER* GetAssociatedObjectInterface();
    CHookedObject* GetAssociatedObject();
    CHookedClass* GetAssociatedHookedClass();
    BOOL IsObjectStillAlive();
    CHookedObject::OBJECT_INFO_INTERFACE_POINTER* GetSelectedInterfaceInCombo();
    FUNCTION_INFO_FOR_CALL* GetSelectedFunctionInCombo();
    void EnableFunctionNameSelection(BOOL bEnable);
    void DisplayHookedObjects();
    void ResetLastSelectedItem();
    void ClearFunctions();
    void ClearInterfaces();
    void SetObjectAddress(PBYTE Address);
    BOOL GetIID(TCHAR* pszIID,OUT IID* pIid);
    void AddInterfaceToComboInterface(IID* pIid);
    BOOL GetValue(int ControlID,DWORD* pValue);
    HTREEITEM AddObjectInterfaceAddressToTreeView(HTREEITEM hTreeObject,IID* pIid,PBYTE Address,LPARAM UserParam);
    static BOOL IDispatchFunctionsRepresentation(TCHAR* pszFuncDesc,CMethodInfo* pMethodInfo,LPVOID UserParam);
    void ClearpLinkListFunctionsInfosForCallContent();

    void OnShowPropertyPageClick();
    void OnSelectedInterfaceChange();
    void OnQueryInterfaceClick();
    void OnCallClick();
    void OnSaveClick();
    void OnEditIIDAssociatedFileClick();
    void OnTreeViewSelectionChanged();

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void Close();
    void Init(HWND hwnd);
    static DWORD WINAPI ModelessDialogThread(PVOID lParam);
    static DWORD WINAPI ModelessDialogThread2(PVOID lParam);
    CLinkListItem* pItemDialog;
public:
    static void Show(HINSTANCE Instance,HWND hWndParent);
    CComInteraction();
    ~CComInteraction();
};
