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
// Object: manages process internal calls
//-----------------------------------------------------------------------------
#include "ProcessInternalCallRequest.h"

//-----------------------------------------------------------------------------
// Name: ProcessInternalCallRequest
// Object: call function at address pFunc with parameters specified in pParams
//          and store function return (eax) in pRet
// Parameters :
//      in: PBYTE pBuffer
//          CMailSlotClient* pMailSlotClient
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CProcessInternalCallRequest::ProcessInternalCallRequestFromMailSlot(PBYTE pBuffer,CMailSlotClient* pMailSlotClient)
{
    FARPROC pFunc;
    PBYTE Data;
    DWORD dw;
    BOOL bRes=FALSE;
    CApiOverrideFuncAndParams* pApiOverrideFuncAndParams=new CApiOverrideFuncAndParams();

    // decode specified buffer
    if (!pApiOverrideFuncAndParams->Decode(pBuffer))
    {
        bRes=FALSE;
        goto error;
    }

    //////////////////////////
    // get hook address
    //////////////////////////
    BOOL bExeDllInternal;
    BOOL bFunctionPointer;
    pFunc=(FARPROC)GetWinAPIOverrideFunctionDescriptionAddress(pApiOverrideFuncAndParams->DecodedLibName,
                                                                pApiOverrideFuncAndParams->DecodedFuncName,
                                                                &bExeDllInternal,&bFunctionPointer);

    // check func pointer
    if (IsBadCodePtr(pFunc))
    {
        TCHAR psz[2*MAX_PATH];
        _sntprintf(psz,2*MAX_PATH,_T("Process internal call: Invalid Code Pointer for %s in %s"),
            pApiOverrideFuncAndParams->DecodedFuncName,
            pApiOverrideFuncAndParams->DecodedLibName);
        DynamicMessageBoxInDefaultStation(NULL,psz,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        bRes=FALSE;
        goto error;
    }

#ifdef WIN32
    if (pApiOverrideFuncAndParams->DecodedCallingConvention==CALLING_CONVENTION_FASTCALL)
    {
        if (_tcsnicmp(pApiOverrideFuncAndParams->DecodedLibName,DLL_OR_EXE_NET_PREFIX,_tcslen(DLL_OR_EXE_NET_PREFIX))==0)
        {
            pApiOverrideFuncAndParams->DecodedCallingConvention=CALLING_CONVENTION_FASTCALL_PUSHED_LEFT_TO_RIGHT;
        }
    }
#endif

    // make call
    bRes=ProcessInternalCallRequestEx(
            (FARPROC)pFunc,
            pApiOverrideFuncAndParams->DecodedCallingConvention,
            pApiOverrideFuncAndParams->DecodedNbParams,
            pApiOverrideFuncAndParams->DecodedParams,
            &pApiOverrideFuncAndParams->DecodedRegisters,
            &pApiOverrideFuncAndParams->DecodedReturnedValue,
            &pApiOverrideFuncAndParams->DecodedFloatingReturn,
            pApiOverrideFuncAndParams->DecodedThreadID,
            pApiOverrideFuncAndParams->DecodedTimeOut
            );

error:
    // encode result
    if (pApiOverrideFuncAndParams->Encode(
        pApiOverrideFuncAndParams->DecodedID,
        pApiOverrideFuncAndParams->DecodedLibName,
        pApiOverrideFuncAndParams->DecodedFuncName,
        pApiOverrideFuncAndParams->DecodedNbParams,
        pApiOverrideFuncAndParams->DecodedParams,
        &pApiOverrideFuncAndParams->DecodedRegisters,
        pApiOverrideFuncAndParams->DecodedReturnedValue,
        pApiOverrideFuncAndParams->DecodedFloatingReturn,
        bRes,
        pApiOverrideFuncAndParams->DecodedThreadID,
        pApiOverrideFuncAndParams->DecodedTimeOut,
        pApiOverrideFuncAndParams->DecodedCallingConvention
        ))
    {
        // send results throw mailslot
        Data=new BYTE[pApiOverrideFuncAndParams->EncodedBufferSize+sizeof(DWORD)];
        dw=CMD_PROCESS_INTERNAL_CALL_REPLY;
        memcpy(Data,&dw,sizeof(DWORD));
        memcpy(&Data[sizeof(DWORD)],pApiOverrideFuncAndParams->EncodedBuffer,pApiOverrideFuncAndParams->EncodedBufferSize);
        bRes=pMailSlotClient->Write(Data,pApiOverrideFuncAndParams->EncodedBufferSize+sizeof(DWORD));
        delete[] Data;
    }
    delete pApiOverrideFuncAndParams;

    return bRes;
}

//-----------------------------------------------------------------------------
// Name: ProcessInternalCallRequest
// Object: call function at address pFunc with parameters specified in pParams
//          and store function return (eax) in pRet
// Parameters :
//      in: FARPROC pFunc : function address
//          int NbParams : nb params in pParams
//          PSTRUCT_FUNC_PARAM pParams : array of STRUCT_FUNC_PARAM. Can be null if no params
//      in out : REGISTERS* pRegisters : in : register before call, out : registers after call
//      out : PBYTE* pRet : returned value
//            double* pFloatingResult : floating returned value
// Return : 
//-----------------------------------------------------------------------------
BOOL CProcessInternalCallRequest::ProcessInternalCallRequest(FARPROC pFunc,tagCALLING_CONVENTION CallingConvention,int NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE* pRet,double* pFloatingResult)
{
    BOOL bRet;
    int cnt;
    DWORD dw;
    BYTE b;
    USHORT us;
    PSTRUCT_FUNC_PARAM pCurrentParam;
    PBYTE OriginalESP;
    DWORD dwEspSecuritySize=ESP_SECURITY_SIZE* REGISTER_BYTE_SIZE;
    REGISTERS LocalRegisters;
    REGISTERS Registers;
    DWORD dwDataSize;
    PBYTE CurrentESP;
    double FloatingResult;
    BYTE FloatingNotSet[8]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff};
    memcpy(&FloatingResult,FloatingNotSet,8);
    memcpy(&Registers,pRegisters,sizeof(REGISTERS));
    WORD wFPUStatusRegister;

    int NbParamsPassedByRegisters;
    int AdjustedNbParams=NbParams;
    PSTRUCT_FUNC_PARAM AdjustedpParams=pParams;

    switch(CallingConvention)
    {
    case CALLING_CONVENTION_CDECL:
    case CALLING_CONVENTION_STDCALL:
    case CALLING_CONVENTION_STDCALL_OR_CDECL:
        // nothing to do parameters are pushed on stack
        break;
    case CALLING_CONVENTION_THISCALL:
        // should use ECX, assume that first param is a pointer or it size is less than a pointer
        if ((pParams[0].bPassAsRef) || (pParams[0].dwDataSize<=sizeof(PBYTE)))
        {
            AdjustedpParams=&pParams[1];
            AdjustedNbParams=NbParams-1;
#ifndef _WIN64
            if (pParams[0].bPassAsRef)
            {
                Registers.ecx=(DWORD)pParams[0].pData;
            }
            else
            {
                Registers.ecx=*((DWORD*)pParams[0].pData);
            }
#else
            TODO
#endif
        }
        break;
    case CALLING_CONVENTION_FASTCALL:
    case CALLING_CONVENTION_FASTCALL_PUSHED_LEFT_TO_RIGHT:
        NbParamsPassedByRegisters=0;
        AdjustedpParams=(PSTRUCT_FUNC_PARAM)_alloca(NbParams*sizeof(STRUCT_FUNC_PARAM));
        AdjustedNbParams=0;
        for(cnt=0;cnt<NbParams;cnt++)
        {
#ifdef WIN32
            if ((NbParamsPassedByRegisters<2)
#else
            if ((NbParamsPassedByRegisters<4)
#endif
            && ((pParams[cnt].bPassAsRef) || (pParams[cnt].dwDataSize<=sizeof(PBYTE))))
            {
                if (pParams[cnt].bPassAsRef)
                {
                    switch(NbParamsPassedByRegisters)
                    {
#ifndef _WIN64
                    case 0:
                        Registers.ecx=(DWORD)pParams[cnt].pData;
                        break;
                    case 1:
                        Registers.edx=(DWORD)pParams[cnt].pData;
                        break;
#else
                        TODO
#endif
                    }
                    NbParamsPassedByRegisters++;
                }
                else
                {
                    switch(NbParamsPassedByRegisters)
                    {
#ifndef _WIN64
                    case 0:
                        Registers.ecx=*((DWORD*)pParams[cnt].pData);
                        break;
                    case 1:
                        Registers.edx=*((DWORD*)pParams[cnt].pData);
                        break;
#else
                        TODO
#endif
                    }
                }
            }
            else
            {
                memcpy(&AdjustedpParams[AdjustedNbParams],&pParams[cnt],sizeof(STRUCT_FUNC_PARAM));
                AdjustedNbParams++;
            }
        }
        break;
    }
#ifdef WIN32
    if (CallingConvention==CALLING_CONVENTION_FASTCALL_PUSHED_LEFT_TO_RIGHT)
    {
        PSTRUCT_FUNC_PARAM NetAdjustedpParams=(PSTRUCT_FUNC_PARAM)_alloca(AdjustedNbParams*sizeof(STRUCT_FUNC_PARAM));
        // reverse order of stack parameters
        for (cnt=0;cnt<AdjustedNbParams;cnt++)
        {
            memcpy(&NetAdjustedpParams[cnt],&AdjustedpParams[AdjustedNbParams-cnt-1],sizeof(STRUCT_FUNC_PARAM));
        }
        AdjustedpParams=NetAdjustedpParams;
    }
#endif    


    _asm
    {
        // store esp to restore it without caring about calling convention
        mov [OriginalESP],ESP

        // use a security to avoid crashing in case of bad parameters number
        Sub ESP, [dwEspSecuritySize]
    }

    // make things cleaner using a try
    // to catch ALL exceptions (even memory access) assume to have the /EHa option 
    // VS2003 "project" / "properties" / "C/C++" / "Command Line" / "Additional options" / "/EHa"
    // VS2005 "project" / "properties" / "C/C++" / "Code Generation" / "Enable C++ exceptions"  --> "Yes With SEH Exceptions (/EHa)"
    try
    {
        CExceptionHardware::RegisterTry();

        // set our allocated memory to 0 
        // (warning memory si allocated by esp-xxx, so to empty buffer we have to do it from new esp addr to old one)
        memset((PBYTE)(OriginalESP-dwEspSecuritySize),0,dwEspSecuritySize);
        // for each param
        for (cnt=AdjustedNbParams-1;cnt>=0;cnt--)
        {
            pCurrentParam=&AdjustedpParams[cnt];

            // if params should be passed as ref
            if (pCurrentParam->bPassAsRef)
            {
                // push param address
                dw=(DWORD)pCurrentParam->pData;
                _asm
                {
                    mov eax,dw
                    push eax
                }
            }
            else // we have to push param value
            {
                // byte
                if (pCurrentParam->dwDataSize==1)
                {
                    b=pCurrentParam->pData[0];
                    _asm
                    {
                        mov al,b
                        push eax
                    }
                }
                // short
                else if (pCurrentParam->dwDataSize==2)
                {
                    memcpy(&us,pCurrentParam->pData,2);
                    _asm
                    {
                        mov ax,us
                        push eax
                    }
                }
                // dword
                else if (pCurrentParam->dwDataSize==4)
                {
                    memcpy(&dw,pCurrentParam->pData,4);
                    _asm
                    {
                        mov eax,dw
                        push eax
                    }
                }
                // more than dword
                else
                {
                    // as we are not always 4 bytes aligned we can't do a loop with push

                    // allocate necessary space in stack
                    dwDataSize=pCurrentParam->dwDataSize;
                    _asm
                    {
                        sub esp, [dwDataSize]
                        mov [CurrentESP],esp
                    }

                    // copy data to stack
                    memcpy(CurrentESP,pCurrentParam->pData,dwDataSize);

                }
            }
        }
        // now all params are pushed in stack --> just make call
        _asm
        {
            ////////////////////////////////
            // save local registers
            ////////////////////////////////
            mov [LocalRegisters.eax],eax
            mov [LocalRegisters.ebx],ebx
            mov [LocalRegisters.ecx],ecx
            mov [LocalRegisters.edx],edx
            mov [LocalRegisters.esi],esi
            mov [LocalRegisters.edi],edi
            pushfd
            pop [LocalRegisters.efl]

            ////////////////////////////////
            // set registers as wanted
            ////////////////////////////////
            mov eax, [Registers.eax]
            mov ebx, [Registers.ebx]
            mov ecx, [Registers.ecx]
            mov edx, [Registers.edx]
            mov esi, [Registers.esi]
            mov edi, [Registers.edi]
            push [Registers.efl]
            popfd

            // call func
            call pFunc

            // save registers after call
            mov [Registers.eax],eax
            mov [Registers.ebx],ebx
            mov [Registers.ecx],ecx
            mov [Registers.edx],edx
            mov [Registers.esi],esi
            mov [Registers.edi],edi
            pushfd
            pop [Registers.efl]


            // put pointer to return address in ecx
            mov ecx,[pRet]
            // put return value in the address pointed by ecx --> *pRet=eax
            mov eax,[Registers.eax]
            mov [ecx],eax

            ////////////////////////////////
            // restore local registers
            ////////////////////////////////
            mov eax, [LocalRegisters.eax]
            mov ebx, [LocalRegisters.ebx]
            mov ecx, [LocalRegisters.ecx]
            mov edx, [LocalRegisters.edx]
            mov esi, [LocalRegisters.esi]
            mov edi, [LocalRegisters.edi]
            push [LocalRegisters.efl]
            popfd
        }

        // check if there's data in the floating stack
        _asm
        {
            // get top of stack
            fstsw [wFPUStatusRegister]
        }
        // top of stack is in bits 13,12,11
        // so if top of stack is not empty
        if (wFPUStatusRegister & 0x3800)
        {
            _asm
            {
                fst qword ptr [FloatingResult] // get data from floating register
            }
        }

        bRet=TRUE;
    }
    catch(CExceptionHardware e)
    {
        DynamicMessageBoxInDefaultStation(0,e.ExceptionText,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        bRet=FALSE;
    }
    catch(...)
    {
        DynamicMessageBoxInDefaultStation(0,_T("A software exception has been thrown by function"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        bRet=FALSE;
    }

    _asm
    {
        // restore esp (works for both calling convention)
        mov ESP,[OriginalESP]
    }

    if (bRet)
        memcpy(pRegisters,&Registers,sizeof(REGISTERS));

    *pFloatingResult=FloatingResult;

    return bRet;
}


//-----------------------------------------------------------------------------
// Name: ProcessInternalCallRequestInThread
// Object: function called after thread context switching, in wanted thread
// Parameters :
//      in: PROCESS_INTERNAL_CALL_REQUEST_EX_PARAM* pProcessInternalCallRequestExParam :
//                          pointer to struct containing all needed parameters to do the call
// Return : 
//-----------------------------------------------------------------------------
__declspec(naked) void __stdcall CProcessInternalCallRequest::ProcessInternalCallRequestInThread(PROCESS_INTERNAL_CALL_REQUEST_EX_PARAM* pProcessInternalCallRequestExParam)
{
    BOOL bRet;
    DWORD LastError;
    PBYTE mEip;
    __asm 
    {
        push ebp
        mov  ebp, esp
        sub  esp, __LOCAL_SIZE

        // debug purpose only cause watch sucks sometimes on naked func (at least on vs2005)
        // mov eax,pProcessInternalCallRequestExParam
       }

    // restore original bytes (without calling func we don't know were eip is, so may in the memcpy func)
    CThreadContext::asm_memcpy(pProcessInternalCallRequestExParam->Eip,
                      pProcessInternalCallRequestExParam->OriginalBytes,PROCESSINTERNALCALLREQUESTINTHREAD_OPCODESIZE);

    // set return address in stack
    mEip=pProcessInternalCallRequestExParam->Eip;
    __asm
    {
        mov eax, ebp
        // add eax,4    // push ebp
        // add eax,4    // return address
        // add eax,4    // pProcessInternalCallRequestExParam parameter
        // add eax,0x24 // size of pushad + pusfd
        add eax,0x30
        mov ebx,mEip
        mov [eax],ebx
    }

    // save last error code before calling any api
    LastError=GetLastError();

    // Resume all other threads of the process (in case of multi threading interactions)
    CThreadContext::ResumeAllOtherThreads(pProcessInternalCallRequestExParam->hThread);

    // call ProcessInternalCallRequest with provided parameters
    bRet=ProcessInternalCallRequest(
                                    pProcessInternalCallRequestExParam->pFunc,
                                    pProcessInternalCallRequestExParam->CallingConvention,
                                    pProcessInternalCallRequestExParam->NbParams,
                                    pProcessInternalCallRequestExParam->pParams,
                                    pProcessInternalCallRequestExParam->pRegisters,
                                    pProcessInternalCallRequestExParam->pRet,
                                    pProcessInternalCallRequestExParam->pFloatingResult
                                    );

    if (!IsBadReadPtr(pProcessInternalCallRequestExParam,sizeof(PROCESS_INTERNAL_CALL_REQUEST_EX_PARAM)))
    {
        // check if memory is ok, as pProcessInternalCallRequestExParam as been stack allocated
        // and so IsBadReadPtr should never return TRUE
        if (pProcessInternalCallRequestExParam->hEndEvent!=0) 
        {
            // fill return
            pProcessInternalCallRequestExParam->bSuccess=bRet;

            // signal the end of function
            SetEvent(pProcessInternalCallRequestExParam->hEndEvent);

            // suspend current thread (only to restore original suspended count of thread,
            // suspended count restoration is done by ProcessInternalCallRequestEx)
            // Notice: Suspend Thread only if all is ok, else you can get a deadlock
            SuspendThread(GetCurrentThread());
        }
    }
    // restore last error code
    SetLastError(LastError);
    __asm  
    {
        mov      esp, ebp
        pop      ebp
        pop      eax // remove return address
        pop      eax // remove param

        popfd // restore flags
        popad // restore registers
        ret   // jump back to eip
    }
}

//-----------------------------------------------------------------------------
// Name: ProcessInternalCallRequestEx
// Object: call function at address pFunc with parameters specified in pParams
//          and store function return (eax) in pRet
// Parameters :
//      in: FARPROC pFunc : function address
//          int NbParams : nb params in pParams
//          PSTRUCT_FUNC_PARAM pParams : array of STRUCT_FUNC_PARAM. Can be null if no params
//          DWORD ThreadId : id of thread into which do the call (0 if no thread preference)
//          DWORD dwTimeOut : call timeout
//      in out : REGISTERS* pRegisters : in : register before call, out : registers after call
//      out : PBYTE* pRet : returned value
//            double* pFloatingResult : floating returned value
// Return : 
//-----------------------------------------------------------------------------
BOOL __stdcall CProcessInternalCallRequest::ProcessInternalCallRequestEx(FARPROC pFunc,tagCALLING_CONVENTION CallingConvention,int NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE* pRet,double* pFloatingResult,DWORD ThreadId,DWORD dwTimeOut)
{
    PROCESS_INTERNAL_CALL_REQUEST_EX_PARAM ProcessInternalCallRequestExParam;
    DWORD OriginalSuspendedCount;
    HANDLE hThread;
    DWORD SuspendedCount;

    if ((ThreadId==0) // if no thread preference
        ||(ThreadId==GetCurrentThreadId()))
    {
        return ProcessInternalCallRequest(pFunc,CallingConvention,NbParams,pParams,pRegisters,pRet,pFloatingResult);
    }
    // else

    // fill our struct
    ProcessInternalCallRequestExParam.pFunc=pFunc;
    ProcessInternalCallRequestExParam.NbParams=NbParams;
    ProcessInternalCallRequestExParam.pParams=pParams;
    ProcessInternalCallRequestExParam.pRegisters=pRegisters;
    ProcessInternalCallRequestExParam.pRet=pRet;
    ProcessInternalCallRequestExParam.pFloatingResult=pFloatingResult;
    ProcessInternalCallRequestExParam.CallingConvention=CallingConvention;
    ProcessInternalCallRequestExParam.hEndEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
    if(!ProcessInternalCallRequestExParam.hEndEvent)
        return FALSE;

    // default bSuccess in case of ProcessInternalCallRequestInThread fully failure
    ProcessInternalCallRequestExParam.bSuccess=FALSE;

    // open specified thread
    hThread=OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT,FALSE,ThreadId);
    if (!hThread)
    {
        DynamicMessageBoxInDefaultStation(NULL,_T("Error opening specified thread"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }
    // fill struct
    ProcessInternalCallRequestExParam.hThread=hThread;

    // assume thread is suspended
    OriginalSuspendedCount=SuspendThread(hThread);

    // get all thread context
    CThreadContext ThreadContext;
    PBYTE Eip;
    if (!ThreadContext.GetFirstHookableEip(hThread,&Eip))
    {
        ResumeThread(hThread);
        CloseHandle(hThread);
        return FALSE;
    }

    // fill struct
    ProcessInternalCallRequestExParam.Eip=Eip;


    DWORD OldProtectionFlag;
    // remove memory protection
    if (!VirtualProtectEx(GetCurrentProcess(),
                        Eip,
                        PROCESSINTERNALCALLREQUESTINTHREAD_OPCODESIZE,
                        PAGE_EXECUTE_READWRITE,
                        &OldProtectionFlag))
    {
        ResumeThread(hThread);
        CloseHandle(hThread);
        return FALSE;
    }

    // assume that all other threads of the process are suspended
    CThreadContext::SuspendAllOtherThreads(hThread);

    // fill window message pump with a stupid msg
    ThreadContext.UnlockWndProc();

    // save original registers
    memcpy(ProcessInternalCallRequestExParam.OriginalBytes,Eip,PROCESSINTERNALCALLREQUESTINTHREAD_OPCODESIZE);

    // replace opcodes by our own
    PBYTE pBuffer=Eip;

    /*
    push eax // reserve space in stack for eip (return address) without modifying registers
    pushad
    pushfd
    push &ProcessInternalCallRequestExParam
    call ProcessInternalCallRequestInThread
    */

    PBYTE TmpAddress;
    int BufferIndex=0;

    // reserve space in stack for eip (return address) without modifying registers
    pBuffer[BufferIndex++]=0x50;// push eax
    // pushad
    pBuffer[BufferIndex++]=0x60;
    // pushfd
    pBuffer[BufferIndex++]=0x9C;

    // push &ProcessInternalCallRequestExParam
    pBuffer[BufferIndex++]=0xB8; // mov eax,
    TmpAddress=(PBYTE)&ProcessInternalCallRequestExParam;
    memcpy(&pBuffer[BufferIndex],&TmpAddress,sizeof(PBYTE));
    BufferIndex+=sizeof(PBYTE);
    pBuffer[BufferIndex++]=0x50;// push eax

    // call ProcessInternalCallRequestInThread
    pBuffer[BufferIndex++]=0xB8; // mov eax,
    TmpAddress=(PBYTE)ProcessInternalCallRequestInThread;
    memcpy(&pBuffer[BufferIndex],&TmpAddress,sizeof(PBYTE));
    BufferIndex+=sizeof(PBYTE);
    // do a call not a jump, as func parameters must be at ebp-4
    // if no return address is pushed, parameters use won't work
    pBuffer[BufferIndex++]=0xFF;pBuffer[BufferIndex++]=0xD0; // call eax

    // fully resume suspended thread
    do 
    {
        SuspendedCount=ResumeThread(hThread);
    } while((SuspendedCount!=(DWORD)-1) && (SuspendedCount!=1));

    // wait the end of the function during dwTimeOut
    DWORD dwRes=WaitForSingleObject(ProcessInternalCallRequestExParam.hEndEvent,dwTimeOut);
    CloseHandle(ProcessInternalCallRequestExParam.hEndEvent);

    if (dwRes!=WAIT_OBJECT_0)
    {
        // restore original opcodes
        memcpy((PBYTE)Eip,ProcessInternalCallRequestExParam.OriginalBytes,PROCESSINTERNALCALLREQUESTINTHREAD_OPCODESIZE);
        CloseHandle(hThread);
        //set ProcessInternalCallRequestExParam.hEndEvent to 0 to try to detect bad memory in "hooked" thread
        ProcessInternalCallRequestExParam.hEndEvent=0;

        // Resume all other threads of the process 
        CThreadContext::ResumeAllOtherThreads(hThread);

        return FALSE;
    }

    BOOL bRet=ProcessInternalCallRequestExParam.bSuccess;

    // restore memory protection
    VirtualProtectEx(GetCurrentProcess(),
                    Eip,
                    PROCESSINTERNALCALLREQUESTINTHREAD_OPCODESIZE,
                    OldProtectionFlag,
                    &OldProtectionFlag);


    // assume thread as suspended itself
    Sleep(500);

    if (OriginalSuspendedCount==0)
    {
        // thread was not suspended, and is currently suspended with SuspendedCount==1
        // --> resume it once
        ResumeThread(hThread);
    }
    else
    {
        // restore original suspended count
        SuspendedCount=1;
        while((SuspendedCount!=(DWORD)-1) && (SuspendedCount<OriginalSuspendedCount))
        {
            SuspendedCount=SuspendThread(hThread);
        }
    }
    CloseHandle(hThread);

    return bRet;
}