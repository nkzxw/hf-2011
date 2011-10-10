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
// Object: display .Net jitted function and allow to show generated asm opcodes
//-----------------------------------------------------------------------------

#include "NetInteraction.h"
#pragma comment (lib,"comctl32.lib")

extern HINSTANCE DllhInstance;
extern CLinkListSimple* pCompiledFunctionList;
extern HOOK_NET_INIT HookNetInfos;
extern CLinkListSimple* pLinkListOpenDialogs;
extern HANDLE hSemaphoreOpenDialogs;

extern "C" __declspec(dllexport) 
BOOL __stdcall ShowNetInteraction()
{
    CNetInteraction::Show();
    return TRUE;
}

CNetInteraction::CNetInteraction(void)
{
    this->pItemDialog=NULL;
    this->hInstance=DllhInstance;
    this->DialogResult=0;
    this->hWndDialog=0;
    this->pListview=NULL;
    this->pToolbar=NULL;
    this->MenuShowGeneratedAsmId=0;
    this->MenuHookSelectedId=0;
    this->MenuUnhookSelectedId=0;
}
CNetInteraction::~CNetInteraction(void)
{

}

void CNetInteraction::RefreshNetHookList()
{
    int ItemIndex;
    TCHAR sz[64];
    TCHAR* psz;

    // clear list view
    this->pListview->Clear();
    
    // for each compiled function
    CLinkListItem* pItem;
    CFunctionInfoEx* pParsedFunctionInfo;
    pCompiledFunctionList->Lock(TRUE);
    for (pItem=pCompiledFunctionList->Head;pItem;pItem=pItem->NextItem)
    {
        pParsedFunctionInfo=(CFunctionInfoEx*)pItem->ItemData;
        // add to list

        ItemIndex=this->pListview->AddItem(pParsedFunctionInfo->szName,pItem);
        
        _stprintf(sz,_T("0x%.8X"),pParsedFunctionInfo->FunctionToken);
        this->pListview->SetItemText(ItemIndex,ListViewColumnIndex_TOKEN,sz);

        this->pListview->SetItemText(ItemIndex,ListViewColumnIndex_MODULE_NAME,pParsedFunctionInfo->szModule);

        _stprintf(sz,_T("0x%p"),pParsedFunctionInfo->AsmCodeStart);
        this->pListview->SetItemText(ItemIndex,ListViewColumnIndex_ASM_CODE_START,sz);

        _stprintf(sz,_T("0x%.8X"),pParsedFunctionInfo->AsmCodeSize);
        this->pListview->SetItemText(ItemIndex,ListViewColumnIndex_ASM_CODE_LENGTH,sz);
            
        if (pParsedFunctionInfo->pItemApiInfo)
            psz=_T("yes");
        else
            psz=_T("no");
        this->pListview->SetItemText(ItemIndex,ListViewColumnIndex_HOOKED,psz);
    }
    pCompiledFunctionList->Unlock();
    this->pListview->ReSort();
}

void CNetInteraction::ShowGeneratedAssembly()
{
    CLinkListItem* pItem;
    int ItemIndex=this->pListview->GetSelectedIndex();
    if (ItemIndex<0)
    {
        HookNetInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("No Item Selected"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    this->pListview->GetItemUserData(ItemIndex,(LPVOID*)&pItem);

    if (!pCompiledFunctionList->IsItemStillInList(pItem))
    {
        HookNetInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Item Has Changed"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        this->RefreshNetHookList();
        return;
    }

    CloseHandle(CreateThread(0,0,CNetInteraction::ShowGeneratedAsmThread,this,0,0));
}

DWORD WINAPI CNetInteraction::ShowGeneratedAsmThread(LPVOID UserParam)
{
    int ItemIndex;
    CLinkListItem* pItem;
    CFunctionInfoEx* pParsedFunctionInfo;
    CNetInteraction* pNetInteraction=(CNetInteraction*)UserParam;

    // item checking just have been made --> don't do it again
    ItemIndex=pNetInteraction->pListview->GetSelectedIndex();
    pNetInteraction->pListview->GetItemUserData(ItemIndex,(LPVOID*)&pItem);

    pParsedFunctionInfo=(CFunctionInfoEx*)pItem->ItemData;

    if (IsBadReadPtr(pParsedFunctionInfo->AsmCodeStart,pParsedFunctionInfo->AsmCodeSize))
    {
        HookNetInfos.DynamicMessageBoxInDefaultStation(pNetInteraction->hWndDialog,_T("Invalid Address"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        // refresh list
        pNetInteraction->RefreshNetHookList();
        return 0;
    }

    PBYTE LocalBuffer=new BYTE[pParsedFunctionInfo->AsmCodeSize];
    memcpy(LocalBuffer,pParsedFunctionInfo->AsmCodeStart,pParsedFunctionInfo->AsmCodeSize);

    // if function is hooked, first bytes may are changed
    if (pParsedFunctionInfo->pItemApiInfo)
    {
        API_INFO* pApiInfo=NULL;
        pApiInfo=(API_INFO*)pParsedFunctionInfo->pItemApiInfo->ItemData;
        memcpy(LocalBuffer,pApiInfo->Opcodes,pApiInfo->OpcodeReplacementSize);
    }

    // get HtmlViewer Dll Path (the same path has the current module)
    TCHAR szHtmlViewerDllPath[MAX_PATH];
    CStdFileOperations::GetModulePath(DllhInstance,szHtmlViewerDllPath,MAX_PATH);


    DISASM_INSTRUCTION_CALLBACK_PARAM DisasmInstructionCallBackParam;
    DisasmInstructionCallBackParam.BufferSize=1024;
    DisasmInstructionCallBackParam.Buffer=new TCHAR[DisasmInstructionCallBackParam.BufferSize];
    *DisasmInstructionCallBackParam.Buffer=0;

    // disasm function
    if (!CDisasm::Disasm((ULONGLONG)pParsedFunctionInfo->AsmCodeStart,
                        LocalBuffer,
                        pParsedFunctionInfo->AsmCodeSize,
#ifndef _WIN64
                        CDisasm::Decode32Bits,
#else
                        CDisasm::Decode64Bits,
#endif
                        CNetInteraction::DisasmInstructionCallBack,
                        &DisasmInstructionCallBackParam)
        )
    {
        HookNetInfos.DynamicMessageBoxInDefaultStation(pNetInteraction->hWndDialog,_T("Error disassembling function"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
    }
    delete[] LocalBuffer;

    CDisplayAsm::Show(pParsedFunctionInfo->szName,DisasmInstructionCallBackParam.Buffer);
    delete DisasmInstructionCallBackParam.Buffer;
    return 0;
}
//-----------------------------------------------------------------------------
// Name: DisasmInstructionCallBack
// Object: called on each asm decoded instruction
// Parameters : CDisasm callback parameters
//     in  : 
//     out :
//     return : TRUE to continue asm instruction parsing
//-----------------------------------------------------------------------------
BOOL CNetInteraction::DisasmInstructionCallBack(ULONGLONG InstructionAddress,DWORD InstructionsSize,TCHAR* InstructionHex,TCHAR* Mnemonic,TCHAR* Operands,PVOID UserParam)
{
    UNREFERENCED_PARAMETER(InstructionsSize);
    _tcsupr(InstructionHex);
    PBYTE TmpInstructionAddress=(PBYTE)InstructionAddress;

    DISASM_INSTRUCTION_CALLBACK_PARAM* pDisasmInstructionCallBackParam;
    pDisasmInstructionCallBackParam=(DISASM_INSTRUCTION_CALLBACK_PARAM*)UserParam;
    TCHAR CurrentInstructionContent[MAX_PATH];
    _sntprintf(CurrentInstructionContent,
                MAX_PATH,
                _T("0x%p   %-24s %s%s%s\r\n"),
                TmpInstructionAddress,
                InstructionHex,
                Mnemonic,
                *Operands != 0 ? _T(" ") : _T(""),
                Operands);

    if (_tcslen(CurrentInstructionContent)+_tcslen(pDisasmInstructionCallBackParam->Buffer)>=
        pDisasmInstructionCallBackParam->BufferSize)
    {
        TCHAR* pszTmp=pDisasmInstructionCallBackParam->Buffer;
        pDisasmInstructionCallBackParam->BufferSize=(pDisasmInstructionCallBackParam->BufferSize+_tcslen(CurrentInstructionContent))*2;
        pDisasmInstructionCallBackParam->Buffer=new TCHAR[pDisasmInstructionCallBackParam->BufferSize];
        _tcscpy(pDisasmInstructionCallBackParam->Buffer,pszTmp);
        delete pszTmp;
    }
    _tcscat(pDisasmInstructionCallBackParam->Buffer,CurrentInstructionContent);

    // continue parsing
    return TRUE;
}
void CNetInteraction::CallbackListViewMenuItemClickStatic(UINT MenuID,LPVOID UserParam)
{
    ((CNetInteraction*)UserParam)->CallbackListViewMenuItemClick(MenuID);
}
void CNetInteraction::CallbackListViewMenuItemClick(UINT MenuID)
{
    if (MenuID==this->MenuShowGeneratedAsmId)
    {
        this->ShowGeneratedAssembly();
    }
    else if (MenuID==this->MenuHookSelectedId)
    {
        this->HookSelected();
    }
    else if (MenuID==this->MenuUnhookSelectedId)
    {
        this->UnhookSelected();
    }
}

BOOL CNetInteraction::HookSelected()
{
    BOOL bRet=TRUE;
    BOOL bFunctionRet;
    int ItemIndex=this->pListview->GetSelectedIndex();
    if (ItemIndex<0)
    {
        HookNetInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("No Item Selected"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }
    CLinkListItem* pItem;
    CFunctionInfoEx* pParsedFunctionInfo;
    // for each selected item in the listview
    for (int Cnt=0;Cnt<this->pListview->GetItemCount();Cnt++)
    {
        if (!this->pListview->IsItemSelected(Cnt))
            continue;
        if (!this->pListview->GetItemUserData(Cnt,(LPVOID*)&pItem))
            continue;
        if (!pCompiledFunctionList->IsItemStillInList(pItem))
            continue;
        pParsedFunctionInfo=(CFunctionInfoEx*)pItem->ItemData;

        bFunctionRet = HookFunctionUsingAutoHooking(pParsedFunctionInfo);
        bRet=bRet && bFunctionRet;
    }
    this->RefreshNetHookList();

    return bRet;
}

BOOL CNetInteraction::UnhookSelected()
{
    BOOL bRet=TRUE;
    BOOL bFunctionRet;
    int ItemIndex=this->pListview->GetSelectedIndex();
    if (ItemIndex<0)
    {
        HookNetInfos.DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("No Item Selected"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }
    CLinkListItem* pItem;
    CFunctionInfoEx* pParsedFunctionInfo;
    // for each selected item in the listview
    for (int Cnt=0;Cnt<this->pListview->GetItemCount();Cnt++)
    {
        if (!this->pListview->IsItemSelected(Cnt))
            continue;
        if (!this->pListview->GetItemUserData(Cnt,(LPVOID*)&pItem))
            continue;
        if (!pCompiledFunctionList->IsItemStillInList(pItem))
            continue;
        pParsedFunctionInfo=(CFunctionInfoEx*)pItem->ItemData;

        bFunctionRet = UnhookFunction(pParsedFunctionInfo,FALSE);
        bRet=bRet && bFunctionRet;
    }
    this->RefreshNetHookList();

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: ModelessDialogThread2
// Object: allow to act like a dialog box in modeless mode
// Parameters :
//     in  : PVOID lParam : HINSTANCE hInstance : application instance
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CNetInteraction::ModelessDialogThread2(PVOID lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    HDESK OldDesktop=NULL;
    HDESK CurrentDesktop=NULL;
    HWINSTA OldStation=NULL;
    HWINSTA CurrentStation=NULL;
    if (!HookNetInfos.SetDefaultStation(&CurrentStation,&OldStation,&CurrentDesktop,&OldDesktop))
    {
        HookNetInfos.DynamicMessageBoxInDefaultStation(NULL,_T("Error setting default station : can't display COM Interaction dialog"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        HookNetInfos.RestoreStation(CurrentStation,OldStation,CurrentDesktop,OldDesktop);
        return 0;
    }

    INT_PTR Ret;
    CNetInteraction* pDialog=new CNetInteraction();
    pDialog->hInstance=DllhInstance;

    InitCommonControls();
    Ret=DialogBoxParam(DllhInstance,(LPCTSTR)IDD_DIALOG_NET_INTERACTION,NULL,(DLGPROC)CNetInteraction::WndProc,(LPARAM)pDialog);

    delete pDialog;

    HookNetInfos.RestoreStation(CurrentStation,OldStation,CurrentDesktop,OldDesktop);

    if (hSemaphoreOpenDialogs)
        ReleaseSemaphore(hSemaphoreOpenDialogs,1,NULL); // must be last instruction

    return (DWORD)Ret;
}
//-----------------------------------------------------------------------------
// Name: ModelessDialogThread
// Object: allow to act like a dialog box in modeless mode
// Parameters :
//     in  : PVOID lParam : HINSTANCE hInstance : application instance
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CNetInteraction::ModelessDialogThread(PVOID lParam)
{
    // as this dialog is shown in hooked module, we have to set window station and desktop
    // like for the Break dialog in Apioverride.dll
    // we have to use Apioverride.dll functions because they manages the count of displayed dialogs
    if (!HookNetInfos.CanWindowInteract())
    {
        HookNetInfos.DynamicMessageBoxInDefaultStation(
            NULL,
            _T("Process can't interact with user interface, so break window can't be displayed.\r\n")
            _T("Your process is currently breaked, to allow you to do some operation\r\n")
            _T("Click ok to resume it")
            _T("\r\n\r\nNotice: To use break dialog with services, please refer to documentation,\r\n")
            _T("in FAQ section: How to use break dialog with services ?"),
            _T("Error"),
            MB_OK|MB_ICONERROR|MB_TOPMOST);
        return 0;
    }

    CloseHandle(HookNetInfos.AdjustThreadSecurityAndLaunchDialogThread(CNetInteraction::ModelessDialogThread2,lParam));
    return 0;
}

//-----------------------------------------------------------------------------
// Name: Show
// Object: show the dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//           TCHAR* pszFile : file to parse at startup
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CNetInteraction::Show()
{
    // create thread instead of using CreateDialogParam to don't have to handle keyboard event like TAB
    CloseHandle(CreateThread(NULL,0,CNetInteraction::ModelessDialogThread,DllhInstance,0,NULL));

}

//-----------------------------------------------------------------------------
// Name: GetAssociatedDialogObject
// Object: Get object associated to window handle
// Parameters :
//     in  : HWND hWndDialog : handle of the window
//     out :
//     return : associated object if found, NULL if not found
//-----------------------------------------------------------------------------
CNetInteraction* CNetInteraction::GetAssociatedDialogObject(HWND hWndDialog)
{
    return (CNetInteraction*)GetWindowLongPtr(hWndDialog,GWLP_USERDATA);
}

//-----------------------------------------------------------------------------
// Name: OnSizing
// Object: check dialog box size
// Parameters :
//     in out  : RECT* pRect : pointer to dialog rect
//     return : 
//-----------------------------------------------------------------------------
void CNetInteraction::OnSizing(RECT* pRect)
{
    // check min width and min height
    if ((pRect->right-pRect->left)<CNetInteraction_DIALOG_MIN_WIDTH)
        pRect->right=pRect->left+CNetInteraction_DIALOG_MIN_WIDTH;
    if ((pRect->bottom-pRect->top)<CNetInteraction_DIALOG_MIN_HEIGHT)
        pRect->bottom=pRect->top+CNetInteraction_DIALOG_MIN_HEIGHT;
}

//-----------------------------------------------------------------------------
// Name: OnSize
// Object: Resize controls
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CNetInteraction::OnSize()
{
    RECT RectWindow;
    RECT Rect;
    DWORD SpaceBetweenControls;

    // get window Rect
    GetClientRect(this->hWndDialog,&RectWindow);

    this->pToolbar->Autosize();

    ///////////////
    // listview 
    ///////////////
    CDialogHelper::GetClientWindowRect(this->hWndDialog,this->pListview->GetControlHandle(),&Rect);
    SpaceBetweenControls=Rect.left;
    SetWindowPos(this->pListview->GetControlHandle(),HWND_NOTOPMOST,
        0,0,
        (RectWindow.right-RectWindow.left)-2*SpaceBetweenControls,
        RectWindow.bottom-Rect.top-SpaceBetweenControls,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);

    // redraw dialog
    CDialogHelper::Redraw(this->hWndDialog);
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CNetInteraction::Init()
{
    this->pItemDialog=pLinkListOpenDialogs->AddItem(this->hWndDialog);

    // toolbar
    this->pToolbar=new CToolbar(DllhInstance,this->hWndDialog,TRUE,TRUE,24,24);
    this->pToolbar->AddButton(IDC_BUTTON_REFRESH,IDI_ICON_REFRESH,_T("Refresh List"));
    this->pToolbar->AddSeparator();
    this->pToolbar->AddButton(IDC_BUTTON_SAVE,IDI_ICON_SAVE,_T("Save List"));
    this->pToolbar->AddSeparator();
    this->pToolbar->AddButton(IDC_BUTTON_SHOW_ASM,IDI_ICON_ASM,_T("Show Generated ASM"));
    this->pToolbar->AddSeparator();
    this->pToolbar->AddButton(IDC_BUTTON_HOOK_SELECTED,IDI_ICON_HOOK,_T("Hook Selected"));
    this->pToolbar->AddButton(IDC_BUTTON_UNHOOK_SELECTED,IDI_ICON_UNHOOK,_T("Unhook Selected"));

    // list view
    this->pListview=new CListview(GetDlgItem(this->hWndDialog,IDC_LIST_NET_INERACTION));
    this->pListview->AddColumn(_T("Name"),230,LVCFMT_LEFT);
    this->pListview->AddColumn(_T("Token"),80,LVCFMT_LEFT);
    this->pListview->AddColumn(_T("Module Name"),230,LVCFMT_LEFT);
    this->pListview->AddColumn(_T("Asm Start"),80,LVCFMT_LEFT);
    this->pListview->AddColumn(_T("Asm Len"),80,LVCFMT_LEFT);
    this->pListview->AddColumn(_T("Hooked"),80,LVCFMT_LEFT);
    this->pListview->SetStyle(TRUE,FALSE,FALSE,FALSE);
    this->pListview->SetPopUpMenuItemClickCallback(CNetInteraction::CallbackListViewMenuItemClickStatic,this);


    this->MenuShowGeneratedAsmId=this->pListview->pPopUpMenu->Add(_T("Show Generated Asm"),(UINT)0);
// TODO / to be implemented
//    this->MenuShowILCodeId=this->pListview->pPopUpMenu->Add(_T("Show Intermediate Language code"),1);
    this->MenuHookSelectedId=this->pListview->pPopUpMenu->Add(_T("Hook Selected"),1);
    this->MenuUnhookSelectedId=this->pListview->pPopUpMenu->Add(_T("Unhook Selected"),2);
    this->pListview->pPopUpMenu->AddSeparator(3);


    // render layout
    this->OnSize();

    this->RefreshNetHookList();

    FlashWindow(this->hWndDialog,FALSE);
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CNetInteraction::Close()
{
    delete this->pToolbar;
    this->pToolbar=NULL;
    delete this->pListview;
    this->pListview=NULL;

    // if pLinkListOpenDialogs is not locked by dll unload
    if (!hSemaphoreOpenDialogs)
        pLinkListOpenDialogs->RemoveItem(this->pItemDialog);

    EndDialog(this->hWndDialog,this->DialogResult);

    return;
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog callback of the dump dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CNetInteraction::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            CNetInteraction* pDialog=(CNetInteraction*)lParam;
            pDialog->hWndDialog=hWnd;

            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pDialog);

            pDialog->Init();
            // load dlg icons
            CDialogHelper::SetIcon(hWnd,IDI_ICON_APP);
        }
        break;
    case WM_CLOSE:
        {
            CNetInteraction* pDialog=CNetInteraction::GetAssociatedDialogObject(hWnd);
            if (!pDialog)
                break;
            pDialog->Close();
        }
        break;
    case WM_SIZING:
        {
            CNetInteraction* pDialog=CNetInteraction::GetAssociatedDialogObject(hWnd);
            if (!pDialog)
                break;
            pDialog->OnSizing((RECT*)lParam);
        }
        break;
    case WM_SIZE:
        {
            CNetInteraction* pDialog=CNetInteraction::GetAssociatedDialogObject(hWnd);
            if (!pDialog)
                break;
            pDialog->OnSize();
        }
        break;
    case WM_COMMAND:
        {
            CNetInteraction* pDialog=CNetInteraction::GetAssociatedDialogObject(hWnd);
            if (!pDialog)
                break;
            switch (LOWORD(wParam))
            {
            case IDC_BUTTON_SHOW_ASM:
                pDialog->ShowGeneratedAssembly();
                break;
            case IDC_BUTTON_UNHOOK_SELECTED:
                pDialog->UnhookSelected();
                break;
            case IDC_BUTTON_HOOK_SELECTED:
                pDialog->HookSelected();
                break;
            case IDC_BUTTON_REFRESH:
                pDialog->RefreshNetHookList();
                break;
            case IDC_BUTTON_SAVE:
                {
                    if (pDialog->pListview)
                    {
                        pDialog->pListview->Save();
                    }
                    break;
                }
            }
        }
        break;
    case WM_NOTIFY:
        {
            CNetInteraction* pDialog=CNetInteraction::GetAssociatedDialogObject(hWnd);
            if (!pDialog)
                break;

            if (pDialog->pListview)
            {
                if (pDialog->pListview->OnNotify(wParam, lParam))
                    break;
            }
            if (pDialog->pToolbar)
            {
                if (pDialog->pToolbar->OnNotify(wParam, lParam))
                    break;
            }
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}
