// AntiHookDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "AntiHook.h"
#include "AntiHookDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "..\\detour\\detours.h"
/////////////////////////////////////////////////////////
static HMODULE (WINAPI* OLD_LoadLibraryExW)(LPCTSTR lpFileName,HANDLE hFile,DWORD dwFlags)=LoadLibraryExW;

VOID MY_OutputDebugStringW (const wchar_t* szFormat,...)
{
	WCHAR szBuf[2048] = {0} ;

	va_list argList ;
	va_start ( argList, szFormat ) ;
	vswprintf ( szBuf, sizeof(szBuf), szFormat, argList ) ;
	va_end ( argList ) ;

	OutputDebugStringW ( szBuf ) ;
}

HMODULE WINAPI NEW_LoadLibraryExW(LPCTSTR lpFileName,HANDLE hFile,DWORD dwFlags)
{
	MY_OutputDebugStringW ( L"%s", lpFileName ) ;
	return NULL ;
	//return OLD_LoadLibraryExW ( lpFileName, hFile, dwFlags ) ;
}

VOID Hook ()
{
	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// �������������ε���DetourAttach������Hook�������
	DetourAttach(&(PVOID&)OLD_LoadLibraryExW, NEW_LoadLibraryExW ) ;

	DetourTransactionCommit();
}

VOID UnHook ()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// �������������ε���DetourDetach�����������������Hook
	DetourDetach(&(PVOID&)OLD_LoadLibraryExW, NEW_LoadLibraryExW ) ;

	DetourTransactionCommit();
}
/////////////////////////////////////////////////////////

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


// CAntiHookDlg �Ի���




CAntiHookDlg::CAntiHookDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAntiHookDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CAntiHookDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAntiHookDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_START, &CAntiHookDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_STOP, &CAntiHookDlg::OnBnClickedStop)
END_MESSAGE_MAP()


// CAntiHookDlg ��Ϣ�������

BOOL CAntiHookDlg::OnInitDialog()
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
	this->bStatus = FALSE ;

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CAntiHookDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CAntiHookDlg::OnPaint()
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
HCURSOR CAntiHookDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CAntiHookDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: �ڴ˴������Ϣ����������
	UnHook () ;
}

void CAntiHookDlg::OnBnClickedStart()
{
	if ( this->bStatus == FALSE )
	{
		this->bStatus = TRUE ;
		Hook () ;
	}
}

void CAntiHookDlg::OnBnClickedStop()
{
	if ( this->bStatus )
	{
		UnHook () ;
		this->bStatus = FALSE ;
	}
}
