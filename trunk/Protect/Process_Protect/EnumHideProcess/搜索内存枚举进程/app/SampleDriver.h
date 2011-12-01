// SampleDriver.h : main header file for the SAMPLEDRIVER application
//

#if !defined(AFX_SAMPLEDRIVER_H__D5004254_9C3F_4A74_9A01_19923EFF9AD0__INCLUDED_)
#define AFX_SAMPLEDRIVER_H__D5004254_9C3F_4A74_9A01_19923EFF9AD0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CSampleDriverApp:
// See SampleDriver.cpp for the implementation of this class
//

class CSampleDriverApp : public CWinApp
{
public:
	CSampleDriverApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSampleDriverApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CSampleDriverApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAMPLEDRIVER_H__D5004254_9C3F_4A74_9A01_19923EFF9AD0__INCLUDED_)
