// QDING_KYODAIDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "QDING_KYODAI.h"
#include "QDING_KYODAIDlg.h"
#include "GameProc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern CHESS_INFO g_ChessInfo;
extern byte chessdata[11][19];


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


// CQDING_KYODAIDlg 对话框
CQDING_KYODAIDlg::CQDING_KYODAIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CQDING_KYODAIDlg::IDD, pParent)
	, m_data(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CQDING_KYODAIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//DDX_Text(pDX, IDC_X_EDIT, m_x);
	//DDX_Text(pDX, IDC_Y_EDIT, m_y);
	DDX_Text(pDX, IDC_DATA_EDIT, m_data);
}

BEGIN_MESSAGE_MAP(CQDING_KYODAIDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, &CQDING_KYODAIDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_PRAC_BUTTON, &CQDING_KYODAIDlg::OnBnClickedPracButton)
	ON_BN_CLICKED(IDC_FIRST_BUTTON, &CQDING_KYODAIDlg::OnBnClickedFirstButton)
	ON_BN_CLICKED(IDC_EXEC, &CQDING_KYODAIDlg::OnBnClickedExec)
END_MESSAGE_MAP()


// CQDING_KYODAIDlg 消息处理程序

BOOL CQDING_KYODAIDlg::OnInitDialog()
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

void CQDING_KYODAIDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CQDING_KYODAIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
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

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CQDING_KYODAIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void 
CQDING_KYODAIDlg::OnBnClickedOk()
/*
开始按钮的点击事件: 
*/
{
	//单机版 开始按钮
	//测试按钮 x = 735;
	//测试按钮 y = 534;

	//网络版本 开始按钮
	//开始按钮 x = 660; 
	//开始按钮 y = 565;
	InitNetChessInfo (660, 565);

	startGame ();
}

void 
CQDING_KYODAIDlg::OnBnClickedPracButton()
/*
练习按钮点击事件：针对连连看练习窗口。
*/
{
	//单机版 练习按钮
	//测试按钮 x = 663;
	//测试按钮 y = 524;

	//网络版本 练习按钮
	//测试按钮 x = 740;   
	//测试按钮 y = 565;  
	InitNetChessInfo (740, 565); 
	
	startGame ();
}

void CQDING_KYODAIDlg::OnBnClickedExec()
{
	int chessnum = ReadChessNum();
	while (chessnum != 0)
	{
		ClearPiar();
		chessnum = ReadChessNum();
	}
}

void 
CQDING_KYODAIDlg::OnBnClickedFirstButton()
/*
Scan按钮事件：测试顺序点击每个棋子
*/
{
	//手工粗略每个棋子图标的大小
	//x = 31, y = 35

	//第一个棋子的坐标位置
	//x = 30, y = 200

	//g_data.addr = 0x0012BB94; //xp虚拟机单机版
	//g_data.addr = 0x0018BB58; //win7 单机版
	//g_data.addr = 0x0012A480; //xp虚拟机网络版
	
	if (!EnablePrivilege ())
	{
		MyMessageBox ("EnablePrivilege", GetLastError ());
	}

	startGame ();

	HWND gameh =::FindWindowA(NULL,g_ChessInfo.caption);
	
	//获取窗口进程ID
	DWORD processid;
	::GetWindowThreadProcessId(gameh,&processid);
	
	//打开指定进程
	HANDLE processH=::OpenProcess(PROCESS_ALL_ACCESS,false,processid);

	DWORD dwOldProtect;
	if(!VirtualProtectEx (processH , g_ChessInfo.lpQPBase, sizeof (chessdata), PAGE_READWRITE, &dwOldProtect)){
		MyMessageBox ("VirtualProtectEx", GetLastError ());
	}

	//读指定进程 内存数据
    DWORD byread;
	LPVOID  nbuffer=(LPVOID)&chessdata;    //存放棋盘数据
	::ReadProcessMemory(processH,g_ChessInfo.lpQPBase,nbuffer,11*19,&byread);


	DWORD dwNewProtect;
	if(!VirtualProtectEx (processH, g_ChessInfo.lpQPBase, sizeof (chessdata), dwOldProtect, &dwNewProtect)){
		MyMessageBox ("VirtualProtectEx2", GetLastError ());
	}

	m_data = "";
	UpdateData(false); 

	//显示数据
	int y = 0;
	char ch[4];
	for (y; y < 11; y++)
	{
		int x = 0;
		for (x; x < 19; x++)
		{
			memset (ch, 0, 4);
			itoa (chessdata[y][x], ch, 16);
			m_data += ch;
			m_data += "  ";
		}

		m_data += "\r\n";
	}

	UpdateData(false); 

	//
	//点击棋盘的所有棋子
	//
	//int x1,y1,x2,y2;
	//for (y1=0; y1<11; y1++){
	//	for (x1=0; x1<19; x1++)
	//	{
	//		int lparam;
	//		POINT p;
	//		p.x = x1;
	//		p.y = y1;
	//		lparam = ((p.y*g_ChessInfo.height + g_ChessInfo.y_offset) << 16) + (p.x*g_ChessInfo.width + g_ChessInfo.x_offset);

	//		::SendMessage(gameh,WM_LBUTTONDOWN,0,lparam);//
	//		::SendMessage(gameh,WM_LBUTTONUP,0,lparam);//
	//		Sleep (500);
	//	}
	//}

	//UpdateData(false); 
}
