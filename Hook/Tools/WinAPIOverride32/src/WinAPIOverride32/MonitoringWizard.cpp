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
// Object: manages the monitoring wizard dialog
//-----------------------------------------------------------------------------

#include "monitoringwizard.h"
CMonitoringWizard* pMonitoringWizardDialog=NULL;

CMonitoringWizard::CMonitoringWizard(tagMonitoringWizardType MonitoringWizardType)
{
    this->IdMenuListviewFileCreateMonitoringFile=0;
    this->IdMenuListviewFileRenameMonitoringFile=0;
    this->IdMenuListviewFileDeleteMonitoringFile=0;
    this->IdMenuListviewFileEditMonitoringFile=0;
    this->MonitoringWizardType=MonitoringWizardType;
    this->pFileLines=NULL;
    this->pListviewFiles=NULL;
    this->pListviewFunctions=NULL;
    this->dwLastSelectedFileIndex=(DWORD)-1;

    this->pFileToLoadList=NULL;
    this->DialogResult=MONITORING_WIZARD_DLG_RES_CANCEL;
    this->hWndComboSearch=NULL;

    this->bClosed=FALSE;
    this->bManualChangesDone=FALSE;

    this->pDialogNewMonitoringFileName_ArrayCommands=NULL;
    this->pDialogNewMonitoringFileName_ArrayCommandsSize=0;

    // create a link list for storing lines content
    this->pFileLines=new CLinkList(sizeof(LOGFILELINE));

    // get current application directory
    CStdFileOperations::GetAppPath(this->pszMonitoringFilesPath,MAX_PATH);

    // add monitoring directory
    if (this->MonitoringWizardType==MonitoringWizardType_COM)
        _tcscat(this->pszMonitoringFilesPath,MONITORING_WIZARD_MONITORING_COM_FILES_PATH);
    else
        _tcscat(this->pszMonitoringFilesPath,MONITORING_WIZARD_MONITORING_FILES_PATH);

    memset(&this->ComboSearchInfos,0,sizeof(ComboSearchInfos));
}

CMonitoringWizard::~CMonitoringWizard(void)
{
    if (this->pFileToLoadList)
        delete this->pFileToLoadList;

    this->FreepFileLines();
    if (this->pFileLines)
        delete this->pFileLines;

    if (this->pDialogNewMonitoringFileName_ArrayCommands)
        delete this->pDialogNewMonitoringFileName_ArrayCommands;
}

//-----------------------------------------------------------------------------
// Name: ShowDialog
// Object: show the filter dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
INT_PTR CMonitoringWizard::ShowDialog(HINSTANCE hInstance,HWND hWndDialog)
{
    INT_PTR iRet;
    // only allow one instance of Monitoring Wizard for all threads
    if (pMonitoringWizardDialog)
        return 0;

    // show dialog
    iRet=DialogBoxParam(hInstance,(LPCTSTR)IDD_DIALOG_MONITORING_WIZARD,hWndDialog,(DLGPROC)CMonitoringWizard::WndProc,(LPARAM)this);
    pMonitoringWizardDialog=NULL;
    return iRet;
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::Init()
{
    TCHAR pszSearch[MAX_PATH];
    TCHAR FileNameWithoutExt[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    BOOL bFinished=FALSE;
    TCHAR* psz;

    this->pListviewFiles=new CListview(GetDlgItem(this->hWndDialog,IDC_LIST_MONITORING_WIZARD_FILES));
    this->pListviewFiles->AddColumn(_T("Monitoring File"),210,LVCFMT_LEFT);
    if (this->MonitoringWizardType==MonitoringWizardType_COM)
        this->pListviewFiles->SetStyle(TRUE,FALSE,FALSE,FALSE);
    else
        this->pListviewFiles->SetStyle(TRUE,FALSE,FALSE,TRUE);
    this->pListviewFiles->SetSelectItemCallback(CMonitoringWizard::CallbackListViewFileSelectionStatic,this);
    this->pListviewFiles->EnableDefaultCustomDraw(FALSE);

    this->pListviewFunctions=new CListview(GetDlgItem(this->hWndDialog,IDC_LIST_MONITORING_WIZARD_FUNCS));
    this->pListviewFunctions->AddColumn(_T("Function Name"),520,LVCFMT_LEFT);
    if (this->MonitoringWizardType==MonitoringWizardType_COM)
        this->pListviewFunctions->AddColumn(_T("Index"),100,LVCFMT_LEFT);
    else
        this->pListviewFunctions->AddColumn(_T("dll"),100,LVCFMT_LEFT);
    this->pListviewFunctions->AddColumn(_T("Line"),0,LVCFMT_LEFT);
    this->pListviewFunctions->SetStyle(TRUE,FALSE,FALSE,TRUE);
    this->pListviewFunctions->SetSortingCompareCallback(CMonitoringWizard::CallbackSortingCompareCallback,this);
    this->pListviewFunctions->SetSelectItemCallback(CMonitoringWizard::CallbackListViewFunctionSelectionStatic,this);

    // make the search string (directory\*.txt)
    _tcscpy(pszSearch,this->pszMonitoringFilesPath);
    _tcscat(pszSearch,_T("*.txt"));

    // search monitoring files in path
    hFind=FindFirstFile(pszSearch,&FindData);
    if (hFind==INVALID_HANDLE_VALUE)
    {
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("No monitoring file found in %s directory"),this->pszMonitoringFilesPath);
        MessageBox(this->hWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
    }
    if (hFind!= INVALID_HANDLE_VALUE)
    {
        while (!bFinished)
        {
            _tcscpy(FileNameWithoutExt,FindData.cFileName);
            // remove extension from result file name
            psz=_tcsrchr(FileNameWithoutExt,'.');
            if (psz)
                *psz=0;

            // add item to listview
            if (this->MonitoringWizardType==MonitoringWizardType_COM)
            {
                int ItemIndex=this->pListviewFiles->AddItem(_T(""));
                this->SetComDisplayFileName(FileNameWithoutExt,ItemIndex);
            }
            else
                this->pListviewFiles->AddItem(FileNameWithoutExt);

            // search next file
            if (!FindNextFile(hFind, &FindData)) 
            {
                if (GetLastError() == ERROR_NO_MORE_FILES) 
                { 
                    bFinished=TRUE;
                    break;
                } 
            }
        }
        FindClose(hFind);

        // sort file list
        this->pListviewFiles->Sort();

        // first item selection
        this->pListviewFiles->SetSelectedIndex(0);
    }

    this->hWndComboSearch=GetDlgItem(this->hWndDialog,IDC_COMBO_WIZARD_FUNC_SEARCH);

    // add file listview menu options
    this->IdMenuListviewFileCreateMonitoringFile=this->pListviewFiles->pPopUpMenu->Add(_T("Create New Monitoring File"),(UINT)0);
    this->IdMenuListviewFileRenameMonitoringFile=this->pListviewFiles->pPopUpMenu->Add(_T("Rename"),1);
    this->IdMenuListviewFileEditMonitoringFile=this->pListviewFiles->pPopUpMenu->Add(_T("Full Edit"),2);
    this->IdMenuListviewFileDeleteMonitoringFile=this->pListviewFiles->pPopUpMenu->Add(_T("Remove"),3);
    this->pListviewFiles->SetPopUpMenuItemClickCallback(CMonitoringWizard::CallbackPopUpMenuItemClickStatic,this);
    
    this->pListviewFiles->pPopUpMenu->AddSeparator(4);


    // get combo search infos
    this->ComboSearchInfos.cbSize= sizeof(ComboSearchInfos);
    ::GetComboBoxInfo(this->hWndComboSearch,&this->ComboSearchInfos);

    // render layout
    this->OnSize();
}

//-----------------------------------------------------------------------------
// Name: SetComDisplayFileName
// Object: set name to display in file name list
// Parameters :
//     in  : TCHAR* FileNameWithoutExt : filename without extension
//           int ListViewItemIndex :
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::SetComDisplayFileName(TCHAR* FileNameWithoutExt,int ListViewItemIndex)
{

    TCHAR pszInterfaceNameAndIID[2*MAX_PATH];
    if (CGUIDStringConvert::GetInterfaceName(FileNameWithoutExt,pszInterfaceNameAndIID,MAX_PATH))
    {
        _tcscat(pszInterfaceNameAndIID,_T("  "));
        _tcscat(pszInterfaceNameAndIID,FileNameWithoutExt);
        this->pListviewFiles->SetItemText(ListViewItemIndex,0,pszInterfaceNameAndIID);
    }
    else
    {
        TCHAR* Content;
        TCHAR FullPath[MAX_PATH];
        _tcscpy(FullPath,this->pszMonitoringFilesPath);
        _tcscat(FullPath,FileNameWithoutExt);
        _tcscat(FullPath,_T(".txt"));
        if (!CTextFile::Read(FullPath,&Content))
        {
            this->pListviewFiles->SetItemText(ListViewItemIndex,0,FileNameWithoutExt);
        }
        else
        {
            // look for "@InterfaceName="
            TCHAR* InterfaceName=_tcsstr(Content,MONITORING_WIZARD_MONITORING_COM_FILES_INTREFACE_NAME);
            if (InterfaceName)
            {
                // point after "@InterfaceName="
                InterfaceName+=_tcslen(MONITORING_WIZARD_MONITORING_COM_FILES_INTREFACE_NAME);

                // find end of interface name (end of line)
                TCHAR* InterfaceNameEnd=_tcspbrk(InterfaceName,_T("\r\n"));
                if (InterfaceNameEnd)
                    *InterfaceNameEnd=0;

                _tcscpy(pszInterfaceNameAndIID,InterfaceName);
                _tcscat(pszInterfaceNameAndIID,_T("  "));
                _tcscat(pszInterfaceNameAndIID,FileNameWithoutExt);
                this->pListviewFiles->SetItemText(ListViewItemIndex,0,pszInterfaceNameAndIID);
            }
            else
                this->pListviewFiles->SetItemText(ListViewItemIndex,0,FileNameWithoutExt);

            delete Content;
        }
    }
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::Close()
{
    this->bClosed=TRUE;

    if (this->pListviewFiles)
    {
        delete this->pListviewFiles;
        this->pListviewFiles=NULL;
    }
    if (this->pListviewFunctions)
    {
        delete this->pListviewFunctions;
        this->pListviewFunctions=NULL;
    }

    EndDialog(this->hWndDialog,this->DialogResult);
}



//-----------------------------------------------------------------------------
// Name: WndProc
// Object: Monitoring wizard window proc
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CMonitoringWizard::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        pMonitoringWizardDialog=(CMonitoringWizard*)lParam;
        pMonitoringWizardDialog->hWndDialog=hWnd;

        pMonitoringWizardDialog->Init();
        // load dlg icons
        CDialogHelper::SetIcon(hWnd,IDI_ICON_WIZARD);

        break;
    case WM_SIZING:
        pMonitoringWizardDialog->OnSizing((RECT*)lParam);
        break;
    case WM_SIZE:
        pMonitoringWizardDialog->OnSize();
        break;

    case WM_CLOSE:
        pMonitoringWizardDialog->Close();
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            pMonitoringWizardDialog->OkClick();
            break;
        case IDCANCEL:
            pMonitoringWizardDialog->Close();
            break;
        case IDC_BUTTON_CHECK_ALL:
            // commit changes if any
            pMonitoringWizardDialog->CommitChanges();
            pMonitoringWizardDialog->pListviewFunctions->SelectAll();
            break;
        case IDC_BUTTON_UNCHECK_ALL:
            // commit changes if any
            pMonitoringWizardDialog->CommitChanges();
            pMonitoringWizardDialog->pListviewFunctions->UnselectAll();
            break;
        case IDC_BUTTON_CHECK_SELECTED:
            // commit changes if any
            pMonitoringWizardDialog->CommitChanges();
            pMonitoringWizardDialog->pListviewFunctions->CheckSelected();
            break;
        case IDC_BUTTON_UNCHECK_SELECTED:
            // commit changes if any
            pMonitoringWizardDialog->CommitChanges();
            pMonitoringWizardDialog->pListviewFunctions->UncheckSelected();
            break;
        case IDC_BUTTON_WIZARD_ADD:
            pMonitoringWizardDialog->AddFunctionDescription();
            break;
        case IDC_BUTTON_WIZARD_MODIFY:
            pMonitoringWizardDialog->ModifySelectedFunctionDescription();
            break;
        case IDC_BUTTON_WIZARD_DELETE:
            pMonitoringWizardDialog->RemoveSelectedFunctionDescription();
            break;
        case IDC_BUTTON_WIZARD_EDIT:
            pMonitoringWizardDialog->FullEditAssociatedFile();
            break;
        case IDC_BUTTON_DEFAULT_VALUES:
            pMonitoringWizardDialog->RestoreDefaultValues();
            break;
        }

        if ((HWND)lParam==pMonitoringWizardDialog->hWndComboSearch)
        {
            switch(HIWORD(wParam))
            {
                case CBN_EDITCHANGE:
                    pMonitoringWizardDialog->SearchFunction();
                    break;
                case CBN_SELCHANGE:
                    pMonitoringWizardDialog->SelectSearchedFunctionInListView();
                    break;
            }
        }

        break;
    case WM_NOTIFY:
        if (pMonitoringWizardDialog->pListviewFiles)
        {
            if (pMonitoringWizardDialog->pListviewFiles->OnNotify(wParam,lParam))
            {
                NMHDR* pnmh=((NMHDR*)lParam);
                if (pnmh->hwndFrom==ListView_GetHeader(pMonitoringWizardDialog->pListviewFiles->GetControlHandle()))
                {
                    switch (pnmh->code) 
                    {
                    // on header click
                    case HDN_ITEMCLICK:
                        // listview has been sorted --> update this->dwLastSelectedFileIndex
                        pMonitoringWizardDialog->dwLastSelectedFileIndex=pMonitoringWizardDialog->pListviewFiles->GetSelectedIndex();
                        break;
                    }
                }
                return TRUE;
            }
        }
        if (pMonitoringWizardDialog->pListviewFunctions)
        {
            if (pMonitoringWizardDialog->pListviewFunctions->OnNotify(wParam,lParam))
                return TRUE;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: OkClick
// Object: apply changes and prepare file for loading
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::OkClick()
{
    TCHAR FileToLoad[MAX_PATH];

    // apply changes to selected item in case some function state has changed
    this->CommitChanges();

    // set the dialog result to ok
    this->DialogResult=MONITORING_WIZARD_DLG_RES_OK;

    // add selected files to pFileToLoadList
    this->pFileToLoadList=new CLinkList(MAX_PATH*sizeof(TCHAR));

    int NbFiles=this->pListviewFiles->GetItemCount();
    for (int cnt=0;cnt<NbFiles;cnt++)
    {
        if (!this->pListviewFiles->IsItemSelected(cnt))
            continue;
        this->dwLastSelectedFileIndex=(DWORD)cnt;
        this->GetSelectedFilePath(FileToLoad);
        // add full path to list has FileToLoad is a pointer, content of string will be copy
        this->pFileToLoadList->AddItem(FileToLoad);
    }

    // close dialog
    this->Close();
}

//-----------------------------------------------------------------------------
// Name: CommitChanges
// Object: apply changes to monitoring file (disable or enable func logging for each func)
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::CommitChanges()
{
    if (this->dwLastSelectedFileIndex==(DWORD)-1)
        return;

    int cnt;
    int ItemCount;
    TCHAR pszFileName[MAX_PATH];
    HANDLE hFile;
    BOOL bCurrentFuncSelected;
    CLinkListItem* pItem;
    CMonitoringWizard::LOGFILELINE* pLogFileLine;
   
    
    if (!this->bManualChangesDone)
    {
        BOOL bChangeOccurs;
        bChangeOccurs=FALSE;

        // for each item function description listview
        ItemCount=this->pListviewFunctions->GetItemCount();
        for (cnt=0;cnt<ItemCount;cnt++)
        {
            // get current state
            bCurrentFuncSelected=this->pListviewFunctions->IsItemSelected(cnt);

            // get item from pFileLines linked list 
            this->pListviewFunctions->GetItemUserData(cnt,(LPVOID*)&pItem);

            pLogFileLine=(LOGFILELINE*)pItem->ItemData;
            // if state differs from original one, we have to rewrite the file                
            if (pLogFileLine->bStateEnabled!=bCurrentFuncSelected)
            {
                bChangeOccurs=TRUE;

                // apply change
                pLogFileLine->bStateEnabled=bCurrentFuncSelected;
            }
        }
        // if no change occurs
        if (!bChangeOccurs)
            return;
    }

    // reset this->bManualChangesDone flag
    this->bManualChangesDone=FALSE;

    // change occurs, rewrite file

    // retrieve name of monitoring file
    this->GetSelectedFilePath(pszFileName);

    // open file for writing
    if (!CTextFile::CreateTextFile(pszFileName,&hFile))
    {
        CAPIError::ShowLastError();
        return;
    }

    // write line by line
    this->pFileLines->Lock();
    for(pItem=this->pFileLines->Head;pItem;pItem=pItem->NextItem)
    {
        // if not first line add \r\n (do it now no after adding line to avoid to add an empty line at each saving)
        if (pItem!=this->pFileLines->Head)
            CTextFile::WriteText(hFile,_T("\r\n"));
        
        pLogFileLine=(CMonitoringWizard::LOGFILELINE*)pItem->ItemData;
        // check state. If disable add '!' before line
        if (!pLogFileLine->bStateEnabled)
            CTextFile::WriteText(hFile,_T("!"));

        // write line content
        CTextFile::WriteText(hFile,pLogFileLine->pszContent);

    }
    this->pFileLines->Unlock();

    // close file
    CloseHandle(hFile);
}
//-----------------------------------------------------------------------------
// Name: CallBackListviewItemSelection
// Object: callback called when user click on a library name
// Parameters :
//     in  : int ItemIndex : index of selected item in library name listview
//           int SubItemIndex : subitem index of selected item in library name listview
//           LPVOID UserParam : CMonitoringWizard* associated with the listview
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::CallbackListViewFileSelectionStatic(int ItemIndex,int SubItemIndex,LPVOID UserParam)
{
    UNREFERENCED_PARAMETER(SubItemIndex);
    ((CMonitoringWizard*)UserParam)->CallbackListViewFileSelection(ItemIndex);
}

//-----------------------------------------------------------------------------
// Name: GetSelectedFilePath
// Object: Get file path of selected file
// Parameters :
//     in  : 
//     out : TCHAR* pszFullPath : path of selected file must be at least MAX_PATH len (in TCHAR)
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::GetSelectedFilePath(TCHAR* pszFullPath)
{
    TCHAR FileName[2*MAX_PATH];
    
    // retrieve file name in listview
    this->pListviewFiles->GetItemText(this->dwLastSelectedFileIndex,FileName,2*MAX_PATH);
    if (this->MonitoringWizardType==MonitoringWizardType_COM)
    {
        // display name is like "InterfaceName {InterfaceID}"
        // and monitoring file name is like "{InterfaceID}.txt"

        // remove "InterfaceName " if any
        TCHAR* psz=_tcschr(FileName,'{');
        if (psz)
        {
            // if there was an interface name
            if (psz!=FileName)
                // update FileName
                _tcscpy(FileName,psz);
        }
    }

    // make full path from filename
    _tcscpy(pszFullPath,this->pszMonitoringFilesPath);
    _tcscat(pszFullPath,FileName);
    _tcscat(pszFullPath,_T(".txt"));
}


//-----------------------------------------------------------------------------
// Name: CallbackPopUpMenuItemClickStatic
// Object: called when listview file menu is clicked
// Parameters :
//     in  : UINT MenuID : clicked menu id
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::CallbackPopUpMenuItemClickStatic(UINT MenuID,LPVOID UserParam)
{
    // re-enter object model
    ((CMonitoringWizard*)UserParam)->CallbackPopUpMenuItemClick(MenuID);
}



//-----------------------------------------------------------------------------
// Name: DialogNewMonitoringFileName ok Click callback
// Object: 
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::DialogNewMonitoringFileName_OnButtonOkClick (CDialogSimple* pDialogSimple,PVOID UserParam)
{
    CMonitoringWizard* pMonitoringWizard=(CMonitoringWizard*)UserParam;
    GetDlgItemText(pDialogSimple->GetControlHandle(),
                    MONITORING_WIZARD_DialogNewMonitoringFileName_EDIT_FILENAME,
                    pMonitoringWizard->DialogNewMonitoringFileName_FileName,
                    MAX_PATH);
    pMonitoringWizard->DialogNewMonitoringFileName_FileName[MAX_PATH-1]=0;

    pDialogSimple->Close(MONITORING_WIZARD_DLG_RES_OK);
}
//-----------------------------------------------------------------------------
// Name: DialogNewMonitoringFileName cancel Click callback
// Object: 
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::DialogNewMonitoringFileName_OnButtonCancelClick (CDialogSimple* pDialogSimple,PVOID UserParam)
{
    UNREFERENCED_PARAMETER(UserParam);
    pDialogSimple->Close(MONITORING_WIZARD_DLG_RES_CANCEL);
}

//-----------------------------------------------------------------------------
// Name: DialogNewMonitoringFileName create callback
// Object: 
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::DialogNewMonitoringFileName_OnCreate(CDialogSimple* pDialogSimple,PVOID UserParam)
{
    CMonitoringWizard* pMonitoringWizard=(CMonitoringWizard*)UserParam;
    CDialogSimple::COMMANDS_CALLBACK_ARRAY_ITEM ArrayCommands[]=
    {
        {IDOK,CMonitoringWizard::DialogNewMonitoringFileName_OnButtonOkClick},
        {IDCANCEL,CMonitoringWizard::DialogNewMonitoringFileName_OnButtonCancelClick}
    };
    if (pMonitoringWizard->pDialogNewMonitoringFileName_ArrayCommands)
        delete pMonitoringWizard->pDialogNewMonitoringFileName_ArrayCommands;
    
    pMonitoringWizard->pDialogNewMonitoringFileName_ArrayCommandsSize=sizeof(ArrayCommands)/sizeof(ArrayCommands[0]);
    pMonitoringWizard->pDialogNewMonitoringFileName_ArrayCommands=new CDialogSimple::COMMANDS_CALLBACK_ARRAY_ITEM[pMonitoringWizard->pDialogNewMonitoringFileName_ArrayCommandsSize];
    memcpy(pMonitoringWizard->pDialogNewMonitoringFileName_ArrayCommands,ArrayCommands,sizeof(ArrayCommands));

    CDialogHelper::CreateButton(pDialogSimple->GetControlHandle(),110,30,75,25,IDOK,_T("Ok"));
    CDialogHelper::CreateButton(pDialogSimple->GetControlHandle(),215,30,75,25,IDCANCEL,_T("Cancel"));
    CDialogHelper::CreateStatic(pDialogSimple->GetControlHandle(),5,5,130,20,
                                MONITORING_WIZARD_DialogNewMonitoringFileName_STATIC_ENTER_FILENAME,
                                _T("Enter Monitoring Name :")
                                );
    CDialogHelper::CreateEdit(pDialogSimple->GetControlHandle(),140,3,250,20,
                              MONITORING_WIZARD_DialogNewMonitoringFileName_EDIT_FILENAME,
                              pMonitoringWizard->DialogNewMonitoringFileName_FileName
                              );
    pDialogSimple->SetCommandsCallBacks(pMonitoringWizard->pDialogNewMonitoringFileName_ArrayCommands,
                                        pMonitoringWizard->pDialogNewMonitoringFileName_ArrayCommandsSize,
                                        UserParam);

    // set focus on edit
    ::SetFocus(
                GetDlgItem(pDialogSimple->GetControlHandle(),
                MONITORING_WIZARD_DialogNewMonitoringFileName_EDIT_FILENAME
                ));
}

//-----------------------------------------------------------------------------
// Name: GetNewMonitoringFileName 
// Object: 
// Parameters :
//     in  : TCHAR* OldFileName : old file name
//     out :
//     return : TRUE if user changes filename
//-----------------------------------------------------------------------------
BOOL CMonitoringWizard::GetNewMonitoringFileName(TCHAR* OldFileName)
{
    TCHAR Title[MAX_PATH];
    if (*OldFileName)
        _tcscpy(Title,_T("Rename"));
    else
        _tcscpy(Title,_T("Create new file"));

    _tcscpy(this->DialogNewMonitoringFileName_FileName,OldFileName);
    CDialogSimple DialogSimple(400,90,TRUE);
    DialogSimple.SetCreateCallback(DialogNewMonitoringFileName_OnCreate,this);
    DialogSimple.Show(CDialogHelper::GetInstance(this->hWndDialog),this->hWndDialog,Title,CDialogHelper::GetInstance(this->hWndDialog),IDI_ICON_WIZARD);
    return DialogSimple.GetDialogResult()==MONITORING_WIZARD_DLG_RES_OK;
}

void CMonitoringWizard::CallbackPopUpMenuItemClick(UINT MenuID)
{
    if (MenuID==this->IdMenuListviewFileCreateMonitoringFile)
    {
        if (this->GetNewMonitoringFileName(_T("")))
        {
            HANDLE hFile;
            int Index;
            TCHAR FullPath[MAX_PATH];

            ///////////////////////////////////
            // create full path from short name
            ///////////////////////////////////
            // assume this->DialogNewMonitoringFileName_FileName has no extension
            CStdFileOperations::RemoveFileExt(this->DialogNewMonitoringFileName_FileName);
            // add directory
            _tcscpy(FullPath,this->pszMonitoringFilesPath);
            // add short name
            _tcscat(FullPath,this->DialogNewMonitoringFileName_FileName);
            // add extension
            _tcscat(FullPath,_T(".txt"));

            // create file
            if (!CTextFile::CreateTextFile(FullPath,&hFile))
            {
                CAPIError::ShowLastError();
                return;
            }
            CloseHandle(hFile);

            // add file to listview
            Index=this->pListviewFiles->AddItem(this->DialogNewMonitoringFileName_FileName);

            // commit changes before sorting
            this->CallbackListViewFileSelection(Index);

            // select new item (BEFORE calling FullEditAssociatedFile)
            this->pListviewFiles->SetSelectedIndex(Index);
            this->pListviewFiles->ReSort();
            this->dwLastSelectedFileIndex=this->pListviewFiles->GetSelectedIndex();
            this->pListviewFiles->ScrollTo(this->dwLastSelectedFileIndex);

            // query full edit mode to user
            if (MessageBox(this->hWndDialog,_T("Do you want to full edit file now ?"),_T("Question"),MB_ICONQUESTION|MB_YESNO)==IDYES)
            {
                this->FullEditAssociatedFile();
            }
        }
    }
    else if (MenuID==this->IdMenuListviewFileDeleteMonitoringFile)
    {
        TCHAR SelectedFileName[MAX_PATH];
        TCHAR Query[MAX_PATH];
        TCHAR ShortFileName[MAX_PATH];

        // get selected file name
        this->GetSelectedFilePath(SelectedFileName);
        _tcscpy(ShortFileName,CStdFileOperations::GetFileName(SelectedFileName));
        CStdFileOperations::RemoveFileExt(ShortFileName);

        _sntprintf(Query,MAX_PATH,_T("Do you really want to delete %s ?"),ShortFileName);
        Query[MAX_PATH-1]=0;
        if (MessageBox(this->hWndDialog,Query,_T("Question"),MB_ICONQUESTION|MB_YESNO)==IDYES)
        {
            // delete file
            if (!::DeleteFile(SelectedFileName))
            {
                CAPIError::ShowLastError();
                return;
            }

            // remove file from list
            this->pListviewFiles->RemoveItem(this->pListviewFiles->GetSelectedIndex());

            // first item selection
            if (this->pListviewFiles->GetItemCount()>0)
            {
                this->pListviewFiles->SetSelectedIndex(0);
                this->CallbackListViewFileSelection(0);
                this->pListviewFiles->ScrollTo(0);
            }
        }
    }
    else if (MenuID==this->IdMenuListviewFileEditMonitoringFile)
    {
        this->FullEditAssociatedFile();
    }
    else if (MenuID==this->IdMenuListviewFileRenameMonitoringFile)
    {
        TCHAR OldFileName[MAX_PATH];
        TCHAR ShortOldFileName[MAX_PATH];
        
        // get file name associated to selected item
        this->GetSelectedFilePath(OldFileName);

        // get short name (without path and extension)
        _tcscpy(ShortOldFileName,CStdFileOperations::GetFileName(OldFileName));
        CStdFileOperations::RemoveFileExt(ShortOldFileName);
        
        // get new short name
        if (this->GetNewMonitoringFileName(ShortOldFileName))
        {
            TCHAR FullPath[MAX_PATH];

            ///////////////////////////////////
            // create full path from short name
            ///////////////////////////////////

            // assume this->DialogNewMonitoringFileName_FileName has no extension
            CStdFileOperations::RemoveFileExt(this->DialogNewMonitoringFileName_FileName);
            // add directory
            _tcscpy(FullPath,this->pszMonitoringFilesPath);
            // add short name
            _tcscat(FullPath,this->DialogNewMonitoringFileName_FileName);
            // add extension
            _tcscat(FullPath,_T(".txt"));

            // rename file
            if (!::MoveFile(OldFileName,FullPath))
            {
                CAPIError::ShowLastError();
                return;
            }
            // update name in listview
            this->SetComDisplayFileName(this->DialogNewMonitoringFileName_FileName,this->pListviewFiles->GetSelectedIndex());

            // sort listview again
            this->pListviewFiles->ReSort();

            this->dwLastSelectedFileIndex=this->pListviewFiles->GetSelectedIndex();
        }
    }
}

//-----------------------------------------------------------------------------
// Name: CallbackListViewFileSelection
// Object: called when a file is clicked
// Parameters :
//     in  : int ItemIndex : clicked item index
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::CallbackListViewFileSelection(int ItemIndex)
{
    TCHAR FileFullPath[MAX_PATH];
    TCHAR AssociatedDefaultFile[MAX_PATH];

    // commit changes if any (save modification done by function selection for the old selected monitoring file)
    pMonitoringWizardDialog->CommitChanges();

    // update dwLastSelectedFileIndex ONLY AFTER HAVING CALLED CommitChanges
    this->dwLastSelectedFileIndex=ItemIndex;

    // get selected file (only after having updated this->dwLastSelectedFileIndex)
    this->GetSelectedFilePath(FileFullPath);

    if (this->GetAssociatedDefaultFile(AssociatedDefaultFile))
        EnableWindow(GetDlgItem(this->hWndDialog,IDC_BUTTON_DEFAULT_VALUES),TRUE);
    else
        EnableWindow(GetDlgItem(this->hWndDialog,IDC_BUTTON_DEFAULT_VALUES),FALSE);

    // update shown functions
    this->UpdateFunctionsList(FileFullPath);

}

//-----------------------------------------------------------------------------
// Name: UpdateFunctionsList
// Object: update displayed function list according to provided filename
// Parameters :
//     in  : TCHAR* FileFullPath : file name containing function description
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::UpdateFunctionsList(TCHAR* FileFullPath)
{
    // empty current link list
    this->FreepFileLines();

    // empty functions listview
    this->pListviewFunctions->Clear();

    // parse selected file
    CTextFile::ParseLines(FileFullPath,CMonitoringWizard::CallBackReadLine,this);

    // resort listview according to the last sorting columns
    this->pListviewFunctions->ReSort();
}

//-----------------------------------------------------------------------------
// Name: CallbackListViewFunctionSelectionStatic
// Object: callback called when user click on a function description
// Parameters :
//     in  : int ItemIndex : index of selected item in library name listview
//           int SubItemIndex : subitem index of selected item in library name listview
//           LPVOID UserParam : CMonitoringWizard* associated with the listview
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::CallbackListViewFunctionSelectionStatic(int ItemIndex,int SubItemIndex,LPVOID UserParam)
{
    UNREFERENCED_PARAMETER(SubItemIndex);
    ((CMonitoringWizard*)UserParam)->CallbackListViewFunctionSelection(ItemIndex);
}
void CMonitoringWizard::CallbackListViewFunctionSelection(int ItemIndex)
{
    UNREFERENCED_PARAMETER(ItemIndex);
    LOGFILELINE* pLog;
    pLog=this->GetLogInfoFromSelectedFunction();
    if (!pLog)
        return;

    SetDlgItemText(this->hWndDialog,IDC_EDIT_WIZARD_MODIFY,pLog->pszContent);
}


//-----------------------------------------------------------------------------
// Name: FreepFileLines
// Object: free memory allocated by CallbackListViwFileSelection
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::FreepFileLines()
{
    if (!this->pFileLines)
        return;
    
    CLinkListItem* pItem;
    LOGFILELINE* pLog;

    // for each item
    this->pFileLines->Lock();
    for (pItem=this->pFileLines->Head;pItem;pItem=pItem->NextItem)
    {
        pLog=(LOGFILELINE*)pItem->ItemData;

        // free line content
        if (pLog->pszContent)
            free(pLog->pszContent);
    }
    this->pFileLines->RemoveAllItems(TRUE);
    this->pFileLines->Unlock();
}


//-----------------------------------------------------------------------------
// Name: CallBackReadLine
// Object: callback called for each line found in monitoring file
// Parameters :
//     in  : TCHAR* FileName : name of file beeing parsed
//           TCHAR* Line : content of the line
//           DWORD dwLineNumber : number of the line
//           LPVOID UserParam : CMonitoringWizard* associated with the listview
//     out :
// return : TRUE to continue parsing, FALSE to stop it
//-----------------------------------------------------------------------------
BOOL CMonitoringWizard::CallBackReadLine(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam)
{
    ((CMonitoringWizard*)UserParam)->CallBackMonitoringFileReadLine(FileName,Line,dwLineNumber);
    return TRUE;
}

void CMonitoringWizard::CallBackMonitoringFileReadLine(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber)
{
    UNREFERENCED_PARAMETER(FileName);
    CMonitoringWizard::LOGFILELINE Log;
    Log.dwLineNumber=dwLineNumber;

    // remove spaces
    Line=CTrimString::TrimString(Line);
    // check if line is disabled (begins with !)
    if (*Line=='!')
    {
        Log.bStateEnabled=FALSE;
        // don't copy the '!'
        Line++;
    }
    else
        Log.bStateEnabled=TRUE;

    // remove line starting and ending spaces
    CTrimString::TrimString(Line);

    // copy line content
    Log.pszContent=_tcsdup(Line);

    this->AddFunctionDescription(&Log);
}

//-----------------------------------------------------------------------------
// Name: CallbackSortingCompareCallback
// Object: allow to make sorting on function name only without caring of return
// Parameters :
//     in  : TCHAR* String1
//           TCHAR* String2
//           CListview::SortingType DataSortingType
//           BOOL Ascending
//           LPVOID UserParam
//     out :
//     return : 
//-----------------------------------------------------------------------------
int CMonitoringWizard::CallbackSortingCompareCallback(TCHAR* String1,TCHAR* String2,CListview::SortingType DataSortingType,BOOL Ascending,LPVOID UserParam)
{
    // reenter object model
    return ((CMonitoringWizard*)UserParam)->CallbackSortingCompareCallback(String1,String2,DataSortingType,Ascending);
}

int CMonitoringWizard::CallbackSortingCompareCallback(TCHAR* String1,TCHAR* String2,CListview::SortingType DataSortingType,BOOL Ascending)
{
    UNREFERENCED_PARAMETER(DataSortingType);

    int iAscendingFlag;
    if (Ascending)
        iAscendingFlag=1;
    else
        iAscendingFlag=-1;

    String1=this->GetFunctionName(String1);
    String2=this->GetFunctionName(String2);

    return (_tcsicmp(String1,String2)*iAscendingFlag);
}

//-----------------------------------------------------------------------------
// Name: GetFunctionName
// Object: extract function name from full definition
// Parameters :
//     in  : TCHAR* FunctionDefinition
//     out :
//     return : FunctionName
//-----------------------------------------------------------------------------
TCHAR* CMonitoringWizard::GetFunctionName(TCHAR* FunctionDefinition)
{
    // remove return type
    TCHAR* EndFuncName;
    TCHAR* BeginFuncName;

    // remove args
    EndFuncName=_tcschr(FunctionDefinition,'(');
    if (EndFuncName)
        *EndFuncName=0;

    // remove return
    BeginFuncName=_tcsrchr(FunctionDefinition,' ');
    if (BeginFuncName)
    {
        while(*(BeginFuncName+1)=='*')
        {
            BeginFuncName++;
        }
        FunctionDefinition=BeginFuncName+1;
    }

    return FunctionDefinition;
}

//-----------------------------------------------------------------------------
// Name: SearchFunction
// Object: Called when user combobox input changes
//          Search results throw listview, select first matching item 
//          gives user choice for other matching function names by filling combo
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::SearchFunction()
{
    TCHAR* FunctionName;
    TCHAR FunctionDescription[MAX_CONFIG_FILE_LINE_SIZE];

    TCHAR* SearchedFunction;
    LONG SearchedFunctionLen;

    LONG ItemCount;
    LONG Cnt;
    BOOL FirstMatchingItemSelected;

    // get searched function len 
    SearchedFunctionLen=(LONG)SendMessage(this->hWndComboSearch,WM_GETTEXTLENGTH,0,0);
    if (SearchedFunctionLen<=0)
    {
        SendMessage(this->hWndComboSearch,CB_SHOWDROPDOWN,(WPARAM)FALSE,0);
        return;
    }
    // allocate memory 
    SearchedFunction=(TCHAR*) _alloca((SearchedFunctionLen+3)*sizeof(TCHAR));// 1 for null + 2 for * before and * after name for a wild char search
    SearchedFunction[0]='*';

    // get searched function 
    if (!SendMessage(this->hWndComboSearch,WM_GETTEXT,(WPARAM)(SearchedFunctionLen+1),(LPARAM)(SearchedFunction+1)))
    {
        SendMessage(this->hWndComboSearch,CB_SHOWDROPDOWN,(WPARAM)FALSE,0);
        return;
    }
   
#ifdef _DEBUG
    TCHAR Msg[256];
    _stprintf (Msg,_T("Content %s Length : %u \r\n"),&SearchedFunction[1],SearchedFunctionLen);
    OutputDebugString(Msg);
#endif


    // hide combo content
    SendMessage(this->hWndComboSearch,CB_SHOWDROPDOWN,(WPARAM)FALSE,0);
    SendMessage(this->hWndComboSearch,CB_RESETCONTENT,0,0);

    _tcscat(SearchedFunction,_T("*"));

    // search in function listview 
    ItemCount=this->pListviewFunctions->GetItemCount();
    FirstMatchingItemSelected=FALSE;
    // for each listview item
    for (Cnt=0;Cnt<ItemCount;Cnt++)
    {
        // get description text
        this->pListviewFunctions->GetItemText(Cnt,FunctionDescription,MAX_CONFIG_FILE_LINE_SIZE);
        // get associated function name
        FunctionName=this->GetFunctionName(FunctionDescription);
        // if function match
        if (CWildCharCompare::WildICmp(SearchedFunction,FunctionName))
        {
            // add to combo
            SendMessage(this->hWndComboSearch,CB_ADDSTRING,0,(LPARAM)FunctionName);

            // if first matching item is not selected
            if (!FirstMatchingItemSelected)
            {
                // set single selection style
                LONG_PTR Styles=GetWindowLongPtr(this->pListviewFunctions->GetControlHandle(),GWL_STYLE);
                SetWindowLongPtr(this->pListviewFunctions->GetControlHandle(),GWL_STYLE,Styles|LVS_SINGLESEL);

                // select item
                this->pListviewFunctions->SetSelectedIndex(Cnt);
                ListView_EnsureVisible(this->pListviewFunctions->GetControlHandle(),Cnt,FALSE);

                // restore multiple selection
                Styles&=~LVS_SINGLESEL;
                SetWindowLongPtr(this->pListviewFunctions->GetControlHandle(),GWL_STYLE,Styles);

                FirstMatchingItemSelected=TRUE;
            }
        }
    }

    // if one item at least is found
    if (FirstMatchingItemSelected)
    {
        // display combo content
        SendMessage(this->hWndComboSearch,CB_SHOWDROPDOWN,(WPARAM)TRUE,0);
    }
    SearchedFunction[_tcslen(SearchedFunction)-1]=0;
    // restore text cause CB_SHOWDROPDOWN,FALSE change content
    SendMessage(this->ComboSearchInfos.hwndItem,WM_SETTEXT,0,(LPARAM)(&SearchedFunction[1]));
    SetFocus(this->ComboSearchInfos.hwndItem);
    SendMessage(this->ComboSearchInfos.hwndItem,EM_SETSEL,SearchedFunctionLen,SearchedFunctionLen);

}

//-----------------------------------------------------------------------------
// Name: SelectSearchedFunctionInListView
// Object: search and select the function selected with the combobox
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::SelectSearchedFunctionInListView()
{
    TCHAR* SearchedFunction;
    LONG SearchedFunctionLen;

    TCHAR* FunctionName;
    TCHAR FunctionDescription[MAX_CONFIG_FILE_LINE_SIZE];

    LONG ItemCount;
    LONG ItemIndex;
    LONG Cnt;

    ItemIndex=(LONG)SendMessage(this->hWndComboSearch,CB_GETCURSEL,0,0);
    if (ItemIndex==CB_ERR)
        return;

    // get searched function len 
    SearchedFunctionLen=(LONG)SendMessage(this->hWndComboSearch,CB_GETLBTEXTLEN,ItemIndex,0);
    if (SearchedFunctionLen<=0)
        return;
    // allocate memory 
    SearchedFunction=(TCHAR*) _alloca((SearchedFunctionLen+1)*sizeof(TCHAR));// 1 for null

    // get searched function 
    if (!SendMessage(this->hWndComboSearch,CB_GETLBTEXT,ItemIndex,(LPARAM)SearchedFunction))
        return;

    // search in function listview 
    ItemCount=this->pListviewFunctions->GetItemCount();

    // for each listview item
    for (Cnt=0;Cnt<ItemCount;Cnt++)
    {
        // get description text
        this->pListviewFunctions->GetItemText(Cnt,FunctionDescription,MAX_CONFIG_FILE_LINE_SIZE);

        // get associated function name
        FunctionName=this->GetFunctionName(FunctionDescription);

        // if function match
        if (_tcscmp(SearchedFunction,FunctionName)==0)
        {
            // set single selection style
            LONG_PTR Styles=GetWindowLongPtr(this->pListviewFunctions->GetControlHandle(),GWL_STYLE);
            SetWindowLongPtr(this->pListviewFunctions->GetControlHandle(),GWL_STYLE,Styles|LVS_SINGLESEL);

            // select item
            this->pListviewFunctions->SetSelectedIndex(Cnt);
            ListView_EnsureVisible(this->pListviewFunctions->GetControlHandle(),Cnt,FALSE);

            // restore multiple selection
            Styles&=~LVS_SINGLESEL;
            SetWindowLongPtr(this->pListviewFunctions->GetControlHandle(),GWL_STYLE,Styles);

            return;
        }
    }
}

//-----------------------------------------------------------------------------
// Name: GetLogInfoFromSelectedFunction
// Object: get monitoring file 'raw' description for selected function
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
CMonitoringWizard::LOGFILELINE* CMonitoringWizard::GetLogInfoFromSelectedFunction()
{
    CLinkListItem* pItem;

    if(!this->pListviewFunctions->GetItemUserData(this->pListviewFunctions->GetSelectedIndex(),(LPVOID*)&pItem))
        return NULL;

    if(IsBadReadPtr(pItem,sizeof(CLinkListItem)))
        return NULL;

    LOGFILELINE* pLog;
    pLog=(LOGFILELINE*)pItem->ItemData;

    if(IsBadReadPtr(pLog,sizeof(LOGFILELINE)))
        return NULL;

    return pLog;
}

//-----------------------------------------------------------------------------
// Name: AddFunctionDescription
// Object: add function to listview according to it's definition
// Parameters :
//     in  : LOGFILELINE* pLog : function definition
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::AddFunctionDescription(LOGFILELINE* pLog)
{
    TCHAR DllName[MAX_PATH];
    TCHAR pszLineNumber[32];
    int ItemIndex;
    CLinkListItem* pItem;
    TCHAR*  psz;

    // add log struct to pFileLines link list
    pItem=this->pFileLines->AddItem(pLog);

    // if not an empty line
    if (_tcslen(pLog->pszContent)>0)
    {
        // if not a comment
        if (*pLog->pszContent!=';')
        {
            psz=_tcschr(pLog->pszContent,'|');
            if (psz)
            {
                // split module name from func name
                _tcsncpy(DllName,pLog->pszContent,psz-pLog->pszContent);
                DllName[psz-pLog->pszContent]=0;

                // make psz point to function description
                psz++;

                _stprintf(pszLineNumber,_T("%u"),pLog->dwLineNumber);
                ItemIndex=this->pListviewFunctions->AddItem(psz,pItem);
                this->pListviewFunctions->SetItemText(ItemIndex,1,DllName);
                this->pListviewFunctions->SetItemText(ItemIndex,2,pszLineNumber);

                // set function checked state
                if (pLog->bStateEnabled)
                    this->pListviewFunctions->SetSelectedState(ItemIndex,TRUE);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Name: SetFunctionDescription
// Object: update function definition in listview
// Parameters :
//     in  : TCHAR* pszDescription : new function definition
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::SetFunctionDescription(TCHAR* pszDescription)
{
    TCHAR* pszLocDescr;
    TCHAR*  psz;
    int ItemIndex;

    pszLocDescr=_tcsdup(pszDescription);
    ItemIndex=this->pListviewFunctions->GetSelectedIndex();

    // split module name from func description
    psz=_tcschr(pszLocDescr,'|');
    if (psz)
    {
        // ends module name
        *psz=0;
        // point to func description
        psz++;
        this->pListviewFunctions->SetItemText(ItemIndex,0,psz);
        this->pListviewFunctions->SetItemText(ItemIndex,1,pszLocDescr);
    }
    delete pszLocDescr;
}

//-----------------------------------------------------------------------------
// Name: CheckFunctionDescription
// Object: check that provided function description has no syntax error
// Parameters :
//     in  : TCHAR* pszDescription : function definition
//     out :
//     return : TRUE if no syntax error, FALSE if syntx error
//-----------------------------------------------------------------------------
BOOL CMonitoringWizard::CheckFunctionDescription(TCHAR* pszDescription)
{
    TCHAR* pszLocalFuncDesc=0;
    TCHAR* psz;
    TCHAR* psz2;
    try
    {
        pszLocalFuncDesc=_tcsdup(pszDescription);
        // assume to have DllNameLike|FuncName()
        psz=_tcschr(pszLocalFuncDesc,'|');
        if (!psz)
            throw FALSE;

        psz++;
        // find (
        psz2=_tcschr(psz,'(');
        if (!psz2)
            throw FALSE;
        *psz2=0;
        // psz contains func name, check if not empty
        CTrimString::TrimString(psz);
        if (*psz==0)
            throw FALSE;

        // point after '('
        psz2++;
        // assume user ends params
        psz=_tcschr(psz2,')');
        if (!psz)
            throw FALSE;

        throw TRUE;
    }
    catch(BOOL bRes)
    {
        if (!bRes)
        {
            MessageBox(this->hWndDialog,_T("Bad function description"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        }
        if (pszLocalFuncDesc)
            delete pszLocalFuncDesc;
        return bRes;
    }
}

//-----------------------------------------------------------------------------
// Name: AddFunctionDescription
// Object: add new function description
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::AddFunctionDescription()
{
    // commit previous changes if any (function selection / unselection)
    this->CommitChanges();

    this->bManualChangesDone=TRUE;

    // get func desc
    TCHAR pszDef[MAX_CONFIG_FILE_LINE_SIZE];
    GetDlgItemText(this->hWndDialog,IDC_EDIT_WIZARD_ADD,pszDef,MAX_CONFIG_FILE_LINE_SIZE);

    // check function description
    if (!this->CheckFunctionDescription(pszDef))
        return;


    LOGFILELINE Log;

    // copy definition
    Log.pszContent=_tcsdup(pszDef);
    Log.dwLineNumber=0;
    Log.bStateEnabled=TRUE;

    // add content to the function listview
    this->AddFunctionDescription(&Log);

    // set single selection style
    LONG_PTR Styles=GetWindowLongPtr(this->pListviewFunctions->GetControlHandle(),GWL_STYLE);
    SetWindowLongPtr(this->pListviewFunctions->GetControlHandle(),GWL_STYLE,Styles|LVS_SINGLESEL);

    // ensure visible and resorted
    this->pListviewFunctions->SetSelectedIndex(this->pListviewFunctions->GetItemCount()-1);
    this->pListviewFunctions->ReSort();
    this->pListviewFunctions->ScrollTo(this->pListviewFunctions->GetSelectedIndex());

    // restore multiple selection
    Styles&=~LVS_SINGLESEL;
    SetWindowLongPtr(this->pListviewFunctions->GetControlHandle(),GWL_STYLE,Styles);

    // save current change
    this->CommitChanges();
}

//-----------------------------------------------------------------------------
// Name: ModifySelectedFunctionDescription
// Object: modify an existing function description
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::ModifySelectedFunctionDescription()
{
    // commit previous changes if any (function selection / unselection)
    this->CommitChanges();

    this->bManualChangesDone=TRUE;

    // get func desc
    TCHAR pszDef[MAX_CONFIG_FILE_LINE_SIZE];
    GetDlgItemText(this->hWndDialog,IDC_EDIT_WIZARD_MODIFY,pszDef,MAX_CONFIG_FILE_LINE_SIZE);

    // check function description
    if (!this->CheckFunctionDescription(pszDef))
        return;

    LOGFILELINE* pLog;
    pLog=this->GetLogInfoFromSelectedFunction();
    if (!pLog)
        return;

    // free old content
    if (pLog->pszContent)
        free(pLog->pszContent);

    // copy new one
    pLog->pszContent=_tcsdup(pszDef);

    // update content of the function listview
    this->SetFunctionDescription(pszDef);

    // save current change
    this->CommitChanges();
}

//-----------------------------------------------------------------------------
// Name: RemoveSelectedFunctionDescription
// Object: remove an existing function description
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::RemoveSelectedFunctionDescription()
{
    // commit previous changes if any (function selection / unselection)
    this->CommitChanges();

    this->bManualChangesDone=TRUE;

    CLinkListItem* pItem;
    if(!this->pListviewFunctions->GetItemUserData(this->pListviewFunctions->GetSelectedIndex(),(LPVOID*)&pItem))
        return;

    // remove item from listview
    this->pListviewFunctions->RemoveItem(this->pListviewFunctions->GetSelectedIndex());
    // remove item from function list
    this->pFileLines->RemoveItem(pItem);

    // save current change
    this->CommitChanges();
}

//-----------------------------------------------------------------------------
// Name: GetAssociatedDefaultFile
// Object: get file containing default values associated to current selected file 
// Parameters :
//     in  : 
//     out : TCHAR* DefaultFile : must be at least MAX_PATH
//     return : TRUE if associated default file exists, FALSE else
//-----------------------------------------------------------------------------
BOOL CMonitoringWizard::GetAssociatedDefaultFile(OUT TCHAR* DefaultFile)
{
    TCHAR CurrentFile[MAX_PATH];
    TCHAR* psz;

    // get current selected file
    this->GetSelectedFilePath(CurrentFile);

    // get filename
    psz=CStdFileOperations::GetFileName(CurrentFile);

    // get default path directory
    CStdFileOperations::GetAppPath(DefaultFile,MAX_PATH);

    // get the default value directories
    switch(this->MonitoringWizardType)
    {
    case MonitoringWizardType_API:
        _tcscat(DefaultFile,MONITORING_WIZARD_MONITORING_FILES_DEFAULT_VALUES_PATH);
        break;
    case MonitoringWizardType_COM:
        _tcscat(DefaultFile,MONITORING_WIZARD_MONITORING_COM_FILES_DEFAULT_VALUES_PATH);
        break;
    }
    _tcscat(DefaultFile,psz);

    // check if default file exists
    return CStdFileOperations::DoesFileExists(DefaultFile);
}

//-----------------------------------------------------------------------------
// Name: RestoreDefaultValues
// Object: restore current selected file default values
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::RestoreDefaultValues()
{
    TCHAR CurrentFile[MAX_PATH];

    // get current selected file
    this->GetSelectedFilePath(CurrentFile);

    TCHAR DefaultFile[MAX_PATH];
    if (!this->GetAssociatedDefaultFile(DefaultFile))
        return;

    // copy file
    if (!CopyFile(DefaultFile,CurrentFile,FALSE))
    {
        CAPIError::ShowLastError();
        return;
    }

    // parse new file
    this->UpdateFunctionsList(CurrentFile);
}

//-----------------------------------------------------------------------------
// Name: FullEditAssociatedFile
// Object: fully edit monitoring file
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::FullEditAssociatedFile()
{
    TCHAR pszFileName[MAX_PATH];

    // commit changes if any before opening file
    this->CommitChanges();

    // get file name
    this->GetSelectedFilePath(pszFileName);

    // launch default .txt editor
    SHELLEXECUTEINFO shInfo={0};
    shInfo.cbSize=sizeof(SHELLEXECUTEINFO);
    shInfo.hwnd=this->hWndDialog;
    shInfo.lpVerb=_T("edit");
    shInfo.lpFile=pszFileName;
    shInfo.fMask=SEE_MASK_NOCLOSEPROCESS;
    shInfo.nShow=SW_SHOWNORMAL;
 
    if (!ShellExecuteEx(&shInfo))
        return;

    // As we use WaitForSingleObjec, in WndProc,
    // messages are not translated to the window.
    // to avoid a not responding interface, hide user interface
    // until WaitForSingleObject ends
    ShowWindow(this->hWndDialog,SW_HIDE);

    // wait for the end of created process
    WaitForSingleObject(shInfo.hProcess,INFINITE);

    // restore interface
    ShowWindow(this->hWndDialog,SW_SHOW);

    // parse file again
    this->CallbackListViewFileSelection(this->dwLastSelectedFileIndex);
}

//-----------------------------------------------------------------------------
// Name: OnSizing
// Object: check dialog box size
// Parameters :
//     in out  : RECT* pRect : pointer to dialog rect
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::OnSizing(RECT* pRect)
{
    // check min width and min height
    if ((pRect->right-pRect->left)<CMonitoringWizard_DIALOG_MIN_WIDTH)
        pRect->right=pRect->left+CMonitoringWizard_DIALOG_MIN_WIDTH;
    if ((pRect->bottom-pRect->top)<CMonitoringWizard_DIALOG_MIN_HEIGHT)
        pRect->bottom=pRect->top+CMonitoringWizard_DIALOG_MIN_HEIGHT;
}

//-----------------------------------------------------------------------------
// Name: OnSize
// Object: Resize controls
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMonitoringWizard::OnSize()
{
    RECT RectWindow;
    RECT RectOKButton;
    RECT RectCancelButton;
    RECT RectGroupSelectApiFile;
    RECT RectGroupSelectApiFunction;
    RECT RectQuickEdit;
    RECT RectSearch;
    RECT Rect;
    RECT PreviousRect;
    HWND hItem;
    DWORD Spacer;
    DWORD OKCancelSpace;
    POINT pt;

    // get margin
    hItem=GetDlgItem(this->hWndDialog,IDC_GROUP_SELECT_API_FILE);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
    Spacer=Rect.left;

    // get dialog rect
    CDialogHelper::GetClientWindowRect(this->hWndDialog,this->hWndDialog,&RectWindow);

    // buttons OK Cancel
    hItem=GetDlgItem(this->hWndDialog,IDCANCEL);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&RectCancelButton);
    hItem=GetDlgItem(this->hWndDialog,IDOK);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&RectOKButton);
    OKCancelSpace=RectCancelButton.left-RectOKButton.right;

    SetWindowPos(hItem,HWND_NOTOPMOST,
        ((RectWindow.right-RectWindow.left) - (RectCancelButton.right-RectOKButton.left))/2 - Spacer + RectWindow.left,
        RectWindow.bottom-Spacer-(RectOKButton.bottom-RectOKButton.top),
        0,
        0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&RectOKButton);
    CDialogHelper::Redraw(hItem);

    hItem=GetDlgItem(this->hWndDialog,IDCANCEL);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RectOKButton.right + OKCancelSpace,
        RectOKButton.top,
        0,
        0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    CDialogHelper::Redraw(hItem);

    // IDC_GROUP_SELECT_API_FILE
    hItem=GetDlgItem(this->hWndDialog,IDC_GROUP_SELECT_API_FILE);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        0,
        0,
        Rect.right-Rect.left,
        RectOKButton.top - SPACE_BETWEEN_CONTROLS - Rect.top,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&RectGroupSelectApiFile);
    CDialogHelper::Redraw(hItem);

    // listview monitoring files
    hItem=this->pListviewFiles->GetControlHandle();
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        0,
        0,
        Rect.right-Rect.left,
        RectGroupSelectApiFile.bottom - SPACE_BETWEEN_CONTROLS - Rect.top,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(hItem);

    // group functions
    hItem=GetDlgItem(this->hWndDialog,IDC_GROUP_SELECT_API_FUNCTION);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        0,
        0,
        RectWindow.right-RectWindow.left - RectGroupSelectApiFile.right-SPACE_BETWEEN_CONTROLS - Spacer + RectWindow.left,
        RectGroupSelectApiFile.bottom - RectGroupSelectApiFile.top,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&RectGroupSelectApiFunction);
    CDialogHelper::Redraw(hItem);

    // group quick edit
    hItem=GetDlgItem(this->hWndDialog,IDC_STATIC_GROUP_QUICK_EDIT);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
    pt.x=RectGroupSelectApiFunction.left+SPACE_BETWEEN_CONTROLS;
    pt.y=RectGroupSelectApiFunction.bottom-SPACE_BETWEEN_CONTROLS- (Rect.bottom-Rect.top);
    CDialogHelper::MoveGroupTo(this->hWndDialog,hItem,&pt);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&RectQuickEdit);

    // group search
    hItem=GetDlgItem(this->hWndDialog,IDC_STATIC_GROUP_SEARCH);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
    pt.x=RectQuickEdit.left;
    pt.y=RectQuickEdit.top-SPACE_BETWEEN_CONTROLS-(Rect.bottom-Rect.top);
    CDialogHelper::MoveGroupTo(this->hWndDialog,hItem,&pt);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&RectSearch);

    // buttons
    hItem=GetDlgItem(this->hWndDialog,IDC_BUTTON_UNCHECK_SELECTED);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RectGroupSelectApiFunction.right -SPACE_BETWEEN_CONTROLS - (Rect.right-Rect.left),
        RectSearch.top-SPACE_BETWEEN_CONTROLS- (Rect.bottom-Rect.top),
        0,
        0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&PreviousRect);
    CDialogHelper::Redraw(hItem);

    hItem=GetDlgItem(this->hWndDialog,IDC_BUTTON_CHECK_SELECTED);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        PreviousRect.left -SPACE_BETWEEN_CONTROLS - (Rect.right-Rect.left),
        PreviousRect.top,
        0,
        0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&PreviousRect);
    CDialogHelper::Redraw(hItem);

    hItem=GetDlgItem(this->hWndDialog,IDC_BUTTON_UNCHECK_ALL);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        PreviousRect.left -SPACE_BETWEEN_CONTROLS - (Rect.right-Rect.left),
        PreviousRect.top,
        0,
        0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&PreviousRect);
    CDialogHelper::Redraw(hItem);

    hItem=GetDlgItem(this->hWndDialog,IDC_BUTTON_CHECK_ALL);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        PreviousRect.left -SPACE_BETWEEN_CONTROLS - (Rect.right-Rect.left),
        PreviousRect.top,
        0,
        0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&PreviousRect);
    CDialogHelper::Redraw(hItem);

    // BUTTON_DEFAULT_VALUES
    hItem=GetDlgItem(this->hWndDialog,IDC_BUTTON_DEFAULT_VALUES);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        Rect.left,
        PreviousRect.top,
        0,
        0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&PreviousRect);
    CDialogHelper::Redraw(hItem);

    // listview
    hItem=this->pListviewFunctions->GetControlHandle();
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        0,
        0,
        RectGroupSelectApiFunction.right-RectGroupSelectApiFunction.left-2*SPACE_BETWEEN_CONTROLS,
        PreviousRect.top - SPACE_BETWEEN_CONTROLS - Rect.top,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(hItem);
}