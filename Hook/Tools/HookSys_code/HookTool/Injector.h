//---------------------------------------------------------------------------
//
// Injector.h
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
#if !defined(_INJECTOR_H_)
#define _INJECTOR_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//---------------------------------------------------------------------------
//
// Includes
//
//---------------------------------------------------------------------------
#include <windows.h>
#include "..\Common\LockMgr.h"

//---------------------------------------------------------------------------
//
// Forward declararions
//
//---------------------------------------------------------------------------
class CExeModuleInstance;
class CNtInjectorThread;

//---------------------------------------------------------------------------
//
// class CInjector  
//
//---------------------------------------------------------------------------
class CInjector  
{
public:
	CInjector(BOOL bServerInstance);
	virtual ~CInjector();
	//
	// examines whether a process should be hooked up by the DLL
	//
	BOOL IsProcessForHooking(PSTR pszExaminedProcessName);
	//
	// Inject the DLL into all running processes
	//
	virtual BOOL InjectModuleIntoAllProcesses() = 0;
	//
	// Eject the DLL from all processes if it has been injected before 
	//
	virtual BOOL EjectModuleFromAllProcesses(HANDLE hWaitOn) = 0;
protected:
	//
	// Determines whether the instance is created by the hook server
	//
	BOOL m_bServerInstance;
private:
	//
	// Get the value of [Scope] / HookAll from the INI file
	//
	BOOL GetHookAllEnabled();
	//
	// Return the name of the INI file
	//
	void GetIniFile(char* pszIniFile);
	//
	// A comma separated list with name of processes
	// for hooking
	//
	char m_szProcessesForHooking[MAX_PATH];
	//
	// ... and those that should be protected and not hooked up
	//
	char  m_szProtectedProcesses[MAX_PATH];
	//
	//
	//
	BOOL m_bHookAllEnabledInitialized;
	//
	// Indicates whether all process must be hooked up
	//
	BOOL m_bHookAllEnabled;
};

//---------------------------------------------------------------------------
//
// class CWinHookInjector  
//
//---------------------------------------------------------------------------
class CWinHookInjector: public CInjector  
{
public:
	CWinHookInjector(BOOL bServerInstance, HHOOK* pHook);
private:
	//
	// Inject the DLL into all running processes
	//
	virtual BOOL InjectModuleIntoAllProcesses();
	//
	// Eject the DLL from all processes if it has been injected before 
	//
	virtual BOOL EjectModuleFromAllProcesses(HANDLE hWaitOn);
	//
	// Pointer to shared hook handle
	//
	static HHOOK* sm_pHook;
};

//---------------------------------------------------------------------------
//
// class CRemThreadInjector  
//
//---------------------------------------------------------------------------
class CRemThreadInjector: public CInjector  
{
public:
	CRemThreadInjector(BOOL bServerInstance);
	virtual ~CRemThreadInjector();
	//
	// Inject the DLL into address space of a specific external process
	//
	BOOL InjectModuleInto(DWORD dwProcessId);
	//
	// Eject the DLL from the address space of an external process
	//
	BOOL EjectModuleFrom(DWORD dwProcessId);
private:
	//
	// Inject the DLL into all running processes
	//
	virtual BOOL InjectModuleIntoAllProcesses();
	//
	// Eject the DLL from all processes if it has been injected before 
	//
	virtual BOOL EjectModuleFromAllProcesses(HANDLE hWaitOn);
	//
	// Attempts to enable SeDebugPrivilege. This is required by use of
	// CreateRemoteThread() under NT/2K
	//
	BOOL EnableDebugPrivilege();
	//
	// Execute injection mechanism for NT/2K systems
	//
	virtual BOOL DoInjectModuleInto(CExeModuleInstance *pProcess);
	//
	// Perform actual ejection of the DLL from the address space of an external process
	//
	virtual BOOL DoEjectModuleFrom(CExeModuleInstance& process);
	//
	// Guard used by InjectModuleInto
	//
	static CCSWrapper sm_CritSecInjector;
	//
	// An object responsible for monitoring process creation/termination
	//
	CNtInjectorThread* m_pNtInjectorThread;
};


#endif // !defined(_INJECTOR_H_)
//----------------------------End of the file -------------------------------