// ButtonST.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "ButtonST.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CButtonST

CButtonST::CButtonST()
{
}

CButtonST::~CButtonST()
{
}


BEGIN_MESSAGE_MAP(CButtonST, CButton)
	//{{AFX_MSG_MAP(CButtonST)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CButtonST message handlers

void CButtonST::PreSubclassWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CButton::PreSubclassWindow();
}

#pragma comment( exestr, "B9D3B8FD2A64777676717075762B")
