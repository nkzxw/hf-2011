#if !defined(AFX_ACLNETSET_H__0373DEE1_1154_4AEF_8907_530991950F78__INCLUDED_)
#define AFX_ACLNETSET_H__0373DEE1_1154_4AEF_8907_530991950F78__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AclNetSet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAclNetSet dialog

class CAclNetSet : public CPasseckDialog
{
// Construction
public:
	CAclNetSet(CWnd* pParent = NULL);   // standard constructor
	PXACL_IP GetIp(){return &m_Ip;}
	void SetEdit(BOOL bIsEdit){m_bIsEdit = bIsEdit;}
	BOOL IsChange(){return m_bIsChange;}

private:
	void InitDialog();

private:
	XACL_IP		m_Ip;
	CButtonST	m_Button[2];
	CColorStatic m_Label[3];
	BOOL		m_bIsEdit;
	BOOL		m_bIsChange;

// Dialog Data
	//{{AFX_DATA(CAclNetSet)
	enum { IDD = IDD_ACL_NET_ADD };
	CIPAddressCtrl	m_IPStart;
	CIPAddressCtrl	m_IPEnd;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAclNetSet)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAclNetSet)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACLNETSET_H__0373DEE1_1154_4AEF_8907_530991950F78__INCLUDED_)
