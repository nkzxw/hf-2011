// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "windows.h"
#include "process.h"
#include "tlhelp32.h"
#include "stdio.h"

typedef 
WINUSERAPI
int
(WINAPI *PFNMESSAGEBOX)(
    __in_opt HWND hWnd,
    __in_opt LPCSTR lpText,
    __in_opt LPCSTR lpCaption,
    __in UINT uType);

int WINAPI MessageBoxProxy(
	__in_opt HWND hWnd,
    __in_opt LPCSTR lpText,
    __in_opt LPCSTR lpCaption,
    __in UINT uType);

int * addr = (int *)MessageBoxA;    //保存函数的入口地址
int * myaddr = (int *)MessageBoxProxy;

int *oldAddr = NULL;
int *newAddr = NULL;

int WINAPI MessageBoxProxy(
	__in_opt HWND hWnd,
    __in_opt LPCSTR lpText,
    __in_opt LPCSTR lpCaption,
    __in UINT uType)
{
	OutputDebugString (L"MessageBoxProxy Entry.\n");
	return ((PFNMESSAGEBOX)oldAddr)(NULL, "hook", "hook", 0);
}	
	
void StartHook (char *functionName)
{
	OutputDebugString (L"StartHook Entry.\n");

	HMODULE hModule = GetModuleHandle (NULL);
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((BYTE *)hModule + pDosHeader->e_lfanew);
	PIMAGE_OPTIONAL_HEADER pOptHeader = (PIMAGE_OPTIONAL_HEADER)&(pNtHeaders->OptionalHeader);

	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE *)hModule + pOptHeader->DataDirectory[1].VirtualAddress);

	while (pImportDescriptor->FirstThunk)
	{
		char *dllName= (char *)((BYTE *)hModule + pImportDescriptor->Name);
		PIMAGE_THUNK_DATA pThunkData = (PIMAGE_THUNK_DATA)((BYTE *)hModule + pImportDescriptor->OriginalFirstThunk);

		int no = 1;
		while (pThunkData->u1.Function)
		{
			char *funname = (char *)((BYTE *)hModule + (DWORD)pThunkData->u1.AddressOfData + 2);
			DWORD *pdwAddr = (DWORD *)((BYTE *)hModule + (DWORD)pImportDescriptor->FirstThunk + (no -1));
			if ((*pdwAddr) == (int)oldAddr)
			{
				DWORD dwAddr;
				MEMORY_BASIC_INFORMATION mbi;
				VirtualQuery (pdwAddr, &mbi, sizeof (mbi));
				VirtualProtect (pdwAddr, sizeof (DWORD), PAGE_READWRITE, &dwAddr);
				WriteProcessMemory (GetCurrentProcess (), pdwAddr, &newAddr, sizeof (DWORD), NULL);
				VirtualProtect (pdwAddr, sizeof (DWORD), dwAddr, 0);
			}
		}
	}
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		StartHook ("MessageBoxA");
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

