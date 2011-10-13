//---------------------------------------------------------------------------
//
// ApplicationScope.h
//
// SUBSYSTEM:   Hook system
//				
// MODULE:      Hook server
//
// DESCRIPTION: Implementation of the CApplicationScope class.
//              This class is designed to provide single interface for 
//              all hook related activities.
// 				
//             
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com)
// DATE:		2001 December v1.00
//
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "HookSrv.h"
#include "ApplicationScope.h"

//---------------------------------------------------------------------------
//
// class CApplicationScope 
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
// Static memeber declarations
//
//---------------------------------------------------------------------------
CApplicationScope* CApplicationScope::sm_pInstance = NULL;

//---------------------------------------------------------------------------
//
// Constructor
//
//---------------------------------------------------------------------------
CApplicationScope::CApplicationScope():
	m_hmodHookTool(NULL),
	m_pfnInstallHook(NULL)
{

}

//---------------------------------------------------------------------------
//
// Destructor 
//
//---------------------------------------------------------------------------
CApplicationScope::~CApplicationScope()
{
	if (m_hmodHookTool)
		::FreeLibrary( m_hmodHookTool );
}

//---------------------------------------------------------------------------
//
// Copy constructor
//
//---------------------------------------------------------------------------
CApplicationScope::CApplicationScope(const CApplicationScope& rhs)
{

}

//---------------------------------------------------------------------------
//
// Assignment operator
//
//---------------------------------------------------------------------------
CApplicationScope& CApplicationScope::operator=(const CApplicationScope& rhs)
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
CApplicationScope& CApplicationScope::GetInstance()
{
	if (!sm_pInstance)
	{
		CLockMgr<CCSWrapper> guard(g_AppSingeltonLock, TRUE);
		if (!sm_pInstance)
		{
			static CApplicationScope instance;
			sm_pInstance = &instance;
		}
	} // if

	return *sm_pInstance;
}

//---------------------------------------------------------------------------
// InstallHook
//
// Delegates the call to the DLL InstallHook function
//---------------------------------------------------------------------------
void CApplicationScope::InstallHook(BOOL bActivate, HWND hwndServer)
{
	if (NULL == m_hmodHookTool)
	{
		m_hmodHookTool = ::LoadLibrary( "HookTool.Dll" );
		if (NULL != m_hmodHookTool)
			m_pfnInstallHook = (PFN_INSTALLHOOK)::GetProcAddress(
				m_hmodHookTool, 
				"InstallHook"
				);
	} // if
	if (m_pfnInstallHook)
		m_pfnInstallHook(bActivate, hwndServer);
}

//---------------------------------------------------------------------------
// OnDllLoaded
//
// Fired when a process loads hooktool dll
//---------------------------------------------------------------------------
void CApplicationScope::OnDllLoaded(DWORD dwProcessId)
{

}

//---------------------------------------------------------------------------
// OnDllUnLoaded
//
// Fired when a process unloads hooktool dll
//---------------------------------------------------------------------------
void CApplicationScope::OnDllUnLoaded(DWORD dwProcessId)
{

}

//----------------------------End of the file -------------------------------
