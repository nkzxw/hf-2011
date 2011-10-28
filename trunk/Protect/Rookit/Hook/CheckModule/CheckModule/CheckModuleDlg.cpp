// CheckModuleDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "CheckModule.h"
#include "CheckModuleDlg.h"
#include <TlHelp32.h>
#include <stdio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

VOID MY_OutputDebugStringW (const wchar_t* szFormat,...)
{
	WCHAR szBuf[2048] = {0} ;

	va_list argList ;
	va_start ( argList, szFormat ) ;
	vswprintf ( szBuf, sizeof(szBuf), szFormat, argList ) ;
	va_end ( argList ) ;
	
	OutputDebugStringW ( szBuf ) ;
}

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CCheckModuleDlg �Ի���




CCheckModuleDlg::CCheckModuleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCheckModuleDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCheckModuleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CCheckModuleDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_FLUSH, &CCheckModuleDlg::OnBnClickedFlush)
	ON_BN_CLICKED(IDC_CHECK, &CCheckModuleDlg::OnBnClickedCheck)
END_MESSAGE_MAP()


// CCheckModuleDlg ��Ϣ�������

BOOL CCheckModuleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CCheckModuleDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CCheckModuleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ��������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù����ʾ��
//
HCURSOR CCheckModuleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// ȡ�õ�ǰģ����Ϣ
VOID CCheckModuleDlg::InitModuleVect()
{
	this->ModuleVect.clear () ;
	DWORD dwProcessId = GetCurrentProcessId () ;
	HANDLE hModuleSnap = CreateToolhelp32Snapshot ( TH32CS_SNAPMODULE, dwProcessId ) ;
	if ( hModuleSnap == INVALID_HANDLE_VALUE ) 
		return ; 

	MODULEENTRY32 me32 = { sizeof(MODULEENTRY32) } ;
	if ( Module32First ( hModuleSnap, &me32 ) )
	{
		do{
			this->ModuleVect.push_back ( me32.szModule ) ;
		}while ( Module32Next ( hModuleSnap, &me32 ) ) ;
	}

	CloseHandle ( hModuleSnap ) ;
}

// ���ָ��ģ���ڴ�������ʼ״̬
BOOL CCheckModuleDlg::IsModuleValid( CString szModuleName )
{
	// ������ʼ״̬��ģ���б�
	for ( int i = 0; i < this->ModuleVect.size(); i++ )
	{
		if ( this->ModuleVect[i] == szModuleName )
			return TRUE ;
	}
	return FALSE ;
}

// ��ͳ�ơ�����
void CCheckModuleDlg::OnBnClickedFlush()
{
	this->InitModuleVect () ;	
	MY_OutputDebugStringW ( L"��ǰģ������:%d", this->ModuleVect.size() ) ;
}

// ����⡱����
void CCheckModuleDlg::OnBnClickedCheck()
{
	DWORD dwProcessId = GetCurrentProcessId () ;
	HANDLE hModuleSnap = CreateToolhelp32Snapshot ( TH32CS_SNAPMODULE, dwProcessId ) ;
	if ( hModuleSnap == INVALID_HANDLE_VALUE ) 
		return ; 

	DWORD	dwCount = 0 ;
	MODULEENTRY32 me32 = { sizeof(MODULEENTRY32) } ;
	if ( Module32First ( hModuleSnap, &me32 ) )
	{
		do{
			if ( !this->IsModuleValid ( me32.szModule ) )
			{
				MY_OutputDebugStringW ( L"[����ģ��]%s", me32.szExePath ) ;
				dwCount ++ ;
			}
		}while ( Module32Next ( hModuleSnap, &me32 ) ) ;
	}

	CloseHandle ( hModuleSnap ) ;
	MY_OutputDebugStringW ( L"����ģ������:%d", dwCount ) ;
}
