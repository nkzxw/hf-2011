#if !defined(AFX_RULEDLG_H__4EF71540_D704_4868_A853_93709B0F806F__INCLUDED_)
#define AFX_RULEDLG_H__4EF71540_D704_4868_A853_93709B0F806F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RuleDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRuleDlg dialog

class CRuleDlg : public CDialog
{
public:
	static RULE_ITEM m_RuleItem;	// Ҫ��ӵĹ���
	static BOOL		 m_bAppQuery;	// �ǲ�������DLLģ���ѯ��
	static CString   m_sPathName;	// ���m_bAppQueryΪTRUE���˱���������
									// ����ѯ�ʵ�Ӧ�ó��������

// Construction
public:
	CRuleDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRuleDlg)
	enum { IDD = IDD_RULE };
	CComboBox	m_ComboType;
	CStatic	m_RuleTitle;
	CEdit	m_EditPort;
	CEdit	m_EditMemo;
	CComboBox	m_ComboDirection;
	CComboBox	m_ComboAction;
	CComboBox	m_ComboApp;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRuleDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRuleDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeType();
	virtual void OnOK();
	afx_msg void OnSelchangeApplication();
	afx_msg void OnAppBrowser();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RULEDLG_H__4EF71540_D704_4868_A853_93709B0F806F__INCLUDED_)
