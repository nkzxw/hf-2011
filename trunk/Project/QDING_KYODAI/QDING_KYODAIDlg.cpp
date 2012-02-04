// QDING_KYODAIDlg.cpp : ʵ���ļ�
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


// CQDING_KYODAIDlg �Ի���
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


// CQDING_KYODAIDlg ��Ϣ�������

BOOL CQDING_KYODAIDlg::OnInitDialog()
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CQDING_KYODAIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
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

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CQDING_KYODAIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void 
CQDING_KYODAIDlg::OnBnClickedOk()
/*
��ʼ��ť�ĵ���¼�: 
*/
{
	//������ ��ʼ��ť
	//���԰�ť x = 735;
	//���԰�ť y = 534;

	//����汾 ��ʼ��ť
	//��ʼ��ť x = 660; 
	//��ʼ��ť y = 565;
	InitNetChessInfo (660, 565);

	startGame ();
}

void 
CQDING_KYODAIDlg::OnBnClickedPracButton()
/*
��ϰ��ť����¼��������������ϰ���ڡ�
*/
{
	//������ ��ϰ��ť
	//���԰�ť x = 663;
	//���԰�ť y = 524;

	//����汾 ��ϰ��ť
	//���԰�ť x = 740;   
	//���԰�ť y = 565;  
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
Scan��ť�¼�������˳����ÿ������
*/
{
	//�ֹ�����ÿ������ͼ��Ĵ�С
	//x = 31, y = 35

	//��һ�����ӵ�����λ��
	//x = 30, y = 200

	//g_data.addr = 0x0012BB94; //xp�����������
	//g_data.addr = 0x0018BB58; //win7 ������
	//g_data.addr = 0x0012A480; //xp����������
	
	if (!EnablePrivilege ())
	{
		MyMessageBox ("EnablePrivilege", GetLastError ());
	}

	startGame ();

	HWND gameh =::FindWindowA(NULL,g_ChessInfo.caption);
	
	//��ȡ���ڽ���ID
	DWORD processid;
	::GetWindowThreadProcessId(gameh,&processid);
	
	//��ָ������
	HANDLE processH=::OpenProcess(PROCESS_ALL_ACCESS,false,processid);

	DWORD dwOldProtect;
	if(!VirtualProtectEx (processH , g_ChessInfo.lpQPBase, sizeof (chessdata), PAGE_READWRITE, &dwOldProtect)){
		MyMessageBox ("VirtualProtectEx", GetLastError ());
	}

	//��ָ������ �ڴ�����
    DWORD byread;
	LPVOID  nbuffer=(LPVOID)&chessdata;    //�����������
	::ReadProcessMemory(processH,g_ChessInfo.lpQPBase,nbuffer,11*19,&byread);


	DWORD dwNewProtect;
	if(!VirtualProtectEx (processH, g_ChessInfo.lpQPBase, sizeof (chessdata), dwOldProtect, &dwNewProtect)){
		MyMessageBox ("VirtualProtectEx2", GetLastError ());
	}

	m_data = "";
	UpdateData(false); 

	//��ʾ����
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
	//������̵���������
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
