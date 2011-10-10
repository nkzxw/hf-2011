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

#pragma once

#include <windows.h>
#include "../APIError/APIError.h"
#include "../Process/ProcessAndThreadID/ProcessAndThreadID.h"
#include "../Process/Memory/ProcessMemory.h"
#include "../Process/ProcessHelper/ProcessHelper.h"

class CThreadContext
{
private:
    #define CTHREADCONTEXT_GETTHREADCONTEXTOFCURRENTPROCESS_OPCODESIZE 15
    #define CTHREADCONTEXT_GETTHREADCONTEXTOFREMOTEPROCESS_OPCODESIZE   5
    typedef struct tagCONTEXT_LITE
    {
        DWORD EAX;
        DWORD EBX;
        DWORD ECX;
        DWORD EDX;
        DWORD ESI;
        DWORD EDI;
        DWORD EFL;
        DWORD EBP;
        DWORD ESP;
        DWORD EIP;
    }CONTEXT_LITE,*PCONTEXT_LITE;

    typedef struct tagCThreadContext_GetThreadContextOfCurrentProcessArgs
    {
        PCONTEXT_LITE pContextLite;
        PVOID EipToRestore;
        BYTE  OriginalBytes[CTHREADCONTEXT_GETTHREADCONTEXTOFCURRENTPROCESS_OPCODESIZE];
        HANDLE hEvent;
    }CTHREADCONTEXT_GETTHREADCONTEXTOFCURRENTPROCESS_ARGS;

    BOOL  bLastGetThreadContextIsForCurrentProcess;
    PBYTE HookRemoteHook;
    PBYTE HookRemoteContext;
    HMODULE User32hModule;
    typedef BOOL (__stdcall *pfPostMessage)(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
    pfPostMessage pPostMessage;

    CProcessMemory* HookProcessMemoryAddress;

    BOOL GetThreadContextFreeContextOfDifferentProcess();
    BOOL HasProcessMemoryBeenFree();
    BOOL GetThreadContextOfCurrentProcess(HANDLE hThreadHandle,PVOID Address,PCONTEXT_LITE pContextLite);
    BOOL GetThreadContextOfDifferentProcess(HANDLE hThreadHandle,PVOID Address,PCONTEXT_LITE pContextLite);
    static DWORD WINAPI CThreadContextTestThreadProc(LPVOID lpParameter);
    static void __stdcall GetThreadContextInside(CTHREADCONTEXT_GETTHREADCONTEXTOFCURRENTPROCESS_ARGS* pArgs);
public:
    CThreadContext(void);
    ~CThreadContext(void);
    BOOL GetThreadEip(HANDLE hThread,PBYTE* pEip);
    BOOL GetSuspendedThreadEip(HANDLE hSuspendedThread,PBYTE* pEip);
    BOOL GetSuspendedThreadEipEbp(HANDLE hSuspendedThread,PBYTE* pEip,PBYTE* pEbp);
    BOOL GetFirstHookableEipEbp(HANDLE hSuspendedThread,PBYTE* pEip,PBYTE* pEbp);
    BOOL GetFirstHookableEip(HANDLE hSuspendedThread,PBYTE* pEip);
    BOOL GetPreviousEipEbp(HANDLE hSuspendedThread,PBYTE CurrentEip,PBYTE CurrentEbp,PBYTE* pEip,PBYTE* pEbp);
    BOOL GetThreadContext(HANDLE hThread,LPCONTEXT lpContext);
    BOOL GetThreadContextFree();

    static BOOL ResumeAllOtherThreads(HANDLE hThread);
    static BOOL SuspendAllOtherThreads(HANDLE hThread); 
    static void _cdecl asm_memcpy(PBYTE Dest,PBYTE Source,DWORD ln);
    void UnlockWndProc();
};
