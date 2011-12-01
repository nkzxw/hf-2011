// procappDlg.h : header file
//

#if !defined(AFX_PROCAPPDLG_H__BAF076B7_33E1_49B8_BEFD_09A10B980754__INCLUDED_)
#define AFX_PROCAPPDLG_H__BAF076B7_33E1_49B8_BEFD_09A10B980754__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CProcappDlg dialog

class CProcappDlg : public CDialog
{
// Construction
public:
	CProcappDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CProcappDlg)
	enum { IDD = IDD_PROCAPP_DIALOG };
	CListBox	m_listbox;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProcappDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CProcappDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROCAPPDLG_H__BAF076B7_33E1_49B8_BEFD_09A10B980754__INCLUDED_)
