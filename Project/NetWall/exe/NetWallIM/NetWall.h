// NetWall Exe.h : main header file for the NETWALL EXE application
//

#if !defined(AFX_NETWALLEXE_H__1ED4BC75_5AD3_49B2_9AFF_FBF1EA8C5348__INCLUDED_)
#define AFX_NETWALLEXE_H__1ED4BC75_5AD3_49B2_9AFF_FBF1EA8C5348__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

//
// Define user defined message identifier
//
#define WM_USER_DISPLAY_RULEITEM    (WM_USER + 10)



class CNetWallAPI;

/////////////////////////////////////////////////////////////////////////////
// CNetWallApp:
// See NetWall Exe.cpp for the implementation of this class
//

class CNetWallApp : public CWinApp
{
private:
    CNetWallAPI        *m_pAPI;

public :

    CString			    m_strApiVersion;

    ULONG               m_nAppAPIVersion;
    ULONG               m_nDllAPIVersion;
    ULONG               m_nDriverAPIVersion;

    CString             m_strDriverDescription;

public:	
	CNetWallApp();

public:
    BOOLEAN GetNetWallInfo();
    CNetWallAPI* GetAPI() {return m_pAPI;};

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetWallApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CNetWallApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NETWALLEXE_H__1ED4BC75_5AD3_49B2_9AFF_FBF1EA8C5348__INCLUDED_)
