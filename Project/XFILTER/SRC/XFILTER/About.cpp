// About.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "About.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAbout dialog


CAbout::CAbout(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CAbout::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAbout)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_hbr.CreateSolidBrush(PASSECK_DIALOG_BKCOLOR);
}


void CAbout::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAbout)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAbout, CDialog)
	//{{AFX_MSG_MAP(CAbout)
	ON_WM_PAINT()
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAbout message handlers

void CAbout::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	
	// Do not call CDialog::OnPaint() for painting messages
}

BOOL CAbout::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_LinkEmail.SubclassDlgItem(IDC_ABOUT_LABLE_EMAIL, this);
	m_LinkUrl.SubclassDlgItem(IDC_ABOUT_LABLE_WEB_ADDRESS, this);

	PXACL_HEADER pHeader = theApp.m_AclFile.GetHeader();
	CString	s;

	s.Format(_T("mailto:%s"), pHeader->sEmail);
	m_LinkEmail.SetURL(s);
	m_LinkUrl.SetURL(pHeader->sWebURL);

	CHttpRequest* m_pInternet = theApp.m_pMainDlg.GetOnLine()->GetInternet();
	m_pInternet->IsRegistered();
	s.Format(GUI_ABOUT_LABLE_ACCREDIT_TO, 
		(m_pInternet->m_UserInfo.sName[0] == 0) ?
		m_pInternet->m_UserInfo.sEmail : m_pInternet->m_UserInfo.sName
		);

	SetDlgItemText(IDC_ABOUT_LABLE_ACCREDIT_TO	, s);
	
	s.Format(GUI_ABOUT_LABLE_COPYRIGHT1, 
		ACL_HEADER_VERSION, 
		ACL_HEADER_MAJOR,
		ACL_HEADER_MINOR,
		GUI_VERSION
		);

	SetDlgItemText(IDC_ABOUT_LABLE_COPYRIGHT1, s);
	SetDlgItemText(IDC_ABOUT_LABLE_WEB_ADDRESS, pHeader->sWebURL);
	SetDlgItemText(IDC_ABOUT_LABLE_EMAIL, pHeader->sEmail);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

HBRUSH CAbout::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	pDC->SetTextColor(COLOR_TEXT_NORMAL);
	pDC->SetBkMode(TRANSPARENT);

	return m_hbr;
}

#pragma comment( exestr, "B9D3B8FD2A63647177762B")
