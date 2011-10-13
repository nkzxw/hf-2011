//---------------------------------------------------------------------------
//
// ModuleScope.h
//
// SUBSYSTEM:   Hook system
//				
// MODULE:      Hook tool
//
// DESCRIPTION: Declares interface for the CModuleScope class. 
//              This class is designed to provide single interface for 
//              all hook related activities.
// 				
//             
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com)
// DATE:		2001 December v1.00
//
//---------------------------------------------------------------------------

#if !defined(_MODULESCOPE_H_)
#define _MODULESCOPE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\Common\LockMgr.h"
#include "..\Common\LogFile.h"
#include "..\Common\SysUtils.h"
#include "Interlocked.h"
#include "ApiHook.h"

//---------------------------------------------------------------------------
//
// Typedefs
// 
//---------------------------------------------------------------------------
typedef CWhenZero<DWORD> CWhenZeroDword;
//---------------------------------------------------------------------------
//
// Forward declarations
// 
//---------------------------------------------------------------------------
class CInjector;

//---------------------------------------------------------------------------
//
// Global variables
// 
//---------------------------------------------------------------------------
//
// A global guard object used for protecting singelton's instantiation 
//
static CCSWrapper g_ModuleSingeltonLock;

//---------------------------------------------------------------------------
//
// class CModuleScope 
//
//---------------------------------------------------------------------------
class CModuleScope  
{
private:
	//
	// Intentionally hide the defualt constructor,
	// copy constructor and assignment operator 
	//

	//
	// Default constructor
	//
	CModuleScope(
		HWND*  phwndServer,
		BOOL*  pbHookInstalled,
		HHOOK* pHook
		);
	//
	// Copy constructor
	//
	CModuleScope(const CModuleScope& rhs);
	//
	// Assignment operator
	//
	CModuleScope& operator=(const CModuleScope& rhs);
public:
	//
	// Destructor - we must declare it as public in order to provide
	// enough visibility for the GetInstance().
	// However the destructor shouldn't be called directly by the 
	// Module's code.
	//
	virtual ~CModuleScope();
	//
	// Implements the "double-checking" locking pattern combined with 
	// Scott Meyers single instance
	// For more details see - 
	// 1. "Modern C++ Design" by Andrei Alexandrescu - 6.9 Living in a 
	//     Multithreaded World
	// 2. "More Effective C++" by Scott Meyers - Item 26
	//
	static CModuleScope* GetInstance(
		HWND*  phwndServer,
		BOOL*  pbHookInstalled,
		HHOOK* pHook
		);
	//
	// 
	//
	void LogMessage(const char* pszBuffer);
	//
	// Accessor method
	//
	char* GetProcessName() const;
	//
	// Accessor method
	//
	DWORD GetProcessId() const;
	//
	// Called on DLL_PROCESS_ATTACH DLL notification
	//
	BOOL ManageModuleEnlistment();
	//
	// Called on DLL_PROCESS_DETACH notification
	//
	void ManageModuleDetachment();
	//
	// Activate/Deactivate hooking engine
	//
	BOOL InstallHookMethod(
		BOOL bActivate, 
		HWND hWndServer
		);
private:
	//
	// Hooks up an API function
	//
	BOOL HookImport(
		PCSTR pszCalleeModName, 
		PCSTR pszFuncName, 
		PROC  pfnHook
		);
	//
	// Restores the original API function pointer
	//
	BOOL UnHookImport(
		PCSTR pszCalleeModName, 
		PCSTR pszFuncName
		);
	//
	// Determines whether Windows hook is going to be used
	//
	BOOL UseWindowsHook();
	//
	// Initialize hook engine
	//
	void InitializeHookManagement();
	//
	// Release all resource required by the hooking engine
	//
	void FinalizeHookManagement();
	//
	// Return the name of the INI file
	//
	void GetIniFile(char* pszIniFile);
	//
	// Get the value of [Trace] / Enabled from the INI file
	//
	BOOL GetTraceEnabled();
	//
	// Get the value of [Scope] / HookAll from the INI file
	//
	BOOL GetHookAllEnabled();
	//
	// An object responsible for injecting/ejecting the DLL into 
	// address space of a process
	//
	CInjector* m_pInjector;
	//
	// Instance's pointer holder
	//
	static CModuleScope* sm_pInstance;
	//
	// Log file management
	//
	static CLogFile* sm_pLogFile;
	//
	// Indicates whether this is the server process
	//
	BOOL m_bIsThisServerProcess;
	//
	// the name of the process the loads this DLL
	//
	char   m_szProcessName[MAX_PATH];
	//
	// and its process id
	//
	DWORD  m_dwProcessId;
	//
	//
	//
	BOOL m_bTraceEnabledInitialized;
	//
	// Indicates whether tracing is enabled
	//
	BOOL m_bTraceEnabled;
	//
	//
	//
	BOOL m_bUseWindowsHookInitialized;
	//
	// Determines whether to use windows hooks or CreateRemoteThread()
	// mecahnism for injecting
	//
	BOOL m_bUseWindowsHook;
	//
	// Holds address to a variable declared in a shared section
	//
	BOOL* m_pbHookInstalled;
	//
	// Pointer to window handle where we should post all our messages
	//
	HWND* m_phwndServer;
	//
	// Instance of the hook manager
	//
	static CApiHookMgr* sm_pHookMgr;
	//
	// A "solution" to the problem caused by incorrect unhooking process order
	// if Windows hooks injection is in use
	//
	CWhenZeroDword* m_pWhenZero;
	//
	// Hook function prototypes
	//
	static BOOL WINAPI MyTextOutA(
		HDC hdc,           // handle to DC
		int nXStart,       // x-coordinate of starting position
		int nYStart,       // y-coordinate of starting position
		LPSTR lpString,    // character string
		int cbString       // number of characters
		);
	static BOOL WINAPI MyTextOutW(
		HDC hdc,           // handle to DC
		int nXStart,       // x-coordinate of starting position
		int nYStart,       // y-coordinate of starting position
		LPWSTR lpString,   // character string
		int cbString       // number of characters
		);
	static VOID WINAPI MyExitProcess(
		UINT uExitCode   // exit code for all threads
		);
};

#endif // !defined(_MODULESCOPE_H_)

//----------------------------End of the file -------------------------------