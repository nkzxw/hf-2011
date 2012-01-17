#if !defined(AFX_PAGE4_H__AE1B77AE_47BF_4C9A_B90B_ACEED3743035__INCLUDED_)
#define AFX_PAGE4_H__AE1B77AE_47BF_4C9A_B90B_ACEED3743035__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// page4.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// page4 dialog

class page4 : public CDialog
{
// Construction
public:
	page4(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(page4)
	enum { IDD = IDD_PAGE4 };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(page4)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(page4)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGE4_H__AE1B77AE_47BF_4C9A_B90B_ACEED3743035__INCLUDED_)
