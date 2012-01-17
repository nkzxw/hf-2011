#if !defined(AFX_PAGE3_H__89BC1E21_B781_4E95_8AB4_D267147CD1AE__INCLUDED_)
#define AFX_PAGE3_H__89BC1E21_B781_4E95_8AB4_D267147CD1AE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// page3.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// page3 dialog

class page3 : public CDialog
{
// Construction
public:
	page3(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(page3)
	enum { IDD = IDD_PAGE3 };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(page3)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(page3)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGE3_H__89BC1E21_B781_4E95_8AB4_D267147CD1AE__INCLUDED_)
