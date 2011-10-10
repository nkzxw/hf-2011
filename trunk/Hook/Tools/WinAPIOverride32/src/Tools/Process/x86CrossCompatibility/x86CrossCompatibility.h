#pragma once


#include <Windows.h>
#include <Tlhelp32.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "x86CrossCompatibilityInterProcessCom.h"
#include "../MailSlot/MailSlotClient.h"
#include "../MailSlot/MailSlotServer.h"

#ifndef __specstrings
	#define __in
	#define __out
	#define __inout
	#define __in_opt
#endif


namespace x86CrossCompatibility
{

class Cx86CrossCompatibility
{
public:
	Cx86CrossCompatibility(TCHAR* x86CrossCompatibilityBinPath);
	virtual ~Cx86CrossCompatibility(void);

	HANDLE64 WINAPI CreateToolhelp32Snapshot(
		__in  DWORD dwFlags,
		__in  DWORD th32ProcessID
		);

	BOOL WINAPI Module32First(
		__in     HANDLE64 hSnapshot,
		__inout  LPMODULEENTRY3264 lpme
		);

	BOOL WINAPI Module32Next(
		__in   HANDLE64 hSnapshot,
		__out  LPMODULEENTRY3264 lpme
		);

	BOOL WINAPI CloseHandle(
		__in  HANDLE64 hObject
		);

	BOOL WINAPI VirtualProtectEx(
		__in   HANDLE64 hProcess,
		__in   __int64 lpAddress,
		__in   __int64 dwSize,
		__in   DWORD flNewProtect,
		__out  PDWORD lpflOldProtect
		);

	__int64 WINAPI VirtualQueryEx(
		__in      HANDLE64 hProcess,
		__in_opt  __int64 lpAddress,
		__out     PMEMORY_BASIC_INFORMATION64 lpBuffer,
		__in      __int64 dwLength
		);

	BOOL WINAPI ReadProcessMemory(
		__in   HANDLE64 hProcess,
		__in   __int64 lpBaseAddress,
		__out  LPVOID lpBuffer,
		__in   __int64 nSize,
		__out  __int64 *lpNumberOfBytesRead
		);

	BOOL WINAPI WriteProcessMemory(
		__in   HANDLE64 hProcess,
		__in   __int64 lpBaseAddress,
		__in   LPCVOID lpBuffer,
		__in   __int64 nSize,
		__out  __int64* lpNumberOfBytesWritten
		);
	HANDLE64 WINAPI OpenProcess(
		__in  DWORD dwDesiredAccess,
		__in  BOOL bInheritHandle,
		__in  DWORD dwProcessId
		);

    LONG WINAPI NtQueryInformationProcess(
        IN HANDLE64 ProcessHandle,
        IN __int64 ProcessInformationClass,
        OUT PVOID ProcessInformation,
        IN ULONG ProcessInformationLength,
        OUT PULONG ReturnLength OPTIONAL
        );

protected:
	HANDLE hProcess;
	CRITICAL_SECTION CriticalSection;
    CMailSlotClient* pMailSlotQuery;
    CMailSlotServer* pMailSlotReply;
    static void MailSlotCallback(PVOID pData,DWORD dwDataSize,PVOID pUserData);

    PVOID MailSlotServerData;
    DWORD MailSlotServerDataSize;
    HANDLE MailSlotServerDataArrival;
    HANDLE MailSlotServerReleaseData;

};

}
