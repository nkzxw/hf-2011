#include "stdafx.h"
#include <Windows.h>
#include <ImageHlp.h>
#include <tlhelp32.h>
#pragma comment(lib, "ImageHlp")
#include "APIHook.h"
#include "HookEngine.h"

#include <Psapi.h>

/*
* API Hook 的原理采用遍历进程的所有模块方式，再HOOK所有模块的IAT.
*/

//CAPIHook CAPIHook::sm_CreateProcessA  ("Kernel32.dll", "CreateProcessA",(PROC) CAPIHook::CreateProcessA);
//CAPIHook CAPIHook::sm_CreateProcessW  ("Kernel32.dll", "CreateProcessW",(PROC) CAPIHook::CreateProcessW);

CAPIHook* CAPIHook::sm_pHead = NULL;
BOOL CAPIHook::ExcludeAPIHookMod = TRUE; 

LONG WINAPI InvalidReadExceptionFilter(PEXCEPTION_POINTERS pep);
CAPIHook::CAPIHook(PSTR pszModName, PSTR pszFuncName, PROC pfnNew) 
{
    m_pNext  = sm_pHead;    
    sm_pHead = this;        

    m_pszModName   = pszModName;
    m_pszFuncName  = pszFuncName;
    m_pfnNew      = pfnNew;

	//
	//由于services.msc 动态加载filemgmt.dll 库的时间延迟与程序启动
	//所以加载时动态加载起来
	//
	HMODULE hModule = GetModuleHandleA("filemgmt.dll");
    if(hModule == NULL)
    {
		OutputDebugPrintf ("[HookAPI]: LoadLibraryA filemgmt.dll.\n");
        LoadLibraryA("filemgmt.dll");
    }

    HMODULE hModuleCallee = GetModuleHandleA(pszModName);
    if(hModuleCallee == NULL)
    {
        LoadLibraryA(pszModName);
    }
    m_pfnOrig =  GetProcAddressRaw(GetModuleHandleA(pszModName), m_pszFuncName);

    if (m_pfnOrig == NULL)
    {
        wchar_t szPathname[MAX_PATH];
        GetModuleFileNameW(NULL, szPathname, _countof(szPathname));
        wchar_t sz[1024];
        OutputDebugStringW(sz);
        return;
    }

    ReplaceIATEntryInAllMods(m_pszModName, m_pfnOrig, m_pfnNew);
}


///////////////////////////////////////////////////////////////////////////////
CAPIHook::~CAPIHook() 
{
    ReplaceIATEntryInAllMods(m_pszModName, m_pfnNew, m_pfnOrig);

    CAPIHook* p = sm_pHead; 
	if (p == this){
        sm_pHead = p->m_pNext; 
    }
    else 
    {
        BOOL bFound = FALSE;

        for (; !bFound && (p->m_pNext != NULL); p = p->m_pNext) 
        {
            if (p->m_pNext == this) { 
                p->m_pNext = p->m_pNext->m_pNext; 
                bFound = TRUE;
            }
        }
    }
}

FARPROC CAPIHook::GetProcAddressRaw(
	HMODULE hmod, 
	PCSTR pszProcName
	) 
{
    return(::GetProcAddress(hmod, pszProcName));
}

static HMODULE ModuleFromAddress(
	PVOID pv
	) 
{
    MEMORY_BASIC_INFORMATION mbi;
    return((VirtualQuery(pv, &mbi, sizeof(mbi)) != 0)  ? (HMODULE) mbi.AllocationBase : NULL);
}

void CAPIHook::ReplaceIATEntryInAllMods(
	PCSTR pszModName,
	PROC pfnCurrent, 
	PROC pfnNew
	)
{
    HMODULE hmodThisMod = ExcludeAPIHookMod ? ModuleFromAddress(ReplaceIATEntryInAllMods) : NULL;

    HANDLE hSnapshot = INVALID_HANDLE_VALUE;

    hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if(hSnapshot == INVALID_HANDLE_VALUE)
    {
		OutputDebugPrintf ("[HookAPI]: CreateToolhelp32Snapshot Error.\n");
        return;
    }

    MODULEENTRY32 me = { sizeof(me) };
    for (BOOL fOk = Module32First(hSnapshot, &me); fOk; fOk = Module32Next(hSnapshot, &me)) 
    {
        if (me.hModule != hmodThisMod) 
		//if (stricmp ("filemgmt.dll", me.szModule) == 0) 
        {
            ReplaceIATEntryInOneMod(pszModName, 
									pfnCurrent, 
									pfnNew, 
									me.hModule);
        }
    }
    ::CloseHandle(hSnapshot);
}

void CAPIHook::ReplaceIATEntryInOneMod(
		PCSTR pszModName, 
		PROC pfnCurrent, 
		PROC pfnNew, 
		HMODULE hmodCaller
		) 
{
    ULONG ulSize;

	//OutputDebugPrintf ("[HookAPI]: ReplaceIATEntryInOneMod Enter.\n");

    PIMAGE_IMPORT_DESCRIPTOR pImportDesc = NULL;
    __try {
        pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR) ImageDirectoryEntryToData(
															            hmodCaller, 
																		TRUE, 
																		IMAGE_DIRECTORY_ENTRY_IMPORT, 
																		&ulSize);
    } 
    __except (InvalidReadExceptionFilter(GetExceptionInformation())) 
    {
		OutputDebugPrintf ("[HookAPI]: ImageDirectoryEntryToData Error.\n");
		//TODO	
    }

    if (pImportDesc == NULL)
    {
		OutputDebugPrintf ("[HookAPI]: pImportDesc NULL.\n");
		//TODO
        return; 
    }

    for (; pImportDesc->Name; pImportDesc++) 
    {
        PSTR pszModName = (PSTR) ((PBYTE) hmodCaller + pImportDesc->Name);
		
		//OutputDebugPrintf ("[HookAPI]: Module Name = %s.\n", pszModName);

        if (lstrcmpiA(pszModName, pszModName) == 0) 
        {
			//
            // Get caller's import address table (IAT) for the callee's functions
			//
            PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA) 
                ((PBYTE) hmodCaller + pImportDesc->FirstThunk);

			//
            // Replace current function address with new function address
			//
            for (; pThunk->u1.Function; pThunk++) 
            {
                PROC* ppfn = (PROC*) &pThunk->u1.Function;
                BOOL bFound = (*ppfn == pfnCurrent);
                if (bFound) {
					OutputDebugPrintf ("[HookAPI]: ReplaceIATEntryInOneMod bFound.\n");

                    if (!WriteProcessMemory(GetCurrentProcess(), ppfn, &pfnNew, sizeof(pfnNew), NULL) && 
                        (ERROR_NOACCESS == GetLastError())) 
                    {
                        DWORD dwOldProtect;
                        if (VirtualProtect(ppfn, sizeof(pfnNew), PAGE_WRITECOPY,&dwOldProtect)) 
                        {
							//OutputDebugPrintf ("[HookAPI]: ReplaceIATEntryInOneMod Enter.\n");

                            WriteProcessMemory(GetCurrentProcess(), ppfn, &pfnNew, sizeof(pfnNew), NULL);
                            VirtualProtect(ppfn, sizeof(pfnNew), dwOldProtect, &dwOldProtect);
                        }
                    }
                    return; 
                }
            }//for
        }  
    }//for
}

LONG WINAPI InvalidReadExceptionFilter(
	PEXCEPTION_POINTERS pep) 
{
    LONG lDisposition = EXCEPTION_EXECUTE_HANDLER;
    return(lDisposition);
}

void CAPIHook::FixupNewlyLoadedModule(
	HMODULE hmod, 
	DWORD dwFlags
	) 
{
    if ((hmod != NULL) &&   
        (hmod != ModuleFromAddress(FixupNewlyLoadedModule)) && 
        ((dwFlags & LOAD_LIBRARY_AS_DATAFILE) == 0) &&
        ((dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) == 0) &&
        ((dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE) == 0)
        ) 
	{
        for (CAPIHook* p = sm_pHead; p != NULL; p = p->m_pNext) {
            if (p->m_pfnOrig != NULL) {
                ReplaceIATEntryInAllMods(p->m_pszModName, 
										 p->m_pfnOrig, 
										 p->m_pfnNew);  
            } else {
				//TODO
            }
        }
    }
}

VOID CAPIHook::StartHook ()
{
	ReplaceIATEntryInAllMods(m_pszModName, m_pfnOrig, m_pfnNew);
}

VOID CAPIHook::UnHook ()
{
	ReplaceIATEntryInAllMods(m_pszModName, m_pfnNew, m_pfnOrig);
}

BOOL WINAPI CAPIHook::CreateProcessA(LPCTSTR lpApplicationName,
                                    LPTSTR lpCommandLine,
                                    LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                    LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                    BOOL bInheritHandles,
                                    DWORD dwCreationFlags,
                                    LPVOID lpEnvironment,
                                    LPCTSTR lpCurrentDirectory,
                                    LPSTARTUPINFO lpStartupInfo,
                                    LPPROCESS_INFORMATION lpProcessInformation)
{
    return(TRUE);
}

BOOL WINAPI CAPIHook::CreateProcessW(LPCTSTR lpApplicationName,
                                    LPTSTR lpCommandLine,
                                    LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                    LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                    BOOL bInheritHandles,
                                    DWORD dwCreationFlags,
                                    LPVOID lpEnvironment,
                                    LPCTSTR lpCurrentDirectory,
                                    LPSTARTUPINFO lpStartupInfo,
                                    LPPROCESS_INFORMATION lpProcessInformation)
{
    return(TRUE);
}