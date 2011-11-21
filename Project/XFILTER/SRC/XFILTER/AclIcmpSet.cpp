// AclIcmpSet.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "AclIcmpSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAclIcmpSet dialog


CAclIcmpSet::CAclIcmpSet(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CAclIcmpSet::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAclIcmpSet)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bIsChange = FALSE;
	memset(&m_Acl, 0, sizeof(XACL_ICMP));
}


void CAclIcmpSet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAclIcmpSet)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAclIcmpSet, CPasseckDialog)
	//{{AFX_MSG_MAP(CAclIcmpSet)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAclIcmpSet message handlers

BOOL CAclIcmpSet::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	int i = 0;
	for(i; i < ACL_ICMP_SET_LABEL_COUNT; i++)
		m_Label[i].SetLabelEx(ACL_ICMP_SET_LABEL[i], this);
	for(i = 0; i < ACL_ICMP_SET_EDIT_COUNT; i++)
		m_Edit[i].SubclassDlgItem(ACL_ICMP_SET_EDIT[i], this);
	for(i = 0; i < ACL_ICMP_SET_LIST_COUNT; i++)
		m_List[i].SubclassDlgItem(ACL_ICMP_SET_LIST[i], this);
	for(i = 0; i < ACL_ICMP_SET_BUTTON_COUNT; i++)
		m_Button[i].SetButton(ACL_ICMP_SET_BUTTON[i], this);

	InitDialog();
	ShowAcl();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAclIcmpSet::InitDialog()
{
	AddListStrings(&m_List[ACL_ICMP_SET_LIST_NET], ACL_NET_TYPE, ACL_NET_TYPE_COUNT);
	AddListStrings(&m_List[ACL_ICMP_SET_LIST_TIME], ACL_TIME_TYPE, ACL_TIME_TYPE_COUNT);
	AddListStrings(&m_List[ACL_ICMP_SET_LIST_DIR], GUI_DIRECTION, GUI_DIRECTION_COUNT);
	AddListStrings(&m_List[ACL_ICMP_SET_LIST_ACTION], GUI_ACTION, GUI_ACTION_COUNT);

	m_Edit[ACL_ICMP_SET_EDIT_MEMO].SetLimitText(50);
}

void CAclIcmpSet::ShowAcl()
{
	m_List[ACL_ICMP_SET_LIST_NET].SetCurSel(m_Acl.bNetType);
	m_List[ACL_ICMP_SET_LIST_TIME].SetCurSel(m_Acl.bTimeType);
	m_List[ACL_ICMP_SET_LIST_ACTION].SetCurSel(m_Acl.bAction);
	m_List[ACL_ICMP_SET_LIST_DIR].SetCurSel(m_Acl.bDirection);

	m_Edit[ACL_ICMP_SET_EDIT_MEMO].SetWindowText(m_Acl.sMemo);
}

void CAclIcmpSet::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	RECT rect;
	GetClientRect(&rect);
	CBrush brush(COLOR_TORJAN_BK);
	dc.FillRect(&rect, &brush);
}

void CAclIcmpSet::OnOK() 
{
	CString tmpStrMemo;

	m_Edit[ACL_ICMP_SET_EDIT_MEMO].GetWindowText(tmpStrMemo);

	if(tmpStrMemo.Compare(m_Acl.sMemo) != 0 
		|| m_Acl.bTimeType != m_List[ACL_ICMP_SET_LIST_TIME].GetCurSel() 
		|| m_Acl.bAction != m_List[ACL_ICMP_SET_LIST_ACTION].GetCurSel()
		|| m_Acl.bDirection != m_List[ACL_ICMP_SET_LIST_DIR].GetCurSel()
		|| m_Acl.bNetType != m_List[ACL_ICMP_SET_LIST_NET].GetCurSel()
		)
	{
		m_Acl.bTimeType  = m_List[ACL_ICMP_SET_LIST_TIME].GetCurSel();
		m_Acl.bAction	 = m_List[ACL_ICMP_SET_LIST_ACTION].GetCurSel();
		m_Acl.bDirection = m_List[ACL_ICMP_SET_LIST_DIR].GetCurSel();
		m_Acl.bNetType	 = m_List[ACL_ICMP_SET_LIST_NET].GetCurSel();

		_tcscpy(m_Acl.sMemo, tmpStrMemo);

		m_bIsChange = TRUE;
	}

	CDialog *dlg = (CDialog*)GetParent()->GetParent();
	dlg->EndDialog(IDOK);
}

void CAclIcmpSet::OnCancel() 
{
	CDialog *dlg = (CDialog*)GetParent()->GetParent();
	dlg->EndDialog(IDCANCEL);
}

#pragma comment( exestr, "B9D3B8FD2A63656E6B656F727567762B")
