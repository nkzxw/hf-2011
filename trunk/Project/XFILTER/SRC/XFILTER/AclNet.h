#if !defined(AFX_ACLNET_H__00A61BB1_F202_4FA9_A4B3_3EBCCB57B068__INCLUDED_)
#define AFX_ACLNET_H__00A61BB1_F202_4FA9_A4B3_3EBCCB57B068__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AclNet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAclNet dialog

class CAclNet : public CPasseckDialog
{
// Construction
public:
	CAclNet(CWnd* pParent = NULL);   // standard constructor
	void Apply();
	void Cancel();
	void Edit();
	void Add();
	void Delete();
	BYTE GetButtonFlags(){return m_bButtonFlags;}

private:
	void ListAddIp();
	int ListAddIp(PXACL_IP pAclIp);
	int ListAddOne(XACL_IP* pAclIp, BOOL bIsSelect = FALSE, BOOL bIsEdit = FALSE, int iIndex = -1);
	int GetType(int nIndex);
	void SetButtonFlags(BYTE bButtonFlags){m_bButtonFlags = bButtonFlags;}
	BOOL SendMessageEx(BYTE nFlags);
	int EnableButton(BOOL bEnableEdit);
	void ReadAllIp();
	void ReadIp(PXACL_IP pFirst, int nType);

private:
	BYTE		m_bButtonFlags;
	CTreeCtrl	m_Tree;
	CListCtrl	m_List;
	CWnd*		m_pParent;
	int			m_iTreeIndex;
	int			m_iListIndex;
	BYTE		m_bType;
	CAclHistory m_History[ACL_NET_TYPE_COUNT];

	CArray<XACL_IP, XACL_IP> m_arIp[ACL_NET_TYPE_COUNT];

// Dialog Data
	//{{AFX_DATA(CAclNet)
	enum { IDD = IDD_ACL_NET };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAclNet)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAclNet)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnSelchangedNetTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickListNet(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkListNet(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedListNet(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACLNET_H__00A61BB1_F202_4FA9_A4B3_3EBCCB57B068__INCLUDED_)
