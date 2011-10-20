#if !defined(AFX_DISPLAYRULEVIEW_H__D55E4495_4BB5_470A_A39B_28C2CEBC9AFC__INCLUDED_)
#define AFX_DISPLAYRULEVIEW_H__D55E4495_4BB5_470A_A39B_28C2CEBC9AFC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DisplayRuleView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDisplayRuleView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CDisplayRuleView : public CFormView
{
protected:
	CDisplayRuleView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CDisplayRuleView)

// Form Data
public:
	//{{AFX_DATA(CDisplayRuleView)
	enum { IDD = IDD_VIEW_RULE_DIALOG };
	CString	m_strAction;
	CString	m_strDirection;
	CString	m_strProtocol;
	CString	m_strUse;
	CString	m_strSrcStartIP;
	CString	m_strSrcEndIP;
	CString	m_strSrcStartPort;
	CString	m_strSrcEndPort;
	CString	m_strDstStartIP;
	CString	m_strDstEndIP;
	CString	m_strDstStartPort;
	CString	m_strDstEndPort;
	CString	m_strMemo;
	CString	m_strAdapterName;
	//}}AFX_DATA

// Attributes
public:

// Operations
public:
	BOOL RefreshRuleItem(RULE_ITEM * pItem);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDisplayRuleView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CDisplayRuleView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CDisplayRuleView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISPLAYRULEVIEW_H__D55E4495_4BB5_470A_A39B_28C2CEBC9AFC__INCLUDED_)
