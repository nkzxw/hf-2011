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
// Object: manages the call stack dialog
//-----------------------------------------------------------------------------

#include "CallStackUserInterface.h"

#include "../../../Disasm/CallInstructionSizeFromRetAddress/CallInstructionSizeFromRetAddress.h"

extern HINSTANCE DllhInstance;
extern CModulesFilters* pModulesFilters;
extern DWORD CallStackEbpRetrievalSize;
extern CNET_Manager* pNetManager;

CCallStackUserInterface::CCallStackUserInterface(PBYTE CallerEbp,PBYTE ReturnAddress,CLinkListSingleThreaded* pLinkListTlsData)
{
    this->CallerEbp=CallerEbp;
    this->ReturnAddress=ReturnAddress;
    this->pLinkListTlsData=pLinkListTlsData;
}

CCallStackUserInterface::~CCallStackUserInterface(void)
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
CCallStackUserInterface* CCallStackUserInterface::GetAssociatedObject(HWND hWndDialog)
{
    return (CCallStackUserInterface*)DynamicGetWindowLong(hWndDialog,GWLP_USERDATA);
}


//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CCallStackUserInterface::Init(HWND hWndDialog)
{
    this->hWndDialog=hWndDialog;

    // associate object to window handle
    DynamicSetWindowLongPtr(hWndDialog,GWLP_USERDATA,(LONG)this);


    // parse call stack

    DWORD Cnt=0;
    PBYTE Ebp;
    PBYTE PreviousEbp;
    PBYTE RetAddress;
    BOOL  ShouldLog;
    HMODULE CallingModuleHandle;
    CLinkList CallList(sizeof(CALLSTACK_ITEM_INFO_TEMP));


    // if we have to get call stack 
    CALLSTACK_ITEM_INFO_TEMP CallStackInfo;
    PBYTE PreviousRetAddress;
    BOOL  SnapshotTaked=FALSE;
    API_HANDLER_TLS_DATA* pTlsData;
    CLinkListItem* pItem;
    DWORD CallInstructionSize;

    Ebp=this->CallerEbp;
    RetAddress=0;
    PreviousRetAddress=0;
    pItem=pLinkListTlsData->Tail;

    if(IsBadReadPtr(Ebp,REGISTER_BYTE_SIZE))
    {
        PreviousEbp = 0;
    }
    else
    {
        PreviousEbp=*(PBYTE*)(Ebp);
    }

EbpParsing:
    for(;;) 
    {
        if(pItem)
        {
            pTlsData=(API_HANDLER_TLS_DATA*)pItem->ItemData;
            if (Ebp>=(PBYTE)pTlsData->OriginalRegisters.ebp)
            {
                // assume current log is traced as a return value
                // (check case of no ebp change callee)
                if (PreviousRetAddress)
                {
                    if (pTlsData->pAPIInfo->HookType==HOOK_TYPE_NET)
                    {
                        if ( (PreviousRetAddress<(PBYTE)pTlsData->pAPIInfo->APIAddress)
                            || (PreviousRetAddress>(PBYTE)pTlsData->pAPIInfo->APIAddress+pNetManager->GetNetCompiledFunctionSize((PBYTE)pTlsData->pAPIInfo->APIAddress))
                            )
                        {
                            RetAddress=(PBYTE)pTlsData->pAPIInfo->APIAddress;
                            // avoid infinite loop
                            PreviousRetAddress=(PBYTE)pTlsData->pAPIInfo->APIAddress;
                            // avoid ebp walk
                            goto LogFrame;
                        }
                    }
                    else
                    {
                        // we got no way to get function size (don't check for pdb file).
                        // "hope" that function size is less than 4096 bytes. 
                        // Notice : we can miss a function call (if callee is located between function start and function start + 4096)
                        //          or we can report the same call twice (if function size is more than 4096)
                        if ( (PreviousRetAddress<(PBYTE)pTlsData->pAPIInfo->APIAddress)
                            || (PreviousRetAddress>(PBYTE)pTlsData->pAPIInfo->APIAddress+4096)
                            )
                        {
                            RetAddress=(PBYTE)pTlsData->pAPIInfo->APIAddress;
                            // avoid infinite loop
                            PreviousRetAddress=(PBYTE)pTlsData->pAPIInfo->APIAddress;
                            // avoid ebp walk
                            goto LogFrame;
                        }
                    }
                }
                // log current log ret value
                Ebp=(PBYTE)pTlsData->OriginalRegisters.ebp;
                // fill return address
                RetAddress=pTlsData->OriginalReturnAddress;
                // remove current item
                pItem=pItem->PreviousItem;
                // avoid ebp walk
                goto LogFrame;
            }
        }

        // return address is at current ebp+REGISTER_BYTE_SIZE
        // so get it
        if(IsBadReadPtr(Ebp+REGISTER_BYTE_SIZE,REGISTER_BYTE_SIZE))
            break;
        PreviousRetAddress=RetAddress;
        RetAddress=*(PBYTE*)(Ebp+REGISTER_BYTE_SIZE);
        if (IsBadCodePtr((FARPROC)RetAddress))
            break;
        if(IsBadReadPtr(Ebp,REGISTER_BYTE_SIZE))
            break;

        // get previous ebp (call -1 ebp)
        PreviousEbp=*(PBYTE*)(Ebp);// can throw hardware exception even IsBadReadPtr on application closing

        // if no previous ebp
        if (IsBadReadPtr(PreviousEbp,REGISTER_BYTE_SIZE))
            break;

        // stack coherence checking : 
        // function having PreviousEbp has been called by function having EBP (we walk stack backward)
        // so PreviousEbp MUSTE BE greater or equal to EBP
        // and there if it's equal, we can't guess previous function ebp --> we have to stop
        if (Ebp>=PreviousEbp)
            break; // the stack crawling failure checking at end of for will restore correct ebp

        // update ebp
        Ebp=PreviousEbp;


        if(pItem)
        {
            pTlsData=(API_HANDLER_TLS_DATA*)pItem->ItemData;
            if (Ebp>=(PBYTE)pTlsData->OriginalRegisters.ebp)
            {
                continue;
            }
        }
LogFrame:
        // get pointer to parameters (params are at ebp+2*REGISTER_BYTE_SIZE for func pushing ebp)
        // ebp was already updated to the caller ebp
        // it's interesting to link retAddress with previous ebp parameters, because we get caller function with caller function parameters
        CallStackInfo.ParametersPointer=(PBYTE)(Ebp+2*REGISTER_BYTE_SIZE); 


        // get return address
        CallStackInfo.Address=RetAddress;

        // get address relative address and calling module name
        CallStackInfo.RelativeAddress=0;
        if (!pModulesFilters->GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress(
            CallStackInfo.Address,
            &CallingModuleHandle,
            CallStackInfo.pszModuleName,
            &CallStackInfo.RelativeAddress,
            &ShouldLog,
            FALSE,TRUE
            ))
        {
            // caller address seems to come from a module which don't belong to process, try again taking a snapshot (do it only once)
            // it could be the case in case of injected memory or not rejitted .NET code
            if (!SnapshotTaked)
            {
                SnapshotTaked=TRUE;
                pModulesFilters->GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress(
                    CallStackInfo.Address,
                    &CallingModuleHandle,
                    CallStackInfo.pszModuleName,
                    &CallStackInfo.RelativeAddress,
                    &ShouldLog,
                    TRUE,TRUE
                    );
            }
        }
        CallInstructionSize=CCallInstructionSizeFromReturnAddress::GetCallInstructionSizeFromReturnAddress(RetAddress);

        // adjust return address to call addr
        if (CallStackInfo.RelativeAddress)
            CallStackInfo.Address-=CallInstructionSize;

        if ((ULONG_PTR)CallStackInfo.RelativeAddress>CallInstructionSize)
            CallStackInfo.RelativeAddress-=CallInstructionSize;
        else
            CallStackInfo.RelativeAddress=0;
        // add item to list
        CallList.AddItem(&CallStackInfo);
    }
    // in case stack crawling fails, as we manage a small trace for hooks,
    // use our trace to get previous ebp and start stack walking again
    if (pItem)
    {
        pTlsData=(API_HANDLER_TLS_DATA*)pItem->ItemData;
        Ebp=(PBYTE)pTlsData->OriginalRegisters.ebp;
        goto EbpParsing;
    }


    if (CallList.GetItemsCount()==0)
    {
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Can't get more than return address"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
        this->Close();
        return;
    }

    TCHAR* psz=new TCHAR[(CallList.GetItemsCount()+1)*(MAX_SIZEOF_ONE_STACK_ITEM_REPRESENTATION+30*CallStackEbpRetrievalSize/sizeof(PBYTE))];
    TCHAR pszOneCall[MAX_SIZEOF_ONE_STACK_ITEM_REPRESENTATION];
    TCHAR* pszParams=new TCHAR[30*CallStackEbpRetrievalSize/sizeof(PBYTE)];
    TCHAR* pName;
    CALLSTACK_ITEM_INFO_TEMP* pCallStackInfo;
    DWORD AccessibleMemorySize;

    *psz=0;

    CPE** PeArray=new CPE*[CallList.GetItemsCount()];
    CPE*  pPe;
    TCHAR PeFileName[MAX_PATH];
    TCHAR FunctionName[MAX_PATH];
    DWORD PeArrayUsedSize=0;
    DWORD Cnt2;
    DWORD CallingModuleNameSize;

    // for each item of the list --> for each call
    
    for (pItem=CallList.Head;pItem; pItem=pItem->NextItem)
    {
        pCallStackInfo=(CALLSTACK_ITEM_INFO_TEMP*)pItem->ItemData;


        ////////////////////////////
        // try to get calling function name
        ////////////////////////////
        *FunctionName=0;
        CallingModuleNameSize=(DWORD)_tcslen(pCallStackInfo->pszModuleName);
        if (CallingModuleNameSize>4)
        {
            if (_tcsicmp(&pCallStackInfo->pszModuleName[CallingModuleNameSize-4],_T(".dll"))==0)
            {

                // check if PE of dll has already been parsed
                pPe=NULL;
                for (Cnt2=0;Cnt2<PeArrayUsedSize;Cnt2++)
                {
                    PeArray[Cnt2]->GetFileName(PeFileName);
                    if (_tcsicmp(PeFileName,pCallStackInfo->pszModuleName)==0)
                    {
                        pPe=PeArray[Cnt2];
                        break;
                    }
                }
                if(pPe==NULL)
                {
                    // pe has not been parsed
                    // --> parse it's export table
                    pPe=new CPE(pCallStackInfo->pszModuleName);
                    pPe->Parse(TRUE,FALSE);

                    // add pPe to PeArray
                    PeArray[PeArrayUsedSize]=pPe;
                    PeArrayUsedSize++;
                }


                this->TryToGetDllFunctionName(pPe,
                    (PBYTE)pCallStackInfo->RelativeAddress,
                    FunctionName,
                    MAX_PATH-1
                    );
            }
        }

        // address and module name
        pName=_tcsrchr(pCallStackInfo->pszModuleName,'\\');
        if (pName)
        {
            // end path
            *pName=0;
            // point to module name only
            pName++;

            _stprintf(pszOneCall,_T("0x%p    %s+0x%p    (%s)\r\n"),pCallStackInfo->Address,pName,pCallStackInfo->RelativeAddress,pCallStackInfo->pszModuleName);
        }
        else
            _stprintf(pszOneCall,_T("0x%p    %s+0x%p\r\n"),pCallStackInfo->Address,pCallStackInfo->pszModuleName,pCallStackInfo->RelativeAddress);

        ////////////////////////////
        // add function name
        ////////////////////////////
        if (*FunctionName)
        {
            _tcscat(pszOneCall,_T("\t\tMAY originated from "));
            _tcscat(pszOneCall,FunctionName);
            _tcscat(pszOneCall,_T("("));
        }
        else
            _tcscat(pszOneCall,_T("\t\tOriginated from ??? ("));

        _tcscat(psz,pszOneCall);


        // call stack
        AccessibleMemorySize=CallStackEbpRetrievalSize;
        // reducing size memory checker loop
        while(   IsBadReadPtr(pCallStackInfo->ParametersPointer,AccessibleMemorySize)
            && (AccessibleMemorySize !=0)
            )
        {
            if (AccessibleMemorySize>REGISTER_BYTE_SIZE)// avoid buffer underflow
                AccessibleMemorySize-=REGISTER_BYTE_SIZE;
            else
                AccessibleMemorySize=0;
        }
        // if there's valid data
        if (AccessibleMemorySize)
        {
            for (Cnt=0;Cnt<AccessibleMemorySize/REGISTER_BYTE_SIZE;Cnt++)
            {
                // if not first element
                if (Cnt>0)
                    // add splitter
                    _tcscat(psz,_T(", "));
                // add param content
                //_stprintf(pszParams,_T("Param%u=0x%p"),Cnt+1,*(PBYTE*)&pCallStackInfo->ParametersPointer[Cnt*REGISTER_BYTE_SIZE]);
                _stprintf(pszParams,_T("0x%p"),*(PBYTE*)&pCallStackInfo->ParametersPointer[Cnt*REGISTER_BYTE_SIZE]);
                _tcscat(psz,pszParams);
            }
            _tcscat(psz,_T(")\r\n"));
        }

    }

    // free memory
    for (Cnt2=0;Cnt2<PeArrayUsedSize;Cnt2++)
    {
        delete PeArray[Cnt2];
    }
    delete[] PeArray;

    delete[] pszParams;

    DynamicSendMessage(DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_CALL_STACK),WM_SETTEXT,0,(LPARAM)psz);

    delete[] psz;

}
//-----------------------------------------------------------------------------
// Name: TryToGetDllFunctionName
// Object: try to get function name from dll name and relative address
// Parameters :
//     in  : TCHAR* DllName : name of the dll
//           PBYTE RelativeAddress : relative address for which we wan't to try to get name
//           DWORD MaxpszFuncNameSize : max size in TCHAR of pszFuncName
//     out : TCHAR* pszFuncName : MAY the function name (only disassembly can assume it)
//     return : FALSE if no function name can be found
//-----------------------------------------------------------------------------
BOOL CCallStackUserInterface::TryToGetDllFunctionName(TCHAR* DllName,PBYTE RelativeAddress, OUT TCHAR* pszFuncName,DWORD MaxpszFuncNameSize)
{
    // parse export table of the dll
    CPE Pe(DllName);
    if (!Pe.Parse(TRUE,FALSE))
        return FALSE;
    return this->TryToGetDllFunctionName(&Pe,RelativeAddress,pszFuncName,MaxpszFuncNameSize);
}

BOOL CCallStackUserInterface::TryToGetDllFunctionName(CPE* pPe,PBYTE RelativeAddress, OUT TCHAR* pszFuncName,DWORD MaxpszFuncNameSize)
{
    // find the nearest lower exported function
    CPE::EXPORT_FUNCTION_ITEM* pNearestLower=NULL;
    CLinkListItem* pItem;
    
    CPE::EXPORT_FUNCTION_ITEM* pCurrent;
    pPe->pExportTable->Lock();
    for(pItem=pPe->pExportTable->Head;pItem;pItem=pItem->NextItem)
    {
        pCurrent=(CPE::EXPORT_FUNCTION_ITEM*)pItem->ItemData;

        // if relative address of function is less than called address
        if (pCurrent->FunctionAddressRVA<=(UINT_PTR)RelativeAddress)
        {
            // if pNearestLower has already been defined
            if (pNearestLower==NULL)
                pNearestLower=pCurrent;
            // if relative address is upper than pNearestLower address
            else if (pCurrent->FunctionAddressRVA>(DWORD)pNearestLower->FunctionAddressRVA)
                pNearestLower=pCurrent;
        }
    }
    pPe->pExportTable->Unlock();

    if (!pNearestLower)
        return FALSE;

    // copy func name to pszFuncName
    _tcsncpy(pszFuncName,pNearestLower->FunctionName,MaxpszFuncNameSize-1);

    // if function has no name
    if (*pszFuncName==0)
        _sntprintf(pszFuncName,MaxpszFuncNameSize-1,_T("function ordinal 0x%X"),pNearestLower->ExportedOrdinal);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: Close like. EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CCallStackUserInterface::Close()
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
INT_PTR CCallStackUserInterface::ShowDialog(HWND ParentHandle)
{
    // show dialog
    return DynamicDialogBoxParam(DllhInstance,(LPTSTR)IDD_DIALOG_CALL_STACK,ParentHandle,(DLGPROC)CCallStackUserInterface::WndProc,(LPARAM)this);
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog callback of the dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CCallStackUserInterface::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CCallStackUserInterface* pDialogObject; 
    switch (uMsg)
    {
    case WM_INITDIALOG:
        pDialogObject=(CCallStackUserInterface*)lParam;
        pDialogObject->Init(hWnd);
        break;
    case WM_CLOSE:
        pDialogObject=CCallStackUserInterface::GetAssociatedObject(hWnd);
        if (pDialogObject)
            pDialogObject->Close();
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            pDialogObject=CCallStackUserInterface::GetAssociatedObject(hWnd);
            if (!pDialogObject)
                break;
            pDialogObject->Close();
            break;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}