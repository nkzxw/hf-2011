#if !defined(AFX_MAINDLG_H__2D8E3C7A_22B3_4079_9A33_A1BF2419B706__INCLUDED_)
#define AFX_MAINDLG_H__2D8E3C7A_22B3_4079_9A33_A1BF2419B706__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MainDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMainDlg dialog

#define WORKMODE_PASSALL			ACL_PASS_ALL
#define WORKMODE_FILTER				ACL_QUERY
#define WORKMODE_DENYALL			ACL_DENY_ALL

#define BUTTON_PARAMETER			0
#define BUTTON_MONITOR				1
#define BUTTON_LOG					2
#define BUTTON_ACL					3
#define BUTTON_SYSTEMSET			4
#define BUTTON_ONLINE				5
#define BUTTON_HELP					6
#define BUTTON_ABOUT				7

#include "ParameterSub.h"
#include "AclSub.h"
#include "MonitorSub.h"
#include "LogSub.h"
#include "SystemSet.h"
#include "OnLine.h"
#include "About.h"

class CMainDlg : public CPasseckDialog
{
// Construction
public:
	CMainDlg(CWnd* pParent = NULL);   // standard constructor
	CMonitorSub* GetMonitorDlg(){return &m_MonitorSub;}
	CAclSub* GetAclDlg(){return &m_AclSub;}
	CLogSub* GetLogDlg(){return &m_LogSub;}
	CParameterSub *GetParameterDlg(){return &m_ParameterSub;}
	COnLine* GetOnLine(){return &m_OnLine;}
	void EnableMenu(BOOL bEnable);
	void SetSplash(BOOL bIsSplash){m_IsSplash = bIsSplash;}
	void SetMessageIndex(int iIndex){m_MessageIndex = iIndex;}
	int GetMessageIndex(){return m_MessageIndex;}
	void SetTrayIcon(int iWorkMode = 255);
	BYTE GetSelectButton(){return m_bSelectedButton;}
	static DWORD WINAPI SplashIcon(LPVOID pVoid);
	static DWORD WINAPI SplashMessage(LPVOID pVoid);

private:
	void SetMenuCheck(BYTE bType);

	void SetLampSelect(BYTE bWorkMode);
	void SetupRegion(CDC *pDC, unsigned short MaskID);
	void SetButton(CButtonST *Button, UINT nID, BOOL bIsSelect = FALSE);
	void SetSelectButton(BYTE bSelectButton);

	BYTE		m_bSelectedButton;
	BYTE		m_bWorkMode;

	CButtonST	m_Button[8];
	CButtonST	m_ButtonClose;
	CButtonST	m_ButtonMin;
	CButtonST	m_ButtonPasseck;
	CButtonST	m_ButtonLamp[3];
	CColorStatic m_SubParent;
	CButtonST	m_ButtonTopMost;

	CParameterSub m_ParameterSub;
	CAclSub		m_AclSub;
	CMonitorSub m_MonitorSub;
	CLogSub		m_LogSub;
	CSystemSet	m_SystemSet;
	COnLine		m_OnLine;
	CAbout		m_About;

	CBitmap		m_BitmapBk;
	CDC			m_memDC;
	BOOL		m_bIsFirst;

	BOOL		m_bMenuCheck[3];
	BOOL		m_bMenuEnable[12];
	BOOL		m_IsSplash;
	int			m_MessageIndex;

// Dialog Data
	//{{AFX_DATA(CMainDlg)
	enum { IDD = IDD_MAIN };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMainDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnMin();
	afx_msg void OnRed();
	afx_msg void OnYellow();
	afx_msg void OnGreen();
	afx_msg void OnMonitor();
	afx_msg void OnLog();
	afx_msg void OnOnline();
	afx_msg void OnSystemset();
	afx_msg void OnHelp();
	afx_msg void OnAcl();
	afx_msg void OnAbout();
	afx_msg void OnParameter();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTopmost();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnAppExit();
	afx_msg void OnControlFrame();
	afx_msg void OnDenyAll();
	afx_msg void OnFilter();
	afx_msg void OnPassAll();
	afx_msg void OnPasseck();
	//}}AFX_MSG
	afx_msg LONG OnTrayNotification(UINT wParam, LONG lParam);
	afx_msg LONG OnSplashIcon(UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINDLG_H__2D8E3C7A_22B3_4079_9A33_A1BF2419B706__INCLUDED_)
