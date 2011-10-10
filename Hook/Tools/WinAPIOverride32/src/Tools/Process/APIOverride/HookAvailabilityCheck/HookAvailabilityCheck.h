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


#pragma once

#include "../Injected_dll/defines.h"
#include "../../../disasm/LengthDisasm/LengthDisasm32.h"
#include "../../../Dll/DllFinder.h"
#include "../../../PE/PE.h"
#include "../../../File/StdFileOperations.h"
#include "../../../Dll/DllStub.h"
#include "../../../LinkList/LinkListSimple.h"

#define HOOK_SIZE                   OPCODE_REPLACEMENT_SIZE
#define MAX_FIRST_BYTES_SEQUENCE    20

class CHookAvailabilityCheck
{
private:
    
    typedef struct tagFirstBytesSequence 
    {
        int CompareSize; // length of size to compare (length of Data)
        int MovableSize; // number of bytes we have to execute at another place
        BYTE Data[MAX_FIRST_BYTES_SEQUENCE]; // matching bytes
    }FIRST_BYTES_SEQUENCE,*PFIRST_BYTES_SEQUENCE;
    static BOOL CheckForAlign(PBYTE Buffer,int NbBytesToCheck);
    static CDllStub* pDllStub;

    static CLinkListSimple* pParsedPe;// list of export table parsed CPE objects
    static CLinkListSimple* GetParsedPe()
    {
        if (!CHookAvailabilityCheck::pParsedPe)
        {
            CHookAvailabilityCheck::pParsedPe = new CLinkListSimple();
        }
        return CHookAvailabilityCheck::pParsedPe;
    }
    static CPE* GetPe(TCHAR* FullPath);
public:
    static CDllStub* GetDllStub() // public only to provide external object to access it and having only one object in memory (quite dirty for object oriented soft)
    {
        if (!CHookAvailabilityCheck::pDllStub)
        {
            CHookAvailabilityCheck::pDllStub = new CDllStub();
        }
        return CHookAvailabilityCheck::pDllStub;
    }

    static void FreeStaticMembers()
    {
        if (CHookAvailabilityCheck::pDllStub)
        {
            delete CHookAvailabilityCheck::pDllStub;
            CHookAvailabilityCheck::pDllStub = NULL;
        }
        if (CHookAvailabilityCheck::pParsedPe)
        {
            CLinkListItem* pItem;
            CPE* Pe;
            CHookAvailabilityCheck::pParsedPe->Lock();
            for (pItem = CHookAvailabilityCheck::pParsedPe->Head;pItem;pItem=pItem->NextItem)
            {
                Pe = (CPE*)pItem->ItemData;
                delete Pe;
            }
            CHookAvailabilityCheck::pParsedPe->Unlock();
            delete CHookAvailabilityCheck::pParsedPe;
            CHookAvailabilityCheck::pParsedPe = NULL;
        }
    }
    enum IsFunctionHookableResult
    {
        IS_FUNCTION_HOOKABLE_RESULT_MAY_HOOKABLE,
        IS_FUNCTION_HOOKABLE_RESULT_NOT_HOOKABLE,
        IS_FUNCTION_HOOKABLE_RESULT_MAY_NOT_HOOKABLE
    };

    enum EnumCanFirstBytesBeExecutedAnyWhereResult
    {
        CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_YES,
        CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_MAY,
        CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_NO,
        CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_YES_NEED_RELATIVE_ADDRESS_CHANGES, // only if address is the same size of the CPU register (32bit for 32bits computers)
                                                                                       // this allows to hook statically functions beginning with jmp RelativeAddressOfAnotherFunction
        CAN_FIRST_BYTES_BE_EXECUTED_ANYWHERE_RESULT_MAY_NEED_RELATIVE_ADDRESS_CHANGES
    };

    typedef struct tagSTRUCT_FIRST_BYTES_CAN_BE_EXECUTED_ANYWHERE_RESULT
    {
        EnumCanFirstBytesBeExecutedAnyWhereResult FirstBytesCanBeExecutedAnyWhereResult;
        DWORD NbBytesToExecuteAtAnotherPlace;// number of bytes that must be executed at another place 
        // We only can hook relative jump or call with a relative address size equal to register size
        //    (that mean jump to relative 32 bits addresses on 32 bit computers)
        //    The relative instruction length for relative 32 jump or call is 1 or 2 bytes length
        //
        // For our hook we need 1 byte (jmp or call) + address of our hook --> 8 bits + 32bits
        //
        // So after having found such instruction, we know we can execute first bytes anywhere
        // modifying address at NbBytesToExecuteAtAnotherPlace - sizeof(PBYTE)
        // 
        // In other word we always get
        // DWORD PositionOfRelativeAddressToChange_RelativeFromFunctionStart=NbBytesToExecuteAtAnotherPlace-sizeof(PBYTE)
    }STRUCT_FIRST_BYTES_CAN_BE_EXECUTED_ANYWHERE_RESULT,*PSTRUCT_FIRST_BYTES_CAN_BE_EXECUTED_ANYWHERE_RESULT;

    typedef struct tagFUNCTION_CHECK_RESULT
    {
        IsFunctionHookableResult CheckResult;
        STRUCT_FIRST_BYTES_CAN_BE_EXECUTED_ANYWHERE_RESULT FirstBytesCanBeExecutedAnyWhereResult;
        BOOL FunctionInExecutableSection;
    }FUNCTION_CHECK_RESULT,*PFUNCTION_CHECK_RESULT;

    typedef struct tagCHECK_EXPORT_TABLE_RESULT 
    {
        TCHAR FunctionName[MAX_PATH]; // name of the function
        BOOL ExportTableCheckedOk;    // TRUE if the length of the function is enough (according to the export table only)
        BOOL HasAnotherFunctionAlreadyTheSameAddress; // FALSE if no previous function in the export table has the same address
                                                      // this value can be FALSE even if a next function in the export table has the same address
        BOOL HasAnotherFunctionTheSameAddress;// FALSE if no other function has the same address
        TCHAR FirstFunctionWithSameAddressName[MAX_PATH]; // name of the first function having the same RVA
        FUNCTION_CHECK_RESULT FunctionFirstByteCheckResult;
        ULONGLONG RVA;
        BOOL ForwardedFunction;
        TCHAR ForwardedFunctionName[MAX_PATH];
    }CHECK_EXPORT_TABLE_RESULT,*PCHECK_EXPORT_TABLE_RESULT;

    typedef struct tagLIBRARY_EXPORT_LIST
    {
        TCHAR LibraryName[MAX_PATH];
        CPE* pPe;
        CLinkList* pListCheckExportTableResult;
    }LIBRARY_EXPORT_LIST,*PLIBRARY_EXPORT_LIST;
    /*
    CHookAvailabilityCheck(void);
    ~CHookAvailabilityCheck(void);
    */

    static BOOL DecodeForwardedName(TCHAR* ForwardedName,OUT TCHAR* ForwardedDllName,OUT TCHAR* ForwardedFunctionName);
    static BOOL CanFirstBytesBeExecutedAnyWhere(HANDLE hFile,ULONGLONG FunctionRawAddress,OUT STRUCT_FIRST_BYTES_CAN_BE_EXECUTED_ANYWHERE_RESULT* pCanFirstBytesBeExecutedAnyWhereResult);
    static BOOL CanFirstBytesBeExecutedAnyWhere(BYTE* pFuncStart,OUT STRUCT_FIRST_BYTES_CAN_BE_EXECUTED_ANYWHERE_RESULT* pCanFirstBytesBeExecutedAnyWhereResult);
    static BOOL IsFunctionHookable(HANDLE hFile,ULONGLONG FunctionRaw,OUT IsFunctionHookableResult* pCheckResult);
    static BOOL IsFunctionHookable(BYTE* pFuncStart,OUT IsFunctionHookableResult* pCheckResult);

    static BOOL FullCheck(TCHAR* WorkingDirectory,TCHAR* DllName,DWORD FunctionOrdinal,OUT FUNCTION_CHECK_RESULT* pFunctionCheckResult);
    static BOOL FullCheck(TCHAR* WorkingDirectory,TCHAR* DllName,TCHAR* FunctionName,OUT FUNCTION_CHECK_RESULT* pFunctionCheckResult);
    static BOOL FullCheck(TCHAR* WorkingDirectory,CPE* pPe,TCHAR* pszFunctionName,DWORD FunctionOrdinal,BOOL bOrdinal,OUT FUNCTION_CHECK_RESULT* pFunctionCheckResult);
    static BOOL FullCheck(TCHAR* WorkingDirectory,CPE* pPe,CPE::PEXPORT_FUNCTION_ITEM pExportFuncInfos,OUT FUNCTION_CHECK_RESULT* pFunctionCheckResult);
    static BOOL FullCheck(TCHAR* WorkingDirectory,
                        TCHAR* DllName,
                        TCHAR* FunctionName,
                        DWORD FunctionOrdinal,
                        BOOL bOrdinal,
                        OUT FUNCTION_CHECK_RESULT* pFunctionCheckResult
                        );
    static BOOL CheckExportTable(TCHAR* WorkingDirectory,TCHAR* DllName,CLinkList* pListCheckExportTableResult,BOOL bCheckAllFunctionsFirstBytes);
    static BOOL CheckExportTable(TCHAR* WorkingDirectory,CPE* pPe,CLinkList* pListCheckExportTableResult,BOOL bCheckAllFunctionsFirstBytes);
    static BOOL GetLastChildForwardedName(IN TCHAR* ImportingModulePath,IN TCHAR* WorkingDirectory,IN TCHAR* CurrentDllName,IN OUT TCHAR* ForwardedDllName,IN OUT TCHAR* ForwardedFunctionName);
};
