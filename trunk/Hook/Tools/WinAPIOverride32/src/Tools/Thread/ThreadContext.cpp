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
// Object: class helper for getting thread context (until m$ func gives quite stupid results)
//         
//-----------------------------------------------------------------------------

#include "threadcontext.h"


CThreadContext::CThreadContext(void)
{
    this->HookRemoteHook=NULL;
    this->HookRemoteContext=NULL;
    this->HookProcessMemoryAddress=NULL;
    this->bLastGetThreadContextIsForCurrentProcess=FALSE;
    this->pPostMessage=0;
    this->User32hModule=0;

}

CThreadContext::~CThreadContext(void)
{
    // don't free memory as user may not have resumed thread
    // before destroying this object
}

// for window app, you generally fall in the window message loop,which is entered only when receiving messages
// so we can be lock by this stuff
// To avoid such lock, we send a stupid message to force message loop entering (using PostMessage to don't wait for return)
// Notice 1 : we could find the corresponding window and send it a message, but the easiest way is to send a broadcast message
// Notice 2 : this tricks not always works as lock can result of WaitForMultipleObjects and other blocking calls
void CThreadContext::UnlockWndProc()
{
    // get proc address of func
    if (!this->pPostMessage)
    {
        // if we don't have a handle to user32.dll try to get it
        if (!this->User32hModule)
        {
            this->User32hModule=GetModuleHandle(_T("user32.dll"));
            if (!this->User32hModule)// if user32.dll was not loaded
                this->User32hModule=LoadLibrary(_T("user32.dll"));
            if (!this->User32hModule)
                return;
        }
        this->pPostMessage=(pfPostMessage)GetProcAddress(this->User32hModule,"PostMessageW");

        if (!this->pPostMessage)
            return;
    }
    this->pPostMessage(HWND_BROADCAST,WM_GETHOTKEY,0,0);
}

//-----------------------------------------------------------------------------
// Name: GetThreadEip
// Object: retrieve Eip for the given thread
// Parameters :
//     in  : HANDLE hThread : handle of thread (running or suspended thread)
//     out : PBYTE* pEip : eip of thread
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CThreadContext::GetThreadEip(HANDLE hThread,PBYTE* pEip)
{
    BOOL bRet;

    // suspend thread
    if (SuspendThread(hThread)==(DWORD)-1)
    {
        CAPIError::ShowLastError();
        return FALSE;
    }

    // get eip
    bRet=this->GetSuspendedThreadEip(hThread,pEip);

    // resume thread
    if (ResumeThread(hThread)==(DWORD)-1)
    {
        CAPIError::ShowLastError();
        return FALSE;
    }

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: GetSuspendedThreadEip
// Object: retrieve real Eip for the given suspended thread
// Parameters :
//     in  : HANDLE hSuspendedThread : handle of the suspended thread
//     out : PBYTE* pEip : eip of thread
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CThreadContext::GetSuspendedThreadEip(HANDLE hSuspendedThread,PBYTE* pEip)
{
    PBYTE ebp;
    return this->GetSuspendedThreadEipEbp(hSuspendedThread,pEip,&ebp);
}

//-----------------------------------------------------------------------------
// Name: GetFirstHookableEipEbp
// Object: get upper eip/ebp on the stack that can be hooked for context retrieval
// Parameters :
//     in  : HANDLE hSuspendedThread : handle of the suspended thread
//     out : PBYTE* pEip : first eip in thread callstack which memory content can be changed
//           PBYTE* pEbp : ebp associated to first eip
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CThreadContext::GetFirstHookableEip(HANDLE hSuspendedThread,PBYTE* pEip)
{
    PBYTE ebp;
    return this->GetFirstHookableEipEbp(hSuspendedThread,pEip,&ebp);
}

//-----------------------------------------------------------------------------
// Name: GetFirstHookableEipEbp
// Object: get upper eip/ebp on the stack that can be hooked for context retrieval
// Parameters :
//     in  : HANDLE hSuspendedThread : handle of the suspended thread
//     out : PBYTE* pEip : first eip in thread callstack which memory content can be changed
//           PBYTE* pEbp : ebp associated to first eip
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CThreadContext::GetFirstHookableEipEbp(HANDLE hSuspendedThread,PBYTE* pEip,PBYTE* pEbp)
{
    if (!this->GetSuspendedThreadEipEbp(hSuspendedThread,pEip,pEbp))
        return FALSE;

    CProcessAndThreadID ProcessAndThreadID;
    DWORD dwProcessId=ProcessAndThreadID.GetProcessIdOfThread(hSuspendedThread);
    HANDLE ProcessHandle;
    BOOL bThreadBelongsToCurrentProcess=(dwProcessId==GetCurrentProcessId());
    DWORD OldProtectionFlag;
    DWORD OpCodeSize;

    if (bThreadBelongsToCurrentProcess)
    {
        ProcessHandle=GetCurrentProcess();
        OpCodeSize=CTHREADCONTEXT_GETTHREADCONTEXTOFREMOTEPROCESS_OPCODESIZE;
    }
    else
    {
        ProcessHandle = OpenProcess(PROCESS_VM_OPERATION,FALSE,dwProcessId);
        if (!ProcessHandle)
            return FALSE;
        OpCodeSize=CTHREADCONTEXT_GETTHREADCONTEXTOFCURRENTPROCESS_OPCODESIZE;
    }
    // until we can't remove memory protection
    while(!VirtualProtectEx(ProcessHandle,
                        *pEip,
                        OpCodeSize,
                        PAGE_EXECUTE_READWRITE,
                        &OldProtectionFlag))
    {
        if (!this->GetPreviousEipEbp(hSuspendedThread,*pEip,*pEbp,pEip,pEbp))
        {
            if (bThreadBelongsToCurrentProcess)
                CloseHandle(ProcessHandle);
            return FALSE;
        }
    }

    // restore memory protection
    VirtualProtectEx(ProcessHandle,
                    *pEip,
                    OpCodeSize,
                    OldProtectionFlag,
                    &OldProtectionFlag);

    if (!bThreadBelongsToCurrentProcess)
        CloseHandle(ProcessHandle);
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: GetPreviousEipEbp
// Object: get previous eip & ebp
// Parameters :
//     in  : HANDLE hSuspendedThread : handle of the suspended thread
//           PBYTE CurrentEip
//           PBYTE CurrentEbp
//     out : PBYTE* pEip
//           PBYTE* pEbp
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CThreadContext::GetPreviousEipEbp(HANDLE hSuspendedThread,PBYTE CurrentEip,PBYTE CurrentEbp,PBYTE* pEip,PBYTE* pEbp)
{
    // only ebp is requiered for retrieving previous eip/ebp
    UNREFERENCED_PARAMETER(CurrentEip);


    // retrieve Process Id of thread (we got time we are in suspended state :D)
    CProcessAndThreadID ProcessAndThreadID;
    DWORD dwProcessId=ProcessAndThreadID.GetProcessIdOfThread(hSuspendedThread);
    BOOL bThreadBelongsToCurrentProcess=(dwProcessId==GetCurrentProcessId());
    SIZE_T ReadBytes=0;

    // use the return address of the call so it's position in the stack is at ebp+4
    if (bThreadBelongsToCurrentProcess)
    {
        if (IsBadReadPtr(CurrentEbp+sizeof(PBYTE),sizeof(PBYTE)))
            return FALSE;
        memcpy(pEip,CurrentEbp+sizeof(PBYTE),sizeof(PBYTE));
    }
    else
    {
        // --> read this address in remote process
        if(!Toolhelp32ReadProcessMemory(dwProcessId,CurrentEbp+sizeof(PBYTE),pEip,sizeof(PBYTE),&ReadBytes))
            return FALSE;
    }
    
    // notice you can retrieve ebp directly too
    // if you want to do it just use the following lines
    //   Notice: real ebp is the content of ct.ebp  --> like a mov dwRealEbp,dword ptr[ct.ebp]
    if (bThreadBelongsToCurrentProcess)
    {
        if (IsBadReadPtr(CurrentEbp,sizeof(PBYTE)))
            return FALSE;
        memcpy(pEbp,CurrentEbp,sizeof(PBYTE));
    }
    else
    {
        if(!Toolhelp32ReadProcessMemory(dwProcessId,CurrentEbp,pEbp,sizeof(PBYTE),&ReadBytes))
            return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetSuspendedThreadEipEbp
// Object:  M$ GetThreadContext func works for eip and ebp, but quite not other
//          regiters (eax,...) on bugos versions of course
//          a trouble too is that eip can be in kernel so not usable,
//          so to get previous eip of the call stack use GetPreviousEipEbp
// Parameters :
//     in  : HANDLE hThread : handle of the suspended thread
//     out : PBYTE* pEip : eip of thread
//     out : PBYTE* pEbp : ebp of thread
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CThreadContext::GetSuspendedThreadEipEbp(HANDLE hSuspendedThread,PBYTE* pEip,PBYTE* pEbp)
{
    CONTEXT ct={0};
    ct.ContextFlags=CONTEXT_ALL;

    // check parameters
    if (IsBadWritePtr(pEip,sizeof(PBYTE)))
        return FALSE;
    if (IsBadWritePtr(pEbp,sizeof(PBYTE)))
        return FALSE;

    // get M$ context
    if (!::GetThreadContext(hSuspendedThread,&ct))
    {
        CAPIError::ShowLastError();
        return FALSE;
    }

    // eip and ebp values are correctly returned by GetThreadContext, so keep them
    *pEip=(PBYTE)ct.Eip;
    *pEbp=(PBYTE)ct.Ebp;

    // return maybe; :D
    return TRUE;

}


//-----------------------------------------------------------------------------
// Name: GetThreadContext
// Object: getting context of the given thread
//         Caller MUST call GetThreadContextFree() after having fully resumed hThread
//         DON'T WORK ON A DEBUGGED BREAKED THREAD
// Parameters :
//     in  : HANDLE hThread : handle of the thread (running or suspended thread)
//     out : LPCONTEXT lpContext : pointer to a context structure
//                                  eip,eax,ebx,ecx,edx,ebp,esp,edi,esi and efl are not
//                                  those returned by the M$ func
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CThreadContext::GetThreadContext(HANDLE hThread,LPCONTEXT lpContext)
{
    PBYTE Eip=0;
    PBYTE Ebp=0;
    DWORD SuspendedCount;
    DWORD dwCnt;
    BOOL bRet;
    CThreadContext::CONTEXT_LITE ContextLite={0};
    CProcessAndThreadID ProcessAndThreadID;

    // check parameters
    if (IsBadWritePtr(lpContext,sizeof(CONTEXT)))
        return FALSE;

    // assume user has free memory of previous GetThreadContext call
    if (!this->HasProcessMemoryBeenFree())
        return FALSE;

    // Dummy user prevention
    if (ProcessAndThreadID.GetThreadId(hThread)==GetCurrentThreadId())
    {
        // for current thread user have direct access to all registers
#ifndef TOOLS_NO_MESSAGEBOX
        // user asks it's one registers --> report an error
        MessageBox(NULL,_T("Can't get context of current thread"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
#endif
        return FALSE;
    }

    // MSDN You cannot get a valid context for a running thread. Use the SuspendThread function to suspend the thread before calling GetThreadContext
    SuspendedCount=SuspendThread(hThread);
    if (SuspendedCount==(DWORD)-1)
    {
        CAPIError::ShowLastError();
        return FALSE;
    }

    // get real eip
    if (!this->GetFirstHookableEipEbp(hThread,&Eip,&Ebp))
    {
        ResumeThread(hThread);
        return FALSE;
    }

    // assume SuspendedCount thread value is 1
    for(dwCnt=0;dwCnt<SuspendedCount;dwCnt++)
    {
        if (ResumeThread(hThread)==(DWORD)-1)
        {
            CAPIError::ShowLastError();
            return FALSE;
        }
    }

    // assume that all other threads of the process are suspended
    CThreadContext::SuspendAllOtherThreads(hThread);

    // get thread context (hoping that some registers are ok :D)
    lpContext->ContextFlags=CONTEXT_ALL;
    ::GetThreadContext(hThread,lpContext);

    if (ProcessAndThreadID.GetProcessIdOfThread(hThread)==GetCurrentProcessId())
    {
        this->bLastGetThreadContextIsForCurrentProcess=TRUE;
        bRet=this->GetThreadContextOfCurrentProcess(hThread,Eip,&ContextLite);
    }
    else
    {
        this->bLastGetThreadContextIsForCurrentProcess=FALSE;
        // put a hook in remote process to retrieve real values of eax,ebx,ecx,edx,ebp,esp,edi,esi and efl
        bRet=this->GetThreadContextOfDifferentProcess(hThread,Eip,&ContextLite);
    }

    // Resume all other threads of the process are suspended
    CThreadContext::ResumeAllOtherThreads(hThread);

    // restore SuspendedCount thread value
    for(dwCnt=0;dwCnt<SuspendedCount;dwCnt++)
    {
        if (SuspendThread(hThread)==(DWORD)-1)
        {
            CAPIError::ShowLastError();
            return FALSE;
        }
    }
    if (ResumeThread(hThread)==(DWORD)-1)
    {
        CAPIError::ShowLastError();
        return FALSE;
    }

    // set context values
    if (bRet)
    {
        lpContext->Eax=ContextLite.EAX;
        lpContext->Ebx=ContextLite.EBX;
        lpContext->Ecx=ContextLite.ECX;
        lpContext->Edx=ContextLite.EDX;
        lpContext->Esp=ContextLite.ESP;
        lpContext->Eip=ContextLite.EIP;
        lpContext->Ebp=ContextLite.EBP;
        lpContext->Esi=ContextLite.ESI;
        lpContext->Edi=ContextLite.EDI;
        lpContext->EFlags=ContextLite.EFL;
    }

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: SuspendAllOtherThreads
// Object: suspend all threads with a different handle in the same process
// Parameters :
//     in  : HANDLE hThread : handle of the only thread of the process that won't be suspended
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CThreadContext::SuspendAllOtherThreads(HANDLE hThread)
{
    CProcessAndThreadID ProcessAndThreadID;
    HANDLE hOtherThread;
    DWORD dwProcessId=ProcessAndThreadID.GetProcessIdOfThread(hThread);
    DWORD dwThreadId=ProcessAndThreadID.GetThreadId(hThread);
    DWORD CurrentThreadId=GetCurrentThreadId();

    THREADENTRY32 te32 = {0}; 
    HANDLE hSnap;
    hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if (hSnap == INVALID_HANDLE_VALUE) 
    {
        CAPIError::ShowLastError();
        return FALSE; 
    }
    // Fill the size of the structure before using it. 
    te32.dwSize = sizeof(THREADENTRY32); 
 
    // Walk the thread list of the system
    if (!Thread32First(hSnap, &te32))
    {
        CloseHandle(hSnap);
        CAPIError::ShowLastError();
        return FALSE;
    }
    do 
    { 
        if (dwProcessId!=te32.th32OwnerProcessID)
            continue;
        if (dwThreadId==te32.th32ThreadID)
            continue;

        if (CurrentThreadId==te32.th32ThreadID)// for current process thread retrieval
            continue;

        hOtherThread=OpenThread(THREAD_ALL_ACCESS,FALSE,te32.th32ThreadID);
        if (SuspendThread(hOtherThread)==(DWORD)-1)
        {
            CAPIError::ShowLastError();
            CloseHandle(hOtherThread);
            CloseHandle (hSnap);
            return FALSE;
        }
        CloseHandle(hOtherThread);

    } 
    while (Thread32Next(hSnap, &te32)); 
 
    // clean up the snapshot object. 
    CloseHandle (hSnap); 

    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: ResumeAllOtherThreads
// Object: resume all threads with a different handle in the same process
// Parameters :
//     in  : HANDLE hThread : handle of the only thread of the process that won't be resumed
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CThreadContext::ResumeAllOtherThreads(HANDLE hThread)
{
    CProcessAndThreadID ProcessAndThreadID;
    HANDLE hOtherThread;
    DWORD dwProcessId=ProcessAndThreadID.GetProcessIdOfThread(hThread);
    DWORD dwThreadId=ProcessAndThreadID.GetThreadId(hThread);
    DWORD CurrentThreadId=GetCurrentThreadId();

    THREADENTRY32 te32 = {0}; 
    HANDLE hSnap;
    hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if (hSnap == INVALID_HANDLE_VALUE) 
    {
        CAPIError::ShowLastError();
        return FALSE; 
    }
    // Fill the size of the structure before using it. 
    te32.dwSize = sizeof(THREADENTRY32); 
 
    // Walk the thread list of the system
    if (!Thread32First(hSnap, &te32))
    {
        CloseHandle(hSnap);
        CAPIError::ShowLastError();
        return FALSE;
    }
    do 
    { 
        if (dwProcessId!=te32.th32OwnerProcessID)
            continue;
        if (dwThreadId==te32.th32ThreadID)
            continue;

        if (CurrentThreadId==te32.th32ThreadID)// for current process thread retrieval
            continue;

        hOtherThread=OpenThread(THREAD_ALL_ACCESS,FALSE,te32.th32ThreadID);
        if (ResumeThread(hOtherThread)==(DWORD)-1)
        {
            CAPIError::ShowLastError();
            CloseHandle(hOtherThread);
            CloseHandle (hSnap);
            return FALSE;
        }
        CloseHandle(hOtherThread);

    } 
    while (Thread32Next(hSnap, &te32)); 
 
    // clean up the snapshot object. 
    CloseHandle (hSnap); 

    return TRUE;
}

void __declspec( naked ) _cdecl CThreadContext::asm_memcpy(PBYTE Dest,PBYTE Source,DWORD ln)
{                           
    __asm 
    {
        push   ebp
        mov    ebp, esp
        sub    esp, __LOCAL_SIZE
        pushad

        //
        cld               
        mov esi, [Source] 
        mov edi, [Dest]   
        mov ecx, [ln]     
        
        shr ecx, 2        
        rep movsd         
        
        mov ecx, [ln]     
        and ecx, 3        
        rep movsb
        //

        popad
        mov      esp, ebp
        pop      ebp
        ret
    }
}

void __declspec( naked ) __stdcall CThreadContext::GetThreadContextInside(CThreadContext::CTHREADCONTEXT_GETTHREADCONTEXTOFCURRENTPROCESS_ARGS* pArgs)
{
    // DON'T CALL ANY FUNC BEFORE RESTORING ORIGINAL BYTES
    CThreadContext::CONTEXT_LITE ContextLite;
    PVOID EipToRestore;
    PBYTE pOriginaleBytes;
    PBYTE CurrentFuncEsp;
    HANDLE hEvent;
    __asm
    {
        push ebp
        mov  ebp, esp
        sub  esp, __LOCAL_SIZE

        // avoid pushad as popad restore ebp too
        push ebx
        push ecx
        push edx
        push esi
        push edi

        // debug purpose only cause watch sucks sometimes on naked func (at least on vs2005)
        // mov eax,pArgs
    }
    
    pOriginaleBytes=pArgs->OriginalBytes;
    EipToRestore=pArgs->EipToRestore;

    __asm
    {
        pop [ContextLite.EDI]
        pop [ContextLite.ESI]
        pop [ContextLite.EDX]
        pop [ContextLite.ECX]
        pop [ContextLite.EBX]

        mov [CurrentFuncEsp],esp
    }

    // restore original bytes (without calling func we don't know were eip is, so may in the memcpy func)
    CThreadContext::asm_memcpy((PBYTE)pArgs->EipToRestore,pArgs->OriginalBytes,CTHREADCONTEXT_GETTHREADCONTEXTOFCURRENTPROCESS_OPCODESIZE);

    __asm
    {
        mov esp , ebp
        pop [ContextLite.EBP]
        pop eax // remove return address from stack
        pop eax // remove pArgs from stack
        pop [ContextLite.EAX] // get eax
        pop [ContextLite.EFL] // get efl

        // here the stack is as the original one --> get esp
        mov [ContextLite.ESP],esp

        // restore local esp for calling func
        mov esp,[CurrentFuncEsp]
    }
    hEvent=pArgs->hEvent;
    memcpy(pArgs->pContextLite,&ContextLite,sizeof(CThreadContext::CONTEXT_LITE));
    SetEvent(hEvent);
    // suspend current thread to allow user to play with context before resuming thread
    SuspendThread(GetCurrentThread());
    CloseHandle(hEvent);
    __asm
    {
        // restore registers like they were
        mov eax, [ContextLite.EAX]
        mov ebx, [ContextLite.EBX]
        mov ecx, [ContextLite.ECX]
        mov edx, [ContextLite.EDX]
        mov esi, [ContextLite.ESI]
        mov edi, [ContextLite.EDI]
        mov esp, [ContextLite.ESP]

        // push return address
        push [EipToRestore]
        // restore ebp at least
        Mov ebp,[ContextLite.EBP] // from now you can't access your function local var

        Ret
    }
}
BOOL CThreadContext::GetThreadContextOfCurrentProcess(HANDLE hThreadHandle,PVOID Address,PCONTEXT_LITE pContextLite)
{
    // check params
    if (IsBadWritePtr(pContextLite,sizeof(CONTEXT_LITE)))
        return FALSE;

    // hThreadHandle is suspended and it's eip is Address
    PBYTE TmpAddress;
    CTHREADCONTEXT_GETTHREADCONTEXTOFCURRENTPROCESS_ARGS Args={0};
    Args.pContextLite=pContextLite;
    Args.EipToRestore=Address;
    Args.hEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
    pContextLite->EIP=(DWORD)Address;

    // for window app, you generally fall in the window message loop,which is entered only when receiving messages
    // so we can be lock by this stuff
    // To avoid such lock, we send a stupid message to force message loop entering (using PostMessage to don't wait for return)
    // Notice 1 : we could find the corresponding window and send it a message, but the easiest way is to send a broadcast message
    // Notice 2 : this tricks not always works as lock can result of WaitForMultipleObjects and other blocking calls
    this->UnlockWndProc();

    // save original bytes
    memcpy(Args.OriginalBytes,Address,CTHREADCONTEXT_GETTHREADCONTEXTOFCURRENTPROCESS_OPCODESIZE);

    DWORD OldProtectionFlag;
    // remove memory protection
    if (!VirtualProtectEx(GetCurrentProcess(),
                    Address,
                    CTHREADCONTEXT_GETTHREADCONTEXTOFCURRENTPROCESS_OPCODESIZE,
                    PAGE_EXECUTE_READWRITE,
                    &OldProtectionFlag))
        return FALSE;

    // replace opcodes by our own
    PBYTE pBuffer=(PBYTE)Address;
    int BufferIndex=0;
    // pushfd
    pBuffer[BufferIndex++]=0x9C;
    // push eax
    pBuffer[BufferIndex++]=0x50;

    // push &Args
    pBuffer[BufferIndex++]=0xB8; // mov eax,
    TmpAddress=(PBYTE)&Args;
    memcpy(&pBuffer[BufferIndex],&TmpAddress,sizeof(PBYTE));
    BufferIndex+=sizeof(PBYTE);
    pBuffer[BufferIndex++]=0x50;// push eax

    // call CThreadContext_GetThreadContextOfCurrentProcess
    pBuffer[BufferIndex++]=0xB8; // mov eax,
    TmpAddress=(PBYTE)CThreadContext::GetThreadContextInside;
    memcpy(&pBuffer[BufferIndex],&TmpAddress,sizeof(PBYTE));
    BufferIndex+=sizeof(PBYTE);
    // do a call not a jump, as func parameters must be at ebp-4
    // if no return address is pushed, parameters use won't work
    pBuffer[BufferIndex++]=0xFF;pBuffer[BufferIndex++]=0xD0; // call eax

    // resume hThreadHandle
    if (ResumeThread(hThreadHandle)==(DWORD)-1)
    {
        CAPIError::ShowLastError();
        // restore original opcodes
        memcpy((PBYTE)Args.EipToRestore,Args.OriginalBytes,CTHREADCONTEXT_GETTHREADCONTEXTOFCURRENTPROCESS_OPCODESIZE);
        return FALSE;
    }

    // wait for the end of context retrieval (10 sec max)
    DWORD dwRes=WaitForSingleObject(Args.hEvent,10000);

    if (dwRes!=WAIT_OBJECT_0)
    {
        // restore original opcodes
        memcpy((PBYTE)Args.EipToRestore,Args.OriginalBytes,CTHREADCONTEXT_GETTHREADCONTEXTOFCURRENTPROCESS_OPCODESIZE);
        return FALSE;
    }

    // restore memory protection
    VirtualProtectEx(GetCurrentProcess(),
                    Address,
                    CTHREADCONTEXT_GETTHREADCONTEXTOFCURRENTPROCESS_OPCODESIZE,
                    OldProtectionFlag,
                    &OldProtectionFlag);

    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: GetThreadContextOfDifferentProcess
// Object: inject code for the specified thread at the specified address
//          to retrieve eax,ebx,ecx,edx,ebp,esp,edi,esi and efl regiters values
// Parameters :
//     in  : HANDLE hThreadHandle : handle of suspended thread
//                                  FOR SECURITY ALL OTHER THREADS OF THE PROCESS MUST BE SUSPENDED
//           PVOID Address : address of thread's EIP
//     out : PCONTEXT_LITE pContextLite : pointer to CONTEXT_LITE struct
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CThreadContext::GetThreadContextOfDifferentProcess(HANDLE hThreadHandle,PVOID Address,PCONTEXT_LITE pContextLite)
{
    // check params
    if (IsBadWritePtr(pContextLite,sizeof(CONTEXT_LITE)))
        return FALSE;

    // for window app, you generally fall in the window message loop,which is entered only when receiving messages
    // so we can be lock by this stuff
    // To avoid such lock, we send a stupid message to force message loop entering (using PostMessage to don't wait for return)
    // Notice 1 : we could find the corresponding window and send it a message, but the easiest way is to send a broadcast message
    // Notice 2 : this tricks not always works as lock can result of WaitForMultipleObjects and other blocking calls
    this->UnlockWndProc();

    // retrieve process associated to thread
    CProcessAndThreadID ProcessAndThreadID;
    DWORD dwProcessId=ProcessAndThreadID.GetProcessIdOfThread(hThreadHandle);

    if(this->HookProcessMemoryAddress)// can appear if previous context query was done for a suspended thread
        delete this->HookProcessMemoryAddress;

    this->HookProcessMemoryAddress=new CProcessMemory(dwProcessId,FALSE);

    #define REGISTER_SIZE 4 // size of registers in byte (4 for 32bits)
    #define SIZEOF_HOOK_PROXY CTHREADCONTEXT_GETTHREADCONTEXTOFREMOTEPROCESS_OPCODESIZE // better to comput size for this 
    #define SIZEOF_HOOK 2000 // something enough (don't need to comput size)
    #define HOOK_END_POOLING_IN_MS 100
    #define HOOK_END_POOLING_MAX_IN_MS 5000
    DWORD dwHookEndFlag=0xBADCAFE;
    DWORD dwBeginTime;
    DWORD dwMaxWait;
    DWORD dw=0;

    // as kernel32 is always mapped at the same address, kernel32 func will get same addr in all processes
    FARPROC pGetCurrentThread=GetProcAddress(GetModuleHandle(_T("kernel32.dll")),"GetCurrentThread");
    FARPROC pSuspendThread=GetProcAddress(GetModuleHandle(_T("kernel32.dll")),"SuspendThread");

    SIZE_T dwTransferedSize=0;

    BYTE BufferIndex;

    BYTE LocalOriginalOpCode[SIZEOF_HOOK_PROXY];
    BYTE LocalProxy[SIZEOF_HOOK_PROXY];
    BYTE LocalHook[SIZEOF_HOOK];

    #define CONTEXT_SIZE (9*sizeof(PBYTE)) // size of this->HookRemoteContext in bytes

    PBYTE EntryPointAddress=(PBYTE)Address;

    // read original eip opcode
    if (!this->HookProcessMemoryAddress->Read(
                                            (LPVOID)EntryPointAddress,
                                            LocalOriginalOpCode,
                                            SIZEOF_HOOK_PROXY,
                                            &dwTransferedSize)
                                            )
        return FALSE;

    // allocate memory for the Hook
    this->HookRemoteHook=(PBYTE)this->HookProcessMemoryAddress->Alloc(SIZEOF_HOOK);
    if (!this->HookRemoteHook)
        return FALSE;

    // allocate memory in remote process to store context lite
    this->HookRemoteContext=(PBYTE)this->HookProcessMemoryAddress->Alloc(CONTEXT_SIZE);
    if (!this->HookRemoteContext)
        return FALSE;

    //// code for absolute jump if you don't want to make a relative one
    // #define SIZEOF_HOOK_PROXY 7
    // // jump Hook Address
    // LocalProxy[0]=0xB8;// mov eax,
    // memcpy(&LocalProxy[1],&RemoteHook,sizeof(DWORD));// Hook Address
    // LocalProxy[5]=0xFF;LocalProxy[6]=0xE0;//jmp eax absolute 

    // make a relative jump
    dw=(DWORD)(this->HookRemoteHook-EntryPointAddress-SIZEOF_HOOK_PROXY);
    // jump relative
    LocalProxy[0]=0xE9;
    memcpy(&LocalProxy[1],&dw,sizeof(DWORD));// Hook Address


    ///////////////////////
    // fill hook data
    // algorithm is the following :
    //
    //      ///////////////////////////////////////
    //      ///// specifics operations to do 
    //      ///////////////////////////////////////
    //
    //       // save context without affecting registers and flag registers
    //            push esp
    //            pushfd
    //            push ebx
    //            push eax
    //
    //            mov eax,RemoteContext
    //
    //            // get eax
    //            pop ebx
    //            mov dword ptr[eax],ebx
    //
    //            // get ebx
    //            pop ebx
    //            add eax,REGISTER_SIZE
    //            mov dword ptr[eax],ebx
    //
    //            // get ecx
    //            add eax,REGISTER_SIZE
    //            mov dword ptr[eax],ecx
    //
    //            // get edx
    //            add eax,REGISTER_SIZE
    //            mov dword ptr[eax],edx
    //
    //            // get esi
    //            add eax,REGISTER_SIZE
    //            mov dword ptr[eax],esi
    //
    //            // get edi
    //            add eax,REGISTER_SIZE
    //            mov dword ptr[eax],edi
    //
    //            // get efl
    //            pop ebx
    //            add eax,REGISTER_SIZE
    //            mov dword ptr[eax],ebx
    //
    //            // get ebp
    //            add eax,REGISTER_SIZE
    //            mov dword ptr[eax],ebp
    //
    //            // get esp
    //            pop ebx
    //            add eax,REGISTER_SIZE
    //            mov dword ptr[eax],ebx
    //
    //            // store efl again
    //            sub eax,2*REGISTER_SIZE
    //            push dword ptr[eax]
    //
    //            // push return address on stack
    //
    //            // restore ebx and eax using our local storage
    //
    //            // restore ebx
    //            mov eax,RemoteContext
    //            add eax,REGISTER_SIZE
    //            mov ebx,dword ptr[eax]
    //
    //            // restore eax
    //            sub eax,REGISTER_SIZE
    //            mov eax,dword ptr[eax]
    //
    //            // restore flags
    //            popfd
    //
    //      ///////////////////////////////////////
    //      ///// End of specifics operations to do 
    //      ///////////////////////////////////////
    //
    //      // save registers and flag registers
    //
    //
    //      // do some action that can tell the calling process that the hook is ending
    //      // so we can restore original opcode
    //      // and free memory
    //
    //      // suspend thread to allow user context use
    //
    //      // restore registers and flag registers
    //
    //      // jump to Entry point
    //
    ///////////////////////

    BufferIndex=0;

    ////////////////////////////////
    // 1) SaveContext
    ////////////////////////////////

    // push esp
    LocalHook[BufferIndex++]=0x54;
    //pushfd
    LocalHook[BufferIndex++]=0x9C;
    //push ebx
    LocalHook[BufferIndex++]=0x53;
    //push eax
    LocalHook[BufferIndex++]=0x50;

    // lea RemoteContext --> mov eax,&RemoteContext
    LocalHook[BufferIndex++]=0xB8; // mov eax,
    memcpy(&LocalHook[BufferIndex],&this->HookRemoteContext,sizeof(DWORD));
    BufferIndex+=sizeof(DWORD);

    ///////////
    // get eax
    ///////////

    // pop ebx
    LocalHook[BufferIndex++]=0x5B;
    // mov dword ptr[eax],ebx
    LocalHook[BufferIndex++]=0x89;LocalHook[BufferIndex++]=0x18;

    ///////////
    // get ebx
    ///////////

    // pop ebx
    LocalHook[BufferIndex++]=0x5B;
    //add eax,REGISTER_SIZE
    LocalHook[BufferIndex++]=0x83;LocalHook[BufferIndex++]=0xC0;LocalHook[BufferIndex++]=REGISTER_SIZE;
    // mov dword ptr[eax],ebx
    LocalHook[BufferIndex++]=0x89;LocalHook[BufferIndex++]=0x18;

    ///////////
    // get ecx
    ///////////

    //add eax,REGISTER_SIZE
    LocalHook[BufferIndex++]=0x83;LocalHook[BufferIndex++]=0xC0;LocalHook[BufferIndex++]=REGISTER_SIZE;
    // mov dword ptr[eax],ecx
    LocalHook[BufferIndex++]=0x89;LocalHook[BufferIndex++]=0x08;

    ///////////
    // get edx
    ///////////

    //add eax,REGISTER_SIZE
    LocalHook[BufferIndex++]=0x83;LocalHook[BufferIndex++]=0xC0;LocalHook[BufferIndex++]=REGISTER_SIZE;
    // mov dword ptr[eax],edx
    LocalHook[BufferIndex++]=0x89;LocalHook[BufferIndex++]=0x10;

    ///////////
    // get esi
    ///////////

    // add eax,REGISTER_SIZE
    LocalHook[BufferIndex++]=0x83;LocalHook[BufferIndex++]=0xC0;LocalHook[BufferIndex++]=REGISTER_SIZE;
    // mov dword ptr[eax],esi
    LocalHook[BufferIndex++]=0x89;LocalHook[BufferIndex++]=0x30;

    ///////////
    // get edi
    ///////////

    // add eax,REGISTER_SIZE
    LocalHook[BufferIndex++]=0x83;LocalHook[BufferIndex++]=0xC0;LocalHook[BufferIndex++]=REGISTER_SIZE;
    // mov dword ptr[eax],edi
    LocalHook[BufferIndex++]=0x89;LocalHook[BufferIndex++]=0x38;

    ///////////
    // get efl
    ///////////

    // pop ebx
    LocalHook[BufferIndex++]=0x5B;
    // add eax,REGISTER_SIZE
    LocalHook[BufferIndex++]=0x83;LocalHook[BufferIndex++]=0xC0;LocalHook[BufferIndex++]=REGISTER_SIZE;
    // mov dword ptr[eax],ebx
    LocalHook[BufferIndex++]=0x89;LocalHook[BufferIndex++]=0x18;

    ///////////
    // get ebp
    ///////////

    // add eax,REGISTER_SIZE
    LocalHook[BufferIndex++]=0x83;LocalHook[BufferIndex++]=0xC0;LocalHook[BufferIndex++]=REGISTER_SIZE;
    // mov dword ptr[eax],ebp
    LocalHook[BufferIndex++]=0x89;LocalHook[BufferIndex++]=0x28;

    ///////////
    // get esp
    ///////////

    // pop ebx
    LocalHook[BufferIndex++]=0x5B;
    // add eax,REGISTER_SIZE
    LocalHook[BufferIndex++]=0x83;LocalHook[BufferIndex++]=0xC0;LocalHook[BufferIndex++]=REGISTER_SIZE;
    // mov dword ptr[eax],ebx
    LocalHook[BufferIndex++]=0x89;LocalHook[BufferIndex++]=0x18;

    ////////////////////////////////////////////
    // push return address on stack
    ////////////////////////////////////////////
    LocalHook[BufferIndex++]=0xBB;//mov ebx, 
    memcpy(&LocalHook[BufferIndex],&EntryPointAddress,sizeof(DWORD));// dwEntryPointAddress
    BufferIndex+=sizeof(DWORD);
    // push eax
    LocalHook[BufferIndex++]=0x53;// push ebx

    ///////////////////
    // push saved efl on stack
    ///////////////////

    // sub eax,2*REGISTER_SIZE
    LocalHook[BufferIndex++]=0x83;LocalHook[BufferIndex++]=0xE8;LocalHook[BufferIndex++]=2*REGISTER_SIZE;
    // push dword ptr[eax]
    LocalHook[BufferIndex++]=0xFF;LocalHook[BufferIndex++]=0x30;

    ////////////////////////////////////////////
    // restore ebx and eax using our local storage
    /////////////////////////////////////////////

    ////////////////
    // restore ebx
    ////////////////

    // lea RemoteContext --> mov eax,&RemoteContext
    LocalHook[BufferIndex++]=0xB8; // mov eax,
    memcpy(&LocalHook[BufferIndex],&this->HookRemoteContext,sizeof(DWORD));
    BufferIndex+=sizeof(DWORD);


    // add eax,REGISTER_SIZE
    LocalHook[BufferIndex++]=0x83;LocalHook[BufferIndex++]=0xC0;LocalHook[BufferIndex++]=REGISTER_SIZE;
    // mov ebx,dword ptr[eax]
    LocalHook[BufferIndex++]=0x8B;LocalHook[BufferIndex++]=0x18;

    ///////////////
    // restore eax
    ///////////////

    // sub eax,REGISTER_SIZE
    LocalHook[BufferIndex++]=0x83;LocalHook[BufferIndex++]=0xE8;LocalHook[BufferIndex++]=REGISTER_SIZE;
    // mov eax,dword ptr[eax]
    LocalHook[BufferIndex++]=0x8B;LocalHook[BufferIndex++]=0x00;

    ////////////////
    // restore flags
    ////////////////

    // popfd
    LocalHook[BufferIndex++]=0x9D;

    // at this point all registers are in the same state
    // as the begin of hook

    ////////////////////////////////
    // 2) restoring context
    ////////////////////////////////

    // save registers and flag registers
    LocalHook[BufferIndex++]=0x60;//pushad
    LocalHook[BufferIndex++]=0x9c;//pushfd

    //////////////////////////////////////////////////////////////////////
    //do some action that can tell the calling process that the hook is ending
    //////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////
    // here we change some remotely allocated memory to signal end of hook
    // it's allow for remote process to do polling on this memory pointer
    // (Notice you can use Named event or whatever you want if you dislike
    // this way of doing)
    //
    //  so here we use the begin of RemoteHook and put first DWORD to dwHookEndFlag
    //////////////////////////////////////////////////////////////////////

    // mov eax,RemoteHook
    LocalHook[BufferIndex++]=0xB8;// mov eax,
    memcpy(&LocalHook[BufferIndex],&this->HookRemoteHook,sizeof(DWORD));
    BufferIndex+=sizeof(DWORD);

    // mov ebx,dwHookEndFlag
    LocalHook[BufferIndex++]=0xBB;// mov ebx,
    memcpy(&LocalHook[BufferIndex],&dwHookEndFlag,sizeof(DWORD));
    BufferIndex+=sizeof(DWORD);

    // *RemoteHook=dwHookEndFlag
    LocalHook[BufferIndex++]=0x89;LocalHook[BufferIndex++]=0x18;// mov dword ptr[eax],ebx


    //////////////////////////////////////////////////////////////////////
    // suspend thread 
    //////////////////////////////////////////////////////////////////////

    // mov eax,pGetCurrentThread
    LocalHook[BufferIndex++]=0xB8;// mov eax,
    memcpy(&LocalHook[BufferIndex],&pGetCurrentThread,sizeof(DWORD));
    BufferIndex+=sizeof(DWORD);

    // call GetCurrentThread
    LocalHook[BufferIndex++]=0xFF;LocalHook[BufferIndex++]=0xD0; // call eax

    // we are in stdcall --> parameters are removed from stack

    // push eax (contains the thread handle
    LocalHook[BufferIndex++]=0x50;// push eax

    // mov eax,pSuspendThread
    LocalHook[BufferIndex++]=0xB8;// mov eax,
    memcpy(&LocalHook[BufferIndex],&pSuspendThread,sizeof(DWORD));
    BufferIndex+=sizeof(DWORD);

    // call SuspendThread
    LocalHook[BufferIndex++]=0xFF;LocalHook[BufferIndex++]=0xD0; // call eax


    // we are in stdcall --> parameters are removed from stack


    //////////////////////////////////////////////////////////////////////
    // restore registers and flag registers
    //////////////////////////////////////////////////////////////////////
    LocalHook[BufferIndex++]=0x9D;//popfd
    LocalHook[BufferIndex++]=0x61;//popad

    //////////////////////////////////////////////////////////////////////
    // remember that return address is on stack
    // so jump to EntryPointAddress using  Ret
    //////////////////////////////////////////////////////////////////////
    LocalHook[BufferIndex++]=0xC3;//ret

    // copy hook data
    if (!this->HookProcessMemoryAddress->Write(
                                                (LPVOID)this->HookRemoteHook,
                                                LocalHook,
                                                SIZEOF_HOOK,
                                                &dwTransferedSize)
                                                )
    {
        return FALSE;
    }

    DWORD OldProtectionFlag;
    // mark allocated memory has executable
    if (!VirtualProtectEx(this->HookProcessMemoryAddress->GetProcessHandle(),
                    this->HookRemoteHook,
                    SIZEOF_HOOK,
                    PAGE_EXECUTE_READWRITE,
                    &OldProtectionFlag))
        return FALSE;



    if (!VirtualProtectEx(this->HookProcessMemoryAddress->GetProcessHandle(),
                    EntryPointAddress,
                    SIZEOF_HOOK_PROXY,
                    PAGE_EXECUTE_READWRITE,
                    &OldProtectionFlag))
        return FALSE;

    // copy proxy data (assume that our hook is in remote process before jumping to it)
    if (!this->HookProcessMemoryAddress->Write(
                                                (LPVOID)EntryPointAddress,
                                                LocalProxy,
                                                SIZEOF_HOOK_PROXY,
                                                &dwTransferedSize)
                                                )
    {
        return FALSE;
    }

    // resume thread a first time to run our hook
    if(ResumeThread(hThreadHandle)==((DWORD)-1))
        return FALSE;

    dwMaxWait=HOOK_END_POOLING_MAX_IN_MS;
    dwBeginTime=GetTickCount();

    // wait until hook has done it's job --> check the memory flags put at the begin of RemoteHook
    for(;;)
    {

        Sleep(HOOK_END_POOLING_IN_MS);
        // read remote process memory to check memory flags
        if (!this->HookProcessMemoryAddress->Read( this->HookRemoteHook,&dw,sizeof(DWORD),&dwTransferedSize))
            return FALSE;

        // if hooking as finished
        if (dw==dwHookEndFlag)
            break;

        // if process is crashed or stopped don't wait anymore
        if (!CProcessHelper::IsAlive(dwProcessId))
        {
#ifndef TOOLS_NO_MESSAGEBOX
            TCHAR pszMsg[2*MAX_PATH];
            _stprintf(pszMsg,_T("Error application seems to be closed"));
            MessageBox(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
#endif
            return FALSE;
        }

        // assume we are not in infinite loop
        if (GetTickCount()-dwBeginTime>HOOK_END_POOLING_MAX_IN_MS)
        {
#ifndef TOOLS_NO_MESSAGEBOX
            TCHAR pszMsg[2*MAX_PATH];
            _stprintf(pszMsg,_T("Thread is currently inside a system function.\r\n")
                             _T("The next EIP inside the application will be 0x%p.\r\n")
                             _T("Do you want to wait %us more for trying to retrieve other registers values ?"),
                             EntryPointAddress,
                             dwMaxWait*2/1000);
            if (MessageBox(NULL,pszMsg,_T("Question"),MB_YESNO|MB_ICONQUESTION|MB_TOPMOST)==IDYES)
            {
                // increase the wait time at each MsgBox
                dwMaxWait*=2;
                // reset begin time to wait dwMaxWait more time
                dwBeginTime=GetTickCount();
            }
            else
#endif
            {
                // restore originals opcode to avoid crash (in case of memory freeing) at the end of the lock state
                if (!this->HookProcessMemoryAddress->Write(
                                                            (LPVOID)EntryPointAddress,
                                                            LocalOriginalOpCode,
                                                            SIZEOF_HOOK_PROXY,
                                                            &dwTransferedSize)
                                                            )
                {
#ifndef TOOLS_NO_MESSAGEBOX
                    // better to terminate thread
                    if (MessageBox(NULL,_T("To avoid process crash it's better to terminate it now.\r\n")
                                        _T("Do you want to terminate it ?"),_T("Question"),MB_YESNO|MB_ICONQUESTION|MB_TOPMOST)==IDYES)
#endif
                    {
                        HANDLE hProcess=OpenProcess(PROCESS_ALL_ACCESS,FALSE,dwProcessId);
                        TerminateProcess(hProcess,(DWORD)-1);
                        CloseHandle(hProcess);
                    }
                    return FALSE;
                }
                return FALSE;
            }
        }
    }

    // restore original opcodes
    if (!this->HookProcessMemoryAddress->Write((LPVOID)EntryPointAddress,
                                                LocalOriginalOpCode,
                                                SIZEOF_HOOK_PROXY,
                                                &dwTransferedSize
                                               )
        )
        return FALSE;
    
    // restore memory protection
    if (!VirtualProtectEx(this->HookProcessMemoryAddress->GetProcessHandle(),
                            EntryPointAddress,
                            SIZEOF_HOOK_PROXY,
                            OldProtectionFlag,
                            &OldProtectionFlag)
        )
        return FALSE;

    // get context result
    if (!this->HookProcessMemoryAddress->Read( this->HookRemoteContext,pContextLite,sizeof(CONTEXT_LITE),&dwTransferedSize))
        return FALSE;

    pContextLite->EIP=(DWORD)Address;

    return TRUE;
}

BOOL CThreadContext::HasProcessMemoryBeenFree()
{
    if (this->bLastGetThreadContextIsForCurrentProcess)
    {

    }
    else
    {
        if (this->HookProcessMemoryAddress!=NULL)
            return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetThreadContextFree
// Object: free memory allocated by GetThreadContext
//          MUST BE CALLED ONLY AFTER CALLER HAS FULLY RESUMED THREAD
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CThreadContext::GetThreadContextFree()
{
    if (this->bLastGetThreadContextIsForCurrentProcess)
    {
        return TRUE;
    }
    else
    {
        return this->GetThreadContextFreeContextOfDifferentProcess();
    }
}

//-----------------------------------------------------------------------------
// Name: GetThreadContextFreeContextOfDifferentProcess
// Object: free memory allocated in remote process 
//          after a call of GetThreadContextOfDifferentProcess
//          MUST BE CALLED ONLY AFTER CALLER HAS FULLY RESUME THREAD
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CThreadContext::GetThreadContextFreeContextOfDifferentProcess()
{
    // don't use IsBadWritePointer here because this is remote process allocated memory
    if (this->HookProcessMemoryAddress==NULL)
        return FALSE;


    // wait a little to assume process don't need allocated memory anymore (only to be sure that the 3 asm instructions
    // required after the ResumeProcess are executed)
    Sleep(100);

    // free memory
    if (this->HookRemoteHook)
    {
        this->HookProcessMemoryAddress->Free(this->HookRemoteHook);
        this->HookRemoteHook=NULL;
    }
    if (this->HookRemoteContext)
    {
        this->HookProcessMemoryAddress->Free(this->HookRemoteContext);
        this->HookRemoteContext=NULL;
    }   
    
    if (this->HookProcessMemoryAddress)
    {
        delete this->HookProcessMemoryAddress;
        this->HookProcessMemoryAddress=NULL;
    }


    return TRUE;
}