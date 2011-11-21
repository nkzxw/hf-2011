#if !defined(AFX_MONITORSUB_H__04CF6CCA_FEC3_49A3_9DE1_457A8663CA8C__INCLUDED_)
#define AFX_MONITORSUB_H__04CF6CCA_FEC3_49A3_9DE1_457A8663CA8C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MonitorSub.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMonitorSub dialog

typedef struct _ONLINE_DATA
{
	DWORD	dwId;
	DWORD	dwRecv;
	DWORD	dwSend;
} ONLINE_DATA, *PONLINE_DATA;

static UINT MONITOR_LIST[] = {
	IDC_LIST1,
	IDC_LIST2,
	IDC_LIST3,
	IDC_LIST4,
	IDC_LIST5
};
#define MONITOR_LIST_COUNT	sizeof(MONITOR_LIST)/sizeof(UINT)

class CMonitorSub : public CPasseckDialog
{
// Construction
public:
	CMonitorSub(CWnd* pParent = NULL);   // standard constructor

private:
	void SetSelectButton(BYTE bSelectButton);

	void AddListCenter(BYTE bType, CListCtrl* pList, PPACKET_LOG pLog);
	void AddApp(CListCtrl* pList, PPACKET_LOG pLog);
	void AddNnb(CListCtrl* pList, PPACKET_LOG pLog);
	void AddIcmp(CListCtrl* pList, PPACKET_LOG pLog);
	void AddListen(PSESSION session);
	void AddOnline(PSESSION session);
	void AddMonitorList(PSESSION session, CListCtrl *pList, LPCTSTR* pString, int nCount);
	void DeleteMonitorList(PSESSION session, CListCtrl *pList);
	void SetButtonText();

private:
	BYTE		m_bSelectedButton;
	CButtonST	m_Button[5];
	CBkStatic	m_LableTitle;
	CListCtrl	m_List[MONITOR_LIST_COUNT];
	CButtonST	m_ButtonEx[3];
	BOOL		m_bIsScroll[3];
	BOOL		m_bIsMonitor[3];
	CColorStatic m_LabelStatus;

	int FindQueryList(DWORD dwId);
	BOOL DeleteQueryList(DWORD dwId);
	CArray<ONLINE_DATA, ONLINE_DATA> m_arOnlineData;


// Dialog Data
	//{{AFX_DATA(CMonitorSub)
	enum { IDD = IDD_MONITOR_SUB };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMonitorSub)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMonitorSub)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnMonitorApp();
	afx_msg void OnMonitorIcmp();
	afx_msg void OnMonitorLine();
	afx_msg void OnMonitorNnb();
	afx_msg void OnMonitorPort();
	afx_msg void OnButtonClear();
	afx_msg void OnButtonMonitor();
	afx_msg void OnButtonScroll();
	//}}AFX_MSG
	afx_msg LRESULT OnAddList(UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MONITORSUB_H__04CF6CCA_FEC3_49A3_9DE1_457A8663CA8C__INCLUDED_)
