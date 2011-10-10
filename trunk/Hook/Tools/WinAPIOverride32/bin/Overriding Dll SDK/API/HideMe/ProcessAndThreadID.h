/*

Thanks to A. Miguel Feijao

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
// Object: retrieve process or thread Id from process or thread handle
//-----------------------------------------------------------------------------

#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <Winternl.h>

#include "ProcessAndThreadIDStruct.h"
#include "APIError.h"
typedef NTSTATUS (WINAPI *ptrNtQueryInformationThread) (
        IN HANDLE ThreadHandle,
        IN THREADINFOCLASS ThreadInformationClass,
        OUT PVOID ThreadInformation,
        IN ULONG ThreadInformationLength,
        OUT PULONG ReturnLength OPTIONAL
        );
typedef NTSTATUS (WINAPI *ptrNtQueryInformationProcess) (
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );
typedef ULONG (WINAPI *ptrRtlNtStatusToDosError)(NTSTATUS Status);

typedef DWORD (WINAPI *ptrGetProcessIdOfThread)(HANDLE Thread);
typedef DWORD (WINAPI *ptrGetThreadId)(HANDLE Thread);
typedef DWORD (WINAPI *ptrGetProcessId)(HANDLE Process);

class CProcessAndThreadID
{
private:
    HMODULE hModuleNtDll;
    HMODULE hModuleKernel32;
    ptrNtQueryInformationThread pNtQueryInformationThread;
    ptrRtlNtStatusToDosError pRtlNtStatusToDosError;
    ptrNtQueryInformationProcess pNtQueryInformationProcess;

    ptrGetProcessIdOfThread pGetProcessIdOfThread;
    ptrGetThreadId pGetThreadId;
    ptrGetProcessId pGetProcessId;

    DWORD GetProcessIdOfThreadOldOS(HANDLE hThread);
    DWORD GetThreadIdOldOS(HANDLE hThread);
    DWORD GetProcessIdOldOS(HANDLE hProcess);
public:
    CProcessAndThreadID();
    ~CProcessAndThreadID();

    DWORD GetProcessIdOfThread(HANDLE hThread);
    DWORD GetThreadId(HANDLE hThread);
    DWORD GetProcessId(HANDLE hProcess);
};
