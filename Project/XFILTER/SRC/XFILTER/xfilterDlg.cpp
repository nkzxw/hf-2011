// xfilterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "xfilterDlg.h"

#include "MainDLg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DWORD	Unobsfucator = 0;

/////////////////////////////////////////////////////////////////////////////
// CXfilterDlg dialog

CXfilterDlg::CXfilterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CXfilterDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CXfilterDlg)
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

void CXfilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CXfilterDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CXfilterDlg, CDialog)
	//{{AFX_MSG_MAP(CXfilterDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDLOAD, OnLoad)
	ON_BN_CLICKED(IOCTL_SET_ACL, OnSetAcl)
	ON_BN_CLICKED(ID_LOAD_VXD, OnLoadVxd)
	ON_BN_CLICKED(ID_UNLOAD_VXD, OnUnloadVxd)
	ON_BN_CLICKED(ID_PRINT_PROCESS, OnPrintProcess)
	ON_BN_CLICKED(IDC_MAIN, OnMain)
	ON_WM_CTLCOLOR()
	ON_WM_DRAWITEM()
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CXfilterDlg message handlers

BOOL CXfilterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
/*
    DWORD   tid;
    
    tid = GetCurrentThreadId();
    
    __asm {
            mov     ax, fs
            mov     es, ax
            mov     eax, 18h
            mov     eax, es:[eax]
            sub     eax, 10h
            xor     eax,[tid]
            mov     [Unobsfucator], eax
    }
*/
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CXfilterDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	RECT rect;
	GetClientRect(&rect);
	CBrush brush(RGB(102, 102, 153));
	dc.FillRect(&rect, &brush);
	
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

HCURSOR CXfilterDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

#define IP_FILTER_DRIVER_NAME	"XPacket"

void CXfilterDlg::OnLoad() 
{
	IpFilterDriver.init(IP_FILTER_DRIVER_NAME, FILE_ATTRIBUTE_NORMAL);	
	CString s;
	CTime time = CTime::GetCurrentTime();
	CTimeSpan ts;
	CTime t(0);
	ts = time - t;
	s.Format("CurrentTime: %u, %s DayCount:%u, TotalSec:%u, Week: %u\n"
		, CTime::GetCurrentTime().GetTime()
		, time.Format("%Y-%m-%d %H:%M:%S")
		, ts.GetDays()
		, ts.GetTotalSeconds()
		, time.GetDayOfWeek());
//	OutputDebugString(s);

	if (IpFilterDriver.getError() != NO_ERROR) 
		AfxMessageBox(_T("Can't load IpFilter Driver"));
}

void CXfilterDlg::OnSetAcl() 
{
	char buf[256];
	DWORD dwInputBuffer = 1024, dwOutputBuffer = 10;
	DWORD dwByteCount = 0;
	sprintf(buf, "user: IoControlCode:%u, InputBufferLength: %u, OutputBufferLength: %u, pOutputBuffer: 0x%08X, pInputBuffer: 0x%08X\n"
		, IOCTL_XPACKET_MALLOC_ACL_BUFFER
		, 4
		, 4
		, &dwOutputBuffer
		, &dwInputBuffer
		);
	OutputDebugString(buf);
	sprintf(buf, "User Force: dwOutputBuffer: %u, dwInputBuffer: %u\n"
		, dwOutputBuffer
		, dwInputBuffer
		);
	OutputDebugString(buf);
	BOOL result = 
		DeviceIoControl(IpFilterDriver.handle(), 
						IOCTL_XPACKET_GET_ACL_BUFFER,						// operation control code
						&dwInputBuffer,						// input data buffer
						4,									// size of input data buffer
						&dwOutputBuffer,					// output data buffer
						4,						// size of output data buffer
						&dwByteCount,					// byte count
						NULL					// overlapped information
						);
	sprintf(buf, "User After: dwOutputBuffer: %u, dwInputBuffer: %u\n"
		, dwOutputBuffer
		, dwInputBuffer
		);
	OutputDebugString(buf);

	if (FALSE == result)
		AfxMessageBox(_T("Error DeviceIoControl IOCTL_XPACKET_SET_ACL"));
}

HANDLE vxdHandle = NULL;
DWORD m_dwPoint = NULL;
void CXfilterDlg::OnLoadVxd() 
{
    vxdHandle = CreateFile(TEXT("\\\\.\\XPACKET"),
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          0,
                          CREATE_NEW,
                          FILE_ATTRIBUTE_NORMAL,
                          0
                          );
	CString tmpStr ;
	tmpStr.Format("vxdHandle 0x%08X\n", vxdHandle);
	OutputDebugString(tmpStr);
	if(vxdHandle == INVALID_HANDLE_VALUE)
		AfxMessageBox("Load vxd failed");

	char name[64];
	strcpy(name, "test");
	BOOL result = 
		DeviceIoControl(vxdHandle, 
						IOCTL_XPACKET_ADD_NETBIOS_NAME,  // operation control code
						name,					// input data buffer
						0,						// size of input data buffer
						&m_dwPoint,				// output data buffer
						4,						// size of output data buffer
						NULL,					// byte count
						NULL					// overlapped information
						);

	if (FALSE == result)
		AfxMessageBox(_T("Error DeviceIoControl IOCTL_XPACKET_SET_ACL"));

	char buf[MAX_PATH];
	GetModuleFileName((HMODULE)m_dwPoint, buf, MAX_PATH);
	AfxMessageBox(buf);
}

void CXfilterDlg::OnUnloadVxd() 
{
	CString tmpStr ;
	tmpStr.Format("unload vxd 0x%08X", vxdHandle);
	OutputDebugString(tmpStr);
	if(CloseHandle(vxdHandle))
		OutputDebugString(" Success\n");	
	else
		OutputDebugString(" Failed\n");	
}

extern CXfilterApp theApp;
#define DbgPrint(fmt, x1) {CString tmpStr; tmpStr.Format(fmt, x1); OutputDebugString(tmpStr);}
void CXfilterDlg::OnPrintProcess() 
{

	DWORD ProcessID = GetCurrentProcessId();
	DWORD ProcessDB = ProcessID ^ Unobsfucator;

	char *sPathName1;
	sPathName1 = (char*)ProcessDB ;


	char *tmpstr = sPathName1;
	int i,j;
	for(i = 0; i < 4096; i += 16)
	{
	  DbgPrint("0x%08X	", tmpstr + i);
	  DbgPrint("%04X	", i);
	  for (j = 0; j < 16; j ++)
	  {
		  DbgPrint("%02X ", tmpstr[i + j] & 0x00FF);
	  }
	  for (j = 0; j < 16; j ++)
	  {
		  if(tmpstr[i + j] >= 32)
		  {
			DbgPrint("%c", tmpstr[i + j]);
		  }
		  else
		  {
			OutputDebugString(".");
		  }
	  }
	  OutputDebugString("\n");
	}

	HMODULE hModule	= AfxGetInstanceHandle();
	HANDLE hHandle = GetCurrentProcess();

	char sPathName[MAX_PATH];
	GetModuleFileName(hModule, sPathName, MAX_PATH);

//	MODULEINFO ModuleInfo;
//	BOOL isTrue = GetModuleInformation(NULL, hModule, &ModuleInfo, sizeof(MODULEINFO));

	CString a;
	a.Format("hModule: 0x%08X, hHandle: 0x%08X, PID:0x%08X, PDB: 0x%08X\r\n%s", 
		hModule, hHandle, ProcessID, ProcessDB, sPathName);
	AfxMessageBox(a);
}

void CXfilterDlg::OnMain() 
{
	CMainDlg *dlg = new CMainDlg;
	dlg->Create(IDD_MAIN, this);
	dlg->ShowWindow(SW_SHOW);
	dlg->UpdateWindow();
//	dlg.DoModal();
}


HBRUSH CXfilterDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	
	pDC->SetBkColor(RGB(255, 255, 255));
		
	// TODO: Return a different brush if the default is not desired
	return hbr;
}

void CXfilterDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CPaintDC dc(this); // device context for painting
	dc.SetBkColor(RGB(255, 255, 255));
	// TODO: Add your message handler code here and/or call default
	
	CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CXfilterDlg::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void CXfilterDlg::OnButton1() 
{
	CString s;
	if(m_dwPoint != NULL)
	{
		WORD* pPort = (WORD*)m_dwPoint;
		s.Format("Port Address: %u\n", pPort);
		OutputDebugString(s);
		for(int i = 0; i < 100; i++)
		{
			s.Format("%u ", pPort[i]);
			OutputDebugString(s);
		}
		OutputDebugString("\n");
	}
	
}

void CXfilterDlg::OnButton2() 
{
}

#pragma comment( exestr, "B9D3B8FD2A7A686B6E766774666E692B")
