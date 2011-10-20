#if !defined(AFX_ADDRULEDLG_H__BFC04DB5_65C6_11D7_9C14_CE97B9E6B96A__INCLUDED_)
#define AFX_ADDRULEDLG_H__BFC04DB5_65C6_11D7_9C14_CE97B9E6B96A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// AddRuleDlg.h : header file
//
//*********************************************************************
#include "DrvFltIp.h"
#include "TDriver.h"
#include "afxwin.h"
#include "BtnST.h"
#include "Label.h"
#include "commonFn.h"
#include "PPTooltip.h"

/////////////////////////////////////////////////////////////////////////////

// CAddRuleDlg dialog

/**
 * This class implements the dialogh box that is used for adding rules to firewall.
 *
 * @author Sudhir Mangla
 */
class CAddRuleDlg : public CDialog
{
private:
	/**
	 * Check wether given rule is valid or not
	 *
	 * @param  Specify the string to check for validity
	 * @return BOOL 
	 */
	BOOL	Verify(CString);			// Check wether given rule is valid

	// Construction
	//These are various file operations that are to performed on files
	/*********************************************
	 * open new or existing file
	 *
	 * @param  
	 * @return BOOL 
	 ********************************************/
	BOOL	NewFile(void);				// open new or existing file

	/*********************************************
	 * move to end of the file
	 *
	 * @param  
	 * @return DWORD 
	 ********************************************/
	DWORD	GotoEnd(void);				//move to end of the file

	/*********************************************
	 * This is the handle for the file which will be created for saving 
	*the user defined rules
	 ********************************************/
	HANDLE	_hFile;

	/*********************************************
	 * Save the File to disk
	 *
	 * @param  
	 * @return DWORD 
	 ********************************************/
	DWORD	SaveFile(LPCTSTR );

	/*********************************************
	 * Close the file
	 *
	 * @return BOOL 
	 ********************************************/
	BOOL	CloseFile();
public:
	/*********************************************
	 * standard constructor
	 *
	 * @param pParent 
	 * @return  
	 ********************************************/
	CAddRuleDlg(CWnd *pParent = NULL);	// standard constructor

	// Dialog Data
	//{{AFX_DATA(CAddRuleDlg)
	enum
	{
		IDD = IDD_ADDRULE
	};

	CComboBox m_protocol;
	CComboBox m_action;
	CString m_strDestIPAddr;
	CString m_strDestPortNo;
	CString m_strSrcIPAddr;
	CString m_strSrcPortNo;
	CString m_strProtocol;
	CString m_strAction;
	//}}AFX_DATA
	//**********************************************************************
	TDriver ipFltDrv;
	DWORD AddFilter(IPFilter);

	//**********************************************************************
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddRuleDlg)
protected:
	virtual void	DoDataExchange(CDataExchange *pDX); // DDX/DDV support
	//}}AFX_VIRTUAL
// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAddRuleDlg)
	afx_msg void	OnAdd();
	afx_msg void	OnKillfocusSadd();
	afx_msg void	OnKillfocusDadd();
	afx_msg void	OnAddsave();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	CNetDefenderRules* m_pActiveRule;
	LRESULT On_MenuCallback(WPARAM wParam, LPARAM lParam);
public:
	virtual BOOL OnInitDialog();
	CButtonST m_btnRuleAddSave;
	CButtonST m_btnCancel;
	CLabel	m_staticAddNewRule;
	CLabel	m_staticSrcAdd;
	CLabel	m_staticDestAdd;
	CLabel	m_staticSrcPort;
	CLabel	m_staticDestPort;
	CLabel	m_staticAction;
	CLabel	m_staticProtocol;
	BOOL	m_bEditRuleMode;
	void SetDlgWithRules(CNetDefenderRules*  pNetDefenderRule);
	void SettingForEditMode();
	BOOL CheckDataValidity();
	void init();
	//CPP Tooltips Object
	CPPToolTip				m_tooltip;
	void AddToolTips();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CButtonST m_cbt_SrcIPPicker;
	CButtonST m_cbt_DestIPPicker;
	afx_msg void OnDestIpMyMachine();
	afx_msg void OnDestIPAnyMachine();
	afx_msg void OnSrcIpMyMachine();
	afx_msg void OnSrcIpAnyMachine();
	CButtonST m_cbtSrcPortMenu;
	CButtonST m_cbtDestPortMenu;
	afx_msg void OnSrcAnyPort();
	afx_msg void OnSrcPortFTP();
	afx_msg void OnSrcPortSSH();
	afx_msg void OnSrcPortTelnet();
	afx_msg void OnSrcPortSMTP();
	afx_msg void OnSrcPortDNS();
	afx_msg void OnSrcPortBootp();
	afx_msg void OnSrcPortHTTP();
	afx_msg void OnSrcPortPOP3();
	afx_msg void OnSrcPortNNTP();
	afx_msg void OnSrcPortNetBIOS();
	afx_msg void OnSrcPortSOCKS();
	afx_msg void OnDestPortAny();
	afx_msg void OnDestPortFTP();
	afx_msg void OnDestPortSSH();
	afx_msg void OnDestPortTelnet();
	afx_msg void OnDestPortSMTP();
	afx_msg void OnDestPortDNS();
	afx_msg void OnDestPortDHCP();
	afx_msg void OnDestPortHTTP();
	afx_msg void OnDestPortPOP3();
	afx_msg void OnDestPortNNTP();
	afx_msg void OnDestPortNetBIOS();
	afx_msg void OnDestPortSocks();
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#endif // !defined(AFX_ADDRULEDLG_H__BFC04DB5_65C6_11D7_9C14_CE97B9E6B96A__INCLUDED_)
