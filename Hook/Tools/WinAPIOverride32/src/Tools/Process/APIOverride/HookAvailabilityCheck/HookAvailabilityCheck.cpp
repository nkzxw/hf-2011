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
// Object: Allow to know if a function is hookable, and if first bytes can execute anywhere
//-----------------------------------------------------------------------------

#include "./hookavailabilitycheck.h"
CDllStub* CHookAvailabilityCheck::pDllStub = NULL;
CLinkListSimple* CHookAvailabilityCheck::pParsedPe = NULL;
/*
CHookAvailabilityCheck::CHookAvailabilityCheck(void)
{
}

CHookAvailabilityCheck::~CHookAvailabilityCheck(void)
{
}
*/

//-----------------------------------------------------------------------------
// Name: CanFirstBytesBeExecutedAnyWhere
// Object:  check functions first bytes
//          1) check if first byte are like the MS XP SP2 style : 
//              functions begins with 
//                 mov edi,edi (0x8B,0xFF)// == nop, nop --> to put hook with their Detour Lib ???
//                 push ebp    (0x55)
//                 mov epb,esp (0x8B,0xEC)
//          2) length disassembler
// Parameters :
//     in  : HANDLE hFile : handle to file to check
//           ULONGLONG FunctionRawAddress : function raw address
//     out : STRUCT_FIRST_BYTES_CAN_BE_EXECUTED_ANYWHERE_RESULT* pCanFirstBytesBeExecutedAnyWhereResult
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CHookAvailabilityCheck::CanFirstBytesBeExecutedAnyWhere(HANDLE hFile,ULONGLONG FunctionRawAddress,OUT STRUCT_FIRST_BYTES_CAN_BE_EXECUTED_ANYWHERE_RESULT* pCanFirstBytesBeExecutedAnyWhereResult)
{
    // default check return
    BYTE FirstBytes[HOOK_SIZE+20];
    DWORD ReadSize;

    if (hFile==INVALID_HANDLE_VALUE)
        return FALSE;

    // load first function bytes from file to memory
    if (!SetFilePointer(hFile,FunctionRawAddress,NULL,FILE_BEGIN))
        return FALSE;
    if (!ReadFile(hFile,FirstBytes,HOOK_SIZE+20,&ReadSize,NULL))
        return FALSE;

    // call the IsFunctionHookable method with pointer to first bytes of the function
    return CHookAvailabilityCheck::CanFirstBytesBeExecutedAnyWhere(FirstBytes,pCanFirstBytesBeExecutedAnyWhereResult);
}

//-----------------------------------------------------------------------------
// Name: CheckForAlign
// Object: check that NbBytesToCheck first bytes of Buffer are only alignment opcodes ( 90 nop or CC break)
// Parameters :
//     in  : PBYTE Buffer : buffer to check
//           int NbBytesToCheck : number of bytes to check
// return : TRUE if first NbBytesToCheck of Buffer are only nop
//          FALSE else
//-----------------------------------------------------------------------------
BOOL CHookAvailabilityCheck::CheckForAlign(PBYTE Buffer, int NbBytesToCheck)
{
    if (NbBytesToCheck<=0)
        return TRUE;
    if (IsBadReadPtr(Buffer,NbBytesToCheck))
        return FALSE;
    for (int cnt=0;cnt<NbBytesToCheck;cnt++)
    {
        // if not a nop operation
        if ((Buffer[cnt]!=0x90) && (Buffer[cnt]!=0xCC))
            return FALSE;
    }
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: CanFirstBytesBeExecutedAnyWhere
// Object:  check functions first bytes
//          1) check if first byte are like the MS XP SP2 style : 
//              functions begins with 
//                 mov edi,edi (0x8B,0xFF)// == nop, nop --> to put hook with their Detour Lib ???
//                 push ebp    (0x55)
//                 mov epb,esp (0x8B,0xEC)
//          2) length disassembler
// Parameters :
//     in  : BYTE* pFuncStart : pointer to first function bytes
//     out : STRUCT_FIRST_BYTES_CAN_BE_EXECUTED_ANYWHERE_RESULT* pCanFirstBytesBeExecutedAnyWhereResult
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CHookAvailabilityCheck::CanFirstBytesBeExecutedAnyWhere(BYTE* pFuncStart,OUT STRUCT_FIRST_BYTES_CAN_BE_EXECUTED_ANYWHERE_RESULT* pCanFirstBytesBeExecutedAnyWhereResult)
{
    // set default values
    pCanFirstBytesBeExecutedAnyWhereResult->FirstBytesCanBeExecutedAnyWhereResult=CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_NO;
    pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace=5;

    // first 
    FIRST_BYTES_SEQUENCE Match[]={
                                    {5,5,{0x8B,0xFF,0x55,0x8B,0xEC}}, // check for new library versions classical start
                                    {5,6,{0x55,0x8b,0xec,0x83,0xec}}, // check for push ebp; mov ebp,esp; sub esp 8bits
                                    {5,9,{0x55,0x8b,0xec,0x81,0xec}}  // check for push ebp; mov ebp,esp; sub esp 32bits
                                };
    for (int cnt=0;cnt<sizeof(Match)/sizeof(FIRST_BYTES_SEQUENCE);cnt++)
    {
        if (memcmp(pFuncStart,Match[cnt].Data,Match[cnt].CompareSize)==0)
        {
            pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace=Match[cnt].MovableSize;
            pCanFirstBytesBeExecutedAnyWhereResult->FirstBytesCanBeExecutedAnyWhereResult=CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_YES;
            return TRUE;
        }
    }


    CLengthDisasm32 DisAsm;
    BYTE* pByte;
    BOOL bFullSecure=FALSE;// true if first asm instruction size is >= HOOK_SIZE
                           // else relative jumps can be done inside changed code
                           // ex : 8b442404 mov eax, [esp+4] // 4 byte instruction size
                           //      668b08   mov cx,  [eax]   // due to the jump, our hook will make target application to crash
                           //      ....     ...
                           //               jmp (mov cx,[eax])

    
    pByte=pFuncStart;
    pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace=0;
    while (pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace<HOOK_SIZE)
    {
        // ret instruction with 16 bits esp pop value
        if ((*pByte==0xC2)||(*pByte==0xCA))
        {
            pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace+=3;// 1 for ret instruction +2 for 16bits value
            // if size is not enough to install a secure hook
            if (pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace<HOOK_SIZE)
            {
                // check for nop used for function alignment
                if (CHookAvailabilityCheck::CheckForAlign(pByte+3,HOOK_SIZE-pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace))
                    pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace=HOOK_SIZE;
            }
            break;
        }
        // ret instruction
        if ((*pByte==0xC3)||(*pByte==0xCB))
        {
            pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace++;
            // if size is not enough to install a secure hook
            if (pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace<HOOK_SIZE)
            {
                // check for nop used for function alignment
                if (CHookAvailabilityCheck::CheckForAlign(pByte+1,HOOK_SIZE-pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace))
                    pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace=HOOK_SIZE;
            }
            break;
        }

        // leave instruction 
        if (*pByte==0xC9)
        {
            pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace++;
            // if size is not enough to install a secure hook
            if (pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace<HOOK_SIZE)
            {
                // check for nop used for function alignment
                if (CHookAvailabilityCheck::CheckForAlign(pByte+1,HOOK_SIZE-pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace))
                    pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace=HOOK_SIZE;
            }
            break;
        }

        // check if we can execute at another place with address change (32 bits call and jumps)
        if ((*pByte==0xE9)   // relative JMP
            ||(*pByte==0xE8) // CALL relative
            ||((*pByte==0x0F)&&((0x80<=*(pByte+1))&&(*(pByte+1)<=0x8F)))// relative conditional jumps
            )
        {
            // if first instruction
            if (pByte==pFuncStart)
                bFullSecure=TRUE;

            // we return the size of 
            pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace+=sizeof(PBYTE)+1;
            if (*pByte==0x0F)
                pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace++;

            if (bFullSecure)
                pCanFirstBytesBeExecutedAnyWhereResult->FirstBytesCanBeExecutedAnyWhereResult=CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_YES_NEED_RELATIVE_ADDRESS_CHANGES;
            else
                pCanFirstBytesBeExecutedAnyWhereResult->FirstBytesCanBeExecutedAnyWhereResult=CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_MAY_NEED_RELATIVE_ADDRESS_CHANGES;
            return TRUE;
        }


        // check for relative 8 bits jumps
        if (((0x70<=*pByte)&&(*pByte<=0x7F)) // 8 bits relative conditional jumps
            ||((0xE0<=*pByte)&&(*pByte<=0xE3))// 8 bits relative loop + jcxz (0xE3)
            ||(*pByte==0xEB)// 8 bits relative JMP
            )
        {
            break;
        }

        // try to disasm opcode
        if (!DisAsm.Disasm(pByte))
            break;

#ifdef _DEBUG
        // for debug purpose only
        TCHAR psz[50];
        DisAsm.ToString(psz);
#endif
        // if first instruction
        if (pByte==pFuncStart)
        {
            if (DisAsm.length>=HOOK_SIZE)
                bFullSecure=TRUE;
        }

        // update remaining size which need to be analyzed
        pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace+=DisAsm.length;

        // mov pointer
        pByte+=DisAsm.length;
    }

    // check if movable bytes are not too small to install hook
    if ((pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace<HOOK_SIZE)
         // and check if movable bytes are not too big for APIOverride buffer
         // NO MORE USE SINCE A BUFFER IS ALLOCATED IF pAPIInfo->FirstBytesCanExecuteAnywhereSize>FIRST_OPCODES_MAX_SIZE
         // || (pCanFirstBytesBeExecutedAnyWhereResult->NbBytesToExecuteAtAnotherPlace>FIRST_OPCODES_MAX_SIZE)
         )
            pCanFirstBytesBeExecutedAnyWhereResult->FirstBytesCanBeExecutedAnyWhereResult=CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_NO;
    else
    {
        if (bFullSecure)
            pCanFirstBytesBeExecutedAnyWhereResult->FirstBytesCanBeExecutedAnyWhereResult=CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_YES;
        else
            pCanFirstBytesBeExecutedAnyWhereResult->FirstBytesCanBeExecutedAnyWhereResult=CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_MAY;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: IsFunctionHookable
// Object:  check functions first bytes to know if function is hookable
// Parameters :
//     in  : HANDLE hFile : handle to file to check
//           ULONGLONG FunctionRaw : function raw address
//     out : OUT FirstBytesCheckResult* pCheckResult : checking result
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CHookAvailabilityCheck::IsFunctionHookable(HANDLE hFile,ULONGLONG FunctionRawAddress,OUT IsFunctionHookableResult* pCheckResult)
{
    // default check return
    *pCheckResult=IS_FUNCTION_HOOKABLE_RESULT_MAY_NOT_HOOKABLE;

    BYTE FirstBytes[HOOK_SIZE+20];
    DWORD ReadSize;

    if (hFile==INVALID_HANDLE_VALUE)
        return FALSE;

    // load first function bytes from file to memory
    if (!SetFilePointer(hFile,FunctionRawAddress,NULL,FILE_BEGIN))
        return FALSE;
    if (!ReadFile(hFile,FirstBytes,HOOK_SIZE+20,&ReadSize,NULL))
        return FALSE;

    // call the IsFunctionHookable method with pointer to first bytes of the function
    return CHookAvailabilityCheck::IsFunctionHookable(FirstBytes,pCheckResult);

}

//-----------------------------------------------------------------------------
// Name: IsFunctionHookable
// Object:  check functions first bytes to know if function is hookable
// Parameters :
//     in  : BYTE* pFuncStart : pointer to first function bytes
//     out : OUT FirstBytesCheckResult* pCheckResult : checking result
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CHookAvailabilityCheck::IsFunctionHookable(BYTE* pFuncStart,OUT IsFunctionHookableResult* pCheckResult)
{
    CLengthDisasm32 DisAsm;
    BYTE* pByte;
    int RemainingRequiredLen;

    // default return
    *pCheckResult=IS_FUNCTION_HOOKABLE_RESULT_MAY_NOT_HOOKABLE;


    // check asm codes until there's sufficient size to install our hook
    RemainingRequiredLen=HOOK_SIZE;
    pByte=pFuncStart;
    while (RemainingRequiredLen>0)
    {
        // ret instruction with 16 bits esp pop value
        if ((*pByte==0xC2)||(*pByte==0xCA))
        {
            RemainingRequiredLen-=3;// 1 for ret instruction +2 for 16bits value
            
            // if size is not enough to install a secure hook
            if (RemainingRequiredLen>0)
            {
                // check for nop used for function alignment
                if (CHookAvailabilityCheck::CheckForAlign(pByte+1,RemainingRequiredLen))
                    RemainingRequiredLen=0;
            }
            break;
        }

        // ret instruction
        if ((*pByte==0xC3)||(*pByte==0xCB))
        {
            RemainingRequiredLen--;

            // if size is not enough to install a secure hook
            if (RemainingRequiredLen>0)
            {
                // check for nop used for function alignment
                if (CHookAvailabilityCheck::CheckForAlign(pByte+1,RemainingRequiredLen))
                    RemainingRequiredLen=0;
            }
            break;
        }

        // leave instruction 
        if (*pByte==0xC9)
        {
            RemainingRequiredLen--;

            // if size is not enough to install a secure hook
            if (RemainingRequiredLen>0)
            {
                // check for nop used for function alignment
                if (CHookAvailabilityCheck::CheckForAlign(pByte+1,RemainingRequiredLen))
                    RemainingRequiredLen=0;
            }
            break;
        }

        // try to disasm opcode
        if (!DisAsm.Disasm(pByte))
        {
            *pCheckResult=IS_FUNCTION_HOOKABLE_RESULT_MAY_NOT_HOOKABLE;
            return TRUE;
        }
#ifdef _DEBUG
        // for debug purpose only
        TCHAR psz[50];
        DisAsm.ToString(psz);
#endif

        // update remaining size which need to be analyzed
        RemainingRequiredLen-=DisAsm.length;

        // if instruction is a jump, function can be not hookable
        if (((0x70<=*pByte)&&(*pByte<=0x7F)) // conditional 8 or 16 bits jumps
            ||(*pByte==0xE3)||(*pByte==0xEB)
            )
        {
            if (RemainingRequiredLen>0)
                *pCheckResult=IS_FUNCTION_HOOKABLE_RESULT_MAY_NOT_HOOKABLE;
            else
                *pCheckResult=IS_FUNCTION_HOOKABLE_RESULT_MAY_HOOKABLE;

            return TRUE;
        }

        // mov pointer
        pByte+=DisAsm.length;
    }

    // if a ret or leave instruction appears before HOOK_SIZE is reached
    if (RemainingRequiredLen>0)
        *pCheckResult=IS_FUNCTION_HOOKABLE_RESULT_NOT_HOOKABLE;
    else
        *pCheckResult=IS_FUNCTION_HOOKABLE_RESULT_MAY_HOOKABLE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: FullCheck
// Object:  IsFunctionHookable + CanFirstBytesBeExecutedAnyWhere
//          THIS FUNCTION DON'T CHECK THE EXPORT TABLE INTEGRITY
// Parameters :
//     in  : TCHAR* WorkingDirectory : directory in which reside analysed binary
//           CPE* pPe : PE of file currently being parsed
//           TCHAR* FunctionName : name of function to analyse (meaningful only if bOrdinal=FALSE)
//           DWORD FunctionOrdinal : ordinal of function to analyse (meaningful only if bOrdinal=TRUE)
//           BOOL bOrdinal : Flag to know if search is done for ordinal or function name
//     out : OUT FUNCTION_CHECK_RESULT* pFunctionCheckResult
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CHookAvailabilityCheck::FullCheck(TCHAR* WorkingDirectory,CPE* pPe,TCHAR* pszFunctionName,DWORD FunctionOrdinal,
                                       BOOL bOrdinal,OUT FUNCTION_CHECK_RESULT* pFunctionCheckResult)
{
    CLinkListItem* pItem;
    CPE::EXPORT_FUNCTION_ITEM* pExportFunc;
    BOOL bFound;

    bFound=FALSE;
    // search function in export table
    pItem=pPe->pExportTable->Head;
    for (pItem=pPe->pExportTable->Head;pItem;pItem=pItem->NextItem)
    {
        pExportFunc=(CPE::EXPORT_FUNCTION_ITEM*)pItem->ItemData;

        // if FunctionOrdinal arg is meaningful
        if (bOrdinal)
            bFound=(FunctionOrdinal==pExportFunc->ExportedOrdinal);
        else // if pszFunctionName arg is meaningful
            bFound=(_tcscmp(pszFunctionName,pExportFunc->FunctionName)==0);

        // if searched function has been found
        if (bFound)
            // call FullCheck function and return
            return CHookAvailabilityCheck::FullCheck(WorkingDirectory,pPe,pExportFunc,pFunctionCheckResult);
    }
     return FALSE;
}

//-----------------------------------------------------------------------------
// Name: FullCheck
// Object:  IsFunctionHookable + CanFirstBytesBeExecutedAnyWhere + function in executable section
//          THIS FUNCTION DON'T CHECK THE EXPORT TABLE INTEGRITY
// Parameters :
//     in  : TCHAR* WorkingDirectory : directory in which reside analysed binary
//           CPE* pPe : PE of file currently being parsed
//           CPE::PEXPORT_FUNCTION_ITEM pExportFuncInfos : exported function infos for function to be checked
//     out : OUT FUNCTION_CHECK_RESULT* pFunctionCheckResult
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CHookAvailabilityCheck::FullCheck(TCHAR* WorkingDirectory,CPE* pPe,CPE::PEXPORT_FUNCTION_ITEM pExportFuncInfos,OUT FUNCTION_CHECK_RESULT* pFunctionCheckResult)
{
    BOOL bFunctionRet;
    TCHAR pszFilename[MAX_PATH];
    pPe->GetFileName(pszFilename);

    ULONGLONG FunctionRawAddress;
    BOOL bRes=TRUE;

    pFunctionCheckResult->CheckResult=CHookAvailabilityCheck::IS_FUNCTION_HOOKABLE_RESULT_MAY_HOOKABLE;
    pFunctionCheckResult->FirstBytesCanBeExecutedAnyWhereResult.FirstBytesCanBeExecutedAnyWhereResult=CHookAvailabilityCheck::CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_NO;
    pFunctionCheckResult->FirstBytesCanBeExecutedAnyWhereResult.NbBytesToExecuteAtAnotherPlace=0;
    pFunctionCheckResult->FunctionInExecutableSection=TRUE;


    if (pExportFuncInfos->Forwarded)
    {
        // pExportFuncInfos->ForwardedName is like "NTDLL.RtlZeroMemory"
        TCHAR ForwardedName[MAX_PATH];
        TCHAR ForwardedDll[MAX_PATH];

        if(!DecodeForwardedName(pExportFuncInfos->ForwardedName,ForwardedDll,ForwardedName))
            return FALSE;

        CDllStub* pDllStub = CHookAvailabilityCheck::GetDllStub();
        if (pDllStub->IsStubDll(ForwardedDll))
        {
            TCHAR RealDllName[MAX_PATH];
            if (!pDllStub->GetRealModuleNameFromStubName(pszFilename,ForwardedDll,RealDllName) )
                return 0;
            _tcscpy(ForwardedDll,RealDllName);
        }

        // the result of the function full check is the result of the forwarded function full check 
        return CHookAvailabilityCheck::FullCheck(WorkingDirectory,ForwardedDll,ForwardedName,pFunctionCheckResult);
    }
    else
    {
        if (!pPe->RvaToRaw(pExportFuncInfos->FunctionAddressRVA,&FunctionRawAddress))
            return FALSE;

        // open file
        HANDLE hFile;
        hFile=CreateFile(pszFilename,GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
        if (hFile==INVALID_HANDLE_VALUE)
            return FALSE;

        // check if function is in an executable section
        pFunctionCheckResult->FunctionInExecutableSection=pPe->IsExecutable(pExportFuncInfos->FunctionAddressRVA);

        // check if function is hookable by disasm
        bRes=CHookAvailabilityCheck::IsFunctionHookable(hFile,FunctionRawAddress,&pFunctionCheckResult->CheckResult);

        // get number of bytes that can be executed anywhere
        bFunctionRet = CHookAvailabilityCheck::CanFirstBytesBeExecutedAnyWhere(hFile,FunctionRawAddress,&pFunctionCheckResult->FirstBytesCanBeExecutedAnyWhereResult);
        bRes=bRes && bFunctionRet;

        // close file
        CloseHandle(hFile);

    }

    return bRes;
}

CPE* CHookAvailabilityCheck::GetPe(TCHAR* FullPath)
{
    CLinkListSimple* PeList = CHookAvailabilityCheck::GetParsedPe();
    CLinkListItem* pItem;
    CPE* Pe;
    TCHAR PeFileName[MAX_PATH];
    PeList->Lock();
    for (pItem = PeList->Head;pItem;pItem=pItem->NextItem)
    {
        Pe = (CPE*)pItem->ItemData;
        Pe->GetFileName(PeFileName);
        if (_tcsicmp(PeFileName,FullPath)==0)
        {
            PeList->Unlock();
            return Pe;
        }
    }
    // not found --> we have to parse pe
    Pe = new CPE(FullPath);
    if (Pe->Parse(TRUE,FALSE,FALSE))
    {
        PeList->AddItem(Pe,TRUE);
    }
    else
    {
        delete Pe;
        Pe = NULL;
    }
    PeList->Unlock();
    return Pe;
}

//-----------------------------------------------------------------------------
// Name: FullCheck
// Object:  IsFunctionHookable + CanFirstBytesBeExecutedAnyWhere
// Parameters :
//     in  : TCHAR* WorkingDirectory : directory in which reside analysed binary
//           TCHAR* DllName : name of dll to analyze
//           TCHAR* FunctionName : name of function to analyse (meanful only if bOrdinal=FALSE)
//           DWORD FunctionOrdinal : ordinal of function to analyse (meanful only if bOrdinal=TRUE)
//           BOOL bOrdinal : Flag to know if search is done for ordinal or function name
//     out : OUT FUNCTION_CHECK_RESULT* pFunctionCheckResult
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CHookAvailabilityCheck::FullCheck(TCHAR* WorkingDirectory,
                                       TCHAR* DllName,
                                       TCHAR* FunctionName,
                                       DWORD FunctionOrdinal,
                                       BOOL bOrdinal,
                                       OUT FUNCTION_CHECK_RESULT* pFunctionCheckResult
                                       )
{
    BOOL bRes=FALSE;
    TCHAR pszFullPath[MAX_PATH];
    if (!CDllFinder::FindDll(WorkingDirectory,DllName,pszFullPath))
        return FALSE;

    // parse PE
    CPE* pPe = CHookAvailabilityCheck::GetPe(pszFullPath);
    if (!pPe)
        return FALSE;

    // find function in exported array
    BOOL FunctionFound=FALSE;
    
    CLinkListItem* pItemExport;
    CPE::EXPORT_FUNCTION_ITEM* pExportFunction;
    for (pItemExport=pPe->pExportTable->Head;pItemExport;pItemExport=pItemExport->NextItem)
    {
        pExportFunction=(CPE::EXPORT_FUNCTION_ITEM*)pItemExport->ItemData;
        if (bOrdinal)
            FunctionFound=(FunctionOrdinal==pExportFunction->ExportedOrdinal);
        else
            FunctionFound=(_tcscmp(FunctionName,pExportFunction->FunctionName)==0);

        // function found
        if (FunctionFound)
        {
            // full check of the function
            bRes=CHookAvailabilityCheck::FullCheck(WorkingDirectory,
                                                pPe,
                                                pExportFunction,
                                                pFunctionCheckResult);
        }
    }


    return bRes;
}

//-----------------------------------------------------------------------------
// Name: FullCheck
// Object:  IsFunctionHookable + CanFirstBytesBeExecutedAnyWhere
// Parameters :
//     in  : TCHAR* WorkingDirectory : directory in which reside analysed binary
//           TCHAR* DllName : name of dll to analyze
//           DWORD FunctionOrdinal : ordinal of function to analyse (meanful only if bOrdinal=TRUE)
//     out : OUT FUNCTION_CHECK_RESULT* pFunctionCheckResult
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CHookAvailabilityCheck::FullCheck(TCHAR* WorkingDirectory,TCHAR* DllName,DWORD FunctionOrdinal,OUT FUNCTION_CHECK_RESULT* pFunctionCheckResult)
{
    return CHookAvailabilityCheck::FullCheck(WorkingDirectory,
                                            DllName,
                                            NULL,
                                            FunctionOrdinal,
                                            TRUE,
                                            pFunctionCheckResult
                                            );
}

//-----------------------------------------------------------------------------
// Name: FullCheck
// Object:  IsFunctionHookable + CanFirstBytesBeExecutedAnyWhere
// Parameters :
//     in  : TCHAR* WorkingDirectory : directory in which reside analysed binary
//           TCHAR* DllName : name of dll to analyze
//           TCHAR* FunctionName : name of function to analyse (meanful only if bOrdinal=FALSE)
//     out : OUT FUNCTION_CHECK_RESULT* pFunctionCheckResult
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CHookAvailabilityCheck::FullCheck(TCHAR* WorkingDirectory,TCHAR* DllName,TCHAR* FunctionName,OUT FUNCTION_CHECK_RESULT* pFunctionCheckResult)
{
    return CHookAvailabilityCheck::FullCheck(WorkingDirectory,
                                            DllName,
                                            FunctionName,
                                            0,
                                            FALSE,
                                            pFunctionCheckResult
                                            );
}

//-----------------------------------------------------------------------------
// Name: DecodeForwardedName
// Object:  split "NTDLL.RtlZeroMemory" into "NTDLL.DLL" + "RtlZeroMemory"
//          Warning ForwardedDllName and ForwardedFunctionName must be allocated and contain sufficent amount of memory
// Parameters :
//     in  : TCHAR* ForwardedName : Forwarded name as stored in PE like "NTDLL.RtlZeroMemory"
//     out : TCHAR* ForwardedDllName : name of the dll like "NTDLL.DLL"
//           TCHAR* ForwardedFunctionName : name of the function like "RtlZeroMemory"
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CHookAvailabilityCheck::DecodeForwardedName(TCHAR* ForwardedName,OUT TCHAR* ForwardedDllName,OUT TCHAR* ForwardedFunctionName)
{
    // ForwardedName is like "NTDLL.RtlZeroMemory"
    TCHAR* psz;
    _tcscpy(ForwardedDllName,ForwardedName);

    // find '.'
    psz=_tcschr(ForwardedDllName,'.');
    if(!psz)
        return FALSE;

    // point to function name
    psz++;
    _tcscpy(ForwardedFunctionName,psz);

    // ends dll name now it's like "NTDLL."
    *psz=0;
    // add "dll" to ForwardedDll
    _tcscat(ForwardedDllName,_T("dll"));

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CheckExportTable
// Object:  check export table and optionally first functions bytes
// Parameters :
//     in  : TCHAR* WorkingDirectory : directory in which reside analysed binary
//           TCHAR* DllName : name of dll to analyze
//           BOOL bCheckAllFunctionsFirstBytes : if TRUE to do a first bytes analysis for each function
//                                               if FALSE, the FunctionFirstByteCheckResult of all items of pListCheckExportTableResult
//                                                       has no meaning (set to 0)
//                                               This allow to earn some times if you just need to do a first bytes analysis for 1 or 2 functions 
//                                               in this case do a call with FALSE and next do as necessary call to FullCheck as requiered
//     inout : CLinkList* pListCheckExportTableResult : linklist of CHECK_EXPORT_TABLE_RESULT structures
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CHookAvailabilityCheck::CheckExportTable(TCHAR* WorkingDirectory,TCHAR* DllName,CLinkList* pListCheckExportTableResult,BOOL bCheckAllFunctionsFirstBytes)
{
    TCHAR pszFullPath[MAX_PATH];
    if (!CDllFinder::FindDll(WorkingDirectory,DllName,pszFullPath))
        return FALSE;

    // parse pe
    CPE* pPe = CHookAvailabilityCheck::GetPe(pszFullPath);
    if (!pPe)
        return FALSE;

    return CheckExportTable(WorkingDirectory,pPe,pListCheckExportTableResult,bCheckAllFunctionsFirstBytes);
}

//-----------------------------------------------------------------------------
// Name: CheckExportTable
// Object:  check export table and optionally first functions bytes
// Parameters :
//     in  : TCHAR* WorkingDirectory : directory in which reside analysed binary
//           CPE* pPe : Pointer to a PE object
//           BOOL bCheckAllFunctionsFirstBytes : if TRUE to do a first bytes analysis for each function
//                                               if FALSE, the FunctionFirstByteCheckResult of all items of pListCheckExportTableResult
//                                                       has no meaning (set to 0)
//                                               This allow to earn some times if you just need to do a first bytes analysis for 1 or 2 functions 
//                                               in this case do a call with FALSE and next do as necessary call to FullCheck as requiered
//     inout : CLinkList* pListCheckExportTableResult : linklist of CHECK_EXPORT_TABLE_RESULT structures
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CHookAvailabilityCheck::CheckExportTable(TCHAR* WorkingDirectory,CPE* pPe,CLinkList* pListCheckExportTableResult,BOOL bCheckAllFunctionsFirstBytes)
{
    CLinkListItem* pItemPE;
    CPE::EXPORT_FUNCTION_ITEM* pExportFunc;
    CPE::EXPORT_FUNCTION_ITEM* pExportFunc2;
    CHECK_EXPORT_TABLE_RESULT* pExportedFuncCheckResult;
    CHECK_EXPORT_TABLE_RESULT ExportedFuncCheckResult;
    LIBRARY_EXPORT_LIST* pLibraryExportListData=NULL;
    CLinkListItem* pForwardedLibraryListItem;
    CLinkListItem* pItem; 
    CLinkListItem* pItem2;
    BOOL bFound;
    TCHAR ForwardedDllName[MAX_PATH];
    TCHAR ForwardedFunctionName[MAX_PATH];
    CLinkList ForwardedLibraryList(sizeof(LIBRARY_EXPORT_LIST));
    LIBRARY_EXPORT_LIST ForwardedList;
    CLinkList OrderedExportList(sizeof(CPE::EXPORT_FUNCTION_ITEM));
    BOOL bInserted;
    TCHAR FullDllPath[MAX_PATH];
    TCHAR CurrentDllName[MAX_PATH];
    TCHAR* CurrentDllShortName;

    if(pPe->pExportTable==NULL)
        return FALSE;

    pPe->GetFileName(CurrentDllName);
    CurrentDllShortName=CStdFileOperations::GetFileName(CurrentDllName);

    // for each exported func
    pPe->pExportTable->Lock();
    for (pItemPE=pPe->pExportTable->Head;pItemPE;pItemPE=pItemPE->NextItem)
    {
        // get the EXPORT_FUNCTION_ITEM struct from pe list
        pExportFunc=(CPE::EXPORT_FUNCTION_ITEM*)pItemPE->ItemData;

        /////////////////////////
        // export table checking 
        /////////////////////////

        // if exported function is forwarded
        if (pExportFunc->Forwarded)
        {
            // forwarded function can be hooked and GetProcAddress will return address of the lower level function address
            // ex : kernel32 forwarded func EnterCriticalSection = ntdll.RtlEnterCriticalSection
            // EnterCriticalSection of kernel32 and RtlEnterCriticalSection of ntdll are the same function
            // EnterCriticalSection points to RtlEnterCriticalSection of the ntdll.dll memory space

            // translate forwarded name to dll name + function name
            if (!CHookAvailabilityCheck::DecodeForwardedName(pExportFunc->ForwardedName,ForwardedDllName,ForwardedFunctionName))
                goto EndForwarded;

            if (!CHookAvailabilityCheck::GetLastChildForwardedName(CurrentDllName,WorkingDirectory,CurrentDllName,ForwardedDllName,ForwardedFunctionName))
                goto EndForwarded;

   
            // reset vars to default
            ExportedFuncCheckResult.ExportTableCheckedOk=TRUE;
            memset(&ExportedFuncCheckResult.FunctionFirstByteCheckResult,0,sizeof(FUNCTION_CHECK_RESULT));
            ExportedFuncCheckResult.HasAnotherFunctionAlreadyTheSameAddress=FALSE;
            ExportedFuncCheckResult.HasAnotherFunctionTheSameAddress=FALSE;
            *ExportedFuncCheckResult.FirstFunctionWithSameAddressName=0;
            ExportedFuncCheckResult.RVA=0;
            _tcscpy(ExportedFuncCheckResult.FunctionName,pExportFunc->FunctionName);
            ExportedFuncCheckResult.ForwardedFunction=TRUE;
            _tcscpy(ExportedFuncCheckResult.ForwardedFunctionName,pExportFunc->ForwardedName);

            // if forwarded dll is the current one --> 2 functions are pointing at the same dll address
            // marque current function as not hookable
            if (_tcsicmp(ForwardedDllName,CurrentDllShortName)==0)
            {
                ExportedFuncCheckResult.ExportTableCheckedOk=FALSE;
                ExportedFuncCheckResult.HasAnotherFunctionAlreadyTheSameAddress=TRUE;

                // add ExportedFuncCheckResult to list pListCheckExportTableResult
                pListCheckExportTableResult->AddItem(&ExportedFuncCheckResult);
                continue;
            }

            // check if library has already been parsed by doing a loop in ForwardedLibraryList
            bFound=FALSE;
            for(pForwardedLibraryListItem=ForwardedLibraryList.Head;pForwardedLibraryListItem;pForwardedLibraryListItem=pForwardedLibraryListItem->NextItem)
            {
                pLibraryExportListData=(LIBRARY_EXPORT_LIST*)pForwardedLibraryListItem->ItemData;
                // if name of library found
                if (_tcscmp(ForwardedDllName,pLibraryExportListData->LibraryName)==0)
                {
                    // pLibraryExportListData points to the correct List --> nothing to do
                    // item has been found
                    bFound=TRUE;
                    break;
                }
            }

            // if not found in forwarded list, we have to parse the export table of the forwarded dll
            if (!bFound)
            {
                CDllFinder::FindDll(CurrentDllName,WorkingDirectory,ForwardedDllName,FullDllPath);
                // create an CheckExportTableResult list
                ForwardedList.pListCheckExportTableResult=new CLinkList(sizeof(CHECK_EXPORT_TABLE_RESULT));
                ForwardedList.pPe= new CPE(FullDllPath);// can be cached by caller --> don't use local cache
                if (!ForwardedList.pPe)
                    goto EndForwarded;
                if (!ForwardedList.pPe->Parse(TRUE,FALSE,FALSE))
                    goto EndForwarded;

                // get export result
                if (CheckExportTable(WorkingDirectory,
                                    ForwardedList.pPe,
                                    ForwardedList.pListCheckExportTableResult,
                                    FALSE
                                    )
                    )
                {
                    // item is ok
                    bFound=TRUE;
                    // fill forwarded dll name of the struct
                    _tcscpy(ForwardedList.LibraryName,ForwardedDllName);
                    // add CheckExportTableResult list into ForwardedLibraryList
                    ForwardedLibraryList.AddItem(&ForwardedList);
                    // fill pLibraryExportListData var
                    pLibraryExportListData=&ForwardedList;
                }

                else
                {
                    // on error free allocated memory
                    delete ForwardedList.pListCheckExportTableResult;
                }
            }
            if(!bFound)
                goto EndForwarded;

            // store result in ExportedFuncCheckResult
            pLibraryExportListData->pListCheckExportTableResult->Lock();
            for (pItem=pLibraryExportListData->pListCheckExportTableResult->Head;pItem;pItem=pItem->NextItem)
            {
                // search function result in result list
                pExportedFuncCheckResult=(CHECK_EXPORT_TABLE_RESULT*)pItem->ItemData;
                if (_tcscmp(pExportedFuncCheckResult->FunctionName,ForwardedFunctionName)==0)
                {
                    // get the Check export result
                    ExportedFuncCheckResult.ExportTableCheckedOk=pExportedFuncCheckResult->ExportTableCheckedOk;
                    // copy the first bytes analysis result
                    memcpy(&ExportedFuncCheckResult.FunctionFirstByteCheckResult,&pExportedFuncCheckResult->FunctionFirstByteCheckResult,sizeof(FUNCTION_CHECK_RESULT));
                    
                    // do first byte checking if required (only after having stored the export result) 
                    if (bCheckAllFunctionsFirstBytes)
                        // do first byte check of exported function
                        FullCheck(WorkingDirectory,pLibraryExportListData->pPe,ForwardedFunctionName,0,FALSE,&ExportedFuncCheckResult.FunctionFirstByteCheckResult);

                    break;
                }
            }
            pLibraryExportListData->pListCheckExportTableResult->Unlock();

EndForwarded:

            // change fields which have not the same meaning for imported function 
            // put HasAnotherFunctionAlreadyTheSameAddress to FALSE, because functions with same address may are not be imported
            ExportedFuncCheckResult.HasAnotherFunctionAlreadyTheSameAddress=FALSE;

            // add ExportedFuncCheckResult to list pListCheckExportTableResult
            pListCheckExportTableResult->AddItem(&ExportedFuncCheckResult);
            continue;
        }

        //else not forwarded function

        // order items according to their RVA
        bInserted=FALSE;
        // for each item in OrderedExportList
        for (pItem=OrderedExportList.Head;pItem;pItem=pItem->NextItem)
        {
            // get the EXPORT_FUNCTION_ITEM struct from list
            pExportFunc2=(CPE::EXPORT_FUNCTION_ITEM*)pItem->ItemData;

            if (pExportFunc->FunctionAddressRVA<pExportFunc2->FunctionAddressRVA)
            {
                OrderedExportList.InsertItem(pItem->PreviousItem,pExportFunc);
                bInserted=TRUE;
                break;
            }
        }
        if (!bInserted)
            OrderedExportList.AddItem(pExportFunc);

    }
    pPe->pExportTable->Unlock();

    // free memory required by forwarded functions
    
    for (pItem=ForwardedLibraryList.Head;pItem;pItem=pItem->NextItem)
    {
        delete ((LIBRARY_EXPORT_LIST*)pItem->ItemData)->pListCheckExportTableResult;
        delete ((LIBRARY_EXPORT_LIST*)pItem->ItemData)->pPe;
    }


    // since items are ordered
    // walk the list of non forwarded items
    for (pItem=OrderedExportList.Head;pItem;pItem=pItem->NextItem)
    {
        // get export info for current item
        pExportFunc=(CPE::EXPORT_FUNCTION_ITEM*)pItem->ItemData;

        // reset vars to default
        ExportedFuncCheckResult.ExportTableCheckedOk=TRUE;
        memset(&ExportedFuncCheckResult.FunctionFirstByteCheckResult,0,sizeof(FUNCTION_CHECK_RESULT));
        ExportedFuncCheckResult.HasAnotherFunctionAlreadyTheSameAddress=FALSE;
        ExportedFuncCheckResult.HasAnotherFunctionTheSameAddress=FALSE;
        ExportedFuncCheckResult.RVA=pExportFunc->FunctionAddressRVA;
        *ExportedFuncCheckResult.FirstFunctionWithSameAddressName=0;
        _tcscpy(ExportedFuncCheckResult.FunctionName,pExportFunc->FunctionName);
        ExportedFuncCheckResult.ForwardedFunction=FALSE;
        *ExportedFuncCheckResult.ForwardedFunctionName=0;

        // do first byte checking if required
        if (bCheckAllFunctionsFirstBytes)
        {
            // do first byte check of exported function
            FullCheck(WorkingDirectory,pPe,pExportFunc,&ExportedFuncCheckResult.FunctionFirstByteCheckResult);
        }

        // if not first item
        if (pItem!=OrderedExportList.Head)
        {
            // get export info for previous item
            pExportFunc2=(CPE::EXPORT_FUNCTION_ITEM*)pItem->PreviousItem->ItemData;

            // compare RVA to previous item
            if (pExportFunc2->FunctionAddressRVA==pExportFunc->FunctionAddressRVA)
            {
                ExportedFuncCheckResult.HasAnotherFunctionAlreadyTheSameAddress=TRUE;
                ExportedFuncCheckResult.HasAnotherFunctionTheSameAddress=TRUE;
                // find first function having the same RVA
                
                for(pItem2=pItem->PreviousItem->PreviousItem;pItem2;pItem2=pItem2->PreviousItem)
                {
                    pExportFunc2=(CPE::EXPORT_FUNCTION_ITEM*)pItem2->ItemData;
                    // if item has a different address, the next item is the good one
                    if (pExportFunc2->FunctionAddressRVA!=pExportFunc->FunctionAddressRVA)
                    {
                        // get next item (the last matching one)
                        pItem2=pItem2->NextItem;
                        // get pExportFunc2 of the last matching item
                        pExportFunc2=(CPE::EXPORT_FUNCTION_ITEM*)pItem2->ItemData;
                        break;
                    }
                }// if pItem2 is null that means first function has the same address --> pExportFunc2 has the correct value
                _tcscpy(ExportedFuncCheckResult.FirstFunctionWithSameAddressName,pExportFunc2->FunctionName);
            }
        }

        // if last item : no need to compare it to the next item --> only add it
        if (pItem==OrderedExportList.Tail)
        {
            // add ExportedFuncCheckResult to list pListCheckExportTableResult
            pListCheckExportTableResult->AddItem(&ExportedFuncCheckResult);
            break;
        }
        // else

        // get export info for next item
        pExportFunc2=(CPE::EXPORT_FUNCTION_ITEM*)pItem->NextItem->ItemData;

        // compare current RVA to the next item RVA
        // if RVA is the same as the current one
        if (pExportFunc2->FunctionAddressRVA==pExportFunc->FunctionAddressRVA)
        {
            // update the HasAnotherFunctionTheSameAddress field
            ExportedFuncCheckResult.HasAnotherFunctionTheSameAddress=TRUE;
            _tcscpy(ExportedFuncCheckResult.FirstFunctionWithSameAddressName,pExportFunc2->FunctionName);
        }
        // if nextRVA-currentRVA<HOOK_SIZE --> hook can't be install
        else if (pExportFunc2->FunctionAddressRVA-pExportFunc->FunctionAddressRVA<HOOK_SIZE)
        {
            // update the ExportTableCheckedOk field
            ExportedFuncCheckResult.ExportTableCheckedOk=FALSE;

            // find all functions having the same RVA
            if (pItem!=OrderedExportList.Head)
            {
                // find all previous func having the same RVA as the current one to update their hook state to false
                pExportFunc2=(CPE::EXPORT_FUNCTION_ITEM*)pItem->PreviousItem->ItemData;
                if (pExportFunc2->FunctionAddressRVA==pExportFunc->FunctionAddressRVA)
                {
                    // another func has the same address
                    // backward walk the list of result to change hook state for same RVA funcs
                    for (pItem2=pItem->PreviousItem;pItem2;pItem2=pItem2->PreviousItem)
                    {
                        pExportedFuncCheckResult=(CHECK_EXPORT_TABLE_RESULT*)pItem2->ItemData;
                        // if RVA is the same, update the ExportTableCheckedOk member to FALSE
                        if (pExportedFuncCheckResult->RVA==pExportFunc->FunctionAddressRVA)
                            pExportedFuncCheckResult->ExportTableCheckedOk=FALSE;
                        else
                            break;
                    }
                }
            }
        }

        // add item to pListCheckExportTableResult list
        pListCheckExportTableResult->AddItem(&ExportedFuncCheckResult);
    }

    return TRUE;
}

// ForwardedDllName and ForwardedFunctionName must be at least MAX_PATH length
BOOL CHookAvailabilityCheck::GetLastChildForwardedName(IN TCHAR* ImportingModulePath,IN TCHAR* WorkingDirectory,IN TCHAR* CurrentDllName,IN OUT TCHAR* ForwardedDllName,IN OUT TCHAR* ForwardedFunctionName)
{

    CDllStub* pDllStub = CHookAvailabilityCheck::GetDllStub();
    if (pDllStub->IsStubDll(ForwardedDllName))
    {
        TCHAR RealDllName[MAX_PATH];
        if (!pDllStub->GetRealModuleNameFromStubName(CurrentDllName,ForwardedDllName,RealDllName) )
            return FALSE;
        _tcscpy(ForwardedDllName,RealDllName);
    }

    TCHAR pszFullPath[MAX_PATH];
    if (!CDllFinder::FindDll(ImportingModulePath,WorkingDirectory,ForwardedDllName,pszFullPath))
        return FALSE;

    CLinkListItem* pItemPE;
    CPE::EXPORT_FUNCTION_ITEM* pExportFunc;
    CPE* pPe= CHookAvailabilityCheck::GetPe(pszFullPath);
    if (!pPe)
        return FALSE;

    for (pItemPE=pPe->pExportTable->Head;pItemPE;pItemPE=pItemPE->NextItem)
    {
        // get the EXPORT_FUNCTION_ITEM struct from pe list
        pExportFunc=(CPE::EXPORT_FUNCTION_ITEM*)pItemPE->ItemData;

        // if function has been found in forwarded dll export table
        if (_tcscmp(pExportFunc->FunctionName,ForwardedFunctionName)==0)
        {
            // if forwarded function is forwarded too
            if (pExportFunc->Forwarded)
            {
                TCHAR NewForwardedDllName[MAX_PATH];
                if (!CHookAvailabilityCheck::DecodeForwardedName(pExportFunc->ForwardedName,NewForwardedDllName,ForwardedFunctionName))
                    return FALSE;
                // do recursive call
                return CHookAvailabilityCheck::GetLastChildForwardedName(pszFullPath,WorkingDirectory,pszFullPath,NewForwardedDllName,ForwardedFunctionName);
            }
            // else
            return TRUE;
        }
    }
    return FALSE;
}