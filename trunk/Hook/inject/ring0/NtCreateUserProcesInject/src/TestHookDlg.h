// TestHookDlg.h : header file
//

#if !defined(AFX_TESTHOOKDLG_H__6A9DBAF4_B3CB_4908_9384_0DFEFD71A73C__INCLUDED_)
#define AFX_TESTHOOKDLG_H__6A9DBAF4_B3CB_4908_9384_0DFEFD71A73C__INCLUDED_

#include <string>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WAIT_NONE			0x00
#define WAIT_IDLE			0x01
#define WAIT_TIME			0x02
#define WAIT_INFINITE		0x04

/////////////////////////////////////////////////////////////////////////////
// CTestHookDlg dialog

class CTestHookDlg : public CDialog
{
// Construction
public:
	CTestHookDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CTestHookDlg)
	enum { IDD = IDD_TESTHOOK_DIALOG };
	CString	m_AutoDll;
	CString	m_InjectDll;
	DWORD	m_InjectPid;
	int		m_Inject;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTestHookDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CTestHookDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnBtnSetup();
	afx_msg void OnBtnSelect1();
	afx_msg void OnBtnAutoStart();
	afx_msg void OnBtnSelect2();
	afx_msg void OnBtnInject();
	afx_msg void OnBtnStartsvc();
	afx_msg void OnBtnAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	void UpdateStatus();
	void RunExe(LPCTSTR command, int m_Show, DWORD m_Wait = WAIT_NONE);
	BOOL GetSelectedFile(CString &m_Path,CString m_InitPath,BOOL IsOpenFile,CString FileName);
private:
	std::string m_AppPath;
	BOOLEAN		m_AutoStart;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TESTHOOKDLG_H__6A9DBAF4_B3CB_4908_9384_0DFEFD71A73C__INCLUDED_)
