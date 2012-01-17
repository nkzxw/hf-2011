#if !defined(AFX_PAGE2_H__DF469C36_0FF0_4DB1_B5D0_90B3705228E1__INCLUDED_)
#define AFX_PAGE2_H__DF469C36_0FF0_4DB1_B5D0_90B3705228E1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// page2.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// page2 dialog

#include "pageview.h"
class page2 : public CDialog
{
// Construction
public:
	page2(CWnd* pParent = NULL);   // standard constructor
    Cpageview m_page2;
    CString getcstring(UINT csID);

// Dialog Data
	//{{AFX_DATA(page2)
	enum { IDD = IDD_PAGE2 };
	CString	m_p2edit1;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(page2)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(page2)
	virtual BOOL OnInitDialog();
	afx_msg void OnButton1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGE2_H__DF469C36_0FF0_4DB1_B5D0_90B3705228E1__INCLUDED_)
