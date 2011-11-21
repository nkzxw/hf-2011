#if !defined(AFX_ACLWEB_H__56B08FC1_5F91_4D1A_8AB4_74AC5C651FA4__INCLUDED_)
#define AFX_ACLWEB_H__56B08FC1_5F91_4D1A_8AB4_74AC5C651FA4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AclWeb.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAclWeb dialog

#define	RADIO_WEB_PASS		ACL_PASS_ALL
#define RADIO_WEB_QUERY		ACL_QUERY

class CAclWeb : public CPasseckDialog
{
// Construction
public:
	CAclWeb(CWnd* pParent = NULL);   // standard constructor
	void Apply();
	void Cancel();
	void Edit();
	void Add();
	void Delete();
	BYTE GetButtonFlags(){return m_bButtonFlags;}
	CListCtrl* GetList(){return &m_List;}
	void AddAcl(PXACL_WEB pAcl, BOOL bIsSelect = TRUE, BOOL bIsEdit = FALSE, int iIndex = -1);
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
	CColorStatic	m_Label[2];
	CButtonST		m_Button[2];
	BYTE			m_bSelectedButton;
	CComboBox		m_QueryCombo;
	CListCtrl		m_List;
	CWnd*			m_pParent;

	CAclHistory		m_History;
	int				m_iListIndex;

// Dialog Data
	//{{AFX_DATA(CAclWeb)
	enum { IDD = IDD_ACL_WEB };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAclWeb)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAclWeb)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnAclRadioPass();
	afx_msg void OnAclRadioQuery();
	afx_msg void OnClickAclList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkAclList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedAclList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeComboSet();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACLWEB_H__56B08FC1_5F91_4D1A_8AB4_74AC5C651FA4__INCLUDED_)
