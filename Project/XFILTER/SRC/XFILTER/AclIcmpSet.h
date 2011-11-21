#if !defined(AFX_ACLICMPSET_H__356231BC_A2E0_48DB_AAC3_E8D2502C49D7__INCLUDED_)
#define AFX_ACLICMPSET_H__356231BC_A2E0_48DB_AAC3_E8D2502C49D7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AclIcmpSet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAclIcmpSet dialog

static UINT	ACL_ICMP_SET_LABEL[] = {
	IDC_LABEL_BASE,
	IDC_ACL_REMOTE_NET_LABLE,
	IDC_ACL_ACCESS_TIME_LABLE,
	IDC_ACL_DERECTION_LABLE,
	IDC_ACL_ACTION_LABLE,
	IDC_LABEL_ADVANCE,
};
#define ACL_ICMP_SET_LABEL_COUNT		sizeof(ACL_ICMP_SET_LABEL)/sizeof(UINT)

#define ACL_ICMP_SET_LIST_NET		0
#define ACL_ICMP_SET_LIST_TIME		1
#define ACL_ICMP_SET_LIST_DIR		2
#define ACL_ICMP_SET_LIST_ACTION	3

static UINT ACL_ICMP_SET_LIST[] = {
	IDC_LIST_NET,
	IDC_LIST_TIME,
	IDC_LIST_DIR,
	IDC_LIST_ACTION
};
#define ACL_ICMP_SET_LIST_COUNT		sizeof(ACL_ICMP_SET_LIST)/sizeof(UINT)

#define ACL_ICMP_SET_EDIT_MEMO			0
static UINT ACL_ICMP_SET_EDIT[] = {
	IDC_EDIT_MEMO
};
#define ACL_ICMP_SET_EDIT_COUNT		sizeof(ACL_ICMP_SET_EDIT)/sizeof(UINT)

static UINT ACL_ICMP_SET_BUTTON[] = {
	IDOK,
	IDCANCEL
};
#define ACL_ICMP_SET_BUTTON_COUNT	sizeof(ACL_ICMP_SET_BUTTON)/sizeof(UINT)


class CAclIcmpSet : public CPasseckDialog
{
// Construction
public:
	CAclIcmpSet(CWnd* pParent = NULL);   // standard constructor

public:
	PXACL_ICMP	GetAcl(){return &m_Acl;}
	void		SetAcl(PXACL_ICMP pAcl){m_Acl = *pAcl;}
	BOOL		IsChange(){return m_bIsChange;}

private:
	void InitDialog();
	void ShowAcl();


private:
	XACL_ICMP	m_Acl;
	BOOL		m_bIsChange;

	CButtonST		m_Button[ACL_ICMP_SET_BUTTON_COUNT];
	CColorStatic	m_Label[ACL_ICMP_SET_LABEL_COUNT];
	CListBox		m_List[ACL_ICMP_SET_LIST_COUNT];
	CEdit			m_Edit[ACL_ICMP_SET_EDIT_COUNT];

// Dialog Data
	//{{AFX_DATA(CAclIcmpSet)
	enum { IDD = IDD_ACL_ICMP_ADD };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAclIcmpSet)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAclIcmpSet)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACLICMPSET_H__356231BC_A2E0_48DB_AAC3_E8D2502C49D7__INCLUDED_)
