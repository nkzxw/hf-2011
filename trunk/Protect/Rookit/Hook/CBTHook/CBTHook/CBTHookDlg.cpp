// CBTHookDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "CBTHook.h"
#include "CBTHookDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////
HHOOK	hCBT = NULL ;	// ����CBT���Ӿ��

// ���̹�����Ϣ�������
LRESULT CALLBACK CBTProc ( int nCode, WPARAM wParam, LPARAM lParam )
{
	switch ( nCode )
	{
	case HCBT_ACTIVATE:		OutputDebugStringA ( "HCBT_ACTIVE" ) ;		break ;
	case HCBT_CREATEWND:	OutputDebugStringA ( "HCBT_CREATEWND" ) ;	break ;
	case HCBT_DESTROYWND:	OutputDebugStringA ( "HCBT_DESTORYWND" ) ;	break ;
	case HCBT_MINMAX:		OutputDebugStringA ( "HCBT_MINMAX" ) ;		break ;
	case HCBT_MOVESIZE:		OutputDebugStringA ( "HCBT_MOVESIZE" ) ;	break ;
	case HCBT_SETFOCUS:		OutputDebugStringA ( "HCBT_SETFOCUS" ) ;	break ;
	case HCBT_SYSCOMMAND:	OutputDebugStringA ( "HCBT_SYSCOMMAND" ) ;	break ;
	}
	// ����������Ϣ
	return CallNextHookEx ( hCBT, nCode, wParam, lParam ) ;
}

BOOL WINAPI SetHook ( BOOL isInstall ) 
{
	// ��Ҫ��װ���ҹ��Ӳ�����
	if ( isInstall && !hCBT )
	{
		// ����ȫ�ֹ���
		hCBT = SetWindowsHookEx ( WH_CBT, (HOOKPROC)CBTProc, 0, GetCurrentThreadId() ) ;
		if ( hCBT == NULL )
			return FALSE ;
	}

	// ��Ҫж�أ��ҹ��Ӵ���
	if ( !isInstall && hCBT )
	{
		// ж�ع���
		BOOL ret = UnhookWindowsHookEx ( hCBT ) ;
		hCBT	= NULL ;
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


// CCBTHookDlg �Ի���




CCBTHookDlg::CCBTHookDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCBTHookDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCBTHookDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CCBTHookDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_START, &CCBTHookDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_STOP, &CCBTHookDlg::OnBnClickedStop)
END_MESSAGE_MAP()


// CCBTHookDlg ��Ϣ�������

BOOL CCBTHookDlg::OnInitDialog()
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

void CCBTHookDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CCBTHookDlg::OnPaint()
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
HCURSOR CCBTHookDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CCBTHookDlg::OnBnClickedStart()
{
	if ( !SetHook ( TRUE ) )
		this->MessageBox ( L"��װ����ʧ�ܣ�" ) ;
}

void CCBTHookDlg::OnBnClickedStop()
{
	if ( !SetHook ( FALSE ) )
		this->MessageBox ( L"ж�ع���ʧ�ܣ�" ) ;
}
