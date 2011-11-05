/*-----------------------------------------------------------------------------
   Copyright (c) 2000  Microsoft Corporation

Module:
  exts.cpp

  Sampe file showing couple of extension examples

-----------------------------------------------------------------------------*/

/*
 * Boxcounter: 编译前请确认设置了 dbgengine 的头文件路径和库文件路径
 */

#include "dbgexts.h"
#include "inc.h"
#include <strsafe.h>

#pragma comment(lib, "strsafe.lib")

#define ALIGN_DOWN_BY(length, alignment) \
    ((ULONG_PTR)(length) & ~(alignment - 1))


#define ALIGN_UP_BY(length, alignment) \
    (ALIGN_DOWN_BY(((ULONG_PTR)(length) + alignment - 1), alignment))

#define ALIGN_UP(length, type) \
    ALIGN_UP_BY(length, sizeof(type))


#define IS_ZERO_END(c)  ('\0'==c)
#define IS_SEPARATOR(c) ((' ' == c) || ('\t' == c))


#define INVALID_TYPE_ID  (ULONG)-1

#define UNW_FLAG_NHANDLER 0x0
#define UNW_FLAG_EHANDLER 0x1
#define UNW_FLAG_UHANDLER 0x2
#define UNW_FLAG_CHAININFO 0x4



ULONG
    GetArgsCount (
        LPCSTR pszArgs
    )
{
    ULONG i = 0;
    LPCSTR ptr = NULL;
    LPCSTR pszHead = NULL;

    pszHead = pszArgs;

    while (TRUE)
    {
        // 跳过连续的空白符
        while (IS_SEPARATOR(*pszHead))
        {
            pszHead++;
        }

        // 如果是'\0'就结束
        if (IS_ZERO_END(*pszHead))
        {
            break;
        }

        ptr = pszHead;

        // 非空白符
        do
        {
            ptr++;
        }
        while (!IS_SEPARATOR(*ptr) && !IS_ZERO_END(*ptr));
        
        i++;

        // 结束符
        if (IS_ZERO_END(*ptr))
        {
            break;
        }

        pszHead = ptr + 1;
    }

    return i;
}

LPSTR*
    ParseArgs (
        LPCSTR pszArgs,
        ULONG *pulArraySize
    )
{
    ULONG i = 0;
    LPCSTR ptr = NULL;
    LPCSTR pszHead = NULL;
    ULONG cchSize = 0;
    LPSTR *ArgArray = NULL;
    ULONG ulArraySize = 0;

    ulArraySize = GetArgsCount(pszArgs);

    ArgArray = (LPSTR*)MemAlloc(sizeof(LPSTR) * ulArraySize);
    if (NULL == ArgArray)
    {
        return NULL;
    }

    pszHead = pszArgs;

    while (TRUE)
    {
        // 跳过连续的空白符
        while (IS_SEPARATOR(*pszHead))
        {
            pszHead++;
        }

        // 如果是'\0'就结束
        if (IS_ZERO_END(*pszHead))
        {
            break;
        }

        ptr = pszHead;

        // 非空白符
        do
        {
            ptr++;
        }
        while (!IS_SEPARATOR(*ptr) && !IS_ZERO_END(*ptr));
        
        cchSize = ptr - pszHead + 1;
        ArgArray[i] = (LPSTR)MemAlloc(sizeof(char) * cchSize);
        StringCchCopyNA(ArgArray[i], cchSize, pszHead, ptr - pszHead);
        i++;
        ASSERT(i <= ulArraySize);

        // 结束符
        if (IS_ZERO_END(*ptr))
        {
            break;
        }

        pszHead = ptr + 1;
    }

    *pulArraySize = i;
    return ArgArray;
}


VOID
    FreeArgs (
        LPSTR *ArgArray,
        ULONG ulArraySize
    )
{
    ASSERT_RETURN(NULL != ArgArray);

    for (ULONG i = 0; i < ulArraySize; i++)
    {
        ASSERT_CONTINUE(NULL != ArgArray[i]);
        MemFree(ArgArray[i]);
    }

    return;
}


ULONG
    GetTypeSize (
        LPSTR lpszTypeName
    )
{
    HRESULT hResult = 0;
    ULONG ulTypeId = 0;
    ULONG64 ulModuleBase = 0;
    ULONG ulTypeSize = 0;

    ASSERT_RETURN_VAL((NULL != lpszTypeName), 0);
    ASSERT_RETURN_VAL((NULL != g_ExtSymbols), 0);

    hResult = g_ExtSymbols->GetSymbolTypeId(lpszTypeName, 
                                            &ulTypeId, 
                                            &ulModuleBase);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: IDebugSymbol::GetSymbolTypeId('%s') FAILED(%u).\n", __LINE__, __FUNCTION__, lpszTypeName, HRESULT_CODE(hResult));
        return 0;
    }


    hResult = g_ExtSymbols->GetTypeSize(ulModuleBase, ulTypeId, &ulTypeSize);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: IDebugSymbol::GetTypeSize('%s') FAILED(%u).\n", __LINE__, __FUNCTION__, lpszTypeName, HRESULT_CODE(hResult));
        return 0;
    }

    return ulTypeSize;
}


BOOL
    ReadFieldValue (
        ULONG64 *pulImageBase,
        ULONG   *pulTypeId,
        ULONG64 ulVarBase,
        LPSTR   lpszTypeName,
        LPSTR   lpszFieldName,
        PVOID   pValue,
        ULONG   cbValueSize,
        ULONG   *pcbRead
    )
{
    HRESULT hResult = S_FALSE;
    ULONG64 ulImageBase = 0;
    ULONG ulOffset = 0;
    ULONG ulTypeId = INVALID_TYPE_ID;

    ASSERT_RETURN_VAL(NULL != g_ExtSymbols, FALSE);
    ASSERT_RETURN_VAL(NULL != g_ExtDataSpace, FALSE);
    ASSERT_RETURN_VAL(NULL != ulVarBase, FALSE);
    ASSERT_RETURN_VAL(NULL != lpszFieldName, FALSE);
    ASSERT_RETURN_VAL(cbValueSize > 0, FALSE);

    if ((NULL == pulImageBase) || (0 == *pulImageBase) || 
        (NULL == pulTypeId) || (INVALID_TYPE_ID == *pulTypeId))
    {
        ASSERT_RETURN_VAL(NULL != lpszTypeName, FALSE);

        hResult = g_ExtSymbols->GetSymbolTypeId(lpszTypeName, 
                                                &ulTypeId, 
                                                &ulImageBase);
        if (FAILED(hResult))
        {
            dprintf("%4u %s: GetSymbolTypeId('%s') FAILED(%u).\n", __LINE__, __FUNCTION__, lpszTypeName, HRESULT_CODE(hResult));
            return FALSE;
        }
    }
    else
    {
        ulImageBase = *pulImageBase;
        ulTypeId = *pulTypeId;
    }

    //
    // 读取 UNWIND_INFO::CountOfCodes
    //
    hResult = g_ExtSymbols->GetFieldOffset(ulImageBase, 
                                           ulTypeId, 
                                           lpszFieldName, 
                                           &ulOffset);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: GetFieldOffset('%s') FAILED(%u).\n", __LINE__, __FUNCTION__, lpszFieldName, HRESULT_CODE(hResult));
        return FALSE;
    }

    hResult = g_ExtDataSpace->ReadVirtual(ulVarBase + ulOffset,
                                          pValue,
                                          cbValueSize,
                                          (NULL != pcbRead) ? pcbRead : NULL);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: ReadVirtual('%s') FAILED(%u).\n", __LINE__, __FUNCTION__, HRESULT_CODE(hResult));
        return FALSE;
    }

    if ((NULL != pulImageBase) && (0 == *pulImageBase))
    {
        *pulImageBase = ulImageBase;
    }

    if ((NULL != pulTypeId) && (INVALID_TYPE_ID == *pulTypeId))
    {
        *pulTypeId = ulTypeId;
    }

    return TRUE;
}


BOOL
    DecodeScopeRecord (
        ULONG64 ulNtKrnlBase,
        ULONG   ulScopeRecordTypeId,
        ULONG64 ulImageBase,
        ULONG64 ulScopeRecordAddr
    )
{
    HRESULT hResult = S_FALSE;
    ULONG ulSubFieldOffset = 0;
    ULONG ulValue = 0;

    //
    // SCOPE_TABLE::BeginAddress
    //
    if (!ReadFieldValue(&ulNtKrnlBase, 
                        &ulScopeRecordTypeId, 
                        ulScopeRecordAddr,
                        NULL,
                        "BeginAddress", 
                        &ulValue,
                        sizeof(ulValue),
                        NULL))
    {
        return FALSE;
    }

    dprintf("        BeginAddress: \n            ");
    hResult = g_ExtSymbols->OutputSymbolByOffset(DEBUG_OUTCTL_THIS_CLIENT,
            DEBUG_OUTSYM_SOURCE_LINE | DEBUG_OUTSYM_FORCE_OFFSET | DEBUG_OUTSYM_ALLOW_DISPLACEMENT,
            ulImageBase + ulValue);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: OutputSymbolByOffset FAILED(%u).\n", __LINE__, __FUNCTION__, HRESULT_CODE(hResult));
        return FALSE;
    }
    dprintf("\n");

    //
    // SCOPE_TABLE::EndAddress
    //
    if (!ReadFieldValue(&ulNtKrnlBase, 
                        &ulScopeRecordTypeId, 
                        ulScopeRecordAddr,
                        NULL,
                        "EndAddress", 
                        &ulValue,
                        sizeof(ulValue),
                        NULL))
    {
        return FALSE;
    }

    dprintf("        EndAddress: \n            ");
    hResult = g_ExtSymbols->OutputSymbolByOffset(DEBUG_OUTCTL_THIS_CLIENT,
            DEBUG_OUTSYM_SOURCE_LINE | DEBUG_OUTSYM_FORCE_OFFSET | DEBUG_OUTSYM_ALLOW_DISPLACEMENT,
            ulImageBase + ulValue);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: OutputSymbolByOffset FAILED(%u).\n", __LINE__, __FUNCTION__, HRESULT_CODE(hResult));
        return FALSE;
    }
    dprintf("\n");

    //
    // SCOPE_TABLE::HandlerAddress
    //
    if (!ReadFieldValue(&ulNtKrnlBase, 
                        &ulScopeRecordTypeId, 
                        ulScopeRecordAddr,
                        NULL,
                        "HandlerAddress", 
                        &ulValue,
                        sizeof(ulValue),
                        NULL))
    {
        return FALSE;
    }

    dprintf("        HandlerAddress: \n            ");
    if (EXCEPTION_EXECUTE_HANDLER == ulValue)
    {
        dprintf("EXCEPTION_EXECUTE_HANDLER");
    }
    else
    {
        hResult = g_ExtSymbols->OutputSymbolByOffset(DEBUG_OUTCTL_THIS_CLIENT,
                DEBUG_OUTSYM_SOURCE_LINE | DEBUG_OUTSYM_FORCE_OFFSET | DEBUG_OUTSYM_ALLOW_DISPLACEMENT,
                ulImageBase + ulValue);
        if (FAILED(hResult))
        {
            dprintf("%4u %s: OutputSymbolByOffset FAILED(%u).\n", __LINE__, __FUNCTION__, HRESULT_CODE(hResult));
            return FALSE;
        }
    }
    dprintf("\n");

    //
    // SCOPE_TABLE::JumpTarget
    //
    if (!ReadFieldValue(&ulNtKrnlBase, 
                        &ulScopeRecordTypeId, 
                        ulScopeRecordAddr,
                        NULL,
                        "JumpTarget", 
                        &ulValue,
                        sizeof(ulValue),
                        NULL))
    {
        return FALSE;
    }

    dprintf("        JumpTarget: \n            ");

    if (0 == ulValue)
    {
        dprintf("0");
    }
    else
    {
        hResult = g_ExtSymbols->OutputSymbolByOffset(DEBUG_OUTCTL_THIS_CLIENT,
                DEBUG_OUTSYM_SOURCE_LINE | DEBUG_OUTSYM_FORCE_OFFSET | DEBUG_OUTSYM_ALLOW_DISPLACEMENT,
                ulImageBase + ulValue);
        if (FAILED(hResult))
        {
            dprintf("%4u %s: OutputSymbolByOffset FAILED(%u).\n", __LINE__, __FUNCTION__, HRESULT_CODE(hResult));
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
    DecodeScopeTable (
        ULONG64 ulImageBase,
        ULONG64 ulScopeTableAddr
    )
{
    HRESULT hResult = S_FALSE;
    ULONG64 ulNtKrnlBase = 0;
    ULONG ulTypeId = INVALID_TYPE_ID;
    ULONG cbScopeRecordSize = 0;
    ULONG ulScopeRecordCount = 0;
    ULONG ulScopeRecordOffset = 0;
    ULONG ulScopeRecordTypeId = INVALID_TYPE_ID;

    //
	// typedef struct _SCOPE_TABLE {
	//     ULONG Count;
	//     struct
	//     {
	//         ULONG BeginAddress;
	//         ULONG EndAddress;
	//         ULONG HandlerAddress;
	//         ULONG JumpTarget;
	//     } ScopeRecord[1];
	// } SCOPE_TABLE, *PSCOPE_TABLE;
    //

    //
    // 获取 SCOPE_TABLE::Count
    //
    if (!ReadFieldValue(&ulNtKrnlBase, 
                        &ulTypeId, 
                        ulScopeTableAddr,
                        "_SCOPE_TABLE",
                        "Count", 
                        &ulScopeRecordCount,
                        sizeof(ulScopeRecordCount),
                        NULL))
    {
        return FALSE;
    }

    dprintf("ScopeTable:\n");
    dprintf("    Count: %u\n", ulScopeRecordCount);

    //
    // 获取 SCOPE_TABLE::ScopeRecord 的 typeid 和 offset
    //
    hResult = g_ExtSymbols->GetFieldTypeAndOffset(ulNtKrnlBase, ulTypeId, "ScopeRecord", &ulScopeRecordTypeId, &ulScopeRecordOffset);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: GetFieldTypeAndOffset FAILED(%u).\n", __LINE__, __FUNCTION__, HRESULT_CODE(hResult));
        return FALSE;
    }

    //
    // 获取 SCOPE_TABLE::ScopeRecord 的 size
    //
    hResult = g_ExtSymbols->GetTypeSize(ulNtKrnlBase, ulScopeRecordTypeId, &cbScopeRecordSize);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: GetTypeSize FAILED(%u).\n", __LINE__, __FUNCTION__, HRESULT_CODE(hResult));
        return FALSE;
    }

    ASSERT_RETURN_VAL(ulScopeRecordCount <= 30, FALSE);
    for (ULONG i = 0; i < ulScopeRecordCount; i++)
    {
        if (CheckControlC())
        {
            dprintf("\n\nrecieve 'Ctrl+C' \n");
            break;
        }

        dprintf("    ScopeRecord[%u]      (%p)\n", i, ulScopeTableAddr + ulScopeRecordOffset + i * cbScopeRecordSize);
        
        if (!DecodeScopeRecord(ulNtKrnlBase, 
                               ulScopeRecordTypeId, 
                               ulImageBase, 
                               ulScopeTableAddr + ulScopeRecordOffset + i * cbScopeRecordSize))
        {
            continue;
        }

        dprintf("\n");
    }

    return TRUE;
}


BOOL
    DecodeUnwindInfo (
        IN  ULONG64 ulImageBase,
        IN  ULONG64 ulUnwindInfoAddr
    )
{
    HRESULT hResult = S_FALSE;
    ULONG64 ulNtKrnlBase = 0;
    ULONG ulTypeId = INVALID_TYPE_ID;
    ULONG ulUnwindCodeOffset = 0;
    BYTE  ucCountOfCodes = 0;
    ULONG ulUnwindCodeSize = 0;
    ULONG ulExceptionRoutineRva = 0;
    ULONG64 ulExceptionRoutineAddr = 0;
    BYTE ucFlags = 0;
    ULONG64 ulExceptionDataAddr = 0;

    if (!ReadFieldValue(&ulNtKrnlBase, 
                        &ulTypeId, 
                        ulUnwindInfoAddr,
                        "_UNWIND_INFO",
                        "CountOfCodes", 
                        &ucCountOfCodes,
                        sizeof(ucCountOfCodes),
                        NULL))
    {
        return FALSE;
    }

    if (!ReadFieldValue(&ulNtKrnlBase, 
                        &ulTypeId, 
                        ulUnwindInfoAddr,
                        "_UNWIND_INFO",
                        "Flags",
                        &ucFlags,
                        sizeof(ucFlags),
                        NULL))
    {
        return FALSE;
    }

    ucFlags = ucFlags >> 3;

    //
    // 输出 UNWIND_INFO::Flags
    //
    dprintf("Flags:\n    ");

    if (UNW_FLAG_NHANDLER == ucFlags)
    {
        dprintf("N");
    }
    else if (UNW_FLAG_CHAININFO == ucFlags)
    {
        dprintf("C");
    }
    else
    {
        if (0 != (UNW_FLAG_EHANDLER & ucFlags))
        {
            dprintf("E");
        }
        
        if (0 != (UNW_FLAG_UHANDLER & ucFlags))
        {
            dprintf("U");
        }
    }

    dprintf("\n");

    if (0 == ((UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER) & ucFlags))
    {
        return TRUE;
    }

    //
    // 获取 
    //
    ulUnwindCodeSize = GetTypeSize("_UNWIND_CODE");
    if (0 == ulUnwindCodeSize)
    {
        return FALSE;
    }

    hResult = g_ExtSymbols->GetFieldOffset(ulNtKrnlBase, 
                                           ulTypeId, 
                                           "UnwindCode", 
                                           &ulUnwindCodeOffset);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: GetFieldOffset('UnwindCode') FAILED(%u).\n", __LINE__, __FUNCTION__, HRESULT_CODE(hResult));
        return FALSE;
    }

    //
    // 读取 UNWIND_INFO::ExceptionRoutine 的 RVA
    //
    hResult = g_ExtDataSpace->ReadVirtual(ulUnwindInfoAddr + ALIGN_UP(ulUnwindCodeOffset + ulUnwindCodeSize * ucCountOfCodes, sizeof(PVOID)),
                                          &ulExceptionRoutineRva,
                                          sizeof(ulExceptionRoutineRva),
                                          NULL);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: ReadVirtual FAILED(%u).\n", __LINE__, __FUNCTION__, HRESULT_CODE(hResult));
        return FALSE;
    }

    ulExceptionRoutineAddr = ulImageBase + ulExceptionRoutineRva; 
    ulExceptionDataAddr = ulUnwindInfoAddr + ALIGN_UP(ulUnwindCodeOffset + ulUnwindCodeSize * ucCountOfCodes, sizeof(PVOID)) + sizeof(ULONG);

    //
    // 输出 UNWIND_INFO::ExceptionRoutine 的符号信息
    //
    dprintf("ExceptionRoutine:\n    ");
    hResult = g_ExtSymbols->OutputSymbolByOffset(DEBUG_OUTCTL_THIS_CLIENT,
            DEBUG_OUTSYM_SOURCE_LINE | DEBUG_OUTSYM_FORCE_OFFSET | DEBUG_OUTSYM_ALLOW_DISPLACEMENT,
            ulImageBase + ulExceptionRoutineRva);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: OutputSymbolByOffset FAILED(%u).\n", __LINE__, __FUNCTION__, HRESULT_CODE(hResult));
        return FALSE;
    }
    dprintf("\n");

    if (!DecodeScopeTable(ulImageBase, ulExceptionDataAddr))
    {
        return FALSE;
    }

    return TRUE;
}



BOOL
    DecodeRuntimeFunction (
        ULONG64 ulImageBase, 
        ULONG64 ulRuntimeFunctionAddr
    )
{
    HRESULT hResult = S_FALSE;
    ULONG64 ulNtKrnlBase = 0;
    ULONG ulTypeId = INVALID_TYPE_ID;
    ULONG ulFieldValue = 0;

    if (!ReadFieldValue (&ulNtKrnlBase, 
                         &ulTypeId, 
                         ulRuntimeFunctionAddr,
                         "_RUNTIME_FUNCTION",
                         "BeginAddress",
                         &ulFieldValue,
                         sizeof(ulFieldValue),
                         NULL))
    {
        return FALSE;
    }

    dprintf("BeginAddress:\n    ");

    hResult = g_ExtSymbols->OutputSymbolByOffset(DEBUG_OUTCTL_THIS_CLIENT,
            DEBUG_OUTSYM_SOURCE_LINE | DEBUG_OUTSYM_FORCE_OFFSET | DEBUG_OUTSYM_ALLOW_DISPLACEMENT,
            ulImageBase + ulFieldValue);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: OutputSymbolByOffset FAILED(%u).\n", __LINE__, __FUNCTION__, HRESULT_CODE(hResult));
        return FALSE;
    }
    dprintf("\n");

    //
    // RUNTIME_FUNCTION::EndAddress
    //
    if (!ReadFieldValue (&ulNtKrnlBase, 
                         &ulTypeId, 
                         ulRuntimeFunctionAddr,
                         "_RUNTIME_FUNCTION",
                         "EndAddress",
                         &ulFieldValue,
                         sizeof(ulFieldValue),
                         NULL))
    {
        return FALSE;
    }

    dprintf("EndAddress:\n    ");

    hResult = g_ExtSymbols->OutputSymbolByOffset(DEBUG_OUTCTL_THIS_CLIENT,
            DEBUG_OUTSYM_SOURCE_LINE | DEBUG_OUTSYM_FORCE_OFFSET | DEBUG_OUTSYM_ALLOW_DISPLACEMENT,
            ulImageBase + ulFieldValue);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: OutputSymbolByOffset FAILED(%u).\n", __LINE__, __FUNCTION__, HRESULT_CODE(hResult));
        return FALSE;
    }
    dprintf("\n");

    //
    // RUNTIME_FUNCTION::UnwindData
    //
    if (!ReadFieldValue (&ulNtKrnlBase, 
                         &ulTypeId, 
                         ulRuntimeFunctionAddr,
                         "_RUNTIME_FUNCTION",
                         "UnwindData",
                         &ulFieldValue,
                         sizeof(ulFieldValue),
                         NULL))
    {
        return FALSE;
    }

    dprintf("UnwindData:\n    %p\n", ulImageBase + ulFieldValue);

    dprintf("_UNWIND_INFO for %p \n", ulImageBase + ulFieldValue);
    if (!DecodeUnwindInfo(ulImageBase, ulImageBase + ulFieldValue))
    {
        return FALSE;
    }

    return TRUE;
}


ULONG64
    ParseAddressParam (
        LPSTR lpszAddr
    )
{
    if ('@' == lpszAddr[0])
    {
        HRESULT hResult = S_FALSE;
        ULONG ulRegIndex = 0;
        DEBUG_VALUE  Value = {0};

        hResult = g_ExtRegister->GetIndexByName(&lpszAddr[1], &ulRegIndex);
        if (FAILED(hResult))
        {
            dprintf("%4u %s: GetIndexByName('%s') FAILED(%u).\n", __LINE__, __FUNCTION__, &lpszAddr[1], HRESULT_CODE(hResult));
            return 0;
        }
        
        hResult = g_ExtRegister->GetValue(ulRegIndex, &Value);
        if (FAILED(hResult))
        {
            dprintf("%4u %s: GetValue('%s') FAILED(%u).\n", __LINE__, __FUNCTION__, &lpszAddr[1], HRESULT_CODE(hResult));
            return 0;
        }

        if (DEBUG_VALUE_INT64 != Value.Type)
        {
            dprintf("%4u %s: Wrong register type - %u \n", __LINE__, __FUNCTION__, Value.Type);
            return 0;
        }

        return Value.I64;
    }
        
    return GetExpression(lpszAddr);
}


HRESULT 
CALLBACK
    scoperecord(
        PDEBUG_CLIENT4 Client, 
        PCSTR pArgs
    )
{
    HRESULT hResult = S_FALSE;
    LPSTR *ArgArray = NULL;
    ULONG ulArraySize = 0;
    ULONG64 ulImageBase = 0;
    ULONG64 ulNtKrnlBase = 0;
    ULONG64 ulScopeRecordAddr = 0;
    ULONG ulScopeTableTypeId = 0;
    ULONG ulScopeRecordTypeId = 0;

    INIT_API();

    if (NULL == pArgs)
    {
        dprintf("%4u %s: Wrong arguments!\n", __LINE__, __FUNCTION__);
        goto __CLEANUP;
    }

    ArgArray = ParseArgs(pArgs, &ulArraySize);
    if (NULL == ArgArray)
    {
        dprintf("%4u %s: ParseArg FAILED!\n", __LINE__, __FUNCTION__);
        goto __CLEANUP;
    }

    if (2 != ulArraySize)
    {
        dprintf("%4u %s: Wrong arguments!\n", __LINE__, __FUNCTION__);
        goto __CLEANUP;
    }

    ulScopeRecordAddr = ParseAddressParam(ArgArray[1]);
    if (0 == ulScopeRecordAddr)
    {
        goto __CLEANUP;
    }

    // 
    // 获取用户输入的 UNWIND_INFO 地址
    //

    dprintf("ScopeRecord for %p \n", ulScopeRecordAddr);

    // 
    // 获取用户输入的模块的地址
    //
    hResult = g_ExtSymbols->GetModuleByModuleName(ArgArray[0], 
                                                  0, 
                                                  NULL, 
                                                  &ulImageBase);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: GetModuleByModuleName('%s') FAILED(%u).\n", __LINE__, __FUNCTION__, ArgArray[0], HRESULT_CODE(hResult));
        goto __CLEANUP;
    }

    //
    // 获取
    hResult = g_ExtSymbols->GetSymbolTypeId("_SCOPE_TABLE", 
                                            &ulScopeTableTypeId, 
                                            &ulNtKrnlBase);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: GetSymbolTypeId('_SCOPE_TABLE') FAILED(%u).\n", __LINE__, __FUNCTION__, HRESULT_CODE(hResult));
        return FALSE;
    }

    //
    // 获取 SCOPE_TABLE::ScopeRecord 的 typeid 和 offset
    //
    hResult = g_ExtSymbols->GetFieldTypeAndOffset(ulNtKrnlBase, ulScopeTableTypeId, "ScopeRecord", &ulScopeRecordTypeId, NULL);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: GetFieldTypeAndOffset FAILED(%u).\n", __LINE__, __FUNCTION__, HRESULT_CODE(hResult));
        return FALSE;
    }

    if (!DecodeScopeRecord(ulNtKrnlBase, 
                           ulScopeRecordTypeId, 
                           ulImageBase, 
                           ulScopeRecordAddr))
    {
        return FALSE;
    }

__CLEANUP:

    FreeArgs(ArgArray, ulArraySize);

    EXIT_API();

    return hResult;
}


HRESULT 
CALLBACK
    unwindinfo(
        PDEBUG_CLIENT4 Client, 
        PCSTR pArgs
    )
{
    HRESULT hResult = S_OK;
    LPSTR *ArgArray = NULL;
    ULONG ulArraySize = 0;
    BYTE ucFlags = 0;
    ULONG64 ulUnwindInfoAddr = 0;
    ULONG64 ulImageBase = NULL;
    ULONG64 ulExceptionRoutineAddr = NULL;
    ULONG64 ulExceptionDataAddr = NULL;
    
    INIT_API();

    if (NULL == pArgs)
    {
        dprintf("%4u %s: Wrong arguments!\n", __LINE__, __FUNCTION__);
        goto __CLEANUP;
    }

    ArgArray = ParseArgs(pArgs, &ulArraySize);
    if (NULL == ArgArray)
    {
        dprintf("%4u %s: ParseArg FAILED!\n", __LINE__, __FUNCTION__);
        goto __CLEANUP;
    }
    
    if (2 != ulArraySize)
    {
        dprintf("%4u %s: Wrong arguments!\n", __LINE__, __FUNCTION__);
    }

    // 
    // 获取用户输入的 UNWIND_INFO 地址
    //
    ulUnwindInfoAddr = ParseAddressParam(ArgArray[1]);
    if (0 == ulUnwindInfoAddr)
    {
        dprintf("%4u %s: GetExpression('%s') FAILED(%u).\n", __LINE__, __FUNCTION__, ArgArray[1], HRESULT_CODE(hResult));
        goto __CLEANUP;
    }
    
    dprintf("_UNWIND_INFO for %p \n", ulUnwindInfoAddr);

    // 
    // 获取用户输入的模块的地址
    //
    hResult = g_ExtSymbols->GetModuleByModuleName(ArgArray[0], 
                                                  0, 
                                                  NULL, 
                                                  &ulImageBase);
    if (FAILED(hResult))
    {
        dprintf("%4u %s: GetModuleByModuleName('%s') FAILED(%u).\n", __LINE__, __FUNCTION__, ArgArray[0], HRESULT_CODE(hResult));
        goto __CLEANUP;
    }
    //dprintf("%4u %s: Base of '%s' is %p \n", __LINE__, __FUNCTION__, ArgArray[0], ulImageBase);

    if (!DecodeUnwindInfo(ulImageBase, ulUnwindInfoAddr/*, &ucFlags, &ulExceptionRoutineAddr, &ulExceptionDataAddr*/))
    {
        goto __CLEANUP;
    }

__CLEANUP:

    FreeArgs(ArgArray, ulArraySize);

    EXIT_API();

    return hResult;
}


HRESULT 
CALLBACK
    rtfn(
        PDEBUG_CLIENT4 Client, 
        PCSTR pArgs
    )
{
    HRESULT hResult = S_OK;
    LPSTR *ArgArray = NULL;
    ULONG ulArraySize = 0;
    ULONG64 ulImageBase = 0;
    ULONG64 ulRuntimeFunctionAddr = 0;

    INIT_API();

    if (NULL == pArgs)
    {
        dprintf("%4u %s: Wrong arguments!\n", __LINE__, __FUNCTION__);
        goto __CLEANUP;
    }

    ArgArray = ParseArgs(pArgs, &ulArraySize);
    if (NULL == ArgArray)
    {
        dprintf("%4u %s: ParseArg FAILED!\n", __LINE__, __FUNCTION__);
        goto __CLEANUP;
    }
    
    if (3 != ulArraySize)
    {
        dprintf("%4u %s: Wrong arguments!\n", __LINE__, __FUNCTION__);
        goto __CLEANUP;
    }

    if ('/' != ArgArray[0][0])
    {
        dprintf("%4u %s: Wrong option!\n", __LINE__, __FUNCTION__);
        goto __CLEANUP;
    }

    // 
    // 获取用户输入的 UNWIND_INFO 地址
    //
    ulRuntimeFunctionAddr = ParseAddressParam(ArgArray[2]);
    if (0 == ulRuntimeFunctionAddr)
    {
        dprintf("%4u %s: GetExpression('%s') FAILED(%u).\n", __LINE__, __FUNCTION__, ArgArray[2], HRESULT_CODE(hResult));
        goto __CLEANUP;
    }
    
    dprintf("_RUNTIME_FUNCTION for %p \n", ulRuntimeFunctionAddr);

    // 
    // 获取用户输入的模块的地址
    //
    if ('n' == ArgArray[0][1])
    {
        // 传入的是模块名
        hResult = g_ExtSymbols->GetModuleByModuleName(ArgArray[1], 
                                                      0, 
                                                      NULL, 
                                                      &ulImageBase);
        if (FAILED(hResult))
        {
            dprintf("%4u %s: GetModuleByModuleName('%s') FAILED(%u).\n", __LINE__, __FUNCTION__, ArgArray[1], HRESULT_CODE(hResult));
            goto __CLEANUP;
        }
    }
    else if ('a' == ArgArray[0][1])
    {
        // 传入的是地址
        ulImageBase = ParseAddressParam(ArgArray[1]);
        if (0 == ulImageBase)
        {
            dprintf("%4u %s: GetExpression('%s') FAILED(%u).\n", __LINE__, __FUNCTION__, ArgArray[1], HRESULT_CODE(hResult));
            goto __CLEANUP;
        }
    }
    else
    {
        dprintf("%4u %s: Unknown option: '%s'\n", __LINE__, __FUNCTION__, ArgArray[0]);
        goto __CLEANUP;
    }

    //dprintf("%4u %s: Base of '%s' is %p \n", __LINE__, __FUNCTION__, ArgArray[0], ulImageBase);

    if (!DecodeRuntimeFunction(ulImageBase, ulRuntimeFunctionAddr/*, &ucFlags, &ulExceptionRoutineAddr, &ulExceptionDataAddr*/))
    {
        goto __CLEANUP;
    }

__CLEANUP:

    FreeArgs(ArgArray, ulArraySize);

    EXIT_API();

    return hResult;
}

/*
   Sample extension to demonstrate executing debugger command

 */
HRESULT CALLBACK
cmdsample(PDEBUG_CLIENT4 Client, PCSTR args)
{
    CHAR Input[256];
    INIT_API();

    UNREFERENCED_PARAMETER(args);

    //
    // Output a 10 frame stack
    //
    g_ExtControl->OutputStackTrace(DEBUG_OUTCTL_ALL_CLIENTS |   // Flags on what to do with output
                                   DEBUG_OUTCTL_OVERRIDE_MASK |
                                   DEBUG_OUTCTL_NOT_LOGGED,
                                   NULL,
                                   10,           // number of frames to display
                                   DEBUG_STACK_FUNCTION_INFO | DEBUG_STACK_COLUMN_NAMES |
                                   DEBUG_STACK_ARGUMENTS | DEBUG_STACK_FRAME_ADDRESSES);
    //
    // Engine interface for print
    //
    g_ExtControl->Output(DEBUG_OUTCTL_ALL_CLIENTS, "\n\nDebugger module list\n");

    //
    // list all the modules by executing lm command
    //
    g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
                          DEBUG_OUTCTL_OVERRIDE_MASK |
                          DEBUG_OUTCTL_NOT_LOGGED,
                          "lm", // Command to be executed
                          DEBUG_EXECUTE_DEFAULT );

    //
    // Ask for user input
    //
    g_ExtControl->Output(DEBUG_OUTCTL_ALL_CLIENTS, "\n\n***User Input sample\n\nEnter Command to run : ");
    GetInputLine(NULL, &Input[0], sizeof(Input));
    g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
                          DEBUG_OUTCTL_OVERRIDE_MASK |
                          DEBUG_OUTCTL_NOT_LOGGED,
                          Input, // Command to be executed
                          DEBUG_EXECUTE_DEFAULT );

    EXIT_API();
    return S_OK;
}

/*
  Sample extension to read and dump a struct on target

  This reads the struct _EXCEPTION_RECORD which is defined as:

  typedef struct _EXCEPTION_RECORD {
    NTSTATUS ExceptionCode;
    ULONG ExceptionFlags;
    struct _EXCEPTION_RECORD *ExceptionRecord;
    PVOID ExceptionAddress;
    ULONG NumberParameters;
    ULONG_PTR ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
    } EXCEPTION_RECORD;
*/
HRESULT CALLBACK
structsample(PDEBUG_CLIENT4 Client, PCSTR args)
{
    ULONG64 Address;
    INIT_API();

    Address = GetExpression(args);

    DWORD Buffer[4], cb;

    // Read and display first 4 dwords at Address
    if (ReadMemory(Address, &Buffer, sizeof(Buffer), &cb) && cb == sizeof(Buffer)) {
        dprintf("%p: %08lx %08lx %08lx %08lx\n\n", Address,
                Buffer[0], Buffer[1], Buffer[2], Buffer[3]);
    }

    //
    // Method 1 to dump a struct
    //
    dprintf("Method 1:\n");
    // Inititalze type read from the Address
    if (InitTypeRead(Address, _EXCEPTION_RECORD) != 0) {
        dprintf("Error in reading _EXCEPTION_RECORD at %p", // Use %p to print pointer values
                Address);
    } else {
        // read and dump the fields
        dprintf("_EXCEPTION_RECORD @ %p\n", Address);
        dprintf("\tExceptionCode           : %lx\n", (ULONG) ReadField(ExceptionCode));
        dprintf("\tExceptionAddress        : %p\n", ReadField(ExceptionAddress));
        dprintf("\tExceptionInformation[1] : %I64lx\n", ReadField(ExceptionInformation[1]));
        // And so on...
    }

    //
    // Method 2 to read a struct
    //
    ULONG64 ExceptionInformation_1, ExceptionAddress, ExceptionCode;
    dprintf("\n\nMethod 2:\n");
    // Read and dump the fields by specifying type and address individually
    if (GetFieldValue(Address, "_EXCEPTION_RECORD", "ExceptionCode", ExceptionCode)) {
        dprintf("Error in reading _EXCEPTION_RECORD at %p\n",
                Address);
    } else {
        // Pointers are read as ULONG64 values
        GetFieldValue(Address, "_EXCEPTION_RECORD", "ExceptionAddress", ExceptionAddress);
        GetFieldValue(Address, "_EXCEPTION_RECORD", "ExceptionInformation[1]", ExceptionInformation_1);
        // And so on..

        dprintf("_EXCEPTION_RECORD @ %p\n", Address);
        dprintf("\tExceptionCode           : %lx\n", ExceptionCode);
        dprintf("\tExceptionAddress        : %p\n", ExceptionAddress);
        dprintf("\tExceptionInformation[1] : %I64lx\n", ExceptionInformation_1);
    }

    ULONG64 Module;
    ULONG   i, TypeId;
    CHAR Name[MAX_PATH];
    //
    // To get/list field names
    //
    g_ExtSymbols->GetSymbolTypeId("_EXCEPTION_RECORD", &TypeId, &Module);
    dprintf("Fields of _EXCEPTION_RECORD\n");
    for (i=0; ;i++) {
        HRESULT Hr;
        ULONG Offset=0;

        Hr = g_ExtSymbols->GetFieldName(Module, TypeId, i, Name, MAX_PATH, NULL);
        if (Hr == S_OK) {
            g_ExtSymbols->GetFieldOffset(Module, TypeId, Name, &Offset);
            dprintf("%lx (+%03lx) %s\n", i, Offset, Name);
        } else if (Hr == E_INVALIDARG) {
            // All Fields done
            break;
        } else {
            dprintf("GetFieldName Failed %lx\n", Hr);
            break;
        }
    }

    //
    // Get name for an enumerate
    //
    //     typedef enum {
    //        Enum1,
    //        Enum2,
    //        Enum3,
    //     } TEST_ENUM;
    //
    ULONG   ValueOfEnum = 0;
    g_ExtSymbols->GetSymbolTypeId("TEST_ENUM", &TypeId, &Module);
    g_ExtSymbols->GetConstantName(Module, TypeId, ValueOfEnum, Name, MAX_PATH, NULL);
    dprintf("Testenum %I64lx == %s\n", ExceptionCode, Name);
    // This prints out, Testenum 0 == Enum1

    //
    // Read an array
    //
    //    typedef struct FOO_TYPE {
    //      ULONG Bar;
    //      ULONG Bar2;
    //    } FOO_TYPE;
    //
    //    FOO_TYPE sampleArray[20];
    ULONG Bar, Bar2;
    CHAR TypeName[100];
    for (i=0; i<20; i++) {
        sprintf_s(TypeName, sizeof(TypeName), "sampleArray[%lx]", i);
        if (GetFieldValue(0, TypeName, "Bar", Bar))
            break;
        GetFieldValue(0, TypeName, "Bar2", Bar2);
        dprintf("%16s -  Bar %2ld  Bar2 %ld\n", TypeName, Bar, Bar2);
    }

    EXIT_API();
    return S_OK;
}

/*
   Extension to look at locals through IDebugSymbolGroup interface

        Usage: !symgrptest [args]

        To demontrate local/args lookup, this will go through the stack
        and set scope with the instruction of the stack frame and
        enumerate all the locals/arguments for the frame function
*/
HRESULT CALLBACK
symgrptest(PDEBUG_CLIENT4 Client, PCSTR args)
{
    HRESULT hRes;
    IDebugSymbolGroup *pDbgSymGroup;
    ULONG ulCount, nFrames;
    DEBUG_STACK_FRAME Stack[50];
    ULONG SymGroupType;

    INIT_API();

    if (!_stricmp(args, "args"))
    {
        // Disply only the arguments
        SymGroupType = DEBUG_SCOPE_GROUP_ARGUMENTS;
    } else
    {
        // Display all the locals
        SymGroupType = DEBUG_SCOPE_GROUP_LOCALS;
    }

    //
    // Get the current stack
    //
    if ((hRes = g_ExtControl->GetStackTrace(0, 0, 0, &Stack[0], 50, &nFrames)) != S_OK) {
        dprintf("Stacktrace failed - %lx\n", hRes);
        nFrames = 0;
    }

    // Create a local symbol group client
    if ((hRes = g_ExtSymbols->
         GetScopeSymbolGroup(SymGroupType,
                             NULL, &pDbgSymGroup)) == E_NOTIMPL)
    {
        EXIT_API();
        return S_OK;
    }

    while (nFrames) {
        //
        // Enumerate locals for this frame
        //
        --nFrames;

        // Set scope at this frame
        g_ExtSymbols->SetScope(0, &Stack[nFrames], NULL, 0);

        // refresh the symbol group with current scope
        if ((hRes = g_ExtSymbols->
             GetScopeSymbolGroup(DEBUG_SCOPE_GROUP_LOCALS,
                                 pDbgSymGroup, &pDbgSymGroup)) == E_NOTIMPL)
        {
            break;
        }
        hRes =
            pDbgSymGroup->GetNumberSymbols ( &ulCount);

        dprintf("\n\n>Scope Frame %lx: %lx Symbols\n",nFrames,ulCount);

        PDEBUG_SYMBOL_PARAMETERS pSymParams =
                 new DEBUG_SYMBOL_PARAMETERS[ ulCount ] ;
        if (ulCount)
        {
            // Get all symbols for the frame
            hRes =
                pDbgSymGroup->GetSymbolParameters (0,
                                               ulCount    ,
                                               pSymParams);
        }
        if ( S_OK == hRes )
        {
            for ( ULONG i = 0 ; i < ulCount ; i++ )
                {
                TCHAR szName[ MAX_PATH ], *pName;
                ULONG ulSize;
                DEBUG_VALUE Value;

                // Lookup symbol name and print
                pName = &szName[1];
                hRes = pDbgSymGroup->GetSymbolName ( i,
                                                     pName,
                                                     MAX_PATH - 1,
                                                     &ulSize ) ;

                // Prefix ! so this is evaluated as symbol
                szName[0] = '!';
                hRes = g_ExtControl->Evaluate(szName,
                                              DEBUG_VALUE_INVALID,
                                              &Value, NULL);

                dprintf("%lx: %32s = 0x%p\n",
                        i, pName, Value.I64);
            }
        }
        delete pSymParams;
    }
    pDbgSymGroup->Release();
    EXIT_API();
    return S_OK;
}

/*
  This gets called (by DebugExtensionNotify whentarget is halted and is accessible
*/
HRESULT
NotifyOnTargetAccessible(PDEBUG_CONTROL Control)
{
    dprintf("Extension dll detected a break");
    if (Connected) {
        dprintf(" connected to ");
        switch (TargetMachine) {
        case IMAGE_FILE_MACHINE_I386:
            dprintf("X86");
            break;
        case IMAGE_FILE_MACHINE_IA64:
            dprintf("IA64");
            break;
        default:
            dprintf("Other");
            break;
        }
    }
    dprintf("\n");

    //
    // show the top frame and execute dv to dump the locals here and return
    //
    Control->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
                     DEBUG_OUTCTL_OVERRIDE_MASK |
                     DEBUG_OUTCTL_NOT_LOGGED,
                     ".frame", // Command to be executed
                     DEBUG_EXECUTE_DEFAULT );
    Control->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
                     DEBUG_OUTCTL_OVERRIDE_MASK |
                     DEBUG_OUTCTL_NOT_LOGGED,
                     "dv", // Command to be executed
                     DEBUG_EXECUTE_DEFAULT );
    return S_OK;
}

/*
  A built-in help for the extension dll
*/
HRESULT CALLBACK
help(PDEBUG_CLIENT4 Client, PCSTR args)
{
    INIT_API();

    UNREFERENCED_PARAMETER(args);

    dprintf("Help for dbgexts.dll\n"
            "  cmdsample           - This does stacktrace and lists\n"
            "  help                = Shows this help\n"
            "  structsample <addr> - This dumps a struct at given address\n"
            );
    EXIT_API();

    return S_OK;
}
