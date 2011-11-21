#if !defined(AFX_ACLAPP_H__F029F811_C53C_4D39_BD28_FFBDE10898C2__INCLUDED_)
#define AFX_ACLAPP_H__F029F811_C53C_4D39_BD28_FFBDE10898C2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AclApp.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAclApp dialog

#define	RADIO_APP_PASS		ACL_PASS_ALL
#define RADIO_APP_DENYIN	ACL_DENY_IN
#define	RADIO_APP_DENYOUT	ACL_DENY_OUT
#define RADIO_APP_DENY		ACL_DENY_ALL
#define RADIO_APP_QUERY		ACL_QUERY

class CAclApp : public CPasseckDialog
{
// Construction
public:
	CAclApp(CWnd* pParent = NULL);   // standard constructor
	void Apply();
	void Cancel();
	void Edit();
	void Add();
	void Delete();
	BYTE GetButtonFlags(){return m_bButtonFlags;}
	CListCtrl* GetList(){return &m_List;}
	void AddAcl(PXACL pAcl, BOOL bIsSelect = TRUE, BOOL bIsEdit = FALSE, int iIndex = -1);
	void SetPass(BOOL IsPassAll = FALSE);

private:
	void SetSelectButton(BYTE bSelectButton);
	void SetButtonFlags(BYTE bButtonFlags){m_bButtonFlags = bButtonFlags;}
	void InitView();
	void InitList();
	BOOL SendSet(BYTE bOptType);
	BOOL SendMessageEx(BYTE nFlags);
	void EnableButton(BOOL bEnable);

private:
	BYTE			m_bButtonFlags;
	CColorStatic	m_Label[5];
	CButtonST		m_Button[5];
	CComboBox		m_QueryCombo;
	CListCtrl		m_List;
	CImageList		m_ImageList;
	CWnd*			m_pParent;
	BYTE			m_bSelectedButton;
	CAclHistory		m_History;
	int				m_iListIndex;

// Dialog Data
	//{{AFX_DATA(CAclApp)
	enum { IDD = IDD_ACL_APP };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAclApp)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAclApp)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnAclRadioPass();
	afx_msg void OnAclRadioDeny();
	afx_msg void OnAclRadioQuery();
	afx_msg void OnSelchangeComboSet();
	afx_msg void OnClickAclList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkAclList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedAclList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteallitemsAclList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitemAclList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnAclRadioDenyin();
	afx_msg void OnAclRadioDenyout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACLAPP_H__F029F811_C53C_4D39_BD28_FFBDE10898C2__INCLUDED_)
