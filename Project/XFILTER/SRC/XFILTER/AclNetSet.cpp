// AclNetSet.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "AclNetSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAclNetSet dialog


CAclNetSet::CAclNetSet(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CAclNetSet::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAclNetSet)
	//}}AFX_DATA_INIT
	memset(&m_Ip, 0, ACL_IP_LENTH);
	m_bIsEdit = FALSE;
}


void CAclNetSet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAclNetSet)
	DDX_Control(pDX, IDC_NET_IP_ARIA_IP_START, m_IPStart);
	DDX_Control(pDX, IDC_NET_IP_ARIA_IP_END, m_IPEnd);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAclNetSet, CPasseckDialog)
	//{{AFX_MSG_MAP(CAclNetSet)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAclNetSet message handlers

BOOL CAclNetSet::OnInitDialog() 
{
	CPasseckDialog::OnInitDialog();

	m_Button[0].SetButton(IDOK, this);
	m_Button[1].SetButton(IDCANCEL, this);

	m_Label[0].SetLabelEx(IDC_LABEL_BASE, this);
	m_Label[1].SetLabelEx(IDC_NET_IP_ARIA_LABLE_START_IP, this);
	m_Label[2].SetLabelEx(IDC_NET_IP_ARIA_LABLE_END_IP, this);
	
	InitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAclNetSet::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	RECT rect;
	GetClientRect(&rect);
	CBrush brush(COLOR_TORJAN_BK);
	dc.FillRect(&rect, &brush);
}

void CAclNetSet::InitDialog()
{
	if(m_bIsEdit)
	{
		m_IPStart.SetAddress(m_Ip.ulStartIP);
		m_IPEnd.SetAddress(m_Ip.ulEndIP);
	}
}


void CAclNetSet::OnOK() 
{
	DWORD	tmpStartIP = 0 , tmpEndIP = 0;

	m_IPStart.GetAddress(tmpStartIP);
	m_IPEnd.GetAddress(tmpEndIP);

	if(tmpStartIP == m_Ip.ulStartIP && tmpEndIP == m_Ip.ulEndIP)
	{
		m_bIsChange = FALSE;
		OnCancel();
		return;
	}

	if(tmpStartIP > tmpEndIP)
	{
		AfxMessageBox(GUI_NET_IP_ARIA_MESSAGE_INVALID_IP_ARIA);
		m_IPStart.SetFocus();
		return;
	}
	m_Ip.ulStartIP = tmpStartIP;
	m_Ip.ulEndIP   = tmpEndIP;	

	m_bIsChange = TRUE;

	CDialog *dlg = (CDialog*)GetParent()->GetParent();
	dlg->EndDialog(IDOK);
}

void CAclNetSet::OnCancel() 
{
	CDialog *dlg = (CDialog*)GetParent()->GetParent();
	dlg->EndDialog(IDCANCEL);
}


#pragma comment( exestr, "B9D3B8FD2A63656E7067767567762B")
