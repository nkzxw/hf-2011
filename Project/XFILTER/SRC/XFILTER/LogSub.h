#if !defined(AFX_LOGSUB_H__5C2230F5_69BB_4C1F_9B54_C240A6FFF1AF__INCLUDED_)
#define AFX_LOGSUB_H__5C2230F5_69BB_4C1F_9B54_C240A6FFF1AF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LogSub.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLogSub dialog

#define LOG_BUTTON_APP		0
#define LOG_BUTTON_NNB		1
#define LOG_BUTTON_ICMP		2

#define LOG_BUTTON_QUERY	0
#define LOG_BUTTON_CLS		1
#define LOG_BUTTON_BACK		2
#define LOG_BUTTON_NEXT		3

static TCHAR *LOG_CAPTION[] = {
	_T("日志查询 - 应用程序"),
	_T("日志查询 - 网上邻居"),
	_T("日志查询 - ICMP")
};

static UINT LOG_LIST[] = {
	IDC_LIST_APP,
	IDC_LIST_NNB,
	IDC_LIST_ICMP,
};
#define LOG_LIST_COUNT	sizeof(LOG_LIST)/sizeof(UINT)

#define LOG_START_DATE		0
#define LOG_START_TIME		1
#define LOG_END_DATE		2
#define LOG_END_TIME		3
static UINT LOG_TIME[] = {
	IDC_START_DATE,
	IDC_START_TIME,
	IDC_END_DATE,
	IDC_END_TIME,
};
#define LOG_TIME_COUNT	sizeof(LOG_TIME)/sizeof(UINT)

class CLogSub : public CPasseckDialog
{
// Construction
public:
	CLogSub(CWnd* pParent = NULL);   // standard constructor
	void AddLog(BYTE bType, PPACKET_LOG pLog);

private:
	void InitLogClass();
	void SetSelectButton(BYTE bSelectButton);

	CXLogFile		m_LogFile[LOG_LIST_COUNT];
	BYTE			m_bSelectedButton;
	CButtonST		m_Button[3];
	CButtonST		m_ButtonEx[4];
	CBkStatic		m_LableTitle;
	CColorStatic	m_Label[3];
	CListCtrl		m_List[LOG_LIST_COUNT];
	CDateTimeCtrl	m_Time[LOG_TIME_COUNT];

// Dialog Data
	//{{AFX_DATA(CLogSub)
	enum { IDD = IDD_LOG_SUB };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLogSub)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLogSub)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnLogApp();
	afx_msg void OnLogIcmp();
	afx_msg void OnLogNnb();
	afx_msg void OnButtonQuery();
	afx_msg void OnButtonCls();
	afx_msg void OnButtonNext();
	afx_msg void OnButtonBack();
	//}}AFX_MSG
	afx_msg LRESULT OnAddLog(UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGSUB_H__5C2230F5_69BB_4C1F_9B54_C240A6FFF1AF__INCLUDED_)
