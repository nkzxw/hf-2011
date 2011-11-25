// PhoenixFWDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PhoenixFW.h"
#include "PhoenixFWDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About


extern CPhoenixApp theApp;

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainDlg dialog

CMainDlg::CMainDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMainDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMainDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMainDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMainDlg, CDialog)
	//{{AFX_MSG_MAP(CMainDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(PM_QUERY_ACL_NOTIFY, OnQueryAcl)
	ON_MESSAGE(PM_SESSION_NOTIFY, OnSessionNotify)
	ON_BN_CLICKED(IDC_APPLY, OnApply)
	ON_BN_CLICKED(IDC_ANNUL, OnAnnul)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainDlg message handlers

BOOL CMainDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
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

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here


	// ���ù�������
	TCHAR sz[256];
	::GetModuleFileName(NULL, sz, 256);
	g_pIoControl->SetPhoenixInstance(m_hWnd, sz);

	// ������ҳ�м����ǩ����ǩ���ɸ����ӶԻ���ı���������
	m_sheet.AddPage(&m_MonitorPage);
	m_sheet.AddPage(&m_RulePage);
	m_sheet.AddPage(&m_KerRulePage);
	m_sheet.AddPage(&m_SyssetPage);

	// ��Create������һ������ҳ
	m_sheet.Create(this, WS_CHILD | WS_VISIBLE, WS_EX_CONTROLPARENT);
		
	// ��������ҳ�Ĵ�С��λ��
	RECT rect;
	GetWindowRect(&rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;
	m_sheet.SetWindowPos(NULL, 
		0, 0, width - 4, height - 25, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

	// ��Ч��Ӧ�á���ť
	GetDlgItem(IDC_APPLY)->EnableWindow(FALSE);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMainDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMainDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMainDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


long CMainDlg::OnQueryAcl(WPARAM wParam, LPARAM lParam)		// ����PM_QUERY_ACL_NOTIFY��Ϣ
{
	// ��ȡѯ�ʵ�Ӧ�ó�������
	LPCTSTR lpszApp = g_pIoControl->GetQueryApp(wParam);

	if(CRulePage::AddQueryRule(lpszApp))
		g_pIoControl->SetQueryApp(wParam, 1); // ����
	else 
		g_pIoControl->SetQueryApp(wParam, 0); // �ܾ�

	return 0; 
}

long CMainDlg::OnSessionNotify(WPARAM wParam, LPARAM lParam)	// ����PM_SESSION_NOTIFY��Ϣ
{
	SESSION session;
	// ��ȡDLL��������Session
	g_pIoControl->GetSession(&session, wParam);
	// ��Session���ݸ�CMonitorPage��
	CMonitorPage::HandleNotifySession(&session, lParam);
	return 0;
}

void CMainDlg::OnApply()		// �û������Ӧ�á���ť
{
	if(!GetDlgItem(IDC_APPLY)->IsWindowEnabled())
		return;

	// ���û������ñ��浽�ļ�
	if(!g_RuleFile.SaveRules())
	{
		MessageBox(L"����������");
		return;
	}
	// ���ļ��е�����Ӧ�õ�DLLģ���SYSģ��
	if(!theApp.ApplyFileData())
		return;

	// ���ÿ����Զ�����
	theApp.SetAutoStart(g_RuleFile.m_header.bAutoStart);

	// ��Ч����ť
	GetDlgItem(IDC_APPLY)->EnableWindow(FALSE);
}

void CMainDlg::OnAnnul()	// �û������ȡ������ť
{	
	if(GetDlgItem(IDC_APPLY)->IsWindowEnabled())
	{
		if(AfxMessageBox(L"Ҫ�����������޸���", MB_YESNO) == IDYES)
		{
			OnApply();
		}
	}
	// ��С������
	SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
}

void CMainDlg::OnOK()		// �û������ȷ������ť
{
	// Ӧ���û�����
	OnApply(); 
	// ��С������
	SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
}

void CMainDlg::OnCancel() 	// �û��رնԻ���
{
	if(GetDlgItem(IDC_APPLY)->IsWindowEnabled())
	{
		if(AfxMessageBox(L"Ҫ�����������޸���", MB_YESNO) == IDYES)
		{
			OnApply();
		}
	}

	CDialog::OnCancel();
}