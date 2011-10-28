// DebugHookDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "DebugHook.h"
#include "DebugHookDlg.h"
#include "stdio.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
///////////////////////////////////////////////////////////////////////
HHOOK	hDebug = NULL ;	// ���Ӿ��

// ���̹�����Ϣ�������
LRESULT CALLBACK DebugProc ( int nCode, WPARAM wParam, LPARAM lParam )
{
	if ( nCode == HC_ACTION )
	{
		PDEBUGHOOKINFO pDebugHookInfo = (PDEBUGHOOKINFO)lParam ;
		switch ( wParam )
		{
		case WH_KEYBOARD:
		case WH_MOUSE:
			{
				// ������Ӳ����ɵ�ǰDEBUG���������̰߳�װ
				// ��ֱ�ӷ��ط�0ֵ��ȡ�����Ӻ������̵���
				if ( pDebugHookInfo->idThread != pDebugHookInfo->idThreadInstaller )
					return 1 ;
			}
			break ;
		}
	}
	// ����������Ϣ
	return CallNextHookEx ( hDebug, nCode, wParam, lParam ) ;
}

BOOL WINAPI SetHook ( BOOL isInstall ) 
{
	// ��Ҫ��װ���ҹ��Ӳ�����
	if ( isInstall && !hDebug )
	{
		// ����ȫ�ֹ���
		hDebug = SetWindowsHookEx ( WH_DEBUG, (HOOKPROC)DebugProc, 0, GetCurrentThreadId() ) ;
		if ( hDebug == NULL )
			return FALSE ;
	}

	// ��Ҫж�أ��ҹ��Ӵ���
	if ( !isInstall && hDebug )
	{
		// ж�ع���
		BOOL ret = UnhookWindowsHookEx ( hDebug ) ;
		hDebug	= NULL ;
		return ret ;
	}

	return TRUE ;
}
//////////////////////////////////////////////////////////////////////////////////

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


// CDebugHookDlg �Ի���




CDebugHookDlg::CDebugHookDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDebugHookDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDebugHookDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDebugHookDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_START, &CDebugHookDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_STOP, &CDebugHookDlg::OnBnClickedStop)
END_MESSAGE_MAP()


// CDebugHookDlg ��Ϣ�������

BOOL CDebugHookDlg::OnInitDialog()
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

void CDebugHookDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CDebugHookDlg::OnPaint()
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
HCURSOR CDebugHookDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CDebugHookDlg::OnBnClickedStart()
{
	SetHook ( TRUE ) ;
}

void CDebugHookDlg::OnBnClickedStop()
{
	SetHook ( FALSE ) ;
}
