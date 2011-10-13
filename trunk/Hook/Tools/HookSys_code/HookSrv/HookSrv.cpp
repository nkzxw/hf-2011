//---------------------------------------------------------------------------
//
// HookSrv.cpp
//
// SUBSYSTEM:   Hook system
//				
// MODULE:      Hook server
//				
// DESCRIPTION: Defines the class behaviors for the application.
//             
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com)
// DATE:		2001 December v1.00
//
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "HookSrv.h"
#include "MainFrm.h"
#include "LimitSingleInstance.h"

//---------------------------------------------------------------------------
//
// class CHookSrvApp
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CHookSrvApp, CWinApp)
	//{{AFX_MSG_MAP(CHookSrvApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
// CHookSrvApp construction
//
//---------------------------------------------------------------------------

CHookSrvApp::CHookSrvApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

//---------------------------------------------------------------------------
//
// Singletons
//
//---------------------------------------------------------------------------
// Application object
CHookSrvApp theApp;
// The one and only CLimitSingleInstance object
CLimitSingleInstance g_SingleInstanceObj("{05CA3573-B449-4e0b-83F5-7FD612E378E9}");


//---------------------------------------------------------------------------
//
// CHookSrvApp initialization
//
//---------------------------------------------------------------------------

BOOL CHookSrvApp::InitInstance()
{
	BOOL bIsAnotherInstanceRunning = 
		g_SingleInstanceObj.IsAnotherInstanceRunning();

	if (!bIsAnotherInstanceRunning)
	{
		#ifdef _AFXDLL
			Enable3dControls();			// Call this when using MFC in a shared DLL
		#else
			Enable3dControlsStatic();	// Call this when linking to MFC statically
		#endif
		// To create the main window, this code creates a new frame window
		// object and then sets it as the application's main window object.
		CMainFrame* pFrame = new CMainFrame;
		m_pMainWnd = pFrame;
		// create and load the frame with its resources
		pFrame->LoadFrame(
			IDR_MAINFRAME,
			WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, 
			NULL,
			NULL
			);

		// The one and only window has been initialized, so show and update it.
		pFrame->ShowWindow(SW_HIDE);
		pFrame->UpdateWindow();
	} // if

	return (!bIsAnotherInstanceRunning);
}

//---------------------------------------------------------------------------
//
// class CAboutDlg dialog used for App About
//
//---------------------------------------------------------------------------
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//---------------------------------------------------------------------------
//
// CAboutDlg constructor 
//
//---------------------------------------------------------------------------
CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

//---------------------------------------------------------------------------
//
// DoDataExchange
//
//---------------------------------------------------------------------------
void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

//---------------------------------------------------------------------------
//
// the message map
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
// OnAppAbout
//
// App command to run the dialog
//---------------------------------------------------------------------------
void CHookSrvApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}
//----------------------------End of the file -------------------------------