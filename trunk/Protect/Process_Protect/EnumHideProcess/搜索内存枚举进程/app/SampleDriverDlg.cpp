// SampleDriverDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SampleDriver.h"
#include "SampleDriverDlg.h"
#include <winioctl.h>
#include "..\SafePsEnum\processenum.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define	SYS32_FILE			_T("safepsenum.sys")
#define	SYS_NAME			_T("safepsenum")
extern "C" BOOL LoadDeviceDriver(const TCHAR * Name, const TCHAR * Path, PDWORD Error);
extern "C" BOOL UnloadDeviceDriver(const TCHAR * Name);

#define DEVICEFILENAME "\\\\.\\safepsenum"

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
// CSampleDriverDlg dialog

CSampleDriverDlg::CSampleDriverDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSampleDriverDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSampleDriverDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSampleDriverDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSampleDriverDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSampleDriverDlg, CDialog)
	//{{AFX_MSG_MAP(CSampleDriverDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSampleDriverDlg message handlers

BOOL CSampleDriverDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	
	CListCtrl* pListCtrl = (CListCtrl*) GetDlgItem(IDC_LIST);

	LVCOLUMN lvc; 
	lvc.mask = LVCF_TEXT|LVCF_WIDTH;
	lvc.pszText = "进程名";
	lvc.cx = 120;
	pListCtrl->InsertColumn(0, &lvc);
	lvc.pszText = "ID";
	lvc.cx = 150;
	pListCtrl->InsertColumn(1, &lvc);
	lvc.pszText = "PEA";
	lvc.cx = 120;
	pListCtrl->InsertColumn(2, &lvc);
 
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

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CSampleDriverDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CSampleDriverDlg::OnPaint() 
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

HCURSOR CSampleDriverDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

BOOL bLoad = FALSE;
void CSampleDriverDlg::OnOK() 
{
	TCHAR Temp[MAX_PATH];
	DWORD dwError;

	GetSystemDirectory(Temp, MAX_PATH);

	strcat(Temp, "\\Drivers\\safepsenum.sys");

//	if (!bLoad && !LoadDeviceDriver(SYS_NAME, Temp, &dwError))
//	{
//		SetDlgItemText(IDC_STATUS, "启动驱动失败!");		 
//	}
//	else
	{		
		TCHAR DeviceName[64];

		bLoad = TRUE;

		SetDlgItemText(IDC_STATUS, "成功启动驱动!");	 
 	
		strcpy(DeviceName, _T("\\\\.\\safepsenum"));

		HANDLE hFile  = CreateFile(
			DeviceName,
			GENERIC_READ,
			FILE_SHARE_READ|FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL );

		if (hFile == INVALID_HANDLE_VALUE)	
			return ;

		DWORD nb;
		
	 	BYTE* Processptr = new BYTE[sizeof(DWORD)+1024*sizeof(Processinfo)];
		Processinfo* ptr = (Processinfo*) (Processptr+sizeof(DWORD));

 		DeviceIoControl(hFile,IOCTL_GETPROCESSPTR,NULL,0,Processptr,sizeof(DWORD)+1024*sizeof(Processinfo),&nb,NULL);

		DWORD dwCount;
		
		memcpy(&dwCount, (BYTE*)Processptr, sizeof(DWORD));

		LVITEM lvi;
		lvi.mask = LVIF_TEXT;
		lvi.pszText = new char[128];

		CListCtrl* pListCtrl = (CListCtrl*)GetDlgItem(IDC_LIST);
		char szTxt[64];

		for (UINT i=0; i<dwCount; i++)
		{
			lvi.iItem = i;
			lvi.iSubItem = 0;
			strcpy(lvi.pszText, ptr[i].Name);
			pListCtrl->InsertItem(&lvi);
			ltoa(ptr[i].PId, szTxt, 10);
			pListCtrl->SetItemText(i, 1, szTxt);
			ltoa(ptr[i].pEProcess, szTxt, 16);
			pListCtrl->SetItemText(i, 2, szTxt);
		}
		delete []lvi.pszText;
		delete []Processptr;
		
		CloseHandle(hFile);		
	} 
}

void CSampleDriverDlg::OnCancel() 
{
//	if (bLoad)
//	{
//		UnloadDeviceDriver(SYS_NAME);
//	}
	CDialog::OnCancel();
}
