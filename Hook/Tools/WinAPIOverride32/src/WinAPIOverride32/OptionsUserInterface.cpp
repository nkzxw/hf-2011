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
// Object: manages the processes filters dialog
//-----------------------------------------------------------------------------

#include "OptionsUserInterface.h"

COptionsUI::COptionsUI(COptions* pOptions)
{
    this->pOptions=pOptions;
    this->hWndTab=NULL;
    this->hWndDialog=NULL;
    this->pTabControl = NULL;
}
COptionsUI::~COptionsUI()
{
    
}

//-----------------------------------------------------------------------------
// Name: Show
// Object: show the dialog
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
INT_PTR COptionsUI::Show(HINSTANCE hInstance,HWND hWndDialog)
{
    this->hInstance=hInstance;
    // show dialog
    return DialogBoxParam(hInstance,(LPCTSTR)IDD_DIALOG_OPTIONS,hWndDialog,(DLGPROC)COptionsUI::WndProc,(LPARAM)this);
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void COptionsUI::Init(HWND hWndDialog)
{
#define COptionsUI_SpaceBetweenControls 5

    RECT Rect;
    RECT OptionDialogRect;
    int TabIndex;

    // store dialog handle
    this->hWndDialog=hWndDialog;

    // add item to tab control
    this->hWndTab=GetDlgItem(this->hWndDialog,IDC_TAB_OPTIONS);

    this->pTabControl = new CTabControl(this->hWndDialog,this->hWndTab, 0 , 0 , COptionsUI_SpaceBetweenControls );
    HMODULE hCurrentModule = ::GetModuleHandle(NULL);
    TabIndex = this->pTabControl->AddTab(_T("General"),hCurrentModule,IDD_DIALOG_OPTIONS_TAB_GENERAL,(DLGPROC)COptionsUI::TabWndProc,(LPARAM)this);
    this->hWndGeneralOptions = this->pTabControl->GetTabItemWindowHandle(TabIndex);
    TabIndex = this->pTabControl->AddTab(_T("COM"),hCurrentModule,IDD_DIALOG_OPTIONS_TAB_COM,(DLGPROC)COptionsUI::TabWndProc,(LPARAM)this);
    this->hWndCOMOptions = this->pTabControl->GetTabItemWindowHandle(TabIndex);
    TabIndex = this->pTabControl->AddTab(_T("NET"),hCurrentModule,IDD_DIALOG_OPTIONS_TAB_NET,(DLGPROC)COptionsUI::TabWndProc,(LPARAM)this);
    this->hWndNETOptions = this->pTabControl->GetTabItemWindowHandle(TabIndex);

    HWND hButtons[2];
    hButtons[0] = ::GetDlgItem(this->hWndDialog,IDOK);
    hButtons[1] = ::GetDlgItem(this->hWndDialog,IDCANCEL);
    this->pTabControl->SetUnderButtonsAndAutoSizeDialog(hButtons,2,CTabControl::UNDER_BUTTONS_POSITION_HORIZONTALLY_CENTERED_ON_TAB_CONTROL);

    // compute center position
    GetParent(this->hWndDialog);
    GetWindowRect(GetParent(this->hWndDialog),&Rect);
    GetWindowRect(this->hWndDialog,&OptionDialogRect);
    // resize window
    SetWindowPos(this->hWndDialog,0,
                (Rect.right-Rect.left-(OptionDialogRect.right - OptionDialogRect.left))/2+Rect.left,
                (Rect.bottom-Rect.top-(OptionDialogRect.bottom - OptionDialogRect.top))/2+Rect.top,
                0,0,
                SWP_NOACTIVATE|SWP_NOSIZE);


    // load icon for buttons
    HICON hIconEdit=(HICON)LoadImage(this->hInstance,MAKEINTRESOURCE(IDI_ICON_EDIT),IMAGE_ICON,24,24,LR_DEFAULTCOLOR|LR_SHARED);
    SendDlgItemMessage(this->hWndCOMOptions,IDC_BUTTON_EDIT_COM_OBJECT_CREATION_FILE_DEFINITION,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIconEdit);
    SendDlgItemMessage(this->hWndCOMOptions,IDC_BUTTON_EDIT_NOT_HOOKED_CLSID,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIconEdit);
    SendDlgItemMessage(this->hWndCOMOptions,IDC_BUTTON_EDIT_HOOKED_CLSID,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIconEdit);

    // load current settings
    this->LoadSettings();
}
//-----------------------------------------------------------------------------
// Name: LoadSettings
// Object: save options
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void COptionsUI::LoadSettings()
{
    TCHAR psz[MAX_PATH];
    ///////////////////////////////////////
    // set interface according to options
    ///////////////////////////////////////
    if (this->pOptions->bLogCallStack)
        SendDlgItemMessage(this->hWndGeneralOptions,IDC_CHECK_GET_CALLSTACK,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);

    _itot(this->pOptions->CallStackEbpRetrievalSize/sizeof(PBYTE),psz,10);
    SendDlgItemMessage(this->hWndGeneralOptions,IDC_EDIT_CALL_STACK_PARAM_SIZE,(UINT) WM_SETTEXT,(WPARAM) 0,(LPARAM)psz);

    if (this->pOptions->BreakDialogDontBreakAPIOverrideThreads)
        SendDlgItemMessage(this->hWndGeneralOptions,IDC_CHECK_BREAK_DONT_STOP_AIOVERRIDE_THREADS,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);

    if (this->pOptions->bMonitoringFileDebugMode)
        SendDlgItemMessage(this->hWndGeneralOptions,IDC_CHECK_MONITORING_FILES_DEBUG_MODE,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);

    if (this->pOptions->bAllowTlsCallbackHooks)
        SendDlgItemMessage(this->hWndGeneralOptions,IDC_CHECK_ALLOW_TLS_CALLBACK_HOOK,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);

    switch(this->pOptions->AutoAnalysis)
    {
    case FIRST_BYTES_AUTO_ANALYSIS_NONE:
        SendDlgItemMessage(this->hWndGeneralOptions,IDC_RADIO_SECURE_BYTES_ANALYSIS,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
        break;
    case FIRST_BYTES_AUTO_ANALYSIS_SECURE:
        SendDlgItemMessage(this->hWndGeneralOptions,IDC_CHECK_DO_FIRST_BYTES_ANALYSIS,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
        SendDlgItemMessage(this->hWndGeneralOptions,IDC_RADIO_SECURE_BYTES_ANALYSIS,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
        break;
    case FIRST_BYTES_AUTO_ANALYSIS_INSECURE:
        SendDlgItemMessage(this->hWndGeneralOptions,IDC_CHECK_DO_FIRST_BYTES_ANALYSIS,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
        SendDlgItemMessage(this->hWndGeneralOptions,IDC_RADIO_INSECURE_BYTES_ANALYSIS,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
        break;
    }

    // com
    if (this->pOptions->ComOptions.bUseClsidFilter)
        SendDlgItemMessage(this->hWndCOMOptions,IDC_CHECK_USE_CLSID_FILTERS,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
    if (this->pOptions->ComOptions.CLSIDFilterType==COM_CLSID_FilterHookOnlySpecified)
        SendDlgItemMessage(this->hWndCOMOptions,IDC_RADIO_HOOK_ONLY_CLSID,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
    else
        SendDlgItemMessage(this->hWndCOMOptions,IDC_RADIO_DONT_HOOK_CLSID,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
    if (this->pOptions->ComOptions.IDispatchAutoMonitoring)
        SendDlgItemMessage(this->hWndCOMOptions,IDC_CHECK_IDISPATCH_AUTOMONITORING,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);    
    SetDlgItemText(this->hWndCOMOptions,IDC_EDIT_COM_OBJECT_CREATION_FILE_DEFINITION,this->pOptions->ComOptions.pszConfigFileComObjectCreationHookedFunctions);
    SetDlgItemText(this->hWndCOMOptions,IDC_EDIT_CLSID_NOT_HOOKED,this->pOptions->ComOptions.pszNotHookedFileName);
    SetDlgItemText(this->hWndCOMOptions,IDC_EDIT_HOOKED_CLSID,this->pOptions->ComOptions.pszOnlyHookedFileName);
    if (this->pOptions->ComOptions.QueryMethodToHookForInterfaceParsedByIDispatch)
        SendDlgItemMessage(this->hWndCOMOptions,IDC_CHECK_QUERY_METHOD_TO_HOOK,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
    if (this->pOptions->ComOptions.ReportCLSIDNotSupportingIDispatch)
        SendDlgItemMessage(this->hWndCOMOptions,IDC_CHECK_REPORT_CLSID_NOT_SUPPORTING_IDISPATCH,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
    if (this->pOptions->ComOptions.ReportHookedCOMObject)
        SendDlgItemMessage(this->hWndCOMOptions,IDC_CHECK_REPORT_HOOKED_COM_OBJECT,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
    if (this->pOptions->ComOptions.ReportIIDHavingNoMonitoringFileAssociated)
        SendDlgItemMessage(this->hWndCOMOptions,IDC_CHECK_REPORT_IID_WITHOUT_MONITORING_FILES,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
    if (this->pOptions->ComOptions.ReportUseNameInsteadOfIDIfPossible)
        SendDlgItemMessage(this->hWndCOMOptions,IDC_CHECK_USE_PROGID_INSTEAD_OF_CLSID,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);

    // NET
    if (this->pOptions->NetOptions.DisableOptimization)
        SendDlgItemMessage(this->hWndNETOptions,IDC_CHECK_DISABLE_NET_OPTIMIZATION,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
    if (this->pOptions->NetOptions.MonitorException)
        SendDlgItemMessage(this->hWndNETOptions,IDC_CHECK_MONITOR_NET_EXCEPTIONS,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
    if (this->pOptions->bNetAutoHookingEnabled)
        SendDlgItemMessage(this->hWndNETOptions,IDC_CHECK_ENABLE_NET_AUTO_HOOKING,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
    if (this->pOptions->NetOptions.EnableFrameworkMonitoring)
        SendDlgItemMessage(this->hWndNETOptions,IDC_CHECK_ENABLE_FRAMEWORK_MONITORING,(UINT) BM_SETCHECK,(WPARAM) BST_CHECKED,0);
    
}

//-----------------------------------------------------------------------------
// Name: SaveSettings
// Object: save options
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL COptionsUI::SaveSettings()
{
    BOOL bRet;
    this->pOptions->bLogCallStack=(IsDlgButtonChecked(this->hWndGeneralOptions,IDC_CHECK_GET_CALLSTACK)==BST_CHECKED);

    this->pOptions->CallStackEbpRetrievalSize=GetDlgItemInt(this->hWndGeneralOptions,IDC_EDIT_CALL_STACK_PARAM_SIZE,&bRet,FALSE)*sizeof(PBYTE);
    if (!bRet)
    {
        MessageBox(this->hWndGeneralOptions,_T("Bad Value for number of parameters"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    this->pOptions->BreakDialogDontBreakAPIOverrideThreads=(IsDlgButtonChecked(this->hWndGeneralOptions,IDC_CHECK_BREAK_DONT_STOP_AIOVERRIDE_THREADS)==BST_CHECKED);

    this->pOptions->bMonitoringFileDebugMode=(IsDlgButtonChecked(this->hWndGeneralOptions,IDC_CHECK_MONITORING_FILES_DEBUG_MODE)==BST_CHECKED);

    this->pOptions->bAllowTlsCallbackHooks=(IsDlgButtonChecked(this->hWndGeneralOptions,IDC_CHECK_ALLOW_TLS_CALLBACK_HOOK)==BST_CHECKED);

    if (IsDlgButtonChecked(this->hWndGeneralOptions,IDC_CHECK_DO_FIRST_BYTES_ANALYSIS)!=BST_CHECKED)
    {
        this->pOptions->AutoAnalysis=FIRST_BYTES_AUTO_ANALYSIS_NONE;
    }
    else
    {
        if (IsDlgButtonChecked(this->hWndGeneralOptions,IDC_RADIO_INSECURE_BYTES_ANALYSIS)==BST_CHECKED)
            this->pOptions->AutoAnalysis=FIRST_BYTES_AUTO_ANALYSIS_INSECURE;
        else
            this->pOptions->AutoAnalysis=FIRST_BYTES_AUTO_ANALYSIS_SECURE;
    }

    // com
    this->pOptions->ComOptions.bUseClsidFilter=(IsDlgButtonChecked(this->hWndCOMOptions,IDC_CHECK_USE_CLSID_FILTERS)==BST_CHECKED);
    
    if (IsDlgButtonChecked(this->hWndCOMOptions,IDC_RADIO_HOOK_ONLY_CLSID)==BST_CHECKED)
        this->pOptions->ComOptions.CLSIDFilterType=COM_CLSID_FilterHookOnlySpecified;
    else
        this->pOptions->ComOptions.CLSIDFilterType=COM_CLSID_FilterDontHookSpecified;
    this->pOptions->ComOptions.IDispatchAutoMonitoring=(IsDlgButtonChecked(this->hWndCOMOptions,IDC_CHECK_IDISPATCH_AUTOMONITORING)==BST_CHECKED);

    GetDlgItemText(this->hWndCOMOptions,IDC_EDIT_COM_OBJECT_CREATION_FILE_DEFINITION,this->pOptions->ComOptions.pszConfigFileComObjectCreationHookedFunctions,MAX_PATH);
    GetDlgItemText(this->hWndCOMOptions,IDC_EDIT_CLSID_NOT_HOOKED,this->pOptions->ComOptions.pszNotHookedFileName,MAX_PATH);
    GetDlgItemText(this->hWndCOMOptions,IDC_EDIT_HOOKED_CLSID,this->pOptions->ComOptions.pszOnlyHookedFileName,MAX_PATH);

    this->pOptions->ComOptions.QueryMethodToHookForInterfaceParsedByIDispatch=(IsDlgButtonChecked(this->hWndCOMOptions,IDC_CHECK_QUERY_METHOD_TO_HOOK)==BST_CHECKED);
    this->pOptions->ComOptions.ReportCLSIDNotSupportingIDispatch=(IsDlgButtonChecked(this->hWndCOMOptions,IDC_CHECK_REPORT_CLSID_NOT_SUPPORTING_IDISPATCH)==BST_CHECKED);
    this->pOptions->ComOptions.ReportHookedCOMObject=(IsDlgButtonChecked(this->hWndCOMOptions,IDC_CHECK_REPORT_HOOKED_COM_OBJECT)==BST_CHECKED);
    this->pOptions->ComOptions.ReportIIDHavingNoMonitoringFileAssociated=(IsDlgButtonChecked(this->hWndCOMOptions,IDC_CHECK_REPORT_IID_WITHOUT_MONITORING_FILES)==BST_CHECKED);
    this->pOptions->ComOptions.ReportUseNameInsteadOfIDIfPossible=(IsDlgButtonChecked(this->hWndCOMOptions,IDC_CHECK_USE_PROGID_INSTEAD_OF_CLSID)==BST_CHECKED);

    // NET
    this->pOptions->NetOptions.DisableOptimization=(IsDlgButtonChecked(this->hWndNETOptions,IDC_CHECK_DISABLE_NET_OPTIMIZATION)==BST_CHECKED);
    this->pOptions->NetOptions.MonitorException=(IsDlgButtonChecked(this->hWndNETOptions,IDC_CHECK_MONITOR_NET_EXCEPTIONS)==BST_CHECKED);
    this->pOptions->NetOptions.EnableFrameworkMonitoring=(IsDlgButtonChecked(this->hWndNETOptions,IDC_CHECK_ENABLE_FRAMEWORK_MONITORING)==BST_CHECKED);
    this->pOptions->bNetAutoHookingEnabled=(IsDlgButtonChecked(this->hWndNETOptions,IDC_CHECK_ENABLE_NET_AUTO_HOOKING)==BST_CHECKED);
    this->pOptions->Save();

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: close dialog
// Parameters :
//     in  : tagDlgRes DlgRes : dialog result
//     out :
//     return : 
//-----------------------------------------------------------------------------
void COptionsUI::Close(tagDlgRes DlgRes)
{
    if (this->pTabControl)
    {
        delete this->pTabControl;
        this->pTabControl = NULL;
    }
    EndDialog(this->hWndDialog,(INT_PTR) DlgRes);
}

// common wndproc for tab windows
LRESULT CALLBACK COptionsUI::TabWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            if (lParam)
            {
                COptionsUI* pOptionsUI=(COptionsUI*)lParam;
                SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pOptionsUI);
            }
        }
        break;
    case WM_COMMAND:
        {
            COptionsUI* pOptionsUI=((COptionsUI*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pOptionsUI)
                break;

            switch (LOWORD(wParam))
            {
            case IDC_BUTTON_BROWSE_COM_OBJECT_CREATION_FILE_DEFINITION:
                pOptionsUI->BrowseForFile(pOptionsUI->hWndCOMOptions,IDC_EDIT_COM_OBJECT_CREATION_FILE_DEFINITION);
                break;
            case IDC_BUTTON_BROWSE_CLSID_NOT_HOOKED:
                pOptionsUI->BrowseForFile(pOptionsUI->hWndCOMOptions,IDC_EDIT_CLSID_NOT_HOOKED);
                break;
            case IDC_BUTTON_BROWSE_HOOKED_CLSID:
                pOptionsUI->BrowseForFile(pOptionsUI->hWndCOMOptions,IDC_EDIT_HOOKED_CLSID);
                break;
            case IDC_BUTTON_EDIT_COM_OBJECT_CREATION_FILE_DEFINITION:
                pOptionsUI->EditFile(pOptionsUI->hWndCOMOptions,IDC_EDIT_COM_OBJECT_CREATION_FILE_DEFINITION);
                break;
            case IDC_BUTTON_EDIT_NOT_HOOKED_CLSID:
                pOptionsUI->EditFile(pOptionsUI->hWndCOMOptions,IDC_EDIT_CLSID_NOT_HOOKED);
                break;
            case IDC_BUTTON_EDIT_HOOKED_CLSID:
                pOptionsUI->EditFile(pOptionsUI->hWndCOMOptions,IDC_EDIT_HOOKED_CLSID);
                break;
            }
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog callback
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK COptionsUI::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            if (lParam) // common wnd proc for Ui and tab child windows
            {
                COptionsUI* pOptionsUI=(COptionsUI*)lParam;

                SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pOptionsUI);
                // load dlg icons
                CDialogHelper::SetIcon(hWnd,IDI_ICON_OPTIONS);

                pOptionsUI->Init(hWnd);
            }
        }
        break;
    case WM_CLOSE:
        {
            COptionsUI* pOptionsUI=((COptionsUI*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pOptionsUI)
                break;
            pOptionsUI->Close(COptionsUI::DLG_RES_CANCEL);
        }
        break;
    case WM_COMMAND:
        {
            COptionsUI* pOptionsUI=((COptionsUI*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pOptionsUI)
                break;

            switch (LOWORD(wParam))
            {
            case IDOK:
                // save filters settings
                if (pOptionsUI->SaveSettings())
                {
                    // exit
                    pOptionsUI->Close(COptionsUI::DLG_RES_OK);
                }
                break;
            case IDCANCEL:
                pOptionsUI->Close(COptionsUI::DLG_RES_CANCEL);
                break;
            }
        }
        break;
    case WM_NOTIFY: 
        {
            COptionsUI* pOptionsUI=((COptionsUI*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pOptionsUI)
                break;
            if (pOptionsUI->pTabControl)
            {
                if (pOptionsUI->pTabControl->OnNotify(wParam,lParam))
                    break;
            }
        }
        break; 
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: BrowseForFile
// Object: open browse dialog for file
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void COptionsUI::BrowseForFile(HWND hWnd,int Idd)
{
    TCHAR pszFile[MAX_PATH]=_T("");

    // open dialog
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=this->hWndDialog;
    ofn.hInstance=this->hInstance;
    ofn.lpstrFilter=_T("txt\0*.txt\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt=_T("txt");
    ofn.lpstrFile=pszFile;
    ofn.nMaxFile=MAX_PATH;

    if (!GetOpenFileName(&ofn))
        return;

    SetDlgItemText(hWnd,Idd,pszFile);
}

//-----------------------------------------------------------------------------
// Name: EditFile
// Object: open associated application for editing file
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void COptionsUI::EditFile(HWND hWnd,int Idd)
{
    TCHAR psz[MAX_PATH];
    GetDlgItemText(hWnd,Idd,psz,MAX_PATH);

    // check if full path is given
    if (!_tcschr(psz,'\\'))// search for a \ 
    {
        TCHAR pszPath[MAX_PATH];
        CStdFileOperations::GetAppPath(pszPath,MAX_PATH);
        _tcscat(pszPath,psz);
        _tcscpy(psz,pszPath);
    }

    if(!CStdFileOperations::DoesFileExists(psz))
    {
        TCHAR pszMsg[2*MAX_PATH];
        _sntprintf(pszMsg,2*MAX_PATH,_T("File %s not found"),psz);
        MessageBox(this->hWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // launch default .txt editor
    if (((int)ShellExecute(NULL,_T("edit"),psz,NULL,NULL,SW_SHOWNORMAL))<33)
    {
        MessageBox(this->hWndDialog,_T("Error launching default editor application"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
    }
}