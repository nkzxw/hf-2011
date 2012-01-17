#if !defined(AFX_PAGE1_H__DD3CA892_98E1_4275_A4D5_0A8E0A33DF80__INCLUDED_)
#define AFX_PAGE1_H__DD3CA892_98E1_4275_A4D5_0A8E0A33DF80__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// page1.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// page1 dialog
#include "pageview.h"
class page1 : public CDialog
{
// Construction
public:
	page1(CWnd* pParent = NULL);   // standard constructor
    Cpageview m_page1;	
	void setcheck();
// Dialog Data
	//{{AFX_DATA(page1)
	enum { IDD = IDD_PAGE1 };
CButton	m_p1check1;
CButton	m_p1check2;
CButton	m_p1check3;
CButton	m_p1check4;
CButton	m_p1check5;
CButton	m_p1check6;
CButton	m_p1check7;
CButton	m_p1check8;
CButton	m_p1check9;
CButton	m_p1check10;
CButton	m_p1check11;
CButton	m_p1check12;
CButton	m_p1check13;
CButton	m_p1check14;

	//}}AFX_DATA
int i_p1check1;
int i_p1check2;
int i_p1check3;
int i_p1check4;
int i_p1check5;
int i_p1check6;
int i_p1check7;
int i_p1check8;
int i_p1check9;
int i_p1check10;
int i_p1check11;
int i_p1check12;
int i_p1check13;
int i_p1check14;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(page1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:


	// Generated message map functions
	//{{AFX_MSG(page1)
	virtual BOOL OnInitDialog();
	afx_msg void OnP1Check1();
	afx_msg void OnP1check2();
afx_msg void OnP1check3();
afx_msg void OnP1check4();
afx_msg void OnP1check5();
afx_msg void OnP1check6();
afx_msg void OnP1check7();
afx_msg void OnP1check8();
afx_msg void OnP1check9();
afx_msg void OnP1check10();
afx_msg void OnP1check11();
afx_msg void OnP1check12();
afx_msg void OnP1check13();
afx_msg void OnP1check14();

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGE1_H__DD3CA892_98E1_4275_A4D5_0A8E0A33DF80__INCLUDED_)
