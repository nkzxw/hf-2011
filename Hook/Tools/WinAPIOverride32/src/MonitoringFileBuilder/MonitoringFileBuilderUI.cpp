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
// Object: manages the monitoring file builder UI
//-----------------------------------------------------------------------------

#include "MonitoringFileBuilderUI.h"


//-----------------------------------------------------------------------------
// Name: CMonitoringFileBuilderUI
// Object: constructor
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
CMonitoringFileBuilderUI::CMonitoringFileBuilderUI()
{
    this->LogContentMaxSize=CMONITORINGFILEBUILDERUI_INITIAL_LOG_SIZE;
    this->pszLogContent=new TCHAR[this->LogContentMaxSize];
    *this->pszLogContent=0;

    this->LogWarningsAndErrorsMaxSize=CMONITORINGFILEBUILDERUI_INITIAL_LOG_SIZE;
    this->pszLogWarningsAndErrors=new TCHAR[this->LogWarningsAndErrorsMaxSize];
    *this->pszLogWarningsAndErrors=0;

    this->NbErrors=0;
    this->NbWarnings=0;

    this->pMonitoringFileBuilder=NULL;

    // if monitoring dir doesn't exists create it
    TCHAR pszDir[MAX_PATH];
    CStdFileOperations::GetAppPath(pszDir,MAX_PATH);
    _tcscat(pszDir,MONITORING_FILES_PATH);
    CStdFileOperations::CreateDirectory(pszDir);

    this->hIconCancel=NULL;
    this->pToolTipCancel=NULL;

    this->hIconWarningsAndErrors=NULL;
    this->pToolWarningsAndErrors=NULL;

}
CMonitoringFileBuilderUI::~CMonitoringFileBuilderUI()
{
    delete[] this->pszLogContent;
    delete[] this->pszLogWarningsAndErrors;
}

//-----------------------------------------------------------------------------
// Name: Show
// Object: show monitoring file builder dialog
// Parameters :
//     in  : HINSTANCE Instance : instance of module containing resources
//           HWND hWndParent : parent window handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::Show(HINSTANCE Instance,HWND hWndParent)
{
    // create object
    CMonitoringFileBuilderUI* pMonitoringFileBuilderUI=new CMonitoringFileBuilderUI();
    pMonitoringFileBuilderUI->hInstance=Instance;
    // show dialog
    DialogBoxParam(Instance, (LPCTSTR)IDD_DIALOG_MAIN,hWndParent, (DLGPROC)WndProc,(LPARAM)pMonitoringFileBuilderUI);
    delete pMonitoringFileBuilderUI;
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: Init monitoring file builder UI
// Parameters :
//     in  : HWND hWnd : window handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::Init(HWND hwnd)
{
    this->hWndDialog=hwnd;
    CAPIError::SetParentWindowHandle(hwnd);

    // set cancel button Tooltip
    pToolTipCancel=new CToolTip(GetDlgItem(this->hWndDialog,IDCANCEL),
                                _T("Cancel current operation"),
                                TRUE);

    // set cancel button icon
    this->hIconCancel=(HICON)LoadImage(this->hInstance,MAKEINTRESOURCE(IDI_ICON_CANCEL),IMAGE_ICON,24,24,LR_DEFAULTCOLOR|LR_SHARED);
    SendDlgItemMessage(this->hWndDialog,IDCANCEL,BM_SETIMAGE,IMAGE_ICON,(LPARAM)this->hIconCancel);


    // set warning and error button tooltip
    this->pToolWarningsAndErrors=new CToolTip(GetDlgItem(this->hWndDialog,IDC_BUTTON_WARNING_ERROR_MSG),
                                            _T("Display warings and errors"),
                                            TRUE);

    // set warning and error button icon
    this->hIconWarningsAndErrors=(HICON)LoadImage(this->hInstance,MAKEINTRESOURCE(IDI_ICON_WARNINGS_AND_ERRORS),IMAGE_ICON,24,24,LR_DEFAULTCOLOR|LR_SHARED);
    SendDlgItemMessage(this->hWndDialog,IDC_BUTTON_WARNING_ERROR_MSG,BM_SETIMAGE,IMAGE_ICON,(LPARAM)this->hIconWarningsAndErrors);

    // Disable Cancel button
    EnableWindow(GetDlgItem(this->hWndDialog,IDCANCEL),FALSE);

    // enable Online MSDN by default
    SendDlgItemMessage(this->hWndDialog,IDC_CHECK_ONLINE_MSDN,(UINT)BM_SETCHECK,BST_CHECKED,0);

    // enable trouble checking by default
    SendDlgItemMessage(this->hWndDialog,IDC_CHECK_CHECK_TROUBLES,(UINT)BM_SETCHECK,BST_CHECKED,0);

    // enable full generation by default
    SendDlgItemMessage(this->hWndDialog,IDC_RADIO_FULL_GENERATION,(UINT)BM_SETCHECK,BST_CHECKED,0);

    // enable full update by default
    SendDlgItemMessage(this->hWndDialog,IDC_RADIO_FULL_UPDATE,(UINT)BM_SETCHECK,BST_CHECKED,0);

    // get progress bar handle
    this->hProgressBar=GetDlgItem(this->hWndDialog,IDC_PROGRESS);

    // Check Call to guess nb params 
    // too dangerous to be put by default : blocking call, make app crash ...
    // SendDlgItemMessage(this->hWndDialog,IDC_CHECK_CALL_TO_GUESS_NB_PARAM,(UINT)BM_SETCHECK,BST_CHECKED,0);

    // accept drag and drop operation
    DragAcceptFiles(this->hWndDialog, TRUE);
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: close monitoring file builder dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::Close()
{
    DragAcceptFiles(this->hWndDialog, FALSE);
    if (this->hIconCancel)
    {
        DestroyIcon(this->hIconCancel);
        this->hIconCancel=NULL;
    }
    if (this->pToolTipCancel)
    {
        delete this->pToolTipCancel;
        this->pToolTipCancel=NULL;
    }
    if (this->hIconWarningsAndErrors)
    {
        DestroyIcon(this->hIconWarningsAndErrors);
        this->hIconWarningsAndErrors=NULL;
    }
    if (this->pToolWarningsAndErrors)
    {
        delete this->pToolWarningsAndErrors;
        this->pToolWarningsAndErrors=NULL;
    }
    // close dialog
    EndDialog(this->hWndDialog,0);
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: monitoring file builder dialog window proc
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CMonitoringFileBuilderUI::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetWindowLong(hWnd,GWLP_USERDATA,(LONG)lParam);
        // set Dialog icon
        CDialogHelper::SetIcon(hWnd,IDI_ICON_MONITORINGFILEBUILDER);
        // call the init method
        ((CMonitoringFileBuilderUI*)lParam)->Init(hWnd);
        break;
    case WM_CLOSE:
        {
            // get CMonitoringFileBuilderUI object
            CMonitoringFileBuilderUI* pMonitoringFileBuilderUI=(CMonitoringFileBuilderUI*)GetWindowLong(hWnd,GWLP_USERDATA);
            if (pMonitoringFileBuilderUI)
                // call the close method
                pMonitoringFileBuilderUI->Close();
            break;
        }
        break;
    case WM_COMMAND:
        {
            // get CMonitoringFileBuilderUI object
            CMonitoringFileBuilderUI* pMonitoringFileBuilderUI=(CMonitoringFileBuilderUI*)GetWindowLong(hWnd,GWLP_USERDATA);
            if (pMonitoringFileBuilderUI)
            {
                switch (LOWORD(wParam))
                {
                case IDCANCEL:
                    // Cancel generation
                    pMonitoringFileBuilderUI->CancelGenerate();
                    break;
                case IDC_BUTTON_GENERATE:
                    {
                        pMonitoringFileBuilderUI->bCheckOnly=FALSE;
                        BOOL bFull=(IsDlgButtonChecked(pMonitoringFileBuilderUI->hWndDialog,IDC_RADIO_FULL_GENERATION)==BST_CHECKED);
                        pMonitoringFileBuilderUI->bUpdate=FALSE;
                        // call the Generate method
                        pMonitoringFileBuilderUI->Generate(bFull);
                    }
                    break;
                case IDC_BUTTON_UPDATE:
                    {
                        if (pMonitoringFileBuilderUI->IsUpdateAllowed(TRUE))
                        {
                            pMonitoringFileBuilderUI->bCheckOnly=FALSE;
                            pMonitoringFileBuilderUI->bUpdate=TRUE;
                            // call the Generate method allowing partial generation
                            pMonitoringFileBuilderUI->Generate(FALSE);
                        }
                    }
                    break;
                case IDC_BUTTON_CHECK_ERROR:
                    {
                        if (pMonitoringFileBuilderUI->IsErrorCheckingAllowed(TRUE))
                        {
                            pMonitoringFileBuilderUI->bCheckOnly=TRUE;
                            pMonitoringFileBuilderUI->bUpdate=FALSE;
                            // call the Generate method
                            pMonitoringFileBuilderUI->Generate(TRUE);
                        }
                    }
                    break;
                case IDC_BUTTON_BROWSE_INPUT:
                    // call the BrowseInputFile method
                    pMonitoringFileBuilderUI->BrowseInputFile();
                    break;
                case IDC_BUTTON_BROWSE_OUTPUT:
                    // call the BrowseOutputFile method
                    pMonitoringFileBuilderUI->BrowseOutputFile();
                    break;
                    
                case IDC_BUTTON_BROWSE_OLD_MONITORING_FILE:
                    // call the BrowseOldMonitoringFile method
                    pMonitoringFileBuilderUI->BrowseOldMonitoringFile();
                    break;
                case IDC_CHECK_FIRST_BYTES_ANALYSIS:
                    // auto check IDC_CHECK_CHECK_TROUBLES if first bytes analysis is checked
                    if (IsDlgButtonChecked(pMonitoringFileBuilderUI->hWndDialog,IDC_CHECK_FIRST_BYTES_ANALYSIS)==BST_CHECKED)
                        SendDlgItemMessage(pMonitoringFileBuilderUI->hWndDialog,IDC_CHECK_CHECK_TROUBLES,BM_SETCHECK,BST_CHECKED,0);
                    break;
                case IDC_BUTTON_WARNING_ERROR_MSG:
                    {
                        CWarningsReportUI WReport(pMonitoringFileBuilderUI->NbErrors,
                                                    pMonitoringFileBuilderUI->NbWarnings,
                                                    pMonitoringFileBuilderUI->pszLogWarningsAndErrors);
                        WReport.Show(pMonitoringFileBuilderUI->hInstance,pMonitoringFileBuilderUI->hWndDialog);
                    }
                    break;
                case IDC_RADIO_PARSE_DEBUG_FILE:
                case IDC_RADIO_PARSE_NET:
                case IDC_RADIO_PARSE_COM_TYPE_LIBRARY:
                case IDC_RADIO_PARSE_COM_ALL_REGISTERED_TYPE_LIBRARY:
                    pMonitoringFileBuilderUI->EnableImportExportTableSpecificPanels(FALSE);
                    break;
                case IDC_RADIO_EXPORT_TABLE:
                case IDC_RADIO_IMPORT_TABLE:
                    pMonitoringFileBuilderUI->EnableImportExportTableSpecificPanels(TRUE);
                    break;
                }
            }
            break;
        }
    case WM_DROPFILES:
        {
            // get CMonitoringFileBuilderUI object
            CMonitoringFileBuilderUI* pMonitoringFileBuilderUI=(CMonitoringFileBuilderUI*)GetWindowLong(hWnd,GWLP_USERDATA);
            if (pMonitoringFileBuilderUI)
                pMonitoringFileBuilderUI->OnDropFile(hWnd, (HDROP)wParam);
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ClearLogs
// Object: clear logs text window
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::ClearLogs()
{
    // clear log buffer
    *this->pszLogContent=0;
    // clear log window
    SetDlgItemText(this->hWndDialog,IDC_EDIT_LOGS,this->pszLogContent);
}

//-----------------------------------------------------------------------------
// Name: IsErrorCheckingAllowed
// Object: check if error checking is allowed
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CMonitoringFileBuilderUI::IsErrorCheckingAllowed(BOOL ShowMsgIfNotAllowed)
{
    if ((IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_IMPORT_TABLE)==BST_CHECKED)
        || (IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_EXPORT_TABLE)==BST_CHECKED))
        return TRUE;

    if (ShowMsgIfNotAllowed)
        MessageBox(this->hWndDialog,_T("Error checking applies only for export or import table parsing"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
    return FALSE;
}
//-----------------------------------------------------------------------------
// Name: IsUpdateAllowed
// Object: check if update is allowed
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CMonitoringFileBuilderUI::IsUpdateAllowed(BOOL ShowMsgIfNotAllowed)
{
    if ((IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_IMPORT_TABLE)==BST_CHECKED)
        || (IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_EXPORT_TABLE)==BST_CHECKED))
        return TRUE;

    if (ShowMsgIfNotAllowed)
        MessageBox(this->hWndDialog,_T("Update applies only for export or import table parsing"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
    return FALSE;
}
//-----------------------------------------------------------------------------
// Name: CancelGenerate
// Object: cancel generation or update
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::CancelGenerate()
{
    if (this->pMonitoringFileBuilder)
        this->pMonitoringFileBuilder->CancelCurrentOperation();
}

//-----------------------------------------------------------------------------
// Name: Generate
// Object: generate monitoring file for export or import table
//         or update an existing monitoring file
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::Generate(BOOL FullGeneration)
{
    this->bFullGeneration=FullGeneration;
    // do generation in another thread to allow UI catch events
    CloseHandle(CreateThread(NULL,0,CMonitoringFileBuilderUI::GenerateThreadProc,this,0,NULL));
}

DWORD WINAPI CMonitoringFileBuilderUI::GenerateThreadProc(LPVOID lpParameter)
{
    CMonitoringFileBuilderUI* pMonitoringFileBuilderUI;
    TCHAR InputFile[MAX_PATH];
    TCHAR OutputFile[MAX_PATH];
    TCHAR pszOldMonitoringFile[MAX_PATH];
    BOOL bSuccess=FALSE;
    HANDLE hFile;
    UPDATE_WAY UpdateWay;

    // get CMonitoringFileBuilderUI object that use this thread
    pMonitoringFileBuilderUI=(CMonitoringFileBuilderUI*)lpParameter;

    BOOL bParseSingleCOMTypeLib = (IsDlgButtonChecked(pMonitoringFileBuilderUI->hWndDialog,IDC_RADIO_PARSE_COM_TYPE_LIBRARY)==BST_CHECKED);
    BOOL bParseAllCOMTypeLib = (IsDlgButtonChecked(pMonitoringFileBuilderUI->hWndDialog,IDC_RADIO_PARSE_COM_ALL_REGISTERED_TYPE_LIBRARY)==BST_CHECKED);

    // particular case : use of debug file information --> launch debug infos viewer
    if (IsDlgButtonChecked(pMonitoringFileBuilderUI->hWndDialog,IDC_RADIO_PARSE_DEBUG_FILE)==BST_CHECKED)
    {
        // get input text
        GetDlgItemText(pMonitoringFileBuilderUI->hWndDialog,IDC_EDIT_INPUT_FILE,InputFile,MAX_PATH);
        // if empty
        if (*InputFile==0)
        {
            MessageBox(pMonitoringFileBuilderUI->hWndDialog,_T("No input file selected"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            return 0;
        }
        // check if file exists
        if (!CStdFileOperations::DoesFileExists(InputFile))
        {
            TCHAR Msg[2*MAX_PATH];
            _sntprintf(Msg,2*MAX_PATH,_T("File %s not found"),InputFile);
            MessageBox(pMonitoringFileBuilderUI->hWndDialog,Msg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            return 0;
        }

        // forge debug info viewer full path
        CStdFileOperations::GetAppPath(OutputFile,MAX_PATH);
        _tcscat(OutputFile,DEBUG_INFOS_VIEWER_APPLICATION_NAME);
        STARTUPINFO StartupInfo;
        PROCESS_INFORMATION ProcessInformation;

        memset(&ProcessInformation,0,sizeof(PROCESS_INFORMATION));
        memset(&StartupInfo,0,sizeof(STARTUPINFO));
        StartupInfo.cb=sizeof(STARTUPINFO);
        TCHAR CmdLine[MAX_PATH+1];
        _tcscpy(CmdLine,_T(" "));
        _tcscat(CmdLine,InputFile);
        
        // launch debug info viewer to generate monitoring file
        if (!CreateProcess(OutputFile,CmdLine,0,0,0,0,0,0,&StartupInfo,&ProcessInformation))
        {
            TCHAR Msg[2*MAX_PATH];
            _sntprintf(Msg,2*MAX_PATH,_T("Error launching %s"),DEBUG_INFOS_VIEWER_APPLICATION_NAME);
            MessageBox(pMonitoringFileBuilderUI->hWndDialog,Msg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            return 0;
        }
        ::CloseHandle(ProcessInformation.hProcess);
        ::CloseHandle(ProcessInformation.hThread);
        return 0;
    }


    // empty pszOldMonitoringFile
    *pszOldMonitoringFile=0;

    // reset percent completed
    pMonitoringFileBuilderUI->CallBackPercentCompleted(0);

    // get update way
    BOOL bChecked=(IsDlgButtonChecked(pMonitoringFileBuilderUI->hWndDialog,IDC_RADIO_FULL_UPDATE)==BST_CHECKED);
    if (bChecked)
    {
        UpdateWay=UPDATE_FULL;
    }
    else
    {
        bChecked=(IsDlgButtonChecked(pMonitoringFileBuilderUI->hWndDialog,IDC_RADIO_UPDATE_BYTES_ANALYSIS)==BST_CHECKED);
        if (bChecked)
            UpdateWay=UPDATE_FIRSTBYTES;
        else // IDC_RADIO_UPDATE_REMOVE_BYTES_ANALYSIS is checked
            UpdateWay=UPDATE_REMOVEFIRSTBYTES;
            
    }

    // if we don't do an update or remove first bytes analysis
    if ( ( (!pMonitoringFileBuilderUI->bUpdate) || (UpdateWay==UPDATE_FULL) )
        // and not generating COM monitoring files for all registered types
         && (!bParseAllCOMTypeLib)
        )
    {
        // get input text
        GetDlgItemText(pMonitoringFileBuilderUI->hWndDialog,IDC_EDIT_INPUT_FILE,InputFile,MAX_PATH);
        // if empty
        if (*InputFile==0)
        {
            MessageBox(pMonitoringFileBuilderUI->hWndDialog,_T("No input file selected"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            return FALSE;
        }

        if (!bParseSingleCOMTypeLib)
        {
            // check if import or export table is checked
            if ((IsDlgButtonChecked(pMonitoringFileBuilderUI->hWndDialog,IDC_RADIO_IMPORT_TABLE)!=BST_CHECKED)
                && (IsDlgButtonChecked(pMonitoringFileBuilderUI->hWndDialog,IDC_RADIO_EXPORT_TABLE)!=BST_CHECKED)
                && (IsDlgButtonChecked(pMonitoringFileBuilderUI->hWndDialog,IDC_RADIO_PARSE_NET)!=BST_CHECKED)
                )
            {
                MessageBox(pMonitoringFileBuilderUI->hWndDialog,_T("Please select parsing type for generation (export, import or .NET)"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
                return FALSE;
            }
        }


    }

    // if we don't do only checking
    if ( (!pMonitoringFileBuilderUI->bCheckOnly)
         && (!bParseAllCOMTypeLib)
         && (!bParseSingleCOMTypeLib)
        )
    {

        // get output text
        GetDlgItemText(pMonitoringFileBuilderUI->hWndDialog,IDC_EDIT_OUTPUT_FILE,OutputFile,MAX_PATH);
        // if empty
        if (*OutputFile==0)
        {
            MessageBox(pMonitoringFileBuilderUI->hWndDialog,_T("No output file selected"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            return FALSE;
        }

        // query old monitoring filename
        if (pMonitoringFileBuilderUI->bUpdate)
        {
            GetDlgItemText(pMonitoringFileBuilderUI->hWndDialog,IDC_EDIT_OLD_MONITORING_FILE,pszOldMonitoringFile,MAX_PATH);
            // if empty
            if (*pszOldMonitoringFile==0)
            {
                MessageBox(pMonitoringFileBuilderUI->hWndDialog,_T("No old monitoring file selected"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
                return FALSE;
            }
        }

        CStdFileOperations::CreateDirectoryForFile(OutputFile);
        // if output file already exist ask to overwrite it
        hFile = CreateFile(OutputFile, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            // close file handle
            CloseHandle(hFile);

            // query user to overwrite
            if (MessageBox(pMonitoringFileBuilderUI->hWndDialog,_T("Warning output file already exists.\r\nDo you want to overwrite it ? "),_T("Warning"),MB_YESNO|MB_ICONWARNING|MB_TOPMOST)
                ==IDNO)
                return FALSE;
        }
    }

    // reset vars
    *pMonitoringFileBuilderUI->pszLogWarningsAndErrors=0;
    pMonitoringFileBuilderUI->NbErrors=0;
    pMonitoringFileBuilderUI->NbWarnings=0;

    // clear logs
    pMonitoringFileBuilderUI->ClearLogs();

    // create CMonitoringFileBuilder object
    CMonitoringFileBuilder MonitoringFileBuilder(pMonitoringFileBuilderUI->hInstance,pMonitoringFileBuilderUI->hWndDialog);

    // set this->pMonitoringFileBuilder
    pMonitoringFileBuilderUI->pMonitoringFileBuilder=&MonitoringFileBuilder;

    // set error and percent completed callbacks
    MonitoringFileBuilder.SetUserMessageInformationCallBack(CMonitoringFileBuilderUI::UserMessageInformationCallBackStatic,pMonitoringFileBuilderUI);
    MonitoringFileBuilder.SetPercentCompletedCallback(CMonitoringFileBuilderUI::CallBackPercentCompleted,pMonitoringFileBuilderUI);

    // update CMonitoringFileBuilder object from options
    pMonitoringFileBuilderUI->UpdateCMonitoringFileBuilderOptionsFromUI(&MonitoringFileBuilder);

    // set UI state in started mode
    pMonitoringFileBuilderUI->SetUIState(TRUE);

    if (bParseSingleCOMTypeLib)
    {
        bSuccess = MonitoringFileBuilder.GenerateCOMTypeLibraryMonitoring(InputFile);
    }
    else if (bParseAllCOMTypeLib)
    {
        bSuccess = MonitoringFileBuilder.GenerateCOMAllRegisteredTypeLibrariesMonitoring();
    }
    else if (IsDlgButtonChecked(pMonitoringFileBuilderUI->hWndDialog,IDC_RADIO_PARSE_NET)==BST_CHECKED)
    {
        bSuccess=MonitoringFileBuilder.GenerateNETMonitoring(InputFile,OutputFile,pMonitoringFileBuilderUI->bFullGeneration);
    }
    else if ((!pMonitoringFileBuilderUI->bUpdate)||(UpdateWay==UPDATE_FULL))
    {
        // call GenerateExportMonitoring or GenerateImportMonitoring depending IDC_RADIO_EXPORT_TABLE stat
        if (IsDlgButtonChecked(pMonitoringFileBuilderUI->hWndDialog,IDC_RADIO_EXPORT_TABLE)==BST_CHECKED)
            bSuccess=MonitoringFileBuilder.GenerateExportMonitoring(InputFile,
                                                                    OutputFile,
                                                                    pMonitoringFileBuilderUI->bFullGeneration,
                                                                    pMonitoringFileBuilderUI->bCheckOnly,
                                                                    pMonitoringFileBuilderUI->bUpdate,
                                                                    pszOldMonitoringFile);
        else
            bSuccess=MonitoringFileBuilder.GenerateImportMonitoring(InputFile,
                                                                    OutputFile,
                                                                    pMonitoringFileBuilderUI->bFullGeneration,
                                                                    pMonitoringFileBuilderUI->bCheckOnly,
                                                                    pMonitoringFileBuilderUI->bUpdate,
                                                                    pszOldMonitoringFile);

    }
    else
    {
        switch(UpdateWay)
        {
        case UPDATE_FIRSTBYTES:
            bSuccess=MonitoringFileBuilder.UpdateBytesAnalysis(pszOldMonitoringFile,OutputFile);
            break;

        case UPDATE_REMOVEFIRSTBYTES:
            bSuccess=MonitoringFileBuilder.RemoveBytesAnalysis(pszOldMonitoringFile,OutputFile);
            break;
        }
    }

    // display report with number of error and warnings
    TCHAR psz[MAX_PATH];
    TCHAR pszError[MAX_PATH];
    TCHAR pszWarning[MAX_PATH];

    _tcscpy(pszError,_T("error"));
    if (pMonitoringFileBuilderUI->NbErrors)
        _tcscat(pszError,_T("s"));

    _tcscpy(pszWarning,_T("warning"));
    if (pMonitoringFileBuilderUI->NbWarnings)
        _tcscat(pszWarning,_T("s"));

    _stprintf(psz,_T("\r\nOperation Completed : %u %s, %u %s\r\n"),pMonitoringFileBuilderUI->NbErrors,pszError,pMonitoringFileBuilderUI->NbWarnings,pszWarning);
    pMonitoringFileBuilderUI->AddLogContent(psz);

    // set UI state in stopped mode
    pMonitoringFileBuilderUI->SetUIState(FALSE);

    // reset this->pMonitoringFileBuilder
    pMonitoringFileBuilderUI->pMonitoringFileBuilder=NULL;

    if (pMonitoringFileBuilderUI->bCheckOnly)
        MessageBox(NULL,_T("Check completed"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
    else
    {
        // if monitoring file generation is successful
        if (bSuccess)
        {
            if (bParseAllCOMTypeLib || bParseSingleCOMTypeLib)
            {
                MessageBox(pMonitoringFileBuilderUI->hWndDialog,
                            _T("Monitoring files successfully generated"),
                            _T("Information"),
                            MB_OK|MB_ICONQUESTION|MB_TOPMOST);
            }
            else
            {
                // query if user want to view/modify generated file
                if (MessageBox(pMonitoringFileBuilderUI->hWndDialog,
                    _T("Monitoring file successfully generated\r\nDo you want to open it now to check it or make some adjustments ?"),
                    _T("Information"),MB_YESNO|MB_ICONQUESTION|MB_TOPMOST)
                    ==IDYES)
                {
                    // launch default .txt editor
                    if (((int)ShellExecute(NULL,_T("open"),OutputFile,NULL,NULL,SW_SHOWNORMAL))<32)
                    {
                        MessageBox(pMonitoringFileBuilderUI->hWndDialog,_T("Error launching default editor application"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
                    }
                }
            }
        }
    }
    CHookAvailabilityCheck::FreeStaticMembers(); // clear cache so dll can change between 2 generations
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: UpdateCMonitoringFileBuilderOptionsFromUI
// Object: Set option of CMonitoringFileBuilder object according to user interface
// Parameters :
//     inout : CMonitoringFileBuilder* : object to update
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::UpdateCMonitoringFileBuilderOptionsFromUI(CMonitoringFileBuilder* pMonitoringFileBuilder)
{
    // update MonitoringFileBuilder fields according to user choice

    // Get Online MSDN status
    pMonitoringFileBuilder->OnlineMSDN=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_ONLINE_MSDN)==BST_CHECKED);

    // get TryToCall status
    pMonitoringFileBuilder->TryToCall=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_CALL_TO_GUESS_NB_PARAM)==BST_CHECKED);
    
    // get IDC_CHECK_CHECK_TROUBLES status
    pMonitoringFileBuilder->bCheckHookAvaibility=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_CHECK_TROUBLES)==BST_CHECKED);

    // get Use Proxy status
    pMonitoringFileBuilder->bUseProxy=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_USEPROXY)==BST_CHECKED);

    // first bytes analysis status
    pMonitoringFileBuilder->bGenerateFirstBytesAnalysisInformation=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_FIRST_BYTES_ANALYSIS)==BST_CHECKED);
}



//-----------------------------------------------------------------------------
// Name: SetUIState
// Object: Set User Interface in started or stopped mode
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::SetUIState(BOOL bStarted)
{
    // enable Cancel button
    EnableWindow(GetDlgItem(this->hWndDialog,IDCANCEL),bStarted);

    // Disable Buttons
    EnableWindow(GetDlgItem(this->hWndDialog,IDC_BUTTON_GENERATE),!bStarted);
    EnableWindow(GetDlgItem(this->hWndDialog,IDC_BUTTON_UPDATE),!bStarted);
}

//-----------------------------------------------------------------------------
// Name: BrowseOldMonitoringFile
// Object: browse dialog for old monitoring file to update file
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::BrowseOldMonitoringFile()
{
// set current path to monitoring file builder dir

    TCHAR pszOldMonitoringFile[MAX_PATH];
    *pszOldMonitoringFile=0;

    // open dialog
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=this->hWndDialog;
    ofn.hInstance=this->hInstance;
    ofn.lpstrFilter=_T("txt\0*.txt\0All\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    ofn.lpstrFile=pszOldMonitoringFile;
    ofn.nMaxFile=MAX_PATH;
    ofn.lpstrTitle=_T("Select the old monitoring file to update");

    // get file name
    if (!GetOpenFileName(&ofn))
        return;

    SetDlgItemText(this->hWndDialog,IDC_EDIT_OLD_MONITORING_FILE,pszOldMonitoringFile);
}

//-----------------------------------------------------------------------------
// Name: EnableImportExportTableSpecificPanels
// Object: enable or disable import/export table specific panels
// Parameters :
//     in  : BOOL bEnable : TRUE to enable, FALSE to disable
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::EnableImportExportTableSpecificPanels(BOOL bEnable)
{
    CDialogHelper::EnableGroup(GetDlgItem(this->hWndDialog,IDC_GROUP_OPTION),bEnable);
    CDialogHelper::EnableGroup(GetDlgItem(this->hWndDialog,IDC_GROUP_DETECT),bEnable);
    CDialogHelper::EnableGroup(GetDlgItem(this->hWndDialog,IDC_GROUP_UPDATE),bEnable);
}

//-----------------------------------------------------------------------------
// Name: BrowseInputFile
// Object: browse dialog for input file
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::BrowseInputFile()
{
    TCHAR pszFile[MAX_PATH]=_T("");

    // open dialog
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=this->hWndDialog;
    ofn.hInstance=this->hInstance;
    ofn.lpstrFilter=_T("exe, ocx, dll, tlb\0*.exe;*.ocx;*.dll;*.tlb\0All\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    ofn.lpstrFile=pszFile;
    ofn.nMaxFile=MAX_PATH;

    // get file name
    if (!GetOpenFileName(&ofn))
        return;
    this->SetInputFile(ofn.lpstrFile);
}
//-----------------------------------------------------------------------------
// Name: SetInputFile
// Object: set input file
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::SetInputFile(TCHAR* InputFile)
{
    TCHAR* psz;

    // put file name in IDC_EDIT_INPUT_FILE
    SetDlgItemText(this->hWndDialog,IDC_EDIT_INPUT_FILE,InputFile);

    // set radio default position depending file extension
    /////////////////////////////
    // uncheck all radio button
    /////////////////////////////
    // uncheck IDC_RADIO_PARSE_NET
    SendDlgItemMessage(this->hWndDialog,IDC_RADIO_PARSE_NET,(UINT)BM_SETCHECK,BST_UNCHECKED,0);
    // uncheck radio parse debug file
    SendDlgItemMessage(this->hWndDialog,IDC_RADIO_PARSE_DEBUG_FILE,(UINT)BM_SETCHECK,BST_UNCHECKED,0);
    // uncheck IDC_RADIO_IMPORT_TABLE
    SendDlgItemMessage(this->hWndDialog,IDC_RADIO_IMPORT_TABLE,(UINT)BM_SETCHECK,BST_UNCHECKED,0);
    // uncheck IDC_RADIO_EXPORT_TABLE
    SendDlgItemMessage(this->hWndDialog,IDC_RADIO_EXPORT_TABLE,(UINT)BM_SETCHECK,BST_UNCHECKED,0);
    // uncheck IDC_RADIO_PARSE_COM_TYPE_LIBRARY
    SendDlgItemMessage(this->hWndDialog,IDC_RADIO_PARSE_COM_TYPE_LIBRARY,(UINT)BM_SETCHECK,BST_UNCHECKED,0);
    // uncheck IDC_RADIO_PARSE_COM_ALL_REGISTERED_TYPE_LIBRARY
    SendDlgItemMessage(this->hWndDialog,IDC_RADIO_PARSE_COM_ALL_REGISTERED_TYPE_LIBRARY,(UINT)BM_SETCHECK,BST_UNCHECKED,0);

    if (_tcsicmp(CStdFileOperations::GetFileExt(InputFile),_T("tlb"))==0)
    {
        SendDlgItemMessage(this->hWndDialog,IDC_RADIO_PARSE_COM_TYPE_LIBRARY,(UINT)BM_SETCHECK,BST_CHECKED,0);
        return;
    }

    // look for .NET file first
    CPE Pe(InputFile);
    if (!Pe.Parse(FALSE,FALSE))
        return;
    if (Pe.IsNET())
    {
        // check IDC_RADIO_PARSE_NET
        SendDlgItemMessage(this->hWndDialog,IDC_RADIO_PARSE_NET,(UINT)BM_SETCHECK,BST_CHECKED,0);

        this->EnableImportExportTableSpecificPanels(FALSE);
    }
    else
    {
        // look for debug informations
        if (CHasDebugInfos::HasDebugInfos(InputFile))
        {
            // check radio parse debug file
            SendDlgItemMessage(this->hWndDialog,IDC_RADIO_PARSE_DEBUG_FILE,(UINT)BM_SETCHECK,BST_CHECKED,0);

            this->EnableImportExportTableSpecificPanels(FALSE);
        }
        else
        {
            // look for file extension to know if we should parse export or import table

            psz=CStdFileOperations::GetFileExt(InputFile);
            // if dll
            if (_tcsicmp(psz,_T("dll"))==0)
            {
                // check IDC_RADIO_EXPORT_TABLE
                SendDlgItemMessage(this->hWndDialog,IDC_RADIO_EXPORT_TABLE,(UINT)BM_SETCHECK,BST_CHECKED,0);
            }
            else
            {
                // check IDC_RADIO_IMPORT_TABLE
                SendDlgItemMessage(this->hWndDialog,IDC_RADIO_IMPORT_TABLE,(UINT)BM_SETCHECK,BST_CHECKED,0);
            }
            this->EnableImportExportTableSpecificPanels(TRUE);
        }
    }

    
    // gives user a friendly output name
    TCHAR pszOutPutFile[MAX_PATH];
    TCHAR pszShortFileName[MAX_PATH];
    // get app path
    CStdFileOperations::GetAppPath(pszOutPutFile,MAX_PATH);
    _tcscat(pszOutPutFile,MONITORING_FILES_PATH);
    _tcscpy(pszShortFileName,CStdFileOperations::GetFileName(InputFile));
    CStdFileOperations::ChangeFileExt(pszShortFileName,_T("txt"));
    _tcscat(pszOutPutFile,pszShortFileName);
    SetDlgItemText(this->hWndDialog,IDC_EDIT_OUTPUT_FILE,pszOutPutFile);

}

//-----------------------------------------------------------------------------
// Name: OnDropFile
// Object: handle drag and drop operations
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::OnDropFile(HWND hWnd, HDROP hDrop)
{
    TCHAR pszFileName[MAX_PATH];
    UINT NbFiles;
    POINT pt;
    HWND SubWindowHandle;

    // retrieve dialog subitem window handle
    DragQueryPoint(hDrop, &pt);
    ClientToScreen(hWnd,&pt);
    SubWindowHandle=WindowFromPoint(pt);

    // get number of files count in the drag and drop
    NbFiles=DragQueryFile(hDrop, 0xFFFFFFFF,NULL, MAX_PATH);
    // get first file name (in case we just need one name)
    DragQueryFile(hDrop, 0,pszFileName, MAX_PATH);

    DragFinish(hDrop);

    if (SubWindowHandle==GetDlgItem(this->hWndDialog,IDC_EDIT_INPUT_FILE))
    {
        this->SetInputFile(pszFileName);
    }
    else if (SubWindowHandle==GetDlgItem(this->hWndDialog,IDC_EDIT_OUTPUT_FILE))
    {
        SetDlgItemText(this->hWndDialog,IDC_EDIT_OUTPUT_FILE,pszFileName);
    }
    else if (SubWindowHandle==GetDlgItem(this->hWndDialog,IDC_EDIT_OLD_MONITORING_FILE))
    {
        SetDlgItemText(this->hWndDialog,IDC_EDIT_OLD_MONITORING_FILE,pszFileName);
    }
}

//-----------------------------------------------------------------------------
// Name: BrowseOutputFile
// Object: browse dialog for output file
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::BrowseOutputFile()
{
    TCHAR pszFile[MAX_PATH]=_T("");


    // open dialog
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=this->hWndDialog;
    ofn.hInstance=this->hInstance;
    ofn.lpstrFilter=_T("txt\0*.txt\0All\0*.*\0");
    ofn.nFilterIndex = 1;
    // overwrite prompt will be done at file generation, so don't it here to avoid 2 warning messages
    ofn.Flags=OFN_EXPLORER|OFN_NOREADONLYRETURN;// |OFN_OVERWRITEPROMPT; 
    ofn.lpstrDefExt=_T("txt");
    ofn.lpstrFile=pszFile;
    ofn.nMaxFile=MAX_PATH;

    if (!GetSaveFileName(&ofn))
        return;

    // fill IDC_EDIT_OUTPUT_FILE
    SetDlgItemText(this->hWndDialog,IDC_EDIT_OUTPUT_FILE,ofn.lpstrFile);
}

//-----------------------------------------------------------------------------
// Name: UserMessageInformationCallBackStatic
// Object: callback for user message to add to log
// Parameters :
//     in  : TCHAR* Message : user message
//           tagUserMessagesTypes MessageType : message type
//           LPVOID UserParam : associated CMonitoringFileBuilderUI object
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::UserMessageInformationCallBackStatic(TCHAR* Message,tagUserMessagesTypes MessageType,LPVOID UserParam)
{
    ((CMonitoringFileBuilderUI*)UserParam)->UserMessageInformationCallBack(Message,MessageType);
}
void CMonitoringFileBuilderUI::UserMessageInformationCallBack(TCHAR* Message,tagUserMessagesTypes MessageType)
{
    switch(MessageType)
    {
    case USER_MESSAGE_ERROR:
        // add to log window
        this->AddLogContent(_T("Error: "));
        // add to Warnings and Errors report
        this->AddWarningAndErrorsContent(_T("Error: "));
        this->AddWarningAndErrorsContent(Message);
        // increase the number of errors
        this->NbErrors++;
        break;
    case USER_MESSAGE_INFORMATION:
        // this->AddLogContent(_T("Information: ")); // better rendering if disabled
        break;
    case USER_MESSAGE_WARNING:
        // add to log window
        this->AddLogContent(_T("Warning: "));
        // add to Warnings and Errors report
        this->AddWarningAndErrorsContent(_T("Warning: "));
        this->AddWarningAndErrorsContent(Message);
        // increase the number of warnings
        this->NbWarnings++;
        break;
    }
    // add to log window
    this->AddLogContent(Message);
}

//-----------------------------------------------------------------------------
// Name: AddLogContent
// Object: add text to log window
// Parameters :
//     in  : TCHAR* pszMessage : message to add
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::AddLogContent(TCHAR* pszMessage)
{
    TCHAR* psz;
    // check for enough place in buffer
    if (_tcslen(this->pszLogContent)+_tcslen(pszMessage)+1>this->LogContentMaxSize)
    {
        this->LogContentMaxSize*=2;
        // save current buffer
        psz=this->pszLogContent;
        // allocate new buffer
        this->pszLogContent=new TCHAR[this->LogContentMaxSize];
        // copy content of old buffer
        _tcscpy(this->pszLogContent,psz);
        // free old buffer
        delete[] psz;
    }
    _tcscat(this->pszLogContent,pszMessage);

    // clear log window
    SetDlgItemText(this->hWndDialog,IDC_EDIT_LOGS,this->pszLogContent);

    // scroll to last line
    SendDlgItemMessage(this->hWndDialog,IDC_EDIT_LOGS, WM_VSCROLL, SB_BOTTOM, 0 );
}

//-----------------------------------------------------------------------------
// Name: AddWarningAndErrorsContent
// Object: add error and warning to pszLogWarningsAndErrors buffer
// Parameters :
//     in  : TCHAR* pszMessage : message to add
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::AddWarningAndErrorsContent(TCHAR* pszMessage)
{
    TCHAR* psz;
    // check for enough place in buffer
    if (_tcslen(this->pszLogWarningsAndErrors)+_tcslen(pszMessage)+1>this->LogWarningsAndErrorsMaxSize)
    {
        this->LogWarningsAndErrorsMaxSize*=2;
        // save current buffer
        psz=this->pszLogWarningsAndErrors;
        // allocate new buffer
        this->pszLogWarningsAndErrors=new TCHAR[this->LogWarningsAndErrorsMaxSize];
        // copy content of old buffer
        _tcscpy(this->pszLogWarningsAndErrors,psz);
        // free old buffer
        delete psz;
    }
    _tcscat(this->pszLogWarningsAndErrors,pszMessage);
}


//-----------------------------------------------------------------------------
// Name: CallBackPercentCompleted
// Object: callback for percent completed
// Parameters :
//     in  : BYTE Percent : new percent
//           LPVOID UserParam : associated CMonitoringFileBuilderUI object
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringFileBuilderUI::CallBackPercentCompleted(BYTE Percent,LPVOID UserParam)
{
    // reenter object model
    ((CMonitoringFileBuilderUI*)UserParam)->CallBackPercentCompleted(Percent);
}

void CMonitoringFileBuilderUI::CallBackPercentCompleted(BYTE Percent)
{
    SendMessage(this->hProgressBar,PBM_SETPOS,Percent,0);
}