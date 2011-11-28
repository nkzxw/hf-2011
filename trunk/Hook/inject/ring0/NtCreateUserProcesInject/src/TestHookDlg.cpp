// TestHookDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TestHook.h"
#include "TestHookDlg.h"
#include "Atlconv.h"
#include "HookLib.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

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
// CTestHookDlg dialog

CTestHookDlg::CTestHookDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestHookDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTestHookDlg)
	m_AutoDll = _T("");
	m_InjectDll = _T("");
	m_InjectPid = 0;
	m_AutoStart = FALSE;
	m_Inject = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestHookDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTestHookDlg)
	DDX_Text(pDX, IDC_EDT_DLL_1, m_AutoDll);
	DDX_Text(pDX, IDC_EDT_DLL_2, m_InjectDll);
	DDX_Text(pDX, IDC_EDT_PID, m_InjectPid);
	DDX_Radio(pDX, IDC_R_INJECT, m_Inject);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTestHookDlg, CDialog)
	//{{AFX_MSG_MAP(CTestHookDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_SETUP, OnBtnSetup)
	ON_BN_CLICKED(IDC_BTN_SELECT1, OnBtnSelect1)
	ON_BN_CLICKED(IDC_BTN_AUTO_START, OnBtnAutoStart)
	ON_BN_CLICKED(IDC_BTN_SELECT2, OnBtnSelect2)
	ON_BN_CLICKED(IDC_BTN_INJECT, OnBtnInject)
	ON_BN_CLICKED(IDC_BTN_STARTSVC, OnBtnStartsvc)
	ON_BN_CLICKED(IDC_BTN_ABOUT, OnBtnAbout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTestHookDlg message handlers

BOOL CTestHookDlg::OnInitDialog()
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

	CHAR t_Tmp[513], *t_Find;
	GetModuleFileName(NULL, t_Tmp, 512);

	t_Find = strrchr(t_Tmp, '\\');
	if(t_Find++)
	{
		*t_Find = 0;
	}
	m_AppPath = t_Tmp;
	
	m_AutoDll = (m_AppPath + "Test.dll").c_str();
	m_InjectDll = (m_AppPath + "Test.dll").c_str();
	
	UpdateStatus();
	UpdateData(FALSE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CTestHookDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CTestHookDlg::OnPaint() 
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
HCURSOR CTestHookDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CTestHookDlg::OnOK() 
{	
	//CDialog::OnOK();
}

void CTestHookDlg::OnCancel() 
{	
	CDialog::OnCancel();
}


void CTestHookDlg::RunExe(LPCTSTR command, int m_Show, DWORD m_Wait)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = m_Show;
	
	ZeroMemory( &pi, sizeof(pi) );
	
	// Start the child process. 
	if(CreateProcess( NULL, // No module name (use command line). 
		(LPSTR)(LPCTSTR)command,		// Command line. 
		NULL,             // Process handle not inheritable. 
		NULL,             // Thread handle not inheritable. 
		FALSE,            // Set handle inheritance to FALSE. 
		0,                // No creation flags. 
		NULL,             // Use parent's environment block. 
		NULL,             // Use parent's starting directory. 
		&si,              // Pointer to STARTUPINFO structure.
		&pi )             // Pointer to PROCESS_INFORMATION structure.
		) 
	{
		if((m_Wait & WAIT_IDLE) == WAIT_IDLE)
		{
			WaitForInputIdle(pi.hProcess,INFINITE);
		}
		else if((m_Wait & WAIT_TIME) == WAIT_TIME)
		{			
			m_Wait &=~WAIT_TIME;
			WaitForSingleObject(pi.hProcess,m_Wait);
		}
		else if((m_Wait & WAIT_INFINITE) == WAIT_INFINITE)
		{
			WaitForSingleObject(pi.hProcess,INFINITE);
		}
		
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
	}
}

BOOL CTestHookDlg::GetSelectedFile(CString &m_Path,CString m_InitPath,BOOL IsOpenFile,CString FileName)
{
	CFileDialog file(IsOpenFile, NULL,FileName,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,"所有文件(*.*)|*.*||");
	//CFileDialog file(IsOpenFile);
	
	//file.m_ofn.lpstrTitle="添加文件";
	file.m_ofn.lpstrInitialDir=m_InitPath;
	if(file.DoModal()==IDOK)
	{
		m_Path=file.GetPathName();
		return TRUE;
	}
	return FALSE;
}

void CTestHookDlg::OnBtnSetup() 
{
	std::string t_CfgName;
	
	t_CfgName = "\""+ m_AppPath + "instdrv.exe\" -i HookApp \"" + m_AppPath + "HookApp.sys\"";
	RunExe(t_CfgName.c_str(), SW_HIDE, WAIT_INFINITE);
}

void CTestHookDlg::OnBtnStartsvc() 
{
	RunExe("net.exe start HookApp", SW_HIDE, WAIT_NONE);
	MessageBox("服务启动完毕！", "提示", MB_OK | MB_ICONWARNING);
}

void CTestHookDlg::OnBtnSelect1() 
{
	UpdateData();

	GetSelectedFile(m_AutoDll, m_AppPath.c_str(), TRUE, "Test.dll");
	
	UpdateData(FALSE);
}

void CTestHookDlg::OnBtnAutoStart() 
{
	UpdateData();
	
	std::string t_Tip1, t_Tip2;
	BOOLEAN t_Start	 = !m_AutoStart;
	BOOL	m_Result = FALSE;

	if(t_Start)
	{
		t_Tip1 = "启动监视";
		if(m_AutoDll.IsEmpty())
		{
			MessageBox("请选择要自动注入的DLL模块！", "提示", MB_OK | MB_ICONWARNING);
			return;
		}

		HANDLE hDevice = OpenDevice();
		if(hDevice == NULL)
		{
			MessageBox((t_Tip1 + t_Tip2).c_str(), "提示", MB_OK|MB_ICONWARNING);
			return;
		}

		USES_CONVERSION;
		std::wstring t_CfgName = A2W((LPCSTR)m_AutoDll);

		m_Result = StartHook(hDevice, (PWCHAR)t_CfgName.c_str());
		CloseDevice(hDevice);
	}
	else
	{
		t_Tip1 = "停止监视";
		HANDLE hDevice = OpenDevice();
		if(hDevice == NULL)
		{
			MessageBox((t_Tip1 + t_Tip2).c_str(), "提示", MB_OK|MB_ICONWARNING);
			return;
		}
		m_Result = StopHook(hDevice);
		CloseDevice(hDevice);
	}
	t_Tip2 = "失败,请确认服务已经安装且启动！";

	if(!m_Result)
	{
		MessageBox((t_Tip1 + t_Tip2).c_str(), "提示", MB_OK|MB_ICONWARNING);
	}
	else
	{
		m_AutoStart = t_Start;
		UpdateStatus();	
		MessageBox((t_Tip1 + "成功.").c_str(), "提示", MB_OK|MB_ICONWARNING);
	}
}

void CTestHookDlg::OnBtnSelect2() 
{
	UpdateData();
	
	GetSelectedFile(m_InjectDll, m_AppPath.c_str(), TRUE, "Test.dll");
	
	UpdateData(FALSE);	
}

void CTestHookDlg::OnBtnInject() 
{
	UpdateData();
	if(m_InjectPid == 0)
	{
		MessageBox("请输入要注入进程的PID！", "提示", MB_OK | MB_ICONWARNING);
		return;
	}

	if(m_InjectDll.IsEmpty())
	{
		MessageBox("请选择要注入指定进程的DLL模块！", "提示", MB_OK | MB_ICONWARNING);
		return;
	}

	USES_CONVERSION;

	std::string t_Tip1, t_Tip2;	
	std::wstring t_CfgName = A2W((LPCSTR)m_InjectDll);

	if(m_Inject == 0)
	{
		t_Tip1 = "注入DLL模块";
	}
	else
	{
		t_Tip1 = "卸载DLL模块";
	}	
	t_Tip2 = "失败,请确认服务已经安装且启动！";

	HANDLE hDevice = OpenDevice();
	if(hDevice == NULL)
	{
		MessageBox((t_Tip1 + t_Tip2).c_str(), "提示", MB_OK|MB_ICONWARNING);
		return;
	}
	
	BOOL m_Result = InjectProcessByID(
		hDevice, 
		m_InjectPid, 
		(PWCHAR)t_CfgName.c_str(), 
		(m_Inject == 0 ? HOOK_LOAD : HOOK_FREE)
		);
	CloseDevice(hDevice);

	if(!m_Result)
	{
		MessageBox((t_Tip1 + t_Tip2).c_str(), "提示", MB_OK|MB_ICONWARNING);
	}
	else
	{
		MessageBox((t_Tip1 + "成功.").c_str(), "提示", MB_OK|MB_ICONWARNING);
	}
}


void CTestHookDlg::UpdateStatus()
{
	if(m_AutoStart)
	{
		SetDlgItemText(IDC_BTN_AUTO_START, "停止监视");
	}
	else
	{
		SetDlgItemText(IDC_BTN_AUTO_START, "启动监视");
	}
}

void CTestHookDlg::OnBtnAbout() 
{
	CAboutDlg dlgAbout;
	dlgAbout.DoModal();	
}
