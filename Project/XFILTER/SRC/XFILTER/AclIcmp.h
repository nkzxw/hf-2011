#if !defined(AFX_ACLICMP_H__B09E44FF_61E4_4796_9634_8B33374AB73E__INCLUDED_)
#define AFX_ACLICMP_H__B09E44FF_61E4_4796_9634_8B33374AB73E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AclIcmp.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAclIcmp dialog

#define	RADIO_ICMP_PASS			ACL_PASS_ALL
#define RADIO_ICMP_DENYIN		ACL_DENY_IN
#define	RADIO_ICMP_DENYOUT		ACL_DENY_OUT
#define RADIO_ICMP_DENYALL		ACL_DENY_ALL
#define RADIO_ICMP_QUERY		ACL_QUERY


class CAclIcmp : public CPasseckDialog
{
// Construction
public:
	CAclIcmp(CWnd* pParent = NULL);   // standard constructor
	void Apply();
	void Cancel();
	void Edit();
	void Add();
	void Delete();
	BYTE GetButtonFlags(){return m_bButtonFlags;}
	CListCtrl* GetList(){return &m_List;}
	void AddAcl(PXACL_ICMP pAcl, BOOL bIsSelect = TRUE, BOOL bIsEdit = FALSE, int iIndex = -1);
	void SetPass(BOOL IsPassAll = FALSE);

private:
	void SetSelectButton(BYTE bSelectButton);
	void SetButtonFlags(BYTE bButtonFlags){m_bButtonFlags = bButtonFlags;}

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
	//{{AFX_DATA(CAclIcmp)
	enum { IDD = IDD_ACL_ICMP };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAclIcmp)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAclIcmp)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnAclRadioDenyin();
	afx_msg void OnAclRadioDenyall();
	afx_msg void OnAclRadioDenyout();
	afx_msg void OnAclRadioPass();
	afx_msg void OnAclRadioQuery();
	afx_msg void OnItemchangedAclList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickAclList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkAclList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeComboSet();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACLICMP_H__B09E44FF_61E4_4796_9634_8B33374AB73E__INCLUDED_)
