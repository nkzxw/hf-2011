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
// Object: class helper to find stack size of a function by calling it
//-----------------------------------------------------------------------------

#include "FindStackSizeByCall.h"

//-----------------------------------------------------------------------------
// Name: FindStackSizeByCall
// Object: try to find stack size (and so get an idea of number of parameters)
//         by calling func with parameters put to NULL
// Parameters :
//     in  : TCHAR* pszLibName : library name
//           TCHAR* pszFuncName : function name (required only if bOrdinal not set)
//           DWORD Ordinal : function ordinal  (required only if bOrdinal is set)
//           BOOL bOrdinal : flag to know if we have to use pszFuncName or Ordinal parameter
//     out : DWORD* pStackSize : required stack size if function call doesn't fail
//     return : ERROR_CODES
//-----------------------------------------------------------------------------
CFindStackSizeByCall::ERROR_CODES CFindStackSizeByCall::FindStackSizeByCall(TCHAR* pszLibName,TCHAR* pszFuncName,DWORD Ordinal,BOOL bOrdinal,DWORD* pStackSize)
{
    FARPROC Function;
    HMODULE hModule;
    ERROR_CODES ERROR_CODE=ERROR_CODE_NO_ERROR;
    char* FunctionName;

    if (bOrdinal)
    {
        FunctionName=(char*)Ordinal;
    }
    else
    {
#if (defined(UNICODE)||defined(_UNICODE))
        CAnsiUnicodeConvert::UnicodeToAnsi(pszFuncName,&FunctionName);
        if (!FunctionName)
            return ERROR_CODE_MEMORY_ALLOCATION_ERROR;
#else
        FunctionName=pszFuncName;
#endif
    }

    // get library handle
    hModule=::GetModuleHandle(pszLibName);
    if (hModule==NULL)
    {
        hModule=::LoadLibrary(pszLibName);
        if (!hModule)
            return ERROR_CODE_LOAD_LIBRARY_ERROR;
    }

    // get function address
    Function=::GetProcAddress(hModule,FunctionName);
    if (!Function)
        return ERROR_CODE_GET_PROC_ADDRESS_ERROR;


    // assume we get full path of module
    TCHAR DllName[MAX_PATH];
    ::GetModuleFileName(hModule,DllName,MAX_PATH);

    CPE pe(DllName);
    if (!pe.Parse(TRUE,FALSE,FALSE))
        return ERROR_CODE_PARSING_PE_ERROR;

    if (!pe.IsExecutable((ULONGLONG)Function-(ULONGLONG)hModule))// /!\ address is RVA --> VA (Function) - module base address (hModule)
    {
        // not forwarded item
        return ERROR_CODE_ADDRESS_IS_NOT_IN_AN_EXECUTABLE_SECTION;
    }

    // find stack by call
    ERROR_CODE=CFindStackSizeByCall::FindStackSizeByCall(Function,pStackSize);

#if (defined(UNICODE)||defined(_UNICODE))
    if (!bOrdinal)
    {
        delete FunctionName;
    }
#endif

    return ERROR_CODE;
}

//-----------------------------------------------------------------------------
// Name: FindStackSizeByCall
// Object: try to find stack size (and so get an idea of number of parameters)
//         by calling func with parameters put to NULL
// Parameters :
//     in  : 
//           FARPROC Function : address of the function
//     out : DWORD* pStackSize : required stack size if function call doesn't fail
//     return : ERROR_CODES
//-----------------------------------------------------------------------------
CFindStackSizeByCall::ERROR_CODES CFindStackSizeByCall::FindStackSizeByCall(FARPROC Function,DWORD* pStackSize)
{
    ERROR_CODES ERROR_CODE=ERROR_CODE_NO_ERROR;
    DWORD dwOriginalESP;
    DWORD dwESPBeforeFuncCall;
    DWORD dwESPAfterFuncCall;
    DWORD dwEspMaxArgsSize;

    dwEspMaxArgsSize=CFindStackSizeByCall::MAX_NUMBER_OF_PARAM*sizeof(PBYTE);
    *pStackSize=0;

    // try to make a call with NULL parameters to guess number of params
    // NOTICE : catching error is sometimes not enough, because if called function
    // catch error internal and do a call to ExitProcess inside its error handler, the function will never return
    // That's why, we better have to create a process to run this code
    try
    {
        CExceptionHardware::RegisterTry();

        _asm
        {
            // store esp to restore it without caring about calling convention
            mov [dwOriginalESP],ESP
        }

        // ok sometimes stack is not big enough to use CFindStackSizeByCall::MAX_NUMBER_OF_PARAM
        // so do a checking before using memory
        // (to avoid this set project properties / linker / system / "Stack Reserved Size" AND "Stack Commit Size" to a sufficient value
        //  0x100000 is enough in our case)
        while ((IsBadReadPtr((void*)(dwOriginalESP-dwEspMaxArgsSize),dwEspMaxArgsSize))&&dwEspMaxArgsSize!=0)
            dwEspMaxArgsSize/=2;

        _asm
        {
            // empty registers
            mov eax,0
            mov ebx,0
            mov ecx,0
            mov edx,0
            mov esi,0
            mov edi,0

            // store esp to restore it without caring about calling convention
            mov [dwOriginalESP],ESP

            // use a security to avoid crashing in case of bad parameters number
            Sub ESP, [dwEspMaxArgsSize]
        }
        // set our allocated memory to 0 
        // (warning memory if allocated by esp-xxx, so to empty buffer we have to do it from new esp addr to old one)
        memset((PBYTE)(dwOriginalESP-dwEspMaxArgsSize),0,dwEspMaxArgsSize);

        _asm
        {
            //get esp value
            mov [dwESPBeforeFuncCall],ESP

            // call func
            call Function

            // get esp value
            Mov [dwESPAfterFuncCall],ESP
        }

        // compute stack size
        *pStackSize=dwESPAfterFuncCall-dwESPBeforeFuncCall;

    }
    catch( CExceptionHardware e )
    {
        ERROR_CODE=ERROR_CODE_HARDWARE_EXCEPTION_INSIDE_CALL;
    }
    catch(...)
    {
        ERROR_CODE=ERROR_CODE_SOFTWARE_EXCEPTION_INSIDE_CALL;
    }

    _asm
    {
        // restore esp
        mov ESP,[dwOriginalESP]
    }
    return ERROR_CODE;
}

