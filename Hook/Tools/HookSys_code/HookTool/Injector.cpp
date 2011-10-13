//---------------------------------------------------------------------------
//
// Injector.cpp
//
// SUBSYSTEM: 
//				API hooking system
// MODULE:    
//              Implements injection mechanism
//
// DESCRIPTION:
//              
//
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com)
// DATE:		2001 November 13,  version 1.0 
//                                                                         
// FIXES:
//              - 2002 November 20
//                Added a mechanism for handling reference count of the HookTool.dll 
//                instances. This allows us to fix a problem caused by the asynchronous 
//                behavior of ::UnhookWindowsHookEx() API                
//
//---------------------------------------------------------------------------

#include "Injector.h"
#include "..\Common\ModuleInstance.h"
#include "..\Common\SysUtils.h"
#include "..\Common\IniFile.h"
#include "NtInjectorThread.h"

//---------------------------------------------------------------------------
//
// Thread function prototype
//
//---------------------------------------------------------------------------
typedef unsigned (__stdcall *PTHREAD_START)(void *);

//---------------------------------------------------------------------------
//
// External declarations
//
//---------------------------------------------------------------------------

extern LRESULT CALLBACK GetMsgProc(
	int code,       // hook code
	WPARAM wParam,  // removal option
	LPARAM lParam   // message
	);


//---------------------------------------------------------------------------
//
// class CInjector  
//
//---------------------------------------------------------------------------

CInjector::CInjector(BOOL bServerInstance):
	m_bServerInstance(bServerInstance),
	m_bHookAllEnabledInitialized(FALSE),
	m_bHookAllEnabled(FALSE)
{

}

CInjector::~CInjector()
{

}

//
// examines whether a process should be hooked up by the DLL
//
BOOL CInjector::IsProcessForHooking(PSTR pszExaminedProcessName)
{
	BOOL  bHoolAll = GetHookAllEnabled();
	BOOL  bProcessProtected = FALSE;
	char  szProcessName[MAX_PATH];
	DWORD dwStartPos = 0;
	long  nCommaPos;
	if (bHoolAll)
	{
		while ( (dwStartPos < strlen(m_szProcessesForHooking)) &&
		        GetNextCommaSeparatedString(
					&m_szProcessesForHooking[dwStartPos],
					szProcessName,
					sizeof(szProcessName),
					&nCommaPos
					) 
		      )
		{
			strcat(szProcessName, ".exe");
			if (0 == stricmp(szProcessName, pszExaminedProcessName))
			{
				bProcessProtected = TRUE;
				return FALSE;
			} // if
			dwStartPos += nCommaPos + 1;
		} // while
	} // if
	if (!bHoolAll)
	{
		dwStartPos = 0;
		while ( (dwStartPos < strlen(m_szProcessesForHooking)) &&
		        GetNextCommaSeparatedString(
					&m_szProcessesForHooking[dwStartPos],
					szProcessName,
					sizeof(szProcessName),
					&nCommaPos
					) 
		      )
		{
			strcat(szProcessName, ".exe");
			if (0 == stricmp(szProcessName, pszExaminedProcessName))
				return TRUE;
			dwStartPos += nCommaPos + 1;
		} // while
		return FALSE;
	} // if

	return TRUE;
}

//
// Return the name of the INI file
//
void CInjector::GetIniFile(char* pszIniFile)
{
	char  *pdest;
	::GetModuleFileName(
		ModuleFromAddress(GetMsgProc), 
		pszIniFile, 
		MAX_PATH
		);
	pdest = &pszIniFile[strlen(pszIniFile) - 4];
	strcpy(pdest, ".ini");
}


//
// Get the value of [Scope] / HookAll from the INI file
//
BOOL CInjector::GetHookAllEnabled()
{
	if (!m_bHookAllEnabledInitialized)
	{
		char szIniFile[MAX_PATH];
		GetIniFile(szIniFile);
		CIniFile iniFile(szIniFile);
		m_bHookAllEnabled = iniFile.ReadBool(
			"Scope",
			"HookAll",
			TRUE
			);
		m_bHookAllEnabledInitialized = TRUE;
		strcpy(m_szProcessesForHooking, "\0");
		strcpy(m_szProtectedProcesses, "\0");
		//
		// Should we process the two lists with processes for hooking
		// and ones that shouldn't be hooked up at all
		//
		if (!m_bHookAllEnabled)
			iniFile.ReadString(
				"Scope",
				"Hook",
				"\0",
				m_szProcessesForHooking
				);
		else
			iniFile.ReadString(
				"Scope",
				"Protect",
				"\0",
				m_szProtectedProcesses
				);
	} // if

	return m_bHookAllEnabled;
}


//---------------------------------------------------------------------------
//
// class CWinHookInjector  
//
//---------------------------------------------------------------------------

HHOOK* CWinHookInjector::sm_pHook = NULL;

CWinHookInjector::CWinHookInjector(BOOL bServerInstance, HHOOK* pHook):
	CInjector(bServerInstance)
{
	sm_pHook = pHook;
}


//
// Inject the DLL into all running processes
//
BOOL CWinHookInjector::InjectModuleIntoAllProcesses()
{
	*sm_pHook = ::SetWindowsHookEx(
		WH_GETMESSAGE,
		(HOOKPROC)(GetMsgProc),
		ModuleFromAddress(GetMsgProc), 
		0
		);
		
	return (NULL != *sm_pHook);
}

//
// Eject the DLL from all processes if it has been injected before 
//
BOOL CWinHookInjector::EjectModuleFromAllProcesses(HANDLE hWaitOn)
{
	BOOL bResult;
	if (NULL != *sm_pHook)
	{
		bResult = ::UnhookWindowsHookEx(*sm_pHook);
		// Wait on the reference counter event handle 
		// until all the instances of the DLL get unloaded
		::WaitForSingleObject(hWaitOn, INFINITE);
	}
	else 
		bResult = TRUE;
	return bResult;
}


//---------------------------------------------------------------------------
//
// class CRemThreadInjector  
//
//---------------------------------------------------------------------------

CCSWrapper  CRemThreadInjector::sm_CritSecInjector;


//
//
//
CRemThreadInjector::CRemThreadInjector(BOOL bServerInstance):
	CInjector(bServerInstance),
	m_pNtInjectorThread(NULL)
{
	if (bServerInstance)
		m_pNtInjectorThread = new CNtInjectorThread(this);
}

//
//
//
CRemThreadInjector::~CRemThreadInjector()
{
	if (m_bServerInstance)
	{
		m_pNtInjectorThread->SetActive(FALSE);
		delete m_pNtInjectorThread;
	} // if
}

//
// Attempts to enable SeDebugPrivilege. This is required by use of
// CreateRemoteThread() under NT/2K
//
BOOL CRemThreadInjector::EnableDebugPrivilege()
{
	HANDLE           hToken;
	LUID             sedebugnameValue;
	TOKEN_PRIVILEGES tp;

	if ( !::OpenProcessToken( 
		GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | // to adjust privileges
		TOKEN_QUERY,              // to get old privileges setting
		&hToken 
		) )
		//
		// OpenProcessToken() failed
		//
		return FALSE;
	//
	// Given a privilege's name SeDebugPrivilege, we should locate its local LUID mapping.
	//
	if ( !::LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &sedebugnameValue ) )
	{
		//
		// LookupPrivilegeValue() failed
		//
		::CloseHandle( hToken );
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = sedebugnameValue;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if ( !::AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof(tp), NULL, NULL ) )
	{
		//
		// AdjustTokenPrivileges() failed
		//
		::CloseHandle( hToken );
		return FALSE;
	}

	::CloseHandle( hToken );
	return TRUE;
}



//
// Inject the DLL into all running processes
//
BOOL CRemThreadInjector::InjectModuleIntoAllProcesses()
{
	BOOL                bResult = FALSE;

	if (m_bServerInstance)
	{
		CTaskManager        taskManager;
		CExeModuleInstance* pProcess; 

		taskManager.Populate(FALSE);
		EnableDebugPrivilege();
		for (long i = 0; i < taskManager.GetProcessCount(); i++)
		{
			pProcess = taskManager.GetProcessByIndex(i);
			bResult = InjectModuleInto( pProcess->Get_ProcessId() ) || bResult; 
		} // for

		m_pNtInjectorThread->SetActive(TRUE);
	} // if

	return bResult;
}

//
// Eject the DLL from all processes if it has been injected before 
//
BOOL CRemThreadInjector::EjectModuleFromAllProcesses(HANDLE hWaitOn)
{
	BOOL                bResult = FALSE;

	if (m_bServerInstance)
	{
		CTaskManager        taskManager;
		CExeModuleInstance* pProcess; 

		m_pNtInjectorThread->SetActive(FALSE);
		taskManager.Populate(FALSE);

		for (long i = 0; i < taskManager.GetProcessCount(); i++)
		{
			pProcess = taskManager.GetProcessByIndex(i);
			bResult = EjectModuleFrom( pProcess->Get_ProcessId() ) || bResult; 
		} // for
	} // if

	return bResult;
}

//
// Inject the DLL into address space of a specific external process
//
BOOL CRemThreadInjector::InjectModuleInto(DWORD dwProcessId)
{
	CLockMgr<CCSWrapper> lockMgr(sm_CritSecInjector, TRUE);	
	BOOL                 bResult  = FALSE; 
	//
	// We shouldn't inject the dll into the address space of the
	// server
	//
	if (dwProcessId != ::GetCurrentProcessId())
	{
		CTaskManager         taskManager;
		taskManager.PopulateProcess(dwProcessId, TRUE);
		CExeModuleInstance  *pProcess = taskManager.GetProcessById(dwProcessId);
		if (TRUE == IsProcessForHooking(pProcess->GetBaseName()))
			bResult = DoInjectModuleInto(pProcess);
	} // if

	return bResult;
}

//
// Eject the DLL from the address space of an external process
//
BOOL CRemThreadInjector::EjectModuleFrom(DWORD dwProcessId)
{
	CLockMgr<CCSWrapper> lockMgr(sm_CritSecInjector, TRUE);	
	BOOL   bResult  = FALSE;
	//
	// We shouldn't eject the dll into the address space of the
	// server
	//
	if (dwProcessId != ::GetCurrentProcessId())
	{
		CTaskManager         taskManager;
		taskManager.PopulateProcess(dwProcessId, TRUE);
		CExeModuleInstance  *pProcess = taskManager.GetProcessById(dwProcessId);
		if (pProcess)
			bResult  = DoEjectModuleFrom(*pProcess);
	} // if
	return bResult;
}

//
// Execute injection mechanism for NT/2K systems
//
BOOL CRemThreadInjector::DoInjectModuleInto(CExeModuleInstance *pProcess)
{
	BOOL   bResult          = FALSE; 
	HANDLE hProcess         = NULL;
	HANDLE hThread          = NULL;
	PSTR   pszLibFileRemote = NULL;
	__try 
	{
		if (TRUE == IsWindows9x())
			__leave;
		if (NULL == pProcess)
			__leave;

		char szLibFile[MAX_PATH];
		::GetModuleFileNameA(
			ModuleFromAddress(GetMsgProc), 
			szLibFile, 
			MAX_PATH
			);

		BOOL bFound = FALSE;
		CModuleInstance* pModuleInstance;
		for (long i = 0; i < pProcess->GetModuleCount(); i++)
		{
			pModuleInstance = pProcess->GetModuleByIndex(i);
			if ( 0 == stricmp(pModuleInstance->Get_Name(), szLibFile) )
			{
				bFound = TRUE;
				break;
			} // if
		} // for
		if (bFound) 
			__leave;

		// Get a handle for the process we want to inject into
		hProcess = ::OpenProcess(
			PROCESS_ALL_ACCESS, // Specifies all possible access flags
			FALSE, 
			pProcess->Get_ProcessId()
			);
		if (hProcess == NULL) 
			__leave;
		
		// Calculate the number of bytes needed for the DLL's pathname
		int cch = 1 + strlen(szLibFile);

		// Allocate space in the remote process for the pathname
		pszLibFileRemote = (PSTR)::VirtualAllocEx(
			hProcess, 
			NULL, 
			cch, 
			MEM_COMMIT, 
			PAGE_READWRITE
			);
		if (pszLibFileRemote == NULL) 
			__leave;
		// Copy the DLL's pathname to the remote process's address space
		if (!::WriteProcessMemory(
			hProcess, 
			(PVOID)pszLibFileRemote, 
			(PVOID)szLibFile, 
			cch, 
			NULL)) 
			__leave;
		// Get the real address of LoadLibraryW in Kernel32.dll
		PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)
			::GetProcAddress(::GetModuleHandle("Kernel32"), "LoadLibraryA");
		if (pfnThreadRtn == NULL) 
			__leave;
		// Create a remote thread that calls LoadLibraryW(DLLPathname)
		hThread = ::CreateRemoteThread(
			hProcess, 
			NULL, 
			0, 
			pfnThreadRtn, 
			(PVOID)pszLibFileRemote, 
			0, 
			NULL
			);
		if (hThread == NULL) 
			__leave;
		// Wait for the remote thread to terminate
		::WaitForSingleObject(hThread, INFINITE);

		bResult = TRUE; 
	}
	__finally 
	{ 
		// Free the remote memory that contained the DLL's pathname
		if (pszLibFileRemote != NULL) 
			::VirtualFreeEx(hProcess, (PVOID)pszLibFileRemote, 0, MEM_RELEASE);

		if (hThread  != NULL) 
			::CloseHandle(hThread);

		if (hProcess != NULL) 
			::CloseHandle(hProcess);
	}
	return bResult;
}

//
// Perform actual ejection of the DLL from the address space of an external process
//
BOOL CRemThreadInjector::DoEjectModuleFrom(CExeModuleInstance& process)
{
	BOOL   bResult  = FALSE; 
	HANDLE hProcess = NULL; 
	HANDLE hThread  = NULL;
	__try 
	{
		if (TRUE == IsWindows9x())
			__leave;
		//
		// Do not force the server to eject the DLL
		//
		if (process.Get_ProcessId() == ::GetCurrentProcessId())
			__leave;

		char szLibFile[MAX_PATH];
		::GetModuleFileNameA(
			ModuleFromAddress(GetMsgProc), 
			szLibFile, 
			MAX_PATH
			);

		BOOL bFound = FALSE;
		CModuleInstance* pModuleInstance;
		for (long i = 0; i < process.GetModuleCount(); i++)
		{
			pModuleInstance = process.GetModuleByIndex(i);
			if ( 0 == stricmp(pModuleInstance->Get_Name(), szLibFile) )
			{
				bFound = TRUE;
				break;
			} // if
		} // for

		if (!bFound) 
			__leave;

		// Get a handle for the target process.
		hProcess = ::OpenProcess(
			PROCESS_ALL_ACCESS, // Specifies all possible access flags
			FALSE, 
			process.Get_ProcessId()
			);
		if (hProcess == NULL) 
			__leave;
		// Get the real address of LoadLibraryW in Kernel32.dll
		PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)
			::GetProcAddress(::GetModuleHandle("Kernel32"), "FreeLibrary");
		if (pfnThreadRtn == NULL) 
			__leave;
		// Create a remote thread that calls LoadLibraryW(DLLPathname)
		hThread = ::CreateRemoteThread(
			hProcess, 
			NULL, 
			0, 
			pfnThreadRtn, 
			pModuleInstance->Get_Module(), 
			0, 
			NULL
			);
		if (hThread == NULL) 
			__leave;
		// Wait for the remote thread to terminate
		::WaitForSingleObject(hThread, INFINITE);

		bResult = TRUE; 
	}
	__finally 
	{ 
		if (hThread != NULL) 
			::CloseHandle(hThread);
		if (hProcess != NULL) 
			::CloseHandle(hProcess);
	}
	return bResult;
}

//----------------------------End of the file -------------------------------