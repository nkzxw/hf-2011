// LLKeyboardDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "LLKeyboard.h"
#include "LLKeyboardDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


HHOOK	hLLKeyboard = NULL ;


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


// CLLKeyboardDlg �Ի���




CLLKeyboardDlg::CLLKeyboardDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLLKeyboardDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CLLKeyboardDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CLLKeyboardDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_INSTALL, &CLLKeyboardDlg::OnBnClickedInstall)
	ON_BN_CLICKED(IDC_UNINSTALL, &CLLKeyboardDlg::OnBnClickedUninstall)
END_MESSAGE_MAP()


// CLLKeyboardDlg ��Ϣ�������

BOOL CLLKeyboardDlg::OnInitDialog()
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

void CLLKeyboardDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CLLKeyboardDlg::OnPaint()
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
HCURSOR CLLKeyboardDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT) lParam;
	if (nCode == HC_ACTION) 
	{
		if ( (p->vkCode == VK_LWIN) || (p->vkCode == VK_RWIN) ||					// ����Win				
			( p->vkCode == VK_TAB && (p->flags & LLKHF_ALTDOWN) != 0) ||			// ����Alt+Tab				
			( p->vkCode == VK_ESCAPE && (p->flags & LLKHF_ALTDOWN) != 0) ||			// ����Alt+Esc
			( p->vkCode == VK_ESCAPE && (GetKeyState(VK_CONTROL) & 0x8000) != 0 ))	// ����Ctrl+Esc
			return TRUE ;
	}

	return CallNextHookEx ( hLLKeyboard, nCode, wParam, lParam ) ;
}

void CLLKeyboardDlg::OnBnClickedInstall()
{
	hLLKeyboard = SetWindowsHookEx ( WH_KEYBOARD_LL, \
		LowLevelKeyboardProc, GetModuleHandle(NULL), 0 ) ;
	if ( hLLKeyboard == NULL )
	{
		this->MessageBox ( L"��װ����ʧ�ܣ�" ) ;
	}
}

void CLLKeyboardDlg::OnBnClickedUninstall()
{
	if  ( hLLKeyboard )
		UnhookWindowsHookEx ( hLLKeyboard ) ;
}
