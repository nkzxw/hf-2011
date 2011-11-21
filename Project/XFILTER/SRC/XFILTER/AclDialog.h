#if !defined(AFX_ACLDIALOG_H__99977CC6_B1B4_4835_B59A_9885E7A17E33__INCLUDED_)
#define AFX_ACLDIALOG_H__99977CC6_B1B4_4835_B59A_9885E7A17E33__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AclDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAclDialog dialog

#include "AclSet.h"
#include "AclNetSet.h"
#include "AclWebSet.h"
#include "AclNnbSet.h"
#include "AclIcmpSet.h"

#define ACL_DIALOG_APP		0
#define ACL_DIALOG_WEB		1
#define ACL_DIALOG_NNB		2
#define ACL_DIALOG_ICMP		3
#define ACL_DIALOG_NET		4

static UINT ACL_DIALOG_LABEL[] = {
	IDC_LABEL_INFO,
	IDC_LABEL_INFO2,
};
#define ACL_DIALOG_LABEL_COUNT	sizeof(ACL_DIALOG_LABEL)/sizeof(UINT)

class CAclDialog : public CPasseckDialog
{
// Construction
public:
	CAclDialog(CWnd* pParent = NULL);   // standard constructor
	void SetCaption(CString sCaption){m_sCaption = sCaption;}
	void SetInfo(CString sInfo){m_sInfo = sInfo;}
	void SetType(BYTE bType){m_bType = bType;}
	void SetDialog(CString sCaption, CString sInfo, BYTE bType){SetCaption(sCaption); SetInfo(sInfo); SetType(bType);}
	CAclSet* GetAclSet(){return &m_AclSet;}
	CAclNetSet* GetAclNetSet(){return &m_NetSet;}
	CAclWebSet* GetAclWebSet(){return &m_WebSet;}
	CAclNnbSet* GetAclNnbSet(){return &m_NnbSet;}
	CAclIcmpSet* GetAclIcmpSet(){return &m_IcmpSet;}

	BYTE		m_bType;
	CButtonST	m_ButtonClose;
	CString		m_sCaption;
	CString		m_sInfo;
	CColorStatic m_Label[ACL_DIALOG_LABEL_COUNT];
	CBkStatic	m_LabelBk;

	CBitmap			m_BitmapBk;
	CDC				m_memDC;
	BOOL			m_bIsFirst;

	CAclSet			m_AclSet;
	CAclWebSet		m_WebSet;
	CAclNnbSet		m_NnbSet;
	CAclIcmpSet		m_IcmpSet;
	CAclNetSet		m_NetSet;

// Dialog Data
	//{{AFX_DATA(CAclDialog)
	enum { IDD = IDD_ACL };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAclDialog)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAclDialog)
	afx_msg void OnClose();
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACLDIALOG_H__99977CC6_B1B4_4835_B59A_9885E7A17E33__INCLUDED_)
