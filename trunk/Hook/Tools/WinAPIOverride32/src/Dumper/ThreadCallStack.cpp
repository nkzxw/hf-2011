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
// Object: class helper for getting thread call stack
//-----------------------------------------------------------------------------

#include "threadcallstack.h"

#include "resource.h"
#include "../Tools/Thread/ThreadContext.h"
#include "../Tools/APIError/APIError.h"
#include "../Tools/Process/ProcessCallStack/ProcessCallStack.h"
#include "../Tools/Process/ProcessAndThreadID/ProcessAndThreadID.h"
#include "../Tools/pe/PE.h"
#include "../Tools/Dll/DllFindFuncNameFromRVA.h"
#include "../Tools/DebugInfos/DebugInfosClasses/DebugInfos.h"
#include "../Tools/File/StdFileOperations.h"

CThreadCallStack::CThreadCallStack(HANDLE hThread)
{
    this->hThread=hThread;
}
CThreadCallStack::~CThreadCallStack()
{

}

//-----------------------------------------------------------------------------
// Name: ShowFilterDialog
// Object: show the filter dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
INT_PTR CThreadCallStack::Show(HINSTANCE hInstance,HWND hWndDialog)
{
    // show dialog
    return DialogBoxParam(hInstance,(LPCTSTR)IDD_DIALOG_CALLSTACK,hWndDialog,(DLGPROC)CThreadCallStack::WndProc,(LPARAM)this);
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CThreadCallStack::Init()
{
    BOOL bError;
    PBYTE Eip;
    PBYTE Ebp;
    CThreadContext ThreadContext;
    CProcessAndThreadID ProcessAndThreadID;
    
    DWORD SuspendedCount;
    DWORD dwProcessId;
    CLinkList* pListCallStackItem=NULL;

    // get process ID of thread
    dwProcessId=ProcessAndThreadID.GetProcessIdOfThread(this->hThread);


    if (ProcessAndThreadID.GetThreadId(this->hThread)==GetCurrentThreadId())
    {
        MessageBox(this->hWndDialog,_T("Can't get stack information for my thread"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
        this->Close();
        return;
    }

    // suspend thread
    SuspendedCount=SuspendThread(this->hThread);
	if (SuspendedCount==(DWORD)-1)
    {
        CAPIError::ShowLastError();
        this->Close();
        return;
    }

    bError=FALSE;

    // get context
    if (ThreadContext.GetSuspendedThreadEipEbp(this->hThread,&Eip,&Ebp))
    {
        pListCallStackItem=new CLinkList(sizeof(CProcessCallStack::CALLSTACK_ITEM));
        bError=!CProcessCallStack::GetCallStack(dwProcessId,Ebp,pListCallStackItem);
        // check if we get at least one call
        if (!bError)
            bError=(pListCallStackItem->GetItemsCount()==0);
        if (bError)
        {
            MessageBox(this->hWndDialog,_T("Can't retrieve call stack or stack empty"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
            this->Close();
        }
    }
    else
        bError=TRUE;

    // resume thread after having retrieve call stack
	if (ResumeThread(hThread)==(DWORD)-1)
        CAPIError::ShowLastError();

    if (pListCallStackItem->GetItemsCount()==0)
    {
        bError=TRUE;
    }
    if (bError)
    {
        if (pListCallStackItem)
            delete pListCallStackItem;
        return;
    }

    // making pretty print
    TCHAR pszOneCall[5*MAX_PATH];
    TCHAR* psz=new TCHAR[pListCallStackItem->GetItemsCount()*5*MAX_PATH];
    TCHAR* pName;
    CLinkListItem* pItem;
    CProcessCallStack::CALLSTACK_ITEM* pCallStackItem;
    TCHAR FunctionName[MAX_PATH];

    *psz=0;

    CPE** PeArray=new CPE*[pListCallStackItem->GetItemsCount()];
    CPE*  pPe;
    DWORD PeArrayUsedSize=0;

    CDebugInfos** DebugInfosArray=new CDebugInfos*[pListCallStackItem->GetItemsCount()];
    CDebugInfos* pDebugInfos;
    DWORD DebugInfosArrayUsedSize=0;

    TCHAR PeFileName[MAX_PATH];
    DWORD Cnt2;
    PBYTE FuncStartAddress;
    BOOL bFunctionNameGetFromDebugInfos;

    // for each item of the list --> for each call
    pItem=pListCallStackItem->Head;
    while (pItem)
    {
        *FunctionName = 0;
        bFunctionNameGetFromDebugInfos = FALSE;

        pCallStackItem=(CProcessCallStack::CALLSTACK_ITEM*)pItem->ItemData;
        if (*(pCallStackItem->CallingModuleName))
        {
            pName=_tcsrchr(pCallStackItem->CallingModuleName,'\\');
            if (pName)
            {
                ////////////////////////////
                // try to get calling function name
                ////////////////////////////

                // 1) try to get infos from debug file if any

                // check if debug file has been parsed
                pDebugInfos=NULL;
                for (Cnt2=0;Cnt2<DebugInfosArrayUsedSize;Cnt2++)
                {
                    if (_tcsicmp(DebugInfosArray[Cnt2]->GetFileName(),pCallStackItem->CallingModuleName)==0)
                    {
                        pDebugInfos=DebugInfosArray[Cnt2];
                        break;
                    }
                }
                if (pDebugInfos==NULL)
                {
                    // debug file has not been parsed
                    pDebugInfos=new CDebugInfos(pCallStackItem->CallingModuleName,FALSE);

                    // add pDebugInfos to DebugInfosArray
                    DebugInfosArray[DebugInfosArrayUsedSize]=pDebugInfos;
                    DebugInfosArrayUsedSize++;
                }
                CFunctionInfos* pFunctionInfos = NULL;
                if (pDebugInfos->HasDebugInfos())
                {
                    pDebugInfos->FindFunctionByRVA((ULONGLONG)pCallStackItem->CallingRelativeAddressFromModuleBase,&pFunctionInfos);
                }
                if (pFunctionInfos)
                {
                    _tcscpy(FunctionName,pFunctionInfos->Name);
                    bFunctionNameGetFromDebugInfos = TRUE;
                    delete pFunctionInfos;
                }
                else
                {
                    // 2) try to find function name with dll exports
                    if (CStdFileOperations::DoesExtensionMatch(pCallStackItem->CallingModuleName,_T("dll")))
                    {

                        // check if PE of dll has already been parsed
                        pPe=NULL;
                        for (Cnt2=0;Cnt2<PeArrayUsedSize;Cnt2++)
                        {
                            PeArray[Cnt2]->GetFileName(PeFileName);
                            if (_tcsicmp(PeFileName,pCallStackItem->CallingModuleName)==0)
                            {
                                pPe=PeArray[Cnt2];
                                break;
                            }
                        }
                        if(pPe==NULL)
                        {
                            // pe has not been parsed
                            // --> parse it's export table
                            pPe=new CPE(pCallStackItem->CallingModuleName);
                            pPe->Parse(TRUE,FALSE);

                            // add pPe to PeArray
                            PeArray[PeArrayUsedSize]=pPe;
                            PeArrayUsedSize++;
                        }

                        CDllFindFuncNameFromRVA::FindFuncNameFromRVA(pPe,
                                                                    (PBYTE)pCallStackItem->CallingRelativeAddressFromModuleBase,
                                                                    FunctionName,
                                                                    MAX_PATH-1,
                                                                    &FuncStartAddress
                            );
                    }
                }

                ////////////////////////////
                // add dll name and path
                ////////////////////////////

                // end path
                *pName=0;
                // point to module name only
                pName++;
                _stprintf(pszOneCall,_T("0x%p    %s+0x%p    (%s)\r\n"),pCallStackItem->CallingAddress,pName,pCallStackItem->CallingRelativeAddressFromModuleBase,pCallStackItem->CallingModuleName);


                ////////////////////////////
                // add function name
                ////////////////////////////
                if (*FunctionName)
                {
                    if (bFunctionNameGetFromDebugInfos)
                    {
                        _tcscat(pszOneCall,_T("\t\tOriginated from "));
                    }
                    else
                    {
                        _tcscat(pszOneCall,_T("\t\tMAY originated from "));
                    }
                    _tcscat(pszOneCall,FunctionName);
                    _tcscat(pszOneCall,_T("("));
                }
            }
            else
                _stprintf(pszOneCall,_T("0x%p    %s+0x%p\r\n"),pCallStackItem->CallingAddress,pCallStackItem->CallingModuleName,pCallStackItem->CallingRelativeAddressFromModuleBase);
        }
        else
            _stprintf(pszOneCall,_T("0x%p\r\n"),pCallStackItem->CallingAddress);

        if (*FunctionName==0)
        {
            _tcscat(pszOneCall,_T("\t\tOriginated from ??? ("));
        }
        _tcscat(psz,pszOneCall);

        for (int Cnt=0;Cnt<CALLSTACK_EBP_RETRIEVAL_SIZE/REGISTER_BYTE_SIZE;Cnt++)
        {
            // if not first element
            if (Cnt>0)
                // add splitter
                _tcscat(psz,_T(", "));
            // add param content
            //_stprintf(pszOneCall,_T("Param%u=0x%p"),Cnt+1,*(PBYTE*)&pCallStackItem->ParametersPointer[Cnt*REGISTER_BYTE_SIZE]);
            _stprintf(pszOneCall,_T("0x%p"),*(PBYTE*)&pCallStackItem->ParametersPointer[Cnt*REGISTER_BYTE_SIZE]);
            _tcscat(psz,pszOneCall);
        }
        _tcscat(psz,_T(")\r\n"));


        // get next call
        pItem=pItem->NextItem;
    }

    // free memory
    for (Cnt2=0;Cnt2<DebugInfosArrayUsedSize;Cnt2++)
    {
        delete DebugInfosArray[Cnt2];
    }
    delete[] DebugInfosArray;


    for (Cnt2=0;Cnt2<PeArrayUsedSize;Cnt2++)
    {
        delete PeArray[Cnt2];
    }
    delete[] PeArray;

    SendMessage(GetDlgItem(this->hWndDialog,IDC_EDIT_CALL_STACK),WM_SETTEXT,0,(LPARAM)psz);

    delete pListCallStackItem;
    delete[] psz;
}



//-----------------------------------------------------------------------------
// Name: Close
// Object: EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CThreadCallStack::Close()
{
    EndDialog(this->hWndDialog,0);
}


//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog callback
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CThreadCallStack::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            CThreadCallStack* pThreadCallStack=(CThreadCallStack*)lParam;
            pThreadCallStack->hWndDialog=hWnd;

            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pThreadCallStack);

            pThreadCallStack->Init();
        }
        break;
    case WM_CLOSE:
        {
            CThreadCallStack* pThreadCallStack=((CThreadCallStack*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pThreadCallStack)
                break;
            pThreadCallStack->Close();
        }
        break;
    case WM_COMMAND:
        {
            CThreadCallStack* pThreadCallStack=((CThreadCallStack*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pThreadCallStack)
                break;

            switch (LOWORD(wParam))
            {
            case IDOK:
                pThreadCallStack->Close();
                break;
            }
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}