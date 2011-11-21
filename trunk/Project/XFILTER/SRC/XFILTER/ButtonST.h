	#if !defined(AFX_BUTTONST_H__E2E0E411_DF06_406C_8C45_6978F19011DF__INCLUDED_)
#define AFX_BUTTONST_H__E2E0E411_DF06_406C_8C45_6978F19011DF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ButtonST.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CButtonST window

class CButtonST : public CButton
{
// Construction
public:
	CButtonST();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CButtonST)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CButtonST();

	// Generated message map functions
protected:
	//{{AFX_MSG(CButtonST)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BUTTONST_H__E2E0E411_DF06_406C_8C45_6978F19011DF__INCLUDED_)
