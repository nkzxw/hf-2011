// PortScanDlg.cpp : implementation file
//
#include "stdafx.h"
#include "fire.h"
#include "PortScanDlg.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include "TheSocket.h"
#include "Mainfrm.h"
#include "..\inc\portscandlg.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
UINT		nIndex = 0;

/////////////////////////////////////////////////////////////////////////////
// CPortScanDlg dialog

/*---------------------------------------------------------------------------------------------
Name				:	run(LPVOID pParam)
Purpose				:	<Purpose>
Parameters			:
						LPVOID  pParam - 
Return				:	<Return Description>
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
UINT run(LPVOID pParam)
{
	CScanData	*objScanData = (CScanData *) pParam;

	objScanData->m_bFinished = false;

	CString strtmp = _T("");
	strtmp.Format(_T("%d"), objScanData->m_nPortNo);

	CPortScanDlg	objPortScanDlg;
	if(objPortScanDlg.TestConnection(objScanData->m_strIPAddress, objScanData->m_nPortNo))
	{
		objScanData->m_strErrorlog += objScanData->m_strIPAddress + _T(":") + strtmp + _T("  Opened!!!\r\n");
		objScanData->m_bOpen = true;
	}
	else
	{
		switch(GetLastError())
		{
			case WSAECONNREFUSED:
				objScanData->m_strErrorlog += objScanData->m_strIPAddress + _T(":") + strtmp + _T(" Closed!!!\r\n");
				break;

			case WSAETIMEDOUT:
				objScanData->m_strErrorlog += objScanData->m_strIPAddress + _T(":") + strtmp + _T(" ***Unable to scan***\r\n");
				break;

			default:
				objScanData->m_strErrorlog += objScanData->m_strIPAddress + _T(":") + strtmp + _T(" ***Unknown Error***\r\n");
				break;
		}
	}

	objScanData->m_bFinished = true;

	return(0);
}

CPortScanDlg::CPortScanDlg(CWnd *pParent /*=NULL*/ ) : CDialog(CPortScanDlg::IDD, pParent)
{
	m_pColumns = new CStringList;
	ASSERT(m_pColumns);
	m_bSinglePort = TRUE;

	/*********************************************
	 * default value, This value has been set on the window
	 ********************************************/
	m_nMaxAttempts = 1; //default value, This value has been set on the window
	m_pStatusList = new CPtrList;
	ASSERT(m_pStatusList);
	//{{AFX_DATA_INIT(CPortScanDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPortScanDlg::~CPortScanDlg()
{
	if(m_pStatusList)
	{
		//First Empty the port status list:
		POSITION	p = m_pStatusList->GetHeadPosition();
		while(p)
		{
			POSITION	temp = p;
			DATA		*pNode = (DATA *) m_pStatusList->GetNext(p);
			m_pStatusList->RemoveAt(temp);
			if(pNode)
			{
				delete pNode;
			}
		}

		//Then remove it from heap:
		delete m_pStatusList;
	}
}

void CPortScanDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialog :: DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPortScanDlg)
	DDX_Control(pDX, IDC_STATUS_BAR, m_static);
	DDX_Control(pDX, IDC_PROGRESS, m_cProgress);
	DDX_Control(pDX, IDC_LIST_RESULT, m_cResult);
	DDX_Control(pDX, IDC_IP_ADDRESS, m_cIP);
	DDX_Control(pDX, IDC_EDIT_SINGLE_PORT_TO, m_cPortTo);
	DDX_Control(pDX, IDC_EDIT_SINGLE_PORT_FROM, m_cPortFrom);
	DDX_Control(pDX, IDC_EDIT_SINGLE_PORT, m_cSinglePort);
	DDX_Control(pDX, IDC_EDIT_ATTEMPTS, m_cAttempts);
	DDX_Control(pDX, IDC_BUTTON_STOP, m_cBtnStop);
	DDX_Control(pDX, IDC_BUTTON_SCAN, m_cBtnScan);
	DDX_Control(pDX, IDC_STATIC_IP, m_lIPAddress);
	DDX_Control(pDX, IDC_STATIC_SCAN_RESULT, m_lScanResult);
	
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPortScanDlg, CDialog)
//{{AFX_MSG_MAP(CPortScanDlg)
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_RADIO_SINGLE, OnRadioSingle)
	ON_BN_CLICKED(IDC_RADIO_RANGE, OnRadioRange)
	ON_BN_CLICKED(IDC_BUTTON_SCAN, OnButtonScan)
	ON_BN_CLICKED(IDC_BUTTON_STOP, OnButtonStop)
//}}AFX_MSG_MAP
	ON_WM_TIMER()
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CPortScanDlg message handlers

/*********************************************
 * Does all the initialization for the dialog box
 *
 * @return BOOL 
 ********************************************/
BOOL CPortScanDlg::OnInitDialog()
{
	CDialog :: OnInitDialog();

	AddHeader(_T("IP address"));
	AddHeader(_T("Port number"));
	AddHeader(_T("Port Status"));
	AddHeader(_T("Attempts"));
	AddHeader(_T("Remarks"));

	/////////////////////////////

	CMainFrame* pMainWnd = (CMainFrame*)AfxGetMainWnd();
	SetIcon(pMainWnd->m_pImgList.ExtractIcon(31), TRUE);
	SetIcon(pMainWnd->m_pImgList.ExtractIcon(31), FALSE);
	m_cBtnScan.SetIcon(pMainWnd->m_pImgList.ExtractIcon(30));
	m_cBtnScan.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, 30);
	m_cBtnScan.DrawFlatFocus(TRUE);
	//m_cBtnScan.EnableBalloonTooltip();
	//m_cBtnScan.SetTooltipText(_T("Start the port scanner to check the system for open ports"));
	m_cBtnScan.DrawBorder();

	m_cBtnStop.SetIcon(pMainWnd->m_pImgList.ExtractIcon(86));
	m_cBtnStop.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, 30);
	m_cBtnStop.DrawFlatFocus(TRUE);
	//m_cBtnStop.EnableBalloonTooltip();
	//m_cBtnStop.SetTooltipText(_T("Stop the port scanner"));
	m_cBtnStop.DrawBorder();
	ShowHeaders();
	CheckRadioButton(IDC_RADIO_SINGLE, IDC_RADIO_RANGE, IDC_RADIO_SINGLE);
	m_cSinglePort.EnableWindow();
	m_cPortFrom.EnableWindow(FALSE);
	m_cPortTo.EnableWindow(FALSE);

	//	m_cBtnStop.EnableWindow(FALSE);
	m_cAttempts.SetWindowText(_T("1"));
	status_timer_id = 0;
	m_cIP.SetWindowText(GetIPAddress());
	m_cResult.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_TRACKSELECT);
	m_cResult.EnableToolTips(TRUE);
	m_cResult.SetGridLines(TRUE);	// SHow grid lines
	m_cBtnScan.EnableWindow(TRUE);
	m_cBtnStop.EnableWindow(FALSE);
	m_lIPAddress.SetFontBold( TRUE );
	//m_lIPAddress.SetTextColor(RGB(  0,100,  0));
	m_lScanResult.SetFontBold(TRUE);
	//m_lScanResult.SetTextColor(RGB(  0,100,  0));
	// Create the CPPToolTip object
	m_tooltip.Create(this);
	m_tooltip.SetBehaviour(PPTOOLTIP_MULTIPLE_SHOW);
	m_tooltip.SetNotify();
	AddToolTips();
	return(TRUE);					// return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

//*************************************************************
BOOL CPortScanDlg::AddColumn(LPCTSTR strItem, int nItem, int nSubItem, int nMask, int nFmt)
{
	LV_COLUMN	lvc;
	lvc.mask = nMask;
	lvc.fmt = nFmt;
	lvc.pszText = (LPTSTR) strItem;
	lvc.cx = m_cResult.GetStringWidth(lvc.pszText) + 25;
	if(nMask & LVCF_SUBITEM)
	{
		if(nSubItem != -1)
		{
			lvc.iSubItem = nSubItem;
		}
		else
		{
			lvc.iSubItem = nItem;
		}
	}

	return(m_cResult.InsertColumn(nItem, &lvc));
}

BOOL CPortScanDlg::AddItem(int nItem, int nSubItem, LPCTSTR strItem, int nImageIndex)
{
	LV_ITEM lvItem;
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = nItem;
	lvItem.iSubItem = nSubItem;
	lvItem.pszText = (LPTSTR) strItem;

	if(nImageIndex != -1)
	{
		lvItem.mask |= LVIF_IMAGE;
		lvItem.iImage |= LVIF_IMAGE;
	}

	if(nSubItem == 0)
	{
		return(m_cResult.InsertItem(&lvItem));
	}

	return(m_cResult.SetItem(&lvItem));
}

void CPortScanDlg::AddHeader(LPTSTR hdr)
{
	if(m_pColumns)
	{
		m_pColumns->AddTail(hdr);
	}
}

void CPortScanDlg::ShowHeaders()
{
	int			nIndex = 0;
	POSITION	pos = m_pColumns->GetHeadPosition();
	while(pos)
	{
		CString hdr = (CString) m_pColumns->GetNext(pos);
		AddColumn(hdr, nIndex++);
	}
}

BOOL CPortScanDlg::TestConnection(CString IP, UINT nPort)
{
	/*CTheSocket* pSocket;
	pSocket = new CTheSocket;
	ASSERT(pSocket);*/
	CSocket objSocket;
	if(!objSocket.Create())
	{
		/*delete pSocket;
		pSocket = NULL;*/
		return(FALSE);
	}

	if(!objSocket.Connect(IP, nPort))
	{
		/*delete pSocket;
		pSocket = NULL;*/
		return(FALSE);
	}

	objSocket.Close();

	//delete pSocket;
	return(TRUE);
}

void CPortScanDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog :: OnShowWindow(bShow, nStatus);

	AddHeader(_T("IP address"));
	AddHeader(_T("Port number"));
	AddHeader(_T("Port Status"));
	AddHeader(_T("Attempts"));
	AddHeader(_T("Remarks"));
}

/*********************************************
 *  message handler code for Scan a single port radio Button
 *
 * @return void 
 ********************************************/
void CPortScanDlg::OnRadioSingle()
{
	m_bSinglePort = TRUE;
	m_cSinglePort.EnableWindow();
	m_cPortFrom.EnableWindow(FALSE);
	m_cPortTo.EnableWindow(FALSE);
	m_cBtnScan.EnableWindow(TRUE);
	m_cBtnStop.EnableWindow(FALSE);
}

/*********************************************
 *  message handler code for scan a range of port radio button
 *
 * @return void 
 ********************************************/
void CPortScanDlg::OnRadioRange()
{
	m_bSinglePort = FALSE;
	m_cSinglePort.EnableWindow(FALSE);
	m_cPortFrom.EnableWindow();
	m_cPortTo.EnableWindow();
	m_cBtnStop.EnableWindow(FALSE);
	m_cBtnScan.EnableWindow(TRUE);
}

/*********************************************
 * message handler code  for Start Button
 *
 * @return void 
 ********************************************/
void CPortScanDlg::OnButtonScan()
{
	CString btnTxt;
	UINT	nSinglePort;
	BYTE	f1, f2, f3, f4;
	TCHAR	temp[10] = _T("\0");

	nIndex = 0;

	m_cProgress.SetPos(0);
	m_cResult.DeleteAllItems();

	//Empty the port status list:
	POSITION	p = m_pStatusList->GetHeadPosition();
	while(p)
	{
		POSITION	temp = p;
		DATA		*pNode = (DATA *) m_pStatusList->GetNext(p);
		m_pStatusList->RemoveAt(temp);
		if(pNode)
		{
			delete pNode;
		}
	}

	if(m_cIP.IsBlank())
	{
		AfxMessageBox(_T("Please specify the remote machine's IP address."), MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	if(m_cIP.GetAddress(f1, f2, f3, f4) < 4)
	{
		AfxMessageBox(_T("Please specify the IP address again."), MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	IP = _itot(f1, temp, 10);
	IP += _T('.');
	IP += _itot(f2, temp, 10);
	IP += _T('.');
	IP += _itot(f3, temp, 10);
	IP += _T('.');
	IP += _itot(f4, temp, 10);

	m_cBtnStop.EnableWindow();
	m_cBtnScan.EnableWindow(FALSE);

	if(m_bSinglePort)
	{
		CString port;
		m_cSinglePort.GetWindowText(port);
		m_minPort = m_maxPort = nSinglePort = _ttoi(port);
		m_cProgress.SetRange32(0,1);
		m_cProgress.SetStep(1);
	}
	else
	{
		CString port1, port2;
		m_cPortFrom.GetWindowText(port1);
		m_cPortTo.GetWindowText(port2);
		m_minPort = _ttoi(port1);
		m_maxPort = _ttoi(port2);
		m_cProgress.SetRange32(0, m_maxPort - m_minPort + 1);
		m_cProgress.SetStep(1);
	}

	if(!m_bSinglePort && m_maxPort < m_minPort)
	{
		AfxMessageBox(_T("The maximum range cannot be less than the minimum one."),
				  	   MB_OK | MB_ICONINFORMATION);
		return;
	}

	UINT	m_nMaxAttempts = GetDlgItemInt(IDC_EDIT_ATTEMPTS);
	if(status_timer_id == 0)
	{
		status_timer_id = SetTimer(1, 20, 0);
	}

	for(m_nCounter = m_minPort; m_nCounter <= m_maxPort; m_nCounter++)
	{
		BOOL	bIsOpen = FALSE;
		UINT	nAttempt = 1;

		while(nAttempt <= m_nMaxAttempts && !bIsOpen)
		{
			TCHAR	temp[10] = _T("\0");
			CString str = _T("Trying port# ");

			str += _itot(m_nCounter, temp, 10);

			str += _T(", IP Address=");
			str += IP;
			str += _T(", Attempt=");

			str += _itot(nAttempt, temp, 10);

			m_static.SetWindowText(str);	//;
			str.Empty();

			CScanData	*pScanData = new CScanData;
			pScanData->m_strIPAddress = IP;
			pScanData->m_nPortNo = m_nCounter;
			arrScanData.Add(pScanData);
			Sleep(10);

			//bIsOpen = TestConnection(IP,m_nCounter);
			/*creates the threads that each checks its own port*/
			AfxBeginThread(run, (void *) pScanData, THREAD_PRIORITY_NORMAL);

			//TRACE(_T("\nin thread\n\n"));
			//if (bIsOpen)
			//{
			//	DATA* pNode = new DATA;
			//	ASSERT(pNode);
			//	strcpy(pNode->IPAddress,IP.GetBuffer(IP.GetLength()));
			//	strcpy(pNode->port,_itoa(m_nCounter,temp,10));
			//	pNode->bStatus = 1; //open
			//	pNode->nAttempts = nAttempt;
			//	m_pStatusList->AddTail(pNode);
			//}
			nAttempt++;
		}

		//if (!bIsOpen)
		//{
		//	DATA* pNode = new DATA;
		//	ASSERT(pNode);
		//	strcpy(pNode->IPAddress,IP.GetBuffer(IP.GetLength()));
		//	strcpy(pNode->port,_itoa(m_nCounter,temp,10));
		//	pNode->bStatus = 0; //close
		//	pNode->nAttempts = nAttempt-1;
		//	m_pStatusList->AddTail(pNode);
		//}
		MSG message;
		if( :: PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
		{
			:: TranslateMessage(&message);
			:: DispatchMessage(&message);
		}

		m_cProgress.StepIt();
	}

	m_static.SetWindowText((CString) _T("Ready"));

	//m_cBtnScan.EnableWindow();
	//m_cBtnStop.EnableWindow(FALSE);
	
		////Filling report view:
	//UINT nIndex = 0;
	//POSITION pos = m_pStatusList->GetHeadPosition();
	//while (pos)
	//{
	//	DATA* pNode = (DATA*)m_pStatusList->GetNext(pos);
	//	AddItem(nIndex,0,pNode->IPAddress);
	//	AddItem(nIndex,1,pNode->port);
	//	if (pNode->bStatus)
	//	{
	//		AddItem(nIndex,2,_T("Open"));
	//		AddItem(nIndex,4,_T("*"));
	//	}
	//	else
	//	{
	//		AddItem(nIndex,2,_T("Close"));
	//		AddItem(nIndex,4,_T(" "));
	//	}
	//	AddItem(nIndex++,3,_itoa(pNode->nAttempts,temp,10));
	//}
}

/*********************************************
 * message handler code for Stop Button
 *
 * @return void 
 ********************************************/
void CPortScanDlg::OnButtonStop()
{
	m_nCounter = m_maxPort + 1;
	m_cBtnStop.EnableWindow(FALSE);
	m_cBtnScan.EnableWindow();
	m_static.SetWindowText((CString) _T("Ready"));
	KillTimer(status_timer_id);
	status_timer_id = 0;
	m_cProgress.SetPos(0); //Set the progress bar to initial state on Stop
}

void CPortScanDlg::OnTimer(UINT nIDEvent)
{
	CString strport;
	m_cResult.SetRedraw(FALSE);

	//TCHAR temp[10] = "\0";
	if(nIDEvent == status_timer_id)
	{
		//int i = arrScanData.GetSize();
		if(arrScanData.GetSize() > 0)
		{
			CScanData	*objScanData = (CScanData *) arrScanData.ElementAt(0);
			if(objScanData->m_bFinished)
			{
				DATA	*pNode = new DATA;
				ASSERT(pNode);
				arrScanData.RemoveAt(0, 1);
				strport.Format(_T("%d"), objScanData->m_nPortNo);
				pNode->IPAddress = IP;

				//strcpy(pNode->port,_itoa(objScanData->m_nPortNo,temp,10));
				pNode->port = strport;
				pNode->bStatus = 1; //open
				if(objScanData->m_bOpen)
				{
					AddItem(nIndex, 0, IP);
					AddItem(nIndex, 1, strport);
					AddItem(nIndex, 2, _T("Open"));
					AddItem(nIndex, 4, _T("*"));
					m_cResult.SetItemTextColor(nIndex, -1, RGB(255, 0, 0));
				}
				else
				{
					AddItem(nIndex, 0, IP);
					AddItem(nIndex, 1, strport);
					AddItem(nIndex, 2, _T("Close"));
					AddItem(nIndex, 4, _T(" "));
				}

				CString strAttampts;
				m_cAttempts.GetWindowText(strAttampts);
				AddItem(nIndex++, 3, strAttampts);
				if(objScanData->m_nPortNo % 2 == 0)
				{
					m_cResult.SetItemBkColor(nIndex - 1, -1, RGB(180, 180, 255));
				}
				else
				{
					m_cResult.SetItemBkColor(nIndex - 1, -1, RGB(220, 220, 255));
				}

				//pNode->nAttempts = nAttempt;
				//m_pStatusList->AddTail(pNode);
			}
		}
		else
		{
			if(status_timer_id != 0)
			{
				//When Task completed set the UI state to initial
				KillTimer(status_timer_id);
				status_timer_id = 0;
				m_cBtnScan.EnableWindow(TRUE);
				m_cBtnStop.EnableWindow(FALSE);
				m_cProgress.SetPos(0);
			}
		}
	}

	m_cResult.SetRedraw(TRUE);
	CDialog :: OnTimer(nIDEvent);
}

///Get the Ip maddress of thre user Machine
CString CPortScanDlg::GetIPAddress()
{
	char			name[256];
	WSADATA			wsaData;
	struct hostent	*hostinfo;
	CString			strIP;

	WSAStartup(MAKEWORD(2, 1), &wsaData);

	//char	crlf[] = { 13, 10, 0 };
	if(gethostname(name, sizeof(name)) == 0)
	{
		if((hostinfo = gethostbyname(name)) != NULL)
		{
			int i;
			for(i = 0; hostinfo->h_addr_list[i] != NULL; i++)
			{
				strIP = inet_ntoa(*(struct in_addr *) hostinfo->h_addr_list[i]);
			}
		}
	}

	WSACleanup();
	return(strIP);
}

BOOL CPortScanDlg::PreTranslateMessage(MSG* pMsg)
{
	// must add RelayEvent function call to pass a mouse message to a tool tip control for processing.
	m_tooltip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}
void CPortScanDlg::AddToolTips()
{
	m_tooltip.AddTool((CWnd*)&m_cBtnScan,
		_T("Start the port scanning for the ports set in the input fields. <br><b>Port Scanner</b> will check the system for the open ports"));
	m_tooltip.AddTool((CWnd*)&m_cBtnStop,
		_T("Stop the port scanner and shows the results for the ports already scanned."));

	m_tooltip.AddTool(GetDlgItem(IDC_IP_ADDRESS),
				  _T("Enter the IP address of the machine for which ports are to scanned"));
	m_tooltip.AddTool(GetDlgItem(IDC_EDIT_SINGLE_PORT),
		_T("Enter the Port number to be Scanned.<br>For example to check if the FTP port is opened or blocked<br>Enter <b>21</b>"));
	m_tooltip.AddTool(GetDlgItem(IDC_EDIT_SINGLE_PORT_FROM),
		_T("Enter the starting range of the Ports to be Scanned.<br>For example to check if any port is opened from 20 - 100.<br>Enter <b>20</b> here."));
	m_tooltip.AddTool(GetDlgItem(IDC_EDIT_SINGLE_PORT_TO),
		_T("Enter the ending range of the Ports to be Scanned.<br>For example to check if any port is opened from 20 - 100.<br>Enter <b>100</b> here."));
	m_tooltip.AddTool(GetDlgItem(IDC_RADIO_SINGLE),
		_T("Check this box, if you wants to scan only a single port."));
	m_tooltip.AddTool(GetDlgItem(IDC_RADIO_RANGE),
		_T("Check this box, If you wants to scan the range of ports."));
	m_tooltip.SetColorBk(RGB(255, 255, 255), RGB(240, 247, 255), RGB(192, 192, 208));
	m_tooltip.SetEffectBk(CPPDrawManager :: EFFECT_SOFTBUMP);
	m_tooltip.SetMaxTipWidth(500);
	m_tooltip.EnableEscapeSequences(TRUE);

}