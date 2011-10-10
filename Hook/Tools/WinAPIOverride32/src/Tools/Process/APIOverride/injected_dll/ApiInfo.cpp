/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
originaly based from APISpy32 v2.1 from Yariv Kaplan @ WWW.INTERNALS.COM

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
// Object: API_Info associated functions
//-----------------------------------------------------------------------------
#include "apiinfo.h"

#pragma intrinsic (memcpy,memset,memcmp)
extern CLinkList* pLinkListAPIInfos;
extern CModulesFilters* pModulesFilters;
extern HANDLE ApiOverrideHeap;
extern BOOL ApiOverrideDllDetaching;

BOOL GetWinAPIOverrideModuleNameLike(TCHAR* pszModuleName,TCHAR* RealModuleName,SIZE_T RealModuleNameMaxSize)
{
    HMODULE hModule;
    TCHAR* psz;
    BOOL bInternalDefinition = FALSE;

    // check if address is specified
    if (
        (_tcsnicmp(pszModuleName,EXE_INTERNAL_RVA_POINTER_PREFIX,_tcslen(EXE_INTERNAL_RVA_POINTER_PREFIX))==0)
        ||(_tcsnicmp(pszModuleName,EXE_INTERNAL_RVA_PREFIX,_tcslen(EXE_INTERNAL_RVA_PREFIX))==0)
        ||(_tcsnicmp(pszModuleName,EXE_INTERNAL_POINTER_PREFIX,_tcslen(EXE_INTERNAL_POINTER_PREFIX))==0)
        ||(_tcsnicmp(pszModuleName,EXE_INTERNAL_PREFIX,_tcslen(EXE_INTERNAL_PREFIX))==0)
        )
    {
        hModule = ::GetModuleHandle(0);
        ::GetModuleFileName(hModule,RealModuleName,RealModuleNameMaxSize);
        bInternalDefinition = TRUE;
    }
    else if (_tcsnicmp(pszModuleName,DLL_INTERNAL_PREFIX,_tcslen(DLL_INTERNAL_PREFIX))==0)
    {
        TCHAR pszDllName[MAX_PATH];
        *pszDllName=0;
        _stscanf(&pszModuleName[_tcslen(DLL_INTERNAL_PREFIX)],_T("%*p@%s"),(TCHAR*)pszDllName);
        _tcsncpy(RealModuleName,pszDllName,RealModuleNameMaxSize);
        RealModuleName[RealModuleNameMaxSize-1]=0;
        bInternalDefinition = TRUE;
    }
    else if (_tcsnicmp(pszModuleName,DLL_INTERNAL_POINTER_PREFIX,_tcslen(DLL_INTERNAL_POINTER_PREFIX))==0)
    {
        TCHAR pszDllName[MAX_PATH];
        *pszDllName=0;
        _stscanf(&pszModuleName[_tcslen(DLL_INTERNAL_POINTER_PREFIX)],_T("%*p@%s"),(TCHAR*)pszDllName);
        _tcsncpy(RealModuleName,pszDllName,RealModuleNameMaxSize);
        RealModuleName[RealModuleNameMaxSize-1]=0;
        bInternalDefinition = TRUE;
    }
    else if (_tcsnicmp(pszModuleName,DLL_ORDINAL_PREFIX,_tcslen(DLL_ORDINAL_PREFIX))==0)
    {
        TCHAR pszDllName[MAX_PATH];
        *pszDllName=0;
        _stscanf(&pszModuleName[_tcslen(DLL_ORDINAL_PREFIX)],_T("%*p@%s"),(TCHAR*)pszDllName);
        _tcsncpy(RealModuleName,pszDllName,RealModuleNameMaxSize);
        RealModuleName[RealModuleNameMaxSize-1]=0;
        bInternalDefinition = TRUE;
    }
    else if (_tcsnicmp(pszModuleName,DLL_OR_EXE_NET_PREFIX,_tcslen(DLL_OR_EXE_NET_PREFIX))==0)
    {
        TCHAR pszDllName[MAX_PATH];
        // pszModuleName is like .NET@AssemblyName@functionToken
        psz = &pszModuleName[_tcslen(DLL_OR_EXE_NET_PREFIX)];
        _tcscpy(pszDllName,psz);
        psz = _tcschr(pszDllName,'@');
        if (psz)
            *psz=0;
        _tcsncpy(RealModuleName,pszDllName,RealModuleNameMaxSize);
        RealModuleName[RealModuleNameMaxSize-1]=0;
        bInternalDefinition = TRUE;
    }
    else
    {
        // pszModuleName is ok
        _tcsncpy(RealModuleName,pszModuleName,RealModuleNameMaxSize);
        RealModuleName[RealModuleNameMaxSize-1]=0;
    }

    if (bInternalDefinition)
    {
        // keep only file name (remove file path)
        psz = CStdFileOperations::GetFileName(RealModuleName);
        _tcscpy(RealModuleName,psz);

        RealModuleName[RealModuleNameMaxSize-2]=0;
        _tcscat(RealModuleName,_T("|"));
        _tcsncat(RealModuleName,pszModuleName,RealModuleNameMaxSize-_tcslen(RealModuleName)-1);
        RealModuleName[RealModuleNameMaxSize-1]=0;
    }

    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: InitializeApiInfo
// Object: Allocate memory and events associated to pAPIInfo
//         Notice1 : ApiAddress is not reset
//         Notice2 : Must be called only with not hooked or initialized pAPIInfo items
// Parameters :
//          in : API_INFO *pAPIInfo : pointer to APIInfo struct to initialize
//               TCHAR* pszModuleName  : module name
//               TCHAR* pszFunctionName : api name
// Return : 
//-----------------------------------------------------------------------------
BOOL __stdcall InitializeApiInfo(API_INFO *pAPIInfo,TCHAR* pszModuleName,TCHAR* pszFunctionName)
{
    // pAPIInfo are all allocated by GetAssociatedItemAPIInfo, which call pLinkListAPIInfos->AddItem
    // which finally call HeapAlloc(this->LinkListHeap,HEAP_ZERO_MEMORY,sizeof(CLinkListItem));
    // --> so all fields are already set to 0
    
    // anyway assume crashing param are set to 0
    pAPIInfo->MonitoringParamCount = 0;
    pAPIInfo->szModuleName=NULL;
    pAPIInfo->szAPIName=NULL;

    // create an event to signal end of hook
    pAPIInfo->evtEndOfHook=CreateEvent(NULL,FALSE,TRUE,NULL);
    if (!pAPIInfo->evtEndOfHook)
        return FALSE;

    // Initialize PARAMETER_INFOS structs with default values
    for (DWORD cnt=0;cnt<MAX_PARAM;cnt++)
    {
        memset(&pAPIInfo->ParamList[cnt],0,sizeof(PARAMETER_INFOS));
        // by default put all params to unknown
        pAPIInfo->ParamList[cnt].dwType=PARAM_UNKNOWN;
    }

    //////////////////////////////////////
    // copy module name
    //////////////////////////////////////
    TCHAR ModuleName[2*MAX_PATH];
    GetWinAPIOverrideModuleNameLike(pszModuleName,ModuleName,2*MAX_PATH);
    pAPIInfo->szModuleName = (TCHAR*)HeapAlloc(ApiOverrideHeap, 0, (_tcslen(ModuleName) + 1)*sizeof(TCHAR));
    if (!pAPIInfo->szModuleName)
    {
        DynamicMessageBoxInDefaultStation(NULL,_T("Memory allocation error"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }
    _tcscpy(pAPIInfo->szModuleName, ModuleName);

    //////////////////////////////////////
    // copy API name
    //////////////////////////////////////
    pAPIInfo->szAPIName = (TCHAR*)HeapAlloc(ApiOverrideHeap, 0, (_tcslen(pszFunctionName) + 1)*sizeof(TCHAR));
    if (!pAPIInfo->szAPIName)
    {
        DynamicMessageBoxInDefaultStation(NULL,_T("Memory allocation error"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }
    _tcscpy(pAPIInfo->szAPIName, pszFunctionName);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetStackSizeToRemove
// Object: get stack size removed according to calling convention
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
DWORD __stdcall GetStackSizeToRemove(API_INFO *pAPIInfo)
{
    // at least return address
    DWORD RemovedStackSize=sizeof(PBYTE);
    switch(pAPIInfo->CallingConvention)
    {
    case CALLING_CONVENTION_CDECL:
        break;
    // as we only compute number of parameter that can be passed throw registers,
    // there is no interest to know which parameters are passed on stack or not
    // but only there number. That's why we use a for++ loop for both cases
    case CALLING_CONVENTION_FASTCALL:
    case CALLING_CONVENTION_FASTCALL_PUSHED_LEFT_TO_RIGHT:
        {
            DWORD NbParamPassedByRegister=0;
            DWORD Cnt;
            BOOL ParamPassedThrowRegister;
            for (Cnt=0;Cnt<pAPIInfo->MonitoringParamCount;Cnt++)
            {
                ParamPassedThrowRegister=FALSE;
                if ((CSupportedParameters::IsParamPassedByRegisterWithFastcall(pAPIInfo->ParamList[Cnt].dwType & SIMPLE_TYPE_FLAG_MASK))
                    && (pAPIInfo->ParamList[Cnt].dwSizeOfData<=sizeof(PBYTE)))
                {
                    if (NbParamPassedByRegister<FASTCALL_NUMBER_OF_PARAM_POTENTIALY_PASSED_BY_REGISTER)
                    {
                        NbParamPassedByRegister++;
                        ParamPassedThrowRegister=TRUE;
                    }
                }
                if (!ParamPassedThrowRegister)
                {
                    // if data is a pointer
                    if (pAPIInfo->ParamList[Cnt].dwSizeOfData==0)
                        RemovedStackSize+=sizeof(PBYTE);
                    else
                        RemovedStackSize+=pAPIInfo->ParamList[Cnt].dwSizeOfData;
                }
            }
        }
        break;
    case CALLING_CONVENTION_THISCALL: // cdecl for vararg
        {
            DWORD Cnt;
            for (Cnt=1;Cnt<pAPIInfo->MonitoringParamCount;Cnt++)
            {
                // if data is a pointer
                if (pAPIInfo->ParamList[Cnt].dwSizeOfData==0)
                    RemovedStackSize+=sizeof(PBYTE);
                else
                    RemovedStackSize+=pAPIInfo->ParamList[Cnt].dwSizeOfData;
            }
        }
        break;
    case CALLING_CONVENTION_STDCALL:
    case CALLING_CONVENTION_STDCALL_OR_CDECL:
    default:
        {
            DWORD Cnt;
            for (Cnt=0;Cnt<pAPIInfo->MonitoringParamCount;Cnt++)
            {
                // if data is a pointer
                if (pAPIInfo->ParamList[Cnt].dwSizeOfData==0)
                    RemovedStackSize+=sizeof(PBYTE);
                else
                    RemovedStackSize+=pAPIInfo->ParamList[Cnt].dwSizeOfData;
            }
        }
        break;
    }
    return RemovedStackSize;
}

//-----------------------------------------------------------------------------
// Name: ReleaseAndFreeApiInfo
// Object: free API_INFO struct associated to hook
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
void __stdcall ReleaseAndFreeApiInfo(CLinkListItem* pItemAPIInfo)
{
    API_INFO *pAPIInfo;
    pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;

    // assume we get original opcode again
    pAPIInfo->bOriginalOpcodes=TRUE;

    // if dll hasn't been unloaded without hook removal
    if (!IsBadCodePtr((FARPROC)pAPIInfo->APIAddress))
    {
        // restore bytes only if not already done
        if (memcmp(pAPIInfo->APIAddress, pAPIInfo->Opcodes, pAPIInfo->OpcodeReplacementSize))
        {
            // assume opcodes is our one !
            // it can appear that for COM dll are unloaded and next reloaded at the same space,
            // if it's done too quickly or during COM unhooking, we can have original bytes
            // with original memory protection (due to reloading of dll), so pAPIInfo->APIAddress can be write protected
            if (memcmp(pAPIInfo->APIAddress,pAPIInfo->pbHookCodes, pAPIInfo->OpcodeReplacementSize)==0)
            {
                // restore original opcodes
                if (!IsBadWritePtr(pAPIInfo->APIAddress,pAPIInfo->OpcodeReplacementSize))
                    memcpy(pAPIInfo->APIAddress, pAPIInfo->Opcodes, pAPIInfo->OpcodeReplacementSize);
            }
        }
    }

    // restore original protection
    // unfortunately we can do it only if no other func belongs to the same memory SystemPageSize,
    // else will get a memory write error
    // DWORD dwScratch;
    // VirtualProtect(pAPIInfo->APIAddress, pAPIInfo->OpcodeReplacementSize, pAPIInfo->dwOldProtectionFlags, &dwScratch);

    // free memory associated to item, and item
    FreeApiInfoItem(pItemAPIInfo);
}

//-----------------------------------------------------------------------------
// Name: FreeApiInfoItem
// Object: Free memory and events associated to pItemAPIInfo
//         Notice : this function don't remove hook
//                  YOU HAVE TO ASSUME THAT ASSOCIATED HOOK HAS BEEN RELEASED BEFORE CALLING IT
// Parameters :
//          in : CLinkListItem* pItemAPIInfo : pointer to pItemAPIInfo to free
// Return : 
//-----------------------------------------------------------------------------
BOOL __stdcall FreeApiInfoItem(CLinkListItem* pItemAPIInfo)
{
    API_INFO* pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;
    // call CallBack only if dll is not detaching,
    // because on DLL_PROCESS_DETACH, we are in an unknown state : 
    // thread(s) may have been terminated and callback could deadlock 
    // (if terminated thread where owning some locks)
    if (!ApiOverrideDllDetaching)
    {
        if (!IsBadReadPtr(pAPIInfo,sizeof(API_INFO)))
        {
            // if a deletion callback has been defined
            if (pAPIInfo->DeletionCallback)
            {
                // if callback is still a valid code pointer
                if (!IsBadCodePtr((FARPROC)pAPIInfo->DeletionCallback))
                {
                    // call callback
                    pAPIInfo->DeletionCallback(pItemAPIInfo);
                }
            }
        }
    }

    // free memory associated to item
    FreeApiInfo(pAPIInfo);
    // remove item from linked list
    pLinkListAPIInfos->RemoveItem(pItemAPIInfo);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: FreeApiInfo
// Object: Free memory and events associated to pAPIInfo
// Parameters :
//          in : API_INFO* pAPIInfo : pointer to APIInfo to free
// Return : 
//-----------------------------------------------------------------------------
BOOL __stdcall FreeApiInfo(API_INFO* pAPIInfo)
{
    if (IsBadReadPtr(pAPIInfo,sizeof(API_INFO)))
        return FALSE;

    FreeOptionalParametersMemory(pAPIInfo);

    // free allocated memory for string
    if (pAPIInfo->szAPIName)
    {
        HeapFree(ApiOverrideHeap, 0, pAPIInfo->szAPIName);
        pAPIInfo->szAPIName=NULL;
    }

    if (pAPIInfo->szModuleName)
    {
        HeapFree(ApiOverrideHeap, 0, pAPIInfo->szModuleName);
        pAPIInfo->szModuleName=NULL;
    }

    if (pAPIInfo->evtEndOfHook)
    {
        CloseHandle(pAPIInfo->evtEndOfHook);
        pAPIInfo->evtEndOfHook=NULL;
    }

    if (pAPIInfo->PostApiCallChain)
    {
        CLinkList* pTmp;
        // delete Call Chain
        // assign to null before destroying content (better for multi threading)
        pTmp=pAPIInfo->PostApiCallChain;
        pAPIInfo->PostApiCallChain=NULL;
        delete pTmp;
    }

    if (pAPIInfo->PreApiCallChain)
    {
        CLinkList* pTmp;
        // delete Call Chain
        // assign to null before destroying content (better for multi threading)
        pTmp=pAPIInfo->PreApiCallChain;
        pAPIInfo->PreApiCallChain=NULL;
        delete pTmp;
    }

    if (pAPIInfo->OpcodesExecutedAtAnotherPlaceExtended)
    {
        HeapFree(ApiOverrideHeap, 0, pAPIInfo->OpcodesExecutedAtAnotherPlaceExtended);
        pAPIInfo->OpcodesExecutedAtAnotherPlaceExtended=NULL;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: FreeOptionalParametersMemory
// Object: Free memory of optional parameters info
// Parameters :
//     in  : API_INFO *pAPIInfo
// Return : 
//-----------------------------------------------------------------------------
void __stdcall FreeOptionalParametersMemory(API_INFO *pAPIInfo)
{
    FreeOptionalParametersMemory(pAPIInfo,0,pAPIInfo->MonitoringParamCount-1);
}
//-----------------------------------------------------------------------------
// Name: FreeOptionalParametersMemory
// Object: Free memory of optional parameters info from index FromIndex
// Parameters :
//     in  : API_INFO *pAPIInfo
//           BYTE FromIndex : index from which delete memory, !=0 only to remove
//                            some too much allocated memory (see number parameter 
//                            correction from APIOverrideKernel)
//           BYTE ToIndex : index to delete memory, must be < MAX_PARAM
//                          should be pAPIInfo->MonitoringParamCount
// Return : 
//-----------------------------------------------------------------------------
void __stdcall FreeOptionalParametersMemory(API_INFO *pAPIInfo,BYTE FromIndex,BYTE ToIndex)
{
    CLinkListItem* pItem;
    PMONITORING_PARAMETER_OPTIONS pParamOption;
    BYTE Cnt;
    CLinkList* pList;

    // remove all breaking or logging options
    // let it to avoid crash : when a flag is specified, it require memory that has been freed !!!
    memset(&pAPIInfo->LogBreakWay,0,sizeof(API_LOG_BREAK_WAY));

    if (ToIndex>=MAX_PARAM)
        ToIndex=MAX_PARAM-1;

    // we have to free optional parameter allocated memory
    for(Cnt=FromIndex;Cnt<=ToIndex;Cnt++)
    {
        pAPIInfo->ParamList[Cnt].dwSizeOfData=0;
        pAPIInfo->ParamList[Cnt].dwSizeOfPointedData=0;
        pAPIInfo->ParamList[Cnt].dwType=0;


        if (pAPIInfo->ParamList[Cnt].pConditionalLogContent!=NULL)
        {
            // store list pointer
            pList=pAPIInfo->ParamList[Cnt].pConditionalLogContent;
            // set pAPIInfo list pointer to NULL
            pAPIInfo->ParamList[Cnt].pConditionalLogContent=NULL;

            // free memory
            pList->Lock();
            for(pItem=pList->Head;pItem;pItem=pItem->NextItem)
            {
                pParamOption=(MONITORING_PARAMETER_OPTIONS*)pItem->ItemData;
                if (pParamOption->pbPointedValue!=0)
                    HeapFree(ApiOverrideHeap,0,pParamOption->pbPointedValue);
            }
            pList->RemoveAllItems(TRUE);
            pList->Unlock();
            delete pList;
        }

        if (pAPIInfo->ParamList[Cnt].pConditionalBreakContent!=NULL)
        {
            // store list pointer
            pList=pAPIInfo->ParamList[Cnt].pConditionalBreakContent;
            // set pAPIInfo list pointer to NULL
            pAPIInfo->ParamList[Cnt].pConditionalBreakContent=NULL;

            // free memory
            pList->Lock();
            for(pItem=pList->Head;pItem;pItem=pItem->NextItem)
            {
                pParamOption=(MONITORING_PARAMETER_OPTIONS*)pItem->ItemData;
                if (pParamOption->pbPointedValue!=0)
                    HeapFree(ApiOverrideHeap,0,pParamOption->pbPointedValue);
            }
            pList->RemoveAllItems(TRUE);
            pList->Unlock();
            delete pList;
        }
        if (pAPIInfo->ParamList[Cnt].pDefineInfos)
        {
            PVOID pMemory = pAPIInfo->ParamList[Cnt].pDefineInfos;
            pAPIInfo->ParamList[Cnt].pDefineInfos=NULL;
            HeapFree(ApiOverrideHeap, 0, pMemory);
        }
        if (pAPIInfo->ParamList[Cnt].pUserTypeInfos)
        {
            PVOID pMemory = pAPIInfo->ParamList[Cnt].pUserTypeInfos;
            pAPIInfo->ParamList[Cnt].pUserTypeInfos=NULL;
            HeapFree(ApiOverrideHeap, 0, pMemory);
        }
    }
}

//-----------------------------------------------------------------------------
// Name: UnHookIfPossible
// Object: unhook api if no other hooking way is needed
// Parameters :
//     in  : CLinkListItem* pItemAPIInfo : pointer to api info item
//           BOOL bRestoreOriginalBytes : FALSE if you don't want to restore original bytes
//                                        usefull if dll has been unloaded and another one
//                                        takes the same address space
// Return : result of UnhookAPIFunction, or TRUE if UnhookAPIFunction is not called
//-----------------------------------------------------------------------------
BOOL __stdcall UnHookIfPossible(CLinkListItem* pItemAPIInfo,BOOL bRestoreOriginalBytes)
{
    API_INFO* pAPIInfo;

    if(!pLinkListAPIInfos->IsItemStillInList(pItemAPIInfo))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        return FALSE;
    }

    if (IsBadReadPtr(pItemAPIInfo,sizeof(CLinkListItem)))
        return FALSE;

    pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;
    if (IsBadReadPtr(pAPIInfo,sizeof(API_INFO)))
        return FALSE;

    // check if a monitoring file ID still exists
    if (pAPIInfo->pMonitoringFileInfos)
        return TRUE;

    // check if a faking file ID still exists
    if (pAPIInfo->pFakeDllInfos)
        return TRUE;

    // check if a pre api call call back still exists
    if (pAPIInfo->PreApiCallChain)
    {
        // check if list has at least one item
        if (pAPIInfo->PreApiCallChain->Head)
            return TRUE;
    }
    // check if a post api call call back still exists
    if (pAPIInfo->PostApiCallChain)
    {
        // check if list has at least one item
        if (pAPIInfo->PostApiCallChain->Head)
            return TRUE;
    }

    if (bRestoreOriginalBytes)
    {
        // hook is useless for everyone --> remove it
        return UnhookAPIFunction(pItemAPIInfo);
    }

    FreeApiInfoItem(pItemAPIInfo);
    return TRUE;
}
