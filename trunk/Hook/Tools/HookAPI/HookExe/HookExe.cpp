// HookExe.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <process.h> 
#include <stdarg.h>
#include <string.h>
#include <TlHelp32.h> 
#include "..\sys\ProcessMonitor.h"

HANDLE g_hDriver = 0;
HANDLE g_hProcessEvent = 0;
DWORD g_dwThreadId = 0;

typedef unsigned (__stdcall *PTHREAD_START) (void *);

unsigned __stdcall ThreadFunc(void* pvParam);


static void OutputDebugPrintf(const char * strOutputString,...)
{
	char strBuffer[4096]={0};
	va_list vlArgs;
	va_start(vlArgs,strOutputString);
	_vsnprintf(strBuffer,sizeof(strBuffer)-1,strOutputString,vlArgs);
	va_end(vlArgs);
	OutputDebugStringA(strBuffer);
}


static BOOL WINAPI IsWindowsNT4() 
{
   OSVERSIONINFO vi = { sizeof(vi) };

   ::GetVersionEx(&vi);
   
   return ( (vi.dwPlatformId == VER_PLATFORM_WIN32_NT) && 
	        (vi.dwMajorVersion == 4) );
}

void injectCode (DWORD dwProcessId)
{

	char fullPath[MAX_PATH]; // MAX_PATH
	GetModuleFileName (NULL,fullPath,MAX_PATH);

	char* pchTempPath = fullPath; 
	while(strchr(pchTempPath,'\\'))
	{
		 pchTempPath = strchr(pchTempPath,'\\');
		 pchTempPath++;
	}		 
	*pchTempPath = '\0';

	char pchDllPath[MAX_PATH];
	sprintf(pchDllPath,_T("%s%s"),fullPath,"HookDll.dll");

	char *pchDll = pchDllPath;

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

	HANDLE hkernel32;

	if (0 == dwProcessId) {
		char *pwchProsess = "mmc.exe";
		HANDLE hSnap;	
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
	}
	else {
		hkernel32 = OpenProcess (PROCESS_CREATE_THREAD | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,FALSE, dwProcessId);
	}

	LPVOID lpAddr;
	FARPROC pfn;

	lpAddr = VirtualAllocEx (hkernel32, NULL, strlen (pchDll), MEM_COMMIT, PAGE_READWRITE);

	DWORD dwError1 = GetLastError ();

	WriteProcessMemory (hkernel32, lpAddr, pchDll, strlen(pchDll), NULL);

	DWORD dwError2 = GetLastError ();

	pfn = GetProcAddress (GetModuleHandleW(L"kernel32.dll"), "LoadLibraryA");

	//CreateRemoteThread(hkernel32,NULL,0,(LPTHREAD_START_ROUTINE)pfn,lpAddr,NULL,0);

	//
	//循环防止mmc进程启动时加载，导致GetLastError () == 8 的错误。
	//
	DWORD count = 50;
	while (count) {
		DWORD dwThreadId;
		HANDLE hThread = CreateRemoteThread(hkernel32,NULL,0,(LPTHREAD_START_ROUTINE)pfn,lpAddr,NULL,0);
		if (0 == hThread){
			count--;
			::Sleep (1);
			continue;
		}
		//WaitForSingleObject (hThread, INFINITE);
		break;
	}

	CloseHandle (hkernel32);
}

DWORD RetrieveProcessInfo()
{
	OVERLAPPED ov          = { 0 };
	BOOL       bReturnCode = FALSE;
	DWORD      dwBytesReturned;

    ov.hEvent = ::CreateEvent(NULL,  
							TRUE,  
							FALSE, 
							NULL);

	DWORD dwProcessId;

	bReturnCode = ::DeviceIoControl(g_hDriver,
								IOCTL_PROCESSMONITOR_GET_PROCINFO,
								0, 
								0,
								&dwProcessId, sizeof(DWORD),
								&dwBytesReturned,
								&ov
								);

	::CloseHandle(ov.hEvent);

	return dwProcessId;
}


unsigned __stdcall ThreadFunc(void* pvParam)
{
	DWORD dwResult;

	while (TRUE)
	{
		dwResult = WaitForSingleObject (g_hProcessEvent, INFINITE);
		
		//dwResult = ::WaitForMultipleObjects(
		//	sizeof(handles)/sizeof(handles[0]), // number of handles in array
		//	&handles[0],                        // object-handle array
		//	FALSE,                              // wait option
		//	INFINITE                            // time-out interval
		//	);

		//if (handles[dwResult - WAIT_OBJECT_0] == me->Get_ShutdownEvent())
		//	break;
		//else
		//	me->RetrieveProcessInfo(
		//		callbackInfo, 
		//		callbackTemp
		//		);

		DWORD dwProcessId = RetrieveProcessInfo ();
		injectCode (dwProcessId);

		printf ("OK");
	} // while

	_endthreadex(0);
	
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	//injectCode (0);
	//return 0;

	char szDriverName[MAX_PATH];
	if ( IsWindowsNT4() )
		strcpy(szDriverName, DOS_NAME);
	else
		strcpy(szDriverName, GLOBAL_DOS_NAME);				

	g_hDriver = CreateFile(szDriverName,
						GENERIC_READ | GENERIC_WRITE, 
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						0,                     
						OPEN_EXISTING,
						FILE_FLAG_OVERLAPPED,  
						0);    
	if (g_hDriver == INVALID_HANDLE_VALUE ){
		//TODO
		return 0;
	}

	g_hProcessEvent = ::OpenEvent(SYNCHRONIZE, FALSE, "MonitorProcessEvent");

	HANDLE hThread = (HANDLE)_beginthreadex((void *)NULL,
											(unsigned)0,
											(PTHREAD_START)(ThreadFunc),
											 NULL,
											(unsigned)0,
											(unsigned *)&g_dwThreadId
											); 

	WaitForSingleObject (hThread, INFINITE);

	return 0;
}

