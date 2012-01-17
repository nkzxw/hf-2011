// page2.cpp : implementation file
//

#include "stdafx.h"
#include "RegSafe.h"
#include "page2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// page2 dialog


page2::page2(CWnd* pParent /*=NULL*/)
	: CDialog(page2::IDD, pParent)
{
	//{{AFX_DATA_INIT(page2)
	m_p2edit1 = _T(getcstring(IDC_EDIT1));
	//}}AFX_DATA_INIT
}


void page2::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(page2)
	DDX_Text(pDX, IDC_EDIT1, m_p2edit1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(page2, CDialog)
	//{{AFX_MSG_MAP(page2)
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// page2 message handlers

BOOL page2::OnInitDialog() 
{
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
CString page2::getcstring(UINT csID)
{
	CString ch1;
	ch1=m_page2.CString_Getdate(csID);
	return ch1;
}

void page2::OnButton1() 
{
	UpdateData(TRUE);
	

LPTSTR lpsz = new TCHAR[m_p2edit1.GetLength()+1];
_tcscpy(lpsz, m_p2edit1);//CString 转换 char*

m_page2.CString_SetKey(IDC_EDIT1,lpsz);
AfxMessageBox("主页已修改");	
}
