// GalaxyAPDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GalaxyAP.h"
#include "GalaxyAPDlg.h"
#include "commhdr.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About
CGalaxyAPDlg	*g_pthis;
bool					g_bWorking=false;
void	setlog(char *pLog)
{
		CString   strDate; 
		CTime   ttime   =   CTime::GetCurrentTime(); 
		strDate.Format( "%d:%d:%d ",ttime.GetHour(),ttime.GetMinute(),ttime.GetSecond()); 
		CString cstmp;
		g_pthis->m_richLOG.GetWindowText(cstmp);
		if (cstmp.GetLength()>1024*10)
		{
				cstmp	=	"";
		}
		cstmp	=	cstmp+strDate;
		cstmp	=	cstmp+pLog;
		
		g_pthis->m_richLOG.SetWindowText(cstmp);
		g_pthis->m_richLOG.SendMessage(WM_VSCROLL, MAKEWPARAM(SB_BOTTOM,0),0); 
}
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
// CGalaxyAPDlg dialog

CGalaxyAPDlg::CGalaxyAPDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGalaxyAPDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGalaxyAPDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	g_pthis	=	this;
}

void CGalaxyAPDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGalaxyAPDlg)
	DDX_Control(pDX, IDC_EDIT_0x, m_0x);
	DDX_Control(pDX, IDC_EDIT_STOD, m_strongOd);
	DDX_Control(pDX, IDC_CHECK1_PID, m_chkPID);
	DDX_Control(pDX, IDC_EDIT_PID, m_edtPID);
	DDX_Control(pDX, IDC_RICHEDIT1, m_richLOG);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGalaxyAPDlg, CDialog)
	//{{AFX_MSG_MAP(CGalaxyAPDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_ReOsAndKidisp, OnBUTTONReOsAndKidisp)
	ON_BN_CLICKED(IDC_BUTTON_cleanLog, OnBUTTONcleanLog)
	ON_BN_CLICKED(IDC_BUTTON_kidisp, OnBUTTONkidisp)
	ON_BN_CLICKED(IDC_BUTTON_unloadKidisp, OnBUTTONunloadKidisp)
	ON_BN_CLICKED(IDC_BUTTON_reloadOS, OnBUTTONreloadOS)
	ON_BN_CLICKED(IDC_BUTTON_UnHookPort, OnBUTTONUnHookPort)
	ON_BN_CLICKED(IDC_BUTTON_PROINFO, OnButtonProinfo)
	ON_BN_CLICKED(IDC_BUTTON_UnloadALl, OnBUTTONUnloadALl)
	ON_BN_CLICKED(IDC_BUTTON1, OnButton_patchSOD)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGalaxyAPDlg message handlers

BOOL CGalaxyAPDlg::OnInitDialog()
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
	m_chkPID.SetCheck(1);
	m_strongOd.SetWindowText("fengyue0.sys");
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGalaxyAPDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CGalaxyAPDlg::OnPaint() 
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
HCURSOR CGalaxyAPDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}
//////////////////////////////////////////////////////////////////////////

DWORD WINAPI	ThreadSetupMyExcAndReloadOS(PVOID	par)
{
		g_bWorking	=	true;
				CGalaxyAPDlg *pthis	=	(CGalaxyAPDlg*)par;
	if (pthis->m_GalaxyApMGR.Initialize())
	{

	
		if(!pthis->m_GalaxyApMGR.SetupReloadOS("GalaxyAPhookPort"))
		{
				myprint("m_GalaxyApMGR.SetupReloadOS 安装失败\r\n");
				g_bWorking	=	false;
				return 1;
		}
	//	myprint("安装m_GalaxyApMGR.SetupReloadOS 成功\r\n");


		if(!pthis->m_GalaxyApMGR.SetupMyKidispatchexcepion("GalaxyAPkidisp"))
		{
				myprint("m_GalaxyApMGR.SetupMyKidispatchexcepion安装失败\r\n");
				g_bWorking	=	false;
				return 1;
		}

		myprint("安装 ReloadOS +  MyKidispatchexcepion 成功\r\n");

	}
	else
	{
		myprint("m_GalaxyApMGR.Initialize()失败\r\n");
	}
	g_bWorking	=	false;
	return 1;

		
}
void CGalaxyAPDlg::OnBUTTONReOsAndKidisp() 
{
	// TODO: Add your control notification handler code here
	DWORD	dwThread;
	if (g_bWorking)
	{
		MessageBox("working");return ;
	}
	CString cs;
	m_strongOd.GetWindowText(cs);
	ZeroMemory(m_GalaxyApMGR.szStrongOD,sizeof(m_GalaxyApMGR.szStrongOD));
	strcpy(m_GalaxyApMGR.szStrongOD, cs.GetBuffer(0));

		::CreateThread(NULL, NULL, ThreadSetupMyExcAndReloadOS, this, NULL, &dwThread);
		setlog("正在启动\r\n");
		::MessageBox(NULL, "启动中..",NULL,NULL);
}

void CGalaxyAPDlg::OnBUTTONcleanLog() 
{
	// TODO: Add your control notification handler code here
	m_richLOG.SetWindowText("");
}
DWORD WINAPI	ThreadSetupMyKidispatchexcepion(PVOID	par)
{
	g_bWorking	=1;
		CGalaxyAPDlg *pthis	=	(CGalaxyAPDlg*)par;
		if (pthis->m_GalaxyApMGR.Initialize())
	{
		if(!pthis->m_GalaxyApMGR.SetupMyKidispatchexcepion("GalaxyAPkidisp"))
		{
				myprint("m_GalaxyApMGR.SetupMyKidispatchexcepion安装失败\r\n");
				g_bWorking	=	false;
				return 1;
		}
		myprint("安装m_GalaxyApMGR.SetupMyKidispatchexcepion 成功\r\n");
	}
	else
	{
		myprint("m_GalaxyApMGR.Initialize()失败\r\n");
	}
	g_bWorking	=	false;
	return 1;
}
void CGalaxyAPDlg::OnBUTTONkidisp() 
{
	// TODO: Add your control notification handler code here
	DWORD	dwThread;
	if (g_bWorking)
	{
		MessageBox("working");return ;
	}
	::CreateThread(NULL, NULL, ThreadSetupMyKidispatchexcepion, this, NULL, &dwThread);
	setlog("正在启动\r\n");
	::MessageBox(NULL, "启动中..",NULL,NULL);
	
}

void CGalaxyAPDlg::OnBUTTONunloadKidisp() 
{
	// TODO: Add your control notification handler code here
	AfxMessageBox("卸载前先关闭OD=调试器");
	if (g_bWorking)
	{
		MessageBox("working");return ;
	}
	if(m_GalaxyApMGR.unSetupMyKidispatchexcepion())
	{
					myprint("成功卸载%s\r\n", m_GalaxyApMGR.szExceptionDriverName);
	}
	else
	myprint("卸载%s失败\r\n", m_GalaxyApMGR.szExceptionDriverName);
}
DWORD	WINAPI	ThreadSetupReloadOs(PVOID par)
{
g_bWorking	=	true;
		CGalaxyAPDlg *pthis	=	(CGalaxyAPDlg*)par;

		if(!pthis->m_GalaxyApMGR.SetupReloadOS("GalaxyAPhookPort"))
		{
				myprint("m_GalaxyApMGR.SetupReloadOS 安装失败\r\n");
				g_bWorking	=	false;
				return 1;
		}
		myprint("安装m_GalaxyApMGR.SetupReloadOS 成功\r\n");
	g_bWorking	=	false;
	return 1;
}
void CGalaxyAPDlg::OnBUTTONreloadOS() 
{
	// TODO: Add your control notification handler code here
		DWORD	dwThread;
	if (g_bWorking)
	{
		MessageBox("working");return ;
	}
	::CreateThread(NULL, NULL, ThreadSetupReloadOs, this, NULL, &dwThread);
	setlog("正在启动\r\n");
	::MessageBox(NULL, "启动中..",NULL,NULL);


}

void CGalaxyAPDlg::OnBUTTONUnHookPort() 
{
	// TODO: Add your control notification handler code here
	if (g_bWorking)
	{
		MessageBox("working");return ;
	}
	if (m_GalaxyApMGR.UnSetupReloadOS())
	{
		myprint("成功卸载ReloadOS\r\n");
	}
	else
	{
					myprint("卸载ReloadOS 失败\r\n");
	}

}

void CGalaxyAPDlg::OnButtonProinfo() 
{
	// TODO: Add your control notification handler code here
	

		ZeroMemory(&m_GalaxyApMGR.m_protect_info, sizeof(m_GalaxyApMGR.m_protect_info));
		CString	cstmp;
		m_edtPID.GetWindowText(cstmp);
		if (cstmp.GetLength()==0)
		{
			AfxMessageBox("无目标信息");
			return ;
		}
		if (m_chkPID.GetCheck())
		{
					m_GalaxyApMGR.m_protect_info.PID	=	atoi(cstmp.GetBuffer(0));
					cstmp.Format("%X", m_GalaxyApMGR.m_protect_info.PID );
					m_0x.SetWindowText(cstmp);
					m_GalaxyApMGR.m_protect_info.PidOrName	=	1;
					if (m_GalaxyApMGR.SetProtectInfo())
					{
						AfxMessageBox("设置成功\r\n");
						return ;
					}
					else
					{
						AfxMessageBox("设置失败\r\n");
					}
		}
		else
		{

					m_GalaxyApMGR.m_protect_info.PidOrName	=	0;

					strcpy(m_GalaxyApMGR.m_protect_info.szNameBuffer, cstmp.GetBuffer(0));

					if (m_GalaxyApMGR.SetProtectInfo())
					{
						AfxMessageBox("设置成功\r\n");
						return ;
					}
					else
					{
						AfxMessageBox("设置失败\r\n");
					}
		}





}

void CGalaxyAPDlg::OnBUTTONUnloadALl() 
{
	// TODO: Add your control notification handler code here
	AfxMessageBox("卸载前先关闭OD=调试器");
	if (g_bWorking)
	{
		MessageBox("working");return ;
	}
	if(m_GalaxyApMGR.unSetupMyKidispatchexcepion())
	{
					myprint("成功卸载%s\r\n", m_GalaxyApMGR.szExceptionDriverName);
	}
	else
	{


	myprint("卸载%s失败\r\n", m_GalaxyApMGR.szExceptionDriverName);
	}
	if (g_bWorking)
	{
		MessageBox("working");return ;
	}
	if (m_GalaxyApMGR.UnSetupReloadOS())
	{
		myprint("成功卸载ReloadOS\r\n");
	}
	else
	{
					myprint("卸载ReloadOS 失败\r\n");
	}

}

void CGalaxyAPDlg::OnButton_patchSOD() 
{
	// TODO: Add your control notification handler code here

	if (m_GalaxyApMGR.SendReplace())
	{
		myprint("成功\r\n");
	}
	else
	{
		myprint("失败\r\n");
		AfxMessageBox("patch 失败");
	}

}
