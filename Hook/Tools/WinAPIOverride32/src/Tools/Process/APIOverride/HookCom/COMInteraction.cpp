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
// Object: manages the Com COM Interaction dialog
//-----------------------------------------------------------------------------

#include "ComInteraction.h"
extern HINSTANCE DllhInstance;
extern CLinkListSimple* pLinkListOpenDialogs;
extern HANDLE hSemaphoreOpenDialogs;
extern HOOK_COM_INIT HookComInfos;
extern CLinkListSimple* pLinkListHookedClasses;

//-----------------------------------------------------------------------------
// Name: ShowCOMInteraction
// Object: Display COM interaction dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall ShowCOMInteraction()
{
    CComInteraction::Show(DllhInstance,NULL);
    return TRUE;
}


CComInteraction::CComInteraction()
{
    this->pItemDialog=NULL;
    this->ResetLastSelectedItem();
    this->pLinkListFunctionsInfosForCall=new CLinkList(sizeof(FUNCTION_INFO_FOR_CALL));
}
CComInteraction::~CComInteraction()
{
    if (this->pLinkListFunctionsInfosForCall)
    {
        this->ClearpLinkListFunctionsInfosForCallContent();
        delete this->pLinkListFunctionsInfosForCall;
        this->pLinkListFunctionsInfosForCall=NULL;
    }
}

//-----------------------------------------------------------------------------
// Name: ResetLastSelectedItem
// Object: empty last selected informations
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::ResetLastSelectedItem()
{
    this->LastSelectedHookedObject=NULL;
    this->LastSelectedObjectInterface=NULL;
}

//-----------------------------------------------------------------------------
// Name: ClearpLinkListFunctionsInfosForCallContent
// Object: empty pLinkListFunctionsInfosForCall content
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::ClearpLinkListFunctionsInfosForCallContent()
{
    CLinkListItem* pItem;
    FUNCTION_INFO_FOR_CALL* pFunctionInfoForCall;
    this->pLinkListFunctionsInfosForCall->Lock();
    for (pItem=this->pLinkListFunctionsInfosForCall->Head;pItem;pItem=pItem->NextItem)
    {
        pFunctionInfoForCall=(FUNCTION_INFO_FOR_CALL*)pItem->ItemData;
        if (pFunctionInfoForCall->pszContent)
            free(pFunctionInfoForCall->pszContent);
    }
    this->pLinkListFunctionsInfosForCall->RemoveAllItems(TRUE);
    this->pLinkListFunctionsInfosForCall->Unlock();
}
//-----------------------------------------------------------------------------
// Name: ModelessDialogThread2
// Object: allow to act like a dialog box in modeless mode
// Parameters :
//     in  : PVOID lParam : HINSTANCE hInstance : application instance
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CComInteraction::ModelessDialogThread2(PVOID lParam)
{

    HDESK OldDesktop=NULL;
    HDESK CurrentDesktop=NULL;
    HWINSTA OldStation=NULL;
    HWINSTA CurrentStation=NULL;
    if (!HookComInfos.SetDefaultStation(&CurrentStation,&OldStation,&CurrentDesktop,&OldDesktop))
    {
        HookComInfos.DynamicMessageBoxInDefaultStation(NULL,_T("Error setting default station : can't display COM Interaction dialog"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        HookComInfos.RestoreStation(CurrentStation,OldStation,CurrentDesktop,OldDesktop);
        return 0;
    }

    CComInteraction* pComInteraction=new CComInteraction();

    DialogBoxParam ((HINSTANCE)lParam,(LPCTSTR)IDD_DIALOG_COM_INTERACTION,NULL,(DLGPROC)CComInteraction::WndProc,(LPARAM)pComInteraction);
    delete pComInteraction->pTreeview;
    delete pComInteraction;

    HookComInfos.RestoreStation(CurrentStation,OldStation,CurrentDesktop,OldDesktop);

        if (hSemaphoreOpenDialogs)
        ReleaseSemaphore(hSemaphoreOpenDialogs,1,NULL); // must be last instruction

    return 0;
}
//-----------------------------------------------------------------------------
// Name: ModelessDialogThread
// Object: allow to act like a dialog box in modeless mode
// Parameters :
//     in  : PVOID lParam : HINSTANCE hInstance : application instance
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CComInteraction::ModelessDialogThread(PVOID lParam)
{
    // as this dialog is shown in hooked module, we have to set window station and desktop
    // like for the Break dialog in Apioverride.dll
    // we have to use Apioverride.dll functions because they manages the count of displayed dialogs
    if (!HookComInfos.CanWindowInteract())
    {
        HookComInfos.DynamicMessageBoxInDefaultStation(
                    NULL,
                    _T("Process can't interact with user interface, so break window can't be displayed.\r\n")
                    _T("Your process is currently breaked, to allow you to do some operation\r\n")
                    _T("Click ok to resume it")
                    _T("\r\n\r\nNotice: To use break dialog with services, please refer to documentation,\r\n")
                    _T("in FAQ section: How to use break dialog with services ?"),
                    _T("Error"),
                    MB_OK|MB_ICONERROR|MB_TOPMOST);
        return 0;
    }

    CloseHandle(HookComInfos.AdjustThreadSecurityAndLaunchDialogThread(CComInteraction::ModelessDialogThread2,lParam));
    return 0;
}
//-----------------------------------------------------------------------------
// Name: Show
// Object: create a modeless compare dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::Show(HINSTANCE hInstance,HWND hWndDialog)
{
    // show dialog
    UNREFERENCED_PARAMETER(hWndDialog);
    // create thread instead of using CreateDialogParam to don't have to handle keyboard event like TAB
    CloseHandle(CreateThread(NULL,0,CComInteraction::ModelessDialogThread,hInstance,0,NULL));
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: init stats dialog
// Parameters :
//     in  : HWND hWndDialog : stat dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::Init(HWND hwnd)
{
    this->hWndDialog=hwnd;
    this->pItemDialog=pLinkListOpenDialogs->AddItem(hwnd);
    
    // create treeview object associated to control
    this->pTreeview=new CTreeview(GetDlgItem(this->hWndDialog,IDC_TREE_COM_INTERACTION));

    // get handle to combo for future use
    this->hWndComboInterfaces=GetDlgItem(this->hWndDialog,IDC_COMBO_INTERFACES);
    this->hWndComboMethods=GetDlgItem(this->hWndDialog,IDC_COMBO_METHODS);

    ////////////////////////////////////////////
    // stupid stuff for comdlg32 ver<6
    RECT Rect;
    CDialogHelper::GetClientWindowRect(this->hWndDialog,this->hWndComboInterfaces,&Rect);
    SetWindowPos(this->hWndComboInterfaces,HWND_NOTOPMOST,
                0,
                0,
                Rect.right-Rect.left,
                300,
                SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,this->hWndComboMethods,&Rect);
    SetWindowPos(this->hWndComboMethods,HWND_NOTOPMOST,
                0,
                0,
                Rect.right-Rect.left,
                300,
                SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    // end of stupid stuff for comdlg32 ver<6
    ////////////////////////////////////////////


    // check radio use function name by default
    SendDlgItemMessage(this->hWndDialog,IDC_RADIO_USE_FUNCTION_NAME,(UINT)BM_SETCHECK,BST_CHECKED,0);

    // get list of hooked object and display it
    this->DisplayHookedObjects();
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: Close stats dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::Close()
{
    // if pLinkListOpenDialogs is not locked by dll unload
    if (!hSemaphoreOpenDialogs)
        pLinkListOpenDialogs->RemoveItem(this->pItemDialog);
    
    EndDialog(this->hWndDialog,0);
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: stats dialog window proc
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CComInteraction::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // init dialog
        SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG)lParam);
        CDialogHelper::SetIcon(hWnd,IDI_ICON_SHOW_METHODS_ADDRESS);
        ((CComInteraction*)lParam)->Init(hWnd);
        break;
    case WM_CLOSE:
        {
            // close dialog
            CComInteraction* pComInteraction=(CComInteraction*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (pComInteraction)
                pComInteraction->Close();
        }
        break;
    case WM_COMMAND:
        {
            CComInteraction* pComInteraction=(CComInteraction*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (!pComInteraction)
                return FALSE;
            switch(LOWORD(wParam))
            {
            case IDOK:
                pComInteraction->OnCallClick();
                break;
            case IDC_BUTTON_EDIT_ASSOCIATED_IID_FILE:
                pComInteraction->OnEditIIDAssociatedFileClick();
                break;
            case IDCANCEL:
                pComInteraction->Close();
                break;
            case IDC_BUTTON_QUERY_NEW_INTERFACE:
                pComInteraction->OnQueryInterfaceClick();
                break;
            case IDC_BUTTON_SAVE:
                pComInteraction->OnSaveClick();
                break;
            case IDC_BUTTON_REFRESH:
                pComInteraction->DisplayHookedObjects();
                break;
            case IDC_BUTTON_PROPERTY_PAGE:
                pComInteraction->OnShowPropertyPageClick();
                break;
            case IDC_COMBO_INTERFACES:
                {
                    switch(HIWORD(wParam))
                    {
                    case CBN_SELCHANGE:
                        pComInteraction->OnSelectedInterfaceChange();
                        break;
                    }

                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR lpnmh = (LPNMHDR) lParam;
            if (lpnmh->idFrom==IDC_TREE_COM_INTERACTION)
            {
                switch (lpnmh->code)
                {
                case TVN_SELCHANGED:
                    {
                        CComInteraction* pComInteraction=(CComInteraction*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
                        if (!pComInteraction)
                            return FALSE;

                        pComInteraction->OnTreeViewSelectionChanged();
                    }
                    break;
                }
            }

        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: OnSaveClick
// Object: save button callback
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::OnSaveClick()
{
    this->pTreeview->Save(FALSE,FALSE,_T("html"));
}

//-----------------------------------------------------------------------------
// Name: OnEditIIDAssociatedFileClick
// Object: Edit IID AssociatedFile button callback
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::OnEditIIDAssociatedFileClick()
{
    TCHAR pszMonitoringFileName[MAX_PATH];
    CHookedObject::OBJECT_INFO_INTERFACE_POINTER* pObjectInterfacePointer=this->GetSelectedInterfaceInCombo();
    if (!pObjectInterfacePointer)
        return;
    if (!GetMonitoringFileName(&pObjectInterfacePointer->Iid,pszMonitoringFileName))
        return;

    // if no monitoring file exists
    if (!CStdFileOperations::DoesFileExists(pszMonitoringFileName))
    {
        if(!CreateInterfaceMonitoringFile(pszMonitoringFileName))
            return;
    }

    // launch default .txt editor
    if (((int)ShellExecute(NULL,_T("edit"),pszMonitoringFileName,NULL,NULL,SW_SHOWNORMAL))<33)
    {
        HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Error launching default editor application"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
    }
    else
        // force event to load changes
        this->OnSelectedInterfaceChange();
}

//-----------------------------------------------------------------------------
// Name: GetIID
// Object: get IID from string IID
// Parameters :
//     in  : TCHAR* pszIID : string IID
//     out : IID* pIid : interface identifier
//     return : 
//-----------------------------------------------------------------------------
BOOL CComInteraction::GetIID(TCHAR* pszIID, OUT IID* pIid)
{
    TCHAR pszMsg[MAX_PATH];
    
    BOOL bError=FALSE;

    // get IID
    CTrimString::TrimString(pszIID);
    if (*pszIID==0)
        return FALSE;
    else
    {
        if (*pszIID!='{') 
        {
            // if first char != '{' --> check for Interface name
            if(!CGUIDStringConvert::IIDFromInterfaceName(pszIID,pszIID))
            {
                bError=TRUE;
                _tcscpy(pszMsg,_T("Unknown Interface name"));
            }
        }
        // know pszIID contains IID
        if (!bError)// assume interface name was ok (in case of Interface name)
        {

            if(!CGUIDStringConvert::IIDFromTchar(pszIID,pIid))
            {
                bError=TRUE;
                _tcscpy(pszMsg,_T("Bad IID value"));
            }
        }
    }
    if (bError)
    {
        HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: OnQueryInterfaceClick
// Object: QueryInterface button click callbacl
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::OnQueryInterfaceClick()
{
    // get current object
    CHookedObject* pHookedObject=this->GetAssociatedObject();
    if (!pHookedObject)
        return;

    IID Iid;
    IUnknown* pNewInterface;
    TCHAR pszIID[MAX_PATH];
    GetDlgItemText(this->hWndDialog,IDC_EDIT_QUERY_NEW_INTERFACE,pszIID,MAX_PATH);

    // get interface ID
    if (!this->GetIID(pszIID,&Iid))
        return;

    // assume interface doesn't already belong to object
    if (pHookedObject->GetInterfacePointer(&Iid))
    {
        HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("This interface already belongs to object"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
        return;
    }

    // query interface
    pNewInterface=NULL;
    if (FAILED(CSecureIUnknown::QueryInterface(pHookedObject->pIUnknown,Iid,(void**)&pNewInterface)))
    {
        HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("QueryInterface call fails"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }
    if (IsBadReadPtr(pNewInterface,sizeof(IUnknown)))
        return;

    // restore object ref count
    CSecureIUnknown::Release(pNewInterface);

    // add new interface to treeview
    this->AddObjectInterfaceAddressToTreeView(this->LastSelectedHookedObject,&Iid,(PBYTE)pNewInterface,(LPARAM)(pHookedObject->GetLinkListInterfacePointersItem(&Iid,FALSE)));

    // add new interface to combo interface
    this->AddInterfaceToComboInterface(&Iid);

    // select last element of combo
    // get number of item in combo
    LRESULT ItemCount=SendMessage(this->hWndComboInterfaces, (UINT) CB_GETCOUNT,0,0);
    if (ItemCount!=CB_ERR)
    {
        // select last element of combo
        SendMessage(this->hWndComboInterfaces, (UINT) CB_SETCURSEL,ItemCount-1,0);

        // force interface change event
        this->OnSelectedInterfaceChange();
    }
    
}

//-----------------------------------------------------------------------------
// Name: AddInterfaceToComboInterface
// Object: add interface to combo containing interface
// Parameters :
//     in  : IID* pIid : interface id
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::AddInterfaceToComboInterface(IID* pIid)
{
    TCHAR pszInterface[MAX_PATH];
    TCHAR pszIID[2*MAX_PATH];
    if (!CGUIDStringConvert::TcharFromIID(pIid,pszIID))
        return;
    if (CGUIDStringConvert::GetInterfaceName(pszIID,pszInterface,MAX_PATH))
    {
        _tcscat(pszIID,_T(" "));
        _tcscat(pszIID,pszInterface);
    }

    // put IID in first, next Interface name as CB_FINDSTRING sucks and works "for an item beginning with the characters in a specified string" 
    SendMessage(this->hWndComboInterfaces,(UINT) CB_ADDSTRING,0,(LPARAM)pszIID);
}

//-----------------------------------------------------------------------------
// Name: AddObjectInterfaceAddressToTreeView
// Object: add interface information to treeview
// Parameters :
//     in  : HTREEITEM hTreeObject : handle to treeview item of current object
//           IID* pIid : interface id
//           PBYTE Address : interface pointer address
//           LPARAM UserParam : treeview user param (CLinkListItem object of CHooked object associated to interface ID)
//     out : 
//     return : 
//-----------------------------------------------------------------------------
HTREEITEM CComInteraction::AddObjectInterfaceAddressToTreeView(HTREEITEM hTreeObject,IID* pIid,PBYTE Address,LPARAM UserParam)
{
    TCHAR psz[2*MAX_PATH];
    TCHAR pszIID[MAX_PATH];
    if (!CGUIDStringConvert::TcharFromIID(pIid,pszIID))
    {
        _stprintf(psz,_T("0x%p"),Address);
        return this->pTreeview->Insert(hTreeObject,psz);
    }
    CGUIDStringConvert::GetInterfaceName(pszIID,pszIID,MAX_PATH);

    _stprintf(psz,_T("%s: 0x%p"),pszIID,Address);
    return this->pTreeview->Insert(hTreeObject,psz,(LPARAM)UserParam);
}

//-----------------------------------------------------------------------------
// Name: DisplayHookedObjects
// Object: display all hooked objects (fill treeview)
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::DisplayHookedObjects()
{
    TCHAR psz[2*MAX_PATH];
    BOOL HookedObjectIsGoingToBeDestroyed;
    HTREEITEM htHookedClass;
    HTREEITEM htHookedObject;
    CLinkListItem* pItemHookedClass;
    CHookedClass* pHookedClass;
    CLinkListItem* pItemHookedObject;
    CHookedObject* pHookedObject;
    CLinkListItem* pItemHookedObjectInterface;
    CHookedObject::OBJECT_INFO_INTERFACE_POINTER* pHookedObjectInterface;
    BOOL ReparsingNeeded;
begin:
    // clear treeview
    this->pTreeview->Clear();

    // for each hooked class
    pLinkListHookedClasses->Lock();
    for(pItemHookedClass=pLinkListHookedClasses->Head;pItemHookedClass;pItemHookedClass=pItemHookedClass->NextItem)
    {
        pHookedClass=(CHookedClass*)pItemHookedClass->ItemData;


        // if there's some hooked object
        if (pHookedClass->pLinkListHookedObjects->GetItemsCount())
        {

            // add Class to treeview
            if (!CGUIDStringConvert::GetClassName(&pHookedClass->Clsid,psz,MAX_PATH))
                CGUIDStringConvert::TcharFromCLSID(&pHookedClass->Clsid,psz);
            htHookedClass=this->pTreeview->Insert(TVI_ROOT,psz,(LPARAM)pItemHookedClass);

            // for each hooked object
            pHookedClass->pLinkListHookedObjects->Lock();
            for (pItemHookedObject=pHookedClass->pLinkListHookedObjects->Head;pItemHookedObject;pItemHookedObject=pItemHookedObject->NextItem)
            {
                pHookedObject=(CHookedObject*)pItemHookedObject->ItemData;

                pHookedObject->Lock(&HookedObjectIsGoingToBeDestroyed);
                // assume object is not being destroyed
                if (HookedObjectIsGoingToBeDestroyed)
                    continue;

                // if IDispatch parsing has been successful
                if (pHookedClass->bIDispatchParsingSuccessFull)
                {
                    ReparsingNeeded=FALSE;

                    try // as pHookedObject is unlocked, it is not secure (exceptional cases)
                    {
                        CExceptionHardware::RegisterTry();

                        // unlock pHookedClass->pLinkListHookedObjects, as GetpIDispatch
                        // can enter hooked functions using the same lock
                        pHookedClass->pLinkListHookedObjects->Unlock();
                        pLinkListHookedClasses->Unlock();
                        pHookedObject->Unlock();

                        // force IDispatch displaying for all hooked objects
                        pHookedObject->GetpIDispatch(pHookedObject->pIUnknown);
                    }
                    catch (CExceptionHardware e)
                    {
                    	ReparsingNeeded=TRUE;
                    }
                    if (ReparsingNeeded)
                    {
                        goto begin;
                    }

                    // restore locks and check if currently parsed items were not removed from list
                    pLinkListHookedClasses->Lock();

                    if (pLinkListHookedClasses->IsItemStillInList(pItemHookedClass,TRUE))
                    {
                        pHookedClass->pLinkListHookedObjects->Lock();

                        if (pHookedClass->pLinkListHookedObjects->IsItemStillInList(pItemHookedObject,TRUE))
                        {
                            // lock object again
                            pHookedObject->Lock(&HookedObjectIsGoingToBeDestroyed);

                            // assume object is not being destroyed
                            if (HookedObjectIsGoingToBeDestroyed)
                            {
                                pHookedClass->pLinkListHookedObjects->Unlock();
                                ReparsingNeeded=TRUE;
                            }
                        }
                        else
                        {
                            pHookedClass->pLinkListHookedObjects->Unlock();
                            ReparsingNeeded=TRUE;
                        }
                    }
                    else
                    {
                        ReparsingNeeded=TRUE;
                    }
                    if (ReparsingNeeded)
                    {
                        // remove locks and redo all objects parsing
                        pLinkListHookedClasses->Unlock();
                        goto begin;
                    }
                }

                // insert object
                _stprintf(psz,_T("0x%p (Thread Owner:0x%X)"),pHookedObject->pObject,pHookedObject->OwningThread);
                htHookedObject=this->pTreeview->Insert(htHookedClass,psz,(LPARAM)pItemHookedObject);

                // insert object references
                pHookedObject->pLinkListInterfacePointers->Lock();
                for (pItemHookedObjectInterface=pHookedObject->pLinkListInterfacePointers->Head;pItemHookedObjectInterface;pItemHookedObjectInterface=pItemHookedObjectInterface->NextItem)
                {
                    pHookedObjectInterface=(CHookedObject::OBJECT_INFO_INTERFACE_POINTER*)pItemHookedObjectInterface->ItemData;

                    this->AddObjectInterfaceAddressToTreeView(htHookedObject,
                                            &pHookedObjectInterface->Iid,
                                            (PBYTE)pHookedObjectInterface->pInterface,
                                            (LPARAM)pItemHookedObjectInterface);

                }
                pHookedObject->pLinkListInterfacePointers->Unlock();

                pHookedObject->Unlock();
            }
            pHookedClass->pLinkListHookedObjects->Unlock();
        }
    }
    pLinkListHookedClasses->Unlock();
}

//-----------------------------------------------------------------------------
// Name: OnTreeViewSelectionChanged
// Object: treeview selection change callback
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::OnTreeViewSelectionChanged()
{
    HTREEITEM hSelectedTreeItem;
    DWORD ItemDepth;
    CHookedObject* pSelectedObject=NULL;
    CHookedObject::OBJECT_INFO_INTERFACE_POINTER* pSelectedInterface=NULL;

    // empty object address
    SetDlgItemText(this->hWndDialog,IDC_EDIT_OBJECT_ADDRESS,_T(""));

    // clear interfaces and functions content
    this->ClearInterfaces();

    // get selected item
    hSelectedTreeItem=this->pTreeview->GetSelectedItem();
    if (!hSelectedTreeItem)
        return;

    // get selected item depth
    ItemDepth=this->pTreeview->GetItemDepth(hSelectedTreeItem);

    switch(ItemDepth)
    {
    case DEPTH_CLASS_NAME:
        return;
    case DEPTH_OBJECTS:
        this->LastSelectedHookedObject=hSelectedTreeItem;
        this->LastSelectedObjectInterface=NULL;
        break;
    case DEPTH_OBJECTS_INTERFACE:
        this->LastSelectedHookedObject=this->pTreeview->GetParent(hSelectedTreeItem);
        this->LastSelectedObjectInterface=hSelectedTreeItem;
        pSelectedInterface=this->GetAssociatedObjectInterface();
        if (!pSelectedInterface)
            return;

        break;
    }

    // get associated objects
    pSelectedObject=this->GetAssociatedObject();
    if(!pSelectedObject)
        return;

    // enable or disable show property page button according to property page support
    EnableWindow(GetDlgItem(this->hWndDialog,IDC_BUTTON_PROPERTY_PAGE),
                CShowPropertyPage::HasPropertyPage(pSelectedObject->pIUnknown));

    // set object / object interface pointer
    if (pSelectedInterface)
        this->SetObjectAddress((PBYTE)pSelectedInterface->pInterface);
    else
        this->SetObjectAddress((PBYTE)pSelectedObject->pObject);

    // fill combobox with interfaces hooked for object, for other interfaces,
    // user have to do a query interface on object
    // insert object references
    CLinkListItem* pItem;
    CHookedObject::OBJECT_INFO_INTERFACE_POINTER* KnownInterfacesForSelectedObject;
    
    pSelectedObject->pLinkListInterfacePointers->Lock();
    for (pItem=pSelectedObject->pLinkListInterfacePointers->Head;pItem;pItem=pItem->NextItem)
    {
        KnownInterfacesForSelectedObject=(CHookedObject::OBJECT_INFO_INTERFACE_POINTER*)pItem->ItemData;

        this->AddInterfaceToComboInterface(&KnownInterfacesForSelectedObject->Iid);
    }
    pSelectedObject->pLinkListInterfacePointers->Unlock();


    LRESULT SelectedItemIndex=0;
    // if an interface is selected
    if (pSelectedInterface)
    {
        TCHAR pszInterface[MAX_PATH];
        // select this interface in combo
        if (CGUIDStringConvert::TcharFromIID(&pSelectedInterface->Iid,pszInterface))
        {
            SelectedItemIndex=SendMessage(this->hWndComboInterfaces,CB_FINDSTRING,(WPARAM)-1,(LPARAM)pszInterface);
            if (SelectedItemIndex==CB_ERR)
                SelectedItemIndex=0;
        }
    }

    // force item selection
    SendMessage(this->hWndComboInterfaces,CB_SETCURSEL ,SelectedItemIndex,0);
    // as CBN_SELCHANGE message is not sent, call OnSelectedInterfaceChange() explicitly
    this->OnSelectedInterfaceChange();
}

//-----------------------------------------------------------------------------
// Name: SetObjectAddress
// Object: set object address user interface field
// Parameters :
//     in  : PBYTE Address : object address
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::SetObjectAddress(PBYTE Address)
{
    TCHAR pszObjectAddress[50];
    _stprintf(pszObjectAddress,_T("0x%p"),Address);
    SetDlgItemText(this->hWndDialog,IDC_EDIT_OBJECT_ADDRESS,pszObjectAddress);
}

//-----------------------------------------------------------------------------
// Name: ClearFunctions
// Object: clear content of functions combo and it's associated memory
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::ClearFunctions()
{
    // clear content of associated methods info content
    this->ClearpLinkListFunctionsInfosForCallContent();

    // clear combo of methods
    SendMessage((HWND) this->hWndComboMethods,(UINT) CB_RESETCONTENT,0,0);
}

//-----------------------------------------------------------------------------
// Name: ClearInterfaces
// Object: clear content of interface combo
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::ClearInterfaces()
{
    this->ClearFunctions();
    SendMessage((HWND) this->hWndComboInterfaces,(UINT) CB_RESETCONTENT,0,0);
}

//-----------------------------------------------------------------------------
// Name: GetSelectedInterfaceInCombo
// Object: get selected interface information
// Parameters :
//     in  : 
//     out : 
//     return : OBJECT_INFO_INTERFACE_POINTER on success, NULL on error
//-----------------------------------------------------------------------------
CHookedObject::OBJECT_INFO_INTERFACE_POINTER* CComInteraction::GetSelectedInterfaceInCombo()
{
    CHookedObject* pHookedObject=this->GetAssociatedObject();
    if (!pHookedObject)
    {
        // clear content of interface list
        this->ClearInterfaces();
        return NULL;
    }

    TCHAR pszInterface[2*MAX_PATH];
    IID Iid;
    GetWindowText(this->hWndComboInterfaces,pszInterface,2*MAX_PATH);

    // keep only IID
    TCHAR* pszIID;
    pszIID=_tcschr(pszInterface,'}');
    if (!pszIID)
        return NULL;

    pszIID++;
    *pszIID=0;

    // get interface ID
    if (!this->GetIID(pszInterface,&Iid))
        return NULL;

    CLinkListItem* pItem=pHookedObject->GetLinkListInterfacePointersItem(&Iid,FALSE);
    if(!pItem)
        return NULL;

    return (CHookedObject::OBJECT_INFO_INTERFACE_POINTER*)pItem->ItemData;
}

//-----------------------------------------------------------------------------
// Name: EnableFunctionNameSelection
// Object: allow function name selection or not (only vtbl index)
//         according to interface
// Parameters :
//     in  : BOOL bEnable : TRUE to enable function name, FALSE to disable
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::EnableFunctionNameSelection(BOOL bEnable)
{
    // enable function name selection
    EnableWindow(GetDlgItem(this->hWndDialog,IDC_RADIO_USE_FUNCTION_NAME),bEnable);

    if (bEnable)
    {
        // select function name
        SendDlgItemMessage(this->hWndDialog,IDC_RADIO_USE_FUNCTION_NAME,(UINT)BM_SETCHECK,BST_CHECKED,0);
        // unselect function index
        SendDlgItemMessage(this->hWndDialog,IDC_RADIO_USE_VTBL_INDEX,(UINT)BM_SETCHECK,BST_UNCHECKED,0);
    }
    else
    {
        // select function index
        SendDlgItemMessage(this->hWndDialog,IDC_RADIO_USE_VTBL_INDEX,(UINT)BM_SETCHECK,BST_CHECKED,0);
        // unselect function name
        SendDlgItemMessage(this->hWndDialog,IDC_RADIO_USE_FUNCTION_NAME,(UINT)BM_SETCHECK,BST_UNCHECKED,0);
    }
}

//-----------------------------------------------------------------------------
// Name: OnSelectedInterfaceChange
// Object: interface selection changed callback
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::OnSelectedInterfaceChange()
{
    BOOL bIDispatchInterface=FALSE;
    this->ClearFunctions();

    // get selected interface in combo
    CHookedObject::OBJECT_INFO_INTERFACE_POINTER* pObjectInterfacePointer;
    pObjectInterfacePointer=this->GetSelectedInterfaceInCombo();
    if (!pObjectInterfacePointer)
        return;

    this->EnableFunctionNameSelection(TRUE);

    CHookedClass* pHookedClass=this->GetAssociatedHookedClass();
    if(!pHookedClass)
        return;

    // update object address
    this->SetObjectAddress((PBYTE)pObjectInterfacePointer->pInterface);
    
    if (pHookedClass->bIDispatchParsingSuccessFull && pHookedClass->pInterfaceExposedByIDispatch)
    {
        // if IDispatch and parsing was successful
        if (IsEqualIID(pObjectInterfacePointer->Iid,pHookedClass->pInterfaceExposedByIDispatch->Iid))
        {
            bIDispatchInterface=TRUE;

            // force IDispatch and IUnknown methods to be shown even they were not monitored
            TCHAR pszFileName[MAX_PATH];
            if (GetMonitoringFileName((IID*)&IID_IDispatch,pszFileName))
            {
                if(CStdFileOperations::DoesFileExists(pszFileName))
                {
                    // parse monitoring files
                    this->ParseInterfaceDefinitionFile((IID*)&IID_IDispatch,pObjectInterfacePointer->pInterface,(IID*)&IID_IDispatch);
                }
            }

            // use IDispatch parsing
            CHookedObject* pHookedObject=this->GetAssociatedObject();
            if(!pHookedObject)
                return;

            if (!pHookedClass->bIDispatchParsingHasBeenTried)
            {
                IID Iid=IID_IUnknown;
                pHookedClass->ParseIDispatch(pHookedObject->pIUnknown,&Iid);
            }

            
            if (CGetIDispatchWinApiOverrideFunctionsRepresentation::GetIDispatchWinApiOverrideFunctionsRepresentation(
                pHookedClass,
                CComInteraction::IDispatchFunctionsRepresentation,
                TRUE,
                this))
            {
                // auto select first function item of combo
                SendMessage(this->hWndComboMethods,CB_SETCURSEL ,0,0);
            }
        } 
    }

    if (!bIDispatchInterface)
    {
        TCHAR pszFileName[MAX_PATH];
        if (!GetMonitoringFileName(&pObjectInterfacePointer->Iid,pszFileName))
            return;
        if(CStdFileOperations::DoesFileExists(pszFileName))
        {
            // parse monitoring files
            this->ParseInterfaceDefinitionFile(&pObjectInterfacePointer->Iid,pObjectInterfacePointer->pInterface,&pObjectInterfacePointer->Iid);

            // auto select first function item of combo
            SendMessage(this->hWndComboMethods,CB_SETCURSEL ,0,0);
        }
        else
        {
            HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,
                                                           _T("No Interface definition file found for selected Interface"),
                                                           _T("Warning"),
                                                           MB_OK|MB_ICONWARNING|MB_TOPMOST
                                                           );
            // disable function name selection
            this->EnableFunctionNameSelection(FALSE);
        }
    }
}

//-----------------------------------------------------------------------------
// Name: IDispatchFunctionsRepresentation
// Object: callback for displaying functions representation of IDispatch in combo functions
// Parameters :
//     in  : TCHAR* pszFuncDesc : function description
//           CMethodInfo* pMethodInfo : CMethodInfo associated to description
//           LPVOID UserParam : current CComInteraction object
//     out : 
//     return : TRUE to continue parsing
//-----------------------------------------------------------------------------
BOOL CComInteraction::IDispatchFunctionsRepresentation(TCHAR* pszFuncDesc,CMethodInfo* pMethodInfo,LPVOID UserParam)
{
    CComInteraction* pComInteraction=(CComInteraction*)UserParam;
    /////////////////////////////////
    // add method name and parameters to combo
    /////////////////////////////////
    LRESULT FunctionIndex;
    FUNCTION_INFO_FOR_CALL* pFuncInfo;
    CLinkListItem* pItem;

    // IDispatch Methods have already been added
    if (pMethodInfo->VTBLIndex<=6)
        return TRUE;

    pItem=pComInteraction->pLinkListFunctionsInfosForCall->AddItem();
    if(!pItem)
        return FALSE;

    // fill this->pLinkListFunctionsInfosForCall
    pFuncInfo=(FUNCTION_INFO_FOR_CALL*)pItem->ItemData;

    if (!pMethodInfo->CanBeHookedByVTBL())
    {
        pFuncInfo->bIDispatchStatic=TRUE;
        pFuncInfo->FunctionAddress=pMethodInfo->Address;
    }

    pFuncInfo->bIDispatchStatic=FALSE;
    // don't use pMethodInfo->Address but *pMethodInfo->VTBLAddress
    // to allow to monitor our own remote call
    if (IsBadReadPtr(pMethodInfo->VTBLAddress,sizeof(PBYTE)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        pFuncInfo->FunctionAddress=0;
        pComInteraction->pLinkListFunctionsInfosForCall->RemoveItem(pItem);
        // continue parsing
        return TRUE;
    }
    else
        pFuncInfo->FunctionAddress=*((PBYTE*)pMethodInfo->VTBLAddress);

    pFuncInfo->pszContent=_tcsdup(pszFuncDesc);

    FunctionIndex=SendMessage(pComInteraction->hWndComboMethods,CB_ADDSTRING,0,(LPARAM)pszFuncDesc);
    if(FunctionIndex==CB_ERR)
    {
        pComInteraction->pLinkListFunctionsInfosForCall->RemoveItem(pItem);
        return FALSE;
    }

    // continue parsing
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: GetSelectedFunctionInCombo
// Object: get selected function information in combo function
// Parameters :
//     in  : 
//     out : 
//     return : FUNCTION_INFO_FOR_CALL on success, NULL on error
//-----------------------------------------------------------------------------
CComInteraction::FUNCTION_INFO_FOR_CALL* CComInteraction::GetSelectedFunctionInCombo()
{
    LRESULT SelectedItem;
    CLinkListItem* pItem;
    FUNCTION_INFO_FOR_CALL* pFunctionInfoForCall;

    // get selected index
    SelectedItem=SendMessage(this->hWndComboMethods,CB_GETCURSEL ,0,0);
    if (SelectedItem==CB_ERR)
        return NULL;
    
    LRESULT TextLen=SendMessage(this->hWndComboMethods,CB_GETLBTEXTLEN ,SelectedItem,0);
    if (TextLen<=0)
        return NULL;
    TextLen++;// for '\0'

    TCHAR* psz=(TCHAR*)_alloca(TextLen*sizeof(TCHAR));
    if (!SendMessage(this->hWndComboMethods,CB_GETLBTEXT,SelectedItem,(LPARAM)psz))
        return NULL;

    // find index in pLinkListFunctionsInfosForCall
    this->pLinkListFunctionsInfosForCall->Lock();
    for (pItem=this->pLinkListFunctionsInfosForCall->Head;pItem;pItem=pItem->NextItem)
    {
        pFunctionInfoForCall=(FUNCTION_INFO_FOR_CALL*)pItem->ItemData;
        if (_tcscmp(pFunctionInfoForCall->pszContent,psz)==0)
        {
            this->pLinkListFunctionsInfosForCall->Unlock();
            // return information
            return pFunctionInfoForCall;
        }
    }
    this->pLinkListFunctionsInfosForCall->Unlock();

    // index not found in list
    return NULL;
}

//-----------------------------------------------------------------------------
// Name: OnCallClick
// Object: Call button click callback
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::OnCallClick()
{
    TCHAR pszMsg[MAX_PATH];
    TCHAR pszPointer[50];
    CParseParametersForRemoteCall* pParseParametersForRemoteCall;
    STRUCT_FUNC_PARAM* pParams=NULL;
    DWORD NbParams=0;
    BOOL bAreSomeParameterPassedAsRef=FALSE;
    REGISTERS Registers={0};
    PBYTE  ReturnedValue=0;
    double FloatingReturn;
    PBYTE pFunc;
    BOOL bStaticFunc=FALSE;
    BOOL ShowRegisters;
    DWORD ThreadId;
    DWORD dwTimeOut;

    // get selected interface in combo
    CHookedObject::OBJECT_INFO_INTERFACE_POINTER* pObjectInterfacePointer;
    pObjectInterfacePointer=this->GetSelectedInterfaceInCombo();
    if (!pObjectInterfacePointer)
        return;

    // get function pointer
    if (IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_USE_FUNCTION_NAME)==BST_CHECKED)
    {
        CComInteraction::FUNCTION_INFO_FOR_CALL* pFunctionInfoForCall;
        pFunctionInfoForCall=this->GetSelectedFunctionInCombo();
        if (!pFunctionInfoForCall)
            return;
        bStaticFunc=pFunctionInfoForCall->bIDispatchStatic;
        pFunc=pFunctionInfoForCall->FunctionAddress;
    }
    else // IDC_RADIO_USE_VTBL_INDEX
    {
        TCHAR psz[50];
        DWORD VTBL_Index;
        PBYTE VTBL_Address;
        GetDlgItemText(this->hWndDialog,IDC_EDIT_VTBL_INDEX,psz,50);
        if (_stscanf(psz,_T("%u"),&VTBL_Index)!=1)
        {
            HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Bad VTBL index value"),_T("Error"),MB_ICONERROR|MB_OK|MB_TOPMOST);
            return;
        }
        VTBL_Address=(*(PBYTE*)pObjectInterfacePointer->pInterface)+VTBL_Index*sizeof(PBYTE);
        if (IsBadReadPtr(VTBL_Address,sizeof(PBYTE)))
        {
            HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Bad VTBL index value"),_T("Error"),MB_ICONERROR|MB_OK|MB_TOPMOST);
            return;
        }
        pFunc=*((PBYTE*)VTBL_Address);
    }

    // assume that pFunc is a code pointer
    if (IsBadCodePtr((FARPROC)pFunc))
    {
        HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Bad code pointer"),_T("Error"),MB_ICONERROR|MB_OK|MB_TOPMOST);
        return;
    }

    //////////////////////////////////
    // forge function parameters
    //////////////////////////////////
    pParseParametersForRemoteCall=new CParseParametersForRemoteCall();
    
    // if auto add object pointer
    *pszPointer=0;
    if(!bStaticFunc)
    {
        if (IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_DONT_ADD_OBJECT_POINTER)!=BST_CHECKED)
        {
            // get interface pointer
            _stprintf(pszPointer,_T("0x%p"),pObjectInterfacePointer->pInterface);
        }
    }
    
    HWND hwndEditCall=GetDlgItem(this->hWndDialog,IDC_EDIT_CALL);
    int TextSize=GetWindowTextLength(hwndEditCall);

    // forge full params
    // pszParams=PointerValue+","+pszParams;
    TCHAR* pszParams=new TCHAR[TextSize+1+_tcslen(pszPointer)+1];// last +1 for potential splitter
    _tcscpy(pszParams,pszPointer);

    if (TextSize!=0)
    {
        // get user params
        TCHAR* pszText=new TCHAR[TextSize+1];
        GetWindowText(hwndEditCall,pszText,TextSize+1);

        // trim user params
        CTrimString::TrimString(pszText);
        // if user params
        if (*pszText)
        {
            // if pointer has a content
            if (*pszPointer)
                // add a splitter
                _tcscat(pszParams,_T(","));
            // add user parameters
            _tcscat(pszParams,pszText);
        }

        delete[] pszText;
    }

    // parse parameters
    BOOL bError=!pParseParametersForRemoteCall->Parse(pszParams,
                                                        &pParams,
                                                        &NbParams,
                                                        &bAreSomeParameterPassedAsRef,
                                                        pszMsg,
                                                        MAX_PATH);

    // free memory
    delete[] pszParams;

    // error checking
    if (bError)
    {
        HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,pszMsg,_T("Error"),MB_ICONERROR|MB_OK|MB_TOPMOST);
        goto Error;
    }

    // retrieve registers value
    if(!this->GetValue(IDC_EDIT_REMOTE_CALL_EAX,&Registers.eax))
        goto Error;
    if(!this->GetValue(IDC_EDIT_REMOTE_CALL_EBX,&Registers.ebx))
        goto Error;
    if(!this->GetValue(IDC_EDIT_REMOTE_CALL_ECX,&Registers.ecx))
        goto Error;
    if(!this->GetValue(IDC_EDIT_REMOTE_CALL_EDX,&Registers.edx))
        goto Error;
    if(!this->GetValue(IDC_EDIT_REMOTE_CALL_ESI,&Registers.esi))
        goto Error;
    if(!this->GetValue(IDC_EDIT_REMOTE_CALL_EDI,&Registers.edi))
        goto Error;
    if(!this->GetValue(IDC_EDIT_REMOTE_CALL_EFL,&Registers.efl))
        goto Error;

    if(!this->GetValue(IDC_EDIT_REMOTE_CALL_TIMEOUT,&dwTimeOut))
        goto Error;
    if (dwTimeOut==0)
        dwTimeOut=INFINITE;

    ShowRegisters=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_REMOTE_CALL_SHOW_OUTPUT_REGISTERS)==BST_CHECKED);

    if (IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_DO_CALL_IN_THREAD_OWNER)==BST_CHECKED)
    {
        CHookedObject* pHookedObject=this->GetAssociatedObject();
        if (!pHookedObject)
            goto Error;
        ThreadId=pHookedObject->OwningThread;
    }
    else
        ThreadId=0;

    // do the internal call
    if (HookComInfos.ProcessInternalCallRequestEx(
                                            (FARPROC)pFunc,
                                            CALLING_CONVENTION_STDCALL,// COM use stdcall
                                            NbParams,
                                            pParams,
                                            &Registers,
                                            &ReturnedValue,
                                            &FloatingReturn,
                                            ThreadId,
                                            dwTimeOut
                                            )
        )
    // according to results
    {
        // if no param as ref
        if ((!bAreSomeParameterPassedAsRef)&&(!ShowRegisters))
        {
            // only message box with returned value
            _stprintf(pszMsg,_T("Function successfully called\r\nReturned Value: 0x%p"),ReturnedValue);
            HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,pszMsg,_T("Information"),MB_ICONINFORMATION|MB_OK|MB_TOPMOST);
        }
        else
        {
            // Small interface presenting parameters
            CRemoteCallResult* pResult=new CRemoteCallResult(NULL,NULL,NbParams,pParams,&Registers,ReturnedValue,ShowRegisters,FloatingReturn);
            pResult->Show(DllhInstance,this->hWndDialog);
            delete pResult;
        }
    }
    else
    {
        HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Error during the call of the function"),_T("Error"),MB_ICONERROR|MB_OK|MB_TOPMOST);
    }

Error:
    // free memory of pointed parameters
    if (pParseParametersForRemoteCall)// delete pParseParametersForRemoteCall only after having call ProcessInternalCall
        delete pParseParametersForRemoteCall;
    if (pParams)
        delete pParams;    
}

//-----------------------------------------------------------------------------
// Name: GetValue
// Object: retrieve DWORD value of specified control
// Parameters :
//     in  : int ControlID : id of control we want to retrieve value
//     out : DWORD* pValue : pointer to Value of control
//     return : TRUE if there's data inside control, false else
//-----------------------------------------------------------------------------
BOOL CComInteraction::GetValue(int ControlID,DWORD* pValue)
{
    TCHAR psz[32];
    *pValue=0;
    GetDlgItemText(this->hWndDialog,ControlID,psz,31);
    if (*psz==0)
        return TRUE;

    if(!CStringConverter::StringToDWORD(psz,pValue))
    {
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("Bad Value : %s"),psz);
        HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: OnShowPropertyPageClick
// Object: Show Property Page button click callback
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CComInteraction::OnShowPropertyPageClick()
{
    UINT x;
    UINT y;
    // get selected object
    CHookedObject* pHookedObject=this->GetAssociatedObject();
    if (!pHookedObject)
        return;
    // check if object has a property page
    if (!CShowPropertyPage::HasPropertyPage(pHookedObject->pIUnknown))
    {
        HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,
                                                        _T("This object has no property page"),
                                                        _T("Error"),
                                                        MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }
    TCHAR psz[MAX_PATH];
    // forge property page caption
    _stprintf(psz,_T("Property page of object 0x%p"),pHookedObject->pObject);
    // display property page
    if (!CShowPropertyPage::ShowPropertyPage(pHookedObject->pIUnknown,
                                        this->hWndDialog,
                                        0,
                                        0,
                                        psz))
    {

        // if property page failed to be executed in the current thread,
        // try to execute it in the original thread
        // use the ProcessInternalCallRequestEx which allow calls in a specified thread
        REGISTERS Registers={0};
        PBYTE  ReturnedValue=0;
        double FloatingReturn;
        STRUCT_FUNC_PARAM pParams[5];
        DWORD NbParams=5;
        pParams[0].bPassAsRef=FALSE;
        pParams[0].dwDataSize=sizeof(IUnknown*);
        pParams[0].pData=(PBYTE)&pHookedObject->pIUnknown;

        pParams[1].bPassAsRef=FALSE;
        pParams[1].dwDataSize=sizeof(HWND);
        pParams[1].pData=(PBYTE)&this->hWndDialog;

        pParams[2].bPassAsRef=FALSE;
        pParams[2].dwDataSize=sizeof(LONG);
        pParams[2].pData=(PBYTE)&x;

        pParams[3].bPassAsRef=FALSE;
        pParams[3].dwDataSize=sizeof(LONG);
        pParams[3].pData=(PBYTE)&y;

        pParams[4].bPassAsRef=TRUE;
        pParams[4].dwDataSize=MAX_PATH*sizeof(TCHAR);
        pParams[4].pData=(PBYTE)psz;
        BOOL bRet=HookComInfos.ProcessInternalCallRequestEx(
                                            (FARPROC)CShowPropertyPage::ShowPropertyPage,
                                            CALLING_CONVENTION_STDCALL,// COM use stdcall
                                            NbParams,
                                            pParams,
                                            &Registers,
                                            &ReturnedValue,
                                            &FloatingReturn,
                                            pHookedObject->OwningThread,
                                            INFINITE
                                            );
        if (!bRet || FAILED(ReturnedValue))
        {
            HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Error can't display property page"),_T("Error"),MB_ICONERROR|MB_OK|MB_TOPMOST);
        }

    }
}

//-----------------------------------------------------------------------------
// Name: GetAssociatedHookedClass
// Object: get CHookedClass object associated to currently selected item in treeview
// Parameters :
//     in  : 
//     out : 
//     return : CHookedClass associated to currently selected item, NULL on error
//-----------------------------------------------------------------------------
CHookedClass* CComInteraction::GetAssociatedHookedClass()
{
    if (this->LastSelectedHookedObject==NULL)
        return NULL;
    // get class htree item associated to selected object
    HTREEITEM htreeClass=this->pTreeview->GetParent(this->LastSelectedHookedObject);

    // get associated class object
    CLinkListItem* pItem;
    if(!this->pTreeview->GetItemUserData(htreeClass,(LPARAM*)&pItem))
        return NULL;
    if (!pItem)
        return NULL;
    if(!pLinkListHookedClasses->IsItemStillInList(pItem,FALSE))
        return NULL;

    return (CHookedClass*)pItem->ItemData;
}

//-----------------------------------------------------------------------------
// Name: GetAssociatedObject
// Object: get CHookedObject object associated to currently selected item in treeview
// Parameters :
//     in  : 
//     out : 
//     return : CHookedObject associated to currently selected item, NULL on error
//-----------------------------------------------------------------------------
CHookedObject* CComInteraction::GetAssociatedObject()
{
    if (this->LastSelectedHookedObject==NULL)
    {
        // report error message
        HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,
                                                        _T("No object selected"),
                                                        _T("Error"),
                                                        MB_OK|MB_ICONERROR|MB_TOPMOST);
        return NULL;
    }

    CHookedClass* pHookedClass=this->GetAssociatedHookedClass();
    if(!pHookedClass)
        return NULL;
    
    // get associated hooked object
    CLinkListItem* pItem;
    if(!this->pTreeview->GetItemUserData(this->LastSelectedHookedObject,(LPARAM*)&pItem))
        return NULL;
    if (!pItem)
        return NULL;
    if(!pHookedClass->pLinkListHookedObjects->IsItemStillInList(pItem,FALSE))
    {
        // if object has been released / destroyed

        // report error message
        HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,
                                                        _T("Object doesn't exists anymore"),
                                                        _T("Error"),
                                                        MB_OK|MB_ICONERROR|MB_TOPMOST);
        // remove object from current list
        if (this->LastSelectedHookedObject)
            this->pTreeview->Remove(this->LastSelectedHookedObject);
        this->LastSelectedHookedObject=NULL;
        this->LastSelectedObjectInterface=NULL;
        return NULL;
    }

    return (CHookedObject*)pItem->ItemData;
}
//-----------------------------------------------------------------------------
// Name: GetAssociatedObjectInterface
// Object: get OBJECT_INFO_INTERFACE_POINTER pointer associated to currently selected item in treeview
// Parameters :
//     in  : 
//     out : 
//     return : OBJECT_INFO_INTERFACE_POINTER associated to currently selected item, NULL on error
//-----------------------------------------------------------------------------
CHookedObject::OBJECT_INFO_INTERFACE_POINTER* CComInteraction::GetAssociatedObjectInterface()
{
    if (this->LastSelectedObjectInterface==NULL)
        return NULL;

    CHookedObject* pHookedObject=this->GetAssociatedObject();
    if(!pHookedObject)
        return NULL;

    // don't check for interface existence as interface pointer are only added not removed
    // until hooked object exists (that has been assumed by this->GetAssociatedObject()!=NULL)
    CLinkListItem* pItem;
    if(!this->pTreeview->GetItemUserData(this->LastSelectedObjectInterface,(LPARAM*)&pItem))
        return NULL;

    return (CHookedObject::OBJECT_INFO_INTERFACE_POINTER*)pItem->ItemData;
}

//-----------------------------------------------------------------------------
// Name: ParseInterfaceDefinitionFile
// Object: parse monitoring file associated to interface 
// Parameters :
//     in  : IID* pIid : current IID
//           IUnknown* pInterfaceAssociatedToIID : interface pointer associated to pIid
//           IID* pFileIid : IID of interface file we are looking for
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CComInteraction::ParseInterfaceDefinitionFile(IID* pIid,
                                                   IUnknown* pInterfaceAssociatedToIID,
                                                   IID* pFileIid
                                                   )
{
    TCHAR pszFileName[MAX_PATH];
    IID Iid=*pIid;
    COMINTERACTION_PARSE_COM_MONITORING_FILE_LINE_PARAM Param;
    if (!GetMonitoringFileName(pFileIid,pszFileName))
        return FALSE;

    if(!CStdFileOperations::DoesFileExists(pszFileName))
    {
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("Interface definition file\r\n%s not found"),pszFileName);
        HookComInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);

        // try to get the IUnknown monitoring file
        Iid=IID_IUnknown;
        if (!GetMonitoringFileName(&Iid,pszFileName))
            return FALSE;
        if(!CStdFileOperations::DoesFileExists(pszFileName))
            return FALSE;
    }

    Param.pComInteraction=this;
    Param.pIid=&Iid;
    Param.pInterfaceAssociatedToIID=pInterfaceAssociatedToIID;

    // parse monitoring file
    return CTextFile::ParseLines(pszFileName,
        CComInteraction::ParseCOMMonitoringFileLine,
        &Param);
}

//-----------------------------------------------------------------------------
// Name: ParseCOMMonitoringFileLine
// Object: parse a COM auto monitoring file line
// Parameters :
//     in  : 
//     out : 
//     return : TRUE to continue parsing
//-----------------------------------------------------------------------------
BOOL CComInteraction::ParseCOMMonitoringFileLine(TCHAR* pszFileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam)
{
    TCHAR* pszFunctionDefinition;
    TCHAR* psz;
    TCHAR* pszParameters;
    HOOK_DEFINITION_INFOS HookDefinitionInfos;
    COMINTERACTION_PARSE_COM_MONITORING_FILE_LINE_PARAM* pParam;

    pParam=(COMINTERACTION_PARSE_COM_MONITORING_FILE_LINE_PARAM*)UserParam;

    CTrimString::TrimString(Line);

    // if empty or commented line (WARNING we keep monitoring disabled lines !!!)
    if ((*Line==0)||(*Line==';'))
        // continue parsing
        return TRUE;

    if (*Line=='!')
    {
        Line++;
        CTrimString::TrimString(Line);
    }
    // line is like 
    // vtbl hooking info|function definition|optional parameters

    // find first splitter (between vtbl hooking info definition and function definition)
    pszFunctionDefinition=_tcschr(Line,FIELDS_SEPARATOR);

    if (pszFunctionDefinition)
    {
        // ends line --> line contain only vtbl hooking info
        *pszFunctionDefinition=0;
        // point to function definition
        pszFunctionDefinition++;
    }

    // get vtbl hooking info definition
    if(!GetHookDefinitionInfo(Line,pParam->pIid,&HookDefinitionInfos))
    {
        return ReportParsingError(pszFileName,dwLineNumber);
    }

    // if we need a full iid spying, the matching IID file needs to be loaded
    if (HookDefinitionInfos.VTBLInfoType==VTBL_INFO_TYPE_OBJECT_FULL_IID)
    {
        pParam->pComInteraction->ParseInterfaceDefinitionFile(pParam->pIid,
                                                                pParam->pInterfaceAssociatedToIID,
                                                                &HookDefinitionInfos.InterfaceID
                                                                );

        // continue parsing
        return TRUE;
    }

    if (!pszFunctionDefinition)
        return ReportParsingError(pszFileName,dwLineNumber);

    // get function name
    // search end of function name
    pszParameters=_tcschr(pszFunctionDefinition, '(');
    if (!pszParameters)
        return ReportParsingError(pszFileName,dwLineNumber);

    *pszParameters=0;
    // remove return type in function name if any or WINAPI, __cdecl keywords 
    // search last space in function name
    psz=_tcsrchr(pszFunctionDefinition,' ');
    // keep only data after last space
    if (psz)
    {
        psz++;// point to char after space
        pszFunctionDefinition=psz;
    }
    // restore parameter '('
    *pszParameters='(';
    // looks for end of parameters
    psz=_tcschr(pszParameters,')');
    if(!psz)
        return ReportParsingError(pszFileName,dwLineNumber);

    // ends string after ')' : remove optional log informations
    psz++;
    *psz=0;

    CLinkListItem* pItem=pParam->pComInteraction->pLinkListFunctionsInfosForCall->AddItem();
    if(!pItem)
        // continue parsing
        return TRUE;

    LRESULT FunctionIndex=SendMessage(pParam->pComInteraction->hWndComboMethods,CB_ADDSTRING,0,(LPARAM)pszFunctionDefinition);
    if(FunctionIndex==CB_ERR)
    {
        pParam->pComInteraction->pLinkListFunctionsInfosForCall->RemoveItem(pItem);
        // continue parsing
        return TRUE;
    }
    
    // get function info
    FUNCTION_INFO_FOR_CALL* pFuncInfo=(FUNCTION_INFO_FOR_CALL*)pItem->ItemData;
    pFuncInfo->bIDispatchStatic=FALSE;

    // get VTBL address
    PBYTE VTBL_Address=*((PBYTE*)pParam->pInterfaceAssociatedToIID)+HookDefinitionInfos.VTBLIndex*sizeof(PBYTE);
    if (IsBadReadPtr(VTBL_Address,sizeof(PBYTE)))
    {
        // report error
        TCHAR pszMsg[2*MAX_PATH];
        _stprintf(pszMsg,_T("Possible bad VTBL index (%u) in file %s at line %u."),HookDefinitionInfos.VTBLIndex,pszFileName,dwLineNumber);
        HookComInfos.ReportMessage(REPORT_MESSAGE_ERROR,pszMsg);

        SendMessage(pParam->pComInteraction->hWndComboMethods,CB_DELETESTRING,(WPARAM)FunctionIndex,0);
        pParam->pComInteraction->pLinkListFunctionsInfosForCall->RemoveItem(pItem);
        // continue parsing
        return TRUE;
    }

    // get function address
    pFuncInfo->FunctionAddress=*((PBYTE*)VTBL_Address);
    if (IsBadCodePtr((FARPROC)pFuncInfo->FunctionAddress))
    {
        // report error
        TCHAR pszMsg[2*MAX_PATH];
        _stprintf(pszMsg,_T("Possible bad VTBL index (%u) in file %s at line %u."),HookDefinitionInfos.VTBLIndex,pszFileName,dwLineNumber);
        HookComInfos.ReportMessage(REPORT_MESSAGE_ERROR,pszMsg);

        SendMessage(pParam->pComInteraction->hWndComboMethods,CB_DELETESTRING,(WPARAM)FunctionIndex,0);
        pParam->pComInteraction->pLinkListFunctionsInfosForCall->RemoveItem(pItem);
        // continue parsing
        return TRUE;
    }
    // get function description
    pFuncInfo->pszContent=_tcsdup(pszFunctionDefinition);

    // continue parsing
    return TRUE;
}
