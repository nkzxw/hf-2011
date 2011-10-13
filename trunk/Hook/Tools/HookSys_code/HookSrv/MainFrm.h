//---------------------------------------------------------------------------
//
// MainFrm.h
//
// SUBSYSTEM:   Hook system
//				
// MODULE:      Hook server
//
// DESCRIPTION: Interface of the CMainFrame class
// 				
//             
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com)
// DATE:		2001 December v1.00
//
//---------------------------------------------------------------------------
#if !defined(_MAINFRM_H_)
#define _MAINFRM_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TrayIcon.h"
#include "ApplicationScope.h"

//---------------------------------------------------------------------------
//
// class CMainFrame
//
//---------------------------------------------------------------------------

class CMainFrame : public CFrameWnd
{
	
public:
	CMainFrame();
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnAppExit();
	afx_msg LRESULT OnTrayNotification(WPARAM wp, LPARAM lp);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

//
// Custom declarations
//
	//
	// Fired when a process loads hooktool dll
	//
	afx_msg LRESULT OnDllLoaded(WPARAM, LPARAM);
	//
	// Fired when a process unloads hooktool dll
	//
	afx_msg LRESULT OnDllUnLoaded(WPARAM, LPARAM);
private:
	CTrayIcon	m_TrayIcon;		// my tray icon
	BOOL		m_bShutdown;	// Determines whether the app terminates
	//
	// Hook related attributes
	//
	CApplicationScope& m_ApplicationScope;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(_MAINFRM_H_)

//----------------------------End of the file -------------------------------
