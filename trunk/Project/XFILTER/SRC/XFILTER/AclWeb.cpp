// AclWeb.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "AclWeb.h"

#include "AclDialog.h"
#include "AclWebSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAclWeb dialog


CAclWeb::CAclWeb(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CAclWeb::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAclWeb)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bSelectedButton = 255;
	m_bButtonFlags = ACL_BUTTON_ENABLE_ONLY_ADD;
	m_iListIndex = -1;
}


void CAclWeb::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAclWeb)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAclWeb, CDialog)
	//{{AFX_MSG_MAP(CAclWeb)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_ACL_RADIO_PASS, OnAclRadioPass)
	ON_BN_CLICKED(IDC_ACL_RADIO_QUERY, OnAclRadioQuery)
	ON_NOTIFY(NM_CLICK, IDC_ACL_LIST, OnClickAclList)
	ON_NOTIFY(NM_DBLCLK, IDC_ACL_LIST, OnDblclkAclList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_ACL_LIST, OnItemchangedAclList)
	ON_CBN_SELCHANGE(IDC_COMBO_SET, OnSelchangeComboSet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAclWeb message handlers

BOOL CAclWeb::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_pParent = GetParent()->GetParent();
	
	m_List.SubclassDlgItem(IDC_ACL_LIST, this);
	AddListHead(&m_List, ACL_WEB_LIST_HEADER, sizeof(ACL_WEB_LIST_HEADER)/sizeof(TCHAR*), ACL_WEB_LIST_LENTH);

	m_QueryCombo.SubclassDlgItem(IDC_COMBO_SET, this);
	AddComboStrings(&m_QueryCombo, ACL_QUERY_TEXT, ACL_QUERY_TEXT_COUNT);
	m_QueryCombo.SetCurSel(theApp.m_AclFile.GetHeader()->bWebQueryEx);

	m_Label[0].SubclassDlgItem(IDC_LABEL_PASSALL, this);
	m_Label[1].SubclassDlgItem(IDC_LABEL_QUERY, this);

	m_Button[RADIO_WEB_PASS].SetRadioButton(IDC_ACL_RADIO_PASS, IDB_RADIO_NORMAL, IDB_RADIO_SELECT, this);
	m_Button[RADIO_WEB_QUERY].SetRadioButton(IDC_ACL_RADIO_QUERY, IDB_RADIO_NORMAL, IDB_RADIO_SELECT, this);

	m_History.InitHistory(ACL_TYPE_WEB, &theApp.m_AclFile);
	InitView();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAclWeb::InitView()
{
	m_QueryCombo.SetCurSel(theApp.m_AclFile.GetHeader()->bWebQueryEx);
	OnSelchangeComboSet();
	SetSelectButton(theApp.m_AclFile.GetHeader()->bWebSet);
	InitList();
}

void CAclWeb::SetPass(BOOL IsPassAll)
{
	if(IsPassAll)
	{
		OnAclRadioPass();
	}
	else
	{
		m_QueryCombo.SetCurSel(ACL_QUERY_PASS);
		OnSelchangeComboSet();
	}
	Apply();
}

BOOL CAclWeb::SendSet(BYTE bOptType)
{
	BOOL bIsChange = (m_History.m_bSet != theApp.m_AclFile.GetHeader()->bWebSet)
		|| (m_History.m_bQueryEx != theApp.m_AclFile.GetHeader()->bWebQueryEx);

	bIsChange ? m_bButtonFlags |= ACL_BUTTON_SHOW_APPLY_GROUP
		: m_bButtonFlags &= ~ACL_BUTTON_SHOW_APPLY_GROUP;

	BOOL bRet = SendMessageEx(m_bButtonFlags); 
	m_History.AddHistory(
		bOptType
		, m_bButtonFlags
		, NULL
		, NULL
		, m_History.m_bSet
		, m_History.m_bQueryEx
		);

	return bRet;
}

BOOL CAclWeb::SendMessageEx(BYTE nFlags)
{
	m_bButtonFlags = nFlags;

	static BYTE bTempButtonFlags = m_bButtonFlags;
	if(m_bSelectedButton != RADIO_WEB_QUERY && m_List.IsWindowEnabled())
	{
		bTempButtonFlags = m_bButtonFlags & ACL_BUTTON_UPDATE_GROUP;
		m_List.EnableWindow(FALSE);
		m_bButtonFlags &= ~ACL_BUTTON_UPDATE_GROUP;
	}
	else if(m_bSelectedButton == RADIO_WEB_QUERY && !m_List.IsWindowEnabled())
	{
		m_bButtonFlags = m_bButtonFlags | bTempButtonFlags;
		m_List.EnableWindow(TRUE);
	}
	if(!m_List.IsWindowEnabled())
	{
		m_bButtonFlags &= ~ACL_BUTTON_UPDATE_GROUP;
	}

	return m_pParent->SendMessage(ACL_WM_SUB_NOTIFY, ACL_BUTTON_WEB, m_bButtonFlags);
}

void CAclWeb::InitList()
{
	m_List.DeleteAllItems();

	PXACL_WEB pAcl = theApp.m_AclFile.GetHeader()->pWeb;
	while(pAcl != NULL)
	{
		AddAcl(pAcl, FALSE);
		pAcl = pAcl->pNext;
	}
	if(m_List.GetItemCount() > 0)
	{
		m_List.SetItemState(0,	LVIS_SELECTED,LVIS_SELECTED);
	}
}

void CAclWeb::AddAcl(PXACL_WEB pAcl, BOOL bIsSelect, BOOL bIsEdit, int iIndex)
{
	CString sString[ACL_WEB_LIST_COUNT];
	sString[0].Format("%u", pAcl->dwId);
	sString[1] = pAcl->sWeb;
	sString[2] = GUI_ACTION[pAcl->bAction];
	sString[3] = pAcl->sMemo[0] == 0 ? MEMO_CONST : pAcl->sMemo;

	AddList(&m_List, (LPCTSTR*)sString, ACL_WEB_LIST_COUNT, bIsSelect, bIsEdit, iIndex);
}

void CAclWeb::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	RECT rect;
	GetClientRect(&rect);
	CBrush brush(PASSECK_DIALOG_BKCOLOR);
	dc.FillRect(&rect, &brush);
}

void CAclWeb::SetSelectButton(BYTE bSelectButton)
{
	if(m_bSelectedButton == bSelectButton) return;
	if(m_bSelectedButton != 255)
	{
		m_Button[m_bSelectedButton].SetSelect(FALSE);
	}
	m_QueryCombo.EnableWindow(bSelectButton == ACL_QUERY);
	m_Button[bSelectButton].SetSelect(TRUE);
	m_bSelectedButton = bSelectButton;

	m_History.m_bSet = bSelectButton;
	SendSet(OPT_TYPE_SET);
}

void CAclWeb::OnAclRadioPass() 
{
	SetSelectButton(RADIO_WEB_PASS);
}

void CAclWeb::OnAclRadioQuery() 
{
	SetSelectButton(RADIO_WEB_QUERY);
}

void CAclWeb::Add()
{
	if((m_bButtonFlags & ACL_BUTTON_ADD_MASK) != ACL_BUTTON_ADD_MASK) return;

	CAclDialog dlg;
	dlg.SetDialog(ACL_CAPTION_ADD, ACL_CAPTION_WEB_ADD, ACL_DIALOG_WEB);
	CAclWebSet* pAclWebSet = dlg.GetAclWebSet();
	PXACL_WEB pAcl = pAclWebSet->GetAcl();

	int nIndex = m_List.GetItemCount();
	if(nIndex == 0)
		pAcl->dwId = 1;
	else
		pAcl->dwId = atol(m_List.GetItemText(nIndex - 1, 0)) + 1;

	int iRet = dlg.DoModal();

	if(iRet == IDCANCEL) return;

	AddAcl(pAcl);
	m_bButtonFlags |= ACL_BUTTON_SHOW_APPLY_GROUP;
	m_History.AddHistory(OPT_TYPE_ADD, m_bButtonFlags, (char*)pAcl);
	SendMessageEx(m_bButtonFlags);
}

void CAclWeb::Edit()
{
	if((m_bButtonFlags & ACL_BUTTON_EDIT_MASK) != ACL_BUTTON_EDIT_MASK) return;

	if(m_iListIndex < 0) 
		return;

	CAclDialog dlg;
	dlg.SetDialog(ACL_CAPTION_EDIT, ACL_CAPTION_WEB_EDIT, ACL_DIALOG_WEB);
	CAclWebSet* pAclWebSet = dlg.GetAclWebSet();
	PXACL_WEB pAcl = pAclWebSet->GetAcl();

	pAcl->dwId = atol(m_List.GetItemText(m_iListIndex, 0));
	PXACL_WEB pAclOld = (PXACL_WEB)m_History.FindAcl(pAcl->dwId);
	if(pAclOld == NULL) pAclOld = (PXACL_WEB)theApp.m_AclFile.FindAcl(pAcl->dwId, ACL_TYPE_WEB);
	if(pAclOld == NULL) return;

	*pAcl = *pAclOld;

	int iRet = dlg.DoModal();

	if(iRet == IDCANCEL || !pAclWebSet->IsChange()) return;

	AddAcl(pAcl, TRUE, TRUE, m_iListIndex);
	m_bButtonFlags |= ACL_BUTTON_SHOW_APPLY_GROUP;
	SendMessageEx(m_bButtonFlags);
	m_History.AddHistory(OPT_TYPE_EDIT, m_bButtonFlags, (char*)pAcl, (char*)pAclOld);
}

void CAclWeb::Delete()
{
	if((m_bButtonFlags & ACL_BUTTON_DEL_MASK) != ACL_BUTTON_DEL_MASK) return;

	if(m_iListIndex < 0)
		return;

	int tmpIndex = m_iListIndex;

	XACL_WEB Acl;
	Acl.dwId = atol(m_List.GetItemText(m_iListIndex, 0));

	m_bButtonFlags |= ACL_BUTTON_SHOW_APPLY_GROUP;
	SendMessageEx(m_bButtonFlags);

	m_History.AddHistory(OPT_TYPE_DELETE, m_bButtonFlags, (char*)&Acl);

	m_List.DeleteItem(m_iListIndex);
	if(m_List.GetItemCount() <= 0)
	{
		EnableButton(FALSE);
		return;
	}

	if(tmpIndex == m_List.GetItemCount())	
		tmpIndex -- ;

	m_List.SetItemState(tmpIndex, LVIS_SELECTED, LVIS_SELECTED);
}

void CAclWeb::Apply()
{
	if((m_bButtonFlags & ACL_BUTTON_APPLY_MASK) != ACL_BUTTON_APPLY_MASK) return;

	m_History.Apply();
	SendMessageEx(m_bButtonFlags & ~ACL_BUTTON_SHOW_APPLY_GROUP);
}

void CAclWeb::Cancel()
{
	if((m_bButtonFlags & ACL_BUTTON_CANCEL_MASK) != ACL_BUTTON_CANCEL_MASK) return;

	m_History.Cancel();
	InitView();
	SendMessageEx(m_bButtonFlags & ~ACL_BUTTON_SHOW_APPLY_GROUP);
}

void CAclWeb::OnItemchangedAclList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;

	if((m_iListIndex = pNMListView->iItem) == -1) 
		return;

	EnableButton(TRUE);
}

void CAclWeb::OnClickAclList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if((m_iListIndex = pNMListView->iItem) == -1) 
		EnableButton(FALSE);
	
	*pResult = 0;
}

void CAclWeb::OnDblclkAclList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if((m_iListIndex = pNMListView->iItem) != -1) 
		Edit();

	*pResult = 0;
}

void CAclWeb::EnableButton(BOOL bEnable)
{
	if(bEnable)
		m_bButtonFlags |= ACL_BUTTON_SHOW_EDIT_GROUP;
	else
		m_bButtonFlags &= ~ACL_BUTTON_SHOW_EDIT_GROUP;
	SendMessageEx(m_bButtonFlags);
}

void CAclWeb::OnSelchangeComboSet() 
{
	m_History.m_bQueryEx = m_QueryCombo.GetCurSel();
	SendSet(OPT_TYPE_QUERY_EX);
}

#pragma comment( exestr, "B9D3B8FD2A63656E7967642B")
