#if !defined(AFX_ADDRULEDLG_H__994E7E78_0AE4_4B2F_B1DC_4C4487162E94__INCLUDED_)
#define AFX_ADDRULEDLG_H__994E7E78_0AE4_4B2F_B1DC_4C4487162E94__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddRuleDlg.h : header file
//

const static int ADD_RULE    = 0;
const static int MODIFY_RULE = 1;

/////////////////////////////////////////////////////////////////////////////
// CAddRuleDlg dialog

class CAddRuleDlg : public CDialog
{
public:
    LPBYTE      m_pBuffer;
    RULE_ITEM * m_pItem;
    int         m_nType; // 0 - ADD_RULE  
                         // 1 - MODIFY_RULE

// Construction
public:
	CAddRuleDlg(CWnd* pParent = NULL);  // standard constructor
    
// Dialog Data
	//{{AFX_DATA(CAddRuleDlg)
	enum { IDD = IDD_ADD_RULE_DIALOG };
	CEdit	    m_ctlMemo;
	CButton	    m_ctlUse;
	CComboBox	m_ctlAction;
	CComboBox	m_ctlDirection;
	CComboBox	m_ctlProtocol;
	CEdit	    m_ctlDstEndPort;
	CEdit	    m_ctlDstStartPort;
	CEdit	    m_ctlSrcEndPort;
	CEdit	    m_ctlSrcStartPort;
	CIPAddressCtrl	m_ctlDstEndIP;
	CIPAddressCtrl	m_ctlDstStartIP;
	CIPAddressCtrl	m_ctlSrcEndIP;
	CIPAddressCtrl	m_ctlSrcStartIP;
	UINT	m_iSrcStartPort;
	UINT	m_iSrcEndPort;
	UINT	m_iDstStartPort;
	UINT	m_iDstEndPort;
	CString	m_strMemo;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddRuleDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddRuleDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDRULEDLG_H__994E7E78_0AE4_4B2F_B1DC_4C4487162E94__INCLUDED_)
