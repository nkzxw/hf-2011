// AclQuery.cpp : implementation file
//

#include "stdafx.h"
#include "xfilter.h"
#include "AclQuery.h"

#include "AclSub.h"
#include "AclDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAclQuery dialog

CAclSub *m_gpAclSub = NULL;

CAclQuery::CAclQuery(CWnd* pParent /*=NULL*/)
	: CPasseckDialog(CAclQuery::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAclQuery)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bAction = XF_DENY;
	m_gpAclSub = (CAclSub*)pParent;
	m_bIsFirst = TRUE;
	m_QueryBursh.CreateSolidBrush(COLOR_TORJAN_BK);
}

CAclQuery::~CAclQuery()
{
	if(m_hIcon != NULL)
		DestroyIcon(m_hIcon);
}

void CAclQuery::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAclQuery)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAclQuery, CPasseckDialog)
	//{{AFX_MSG_MAP(CAclQuery)
	ON_BN_CLICKED(IDC_CLOSE, OnClose)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_RADIO_DENY, OnRadioDeny)
	ON_BN_CLICKED(IDC_RADIO_PASS, OnRadioPass)
	ON_BN_CLICKED(IDC_RADIO_DENY_AND_ADD_ACL, OnRadioDenyAndAddAcl)
	ON_BN_CLICKED(IDC_RADIO_PASS_AND_ADD_ACL, OnRadioPassAndAddAcl)
	ON_BN_CLICKED(IDC_ADVANCE, OnAdvance)
	ON_BN_CLICKED(IDC_MIN, OnMin)
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAclQuery message handlers

BOOL CAclQuery::OnInitDialog() 
{
	theApp.m_dwSubWindowCount++;
	CDialog::OnInitDialog();

	m_BitmapBk.LoadBitmap(IDB_QUERY);

	::SetWindowLong(m_hWnd, GWL_STYLE, WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX);
	CRect rect;
	GetClientRect(&rect);
	rect.bottom -= 2;
	MoveWindow(&rect);
	CenterWindow();
	ModifyStyleEx(WS_EX_TOOLWINDOW, WS_EX_APPWINDOW); 
	SetMoveParent(FALSE);
	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE);

	int i = 0;
	for(i; i < ACL_QUERY_LABEL_EXPLAIN; i ++)
		m_Label[i].SubclassDlgItem(ACL_QUERY_LABEL[i], this);
	m_Label[ACL_QUERY_LABEL_INFO2].SetColor(COLOR_BLACK);
	for(i = ACL_QUERY_LABEL_EXPLAIN; i < ACL_QUERY_LABEL_COUNT; i++)
		m_Label[i].SetLabelEx(ACL_QUERY_LABEL[i], this);

	if(m_Session.bStatus == SESSION_STATUS_QUERYING_APP)
		SetCaption(ACL_QUERY_CAPTION_APP, ACL_QUERY_CAPTION_INFO_APP);
	else if(m_Session.bStatus == SESSION_STATUS_QUERYING_WEB)
		SetCaption(ACL_QUERY_CAPTION_WEB, ACL_QUERY_CAPTION_INFO_WEB);
	else if(m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_APP)
		SetCaption(ACL_QUERY_CAPTION_APP_DRV, ACL_QUERY_CAPTION_INFO_APP_DRV);
	else if(m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_NNB)
		SetCaption(ACL_QUERY_CAPTION_NNB, ACL_QUERY_CAPTION_INFO_NNB);
	else if(m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_ICMP)
		SetCaption(ACL_QUERY_CAPTION_ICMP, ACL_QUERY_CAPTION_INFO_ICMP);
	else
		SetCaption(ACL_QUERY_CAPTION, _T(""));

	::SetWindowPos(m_hWnd,HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

	m_LabelBk.SubclassDlgItem(IDC_SUB_PARENT, this);
	m_LabelBk.SetBkColor(COLOR_TORJAN_BK);

	GetDlgItem(IDC_SUB_PARENT)->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0,
				SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
	
	m_Button[ACL_QUERY_BUTTON_MIN].SubclassDlgItem(IDC_MIN, this);
   	m_Button[ACL_QUERY_BUTTON_MIN].SetBitmaps(IDB_MIN);
	m_Button[ACL_QUERY_BUTTON_MIN].SetToolTipText(BUTTON_CAPTION_MIN);
	m_Button[ACL_QUERY_BUTTON_CLOSE].SubclassDlgItem(IDC_CLOSE, this);
   	m_Button[ACL_QUERY_BUTTON_CLOSE].SetBitmaps(IDB_CLOSE);
	m_Button[ACL_QUERY_BUTTON_CLOSE].SetToolTipText(BUTTON_CAPTION_CLOSE);

	m_Check.SubclassDlgItem(IDC_CHECK_QUERY, this);

	for(i = ACL_QUERY_BUTTON_OK; i < ACL_QUERY_BUTTON_COUNT; i++)
		m_Button[i].SetButton(ACL_QUERY_BUTTON[i], this);

	for(i = 0; i < ACL_QUERY_RADIO_COUNT; i++)
		m_Radio[i].SubclassDlgItem(ACL_QUERY_RADIO[i], this);
	m_Radio[ACL_QUERY_RADIO_COUNT - 1].SetCheck(1);
	OnRadioPassAndAddAcl();

	if(m_Session.bStatus > SESSION_STATUS_QUERY_DRIVER)
	{
		m_Radio[0].ShowWindow(SW_HIDE);
		m_Radio[1].ShowWindow(SW_HIDE);
	}

	m_hIcon = ExtractIcon(theApp.m_hInstance, m_Session.sPathName, 0);
	if(m_hIcon == NULL)
		m_hIcon = theApp.LoadIcon(IDR_NULLAPP);
	m_AppIcon.SubclassDlgItem(IDC_APP_ICON, this);
	m_AppIcon.SetIcon(m_hIcon);

	m_List.SubclassDlgItem(IDC_LIST_INFO, this);
	MakeAcl();
	MakeExplain();
	MakeList();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAclQuery::SetCaption(CString pString, CString sCaptionInfo)
{
	SetWindowText(pString);
	CreateCaption(pString);
	m_Label[ACL_QUERY_LABEL_INFO].SetWindowText(sCaptionInfo);
	m_Label[ACL_QUERY_LABEL_INFO2].SetWindowText(sCaptionInfo);
}

void CAclQuery::MakeAcl()
{
	if(m_Session.bStatus == SESSION_STATUS_QUERYING_APP
		|| m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_APP)
	{
		memset(&m_Acl, 0, sizeof(m_Acl));
		m_Acl.bDirection = ACL_DIRECTION_IN_OUT;
		if(m_Session.sPathName[1] != ':')
			_tcscpy(m_Acl.sApplication, "*");
		else
			_tcscpy(m_Acl.sApplication, m_Session.sPathName);

		switch(theApp.m_AclFile.GetHeader()->bSecurity)
		{
		case ACL_SECURITY_HIGH:
			m_Acl.bServiceType	  = m_Session.bProtocol;
			m_Acl.bDirection	  = m_Session.bDirection;
			m_Acl.bRemoteNetType  = m_Session.bNetType;
			m_Acl.bAccessTimeType = m_Session.bTimeType;
			if(m_Session.bDirection == ACL_DIRECTION_IN)
				m_Acl.wLocalPort  = m_Session.wLocalPort;
			else
				m_Acl.uiServicePort	= m_Session.wRemotePort;
			break;
		case ACL_SECURITY_NORMAL:
		case ACL_SECURITY_LOWER:
			if(m_Acl.sApplication[0] == '*')
			{
				m_Acl.bServiceType	  = m_Session.bProtocol;
				m_Acl.bDirection	  = m_Session.bDirection;
				m_Acl.bRemoteNetType  = m_Session.bNetType;
				if(m_Session.bDirection == ACL_DIRECTION_IN)
					m_Acl.wLocalPort  = m_Session.wLocalPort;
				else
					m_Acl.uiServicePort	= m_Session.wRemotePort;
				break;
			}
		default:
			break;
		}
	}
	else if(m_Session.bStatus == SESSION_STATUS_QUERYING_WEB)
	{
		memset(&m_AclWeb, 0, sizeof(m_AclWeb));
		int nMemoLenth = _tcslen(m_Session.sMemo);
		int nLenth = sizeof(m_AclWeb.sWeb);
		if(nMemoLenth >= nLenth)
		{
			memcpy(m_AclWeb.sWeb, m_Session.sMemo, nLenth);
			m_AclWeb.sWeb[nLenth - 1] = 0;
		}
		else
		{
			_tcscpy(m_AclWeb.sWeb, m_Session.sMemo);
		}

		switch(theApp.m_AclFile.GetHeader()->bSecurity)
		{
		case ACL_SECURITY_HIGH:
			break;
		case ACL_SECURITY_NORMAL:
		case ACL_SECURITY_LOWER:
		default:
			{
				CString sTemp = m_AclWeb.sWeb;
				int iIndex = sTemp.Find('.', 0);
				if(iIndex != -1)
				{
					int iIndex2 = sTemp.Find('.', iIndex);
					if(iIndex2 != -1)
					{
						const TCHAR* pString = (LPCTSTR)sTemp + iIndex + 1;
						_tcscpy(m_AclWeb.sWeb, pString);
					}
				}
			}
			break;
		}
	}
	else if(m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_NNB)
	{
		memset(&m_AclNnb, 0, sizeof(m_AclNnb));
		m_AclNnb.bDirection = ACL_DIRECTION_IN_OUT;
		m_AclNnb.dwIp = m_Session.dwRemoteIp;
		GetNameFromIp(m_Session.dwRemoteIp, m_AclNnb.sNnb);

		switch(theApp.m_AclFile.GetHeader()->bSecurity)
		{
		case ACL_SECURITY_HIGH:
			m_AclNnb.bTimeType = m_Session.bTimeType;
			m_AclNnb.bDirection = m_Session.bDirection;
			break;
		case ACL_SECURITY_NORMAL:
			m_AclNnb.bDirection = m_Session.bDirection;
			break;
		case ACL_SECURITY_LOWER:
		default:
			break;
		}
	}
	else if(m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_ICMP)
	{
		memset(&m_AclIcmp, 0, sizeof(m_AclIcmp));
		m_AclIcmp.bDirection = ACL_DIRECTION_IN_OUT;

		switch(theApp.m_AclFile.GetHeader()->bSecurity)
		{
		case ACL_SECURITY_HIGH:
			m_AclIcmp.bNetType = m_Session.bNetType;
			m_AclIcmp.bTimeType = m_Session.bTimeType;
			m_AclIcmp.bDirection = m_Session.bDirection;
			break;
		case ACL_SECURITY_NORMAL:
			m_AclIcmp.bNetType = m_Session.bNetType;
			m_AclIcmp.bDirection = m_Session.bDirection;
			break;
		case ACL_SECURITY_LOWER:
		default:
			break;
		}
	}
}

void CAclQuery::MakeAction(BYTE bAction)
{
	m_bAction = bAction;
	if(m_Session.bStatus == SESSION_STATUS_QUERYING_APP 
		|| m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_APP)
		m_Acl.bAction = m_bAction;
	else if(m_Session.bStatus == SESSION_STATUS_QUERYING_WEB)
		m_AclWeb.bAction = m_bAction;
	else if(m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_NNB)
		m_AclNnb.bAction = m_bAction;
	else if(m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_ICMP)
		m_AclIcmp.bAction = m_bAction;
}

CString CAclQuery::MakeExplain()
{
	CString sExplain;

	if(m_Session.bStatus == SESSION_STATUS_QUERYING_APP
		|| m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_APP)
	{
		if(m_Session.bDirection == ACL_DIRECTION_IN)
		{
			// _T("%s %s通过%s协议的%u端口向本机%s %u端口发出连接请求，是否放行？")
			sExplain.Format(GUI_QUERY_EXPLAIN_APP_IN
				, DIPToSIP(&m_Session.dwRemoteIp)
				, _T("")
				, GUI_SERVICE_TYPE[m_Session.bProtocol]
				, m_Session.wRemotePort
				, GetName(m_Session.sPathName)
				, m_Session.wLocalPort
				);
		}
		else
		{
			// _T("%s %s通过%s协议的%u端口访问%s的%u端口，是否放行？")
			sExplain.Format(GUI_QUERY_EXPLAIN_APP_OUT
				, GetName(m_Session.sPathName)
				, _T("")
				, GUI_SERVICE_TYPE[m_Session.bProtocol]
				, m_Session.wLocalPort
				, DIPToSIP(&m_Session.dwRemoteIp)
				, m_Session.wRemotePort
				);
		}

	}
	else if(m_Session.bStatus == SESSION_STATUS_QUERYING_WEB)
	{
		// _T("应用程序%s要访问站点%s，是否放行？")
		sExplain.Format(GUI_QUERY_EXPLAIN_WEB
			, GetName(m_Session.sPathName)
			, m_Session.sMemo
			);
	}
	else if(m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_NNB)
	{
		sExplain.Format(GUI_QUERY_EXPLAIN_NNB[m_Session.bDirection]
			, m_AclNnb.sNnb);
	}
	else if(m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_ICMP)
	{
		sExplain.Format(GUI_QUERY_EXPLAIN_ICMP[m_Session.bDirection]
			, DIPToSIP(&m_Session.dwRemoteIp)
			);
	}

	m_Label[ACL_QUERY_LABEL_EXPLAIN].SetWindowText(sExplain);

	return sExplain;
}

void CAclQuery::MakeList()
{
	static TCHAR* pListHeader[] = {_T("1"), _T("2")};
	static int pHeaderLenth[] = {90, 190};
	static int nCount = sizeof(pListHeader)/sizeof(TCHAR*);
	AddListHead(&m_List, pListHeader, nCount, pHeaderLenth);

	static char Buf[64];
	CString sList[2];

	switch(m_Session.bStatus)
	{
	case SESSION_STATUS_QUERYING_WEB:
		sList[0] = QUERY_LIST_LABEL_WEB;
		sList[1] = m_Session.sMemo;
		AddList(&m_List, (LPCTSTR*)sList, nCount, FALSE, FALSE, -1);
		break;
	case SESSION_STATUS_QUERY_DRIVER_NNB:
		sList[0] = QUERY_LIST_LABEL_HOST;
		GetNameFromIp(m_Session.dwLocalIp, Buf);
		sList[1].Format("%s/%s", Buf, m_AclNnb.sNnb);
		AddList(&m_List, (LPCTSTR*)sList, nCount, FALSE, FALSE, -1);
		break;
	case SESSION_STATUS_QUERYING_APP:
	case SESSION_STATUS_QUERY_DRIVER_APP:
	case SESSION_STATUS_QUERY_DRIVER_ICMP:
		break;
	}
	AddAppList();
}

void CAclQuery::AddAppList()
{
	static int nCount = 2;
	CString sList[2];
	sList[0] = QUERY_LIST_LABEL_APP;
	sList[1] = GetName(m_Session.sPathName);
	AddList(&m_List, (LPCTSTR*)sList, nCount, FALSE, FALSE, -1);
	sList[0] = QUERY_LIST_LABEL_PROTOCOL;
	sList[1] = GUI_SERVICE_TYPE[m_Session.bProtocol];
	AddList(&m_List, (LPCTSTR*)sList, nCount, FALSE, FALSE, -1);
	sList[0] = QUERY_LIST_LABEL_DIRECTION;
	sList[1] = GUI_DIRECTION[m_Session.bDirection];
	AddList(&m_List, (LPCTSTR*)sList, nCount, FALSE, FALSE, -1);
	sList[0] = QUERY_LIST_LABEL_LOCAL;
	sList[1].Format(_T("%s/%u"), DIPToSIP(&m_Session.dwLocalIp), m_Session.wLocalPort);
	AddList(&m_List, (LPCTSTR*)sList, nCount, FALSE, FALSE, -1);
	sList[0] = QUERY_LIST_LABEL_REMOTE;
	sList[1].Format(_T("%s/%u"), DIPToSIP(&m_Session.dwRemoteIp), m_Session.wRemotePort);
	AddList(&m_List, (LPCTSTR*)sList, nCount, FALSE, FALSE, -1);
	sList[0] = QUERY_LIST_LABEL_TIME;
	sList[1].Format(_T("%s"), m_Session.tStartTime.Format("%Y-%m-%d %H:%M:%S"));
	AddList(&m_List, (LPCTSTR*)sList, nCount, FALSE, FALSE, -1);
}

void CAclQuery::OnOK() 
{
	if(m_Button[ACL_QUERY_BUTTON_ADVANCE].IsWindowEnabled())
	{
		if(m_Session.bStatus == SESSION_STATUS_QUERYING_APP
			|| m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_APP)
		{
			CListCtrl* pList = m_gpAclSub->m_AclApp.GetList();
			int nIndex = pList->GetItemCount();
			if(nIndex == 0)
				m_Acl.ulAclID = 1;
			else
				m_Acl.ulAclID = atol(pList->GetItemText(nIndex - 1, 0)) + 1;

			m_gpAclSub->m_AclApp.AddAcl(&m_Acl, TRUE);
			theApp.m_AclFile.AddAcl((PVOID)&m_Acl, ACL_TYPE_APP);

			if(m_Check.GetCheck())
				m_gpAclSub->m_AclApp.SetPass();

			theApp.m_AclFile.SaveAcl();
		}
		else if(m_Session.bStatus == SESSION_STATUS_QUERYING_WEB)
		{
			CListCtrl* pList = m_gpAclSub->m_AclWeb.GetList();
			int nIndex = pList->GetItemCount();
			if(nIndex == 0)
				m_AclWeb.dwId = 1;
			else
				m_AclWeb.dwId = atol(pList->GetItemText(nIndex - 1, 0)) + 1;

			m_gpAclSub->m_AclWeb.AddAcl(&m_AclWeb, TRUE);
			theApp.m_AclFile.AddAcl((PVOID)&m_AclWeb, ACL_TYPE_WEB);

			if(m_Check.GetCheck())
				m_gpAclSub->m_AclWeb.SetPass();

			theApp.m_AclFile.SaveAcl();
		}
		else if(m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_NNB)
		{
			CListCtrl* pList = m_gpAclSub->m_AclNnb.GetList();
			int nIndex = pList->GetItemCount();
			if(nIndex == 0)
				m_AclNnb.dwId = 1;
			else
				m_AclNnb.dwId = atol(pList->GetItemText(nIndex - 1, 0)) + 1;

			m_gpAclSub->m_AclNnb.AddAcl(&m_AclNnb, TRUE);
			theApp.m_AclFile.AddAcl((PVOID)&m_AclNnb, ACL_TYPE_NNB);

			if(m_Check.GetCheck())
				m_gpAclSub->m_AclNnb.SetPass();

			theApp.m_AclFile.SaveAcl();
		}
		else if(m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_ICMP)
		{
			CListCtrl* pList = m_gpAclSub->m_AclIcmp.GetList();
			int nIndex = pList->GetItemCount();
			if(nIndex == 0)
				m_AclIcmp.dwId = 1;
			else
				m_AclIcmp.dwId = atol(pList->GetItemText(nIndex - 1, 0)) + 1;

			m_gpAclSub->m_AclIcmp.AddAcl(&m_AclIcmp, TRUE);
			theApp.m_AclFile.AddAcl((PVOID)&m_AclIcmp, ACL_TYPE_ICMP);

			if(m_Check.GetCheck())
				m_gpAclSub->m_AclIcmp.SetPass();

			theApp.m_AclFile.SaveAcl();
		}
	}

	EndDialog(IDOK);
}

void CAclQuery::OnCancel() 
{
	MakeAction(XF_DENY);
	EndDialog(IDCANCEL);
}

void CAclQuery::OnClose() 
{
	OnCancel();	
}

HBRUSH CAclQuery::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	int nID = pWnd->GetDlgCtrlID();
	if(nID == IDC_RADIO_DENY || nID == IDC_RADIO_PASS
	   || nID == IDC_RADIO_DENY_AND_ADD_ACL
	   || nID == IDC_RADIO_PASS_AND_ADD_ACL
	   || nID == IDC_CHECK_QUERY
	   || nID == IDC_APP_ICON)
	{
		pDC->SetBkMode(TRANSPARENT);
		return m_QueryBursh;
	}

	HBRUSH hbr = CPasseckDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	return hbr;
}

void CAclQuery::OnRadioDeny() 
{
	m_Button[ACL_QUERY_BUTTON_ADVANCE].EnableWindow(FALSE);
	m_Check.ShowWindow(SW_HIDE);
	MakeAction(XF_DENY);
}

void CAclQuery::OnRadioPass() 
{
	m_Button[ACL_QUERY_BUTTON_ADVANCE].EnableWindow(FALSE);
	m_Check.ShowWindow(SW_HIDE);
	MakeAction(XF_PASS);
}

void CAclQuery::OnRadioDenyAndAddAcl() 
{
	m_Button[ACL_QUERY_BUTTON_ADVANCE].EnableWindow(TRUE);
	m_Check.ShowWindow(SW_SHOW);
	MakeAction(XF_DENY);
}

void CAclQuery::OnRadioPassAndAddAcl() 
{
	m_Button[ACL_QUERY_BUTTON_ADVANCE].EnableWindow(TRUE);
	m_Check.ShowWindow(SW_SHOW);
	MakeAction(XF_PASS);
}

void CAclQuery::OnAdvance() 
{
	if(!m_Button[ACL_QUERY_BUTTON_ADVANCE].IsWindowEnabled())
		return;
	if(m_Session.bStatus == SESSION_STATUS_QUERYING_APP
		|| m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_APP)
	{
		CAclDialog dlg;
		dlg.SetDialog(ACL_CAPTION_ADD, ACL_CAPTION_APP_ADD, ACL_DIALOG_APP);
		CAclSet* pAclSet = dlg.GetAclSet();
		PXACL pAcl = pAclSet->GetAcl();
		*pAcl = m_Acl;
		pAclSet->SetAutoPort(FALSE);

		int iRet = dlg.DoModal();
		if(iRet == IDCANCEL) return;
		if(pAclSet->IsChange())
			m_Acl = *pAcl;
	}
	else if(m_Session.bStatus == SESSION_STATUS_QUERYING_WEB)
	{
		CAclDialog dlg;
		dlg.SetDialog(ACL_CAPTION_ADD, ACL_CAPTION_WEB_ADD, ACL_DIALOG_WEB);
		CAclWebSet* pAclWebSet = dlg.GetAclWebSet();
		PXACL_WEB pAcl = pAclWebSet->GetAcl();
		*pAcl = m_AclWeb;

		int iRet = dlg.DoModal();
		if(iRet == IDCANCEL) return;
		if(pAclWebSet->IsChange())
			m_AclWeb = *pAcl;
	}
	else if(m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_NNB)
	{
		CAclDialog dlg;
		dlg.SetDialog(ACL_CAPTION_ADD, ACL_CAPTION_NNB_ADD, ACL_DIALOG_NNB);
		CAclNnbSet* pAclNnbSet = dlg.GetAclNnbSet();
		PXACL_NNB pAcl = pAclNnbSet->GetAcl();
		*pAcl = m_AclNnb;

		int iRet = dlg.DoModal();
		if(iRet == IDCANCEL) return;
		if(pAclNnbSet->IsChange())
			m_AclNnb = *pAcl;
	}
	else if(m_Session.bStatus == SESSION_STATUS_QUERY_DRIVER_ICMP)
	{
		CAclDialog dlg;
		dlg.SetDialog(ACL_CAPTION_ADD, ACL_CAPTION_ICMP_ADD, ACL_DIALOG_ICMP);
		CAclIcmpSet* pAclIcmpSet = dlg.GetAclIcmpSet();
		PXACL_ICMP pAcl = pAclIcmpSet->GetAcl();
		*pAcl = m_AclIcmp;

		int iRet = dlg.DoModal();
		if(iRet == IDCANCEL) return;
		if(pAclIcmpSet->IsChange())
			m_AclIcmp = *pAcl;
	}
}

void CAclQuery::OnMin() 
{
	CloseWindow();	
}

BOOL CAclQuery::OnEraseBkgnd(CDC* pDC)
{
	if(m_bIsFirst)
	{
		m_memDC.CreateCompatibleDC(pDC);
		m_memDC.SelectObject(&m_BitmapBk);
		m_bIsFirst = FALSE;
	}

	CRect		rect;
	GetWindowRect(&rect);
	pDC->BitBlt(0, 0, rect.Width(), rect.Height(), &m_memDC, 0, 0, SRCCOPY);

	return TRUE;
}

void CAclQuery::SetPacket(PPACKET_BUFFER pPacket)
{
	m_Session.bStatus = pPacket->AclType + SESSION_STATUS_QUERY_DRIVER;

	m_Session.dwRemoteIp	= pPacket->DestinationIp;
	m_Session.wRemotePort	= pPacket->DestinationPort;
	m_Session.bNetType		= pPacket->NetType;
	m_Session.dwLocalIp		= pPacket->SourceIp;
	m_Session.wLocalPort	= pPacket->SourcePort;
	m_Session.tStartTime	= pPacket->Time;
	m_Session.bTimeType		= pPacket->TimeType;
	m_Session.bDirection	= pPacket->Direction;
	m_Session.bProtocol		= pPacket->Protocol;

	if(strnicmp(pPacket->sProcess, "SYSTEM", strlen(pPacket->sProcess) == 0))
		strcpy(m_Session.sPathName, "*");
	else
		strcpy(m_Session.sPathName, pPacket->sProcess);
}

BOOL CAclQuery::DestroyWindow() 
{
	theApp.m_dwSubWindowCount--;
	
	return CPasseckDialog::DestroyWindow();
}

#pragma comment( exestr, "B9D3B8FD2A63656E737767747B2B")
