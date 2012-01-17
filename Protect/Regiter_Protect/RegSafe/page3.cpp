// page3.cpp : implementation file
//

#include "stdafx.h"
#include "RegSafe.h"
#include "page3.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// page3 dialog


page3::page3(CWnd* pParent /*=NULL*/)
	: CDialog(page3::IDD, pParent)
{
	//{{AFX_DATA_INIT(page3)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void page3::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(page3)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(page3, CDialog)
	//{{AFX_MSG_MAP(page3)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// page3 message handlers
