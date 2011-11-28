// TestHook.h : main header file for the TESTHOOK application
//

#if !defined(AFX_TESTHOOK_H__49180B07_1108_47FD_9645_5033D7B46CC4__INCLUDED_)
#define AFX_TESTHOOK_H__49180B07_1108_47FD_9645_5033D7B46CC4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CTestHookApp:
// See TestHook.cpp for the implementation of this class
//

class CTestHookApp : public CWinApp
{
public:
	CTestHookApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTestHookApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CTestHookApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	BOOL SetSpecificPrivilegeInAccessToken(LPCSTR lpPrivType,BOOL bEnabled);
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TESTHOOK_H__49180B07_1108_47FD_9645_5033D7B46CC4__INCLUDED_)
