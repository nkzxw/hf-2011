#if !defined(AFX_ACLSUB_H__8A95C596_5BBA_4F27_BEB1_1043A34A5B4E__INCLUDED_)
#define AFX_ACLSUB_H__8A95C596_5BBA_4F27_BEB1_1043A34A5B4E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AclSub.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAclSub dialog

#include "AclApp.h"
#include "AclWeb.h"
#include "AclNnb.h"
#include "AclIcmp.h"
#include "AclTorjan.h"
#include "AclTime.h"
#include "AclNet.h"

#include "AclQuery.h"

class CAclSub : public CPasseckDialog
{
// Construction
public:
	CAclSub(CWnd* pParent = NULL);   // standard constructor

	void SetSelectButton(BYTE bSelectButton);
	void ShowButtonCase(BYTE bSelectButton);
	void ShowButton(BYTE nFlags = ACL_BUTTON_SHOW_ALL);
	void EnableButtonCase(BYTE bSelectButton);
	void EnableButton(BYTE nFlags);
	BOOL IsChange();
	void Apply();
	void Cancel();

	BYTE		m_bSelectedButton;
	CButtonST	m_Button[ACL_BUTTON_COUNT];
	CButtonST	m_ButtonEx[BUTTON_EX_COUNT];
	CBkStatic	m_LableTitle;

	CAclApp		m_AclApp;
	CAclWeb		m_AclWeb;
	CAclNnb		m_AclNnb;
	CAclIcmp	m_AclIcmp;
	CAclTorjan	m_AclTorjan;
	CAclTime	m_AclTime;
	CAclNet		m_AclNet;

	int FindQueryList(DWORD dwId);
	BOOL DeleteQueryList(DWORD dwId);
	CArray<DWORD, DWORD> m_arQueryList;

// Dialog Data
	//{{AFX_DATA(CAclSub)
	enum { IDD = IDD_ACL_SUB };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAclSub)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAclSub)
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnAclApp();
	afx_msg void OnAclWeb();
	afx_msg void OnAclNnb();
	afx_msg void OnAclIcmp();
	afx_msg void OnAclTorjan();
	afx_msg void OnAclNet();
	afx_msg void OnAclTime();
	afx_msg void OnAclButtonAdd();
	afx_msg void OnAclButtonEdit();
	afx_msg void OnAclButtonDelete();
	afx_msg void OnAclButtonApply();
	afx_msg void OnAclButtonCancel();
	//}}AFX_MSG
	afx_msg LRESULT OnSubNotify(UINT wParam, LONG lParam);
	afx_msg LRESULT OnAclQuery(UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACLSUB_H__8A95C596_5BBA_4F27_BEB1_1043A34A5B4E__INCLUDED_)
