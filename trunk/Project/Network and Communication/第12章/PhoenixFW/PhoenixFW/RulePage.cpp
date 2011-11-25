// RulePage.cpp : implementation file
//

#include "stdafx.h"
#include "PhoenixFW.h"
#include "RulePage.h"
#include "Ruledlg.h"
#include "PhoenixFWDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRulePage property page
extern CPhoenixApp theApp;


IMPLEMENT_DYNCREATE(CRulePage, CPropertyPage)

CRulePage::CRulePage() : CPropertyPage(CRulePage::IDD)
{
	//{{AFX_DATA_INIT(CRulePage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CRulePage::~CRulePage()
{
}

void CRulePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRulePage)
	DDX_Control(pDX, IDC_RULES, m_rules);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRulePage, CPropertyPage)
	//{{AFX_MSG_MAP(CRulePage)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_DEL, OnDel)
	ON_NOTIFY(NM_CLICK, IDC_RULES, OnClickRules)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_RULES, OnItemchangedRules)
	ON_NOTIFY(NM_DBLCLK, IDC_RULES, OnDblclkRules)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRulePage message handlers


BOOL CRulePage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// ��ʼ���б���ͼ�ؼ�
	m_rules.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_rules.InsertColumn(0, L"Ӧ�ó���", LVCFMT_LEFT, sizeof(L"Ӧ�ó���")*8, 0);
	m_rules.InsertColumn(1, L"����", LVCFMT_LEFT, sizeof( L"����")*8, 1);
	m_rules.InsertColumn(2, L"����/�˿�", LVCFMT_LEFT, sizeof(L"����/�˿�")*8, 2);
	m_rules.InsertColumn(3, L"Ӧ�ó���·��", LVCFMT_LEFT, sizeof(L"Ӧ�ó���·��")*12, 3);
	m_rules.InsertColumn(4, L"˵��", LVCFMT_LEFT, sizeof(L"˵��")*12, 4);

	// �����б������б�����ӹ���
	UpdateList();

	// ��Чɾ���ͱ༭��ť
	GetDlgItem(IDC_DEL)->EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT)->EnableWindow(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CRulePage::UpdateList()
{
	// ����б�
	m_rules.DeleteAllItems();

	// ���б�����ӹ���
	for(int i=0; i<(int)g_RuleFile.m_header.ulLspRuleCount; i++)
		EditARule(&g_RuleFile.m_pLspRules[i]);
}

void CRulePage::EditARule(RULE_ITEM *pItem, int nEditIndex)
{
	// ���nEditIndex���ڵ���0�ͱ༭����ΪnEditIndex����������һ������
	int nIndex = m_rules.GetItemCount();
	if(nEditIndex >= 0)
		nIndex = nEditIndex;
	else
		m_rules.InsertItem(nIndex, L"", 0);

	// ����������ı�
	CString sAction, sServType;

	sAction = (pItem->ucAction == 0) ? L"����" : L"�ܾ�";

	switch(pItem->ucServiceType)
	{	
	case RULE_SERVICE_TYPE_ALL:
		sServType.Format(L"����/%d", pItem->usServicePort);
		break;
	case RULE_SERVICE_TYPE_TCP:
		sServType.Format(L"TCP/%d", pItem->usServicePort);
		break;
	case RULE_SERVICE_TYPE_UDP:
		sServType.Format(L"UDP/%d", pItem->usServicePort);
		break;
	case RULE_SERVICE_TYPE_FTP:
		sServType.Format(L"FTP/%d", pItem->usServicePort);
		break;
	case RULE_SERVICE_TYPE_TELNET:
		sServType.Format(L"TELNET/%d", pItem->usServicePort);
		break;
	case RULE_SERVICE_TYPE_HTTP:
		sServType.Format(L"HTTP/%d", pItem->usServicePort);
		break;
	case RULE_SERVICE_TYPE_NNTP:
		sServType.Format(L"NNTP/%d", pItem->usServicePort);
		break;
	case RULE_SERVICE_TYPE_POP3:
		sServType.Format(L"POP3/%d", pItem->usServicePort);
		break;
	}

	// ����������ı�
	m_rules.SetItemText(nIndex, 0, GetFileName(pItem->szApplication));
	m_rules.SetItemText(nIndex, 1, sAction);
	m_rules.SetItemText(nIndex, 2, sServType);
	m_rules.SetItemText(nIndex, 3, GetFilePath(pItem->szApplication));
	m_rules.SetItemText(nIndex, 4, pItem->sDemo);
}




int CRulePage::InitAddRule(LPCTSTR szQueryApp)
{
	if(g_RuleFile.m_header.ulLspRuleCount > MAX_RULE_COUNT)
	{
		AfxMessageBox(L" ��������������Ŀ�����ܹ������");
		return -1;
	}

	// ����һ��Ĭ�ϵĹ���
	RULE_ITEM tmpRule;

	_tcscpy(tmpRule.sDemo, L"");
	_tcscpy(tmpRule.szApplication, L"");
	tmpRule.ucAction = RULE_ACTION_PASS;
	tmpRule.ucDirection = RULE_DIRECTION_IN_OUT;
	tmpRule.ucServiceType = RULE_SERVICE_TYPE_ALL;
	tmpRule.usServicePort = RULE_SERVICE_PORT_ALL;

	// ���ô��ݵĲ���
	CRuleDlg::m_sPathName = szQueryApp;
	CRuleDlg::m_RuleItem = tmpRule;
	CRuleDlg::m_bAppQuery = (szQueryApp == NULL) ? 0 : 1;

	// ���ñ�ҳ��Ϊ�ҳ��
	if(CRuleDlg::m_bAppQuery)
		((CMainDlg*)theApp.m_pMainWnd)->m_sheet.SetActivePage(this);

	// ������ӹ���Ի���
	CRuleDlg dlg;
	if(dlg.DoModal() == IDCANCEL)
	{
		return -1;
	}

	// ��������ӵ��ļ�
	if(!g_RuleFile.AddLspRules(&CRuleDlg::m_RuleItem, 1))
	{
		AfxMessageBox(L"��� ACL �������");
		return -1;
	}

	// ��������ӵ��б���ͼ
	EditARule(&CRuleDlg::m_RuleItem);

	return CRuleDlg::m_RuleItem.ucAction;
}

BOOL CRulePage::AddQueryRule(LPCTSTR pszQueryApp) // ��̬����
{
	int nRet = ((CMainDlg*)theApp.m_pMainWnd)->m_RulePage.InitAddRule(pszQueryApp);
	if( nRet == -1 )
		return FALSE;

	// �����򱣴浽�ļ�
	g_RuleFile.SaveRules();

	// ������Ӧ�õ�DLLģ��
	theApp.ApplyFileData();
	// ��Ч���Ի����Ӧ�ð�ť
	((CMainDlg*)theApp.m_pMainWnd)->GetDlgItem(IDC_APPLY)->EnableWindow(FALSE);

	return nRet == RULE_ACTION_PASS;
}

void CRulePage::OnAdd()		// �û��������ӡ���ť
{
	if(InitAddRule() != 0)
		return;
	// ��Ч���Ի����Ӧ�ð�ť
	GetOwner()->GetOwner()->GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}



void CRulePage::OnEdit()									// �û�������༭�� ��ť
{
	if(m_nListIndex < 0)
		return;

	CRuleDlg::m_RuleItem = g_RuleFile.m_pLspRules[m_nListIndex];
	CRuleDlg::m_bAppQuery = FALSE;

	CRuleDlg dlg;
	if(dlg.DoModal() == IDOK)
	{
		g_RuleFile.m_pLspRules[m_nListIndex] = CRuleDlg::m_RuleItem;
		EditARule(&CRuleDlg::m_RuleItem, m_nListIndex);
		GetOwner()->GetOwner()->GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
	}
}

void CRulePage::OnDblclkRules(NMHDR* pNMHDR, LRESULT* pResult) 	// �û�˫���б�
{
	NM_LISTVIEW* pNMList = (NM_LISTVIEW*)pNMHDR;
	if((m_nListIndex = pNMList->iItem) != -1)
	{
		OnEdit();
	}
	*pResult = 0;
}

void CRulePage::OnDel()					// �û������ɾ���� ��ť
{	
	if(m_nListIndex < 0)
		return;

	// ���ļ��н�����ɾ��
	g_RuleFile.DelLspRule(m_nListIndex);
	// ���б���ͼ�н��ļ�ɾ��
	m_rules.DeleteItem(m_nListIndex);
	// ��Ч�����ڵġ�Ӧ�á���ť
	GetOwner()->GetOwner()->GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);

	// ���û�й����ˣ�����Ч���༭���͡�ɾ������ť
	if(m_rules.GetItemCount() <= 0)
	{
		GetDlgItem(IDC_DEL)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT)->EnableWindow(FALSE);	
		return;
	}
	// ����ѡ����һ������
	if(m_nListIndex == m_rules.GetItemCount()) // ���ɾ���������һ��
		m_nListIndex--;
	m_rules.SetItemState(m_nListIndex, LVIS_SELECTED, LVIS_SELECTED);
}

void CRulePage::OnItemchangedRules(NMHDR* pNMHDR, LRESULT* pResult) // �û��ı���ѡ��
{
	NM_LISTVIEW* pNMList = (NM_LISTVIEW*)pNMHDR;
	// ��ȡ��ǰѡ��������������û��ѡ���κ���Ŀ������Ч���༭���͡�ɾ������ť
	if((m_nListIndex = pNMList->iItem) != -1)
	{
		GetDlgItem(IDC_DEL)->EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT)->EnableWindow(TRUE);
	}

	*pResult = 0;
}

void CRulePage::OnClickRules(NMHDR* pNMHDR, LRESULT* pResult)		// �û������б�
{
	NM_LISTVIEW* pNMList = (NM_LISTVIEW*)pNMHDR;
	// ��ȡ��ǰѡ��������������û��ѡ���κ���Ŀ������Ч���༭���͡�ɾ������ť
	if((m_nListIndex = pNMList->iItem) == -1)
	{
		GetDlgItem(IDC_DEL)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT)->EnableWindow(FALSE);
	}
	*pResult = 0;
}

