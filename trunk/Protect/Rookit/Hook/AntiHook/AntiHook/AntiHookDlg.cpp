// AntiHookDlg.cpp : 实现文件
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

	// 这里可以连续多次调用DetourAttach，表明Hook多个函数
	DetourAttach(&(PVOID&)OLD_LoadLibraryExW, NEW_LoadLibraryExW ) ;

	DetourTransactionCommit();
}

VOID UnHook ()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// 这里可以连续多次调用DetourDetach，表明撤消多个函数Hook
	DetourDetach(&(PVOID&)OLD_LoadLibraryExW, NEW_LoadLibraryExW ) ;

	DetourTransactionCommit();
}
/////////////////////////////////////////////////////////

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


// CAntiHookDlg 对话框




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


// CAntiHookDlg 消息处理程序

BOOL CAntiHookDlg::OnInitDialog()
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
	this->bStatus = FALSE ;

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CAntiHookDlg::OnPaint()
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
HCURSOR CAntiHookDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CAntiHookDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: 在此处添加消息处理程序代码
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
