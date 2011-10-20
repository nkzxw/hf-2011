// fire.h : main header file for the FIRE application
//
#if !defined(AFX_FIRE_H__BFC04DA5_65C6_11D7_9C14_CE97B9E6B96A__INCLUDED_)
#define AFX_FIRE_H__BFC04DA5_65C6_11D7_9C14_CE97B9E6B96A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif
#include "resource.h"	// main symbols
#include "PortInfo.h"

/////////////////////////////////////////////////////////////////////////////

// CFireApp:

// See fire.cpp for the implementation of this class

//

/**
 * class CFireApp:This is the main class that start the Application .
 *In a tipical MFC application it is called an application class.
 *
 * @author Sudhir Mangla
 */
class CFireApp : public CWinApp
{
public:
	CFireApp();
	CPortInfo m_portInfo;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFireApp)
public:
	virtual BOOL	InitInstance();
	//}}AFX_VIRTUAL
	// Implementation
	//{{AFX_MSG(CFireApp)
	afx_msg void	OnAppAbout();
	// NOTE - the ClassWizard will add and remove member functions here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#endif // !defined(AFX_FIRE_H__BFC04DA5_65C6_11D7_9C14_CE97B9E6B96A__INCLUDED_)
