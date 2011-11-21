// xfilterDlg.h : header file
//

#if !defined(AFX_XFILTERDLG_H__8C3F75B2_BE7E_405E_A854_1E553B90EBFF__INCLUDED_)
#define AFX_XFILTERDLG_H__8C3F75B2_BE7E_405E_A854_1E553B90EBFF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CXfilterDlg dialog

class CXfilterDlg : public CDialog
{
// Construction
public:
	CXfilterDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CXfilterDlg)
	enum { IDD = IDD_XFILTER_DIALOG };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXfilterDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	ServiceControl	IpFilterDriver;

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CXfilterDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnLoad();
	afx_msg void OnSetAcl();
	afx_msg void OnLoadVxd();
	afx_msg void OnUnloadVxd();
	afx_msg void OnPrintProcess();
	afx_msg void OnMain();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void OnOK();
	afx_msg void OnButton1();
	afx_msg void OnButton2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XFILTERDLG_H__8C3F75B2_BE7E_405E_A854_1E553B90EBFF__INCLUDED_)
