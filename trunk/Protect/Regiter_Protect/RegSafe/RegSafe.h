// RegSafe.h : main header file for the REGSAFE application
//

#if !defined(AFX_REGSAFE_H__F1F8EC20_09D7_42D3_86B5_656B97863CA7__INCLUDED_)
#define AFX_REGSAFE_H__F1F8EC20_09D7_42D3_86B5_656B97863CA7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CRegSafeApp:
// See RegSafe.cpp for the implementation of this class
//

class CRegSafeApp : public CWinApp
{
public:
	CRegSafeApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRegSafeApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CRegSafeApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REGSAFE_H__F1F8EC20_09D7_42D3_86B5_656B97863CA7__INCLUDED_)
