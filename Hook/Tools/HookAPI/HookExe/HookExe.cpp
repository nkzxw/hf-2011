// HookExe.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdarg.h>
#include <string.h>
#include <TlHelp32.h> 

void injectCode ()
{
	char *pwchProsess = "mmc.exe";
	char *pchDll = "E:\\Work\\HookAPI\\Debug\\HookDll.dll";
	

	HANDLE hToken;
	if (!OpenProcessToken (GetCurrentProcess (), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		OutputDebugString ("OpenProcessToken Error.\n");
		return;
	}

	LUID luid;
	if (!LookupPrivilegeValue (NULL, SE_DEBUG_NAME,&luid))
	{
		OutputDebugString ("LookupPrivilegeValue Error.\n");
		return;
	}

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	tp.Privileges[0].Luid = luid;

	if (!AdjustTokenPrivileges (hToken, 0, &tp, sizeof (TOKEN_PRIVILEGES), NULL, NULL))
	{
		OutputDebugString ("AdjustTokenPrivileges Error.\n");
		return;
	}

	HANDLE hSnap;
	HANDLE hkernel32;
	PROCESSENTRY32 pe;
	BOOL bNext;

	pe.dwSize = sizeof (pe);
	hSnap = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
	bNext = Process32First (hSnap, &pe);
	while (bNext)
	{
		if (!strcmp (pe.szExeFile, pwchProsess)){
			hkernel32 = OpenProcess (PROCESS_CREATE_THREAD | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,FALSE, pe.th32ProcessID);
			OutputDebugString ("Hookapi: OpenProcess.\n");
			break;
		}
		bNext = Process32Next (hSnap, &pe);
	}

	CloseHandle  (hSnap);

	LPVOID lpAddr;
	FARPROC pfn;

	lpAddr = VirtualAllocEx (hkernel32, NULL, strlen (pchDll), MEM_COMMIT, PAGE_READWRITE);

	WriteProcessMemory (hkernel32, lpAddr, pchDll, strlen(pchDll), NULL);

	pfn = GetProcAddress (GetModuleHandleW(L"kernel32.dll"), "LoadLibraryA");

	CreateRemoteThread(hkernel32,NULL,0,(LPTHREAD_START_ROUTINE)pfn,lpAddr,NULL,0);

	CloseHandle (hkernel32);
}


int _tmain(int argc, _TCHAR* argv[])
{
	injectCode (); 

	return 0;
}

