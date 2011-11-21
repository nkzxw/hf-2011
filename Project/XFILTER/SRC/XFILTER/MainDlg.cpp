// MainDlg.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "MainDlg.h"

#include "Splash.h"
#include "xfilterdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainDlg dialog

CSystemTray	m_TrayIcon;

CMainDlg::CMainDlg(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CMainDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMainDlg)
	//}}AFX_DATA_INIT

	m_bWorkMode = 255; //WORKMODE_PASSALL;
	m_bSelectedButton = 255;
	m_IsSplash = FALSE;
	m_MessageIndex = 0;
	m_bIsFirst = TRUE;
}

void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMainDlg)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMainDlg, CPasseckDialog)
	//{{AFX_MSG_MAP(CMainDlg)
	ON_BN_CLICKED(IDC_CLOSE, OnClose)
	ON_BN_CLICKED(IDC_MIN, OnMin)
	ON_BN_CLICKED(IDC_RED, OnRed)
	ON_BN_CLICKED(IDC_YELLOW, OnYellow)
	ON_BN_CLICKED(IDC_GREEN, OnGreen)
	ON_BN_CLICKED(IDC_MONITOR, OnMonitor)
	ON_BN_CLICKED(IDC_LOG, OnLog)
	ON_BN_CLICKED(IDC_ONLINE, OnOnline)
	ON_BN_CLICKED(IDC_SYSTEMSET, OnSystemset)
	ON_BN_CLICKED(IDHELP, OnHelp)
	ON_BN_CLICKED(IDC_ACL, OnAcl)
	ON_BN_CLICKED(IDC_ABOUT, OnAbout)
	ON_BN_CLICKED(IDC_PARAMETER, OnParameter)
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED(IDC_TOPMOST, OnTopmost)
	ON_WM_CREATE()
	ON_COMMAND(ID_APP_EXIT, OnAppExit)
	ON_COMMAND(ID_CONTROL_FRAME, OnControlFrame)
	ON_COMMAND(ID_DENY_ALL, OnDenyAll)
	ON_COMMAND(ID_FILTER, OnFilter)
	ON_COMMAND(ID_PASS_ALL, OnPassAll)
	ON_BN_CLICKED(IDC_PASSECK, OnPasseck)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_ICON_NOTIFY, OnTrayNotification)
	ON_MESSAGE(WM_ICON_SPLASH, OnSplashIcon)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainDlg message handlers

BOOL CMainDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_BitmapBk.LoadBitmap(IDB_MAIN);

	::SetWindowLong(m_hWnd, GWL_STYLE, WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX);
	CRect rect;
	GetClientRect(&rect);
	rect.bottom -= 3;
	rect.right -= 2;
	MoveWindow(&rect);
	CenterWindow();
	SetMoveParent(FALSE);

	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE);
	SetWindowText(WINDOW_CAPTION);
	CreateCaption(WINDOW_CAPTION);

	SetButton(&m_Button[BUTTON_MONITOR], IDC_MONITOR);
	SetButton(&m_Button[BUTTON_LOG], IDC_LOG);
	SetButton(&m_Button[BUTTON_ACL], IDC_ACL);
	SetButton(&m_Button[BUTTON_SYSTEMSET], IDC_SYSTEMSET);
	SetButton(&m_Button[BUTTON_ONLINE], IDC_ONLINE);
	SetButton(&m_Button[BUTTON_HELP], IDHELP);
	SetButton(&m_Button[BUTTON_ABOUT], IDC_ABOUT);
	SetButton(&m_Button[BUTTON_PARAMETER], IDC_PARAMETER);
	
	m_ButtonClose.SubclassDlgItem(IDC_CLOSE, this);
   	m_ButtonClose.SetBitmaps(IDB_CLOSE);
	m_ButtonClose.SetToolTipText(BUTTON_CAPTION_HIDE);

	m_ButtonMin.SubclassDlgItem(IDC_MIN, this);
   	m_ButtonMin.SetBitmaps(IDB_MIN);
	m_ButtonMin.SetToolTipText(BUTTON_CAPTION_MIN);

	m_ButtonTopMost.SubclassDlgItem(IDC_TOPMOST, this);
   	m_ButtonTopMost.SetBitmaps(IDB_TOPMOST_NORMAL);
	m_ButtonTopMost.SetToolTipText(BUTTON_CAPTION_TOPMOST);
    
	m_ButtonPasseck.SubclassDlgItem(IDC_PASSECK, this);
   	m_ButtonPasseck.SetBitmaps(IDB_PASSECK_FOCUS, IDB_PASSECK_NORMAL);
	m_ButtonPasseck.DrawBorder(FALSE);
	m_ButtonPasseck.SetBtnCursor(IDC_HAND);

	m_ButtonLamp[WORKMODE_PASSALL].SetLamp(IDC_GREEN, IDB_GREEN_FOCUS, IDB_GREEN_NORMAL, IDB_GREEN_SELECT, BUTTON_CAPTION_GREEN, this);
	m_ButtonLamp[WORKMODE_FILTER].SetLamp(IDC_YELLOW, IDB_YELLOW_FOCUS, IDB_YELLOW_NORMAL, IDB_YELLOW_SELECT, BUTTON_CAPTION_YELLOW, this);
	m_ButtonLamp[WORKMODE_DENYALL].SetLamp(IDC_RED, IDB_RED_FOCUS, IDB_RED_NORMAL, IDB_RED_SELECT, BUTTON_CAPTION_RED, this);

	SetLampSelect(theApp.m_AclFile.GetHeader()->bWorkMode);
    
	m_SubParent.SubclassDlgItem(IDC_SUB_PARENT, this);
	m_ParameterSub.Create(IDD_PARAMETER_SUB, &m_SubParent);
	m_AclSub.Create(IDD_ACL_SUB, &m_SubParent);
	m_MonitorSub.Create(IDD_MONITOR_SUB, &m_SubParent);
	m_LogSub.Create(IDD_LOG_SUB, &m_SubParent);
	m_SystemSet.Create(IDD_SYSTEM_SET, &m_SubParent);
	m_OnLine.Create(IDD_ONLINE, &m_SubParent);
	m_About.Create(IDD_ABOUT, &m_SubParent);

	OnParameter();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CMainDlg::SetButton(CButtonST *Button, UINT nID, BOOL bIsSelect)
{ 
	Button->SubclassDlgItem(nID, this);
	Button->SetBitmaps(IDB_BUTTON);
	Button->SetInactiveFgColor(COLOR_MAIN_NORMAL);
	Button->SetHeightType(HEIGHTTYPE_3_4);
	Button->SetActiveFgColor(COLOR_MAIN_FOCUS);
	Button->SetSelect(bIsSelect);
	Button->SetSelectFgColor(COLOR_MAIN_SELECT);
}

void CMainDlg::SetLampSelect(BYTE bWorkMode)
{
	if(m_bWorkMode == bWorkMode) return;

	if(theApp.m_pDllIoControl == NULL)
	{
		AfxMessageBox(GUI_ACL_MESSAGE_FUNCTION_NOT_FOUNT);
		return;
	}

	XFILTER_IO_CONTROL ioControl;
	ioControl.Byte = bWorkMode;

	if(theApp.m_pDllIoControl(IO_CONTROL_SET_WORK_MODE, &ioControl) != XERR_SUCCESS)
	{
		AfxMessageBox(GUI_ACL_MESSAGE_SET_WORK_MODE_ERROR);
		return;
	}

	if(theApp.m_pDllIoControl(IO_CONTROL_SET_WORK_MODE, &ioControl) != XERR_SUCCESS)
	{
		AfxMessageBox(ERROR_STRING_FILE_SAVE_ERROR);
	}

	if(m_bWorkMode != 255)
	{
		m_ButtonLamp[m_bWorkMode].SetSelect(FALSE);
	}
	m_ButtonLamp[bWorkMode].SetSelect(TRUE);
	m_bWorkMode = bWorkMode;

	SetTrayIcon(bWorkMode);

	static BOOL bIsFirst = TRUE;
	if(bIsFirst)
	{
		bIsFirst = FALSE;
		return;
	}
	PXACL_HEADER pHeader = theApp.m_AclFile.GetHeader();
	DWORD nPosition = (DWORD)&pHeader->bWorkMode - (DWORD)pHeader;
	if(!theApp.m_AclFile.UpdateFile(nPosition, &m_bWorkMode, sizeof(m_bWorkMode)))
	{
		AfxMessageBox(GUI_ACL_MESSAGE_SET_WORK_MODE_ERROR);
		return;
	}
	pHeader->bWorkMode = m_bWorkMode;

}

void CMainDlg::SetSelectButton(BYTE bSelectButton)
{
	if(m_bSelectedButton == bSelectButton) return;
	if(m_bSelectedButton != 255)
	{
		m_Button[m_bSelectedButton].SetSelect(FALSE);
	}
	m_Button[bSelectButton].SetSelect(TRUE);

	m_bSelectedButton = bSelectButton;

	m_ParameterSub.ShowWindow(bSelectButton == BUTTON_PARAMETER ? SW_SHOW : SW_HIDE);
	m_MonitorSub.ShowWindow(bSelectButton == BUTTON_MONITOR ? SW_SHOW : SW_HIDE);
	m_LogSub.ShowWindow(bSelectButton == BUTTON_LOG ? SW_SHOW : SW_HIDE);
	m_AclSub.ShowWindow(bSelectButton == BUTTON_ACL ? SW_SHOW : SW_HIDE);
	m_SystemSet.ShowWindow(bSelectButton == BUTTON_SYSTEMSET ? SW_SHOW : SW_HIDE);
	m_OnLine.ShowWindow(bSelectButton == BUTTON_ONLINE ? SW_SHOW : SW_HIDE);
	m_About.ShowWindow(bSelectButton == BUTTON_ABOUT ? SW_SHOW : SW_HIDE);
}

void CMainDlg::OnClose() 
{
	if((m_AclSub.IsChange() || m_SystemSet.IsChangeEx())
		&& AfxMessageBox(GUI_ACL_MESSAGE_ACL_ASK_SAVE, MB_YESNO) == IDYES)
	{
		m_AclSub.Apply();
		m_SystemSet.Apply();
	}
	else
	{
		m_AclSub.Cancel();
		m_SystemSet.Cancel();
	}
	
	ShowWindow(SW_HIDE);
}

void CMainDlg::OnMin() 
{
	CloseWindow();
}

void CMainDlg::OnRed() 
{
	SetLampSelect(WORKMODE_DENYALL);
}

void CMainDlg::OnYellow() 
{
	SetLampSelect(WORKMODE_FILTER);
}

void CMainDlg::OnGreen() 
{
	SetLampSelect(WORKMODE_PASSALL);
}

void CMainDlg::OnMonitor() 
{
	SetSelectButton(BUTTON_MONITOR);
}

void CMainDlg::OnLog() 
{
	SetSelectButton(BUTTON_LOG);
}

void CMainDlg::OnOnline() 
{
	SetSelectButton(BUTTON_ONLINE);		
}

void CMainDlg::OnSystemset() 
{
	SetSelectButton(BUTTON_SYSTEMSET);		
}

void CMainDlg::OnHelp() 
{
	theApp.WinHelp(0, HH_HELP_CONTEXT );
}

void CMainDlg::OnAcl() 
{
	SetSelectButton(BUTTON_ACL);
}

void CMainDlg::OnAbout() 
{
	SetSelectButton(BUTTON_ABOUT);		
}

void CMainDlg::OnParameter() 
{
	SetSelectButton(BUTTON_PARAMETER);

	XFILTER_IO_CONTROL	IoControl;
	XF_IO_CONTROL		m_XfIoControl;
	HMODULE m_hTcpIpDog = NULL;
	TCHAR				sPathName[MAX_PATH];

	_stprintf(sPathName, _T("%s%s"), GetAppPath(), XFILTER_SERVICE_DLL_NAME);

	if ((m_hTcpIpDog = LoadLibrary(sPathName)) == NULL)
	{
		AfxMessageBox(GUI_ACL_MESSAGE_DLL_NOT_FOUND);
		return ;
	}

	m_XfIoControl	= (XF_IO_CONTROL)GetProcAddress(m_hTcpIpDog, _T("XfIoControl"));

	if (m_XfIoControl == NULL)
	{
		AfxMessageBox(GUI_ACL_MESSAGE_FUNCTION_NOT_FOUNT);
		return ;
	}

	IoControl.DWord = (DWORD)theApp.m_AclFile.GetMemoryFileHandle();
	m_XfIoControl(8, &IoControl);
}

BOOL CMainDlg::OnEraseBkgnd(CDC* pDC)
{
	//return CDialog::OnEraseBkgnd(pDC);

	if(m_bIsFirst)
	{
		m_memDC.CreateCompatibleDC(pDC);
		m_memDC.SelectObject(&m_BitmapBk);
		m_bIsFirst = FALSE;
	}

	CRect		rect;
	GetWindowRect(&rect);

	pDC->BitBlt(0, 0, rect.Width(), rect.Height(), &m_memDC, 0, 0, SRCCOPY);

	return TRUE;
}

void CMainDlg::OnOK() 
{
	OnClose();
}

void CMainDlg::OnCancel() 
{
	OnClose();
}

void CMainDlg::OnTopmost() 
{
	static BOOL IsTopMost = FALSE;
	IsTopMost = !IsTopMost;
	if(IsTopMost)
	{
		::SetWindowPos(m_hWnd,HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	   	m_ButtonTopMost.SetBitmaps(IDB_TOPMOST_SELECT);
	}
	else
	{
		::SetWindowPos(m_hWnd,HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	   	m_ButtonTopMost.SetBitmaps(IDB_TOPMOST_NORMAL);
	}
}

void CMainDlg::OnAppExit() 
{
	m_TrayIcon.HideIcon();
	EndDialog(IDCANCEL);
	//theApp.ExitInstance();
	theApp.m_pMainDlg.DestroyWindow();
}
void CMainDlg::OnControlFrame() 
{
	ShowWindow(TRUE);	
}

void CMainDlg::OnDenyAll() 
{
	SetLampSelect(ACL_DENY_ALL);
}
void CMainDlg::OnFilter() 
{
	SetLampSelect(ACL_QUERY);
}
void CMainDlg::OnPassAll() 
{
	SetLampSelect(ACL_PASS_ALL);
}

int CMainDlg::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPasseckDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_TrayIcon.Create(this, WM_ICON_NOTIFY, GUI_APP_CAPTION, theApp.LoadIcon(IDI_SMALL_PASS), IDR_MAINFRAME))
		return -1;

	if(!m_TrayIcon.SetMenuDefaultItem(0, TRUE))
		return -1;
	
	CSplashWnd::ShowSplashScreen(this);

	return 0;
}

void CMainDlg::SetTrayIcon(int iWorkMode)
{
	if(iWorkMode == 255)
		iWorkMode = m_bWorkMode;

	if(iWorkMode == XF_PASS_ALL)
		m_TrayIcon.SetIcon(IDI_SMALL_PASS);
	else if(iWorkMode == XF_QUERY_ALL)
		m_TrayIcon.SetIcon(IDI_SMALL_QUERY);
	else if(iWorkMode == XF_DENY_ALL)
		m_TrayIcon.SetIcon(IDI_SMALL_DENY);
	else
		m_TrayIcon.SetIcon(IDI_SMALL_PASS);
}

LRESULT CMainDlg::OnTrayNotification(WPARAM wParam, LPARAM lParam)
{
	if(wParam == IDR_MAINFRAME && 
		(lParam == WM_LBUTTONDOWN || lParam == WM_RBUTTONDOWN))
		SetTrayIcon();

	if(wParam == IDR_MAINFRAME 
		&& lParam == WM_RBUTTONUP
		&& theApp.m_dwSubWindowCount > 0)
		lParam = WM_LBUTTONDOWN;

	if(wParam == IDR_MAINFRAME && lParam == WM_LBUTTONDOWN)
	{
		if(m_MessageIndex != 0)
		{
			int tmpIndex = m_MessageIndex;
			m_MessageIndex = 0;
			AfxMessageBox(m_OnLine.GetInternet()->m_sMessage[tmpIndex], MB_ICONINFORMATION);
			m_OnLine.GetInternet()->m_sMessage[tmpIndex][0] = 0;
			SetTrayIcon();
			return 0;
		}

		lParam = WM_LBUTTONDBLCLK;
	}

    return m_TrayIcon.OnTrayNotification(wParam, lParam);
}

LRESULT CMainDlg::OnSplashIcon(UINT wParam, LONG lParam)
{	
	DWORD	dwThreadId = NULL;
	if(ICON_SPLASH_MESSAGE == wParam && lParam > 0 && lParam < MAX_NET_COMMAND)
	{
		static HANDLE thread = NULL;
		::TerminateThread(thread, 0);
		thread = ::CreateThread(NULL, 0, SplashMessage, this, 0, &dwThreadId);
	}
	else if(ICON_SPLASH_ALERT == wParam && !m_IsSplash)
	{
		static HANDLE thread = NULL;
		::TerminateThread(thread, 0);
		thread = ::CreateThread(NULL, 0, SplashIcon, this, 0, &dwThreadId);
	}

	return 0;
}

DWORD WINAPI CMainDlg::SplashIcon(LPVOID pVoid)
{
	CMainDlg* pMainDlg = (CMainDlg*)pVoid;
	pMainDlg->SetSplash(TRUE);

	for(int i = 0; i < 3; i++)
	{
		m_TrayIcon.SetIcon(IDI_SMALL_ALERT);
		if(i == 2)
			continue;
		Sleep(180);
		m_TrayIcon.SetIcon(IDI_SMALL_NULL);
		Sleep(400);
	}
	
	pMainDlg->SetSplash(FALSE);

	return 0;
}

DWORD WINAPI CMainDlg::SplashMessage(LPVOID pVoid)
{
	CMainDlg* pMainDlg = (CMainDlg*)pVoid;
	for(int i = 0; i < MAX_NET_COMMAND; i++)
	{
		if(pMainDlg->GetOnLine()->GetInternet()->m_sMessage[i][0] != 0)
		{
			pMainDlg->SetMessageIndex(i);

			pMainDlg->SetSplash(TRUE);
			while(pMainDlg->GetMessageIndex())
			{
				m_TrayIcon.SetIcon(IDI_SMALL_MESSAGE);
				Sleep(180);
				m_TrayIcon.SetIcon(IDI_SMALL_NULL);
				Sleep(400);
			}
			pMainDlg->SetSplash(FALSE);
		}
	}
	
	ODS("Exit Thread SplashMessage...");
	return 0;
}

void CMainDlg::OnPasseck() 
{
	PXACL_HEADER pHeader = theApp.m_AclFile.GetHeader();
	if(pHeader == NULL)
		return;

	ShellExecute(NULL, _T("open"), pHeader->sWebURL, NULL,NULL, SW_SHOW);	
}

#pragma comment( exestr, "B9D3B8FD2A6F636B70666E692B")
