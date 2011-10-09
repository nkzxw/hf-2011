// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

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
	
void StartHook (char *funName, PROC pfnNew)
{
	OutputDebugStringW (L"[HookExportApi]: StartHook Entry.\n");

	USHORT index = 0 ; 
	ULONG i;
	HANDLE   hMod;
	IMAGE_DOS_HEADER * dosheader;
	IMAGE_OPTIONAL_HEADER * opthdr;
    PIMAGE_EXPORT_DIRECTORY exports;
	
    PUCHAR pFuncName = NULL;
	PULONG pAddressOfFunctions,pAddressOfNames;
    PUSHORT pAddressOfNameOrdinals;
    
	hMod = LoadLibrary (L"Kernel32.dll");
	
	dosheader = (IMAGE_DOS_HEADER *)hMod;
	opthdr = (PIMAGE_OPTIONAL_HEADER)(((PIMAGE_NT_HEADERS)((BYTE*)hMod+dosheader->e_lfanew))->OptionalHeader);
    exports = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)dosheader+ opthdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
  
    pAddressOfFunctions=(ULONG*)((BYTE*)hMod+exports->AddressOfFunctions);   
	pAddressOfNames=(ULONG*)((BYTE*)hMod+exports->AddressOfNames);           
	pAddressOfNameOrdinals=(USHORT*)((BYTE*)hMod+exports->AddressOfNameOrdinals); 


    
    for (i = 0; i < exports->NumberOfNames; i++) 
	{
		index=pAddressOfNameOrdinals[i];
		pFuncName = (PUCHAR)( (BYTE*)hMod + pAddressOfNames[i]);

		if (_stricmp( (char*)pFuncName,funName) == 0)
		{
			//addr=pAddressOfFunctions[index];
			break;
		}

	}
	
	OutputDebugString (L"[HideService]: Hook Start.\n");

	DWORD dwAddr;
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery ((LPVOID)(&(pAddressOfFunctions[index])), &mbi, sizeof (mbi));
	VirtualProtect ((LPVOID)(&(pAddressOfFunctions[index])), sizeof (DWORD), PAGE_READWRITE, &dwAddr);
	WriteProcessMemory (GetCurrentProcess (), (LPVOID)(&(pAddressOfFunctions[index])), (LPVOID)((PCHAR)pfnNew - (PCHAR)hMod), sizeof (DWORD), NULL);
	VirtualProtect ((LPVOID)(&(pAddressOfFunctions[index])), sizeof (DWORD), dwAddr, 0);
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
		break;
	}
	return TRUE;
}

