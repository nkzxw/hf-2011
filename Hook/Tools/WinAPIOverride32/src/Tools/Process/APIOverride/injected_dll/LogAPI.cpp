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
// Object: manages the sending of logs to main application
//-----------------------------------------------------------------------------

#include "LogAPI.h"
#include "../InterProcessCommunication.h"
#include "ModuleFilters.h"
#include "../../MailSlot/MailSlotClient.h"
#include "../../../LinkList/SingleThreaded/LinkListSingleThreaded.h"
#include "../../../Disasm/CallInstructionSizeFromRetAddress/CallInstructionSizeFromRetAddress.h"

#pragma intrinsic (memcpy,memset,memcmp)
extern CMailSlotClient* pMailSlotClt;
extern DWORD dwCurrentProcessID;
extern BOOL  bLogCallStack;
extern DWORD CallStackEbpRetrievalSize;
extern CModulesFilters* pModulesFilters;
extern CNET_Manager* pNetManager;
extern HANDLE ApiOverrideLogApiTmpHeap;

//-----------------------------------------------------------------------------
// Name: AddLogEntry
// Object: write log informations to logging mailslot
// Parameters :
//     in : 
//          API_INFO* pAPIInfo : pointer to current APIInfo to log
//          LOG_INFOS* pLogInfo : pointer to Log Info containing parameters value and other log content
//          DWORD dwReturnValue : return address
//          PVOID pOriginAddress : original address
//          int iParametersType : specify parameter log direction type
//          TCHAR* pszCallingModuleName : calling module name
//          DWORD dwCallingModuleRelativeAddress : calling relative address (absolute adress - calling module base address)
//          PREGISTERS pRegistersBeforeCall : registers before call 
//          PREGISTERS pRegistersAfterCall : registers after call
//          BYTE NumberOfParameterLogged : number of params log : 
//                                          useful if x param log before call and boggus monitoring file, so 
//                                          pAPIInfo->ParamCount may contain more param than really logged
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CLogAPI::AddLogEntry(API_INFO* pAPIInfo,
                          LOG_INFOS* pLogInfo,
                          PBYTE ReturnValue,
                          double DoubleResult,
                          BOOL bFailure,
                          PBYTE ReturnAddress,
                          int iParametersType,
                          TCHAR* pszCallingModuleName,
                          PBYTE CallingModuleRelativeAddress,
                          PREGISTERS pRegistersBeforeCall,
                          PREGISTERS pRegistersAfterCall,
                          PBYTE CallerEbp,
                          DWORD ThreadId,
                          CLinkListSingleThreaded* pLinkListTlsData
                          )
{

    BOOL bResult;
    SEND_LOG_FIXED_SIZE_STRUCT SendStruct;
    BYTE Cnt;
    SIZE_T ModuleNameLength;
    SIZE_T APINameLength;
    SIZE_T Length;
    DWORD CallingModuleNameLength;
    PBYTE Buffer;
    SIZE_T BufferIndex;
    SIZE_T SubStructSize;
    DWORD NumberOfParameterLogged=pLogInfo->NbParameters;
    DWORD CallInstructionSize=CCallInstructionSizeFromReturnAddress::GetCallInstructionSizeFromReturnAddress(ReturnAddress);
    PBYTE pOriginAddress= ReturnAddress - CallInstructionSize;
    CLinkListItem* pItem;
    API_HANDLER_TLS_DATA* pTlsData;

    // NumberOfParameterLogged < pAPIInfo->ParamCount can be if monitoring 
    // file contains 0 args and func use really 4. "In" direction log wont contain params value
    // but if file contains more args than really used, adjust param number to real ones
    if (NumberOfParameterLogged>pAPIInfo->MonitoringParamCount)
        NumberOfParameterLogged=pAPIInfo->MonitoringParamCount;

    SendStruct.dwCommande=CMD_MONITORING_LOG;

    // fill tagLogEntry struct
    SendStruct.LogEntry.dwProcessId = dwCurrentProcessID;
    SendStruct.LogEntry.dwThreadId = ThreadId;
    SendStruct.LogEntry.ReturnValue = ReturnValue;
    SendStruct.LogEntry.DoubleResult=DoubleResult;
    SendStruct.LogEntry.pOriginAddress = pOriginAddress;
    SendStruct.LogEntry.bParamDirectionType=(BYTE)iParametersType;
    SendStruct.LogEntry.RelativeAddressFromCallingModuleName=CallingModuleRelativeAddress;
    SendStruct.LogEntry.dwLastError=pLogInfo->dwLastErrorCode;
    SendStruct.LogEntry.CallTime=pLogInfo->CallTime;
    SendStruct.LogEntry.dwCallDuration=pLogInfo->dwCallDuration;
    SendStruct.LogEntry.bFailure=(BOOLEAN)bFailure;
    SendStruct.LogEntry.RegistersBeforeCall=*pRegistersBeforeCall;
    SendStruct.LogEntry.RegistersAfterCall=*pRegistersAfterCall;
    SendStruct.LogEntry.FirstHookedParentCallTime.dwHighDateTime=0;
    SendStruct.LogEntry.FirstHookedParentCallTime.dwLowDateTime=0;
    SendStruct.LogEntry.bNumberOfParameters=(BYTE)NumberOfParameterLogged;

    // get length of strings
    ModuleNameLength=(DWORD)(_tcslen(pAPIInfo->szModuleName)+1)*sizeof(TCHAR);
    APINameLength=(DWORD)(_tcslen(pAPIInfo->szAPIName)+1)*sizeof(TCHAR);
    CallingModuleNameLength=(DWORD)(_tcslen(pszCallingModuleName)+1)*sizeof(TCHAR);

    // compute full message size (without fields dwCommande and dwFullMessageSize)
    SendStruct.dwFullMessageSize=sizeof(LOG_ENTRY_FIXED_SIZE);
    SendStruct.dwFullMessageSize+=sizeof(DWORD)+ModuleNameLength;
    SendStruct.dwFullMessageSize+=sizeof(DWORD)+APINameLength;
    SendStruct.dwFullMessageSize+=sizeof(DWORD)+CallingModuleNameLength;

    // for each parameter, get it's required size
    for (Cnt=0;Cnt<NumberOfParameterLogged;Cnt++)
    {
        SendStruct.dwFullMessageSize+=sizeof(PARAMETER_LOG_INFOS)
                                      -sizeof(BYTE*)// because BYTE* pbPointedValue belongs to structure
                                      +pLogInfo->ParamLogList[Cnt].dwSizeOfPointedValue;
        if (pLogInfo->ParamLogList[Cnt].dwSizeOfData>sizeof(PBYTE))
            SendStruct.dwFullMessageSize+=pLogInfo->ParamLogList[Cnt].dwSizeOfData;

        if (pLogInfo->ParamLogList[Cnt].dwType & EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_DEFINE_VALUES_FILE)
        {
            SendStruct.dwFullMessageSize+=sizeof(DWORD);
            SendStruct.dwFullMessageSize+=( _tcslen(pAPIInfo->ParamList[Cnt].pDefineInfos->szFileName)+1 )*sizeof(TCHAR);
        }
        if (pLogInfo->ParamLogList[Cnt].dwType & EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_USER_DATA_TYPE_FILE)
        {
            SendStruct.dwFullMessageSize+=sizeof(DWORD);
            SendStruct.dwFullMessageSize+=(_tcslen(pAPIInfo->ParamList[Cnt].pUserTypeInfos->szName)+1)*sizeof(TCHAR);
        }
    }


    SendStruct.LogEntry.CallStackSize=0;
    SendStruct.LogEntry.CallStackEbpRetrievalSize=(WORD)(CallStackEbpRetrievalSize&0xffff);
    CLinkListSingleThreaded CallList(sizeof(CALLSTACK_ITEM_INFO_TEMP));
    CallList.SetHeap(ApiOverrideLogApiTmpHeap);

    // get first hooked caller start time for call analysis
    pItem=pLinkListTlsData->Tail->PreviousItem;
    if (pItem)
    {
        pTlsData=(API_HANDLER_TLS_DATA*)pItem->ItemData;
        SendStruct.LogEntry.FirstHookedParentCallTime=pTlsData->LogInfoIn.CallTime;
    }

    Cnt=0;
    // if we have to get call stack 
    if (bLogCallStack)
    {
        CALLSTACK_ITEM_INFO_TEMP CallStackInfo;
        HMODULE CallingModuleHandle;
        PBYTE Ebp;
        PBYTE PreviousEbp;
        PBYTE RetAddress;
        PBYTE PreviousRetAddress;
        BOOL  ShouldLog;
        BOOL  SnapshotTaked=FALSE;

        Ebp=CallerEbp;
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

            // increase size of required buffer length
            SendStruct.dwFullMessageSize+=2*sizeof(PBYTE)+sizeof(DWORD)+((DWORD)_tcslen(CallStackInfo.pszModuleName)+1)*sizeof(TCHAR)
                                        +SendStruct.LogEntry.CallStackEbpRetrievalSize;

        }


        // in case stack crawling fails, as we manage a small trace for hooks,
        // use our trace to get previous ebp and start stack walking again
        if (pItem)
        {
            pTlsData=(API_HANDLER_TLS_DATA*)pItem->ItemData;
            Ebp=(PBYTE)pTlsData->OriginalRegisters.ebp;
            goto EbpParsing;
        }

        // update size of retrieved call stack
        SendStruct.LogEntry.CallStackSize=(WORD)CallList.GetItemsCount();

    }

    SendStruct.LogEntry.HookType = pAPIInfo->HookType;
    switch (SendStruct.LogEntry.HookType)
    {
    case HOOK_TYPE_COM:
        SendStruct.dwFullMessageSize+=sizeof(tagExtendedFunctionInfosForCOM);
        break;
    case HOOK_TYPE_NET:
        SendStruct.dwFullMessageSize+=sizeof(tagExtendedFunctionInfosForNET);
        break;
    //case HOOK_TYPE_API:
    //default:
    //    SendStruct.dwFullMessageSize+=0;
    //    break;
    }
    
    // allocate buffer to send full data throw mail slot
    // Buffer=(PBYTE)HeapAlloc(ApiOverrideHeap,0,SendStruct.dwFullMessageSize+2*sizeof(DWORD));
    // use of _alloca to earn some time
    Buffer=(PBYTE)_alloca(SendStruct.dwFullMessageSize+2*sizeof(DWORD));
    if (!Buffer)
        return FALSE;

    // copy fixed size struct
    memcpy(Buffer,&SendStruct,sizeof(SEND_LOG_FIXED_SIZE_STRUCT));
    BufferIndex=sizeof(SEND_LOG_FIXED_SIZE_STRUCT);

    // copy ModuleName, APIName, CallingModuleName
    memcpy(&Buffer[BufferIndex],&ModuleNameLength,sizeof(DWORD));
    BufferIndex+=sizeof(DWORD);
    memcpy(&Buffer[BufferIndex],pAPIInfo->szModuleName,ModuleNameLength);
    BufferIndex+=ModuleNameLength;

    memcpy(&Buffer[BufferIndex],&APINameLength,sizeof(DWORD));
    BufferIndex+=sizeof(DWORD);
    memcpy(&Buffer[BufferIndex],pAPIInfo->szAPIName,APINameLength);
    BufferIndex+=APINameLength;

    memcpy(&Buffer[BufferIndex],&CallingModuleNameLength,sizeof(DWORD));
    BufferIndex+=sizeof(DWORD);
    memcpy(&Buffer[BufferIndex],pszCallingModuleName,CallingModuleNameLength);
    BufferIndex+=CallingModuleNameLength;

    // copy all parameters informations
    for (Cnt=0;Cnt<NumberOfParameterLogged;Cnt++)
    {
        // copy first parameters of PARAMETER_LOG_INFOS
        //DWORD dwType;
        //PBYTE Value;// if (dwSizeOfPointedData >0 )
        //DWORD dwSizeOfData;// size of Data. If <=REGISTER_BYTE_SIZE param value is stored in Value (no memory allocation) else in pbValue 
        //DWORD dwSizeOfPointedValue;// size of pbValue.
        SubStructSize = 3*sizeof(DWORD)+sizeof(PBYTE);

        memcpy(&Buffer[BufferIndex],&pLogInfo->ParamLogList[Cnt],SubStructSize);
        BufferIndex+=SubStructSize;

        memcpy(&Buffer[BufferIndex],pAPIInfo->ParamList[Cnt].pszParameterName,PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE*sizeof(TCHAR));
        BufferIndex+=PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE*sizeof(TCHAR);

        // if param is a pointer and the pointed value is valid
        if (pLogInfo->ParamLogList[Cnt].dwSizeOfPointedValue!=0)
        {
            // copy pointed data buffer
            memcpy(&Buffer[BufferIndex],pLogInfo->ParamLogList[Cnt].pbValue,pLogInfo->ParamLogList[Cnt].dwSizeOfPointedValue);
            BufferIndex+=pLogInfo->ParamLogList[Cnt].dwSizeOfPointedValue;
        }

        // if param is more than sizeof(PBYTE) bytes length it's content is in pbValue
        if (pLogInfo->ParamLogList[Cnt].dwSizeOfData>sizeof(PBYTE))
        {
            // copy pointed data buffer
            memcpy(&Buffer[BufferIndex],pLogInfo->ParamLogList[Cnt].pbValue,pLogInfo->ParamLogList[Cnt].dwSizeOfData);
            BufferIndex+=pLogInfo->ParamLogList[Cnt].dwSizeOfData;
        }

        // if define file is defined for parameter
        if (pLogInfo->ParamLogList[Cnt].dwType & EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_DEFINE_VALUES_FILE)
        {
            Length = (_tcslen(pAPIInfo->ParamList[Cnt].pDefineInfos->szFileName)+1)*sizeof(TCHAR);
            memcpy(&Buffer[BufferIndex],&Length,sizeof(DWORD));
            BufferIndex+=sizeof(DWORD);

            memcpy(&Buffer[BufferIndex],pAPIInfo->ParamList[Cnt].pDefineInfos->szFileName,Length);
            BufferIndex+=Length;

        }

        // if User Define Type is defined for parameter
        if (pLogInfo->ParamLogList[Cnt].dwType & EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_USER_DATA_TYPE_FILE)
        {
            Length = (_tcslen(pAPIInfo->ParamList[Cnt].pUserTypeInfos->szName)+1)*sizeof(TCHAR);
            memcpy(&Buffer[BufferIndex],&Length,sizeof(DWORD));
            BufferIndex+=sizeof(DWORD);

            memcpy(&Buffer[BufferIndex],pAPIInfo->ParamList[Cnt].pUserTypeInfos->szName,Length);
            BufferIndex+=Length;
        }
    }

    // if we have to get call stack and this one is not empty
    if (bLogCallStack && SendStruct.LogEntry.CallStackSize)
    {
        CLinkListItem* pItem;
        CALLSTACK_ITEM_INFO_TEMP* pCallStackInfo;
        DWORD AccessibleMemorySize;
        
        // for each item of call stack
        for(pItem=CallList.Head;pItem;pItem=pItem->NextItem)
        {
            pCallStackInfo=(CALLSTACK_ITEM_INFO_TEMP*)pItem->ItemData;

            // copy address
            memcpy(&Buffer[BufferIndex],&pCallStackInfo->Address,sizeof(PBYTE));
            BufferIndex+=sizeof(PBYTE);

            // copy relative address
            memcpy(&Buffer[BufferIndex],&pCallStackInfo->RelativeAddress,sizeof(PBYTE));
            BufferIndex+=sizeof(PBYTE);

            // copy length of module name
            CallingModuleNameLength=(DWORD)(_tcslen(pCallStackInfo->pszModuleName)+1)*sizeof(TCHAR);
            memcpy(&Buffer[BufferIndex],&CallingModuleNameLength,sizeof(DWORD));
            BufferIndex+=sizeof(DWORD);

            // copy module name
            memcpy(&Buffer[BufferIndex],pCallStackInfo->pszModuleName,CallingModuleNameLength);
            BufferIndex+=CallingModuleNameLength;

            // copy parameters (assume memory is readable as we get an arbitrary number of parameters)
            memset(&Buffer[BufferIndex],0,SendStruct.LogEntry.CallStackEbpRetrievalSize);

            AccessibleMemorySize=SendStruct.LogEntry.CallStackEbpRetrievalSize;
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
                memcpy(&Buffer[BufferIndex],pCallStackInfo->ParametersPointer,AccessibleMemorySize);
            BufferIndex+=SendStruct.LogEntry.CallStackEbpRetrievalSize;

            
        }
    }

    switch (SendStruct.LogEntry.HookType)
    {
    case HOOK_TYPE_COM:
        memcpy(&Buffer[BufferIndex],&pAPIInfo->HookTypeExtendedFunctionInfos.InfosForCOM,sizeof(tagExtendedFunctionInfosForCOM));
        BufferIndex+=sizeof(tagExtendedFunctionInfosForCOM);
        break;
    case HOOK_TYPE_NET:
        memcpy(&Buffer[BufferIndex],&pAPIInfo->HookTypeExtendedFunctionInfos.InfosForNET,sizeof(tagExtendedFunctionInfosForNET));
        BufferIndex+=sizeof(tagExtendedFunctionInfosForNET);
        break;
    //case HOOK_TYPE_API:
    //default:
    //    break;
    }


    // transmit struct to remote process
    bResult=pMailSlotClt->Write(Buffer, SendStruct.dwFullMessageSize+2*sizeof(DWORD));

    // free allocated memory _alloca --> no free
    // HeapFree(ApiOverrideHeap,0,Buffer);

    return bResult;
}