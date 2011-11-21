#if !defined(AFX_SYSTEMSET_H__1982AD04_521D_48EE_ACC5_FB6000BF571E__INCLUDED_)
#define AFX_SYSTEMSET_H__1982AD04_521D_48EE_ACC5_FB6000BF571E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SystemSet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSystemSet dialog

class CSystemSet : public CPasseckDialog
{
// Construction
public:
	CSystemSet(CWnd* pParent = NULL);   // standard constructor
	BOOL IsChangeEx();
	void Apply();
	void Cancel();

private:
	void EnableButton(BOOL IsEnabled);
	BOOL IsChange();
	void Refresh();

private:
	CButtonST	 m_ButtonApply;
	CButtonST	 m_ButtonCancel;

	CComboBox	 m_ListLogSize;
	CButton		 m_CheckSplash;
	CButton		 m_CheckLog;
	CButton		 m_CheckAlertSpeaker;
	CButton		 m_CheckAutoStart;
	CButton		 m_CheckAlertDialog;
	CBrush		 m_hbr;


// Dialog Data
	//{{AFX_DATA(CSystemSet)
	enum { IDD = IDD_SYSTEM_SET };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSystemSet)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSystemSet)
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnSystemSetCheckLog();
	afx_msg void OnSystemSetCheckAlertDialog();
	afx_msg void OnSystemSetCheckAlertPcspeaker();
	afx_msg void OnSystemSetCheckAutostart();
	afx_msg void OnSystemSetCheckSplash();
	afx_msg void OnSelchangeSystemSetListLogSize();
	afx_msg void OnApply();
	virtual void OnCancel();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSTEMSET_H__1982AD04_521D_48EE_ACC5_FB6000BF571E__INCLUDED_)
