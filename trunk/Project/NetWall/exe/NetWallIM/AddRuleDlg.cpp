// AddRuleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "netwall.h"
#include "AddRuleDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddRuleDlg dialog


CAddRuleDlg::CAddRuleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddRuleDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddRuleDlg)
	m_iSrcStartPort = 0;
	m_iSrcEndPort   = 65540;
	m_iDstStartPort = 0;
	m_iDstEndPort   = 65540;
	m_strMemo       = _T("");
	//}}AFX_DATA_INIT

    m_nType   = ADD_RULE;
    m_pBuffer = NULL;
    m_pItem   = NULL;
}

void CAddRuleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddRuleDlg)
	DDX_Control(pDX, IDC_RULE_MEMO,     m_ctlMemo);
	DDX_Control(pDX, IDC_RULE_USE,      m_ctlUse);
	DDX_Control(pDX, IDC_RULE_ACTION,   m_ctlAction);
	DDX_Control(pDX, IDC_RULE_DIRECTION,m_ctlDirection);
	DDX_Control(pDX, IDC_RULE_PROTOCOL, m_ctlProtocol);
	DDX_Control(pDX, IDC_DST_END_PORT,  m_ctlDstEndPort);
	DDX_Control(pDX, IDC_DST_START_PORT,m_ctlDstStartPort);
	DDX_Control(pDX, IDC_SRC_END_PORT,  m_ctlSrcEndPort);
	DDX_Control(pDX, IDC_SRC_START_PORT,m_ctlSrcStartPort);
	DDX_Control(pDX, IDC_DST_END_IP,    m_ctlDstEndIP);
	DDX_Control(pDX, IDC_DST_START_IP,  m_ctlDstStartIP);
	DDX_Control(pDX, IDC_SRC_END_IP,    m_ctlSrcEndIP);
	DDX_Control(pDX, IDC_SRC_START_IP,  m_ctlSrcStartIP);
	DDX_Text(pDX, IDC_SRC_START_PORT,   m_iSrcStartPort);
	DDV_MinMaxUInt(pDX, m_iSrcStartPort, 0, 65540);
	DDX_Text(pDX, IDC_SRC_END_PORT,     m_iSrcEndPort);
	DDV_MinMaxUInt(pDX, m_iSrcEndPort, 1, 65540);
	DDX_Text(pDX, IDC_DST_START_PORT,   m_iDstStartPort);
	DDV_MinMaxUInt(pDX, m_iDstStartPort, 0, 65540);
	DDX_Text(pDX, IDC_DST_END_PORT,     m_iDstEndPort);
	DDV_MinMaxUInt(pDX, m_iDstEndPort, 1, 65540);
	DDX_Text(pDX, IDC_RULE_MEMO,        m_strMemo);
	DDV_MaxChars(pDX, m_strMemo, 256);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddRuleDlg, CDialog)
	//{{AFX_MSG_MAP(CAddRuleDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddRuleDlg message handlers

BOOL CAddRuleDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();	    

	// Initialization Protocol ComboBox
	m_ctlProtocol.ResetContent();
    //m_ctlProtocol.SetItemData(m_ctlProtocol.AddString(_T("全部")), 0);    
    m_ctlProtocol.SetItemData(m_ctlProtocol.AddString(_T("TCP")),  IPPROTO_TCP);
    m_ctlProtocol.SetItemData(m_ctlProtocol.AddString(_T("UDP")),  IPPROTO_UDP);
    m_ctlProtocol.SetItemData(m_ctlProtocol.AddString(_T("ICMP")), IPPROTO_ICMP);
    m_ctlProtocol.SetItemData(m_ctlProtocol.AddString(_T("IGMP")), IPPROTO_IGMP);
    m_ctlProtocol.SetItemData(m_ctlProtocol.AddString(_T("ARP")),  PROTOCOL_ARP);
    m_ctlProtocol.SetCurSel(0);

    // Initialization Direction ComboBox
    m_ctlDirection.ResetContent();
    m_ctlDirection.SetItemData(m_ctlDirection.AddString(_T("进")),  NETWALL_DIRECTION_IN);
    m_ctlDirection.SetItemData(m_ctlDirection.AddString(_T("出")),  NETWALL_DIRECTION_OUT);
    m_ctlDirection.SetItemData(m_ctlDirection.AddString(_T("双向")),NETWALL_DIRECTION_BOTH);
    m_ctlDirection.SetCurSel(0);
    
    // Initialization Action ComboBox
    m_ctlAction.ResetContent();
    m_ctlAction.SetItemData(m_ctlAction.AddString(_T("通过")), NETWALL_ACTION_PASS);
    m_ctlAction.SetItemData(m_ctlAction.AddString(_T("拒绝")), NETWALL_ACTION_DROP);
    m_ctlAction.SetItemData(m_ctlAction.AddString(_T("日志")), NETWALL_ACTION_LOG);    
    m_ctlAction.SetCurSel(0);
    
    if (MODIFY_RULE == m_nType)
    {
        ASSERT(m_pItem);
        RULE_ITEM * pItem = m_pItem;
        
        // Protocol
        for (UINT i = 0; i < (UINT)m_ctlProtocol.GetCount(); i++)
        {
            if (m_ctlProtocol.GetItemData(i) == pItem->iProto)
            {
                m_ctlProtocol.SetCurSel(i);
                break;
            }
        }
                
        // Direction
        for (i = 0; i < (UINT)m_ctlDirection.GetCount(); i++)
        {
            if (m_ctlDirection.GetItemData(i) == pItem->ucDirection)
            {
                m_ctlDirection.SetCurSel(i);
                break;
            }
        }
                
        // Action
        for (i = 0; i < (UINT)m_ctlAction.GetCount(); i++)
        {
            if (m_ctlAction.GetItemData(i) == pItem->ucAction)
            {
                m_ctlAction.SetCurSel(i);
                break;
            }
        }
                
        // Use        
        if (0 == pItem->bUse)m_ctlUse.SetCheck(1);
                
        // Source Start IP
        m_ctlSrcStartIP.SetAddress(pItem->ulSrcStartIp);
                
        // Source End IP
        m_ctlSrcEndIP.SetAddress(pItem->ulSrcEndIp);
                
        // Source StartPort
        m_iSrcStartPort = pItem->usSrcStartPort;
                
        // Source End Port
        m_iSrcEndPort = pItem->usSrcEndPort;
                
        // Destination Start IP
        m_ctlDstStartIP.SetAddress(pItem->ulDestStartIp);
                
        // Destination End IP
        m_ctlDstEndIP.SetAddress(pItem->ulDestEndIp);
        
        // Destination Start Port
        m_iDstStartPort = pItem->usDestStartPort;
                
        // Destination End Port
        m_iDstEndPort = pItem->usDestEndPort;
                
        // Description
        m_strMemo = _T(pItem->chMsg);
        
        UpdateData(FALSE);
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddRuleDlg::OnOK() 
{
	UpdateData();

    BYTE    b1, b2, b3, b4;
    CString strError = _T("");

    do
    {    
        // Source Start IP
        if (m_ctlSrcStartIP.IsBlank())
        {
            strError = _T("请输入源起始地址!");
            m_ctlSrcStartIP.SetFieldFocus(0);
            break;
        }
                
        int nonBlankCount = m_ctlSrcStartIP.GetAddress(b1, b2, b3, b4);
        if (4 != nonBlankCount)
        {
            strError = _T("输入的源起始地址不正确!");
            m_ctlSrcStartIP.SetFieldFocus(0);
            break;
        }

        // Source End IP
        if (m_ctlSrcEndIP.IsBlank())
        {
            strError = _T("请输入源结束地址!");
            m_ctlSrcEndIP.SetFieldFocus(0);
            break;
        }
        nonBlankCount = m_ctlSrcEndIP.GetAddress(b1, b2, b3, b4);
        if (4 != nonBlankCount)
        {
            strError = _T("输入的源结束地址不正确!");
            m_ctlSrcEndIP.SetFieldFocus(0);
            break;
        }

        // Source Port
        if (m_iSrcStartPort > m_iSrcEndPort)
        {
            strError = _T("输入的源起始端口必须小于或等于源结束端口!");
            m_ctlSrcStartPort.SetFocus();
            break;
        }

        // Destination Start IP
        if (m_ctlDstStartIP.IsBlank())
        {
            strError = _T("请输入目的起始地址!");
            m_ctlDstStartIP.SetFieldFocus(0);
            break;
        }
        nonBlankCount = m_ctlDstStartIP.GetAddress(b1, b2, b3, b4);
        if (4 != nonBlankCount)
        {
            strError = _T("输入的目的起始地址不正确!");
            m_ctlDstStartIP.SetFieldFocus(0);
            break;
        }
        
        // Destination End IP
        if (m_ctlDstEndIP.IsBlank())
        {
            strError = _T("请输入目的结束地址!");
            m_ctlDstEndIP.SetFieldFocus(0);
            break;
        }
        nonBlankCount = m_ctlDstEndIP.GetAddress(b1, b2, b3, b4);
        if (4 != nonBlankCount)
        {
            strError = _T("输入的目的结束地址不正确!");
            m_ctlDstEndIP.SetFieldFocus(0);
            break;
        }
        
        // Destination Port
        if (m_iDstStartPort > m_iDstEndPort)
        {
            strError = _T("输入的目的起始端口必须小于或等于目的结束端口!");            
            m_ctlDstStartPort.SetFocus();
            m_ctlDstStartPort.SetSel(0, -1);
            break;
        }

    } while (FALSE);

    // Error Dealwith
    if (! strError.IsEmpty() && strError.GetLength() > 0)
    {
        AfxMessageBox(strError);
        return;
    }

    // Create A Rule Item
    UINT  nMemoLen = 0;    
    if (! m_strMemo.IsEmpty() && 1 < m_strMemo.GetLength())
    {
        nMemoLen = m_strMemo.GetLength();
    }
    
    try
    {  
        DWORD dwRuleSize = sizeof(RULE_ITEM) + nMemoLen;
        
        m_pBuffer = new BYTE[dwRuleSize];
    
        ASSERT(m_pBuffer);

        RtlZeroMemory(m_pBuffer, dwRuleSize);
        RULE_ITEM * pItem = (PRULE_ITEM)m_pBuffer;
    
        // Base member
        pItem->cbSize      = dwRuleSize;
        pItem->iProto      = m_ctlProtocol.GetItemData(m_ctlProtocol.GetCurSel());
        pItem->ucDirection = (UCHAR)m_ctlDirection.GetItemData(m_ctlDirection.GetCurSel());
        pItem->ucAction    = (UCHAR)m_ctlAction.GetItemData(m_ctlAction.GetCurSel());
        pItem->bUse        = m_ctlUse.GetCheck() == 1 ? 0 : 1;

        // Source member
        m_ctlSrcStartIP.GetAddress(b1, b2, b3, b4);        
        pItem->ulSrcStartIp = b1 << 24 | b2 << 16 | b3 << 8 | b4;
        
        m_ctlSrcEndIP.GetAddress(b1, b2, b3, b4);        
        pItem->ulSrcEndIp = b1 << 24 | b2 << 16 | b3 << 8 | b4;
        if (0 == pItem->ulSrcEndIp)
        {
            pItem->ulSrcEndIp = 0xFFFFFFFF;
        }

        pItem->usSrcStartPort = m_iSrcStartPort;
        pItem->usSrcEndPort   = m_iSrcEndPort;
        if (0 == pItem->usSrcEndPort)
        {
            pItem->usSrcEndPort = 0xFFFF;
        }

        // Destination member
        m_ctlDstStartIP.GetAddress(b1, b2, b3, b4);        
        pItem->ulDestStartIp = b1 << 24 | b2 << 16 | b3 << 8 | b4;
    
        m_ctlDstEndIP.GetAddress(b1, b2, b3, b4);        
        pItem->ulDestEndIp = b1 << 24 | b2 << 16 | b3 << 8 | b4;    
        if (0 == pItem->ulDestEndIp)
        {
            pItem->ulDestEndIp = 0xFFFFFFFF;
        }

        pItem->usDestStartPort = m_iSrcStartPort;
        pItem->usDestEndPort   = m_iSrcEndPort;
        if (0 == pItem->usDestEndPort)
        {
            pItem->usDestEndPort = 0xFFFF;
        }
    
        // Memo member
        if (! m_strMemo.IsEmpty())
        {        
            _tcsncpy(&pItem->chMsg[0], m_strMemo, m_strMemo.GetLength());
        }
    }
    catch (...)
    {
        AfxMessageBox(_T("设置规则出现异常!"));

        if (NULL != m_pBuffer)
        {
            delete []m_pBuffer;
            m_pBuffer = NULL;
        }
    }
    
	CDialog::OnOK();
}
























