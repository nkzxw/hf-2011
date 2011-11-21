// xfilter.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "xfilter.h"
#include "xfilterDlg.h"

#include "Splash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef ZIGUANG
void MyInter(char *SWcode);
#endif

/////////////////////////////////////////////////////////////////////////////
// CXfilterApp

BEGIN_MESSAGE_MAP(CXfilterApp, CWinApp)
	//{{AFX_MSG_MAP(CXfilterApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CXfilterApp construction

CXfilterApp::CXfilterApp()
{
	m_hTcpIpDog = NULL;
	m_pDllIoControl = NULL;
	m_dwSubWindowCount = 0;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CXfilterApp object

CXfilterApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CXfilterApp initialization

BOOL CXfilterApp::InitInstance()
{
	//CXfilterDlg mydlg;
	//mydlg.DoModal();
	//return FALSE;

	TCHAR sTemp[MAX_PATH];
	_stprintf(sTemp, _T("%s%s"), m_AclFile.GetAppPath(), XFILTER_HELP_FILE_NAME);
	theApp.m_pszHelpFilePath = _tcsdup(sTemp);

	if(!RunCommandLine(theApp.m_lpCmdLine))
		return FALSE;

	CWnd *PrevCWnd = CWnd::FindWindow(NULL, WINDOW_CAPTION);
	if (PrevCWnd != NULL) 
		return FALSE;

	CXInstall m_Install;

	if(!m_Install.IsWinsock2())
	{
		AfxMessageBox(GUI_ACL_MESSAGE_NO_WINSOCK2);
		return FALSE;
	}

	if(!m_Install.IsInstalled() || !m_Install.IsRightVersion())
	{
		AfxMessageBox(ERROR_STRING_HAVE_NOT_INSTALL);
		return FALSE;
	}

	//
	// 2002/06/10 for windows xp
	//
	m_Install.UpdateInstall();

#ifndef ZIGUANG
	CHttpRequest m_Internet;
	if(!m_Internet.IsRegistered())
	{
		int iRet = AfxMessageBox(USER_REGISTER_QUESTION, MB_ICONQUESTION | MB_YESNO); 
		if(iRet == IDYES)
		{
			CShell m_Shell;
			if(m_Shell.RunProgram(PROGRAM_USERREG, TRUE) != SHELL_ERROR_SUCCESS)
			{
				CString Message, Caption;
				Message.LoadString(IDS_ERROR_CANT_RUN_USERREG);
				Caption.LoadString(IDS_CAPTION);
				::MessageBox(NULL, Message, Caption, MB_OK | MB_ICONWARNING);
			}
		}
	}
#endif

	if(XF_OpenDriver() == NULL)
	{
		AfxMessageBox(ERROR_STRING_DRIVER_NOT_FOUNT);
		return FALSE;
	}

	if(!OpenDll()) return FALSE;
	m_AclFile.SetDllHandle(m_pDllIoControl);

	XFILTER_IO_CONTROL ioControl;
	GetModuleFileName(NULL, sTemp, MAX_PATH);
	ioControl.DWord = GetCurrentProcessId();
	ioControl.DWord2 = (DWORD)sTemp;
	m_pDllIoControl(IO_CONTROL_SET_XFILTER_PROCESS_ID, &ioControl);

	int iRet;
	if((iRet = m_AclFile.ReadAcl()) != XERR_SUCCESS
		|| m_AclFile.GetHeader() == NULL)
	{
		AfxMessageBox(m_AclFile.GetErrorString(iRet));
		return FALSE;
	}

	if(m_AclFile.GetHeader()->bShowWelcome)
	{
		CCommandLineInfo cmdInfo;
		ParseCommandLine(cmdInfo);
		CSplashWnd::EnableSplashScreen(cmdInfo.m_bShowSplash);
	}

	m_Install.SetAutoStart(m_AclFile.GetHeader()->bAutoStart);

#ifdef ZIGUANG
	MyInter("SY0000005");
#endif

	m_pMainDlg.Create(IDD_MAIN, NULL);
	m_pMainWnd = &m_pMainDlg;
	m_pMainDlg.ShowWindow(SW_HIDE);
	m_pMainDlg.UpdateWindow();
	
	if(!StartMonitor())
	{
		AfxMessageBox(ERROR_STRING_CAN_NOT_READ_PACKET);
		m_pMainDlg.EndDialog(IDCANCEL);
		return FALSE;
	}

	StartUpdateNnb();

	return TRUE;
}

int CXfilterApp::ExitInstance() 
{
	//
	// 2002/06/10 for windows xp
	//
	CXInstall m_Install;
	m_Install.UpdateInstall();
	
	if(m_bIsUpdateNnb)
		StopUpdateNnb();
	if(m_bIsMonitor)
		StopMonitor();

	if(m_pDllIoControl != NULL)
	{
		XFILTER_IO_CONTROL ioControl;
		ioControl.Byte = XF_PASS_ALL;
		m_pDllIoControl(IO_CONTROL_SET_WORK_MODE, &ioControl);

		ioControl.DWord = 0;
		ioControl.DWord2 = 0;
		m_pDllIoControl(IO_CONTROL_SET_XFILTER_PROCESS_ID, &ioControl);
	}

	if(m_hTcpIpDog != NULL)
		FreeLibrary(m_hTcpIpDog);

	m_pMainDlg.GetOnLine()->GetInternet()->m_bIsClose = TRUE;
	m_pMainDlg.GetOnLine()->GetInternet()->StopDownload();
	while(m_pMainDlg.GetOnLine()->GetInternet()->m_IsConnecting)
		Sleep(1000);

	return CWinApp::ExitInstance();
}

BOOL CXfilterApp::RunCommandLine(LPCTSTR lpszCommandLine)
{
	TCHAR sPathName[MAX_PATH];
	_stprintf(sPathName, _T("%s%s"), GetAppPath(), XFILTER_SERVICE_DLL_NAME);

	CXInstall m_Install;
	if(_tcscmp(lpszCommandLine, _T("-install")) == 0)
	{
		int iRet;
		if((iRet = m_Install.InstallProvider(sPathName)) != XERR_SUCCESS
			&& iRet != XERR_PROVIDER_ALREADY_INSTALL)
			AfxMessageBox(GUI_ACL_MESSAGE_INSTALL_FAILED);
		m_Install.SetAutoStart(TRUE);
		return FALSE;
	}
	if(_tcscmp(lpszCommandLine, _T("-remove")) == 0)
	{
		m_Install.RemoveProvider();
		m_Install.SetAutoStart(FALSE);
		return FALSE;
	}
	return TRUE;
}

BOOL CXfilterApp::OpenDll()
{
	TCHAR sPathName[MAX_PATH];
	CXInstall m_Install;
	//if(!m_Install.IsInstalled(sPathName))
	//	_stprintf(sPathName, _T("%s%s"), GetAppPath(), XFILTER_SERVICE_DLL_NAME);

	if(!m_Install.IsInstalled(sPathName) 
		|| (m_hTcpIpDog = LoadLibrary(sPathName)) == NULL)
	{
		AfxMessageBox(GUI_ACL_MESSAGE_DLL_NOT_FOUND);
		return FALSE;
	}

	m_pDllIoControl	= (XF_IO_CONTROL)GetProcAddress(m_hTcpIpDog, _T("XfIoControl"));

	if (m_pDllIoControl == NULL)
	{
		FreeLibrary(m_hTcpIpDog);
		AfxMessageBox(GUI_ACL_MESSAGE_FUNCTION_NOT_FOUNT);
		return FALSE;
	}

	return TRUE;
}

BOOL CXfilterApp::PreTranslateMessage(MSG* pMsg)
{
	if (CSplashWnd::PreTranslateAppMessage(pMsg))
		return TRUE;

	return CWinApp::PreTranslateMessage(pMsg);
}

void CXfilterApp::WinHelp(DWORD dwData, UINT nCmd) 
{
	//if(dwData > 100) dwData = m_pMainDlg.GetSelectButton() + 2;
	//HWND hHtmlHelp = HtmlHelpA(theApp.m_pMainWnd->m_hWnd, 
	//						  theApp.m_pszHelpFilePath, 
	//						  HH_HELP_CONTEXT,
	//						  0,
	//						  dwData);

	//if(hHtmlHelp == 0)
	//	AfxMessageBox(GUI_MESSAGE_OPEN_HELP_FAILED);
}

#pragma comment( exestr, "B9D3B8FD2A7A686B6E7667742B")
