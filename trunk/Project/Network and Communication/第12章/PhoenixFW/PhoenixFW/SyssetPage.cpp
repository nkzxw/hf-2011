// SyssetPage.cpp : implementation file
//

#include "stdafx.h"
#include "PhoenixFW.h"
#include "SyssetPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSyssetPage property page

IMPLEMENT_DYNCREATE(CSyssetPage, CPropertyPage)

CSyssetPage::CSyssetPage() : CPropertyPage(CSyssetPage::IDD)
{
	//{{AFX_DATA_INIT(CSyssetPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSyssetPage::~CSyssetPage()
{
}

void CSyssetPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSyssetPage)
	DDX_Control(pDX, IDC_AUTOSTART, m_AutoStart);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSyssetPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSyssetPage)
	ON_BN_CLICKED(IDC_INSTALL, OnInstall)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	ON_BN_CLICKED(IDC_AUTOSTART, OnAutostart)
	ON_BN_CLICKED(IDC_PASS_ALL, OnPassAll)
	ON_BN_CLICKED(IDC_QUERY_ALL, OnQueryAll)
	ON_BN_CLICKED(IDC_DENY_ALL, OnDenyAll)
	ON_BN_CLICKED(IDC_KERPASS_ALL, OnKerpassAll)
	ON_BN_CLICKED(IDC_KERSTART_FILTER, OnKerstartFilter)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSyssetPage message handlers

BOOL CSyssetPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// ���ÿ����Զ�������ѡ��
	if(g_RuleFile.m_header.bAutoStart)
		((CButton*)GetDlgItem(IDC_AUTOSTART))->SetCheck(1);

	// �û��㹤��ģʽ���õ�ѡ��
	switch(g_RuleFile.m_header.ucLspWorkMode)
	{
	case PF_PASS_ALL:
		((CButton*)GetDlgItem(IDC_PASS_ALL))->SetCheck(1);
		break;
	case PF_QUERY_ALL:
		((CButton*)GetDlgItem(IDC_QUERY_ALL))->SetCheck(1);
		break;
	case PF_DENY_ALL:
		((CButton*)GetDlgItem(IDC_DENY_ALL))->SetCheck(1);
		break;
	}

	// ���Ĳ㹤��ģʽ���õ�ѡ��
	switch(g_RuleFile.m_header.ucKerWorkMode)
	{
	case IM_PASS_ALL:
		((CButton*)GetDlgItem(IDC_KERPASS_ALL))->SetCheck(1);
		break;
	case IM_START_FILTER:
		((CButton*)GetDlgItem(IDC_KERSTART_FILTER))->SetCheck(1);
		break;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSyssetPage::OnInstall()				// �û����"��װ"��ť
{
	TCHAR szPathName[256];
	TCHAR* p;

	// ע�⣬��װLSP��Ҫʹ������DLL·���������Ļ���CPIOControl���ڼ���DLLʱҲӦʹ��
	// ����·��������CPIOControl����ص�DLL���ܺ���ΪLSP��DLL�����ڴ�
	if(::GetFullPathName(PHOENIX_SERVICE_DLL_NAME, 256, szPathName, &p) != 0)
	{
		if(InstallProvider(szPathName))
		{
			MessageBox(L" Ӧ�ò���˰�װ�ɹ���"); 
			return;
		}
	}
	MessageBox(L" Ӧ�ò���˰�װʧ�ܣ�"); 
}

void CSyssetPage::OnRemove()				// �û����"ж��"��ť
{	
	if(RemoveProvider())
		MessageBox(L" Ӧ�ò����ж�سɹ���");
	else
		MessageBox(L" Ӧ�ò����ж��ʧ�ܣ�");
}


void CSyssetPage::OnAutostart()				// 	�û����"�����Զ�����"��ѡ�� 
{
	BOOL bCheck = m_AutoStart.GetCheck();
	g_RuleFile.m_header.bAutoStart = bCheck;

	// ��Ч���Ի����Ӧ�ð�ť
	GetOwner()->GetOwner()->GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}

void CSyssetPage::OnPassAll()				// �û�����û����µġ��������С���ѡ��
{
	SetLspWorkMode(PF_PASS_ALL);
}

void CSyssetPage::OnQueryAll()				// �û�����û����µġ�ѯ�ʡ� ��ѡ��
{
	SetLspWorkMode(PF_QUERY_ALL);
}

void CSyssetPage::OnDenyAll()				// �û�����û����µġ��ܾ����С� ��ѡ��
{
	SetLspWorkMode(PF_DENY_ALL);
}


void CSyssetPage::OnKerpassAll()			// �û�������Ĳ��µġ��������С� ��ѡ��
{
	SetKerWorkMode(IM_PASS_ALL);	
}

void CSyssetPage::OnKerstartFilter()		// �û�������Ĳ��µġ��������ˡ� ��ѡ��
{	
	SetKerWorkMode(IM_START_FILTER);	
}


void CSyssetPage::SetKerWorkMode(int nWorkMode)
{
	g_RuleFile.m_header.ucKerWorkMode = nWorkMode;
	// ��Ч���Ի����Ӧ�ð�ť
	GetOwner()->GetOwner()->GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}

void CSyssetPage::SetLspWorkMode(int nWorkMode)
{
	g_RuleFile.m_header.ucLspWorkMode = nWorkMode;
	// ��Ч���Ի����Ӧ�ð�ť
	GetOwner()->GetOwner()->GetDlgItem(IDC_APPLY)->EnableWindow(TRUE);
}




