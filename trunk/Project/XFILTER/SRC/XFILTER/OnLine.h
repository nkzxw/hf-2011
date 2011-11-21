#if !defined(AFX_ONLINE_H__33EE38AF_CC9C_4D3A_9C59_22B6ACA1923D__INCLUDED_)
#define AFX_ONLINE_H__33EE38AF_CC9C_4D3A_9C59_22B6ACA1923D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OnLine.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COnLine dialog

static UINT ONLINE_BUTTON[] = {
	IDC_BUTTON_REGISTER,
	IDC_BUTTON_CHECK_NET_COMMAND,
};
#define ONLINE_BUTTON_COUNT		sizeof(ONLINE_BUTTON)/sizeof(UINT)

static UINT ONLINE_HYPERLINK[] = {
	IDC_ABOUT_LABLE_WEB_ADDRESS,
	IDC_ABOUT_LABLE_EMAIL,
	IDC_ABOUT_LABLE_EMAIL_TORJAN,
};
#define ONLINE_HYPERYLINK_COUNT	sizeof(ONLINE_HYPERLINK)/sizeof(UINT)

class COnLine : public CPasseckDialog
{
// Construction
public:
	COnLine(CWnd* pParent = NULL);   // standard constructor
	CHttpRequest* GetInternet(){return &m_Internet;}		


private:
	BOOL UpdateInterval();

private:
	CButtonST	m_Button[ONLINE_BUTTON_COUNT];
	CHyperLink	m_HyperLink[ONLINE_HYPERYLINK_COUNT];
	CButton		m_CheckUpdate;
	CEdit		m_EditUpdate;
	CBrush		m_hbr;

	CHttpRequest m_Internet;

// Dialog Data
	//{{AFX_DATA(COnLine)
	enum { IDD = IDD_ONLINE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COnLine)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(COnLine)
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnCheckNetCommand();
	afx_msg void OnChangeEditUpdateInteral();
	afx_msg void OnButtonRegister();
	afx_msg void OnButtonCheckNetCommand();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	afx_msg LRESULT OnReturn(UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ONLINE_H__33EE38AF_CC9C_4D3A_9C59_22B6ACA1923D__INCLUDED_)
