#if !defined(AFX_ABOUT_H__FEFA1150_7AA4_4384_8092_9E4BBE64C1BD__INCLUDED_)
#define AFX_ABOUT_H__FEFA1150_7AA4_4384_8092_9E4BBE64C1BD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// About.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAbout dialog

class CAbout : public CPasseckDialog
{
// Construction
public:
	CAbout(CWnd* pParent = NULL);   // standard constructor

private:
	CHyperLink		m_LinkEmail;
	CHyperLink		m_LinkUrl;
	CBrush			m_hbr;

// Dialog Data
	//{{AFX_DATA(CAbout)
	enum { IDD = IDD_ABOUT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAbout)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAbout)
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ABOUT_H__FEFA1150_7AA4_4384_8092_9E4BBE64C1BD__INCLUDED_)
