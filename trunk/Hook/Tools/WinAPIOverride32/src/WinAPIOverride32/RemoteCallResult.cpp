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
// Object: manages the Remote Call result dialog
//-----------------------------------------------------------------------------

#include "remotecallresult.h"

#ifdef HOOKCOM_EXPORTS // if inside HookCom dll
extern CLinkListSimple* pLinkListOpenDialogs;
extern HANDLE hSemaphoreOpenDialogs;
#endif

//-----------------------------------------------------------------------------
// Name: CRemoteCallResult
// Object: constructor
// Parameters :
//     in  : TCHAR* pszDllName : name of called dll (or EXE_INTERNAL@ or DLL_INTERNAL@)
//           TCHAR* pszFuncName : name of called function
//           DWORD NbParams : number of parameters
//           STRUCT_FUNC_PARAM* pParam : pointer to an array of STRUCT_FUNC_PARAM containing NbParams items
//           REGISTERS* pRegisters : pointer to output registers
//           PBYTE RetValue : returned value
//           BOOL ShowRegisters : TRUE if registers or floating result have to be shown
//           double FloatingResult : floating result value
//     out :
//     return : 
//-----------------------------------------------------------------------------
CRemoteCallResult::CRemoteCallResult(TCHAR* pszDllName,TCHAR* pszFuncName,DWORD NbParams,STRUCT_FUNC_PARAM* pParam,REGISTERS* pRegisters,PBYTE RetValue,BOOL ShowRegisters,double FloatingResult)
{
    this->pParam=pParam;
    this->NbParams=NbParams;
    this->pszDllName=pszDllName;
    this->pszFuncName=pszFuncName;
    this->RetValue=RetValue;
    memcpy(&this->Registers,pRegisters,sizeof(REGISTERS));
    this->ShowRegisters=ShowRegisters;
    this->FloatingResult=FloatingResult;
#ifdef HOOKCOM_EXPORTS // if inside HookCom dll
    this->pItemDialog=NULL;
#endif
}
CRemoteCallResult::~CRemoteCallResult()
{
}

//-----------------------------------------------------------------------------
// Name: Show
// Object: display Remote Call result dialog
// Parameters :
//     in  : HINSTANCE hInstance : instance containing dialog resource
//           HWND hWndDialog : parent window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CRemoteCallResult::Show(HINSTANCE hInstance,HWND hWndParentDialog)
{
    DialogBoxParam(hInstance, (LPCTSTR)IDD_DIALOG_REMOTE_CALL_RESULT, hWndParentDialog, (DLGPROC)WndProc,(LPARAM)this);
#ifdef HOOKCOM_EXPORTS // if inside HookCom dll
    if (hSemaphoreOpenDialogs)
        ReleaseSemaphore(hSemaphoreOpenDialogs,1,NULL); // must be last instruction
#endif
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: init Remote Call result dialog
// Parameters :
//     in  : HWND hWnd : dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CRemoteCallResult::Init(HWND hwnd)
{
    this->hWndDialog=hwnd;

#ifdef HOOKCOM_EXPORTS // if inside HookCom dll
    this->pItemDialog=pLinkListOpenDialogs->AddItem(hwnd);
#endif

    this->OnSize();

    HWND hWndControl;
    // create fixed with font
    this->hFont=CreateFont(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, NULL);

    // set hexa textboxes fonts (fixed width font)
    if (this->hFont)
    {
        hWndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_REMOTE_CALL_RESULT);
        SendMessage((HWND) hWndControl,(UINT) WM_SETFONT,(WPARAM)this->hFont,FALSE);
    }

    // display parameters values
    DWORD cnt;
    DWORD cnt2;
    TCHAR* pszFullData;
    TCHAR psz[200];
    TCHAR* Buffer;

    size_t StrBufferSize=1024;

    pszFullData=new TCHAR[StrBufferSize];

    *pszFullData=NULL;
    if (this->pszDllName&&this->pszFuncName)
    {
        _stprintf(pszFullData,
            _T("Module Name: %s\r\n")
            _T("Function Name: %s\r\n"),
            this->pszDllName,this->pszFuncName);
    }

    for (cnt=0;cnt<this->NbParams;cnt++)
    {
        // add paramX: 
        _stprintf(psz,_T("Param%u: "),cnt+1);
        this->_tcsIncreaseIfNecessaryAndCat(&pszFullData,psz,&StrBufferSize);

        if ((this->pParam[cnt].bPassAsRef && this->pParam[cnt].dwDataSize!=sizeof(PBYTE)) // <-- this quite sucks for BYTE array of 4 bytes
                                                                              // but if we don't do it any &outvalue is displayed like a byte buffer
                                                                              // remove the && this->pParam[cnt].dwDataSize!=4 if you dislike it
            ||this->pParam[cnt].dwDataSize>sizeof(PBYTE))
        {
            // display like a buffer
            Buffer=new TCHAR[this->pParam[cnt].dwDataSize*2+1];
            for (cnt2=0;cnt2<this->pParam[cnt].dwDataSize;cnt2++)
            {
                _stprintf(&Buffer[cnt2*2],_T("%.2X"),((BYTE*)this->pParam[cnt].pData)[cnt2]);
            }
            this->_tcsIncreaseIfNecessaryAndCat(&pszFullData,Buffer,&StrBufferSize);
            delete[] Buffer;
        }
        else
        {
            // add like a value
            _stprintf(psz,_T("0x%p"),*((PBYTE*)this->pParam[cnt].pData));
            this->_tcsIncreaseIfNecessaryAndCat(&pszFullData,psz,&StrBufferSize);
        }
        // add line break
        this->_tcsIncreaseIfNecessaryAndCat(&pszFullData,_T("\r\n"),&StrBufferSize);
    }

    _stprintf(psz,_T("\r\nResult: 0x%p"),this->RetValue);
    this->_tcsIncreaseIfNecessaryAndCat(&pszFullData,psz,&StrBufferSize);

    if (this->ShowRegisters)
    {
        _stprintf(psz,_T("\r\nFloating result: %.19g"), this->FloatingResult);
        this->_tcsIncreaseIfNecessaryAndCat(&pszFullData,psz,&StrBufferSize);

        _tcscat(pszFullData,_T("\r\n\r\nAfter Call Registers\r\n"));

        _stprintf(psz,_T("Eax: 0x%.8X\r\n"),this->Registers.eax);
        this->_tcsIncreaseIfNecessaryAndCat(&pszFullData,psz,&StrBufferSize);

        _stprintf(psz,_T("Ebx: 0x%.8X\r\n"),this->Registers.ebx);
        this->_tcsIncreaseIfNecessaryAndCat(&pszFullData,psz,&StrBufferSize);

        _stprintf(psz,_T("Ecx: 0x%.8X\r\n"),this->Registers.ecx);
        this->_tcsIncreaseIfNecessaryAndCat(&pszFullData,psz,&StrBufferSize);

        _stprintf(psz,_T("Edx: 0x%.8X\r\n"),this->Registers.edx);
        this->_tcsIncreaseIfNecessaryAndCat(&pszFullData,psz,&StrBufferSize);

        _stprintf(psz,_T("Esi: 0x%.8X\r\n"),this->Registers.esi);
        this->_tcsIncreaseIfNecessaryAndCat(&pszFullData,psz,&StrBufferSize);

        _stprintf(psz,_T("Edi: 0x%.8X\r\n"),this->Registers.edi);
        this->_tcsIncreaseIfNecessaryAndCat(&pszFullData,psz,&StrBufferSize);

        _stprintf(psz,_T("Efl: 0x%.8X"),this->Registers.efl);
        this->_tcsIncreaseIfNecessaryAndCat(&pszFullData,psz,&StrBufferSize);

    }

    SetDlgItemText(this->hWndDialog,IDC_EDIT_REMOTE_CALL_RESULT,pszFullData);

    delete[] pszFullData;
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: Close Remote Call result dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CRemoteCallResult::Close()
{
#ifdef HOOKCOM_EXPORTS // if inside HookCom dll
    // if pLinkListOpenDialogs is not locked by dll unload
    if (!hSemaphoreOpenDialogs)
        pLinkListOpenDialogs->RemoveItem(this->pItemDialog);
#endif

    DeleteObject(this->hFont);
    EndDialog(this->hWndDialog,0);
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: Remote Call result dialog window proc
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CRemoteCallResult::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG)lParam);
        ((CRemoteCallResult*)lParam)->Init(hWnd);
        // load dlg icons
        CDialogHelper::SetIcon(hWnd,IDI_ICON_REMOTE_CALL);
        break;
    case WM_CLOSE:
        {
            CRemoteCallResult* pRemoteCallResult=(CRemoteCallResult*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (pRemoteCallResult)
                pRemoteCallResult->Close();
            break;
        }
        break;
    case WM_SIZE:
        {
            CRemoteCallResult* pRemoteCallResult=(CRemoteCallResult*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (!pRemoteCallResult)
                return FALSE;
            pRemoteCallResult->OnSize();
        }
        break;
    case WM_SIZING:
        {
            CRemoteCallResult* pRemoteCallResult=(CRemoteCallResult*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (!pRemoteCallResult)
                return FALSE;
            pRemoteCallResult->OnSizing((RECT*)lParam);
        }
        break;
    case WM_COMMAND:
        {
            CRemoteCallResult* pRemoteCallResult=(CRemoteCallResult*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (!pRemoteCallResult)
                break;

            switch (LOWORD(wParam))
            {
                case IDOK:
                    pRemoteCallResult->Close();
                    break;
            }
        }
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: _tcsIncreaseIfNecessaryAndCat
// Object: _tcscat like with buffer reallocation if necessary
// Parameters :
//     in : TCHAR** ppsz1 : pointer to Pointer to a null-terminated string to which the function appends psz2.
//                          MUST HAVE BEEN dynamically allocated. Can be reallocated
//          TCHAR* psz2 : Pointer to a null-terminated string to be appended to psz1
//     inout : size_t* p_psz1MaxLength : pointer to *ppsz1 max length
//     out :
//     return : Returns a pointer to *ppsz1, which holds the combined strings
//-----------------------------------------------------------------------------
TCHAR* CRemoteCallResult::_tcsIncreaseIfNecessaryAndCat(TCHAR** ppsz1,TCHAR* psz2,size_t* p_psz1MaxLength)
{
    size_t NewSize;
    NewSize=_tcslen(*ppsz1)+_tcslen(psz2)+1;
    // if not enough memory
    if (NewSize>*p_psz1MaxLength)
    {
        // double size at least
        if (NewSize<2*(*p_psz1MaxLength))
            NewSize=2*(*p_psz1MaxLength);

        // allocate a new buffer
        TCHAR* psz;
        psz=new TCHAR[NewSize];
        // on allocation error
        if (!psz)
            // do nothing
            return *ppsz1;
        // else
        // update length
        *p_psz1MaxLength=NewSize;
        // copy content of *ppsz1
        _tcscpy(psz,*ppsz1);
        // delete previously allocated buffer
        delete *ppsz1;

        // make *ppsz1 point to new buffer
        *ppsz1=psz;
    }
    // do the _tcscat
    _tcscat(*ppsz1,psz2);

    // return pointer to *ppsz1
    return *ppsz1;
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
void CRemoteCallResult::OnSizing(RECT* pWinRect)
{
    // check min width and min height
    if ((pWinRect->right-pWinRect->left)<CRemoteCallResult_DIALOG_MIN_WIDTH)
        pWinRect->right=pWinRect->left+CRemoteCallResult_DIALOG_MIN_WIDTH;
    if ((pWinRect->bottom-pWinRect->top)<CRemoteCallResult_DIALOG_MIN_HEIGHT)
        pWinRect->bottom=pWinRect->top+CRemoteCallResult_DIALOG_MIN_HEIGHT;
}
//-----------------------------------------------------------------------------
// Name: OnSize
// Object: called on WM_SIZE. Resize some controls
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CRemoteCallResult::OnSize()
{
    RECT RectWindow;
    RECT RectOk;
    RECT RectText;
    HWND hItemText;
    HWND hItemOk;
    DWORD Spacer;
    hItemText=GetDlgItem(this->hWndDialog,IDC_EDIT_REMOTE_CALL_RESULT);
    hItemOk=GetDlgItem(this->hWndDialog,IDOK);

    CDialogHelper::GetClientWindowRect(this->hWndDialog,this->hWndDialog,&RectWindow);

    // Spacer
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItemText,&RectText);
    Spacer=RectText.left;

    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItemOk,&RectOk);
    SetWindowPos(hItemOk,HWND_NOTOPMOST,
                RectWindow.left+((RectWindow.right-RectWindow.left)-(RectOk.right-RectOk.left))/2,
                RectWindow.bottom-Spacer-(RectOk.bottom-RectOk.top),
                0,
                0,
                SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItemOk,&RectOk);
    CDialogHelper::Redraw(hItemOk);

    SetWindowPos(hItemText,HWND_NOTOPMOST,
                0,
                0,
                RectWindow.right+RectWindow.left-2*Spacer,
                RectOk.top-RectText.top-CRemoteCallResult_SPACE_BETWEEN_CONTROLS,
                SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(hItemText);
}