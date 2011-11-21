// SystemSet.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "SystemSet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSystemSet dialog


CSystemSet::CSystemSet(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CSystemSet::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSystemSet)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_hbr.CreateSolidBrush(PASSECK_DIALOG_BKCOLOR);
}


void CSystemSet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSystemSet)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSystemSet, CDialog)
	//{{AFX_MSG_MAP(CSystemSet)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_SYSTEM_SET_CHECK_LOG, OnSystemSetCheckLog)
	ON_BN_CLICKED(IDC_SYSTEM_SET_CHECK_ALERT_DIALOG, OnSystemSetCheckAlertDialog)
	ON_BN_CLICKED(IDC_SYSTEM_SET_CHECK_ALERT_PCSPEAKER, OnSystemSetCheckAlertPcspeaker)
	ON_BN_CLICKED(IDC_SYSTEM_SET_CHECK_AUTOSTART, OnSystemSetCheckAutostart)
	ON_BN_CLICKED(IDC_SYSTEM_SET_CHECK_SPLASH, OnSystemSetCheckSplash)
	ON_CBN_SELCHANGE(IDC_SYSTEM_SET_LIST_LOG_SIZE, OnSelchangeSystemSetListLogSize)
	ON_BN_CLICKED(IDAPPLY, OnApply)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSystemSet message handlers

void CSystemSet::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	RECT rect;
	GetClientRect(&rect);
	CBrush brush(COLOR_DIALOG_BK);
	dc.FillRect(&rect, &brush);
	
	// Do not call CDialog::OnPaint() for painting messages
}

BOOL CSystemSet::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_ListLogSize.SubclassDlgItem(IDC_SYSTEM_SET_LIST_LOG_SIZE, this);
	m_CheckLog.SubclassDlgItem(IDC_SYSTEM_SET_CHECK_LOG, this);
	m_CheckAutoStart.SubclassDlgItem(IDC_SYSTEM_SET_CHECK_AUTOSTART, this);
	m_CheckSplash.SubclassDlgItem(IDC_SYSTEM_SET_CHECK_SPLASH, this);
	m_CheckAlertSpeaker.SubclassDlgItem(IDC_SYSTEM_SET_CHECK_ALERT_PCSPEAKER, this);
	m_CheckAlertDialog.SubclassDlgItem(IDC_SYSTEM_SET_CHECK_ALERT_DIALOG, this);
	
	m_ButtonApply.SetButtonEx(IDAPPLY, this);
	m_ButtonCancel.SetButtonEx(IDCANCEL, this);

	TCHAR buf[10];
	for(int i = 1; i <= 10; i++)
		m_ListLogSize.InsertString(i-1,_itot(i,buf,10));

	Refresh();

	EnableButton(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSystemSet::EnableButton(BOOL IsEnabled)
{
	m_ButtonApply.EnableWindow(IsEnabled);
	m_ButtonCancel.EnableWindow(IsEnabled);
}

BOOL CSystemSet::IsChange()
{
	PXACL_HEADER pAclHeader = theApp.m_AclFile.GetHeader();
	if(pAclHeader->bWriteLog != m_CheckLog.GetCheck()
		|| pAclHeader->bAutoStart != m_CheckAutoStart.GetCheck()
		|| pAclHeader->bShowWelcome != m_CheckSplash.GetCheck()
		|| pAclHeader->bAudioAlert != m_CheckAlertSpeaker.GetCheck()
		|| pAclHeader->bSplashAlert != m_CheckAlertDialog.GetCheck()
		|| pAclHeader->uiLogSize != m_ListLogSize.GetCurSel() + 1
		)
		return TRUE;
	return FALSE;
}

void CSystemSet::Refresh()
{
	PXACL_HEADER pAclHeader = theApp.m_AclFile.GetHeader();

	m_CheckLog.SetCheck(pAclHeader->bWriteLog);
	m_CheckAutoStart.SetCheck(pAclHeader->bAutoStart);
	m_CheckSplash.SetCheck(pAclHeader->bShowWelcome);
	m_CheckAlertSpeaker.SetCheck(pAclHeader->bAudioAlert);
	m_CheckAlertDialog.SetCheck(pAclHeader->bSplashAlert);

	m_ListLogSize.SetCurSel(pAclHeader->uiLogSize - 1);
	m_ListLogSize.EnableWindow(m_CheckLog.GetCheck());
}

void CSystemSet::OnSystemSetCheckLog() 
{
	EnableButton(IsChange());
	m_ListLogSize.EnableWindow(m_CheckLog.GetCheck());
}

void CSystemSet::OnSystemSetCheckAutostart() 
{
	EnableButton(IsChange());
}

void CSystemSet::OnSystemSetCheckSplash() 
{
	EnableButton(IsChange());
}

void CSystemSet::OnSystemSetCheckAlertPcspeaker() 
{
	EnableButton(IsChange());
}

void CSystemSet::OnSystemSetCheckAlertDialog() 
{
	EnableButton(IsChange());
}

void CSystemSet::OnSelchangeSystemSetListLogSize() 
{
	EnableButton(IsChange());
}


void CSystemSet::OnApply() 
{
	if(!m_ButtonApply.IsWindowEnabled())
		return;

	PXACL_HEADER pAclHeader = theApp.m_AclFile.GetHeader();

	pAclHeader->bWriteLog	 = m_CheckLog.GetCheck();
	pAclHeader->bAutoStart	 = m_CheckAutoStart.GetCheck();
	pAclHeader->bShowWelcome = m_CheckSplash.GetCheck();
	pAclHeader->bAudioAlert  = m_CheckAlertSpeaker.GetCheck();
	pAclHeader->bSplashAlert = m_CheckAlertDialog.GetCheck();
	pAclHeader->uiLogSize	 = m_ListLogSize.GetCurSel() + 1;
	
	DWORD nLength = (DWORD)&pAclHeader->uiSerial - (DWORD)&pAclHeader->bWriteLog;
	DWORD nPosition = (DWORD)&pAclHeader->bWriteLog - (DWORD)pAclHeader;
	if(!theApp.m_AclFile.UpdateFile(nPosition, &pAclHeader->bWriteLog, nLength))
		AfxMessageBox(ERROR_STRING_FILE_SAVE_ERROR);

	CXInstall Install;
	Install.SetAutoStart(pAclHeader->bAutoStart);

	EnableButton(FALSE);
}

void CSystemSet::OnCancel() 
{
	if(!m_ButtonCancel.IsWindowEnabled())
		return;

	Refresh();
	EnableButton(FALSE);
}

HBRUSH CSystemSet::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{

	if(pWnd->GetDlgCtrlID() == IDC_SYSTEM_SET_LIST_LOG_SIZE)
	{
		HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
		return hbr;
	}

	pDC->SetTextColor(COLOR_TEXT_NORMAL);
	pDC->SetBkMode(TRANSPARENT);

	return m_hbr;
}

BOOL CSystemSet::IsChangeEx()
{
	return m_ButtonApply.IsWindowEnabled();
}

void CSystemSet::Apply()
{
	OnApply();
}

void CSystemSet::Cancel()
{
	OnCancel();
}

#pragma comment( exestr, "B9D3B8FD2A757B7576676F7567762B")
