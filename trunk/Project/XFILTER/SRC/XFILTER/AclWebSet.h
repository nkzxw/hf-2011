#if !defined(AFX_ACLWEBSET_H__D97E8A74_422B_40F3_B3B9_740865538DD1__INCLUDED_)
#define AFX_ACLWEBSET_H__D97E8A74_422B_40F3_B3B9_740865538DD1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AclWebSet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAclWebSet dialog

static UINT	ACL_WEB_SET_LABEL[] = {
	IDC_LABEL_BASE,
	IDC_LABEL_ADVANCE,
	IDC_LABEL_EXPLAIN,
};
#define ACL_WEB_SET_LABEL_COUNT		sizeof(ACL_WEB_SET_LABEL)/sizeof(UINT)

#define ACL_WEB_SET_LIST_ACTION		0
static UINT ACL_WEB_SET_LIST[] = {
	IDC_LIST_ACTION
};
#define ACL_WEB_SET_LIST_COUNT		sizeof(ACL_WEB_SET_LIST)/sizeof(UINT)

#define ACL_WEB_SET_EDIT_WEB			0
#define ACL_WEB_SET_EDIT_MEMO			1
static UINT ACL_WEB_SET_EDIT[] = {
	IDC_EDIT_WEB,
	IDC_EDIT_MEMO
};
#define ACL_WEB_SET_EDIT_COUNT		sizeof(ACL_WEB_SET_EDIT)/sizeof(UINT)

static UINT ACL_WEB_SET_BUTTON[] = {
	IDOK,
	IDCANCEL
};
#define ACL_WEB_SET_BUTTON_COUNT	sizeof(ACL_WEB_SET_BUTTON)/sizeof(UINT)

class CAclWebSet : public CPasseckDialog
{
// Construction
public:
	CAclWebSet(CWnd* pParent = NULL);   // standard constructor

private:
	void InitDialog();
	void ShowAcl();

public:
	PXACL_WEB	GetAcl(){return &m_Acl;}
	void		SetAcl(PXACL_WEB pAcl){m_Acl = *pAcl;}
	BOOL		IsChange(){return m_bIsChange;}

private:
	XACL_WEB	m_Acl;
	BOOL		m_bIsChange;

	CButtonST		m_Button[ACL_WEB_SET_BUTTON_COUNT];
	CColorStatic	m_Label[ACL_WEB_SET_LABEL_COUNT];
	CListBox		m_List[ACL_WEB_SET_LIST_COUNT];
	CEdit			m_Edit[ACL_WEB_SET_EDIT_COUNT];

// Dialog Data
	//{{AFX_DATA(CAclWebSet)
	enum { IDD = IDD_ACL_WEB_ADD };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAclWebSet)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAclWebSet)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACLWEBSET_H__D97E8A74_422B_40F3_B3B9_740865538DD1__INCLUDED_)
