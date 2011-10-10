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
// Object: manages the registers dialog
//-----------------------------------------------------------------------------

#include "registersuserinterface.h"

extern HINSTANCE DllhInstance;

CRegistersUserInterface::CRegistersUserInterface(REGISTERS* pRegisters,double* pDoubleResult,BOOL BeforeCall)
{
    this->pRegisters=pRegisters;
    this->pDoubleResult=pDoubleResult;
    this->BeforeCall=BeforeCall;
}

CRegistersUserInterface::~CRegistersUserInterface(void)
{
}

//-----------------------------------------------------------------------------
// Name: GetAssociatedObject
// Object: Get object associated to window handle
// Parameters :
//     in  : HWND hWndDialog : handle of the window
//     out :
//     return : associated object if found, NULL if not found
//-----------------------------------------------------------------------------
CRegistersUserInterface* CRegistersUserInterface::GetAssociatedObject(HWND hWndDialog)
{
    return (CRegistersUserInterface*)DynamicGetWindowLong(hWndDialog,GWLP_USERDATA);
}


//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CRegistersUserInterface::Init(HWND hWndDialog)
{
    this->hWndDialog=hWndDialog;

    // associate object to window handle
    DynamicSetWindowLongPtr(hWndDialog,GWLP_USERDATA,(LONG)this);

    this->SetUIRegisterValue(IDC_EDIT_REGISTERS_EAX,this->pRegisters->eax);
    this->SetUIRegisterValue(IDC_EDIT_REGISTERS_EBX,this->pRegisters->ebx);
    this->SetUIRegisterValue(IDC_EDIT_REGISTERS_ECX,this->pRegisters->ecx);
    this->SetUIRegisterValue(IDC_EDIT_REGISTERS_EDX,this->pRegisters->edx);
    this->SetUIRegisterValue(IDC_EDIT_REGISTERS_ESI,this->pRegisters->esi);
    this->SetUIRegisterValue(IDC_EDIT_REGISTERS_EDI,this->pRegisters->edi);
    this->SetUIRegisterValue(IDC_EDIT_REGISTERS_EFL,this->pRegisters->efl);
    this->SetUIRegisterValue(IDC_EDIT_REGISTERS_ES,this->pRegisters->es);
    this->SetUIRegisterValue(IDC_EDIT_REGISTERS_FS,this->pRegisters->fs);
    this->SetUIRegisterValue(IDC_EDIT_REGISTERS_GS,this->pRegisters->gs);

    if (this->BeforeCall)
    {
        DynamicEnableWindow(DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_REGISTERS_ST0),FALSE);
        DynamicEnableWindow(DynamicGetDlgItem(this->hWndDialog,IDC_STATIC_FLOAT_REGISTERS),FALSE);
        DynamicEnableWindow(DynamicGetDlgItem(this->hWndDialog,IDC_STATIC_REGISTER_ST_0),FALSE);
    }
    else
    {
        TCHAR psz[50];
        // load registers values
        *psz=0;
        _stprintf(psz,_T("%.19g"), *this->pDoubleResult);
        DynamicSendMessage(DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_REGISTERS_ST0),WM_SETTEXT,0,(LPARAM)psz);
    }
}

//-----------------------------------------------------------------------------
// Name: SetUIRegisterValue
// Object: display dword value into specified dialog item
// Parameters :
//     in  : int DlgItemId : dialog item
//           DWORD dwRegisterValue : value
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CRegistersUserInterface::SetUIRegisterValue(int DlgItemId,DWORD dwRegisterValue)
{
    TCHAR psz[MAX_PATH];
    *psz=0;
    _sntprintf(psz,MAX_PATH,_T("0x%.8X") ,dwRegisterValue);

    DynamicSendMessage(DynamicGetDlgItem(this->hWndDialog,DlgItemId),WM_SETTEXT,0,(LPARAM)psz);
}

//-----------------------------------------------------------------------------
// Name: ShowBadRegisterValueMessage
// Object: show error messagebox with register name
// Parameters :
//     in  : TCHAR* pszRegisterName : use on error to display a friendly user message error
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CRegistersUserInterface::ShowBadRegisterValueMessage(TCHAR* pszRegisterName)
{
    if (!pszRegisterName)
        return;
    TCHAR psz[MAX_PATH];
    _tcscpy(psz,_T("Bad "));
    _tcscat(psz,pszRegisterName);
    _tcscat(psz,_T(" Register Value"));
    DynamicMessageBoxInDefaultStation(this->hWndDialog,psz,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
}

//-----------------------------------------------------------------------------
// Name: GetUIRegisterValue
// Object: get dword value from specified dialog item
// Parameters :
//     in  : int DlgItemId : dialog item
//           TCHAR* pszRegisterName : use on error to display a friendly user message error
//     out : DWORD* pdwRegisterValue : value
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CRegistersUserInterface::GetUIRegisterValue(int DlgItemId,DWORD* pdwRegisterValue,TCHAR* pszRegisterName)
{
    TCHAR psz[MAX_PATH];

    if (IsBadWritePtr(pdwRegisterValue,sizeof(DWORD)))
        return FALSE;

    // retrieve item content
    DynamicSendMessage(DynamicGetDlgItem(this->hWndDialog,DlgItemId),(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);

    if (!CStringConverter::StringToDWORD(psz,pdwRegisterValue))
    {
        this->ShowBadRegisterValueMessage(pszRegisterName);
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ModifyRegisters
// Object: Apply user interface value to registers
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CRegistersUserInterface::ModifyRegisters()
{
    if (!this->GetUIRegisterValue(IDC_EDIT_REGISTERS_EAX,&this->pRegisters->eax,_T("EAX")))
        return;
    if (!this->GetUIRegisterValue(IDC_EDIT_REGISTERS_EBX,&this->pRegisters->ebx,_T("EBX")))
        return;
    if (!this->GetUIRegisterValue(IDC_EDIT_REGISTERS_ECX,&this->pRegisters->ecx,_T("ECX")))
        return;
    if (!this->GetUIRegisterValue(IDC_EDIT_REGISTERS_EDX,&this->pRegisters->edx,_T("EDX")))
        return;
    if (!this->GetUIRegisterValue(IDC_EDIT_REGISTERS_ESI,&this->pRegisters->esi,_T("ESI")))
        return;
    if (!this->GetUIRegisterValue(IDC_EDIT_REGISTERS_EDI,&this->pRegisters->edi,_T("EDI")))
        return;
    if (!this->GetUIRegisterValue(IDC_EDIT_REGISTERS_EFL,&this->pRegisters->efl,_T("EFL")))
        return;
    if (!this->GetUIRegisterValue(IDC_EDIT_REGISTERS_ES,&this->pRegisters->es,_T("ES")))
        return;
    if (!this->GetUIRegisterValue(IDC_EDIT_REGISTERS_FS,&this->pRegisters->fs,_T("FS")))
        return;
    if (!this->GetUIRegisterValue(IDC_EDIT_REGISTERS_GS,&this->pRegisters->gs,_T("GS")))
        return;


    // retrieve item content
    if (!this->BeforeCall)
    {    
        TCHAR psz[MAX_PATH];
        *psz=0;
        DynamicSendMessage(DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_REGISTERS_ST0),(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);
        if (_tcsicmp(psz,_T("-1.#IND"))!=0)// check if floating result is defined
        {
            if(_stscanf(psz,_T("%le"),this->pDoubleResult)!=1)
            {
                this->ShowBadRegisterValueMessage(_T("ST_0"));
                return;
            }
        }
    }

    this->Close();
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: Close like. EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CRegistersUserInterface::Close()
{
    DynamicEndDialog(this->hWndDialog,0);
}

//-----------------------------------------------------------------------------
// Name: ShowDialog
// Object: show the dialog box
// Parameters :
//     in  : HWND ParentHandle : parent window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
INT_PTR CRegistersUserInterface::ShowDialog(HWND ParentHandle)
{
    // show dialog
    return DynamicDialogBoxParam(DllhInstance,(LPTSTR)IDD_DIALOG_REGISTERS,ParentHandle,(DLGPROC)CRegistersUserInterface::WndProc,(LPARAM)this);
}

//-----------------------------------------------------------------------------
// Name: RegistersWndProc
// Object: dialog callback of the dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CRegistersUserInterface::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CRegistersUserInterface* pDialogObject; 
    switch (uMsg)
    {
    case WM_INITDIALOG:
        pDialogObject=(CRegistersUserInterface*)lParam;
        pDialogObject->Init(hWnd);
        break;
    case WM_CLOSE:
        pDialogObject=CRegistersUserInterface::GetAssociatedObject(hWnd);
        if (pDialogObject)
            pDialogObject->Close();
        break;
    case WM_COMMAND:

        pDialogObject=CRegistersUserInterface::GetAssociatedObject(hWnd);
        if (!pDialogObject)
            break;
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            pDialogObject->Close();
            break;
        case IDOK:
            pDialogObject->ModifyRegisters();
            break;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}