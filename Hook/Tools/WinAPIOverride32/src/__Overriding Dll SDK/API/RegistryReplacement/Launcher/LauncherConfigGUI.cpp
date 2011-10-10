/*
Copyright (C) 2010 Jacquelin POTIER <jacquelin.potier@free.fr>
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

#include "LauncherConfigGUI.h"
#include "resource.h"
#include <stdio.h>

#include "../../../../../Tools/file/StdFileOperations.h"

extern int LaunchApplicationWithEmulation( CLauncherOptions* pLauncherOptions,BOOL bSpyModeOnly);

BOOL CLauncherConfigGUI::Show(HINSTANCE hInstance,IN OUT CLauncherOptions* pLauncherOptions)
{
    CLauncherConfigGUI Dialog;
    Dialog.pLauncherOptions = pLauncherOptions;
    INT_PTR DialogRet = Dialog.CDialogBase::Show(hInstance,NULL,IDD_DIALOG_APP,IDI_ICON_APP);
    return (DialogRet == 1);
}



void CLauncherConfigGUI::OnOk()
{
    if (this->GetAndCheckOptions(FALSE))
        this->Close(1); // close the dialog : this will create .ini file and launch target with emulated registry
}

BOOL CLauncherConfigGUI::GetAndCheckOptions(BOOL bSpyMode)
{
    this->GetDlgItemText(IDC_EDIT_TARGET_NAME,this->pLauncherOptions->TargetName,MAX_PATH);
    this->GetDlgItemText(IDC_EDIT_TARGET_COMMAND_LINE,this->pLauncherOptions->TargetCommandLine,MAX_PATH);
    this->GetDlgItemText(IDC_EDIT_EMULATED_REGISTRY_FILE,this->pLauncherOptions->EmulatedRegistryConfigFile,MAX_PATH);

    this->GetDlgItemText(IDC_EDIT_FILTERING_FILE,this->pLauncherOptions->FilteringTypeFileName,MAX_PATH);

    if (this->IsDlgButtonChecked(IDC_RADIO_BASE_MODULE))
    {
        this->pLauncherOptions->FilteringType = FilteringType_ONLY_BASE_MODULE;
    }
    else if (this->IsDlgButtonChecked(IDC_RADIO_FILTER_INCLUDE))
    {
        this->pLauncherOptions->FilteringType = FilteringType_INCLUDE_ONLY_SPECIFIED;
    }
    else if (this->IsDlgButtonChecked(IDC_RADIO_FILTER_EXCLUDE))
    {
        this->pLauncherOptions->FilteringType = FilteringType_INCLUDE_ALL_BUT_SPECIFIED;
    }

    return this->pLauncherOptions->CheckConsistency(bSpyMode);
}

void CLauncherConfigGUI::OnSpy()
{
    if (this->GetAndCheckOptions(TRUE))
    {
        this->Hide();
        int iRes = LaunchApplicationWithEmulation( this->pLauncherOptions,TRUE);
        this->CDialogBase::Show();

        // in case of success, auto launch Registry editor with spy file loaded
        if (iRes ==0)
        {
            TCHAR CommandLine[3*MAX_PATH];
            TCHAR EmulatedRegistryEditorPath[MAX_PATH];
            TCHAR RegistryPath[MAX_PATH];
            CStdFileOperations::GetAppPath(EmulatedRegistryEditorPath,MAX_PATH);
            _tcscat(EmulatedRegistryEditorPath,CLauncherConfigGUI_EMULATED_REGISTRY_EDITOR);
            
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // warning use the same algorithm as defined in RegistryReplacement.dll to forge output spying file name
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////
            CStdFileOperations::GetAbsolutePath(this->pLauncherOptions->EmulatedRegistryConfigFile,RegistryPath);
            // change file extension from ".xml" to ".spy.xml" (try to avoid users to use spy file as portable registry)
            if (_tcslen(RegistryPath)+_tcslen(_T(".spy"))<MAX_PATH)
            {
                TCHAR* psz = _tcsrchr(RegistryPath,'.');
                if (psz)
                {
                    // point after '.'
                    psz++;

                    // store new extension
                    _tcscpy(psz,_T("spy.xml"));
                }
                else // file with no extension
                    _tcscat(RegistryPath,_T(".spy.xml"));
            }
            
            
            _sntprintf(CommandLine,3*MAX_PATH,_T("\"%s\" File=\"%s\""),EmulatedRegistryEditorPath,RegistryPath);

            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            memset( &si,0, sizeof(si) );
            si.cb = sizeof(si);
            memset( &pi,0, sizeof(pi) );
            if (!::CreateProcess(0,CommandLine,0,0,0,0,0,0,&si,&pi))
            {
                TCHAR Msg[2*MAX_PATH];
                _stprintf(Msg,_T("Error creating process %s"),EmulatedRegistryEditorPath);
                this->ReportError(Msg);
            }


        }
    }
}

void CLauncherConfigGUI::OnInit()
{
    this->EnableDragAndDrop(TRUE);

    this->SetDlgItemText(IDC_EDIT_TARGET_NAME,this->pLauncherOptions->TargetName);
    this->SetDlgItemText(IDC_EDIT_TARGET_COMMAND_LINE,this->pLauncherOptions->TargetCommandLine);
    this->SetDlgItemText(IDC_EDIT_EMULATED_REGISTRY_FILE,this->pLauncherOptions->EmulatedRegistryConfigFile);

    this->SetDlgItemText(IDC_EDIT_FILTERING_FILE,this->pLauncherOptions->FilteringTypeFileName);
    switch (this->pLauncherOptions->FilteringType)
    {
    case FilteringType_ONLY_BASE_MODULE:
        this->SetDlgButtonCheckState(IDC_RADIO_BASE_MODULE,TRUE);
    	break;
    case FilteringType_INCLUDE_ONLY_SPECIFIED:
        this->SetDlgButtonCheckState(IDC_RADIO_FILTER_INCLUDE,TRUE);
        break;
    case FilteringType_INCLUDE_ALL_BUT_SPECIFIED:
        this->SetDlgButtonCheckState(IDC_RADIO_FILTER_EXCLUDE,TRUE);
        break;
    }
}

void CLauncherConfigGUI::OnCommand(WPARAM wParam,LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case IDC_BUTTON_BROWSE_TARGET_NAME:
        this->OnBrowse(IDC_EDIT_TARGET_NAME, _T("exe\0*.exe\0All\0*.*\0"));
        break;
    case IDC_BUTTON_BROWSE_FOR_REGISTRY_FILE:
        this->OnBrowse(IDC_EDIT_EMULATED_REGISTRY_FILE, _T("xml\0*.xml\0All\0*.*\0"));
        break;
    case IDC_BUTTON_BROWSE_FILTERING_FILE:
        this->OnBrowse(IDC_EDIT_FILTERING_FILE, _T("txt\0*.txt\0All\0*.*\0"));
        break;
    case IDC_RADIO_FILTER_EXCLUDE:
        this->SetDlgItemText(IDC_EDIT_FILTERING_FILE,CLauncherConfigGUI_DEFAULT_FILTER_EXCLUDE_FILE);
        break;
    case IDC_RADIO_FILTER_INCLUDE:
        this->SetDlgItemText(IDC_EDIT_FILTERING_FILE,CLauncherConfigGUI_DEFAULT_FILTER_INCLUDE_FILE);
        break;
    case IDC_BUTTON_SPY:
        this->OnSpy();
        break;
    case IDC_BUTTON_CREATE_REGISTRY_FILE:
        this->OnCreateRegistry();
        break;
    case IDOK:
        this->OnOk();
        break;
    case IDCANCEL:
        this->Close(0);
        break;
    }
}

void CLauncherConfigGUI::OnCreateRegistry()
{
    TCHAR EmulatedRegistryCreatorPath[MAX_PATH];
    CStdFileOperations::GetAppPath(EmulatedRegistryCreatorPath,MAX_PATH);
    _tcscat(EmulatedRegistryCreatorPath,CLauncherConfigGUI_EMULATED_REGISTRY_CREATOR);
    if (!CStdFileOperations::DoesDirectoryExists(EmulatedRegistryCreatorPath))
    {
        TCHAR Msg[2*MAX_PATH];
        _stprintf(Msg,_T("File %s not found"),EmulatedRegistryCreatorPath);
        this->ReportError(Msg);
        return;
    }
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    memset( &si,0, sizeof(si) );
    si.cb = sizeof(si);
    memset( &pi,0, sizeof(pi) );
    if (!::CreateProcess(EmulatedRegistryCreatorPath,0,0,0,0,0,0,0,&si,&pi))
    {
        TCHAR Msg[2*MAX_PATH];
        _stprintf(Msg,_T("Error creating process %s"),EmulatedRegistryCreatorPath);
        this->ReportError(Msg);
    }
}

void CLauncherConfigGUI::OnBrowse(int EditId,TCHAR* Filter)
{
    TCHAR FileName[MAX_PATH];
    *FileName=0;
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=this->GetControlHandle();
    ofn.hInstance=this->GetInstance();
    ofn.lpstrFilter= Filter;
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    ofn.lpstrFile=FileName;
    ofn.nMaxFile=MAX_PATH;
    ofn.lpstrTitle=_T("Select File");

    // get file name
    if (::GetOpenFileName(&ofn))
    {
        this->SetDlgItemText(EditId,FileName);
    }
}

void CLauncherConfigGUI::OnDropFiles(WPARAM wParam,LPARAM lParam)
{
    HDROP hDrop= (HDROP)wParam;
    TCHAR pszFileName[MAX_PATH];
    POINT pt;
    HWND SubWindowHandle;

    // retrieve dialog subitem window handle
    DragQueryPoint(hDrop, &pt);
    ClientToScreen(this->GetControlHandle(),&pt);
    SubWindowHandle=WindowFromPoint(pt);

    ::DragQueryFile(hDrop, 0,pszFileName, MAX_PATH);

    if (SubWindowHandle==this->GetDlgItem(IDC_EDIT_TARGET_NAME))
    {
        this->SetDlgItemText(IDC_EDIT_TARGET_NAME,pszFileName);
    }
    else if (SubWindowHandle==this->GetDlgItem(IDC_EDIT_EMULATED_REGISTRY_FILE))
    {
        this->SetDlgItemText(IDC_EDIT_EMULATED_REGISTRY_FILE,pszFileName);
    }
    else if (SubWindowHandle==this->GetDlgItem(IDC_EDIT_FILTERING_FILE))
    {
        this->SetDlgItemText(IDC_EDIT_FILTERING_FILE,pszFileName);
    }

    ::DragFinish(hDrop);
}