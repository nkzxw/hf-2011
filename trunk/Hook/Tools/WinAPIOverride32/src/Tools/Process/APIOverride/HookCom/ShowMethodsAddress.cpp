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

#include "showmethodsaddress.h"
extern HINSTANCE DllhInstance;
extern CLinkListSimple* pLinkListOpenDialogs;
extern HANDLE hSemaphoreOpenDialogs;
extern HOOK_COM_INIT HookComInfos;

extern "C" __declspec(dllexport) void __stdcall ShowMethodsAddress()
{
    CComShowMethodsAddress::Show(DllhInstance,NULL);
}

CComShowMethodsAddress::CComShowMethodsAddress()
{
}
CComShowMethodsAddress::~CComShowMethodsAddress()
{
}

//-----------------------------------------------------------------------------
// Name: ModelessDialogThread
// Object: allow to act like a dialog box in modeless mode
// Parameters :
//     in  : PVOID lParam : HINSTANCE hInstance : application instance
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CComShowMethodsAddress::ModelessDialogThread(PVOID lParam)
{
    CComShowMethodsAddress* pComShowMethodsAddress=new CComShowMethodsAddress();
    // force ole initialization for current thread
    OleInitialize(NULL);
    
    DialogBoxParam ((HINSTANCE)lParam,(LPCTSTR)IDD_DIALOG_SHOW_METHODS_ADDRESS,NULL,(DLGPROC)CComShowMethodsAddress::WndProc,(LPARAM)pComShowMethodsAddress);
    delete pComShowMethodsAddress->pTreeview;

    // uninitialize ole for current thread
    // OleUninitialize(); // too slow
    delete pComShowMethodsAddress;

    if (hSemaphoreOpenDialogs)
        ReleaseSemaphore(hSemaphoreOpenDialogs,1,NULL); // must be last instruction

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
void CComShowMethodsAddress::Show(HINSTANCE hInstance,HWND hWndDialog)
{
    // assume HookComInfos.DynamicMessageBoxInDefaultStation has a meaning
    if (IsBadCodePtr((FARPROC)HookComInfos.DynamicMessageBoxInDefaultStation))
    {
#if (defined(UNICODE)||defined(_UNICODE))
        HookComInfos.DynamicMessageBoxInDefaultStation=(pfDynamicMessageBoxInDefaultStation)GetProcAddress(LoadLibrary(_T("User32.dll")),"MessageBoxW");
#else
        HookComInfos.DynamicMessageBoxInDefaultStation=(pfDynamicMessageBoxInDefaultStation)GetProcAddress(LoadLibrary(_T("User32.dll")),"MessageBoxA");
#endif
    }


    // show dialog
    UNREFERENCED_PARAMETER(hWndDialog);
    // create thread instead of using CreateDialogParam to don't have to handle keyboard event like TAB
    CloseHandle(CreateThread(NULL,0,CComShowMethodsAddress::ModelessDialogThread,hInstance,0,NULL));
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: init stats dialog
// Parameters :
//     in  : HWND hWndDialog : stat dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComShowMethodsAddress::Init(HWND hwnd)
{
    this->hWndDialog=hwnd;
    this->pItemDialog=pLinkListOpenDialogs->AddItem(hwnd);
    
    // create treeview object associated to control
    this->pTreeview=new CTreeview(GetDlgItem(this->hWndDialog,IDC_TREE_METHODS_ADDRESS));

    // load icon for buttons
    HICON hIconEdit=(HICON)LoadImage(DllhInstance,MAKEINTRESOURCE(IDI_ICON_EDIT),IMAGE_ICON,24,24,LR_DEFAULTCOLOR|LR_SHARED);
    SendDlgItemMessage(this->hWndDialog,IDC_BUTTON_EDIT_ASSOCIATED_IID_FILE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIconEdit);

    this->OnSize();

    // check try IDispatch parsing option by default
    SendDlgItemMessage(this->hWndDialog,IDC_CHECK_TRY_IDISPATCH_PARSING,(UINT)BM_SETCHECK,BST_CHECKED,0);

}

//-----------------------------------------------------------------------------
// Name: Close
// Object: Close stats dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComShowMethodsAddress::Close()
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
LRESULT CALLBACK CComShowMethodsAddress::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // init dialog
        SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG)lParam);
        CDialogHelper::SetIcon(hWnd,IDI_ICON_SHOW_METHODS_ADDRESS);
        ((CComShowMethodsAddress*)lParam)->Init(hWnd);
        break;
    case WM_CLOSE:
        {
            // close dialog
            CComShowMethodsAddress* pComShowMethodsAddress=(CComShowMethodsAddress*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (pComShowMethodsAddress)
                pComShowMethodsAddress->Close();
        }
        break;
    case WM_SIZE:
        {
            CComShowMethodsAddress* pComShowMethodsAddress=(CComShowMethodsAddress*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (!pComShowMethodsAddress)
                return FALSE;
            pComShowMethodsAddress->OnSize();
        }
        break;
    case WM_SIZING:
        {
            CComShowMethodsAddress* pComShowMethodsAddress=(CComShowMethodsAddress*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (!pComShowMethodsAddress)
                return FALSE;
            pComShowMethodsAddress->OnSizing((RECT*)lParam);
        }
        break;
    case WM_COMMAND:
        {
            CComShowMethodsAddress* pComShowMethodsAddress=(CComShowMethodsAddress*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (!pComShowMethodsAddress)
                return FALSE;
            switch(LOWORD(wParam))
            {
            case IDOK:
                pComShowMethodsAddress->OnOkClick();
                break;
            case IDC_BUTTON_EDIT_ASSOCIATED_IID_FILE:
                pComShowMethodsAddress->OnEditIIDAssociatedFileClick();
                break;
            case IDC_BUTTON_SAVE:
                pComShowMethodsAddress->OnSaveClick();
                break;
            case IDCANCEL:
                pComShowMethodsAddress->Close();
                break;
            }
        }
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: OnSizing
// Object: called on WM_SIZING. Assume main dialog has a min with and hight
// Parameters :
//     in  : 
//     out :
//     In Out : RECT* pWinRect : window rect
//     return : 
//-----------------------------------------------------------------------------
void CComShowMethodsAddress::OnSizing(RECT* pWinRect)
{
    // check min width and min height
    if ((pWinRect->right-pWinRect->left)<CComShowMethodsAddress_DIALOG_MIN_WIDTH)
        pWinRect->right=pWinRect->left+CComShowMethodsAddress_DIALOG_MIN_WIDTH;
    if ((pWinRect->bottom-pWinRect->top)<CComShowMethodsAddress_DIALOG_MIN_HEIGHT)
        pWinRect->bottom=pWinRect->top+CComShowMethodsAddress_DIALOG_MIN_HEIGHT;
}
//-----------------------------------------------------------------------------
// Name: OnSize
// Object: called on WM_SIZE. Resize some controls
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComShowMethodsAddress::OnSize()
{
    RECT RectWindow;
    RECT RectExit;
    RECT RectTreeview;
    RECT RectSave;
    HWND hItem;
    DWORD Spacer;
    
    CDialogHelper::GetClientWindowRect(this->hWndDialog,this->hWndDialog,&RectWindow);
    // Exit button

    // Spacer
    CDialogHelper::GetClientWindowRect(this->hWndDialog,this->pTreeview->GetControlHandle(),&RectTreeview);
    Spacer=RectTreeview.left;

    // Exit button
    hItem=GetDlgItem(this->hWndDialog,IDCANCEL);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&RectExit);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RectWindow.right-Spacer-(RectExit.right-RectExit.left)+RectWindow.left,
        RectWindow.bottom-Spacer-(RectExit.bottom-RectExit.top),
        0,
        0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    // get new rect exit pos
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&RectExit);
    CDialogHelper::Redraw(hItem);

    // treeview
    SetWindowPos(this->pTreeview->GetControlHandle(),HWND_NOTOPMOST,
        0,
        0,
        RectExit.right-RectTreeview.left,
        RectExit.top-RectTreeview.top-CComShowMethodsAddress_SPACE_BETWEEN_CONTROLS,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(hItem);

    // save button
    hItem=GetDlgItem(this->hWndDialog,IDC_BUTTON_SAVE);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&RectSave);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RectSave.left,
        RectExit.top,
        0,
        0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    // get new rect exit pos
    CDialogHelper::Redraw(hItem);
}

//-----------------------------------------------------------------------------
// Name: OnSaveClick
// Object: called when user clicks Save button
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComShowMethodsAddress::OnSaveClick()
{
    this->pTreeview->Save(FALSE,FALSE,_T("html"));
}

//-----------------------------------------------------------------------------
// Name: GetCLSID
// Object: get current select object CLSID
// Parameters :
//     in  : 
//     out : CLSID* pClsid : selected object CLSID
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CComShowMethodsAddress::GetCLSID(OUT CLSID* pClsid)
{
    TCHAR pszMsg[MAX_PATH];
    TCHAR pszCLSID[MAX_PATH];
    BOOL bError=FALSE;

    // get clsid
    GetDlgItemText(this->hWndDialog,IDC_EDIT_CLSID,pszCLSID,MAX_PATH);
    CTrimString::TrimString(pszCLSID);

    if (*pszCLSID=='{') 
    {
        if(!CGUIDStringConvert::CLSIDFromTchar(pszCLSID,pClsid))
        {
            bError=TRUE;
            _tcscpy(pszMsg,_T("Bad CLSID value"));
        }
    }
    else
    {
        // if first char != '{' --> check for ProgId
        if(!CGUIDStringConvert::CLSIDFromProgId(pszCLSID,pClsid))
        {
            bError=TRUE;
            _tcscpy(pszMsg,_T("Unknown ProgId"));
        }
    }
    // in case of error
    if (bError)
    {
        MessageBox(this->hWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: GetIID
// Object: get current select object IID
// Parameters :
//     in  : 
//     out : IID* pIid : selected object IID
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CComShowMethodsAddress::GetIID(OUT IID* pIid)
{
    TCHAR pszMsg[MAX_PATH];
    TCHAR pszIID[MAX_PATH];
    BOOL bError=FALSE;

    // get IID
    GetDlgItemText(this->hWndDialog,IDC_EDIT_IID,pszIID,MAX_PATH);
    CTrimString::TrimString(pszIID);
    if (*pszIID==0)
        *pIid=IID_IUnknown;
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
        MessageBox(this->hWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: OnEditIIDAssociatedFileClick
// Object: called when user clicks edit associated monitoring file button
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComShowMethodsAddress::OnEditIIDAssociatedFileClick()
{
    TCHAR pszMonitoringFileName[MAX_PATH];
    IID Iid;
    if (!this->GetIID(&Iid))
        return;
    if (!GetMonitoringFileName(&Iid,pszMonitoringFileName))
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
        MessageBox(this->hWndDialog,_T("Error launching default editor application"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
    }
}
//-----------------------------------------------------------------------------
// Name: OnOkClick
// Object: called when user clicks ok button
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComShowMethodsAddress::OnOkClick()
{
    CLSID Clsid;
    IID Iid;
    BOOL bTryIDispatchParsing;

    // reset vars
    this->hModule=NULL;
    this->VTBLAddress=0;

    // clear treeview
    this->pTreeview->Clear();

    // get CLSID and IID from interface
    if (!this->GetCLSID(&Clsid))
        return;
    if (!this->GetIID(&Iid))
        return;

    // get the try IDispatch parsing value
    bTryIDispatchParsing=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_TRY_IDISPATCH_PARSING)==BST_CHECKED);

    this->ParseObjectMethodAddress(&Clsid,&Iid,bTryIDispatchParsing);
}

//-----------------------------------------------------------------------------
// Name: ModuleFoundCallback
// Object: called for each module found
// Parameters :
//     in  : MODULEENTRY* pModuleEntry : found module struct
//           PVOID UserParam : pointer on CComShowMethodsAddress
//     out : 
//     return : TRUE to continue parsing
//-----------------------------------------------------------------------------
BOOL CComShowMethodsAddress::ModuleFoundCallback(MODULEENTRY* pModuleEntry,PVOID UserParam)
{
    CComShowMethodsAddress* pComShowMethodsAddress=(CComShowMethodsAddress*)UserParam;
    // check if lpBaseOfDll<=VTBLAddress<=lpBaseOfDll+SizeOfImage
    if (((pModuleEntry->modBaseAddr)<=(pComShowMethodsAddress->VTBLAddress))
        &&((pModuleEntry->modBaseAddr+pModuleEntry->modBaseSize)>=pComShowMethodsAddress->VTBLAddress))
    {
        // module has been found
        pComShowMethodsAddress->hModule=pModuleEntry->hModule;
        // stop parsing
        return FALSE;
    }
    // continue parsing
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: FindAssociatedModuleHandle
// Object: find module associated to vtbl
// Parameters :
//     in  : PBYTE VTBLAddress : virtual function table address
//     out : 
//     return : module handle on success, NULL on failure
//-----------------------------------------------------------------------------
HMODULE CComShowMethodsAddress::FindAssociatedModuleHandle(PBYTE VTBLAddress)
{
    this->VTBLAddress=VTBLAddress;
    this->hModule=NULL;
    CModulesParser::Parse(GetCurrentProcessId(),CComShowMethodsAddress::ModuleFoundCallback,this);
    return this->hModule;
}

//-----------------------------------------------------------------------------
// Name: AddAddressToTreeView
// Object: add specified address to treeview with VA, not rebased VA and raw values
// Parameters :
//     in  : HTREEITEM hTreeItem : handle to parent treeview node
//           PBYTE Address : address to add
//           CPE* pPe : parsed pseudo header
//     out : 
//     return : TRUE in case of success
//-----------------------------------------------------------------------------
BOOL CComShowMethodsAddress::AddAddressToTreeView(HTREEITEM hTreeItem,PBYTE Address,CPE* pPe)
{
    TCHAR psz[MAX_PATH];

    // relative RVA
    
    Address=(PBYTE)(Address-(PBYTE)this->hModule);
    _stprintf(psz,_T("Relative Virtual Address: 0x%p"),Address);
    this->pTreeview->Insert(hTreeItem,psz);

    // non rebased RVA
    _stprintf(psz,_T("Not-Rebased Virtual Address: 0x%p"),(PBYTE)pPe->GetUnrebasedVirtualAddress((DWORD)Address));
    this->pTreeview->Insert(hTreeItem,psz);

    // RAW
    ULONGLONG TmpAddress=(ULONGLONG)Address;
    if (pPe->RvaToRaw(TmpAddress,&TmpAddress))
    {
        Address=(PBYTE)TmpAddress;
        _stprintf(psz,_T("RAW Address: 0x%p"),Address);
        this->pTreeview->Insert(hTreeItem,psz);
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: DisplayObjectMethodAddress
// Object: display object methods addresses
// Parameters :
//     in  : CHookedClass* pHookedClass : hooked class object
//           IUnknown* pInterfaceAssociatedToIID : pointer to interface associated with specified IID
//           IID* pIid : IID
//     out : 
//     return : TRUE in case of success
//-----------------------------------------------------------------------------
void CComShowMethodsAddress::DisplayObjectMethodAddress(CHookedClass* pHookedClass,IUnknown* pInterfaceAssociatedToIID,IID* pIid)
{

    TCHAR psz[3*MAX_PATH];
    TCHAR AssociatedModuleName[MAX_PATH];
    CLinkListItem* pItemInterface;
    CHookedInterface* pHookedInterface;
    CLinkListItem* pItemMethodInfo;
    CMethodInfo* pMethodInfo;
    CLinkListItem* pItemIID;
    IID* pDerivedIID;
    CLinkListItem* pItemPropertyInfo;
    CPropertyInfo* pPropertyInfo;
    IID Iid;
    CIIDDisplayTreeInfo* pIIDDisplayTreeInfo;
    CIIDDisplayTreeInfo* pDerivedIIDDisplayTreeInfo;
    CIIDDisplayTreeInfo* pNextDerivedIIDDisplayTreeInfo;
    BOOL bIDispatchParsed;
#if ((!defined(UNICODE))&&(!defined(_UNICODE)))
    size_t Size;
#endif

    HTREEITEM htRoot;
    HTREEITEM htParsedInterface;
    HTREEITEM htInterface;
    HTREEITEM htDerivedInterface;
    HTREEITEM htMethod;
    HTREEITEM htVTBLAddress;
    HTREEITEM htAddressInVTBL;
    HTREEITEM htProperties;
    HTREEITEM htIDispatch=NULL;


    // get associated module name
    GetModuleFileName(this->hModule,AssociatedModuleName,MAX_PATH);
    // parse pe of associated dll
    CPE Pe(AssociatedModuleName);
    Pe.Parse(FALSE,FALSE);

    // try to get prog id
    if (!CGUIDStringConvert::GetClassProgId(&pHookedClass->Clsid,psz,MAX_PATH))
    {
        // in case of failure, try to get class name
        if(!CGUIDStringConvert::GetClassName(&pHookedClass->Clsid,psz,MAX_PATH))
            // in case of failure show CLSID
            CGUIDStringConvert::TcharFromCLSID(&pHookedClass->Clsid,psz);
    }

    // add clsid has treeview root node
    htRoot=this->pTreeview->Insert(NULL,psz);


    // add dll name of class as first treeview node
    _tcscpy(psz,_T("Dll name: "));
    _tcscat(psz,AssociatedModuleName);
    this->pTreeview->Insert(htRoot,psz);

    // add Interface Name
    // convert IID to string
    CGUIDStringConvert::TcharFromIID(pIid,psz);
    // try to get Interface name
    CGUIDStringConvert::GetInterfaceName(psz,psz,MAX_PATH);
    // add to treeview
    htParsedInterface=this->pTreeview->Insert(htRoot,psz);

    // add VTBL address
    pHookedInterface=pHookedClass->GetHookedInterface(pInterfaceAssociatedToIID,pIid);
    if (!pHookedInterface) // can occurs if no monitoring file is specified
    {
        // get pointer to the IUnknown representation of the interface
        // as there VTBL are the same, and we only want to retrieve VTBL
        Iid=IID_IUnknown;
        pHookedInterface=pHookedClass->GetHookedInterface(pInterfaceAssociatedToIID,&Iid);
    }
    if (pHookedInterface)
    {
        htVTBLAddress=this->pTreeview->Insert(htParsedInterface,_T("VTBL Address"));
        this->AddAddressToTreeView(htVTBLAddress,pHookedInterface->VTBLAddress,&Pe);
        this->pTreeview->Expand(htVTBLAddress);
    }

    // for each interface
    for (pItemInterface=pHookedClass->pLinkListHookedInterfaces->Head;pItemInterface;pItemInterface=pItemInterface->NextItem)
    {
        

        pHookedInterface=(CHookedInterface*)pItemInterface->ItemData;
        bIDispatchParsed=FALSE;
        htInterface=htRoot;

        if (IsEqualIID(pHookedInterface->Iid,*pIid))
        {
            htInterface=htParsedInterface;
        }
        else
        {

            // convert iid to string
            CGUIDStringConvert::TcharFromIID(&pHookedInterface->Iid,psz);
            // try to get interface name
            CGUIDStringConvert::GetInterfaceName(psz,psz,MAX_PATH);

            // if current interface is not derived from IDispatch, but class support IDispatch
            // add IDispatch to root node instead of currently parsed interface node
            if (pHookedClass->bIDispatchParsingSuccessFull && pHookedClass->pInterfaceExposedByIDispatch)
            {
                if (IsEqualIID(pHookedInterface->Iid,pHookedClass->pInterfaceExposedByIDispatch->Iid))
                {
                    bIDispatchParsed=TRUE;

                    // insert IDispatch interface to Root
                    _tcscat(psz,_T(" (Parsed by IDispatch)"));
                    htIDispatch=this->pTreeview->Insert(htRoot,psz);

                    // add Interface VTBL Address
                    htVTBLAddress=this->pTreeview->Insert(htIDispatch,_T("VTBL Address"));
                    this->AddAddressToTreeView(htVTBLAddress,pHookedInterface->VTBLAddress,&Pe);
                    this->pTreeview->Expand(htVTBLAddress);

                    htInterface=htIDispatch;
                }
            }

            if (!bIDispatchParsed)
            {
                // insert interface to treeview
                htInterface=this->pTreeview->Insert(htParsedInterface,psz);

                //// pLinkListHookedInterfaces contains Interface to parse, IDispatch,
                //// and potentially IUnknown in case of Interface to parse error, and in this case IUnknown has the same VTBL Address
                //// so VTBL address should not be added
                //// add Interface VTBL Address
                //htVTBLAddress=this->pTreeview->Insert(htInterface,_T("VTBL Address"));
                //this->AddAddressToTreeView(htVTBLAddress,pHookedInterface->VTBLAddress,&Pe);
            }
        }

        pIIDDisplayTreeInfo=new CIIDDisplayTreeInfo(htInterface,&pHookedInterface->Iid);
        
        // for each method
        for (pItemMethodInfo=pHookedInterface->pMethodInfoList->Head;pItemMethodInfo;pItemMethodInfo=pItemMethodInfo->NextItem)
        {

            ////////////////////////////////////////
            // Insert/Get base interfaces tree
            ////////////////////////////////////////

            pMethodInfo=(CMethodInfo*)pItemMethodInfo->ItemData;
            if (IsBadReadPtr(pMethodInfo,sizeof(CMethodInfo)))
                continue;
            pDerivedIIDDisplayTreeInfo=pIIDDisplayTreeInfo;
            pNextDerivedIIDDisplayTreeInfo=pDerivedIIDDisplayTreeInfo;
            htDerivedInterface=htInterface;

            // for each base interface
            pMethodInfo->pLinkListOfBaseInterfacesID->Lock();
            for (pItemIID=pMethodInfo->pLinkListOfBaseInterfacesID->Head;pItemIID;pItemIID=pItemIID->NextItem)
            {
                pDerivedIID=(IID*)pItemIID->ItemData;

                // get handle to matching interface tree item
                pNextDerivedIIDDisplayTreeInfo=pNextDerivedIIDDisplayTreeInfo->GetTreeItemForIID(pDerivedIID);
                
                if (pNextDerivedIIDDisplayTreeInfo==NULL)
                {
                    // convert iid to string
                    CGUIDStringConvert::TcharFromIID(pDerivedIID,psz);
                    // try to get interface name
                    CGUIDStringConvert::GetInterfaceName(psz,psz,MAX_PATH);

                    // insert interface to treeview
                    htDerivedInterface=this->pTreeview->Insert(htDerivedInterface,psz);
                    this->pTreeview->Expand(htDerivedInterface);

                    // if interface is IDispatch
                    if (IsEqualIID(*pDerivedIID,IID_IDispatch))
                    {
                        // store IDispatch hTreeItem pointer
                        if (!htIDispatch)
                            htIDispatch=htDerivedInterface;
                    }

                    // add new CIIDDisplayTreeInfo object to store association hTreeItem<->IID
                    pNextDerivedIIDDisplayTreeInfo=new CIIDDisplayTreeInfo(htDerivedInterface,pDerivedIID);
                    pDerivedIIDDisplayTreeInfo->pLinkListDerivedIID->AddItem(pNextDerivedIIDDisplayTreeInfo);

                    // update current pDerivedIIDDisplayTreeInfo
                    pDerivedIIDDisplayTreeInfo=pNextDerivedIIDDisplayTreeInfo;

                }
                // update current htDerivedInterface
                htDerivedInterface=pNextDerivedIIDDisplayTreeInfo->hTreeIID;
            }
            pMethodInfo->pLinkListOfBaseInterfacesID->Unlock();


            ////////////////////////////////////////
            // Insert method name
            ////////////////////////////////////////

            *psz=0;
            if (bIDispatchParsed)
                pMethodInfo->ToString(TRUE,psz);
            else
            {
                if (pMethodInfo->Name)
                {
#if (defined(UNICODE)||defined(_UNICODE))
                    _tcscat(psz,pMethodInfo->Name);
#else
                    Size=_tcslen(psz);
                    CAnsiUnicodeConvert::UnicodeToAnsi(pMethodInfo->Name,&psz[Size],MAX_PATH-Size);
#endif
                }
            }

            htMethod=this->pTreeview->Insert(htDerivedInterface,psz);

            // add function address
            this->AddAddressToTreeView(htMethod,pMethodInfo->Address,&Pe);

            // VTBL Infos

            // 1) add VTBL Index
            if (pMethodInfo->CanBeHookedByVTBL())
            {
                htAddressInVTBL=this->pTreeview->Insert(htMethod,_T("Address In VTBL"));

                _stprintf(psz,_T("VTBL Index: %u"),pMethodInfo->VTBLIndex);
                this->pTreeview->Insert(htAddressInVTBL,psz);

                // 2) VTLBAddress
                this->AddAddressToTreeView(htAddressInVTBL,pMethodInfo->VTBLAddress,&Pe);

            }
        }

        ////////////////////////////////////////
        // Insert property name
        ////////////////////////////////////////
        if (bIDispatchParsed)
        {
            if (pHookedInterface->pPropertyInfoList->GetItemsCount()>0)
            {
                // insert "Properties" node to treeview
                htProperties=this->pTreeview->Insert(htInterface,TVI_FIRST,_T("Properties"));

                // for each property
                pHookedInterface->pPropertyInfoList->Lock();
                for (pItemPropertyInfo=pHookedInterface->pPropertyInfoList->Head;pItemPropertyInfo;pItemPropertyInfo=pItemPropertyInfo->NextItem)
                {
                    pPropertyInfo=(CPropertyInfo*)pItemPropertyInfo->ItemData;

                    // get property type
                    _tcscpy(psz,CSupportedParameters::GetParamName(pPropertyInfo->GetWinAPIOverrideType()));
                    if (pPropertyInfo->Name)
                    {
                        _tcscat(psz,_T(" "));
                    // get property name
#if (defined(UNICODE)||defined(_UNICODE))
                        _tcscat(psz,pPropertyInfo->Name);
#else
                        Size=_tcslen(psz);
                        CAnsiUnicodeConvert::UnicodeToAnsi(pPropertyInfo->Name,&psz[Size],MAX_PATH-Size);
#endif
                    }
                    // add property to treeview
                    this->pTreeview->Insert(htProperties,psz);
                }
                pHookedInterface->pPropertyInfoList->Unlock();
                // expand properties treeview node
                this->pTreeview->Expand(htProperties);
            }
        }

        // expand all interfaces and sub interfaces
        pIIDDisplayTreeInfo->ExpandAllSubItems(this->pTreeview);
        delete pIIDDisplayTreeInfo;
    }
    this->pTreeview->Expand(htParsedInterface);// in case of default (IUnknown) parsing substitution
    this->pTreeview->Expand(this->pTreeview->GetRoot());
    
}

//-----------------------------------------------------------------------------
// Name: ParseObjectMethodAddress
// Object: create object, parse and display object methods addresses
// Parameters :
//     in  : CLSID* pClsid : class identifier
//           IID* pIid : interface ID
//           BOOL bTryIDispatchParsing : TRUE if IDispatch parsing should be tried
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CComShowMethodsAddress::ParseObjectMethodAddress(CLSID* pClsid,IID* pIid,BOOL bTryIDispatchParsing)
{
    IUnknown* pObject;
    HRESULT hResult=CoCreateInstance(*pClsid,
                                    NULL,
                                    CLSCTX_ALL,
                                    *pIid,
                                    (void**)&pObject
                                    );
    if (FAILED(hResult))
    {
        MessageBox(this->hWndDialog,_T("Error creating object"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    CHookedClass* pHookedClass;
    
    pHookedClass=new CHookedClass(pClsid);
    CHookedObject* pHookedObject=new CHookedObject(pObject,pIid);

    // get associated dll module handle
    this->FindAssociatedModuleHandle(*(PBYTE*)pObject);

    // parse interface
    if (!this->ParseInterfaceDefinitionFile(pHookedClass,pIid,pObject,pHookedObject,pIid,NULL))
        return;

    // if IDispatch parsing queried
    if (bTryIDispatchParsing)
        pHookedClass->ParseIDispatch(pObject,pIid);

    this->DisplayObjectMethodAddress(pHookedClass,pObject,pIid);

    // release created object
    CSecureIUnknown::Release(pObject);
    delete pHookedObject;
    delete pHookedClass;
}

//-----------------------------------------------------------------------------
// Name: ParseInterfaceDefinitionFile
// Object: parse an interface monitoring definition file to get VTBL index of functions
// Parameters :
//     in  : CHookedClass* pHookedClass : hooked class object
//           IID* pIid : interface ID
//           IUnknown* pInterfaceAssociatedToIID : pointer to associated interface
//           CHookedObject* pHookedObject : pointer to hooked object
//           IID* pFileIid : Interface id of monitoring file to be parsed (can be a base interface of pIid)
//           CLinkList* pLinkListOfBaseInterfacesID : liked list of base interfaces
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CComShowMethodsAddress::ParseInterfaceDefinitionFile(CHookedClass* pHookedClass,
                                                          IID* pIid,
                                                          IUnknown* pInterfaceAssociatedToIID,
                                                          CHookedObject* pHookedObject,
                                                          IID* pFileIid,
                                                          CLinkList* pLinkListOfBaseInterfacesID
                                                          )
{
    TCHAR pszFileName[MAX_PATH];
    IID Iid=*pIid;
    SHOWMETHODSADDRESS_PARSE_COM_MONITORING_FILE_LINE_PARAM Param;
    if (!GetMonitoringFileName(pFileIid,pszFileName))
        return FALSE;

    if(!CStdFileOperations::DoesFileExists(pszFileName))
    {
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("Interface definition file\r\n%s not found"),pszFileName);
        MessageBox(this->hWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);

        // try to get the IUnknown monitoring file
        Iid=IID_IUnknown;
        if (!GetMonitoringFileName(&Iid,pszFileName))
            return FALSE;
        if(!CStdFileOperations::DoesFileExists(pszFileName))
            return FALSE;
    }

    Param.pComShowMethodsAddress=this;
    Param.pHookedClass=pHookedClass;
    Param.pIid=&Iid;
    Param.pInterfaceAssociatedToIID=pInterfaceAssociatedToIID;
    Param.pHookedObject=pHookedObject;
    Param.pLinkListOfBaseInterfacesID=pLinkListOfBaseInterfacesID;

    // parse monitoring file
    return CTextFile::ParseLines(pszFileName,
                        CComShowMethodsAddress::ParseCOMMonitoringFileLine,
                        &Param);
}

//-----------------------------------------------------------------------------
// Name: ParseCOMMonitoringFileLine
// Object: COM monitoring file line parser
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CComShowMethodsAddress::ParseCOMMonitoringFileLine(TCHAR* pszFileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam)
{
    CMethodInfo* pMethodInfo;
    TCHAR* pszFunctionDefinition;
    TCHAR* psz;
    HOOK_DEFINITION_INFOS HookDefinitionInfos;
    BOOL bAlreadyExisting;

    SHOWMETHODSADDRESS_PARSE_COM_MONITORING_FILE_LINE_PARAM* pParam;

    pParam=(SHOWMETHODSADDRESS_PARSE_COM_MONITORING_FILE_LINE_PARAM*)UserParam;


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
        CLinkList* mpLinkListOfBaseInterfacesID;
        if (pParam->pLinkListOfBaseInterfacesID)
            mpLinkListOfBaseInterfacesID=pParam->pLinkListOfBaseInterfacesID;
        else
            mpLinkListOfBaseInterfacesID=new CLinkList(sizeof(IID));

        CLinkListItem* pItem=mpLinkListOfBaseInterfacesID->AddItem(&HookDefinitionInfos.InterfaceID);
        pParam->pComShowMethodsAddress->ParseInterfaceDefinitionFile(pParam->pHookedClass,
                                                                     pParam->pIid,
                                                                     pParam->pInterfaceAssociatedToIID,
                                                                     pParam->pHookedObject,
                                                                     &HookDefinitionInfos.InterfaceID,
                                                                     mpLinkListOfBaseInterfacesID
                                                                    );
        mpLinkListOfBaseInterfacesID->RemoveItem(pItem);
        if (!pParam->pLinkListOfBaseInterfacesID)
            delete mpLinkListOfBaseInterfacesID;

        // continue parsing
        return TRUE;
    }

    if (!pszFunctionDefinition)
    {
        return ReportParsingError(pszFileName,dwLineNumber);
    }

    // get function name (avoid HookComInfos.ParseFunctionDescription to avoid to load APIOverride dll)
    // search end of function name
    psz=_tcschr(pszFunctionDefinition, '(');
    if (psz)
        *psz=0;
    // remove return type in function name if any or WINAPI, __cdecl keywords 
    // search last space in function name
    psz=_tcsrchr(pszFunctionDefinition,' ');
    // keep only data after last space
    if (psz)
    {
        psz++;// point to char after space
        pszFunctionDefinition=psz;
    }


    // get method info (this will auto add pMethodInfo object to matching interface)
    if (!pParam->pHookedClass->GetMethodInfoForHook(pParam->pHookedObject,
                                                    pParam->pInterfaceAssociatedToIID,
                                                    pParam->pLinkListOfBaseInterfacesID,
                                                    &HookDefinitionInfos,
                                                    pszFunctionDefinition,
                                                    pszFileName,
                                                    dwLineNumber,
                                                    HOOK_TYPE_MONITORING,
                                                    TRUE,// act like auto hooking
                                                    &pMethodInfo,
                                                    &bAlreadyExisting))
    {
        // continue parsing
        return TRUE;
    }

    // copy api name into pMethodInfo
    pMethodInfo->SetName(pszFunctionDefinition);

    // continue parsing
    return TRUE;
}