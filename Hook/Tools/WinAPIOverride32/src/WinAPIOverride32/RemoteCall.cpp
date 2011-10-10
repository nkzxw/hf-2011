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
// Object: manages the Remote Call dialog
//-----------------------------------------------------------------------------

#define CRemoteCall_CALLING_CONVENTION_CDECL_OR_STDCALL _T("cdecl or stdcall (default)")
#define CRemoteCall_CALLING_CONVENTION_FASTCALL _T("fastcall (often used in .NET)")
#define CRemoteCall_CALLING_CONVENTION_THISCALL _T("thiscall")
enum CRemoteCall_CALLING_CONVENTION_INDEX
{
    CRemoteCall_CALLING_CONVENTION_INDEX_CDECL_OR_STDCALL=0,
    CRemoteCall_CALLING_CONVENTION_INDEX_FASTCALL=1,
    CRemoteCall_CALLING_CONVENTION_INDEX_THISCALL=2
};

#include "RemoteCall.h"

extern CLinkListSimple* pApiOverrideList;

CRemoteCall::CRemoteCall(void)
{
    this->pListview=NULL;
}

CRemoteCall::~CRemoteCall(void)
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
DWORD WINAPI CRemoteCall::ModelessDialogThread(PVOID lParam)
{
    CRemoteCall* pRemoteCall=new CRemoteCall();
    pRemoteCall->hInstance=(HINSTANCE)lParam;
    DialogBoxParam (pRemoteCall->hInstance,(LPCTSTR)IDD_DIALOG_REMOTE_CALL,NULL,(DLGPROC)CRemoteCall::WndProc,(LPARAM)pRemoteCall);
    delete pRemoteCall;
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
void CRemoteCall::ShowDialog(HINSTANCE hInstance,HWND hWndDialog)
{
    // show dialog
    // don't use hWndDialog to allow to put winapioverride to an upper Z-order
    UNREFERENCED_PARAMETER(hWndDialog);
    // create thread instead of using CreateDialogParam to don't have to handle keyboard event like TAB
    CloseHandle(CreateThread(NULL,0,CRemoteCall::ModelessDialogThread,hInstance,0,NULL));
}

void CRemoteCall::UpdateHookedProcessList()
{
    // clear current content
    this->pListview->Clear();

    TCHAR** ppsz;
    ppsz=new TCHAR*[2];
    ppsz[0]=new TCHAR[MAX_PATH];
    ppsz[1]=new TCHAR[80];

    CApiOverride* pApiOverride;

    CLinkListItem* pItem;

    // loop through pApiOverrideList
    pApiOverrideList->Lock(TRUE);
    for (pItem=pApiOverrideList->Head;pItem;pItem=pItem->NextItem)
    {
        pApiOverride=(CApiOverride*)pItem->ItemData;
        // get name and pid
        pApiOverride->GetProcessName(ppsz[0],MAX_PATH);
        _stprintf(ppsz[1],_T("0x%.8X"),pApiOverride->GetProcessID());
        this->pListview->AddItemAndSubItems(2,ppsz,FALSE,pItem);
    }
    pApiOverrideList->Unlock();

    delete ppsz[1];
    delete ppsz[0];
    delete[] ppsz;
    // if only one hooked process, select it automatically
    if (pApiOverrideList->GetItemsCount()==1)
        this->pListview->SetSelectedIndex(0);
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CRemoteCall::Init()
{
    this->pListview=new CListview(GetDlgItem(this->hWndDialog,IDC_LIST_REMOTE_CALL_PROCESSES_LIST));
    this->pListview->SetStyle(TRUE,FALSE,FALSE,FALSE);

    this->pListview->AddColumn(_T("Process Name"),450,LVCFMT_LEFT);
    this->pListview->AddColumn(_T("Id"),160,LVCFMT_CENTER);

    // get combo item handle
    this->hWndComboCallingConvention=GetDlgItem(this->hWndDialog,IDC_COMBO_REMOTE_CALL_CALLING_CONVENTION);
    SendMessage(this->hWndComboCallingConvention,CB_RESETCONTENT,0,0);
    SendMessage(this->hWndComboCallingConvention,CB_ADDSTRING,0,(LPARAM)CRemoteCall_CALLING_CONVENTION_CDECL_OR_STDCALL);
    SendMessage(this->hWndComboCallingConvention,CB_ADDSTRING,0,(LPARAM)CRemoteCall_CALLING_CONVENTION_FASTCALL);
    SendMessage(this->hWndComboCallingConvention,CB_ADDSTRING,0,(LPARAM)CRemoteCall_CALLING_CONVENTION_THISCALL);
    // select default calling convention
    SendMessage(this->hWndComboCallingConvention,CB_SETCURSEL,0,0);

    // update hooked process list
    this->UpdateHookedProcessList();
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CRemoteCall::Close()
{
    if (this->pListview)
    {
        delete this->pListview;
        this->pListview=NULL;
    }
    EndDialog(this->hWndDialog,0);
}

//-----------------------------------------------------------------------------
// Name: Call
// Object: Make the remote call
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CRemoteCall::Call()
{
    TCHAR psz[2*MAX_PATH];
    TCHAR pszMsg[MAX_PATH];
    DWORD dwPID;
    CLinkListItem* pItem;
    CApiOverride* pApiOverride=NULL;
    PBYTE ReturnValue=0;
    TCHAR pszDllName[MAX_PATH];
    TCHAR* pszFuncNameAndParam;
    TCHAR* pszFuncName;
    TCHAR* pszParams;
    TCHAR* pc;
    HWND hWndControl;
    DWORD dwFuncNameAndParamSize;
    STRUCT_FUNC_PARAM* pParams=NULL;
    DWORD NbParams=0;
    BOOL bError;
    BOOL bAreSomeParameterPassedAsRef=FALSE;
    DWORD dwTimeout;
    int SelectedIndex;
    REGISTERS Registers={0};
    double FloatingReturn;
    DWORD ThreadId;
    BOOL bProcessNoMoreHooked;
    BOOL ShowRegisters=(IsDlgButtonChecked(this->hWndDialog,IDC_CHECK_REMOTE_CALL_SHOW_OUTPUT_REGISTERS)==BST_CHECKED);

    // find selected process
    SelectedIndex=this->pListview->GetSelectedIndex();
    if (SelectedIndex<0)
    {
        MessageBox(this->hWndDialog,_T("No process selected"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }
    
    this->pListview->GetItemUserData(SelectedIndex,(LPVOID*)&pItem);
    
    bProcessNoMoreHooked=FALSE;
    if (!pApiOverrideList->IsItemStillInList(pItem))
    {
        bProcessNoMoreHooked=TRUE;
    }
    else
    {
        pApiOverride=(CApiOverride*)pItem->ItemData;
        if (IsBadReadPtr(pApiOverride,sizeof(CApiOverride)))
        {
            bProcessNoMoreHooked=TRUE;
        }
    }

    if (bProcessNoMoreHooked)
    {
        this->pListview->GetItemText(SelectedIndex,1,psz,MAX_PATH);
        if (CStringConverter::StringToDWORD(psz,&dwPID))
        {
            // process xxx is no more hooked
            _stprintf(pszMsg,_T("Process 0x") _T("%.8X") _T(" is no more hooked"),dwPID);
        }
        else
            _tcscpy(pszMsg,_T("Process is no more hooked"));

        _tcscat(pszMsg,_T("\r\nHooked process list is going to be updated"));
        MessageBox(this->hWndDialog,pszMsg,_T("Error"),MB_ICONERROR|MB_OK|MB_TOPMOST);

        // update hooked process list
        this->UpdateHookedProcessList();

        return;
    }


    // retrieve dll name
    if (GetDlgItemText(this->hWndDialog,IDC_EDIT_REMOTE_CALL_DLL_NAME,pszDllName,MAX_PATH)<=0)
    {
        MessageBox(this->hWndDialog,_T("No Dll Name specified"),_T("Error"),MB_ICONERROR|MB_OK|MB_TOPMOST);
        return;
    }

    // check if we have to add ".dll" at the end of dll name
    // if not exe_internal
    if (   (_tcsnicmp(pszDllName,EXE_INTERNAL_PREFIX,_tcslen(EXE_INTERNAL_PREFIX))!=0)
        && (_tcsnicmp(pszDllName,EXE_INTERNAL_POINTER_PREFIX,_tcslen(EXE_INTERNAL_POINTER_PREFIX))!=0)
        && (_tcsnicmp(pszDllName,EXE_INTERNAL_RVA_PREFIX,_tcslen(EXE_INTERNAL_RVA_PREFIX))!=0)
        && (_tcsnicmp(pszDllName,EXE_INTERNAL_RVA_POINTER_PREFIX,_tcslen(EXE_INTERNAL_RVA_POINTER_PREFIX))!=0)
       )
    {
        // if no dot found
        if (!_tcsrchr(pszDllName,'.'))
        {
            // add ".dll" extension
            _tcscat(pszDllName,_T(".dll"));
        }
    }

    ////////////////////////////////
    // retrieve func name with params
    // text is like Func(4,0x12,&04000000,&abcd,&Value=0x12,&outvalue,&outbuffer[255])
    ///////////////////////////////

    // retrieve text of edit control
    hWndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_REMOTE_CALL_FUNC_CALL);
    dwFuncNameAndParamSize=(DWORD)SendMessage((HWND) hWndControl,(UINT) WM_GETTEXTLENGTH,0,0)+1;//+1 for \0
    if (dwFuncNameAndParamSize<=1)
    {
        MessageBox(this->hWndDialog,_T("No function nor paramameters specified"),_T("Error"),MB_ICONERROR|MB_OK|MB_TOPMOST);
        return;
    }
    pszFuncNameAndParam=new TCHAR[dwFuncNameAndParamSize];
    SendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,dwFuncNameAndParamSize,(LPARAM)pszFuncNameAndParam);

    // split func name and parameters (search '(' )
    pszParams=_tcschr(pszFuncNameAndParam,'(');


    CParseParametersForRemoteCall* pParseParametersForRemoteCall=new CParseParametersForRemoteCall();
    bError=FALSE;
    if (pszParams)
    {
        *pszParams=0;// to end pszFuncNameAndParam
        // points after (
        pszParams++;

        // find )
        pc=_tcschr(pszParams,')');
        if (pc)
            *pc=0;// to end pszParams
    }
    else
    {
        // allow quick parameter use params splitted by ','; without function name and '(' ')'
        if (   (_tcsnicmp(pszDllName,EXE_INTERNAL_PREFIX,_tcslen(EXE_INTERNAL_PREFIX))!=0)
            && (_tcsnicmp(pszDllName,EXE_INTERNAL_POINTER_PREFIX,_tcslen(EXE_INTERNAL_POINTER_PREFIX))!=0)
            && (_tcsnicmp(pszDllName,EXE_INTERNAL_RVA_PREFIX,_tcslen(EXE_INTERNAL_RVA_PREFIX))!=0)
            && (_tcsnicmp(pszDllName,EXE_INTERNAL_RVA_POINTER_PREFIX,_tcslen(EXE_INTERNAL_RVA_POINTER_PREFIX))!=0)
            && (_tcsnicmp(pszDllName,DLL_INTERNAL_PREFIX,_tcslen(DLL_INTERNAL_PREFIX))!=0)
            && (_tcsnicmp(pszDllName,DLL_INTERNAL_POINTER_PREFIX,_tcslen(DLL_INTERNAL_POINTER_PREFIX))!=0)
            && (_tcsnicmp(pszDllName,DLL_ORDINAL_PREFIX,_tcslen(DLL_ORDINAL_PREFIX))!=0)
            && (_tcsnicmp(pszDllName,DLL_OR_EXE_NET_PREFIX,_tcslen(DLL_OR_EXE_NET_PREFIX))!=0)
            )
        {
            MessageBox(this->hWndDialog,_T("Error no function name or no parameters specified"),_T("Error"),MB_ICONERROR|MB_OK|MB_TOPMOST);
            goto Error;
        }
        // else
        pszParams=pszFuncNameAndParam;
    }

    // parse parameters
    bError=!pParseParametersForRemoteCall->Parse(pszParams,
                                                &pParams,
                                                &NbParams,
                                                &bAreSomeParameterPassedAsRef,
                                                pszMsg,
                                                MAX_PATH);
    if (bError)
    {
        MessageBox(this->hWndDialog,pszMsg,_T("Error"),MB_ICONERROR|MB_OK|MB_TOPMOST);
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

    
    if(!this->GetValue(IDC_EDIT_REMOTE_CALL_TIMEOUT,&dwTimeout))
        goto Error;
    if (dwTimeout==0)
        dwTimeout=INFINITE;
    else
        // convert from seconds to ms
        dwTimeout*=1000;

    if(!this->GetValue(IDC_EDIT_REMOTE_CALL_THREADID,&ThreadId))
        goto Error;

    // if no error

    // get function name
    pszFuncName=CTrimString::TrimString(pszFuncNameAndParam);// pszFuncNameAndParam as been stopped at (

    tagCALLING_CONVENTION CallingConvention;

    // get calling convention
    LRESULT lResult = SendMessage(this->hWndComboCallingConvention,CB_GETCURSEL,0,0);
    switch(lResult)
    {
    case CB_ERR:
    case CRemoteCall_CALLING_CONVENTION_INDEX_CDECL_OR_STDCALL:
    default:
        CallingConvention=CALLING_CONVENTION_STDCALL_OR_CDECL;
        break;
    case CRemoteCall_CALLING_CONVENTION_INDEX_FASTCALL:
        CallingConvention=CALLING_CONVENTION_FASTCALL;
        break;
    case CRemoteCall_CALLING_CONVENTION_INDEX_THISCALL:
        CallingConvention=CALLING_CONVENTION_THISCALL;
        break;
    }


    // make the internal call
    if (pApiOverride->ProcessInternalCall(pszDllName,pszFuncName,NbParams,pParams,&Registers,&ReturnValue,&FloatingReturn,dwTimeout,ThreadId,CallingConvention))
    {
        // if no param as ref
        if ((!bAreSomeParameterPassedAsRef)&&(!ShowRegisters))
        {
            // only message box with returned value
            _stprintf(pszMsg,_T("Function successfully called\r\nReturned Value: 0x%p"),ReturnValue);
            MessageBox(this->hWndDialog,pszMsg,_T("Information"),MB_ICONINFORMATION|MB_OK|MB_TOPMOST);
        }
        else
        {
            // Small interface presenting parameters
            CRemoteCallResult* pResult=new CRemoteCallResult(pszDllName,pszFuncName,NbParams,pParams,&Registers,ReturnValue,ShowRegisters,FloatingReturn);
            pResult->Show(this->hInstance,this->hWndDialog);
            delete pResult;
        }
    }

Error:
    // free memory of pointed parameters
    if (pParseParametersForRemoteCall)// delete pParseParametersForRemoteCall only after having call ProcessInternalCall
        delete pParseParametersForRemoteCall;
    if (pParams)
        delete pParams;
   delete[] pszFuncNameAndParam;
}

//-----------------------------------------------------------------------------
// Name: GetValue
// Object: retrieve DWORD value of specified control
// Parameters :
//     in  : int ControlID : id of control we want to retrieve value
//     out : DWORD* pValue : pointer to Value of control
//     return : TRUE if there's data inside control, false else
//-----------------------------------------------------------------------------
BOOL CRemoteCall::GetValue(int ControlID,DWORD* pValue)
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
        MessageBox(this->hWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog callback of the dump dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CRemoteCall::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // init dialog
            CRemoteCall* pRemoteCall;
            pRemoteCall=(CRemoteCall*)lParam;
            pRemoteCall->hWndDialog=hWnd;
            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pRemoteCall);

            pRemoteCall->Init();
            // load dlg icons
            CDialogHelper::SetIcon(hWnd,IDI_ICON_REMOTE_CALL);
        }
        break;
    case WM_CLOSE:
        // close dialog
        ((CRemoteCall*)GetWindowLongPtr(hWnd,GWLP_USERDATA))->Close();
        break;
    case WM_COMMAND:
        {
            CRemoteCall* pRemoteCall=((CRemoteCall*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pRemoteCall)
                break;

            switch (LOWORD(wParam))
            {
                case IDOK:
                    pRemoteCall->Call();
                    break;
                case IDCANCEL:
                    pRemoteCall->Close();
                    break;
            }
            if ((HWND)lParam==pRemoteCall->hWndComboCallingConvention)
            {
                switch(HIWORD(wParam))
                {
                case CBN_SELCHANGE:
                    pRemoteCall->OnCallingConventionChange();
                    break;
                }
            }
        }
        break;
    case WM_NOTIFY:
        {
            CRemoteCall* pRemoteCall=((CRemoteCall*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pRemoteCall)
                break;

            //we are only interested in NM_CUSTOMDRAW notifications for ListView
            if (pRemoteCall->pListview)
            {
                LPNMLISTVIEW pnm;
                pnm = (LPNMLISTVIEW)lParam;

                if (pnm->hdr.hwndFrom==ListView_GetHeader(pRemoteCall->pListview->GetControlHandle()))
                {
                    if (pnm->hdr.code==HDN_ITEMCLICK) 
                    {
                        // on header click
                        if (pnm->iItem==1)
                            pRemoteCall->pListview->SetSortingType(CListview::SortingTypeNumber);
                        else
                            pRemoteCall->pListview->SetSortingType(CListview::SortingTypeString);
                    }
                }

                if (pRemoteCall->pListview->OnNotify(wParam,lParam))
                    // if message has been proceed by pListview->OnNotify
                    break;
            }
        }
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

void CRemoteCall::OnCallingConventionChange()
{
    // according to calling convention enable/disable ECX and/or EDX
    LRESULT lResult = SendMessage(this->hWndComboCallingConvention,CB_GETCURSEL,0,0);

    switch(lResult)
    {
    case CB_ERR:
    case CRemoteCall_CALLING_CONVENTION_INDEX_CDECL_OR_STDCALL:
    default:
        // enable ecx/edx
        EnableWindow(GetDlgItem(this->hWndDialog,IDC_EDIT_REMOTE_CALL_ECX),TRUE);
        EnableWindow(GetDlgItem(this->hWndDialog,IDC_EDIT_REMOTE_CALL_EDX),TRUE);
        break;
    case CRemoteCall_CALLING_CONVENTION_INDEX_FASTCALL:
        // disable ecx/edx
        EnableWindow(GetDlgItem(this->hWndDialog,IDC_EDIT_REMOTE_CALL_ECX),FALSE);
        EnableWindow(GetDlgItem(this->hWndDialog,IDC_EDIT_REMOTE_CALL_EDX),FALSE);
        break;
    case CRemoteCall_CALLING_CONVENTION_INDEX_THISCALL:
        //disable ecx, enable edx
        EnableWindow(GetDlgItem(this->hWndDialog,IDC_EDIT_REMOTE_CALL_ECX),FALSE);
        EnableWindow(GetDlgItem(this->hWndDialog,IDC_EDIT_REMOTE_CALL_EDX),TRUE);
        break;
    }
}