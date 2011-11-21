#if !defined(AFX_ACLSET_H__D71817D0_3892_4781_BAE1_04FA28D59DFD__INCLUDED_)
#define AFX_ACLSET_H__D71817D0_3892_4781_BAE1_04FA28D59DFD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AclSet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAclSet dialog

#define ACL_SET_LABEL_EXPAIN	0

static UINT	ACL_SET_LABEL[] = {
	IDC_LABEL_ACL_APP_ADD,
	IDC_LABEL_EXPLAIN,
	IDC_LABEL_BASE,
	IDC_ACL_APPLICATION_LABLE,
	IDC_ACL_REMOTE_NET_LABLE,
	IDC_ACL_ACCESS_TIME_LABLE,
	IDC_ACL_DERECTION_LABLE,
	IDC_ACL_ACTION_LABLE,
//	IDC_LABEL_ADVANCE,
	IDC_ACL_SERVICE_TYPE_LABLE,
	IDC_LABEL_LOCAL_PORT,
	IDC_ACL_SERVICE_PORT_LABLE,
	IDC_LABEL_MEM
};
#define ACL_SET_LABEL_COUNT		sizeof(ACL_SET_LABEL)/sizeof(UINT)

#define ACL_SET_COMBO_APP		0
#define ACL_SET_COMBO_NET		1
#define ACL_SET_COMBO_TIME		2
#define ACL_SET_COMBO_DIR		3
#define ACL_SET_COMBO_ACTION	4
#define ACL_SET_COMBO_PROTOCOL	5

static UINT ACL_SET_COMBO[] = {
	IDC_COMBO_APPLICATION,
	IDC_COMBO_REMOTE_NET,
	IDC_COMBO_ACCESS_TIME,
	IDC_COMBO_DIRECTION,
	IDC_COMBO_ACTION,
	IDC_COMBO_SERVICE_TYPE
};
#define ACL_SET_COMBO_COUNT		sizeof(ACL_SET_COMBO)/sizeof(UINT)

#define ACL_SET_EDIT_LOCAL_PORT		0
#define ACL_SET_EDIT_SERVICE_PORT	1
#define ACL_SET_EDIT_MEMO			2
static UINT ACL_SET_EDIT[] = {
	IDC_EDIT_LOCAL_PORT,
	IDC_EDIT_SERVICE_PORT,
	IDC_EDIT_MEMO,
};
#define ACL_SET_EDIT_COUNT		sizeof(ACL_SET_EDIT)/sizeof(UINT)

static UINT ACL_SET_BUTTON[] = {
	IDC_BUTTON_APPLICATION,
	IDOK,
	IDCANCEL
};
#define ACL_SET_BUTTON_COUNT	sizeof(ACL_SET_BUTTON)/sizeof(UINT)

class CAclSet : public CPasseckDialog
{
// Construction
public:
	CAclSet(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAclSet();

private:
	void InitDialog();
	void ShowAcl();
	CString MakeExplain(BOOL bIsAppSelChange = FALSE);
	void SetPort(WORD wLocalPort, BOOL bLocalIsEnable, WORD wRemotePort, BOOL bRemoteIsEnable);

public:
	PXACL	GetAcl(){return &m_Acl;}
	void	SetAcl(PXACL pAcl){m_Acl = *pAcl;}
	BOOL	IsChange(){return m_bIsChange;}
	void	SetAutoPort(BOOL bIsAutoPort){m_bIsAutoPort = bIsAutoPort;}

private:
	XACL	m_Acl;
	BOOL	m_bIsChange;
	CString m_sExplain;
	CString m_sLastExplain;

	CButtonST		m_Button[ACL_SET_BUTTON_COUNT];
	CColorStatic	m_Label[ACL_SET_LABEL_COUNT];
	CComboBox		m_Combo[ACL_SET_COMBO_COUNT];
	CEdit			m_Edit[ACL_SET_EDIT_COUNT];
	CStatic			m_AppIcon;
	HICON			m_hIcon;

	BOOL m_bIsAutoPort;

// Dialog Data
	//{{AFX_DATA(CAclSet)
	enum { IDD = IDD_ACL_APP_ADD };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAclSet)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAclSet)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnButtonApplication();
	afx_msg void OnSelchangeComboServiceType();
	afx_msg void OnSelchangeComboAccessTime();
	afx_msg void OnEditchangeComboApplication();
	afx_msg void OnSelchangeComboApplication();
	afx_msg void OnSelchangeComboAction();
	afx_msg void OnSelchangeComboDirection();
	afx_msg void OnSelchangeComboRemoteNet();
	afx_msg void OnChangeEditLocalPort();
	afx_msg void OnChangeEditMemo();
	afx_msg void OnChangeEditServicePort();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACLSET_H__D71817D0_3892_4781_BAE1_04FA28D59DFD__INCLUDED_)
