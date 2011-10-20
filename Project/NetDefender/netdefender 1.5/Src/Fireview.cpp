// fireView.cpp : implementation of the CFireView class
//
#pragma once
#include "stdafx.h"
#include "fire.h"

#include "fireDoc.h"
#include "fireView.h"
//#include "Sockutil.h"
//#include "Winsock2.h"

#include "RuleDesc.h"

//#include "DrvFltip.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFireView
IMPLEMENT_DYNCREATE(CFireView, CFormView)
BEGIN_MESSAGE_MAP(CFireView, CFormView)
//{{AFX_MSG_MAP(CFireView)
    ON_BN_CLICKED(IDC_BUT_ADDRULE, OnAddrule)
    ON_BN_CLICKED(IDC_START, OnStart)
    ON_BN_CLICKED(IDC_BLOCKPING, OnBlockping)
    ON_BN_CLICKED(IDC_BLOCKALL, OnBlockall)
    ON_BN_CLICKED(IDC_ALLOWALL, OnAllowall)
    ON_WM_CTLCOLOR()
    ON_WM_SHOWWINDOW()
    ON_UPDATE_COMMAND_UI(ID_Start, OnUpdateStart)
    ON_COMMAND(ID_STOP, OnStop)
    ON_UPDATE_COMMAND_UI(ID_STOP, OnUpdateStop)
    ON_UPDATE_COMMAND_UI(ID_ALLOWALL, OnUpdateAllowall)
    ON_UPDATE_COMMAND_UI(ID_BLOCKALL, OnUpdateBlockall)
    ON_COMMAND(ID_Start, OnStart)
    ON_COMMAND(ID_BLOCKALL, OnBlockall)
    ON_COMMAND(ID_ALLOWALL, OnAllowall)
    ON_COMMAND(ID_BLOCKPING, OnBlockping)
    ON_UPDATE_COMMAND_UI(ID_BLOCKPING, OnUpdateBlockping)
	ON_NOTIFY (UDM_TOOLTIP_DISPLAY, NULL, NotifyDisplayTooltip)
	ON_UPDATE_COMMAND_UI(IDC_BUT_EDIT_RULE, OnUpdateEditRule)
//}}AFX_MSG_MAP
    ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_RESULT, &CFireView::OnLvnItemchangedListResult)
	ON_BN_CLICKED(IDC_BUT_REMOVE_RULE, &CFireView::OnBnClickedButRemoveRule)
	ON_BN_CLICKED(IDC_BUT_EDIT_RULE, &CFireView::OnBnClickedButEditRule)
	ON_BN_CLICKED(IDC_REREGISTER_RULE, &CFireView::OnBnClickedReregisterRule)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CFireView construction/destruction

/**
 * CFireView:all the Initializtion  related to GUI is done here.
 * Like for start and stop indicator. and initial state of buttons.
 *
 * @return  
 */
CFireView::CFireView() : CFormView(CFireView::IDD)
{
    //{{AFX_DATA_INIT(CFireView)
    //}}AFX_DATA_INIT
    //********************************************************
    /**
	 * m_pBrush:Object of CBrush .
	 * use for drawing start and stop firewall indicator
	 */
    m_pBrush = new CBrush;
    ASSERT(m_pBrush);
    m_clrBk = RGB(148, 210, 252);
    m_clrText = RGB(0, 0, 0);
    m_pBrush->CreateSolidBrush(m_clrBk);

    //**************************
    //list control
    m_pColumns = new CStringList;
    ASSERT(m_pColumns);
    _rows = 1;
    start = TRUE;
    block = TRUE;
    allow = TRUE;
    ping = TRUE;
	m_style=StyleTile;
}

CFireView::~CFireView()
{
    if(m_pBrush)
    {
        delete m_pBrush;
    }
}

void CFireView::DoDataExchange(CDataExchange *pDX)
{
	CFormView :: DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFireView)
	DDX_Control(pDX, IDC_LIST_RESULT, m_cResult);
	DDX_Control(pDX, IDC_BLOCKPING, m_cping);
	DDX_Control(pDX, IDC_BLOCKALL, m_cblockall);
	DDX_Control(pDX, IDC_START, m_cstart);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_BUT_ADDRULE, m_btnAddRule);
	DDX_Control(pDX, IDC_BUT_EDIT_RULE, m_cEditRule);
	DDX_Control(pDX, IDC_BUT_REMOVE_RULE, m_cRemoveRule);
	DDX_Control(pDX, IDC_REREGISTER_RULE, m_btnReInitializeRule);
	
	DDX_Control(pDX, IDC_STATIC_RULE, m_lFirewallRule);
}

BOOL CFireView::PreCreateWindow(CREATESTRUCT &cs)
{
    //*****************************************************************
    /**
	 * LoadDriver:This function load the <b> IpFilterDriver </b>
	 *
	 * @param  : Name of the Driver
	 * @param  : Path of the driver
	 * @param  
	 * @param  
	 * @return m_filterDriver. 
	 */
    m_filterDriver.LoadDriver(_T("IpFilterDriver"), _T("System32\\Drivers\\IpFltDrv.sys"), NULL, TRUE);

    /**
	 * SetRemovable:we do not deregister the driver at destructor
	 *
	 * @param  
	 * @return m_filterDriver. 
	 */
    m_filterDriver.SetRemovable(FALSE);

    /**
	 * LoadDriver:This function load the <b> Flter - Hook Driver </b>
	 *
	 * @param  Driver Name
	 * @param  Driver Path
	 * @param  
	 * @param  
	 * @return m_ipFltDrv. 
	 */
    m_ipFltDrv.LoadDriver(_T("DrvFltIp"), NULL, NULL, TRUE);

    //****************************************************************
    return(CFormView :: PreCreateWindow(cs));
}

void CFireView::OnInitialUpdate()
{
    CFormView :: OnInitialUpdate();
    GetParentFrame()->RecalcLayout();
    ResizeParentToFit();

    //******************
    m_parent = (CMainFrame *) GetParent();
    // XListCtrl must have LVS_EX_FULLROWSELECT if combo boxes are used
    m_cResult.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_TRACKSELECT);
	CMainFrame* pMainWnd = (CMainFrame*)AfxGetMainWnd();
	SetBitmapStyle(StyleStretch);
	SetBitmap(IDB_BMP_BK);
	//For add rule Button
	m_btnAddRule.SetIcon(pMainWnd->m_pImgList.ExtractIcon(3));
	m_btnAddRule.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, 30);
	m_btnAddRule.DrawFlatFocus(TRUE);

	//For Add and save button
	m_cEditRule.SetIcon(pMainWnd->m_pImgList.ExtractIcon(14));
	m_cEditRule.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, 30);
	m_cEditRule.DrawFlatFocus(TRUE);
	m_cRemoveRule.SetIcon(pMainWnd->m_pImgList.ExtractIcon(21));
	m_cRemoveRule.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, 30);
	m_cRemoveRule.DrawFlatFocus(TRUE);

	//For Re Register Btn
	m_btnReInitializeRule.SetIcon(pMainWnd->m_pImgList.ExtractIcon(53));
	m_btnReInitializeRule.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, 30);
	m_btnReInitializeRule.DrawFlatFocus(TRUE);
	m_lFirewallRule.SetFontBold( TRUE );

    // call EnableToolTips to enable tool tip display
    m_cResult.EnableToolTips(TRUE);
    ShowHeaders();
    m_cResult.SetGridLines(TRUE);   // SHow grid lines
	// Create the CPPToolTip object
	m_tooltip.Create(this);
	m_tooltip.SetBehaviour(PPTOOLTIP_MULTIPLE_SHOW);
	m_tooltip.SetNotify();
	AddToolTips();
}

/////////////////////////////////////////////////////////////////////////////
// CFireView diagnostics
#ifdef _DEBUG
void CFireView::AssertValid() const
{
    CFormView :: AssertValid();
}

void CFireView::Dump(CDumpContext &dc) const
{
    CFormView :: Dump(dc);
}

CFireDoc *CFireView::GetDocument()  // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFireDoc)));
    return (CFireDoc *) m_pDocument;
}
#endif //_DEBUG
    	
    	/////////////////////////////////////////////////////////////////////////////

// CFireView message handlers

/**
 * OnAddrule:Event handler for Add Rule Button
 * It will display add rule dialog box
 *
 * @return void 
 */
void CFireView::OnAddrule()
{
	//CAddRuleDlg objRuleDlg;
    m_Addrule.DoModal();
	RegisterRulesFromFile();
	
}

/*---------------------------------------------------------------------------------------------
Name				:	OnStart(void)
Purpose				:	Event handler for Start Button.It will start the firewall
Parameters			:	None.
Return				:	<Return Description>
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
void CFireView::OnStart()
{
    // if the filter is started sucessfully
    CString _text;

    //_text = "";
    // obtain the current state of the button and if the state is "start"
    // then perform the requested operation
    m_cstart.GetWindowText(_text);

    if(_text != "Stop")
    {
        if(m_ipFltDrv.WriteIo(START_IP_HOOK, NULL, 0) != DRV_ERROR_IO)
        {
            AfxMessageBox(_T("Firewall Started Successfully"),MB_ICONINFORMATION);

            //////////////////////////////////////////
            start = FALSE;
            GetDocument()->setIsFirewallStart();

            m_cstart.SetWindowText(_T("Stop"));

            //BOOL tmp = m_SysTray.SetTooltipText("Firewall Stops");
            //Change the led to indicate that Firewall has Started
            m_parent->SetOnlineLed(TRUE);
            m_parent->SetOfflineLed(FALSE);
        }
    }

    // else if the current text on the button is stop perform the operation
    // below
    else
    {
        if(m_ipFltDrv.WriteIo(STOP_IP_HOOK, NULL, 0) != DRV_ERROR_IO)
        {
            AfxMessageBox(_T("Firewall Stopped Successfully"),MB_ICONINFORMATION);
            m_cstart.SetWindowText(_T("Start"));
            start = TRUE;

            //Change the led to indicate that Firewall is Stoped
            m_parent->SetOnlineLed(FALSE);
            m_parent->SetOfflineLed(TRUE);

            //BOOL tmp = m_SysTray.SetTooltipText("Firewall Running");
        }
    }

    //	m_bStart.EnableWindow(FALSE);
    //	m_bStop.EnableWindow(TRUE);
    //	}
}

/*---------------------------------------------------------------------------------------------
Name				:	OnBlockping(void)
Purpose				:	Event handler for Block Ping Button.It will Block all the ping commands 
Parameters			:	None.
Return				:	None
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
void CFireView::OnBlockping()
{
    // Check if the firewall is already started
    if(!GetDocument()->ISFirewallStarted())
    {
        return;
    }

    if(AfxMessageBox(_T("Are you sure to block all Incoming Ping Messages?"),  MB_YESNO|MB_ICONQUESTION) == IDYES)
    {
        IPFilter    IPflt;

        IPflt.protocol = 1;         // ICMP Protocol
        IPflt.destinationIp = 0;    //inet_addr("127.0.0.1");	// all destinations
        IPflt.destinationMask = 0;  //inet_addr("255.255.255.255");// all destination masks
        IPflt.destinationPort = 0;  // all ports
        IPflt.sourceIp = 0;         // drop all packets irrespective of source
        IPflt.sourceMask = 0;
        IPflt.sourcePort = 0;       // from any source port
        IPflt.drop = TRUE;

        /**
		* AddFilter:Add rile to the filter- hook driver
		*
		* @param  IPFilter
		* @return m_Addrule. 
		*/
        m_Addrule.AddFilter(IPflt);

        //m_cping.EnableWindow(FALSE);
        //ping = FALSE;
        //allow = TRUE;
        //block = TRUE;
        GetDocument()->setBlockPing();
    }
}

/*---------------------------------------------------------------------------------------------
Name				:	OnBlockall(void)
Purpose				:	Event handler for Block ALL Button.It will Block all the the traffic both incomming and outgoing
Parameters			:	None.
Return				:	<Return Description>
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
void CFireView::OnBlockall()
{
    // Check if the firewall is already started
    if(!GetDocument()->ISFirewallStarted())
    {
        return;
    }

    if(AfxMessageBox(
           _T("This action will prevent any further transferor receiving of the data to and from your computer, Are you sure to proceed with it?")
          , MB_YESNO|MB_ICONQUESTION) == IDYES)
    {
        //	CAddRuleDlg     rule;
        IPFilter    IPflt;

        IPflt.protocol = 0;         // all the protocols
        IPflt.destinationIp = 0;    // all destinations
        IPflt.destinationMask = 0;  // all destination masks
        IPflt.destinationPort = 0;  // all ports
        IPflt.sourceIp = 0;         // drop all packets irrespective of source
        IPflt.sourceMask = 0;
        IPflt.sourcePort = 0;       // from any source port
        IPflt.drop = TRUE;

        m_Addrule.AddFilter(IPflt);

        // Disable this button till further notice
        //m_cblockall.SetCheck(0);
        //block = FALSE;
        //ping = FALSE;
        //allow = TRUE;
        //m_cblockall.EnableWindow(FALSE);
        GetDocument()->setBlockAll();
    }
}

/*---------------------------------------------------------------------------------------------
Name				:	OnAllowall(void)
Purpose				:	Event handler for Allow all Button.	It will aloow all traffic to pass threw firewall.
						It will clear all the rules.
Parameters			:	None.
Return				:	void
Globals Modified	:	None.
--------------------------------------------------------------------------------------------*/
void CFireView::OnAllowall()
{
    // Check if the firewall is already started
    if(!GetDocument()->ISFirewallStarted())
    {
        return;
    }

    if(AfxMessageBox(_T("This action will clear all the rules from the firewall?"), MB_YESNO|MB_ICONQUESTION) == IDYES)
    {
        if(m_ipFltDrv.WriteIo(CLEAR_FILTER, NULL, 0) != DRV_ERROR_IO)
        {
            AfxMessageBox(_T("All Rules had been cleared"),MB_ICONQUESTION);
            m_cResult.DeleteAllItems();
            m_cping.EnableWindow();
            m_cblockall.EnableWindow();

            //allow = FALSE;
            //block = TRUE;
            //ping = TRUE;
            GetDocument()->setAllowAll();

            _rows = 1;
        }
    }
}

BOOL CFireView::Create(LPCTSTR         lpszClassName,
                       LPCTSTR         lpszWindowName,
                       DWORD           dwStyle,
                       const RECT      &rect,
                       CWnd            *pParentWnd,
                       UINT            nID,
                       CCreateContext  *pContext)
{
    return(CFormView :: Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext));
}

//***********************************************************************

/**
 * OnCtlColor:Change the background color of the dialog 
 *
 * @param pDC : pointer to CDC Class
 * @param pWnd 
 * @param nCtlColor 
 * @return HBRUSH 
 */
HBRUSH CFireView::OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor)
{
    /*HBRUSH	hbr = */
    CFormView :: OnCtlColor(pDC, pWnd, nCtlColor);

    //break statement must be ignored:
   switch(nCtlColor)
    {
        case CTLCOLOR_BTN:
        case CTLCOLOR_STATIC:
            pDC->SetBkColor(m_clrBk);
            pDC->SetTextColor(m_clrText);

        case CTLCOLOR_DLG:
            return (static_cast<HBRUSH>(m_pBrush->GetSafeHandle ()));
    }

    return(CFormView :: OnCtlColor(pDC, pWnd, nCtlColor));
}

/**
 * ImplementRule:This will read the rules from file and 
 * Show in list control
 *
 * @param  
 * @return BOOL 
 */
BOOL CFireView::ImplementRule(void)
{
	CMainFrame* pmain = ((CMainFrame*)AfxGetMainWnd());
	_rows = 0;
	for( POSITION pos = pmain->m_liFirewallRules.GetHeadPosition(); pos != NULL; )
	{
		CNetDefenderRules* objRule =  (CNetDefenderRules*)pmain->m_liFirewallRules.GetNext( pos );
		//adds the Source IP
		//adds the CNetDefenderRules object to list control as LPARAM
		AddItem(0, 0, GiveDescIp(objRule->m_nSourceIp),-1,(LPARAM)objRule);
		//adds the Destination IP
		AddItem(0, 1, GiveDescIp(objRule->m_nDestinationIp));
		//adds the Source Port
		AddItem(0, 2, PortNoToStr(static_cast<USHORT>(objRule->m_nSourcePort)));
		//adds the destination Port
		AddItem(0, 3, PortNoToStr(static_cast<USHORT>(objRule->m_nDestinationPort)));
		//adds the Protocol Port
		AddItem(0, 4, intToPotocol(objRule->m_nProtocol));
		//adds the Action
		AddItem(0, 5, BoolToAction(objRule->m_bDrop));
		//adds the Source Mask
		AddItem(0, 6, LongToIPStr(objRule->m_nSourceMask));
		//Add the destination Mask
		AddItem(0, 7, LongToIPStr(objRule->m_nDestinationMask));

		//Register the rules with the firewall
		IPFilter    ip1;
		ip1 = objRule->GetIpFilter();
		m_Addrule.AddFilter(ip1);
		_rows = _rows + 1;

	}
	return TRUE;
}

BOOL CFireView::AddColumn(LPCTSTR strItem, int nItem, int nSubItem, int nMask, int nFmt)
{
    LV_COLUMN   lvc;
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

BOOL CFireView::AddItem(int nItem, int nSubItem, LPCTSTR strItem, int nImageIndex,LPARAM lParam)
{
    LV_ITEM lvItem;
	memset(&lvItem, 0, sizeof(lvItem));
	//LPARAM would be attached to the first column only
	if(nSubItem==0)
	{
		lvItem.mask = LVIF_TEXT|LVIF_PARAM;
		lvItem.lParam = lParam;
	}
	else
	{
		lvItem.mask = LVIF_TEXT;
	}
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

void CFireView::AddHeader(LPTSTR hdr)
{
    if(m_pColumns)
    {
        m_pColumns->AddTail(hdr);
    }
}

void CFireView::ShowHeaders()
{
    int         nIndex = 0;
    POSITION    pos = m_pColumns->GetHeadPosition();
    while(pos)
    {
        CString hdr = (CString) m_pColumns->GetNext(pos);
        AddColumn(hdr, nIndex++);
    }
}

/**
 * OnShowWindow: Override the default implementation to initialize List control headers
 *
 * @param bShow 
 * @param nStatus 
 * @return void 
 */
void CFireView::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CFormView :: OnShowWindow(bShow, nStatus);

	AddHeader(_T("SOURCE IP"));
	AddHeader(_T("DESTINATION IP"));
	AddHeader(_T("SOURCE PORT"));
	AddHeader(_T("DESTINATION PORT"));
	AddHeader(_T("PROTOCOL"));
	AddHeader(_T("ACTION"));
	AddHeader(_T("SOURCE MASK"));
    AddHeader(_T("DESTINATION MASK"));

}

void CFireView::OnUpdateStart(CCmdUI *pCmdUI)
{
    //CString str;
    //m_cstart.GetWindowText(str);
    pCmdUI->Enable(start);
}

/**
 * OnStop:event handler for Stop button
 * This will stop the firewall
 *
 * @return void 
 */
void CFireView::OnStop()
{
    OnStart();
    GetDocument()->setStopFirewall();
}

void CFireView::OnUpdateStop(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(!GetDocument()->getStopFirewall());
}

void CFireView::OnUpdateAllowall(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(!GetDocument()->getAllowAll());
}

void CFireView::OnUpdateBlockall(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(!GetDocument()->getBlockAll());
}

void CFireView::OnUpdateBlockping(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(!GetDocument()->getBlockPing());
}
void CFireView::AddToolTips()
{
	m_tooltip.AddTool((CWnd*)&m_btnAddRule,
		_T("Adds a new rule to the firewall. New rules can be defined based on following: <br><t>· IP address <br><t>· Port Number <br> <t>· Protocol <br>  "));

	m_tooltip.AddTool((CWnd*)&m_cEditRule,
		_T("Modify the selected rulem in the firewall"));
		//m_tooltip.SetNotify();
	m_tooltip.AddTool((CWnd*)&m_btnReInitializeRule,
			_T("Re-Initialize all rules of the <b>firewall</b> by loading the rules from the rule file again. <br>Most useful after calling <b>Allow All, Block All or Block Ping</b> commands."));
	m_tooltip.AddTool((CWnd*)&m_cRemoveRule,
		_T("Remove the selected rules from the <b>Netdefender</b>"));
		m_tooltip.SetNotify();
	m_tooltip.AddTool(&m_cResult);
	m_tooltip.SetColorBk(RGB(255, 255, 255), RGB(240, 247, 255), RGB(192, 192, 208));
	m_tooltip.SetEffectBk(CPPDrawManager :: EFFECT_SOFTBUMP);
	m_tooltip.SetMaxTipWidth(500);
	m_tooltip.EnableEscapeSequences(TRUE);


}
BOOL CFireView::PreTranslateMessage(MSG* pMsg)
{
	// must add RelayEvent function call to pass a mouse message to a tool tip control for processing.
	m_tooltip.RelayEvent(pMsg);
	return CFormView::PreTranslateMessage(pMsg);
}
void CFireView::NotifyDisplayTooltip(NMHDR * pNMHDR, LRESULT * result)
{
	*result = 0;
	NM_PPTOOLTIP_DISPLAY * pNotify = (NM_PPTOOLTIP_DISPLAY*)pNMHDR;

	if (NULL == pNotify->hwndTool)
	{
		//Order to update a tool tip for a current Tool tip Help
		//He has not a handle of the window
		//If you want change tooltip's parameter than make it's here
	}
	else
	{
		//Order to update a tool tip for a specified window as tooltip's tool
		//Gets a ID of the window if needed
		UINT nID = CWnd::FromHandle(pNotify->hwndTool)->GetDlgCtrlID();
		//Change a tooltip's parameters for a current window (control)
		//BOOL bOutside = FALSE;
		CPoint pt = *pNotify->pt;
		CRect rect, rcCtrl;
		if (IDC_LIST_RESULT == nID)
		{
			m_cResult.GetWindowRect(&rcCtrl);
			pt -= rcCtrl.TopLeft();
			LVHITTESTINFO li;
			li.pt = pt;
			int nItem = m_cResult.SubItemHitTest(&li);
			//int nSubItem = li.iSubItem;
			UINT nFlags =   li.flags;
			if (nFlags & LVHT_ONITEM)
			{
				CNetDefenderRules* pRules= (CNetDefenderRules*)m_cResult.GetItemData(nItem);
				CRuleDesc ruleDesc(pRules);
				pNotify->ti->sTooltip = ruleDesc.GetRuleDesc();//= m_cResult.GetItemText(li.iItem, li.iSubItem);
				CHeaderCtrl* pHeader = (CHeaderCtrl*)m_cResult.GetDlgItem(0);
				CRect rcHeader;
				Header_GetItemRect(pHeader->m_hWnd, li.iSubItem, &rcHeader);
				rcHeader.OffsetRect(-m_cResult.GetScrollPos(SB_HORZ), 0);
				if (rcHeader.PtInRect(pt))
				{
					//We are over the header
					CString str;
					CNetDefenderRules* pRules= (CNetDefenderRules*)m_cResult.GetItemData(nItem);
					CRuleDesc ruleDesc(pRules);
					str.Format(_T("%s"), ruleDesc.GetRuleDesc());
					pNotify->ti->sTooltip = str;
					pt = *pNotify->pt;
					pt.x += 5;
					pt.y += 20;
					*pNotify->pt = pt;
				}
				else
				{
					m_cResult.GetSubItemRect(li.iItem, li.iSubItem, LVIR_BOUNDS, rect);
					/*if (TRUE)
					{*/
						pt = rcCtrl.TopLeft();
						rect.OffsetRect(pt);
						rect.OffsetRect(1, 1);
						pt = rect.TopLeft();
						
						pt.y = rect.bottom;
						*pNotify->pt = pt;
					//}
					//else
					//{
					//	*pNotify->pt = CPoint(rcCtrl.left, rcCtrl.bottom);
					//} //if
				}
			} //if

		}

		//Change a tooltip's parameters for a current window (control)
	}
}//End NotifyDisplayTooltip
void CFireView::RegisterRulesFromFile()
{
	m_cResult.DeleteAllItems();
	ImplementRule();
	for(int n = 0; n < m_cResult.GetItemCount(); n++)
	{
		if(n % 2 == 0)
		{
			m_cResult.SetItemBkColor(n, -1, RGB(193, 193, 255));
		}
		else
		{
			m_cResult.SetItemBkColor(n, -1, RGB(255, 255, 255)/*RGB(180, 180, 255)*/);			
		}
	}

}
void CFireView::OnUpdateEditRule(CCmdUI *pCmdUI)
{
	//CString str;
	//m_cstart.GetWindowText(str);
	//int a = m_cResult.GetSelectedCount();
	pCmdUI->Enable(start);
}

void CFireView::OnLvnItemchangedListResult(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int nItemCount = m_cResult.GetSelectedCount();
	if(pNMLV->iItem >= 0 && pNMLV->iItem != -1 && nItemCount > 0)
	{
		m_cEditRule.EnableWindow(TRUE);
		m_cRemoveRule.EnableWindow(TRUE);
	}
	else
	{
		m_cEditRule.EnableWindow(FALSE);
		m_cRemoveRule.EnableWindow(FALSE);

	}
	*pResult = 0;
}

void CFireView::OnBnClickedButRemoveRule()
{
	int nItemCount = m_cResult.GetSelectedCount();
	if(nItemCount>0)
	{
		CNetDefenderRules*  pDelRule = GetSelectedItem();
		CMainFrame* pmain = ((CMainFrame*)AfxGetMainWnd());
		if(pmain)
		{
			POSITION pos = pmain->m_liFirewallRules.Find(pDelRule);
			pmain->m_liFirewallRules.RemoveAt(pos);
		}
		if(ReInitializeRules()==FALSE)
		{
			AfxMessageBox(_T("Unexpected error has occurred. NetDefender can't recovered from it.Try restarting the application"),MB_ICONERROR);
		}
	}
	
}
//Get the selected rules from the list ctrl
CNetDefenderRules* CFireView::GetSelectedItem()
{
	LVITEM Item;
	ZeroMemory(&Item, sizeof(LVITEM));
	Item.iItem =m_cResult.GetNextItem(-1 , LVNI_SELECTED);
	Item.mask = LVIF_PARAM |LVIF_TEXT | LVIF_IMAGE;
	m_cResult.GetItem(&Item);
	CNetDefenderRules* pDeleteComponents = (CNetDefenderRules*)Item.lParam;
	if(pDeleteComponents == NULL)
	{
		return NULL;
	}
	return pDeleteComponents;
}
//Re Register the rules to the firewall
BOOL CFireView::ReInitializeRules()
{
	//As there is no method in Driver to remove the rule from the firewall while it is running 
	//We needs to stop the firewall and then start it
	//Stop the firewall
	if(m_ipFltDrv.WriteIo(STOP_IP_HOOK, NULL, 0) == DRV_ERROR_IO)
	{
		//Fails to stop
		return FALSE;
	}
	//Restart the firewall
	if(m_ipFltDrv.WriteIo(START_IP_HOOK, NULL, 0) == DRV_ERROR_IO)
	{
		//Fails to Start
		return FALSE;
	}
	RegisterRulesFromFile();
	CFile FileObj;
	if(FileObj.Open(RULE_FILE_NAME,CFile::modeReadWrite|CFile::modeCreate))
	{
		CArchive ar( &FileObj, CArchive::store );
		((CMainFrame*)AfxGetMainWnd())->m_liFirewallRules.Serialize(ar);
	}
	return TRUE;
}

void CFireView::OnBnClickedButEditRule()
{
	int nItemCount = m_cResult.GetSelectedCount();
	if(nItemCount>0)
	{
		CNetDefenderRules*  pEditRule = GetSelectedItem();
		//CMainFrame* pmain = ((CMainFrame*)AfxGetMainWnd());
		CAddRuleDlg objRuleDlg;
		objRuleDlg.SetDlgWithRules(pEditRule);
		objRuleDlg.DoModal();

		//Re-Register the rules to the firewall
		if(ReInitializeRules()==FALSE)
		{
			AfxMessageBox(_T("Unexpected error has occurred. NetDefender can't recovered from it.Try restarting the application"),MB_ICONERROR);
		}
	}
}

void CFireView::OnBnClickedReregisterRule()
{
	ReInitializeRules();
}
BOOL CFireView::OnEraseBkgnd(CDC* pDC) 
{
	CFormView::OnEraseBkgnd(pDC);	
	if(!m_bitmap.m_hObject)
		return true;

	CRect rect;
	GetClientRect(&rect);
	CDC dc;
	dc.CreateCompatibleDC(pDC);
	CBitmap* pOldBitmap = dc.SelectObject(&m_bitmap);
	int bmw, bmh ;
	BITMAP bmap;
	m_bitmap.GetBitmap(&bmap);
	bmw = bmap.bmWidth;
	bmh = bmap.bmHeight;
	int xo=0, yo=0;

	if(m_style == StyleTile)
	{
		for (yo = 0; yo < rect.Height(); yo += bmh)
		{
			for (xo = 0; xo < rect.Width(); xo += bmw)
			{
				pDC->BitBlt (xo, yo, rect.Width(),
					rect.Height(), &dc,
					0, 0, SRCCOPY);
			}
		}

	}

	if(m_style == StyleCenter)
	{
		if(bmw < rect.Width())
			xo = (rect.Width() - bmw)/2;
		else 
			xo=0;
		if(bmh < rect.Height())
			yo = (rect.Height()-bmh)/2;
		else
			yo=0;
		pDC->BitBlt (xo, yo, rect.Width(),
			rect.Height(), &dc,
			0, 0, SRCCOPY);
	}

	if(m_style == StyleStretch)
	{
		pDC->StretchBlt(xo, yo, rect.Width(),
			rect.Height(), &dc,
			0, 0,bmw,bmh, SRCCOPY);
	}


	dc.SelectObject(pOldBitmap);


	return true;
}
void CFireView::SetBitmapStyle(int style)
{
	if((style==StyleTile)||
		(style==StyleCenter)||
		(style==StyleStretch))
	{	
		m_style = style;
	}

}

int CFireView::SetBitmap(UINT nIDResource)
{
	if(m_bitmap.LoadBitmap(nIDResource))
		return 0;
	else
		return 1;//error
}