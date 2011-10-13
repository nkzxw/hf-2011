//---------------------------------------------------------------------------
//
// MainFrm.cpp
//
// SUBSYSTEM:   Hook system
//				
// MODULE:      Hook server
//				
// DESCRIPTION: Implementation of the CMainFrame class
//             
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com)
// DATE:		2001 December v1.00
//
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "HookSrv.h"
#include "MainFrm.h"
#include "..\Common\CustomMessages.h"

//---------------------------------------------------------------------------
//
// Constants
// 
//---------------------------------------------------------------------------

// Message ID used for tray notifications
#define WM_MY_TRAY_NOTIFICATION WM_USER + 0x500

//---------------------------------------------------------------------------
//
// class CMainFrame
// 
//---------------------------------------------------------------------------

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_COMMAND(ID_APP_EXIT, OnAppExit)
	ON_MESSAGE(WM_MY_TRAY_NOTIFICATION, OnTrayNotification)
	//}}AFX_MSG_MAP
	ON_REGISTERED_MESSAGE(UWM_HOOKTOOL_DLL_LOADED, OnDllLoaded)
	ON_REGISTERED_MESSAGE(UWM_HOOKTOOL_DLL_UNLOADED, OnDllUnLoaded)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
// CMainFrame construction
//
//---------------------------------------------------------------------------
CMainFrame::CMainFrame():
	m_TrayIcon(IDR_TRAYICON),
	m_bShutdown(FALSE),
	m_ApplicationScope( CApplicationScope::GetInstance() )
{
	
}

//---------------------------------------------------------------------------
//
// CMainFrame destruction
//
//---------------------------------------------------------------------------
CMainFrame::~CMainFrame()
{
	m_ApplicationScope.InstallHook( FALSE, 0 );
}

//---------------------------------------------------------------------------
//
// OnCreate
//
//---------------------------------------------------------------------------
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Set up tray icon
	m_TrayIcon.SetNotificationWnd(this, WM_MY_TRAY_NOTIFICATION);
	m_TrayIcon.SetIcon(IDI_TRAYICON);

	//
	// Install the hook here
	//
	m_ApplicationScope.InstallHook( TRUE, m_hWnd );

	return 0;
}

//---------------------------------------------------------------------------
//
// OnClose
//
//---------------------------------------------------------------------------
void CMainFrame::OnClose() 
{
	if (m_bShutdown)
	{
		CFrameWnd::OnClose();
	}
	else
	{
		ShowWindow(SW_HIDE);
	}
}

//---------------------------------------------------------------------------
// OnAppExit
//
// Shut down the process
//---------------------------------------------------------------------------
void CMainFrame::OnAppExit() 
{
	m_bShutdown = TRUE;		
	SendMessage(WM_CLOSE);	
}

//---------------------------------------------------------------------------
// OnTrayNotification
//
// Handle notification from tray icon: display a message.
//---------------------------------------------------------------------------
LRESULT CMainFrame::OnTrayNotification(WPARAM uID, LPARAM lEvent)
{
	// let tray icon do default stuff
	return m_TrayIcon.OnTrayNotification(uID, lEvent);
}

//---------------------------------------------------------------------------
// OnDllLoaded
//
// Fired when a process loads hook tool dll
//---------------------------------------------------------------------------
LRESULT CMainFrame::OnDllLoaded(WPARAM wParam, LPARAM lParam)
{
	m_ApplicationScope.OnDllLoaded( lParam );
	return 0;
}

//---------------------------------------------------------------------------
// OnDllUnLoaded
//
// Fired when a process unloads hook tool dll
//---------------------------------------------------------------------------
LRESULT CMainFrame::OnDllUnLoaded(WPARAM wParam, LPARAM lParam)
{
	m_ApplicationScope.OnDllUnLoaded( lParam );
	return 0;
}
//----------------------------End of the file -------------------------------

