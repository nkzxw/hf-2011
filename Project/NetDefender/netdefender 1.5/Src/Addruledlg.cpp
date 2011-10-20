// AddRuleDlg.cpp : implementation file
//
#include "stdafx.h"
#include "fire.h"
#include "AddRuleDlg.h"
#include "Mainfrm.h"



//********************************************************
#include <winsock2.h>
#include "..\inc\addruledlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddRuleDlg dialog

/*********************************************
 * initialize the all GUI controls initial values <br>
 * Load the driver
 ********************************************/
CAddRuleDlg::CAddRuleDlg(CWnd *pParent /*=NULL*/ ) : CDialog(CAddRuleDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddRuleDlg)
	m_bEditRuleMode = FALSE;
	m_pActiveRule = NULL;
	//}}AFX_DATA_INIT
	//****************************************************************
	//if(ipFltDrv.IsLoaded() == FALSE)
	ipFltDrv.LoadDriver(_T("DrvFltIp"), NULL, NULL, TRUE);
}

void CAddRuleDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialog :: DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddRuleDlg)
	DDX_Control(pDX, IDC_COMBO2, m_protocol);
	DDX_Control(pDX, IDC_COMBO1, m_action);
	DDX_Text(pDX, IDC_DADD, m_strDestIPAddr);
	DDV_MaxChars(pDX, m_strDestIPAddr, 15);
	DDX_Text(pDX, IDC_DPORT, m_strDestPortNo);
	DDX_Text(pDX, IDC_SADD, m_strSrcIPAddr);
	DDV_MaxChars(pDX, m_strSrcIPAddr, 15);
	DDX_Text(pDX, IDC_SPORT, m_strSrcPortNo);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_ADDSAVE, m_btnRuleAddSave);
	DDX_Control(pDX, IDCANCEL, m_btnCancel);
	DDX_Control(pDX, IDC_STATIC_ADD_NEW_RULE, m_staticAddNewRule);
	DDX_Control(pDX, IDC_STATIC_SRC_ADD, m_staticSrcAdd);
	DDX_Control(pDX, IDC_STATIC_DEST_ADD, m_staticDestAdd);
	DDX_Control(pDX, IDC_STATIC_SRC_PORT, m_staticSrcPort);
	DDX_Control(pDX, IDC_STATIC_DEST_PORT, m_staticDestPort);
	DDX_Control(pDX, IDC_STATIC_ACTION, m_staticAction);
	DDX_Control(pDX, IDC_STATIC_PROTOCOL, m_staticProtocol);
	DDX_Control(pDX, IDC_BUT_SRCIP_PICKER, m_cbt_SrcIPPicker);
	DDX_Control(pDX, IDC_BUT_DEST_IP_PICKER, m_cbt_DestIPPicker);
	DDX_Control(pDX, IDC_BUT_SRC_PORT_PICKER, m_cbtSrcPortMenu);
	DDX_Control(pDX, IDC_BUT_DEST_PORTPICKER, m_cbtDestPortMenu);
}

BEGIN_MESSAGE_MAP(CAddRuleDlg, CDialog)
//{{AFX_MSG_MAP(CAddRuleDlg)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_EN_KILLFOCUS(IDC_SADD, OnKillfocusSadd)
	ON_EN_KILLFOCUS(IDC_DADD, OnKillfocusDadd)
	ON_BN_CLICKED(IDC_ADDSAVE, OnAddsave)
	ON_MESSAGE(WM_USER + 100, On_MenuCallback)
//}}AFX_MSG_MAP
ON_COMMAND(ID__DESTMYMACHINE, OnDestIpMyMachine)
ON_COMMAND(ID__DESTANYMACHINE, OnDestIPAnyMachine)
ON_COMMAND(ID__SRCIPMYMACHINE, OnSrcIpMyMachine)
ON_COMMAND(ID__SRCIPANYMACHINE, OnSrcIpAnyMachine)
ON_COMMAND(ID__ANYPORT, OnSrcAnyPort)
ON_COMMAND(ID__FTP, OnSrcPortFTP)
ON_COMMAND(ID__SSH, OnSrcPortSSH)
ON_COMMAND(ID__TELNET32821, OnSrcPortTelnet)
ON_COMMAND(ID__SMTP, OnSrcPortSMTP)
ON_COMMAND(ID__DNS, OnSrcPortDNS)
ON_COMMAND(ID__BOOTP, OnSrcPortBootp)
ON_COMMAND(ID__HTTP, OnSrcPortHTTP)
ON_COMMAND(ID__POP3, OnSrcPortPOP3)
ON_COMMAND(ID__NNTP, OnSrcPortNNTP)
ON_COMMAND(ID__NETBIOSNAMESERVICENBTSTAT, OnSrcPortNetBIOS)
ON_COMMAND(ID__SOCKS, OnSrcPortSOCKS)
ON_COMMAND(ID__ANYPORT1, OnDestPortAny)
ON_COMMAND(ID__FTP1, OnDestPortFTP)
ON_COMMAND(ID__SSH1, OnDestPortSSH)
ON_COMMAND(ID__TELNET328211, OnDestPortTelnet)
ON_COMMAND(ID__SMTP1, OnDestPortSMTP)
ON_COMMAND(ID__DNS1, OnDestPortDNS)
ON_COMMAND(ID__BOOTP1, OnDestPortDHCP)
ON_COMMAND(ID__HTTP1, OnDestPortHTTP)
ON_COMMAND(ID__POP31, OnDestPortPOP3)
ON_COMMAND(ID__NNTP1, OnDestPortNNTP)
ON_COMMAND(ID__NETBIOSNAMESERVICENBTSTAT1, OnDestPortNetBIOS)
ON_COMMAND(ID__SOCKS1, OnDestPortSocks)
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CAddRuleDlg message handlers
//**************************************************************************

/*---------------------------------------------------------------------------------------------
Name				:	AddFilter(IPFilter)
Purpose				:	Add the filter rule to driver
Parameters			:
						IPFilter  - Ip Filter structure
Return				:	<Return Description>
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
DWORD CAddRuleDlg::AddFilter(IPFilter pf)
{
	DWORD	result = ipFltDrv.WriteIo(ADD_FILTER, &pf, sizeof(pf));

	if(result != DRV_SUCCESS)
	{
		AfxMessageBox(_T("Unable to add rule to the driver"),MB_ICONEXCLAMATION);

		return(FALSE);
	}
	else
	{
		return(TRUE);
	}
}

/*---------------------------------------------------------------------------------------------
Name				:	OnAdd(void)
Purpose				:	Add the event handlar for the add rule button Initilize the driver with the added rules
Parameters			:	None.
Return				:	void
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
void CAddRuleDlg::OnAdd()
{
	UpdateData();

	BOOL	setact;
	int		setproto = 1;
	int		action = m_action.GetCurSel();
	TCHAR	ch[30];

	if(action == 0)
	{
		setact = FALSE;
	}
	else
	{
		setact = TRUE;
	}

	int proto = m_protocol.GetCurSel();
	if(proto == 0)
	{
		setproto = 1;
	}

	if(proto == 1)
	{
		setproto = 6;
	}

	if(proto == 2)
	{
		setproto = 17;
	}

	wsprintf(ch, _T("Action: %d, Protocol %d"), action, proto);
	AfxMessageBox(_T("Rule Added successfully"),MB_ICONINFORMATION);

	IPFilter	ip;
	ip.destinationIp = inet_addr(CStringA(m_strDestIPAddr));
	ip.destinationMask = inet_addr("255.255.255.255");
	ip.destinationPort = static_cast<USHORT>(htons((USHORT)_ttoi((LPCTSTR) m_strDestPortNo)));
	ip.sourceIp = inet_addr(CStringA(m_strSrcIPAddr));
	ip.sourceMask = inet_addr("255.255.255.255");
	ip.sourcePort = static_cast<USHORT>(htons((USHORT)_ttoi((LPCTSTR) m_strSrcPortNo)));
	ip.protocol = static_cast<USHORT>(setproto);
	ip.drop = setact;

	/*DWORD	result = */AddFilter(ip);
}
/*---------------------------------------------------------------------------------------------
Name				:	Verify(CString)
Purpose				:	Verify the vaidity of the ip address
Parameters			:
						CString  - String to be Varified
Return				:	<Return Description>
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
BOOL CAddRuleDlg::Verify(CString str)
{
	
	int		pos = 0, prevpos = -1;	// Keeps track of current and previous

	// positins in the string
	CString str1;

	// if string doesn't contains any . it means it is invalid IP
	if(str.Find('.') == -1)
	{
		return(FALSE);
	}

	// if the input string contains any invalid entry like any alpahabets
	// or it may contains some symbols then report it as an error
	if(str.FindOneOf(_T("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")) != -1)
	{
		return(FALSE);
	}

	if(str.FindOneOf(_T("!@#$%^&*()_+|-;:'\"/?><,")) != -1)
	{
		return(FALSE);
	}

	// if string contains . but not at the right position this will
	// return false for that
	int _pos = 0;
	_pos = str.Find('.');
	if((0 > _pos) || (_pos > 3))
	{
		return(FALSE);
	}

	int newpos = _pos;
	_pos = str.Find('.', _pos + 1);
	if((newpos + 1 >= _pos) || (_pos > newpos + 4))
	{
		return(FALSE);
	}

	newpos = _pos;
	_pos = str.Find('.', _pos + 1);
	if((newpos + 1 >= _pos) || (_pos > newpos + 4))
	{
		return(FALSE);
	}

	//if a dot is found verify that the ip address is within valid
	// range 0.0.0.0  & 255.255.255.255
	for(int cnt = 0; cnt <= 3; cnt++)
	{
		if(cnt < 3)
		{
			pos = str.Find('.', pos + 1);
		}
		else
		{
			pos = str.GetLength();
		}

		str1 = str.Left(pos);

		//char	ch[30];

		str1 = str1.Right(pos - (prevpos + 1));

		unsigned int	a = _ttoi(LPCTSTR(str1));
		if((0 > a) || (a > 255))
		{
			return(FALSE);
		}

		prevpos = pos;
	}

	return(TRUE);
}

//*****************************************************************

/*********************************************
 * event handlar when the focus is lost from source Ip address 
 *This will check wether the IP address you had given
 * corresponds to a valid IP address or not. If not it
 * will prompt you for a valid IP address.
 *
 * @return void 
 ********************************************/
void CAddRuleDlg::OnKillfocusSadd()
{

}

/*********************************************
 * event handlar when the focus is lost from Destination Ip address 
 *This will check wether the IP address you had given
 * corresponds to a valid IP address or not. If not it
 * will prompt you for a valid IP address.
 * @return void 
 ********************************************/
void CAddRuleDlg::OnKillfocusDadd()
{

}
BOOL CAddRuleDlg::CheckDataValidity()
{
	BOOL	bresult = Verify(m_strSrcIPAddr);
	if(bresult == FALSE)
	{
		AfxMessageBox(_T("Invalid IP Address"),MB_ICONSTOP);
		GetDlgItem(IDC_SADD)->SetFocus();
		return FALSE;
	}
	bresult = Verify(m_strDestIPAddr);
	if(bresult == FALSE)
	{
		AfxMessageBox(_T("Invalid IP Address"),MB_ICONSTOP);
		GetDlgItem(IDC_DADD)->SetFocus();
		return FALSE;
	}
	return TRUE;

}

/*********************************************
 *this function will both add the rule to driver and save them in the rule file so that next time you start
 * the firewall that rule will be loaded from the rule file
 *
 * @return void 
 ********************************************/
void CAddRuleDlg::OnAddsave()
{
	UpdateData();
	if(!CheckDataValidity())
	{
		return;
	}
	

	BOOL	setact;
	int		setproto = 0;
	int		action = m_action.GetCurSel();
	//char	ch[30];

	if(action == 0)
	{
		setact = FALSE;
	}
	else
	{
		setact = TRUE;
	}

	int proto = m_protocol.GetCurSel();
	if(proto == 0)
	{
		setproto = 1;
	}

	if(proto == 1)
	{
		setproto = 6;
	}

	if(proto == 2)
	{
		setproto = 17;
	}

	//wsprintf(ch, "Action: %d, Protocol %d", action, proto);
	//MessageBox(ch);

	IPFilter	ip;
	ip.destinationIp = inet_addr(CStringA( m_strDestIPAddr));
	ip.destinationMask = inet_addr("255.255.255.255");
	ip.destinationPort = static_cast<USHORT>(htons((USHORT)_ttoi((LPCTSTR) m_strDestPortNo)));
	ip.sourceIp = inet_addr(CStringA( m_strSrcIPAddr));
	ip.sourceMask = inet_addr("255.255.255.255");
	ip.sourcePort = static_cast<USHORT>(htons((USHORT)_ttoi((LPCTSTR) m_strSrcPortNo)));
	ip.protocol = static_cast<USHORT>(setproto);
	ip.drop = setact;
	//If Edit rule Mode Just update the rule with new values
	if(m_bEditRuleMode)
	{
		if(m_pActiveRule)
		{
			m_pActiveRule->SetIpFilter(ip);
		}
	}
	else //In Add rule mode add the new rule to firewall
	{
		CNetDefenderRules* objRule = new CNetDefenderRules;
		objRule->SetIpFilter(ip);
		CMainFrame* pmain = ((CMainFrame*)AfxGetMainWnd());
		pmain->m_liFirewallRules.AddTail(objRule);
	}
	
	//Save the rule to Rule file
	CFile FileObj;
	if(FileObj.Open(RULE_FILE_NAME,CFile::modeReadWrite|CFile::modeCreate))
	{
		CArchive ar( &FileObj, CArchive::store );
		//Serialize the list to save the rule to rule file
		((CMainFrame*)AfxGetMainWnd())->m_liFirewallRules.Serialize(ar);
	}
	//If Edit rule Mode do nothing
	if(!m_bEditRuleMode)
	{
		//If Add rule Dialog - Add the rule to Firewall
		AddFilter(ip);
	}
	
	OnOK();
}

/*********************************************
 This will open an existing file or open a new file if the file
   doesnot exists
 * @param  
 * @return BOOL 
 ********************************************/
BOOL CAddRuleDlg::NewFile(void)
{
	_hFile = CreateFile(_T("saved.rul"),
						GENERIC_WRITE | GENERIC_READ,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						OPEN_ALWAYS,
						NULL,
						NULL);

	/* If unable to obtain the handle values*/
	if(_hFile == INVALID_HANDLE_VALUE)
	{
		return(FALSE);
	}

	return(TRUE);	// File has been opened succesfully
}

/*********************************************
This will move the file pointer to the end of the file so that 
// it can be easily added to the file
 *
 * @param  
 * @return DWORD 
 ********************************************/
DWORD CAddRuleDlg::GotoEnd(void)
{
	DWORD	val;

	DWORD	size = GetFileSize(_hFile, NULL);
	if(size == 0)
	{
		return(size);
	}

	val = SetFilePointer(_hFile, 0, NULL, FILE_END);

	/* If unable to set the file pointer to the end of the file */
	if(val == 0)
	{
		AfxMessageBox(_T("Unable to set file pointer"),MB_ICONEXCLAMATION);
		return(GetLastError());
	}

	/* IF all goes well then return the current file position */
	//	else
	//	{
	return(val);

	//	}
}

/*********************************************
 * This code will save the data into the file which is given by the parameter
 *
 * @param name of the file to which data will be saved 
 * @return DWORD 
 ********************************************/
DWORD CAddRuleDlg::SaveFile(LPCTSTR str)
{
	DWORD	bytesWritten;

	/* Try to write the string passed as parameter to the file and if any 
   error occurs return the appropriate values
*/
	DWORD	_len = _tcslen(str)* sizeof (TCHAR);

	//	MessageBox(str);
	//	DWORD	filepointer =
	//	if(LockFile(
	//	char	ch[100];
	//	_tcscpy(ch,(LPCTSTR)str);
	if(WriteFile(_hFile, str, _len, &bytesWritten, NULL) == 0)
	{
		AfxMessageBox(_T("Unable to write to the file\n"),MB_ICONEXCLAMATION);
		return(FALSE);
	}

	/* If all goes well then return TRUE */
	return(TRUE);
}

/*********************************************
 * This function will close the existing file 
 *
 * @return BOOL 
 ********************************************/
BOOL CAddRuleDlg::CloseFile()
{
	// if the file handle does not exist report it to user and then
	// return  the appropriate value
	if(!_hFile)
	{
		//	MessageBox("File handle does not exist");
		return(FALSE);
	}

	// if there is an appropriate handle then close it and return app values
	else
	{
		if(CloseHandle(_hFile) != 0)
		{
			//MessageBox("File Handled closed");
			return(TRUE);
		}
		else
		{
			return(FALSE);
		}
	}
}

BOOL CAddRuleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	init();
	UpdateData(FALSE);
	

	SetIcon(AfxGetApp()->LoadIcon(MAKEINTRESOURCE(IDR_MAINFRAME)), TRUE);
	SetIcon(AfxGetApp()->LoadIcon(MAKEINTRESOURCE(IDR_MAINFRAME)), FALSE);
	CMainFrame* pMainWnd = (CMainFrame*)AfxGetMainWnd();
	SetIcon(pMainWnd->m_pImgList.ExtractIcon(14), TRUE);
	SetIcon(pMainWnd->m_pImgList.ExtractIcon(14), FALSE);

	//For add rule Button
	//m_btnRuleAdd.SetFlat(FALSE);
	//For Add and save button
	//m_btnRuleAddSave.SetFlat(FALSE);
	//Button for Adding and Saving Rule to Firewall 
	m_btnRuleAddSave.SetIcon(pMainWnd->m_pImgList.ExtractIcon(14));
	m_btnRuleAddSave.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, 30);
	m_btnRuleAddSave.DrawFlatFocus(TRUE);
	//m_btnRuleAddSave.EnableBalloonTooltip();
	//m_btnRuleAddSave.SetTooltipText(_T("Add the new rule to Netdefender and also saves it to rule file"));

	//For Cancel button
	//m_btnCancel.SetFlat(FALSE);
	//Button for Cancel Button
	m_btnCancel.SetIcon(pMainWnd->m_pImgList.ExtractIcon(1));
	m_btnCancel.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, 30);
	m_btnCancel.DrawFlatFocus(TRUE);
	//button for Menu shown to user for choosing Source IP
	m_cbt_SrcIPPicker.SetIcon(pMainWnd->m_pImgList.ExtractIcon(39));
	m_cbt_SrcIPPicker.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, 30);
	m_cbt_SrcIPPicker.OffsetColor(CButtonST::BTNST_COLOR_BK_FOCUS, 25);
	//button for Menu shown to user for choosing Destination IP
	m_cbt_DestIPPicker.SetIcon(pMainWnd->m_pImgList.ExtractIcon(39));
	m_cbt_DestIPPicker.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, 30);
	m_cbt_DestIPPicker.OffsetColor(CButtonST::BTNST_COLOR_BK_FOCUS, 25);
	//button for Menu shown to user for choosing Source Port
	m_cbtSrcPortMenu.SetIcon(pMainWnd->m_pImgList.ExtractIcon(39));
	m_cbtSrcPortMenu.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, 30);
	m_cbtSrcPortMenu.OffsetColor(CButtonST::BTNST_COLOR_BK_FOCUS, 25);
	//button for Menu shown to user for choosing Destination Port
	m_cbtDestPortMenu.SetIcon(pMainWnd->m_pImgList.ExtractIcon(39));
	m_cbtDestPortMenu.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, 30);
	m_cbtDestPortMenu.OffsetColor(CButtonST::BTNST_COLOR_BK_FOCUS, 25);
#ifdef	BTNST_USE_BCMENU
	m_cbt_SrcIPPicker.SetMenu(IDR_MENU_SRCIP, m_hWnd, TRUE);
	m_cbt_DestIPPicker.SetMenu(IDR_MENU_DESTIP, m_hWnd, TRUE);
	m_cbtSrcPortMenu.SetMenu(IDR_MENU_SRC_PORT, m_hWnd, TRUE);
	m_cbtDestPortMenu.SetMenu(IDR_MENU_DESTPORT, m_hWnd, TRUE);
#else
	m_cbt_SrcIPPicker.SetMenu(IDR_MENU_SRCIP, m_hWnd);
	m_cbt_DestIPPicker.SetMenu(IDR_MENU_DESTIP, m_hWnd, TRUE);
	m_cbtSrcPortMenu.SetMenu(IDR_MENU_SRC_PORT, m_hWnd, TRUE);
	m_cbtDestPortMenu.SetMenu(IDR_MENU_DESTPORT, m_hWnd, TRUE);
#endif
	m_cbt_SrcIPPicker.SetMenuCallback(m_hWnd, WM_USER + 100);
	m_staticAddNewRule.SetFontBold( TRUE );
	if(!m_strProtocol.IsEmpty())
	{
		m_protocol.SelectString(0,m_strProtocol);
	}
	if(!m_strAction.IsEmpty())
	{
		m_action.SelectString(0,m_strAction);
	}

	SettingForEditMode();
	// Create the CPPToolTip object
	m_tooltip.Create(this);
	m_tooltip.SetBehaviour(PPTOOLTIP_MULTIPLE_SHOW);
	m_tooltip.SetNotify();
	AddToolTips();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
void CAddRuleDlg::SetDlgWithRules(CNetDefenderRules*  pNetDefenderRule)
{
	in_addr		stAddr;
	//prepare the local address field
	stAddr.s_addr = static_cast<u_long>(pNetDefenderRule->m_nDestinationIp);
	m_strDestIPAddr = inet_ntoa(stAddr);
	//prepare the local address field
	stAddr.s_addr = static_cast<u_long>(pNetDefenderRule->m_nSourceIp);
	m_strSrcIPAddr = inet_ntoa(stAddr);
	int nTemp  = ntohs(static_cast<u_short>(pNetDefenderRule->m_nDestinationPort));
	m_strDestPortNo.Format(_T("%d"),nTemp);
	nTemp  = ntohs(static_cast<u_short>(pNetDefenderRule->m_nSourcePort));
	m_strSrcPortNo.Format(_T("%d"),nTemp);
	m_strProtocol = intToPotocol(pNetDefenderRule->m_nProtocol);
	m_strAction = BoolToAction(pNetDefenderRule->m_bDrop);
	m_bEditRuleMode = TRUE; //Set the Edit rule mode to TRUE
	m_pActiveRule = pNetDefenderRule;

	
}
void CAddRuleDlg::SettingForEditMode()
{
	if(m_bEditRuleMode)
	{
		m_btnRuleAddSave.SetWindowText(_T("&Save Rule"));
	}

}
void CAddRuleDlg::init()
{
	if(!m_bEditRuleMode)
	{
		m_strDestIPAddr = _T("");
		m_strDestPortNo = _T("");
		m_strSrcIPAddr = _T("");
		m_strSrcPortNo = _T("");
		m_protocol.SelectString(0,_T("TCP"));
		m_action.SelectString(0,_T("Deny"));
	}
}
BOOL CAddRuleDlg::PreTranslateMessage(MSG* pMsg)
{
	// must add RelayEvent function call to pass a mouse message to a tool tip control for processing.
	m_tooltip.RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}
void CAddRuleDlg::AddToolTips()
{
	m_tooltip.AddTool((CWnd*)&m_protocol,
		_T("Choose the network communication protocol. If unsure left it as TCP"));
	m_tooltip.AddTool((CWnd*)&m_action,
		_T("Choose the Action whether to <b>allow</b> the Traffic or <b>Deny</b> the traffic specifying the rule."));

	m_tooltip.AddTool(GetDlgItem(IDC_SADD),
		_T("Fill the <b>IP address</b> of the machine which originate the Call.<br><font color='red'><b>For Example :</b></font><br>If My machine is connecting to an <i>FTP server</i> running<br>at IP address <b>243.168.6.45</b>. Now to block all the traffic originated from the FTP server We needs to give <b>Source Ip</b> as <b>243.168.6.45</b><br><br><b>Note :</b> If network traffic can come from any IP address the choose IP as <b>0.0.0.0</b>."));
	m_tooltip.AddTool(GetDlgItem(IDC_DADD),
		_T("Fill the <b>IP address</b> of the machine to which connection request is going.<br><font color='red'><b>For Example :</b></font><br>If My machine is connecting to an <i>FTP server</i> running<br>at IP address <b>243.168.6.45</b>. Now to block all the any connection from my machine to FTP server We needs to give <b>Destination Ip</b> as <b>243.168.6.45</b><br><br><b>Note :</b> If network traffic can go to any IP address the choose IP as <b>0.0.0.0</b>."));
	m_tooltip.AddTool(GetDlgItem(IDC_SPORT),
		_T("Fill the <b>Port number</b> of the machine which originate the Call.<br><font color='red'><b>For Example :</b></font><br>If My machine is connecting to an <i>FTP server</i> running<br>at Port Number <b>21</b>. Now to block all the calls to FTP server We needs to give <b>Source port </b> as any port i.e <b>0</b><br><br><b>Note :</b> If network traffic can come from any Port the choose Port number as <b>0</b>."));
	m_tooltip.AddTool(GetDlgItem(IDC_DPORT),
		_T("Fill the <b>Port number</b> of the machineto which connection request is going.<br><font color='red'><b>For Example :</b></font><br>If My machine is connecting to an <i>FTP server</i> running<br>at Port Number <b>21</b>. Now to block all the calls to FTP server We needs to give <b>Destination port </b> as port number <b>21</b><br><br><b>Note :</b> If network traffic can go to any Port the choose Port number as <b>0</b>."));
	m_tooltip.AddTool((CWnd*)&m_btnRuleAddSave,
		_T("Add the new rule to <b>Netdefender</b> and also saves it to rule file."));
	m_tooltip.AddTool((CWnd*)&m_btnCancel,
		_T("Dismiss the dialog without saving the rules"));

	m_tooltip.SetColorBk(RGB(255, 255, 255), RGB(240, 247, 255), RGB(192, 192, 208));
	m_tooltip.SetEffectBk(CPPDrawManager :: EFFECT_SOFTBUMP);
	m_tooltip.SetMaxTipWidth(400);
	m_tooltip.EnableEscapeSequences(TRUE);

}
LRESULT CAddRuleDlg::On_MenuCallback(WPARAM wParam, LPARAM lParam)
{
//#ifdef	BTNST_USE_BCMENU
//	BCMenu*	pMenu = (BCMenu*)wParam;
//	pMenu->EnableMenuItem(IDM_ITEM3, TRUE);
//#else
//	::EnableMenuItem((HMENU)wParam, IDM_ITEM3, MF_BYCOMMAND | MF_GRAYED);
//#endif
	return 0;
}
void CAddRuleDlg::OnDestIpMyMachine()
{
	//Set the Current Ip Address of my machine on Destination Ip Field
	GetDlgItem(IDC_DADD)->SetWindowText(GetCurrentIp());
}

void CAddRuleDlg::OnDestIPAnyMachine()
{
	//Set the Any Ip Address(i.e 0.0.0.0) on Destination Ip Field
	GetDlgItem(IDC_DADD)->SetWindowText(NETDEF_ANY_IP);
}

void CAddRuleDlg::OnSrcIpMyMachine()
{
	//Set the Current Ip Address of my machine on Source Ip Field
	GetDlgItem(IDC_SADD)->SetWindowText(GetCurrentIp());
}

void CAddRuleDlg::OnSrcIpAnyMachine()
{
	//Set the Any Ip Address(i.e 0.0.0.0) on Source Ip Field
	GetDlgItem(IDC_SADD)->SetWindowText(NETDEF_ANY_IP);
	
}

void CAddRuleDlg::OnSrcAnyPort()
{
	GetDlgItem(IDC_SPORT)->SetWindowText(NETDEF_ANY_PORT);
}

void CAddRuleDlg::OnSrcPortFTP()
{
	GetDlgItem(IDC_SPORT)->SetWindowText(NETDEF_FTP_PORT);
}

void CAddRuleDlg::OnSrcPortSSH()
{
	GetDlgItem(IDC_SPORT)->SetWindowText(NETDEF_SSH_PORT);
}

void CAddRuleDlg::OnSrcPortTelnet()
{
	GetDlgItem(IDC_SPORT)->SetWindowText(NETDEF_TELNET_PORT);
}

void CAddRuleDlg::OnSrcPortSMTP()
{
	GetDlgItem(IDC_SPORT)->SetWindowText(NETDEF_SMTP_PORT);
}

void CAddRuleDlg::OnSrcPortDNS()
{
	GetDlgItem(IDC_SPORT)->SetWindowText(NETDEF_DNS_PORT);
}

void CAddRuleDlg::OnSrcPortBootp()
{
	GetDlgItem(IDC_SPORT)->SetWindowText(NETDEF_DHCP_PORT);
}

void CAddRuleDlg::OnSrcPortHTTP()
{
	GetDlgItem(IDC_SPORT)->SetWindowText(NETDEF_HTTP_PORT);
}

void CAddRuleDlg::OnSrcPortPOP3()
{
	GetDlgItem(IDC_SPORT)->SetWindowText(NETDEF_POP3_PORT);
}

void CAddRuleDlg::OnSrcPortNNTP()
{
	GetDlgItem(IDC_SPORT)->SetWindowText(NETDEF_NNTP_PORT);
}

void CAddRuleDlg::OnSrcPortNetBIOS()
{
	GetDlgItem(IDC_SPORT)->SetWindowText(NETDEF_NETBIOS_PORT);
}

void CAddRuleDlg::OnSrcPortSOCKS()
{
	GetDlgItem(IDC_SPORT)->SetWindowText(NETDEF_SOCKS_PORT);
}

void CAddRuleDlg::OnDestPortAny()
{
	GetDlgItem(IDC_DPORT)->SetWindowText(NETDEF_ANY_PORT);
}

void CAddRuleDlg::OnDestPortFTP()
{
	GetDlgItem(IDC_DPORT)->SetWindowText(NETDEF_FTP_PORT);
}

void CAddRuleDlg::OnDestPortSSH()
{
	GetDlgItem(IDC_DPORT)->SetWindowText(NETDEF_SSH_PORT);
}

void CAddRuleDlg::OnDestPortTelnet()
{
	GetDlgItem(IDC_DPORT)->SetWindowText(NETDEF_TELNET_PORT);
}

void CAddRuleDlg::OnDestPortSMTP()
{
	GetDlgItem(IDC_DPORT)->SetWindowText(NETDEF_SMTP_PORT);
}

void CAddRuleDlg::OnDestPortDNS()
{
	GetDlgItem(IDC_DPORT)->SetWindowText(NETDEF_DNS_PORT);
}

void CAddRuleDlg::OnDestPortDHCP()
{
	GetDlgItem(IDC_DPORT)->SetWindowText(NETDEF_DHCP_PORT);
}

void CAddRuleDlg::OnDestPortHTTP()
{
	GetDlgItem(IDC_DPORT)->SetWindowText(NETDEF_HTTP_PORT);
}

void CAddRuleDlg::OnDestPortPOP3()
{
	GetDlgItem(IDC_DPORT)->SetWindowText(NETDEF_POP3_PORT);
}

void CAddRuleDlg::OnDestPortNNTP()
{
	GetDlgItem(IDC_DPORT)->SetWindowText(NETDEF_NNTP_PORT);
}

void CAddRuleDlg::OnDestPortNetBIOS()
{
	GetDlgItem(IDC_DPORT)->SetWindowText(NETDEF_NETBIOS_PORT);
}

void CAddRuleDlg::OnDestPortSocks()
{
	GetDlgItem(IDC_DPORT)->SetWindowText(NETDEF_SOCKS_PORT);
}
