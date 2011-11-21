#if !defined(AFX_ACLNNBSET_H__C3C1729A_5BB1_4916_8ADF_AC998C78D969__INCLUDED_)
#define AFX_ACLNNBSET_H__C3C1729A_5BB1_4916_8ADF_AC998C78D969__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AclNnbSet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAclNnbSet dialog

static UINT	ACL_NNB_SET_LABEL[] = {
	IDC_LABEL_BASE,
	IDC_ACL_REMOTE_NET_LABLE,
	IDC_ACL_ACCESS_TIME_LABLE,
	IDC_ACL_DERECTION_LABLE,
	IDC_ACL_ACTION_LABLE,
	IDC_LABEL_ADVANCE,
	IDC_LABEL_COSTOM
};
#define ACL_NNB_SET_LABEL_COUNT		sizeof(ACL_NNB_SET_LABEL)/sizeof(UINT)

#define ACL_NNB_SET_LIST_NET		0
#define ACL_NNB_SET_LIST_NNB		ACL_NNB_SET_LIST_NET
#define ACL_NNB_SET_LIST_TIME		1
#define ACL_NNB_SET_LIST_DIR		2
#define ACL_NNB_SET_LIST_ACTION		3

static UINT ACL_NNB_SET_LIST[] = {
	IDC_LIST_NET,
	IDC_LIST_TIME,
	IDC_LIST_DIR,
	IDC_LIST_ACTION
};
#define ACL_NNB_SET_LIST_COUNT		sizeof(ACL_NNB_SET_LIST)/sizeof(UINT)

#define ACL_NNB_SET_EDIT_MEMO			0
#define ACL_NNB_SET_EDIT_COSTOM			1
static UINT ACL_NNB_SET_EDIT[] = {
	IDC_EDIT_MEMO,
	IDC_EDIT_COSTOM
};
#define ACL_NNB_SET_EDIT_COUNT		sizeof(ACL_NNB_SET_EDIT)/sizeof(UINT)

static UINT ACL_NNB_SET_BUTTON[] = {
	IDOK,
	IDCANCEL
};
#define ACL_NNB_SET_BUTTON_COUNT	sizeof(ACL_NNB_SET_BUTTON)/sizeof(UINT)

class CAclNnbSet : public CPasseckDialog
{
// Construction
public:
	CAclNnbSet(CWnd* pParent = NULL);   // standard constructor

private:
	void InitDialog();
	void ShowAcl();
	void EnumNnb();
	DWORD GetIpFromName(LPCTSTR sName);

public:
	PXACL_NNB	GetAcl(){return &m_Acl;}
	void		SetAcl(PXACL_NNB pAcl){m_Acl = *pAcl;}
	BOOL		IsChange(){return m_bIsChange;}

private:
	XACL_NNB	m_Acl;
	BOOL		m_bIsChange;

	CButtonST		m_Button[ACL_NNB_SET_BUTTON_COUNT];
	CColorStatic	m_Label[ACL_NNB_SET_LABEL_COUNT];
	CListBox		m_List[ACL_NNB_SET_LIST_COUNT];
	CEdit			m_Edit[ACL_NNB_SET_EDIT_COUNT];

// Dialog Data
	//{{AFX_DATA(CAclNnbSet)
	enum { IDD = IDD_ACL_NNB_ADD };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAclNnbSet)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAclNnbSet)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnSelchangeListNet();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACLNNBSET_H__C3C1729A_5BB1_4916_8ADF_AC998C78D969__INCLUDED_)
