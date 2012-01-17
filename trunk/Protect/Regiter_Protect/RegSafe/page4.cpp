// page4.cpp : implementation file
//

#include "stdafx.h"
#include "RegSafe.h"
#include "page4.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// page4 dialog


page4::page4(CWnd* pParent /*=NULL*/)
	: CDialog(page4::IDD, pParent)
{
	//{{AFX_DATA_INIT(page4)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void page4::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(page4)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(page4, CDialog)
	//{{AFX_MSG_MAP(page4)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// page4 message handlers
