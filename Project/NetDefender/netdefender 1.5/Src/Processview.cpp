// ProcessView.cpp : implementation file
//
#include "stdafx.h"
#include "fire.h"
#include "ProcessView.h"
#include "commonFn.h"
#include "Mainfrm.h"

// CProcessView dialog
IMPLEMENT_DYNAMIC(CProcessView, CDialog)
CProcessView::CProcessView(CWnd *pParent /*=NULL*/ ) : CDialog(CProcessView::IDD, pParent)
{
	m_pTcp = new CTCPTable();
	m_pUdp = new CUDPClass();
	m_pImageListMain = NULL;
}

CProcessView::~CProcessView()
{
}

void CProcessView::DoDataExchange(CDataExchange *pDX)
{
	CDialog :: DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PROCESS_VIEW, m_process_ListCtrl);
	DDX_Control(pDX, IDOK, m_btnOK);
	DDX_Control(pDX, IDCANCEL, m_btnCancel);
	DDX_Control(pDX, IDC_STATIC_RUN_APP, m_lRunningAppMsg);
}

BEGIN_MESSAGE_MAP(CProcessView, CDialog)
END_MESSAGE_MAP()

// CProcessView message handlers
BOOL CProcessView::OnInitDialog()
{
	CDialog :: OnInitDialog();
	if(DetectWinVersion()==_WINVER_2K_)
	{
		AfxMessageBox(_T("Functionality is not supported on Windows 2000, To use the functionality, please upgrade your OS to Windows Xp or above"),MB_ICONINFORMATION);
		EndDialog(FALSE);
		return FALSE;

	}
	CMainFrame* pMainWnd = (CMainFrame*)AfxGetMainWnd();
	SetIcon(pMainWnd->m_pImgList.ExtractIcon(32), TRUE);
	SetIcon(pMainWnd->m_pImgList.ExtractIcon(32), FALSE);
	m_btnOK.SetBitmaps(IDB_OK, RGB(255, 0, 255), IDB_OK, RGB(255, 0, 255));
	m_btnOK.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, 30);
	m_btnOK.DrawBorder();
	m_btnOK.DrawFlatFocus(TRUE);

	m_btnCancel.SetBitmaps(IDB_CANCEL, RGB(255, 0, 255), IDB_CANCEL, RGB(255, 0, 255));
	m_btnCancel.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, 30);
	m_btnCancel.DrawBorder();
	m_btnCancel.DrawFlatFocus(TRUE);

	m_lRunningAppMsg.SetFontBold( TRUE );
	//m_lRunningAppMsg.SetTextColor(RGB(  0,100,  0));

	InitCtrlList();

	//Init
	InitListAll();
	for(int n = 0; n < m_process_ListCtrl.GetItemCount(); n++)
	{
		if(n % 2 == 0)
		{
			m_process_ListCtrl.SetItemBkColor(n, -1, RGB(180, 180, 255));
		}
		else
		{
			m_process_ListCtrl.SetItemBkColor(n, -1, RGB(220, 220, 255));
		}
	}

	return(TRUE);	// return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CProcessView::InitListTcpEx(bool exclusive)	//exclude "127.0.0.1" & "0.0.0.0"
{
	TCPTABLE	sTcpTable;
	in_addr		stAddr;
	CString		strText(L"");
	DWORD		iItem = 0;

	//clean list
	m_process_ListCtrl.SetRedraw(FALSE);
	if(exclusive)
	{
		m_process_ListCtrl.DeleteAllItems();
		iItem = 0;
	}
	else
	{
		iItem = m_process_ListCtrl.GetItemCount();
	}

	/**********************Get TCP info**********************/
	//fill buffer
	DWORD	dwRez = 0;

	// init tcp table
	if(m_pTcp == NULL)
	{
		return;
	}

	if(NO_ERROR != (dwRez = m_pTcp->GetTableEx()))
	{
		return;
	}

	//print the info
//	int iSubItem = 1;
	for(DWORD i = 0; i < m_pTcp->GetNumberOfEntries(); i++)
	{
		//prepare the local address field
		stAddr.s_addr = static_cast<u_long>(m_pTcp->GetLocalAddress(i));
		sTcpTable.strLocalAddress = inet_ntoa(stAddr);

		//prepare the local port field
		sTcpTable.ulLocalPort = ntohs (static_cast<u_short>(m_pTcp->GetLocalPort (i)));

		//prepare the remote address field
		stAddr.s_addr = static_cast<u_long>(m_pTcp->GetRemoteAddress(i));
		sTcpTable.strRemoteAddress = inet_ntoa(stAddr);

		//prepare the remote port field
		sTcpTable.ulRemotePort = ntohs (static_cast<u_short>(m_pTcp->GetRemotePort (i)));

		//prepare the Pid
		sTcpTable.dwPid = m_pTcp->GetProcessId(i);
		sTcpTable.strProcessName = m_pTcp->GetProcById(sTcpTable.dwPid);

		//prepare the state
		sTcpTable.strConnectionState = m_pTcp->Convert2State(m_pTcp->GetState(i));

		if(sTcpTable.strConnectionState == _T("ESTABLISHED"))
		{
			//print TCP info
			m_process_ListCtrl.InsertItem(iItem, _T("TCP"));

			LVITEM	item;
			item.mask = LVIF_TEXT | LVIF_IMAGE;
			item.iItem = iItem;
			item.pszText = _T("TCP");
			item.iSubItem = hProto;
			item.iImage = GetProcessIcon(sTcpTable.strProcessName, m_pTcp->GetProcPath(sTcpTable.dwPid));
			m_process_ListCtrl.SetItem(&item);

			//print info for locals
			strText.Format(_T("%d"), sTcpTable.dwPid);
			m_process_ListCtrl.SetItemText(iItem, hPid, strText);

			//proc icon			
			m_process_ListCtrl.SetItemText(iItem, hProcessName, sTcpTable.strProcessName);
			m_process_ListCtrl.SetItemText(iItem, hLocalAddress, sTcpTable.strLocalAddress);
			strText.Format(_T("%u"), sTcpTable.ulLocalPort);
			m_process_ListCtrl.SetItemText(iItem, hLocalPort, strText);

			//print info for remote
			m_process_ListCtrl.SetItemText(iItem, hRemoteAddress, sTcpTable.strRemoteAddress);

			//print info for state
			m_process_ListCtrl.SetItemText(iItem, hState, sTcpTable.strConnectionState);

			if(sTcpTable.strConnectionState == "ESTABLISHED")
			{
				strText.Format(_T("%u"), sTcpTable.ulRemotePort);
				m_process_ListCtrl.SetItemText(iItem, hRemotePort, strText);
			}
			else
			{
				m_process_ListCtrl.SetItemText(iItem, hRemotePort, _T("-"));
			}

			iItem++;	//set the next entry
		}				//end if established state
	}					//end for

	m_process_ListCtrl.SetRedraw(TRUE);
}

void CProcessView::InitListUdpEx(bool exclusive)	//exclude "127.0.0.1" & "0.0.0.0"
{
	UDPTABLE	sUdpTable;
	in_addr		stAddr;
	CString		strText(L"");
	DWORD		iItem = 0;

	//clean list
	m_process_ListCtrl.SetRedraw(FALSE);
	if(exclusive)
	{
		m_process_ListCtrl.DeleteAllItems();
		iItem = 0;
	}
	else
	{
		iItem = m_process_ListCtrl.GetItemCount();
	}

	/**********************Get UDP info**********************/
	//fill buffer
	m_pUdp->GetTableEx();

	//print the info
	//int iSubItem = 1;
	for(DWORD i = 0; i < m_pUdp->m_pBuffUdpTableEx->dwNumEntries; i++)
	{
		//prepare the local address field
		stAddr.s_addr = static_cast<u_long>(m_pUdp->m_pBuffUdpTableEx->table[i].dwLocalAddr);;
		sUdpTable.strLocalAddress = inet_ntoa(stAddr);

		//prepare the local port field
		sUdpTable.ulLocalPort = ntohs (static_cast<u_short>(m_pUdp->m_pBuffUdpTableEx->table[i].dwLocalPort));

		//prepare the Pid
		sUdpTable.dwPid = m_pUdp->m_pBuffUdpTableEx->table[i].dwProcessId;
		sUdpTable.strProcessName = m_pUdp->GetProcById(sUdpTable.dwPid);

		//if ((sUdpTable.strLocalAddress != "0.0.0.0") && (sUdpTable.strLocalAddress != "127.0.0.1"))
		{
			//print TCP info
			m_process_ListCtrl.InsertItem(iItem, _T("UDP"));

			//proc icon
			LVITEM	item;
			item.mask = LVIF_TEXT | LVIF_IMAGE;
			item.iItem = iItem;
			item.iSubItem = hProto;
			item.iImage = GetProcessIcon(sUdpTable.strProcessName, m_pTcp->GetProcPath(sUdpTable.dwPid));
			m_process_ListCtrl.SetItem(&item);

			//print info for locals
			strText.Format(_T("%d"), sUdpTable.dwPid);
			m_process_ListCtrl.SetItemText(iItem, hPid, strText);

			m_process_ListCtrl.SetItemText(iItem, hProcessName, sUdpTable.strProcessName);

			m_process_ListCtrl.SetItemText(iItem, hLocalAddress, sUdpTable.strLocalAddress);

			strText.Format(_T("%u"), sUdpTable.ulLocalPort);
			m_process_ListCtrl.SetItemText(iItem, hLocalPort, strText);

			//print info for remote and state
			m_process_ListCtrl.SetItemText(iItem, hRemoteAddress,_T("-"));
			m_process_ListCtrl.SetItemText(iItem, hRemotePort, _T("-"));
			m_process_ListCtrl.SetItemText(iItem, hState, _T("-"));

			iItem++;						//set the next entry
		}									//end if real ip
	}										//end for

	m_process_ListCtrl.SetRedraw(TRUE);
}

void CProcessView::InitListAll()
{
	/*if (m_eConnView == viewConnected)
	{
		InitListTcpEx(true);
		return;
	}*/
	in_addr stAddr;
	TCHAR	strLocAddress[16];
	TCHAR	strRemAddress[16];
	CString strText(L"");
	DWORD	iItem;
	LVITEM	item;

	//clean list
	m_process_ListCtrl.SetRedraw(FALSE);
	m_process_ListCtrl.DeleteAllItems();

	/**********************Get TCP info**********************/
	//fill buffer
	DWORD	dwRez = 0;

	// init tcp table
	if(m_pTcp == NULL)
	{
		return;
	}

	if(NO_ERROR != (dwRez = m_pTcp->GetTableEx()))
	{
		return;
	}

	//print the info
	//int iSubItem = 1;
	for(iItem = 0; iItem < m_pTcp->GetNumberOfEntries(); iItem++)
	{
		//prepare the local address field
		u_long	ulLocalAddress = static_cast<u_long>(m_pTcp->GetLocalAddress(iItem));
		stAddr.s_addr = ulLocalAddress;
		USES_CONVERSION;

		_tcscpy(strLocAddress, A2T(inet_ntoa(stAddr)));

		//prepare the local port field
		u_short usLocalPort = ntohs (static_cast<u_short>(m_pTcp->GetLocalPort (iItem)));

		//prepare the remote address field
		u_long	ulRemoteAddress = static_cast<u_long>(m_pTcp->GetRemoteAddress(iItem));
		stAddr.s_addr = ulRemoteAddress;
		_tcscpy(strRemAddress, A2T(inet_ntoa(stAddr)));

		//prepare the remote port field
		u_short usRemotePort = ntohs (static_cast<u_short>(m_pTcp->GetRemotePort (iItem)));

		//prepare the Pid
		DWORD	dwPid = m_pTcp->GetProcessId(iItem);
		CString strProcName = m_pTcp->GetProcById(dwPid);

		//print TCP info
		if("TCP" != m_process_ListCtrl.GetItemText(iItem, hProto))
		{
			m_process_ListCtrl.InsertItem(iItem, _T("TCP"));
		}

		//print info for locals
		strText.Format(_T("%d"), dwPid);

		m_process_ListCtrl.SetItemText(iItem, hPid, strText);

		//proc icon
		//LVITEM item;
		item.mask = LVIF_TEXT | LVIF_IMAGE;
		item.iItem = iItem;
		item.iSubItem = hProto;				//hProcessName;
		item.pszText = _T("TCP");				//to be ignored
		int iIcoIndex = -1;
		iIcoIndex = GetProcessIcon(strProcName, m_pTcp->GetProcPath(dwPid));
		item.iImage = iIcoIndex;
		m_process_ListCtrl.SetItem(&item);
		strText.Format(_T("%s"), strProcName);
		m_process_ListCtrl.SetItemText(iItem, hProcessName, strText);

		strText.Format(_T("%s"), strLocAddress);

		m_process_ListCtrl.SetItemText(iItem, hLocalAddress, strText);

		strText.Format(_T("%u"), usLocalPort);

		m_process_ListCtrl.SetItemText(iItem, hLocalPort, strText);

		//print info for remote
		strText.Format(_T("%s"), strRemAddress);

		m_process_ListCtrl.SetItemText(iItem, hRemoteAddress, strText);

		//print info for state
		strText.Format(_T("%s"), m_pTcp->Convert2State(m_pTcp->m_pBuffTcpTableEx->table[iItem].dwState));

		m_process_ListCtrl.SetItemText(iItem, hState, strText);

		if(m_pTcp->m_pBuffTcpTableEx->table[iItem].dwState == MIB_TCP_STATE_ESTAB)
		{
			strText.Format(_T("%u"), usRemotePort);
			m_process_ListCtrl.SetItemText(iItem, hRemotePort, strText);
		}
		else
		{
			strText = "-";
			m_process_ListCtrl.SetItemText(iItem, hRemotePort, strText);
		}
	}

	/**********************Get UDP info**********************/
	in_addr _stAddr;
	TCHAR	_strLocAddress[16];
	u_long	_ulLocalAddress = 0;
	u_short _usLocalPort = 0;
	DWORD	_dwPid = 0;
	CString _strProcName(L"");
	//int		_iSubItem = 1;

	//fill buffer
	m_pUdp->GetTableEx();

	//print the info
	for(DWORD _iItem = 0; _iItem < m_pUdp->m_pBuffUdpTableEx->dwNumEntries; _iItem++)
	{
		//prepare the local address field
		_ulLocalAddress = static_cast<u_long>(m_pUdp->m_pBuffUdpTableEx->table[_iItem].dwLocalAddr);
		_stAddr.s_addr = _ulLocalAddress;
		USES_CONVERSION;
		_tcscpy(_strLocAddress, A2T(inet_ntoa(_stAddr)));

		//prepare the local port field
		_usLocalPort = ntohs (static_cast<u_short>(m_pUdp->m_pBuffUdpTableEx->table[_iItem].dwLocalPort));

		//prepare the Pid
		_dwPid = m_pUdp->m_pBuffUdpTableEx->table[_iItem].dwProcessId;

		_strProcName = m_pUdp->GetProcById(m_pUdp->m_pBuffUdpTableEx->table[_iItem].dwProcessId);

		//print UDP info
		if(_T("UDP") != m_process_ListCtrl.GetItemText(iItem, hProto))
		{
			m_process_ListCtrl.InsertItem(iItem, _T("UDP"));
		}

		//print info for locals
		strText.Format(_T("%d"), _dwPid);
		m_process_ListCtrl.SetItemText(iItem, hPid, strText);

		//proc icon
		//LVITEM item;
		item.mask = LVIF_TEXT | LVIF_IMAGE;
		item.iItem = iItem;
		item.iSubItem = hProto;				//hProcessName;
		item.pszText = _T("UDP");				//to be ignored
		int _iIcoIndex = GetProcessIcon(_strProcName, m_pTcp->GetProcPath(_dwPid));
		item.iImage = _iIcoIndex;
		m_process_ListCtrl.SetItem(&item);
		strText.Format(_T("%s"), _strProcName);
		m_process_ListCtrl.SetItemText(iItem, hProcessName, strText);

		strText.Format(_T("%s"), _strLocAddress);
		m_process_ListCtrl.SetItemText(iItem, hLocalAddress, strText);

		strText.Format(_T("%u"), _usLocalPort);
		m_process_ListCtrl.SetItemText(iItem, hLocalPort, strText);

		//print info for remote
		m_process_ListCtrl.SetItemText(iItem, hRemoteAddress, _T("-"));
		m_process_ListCtrl.SetItemText(iItem, hRemotePort, _T("-"));

		//print info for state
		m_process_ListCtrl.SetItemText(iItem, hState, _T("-"));

		iItem++;
	}

	m_process_ListCtrl.SetRedraw(TRUE);
}

void CProcessView::InitCtrlList()
{
	// Init ImageList for m_process_ListCtrl
	CWinApp *pApp = AfxGetApp();
	m_pImageListMain = new CImageList();
	ASSERT(m_pImageListMain != NULL);		// serious allocation failure checking
	/*BOOL	brez = */m_pImageListMain->Create(16, 16, ILC_COLOR32, 16, 64);
	m_pImageListMain->Add(pApp->LoadIcon(IDI_ICON_PROTO));
	m_pImageListMain->Add(pApp->LoadIcon(IDI_ICON_UNK_PROCESS));
	m_pImageListMain->SetBkColor(CLR_DEFAULT);
	m_process_ListCtrl.SetImageList(m_pImageListMain);

	//set style of the list
	m_process_ListCtrl.SetExtendedStyle(LVS_EX_SUBITEMIMAGES | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EDITLABELS);

	// Init ListView
	m_process_ListCtrl.InsertColumn(hProto, _T("Proto"), LVCFMT_LEFT, 60, -1);
	m_process_ListCtrl.InsertColumn(hPid, _T("Pid"), LVCFMT_LEFT, 40, -1);
	m_process_ListCtrl.InsertColumn(hProcessName, _T("Process Name"), LVCFMT_LEFT, 100, -1);
	m_process_ListCtrl.InsertColumn(hLocalAddress, _T("Local Address"), LVCFMT_LEFT, 100, -1);
	m_process_ListCtrl.InsertColumn(hLocalPort, _T("Local Port"), LVCFMT_LEFT, 80, -1);
	m_process_ListCtrl.InsertColumn(hRemoteAddress, _T("Remote Address"), LVCFMT_LEFT, 100, -1);
	m_process_ListCtrl.InsertColumn(hRemotePort, _T("Remote Port"), LVCFMT_LEFT, 80, -1);
	m_process_ListCtrl.InsertColumn(hState, _T("State"), LVCFMT_LEFT, 100, -1);

	m_process_ListCtrl.SetGridLines(TRUE);	// SHow grid lines
	m_process_ListCtrl.SortItems(0, TRUE);	// sort the 1st column, ascending
}

int CProcessView::GetProcessIcon(CString strAppName, CString strPathAppName)
{
	/*
	 *	Note: -> there is no need to clean-up the map: knowledge is better
	 *        -> icon cache mechanism 
	 */
	int nUnk = 1;		//default icon (app without icon)
	int nIcoIndex = -1;

	if(strPathAppName.IsEmpty() || (strPathAppName == "error"))
	{
		return(nUnk);	//default for unk process
	}

	if(strAppName.IsEmpty())
	{
		strAppName = strPathAppName.Right((strPathAppName.GetLength() - 1) - strPathAppName.ReverseFind('\\'));
	}

	//is it already assigned ?
	Ico2Index :: iterator	it = m_mProcName2IcoIndex.find(strAppName);
	if(it != m_mProcName2IcoIndex.end())
	{
		TRACE2("\n Existing map, process: %s, icon: %d\n", (*it).first, m_mProcName2IcoIndex[strAppName]);
		nIcoIndex = m_mProcName2IcoIndex[strAppName];
		return(--nIcoIndex);
	}

	//add a new record
	HICON	hSmallIco = NULL;
	if(NULL == (hSmallIco = :: ExtractIcon(NULL, strPathAppName, 0)))
	{
		return(nUnk);
	}

	m_pImageListMain->Add(hSmallIco);
	DestroyIcon(hSmallIco);

	nIcoIndex = (m_pImageListMain->GetImageCount());
	if(nIcoIndex >= 1)
	{
		TRACE2("\n New map, process: %s, icon: %d\n", strAppName, nIcoIndex);
		m_mProcName2IcoIndex[strAppName] = nIcoIndex;
		return(--nIcoIndex);
	}
	else
	{
		return(nUnk);
	}
}
