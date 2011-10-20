// NetWall Exe.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "NetWall.h"

#include "MainFrm.h"
#include "NetWallDoc.h"
#include "NetWallView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetWallApp

BEGIN_MESSAGE_MAP(CNetWallApp, CWinApp)
	//{{AFX_MSG_MAP(CNetWallApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNetWallApp construction

CNetWallApp::CNetWallApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
    m_nAppAPIVersion    = NETWALL_API_VERSION;
    m_nDllAPIVersion    = 0;
    m_nDriverAPIVersion = 0;

    m_pAPI = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CNetWallApp object

CNetWallApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CNetWallApp initialization

BOOL CNetWallApp::InitInstance()
{
    // Only Run One Instance
    HANDLE iHandle = ::CreateMutex(NULL,TRUE, m_pszExeName);
    if (GetLastError() == ERROR_ALREADY_EXISTS)	
    {
        CWnd *pPreWnd = CWnd::GetDesktopWindow()->GetWindow(GW_CHILD);
        while (pPreWnd)
        {
            if (::GetProp(pPreWnd->GetSafeHwnd(), m_pszExeName))
            {
                if (pPreWnd->IsIconic()) 
                {
                    pPreWnd->ShowWindow(SW_RESTORE);
                }
                
                pPreWnd->SetForegroundWindow();
                // If the window has a popup window set focus to pop-up
                pPreWnd->GetLastActivePopup ()->SetForegroundWindow();
                return FALSE;
            }
            
            pPreWnd = pPreWnd->GetWindow(GW_HWNDNEXT);
        }
        return FALSE;
    }

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

    if (NULL == m_pAPI)
    {
        m_pAPI = new CNetWallAPI();      
        if (NULL == m_pAPI)
        {
            AfxMessageBox("System Resources is not enough, Please Close Some Application.");
            return FALSE;
        }
    }

    //
    // Get NetWal Driver Information
    //
    if (! GetNetWallInfo())
    {
        delete m_pAPI;
        m_pAPI = NULL;
        return FALSE;
    }

    //
    // Verify App, DLL & Driver API Versions
    //
    if ((m_nDriverAPIVersion & 0xFFFFFF00) != (m_nAppAPIVersion & 0xFFFFFF00)
        || (m_nDllAPIVersion & 0xFFFFFF00) != (m_nAppAPIVersion & 0xFFFFFF00))
    {
       AfxMessageBox(IDS_API_VER_MISMATCH, MB_OK | MB_APPLMODAL | MB_ICONSTOP);
       delete m_pAPI;
	   return FALSE;
    }

    if (! AfxSocketInit())
    {        
        AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
        delete m_pAPI;
        m_pAPI = NULL;
        return FALSE;
    }

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CNetWallDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CNetWallView));
	AddDocTemplate(pDocTemplate);

    ::SetProp(m_pMainWnd->GetSafeHwnd(), m_pszExeName, (HANDLE)1);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
    if (!ProcessShellCommand(cmdInfo)) 
    {    
        delete m_pAPI;
        m_pAPI = NULL;
		return FALSE;
    }

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CString	m_strApiVersion;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
    CNetWallApp *pApp = (CNetWallApp * )AfxGetApp();
    
	//{{AFX_DATA_INIT(CAboutDlg)
	m_strApiVersion = _T("");
	//}}AFX_DATA_INIT
    
    m_strApiVersion = pApp->m_strApiVersion;
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Text(pDX, IDC_API_VERSION, m_strApiVersion);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CNetWallApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CNetWallApp message handlers


BOOLEAN CNetWallApp::GetNetWallInfo()
{
    CString  sText;
    BOOLEAN  bResult   = FALSE;
    HANDLE   hIMHandle = INVALID_HANDLE_VALUE;
    DWORD    nSize;

    //
    // Call DLL To Get API Version
    //
    bResult = m_pAPI->GetDllAPIVersion(&m_nDllAPIVersion);

	if (! bResult)
	{
        //
        // Could Not Get DLL API Version
        //
	}

    //
    // Open The Device Handle
    //
    hIMHandle = CreateFile(NETWALL_WDM_DEVICE_FILENAME,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL
                           );

    if (INVALID_HANDLE_VALUE == hIMHandle)
    {
        AfxMessageBox(_T("Could Not Open NetWall Driver."));
        return FALSE;
    }

    //
    // Call Driver To Get API Version
    //
    bResult = m_pAPI->GetDriverAPIVersion(hIMHandle, &m_nDriverAPIVersion);

	if (! bResult)
	{
        //
        // Could Not Get Driver API Version
        //
        AfxMessageBox(_T("Could Not Get Driver API Version."));
        return FALSE;
    }
    
    //
    // Get Driver Description
    //
    nSize = MAX_PATH + 1;

	LPTSTR pszBuffer = sText.GetBuffer(nSize);

    bResult = m_pAPI->GetDriverDescription(hIMHandle, pszBuffer, &nSize);

    CloseHandle(hIMHandle);

	if (bResult)
	{
        m_strDriverDescription = pszBuffer;
    }
    else
    {
        m_strDriverDescription = _T("Unknown");
    }

    sText.ReleaseBuffer();

	char szScratch[16];

	wsprintf(szScratch, "%d.%2.2d.%2.2d.%2.2d",
                  ((PUCHAR)&m_nDriverAPIVersion)[3],
                  ((PUCHAR)&m_nDriverAPIVersion)[2],
                  ((PUCHAR)&m_nDriverAPIVersion)[1],
                  ((PUCHAR)&m_nDriverAPIVersion)[0]
                );

	m_strApiVersion = szScratch;

    return bResult;
}

int CNetWallApp::ExitInstance() 
{
	if (NULL != m_pAPI)
    {
        delete m_pAPI;
        m_pAPI = NULL;        
    }

	return CWinApp::ExitInstance();
}
