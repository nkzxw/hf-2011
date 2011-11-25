// MonitorPage.cpp : implementation file
//

#include "stdafx.h"
#include "PhoenixFW.h"
#include "MonitorPage.h"

#include "RulePage.h"

#include "PhoenixFWDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMonitorPage property page

extern CPhoenixApp theApp;

IMPLEMENT_DYNCREATE(CMonitorPage, CPropertyPage)

CMonitorPage::CMonitorPage() : CPropertyPage(CMonitorPage::IDD)
{
	//{{AFX_DATA_INIT(CMonitorPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CMonitorPage::~CMonitorPage()
{
}

void CMonitorPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMonitorPage)
	DDX_Control(pDX, IDC_TREEMONITOR, m_MonitorTree);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMonitorPage, CPropertyPage)
	//{{AFX_MSG_MAP(CMonitorPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMonitorPage message handlers

BOOL CMonitorPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



void CMonitorPage::HandleNotifySession(SESSION *pSession, int nCode)
{
	// �õ���ǰCMonitorPage���ָ�루ע�⣬HandleNotifySession��һ����̬������
	CMonitorPage *pThis = &(((CMainDlg*)theApp.m_pMainWnd)->m_MonitorPage);

	// һ���Ự�����Ըı���
	if(nCode == CODE_CHANGE_SESSION)	
		pThis->AddASession(pSession);
	// һ���Ự��ɾ��
	else if(nCode == CODE_DELETE_SESSION)
		pThis->DeleteASession(pSession, FALSE);
	// һ��Ӧ�ó����˳�������Ҫɾ����Ӧ�ó�������лỰ
	else if(nCode == CODE_APP_EXIT)
		pThis->DeleteASession(pSession, TRUE);
}

void CMonitorPage::AddASession(SESSION *pSession)
{
	TRACE(L" AddASession... ");

	// ���Ȳ鿴������Ӧ�ó������û�У��Ͳ���һ���µ�Ӧ�ó�����
	// Ҫ�ڴ�Ӧ�ó���������ӻỰ
	HTREEITEM hAppItem = FindAppItem(pSession->szPathName);
	if(hAppItem == NULL) 
	{
		hAppItem = m_MonitorTree.InsertItem(pSession->szPathName);
	}

	// ͨ��SESSION�ṹ����������ʾ���ı�
	CString sText = BuildSessionText(pSession);

	// ��Ӧ�ó������£������׽��־���鿴�˻Ự�Ƿ��Ѿ����ڣ�
	// ������ڣ�������������ı�����������ڣ�Ҫ����һ���µ�����
	HTREEITEM hSessionItem = FindSessionItem(hAppItem, pSession);
	if(hSessionItem != NULL)
	{
		m_MonitorTree.SetItemText(hSessionItem, sText);
	}
	else
	{	
		hSessionItem = m_MonitorTree.InsertItem(sText,hAppItem);
		m_MonitorTree.SetItemData(hSessionItem, pSession->s);
	}
}

HTREEITEM CMonitorPage::FindAppItem(TCHAR *pszPathName)
{
	// ��������Ӧ�ó��������ָ��Ӧ�ó����Ƿ����
	HTREEITEM hAppItem = m_MonitorTree.GetNextItem(TVI_ROOT, TVGN_CHILD);
	while(hAppItem != NULL)
	{
		if(m_MonitorTree.GetItemText(hAppItem).CompareNoCase(pszPathName) == 0)
			return hAppItem; // ���ڣ���������

		hAppItem = m_MonitorTree.GetNextItem(hAppItem, TVGN_NEXT);
	}
	return NULL;
}

HTREEITEM CMonitorPage::FindSessionItem(HTREEITEM hAppItem, SESSION *pSession)
{
	// �������лỰ�����ָ���Ự�Ƿ����
	HTREEITEM hSessionItem = m_MonitorTree.GetNextItem(hAppItem, TVGN_CHILD);
	while(hSessionItem != NULL)
	{
		if(pSession->s == m_MonitorTree.GetItemData(hSessionItem))
			return hSessionItem; // ���ڣ���������

		hSessionItem = m_MonitorTree.GetNextItem(hSessionItem, TVGN_NEXT);
	}
	return NULL;
}

CString CMonitorPage::BuildSessionText(SESSION *pSession)
{
	CString sText;

	CString sServType, sLocal, sRemote, sDirection;

	// ����IP��ַ
	BYTE *pByte = (BYTE *)&pSession->ulLocalIP; // ע�⣬�����IP��ַ�������ֽ�˳��
	sLocal.Format(L"%d.%d.%d.%d��%d", pByte[0], pByte[1], pByte[2], pByte[3], pSession->usLocalPort);

	// Զ��IP��ַ
	pByte = (BYTE *)&pSession->ulRemoteIP;
	sRemote.Format(L"%d.%d.%d.%d��%d", pByte[0], pByte[1], pByte[2], pByte[3], pSession->usRemotePort);

	// ��������
	sServType = L"����";
	switch(pSession->nProtocol)
	{	
	case RULE_SERVICE_TYPE_ALL:
		sServType.Format(L"����");
		break;
	case RULE_SERVICE_TYPE_TCP:
		sServType.Format(L"TCP");
		break;
	case RULE_SERVICE_TYPE_UDP:
		sServType.Format(L"UDP");
		break;
	case RULE_SERVICE_TYPE_FTP:
		sServType.Format(L"FTP");
		break;
	case RULE_SERVICE_TYPE_TELNET:
		sServType.Format(L"TELNET");
		break;
	case RULE_SERVICE_TYPE_HTTP:
		sServType.Format(L"HTTP");
		break;
	case RULE_SERVICE_TYPE_NNTP:
		sServType.Format(L"NNTP");
		break;
	case RULE_SERVICE_TYPE_POP3:
		sServType.Format(L"POP3");
		break;
	case RULE_SERVICE_TYPE_SMTP:
		sServType.Format(L"SMTP");
		break;
	}

	// ����
	switch(pSession->ucDirection)
	{
	case RULE_DIRECTION_IN:
		sDirection = L"<����";
		break;
	case RULE_DIRECTION_OUT:
		sDirection = L"����>";
		break;
	case RULE_DIRECTION_IN_OUT:
		sDirection = L"<����>";
		break;
	default:
		sDirection = L"����";
	}

	sText.Format(L" %s Э��	    ��%s�� %s ��%s�� ", sServType, sLocal, sDirection, sRemote);
	return sText;
}


void CMonitorPage::DeleteASession(SESSION *pSession, BOOL bAppExit)
{
	TRACE(L" DeleteASession... ");

	HTREEITEM hAppItem = FindAppItem(pSession->szPathName);
	if(hAppItem != NULL)
	{
		if(bAppExit) // Ӧ�ó����˳���ɾ������Ӧ�ó������������ĻỰ���
		{
			m_MonitorTree.DeleteItem(hAppItem);
		}
		else		// ���Ựɾ������Ӧ�ó����������ҵ�����Ự����֮ɾ��
		{
			HTREEITEM hSessionItem = FindSessionItem(hAppItem, pSession);
			if(hSessionItem != NULL)
			{	
				m_MonitorTree.DeleteItem(hSessionItem);
			}
			// û��Session�����ˣ���Ӧ�ó�����Ҳɾ��
			if(m_MonitorTree.GetNextItem(hAppItem, TVGN_CHILD) == NULL) 
				m_MonitorTree.DeleteItem(hAppItem);
		}
	}
}
