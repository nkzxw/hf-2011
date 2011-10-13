//---------------------------------------------------------------------------
//
// ModuleScope.h
//
// SUBSYSTEM:   Hook system
//				
// MODULE:      Hook tool
//
// DESCRIPTION: Implementation of the CModuleScope class.
//              This class is designed to provide single interface for 
//              all hook related activities.
// 				
//             
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com)
// DATE:		2001 December v1.00
//
//---------------------------------------------------------------------------

#include "..\Common\Common.h"
#include "ModuleScope.h"
#include "..\Common\SysUtils.h"
#include "..\Common\IniFile.h"
#include "..\Common\CustomMessages.h"

#include "Injector.h"

//---------------------------------------------------------------------------
//
// class CModuleScope 
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
// Static memeber declarations
//
//---------------------------------------------------------------------------
CModuleScope* CModuleScope::sm_pInstance      = NULL;
CLogFile*     CModuleScope::sm_pLogFile       = NULL;
CApiHookMgr*  CModuleScope::sm_pHookMgr       = NULL;
//---------------------------------------------------------------------------
//
// Constructor
//
//---------------------------------------------------------------------------
CModuleScope::CModuleScope(
	HWND*  phwndServer,
	BOOL*  pbHookInstalled,
	HHOOK* pHook
	):
	m_phwndServer(phwndServer),
	m_bTraceEnabledInitialized(FALSE),
	m_bTraceEnabled(FALSE),
	m_bUseWindowsHookInitialized(FALSE),
	m_bUseWindowsHook(TRUE),
	m_pInjector(NULL),
	m_pbHookInstalled(pbHookInstalled),
	m_pWhenZero(NULL)
{
	//
	// Make sure we now where we are.
	//
	m_bIsThisServerProcess = (NULL == *phwndServer);
	// 
	// Instantiate the object that looks after DLL ref counting
	//
	m_pWhenZero = new CWhenZeroDword(m_bIsThisServerProcess);
	//
	// Get the name of the current process
	//
	GetProcessHostName(m_szProcessName);
	//
	// and its process id
	//
	m_dwProcessId = ::GetCurrentProcessId();
	//
	// Create instance of the log file manager
	//
	sm_pLogFile = new CLogFile(GetTraceEnabled());
	//
	// Instantiate the only one hook manager
	//
	sm_pHookMgr = new CApiHookMgr(this);
	//
	// Which kind of injection we would like to use?
	//
	if (IsWindows9x() || UseWindowsHook())
		m_pInjector = new CWinHookInjector(m_bIsThisServerProcess, pHook);
	else
		m_pInjector = new CRemThreadInjector(m_bIsThisServerProcess);
}

//---------------------------------------------------------------------------
//
// Destructor 
//
//---------------------------------------------------------------------------
CModuleScope::~CModuleScope()
{
	delete m_pInjector;
	delete m_pWhenZero;
	delete sm_pHookMgr;
	delete sm_pLogFile;
}

//---------------------------------------------------------------------------
//
// Copy constructor
//
//---------------------------------------------------------------------------
CModuleScope::CModuleScope(const CModuleScope& rhs)
{
	
}

//---------------------------------------------------------------------------
//
// Assignment operator
//
//---------------------------------------------------------------------------
CModuleScope& CModuleScope::operator=(const CModuleScope& rhs)
{
	if (this == &rhs) 
		return *this;

	return *this; // return reference to left-hand object
}


//---------------------------------------------------------------------------
// GetInstance
//
// Implements the "double-checking" locking pattern combined with 
// Scott Meyers single instance
// For more details see - 
// 1. "Modern C++ Design" by Andrei Alexandrescu - 6.9 Living in a 
//     Multithreaded World
// 2. "More Effective C++" by Scott Meyers - Item 26
//---------------------------------------------------------------------------
CModuleScope* CModuleScope::GetInstance(
	HWND*  phwndServer,
	BOOL*  pbHookInstalled,
	HHOOK* pHook
	)
{
	if (!sm_pInstance)
	{
		CLockMgr<CCSWrapper> guard(g_ModuleSingeltonLock, TRUE);
		if (!sm_pInstance)
		{
			static CModuleScope instance(phwndServer, pbHookInstalled, pHook);
			sm_pInstance = &instance;

			char  szFileName[MAX_PATH];
			char  *pdest;
			::GetModuleFileName(
				ModuleFromAddress(CModuleScope::GetInstance), 
				szFileName, 
				MAX_PATH
				);
			pdest = &szFileName[strlen(szFileName) - 4];
			strcpy(pdest, ".log");
			sm_pLogFile->InitializeFileName(szFileName);
		}
	} // if

	return sm_pInstance;
}

//
// Called on DLL_PROCESS_ATTACH DLL notification
//
BOOL CModuleScope::ManageModuleEnlistment()
{
	BOOL bResult = FALSE;
	//
	// Check if it is the hook server we should allow mapping of the DLL into
	// its address space
	//
	if (FALSE == *m_pbHookInstalled)
	{
		LogMessage(	"------- Hook server loads HookTool library -------" );
		//
		// Set the flag, thus we will know that the server has been installed
		//
		*m_pbHookInstalled = TRUE;
		//
		// and return success error code
		//
		bResult = TRUE;
	}
	//
	// and any other process should be examined whether it should be
	// hooked up by the DLL
	//
	else
	{
		bResult = m_pInjector->IsProcessForHooking(m_szProcessName);

		if (bResult)
			InitializeHookManagement();
		//
		// DLL is about to be mapped
		//
		//
		// Notify the server process the DLL will be mapped
		//
		::PostMessage(
			*m_phwndServer, 
			UWM_HOOKTOOL_DLL_LOADED, 
			0, 
			::GetCurrentProcessId()
			);
	}

	return bResult;
}

//
// Called on DLL_PROCESS_DETACH notification
//
void CModuleScope::ManageModuleDetachment()
{
	//
	// Check if the request comes from hooked up application
	//
	if ( !m_bIsThisServerProcess )
	{
		FinalizeHookManagement();
		//
		// Is the server still running ?
		//
		if (NULL != *m_phwndServer)
			//
			// Notify the server process the DLL is about to be unmapped
			//
			::PostMessage(
				*m_phwndServer, 
				UWM_HOOKTOOL_DLL_UNLOADED, 
				0, 
				::GetCurrentProcessId()
				);
	} // if
	else
	{
		//
		// attempt to eject the dll
		//
		m_pInjector->EjectModuleFromAllProcesses(m_pWhenZero->GetZeroHandle());
		LogMessage(	"------- Hook server shuts down and unloads HookTool library -------");
	}
}


//
// Activate/Deactivate hooking engine
//
BOOL CModuleScope::InstallHookMethod(
	BOOL bActivate, 
	HWND hWndServer
	)
{
	BOOL bResult;
	if (bActivate)
	{
		*m_phwndServer = hWndServer;
		bResult = m_pInjector->InjectModuleIntoAllProcesses();
	}
	else
	{
		m_pInjector->EjectModuleFromAllProcesses(m_pWhenZero->GetZeroHandle());
		*m_phwndServer = NULL;
		bResult = TRUE;
	}
	return bResult;
}


//
// 
//
void CModuleScope::LogMessage(const char* pszBuffer)
{
	char    szPrintBuffer[MAX_PATH];
	sprintf(
		szPrintBuffer, 
		"%s(%u) - %s", 
		m_szProcessName, 
		m_dwProcessId, 
		pszBuffer
		);
	sm_pLogFile->DoLogMessage(szPrintBuffer);
}

//
// Accessor method
//
char* CModuleScope::GetProcessName() const
{
	return const_cast<char*>(m_szProcessName);
}

//
// Accessor method
//
DWORD CModuleScope::GetProcessId() const
{
	return m_dwProcessId;
}

//
// Return the name of the INI file
//
void CModuleScope::GetIniFile(char* pszIniFile)
{
	char  *pdest;
	::GetModuleFileName(
		ModuleFromAddress(CModuleScope::GetInstance), 
		pszIniFile, 
		MAX_PATH
		);
	pdest = &pszIniFile[strlen(pszIniFile) - 4];
	strcpy(pdest, ".ini");
}

//
// Get the value of [Trace] / Enabled from the INI file
//
BOOL CModuleScope::GetTraceEnabled()
{
	if (!m_bTraceEnabledInitialized)
	{
		m_bTraceEnabled = FALSE;
		char szIniFile[MAX_PATH];
		GetIniFile(szIniFile);
		CIniFile iniFile(szIniFile);
		m_bTraceEnabled = iniFile.ReadBool(
			"Trace",
			"Enabled",
			FALSE
			);
		m_bTraceEnabledInitialized = TRUE;
	} // if
	return m_bTraceEnabled;
}

//
// Determines whether Windows hook is going to be used
//
BOOL CModuleScope::UseWindowsHook()
{
	if (!m_bUseWindowsHookInitialized)
	{
		char szIniFile[MAX_PATH];
		GetIniFile(szIniFile);
		CIniFile iniFile(szIniFile);
		m_bUseWindowsHook = iniFile.ReadBool(
			"Scope",
			"UseWindowsHook",
			TRUE
			);
		m_bUseWindowsHookInitialized = TRUE;
	}
	return m_bUseWindowsHook;
}


//
// Hooks up an API function
//
BOOL CModuleScope::HookImport(
	PCSTR pszCalleeModName, 
	PCSTR pszFuncName, 
	PROC  pfnHook
	)
{
	return sm_pHookMgr->HookImport(
		pszCalleeModName,
		pszFuncName,
		pfnHook
		);
}

//
// Restores the original API function pointer
//
BOOL CModuleScope::UnHookImport(
	PCSTR pszCalleeModName, 
	PCSTR pszFuncName
	)
{
	return sm_pHookMgr->UnHookImport(
		pszCalleeModName,
		pszFuncName
		);
}

//
// Initialize hook engine
//
void CModuleScope::InitializeHookManagement()
{
	//
	// Initially we must hook a few important functions
	//
	sm_pHookMgr->HookSystemFuncs();
	//
	// and now we can set-up some custom (demonstration) hooks
	//
	sm_pHookMgr->HookImport("Gdi32.DLL", "TextOutA", (PROC)CModuleScope::MyTextOutA);
	sm_pHookMgr->HookImport("Gdi32.DLL", "TextOutW", (PROC)CModuleScope::MyTextOutW);
	sm_pHookMgr->HookImport("Kernel32.DLL", "ExitProcess", (PROC)CModuleScope::MyExitProcess);

	LogMessage("The hook engine has been activated.");
}

//
// Release all resource required by the hooking engine
//
void CModuleScope::FinalizeHookManagement()
{
	if (sm_pHookMgr->AreThereHookedFunctions())
		LogMessage("The hook engine has been deactivated.");
	sm_pHookMgr->UnHookAllFuncs();
}


//
// Our own TextOutA
//
BOOL WINAPI CModuleScope::MyTextOutA(
	HDC   hdc,           // handle to DC
	int   nXStart,       // x-coordinate of starting position
	int   nYStart,       // y-coordinate of starting position
	LPSTR lpString,      // character string
	int   cbString       // number of characters
	)
{
	BOOL bResult;
	char szBuffer[1024];

	::ZeroMemory((PBYTE)szBuffer, sizeof(szBuffer));
	if (0 == stricmp(lpString, "Hello from TestApp!"))
	{
		strcpy(szBuffer, "Do you recon this is the original text ?");
		cbString = strlen(szBuffer);
	} // if
	else
		memcpy((PBYTE)szBuffer, (PBYTE)lpString, cbString*sizeof(char));

	bResult = ::TextOutA(
		hdc,        // handle to DC
		nXStart,    // x-coordinate of starting position
		nYStart,    // y-coordinate of starting position
		szBuffer,   // character string
		cbString    // number of characters
		);

	return bResult;
}

//
// Our own TextOutA
//
BOOL WINAPI CModuleScope::MyTextOutW(
	HDC   hdc,           // handle to DC
	int   nXStart,       // x-coordinate of starting position
	int   nYStart,       // y-coordinate of starting position
	LPWSTR lpString,     // character string
	int   cbString       // number of characters
	)
{
	BOOL    bResult;
	wchar_t szBuffer[1024];

	::ZeroMemory((PBYTE)szBuffer, sizeof(szBuffer));
	if (0 == wcsicmp(lpString, L"Hello from TestApp!"))
	{
		wcscpy(szBuffer, L"Do you recon this is the original text ?");
		cbString = wcslen(szBuffer);
	} // if
	else
		memcpy((PBYTE)szBuffer, (PBYTE)lpString, cbString*sizeof(wchar_t));

	bResult = ::TextOutW(
		hdc,        // handle to DC
		nXStart,    // x-coordinate of starting position
		nYStart,    // y-coordinate of starting position
		szBuffer,   // character string
		cbString    // number of characters
		);

	return bResult;
}


//
// Our own ExitProcess
//
VOID WINAPI CModuleScope::MyExitProcess(
	UINT uExitCode   // exit code for all threads
	)
{
	char szBuffer[MAX_PATH];
	sprintf(szBuffer, "Process (%u) shuts down.", ::GetCurrentProcessId());
	sm_pInstance->LogMessage(szBuffer);

	::ExitProcess(uExitCode);
	return;
}


//----------------------------End of the file -------------------------------
