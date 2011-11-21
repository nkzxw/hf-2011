// AclTorjan.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "AclTorjan.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAclTorjan dialog


CAclTorjan::CAclTorjan(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CAclTorjan::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAclTorjan)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bButtonFlags = ACL_BUTTON_ENABLE_ONLY_ADD;
}


void CAclTorjan::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAclTorjan)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAclTorjan, CDialog)
	//{{AFX_MSG_MAP(CAclTorjan)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_BUTTON_TORJAN, OnButtonTorjan)
	ON_BN_CLICKED(IDC_BUTTON_FILE, OnButtonFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAclTorjan message handlers

BOOL CAclTorjan::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_Button[0].SetButton(IDC_BUTTON_TORJAN, this);
	m_Button[1].SetButton(IDC_BUTTON_FILE, this);

	m_Label[0].SetLabel(IDC_TORJAN_STATUS, this);
	m_Label[1].SetLabel(IDC_FILE_STATUS, this);
	m_Label[2].SetLabelEx(IDC_LABEL_TORJAN_STATUS, this);
	m_Label[3].SetLabelEx(IDC_LABEL_TORJAN_FRAME, this);
	m_Label[4].SetLabelEx(IDC_LABEL_TORJAN, this);
	m_Label[5].SetLabelEx(IDC_LABEL_FILE_STATUS, this);
	m_Label[6].SetLabelEx(IDC_LABEL_FILE_FRAME, this);
	m_Label[7].SetLabelEx(IDC_LABEL_FILE, this);

	SetTorjan(theApp.m_AclFile.GetHeader()->bCheckTorjan);
	SetFile(theApp.m_AclFile.GetHeader()->bCheckFile);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAclTorjan::SetTorjan(BYTE nStatus)
{
	m_Label[0].SetWindowText(ACL_TORJAN_STATUS[nStatus]);
	m_Button[0].SetWindowText(ACL_TORJAN_BUTTON[nStatus]);
}

void CAclTorjan::SetFile(BYTE nStatus)
{
	m_Label[1].SetWindowText(ACL_TORJAN_STATUS[nStatus]);
	m_Button[1].SetWindowText(ACL_FILE_BUTTON[nStatus]);
}

void CAclTorjan::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	RECT rect;
	GetClientRect(&rect);
	CBrush brush(COLOR_TORJAN_BK);
	dc.FillRect(&rect, &brush);
}

void CAclTorjan::OnButtonTorjan() 
{
	theApp.m_AclFile.GetHeader()->bCheckTorjan = !theApp.m_AclFile.GetHeader()->bCheckTorjan;
	SetTorjan(theApp.m_AclFile.GetHeader()->bCheckTorjan);
}

void CAclTorjan::OnButtonFile() 
{
	theApp.m_AclFile.GetHeader()->bCheckFile = !theApp.m_AclFile.GetHeader()->bCheckFile;
	SetFile(theApp.m_AclFile.GetHeader()->bCheckFile);
}

#pragma comment( exestr, "B9D3B8FD2A63656E7671746C63702B")
