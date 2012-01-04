// GalaxyAPDlg.h : header file
//

#if !defined(AFX_GALAXYAPDLG_H__2AB502A3_28E6_4663_93BD_F16D2898361A__INCLUDED_)
#define AFX_GALAXYAPDLG_H__2AB502A3_28E6_4663_93BD_F16D2898361A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "GalaxyApMGR.h"
/////////////////////////////////////////////////////////////////////////////
// CGalaxyAPDlg dialog

class CGalaxyAPDlg : public CDialog
{
// Construction
public:
	CGalaxyAPDlg(CWnd* pParent = NULL);	// standard constructor
	GalaxyApMGR		m_GalaxyApMGR;
// Dialog Data
	//{{AFX_DATA(CGalaxyAPDlg)
	enum { IDD = IDD_GALAXYAP_DIALOG };
	CEdit	m_0x;
	CEdit	m_strongOd;
	CButton	m_chkPID;
	CEdit	m_edtPID;
	CRichEditCtrl	m_richLOG;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGalaxyAPDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CGalaxyAPDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBUTTONReOsAndKidisp();
	afx_msg void OnBUTTONcleanLog();
	afx_msg void OnBUTTONkidisp();
	afx_msg void OnBUTTONunloadKidisp();
	afx_msg void OnBUTTONreloadOS();
	afx_msg void OnBUTTONUnHookPort();
	afx_msg void OnButtonProinfo();
	afx_msg void OnBUTTONUnloadALl();
	afx_msg void OnButton_patchSOD();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GALAXYAPDLG_H__2AB502A3_28E6_4663_93BD_F16D2898361A__INCLUDED_)
