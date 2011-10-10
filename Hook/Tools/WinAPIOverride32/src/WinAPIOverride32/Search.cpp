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

#include "search.h"

extern CLinkList* pLogList;
extern CListview* pListview;
extern void CrossThreadSetFocus(HWND hWnd);

CSearch::CSearch(void)
{
    this->pParameters=NULL;

    this->hWndDialog = NULL;
    this->bCheckApiName = FALSE;
    this->bCheckModuleName = FALSE;
    this->bCheckCallingModuleName = FALSE;
    this->bCheckCallerAddress = FALSE;
    this->bCheckProcessID = FALSE;
    this->bCheckThreadID = FALSE;
    this->bCheckResult = FALSE;
    this->bCheckFloatingResult = FALSE;
    this->bCheckStartMin = FALSE;
    this->bCheckStartMax = FALSE;
    this->bCheckDurationMin = FALSE;
    this->bCheckDurationMax = FALSE;
    this->bCheckEaxBefore = FALSE;
    this->bCheckEbxBefore = FALSE;
    this->bCheckEcxBefore = FALSE;
    this->bCheckEdxBefore = FALSE;
    this->bCheckEsiBefore = FALSE;
    this->bCheckEdiBefore = FALSE;
    this->bCheckEflBefore = FALSE;
    this->bCheckEaxAfter = FALSE;
    this->bCheckEbxAfter = FALSE;
    this->bCheckEcxAfter = FALSE;
    this->bCheckEdxAfter = FALSE;
    this->bCheckEsiAfter = FALSE;
    this->bCheckEdiAfter = FALSE;
    this->bCheckEflAfter = FALSE;
    this->bCheckParameters = FALSE;
    this->ShouldMatchAllParamValues = FALSE;
    this->UserInputFieldError = FALSE;

}

CSearch::~CSearch(void)
{
    this->FreepParameters();
}

//-----------------------------------------------------------------------------
// Name: FreepParameters
// Object: free memory associated with parameters conditions
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::FreepParameters()
{
    if (!this->pParameters)
        return;
    // delete parameters
    CLinkListItem* pItem;
    PARAMETER_SEARCH_OPTION* pParamOption;
    
    CLinkList* pList;
    pList=this->pParameters;
    this->pParameters=NULL;

    pList->Lock();
    for (pItem=pList->Head;pItem;pItem=pItem->NextItem)
    {
        // free pointed value if any
        pParamOption=(PARAMETER_SEARCH_OPTION*)pItem->ItemData;
        if (pParamOption->PointedValue)
        {
            delete pParamOption->PointedValue;
            pParamOption->PointedValue=NULL;
        }
    }
    pList->RemoveAllItems(TRUE);
    pList->Unlock();
    delete pList;
}
//-----------------------------------------------------------------------------
// Name: ModelessDialogThread
// Object: allow to act like a dialog box in modeless mode
// Parameters :
//     in  : PVOID lParam : HINSTANCE hInstance : application instance
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CSearch::ModelessDialogThread(PVOID lParam)
{
    CSearch* pSearch = (CSearch*) lParam;
    DialogBoxParam (pSearch->hInstance,(LPCTSTR)IDD_DIALOG_SEARCH,NULL,(DLGPROC)CSearch::WndProc,(LPARAM)pSearch);
    delete pSearch;
    return 0;
}

//-----------------------------------------------------------------------------
// Name: ShowDialog
// Object: create a modeless compare dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::ShowDialog(HINSTANCE hInstance,HWND hWndDialog)
{
    // show dialog
    // don't use hWndDialog to allow to put winapioverride to an upper Z-order
    
    CSearch* pSearch=new CSearch();
    pSearch->hWndParent = hWndDialog;
    pSearch->hInstance = hInstance;
    // create thread instead of using CreateDialogParam to don't have to handle keyboard event like TAB
    CloseHandle(CreateThread(NULL,0,CSearch::ModelessDialogThread,pSearch,0,NULL));
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::Init()
{

}

//-----------------------------------------------------------------------------
// Name: Close
// Object: EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::Close()
{
    EndDialog(this->hWndDialog,0);
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: search dialog window proc
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CSearch::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // init dialog
            CSearch* pSearch;
            pSearch=(CSearch*)lParam;
            pSearch->hWndDialog=hWnd;
            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pSearch);

            pSearch->Init();
            // load dlg icons
            CDialogHelper::SetIcon(hWnd,IDI_ICON_SEARCH);
           
        }
        break;
    case WM_CLOSE:
        // close dialog
        ((CSearch*)GetWindowLongPtr(hWnd,GWLP_USERDATA))->Close();
        break;
    case WM_COMMAND:
        {
            CSearch* pSearch=((CSearch*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pSearch)
                break;

            switch (LOWORD(wParam))
            {
                case IDC_BUTTON_SEARCH_FIND:
                    pSearch->FindFromGUI();
                    break;
                case IDC_BUTTON_SEARCH_FIND_NEXT:
                    pSearch->FindNextFromGUI(pListview->GetSelectedIndex()+1);
                    break;
                case IDC_BUTTON_SEARCH_FIND_PREVIOUS:
                    pSearch->FindPreviousFromGUI(pListview->GetSelectedIndex()-1);
                    break;
                case IDC_BUTTON_SEARCH_SELECT_ALL:
                    pSearch->SelectAllFromGUI();
                    break;
                case IDCANCEL:
                    pSearch->Close();
                    break;
            }
        }
    default:
        return FALSE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: GetValue
// Object: retrieve PBYTE value of specified control
// Parameters :
//     in  : int ControlID : id of control we want to retrieve value
//     out : PBYTE* pValue : pointer to Value of control
//     return : TRUE if there's data inside control, false else
//-----------------------------------------------------------------------------
BOOL CSearch::GetValue(int ControlID,PBYTE* pValue)
{
    TCHAR psz[32];
    if (!this->GetValue(ControlID,psz,31))
        return FALSE;

    if (!CStringConverter::StringToPBYTE(psz,pValue))
    {
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("Bad Value : %s"),psz);
        MessageBox(this->hWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        this->UserInputFieldError=TRUE;
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetValue
// Object: retrieve DWORD value of specified control
// Parameters :
//     in  : int ControlID : id of control we want to retrieve value
//     out : DWORD* pValue : pointer to Value of control
//     return : TRUE if there's data inside control, false else
//-----------------------------------------------------------------------------
BOOL CSearch::GetValue(int ControlID,DWORD* pValue)
{
    TCHAR psz[32];
    int iScanfRes;
    if (!this->GetValue(ControlID,psz,31))
        return FALSE;
    if(_tcsnicmp(psz,_T("0x"),2)==0)
        iScanfRes=_stscanf(psz+2,_T("%x"),pValue);
    else
        iScanfRes=_stscanf(psz,_T("%u") ,pValue);

    if (iScanfRes!=1)
    {
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("Bad Value : %s"),psz);
        MessageBox(this->hWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        this->UserInputFieldError=TRUE;
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetValue
// Object: retrieve DWORD value of specified control
// Parameters :
//     in  : int ControlID : id of control we want to retrieve value
//     out : double* pValue : pointer to Value of control
//     return : TRUE if there's data inside control, false else
//-----------------------------------------------------------------------------
BOOL CSearch::GetValue(int ControlID,double* pValue)
{
    TCHAR psz[50];
    if (!this->GetValue(ControlID,psz,50))
        return FALSE;

    if (!_stscanf(psz,_T("%g"),pValue))
    {
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("Bad Value : %s"),psz);
        MessageBox(this->hWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        this->UserInputFieldError=TRUE;
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetValue
// Object: retrieve string of specified control
// Parameters :
//     in  : int ControlID : id of control we want to retrieve value
//           DWORD MaxChar : max length of pValue in TCHAR
//     out : TCHAR* pValue : pointer to Value of control
//     return : TRUE if there's data inside control, FALSE else
//-----------------------------------------------------------------------------
BOOL CSearch::GetValue(int ControlID,TCHAR* pValue,DWORD MaxChar)
{
    GetDlgItemText(this->hWndDialog,ControlID,pValue,MaxChar);
    if (*pValue==0)
        return FALSE;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: EncodeTime
// Object: encode an hh:mm:ss:mmm encoded time into a DWORD as FILETIME struct does
// Parameters :
//     in  : TCHAR String : string in the hh:mm:ss:mmm format
//     out :FILETIME* pValue : Time encoded like in the FILETIME struct
//     return : TRUE on successfull encoding
//-----------------------------------------------------------------------------
BOOL CSearch::EncodeTime(TCHAR* String,FILETIME* pValue)
{
    memset(pValue,0,sizeof(FILETIME));
    DWORD H;
    DWORD M;
    DWORD S;
    DWORD MS;
    int iScanRes=_stscanf(String,_T("%u:%u:%u:%u"),
                        &H,
                        &M,
                        &S,
                        &MS
                        );
    if (iScanRes!=4)
    {
        MessageBox(this->hWndDialog,_T("Bad Time Format, you have to enter an hh:mm:ss:mmm time format"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        this->UserInputFieldError=TRUE;
        return FALSE;
    }

    // get time in ms
    ULONGLONG ul=MS%1000 +(S%60)*1000 +(M%60)*60*1000 +(H%24)*60*60*1000;
    // translate it into 100ns
    ul*=10000;
    pValue->dwHighDateTime=(DWORD)(ul>>32);
    pValue->dwLowDateTime=(DWORD)ul;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: UpdateSearchFields
// Object: update search param depending user interface
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CSearch::UpdateSearchFields()
{
    this->UserInputFieldError=FALSE;

    TCHAR psz[MAX_PATH];
    this->bCheckApiName=this->GetValue(IDC_EDIT_SEARCH_FUNCTION_NAME,this->ApiName,MAX_PATH);
    this->bCheckModuleName=this->GetValue(IDC_EDIT_SEARCH_MODULE_NAME,this->ModuleName,MAX_PATH);
    this->bCheckCallingModuleName=this->GetValue(IDC_EDIT_SEARCH_CALLING_MODULE_NAME,this->CallingModuleName,MAX_PATH);
    this->bCheckCallerAddress=this->GetValue(IDC_EDIT_SEARCH_CALLER_ADDRESS,&this->CallerAddress);
    this->bCheckProcessID=this->GetValue(IDC_EDIT_SEARCH_PROCESS_ID,&this->ProcessID);
    this->bCheckThreadID=this->GetValue(IDC_EDIT_SEARCH_THREAD_ID,&this->ThreadID);
    this->bCheckResult=this->GetValue(IDC_EDIT_SEARCH_RESULT,&this->Result);
    this->bCheckFloatingResult=this->GetValue(IDC_EDIT_SEARCH_FLOATING_RESULT,&this->FloatingResult);

    this->bCheckStartMin=this->GetValue(IDC_EDIT_SEARCH_TIME_MIN,psz,MAX_PATH);
    if (this->bCheckStartMin)
        this->bCheckStartMin=this->EncodeTime(psz,&this->StartMin);

    this->bCheckStartMax=this->GetValue(IDC_EDIT_SEARCH_TIME_MAX,psz,MAX_PATH);
    if (this->bCheckStartMax)
        this->bCheckStartMax=this->EncodeTime(psz,&this->StartMax);

    this->bCheckDurationMin=this->GetValue(IDC_EDIT_SEARCH_DURATION_MIN,&this->DurationMin);
    this->bCheckDurationMax=this->GetValue(IDC_EDIT_SEARCH_DURATION_MAX,&this->DurationMax);

    this->bCheckEaxBefore=this->GetValue(IDC_EDIT_SEARCH_REGISTERS_EAX_BEFORE_CALL,&this->EaxBefore);
    this->bCheckEbxBefore=this->GetValue(IDC_EDIT_SEARCH_REGISTERS_EBX_BEFORE_CALL,&this->EbxBefore);
    this->bCheckEcxBefore=this->GetValue(IDC_EDIT_SEARCH_REGISTERS_ECX_BEFORE_CALL,&this->EcxBefore);
    this->bCheckEdxBefore=this->GetValue(IDC_EDIT_SEARCH_REGISTERS_EDX_BEFORE_CALL,&this->EdxBefore);
    this->bCheckEsiBefore=this->GetValue(IDC_EDIT_SEARCH_REGISTERS_ESI_BEFORE_CALL,&this->EsiBefore);
    this->bCheckEdiBefore=this->GetValue(IDC_EDIT_SEARCH_REGISTERS_EDI_BEFORE_CALL,&this->EdiBefore);
    this->bCheckEflBefore=this->GetValue(IDC_EDIT_SEARCH_REGISTERS_EFL_BEFORE_CALL,&this->EflBefore);

    this->bCheckEaxAfter=this->GetValue(IDC_EDIT_SEARCH_REGISTERS_EAX_AFTER_CALL,&this->EaxAfter);
    this->bCheckEbxAfter=this->GetValue(IDC_EDIT_SEARCH_REGISTERS_EBX_AFTER_CALL,&this->EbxAfter);
    this->bCheckEcxAfter=this->GetValue(IDC_EDIT_SEARCH_REGISTERS_ECX_AFTER_CALL,&this->EcxAfter);
    this->bCheckEdxAfter=this->GetValue(IDC_EDIT_SEARCH_REGISTERS_EDX_AFTER_CALL,&this->EdxAfter);
    this->bCheckEsiAfter=this->GetValue(IDC_EDIT_SEARCH_REGISTERS_ESI_AFTER_CALL,&this->EsiAfter);
    this->bCheckEdiAfter=this->GetValue(IDC_EDIT_SEARCH_REGISTERS_EDI_AFTER_CALL,&this->EdiAfter);
    this->bCheckEflAfter=this->GetValue(IDC_EDIT_SEARCH_REGISTERS_EFL_AFTER_CALL,&this->EflAfter);



    // retrieve parameters options
    // sample command line
    // Param1Value=32 Param2Value=0x16 Param3PointedValue=FFAB3300
    LRESULT pszParamSize;
    pszParamSize = SendMessage(GetDlgItem(this->hWndDialog,IDC_EDIT_SEARCH_PARAMETERS),
                          WM_GETTEXTLENGTH,
                          0,
                          0);
    this->bCheckParameters=FALSE;
    if (pszParamSize)
    {
        pszParamSize++;//WM_GETTEXTLENGTH don't include \0
        TCHAR* pszParam=new TCHAR[pszParamSize];
        // retrieve full text from user interface
        GetDlgItemText(this->hWndDialog,IDC_EDIT_SEARCH_PARAMETERS,pszParam,(int)pszParamSize);

        // create a new param option list
        this->FreepParameters();
        this->pParameters=new CLinkList(sizeof(PARAMETER_SEARCH_OPTION));

        TCHAR* CurrentParamPos;
        TCHAR* NextParamPos;
        PARAMETER_SEARCH_OPTION ParamOption;
        CurrentParamPos=pszParam;

        // trim string
        CurrentParamPos=CTrimString::TrimString(CurrentParamPos);
        while (CurrentParamPos)
        {
            // find next splitter
            NextParamPos=_tcsstr(CurrentParamPos,CSEARCH_PARAM_SPLITTER);
            if (NextParamPos)
            {
                // ends CurrentParamPos string
                *NextParamPos=0;
                // point to next condition
                NextParamPos+=_tcslen(CSEARCH_PARAM_SPLITTER);
            }

            // trim string
            CurrentParamPos=CTrimString::TrimString(CurrentParamPos);

            // if condition is valid
            if (this->ParseParamCondition(CurrentParamPos,&ParamOption))
                // add it to list
                this->pParameters->AddItem(&ParamOption);

            // find next condition
            CurrentParamPos=NextParamPos;
        }

        // assume there's at least on defined param
        if (this->pParameters->GetItemsCount()>0)
            this->bCheckParameters=TRUE;

        delete[] pszParam;
    }

    this->ShouldMatchAllParamValues=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_SEARCH_PARAM_MUST_MATCH_ALL_VALUES)==BST_CHECKED);

    return (!this->UserInputFieldError);
}

//-----------------------------------------------------------------------------
// Name: ParseParamCondition
// Object: parse a single param optional condition
// Parameters :
//     in  : int ItemIndex : item index of dialog main pListview
//     out :
//     return : TRUE on successful parsing FALSE else
//-----------------------------------------------------------------------------
BOOL CSearch::ParseParamCondition(TCHAR* StringCondition,PARAMETER_SEARCH_OPTION* pParamOption)
{
    BOOL Ret=TRUE;

    // sample command line
    // Param1Value=32 Param2Value=0x16 Param3PointedValue=FFAB3300

    // if empty command return FALSE
    if (*StringCondition==0)
        return FALSE;

    // initialize pParamOption
    memset(pParamOption,0,sizeof(PARAMETER_SEARCH_OPTION));

    
    // make a local copy of StringCondition
    TCHAR* Condition=_tcsdup(StringCondition);

    // put Condition in lower case to make insensitive search
    _tcslwr(Condition);

    int iScanfRes;
    TCHAR* psz=new TCHAR[100+_tcslen(Condition)+1];

    pParamOption->ParamValueSize=0;
    pParamOption->PointedValueSize=0;

    // check for ParamXPointedValue=
    if (_stscanf(Condition,_T("param%upointedvalue=%s"),&pParamOption->ParamNumber,psz)==2)
    {
        // it's ok there's a pointed data defined
        pParamOption->PointedValue=CStrToHex::StrHexArrayToByteArray(psz,&pParamOption->PointedValueSize);
    }
    // check for ParamXBufferValue=
    else if(_stscanf(Condition,_T("param%ustructvalue=%s"),&pParamOption->ParamNumber,psz)==2)
    {
        // it's ok there's a buffer data defined
        pParamOption->PointedValue=CStrToHex::StrHexArrayToByteArray(psz,&pParamOption->ParamValueSize);        
    }
    else
    {
        iScanfRes=_stscanf(Condition,_T("param%uvalue=%s"),&pParamOption->ParamNumber,psz);
        if (iScanfRes==2)
        {
            // it's ok a value is defined

            // get value
            if (!CStringConverter::StringToPBYTE(psz,&pParamOption->ParamValue))
            {
                TCHAR pszMsg[MAX_PATH];
                _stprintf(pszMsg,_T("Bad Value : %s"),psz);
                MessageBox(this->hWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
                Ret=FALSE;
            }
        }
        else
        {
            // bad param format
            Ret=FALSE;
            _stprintf(psz,_T("Invalid Parameter Condition : %s"),StringCondition);
            MessageBox(this->hWndDialog,psz,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            this->UserInputFieldError=TRUE;
        }
    }

    
    delete[] psz;
    free(Condition);
    return Ret;
}

//-----------------------------------------------------------------------------
// Name: DoesItemMatch
// Object: check if item with index ItemIndex in main dialog list view match the filters
//          and if item match, select it
// Parameters :
//     in  : int ItemIndex : item index of dialog main pListview
//     out :
//     return : TRUE if item match, FALSE else
//-----------------------------------------------------------------------------
BOOL CSearch::DoesItemMatch(int ItemIndex)
{
    pLogList->Lock(TRUE);

    // get log entry
    LOG_LIST_ENTRY* pLogEntry=NULL;
    LOG_ENTRY* pLog;
    if(!pListview->GetItemUserData(ItemIndex,(LPVOID*)(&pLogEntry)))
    {
        pLogList->Unlock();
        return FALSE;
    }
    if (pLogEntry==0)
    {
        pLogList->Unlock();
        return FALSE;
    }
    if (IsBadReadPtr(pLogEntry,sizeof(LOG_LIST_ENTRY)))
    {
        pLogList->Unlock();
        return FALSE;
    }
    if (pLogEntry->Type!=ENTRY_LOG)
    {
        pLogList->Unlock();
        return FALSE;
    }
    if (IsBadReadPtr(pLogEntry->pLog,sizeof(LOG_ENTRY)))
    {
        pLogList->Unlock();
        return FALSE;
    }

    pLog=pLogEntry->pLog;

    // check values
    if (this->bCheckCallerAddress)
        if (pLog->pHookInfos->pOriginAddress!=this->CallerAddress)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckProcessID)
        if (pLog->pHookInfos->dwProcessId!=this->ProcessID)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckThreadID)
        if (pLog->pHookInfos->dwThreadId!=this->ThreadID)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckResult)
        if (pLog->pHookInfos->ReturnValue!=this->Result)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckStartMin)
    {
        ULONGLONG ulTime;
        ULONGLONG ulTimeMin;
        ulTime=(((ULONGLONG)pLog->pHookInfos->CallTime.dwHighDateTime)<<32)+pLog->pHookInfos->CallTime.dwLowDateTime;
        ulTime%=_DAY;
        ulTimeMin=(((ULONGLONG)this->StartMin.dwHighDateTime)<<32)+this->StartMin.dwLowDateTime;
        if (ulTime<ulTimeMin)
        {
            pLogList->Unlock();
            return FALSE;
        }
    }

    if (this->bCheckStartMax)
    {
        ULONGLONG ulTime;
        ULONGLONG ulTimeMax;
        ulTime=(((ULONGLONG)pLog->pHookInfos->CallTime.dwHighDateTime)<<32)+pLog->pHookInfos->CallTime.dwLowDateTime;
        ulTime%=_DAY;
        ulTimeMax=(((ULONGLONG)this->StartMax.dwHighDateTime)<<32)+this->StartMax.dwLowDateTime;
        if (ulTimeMax<ulTime)
        {
            pLogList->Unlock();
            return FALSE;
        }
    }

    if (this->bCheckDurationMin)
        if (pLog->pHookInfos->dwCallDuration<this->DurationMin)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckDurationMax)
        if (pLog->pHookInfos->dwCallDuration>this->DurationMax)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckEaxBefore)
        if (pLog->pHookInfos->RegistersBeforeCall.eax!=this->EaxBefore)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckEbxBefore)
        if (pLog->pHookInfos->RegistersBeforeCall.ebx!=this->EbxBefore)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckEcxBefore)
        if (pLog->pHookInfos->RegistersBeforeCall.ecx!=this->EcxBefore)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckEdxBefore)
        if (pLog->pHookInfos->RegistersBeforeCall.edx!=this->EdxBefore)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckEsiBefore)
        if (pLog->pHookInfos->RegistersBeforeCall.esi!=this->EsiBefore)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckEdiBefore)
        if (pLog->pHookInfos->RegistersBeforeCall.edi!=this->EdiBefore)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckEflBefore)
        if (pLog->pHookInfos->RegistersBeforeCall.efl!=this->EflBefore)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckEaxAfter)
        if (pLog->pHookInfos->RegistersAfterCall.eax!=this->EaxAfter)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckEbxAfter)
        if (pLog->pHookInfos->RegistersAfterCall.ebx!=this->EbxAfter)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckEcxAfter)
        if (pLog->pHookInfos->RegistersAfterCall.ecx!=this->EcxAfter)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckEdxAfter)
        if (pLog->pHookInfos->RegistersAfterCall.edx!=this->EdxAfter)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckEsiAfter)
        if (pLog->pHookInfos->RegistersAfterCall.esi!=this->EsiAfter)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckEdiAfter)
        if (pLog->pHookInfos->RegistersAfterCall.edi!=this->EdiAfter)
        {
            pLogList->Unlock();
            return FALSE;
        }

    if (this->bCheckEflAfter)
        if (pLog->pHookInfos->RegistersAfterCall.efl!=this->EflAfter)
        {
            pLogList->Unlock();
            return FALSE;
        }


    // check string at least as it's the most time consuming
    if (this->bCheckApiName)
        if (!CWildCharCompare::WildICmp(this->ApiName,pLog->pszApiName))
        {
            pLogList->Unlock();
            return FALSE;
        }
    if (this->bCheckModuleName)
        if (!CWildCharCompare::WildICmp(this->ModuleName,pLog->pszModuleName))
        {
            pLogList->Unlock();
            return FALSE;
        }
    if (this->bCheckCallingModuleName)
        if (!CWildCharCompare::WildICmp(this->CallingModuleName,pLog->pszCallingModuleName))
        {
            pLogList->Unlock();
            return FALSE;
        }


    // check params
    if (this->bCheckParameters)
    {
        BOOL ParameterOK;
        BOOL AllParametersOK=TRUE;
        BOOL AtLeastOneParameterOK=FALSE;
        int ParamIndex;

        CLinkListItem* pItem;
        PARAMETER_SEARCH_OPTION* pParamOption;
        this->pParameters->Lock();
        for (pItem=this->pParameters->Head;pItem;pItem=pItem->NextItem)
        {
            // free pointed value if any
            pParamOption=(PARAMETER_SEARCH_OPTION*)pItem->ItemData;

            ParameterOK=FALSE;

            // check param index
            if ((pParamOption->ParamNumber>pLog->pHookInfos->bNumberOfParameters)
                ||(pParamOption->ParamNumber==0))
            {
                ParameterOK=FALSE;
            }
            else
            {
                // ParamNumber is entered by user as 1 based index.
                // Put it to 0 based index for our array
                ParamIndex=pParamOption->ParamNumber-1;

                if (pParamOption->PointedValueSize)// check pointed value
                {
                    // assume buffer is enough
                    if (pLog->ParametersInfoArray[ParamIndex].dwSizeOfPointedValue<pParamOption->PointedValueSize)
                        ParameterOK=FALSE;
                    else if (memcmp(pParamOption->PointedValue,pLog->ParametersInfoArray[ParamIndex].pbValue,pParamOption->PointedValueSize)==0)
                        ParameterOK=TRUE;
                }
                else if(pParamOption->ParamValueSize)// check buffer value
                {
                    // assume buffer is enough
                    if (pLog->ParametersInfoArray[ParamIndex].dwSizeOfPointedValue<pParamOption->ParamValueSize)
                        ParameterOK=FALSE;
                    else if (memcmp(pParamOption->PointedValue,pLog->ParametersInfoArray[ParamIndex].pbValue,pParamOption->ParamValueSize)==0)
                        ParameterOK=TRUE;
                }
                else // check value
                {
                    if (pParamOption->ParamValue==pLog->ParametersInfoArray[ParamIndex].Value)
                        ParameterOK=TRUE;
                }

            }

            // update AllParametersOK with new checked param result
            AllParametersOK=AllParametersOK&&ParameterOK;
            // update AtLeastOneParameterOK with new checked param result
            AtLeastOneParameterOK=AtLeastOneParameterOK||ParameterOK;

            // if all param should match stop at least one param is not OK
            if (this->ShouldMatchAllParamValues)
            {
                if (!AllParametersOK)
                {
                    this->pParameters->Unlock();
                    pLogList->Unlock();
                    return FALSE;
                }
            }
            else // stop at least one param is ok
            {
                if (AtLeastOneParameterOK)
                    break;
            }
        }// end of all conditions checking
        this->pParameters->Unlock();


        // if only one value should match
        if (!this->ShouldMatchAllParamValues)
        {
            // if no values match
            if (!AtLeastOneParameterOK)
            {
                pLogList->Unlock();
                return FALSE;
            }
        }
    }

    pLogList->Unlock();

    // item is ok select it
    pListview->SetSelectedState(ItemIndex,TRUE,FALSE);
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: NoItemFoundMessage
// Object: show a Not Found Item Message box
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::NoItemFoundMessage()
{
    MessageBox(this->hWndDialog,_T("No Item Found"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
}

void CSearch::FindFromGUI()
{
    this->FindNextFromGUI(0);
}
void CSearch::FindNextFromGUI(int StartItemIndex)
{
    // updates search fields in case they have changed
    if (!this->UpdateSearchFields())
        return;
    this->FindNext(StartItemIndex);
}
void CSearch::FindPreviousFromGUI(int StartItemIndex)
{
    // updates search fields in case they have changed
    if (!this->UpdateSearchFields())
        return;
    this->FindPrevious(StartItemIndex);
}
void CSearch::SelectAllFromGUI()
{
    // updates search fields in case they have changed
    if (!this->UpdateSearchFields())
        return;

    this->UnselectAll();

    this->SelectAll();
}

//-----------------------------------------------------------------------------
// Name: Find
// Object: find first item matching conditions
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::Find()
{
    this->FindNext(0);
}
//-----------------------------------------------------------------------------
// Name: FindNext
// Object: find next item matching conditions
// Parameters :
//     in  : int StartItemIndex : current selected item, so search begin with the next item
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::FindNext(int StartItemIndex)
{
    // if we are at the end of the list
    if (StartItemIndex>=pListview->GetItemCount())
    {
        // we can't find more item
        this->NoItemFoundMessage();
        return;
    }
    
    // StartItemIndex check
    if (StartItemIndex<0)
        StartItemIndex=0;

    // set single selection style
    LONG_PTR Styles=GetWindowLongPtr(pListview->GetControlHandle(),GWL_STYLE);
    SetWindowLongPtr(pListview->GetControlHandle(),GWL_STYLE,Styles|LVS_SINGLESEL);

    // search for next matching item in listview
    for(int cnt=StartItemIndex;cnt<pListview->GetItemCount();cnt++)
    {
        // stop if item matchs
        if (this->DoesItemMatch(cnt))
        {
            // restore style
            SetWindowLongPtr(pListview->GetControlHandle(),GWL_STYLE,Styles);

            // Set Focus to Main dialog
            SetActiveWindow(this->hWndParent);
            CrossThreadSetFocus(pListview->GetControlHandle());

            // assume item is visible
            pListview->ScrollTo(cnt);

            return;
        }
    }
    // restore style
    SetWindowLongPtr(pListview->GetControlHandle(),GWL_STYLE,Styles);
    // if no item found
    this->NoItemFoundMessage();
}
//-----------------------------------------------------------------------------
// Name: FindPrevious
// Object: find previous item matching conditions
// Parameters :
//     in  : int StartItemIndex : current selected item, so search begin with the previous item
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSearch::FindPrevious(int StartItemIndex)
{
    // if we are at the begin of the list
    if (StartItemIndex<0)
    {
        // we can't find more item
        this->NoItemFoundMessage();
        return;
    }
    
    // StartItemIndex check
    if (StartItemIndex>pListview->GetItemCount()-1)
        StartItemIndex=pListview->GetItemCount()-1;

    // set single selection style
    LONG_PTR Styles=GetWindowLongPtr(pListview->GetControlHandle(),GWL_STYLE);
    SetWindowLongPtr(pListview->GetControlHandle(),GWL_STYLE,Styles|LVS_SINGLESEL);

    // search for previous matching item in listview
    for(int cnt=StartItemIndex;cnt>=0;cnt--)
    {
        // stop if item matchs
        if (this->DoesItemMatch(cnt))
        {
            // restore style
            SetWindowLongPtr(pListview->GetControlHandle(),GWL_STYLE,Styles);
            // Set Focus to Main dialog
            SetActiveWindow(this->hWndParent);
            CrossThreadSetFocus(pListview->GetControlHandle());

            // assume item is visible
            pListview->ScrollTo(cnt);

            return;
        }
    }

    // restore style
    SetWindowLongPtr(pListview->GetControlHandle(),GWL_STYLE,Styles);

    // if no item found
    this->NoItemFoundMessage();
}

void CSearch::UnselectAll()
{
    // set single selection style
    LONG_PTR Styles=GetWindowLongPtr(pListview->GetControlHandle(),GWL_STYLE);
    SetWindowLongPtr(pListview->GetControlHandle(),GWL_STYLE,Styles|LVS_SINGLESEL);
    // select first and then unselect it (only to unselect all item without doing a loop)
    pListview->SetSelectedIndex(0);
    pListview->SetSelectedState(0,FALSE,FALSE);
    // allow multiple selection
    Styles&=~LVS_SINGLESEL;
    SetWindowLongPtr(pListview->GetControlHandle(),GWL_STYLE,Styles);
}

void CSearch::SelectAll()
{
    BOOL bFound=FALSE;
    // search for next matching item in listview
    for(int cnt=0;cnt<pListview->GetItemCount();cnt++)
    {
        // check if item match
        if (this->DoesItemMatch(cnt))
        {
            if(!bFound)
            {
                bFound=TRUE;
                // after the first matching type, disable items selection callback
                // to avoid parsing and a display in the details listview of each 
                // matching item
                pListview->SetSelectionCallbackState(FALSE);
            }
        }
    }

    // enable selection callback again
    pListview->SetSelectionCallbackState(TRUE);

    if (bFound)
    {
        // Set Focus to Main dialog
        SetActiveWindow(this->hWndParent);
        CrossThreadSetFocus(pListview->GetControlHandle());
    }
    else
    {
        // if no item found
        this->NoItemFoundMessage();
    }
}