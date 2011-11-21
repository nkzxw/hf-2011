// AclSub.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "AclSub.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAclSub dialog


CAclSub::CAclSub(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CAclSub::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAclSub)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bSelectedButton = 255;
}


void CAclSub::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAclSub)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAclSub, CDialog)
	//{{AFX_MSG_MAP(CAclSub)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_ACL_APP, OnAclApp)
	ON_BN_CLICKED(IDC_ACL_WEB, OnAclWeb)
	ON_BN_CLICKED(IDC_ACL_NNB, OnAclNnb)
	ON_BN_CLICKED(IDC_ACL_ICMP, OnAclIcmp)
	ON_BN_CLICKED(IDC_ACL_TORJAN, OnAclTorjan)
	ON_BN_CLICKED(IDC_ACL_NET, OnAclNet)
	ON_BN_CLICKED(IDC_ACL_TIME, OnAclTime)
	ON_BN_CLICKED(IDC_ACL_BUTTON_ADD, OnAclButtonAdd)
	ON_BN_CLICKED(IDC_ACL_BUTTON_EDIT, OnAclButtonEdit)
	ON_BN_CLICKED(IDC_ACL_BUTTON_DELETE, OnAclButtonDelete)
	ON_BN_CLICKED(IDC_ACL_BUTTON_APPLY, OnAclButtonApply)
	ON_BN_CLICKED(IDC_ACL_BUTTON_CANCEL, OnAclButtonCancel)
	//}}AFX_MSG_MAP
	ON_MESSAGE(ACL_WM_SUB_NOTIFY, OnSubNotify)
	ON_MESSAGE(ACL_WM_QUERY, OnAclQuery)
END_MESSAGE_MAP()

LRESULT CAclSub::OnSubNotify(UINT wParam, LONG lParam)
{
	BYTE bFlags = (BYTE)lParam;
	EnableButton(bFlags);

	return 0;
}

LRESULT CAclSub::OnAclQuery(UINT wParam, LONG lParam)
{
	CAclQuery m_AclQuery(this);
	if(wParam == MON_ADD_SPI_ONLINE)
	{
		m_pSession[lParam].bStatus -= SESSION_STATUS_QUERY_MARGIN;
		m_AclQuery.SetSession(&m_pSession[lParam]);
		m_AclQuery.DoModal();
		m_pSession[lParam].bAction = m_AclQuery.GetAction();
		m_pSession[lParam].bStatus = SESSION_STATUS_FREE;
	}
	else if(wParam == MON_ADD_PACKET)
	{
		DWORD dwId = m_BufferPoint.pPacket[lParam].Id;
		int iIndex = FindQueryList(dwId);
		if(iIndex != -1) return 0;
		m_arQueryList.Add(dwId);
		m_AclQuery.SetPacket(&m_BufferPoint.pPacket[lParam]);
		m_AclQuery.DoModal();
		DeleteQueryList(dwId);
	}

	return 0;
}

int CAclSub::FindQueryList(DWORD dwId)
{
	int nCount = m_arQueryList.GetSize();
	for(int i = 0; i < nCount; i++)
	{
		if(dwId == m_arQueryList[i])
			return i;
	}
	return -1;
}

BOOL CAclSub::DeleteQueryList(DWORD dwId)
{
	int iIndex = FindQueryList(dwId);
	if(iIndex == -1)
		return FALSE;
	m_arQueryList.RemoveAt(iIndex);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAclSub message handlers

void CAclSub::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	RECT rect;
	GetClientRect(&rect);
	CBrush brush(PASSECK_DIALOG_BKCOLOR);
	dc.FillRect(&rect, &brush);
	
	// Do not call CDialog::OnPaint() for painting messages
}

BOOL CAclSub::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CreateCaptionEx(ACL_CAPTION[ACL_BUTTON_APP]);
	m_LableTitle.SubclassDlgItem(IDC_LABLE_TITLE, this);
	m_LableTitle.SetBkColor(COLOR_LABLE_BK);

	m_Button[ACL_BUTTON_APP].SetButton(IDC_ACL_APP, this);
	m_Button[ACL_BUTTON_WEB].SetButton(IDC_ACL_WEB, this);
	m_Button[ACL_BUTTON_NNB].SetButton(IDC_ACL_NNB, this);
	m_Button[ACL_BUTTON_ICMP].SetButton(IDC_ACL_ICMP, this);
	m_Button[ACL_BUTTON_TORJAN].SetButton(IDC_ACL_TORJAN, this);
	m_Button[ACL_BUTTON_TIME].SetButton(IDC_ACL_TIME, this);
	m_Button[ACL_BUTTON_NET].SetButton(IDC_ACL_NET, this);

	m_ButtonEx[ACL_BUTTON_ADD].SetButton(IDC_ACL_BUTTON_ADD, this);
	m_ButtonEx[ACL_BUTTON_EDIT].SetButton(IDC_ACL_BUTTON_EDIT, this);
	m_ButtonEx[ACL_BUTTON_DEL].SetButton(IDC_ACL_BUTTON_DELETE, this);
	m_ButtonEx[ACL_BUTTON_APPLY].SetButton(IDC_ACL_BUTTON_APPLY, this);
	m_ButtonEx[ACL_BUTTON_CANCEL].SetButton(IDC_ACL_BUTTON_CANCEL, this);
	
	m_AclApp.Create(IDD_ACL_APP, GetDlgItem(IDC_ACL_SUB_PARENT));
	m_AclWeb.Create(IDD_ACL_WEB, GetDlgItem(IDC_ACL_SUB_PARENT));
	m_AclNnb.Create(IDD_ACL_NNB, GetDlgItem(IDC_ACL_SUB_PARENT));
	m_AclIcmp.Create(IDD_ACL_ICMP, GetDlgItem(IDC_ACL_SUB_PARENT));
	m_AclTorjan.Create(IDD_ACL_TORJAN, GetDlgItem(IDC_ACL_SUB_PARENT));
	m_AclTime.Create(IDD_ACL_TIME, GetDlgItem(IDC_ACL_SUB_PARENT));
	m_AclNet.Create(IDD_ACL_NET, GetDlgItem(IDC_ACL_SUB_PARENT));

	OnAclApp();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAclSub::SetSelectButton(BYTE bSelectButton)
{
	if(m_bSelectedButton == bSelectButton) return;
	if(m_bSelectedButton != 255)
	{
		m_Button[m_bSelectedButton].SetSelect(FALSE);
	}
	m_Button[bSelectButton].SetSelect(TRUE);
	SetWindowCaption(ACL_CAPTION[bSelectButton]);
	m_bSelectedButton = bSelectButton;

	m_AclApp.ShowWindow(bSelectButton == ACL_BUTTON_APP ? SW_SHOW : SW_HIDE);
	m_AclWeb.ShowWindow(bSelectButton == ACL_BUTTON_WEB ? SW_SHOW : SW_HIDE);
	m_AclNnb.ShowWindow(bSelectButton == ACL_BUTTON_NNB ? SW_SHOW : SW_HIDE);
	m_AclIcmp.ShowWindow(bSelectButton == ACL_BUTTON_ICMP ? SW_SHOW : SW_HIDE);
	m_AclTorjan.ShowWindow(bSelectButton == ACL_BUTTON_TORJAN ? SW_SHOW : SW_HIDE);
	m_AclTime.ShowWindow(bSelectButton == ACL_BUTTON_TIME ? SW_SHOW : SW_HIDE);
	m_AclNet.ShowWindow(bSelectButton == ACL_BUTTON_NET ? SW_SHOW : SW_HIDE);

	ShowButtonCase(bSelectButton);

	EnableButtonCase(bSelectButton);
}

void CAclSub::ShowButtonCase(BYTE bSelectButton)
{
	switch(bSelectButton)
	{
	case ACL_BUTTON_APP:
	case ACL_BUTTON_WEB:
	case ACL_BUTTON_NNB:
	case ACL_BUTTON_ICMP:
	case ACL_BUTTON_NET:
		ShowButton(ACL_BUTTON_SHOW_ALL);
		break;
	case ACL_BUTTON_TIME:
		ShowButton(ACL_BUTTON_SHOW_APPLY_GROUP);
		break;
	case ACL_BUTTON_TORJAN:
		ShowButton(ACL_BUTTON_HIDE_ALL);
		break;
	default:
		break;
	}
}

void CAclSub::ShowButton(BYTE nFlags)
{
	for(int i = 0; i < BUTTON_EX_COUNT; i++)
	{
		m_ButtonEx[i].ShowWindow((nFlags & BUTTON_MASK[i]) == BUTTON_MASK[i]);
	}
}

void CAclSub::EnableButtonCase(BYTE bSelectButton)
{
	BYTE bFlags;
	switch(bSelectButton)
	{
	case ACL_BUTTON_APP:
		bFlags = m_AclApp.GetButtonFlags();
		break;
	case ACL_BUTTON_WEB:
		bFlags = m_AclWeb.GetButtonFlags();
		break;
	case ACL_BUTTON_NNB:
		bFlags = m_AclNnb.GetButtonFlags();
		break;
	case ACL_BUTTON_ICMP:
		bFlags = m_AclIcmp.GetButtonFlags();
		break;
	case ACL_BUTTON_NET:
		bFlags = m_AclNet.GetButtonFlags();
		break;
	case ACL_BUTTON_TIME:
		bFlags = m_AclTime.GetButtonFlags();
		break;
	case ACL_BUTTON_TORJAN:
		bFlags = m_AclTorjan.GetButtonFlags();
		break;
	default:
		break;
	}
	EnableButton(bFlags);
}

void CAclSub::EnableButton(BYTE nFlags)
{
	for(int i = 0; i < BUTTON_EX_COUNT; i++)
	{
		m_ButtonEx[i].EnableWindow((nFlags & BUTTON_MASK[i]) == BUTTON_MASK[i]);
	}
}

void CAclSub::OnAclApp() 
{
	SetSelectButton(ACL_BUTTON_APP);
}

void CAclSub::OnAclWeb() 
{
	SetSelectButton(ACL_BUTTON_WEB);
}

void CAclSub::OnAclNnb() 
{
	SetSelectButton(ACL_BUTTON_NNB);
}

void CAclSub::OnAclIcmp() 
{
	SetSelectButton(ACL_BUTTON_ICMP);
}

void CAclSub::OnAclTorjan() 
{
	SetSelectButton(ACL_BUTTON_TORJAN);
}

void CAclSub::OnAclNet() 
{
	SetSelectButton(ACL_BUTTON_NET);
}

void CAclSub::OnAclTime() 
{
	SetSelectButton(ACL_BUTTON_TIME);
}

void CAclSub::OnAclButtonAdd() 
{
	switch(m_bSelectedButton)
	{
	case ACL_BUTTON_APP:
		m_AclApp.Add();
		break;
	case ACL_BUTTON_WEB:
		m_AclWeb.Add();
		break;
	case ACL_BUTTON_NNB:
		m_AclNnb.Add();
		break;
	case ACL_BUTTON_ICMP:
		m_AclIcmp.Add();
		break;
	case ACL_BUTTON_NET:
		m_AclNet.Add();
		break;
	case ACL_BUTTON_TIME:
	case ACL_BUTTON_TORJAN:
	default:
		break;
	}
}

void CAclSub::OnAclButtonEdit() 
{
	switch(m_bSelectedButton)
	{
	case ACL_BUTTON_APP:
		m_AclApp.Edit();
		break;
	case ACL_BUTTON_WEB:
		m_AclWeb.Edit();
		break;
	case ACL_BUTTON_NNB:
		m_AclNnb.Edit();
		break;
	case ACL_BUTTON_ICMP:
		m_AclIcmp.Edit();
		break;
	case ACL_BUTTON_NET:
		m_AclNet.Edit();
		break;
	case ACL_BUTTON_TIME:
	case ACL_BUTTON_TORJAN:
	default:
		break;
	}
}

void CAclSub::OnAclButtonDelete() 
{
	switch(m_bSelectedButton)
	{
	case ACL_BUTTON_APP:
		m_AclApp.Delete();
		break;
	case ACL_BUTTON_WEB:
		m_AclWeb.Delete();
		break;
	case ACL_BUTTON_NNB:
		m_AclNnb.Delete();
		break;
	case ACL_BUTTON_ICMP:
		m_AclIcmp.Delete();
		break;
	case ACL_BUTTON_NET:
		m_AclNet.Delete();
		break;
	case ACL_BUTTON_TIME:
	case ACL_BUTTON_TORJAN:
	default:
		break;
	}
}

void CAclSub::OnAclButtonApply() 
{
	switch(m_bSelectedButton)
	{
	case ACL_BUTTON_APP:
		m_AclApp.Apply();
		break;
	case ACL_BUTTON_WEB:
		m_AclWeb.Apply();
		break;
	case ACL_BUTTON_NNB:
		m_AclNnb.Apply();
		break;
	case ACL_BUTTON_ICMP:
		m_AclIcmp.Apply();
		break;
	case ACL_BUTTON_NET:
		m_AclNet.Apply();
		break;
	case ACL_BUTTON_TIME:
		m_AclTime.Apply();
		break;
	case ACL_BUTTON_TORJAN:
	default:
		break;
	}
}

void CAclSub::OnAclButtonCancel() 
{
	switch(m_bSelectedButton)
	{
	case ACL_BUTTON_APP:
		m_AclApp.Cancel();
		break;
	case ACL_BUTTON_WEB:
		m_AclWeb.Cancel();
		break;
	case ACL_BUTTON_NNB:
		m_AclNnb.Cancel();
		break;
	case ACL_BUTTON_ICMP:
		m_AclIcmp.Cancel();
		break;
	case ACL_BUTTON_NET:
		m_AclNet.Cancel();
		break;
	case ACL_BUTTON_TIME:
		m_AclTime.Cancel();
		break;
	case ACL_BUTTON_TORJAN:
	default:
		break;
	}
}

BOOL CAclSub::IsChange()
{
	if((m_AclApp.GetButtonFlags() & ACL_BUTTON_APPLY_MASK) != 0
		|| (m_AclWeb.GetButtonFlags() & ACL_BUTTON_APPLY_MASK) != 0
		|| (m_AclNnb.GetButtonFlags() & ACL_BUTTON_APPLY_MASK) != 0
		|| (m_AclIcmp.GetButtonFlags() & ACL_BUTTON_APPLY_MASK) != 0
		|| (m_AclNet.GetButtonFlags() & ACL_BUTTON_APPLY_MASK) != 0
		|| (m_AclTime.GetButtonFlags() & ACL_BUTTON_APPLY_MASK) != 0
		)
		return TRUE;
	return FALSE;
}

void CAclSub::Apply()
{
	m_AclApp.Apply();
	m_AclWeb.Apply();
	m_AclNnb.Apply();
	m_AclIcmp.Apply();
	m_AclNet.Apply();
	m_AclTime.Apply();
}

void CAclSub::Cancel()
{
	m_AclApp.Cancel();
	m_AclWeb.Cancel();
	m_AclNnb.Cancel();
	m_AclIcmp.Cancel();
	m_AclNet.Cancel();
	m_AclTime.Cancel();
}
#pragma comment( exestr, "B9D3B8FD2A63656E7577642B")
