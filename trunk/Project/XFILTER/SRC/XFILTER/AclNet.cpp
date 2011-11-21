// AclNet.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "AclNet.h"

#include "AclDialog.h"
#include "AclNetSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAclNet dialog


CAclNet::CAclNet(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CAclNet::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAclNet)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bButtonFlags = ACL_BUTTON_ENABLE_ONLY_ADD;
}


void CAclNet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAclNet)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAclNet, CDialog)
	//{{AFX_MSG_MAP(CAclNet)
	ON_WM_PAINT()
	ON_NOTIFY(TVN_SELCHANGED, IDC_NET_TREE, OnSelchangedNetTree)
	ON_NOTIFY(NM_CLICK, IDC_LIST_NET, OnClickListNet)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_NET, OnDblclkListNet)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_NET, OnItemchangedListNet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAclNet message handlers

BOOL CAclNet::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_pParent = GetParent()->GetParent();
	
	m_Tree.SubclassDlgItem(IDC_NET_TREE, this);
	AddTreeList(&m_Tree, ACL_NET_TYPE, sizeof(ACL_NET_TYPE)/sizeof(TCHAR*));

	m_List.SubclassDlgItem(IDC_LIST_NET, this);
	AddListHead(&m_List, ACL_IP_LIST_HEADER, sizeof(ACL_IP_LIST_HEADER)/sizeof(TCHAR*), ACL_IP_LIST_LENTH);

	for(int i = 0; i < ACL_NET_TYPE_COUNT; i++)
	{
		m_History[i].InitHistory(GetType(i), &theApp.m_AclFile);
	}

	ReadAllIp();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAclNet::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	RECT rect;
	GetClientRect(&rect);
	CBrush brush(PASSECK_DIALOG_BKCOLOR);
	dc.FillRect(&rect, &brush);
}

void CAclNet::ListAddIp()
{
	if(m_iTreeIndex < 0) return;
	m_List.DeleteAllItems();
	int nCount = m_arIp[m_iTreeIndex].GetSize();
	for(int i = 0; i < nCount; i++)
		ListAddOne(&m_arIp[m_iTreeIndex][i]);
	if(m_List.GetItemCount() > 0)
	{
		m_List.SetItemState(0,	LVIS_SELECTED,LVIS_SELECTED);
	}
}

int CAclNet::ListAddIp(PXACL_IP pAclIp)
{
	m_List.DeleteAllItems();

	while(pAclIp != NULL)
	{
		ListAddOne(pAclIp);
		pAclIp = pAclIp->pNext;
	}
	if(m_List.GetItemCount() > 0)
	{
		m_List.SetItemState(0,	LVIS_SELECTED,LVIS_SELECTED);
	}

	return 0;
}

int CAclNet::ListAddOne(XACL_IP* pAclIp, BOOL bIsSelect, BOOL bIsEdit, int iIndex)
{
	CString sId; sId.Format("%u", pAclIp->dwId);
	CString sStartIp =DIPToSIP(&pAclIp->ulStartIP);
	CString sEndIp =DIPToSIP(&pAclIp->ulEndIP);
	const TCHAR *pString[] = 
	{
		sId,
		sStartIp,
		sEndIp
	};

	AddList(&m_List, pString, sizeof(pString)/sizeof(TCHAR*), bIsSelect, bIsEdit, iIndex);

	return 0;
}

int CAclNet::GetType(int nIndex)
{
	switch(nIndex)
	{
	case 0:
		return ACL_TYPE_ALL_IP;
	case 1:
		return ACL_TYPE_INTRANET_IP;
	case 2:
		return ACL_TYPE_DISTRUST_IP;
	case 3:
		return ACL_TYPE_TRUST_IP;
	case 4:
		return ACL_TYPE_CUSTOM_IP;
	default:
		return -1;
	}
}

void CAclNet::OnSelchangedNetTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	
	CString sText = m_Tree.GetItemText(m_Tree.GetSelectedItem());
	int nCount = sizeof(ACL_NET_TYPE)/sizeof(TCHAR*);
	int nIndex =TextToIndex(sText , ACL_NET_TYPE, nCount);
	int nType = GetType(nIndex);

	m_iTreeIndex = nIndex;
	m_bType = nType;

	//ListAddIp((PXACL_IP)theApp.m_AclFile.FindAcl(1, nType));
	ListAddIp();

	if(m_iTreeIndex == 0)
		m_bButtonFlags = 0;
	else
		m_bButtonFlags |= ACL_BUTTON_ENABLE_ONLY_ADD;
	SendMessageEx(m_bButtonFlags);

	*pResult = 0;
}

BOOL CAclNet::SendMessageEx(BYTE nFlags)
{
	return GetParent()->GetParent()->SendMessage(ACL_WM_SUB_NOTIFY, ACL_BUTTON_NET, nFlags);
}

void CAclNet::ReadAllIp()
{
	for(int i = 0; i < ACL_NET_TYPE_COUNT; i++)
		m_arIp[i].RemoveAll();
	PXACL_HEADER pHeader = theApp.m_AclFile.GetHeader();
	ReadIp(pHeader->pAllIp, 0);
	ReadIp(pHeader->pIntranetIp, 1);
	ReadIp(pHeader->pDistrustIp, 2);
	ReadIp(pHeader->pTrustIp, 3);
	ReadIp(pHeader->pCustomIp, 4);
}

void CAclNet::ReadIp(PXACL_IP pFirst, int nType)
{
	while(pFirst != NULL)
	{
		m_arIp[nType].Add(*pFirst);
		pFirst = pFirst->pNext;
	}
}

void CAclNet::Add()
{
	if((m_bButtonFlags & ACL_BUTTON_ADD_MASK) != ACL_BUTTON_ADD_MASK) return;

	CAclDialog dlg;
	dlg.SetDialog(ACL_CAPTION_ADD, ACL_CAPTION_NET_ADD, ACL_DIALOG_NET);
	CAclNetSet* pSet = dlg.GetAclNetSet();
	PXACL_IP pIp = pSet->GetIp();

	int nIndex = m_List.GetItemCount();
	if(nIndex == 0)
		pIp->dwId = 1;
	else
		pIp->dwId = atol(m_List.GetItemText(nIndex - 1, 0)) + 1;

	int iRet = dlg.DoModal();

	if(iRet == IDCANCEL) return;

	m_arIp[m_iTreeIndex].Add(*pIp);

	ListAddOne(pIp);
	m_bButtonFlags |= ACL_BUTTON_SHOW_APPLY_GROUP;
	m_History[m_iTreeIndex].AddHistory(OPT_TYPE_ADD, m_bButtonFlags & (pIp->bNotAllowEdit ? 0x00 : 0xFF), (char*)pIp);
	SendMessageEx(m_bButtonFlags);
}
void CAclNet::Edit()
{
	if((m_bButtonFlags & ACL_BUTTON_EDIT_MASK) != ACL_BUTTON_EDIT_MASK) return;
	if(m_iListIndex < 0) 
		return;

	CAclDialog dlg;
	dlg.SetDialog(ACL_CAPTION_EDIT, ACL_CAPTION_NET_EDIT, ACL_DIALOG_NET);
	CAclNetSet* pSet = dlg.GetAclNetSet();
	PXACL_IP pIp = pSet->GetIp();
	pSet->SetEdit(TRUE);

	*pIp = m_arIp[m_iTreeIndex][m_iListIndex];

	int iRet = dlg.DoModal();

	if(iRet == IDCANCEL || !pSet->IsChange()) return;

	ListAddOne(pIp, TRUE, TRUE, m_iListIndex);
	m_bButtonFlags |= ACL_BUTTON_SHOW_APPLY_GROUP;
	SendMessageEx(m_bButtonFlags);
	m_History[m_iTreeIndex].AddHistory(OPT_TYPE_EDIT, m_bButtonFlags, (char*)pIp, (char*)&m_arIp[m_iTreeIndex][m_iListIndex]);
	m_arIp[m_iTreeIndex][m_iListIndex] = *pIp;
}
void CAclNet::Delete()
{
	if((m_bButtonFlags & ACL_BUTTON_DEL_MASK) != ACL_BUTTON_DEL_MASK) return;
	if(m_iListIndex < 0)
		return;

	int tmpIndex = m_iListIndex;

	m_bButtonFlags |= ACL_BUTTON_SHOW_APPLY_GROUP;
	SendMessageEx(m_bButtonFlags);

	m_History[m_iTreeIndex].AddHistory(OPT_TYPE_DELETE, m_bButtonFlags, (char*)&m_arIp[m_iTreeIndex][m_iListIndex]);

	m_arIp[m_iTreeIndex].RemoveAt(m_iListIndex);
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
void CAclNet::Apply()
{
	if((m_bButtonFlags & ACL_BUTTON_APPLY_MASK) != ACL_BUTTON_APPLY_MASK) return;
	for(int i = 0; i < ACL_NET_TYPE_COUNT; i++)
		m_History[i].Apply();
	m_bButtonFlags &= ~ACL_BUTTON_SHOW_APPLY_GROUP;
	SendMessageEx(m_bButtonFlags);
}
void CAclNet::Cancel()
{	
	if((m_bButtonFlags & ACL_BUTTON_CANCEL_MASK) != ACL_BUTTON_CANCEL_MASK) return;
	for(int i = 0; i < ACL_NET_TYPE_COUNT; i++)
		m_History[i].Cancel();
	ReadAllIp();
	ListAddIp();
	m_bButtonFlags &= ~ACL_BUTTON_SHOW_APPLY_GROUP;
	SendMessageEx(m_bButtonFlags);
}

int CAclNet::EnableButton(BOOL bEnableEdit)
{
	if(bEnableEdit)
		m_bButtonFlags |= ACL_BUTTON_SHOW_EDIT_GROUP;
	else
		m_bButtonFlags &= ~ACL_BUTTON_SHOW_EDIT_GROUP;
	SendMessageEx(m_bButtonFlags);
	return 0;
}

void CAclNet::OnClickListNet(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if((m_iListIndex = pNMListView->iItem) == -1)
		EnableButton(FALSE);

	*pResult = 0;
}

void CAclNet::OnDblclkListNet(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if((m_iListIndex = pNMListView->iItem) != -1)
		Edit();
	
	*pResult = 0;
}

void CAclNet::OnItemchangedListNet(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView	= (NM_LISTVIEW*)pNMHDR;
	*pResult					= 0;

	if((m_iListIndex = pNMListView->iItem) == -1)		
		return;

	if(m_iTreeIndex != 0)			
		EnableButton(TRUE);
}

#pragma comment( exestr, "B9D3B8FD2A63656E7067762B")
