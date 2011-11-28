// InjectDll.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"  
#include "stdio.h"  
#include "tchar.h"  
#include <WtsApi32.h>
#include <UserEnv.h>
#include <tlhelp32.h>
#pragma comment(lib,"WtsApi32.lib")
#pragma comment(lib,"UserEnv.lib")

#pragma comment(lib,"Advapi32.lib")  
BOOL SetPrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)   
{  
    TOKEN_PRIVILEGES tp;  
    HANDLE hToken;  
    LUID luid;  
    if( !OpenProcessToken(GetCurrentProcess(),  
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,   
                          &hToken) )  
    {  
        _tprintf("OpenProcessToken error: %u/n", GetLastError());  
        return FALSE;  
    }  
    if( !LookupPrivilegeValue(NULL,  
                              lpszPrivilege,  
                              &luid) )  
    {  
        _tprintf("LookupPrivilegeValue error: %u/n", GetLastError() );   
        return FALSE;   
    }  
    tp.PrivilegeCount = 1;  
    tp.Privileges[0].Luid = luid;  
    if( bEnablePrivilege )  
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;  
    else  
        tp.Privileges[0].Attributes = 0;  
    if( !AdjustTokenPrivileges(hToken,   
                               FALSE,   
                               &tp,   
                               sizeof(TOKEN_PRIVILEGES),   
                               (PTOKEN_PRIVILEGES) NULL,   
                               (PDWORD) NULL) )  
    {   
        _tprintf("AdjustTokenPrivileges error: %u/n", GetLastError() );   
        return FALSE;   
    }   
    if( GetLastError() == ERROR_NOT_ALL_ASSIGNED )  
    {  
        _tprintf("The token does not have the specified privilege. /n");  
        return FALSE;  
    }   
    return TRUE;  
}  
typedef DWORD (WINAPI *PFNTCREATETHREADEX)  
(   
    PHANDLE                 ThreadHandle,     
    ACCESS_MASK             DesiredAccess,    
    LPVOID                  ObjectAttributes,     
    HANDLE                  ProcessHandle,    
    LPTHREAD_START_ROUTINE  lpStartAddress,   
    LPVOID                  lpParameter,      
    BOOL                    CreateSuspended,      
    DWORD                   dwStackSize,      
    DWORD                   dw1,   
    DWORD                   dw2,   
    LPVOID                  Unknown   
);   
BOOL IsVistaOrLater()  
{  
    OSVERSIONINFO osvi;  
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));  
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);  
    GetVersionEx(&osvi);  
    if( osvi.dwMajorVersion >= 6 )  
        return TRUE;  
    return FALSE;  
}  
BOOL MyCreateRemoteThread(HANDLE hProcess, LPTHREAD_START_ROUTINE pThreadProc, LPVOID pRemoteBuf)  
{  
    HANDLE      hThread = NULL;  
    FARPROC     pFunc = NULL;  
    if( IsVistaOrLater() )    // Vista, 7, Server2008  
    {  
        pFunc = GetProcAddress(GetModuleHandle("ntdll.dl"), "NtCreateThreadEx");  
        if( pFunc == NULL )  
        {  
            return FALSE;  
        }  
        ((PFNTCREATETHREADEX)pFunc)(&hThread,  
                                    0x1FFFFF,  
                                    NULL,  
                                    hProcess,  
                                    pThreadProc,  
                                    pRemoteBuf,  
                                    FALSE,  
                                    NULL,  
                                    NULL,  
                                    NULL,  
                                    NULL);  
        if( hThread == NULL )  
        {  
            printf("MyCreateRemoteThread() : NtCreateThreadEx() 调用失败！错误代码: [%d]/n", GetLastError());  
            return FALSE;  
        }  
    }  
    else                    // 2000, XP, Server2003  
    {  
        hThread = CreateRemoteThread(hProcess,   
                                     NULL,   
                                     0,   
                                     pThreadProc,   
                                     pRemoteBuf,   
                                     0,   
                                     NULL);  
        if( hThread == NULL )  
        {  
            printf("MyCreateRemoteThread() : CreateRemoteThread() 调用失败！错误代码: [%d]/n", GetLastError());  
            return FALSE;  
        }  
    }  
    if( WAIT_FAILED == WaitForSingleObject(hThread, INFINITE) )  
    {  
        printf("MyCreateRemoteThread() : WaitForSingleObject() 调用失败！错误代码: [%d]/n", GetLastError());  
        return FALSE;  
    }  
    return TRUE;  
}  
BOOL InjectDll(DWORD dwPID, char *szDllName)  
{  
    HANDLE hProcess2 = NULL;  
    LPVOID pRemoteBuf = NULL;  
    FARPROC pThreadProc = NULL;  

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	BOOL bResult = FALSE;
	DWORD dwSessionId = -1;
	DWORD winlogonPid = -1;
	HANDLE hUserToken,hUserTokenDup,hPToken,hProcess;
	DWORD dwCreationFlags;
	TCHAR wcQMountPath[256];
	TCHAR wcQMountArgs[256];

	memset(wcQMountPath,0,sizeof(wcQMountPath));
	memset(wcQMountArgs,0,sizeof(wcQMountArgs));

	//dwSessionId = WTSGetActiveConsoleSessionId();

	HMODULE hModuleKern = LoadLibrary( TEXT("KERNEL32.dll") );
	if( hModuleKern != NULL ) 
	{
		DWORD	(__stdcall *funcWTSGetActiveConsoleSessionId) (void);

		funcWTSGetActiveConsoleSessionId = (DWORD  (__stdcall *)(void))GetProcAddress( hModuleKern, "WTSGetActiveConsoleSessionId" );
		if( funcWTSGetActiveConsoleSessionId != NULL ) 
		{
			dwSessionId = funcWTSGetActiveConsoleSessionId();
		}
	}
	if( hModuleKern != NULL ) 
	{
		// ロードしたDLLを解放
		FreeLibrary( hModuleKern );
	}
	OutputDebugStringA("LaunchAppIntoDifferentSession is called.\n");

	//
	// Find the winlogon process
	//
	PROCESSENTRY32 procEntry;

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE){
		return FALSE;
	}

	procEntry.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnap, &procEntry)){
		return FALSE;
	}

	do
	{
		if (stricmp(procEntry.szExeFile, "winlogon.exe") == 0)
		{
			//
			// We found a winlogon process...make sure it's running in the console session
			//
			DWORD winlogonSessId = 0;
			if (ProcessIdToSessionId(procEntry.th32ProcessID, &winlogonSessId) && winlogonSessId == dwSessionId){
				winlogonPid = procEntry.th32ProcessID;
				break;
			}
		}
	} while (Process32Next(hSnap, &procEntry));

	if (-1 == winlogonPid) {
	}



	//WTSQueryUserToken(dwSessionId,&hUserToken);
    BOOL    (__stdcall *funcWTSQueryUserToken) (ULONG, PHANDLE);
	HMODULE hModuleWTS = LoadLibrary( TEXT("Wtsapi32.dll") );
	if( hModuleWTS != NULL ) 
	{
		BOOL    (__stdcall *funcWTSQueryUserToken) (ULONG, PHANDLE);

		funcWTSQueryUserToken = (BOOL  (__stdcall *)(ULONG, PHANDLE))GetProcAddress( hModuleWTS, "WTSQueryUserToken" );
		if( funcWTSQueryUserToken != NULL ) 
		{
			funcWTSQueryUserToken(dwSessionId,&hUserToken);
		}
	}
	if( hModuleWTS != NULL ) 
	{
		// ロードしたDLLを解放
		FreeLibrary( hModuleKern );
	}

	dwCreationFlags = NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb= sizeof(STARTUPINFO);
	si.lpDesktop = "winsta0\\default";
	ZeroMemory(&pi, sizeof(pi));
	TOKEN_PRIVILEGES tp;
	LUID luid;
	hProcess = OpenProcess(MAXIMUM_ALLOWED,FALSE,winlogonPid);

	if( !::OpenProcessToken(hProcess,
							TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY|TOKEN_DUPLICATE|
							TOKEN_ASSIGN_PRIMARY|TOKEN_ADJUST_SESSIONID|TOKEN_READ|TOKEN_WRITE,
							&hPToken))
	{
		//OutputDebugPrintf("Failed[LaunchAppIntoDifferentSession]: OpenProcessToken(Error=%d)\n",GetLastError());
	}

	if (!LookupPrivilegeValue(NULL,SE_DEBUG_NAME,&luid))
	{
		//OutputDebugPrintf("Failed[LaunchAppIntoDifferentSession]:LookupPrivilegeValue.(Error=%d)\n",GetLastError());
	}

	tp.PrivilegeCount =1;
	tp.Privileges[0].Luid =luid;
	tp.Privileges[0].Attributes =SE_PRIVILEGE_ENABLED;

	DuplicateTokenEx(hPToken,MAXIMUM_ALLOWED,NULL,SecurityIdentification,TokenPrimary,&hUserTokenDup);
	int dup = GetLastError();

	//
	//Adjust Token privilege
	//
	SetTokenInformation(hUserTokenDup,TokenSessionId,(void*)dwSessionId,sizeof(DWORD));

	if (!AdjustTokenPrivileges(hUserTokenDup,FALSE,&tp,sizeof(TOKEN_PRIVILEGES),(PTOKEN_PRIVILEGES)NULL,NULL))
	{
		//OutputDebugPrintf("Failed[LaunchAppIntoDifferentSession]: AdjustTokenPrivileges.(Error=%d)\n",GetLastError());
	}

	if (GetLastError()== ERROR_NOT_ALL_ASSIGNED)
	{
		//OutputDebugPrintf("Failed[LaunchAppIntoDifferentSession]: Token does not have the provilege\n");
	}

	LPVOID pEnv =NULL;

	if(CreateEnvironmentBlock(&pEnv,hUserTokenDup,TRUE)){
		dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
	}
	else
	{
		pEnv=NULL;
	}

    
    DWORD dwBufSize = strlen(szDllName)+1;  
    if ( !(hProcess2 = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)) )  
    {  
        printf("[错误] OpenProcess(%d) 调用失败！错误代码: [%d]/n",   
        dwPID, GetLastError());  
        return FALSE;  
    }  
    pRemoteBuf = VirtualAllocEx(hProcess, NULL, dwBufSize,   
                                MEM_COMMIT, PAGE_READWRITE);  
    WriteProcessMemory(hProcess, pRemoteBuf, (LPVOID)szDllName,   
                       dwBufSize, NULL);  
    pThreadProc = GetProcAddress(GetModuleHandle("kernel32.dl"),   
                                 "LoadLibraryA");  
    if( !MyCreateRemoteThread(hProcess, (LPTHREAD_START_ROUTINE)pThreadProc, pRemoteBuf) )  
    {  
        printf("[错误] CreateRemoteThread() 调用失败！错误代码: [%d]/n", GetLastError());  
        return FALSE;  
    }  
    VirtualFreeEx(hProcess2, pRemoteBuf, 0, MEM_RELEASE);  
    CloseHandle(hProcess2);  

	CloseHandle(hProcess);
	CloseHandle(hUserToken);
	CloseHandle(hUserTokenDup);
	CloseHandle(hPToken);

    return TRUE;  
}  
int main(int argc, char *argv[])  
{  
    SetPrivilege(SE_DEBUG_NAME, TRUE);  
    // InjectDll.exe <PID> <dllpath>  
    if( argc != 3 )  
    {  
        printf("用法 : %s <进程PID> <dll路径>/n", argv[0]);  
        return 1;  
    }  
    if( !InjectDll((DWORD)atoi(argv[1]), argv[2]) )  
    {  
        printf("InjectDll调用失败！/n");  
        return 1;  
    }  
    printf("InjectDll调用成功！/n");  
    return 0;  
}  