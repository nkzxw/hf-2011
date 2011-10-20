// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_MAINFRM_H__BFC04DA9_65C6_11D7_9C14_CE97B9E6B96A__INCLUDED_)
#define AFX_MAINFRM_H__BFC04DA9_65C6_11D7_9C14_CE97B9E6B96A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "SystemTray.h"
#include "portscandlg.h"
#include "BCMenu.h"
#include "commonFn.h"
#include "PPTooltip.h"

/**
 * class CMainFrame: CMainFrame, which is derived from
    CFrameWnd and controls all SDI frame features
 *
 *
 * @author 
 */
class CMainFrame : public CFrameWnd
{
protected:	// create from serialization only
	/*********************************************
	 * default constructor of the class.
	 *
	 * @return  
	 ********************************************/
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)
// Attributes
public:
	/*********************************************
		 * object that send the firewall to system tray
		 ********************************************/
	CSystemTray m_SysTray;

	/**
	 * SetOnlineLed:Set the green led to indicate that firewall is running
	 *
	 * @param bOnline  
	 * @return void 
	 */
	void		SetOnlineLed(BOOL bOnline);

	/**
	 * SetOfflineLed:Set the green led to indicate that firewall is Not running
	 *
	 * @param bOffline 
	 * @return void 
	 */
	void		SetOfflineLed(BOOL bOffline);

// Operations
public:
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL	PreCreateWindow(CREATESTRUCT &cs);
	//}}AFX_VIRTUAL
// Implementation
public:
	/*********************************************
	 * default destructor of the class.
	 *
	 * @return virtual 
	 ********************************************/
	virtual			~CMainFrame();
#ifdef _DEBUG
	virtual void	AssertValid() const;
	virtual void	Dump(CDumpContext &dc) const;
#endif
protected:	// control bar embedded members
	/*********************************************
	 *  Object represents the Status bar of the application of the application
	 ********************************************/
	CStatusBar	m_wndStatusBar;

	/*********************************************
	 * Object represents the Toolbar of the application
	 ********************************************/
	CToolBar	m_wndToolBar;

	//**************************************************
	BOOL		m_bInitialized;
	HICON		m_hIcon;
	CImageList	m_ToolbarImages;
	CImageList	m_ToolbarImagesDisabled;
	
	BCMenu m_menu; // Our XP-style menu
	BOOL m_bMenu; // Menu created?

	//****************************************************

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void	OnClose();
	afx_msg void	OnPopupExit();
	afx_msg void	OnPopupMinimize();
	afx_msg void	OnPopupMaximize();
	afx_msg void	OnExit();
	afx_msg void	OnIcon();
	afx_msg void	OnToolsPortscanner();
	afx_msg void	OnUpdateStop(CCmdUI *pCmdUI);
	afx_msg void	OnStop();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void	OnToolsBlockapplication();
private:
	// load the high color toolbar images and attach them to m_wndToolBar
	void	AttachToolbarImages(UINT inNormalImageID, UINT inDisabledImageID);
public:
	afx_msg void OnHelpVisitnetdefenderhomepage();
	void CreateHiColorImageList(CImageList *pImageList, WORD wResourceID, int czSize);
	CImageList	m_pImgList;
	CObList m_liFirewallRules;
	//CPP Tooltips Object
	CPPToolTip				m_tooltip;

	virtual BOOL PreTranslateMessage(MSG* pMsg);
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#endif // !defined(AFX_MAINFRM_H__BFC04DA9_65C6_11D7_9C14_CE97B9E6B96A__INCLUDED_)
