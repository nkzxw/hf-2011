// PhoenixFWDlg.h : header file
//

#if !defined(AFX_PHOENIXFWDLG_H__9610B9FB_FA76_47FE_BAEE_177797F7B329__INCLUDED_)
#define AFX_PHOENIXFWDLG_H__9610B9FB_FA76_47FE_BAEE_177797F7B329__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CMainDlg dialog

#include "MonitorPage.h"
#include "RulePage.h"
#include "SyssetPage.h"
#include "KerRulePage.h"

class CMainDlg : public CDialog
{
// Construction
public:
	CMainDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMainDlg)
	enum { IDD = IDD_PHOENIXFW_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	HICON m_hIcon;

	CPropertySheet	m_sheet;		// ����ҳ��������������4����ǩ��ҳ�棩

	CRulePage		m_RulePage;		// ������ʼ���ҳ��
	CMonitorPage	m_MonitorPage;	// Ӧ�ò���˹���ҳ��
	CSyssetPage		m_SyssetPage;	// ���Ĳ���˹���ҳ��
	CKerRulePage	m_KerRulePage;	// ϵͳ����ҳ��

	// Generated message map functions
	//{{AFX_MSG(CMainDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	afx_msg long OnQueryAcl(WPARAM wParam, LPARAM lParam);
	afx_msg long OnSessionNotify(WPARAM wParam, LPARAM lParam);
	afx_msg void OnApply();
	virtual void OnCancel();
	afx_msg void OnAnnul();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PHOENIXFWDLG_H__9610B9FB_FA76_47FE_BAEE_177797F7B329__INCLUDED_)
