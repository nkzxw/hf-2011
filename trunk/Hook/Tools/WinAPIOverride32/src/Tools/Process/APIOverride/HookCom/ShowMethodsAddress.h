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
// Object: manages the Com ShowMethodsAddress dialog
//-----------------------------------------------------------------------------

#pragma once

#include "HookedClass.h"

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "resource.h"
#include "../../../GUI/Dialog/DialogHelper.h"
#include "../../../GUI/TreeView/TreeView.h"
#include "../../../Com/GUIDStringConvert.h"
#include "../../../String/TrimString.h"
#include "../../../Pe/pe.h"
#include "../../../LinkList/LinkListSimple.h"
#include "../../../Process/ModulesParser/ModulesParser.h"

#define CComShowMethodsAddress_DIALOG_MIN_WIDTH 500
#define CComShowMethodsAddress_DIALOG_MIN_HEIGHT 200
#define CComShowMethodsAddress_SPACE_BETWEEN_CONTROLS 5

class CIIDDisplayTreeInfo
{
public:
    HTREEITEM hTreeIID;
    IID Iid;
    CLinkListSimple* pLinkListDerivedIID;
    CIIDDisplayTreeInfo(HTREEITEM hTreeIID,IID* pIID)
    {
        this->hTreeIID=hTreeIID;
        this->Iid=*pIID;
        this->pLinkListDerivedIID=new CLinkListSimple();
    }
    ~CIIDDisplayTreeInfo()
    {
        CLinkListItem* pItem;
        this->pLinkListDerivedIID->Lock();
        // for each item of pLinkListDerivedIID
        for(pItem=this->pLinkListDerivedIID->Head;pItem;pItem=pItem->NextItem)
        {
            // look for an existing item
            delete ((CIIDDisplayTreeInfo*)pItem->ItemData);
        }
        this->pLinkListDerivedIID->RemoveAllItems(TRUE);
        this->pLinkListDerivedIID->Unlock();
        delete this->pLinkListDerivedIID;
    }
    CIIDDisplayTreeInfo* GetTreeItemForIID(IID *pIID)
    {
        CIIDDisplayTreeInfo* pIIDDisplayTreeInfo=NULL;
        CLinkListItem* pItem;

        this->pLinkListDerivedIID->Lock();
        // for each item of pLinkListDerivedIID
        for(pItem=this->pLinkListDerivedIID->Head;pItem;pItem=pItem->NextItem)
        {
            if (IsEqualIID(((CIIDDisplayTreeInfo*)pItem->ItemData)->Iid,*pIID))
            {
                pIIDDisplayTreeInfo=((CIIDDisplayTreeInfo*)pItem->ItemData);
                break;
            }
        }
        this->pLinkListDerivedIID->Unlock();

        return pIIDDisplayTreeInfo;
    }
    BOOL ExpandAllSubItems(CTreeview* pTreeView)
    {
        CIIDDisplayTreeInfo* pIIDDisplayTreeInfo=NULL;
        CLinkListItem* pItem;

        // expand current interface item
        pTreeView->Expand(this->hTreeIID);

        this->pLinkListDerivedIID->Lock();
        // for each item of pLinkListDerivedIID
        for(pItem=this->pLinkListDerivedIID->Head;pItem;pItem=pItem->NextItem)
        {
            pIIDDisplayTreeInfo=(CIIDDisplayTreeInfo*)pItem->ItemData;
            // expand interface item
            pTreeView->Expand(pIIDDisplayTreeInfo->hTreeIID);
            // expand all interface subitems
            pIIDDisplayTreeInfo->ExpandAllSubItems(pTreeView);
        }
        this->pLinkListDerivedIID->Unlock();
        return TRUE;
    }
};

class CComShowMethodsAddress
{
private:

    // parameter provide to ParseCOMMonitoringFileLine function
    typedef struct  tagParseShowMethodsAddressCOMMonitoringFileLineParam
    {
        CComShowMethodsAddress* pComShowMethodsAddress;
        CHookedClass* pHookedClass;
        CHookedObject* pHookedObject;
        IUnknown* pInterfaceAssociatedToIID;
        IID*          pIid;
        CLinkList* pLinkListOfBaseInterfacesID;
    }SHOWMETHODSADDRESS_PARSE_COM_MONITORING_FILE_LINE_PARAM;

    HWND hWndDialog;
    CTreeview* pTreeview;
    HMODULE hModule;
    PBYTE VTBLAddress;

    BOOL AddAddressToTreeView(HTREEITEM hTreeItem,PBYTE Address,CPE* pPe);
    HMODULE FindAssociatedModuleHandle(PBYTE VTBLAddress);
    void DisplayObjectMethodAddress(CHookedClass* pHookedClass,IUnknown* pInterfaceAssociatedToIID,IID* pIid);
    void ParseObjectMethodAddress(CLSID* pClsid,IID* pIid,BOOL bTryIDispatchParsing);
    BOOL ParseInterfaceDefinitionFile(CHookedClass* pHookedClass,
                                        IID* pIid,
                                        IUnknown* pInterfaceAssociatedToIID,
                                        CHookedObject* pHookedObject,
                                        IID* pFileIid,
                                        CLinkList* pLinkListOfBaseInterfacesID
                                        );
    static BOOL ParseCOMMonitoringFileLine(TCHAR* pszFileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam);
    static BOOL ModuleFoundCallback(MODULEENTRY* pModuleEntry,PVOID UserParam);
    BOOL GetIID(OUT IID* pIid);
    BOOL GetCLSID(OUT CLSID* pClsid);

    void OnSaveClick();
    void OnOkClick();
    void OnEditIIDAssociatedFileClick();
    void OnSize();
    void OnSizing(RECT* pWinRect);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void Close();
    void Init(HWND hwnd);
    static DWORD WINAPI ModelessDialogThread(PVOID lParam);
    CLinkListItem* pItemDialog;
public:
    static void Show(HINSTANCE Instance,HWND hWndParent);
    CComShowMethodsAddress();
    ~CComShowMethodsAddress();
};
