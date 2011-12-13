// hf_packDlg.cpp : implementation file
//

#include "stdafx.h"
#include "hf_pack.h"
#include "hf_packDlg.h"
#include "inject.h"

#include "Encrypt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
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


// Chf_packDlg dialog




Chf_packDlg::Chf_packDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Chf_packDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Chf_packDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Chf_packDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_OPEN, &Chf_packDlg::OnBnClickedOpen)
	ON_BN_CLICKED(ID_ENCRYPT, &Chf_packDlg::OnBnClickedEncrypt)
	ON_BN_CLICKED(ID_CANCEL, &Chf_packDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// Chf_packDlg message handlers

BOOL Chf_packDlg::OnInitDialog()
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

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void Chf_packDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void Chf_packDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

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

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR Chf_packDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Chf_packDlg::OnBnClickedOpen()
{
	CString strFile = _T("");
	CFileDialog dlgFile(TRUE, NULL, NULL, 
						OFN_HIDEREADONLY, 
						_T("Describe Files (*.exe)|*.exe|All Files (*.*)|*.*||"), 
						NULL);

	if (dlgFile.DoModal())
	{
		strFile = dlgFile.GetPathName();
	}

	GetDlgItem(IDC_FILE_EDIT)->SetWindowText (strFile);
}

inline bool CheckInput (LPCSTR fileName, LPCSTR pwd, LPCSTR pwd2)
{
	if (GetFileAttributes (fileName) == -1){
		MessageBox (NULL, "文件不存在.", "hf_pack", 0);
		return false;
	}
	else if (strlen (pwd) == 0 || strlen (pwd2) == 0){
		MessageBox (NULL, "密码不能为空.","hf_pack", 0);
		return false;
	}
	else if (strcmp (pwd, pwd2) != 0){
		MessageBox (NULL, "密码不一致.","hf_pack", 0);
		return false;
	}
	return true;
}

void Chf_packDlg::OnBnClickedEncrypt()
{
	DWORD dwKern = (DWORD)LoadLibrary ("kernel32.dll");
	DWORD dwProc = (DWORD)GetProcAddress ((HMODULE)dwKern, "GetProcAddress");
	DWORD dw = (DWORD)LoadLibrary ("user32.dll");
	
	CString csPwd;
	CString csPwd2;
	CString csFile;
	GetDlgItem(IDC_PWD_EDIT)->GetWindowTextA (csPwd);
	GetDlgItem(IDC_CONFIRM_EDIT)->GetWindowTextA (csPwd2);
	GetDlgItem(IDC_FILE_EDIT)->GetWindowTextA (csFile);

	if (CheckInput (csFile, csPwd, csPwd2) == false){
		return;
	}
	

	LPCSTR lpfile = csFile;
	LPCSTR lppwd = csPwd;

	//Encrypt ((char *)lpfile, (char *)lppwd, true);

	inject ((char *)lpfile, (char *)lppwd);
}



void Chf_packDlg::OnBnClickedCancel()
{
	CDialog::OnCancel(); 
}
