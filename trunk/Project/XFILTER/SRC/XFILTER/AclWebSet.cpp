// AclWebSet.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "AclWebSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAclWebSet dialog


CAclWebSet::CAclWebSet(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CAclWebSet::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAclWebSet)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bIsChange = FALSE;
	memset(&m_Acl, 0, sizeof(XACL_WEB));
}


void CAclWebSet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAclWebSet)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAclWebSet, CPasseckDialog)
	//{{AFX_MSG_MAP(CAclWebSet)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAclWebSet message handlers

BOOL CAclWebSet::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	int i = 0;
	for(i; i < ACL_WEB_SET_LABEL_COUNT; i++)
		m_Label[i].SetLabelEx(ACL_WEB_SET_LABEL[i], this);
	for(i = 0; i < ACL_WEB_SET_EDIT_COUNT; i++)
		m_Edit[i].SubclassDlgItem(ACL_WEB_SET_EDIT[i], this);
	for(i = 0; i < ACL_WEB_SET_LIST_COUNT; i++)
		m_List[i].SubclassDlgItem(ACL_WEB_SET_LIST[i], this);
	for(i = 0; i < ACL_WEB_SET_BUTTON_COUNT; i++)
		m_Button[i].SetButton(ACL_WEB_SET_BUTTON[i], this);

	InitDialog();
	ShowAcl();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAclWebSet::InitDialog()
{
	AddListStrings(&m_List[ACL_WEB_SET_LIST_ACTION], GUI_ACTION, GUI_ACTION_COUNT);

	m_Edit[ACL_WEB_SET_EDIT_WEB].SetLimitText(63);
	m_Edit[ACL_WEB_SET_EDIT_MEMO].SetLimitText(50);
}

void CAclWebSet::ShowAcl()
{
	m_List[ACL_WEB_SET_LIST_ACTION].SetCurSel(m_Acl.bAction);

	m_Edit[ACL_WEB_SET_EDIT_WEB].SetWindowText(m_Acl.sWeb);
	m_Edit[ACL_WEB_SET_EDIT_MEMO].SetWindowText(m_Acl.sMemo);
}

void CAclWebSet::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	RECT rect;
	GetClientRect(&rect);
	CBrush brush(COLOR_TORJAN_BK);
	dc.FillRect(&rect, &brush);
}

void CAclWebSet::OnOK() 
{
	CString tmpStrMemo, tmpStrWeb;

	m_Edit[ACL_WEB_SET_EDIT_MEMO].GetWindowText(tmpStrMemo);
	m_Edit[ACL_WEB_SET_EDIT_WEB].GetWindowText(tmpStrWeb);

	if(tmpStrWeb == "")
	{
		AfxMessageBox(GUI_ACL_MESSAGE_APP_PATH_ERROR);
		m_Edit[ACL_WEB_SET_EDIT_WEB].SetFocus();
		return;
	}

	if(tmpStrMemo.Compare(m_Acl.sMemo) != 0 
		|| tmpStrWeb.Compare(m_Acl.sWeb) != 0
		|| m_Acl.bAction != m_List[ACL_WEB_SET_LIST_ACTION].GetCurSel()
		)
	{
		m_Acl.bAction = m_List[ACL_WEB_SET_LIST_ACTION].GetCurSel();

		_tcscpy(m_Acl.sMemo, tmpStrMemo);
		_tcscpy(m_Acl.sWeb, tmpStrWeb);

		m_bIsChange = TRUE;
	}

	CDialog *dlg = (CDialog*)GetParent()->GetParent();
	dlg->EndDialog(IDOK);
}

void CAclWebSet::OnCancel() 
{
	CDialog *dlg = (CDialog*)GetParent()->GetParent();
	dlg->EndDialog(IDCANCEL);
}

#pragma comment( exestr, "B9D3B8FD2A63656E7967647567762B")
