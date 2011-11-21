#if !defined(AFX_ACLNNB_H__0A4102D3_E8D1_4558_8B30_9F961FB02914__INCLUDED_)
#define AFX_ACLNNB_H__0A4102D3_E8D1_4558_8B30_9F961FB02914__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AclNnb.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAclNnb dialog

#define	RADIO_NNB_PASS			ACL_PASS_ALL
#define RADIO_NNB_DENYIN		ACL_DENY_IN
#define	RADIO_NNB_DENYOUT		ACL_DENY_OUT
#define RADIO_NNB_DENYALL		ACL_DENY_ALL
#define RADIO_NNB_QUERY			ACL_QUERY

class CAclNnb : public CPasseckDialog
{
// Construction
public:
	CAclNnb(CWnd* pParent = NULL);   // standard constructor
	void Apply();
	void Cancel();
	void Edit();
	void Add();
	void Delete();
	BYTE GetButtonFlags(){return m_bButtonFlags;}
	CListCtrl* GetList(){return &m_List;}
	void AddAcl(PXACL_NNB pAcl, BOOL bIsSelect = TRUE, BOOL bIsEdit = FALSE, int iIndex = -1);
	void SetPass(BOOL IsPassAll = FALSE);

private:
	void SetButtonFlags(BYTE bButtonFlags){m_bButtonFlags = bButtonFlags;}
	void SetSelectButton(BYTE bSelectButton);

	void InitView();
	void InitList();
	BOOL SendSet(BYTE bOptType);
	BOOL SendMessageEx(BYTE nFlags);
	void EnableButton(BOOL bEnable);

	BYTE			m_bButtonFlags;
	CColorStatic	m_Label[5];
	CButtonST		m_Button[5];
	CComboBox		m_QueryCombo;
	BYTE			m_bSelectedButton;
	CListCtrl		m_List;
	CWnd*			m_pParent;

	CAclHistory		m_History;
	int				m_iListIndex;

// Dialog Data
	//{{AFX_DATA(CAclNnb)
	enum { IDD = IDD_ACL_NNB };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAclNnb)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAclNnb)
	afx_msg void OnAclRadioPass();
	afx_msg void OnAclRadioDenyin();
	afx_msg void OnAclRadioDenyall();
	afx_msg void OnAclRadioDenyout();
	afx_msg void OnAclRadioQuery();
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnClickAclList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkAclList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedAclList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeComboSet();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACLNNB_H__0A4102D3_E8D1_4558_8B30_9F961FB02914__INCLUDED_)
