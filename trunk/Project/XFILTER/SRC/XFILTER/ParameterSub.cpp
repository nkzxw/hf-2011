// ParameterSub.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "ParameterSub.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CParameterSub dialog

extern CSystemTray	m_TrayIcon;
BOOL m_bInTrayIcon = FALSE;
BOOL m_bIsStartTrayIcon = FALSE;
DWORD m_dwTrayRecvData = 0;
DWORD m_dwTraySendData = 0;
void SetTrayIcon(BYTE bIsSend);
DWORD WINAPI TrayIconThread(PVOID pVoid);
void StartTrayIcon();
void StopTrayIcon();

CParameterSub::CParameterSub(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CParameterSub::IDD, pParent)
{
	//{{AFX_DATA_INIT(CParameterSub)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_dwInData = 0;
	m_dwOutData = 0;
	m_dwDenyPackets = 0;
	m_dwPassPackets = 0;
	m_hbr.CreateSolidBrush(PASSECK_DIALOG_BKCOLOR);
}

CParameterSub::~CParameterSub()
{
}

void CParameterSub::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CParameterSub)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CParameterSub, CDialog)
	//{{AFX_MSG_MAP(CParameterSub)
	ON_WM_PAINT()
	ON_WM_CTLCOLOR()
	ON_WM_TIMER()
	ON_WM_VSCROLL()
	//}}AFX_MSG_MAP
	ON_MESSAGE(PAM_WM_UPDATE_DATA, UpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CParameterSub message handlers

BOOL CParameterSub::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CreateCaptionEx(PARAMETER_CAPTION);

	m_Slider.SubclassDlgItem(IDC_SECURITY, this);
	m_Slider.SetRangeMax(2);

	for(int i = 0; i< PARAMETER_LABEL_COUNT; i++)
		m_Label[i].SubclassDlgItem(PARAMETER_LABEL[i], this);

	memset(m_sSpace, ' ', 36);
	m_sSpace[35] = 0;

	m_tStartTime = CTime::GetCurrentTime();
	CString sStartTime;
	sStartTime.Format(PARAMETER_FORMAT[0], m_tStartTime.Format("%H:%M:%S"));
	m_Label[0].SetWindowText(sStartTime);

	PXACL_HEADER pHeader = theApp.m_AclFile.GetHeader();
	if(pHeader != NULL)
		m_Slider.SetPos(pHeader->bSecurity);

	Refresh();
	SetTimer(1, 1000, NULL);

	StartTrayIcon();

	m_OsilloGraph.SubclassDlgItem(IDC_STATUS, this);
	m_OsilloGraph.Startup();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CParameterSub::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
}


HBRUSH CParameterSub::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	UINT nID = pWnd->GetDlgCtrlID();
	if(nID == 0xFFFF)
	{
		HBRUSH hbr = CPasseckDialog::OnCtlColor(pDC, pWnd, nCtlColor);
		pDC->SetBkMode(TRANSPARENT);
		return hbr;
	}
	else
	{
		pDC->SetTextColor(COLOR_TEXT_NORMAL);
		pDC->SetBkColor(PASSECK_DIALOG_BKCOLOR);
	}

	return m_hbr;
}

DWORD WINAPI TrayIconThread(PVOID pVoid)
{
	static DWORD dwInData = 0;
	static DWORD dwOutData = 0;

	m_bInTrayIcon = TRUE;
	while(m_bIsStartTrayIcon)
	{
		if(dwOutData != m_dwTraySendData)
		{
			SetTrayIcon(TRUE);
			dwOutData = m_dwTraySendData;
		}
		else
		{
			Sleep(100);
		}

		if(dwInData != m_dwTrayRecvData)
		{
			SetTrayIcon(FALSE);
			dwInData = m_dwTrayRecvData;
		}
		else
		{
			Sleep(100);
		}
	}
	m_bInTrayIcon = FALSE;
	return 0;
}

void StartTrayIcon()
{
	m_bIsStartTrayIcon = TRUE;
	DWORD dwThreadId;
	CreateThread(0, 0, TrayIconThread, NULL, 0, &dwThreadId);
}
void StopTrayIcon()
{
	m_bIsStartTrayIcon = FALSE;
	if(m_bInTrayIcon)
		Sleep(50);
}

void SetTrayIcon(BYTE bIsSend)
{
	UINT nID = m_TrayIcon.GetCurrentIconId();
	if(nID == IDI_SMALL_ALERT || nID == IDI_SMALL_MESSAGE 
		|| nID == IDI_SMALL_NULL)
		return;

	PXACL_HEADER pHeader = theApp.m_AclFile.GetHeader();
	if(pHeader == NULL)
		return;

	switch(pHeader->bWorkMode)
	{
	case WORKMODE_PASSALL:
		m_TrayIcon.SetIcon(bIsSend ? IDI_SMALL_PASS_SEND : IDI_SMALL_PASS_RECV);
		break;
	case WORKMODE_FILTER:
		m_TrayIcon.SetIcon(bIsSend ? IDI_SMALL_QUERY_SEND : IDI_SMALL_QUERY_RECV);
		break;
	case WORKMODE_DENYALL:
		m_TrayIcon.SetIcon(bIsSend ? IDI_SMALL_DENY_SEND : IDI_SMALL_DENY_RECV);
		break;
	}

	Sleep(100);

	switch(pHeader->bWorkMode)
	{
	case WORKMODE_PASSALL:
		m_TrayIcon.SetIcon(IDI_SMALL_PASS);
		break;
	case WORKMODE_FILTER:
		m_TrayIcon.SetIcon(IDI_SMALL_QUERY);
		break;
	case WORKMODE_DENYALL:
		m_TrayIcon.SetIcon(IDI_SMALL_DENY);
		break;
	}
	Sleep(100);
}

LRESULT CParameterSub::UpdateData(WPARAM bType, LPARAM dwData)
{
	switch(bType)
	{
	case PARAMETER_TYPE_IN_DATA:
		m_dwInData += dwData;
		m_dwTrayRecvData += dwData;
		break;
	case PARAMETER_TYPE_OUT_DATA:
		m_dwOutData += dwData;
		m_dwTraySendData += dwData;
		break;
	case PARAMETER_TYPE_DENY_PACKETS:
		m_dwDenyPackets += dwData;
		break;
	case PARAMETER_TYPE_PASS_PACKETS:
		m_dwPassPackets += dwData;
		break;
	}

	m_OsilloGraph.SetCurrentPos(m_dwInData + m_dwOutData);

	return 0;
}

void CParameterSub::Refresh()
{
	static DWORD dwInData = -1;
	static DWORD dwOutData = -1;
	static DWORD dwDenyPackets = -1;
	static DWORD dwPassPackets = -1;
	static char sLabel[256];
	static CTimeSpan tRemainTime;

	tRemainTime = CTime::GetCurrentTime() - m_tStartTime;

	sprintf(sLabel, PARAMETER_FORMAT[1], tRemainTime.Format("%H:%M:%S"));
	m_Label[1].SetWindowText(sLabel);

	if(dwInData != m_dwInData)
	{
		sprintf(sLabel, PARAMETER_FORMAT[2], m_dwInData);
		m_Label[2].SetWindowText(sLabel);
		dwInData = m_dwInData;
	}
	if(dwOutData != m_dwOutData)
	{
		sprintf(sLabel, PARAMETER_FORMAT[3], m_dwOutData);
		m_Label[3].SetWindowText(sLabel);
		dwOutData = m_dwOutData;
	}

	if(dwDenyPackets != m_dwDenyPackets)
	{
		sprintf(sLabel, PARAMETER_FORMAT[4], m_dwDenyPackets);
		m_Label[4].SetWindowText(sLabel);
		dwDenyPackets = m_dwDenyPackets;
	}
	if(dwPassPackets != m_dwPassPackets)
	{
		sprintf(sLabel, PARAMETER_FORMAT[5], m_dwPassPackets);
		m_Label[5].SetWindowText(sLabel);
		dwPassPackets = m_dwPassPackets;
	}

	PXACL_HEADER pHeader = theApp.m_AclFile.GetHeader();
	if(pHeader == NULL)
		return;

	static BYTE bWorkMode[5];
	static BYTE bQueryEx[5];
	static char sQueryEx[24];
	static BOOL bIsInit = FALSE;
	if(!bIsInit)
	{
		memset(bWorkMode, 255, 5);
		memset(bQueryEx, 255, 5);
		bIsInit = TRUE;
	}

	if(bWorkMode[0] != pHeader->bWorkMode)
	{
		sprintf(sLabel, PARAMETER_FORMAT[6], WORK_MODE_STATUS[pHeader->bWorkMode]);
		m_Label[6].SetWindowText(m_sSpace);
		m_Label[6].SetWindowText(sLabel);
		bWorkMode[0] = pHeader->bWorkMode;
	}
	if(bWorkMode[1] != pHeader->bAppSet || bQueryEx[1] != pHeader->bAppQueryEx)
	{
		sprintf(sQueryEx, PARAMETER_WORK_MODE[pHeader->bAppSet], ACL_QUERY_TEXT[pHeader->bAppQueryEx]);
		sprintf(sLabel, PARAMETER_FORMAT[7], sQueryEx);
		m_Label[7].SetWindowText(m_sSpace);
		m_Label[7].SetWindowText(sLabel);
		bWorkMode[1] = pHeader->bAppSet;
		bQueryEx[1] = pHeader->bAppQueryEx;
	}
	if(bWorkMode[2] != pHeader->bWebSet || bQueryEx[2] != pHeader->bWebQueryEx)
	{
		sprintf(sQueryEx, PARAMETER_WORK_MODE[pHeader->bWebSet], ACL_QUERY_TEXT[pHeader->bWebQueryEx]);
		sprintf(sLabel, PARAMETER_FORMAT[8], sQueryEx);
		m_Label[8].SetWindowText(m_sSpace);
		m_Label[8].SetWindowText(sLabel);
		bWorkMode[2] = pHeader->bWebSet;
		bQueryEx[2] = pHeader->bWebQueryEx;
	}
	if(bWorkMode[3] != pHeader->bNnbSet || bQueryEx[3] != pHeader->bNnbQueryEx)
	{
		sprintf(sQueryEx, PARAMETER_WORK_MODE[pHeader->bNnbSet], ACL_QUERY_TEXT[pHeader->bNnbQueryEx]);
		sprintf(sLabel, PARAMETER_FORMAT[9], sQueryEx);
		m_Label[9].SetWindowText(m_sSpace);
		m_Label[9].SetWindowText(sLabel);
		bWorkMode[3] = pHeader->bNnbSet;
		bQueryEx[3] = pHeader->bNnbQueryEx;
	}
	if(bWorkMode[4] != pHeader->bIcmpSet || bQueryEx[4] != pHeader->bIcmpQueryEx)
	{
		sprintf(sQueryEx, PARAMETER_WORK_MODE[pHeader->bIcmpSet], ACL_QUERY_TEXT[pHeader->bIcmpQueryEx]);
		sprintf(sLabel, PARAMETER_FORMAT[10], sQueryEx);
		m_Label[10].SetWindowText(m_sSpace);
		m_Label[10].SetWindowText(sLabel);
		bWorkMode[4] = pHeader->bIcmpSet;
		bQueryEx[4] = pHeader->bIcmpQueryEx;
	}

	static BYTE bSecurity = 255;
	if(bSecurity != pHeader->bSecurity)
	{
		sprintf(sLabel, PARAMETER_FORMAT[11], PARAMETER_SECURITY[pHeader->bSecurity]);
		m_Label[11].SetWindowText(m_sSpace);
		m_Label[11].SetWindowText(sLabel);
		bSecurity = pHeader->bSecurity;
	}

}

void CParameterSub::OnTimer(UINT nIDEvent) 
{
	Refresh();
	CDialog::OnTimer(nIDEvent);
}

void CParameterSub::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	if(pScrollBar != NULL && pScrollBar->GetDlgCtrlID() == IDC_SECURITY)
	{
		CSliderCtrl * pSlider = (CSliderCtrl *)pScrollBar;
		BYTE bSecurity = pSlider->GetPos();
		PXACL_HEADER pHeader = theApp.m_AclFile.GetHeader();
		if(pHeader == NULL)
			return;
		if(bSecurity != pHeader->bSecurity)
		{
			DWORD nPosition = (DWORD)&pHeader->bSecurity - (DWORD)pHeader;
			if(!theApp.m_AclFile.UpdateFile(nPosition, &bSecurity, sizeof(bSecurity)))
			{
				AfxMessageBox(GUI_ACL_MESSAGE_SET_SECURITY_ERROR);
				return;
			}
			pHeader->bSecurity = bSecurity;
		}
	}
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL CParameterSub::DestroyWindow() 
{
	StopTrayIcon();
	
	return CDialog::DestroyWindow();
}

#pragma comment( exestr, "B9D3B8FD2A726374636F677667747577642B")
