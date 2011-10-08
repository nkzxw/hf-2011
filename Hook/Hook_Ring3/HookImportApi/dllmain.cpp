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

PFNMESSAGEBOX OldMessageBox = (PFNMESSAGEBOX)MessageBoxA;

int WINAPI MessageBoxProxy(
	__in_opt HWND hWnd,
    __in_opt LPCSTR lpText,
    __in_opt LPCSTR lpCaption,
    __in UINT uType)
{
	OutputDebugStringW (L"MessageBoxProxy Entry.\n");
	return ((PFNMESSAGEBOX)OldMessageBox)(NULL, "hook", "hook", 0);
}	
	
void StartHook (char *functionName, PROC pfnNew)
{
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

		//
		// 确保函数不是以序号导入的。
		//
		if ((pOriginalThunk->u1.ForwarderString & IMAGE_ORDINAL_FLAG32) != 0)
			return;

		while (pOriginalThunk->u1.Function)
		{
			PIMAGE_IMPORT_BY_NAME pImageByName = (PIMAGE_IMPORT_BY_NAME)((PBYTE)hModule + pOriginalThunk->u1.AddressOfData);

			if (lstrcmpi((LPCSTR)pImageByName->Name, functionName) == 0)
			{
				OutputDebugStringA ("[HideService]: StartHook Enter.\n");

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


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		StartHook ("MessageBoxA", (PROC)MessageBoxProxy);

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		StartHook ("MessageBoxA", (PROC)OldMessageBox);
		break;
	}
	return TRUE;
}

