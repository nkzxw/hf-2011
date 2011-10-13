//---------------------------------------------------------------------------
//
// HookTool.cpp
//
// SUBSYSTEM:   Hook system
//				
// MODULE:      Hook tool    
//				
// DESCRIPTION: Defines the entry point for the DLL application.
// 
//             
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com)
// DATE:		2001 December v1.00
//
//---------------------------------------------------------------------------

#include "..\Common\Common.h"
#include "..\Common\SysUtils.h"
#include "ModuleScope.h"

//---------------------------------------------------------------------------
//
// Shared by all processes variables
//
//---------------------------------------------------------------------------
#pragma data_seg(".HKT")
// The hook handle
HHOOK sg_hGetMsgHook       = NULL;
// Indicates whether the hook has been installed
BOOL  sg_bHookInstalled    = FALSE;
// We get this from the application who calls SetWindowsHookEx()'s wrapper
HWND  sg_hwndServer        = NULL; 
#pragma data_seg()


//---------------------------------------------------------------------------
//
// Global (per process) variables
//
//---------------------------------------------------------------------------
static CModuleScope* g_pModuleScope = NULL;

//---------------------------------------------------------------------------
//
// Forward declarations
//
//---------------------------------------------------------------------------
BOOL WINAPI InstallHook(
	BOOL bActivate, 
	HWND hWndServer
	);

LRESULT CALLBACK GetMsgProc(
	int code,       // hook code
	WPARAM wParam,  // removal option
	LPARAM lParam   // message
	);
//---------------------------------------------------------------------------
// DllMain
//
// Entry point
//---------------------------------------------------------------------------
BOOL APIENTRY DllMain( 
	HANDLE hModule, 
	DWORD  ul_reason_for_call, 
	LPVOID lpReserved
	)
{
	BOOL bResult = TRUE;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			// We disable thread notifications
			// Prevent the system from calling DllMain
			// when threads are created or destroyed.
			::DisableThreadLibraryCalls( (HINSTANCE)hModule );

			g_pModuleScope = CModuleScope::GetInstance(
				&sg_hwndServer, 
				&sg_bHookInstalled,
				&sg_hGetMsgHook
				);
			g_pModuleScope->ManageModuleEnlistment();
			break;
		}
	case DLL_PROCESS_DETACH:
		{
			//
			// The DLL is being unmapped from the process's address space.
			//
			g_pModuleScope->ManageModuleDetachment();
			break;
		}
	} // switch

	return TRUE;
}

//---------------------------------------------------------------------------
// InstallHook
//
//---------------------------------------------------------------------------
BOOL WINAPI InstallHook(
	BOOL bActivate, 
	HWND hWndServer
	)
{
	return g_pModuleScope->InstallHookMethod(bActivate, hWndServer);
}

//---------------------------------------------------------------------------
// GetMsgProc
//
// Filter function for the WH_GETMESSAGE - it's just a dummy function
//---------------------------------------------------------------------------
LRESULT CALLBACK GetMsgProc(
	int code,       // hook code
	WPARAM wParam,  // removal option
	LPARAM lParam   // message
	)
{
	//
	// We must pass the all messages on to CallNextHookEx.
	//
	return ::CallNextHookEx(sg_hGetMsgHook, code, wParam, lParam);
}



//----------------------------End of the file -------------------------------