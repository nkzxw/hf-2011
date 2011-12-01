// SampleDriverDlg.h : header file
//

#if !defined(AFX_SAMPLEDRIVERDLG_H__6216E412_B260_4571_A3E5_3CF7B31F2D62__INCLUDED_)
#define AFX_SAMPLEDRIVERDLG_H__6216E412_B260_4571_A3E5_3CF7B31F2D62__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CSampleDriverDlg dialog

class CSampleDriverDlg : public CDialog
{
// Construction
public:
	CSampleDriverDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CSampleDriverDlg)
	enum { IDD = IDD_SAMPLEDRIVER_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSampleDriverDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CSampleDriverDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAMPLEDRIVERDLG_H__6216E412_B260_4571_A3E5_3CF7B31F2D62__INCLUDED_)
