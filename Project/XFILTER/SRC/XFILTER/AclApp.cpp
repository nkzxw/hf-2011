// AclApp.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "AclApp.h"

#include "AclDialog.h"
#include "AclSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAclApp dialog


CAclApp::CAclApp(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CAclApp::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAclApp)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bSelectedButton = 255;
	m_bButtonFlags = ACL_BUTTON_ENABLE_ONLY_ADD;
	m_iListIndex = -1;
}


void CAclApp::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAclApp)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAclApp, CDialog)
	//{{AFX_MSG_MAP(CAclApp)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_ACL_RADIO_PASS, OnAclRadioPass)
	ON_BN_CLICKED(IDC_ACL_RADIO_DENY, OnAclRadioDeny)
	ON_BN_CLICKED(IDC_ACL_RADIO_QUERY, OnAclRadioQuery)
	ON_CBN_SELCHANGE(IDC_COMBO_SET, OnSelchangeComboSet)
	ON_NOTIFY(NM_CLICK, IDC_ACL_LIST, OnClickAclList)
	ON_NOTIFY(NM_DBLCLK, IDC_ACL_LIST, OnDblclkAclList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_ACL_LIST, OnItemchangedAclList)
	ON_NOTIFY(LVN_DELETEALLITEMS, IDC_ACL_LIST, OnDeleteallitemsAclList)
	ON_NOTIFY(LVN_DELETEITEM, IDC_ACL_LIST, OnDeleteitemAclList)
	ON_BN_CLICKED(IDC_ACL_RADIO_DENYIN, OnAclRadioDenyin)
	ON_BN_CLICKED(IDC_ACL_RADIO_DENYOUT, OnAclRadioDenyout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAclApp message handlers

BOOL CAclApp::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_pParent = GetParent()->GetParent();

	m_History.InitHistory(ACL_TYPE_APP, &theApp.m_AclFile);
	
	m_QueryCombo.SubclassDlgItem(IDC_COMBO_SET, this);
	AddComboStrings(&m_QueryCombo, ACL_QUERY_TEXT, ACL_QUERY_TEXT_COUNT);

	m_Label[0].SubclassDlgItem(IDC_LABEL_PASSALL, this);
	m_Label[1].SubclassDlgItem(IDC_LABEL_DENYIN, this);
	m_Label[2].SubclassDlgItem(IDC_LABEL_DENYOUT, this);
	m_Label[3].SubclassDlgItem(IDC_LABEL_DENYALL, this);
	m_Label[4].SubclassDlgItem(IDC_LABEL_QUERY, this);

	m_Button[RADIO_APP_PASS].SetRadioButton(IDC_ACL_RADIO_PASS, IDB_RADIO_NORMAL, IDB_RADIO_SELECT, this);
	m_Button[RADIO_APP_DENYIN].SetRadioButton(IDC_ACL_RADIO_DENYIN, IDB_RADIO_NORMAL, IDB_RADIO_SELECT, this);
	m_Button[RADIO_APP_DENYOUT].SetRadioButton(IDC_ACL_RADIO_DENYOUT, IDB_RADIO_NORMAL, IDB_RADIO_SELECT, this);
	m_Button[RADIO_APP_DENY].SetRadioButton(IDC_ACL_RADIO_DENY, IDB_RADIO_NORMAL, IDB_RADIO_SELECT, this);
	m_Button[RADIO_APP_QUERY].SetRadioButton(IDC_ACL_RADIO_QUERY, IDB_RADIO_NORMAL, IDB_RADIO_SELECT, this);

	m_List.SubclassDlgItem(IDC_ACL_LIST, this);
	AddListHead(&m_List, ACL_APP_LIST_HEADER, sizeof(ACL_APP_LIST_HEADER)/sizeof(TCHAR*), ACL_APP_LIST_LENTH);

	m_ImageList.Create(16, 16, ILC_COLORDDB, 0, 50);

	InitView();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAclApp::SetPass(BOOL IsPassAll)
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

void CAclApp::InitView()
{
	m_QueryCombo.SetCurSel(theApp.m_AclFile.GetHeader()->bAppQueryEx);
	OnSelchangeComboSet();
	SetSelectButton(theApp.m_AclFile.GetHeader()->bAppSet);
	InitList();
}

void CAclApp::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	RECT rect;
	GetClientRect(&rect);
	CBrush brush(PASSECK_DIALOG_BKCOLOR);
	dc.FillRect(&rect, &brush);
	
}

void CAclApp::SetSelectButton(BYTE bSelectButton)
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

BOOL CAclApp::SendSet(BYTE bOptType)
{
	BOOL bIsChange = (m_History.m_bSet != theApp.m_AclFile.GetHeader()->bAppSet)
		|| (m_History.m_bQueryEx != theApp.m_AclFile.GetHeader()->bAppQueryEx);

	bIsChange ? m_bButtonFlags |= ACL_BUTTON_SHOW_APPLY_GROUP
		: m_bButtonFlags &= ~ACL_BUTTON_SHOW_APPLY_GROUP;

	BOOL bRet = SendMessageEx(m_bButtonFlags); 
	m_History.AddHistory(
		bOptType
		, m_bButtonFlags
		, NULL
		, NULL
		, m_History.m_bSet
		, m_History.m_bQueryEx);

	return bRet;
}

BOOL CAclApp::SendMessageEx(BYTE nFlags)
{
	m_bButtonFlags = nFlags;

	static BYTE bTempButtonFlags = m_bButtonFlags;
	if(m_bSelectedButton != RADIO_APP_QUERY && m_List.IsWindowEnabled())
	{
		bTempButtonFlags = m_bButtonFlags & ACL_BUTTON_UPDATE_GROUP;
		m_List.EnableWindow(FALSE);
		m_bButtonFlags &= ~ACL_BUTTON_UPDATE_GROUP;
	}
	else if(m_bSelectedButton == RADIO_APP_QUERY && !m_List.IsWindowEnabled())
	{
		m_bButtonFlags = m_bButtonFlags | bTempButtonFlags;
		m_List.EnableWindow(TRUE);
	}
	if(!m_List.IsWindowEnabled())
	{
		m_bButtonFlags &= ~ACL_BUTTON_UPDATE_GROUP;
	}

	return m_pParent->SendMessage(ACL_WM_SUB_NOTIFY, ACL_BUTTON_APP, m_bButtonFlags);
}

void CAclApp::OnAclRadioPass() 
{
	SetSelectButton(RADIO_APP_PASS);
}

void CAclApp::OnAclRadioDenyin() 
{
	SetSelectButton(RADIO_APP_DENYIN);
}

void CAclApp::OnAclRadioDenyout() 
{
	SetSelectButton(RADIO_APP_DENYOUT);
}

void CAclApp::OnAclRadioDeny() 
{
	SetSelectButton(RADIO_APP_DENY);
}

void CAclApp::OnAclRadioQuery() 
{
	SetSelectButton(RADIO_APP_QUERY);
}

void CAclApp::OnSelchangeComboSet() 
{
	m_History.m_bQueryEx = m_QueryCombo.GetCurSel();
	SendSet(OPT_TYPE_QUERY_EX);
}

void CAclApp::InitList()
{
	m_List.DeleteAllItems();
	m_List.SetImageList(&m_ImageList, LVSIL_SMALL);

	PXACL pAcl = theApp.m_AclFile.GetHeader()->pAcl;
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

void CAclApp::AddAcl(PXACL pAcl, BOOL bIsSelect, BOOL bIsEdit, int iIndex)
{
	CString sString[ACL_APP_LIST_COUNT];
	sString[0].Format("%u", pAcl->ulAclID);
	sString[1] = (pAcl->sApplication[0] == '*' ? "*" : GetName(pAcl->sApplication));
	sString[2] = GUI_DIRECTION[pAcl->bDirection];
	sString[3] = ACL_NET_TYPE[pAcl->bRemoteNetType];
	sString[4].Format("%u", pAcl->uiServicePort);
	sString[5].Format("%u", pAcl->wLocalPort);
	sString[6] = GUI_ACTION[pAcl->bAction];
	sString[7] = GUI_SERVICE_TYPE[pAcl->bServiceType];
	sString[8] = ACL_TIME_TYPE[pAcl->bAccessTimeType];
	sString[9] = pAcl->sMemo[0] == 0 ? MEMO_CONST : pAcl->sMemo;
	sString[10] = (pAcl->sApplication[0] == '*' ? "*" : GetPath(pAcl->sApplication));

	HICON hIcon = ExtractIcon(theApp.m_hInstance, pAcl->sApplication, 0);
	if(hIcon == NULL)
		hIcon = theApp.LoadIcon(IDR_NULLAPP);
	int iIcon;
	if(bIsEdit)
		iIcon = m_ImageList.Replace(iIndex, hIcon);
	else
		iIcon = m_ImageList.Add(hIcon);
	DestroyIcon(hIcon);

	iIndex = AddList(&m_List, (LPCTSTR*)sString, ACL_APP_LIST_COUNT, bIsSelect, bIsEdit, iIndex, iIcon);
}

void CAclApp::Add()
{
	if((m_bButtonFlags & ACL_BUTTON_ADD_MASK) != ACL_BUTTON_ADD_MASK) return;

	CAclDialog dlg;
	dlg.SetDialog(ACL_CAPTION_ADD, ACL_CAPTION_APP_ADD, ACL_DIALOG_APP);
	CAclSet* pAclSet = dlg.GetAclSet();
	PXACL pAcl = pAclSet->GetAcl();

	int nIndex = m_List.GetItemCount();
	if(nIndex == 0)
		pAcl->ulAclID = 1;
	else
		pAcl->ulAclID = atol(m_List.GetItemText(nIndex - 1, 0)) + 1;

	pAcl->bDirection = ACL_DIRECTION_IN_OUT;
	pAcl->sApplication[0] = '*';

	int iRet = dlg.DoModal();

	if(iRet == IDCANCEL) return;

	AddAcl(pAcl);
	m_bButtonFlags |= ACL_BUTTON_SHOW_APPLY_GROUP;
	m_History.AddHistory(OPT_TYPE_ADD, m_bButtonFlags, (char*)pAcl);
	SendMessageEx(m_bButtonFlags);
}

void CAclApp::Edit()
{
	if((m_bButtonFlags & ACL_BUTTON_EDIT_MASK) != ACL_BUTTON_EDIT_MASK) return;

	if(m_iListIndex < 0) 
		return;

	CAclDialog dlg;
	dlg.SetDialog(ACL_CAPTION_EDIT, ACL_CAPTION_APP_EDIT, ACL_DIALOG_APP);
	CAclSet* pAclSet = dlg.GetAclSet();
	pAclSet->SetAutoPort(FALSE);
	PXACL pAcl = pAclSet->GetAcl();

	pAcl->ulAclID = atol(m_List.GetItemText(m_iListIndex, 0));
	PXACL pAclOld = (PXACL)m_History.FindAcl(pAcl->ulAclID);
	if(pAclOld == NULL) pAclOld = (PXACL)theApp.m_AclFile.FindAcl(pAcl->ulAclID, ACL_TYPE_APP);
	if(pAclOld == NULL) return;

	*pAcl = *pAclOld;

	int iRet = dlg.DoModal();

	if(iRet == IDCANCEL || !pAclSet->IsChange()) return;

	AddAcl(pAcl, TRUE, TRUE, m_iListIndex);
	m_bButtonFlags |= ACL_BUTTON_SHOW_APPLY_GROUP;
	SendMessageEx(m_bButtonFlags);
	m_History.AddHistory(OPT_TYPE_EDIT, m_bButtonFlags, (char*)pAcl, (char*)pAclOld);
}

void CAclApp::Delete()
{
	if((m_bButtonFlags & ACL_BUTTON_DEL_MASK) != ACL_BUTTON_DEL_MASK) return;

	if(m_iListIndex < 0)
		return;
	
	int tmpIndex;
//	do
//	{
//		m_iListIndex = m_List.GetNextItem(-1, LVNI_SELECTED);
		tmpIndex = m_iListIndex;
//		if(m_iListIndex == -1) break;

		XACL Acl;
		Acl.ulAclID = atol(m_List.GetItemText(m_iListIndex, 0));

		m_bButtonFlags |= ACL_BUTTON_SHOW_APPLY_GROUP;
		SendMessageEx(m_bButtonFlags);

		m_History.AddHistory(OPT_TYPE_DELETE, m_bButtonFlags, (char*)&Acl);

		m_List.DeleteItem(m_iListIndex);

//	}while(true);

	if(m_List.GetItemCount() <= 0) EnableButton(FALSE);

	if(tmpIndex == m_List.GetItemCount())	
		tmpIndex -- ;
	m_List.SetItemState(tmpIndex, LVIS_SELECTED, LVIS_SELECTED);
}

void CAclApp::Apply()
{
	if((m_bButtonFlags & ACL_BUTTON_APPLY_MASK) != ACL_BUTTON_APPLY_MASK) return;

	m_History.Apply();
	SendMessageEx(m_bButtonFlags & ~ACL_BUTTON_SHOW_APPLY_GROUP);
}

void CAclApp::Cancel()
{
	if((m_bButtonFlags & ACL_BUTTON_CANCEL_MASK) != ACL_BUTTON_CANCEL_MASK) return;

	m_History.Cancel();
	InitView();
}

void CAclApp::OnClickAclList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if((m_iListIndex = pNMListView->iItem) == -1) 
		EnableButton(FALSE);

	*pResult = 0;
}

void CAclApp::OnDblclkAclList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if((m_iListIndex = pNMListView->iItem) != -1) 
		Edit();

	*pResult = 0;	
}

void CAclApp::OnItemchangedAclList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;

	if((m_iListIndex = pNMListView->iItem) == -1) 
		return;

	EnableButton(TRUE);
}

void CAclApp::EnableButton(BOOL bEnable)
{
	if(bEnable)
		m_bButtonFlags |= ACL_BUTTON_SHOW_EDIT_GROUP;
	else
		m_bButtonFlags &= ~ACL_BUTTON_SHOW_EDIT_GROUP;
	SendMessageEx(m_bButtonFlags);
}

void CAclApp::OnDeleteallitemsAclList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	int nCount = m_ImageList.GetImageCount();
	for(int i = nCount - 1; i >= 0; i--)
		m_ImageList.Remove(i);

	*pResult = 0;
}

void CAclApp::OnDeleteitemAclList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
	//int iIndex = pNMListView->iItem;
	//if(iIndex != -1) 
	//	m_ImageList.Remove(iIndex);

	*pResult = 0;
}


#pragma comment( exestr, "B9D3B8FD2A63656E6372722B")
