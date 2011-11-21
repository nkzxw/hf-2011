#if !defined(AFX_ACLTORJAN_H__BE2A2E20_83B1_45E7_90EC_FFFB9FFD4655__INCLUDED_)
#define AFX_ACLTORJAN_H__BE2A2E20_83B1_45E7_90EC_FFFB9FFD4655__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AclTorjan.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAclTorjan dialog

class CAclTorjan : public CPasseckDialog
{
// Construction
public:
	CAclTorjan(CWnd* pParent = NULL);   // standard constructor

	BYTE GetButtonFlags(){return m_bButtonFlags;}
	void SetButtonFlags(BYTE bButtonFlags){m_bButtonFlags = bButtonFlags;}
	BYTE m_bButtonFlags;

	CColorStatic m_Label[8];
	CButtonST	m_Button[2];

	void SetTorjan(BYTE nStatus);
	void SetFile(BYTE nStatus);

// Dialog Data
	//{{AFX_DATA(CAclTorjan)
	enum { IDD = IDD_ACL_TORJAN };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAclTorjan)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAclTorjan)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnButtonTorjan();
	afx_msg void OnButtonFile();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACLTORJAN_H__BE2A2E20_83B1_45E7_90EC_FFFB9FFD4655__INCLUDED_)
