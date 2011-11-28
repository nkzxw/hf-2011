// TestHook.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "TestHook.h"
#include "TestHookDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTestHookApp

BEGIN_MESSAGE_MAP(CTestHookApp, CWinApp)
	//{{AFX_MSG_MAP(CTestHookApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTestHookApp construction

CTestHookApp::CTestHookApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CTestHookApp object

CTestHookApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CTestHookApp initialization

BOOL CTestHookApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	SetSpecificPrivilegeInAccessToken(SE_DEBUG_NAME, TRUE);	

	CTestHookDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

BOOL CTestHookApp::SetSpecificPrivilegeInAccessToken(LPCSTR lpPrivType,BOOL bEnabled)
{	
	HANDLE           hProcess;
	HANDLE           hAccessToken;
	LUID             luidPrivilegeLUID;
	TOKEN_PRIVILEGES tpTokenPrivilege;
	BOOL bReturn = FALSE;
	
	hProcess =::GetCurrentProcess();
	if (!hProcess)
	{ 
		return FALSE;
	}
	
	if (!OpenProcessToken(hProcess,
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
		&hAccessToken))
	{ 
		CloseHandle(hProcess);
		return FALSE;
	} 
	
	if(!LookupPrivilegeValue(NULL,
		lpPrivType,
		&luidPrivilegeLUID))
	{ 
		CloseHandle(hAccessToken);
		CloseHandle(hProcess);
		return FALSE;
	}
	
	tpTokenPrivilege.PrivilegeCount = 1;
	tpTokenPrivilege.Privileges[0].Luid = luidPrivilegeLUID;
	tpTokenPrivilege.Privileges[0].Attributes = bEnabled?SE_PRIVILEGE_ENABLED:0;
	SetLastError(ERROR_SUCCESS);
	bReturn = AdjustTokenPrivileges(hAccessToken,
		FALSE,  // Do not disable all
		&tpTokenPrivilege,
		sizeof(TOKEN_PRIVILEGES),
		NULL,   // Ignore previous info
		NULL);  // Ignore previous info
	
	if (GetLastError()!=ERROR_SUCCESS) 
		bReturn = FALSE;
	CloseHandle(hAccessToken);
	CloseHandle(hProcess);
	
	return bReturn;
}

