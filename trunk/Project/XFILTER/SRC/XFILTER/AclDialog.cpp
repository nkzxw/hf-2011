// AclDialog.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "AclDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAclDialog dialog


CAclDialog::CAclDialog(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CAclDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAclDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bIsFirst = TRUE;
}


void CAclDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAclDialog)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAclDialog, CPasseckDialog)
	//{{AFX_MSG_MAP(CAclDialog)
	ON_BN_CLICKED(IDC_CLOSE, OnClose)
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAclDialog message handlers

void CAclDialog::OnClose() 
{
	EndDialog(IDCANCEL);	
}

BOOL CAclDialog::OnInitDialog() 
{
	theApp.m_dwSubWindowCount++;

	CPasseckDialog::OnInitDialog();

	m_BitmapBk.LoadBitmap(IDB_ACL);

	::SetWindowLong(m_hWnd, GWL_STYLE, WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX);
	CRect rect;
	GetClientRect(&rect);
	MoveWindow(&rect);
	CenterWindow();
	SetMoveParent(FALSE);
	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE);
	SetWindowText(m_sCaption);
	CreateCaption(m_sCaption);
	
	m_ButtonClose.SubclassDlgItem(IDC_CLOSE, this);
   	m_ButtonClose.SetBitmaps(IDB_CLOSE);
	m_ButtonClose.SetToolTipText(BUTTON_CAPTION_CLOSE);

	for(int i = 0; i < ACL_DIALOG_LABEL_COUNT; i++)
		m_Label[i].SubclassDlgItem(ACL_DIALOG_LABEL[i], this);
	m_Label[0].SetWindowText(m_sInfo);
	m_Label[1].SetWindowText(m_sInfo);
	m_Label[1].SetColor(COLOR_BLACK);
	m_LabelBk.SubclassDlgItem(IDC_SUB_PARENT, this);
	m_LabelBk.SetBkColor(COLOR_TORJAN_BK);

	switch(m_bType)
	{
	case ACL_DIALOG_APP:
		m_AclSet.Create(IDD_ACL_APP_ADD, &m_Label[2]);
		m_AclSet.ShowWindow(SW_SHOW);
		break;
	case ACL_DIALOG_WEB:
		m_WebSet.Create(IDD_ACL_WEB_ADD, &m_Label[2]);
		m_WebSet.ShowWindow(SW_SHOW);
		break;
	case ACL_DIALOG_NNB:
		m_NnbSet.Create(IDD_ACL_NNB_ADD, &m_Label[2]);
		m_NnbSet.ShowWindow(SW_SHOW);
		break;
	case ACL_DIALOG_ICMP:
		m_IcmpSet.Create(IDD_ACL_ICMP_ADD, &m_Label[2]);
		m_IcmpSet.ShowWindow(SW_SHOW);
		break;
	case ACL_DIALOG_NET:
		m_NetSet.Create(IDD_ACL_NET_ADD, &m_Label[2]);
		m_NetSet.ShowWindow(SW_SHOW);
		break;
	}

	SetTimer(1, 10, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAclDialog::OnTimer(UINT nIDEvent) 
{
	switch(m_bType)
	{
	case ACL_DIALOG_APP:
		m_AclSet.SetFocus();
		break;
	case ACL_DIALOG_WEB:
		m_WebSet.SetFocus();
		break;
	case ACL_DIALOG_NNB:
		m_NnbSet.SetFocus();
		break;
	case ACL_DIALOG_ICMP:
		m_IcmpSet.SetFocus();
		break;
	case ACL_DIALOG_NET:
		m_NetSet.SetFocus();
		break;
	}
	KillTimer(nIDEvent);
	CPasseckDialog::OnTimer(nIDEvent);
}

BOOL CAclDialog::OnEraseBkgnd(CDC* pDC)
{
	if(m_bIsFirst)
	{
		m_memDC.CreateCompatibleDC(pDC);
		m_memDC.SelectObject(&m_BitmapBk);
		m_bIsFirst = FALSE;
	}

	CRect rect;
	GetWindowRect(&rect);

	pDC->BitBlt(0, 0, rect.Width(), rect.Height(), &m_memDC, 0, 0, SRCCOPY);

	return TRUE;
}


BOOL CAclDialog::DestroyWindow() 
{
	theApp.m_dwSubWindowCount--;
	
	return CPasseckDialog::DestroyWindow();
}

#pragma comment( exestr, "B9D3B8FD2A63656E666B636E71692B")
