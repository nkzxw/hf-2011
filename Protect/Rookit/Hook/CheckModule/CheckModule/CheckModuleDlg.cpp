// CheckModuleDlg.cpp : 实现文件
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

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
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


// CCheckModuleDlg 对话框




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


// CCheckModuleDlg 消息处理程序

BOOL CCheckModuleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CCheckModuleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
//
HCURSOR CCheckModuleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// 取得当前模块信息
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

// 检测指定模块在存在于起始状态
BOOL CCheckModuleDlg::IsModuleValid( CString szModuleName )
{
	// 遍历起始状态的模块列表
	for ( int i = 0; i < this->ModuleVect.size(); i++ )
	{
		if ( this->ModuleVect[i] == szModuleName )
			return TRUE ;
	}
	return FALSE ;
}

// “统计”功能
void CCheckModuleDlg::OnBnClickedFlush()
{
	this->InitModuleVect () ;	
	MY_OutputDebugStringW ( L"当前模块数量:%d", this->ModuleVect.size() ) ;
}

// “检测”功能
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
				MY_OutputDebugStringW ( L"[可疑模块]%s", me32.szExePath ) ;
				dwCount ++ ;
			}
		}while ( Module32Next ( hModuleSnap, &me32 ) ) ;
	}

	CloseHandle ( hModuleSnap ) ;
	MY_OutputDebugStringW ( L"可疑模块数量:%d", dwCount ) ;
}
