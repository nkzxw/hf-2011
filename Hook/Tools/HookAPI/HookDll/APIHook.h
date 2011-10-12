/******************************************************************************
Module:  APIHook.h
Notices: Copyright (c) 2011
******************************************************************************/
#pragma once

///////////////////////////////////////////////////////////////////////////////
class CAPIHook {
public:
    CAPIHook(PSTR pszModName, PSTR pszFuncName, PROC pfnHook);

    ~CAPIHook();

	/*
    * Returns the original address of the hooked function
	*/
    operator PROC() { return(m_pfnOrig); }

	/*
	* Hook Start.
	*/
	VOID StartHook ();

	/*
	* Unhook Start.
	*/
	VOID UnHook ();

    static BOOL ExcludeAPIHookMod; 
public:
	/*
    * Calls the real GetProcAddress 
    */
	static FARPROC WINAPI GetProcAddressRaw(HMODULE hmod, PCSTR pszProcName);

private:
    static PVOID sm_pvMaxAppAddr; // Maximum private memory address
    static CAPIHook* sm_pHead;    // Address of first object
    CAPIHook* m_pNext;            // Address of next  object

    PCSTR m_pszModName;     // Module containing the function (ANSI)
    PCSTR m_pszFuncName;    // Function name in callee (ANSI)
    PROC  m_pfnOrig;        // Original function address in callee
    PROC  m_pfnNew;        // Hook function address

private:
    /*
	* Replaces a symbol's address in a module's import section
	*/
    static void WINAPI ReplaceIATEntryInAllMods(PCSTR pszCalleeModName,PROC pfnOrig, PROC pfnHook);

	/*
    * Replaces a symbol's address in all modules' import sections
	*/
    static void WINAPI ReplaceIATEntryInOneMod(PCSTR pszCalleeModName, PROC pfnOrig, PROC pfnHook, HMODULE hmodCaller);

private:
	/*
	* Used when a DLL is newly loaded after hooking a function
	*/
    static void    WINAPI FixupNewlyLoadedModule(HMODULE hmod, DWORD dwFlags);

	/*
    * Used to trap when DLLs are newly loaded
	*/
    static BOOL WINAPI CreateProcessA(LPCTSTR lpApplicationName,
                                    LPTSTR lpCommandLine,
                                    LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                    LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                    BOOL bInheritHandles,
                                    DWORD dwCreationFlags,
                                    LPVOID lpEnvironment,
                                    LPCTSTR lpCurrentDirectory,
                                    LPSTARTUPINFO lpStartupInfo,
                                    LPPROCESS_INFORMATION lpProcessInformation);

    static BOOL WINAPI CreateProcessW(LPCTSTR lpApplicationName,
                                    LPTSTR lpCommandLine,
                                    LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                    LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                    BOOL bInheritHandles,
                                    DWORD dwCreationFlags,
                                    LPVOID lpEnvironment,
                                    LPCTSTR lpCurrentDirectory,
                                    LPSTARTUPINFO lpStartupInfo,
                                    LPPROCESS_INFORMATION lpProcessInformation);

private:
    static CAPIHook sm_CreateProcessA;
    static CAPIHook sm_CreateProcessW;
};


//////////////////////////////// End of File //////////////////////////////////