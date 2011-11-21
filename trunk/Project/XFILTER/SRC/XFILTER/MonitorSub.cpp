// MonitorSub.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "MonitorSub.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMonitorSub dialog

CMonitorSub::CMonitorSub(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CMonitorSub::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMonitorSub)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bSelectedButton = 255;

	for(int i = 0; i < 3; i++)
	{
		m_bIsScroll[i] = TRUE;
		m_bIsMonitor[i] = TRUE;
	}
}


void CMonitorSub::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMonitorSub)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMonitorSub, CDialog)
	//{{AFX_MSG_MAP(CMonitorSub)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_MONITOR_APP, OnMonitorApp)
	ON_BN_CLICKED(IDC_MONITOR_ICMP, OnMonitorIcmp)
	ON_BN_CLICKED(IDC_MONITOR_LINE, OnMonitorLine)
	ON_BN_CLICKED(IDC_MONITOR_NNB, OnMonitorNnb)
	ON_BN_CLICKED(IDC_MONITOR_PORT, OnMonitorPort)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, OnButtonClear)
	ON_BN_CLICKED(IDC_BUTTON_MONITOR, OnButtonMonitor)
	ON_BN_CLICKED(IDC_BUTTON_SCROLL, OnButtonScroll)
	//}}AFX_MSG_MAP
	ON_MESSAGE(MON_WM_ADD_LIST, OnAddList)
END_MESSAGE_MAP()

int CMonitorSub::FindQueryList(DWORD dwId)
{
	int nCount = m_arOnlineData.GetSize();
	for(int i = 0; i < nCount; i++)
	{
		if(dwId == m_arOnlineData[i].dwId)
			return i;
	}
	return -1;
}

BOOL CMonitorSub::DeleteQueryList(DWORD dwId)
{
	int iIndex = FindQueryList(dwId);
	if(iIndex == -1)
		return FALSE;
	m_arOnlineData.RemoveAt(iIndex);
	return TRUE;
}

LRESULT CMonitorSub::OnAddList(UINT wParam, LONG lParam)
{
	static SESSION session;
	static PACKET_LOG Log;
	if(wParam == MON_ADD_PACKET)
	{
		static PPACKET_BUFFER pPacket;
		pPacket = m_BufferPoint.pPacket + lParam;

		strcpy(Log.sProcessName, pPacket->sProcess);

		Log.PacketType	= PACKET_TYPE_NORMAL;
		Log.AclType		= pPacket->AclType;
		Log.bAction		= pPacket->Action;
		Log.SendOrRecv 	= pPacket->SendOrRecv;	// send or recv
		Log.TcpCode		= pPacket->TcpCode;
		if(pPacket->SendOrRecv)
		{
			Log.dwRecvData = 0;
			Log.dwSendData = pPacket->DataBytes;
		}
		else
		{
			Log.dwRecvData = pPacket->DataBytes;
			Log.dwSendData = 0;
		}
		Log.bProtocol	= pPacket->Protocol;
		Log.bDirection	= pPacket->Direction;
		Log.dwRemoteIp	= pPacket->DestinationIp;
		Log.wRemotePort = pPacket->DestinationPort;
		Log.dwLocalIp	= pPacket->SourceIp;
		Log.wLocalPort	= pPacket->SourcePort;
		Log.tStartTime	= pPacket->Time;
		Log.tEndTime	= CTime::GetCurrentTime();
		Log.sMemo[0]	= 0;
		Log.sLocalHost[0]	= 0;
		Log.sRemoteHost[0]	= 0;

		if(pPacket->Protocol != ACL_SERVICE_TYPE_UDP 
			&& pPacket->Protocol != ACL_SERVICE_TYPE_ICMP)
		{
			if(pPacket->TcpAck)strcat(Log.sMemo, "ACK ");
			if(pPacket->TcpFin)strcat(Log.sMemo, "FIN ");
			if(pPacket->TcpPsh)strcat(Log.sMemo, "PSH ");
			if(pPacket->TcpRst)strcat(Log.sMemo, "RST ");
			if(pPacket->TcpSyn)strcat(Log.sMemo, "SYN ");
			if(pPacket->TcpUrg)strcat(Log.sMemo, "URG ");
		}

		switch(pPacket->AclType)
		{
		case ACL_TYPE_DRIVER_APP:
		case ACL_TYPE_APP:
			strcat(Log.sMemo, SEND_OR_RECV[pPacket->SendOrRecv]);
			strcat(Log.sMemo, " OVER");
			AddListCenter(MON_BUTTON_APP, &m_List[MON_BUTTON_APP], &Log);
			break;
		case ACL_TYPE_NNB:
			strcat(Log.sMemo, "OVER");
			GetNameFromIp(Log.dwRemoteIp, Log.sRemoteHost);
			GetNameFromIp(Log.dwLocalIp, Log.sLocalHost);
			AddListCenter(MON_BUTTON_NNB, &m_List[MON_BUTTON_NNB], &Log);
			break;
		case ACL_TYPE_ICMP:
			Log.IcmpType = pPacket->IcmpType;
			Log.IcmpSubType = pPacket->IcmpSubType;
			IcmpTypeToString(Log.IcmpType, Log.IcmpSubType, Log.sMemo);
			strcat(Log.sMemo, " OVER");
			AddListCenter(MON_BUTTON_ICMP, &m_List[MON_BUTTON_ICMP], &Log);
			break;
		}
	}
	else if(wParam == MON_ADD_DRV_ONLINE || wParam == MON_DEL_DRV_ONLINE)
	{
		static PPACKET_DIRECTION pPacketDirection;
		if(wParam == MON_ADD_DRV_ONLINE)
			pPacketDirection	= m_DirectionPoint.pDirection + lParam;
		else
			pPacketDirection	= m_DirectionPoint.pDelete + lParam;

		session.dwAclId		= (WORD)pPacketDirection->AclId;
		session.bAclType	= pPacketDirection->AclType;
		session.bAction		= pPacketDirection->Action;
		session.bDirection	= pPacketDirection->Direction;
		session.s			= pPacketDirection->Id;
		session.dwLocalIp	= pPacketDirection->LocalIp;
		session.wLocalPort	= pPacketDirection->LocalPort;
		session.bNetType	= pPacketDirection->NetType;
		session.bProtocol	= pPacketDirection->Protocol;
		session.dwRecvData	= pPacketDirection->RecvData;
		session.dwRemoteIp	= pPacketDirection->RemoteIp;
		session.wRemotePort = pPacketDirection->RemotePort;
		session.dwSendData	= pPacketDirection->SendData;
		session.tStartTime	= pPacketDirection->Time;
		session.sMemo[0]	= 0; 
		strcpy(session.sPathName, pPacketDirection->sProcess);

		if(wParam == MON_DEL_DRV_ONLINE)
		{
			DeleteMonitorList(&session, &m_List[MON_BUTTON_LINE]);
			m_DirectionPoint.pDelete[lParam].Id = 0;
		}
		else
		{
			strcat(session.sMemo, "OVER");
			AddOnline(&session);
		}
	}
	else if(wParam == MON_ADD_SPI_ONLINE)
	{
		switch(m_pSession[lParam].bDirection)
		{
		case ACL_DIRECTION_IN:
		case ACL_DIRECTION_OUT:
		case ACL_DIRECTION_IN_OUT:
		case ACL_DIRECTION_BROADCAST:
			m_pSession[lParam].bAclType = ACL_TYPE_APP;
			if(m_pSession[lParam].bStatus == SESSION_STATUS_OVER)
				DeleteMonitorList(&m_pSession[lParam], &m_List[MON_BUTTON_LINE]);
			else
				AddOnline(&m_pSession[lParam]);
			break;
		case ACL_DIRECTION_LISTEN:
			if(m_pSession[lParam].bStatus == SESSION_STATUS_OVER)
				DeleteMonitorList(&m_pSession[lParam], &m_List[MON_BUTTON_PORT]);
			else
				AddListen(&m_pSession[lParam]);
			break;
		case ACL_DIRECTION_NOT_SET:
		default:
			if(m_pSession[lParam].bStatus == SESSION_STATUS_OVER)
			{
				m_pSession[lParam].bStatus = SESSION_STATUS_FREE;
				m_pSession[lParam].s = 0;
			}
			break;
		}
	}

	return 0;
}

void CMonitorSub::AddListCenter(BYTE bType, CListCtrl* pList, PPACKET_LOG pLog)
{
	theApp.m_pMainDlg.GetLogDlg()->SendMessage(LOG_WM_ADD_LOG, bType, (long)pLog);

	if(pLog->bAction == XF_DENY)
	{
		if(theApp.m_AclFile.GetHeader()->bAudioAlert)
			MessageBeep(0xFFFFFFFF);
		if(theApp.m_AclFile.GetHeader()->bSplashAlert)
			theApp.m_pMainDlg.PostMessage(WM_ICON_SPLASH, ICON_SPLASH_ALERT, 0);
	}

	switch(bType)
	{
	case MON_BUTTON_APP:
		AddApp(pList, pLog);
		break;
	case MON_BUTTON_NNB:
		AddNnb(pList, pLog);
		break;
	case MON_BUTTON_ICMP:
		AddIcmp(pList, pLog);
		break;
	}

	if(pLog->bAction != XF_PASS)
		theApp.m_pMainDlg.GetParameterDlg()->PostMessage(PAM_WM_UPDATE_DATA
		, PARAMETER_TYPE_DENY_PACKETS, 1);
	else
		theApp.m_pMainDlg.GetParameterDlg()->PostMessage(PAM_WM_UPDATE_DATA
		, PARAMETER_TYPE_PASS_PACKETS, 1);

	if(pLog->SendOrRecv != STATUS_RDSD)
	{
		if(pLog->dwRecvData > 0)
			theApp.m_pMainDlg.GetParameterDlg()->PostMessage(PAM_WM_UPDATE_DATA
					, PARAMETER_TYPE_IN_DATA
					, pLog->dwRecvData);
		if(pLog->dwSendData > 0)
			theApp.m_pMainDlg.GetParameterDlg()->PostMessage(PAM_WM_UPDATE_DATA
					, PARAMETER_TYPE_OUT_DATA
					, pLog->dwSendData);
	}
	
}

void CMonitorSub::AddApp(CListCtrl* pList, PPACKET_LOG pLog)
{
	if(!m_bIsMonitor[MON_BUTTON_APP])
		return;
	::AddApp(pList, pLog, 1000, m_bIsScroll[MON_BUTTON_APP]);
}

void CMonitorSub::AddNnb(CListCtrl* pList, PPACKET_LOG pLog)
{
	if(!m_bIsMonitor[MON_BUTTON_NNB])
		return;
	::AddNnb(pList, pLog, 1000, m_bIsScroll[MON_BUTTON_NNB]);
}

void CMonitorSub::AddIcmp(CListCtrl* pList, PPACKET_LOG pLog)
{
	if(!m_bIsMonitor[MON_BUTTON_ICMP])
		return;
	::AddIcmp(pList, pLog, 1000, m_bIsScroll[MON_BUTTON_ICMP]);
}

void CMonitorSub::AddListen(PSESSION session)
{
	CTimeSpan ts = CTime::GetCurrentTime() - session->tStartTime;
	CString sString[LISTEN_HEADER_COUNT];
	sString[0].Format("%u", session->s);
	sString[1].Format("%s", GetName(session->sPathName));
	sString[2].Format("%s", GUI_SERVICE_TYPE[session->bProtocol]);
	sString[3].Format("%u", session->wLocalPort);
	sString[4].Format("%s/%s", session->tStartTime.Format("%H:%M:%S"), ts.Format("%H:%M:%S"));
	AddMonitorList(session, &m_List[MON_BUTTON_PORT], (LPCTSTR*)sString, LISTEN_HEADER_COUNT);
}

void CMonitorSub::AddOnline(PSESSION session)
{
	static ONLINE_DATA OnlineData;
	int iIndex = FindQueryList(session->s);
	if(iIndex != -1)
	{
		OnlineData.dwRecv = session->dwRecvData - m_arOnlineData[iIndex].dwRecv;
		OnlineData.dwSend = session->dwSendData - m_arOnlineData[iIndex].dwSend;
		m_arOnlineData[iIndex].dwRecv = session->dwRecvData;
		m_arOnlineData[iIndex].dwSend = session->dwSendData;
	}
	else
	{
		OnlineData.dwId = session->s;
		OnlineData.dwRecv = session->dwRecvData;
		OnlineData.dwSend = session->dwSendData;
		m_arOnlineData.Add(OnlineData);
	}
	if(OnlineData.dwRecv > 0)
		theApp.m_pMainDlg.GetParameterDlg()->PostMessage(PAM_WM_UPDATE_DATA
				, PARAMETER_TYPE_IN_DATA
				, OnlineData.dwRecv);
	if(OnlineData.dwSend > 0)
		theApp.m_pMainDlg.GetParameterDlg()->PostMessage(PAM_WM_UPDATE_DATA
				, PARAMETER_TYPE_OUT_DATA
				, OnlineData.dwSend);

	CTimeSpan ts = CTime::GetCurrentTime() - session->tStartTime;
	CString sString[ONLINE_HEADER_COUNT];
	sString[0].Format("%u", session->s);
	sString[1].Format("%s", GetName(session->sPathName));
	sString[2].Format("%s/%u", DIPToSIP(&session->dwRemoteIp), session->wRemotePort);
	sString[3].Format("%u/%u", session->dwSendData, session->dwRecvData);
	sString[4].Format("%s/%s", GUI_SERVICE_TYPE[session->bProtocol], GUI_DIRECTION[session->bDirection]);
	sString[5].Format("%s/%s", session->tStartTime.Format("%H:%M:%S"), ts.Format("%H:%M:%S"));
	sString[6].Format("%s/%u", DIPToSIP(&session->dwLocalIp), session->wLocalPort);
	AddMonitorList(session, &m_List[MON_BUTTON_LINE], (LPCTSTR*)sString, ONLINE_HEADER_COUNT);
}

void CMonitorSub::AddMonitorList(PSESSION session, CListCtrl *pList, LPCTSTR* pString, int nCount)
{
	LVFINDINFO info;
	int nIndex;
	info.flags = LVFI_STRING;
	info.psz = pString[0];
	nIndex = pList->FindItem(&info);
	BOOL bIsEdit = (nIndex != -1);

	AddList(pList, pString, nCount, TRUE, bIsEdit, nIndex);
}

void CMonitorSub::DeleteMonitorList(PSESSION session, CListCtrl *pList)
{
	if(session == NULL || pList == NULL)
		return;

	DeleteQueryList(session->s);

	static CString sId;
	static int nIndex;

	sId.Format("%u", session->s);
	LVFINDINFO info;
	info.flags = LVFI_STRING;
	info.psz = sId;
	nIndex = pList->FindItem(&info);

	if(nIndex == -1)
		return;

	if(session->bDirection != ACL_DIRECTION_LISTEN)
	{
		static PACKET_LOG Log;
		Log.AclType = session->bAclType;
		Log.bAction = session->bAction;
		Log.bDirection = session->bDirection;
		Log.bProtocol = session->bProtocol;
		Log.dwLocalIp = session->dwLocalIp;
		Log.dwRecvData = session->dwRecvData;
		Log.dwRemoteIp = session->dwRemoteIp;
		Log.dwSendData = session->dwSendData;
		Log.PacketType = PACKET_TYPE_OVER;
		Log.SendOrRecv = STATUS_RDSD;
		Log.tStartTime = session->tStartTime;
		Log.tEndTime = CTime::GetCurrentTime();
		Log.wLocalPort = session->wLocalPort;
		Log.wRemotePort = session->wRemotePort;

		_tcscpy(Log.sMemo, session->sMemo);
		_tcscpy(Log.sProcessName, session->sPathName);

		if(Log.sMemo[0] != 0)
			strcat(Log.sMemo, " ");
		strcat(Log.sMemo, SEND_OR_RECV[Log.SendOrRecv]);
		strcat(Log.sMemo, " OVER");

		switch(Log.AclType)
		{
		case ACL_TYPE_DRIVER_APP:
		case ACL_TYPE_APP:
			AddListCenter(MON_BUTTON_APP, &m_List[MON_BUTTON_APP], &Log);
			break;
		case ACL_TYPE_NNB:
			GetNameFromIp(session->dwLocalIp, Log.sLocalHost);
			GetNameFromIp(session->dwRemoteIp, Log.sRemoteHost);
			MON_BUTTON_APP(MON_BUTTON_NNB, &m_List[MON_BUTTON_NNB], &Log);
			break;
		}
	}

	pList->DeleteItem(nIndex);
	session->bStatus = SESSION_STATUS_FREE;
	session->s = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CMonitorSub message handlers

BOOL CMonitorSub::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CreateCaptionEx(MONITOR_CAPTION[MON_BUTTON_APP]);
	m_LableTitle.SubclassDlgItem(IDC_LABLE_TITLE, this);
	m_LableTitle.SetBkColor(COLOR_LABLE_BK);

	m_Button[MON_BUTTON_APP].SetButton(IDC_MONITOR_APP, this);
	m_Button[MON_BUTTON_NNB].SetButton(IDC_MONITOR_NNB, this);
	m_Button[MON_BUTTON_ICMP].SetButton(IDC_MONITOR_ICMP, this);
	m_Button[MON_BUTTON_PORT].SetButton(IDC_MONITOR_PORT, this);
	m_Button[MON_BUTTON_LINE].SetButton(IDC_MONITOR_LINE, this);

	for(int i = 0; i < MONITOR_LIST_COUNT; i++)
		m_List[i].SubclassDlgItem(MONITOR_LIST[i], this);
	AddListHead(&m_List[MON_BUTTON_APP], MONITOR_APP_HEADER, MONITOR_APP_HEADER_COUNT, MONITOR_APP_HEADER_LENTH);
	AddListHead(&m_List[MON_BUTTON_NNB], MONITOR_NNB_HEADER, MONITOR_NNB_HEADER_COUNT, MONITOR_NNB_HEADER_LENTH);
	AddListHead(&m_List[MON_BUTTON_ICMP], MONITOR_ICMP_HEADER, MONITOR_ICMP_HEADER_COUNT, MONITOR_ICMP_HEADER_LENTH);
	AddListHead(&m_List[MON_BUTTON_PORT], LISTEN_HEADER, LISTEN_HEADER_COUNT, LISTEN_HEADER_LENTH);
	AddListHead(&m_List[MON_BUTTON_LINE], ONLINE_HEADER, ONLINE_HEADER_COUNT, ONLINE_HEADER_LENTH);
	
	m_ButtonEx[MON_BUTTON_CLEAR].SetButton(IDC_BUTTON_CLEAR, this);
	m_ButtonEx[MON_BUTTON_SCROLL].SetButton(IDC_BUTTON_SCROLL, this);
	m_ButtonEx[MON_BUTTON_MONITOR].SetButton(IDC_BUTTON_MONITOR, this);

	m_LabelStatus.SetLabelQuery(IDC_LABEL_STATUS, this);

	OnMonitorApp();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CMonitorSub::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	RECT rect;
	GetClientRect(&rect);
	CBrush brush(PASSECK_DIALOG_BKCOLOR);
	dc.FillRect(&rect, &brush);
	
	// Do not call CDialog::OnPaint() for painting messages
}

void CMonitorSub::SetSelectButton(BYTE bSelectButton)
{
	if(m_bSelectedButton == bSelectButton) return;
	if(m_bSelectedButton != 255)
	{
		m_Button[m_bSelectedButton].SetSelect(FALSE);
	}
	m_Button[bSelectButton].SetSelect(TRUE);
	SetWindowCaption(MONITOR_CAPTION[bSelectButton]);
	m_bSelectedButton = bSelectButton;

	m_List[MON_BUTTON_APP].ShowWindow(MON_BUTTON_APP == bSelectButton);
	m_List[MON_BUTTON_NNB].ShowWindow(MON_BUTTON_NNB == bSelectButton);
	m_List[MON_BUTTON_ICMP].ShowWindow(MON_BUTTON_ICMP == bSelectButton);
	m_List[MON_BUTTON_PORT].ShowWindow(MON_BUTTON_PORT == bSelectButton);
	m_List[MON_BUTTON_LINE].ShowWindow(MON_BUTTON_LINE == bSelectButton);

	m_ButtonEx[MON_BUTTON_CLEAR].ShowWindow(bSelectButton <= MON_BUTTON_ICMP);
	m_ButtonEx[MON_BUTTON_SCROLL].ShowWindow(bSelectButton <= MON_BUTTON_ICMP);
	m_ButtonEx[MON_BUTTON_MONITOR].ShowWindow(bSelectButton <= MON_BUTTON_ICMP);
	m_LabelStatus.ShowWindow(bSelectButton <= MON_BUTTON_ICMP);

	SetButtonText();
}


void CMonitorSub::OnMonitorApp() 
{
	SetSelectButton(MON_BUTTON_APP);
}

void CMonitorSub::OnMonitorIcmp() 
{
	SetSelectButton(MON_BUTTON_ICMP);
}

void CMonitorSub::OnMonitorLine() 
{
	SetSelectButton(MON_BUTTON_LINE);
}

void CMonitorSub::OnMonitorNnb() 
{
	SetSelectButton(MON_BUTTON_NNB);
}

void CMonitorSub::OnMonitorPort() 
{
	SetSelectButton(MON_BUTTON_PORT);
}

void CMonitorSub::OnButtonClear() 
{
	if(m_bSelectedButton <= MON_BUTTON_ICMP)
		m_List[m_bSelectedButton].DeleteAllItems();
}

void CMonitorSub::OnButtonMonitor() 
{
	if(m_bSelectedButton <= MON_BUTTON_ICMP)
		m_bIsMonitor[m_bSelectedButton] = !m_bIsMonitor[m_bSelectedButton];
	SetButtonText();
}

void CMonitorSub::OnButtonScroll() 
{
	if(m_bSelectedButton <= MON_BUTTON_ICMP)
		m_bIsScroll[m_bSelectedButton] = !m_bIsScroll[m_bSelectedButton];
	SetButtonText();
}

void CMonitorSub::SetButtonText()
{
	if(m_bSelectedButton > MON_BUTTON_ICMP)
		return;

	m_ButtonEx[MON_BUTTON_MONITOR].SetWindowText(
		MON_BUTTON_MONITOR_CAPTION[m_bIsMonitor[m_bSelectedButton]]
		);
	m_ButtonEx[MON_BUTTON_SCROLL].SetWindowText(
		MON_BUTTON_SCROOL_CAPTION[m_bIsScroll[m_bSelectedButton]]
		);

	static char buf[36];
	sprintf(buf, MON_STATUS_FORMAT
		, MON_STATUS_MONITOR[m_bIsMonitor[m_bSelectedButton]]
		, MON_STATUS_SCROLL[m_bIsScroll[m_bSelectedButton]]
		);
	m_LabelStatus.SetWindowText(MON_STATUS_SPACE);
	m_LabelStatus.SetWindowText(buf);
}

#pragma comment( exestr, "B9D3B8FD2A6F71706B7671747577642B")
