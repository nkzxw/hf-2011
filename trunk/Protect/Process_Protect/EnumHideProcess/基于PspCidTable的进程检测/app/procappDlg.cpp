// procappDlg.cpp : implementation file
//

#include "stdafx.h"
#include "procapp.h"
#include "procappDlg.h"
#include <winioctl.h>
#include <winsvc.h>
#include "..\\源文件\\listproc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProcappDlg dialog

CProcappDlg::CProcappDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProcappDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProcappDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CProcappDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProcappDlg)
	DDX_Control(pDX, IDC_LIST1, m_listbox);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CProcappDlg, CDialog)
	//{{AFX_MSG_MAP(CProcappDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProcappDlg message handlers

BOOL CProcappDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CProcappDlg::OnPaint() 
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
HCURSOR CProcappDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

TCHAR szServiceName[] = "ListProcessDrv";

BOOL IsInstalled()
{
    BOOL bResult = FALSE;

	//打开服务控制管理器
    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM != NULL)
    {
		//打开服务
        SC_HANDLE hService = ::OpenService(hSCM, szServiceName, SERVICE_QUERY_CONFIG);
        if (hService != NULL)
        {
            bResult = TRUE;
            ::CloseServiceHandle(hService);
        }
        ::CloseServiceHandle(hSCM);
    }
    return bResult;
}

void OnInstallDriver() 
{
	char driverpath[MAX_PATH];
	strcpy(driverpath, "c:\\ListProcess.sys");	
 	SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCM == NULL)
	{
		AfxMessageBox("打开服务控制管理器失败!");
		return ;
	}
 	SC_HANDLE hService = ::CreateService(
		hSCM, 
		szServiceName, 
		szServiceName,
		SERVICE_ALL_ACCESS, 
		SERVICE_KERNEL_DRIVER,
		SERVICE_SYSTEM_START,
		SERVICE_ERROR_NORMAL,
		driverpath, 
		NULL, 
		NULL, 
		NULL, 
		NULL, 
		NULL);
	if (hService == NULL)
	{
		hService = OpenService(hSCM, szServiceName, SERVICE_START);
	}
	if (hService)
	{		 
		StartService(hService, 0, NULL);		
		::CloseServiceHandle(hService);
	}
	else
	{
		AfxMessageBox("安装驱动失败!");
	}
	::CloseServiceHandle(hSCM);
}

// 
BOOL UnloadDriver()
{
    if (!IsInstalled())
        return TRUE;

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM == NULL)
    {
        MessageBox(NULL, _T("不能打开服务控制管理器"), szServiceName, MB_OK);
        return FALSE;
    }

    SC_HANDLE hService = ::OpenService(hSCM, szServiceName, SERVICE_STOP | DELETE);

    if (hService == NULL)
    {
        ::CloseServiceHandle(hSCM);
        AfxMessageBox( _T("不能打开服务") );
        return FALSE;
    }
    SERVICE_STATUS status;
    ::ControlService(hService, SERVICE_CONTROL_STOP, &status);
	Sleep( 1000 );

	//删除服务
    BOOL bDelete = ::DeleteService(hService);
    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);

    if (bDelete)
        return TRUE;

    AfxMessageBox(_T("服务不能被删除"));
    return FALSE;
}
 
void CProcappDlg::OnOK() 
{
	// 启动驱动
	OnInstallDriver();
	// 访问设备
	HANDLE hDevice = CreateFile("\\\\.\\proclistdrv", 
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			0,
			NULL );
	if (hDevice == INVALID_HANDLE_VALUE)	
		return ;
	BYTE* Processptr = new BYTE[1024*sizeof(ProcessInfo)];
	ProcessInfo* ptr = (ProcessInfo*) (Processptr);
	DWORD nb;
 	DeviceIoControl(hDevice,IOCTL_GETPROC_LIST,NULL,0,Processptr,1024*sizeof(ProcessInfo),&nb,NULL);
	CloseHandle(hDevice);

	// 卸载驱动
	UnloadDriver();

	// 解析processptr中数据
	ProcessInfo* p;
	CString str;
	for (p=ptr; p; p=p->next)
	{
		str.Format("%x %x %s", p->addr, p->pid, p->name);
		m_listbox.AddString(str);
	}
}
