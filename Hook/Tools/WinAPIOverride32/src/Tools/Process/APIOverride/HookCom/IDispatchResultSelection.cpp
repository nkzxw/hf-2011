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
// Object: manages the Com IDispatchResultSelection dialog
//-----------------------------------------------------------------------------
#include "idispatchresultselection.h"

extern HINSTANCE DllhInstance;
extern CLinkListSimple* pLinkListOpenDialogs;
extern HANDLE hSemaphoreOpenDialogs;
extern HOOK_COM_INIT HookComInfos;
extern CLinkListSimple* pLinkListHookedClasses;

CIDispatchResultSelection::CIDispatchResultSelection(CHookedClass* pHookedClass)
{
    this->pHookedClass=pHookedClass;
    this->pItemDialog=NULL;
}

CIDispatchResultSelection::~CIDispatchResultSelection(void)
{
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
void CIDispatchResultSelection::Show(CHookedClass* pHookedClass)
{
    // if only IDispatch interface, let config files as owner
    CLinkListItem* pItemMethod;
    CMethodInfo* pMethodInfo;
    DWORD ItemCount=0;

    // for each method of IDispatch
    pHookedClass->pInterfaceExposedByIDispatch->pMethodInfoList->Lock();
    for(pItemMethod=pHookedClass->pInterfaceExposedByIDispatch->pMethodInfoList->Head;pItemMethod;pItemMethod=pItemMethod->NextItem)
    {
        pMethodInfo=(CMethodInfo*)pItemMethod->ItemData;
        // assume pointer validity
        if (IsBadReadPtr(pMethodInfo,sizeof(CMethodInfo)))
            continue;
        if ((pMethodInfo->VTBLIndex>6) && (pMethodInfo->VTBLAddress!=(PBYTE)-1))
        {
            pMethodInfo->AskedToBeNotLogged=FALSE;
            ItemCount++;
        }
        else
        {
            pMethodInfo->AskedToBeNotLogged=TRUE;
        }
    }
    pHookedClass->pInterfaceExposedByIDispatch->pMethodInfoList->Unlock();
    if (ItemCount==0)
        return;

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
        return;
    }

    HANDLE hthread=HookComInfos.AdjustThreadSecurityAndLaunchDialogThread(CIDispatchResultSelection::ModelessDialogThread,pHookedClass);
    WaitForSingleObject(hthread,INFINITE);
    CloseHandle(hthread);
    if (hSemaphoreOpenDialogs)
        ReleaseSemaphore(hSemaphoreOpenDialogs,1,NULL); // must be last instruction
}

//-----------------------------------------------------------------------------
// Name: ModelessDialogThread
// Object: allow to act like a dialog box in modeless mode
// Parameters :
//     in  : PVOID lParam : HINSTANCE hInstance : application instance
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CIDispatchResultSelection::ModelessDialogThread(PVOID lParam)
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

    CIDispatchResultSelection* pIDispatchResultSelection=new CIDispatchResultSelection((CHookedClass*)lParam);

    DialogBoxParam (DllhInstance,(LPCTSTR)IDD_DIALOG_IDISPATCH_SELECTION,NULL,(DLGPROC)CIDispatchResultSelection::WndProc,(LPARAM)pIDispatchResultSelection);

    delete pIDispatchResultSelection;

    HookComInfos.RestoreStation(CurrentStation,OldStation,CurrentDesktop,OldDesktop);

    return 0;
}
//-----------------------------------------------------------------------------
// Name: Init
// Object: init stats dialog
// Parameters :
//     in  : HWND hWndDialog : stat dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CIDispatchResultSelection::Init(HWND hwnd)
{
    this->hWndDialog=hwnd;
    this->pItemDialog=pLinkListOpenDialogs->AddItem(hwnd);

    this->pListview=new CListview(GetDlgItem(this->hWndDialog,IDC_LIST_IDISPATCH));
    this->pListview->SetStyle(TRUE,FALSE,FALSE,TRUE);
    this->pListview->AddColumn(_T("Function Description"),600,LVCFMT_LEFT);
    this->pListview->EnableDefaultCustomDraw(FALSE);

    // get IDispatch content
    CGetIDispatchWinApiOverrideFunctionsRepresentation::GetIDispatchWinApiOverrideFunctionsRepresentation(
            this->pHookedClass,
            CIDispatchResultSelection::IDispatchFunctionsRepresentation,
            TRUE,
            this);

    // set interface name
    TCHAR pszIID[MAX_PATH];
    CGUIDStringConvert::TcharFromIID(&this->pHookedClass->pInterfaceExposedByIDispatch->Iid,pszIID);
    SetDlgItemText(this->hWndDialog,IDC_STATIC_INTERFACE_NAME,pszIID);
    if (CGUIDStringConvert::GetInterfaceName(pszIID,pszIID,MAX_PATH))
        SetDlgItemText(this->hWndDialog,IDC_STATIC_INTERFACE_NAME,pszIID);
    else
        SetDlgItemText(this->hWndDialog,IDC_STATIC_INTERFACE_NAME,_T(""));

    // set monitoring file name
    TCHAR pszFileName[MAX_PATH];
    GetMonitoringFileName(&pHookedClass->pInterfaceExposedByIDispatch->Iid,pszFileName);
    SetDlgItemText(this->hWndDialog,IDC_EDIT_FILE_NAME,pszFileName);

}

//-----------------------------------------------------------------------------
// Name: IDispatchFunctionsRepresentation
// Object: add functions description to listview
// Parameters :
//     in  : TCHAR* pszFuncDesc : function description
//           CMethodInfo* pMethodInfo : associated method info
//           LPVOID UserParam : pointer to CIDispatchResultSelection
//     out :
//     return : TRUE to continue parsing
//-----------------------------------------------------------------------------
BOOL CIDispatchResultSelection::IDispatchFunctionsRepresentation(TCHAR* pszFuncDesc,CMethodInfo* pMethodInfo,LPVOID UserParam)
{
    CIDispatchResultSelection* pIDispatchResultSelection=(CIDispatchResultSelection*)UserParam;
    
    // hide IDispatch method infos, as they are taken from IDispatch monitoring file
    // and don't take care of user interaction; so hiding them avoid user confusion
    if ((pMethodInfo->VTBLIndex<=6) || (pMethodInfo->VTBLAddress==(PBYTE)-1))
    {
        pMethodInfo->AskedToBeNotLogged=TRUE;
        // continue list parsing
        return TRUE;
    }
    // add item to list view
    int ItemIndex=pIDispatchResultSelection->pListview->AddItem(pszFuncDesc,pMethodInfo);
    // set item to selected by default
    pIDispatchResultSelection->pListview->SetSelectedState(ItemIndex,TRUE);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: Close stats dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CIDispatchResultSelection::Close()
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
LRESULT CALLBACK CIDispatchResultSelection::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // init dialog
        SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG)lParam);
        CDialogHelper::SetIcon(hWnd,IDI_ICON_SHOW_METHODS_ADDRESS);
        ((CIDispatchResultSelection*)lParam)->Init(hWnd);
        break;
    case WM_CLOSE:
        {
            // close dialog
            CIDispatchResultSelection* pIDispatchResultSelection=(CIDispatchResultSelection*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (pIDispatchResultSelection)
                pIDispatchResultSelection->Close();
        }
        break;
    case WM_COMMAND:
        {
            CIDispatchResultSelection* pIDispatchResultSelection=(CIDispatchResultSelection*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (!pIDispatchResultSelection)
                return FALSE;
            switch(LOWORD(wParam))
            {
            case IDOK:
                pIDispatchResultSelection->OnOKClick();
                break;
            case IDCANCEL:
                pIDispatchResultSelection->Close();
                break;
            case IDC_BUTTON_CHECK_ALL:
                pIDispatchResultSelection->pListview->SelectAll();
                break;
            case IDC_BUTTON_UNCHECK_ALL:
                pIDispatchResultSelection->pListview->UnselectAll();
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            CIDispatchResultSelection* pIDispatchResultSelection=(CIDispatchResultSelection*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (!pIDispatchResultSelection)
                return FALSE;

            if (pIDispatchResultSelection->pListview->OnNotify(wParam,lParam))
                // if message has been proceed by pListview->OnNotify
                break;
        }
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: OnOKClick
// Object: called when user click the ok button
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CIDispatchResultSelection::OnOKClick()
{
    HANDLE hFile=INVALID_HANDLE_VALUE;
    // if user wants to create a monitoring file to save settings
    if (IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_SAVE_SETTINGS_UNDER_FILE)==BST_CHECKED)
    {
        TCHAR pszMsg[2*MAX_PATH];
        TCHAR pszMonitoringFileName[MAX_PATH];
        GetDlgItemText(this->hWndDialog,IDC_EDIT_FILE_NAME,pszMonitoringFileName,MAX_PATH);
        
        // create a new monitoring file
        if (!CTextFile::CreateTextFile(pszMonitoringFileName,&hFile))
        {
            _stprintf(pszMsg,_T("Error creating file \r\n%s"),pszMonitoringFileName);
            HookComInfos.DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        }

    }

    CMethodInfo* pMethodInfo;
    TCHAR pszFuncDescr[2048];
    TCHAR psz[50];
    int NbItems=this->pListview->GetItemCount();
    int CurrentItem;
    BOOL InterfaceNameRetrievalSuccess;
    TCHAR pszIID[MAX_PATH];

    InterfaceNameRetrievalSuccess=FALSE;
    CGUIDStringConvert::TcharFromIID(&this->pHookedClass->pInterfaceExposedByIDispatch->Iid,pszIID);
    if (CGUIDStringConvert::GetInterfaceName(pszIID,pszIID,MAX_PATH))
        InterfaceNameRetrievalSuccess=TRUE;

    // if user wants to create a monitoring file
    if (hFile!=INVALID_HANDLE_VALUE)
    {
        // write monitoring file header
        CTextFile::WriteText(hFile,HOOK_COM_INTERFACE_MONITORING_FILE_HEADER);

        // write interface name
        if (InterfaceNameRetrievalSuccess)
        {
            CTextFile::WriteText(hFile,_T(";@InterfaceName="));
            CTextFile::WriteText(hFile,pszIID);
            CTextFile::WriteText(hFile,_T("\r\n\r\n"));
        }

        // add derivation from IDispatch
        CTextFile::WriteText(hFile,_T(";Derived from IDispatch\r\n"));
        CTextFile::WriteText(hFile,_T("BaseIID={00020400-0000-0000-C000-000000000046}\r\n\r\n"));
    }

    // for each methods in list
    for (CurrentItem=0;CurrentItem<NbItems;CurrentItem++)
    {
        // get method info associated to item
        this->pListview->GetItemUserData(CurrentItem,(LPVOID*)&pMethodInfo);

        // check pointer validity
        if (IsBadReadPtr(pMethodInfo,sizeof(CMethodInfo)))
            continue;

        if (pMethodInfo->VTBLIndex<=6) // IDispatch method, they are already included by BaseIID=
            continue;

        // get selected sate
        pMethodInfo->AskedToBeNotLogged=!this->pListview->IsItemSelected(CurrentItem);

        // if user wants to create a monitoring file
        if (hFile!=INVALID_HANDLE_VALUE)
        {
            // if item should not be logged, add '!' char
            if (pMethodInfo->AskedToBeNotLogged)
                CTextFile::WriteText(hFile,_T("!"));

            CTextFile::WriteText(hFile,COM_DEFINITION_CURRENT_IID_VTBL_INDEX);
            // compute VTBL Index
            _stprintf(psz,_T("%u"),(pMethodInfo->VTBLAddress-this->pHookedClass->pInterfaceExposedByIDispatch->VTBLAddress)/sizeof(PBYTE));
            CTextFile::WriteText(hFile,psz);
            CTextFile::WriteText(hFile,_T("|"));

            if (InterfaceNameRetrievalSuccess)
            {
                CTextFile::WriteText(hFile,pszIID);
                CTextFile::WriteText(hFile,_T("::"));
            }

            // write func desc
            pMethodInfo->ToString(FALSE,pszFuncDescr);
            CTextFile::WriteText(hFile,pszFuncDescr);

            // add |Out key word if method has an out parameter
            if (pMethodInfo->HasAnOutParameter())
                CTextFile::WriteText(hFile,_T("|Out"));

            CTextFile::WriteText(hFile,_T("\r\n"));
        }
    }

    // if user wants to create a monitoring file
    if (hFile!=INVALID_HANDLE_VALUE)
        // close file
        CloseHandle(hFile);

    this->Close();
}