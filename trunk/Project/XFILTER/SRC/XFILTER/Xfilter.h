// xfilter.h : main header file for the XFILTER application
//

#if !defined(AFX_XFILTER_H__2FF681AA_8106_4780_96EA_2DD63FB548C6__INCLUDED_)
#define AFX_XFILTER_H__2FF681AA_8106_4780_96EA_2DD63FB548C6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CXfilterApp:
// See xfilter.cpp for the implementation of this class
//

#include "MainDlg.h"

//extern CBrush m_hbr;

class CXfilterApp : public CWinApp
{
public:
	CXfilterApp();
	BOOL RunCommandLine(LPCTSTR lpszCommandLine);
	BOOL OpenDll();

	CMainDlg		m_pMainDlg;
	CAclFile		m_AclFile;
	HINSTANCE		m_hTcpIpDog;
	XF_IO_CONTROL	m_pDllIoControl;

	DWORD			m_dwSubWindowCount;
	

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXfilterApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void WinHelp(DWORD dwData, UINT nCmd = HH_HELP_CONTEXT);
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CXfilterApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CXfilterApp theApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XFILTER_H__2FF681AA_8106_4780_96EA_2DD63FB548C6__INCLUDED_)
