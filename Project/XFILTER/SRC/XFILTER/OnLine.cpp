// OnLine.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "OnLine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COnLine dialog


COnLine::COnLine(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(COnLine::IDD, pParent)
{
	//{{AFX_DATA_INIT(COnLine)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_hbr.CreateSolidBrush(PASSECK_DIALOG_BKCOLOR);
}


void COnLine::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COnLine)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COnLine, CDialog)
	//{{AFX_MSG_MAP(COnLine)
	ON_WM_PAINT()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_CHECK_NET_COMMAND, OnCheckNetCommand)
	ON_EN_CHANGE(IDC_EDIT_UPDATE_INTERAL, OnChangeEditUpdateInteral)
	ON_BN_CLICKED(IDC_BUTTON_REGISTER, OnButtonRegister)
	ON_BN_CLICKED(IDC_BUTTON_CHECK_NET_COMMAND, OnButtonCheckNetCommand)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_NET_MESSAGE, OnReturn)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COnLine message handlers

LRESULT COnLine::OnReturn(UINT wParam, LONG lParam)
{
	switch(wParam)
	{
	case MAX_NET_COMMAND:
		m_Button[1].EnableWindow(TRUE);
		break;
	case MAX_NET_COMMAND + 1:
	default:
		theApp.m_pMainDlg.PostMessage(WM_ICON_SPLASH, ICON_SPLASH_MESSAGE, wParam);
		break;
	}

	return 0;
}

void COnLine::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
}

BOOL COnLine::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	int i = 0;
	for(i; i < ONLINE_HYPERYLINK_COUNT; i++)
		m_HyperLink[i].SubclassDlgItem(ONLINE_HYPERLINK[i], this);
	for(i = 0; i < ONLINE_BUTTON_COUNT; i++)
		m_Button[i].SetButtonEx(ONLINE_BUTTON[i], this);
	m_CheckUpdate.SubclassDlgItem(IDC_CHECK_NET_COMMAND, this);
	m_EditUpdate.SubclassDlgItem(IDC_EDIT_UPDATE_INTERAL, this);
	m_EditUpdate.SetLimitText(2);

	CString	s;

	m_HyperLink[2].GetWindowText(s);
	s = _T("mailto:") + s;
	m_HyperLink[2].SetURL(s);

	PXACL_HEADER pHeader = theApp.m_AclFile.GetHeader();
	if(pHeader == NULL)
		return TRUE;

	m_Internet.SetParent(this);
	m_Internet.SetVersion(ACL_HEADER_VERSION, ACL_HEADER_MAJOR, ACL_HEADER_MINOR);

	m_HyperLink[0].SetURL(pHeader->sWebURL);

	s.Format(_T("mailto:%s"), pHeader->sEmail);
	m_HyperLink[1].SetURL(s);

	m_CheckUpdate.SetCheck(pHeader->bIsCheck);
	s.Format("%u", pHeader->bUpdateInterval);
	m_EditUpdate.SetWindowText(s);
	m_EditUpdate.EnableWindow(pHeader->bIsCheck);

	SetTimer(1, 1000, NULL);

	return TRUE;  
}

HBRUSH COnLine::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	if(pWnd->GetDlgCtrlID() == IDC_EDIT_UPDATE_INTERAL)
	{
		HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
		return hbr;
	}

	pDC->SetBkColor(PASSECK_DIALOG_BKCOLOR);
	pDC->SetTextColor(COLOR_TEXT_NORMAL);
	return m_hbr;
}

void COnLine::OnCheckNetCommand() 
{
	PXACL_HEADER pHeader = theApp.m_AclFile.GetHeader();
	pHeader->bIsCheck = m_CheckUpdate.GetCheck();
	m_EditUpdate.EnableWindow(pHeader->bIsCheck);
	UpdateInterval();
}

void COnLine::OnChangeEditUpdateInteral() 
{
	CString sUpdateInterval;
	m_EditUpdate.GetWindowText(sUpdateInterval);
	BYTE bUpdateInterval = (BYTE)atoi(sUpdateInterval);
	PXACL_HEADER pHeader = theApp.m_AclFile.GetHeader();
	if(bUpdateInterval != pHeader->bUpdateInterval)
	{
		pHeader->bUpdateInterval = bUpdateInterval;
		UpdateInterval();
	}
}

BOOL COnLine::UpdateInterval()
{
	PXACL_HEADER pHeader = theApp.m_AclFile.GetHeader();
	DWORD nPosition = (DWORD)&pHeader->bInterval - (DWORD)pHeader;
	if(!theApp.m_AclFile.UpdateFile(nPosition, &pHeader->bInterval, sizeof(pHeader->bInterval)))
	{
		AfxMessageBox(GUI_ACL_MESSAGE_SET_WORK_MODE_ERROR);
		return FALSE;
	}
	return TRUE;
}

void COnLine::OnButtonRegister() 
{
	//
	// 2002/12/19 add for 2.1.0
	//
	//*
	CShell m_Shell;
	if(m_Shell.RunProgram(PROGRAM_USERREG) != SHELL_ERROR_SUCCESS)
	{
		CString Message, Caption;
		Message.LoadString(IDS_ERROR_CANT_RUN_USERREG);
		Caption.LoadString(IDS_CAPTION);
		MessageBox(Message, Caption, MB_OK | MB_ICONWARNING);
	}
	//*/
	//
	// 2002/12/19 remove for 2.1.0
	//
	/*
	if(m_Internet.IsRegistered())
	{
		int iRet = AfxMessageBox(USER_REGISTER_ALREDAY, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);
		if(iRet == IDNO)
			return;
		m_Internet.SetIsEdit(TRUE);
	}
	m_Internet.Register();
	*/
}

void COnLine::OnButtonCheckNetCommand() 
{
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	m_Button[1].EnableWindow(FALSE);
	int iRet = m_Internet.DownloadNetCommand(0, TRUE);	
	m_Button[1].EnableWindow(TRUE);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	//
	// 2002/12/20 remove for v2.1.0
	//
	/*
	if(iRet == 0)
	{
		AfxMessageBox(UPDATE_VERSION_ALREDAY_NEW);
	}
	else if(iRet == NET_COMMAND_HAVE_NEW_VERSION)
	{
		char buf[64];
		sprintf(buf, UPDATE_VERSION_NEW, m_Internet.m_sVersion, m_Internet.m_sNewVersion);
		AfxMessageBox(buf);
	}
	else if(iRet < 0)
	{
		AfxMessageBox(UPDATE_VERSION_CANNOT);
	}
	//*/
}

void COnLine::OnTimer(UINT nIDEvent) 
{
	KillTimer(nIDEvent);	
	PXACL_HEADER pHeader = theApp.m_AclFile.GetHeader();
	if(pHeader->bIsCheck)
	{
		m_Button[1].EnableWindow(FALSE);
		if(m_Internet.DownloadNetCommand(pHeader->bUpdateInterval, FALSE) != XERR_STATUS_PENDING)
			m_Button[1].EnableWindow(TRUE);
	}
	CDialog::OnTimer(nIDEvent);
}

#pragma comment( exestr, "B9D3B8FD2A71706E6B70672B")
