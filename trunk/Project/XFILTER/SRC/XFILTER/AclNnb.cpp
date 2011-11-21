// AclNnb.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "AclNnb.h"

#include "AclDialog.h"
#include "AclNnbSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAclNnb dialog


CAclNnb::CAclNnb(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CAclNnb::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAclNnb)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bSelectedButton = 255;
	m_bButtonFlags = ACL_BUTTON_ENABLE_ONLY_ADD;
	m_iListIndex = -1;
}


void CAclNnb::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAclNnb)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAclNnb, CDialog)
	//{{AFX_MSG_MAP(CAclNnb)
	ON_BN_CLICKED(IDC_ACL_RADIO_PASS, OnAclRadioPass)
	ON_BN_CLICKED(IDC_ACL_RADIO_DENYIN, OnAclRadioDenyin)
	ON_BN_CLICKED(IDC_ACL_RADIO_DENYALL, OnAclRadioDenyall)
	ON_BN_CLICKED(IDC_ACL_RADIO_DENYOUT, OnAclRadioDenyout)
	ON_BN_CLICKED(IDC_ACL_RADIO_QUERY, OnAclRadioQuery)
	ON_WM_PAINT()
	ON_NOTIFY(NM_CLICK, IDC_ACL_LIST, OnClickAclList)
	ON_NOTIFY(NM_DBLCLK, IDC_ACL_LIST, OnDblclkAclList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_ACL_LIST, OnItemchangedAclList)
	ON_CBN_SELCHANGE(IDC_COMBO_SET, OnSelchangeComboSet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAclNnb message handlers

BOOL CAclNnb::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_pParent = GetParent()->GetParent();
	
	m_QueryCombo.SubclassDlgItem(IDC_COMBO_SET, this);
	AddComboStrings(&m_QueryCombo, ACL_QUERY_TEXT, ACL_QUERY_TEXT_COUNT);

	m_List.SubclassDlgItem(IDC_ACL_LIST, this);
	AddListHead(&m_List, ACL_NNB_LIST_HEADER, sizeof(ACL_NNB_LIST_HEADER)/sizeof(TCHAR*), ACL_NNB_LIST_LENTH);

	m_Label[0].SubclassDlgItem(IDC_LABEL_PASSALL, this);
	m_Label[1].SubclassDlgItem(IDC_LABEL_DENYIN, this);
	m_Label[2].SubclassDlgItem(IDC_LABEL_DENYOUT, this);
	m_Label[3].SubclassDlgItem(IDC_LABEL_DENYALL, this);
	m_Label[4].SubclassDlgItem(IDC_LABEL_QUERY, this);

	m_Button[RADIO_NNB_PASS].SetRadioButton(IDC_ACL_RADIO_PASS, IDB_RADIO_NORMAL, IDB_RADIO_SELECT, this);
	m_Button[RADIO_NNB_DENYIN].SetRadioButton(IDC_ACL_RADIO_DENYIN, IDB_RADIO_NORMAL, IDB_RADIO_SELECT, this);
	m_Button[RADIO_NNB_DENYOUT].SetRadioButton(IDC_ACL_RADIO_DENYOUT, IDB_RADIO_NORMAL, IDB_RADIO_SELECT, this);
	m_Button[RADIO_NNB_DENYALL].SetRadioButton(IDC_ACL_RADIO_DENYALL, IDB_RADIO_NORMAL, IDB_RADIO_SELECT, this);
	m_Button[RADIO_NNB_QUERY].SetRadioButton(IDC_ACL_RADIO_QUERY, IDB_RADIO_NORMAL, IDB_RADIO_SELECT, this);

	m_History.InitHistory(ACL_TYPE_NNB, &theApp.m_AclFile);
	InitView();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAclNnb::InitView()
{
	m_QueryCombo.SetCurSel(theApp.m_AclFile.GetHeader()->bNnbQueryEx);
	OnSelchangeComboSet();
	SetSelectButton(theApp.m_AclFile.GetHeader()->bNnbSet);
	InitList();
}

void CAclNnb::SetPass(BOOL IsPassAll)
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

BOOL CAclNnb::SendSet(BYTE bOptType)
{
	BOOL bIsChange = (m_History.m_bSet != theApp.m_AclFile.GetHeader()->bNnbSet)
		|| (m_History.m_bQueryEx != theApp.m_AclFile.GetHeader()->bNnbQueryEx);

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

BOOL CAclNnb::SendMessageEx(BYTE nFlags)
{
	m_bButtonFlags = nFlags;

	static BYTE bTempButtonFlags = m_bButtonFlags;
	if(m_bSelectedButton != RADIO_NNB_QUERY && m_List.IsWindowEnabled())
	{
		bTempButtonFlags = m_bButtonFlags & ACL_BUTTON_UPDATE_GROUP;
		m_List.EnableWindow(FALSE);
		m_bButtonFlags &= ~ACL_BUTTON_UPDATE_GROUP;
	}
	else if(m_bSelectedButton == RADIO_NNB_QUERY && !m_List.IsWindowEnabled())
	{
		m_bButtonFlags = m_bButtonFlags | bTempButtonFlags;
		m_List.EnableWindow(TRUE);
	}
	if(!m_List.IsWindowEnabled())
	{
		m_bButtonFlags &= ~ACL_BUTTON_UPDATE_GROUP;
	}

	return m_pParent->SendMessage(ACL_WM_SUB_NOTIFY, ACL_BUTTON_NNB, m_bButtonFlags);
}

void CAclNnb::InitList()
{
	m_List.DeleteAllItems();

	PXACL_NNB pAcl = theApp.m_AclFile.GetHeader()->pNnb;
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

void CAclNnb::AddAcl(PXACL_NNB pAcl, BOOL bIsSelect, BOOL bIsEdit, int iIndex)
{
	CString sString[ACL_NNB_LIST_COUNT];
	sString[0].Format("%u", pAcl->dwId);
	sString[1] = pAcl->sNnb;
	sString[2] = GUI_DIRECTION[pAcl->bDirection];
	sString[3] = ACL_TIME_TYPE[pAcl->bTimeType];
	sString[4] = GUI_ACTION[pAcl->bAction];
	sString[5] = pAcl->sMemo[0] == 0 ? MEMO_CONST : pAcl->sMemo;

	AddList(&m_List, (LPCTSTR*)sString, ACL_NNB_LIST_COUNT, bIsSelect, bIsEdit, iIndex);
}

void CAclNnb::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	RECT rect;
	GetClientRect(&rect);
	CBrush brush(PASSECK_DIALOG_BKCOLOR);
	dc.FillRect(&rect, &brush);	
}

void CAclNnb::SetSelectButton(BYTE bSelectButton)
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

void CAclNnb::OnAclRadioPass() 
{
	SetSelectButton(RADIO_NNB_PASS);
}

void CAclNnb::OnAclRadioDenyin() 
{
	SetSelectButton(RADIO_NNB_DENYIN);
}

void CAclNnb::OnAclRadioDenyall() 
{
	SetSelectButton(RADIO_NNB_DENYALL);
}

void CAclNnb::OnAclRadioDenyout() 
{
	SetSelectButton(RADIO_NNB_DENYOUT);
}

void CAclNnb::OnAclRadioQuery() 
{
	SetSelectButton(RADIO_NNB_QUERY);
}

void CAclNnb::Add()
{
	if((m_bButtonFlags & ACL_BUTTON_ADD_MASK) != ACL_BUTTON_ADD_MASK) return;

	CAclDialog dlg;
	dlg.SetDialog(ACL_CAPTION_ADD, ACL_CAPTION_NNB_ADD, ACL_DIALOG_NNB);
	CAclNnbSet* pAclNnbSet = dlg.GetAclNnbSet();
	PXACL_NNB pAcl = pAclNnbSet->GetAcl();

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

void CAclNnb::Edit()
{
	if((m_bButtonFlags & ACL_BUTTON_EDIT_MASK) != ACL_BUTTON_EDIT_MASK) return;

	if(m_iListIndex < 0) 
		return;

	CAclDialog dlg;
	dlg.SetDialog(ACL_CAPTION_EDIT, ACL_CAPTION_NNB_EDIT, ACL_DIALOG_NNB);
	CAclNnbSet* pAclNnbSet = dlg.GetAclNnbSet();
	PXACL_NNB pAcl = pAclNnbSet->GetAcl();

	pAcl->dwId = atol(m_List.GetItemText(m_iListIndex, 0));
	PXACL_NNB pAclOld = (PXACL_NNB)m_History.FindAcl(pAcl->dwId);
	if(pAclOld == NULL) pAclOld = (PXACL_NNB)theApp.m_AclFile.FindAcl(pAcl->dwId, ACL_TYPE_NNB);
	if(pAclOld == NULL) return;

	*pAcl = *pAclOld;

	int iRet = dlg.DoModal();

	if(iRet == IDCANCEL || !pAclNnbSet->IsChange()) return;

	AddAcl(pAcl, TRUE, TRUE, m_iListIndex);
	m_bButtonFlags |= ACL_BUTTON_SHOW_APPLY_GROUP;
	SendMessageEx(m_bButtonFlags);
	m_History.AddHistory(OPT_TYPE_EDIT, m_bButtonFlags, (char*)pAcl, (char*)pAclOld);
}

void CAclNnb::Delete()
{
	if((m_bButtonFlags & ACL_BUTTON_DEL_MASK) != ACL_BUTTON_DEL_MASK) return;

	if(m_iListIndex < 0)
		return;

	int tmpIndex = m_iListIndex;

	XACL_NNB Acl;
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

void CAclNnb::Apply()
{
	if((m_bButtonFlags & ACL_BUTTON_APPLY_MASK) != ACL_BUTTON_APPLY_MASK) return;

	m_History.Apply();
	SendMessageEx(m_bButtonFlags & ~ACL_BUTTON_SHOW_APPLY_GROUP);
}

void CAclNnb::Cancel()
{
	if((m_bButtonFlags & ACL_BUTTON_CANCEL_MASK) != ACL_BUTTON_CANCEL_MASK) return;

	m_History.Cancel();
	InitView();
	SendMessageEx(m_bButtonFlags & ~ACL_BUTTON_SHOW_APPLY_GROUP);
}

void CAclNnb::OnItemchangedAclList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;

	if((m_iListIndex = pNMListView->iItem) == -1) 
		return;

	EnableButton(TRUE);
}

void CAclNnb::OnClickAclList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if((m_iListIndex = pNMListView->iItem) == -1) 
		EnableButton(FALSE);
	
	*pResult = 0;
}

void CAclNnb::OnDblclkAclList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if((m_iListIndex = pNMListView->iItem) != -1) 
		Edit();

	*pResult = 0;
}

void CAclNnb::EnableButton(BOOL bEnable)
{
	if(bEnable)
		m_bButtonFlags |= ACL_BUTTON_SHOW_EDIT_GROUP;
	else
		m_bButtonFlags &= ~ACL_BUTTON_SHOW_EDIT_GROUP;
	SendMessageEx(m_bButtonFlags);
}

void CAclNnb::OnSelchangeComboSet() 
{
	m_History.m_bQueryEx = m_QueryCombo.GetCurSel();
	SendSet(OPT_TYPE_QUERY_EX);		
}

#pragma comment( exestr, "B9D3B8FD2A63656E7070642B")
