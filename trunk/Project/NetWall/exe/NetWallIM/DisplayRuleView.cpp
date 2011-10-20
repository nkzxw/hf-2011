// DisplayRuleView.cpp : implementation file
//

#include "stdafx.h"
#include "netwall.h"
#include "DisplayRuleView.h"
#include "MainFrm.h"
#include "RuleUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDisplayRuleView

IMPLEMENT_DYNCREATE(CDisplayRuleView, CFormView)

CDisplayRuleView::CDisplayRuleView()
	: CFormView(CDisplayRuleView::IDD)
{
	//{{AFX_DATA_INIT(CDisplayRuleView)
	m_strAction         = _T("");
	m_strDirection      = _T("");
	m_strProtocol       = _T("");
	m_strUse            = _T("");
	m_strSrcStartIP     = _T("");
	m_strSrcEndIP       = _T("");
	m_strSrcStartPort   = _T("");
	m_strSrcEndPort     = _T("");
	m_strDstStartIP     = _T("");
	m_strDstEndIP       = _T("");
	m_strDstStartPort   = _T("");
	m_strDstEndPort     = _T("");
	m_strMemo           = _T("");
	m_strAdapterName    = _T("");
	//}}AFX_DATA_INIT
}

CDisplayRuleView::~CDisplayRuleView()
{
}

void CDisplayRuleView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDisplayRuleView)
	DDX_Text(pDX, IDC_VIEW_RULE_ACTION,         m_strAction);
	DDX_Text(pDX, IDC_VIEW_RULE_DIRECTION,      m_strDirection);
	DDX_Text(pDX, IDC_VIEW_RULE_PROTOCOL,       m_strProtocol);
	DDX_Text(pDX, IDC_VIEW_RULE_USE,            m_strUse);
	DDX_Text(pDX, IDC_VIEW_RULE_SRCSTARTIP,     m_strSrcStartIP);
	DDX_Text(pDX, IDC_VIEW_RULE_SRCENDIP,       m_strSrcEndIP);
	DDX_Text(pDX, IDC_VIEW_RULE_SRCSTARTPORT,   m_strSrcStartPort);
	DDX_Text(pDX, IDC_VIEW_RULE_SRCENDPORT,     m_strSrcEndPort);
	DDX_Text(pDX, IDC_VIEW_RULE_DSTSTARTIP,     m_strDstStartIP);
	DDX_Text(pDX, IDC_VIEW_RULE_DSTENDIP,       m_strDstEndIP);
	DDX_Text(pDX, IDC_VIEW_RULE_DSTSTARTPORT,   m_strDstStartPort);
	DDX_Text(pDX, IDC_VIEW_RULE_DSTENDPORT,     m_strDstEndPort);
	DDX_Text(pDX, IDC_VIEW_RULE_MEMO,           m_strMemo);
	DDX_Text(pDX, IDC_ADAPTER, m_strAdapterName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDisplayRuleView, CFormView)
	//{{AFX_MSG_MAP(CDisplayRuleView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDisplayRuleView diagnostics

#ifdef _DEBUG
void CDisplayRuleView::AssertValid() const
{
	CFormView::AssertValid();
}

void CDisplayRuleView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDisplayRuleView message handlers

void CDisplayRuleView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	// TODO: Add your specialized code here and/or call the base class
    UINT nType = (UINT)lHint;
    
    switch (nType) 
    {    
    case RULEITEM:
        RefreshRuleItem((PRULE_ITEM)pHint);
        break;
        
    default:
        break;
    }
    
    ((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_SIZE);	
}

BOOL CDisplayRuleView::RefreshRuleItem(RULE_ITEM *pItem)
{
    ASSERT(pItem);

    // Protocol
    char * pTemp = CRuleUtil::GetProtocolDescById(pItem->iProto);
    m_strProtocol = _T(pTemp);
    
    // Direction
    pTemp = CRuleUtil::GetDescByDirectionId(pItem->ucDirection);
    m_strDirection = _T(pTemp);
    
    // Action
    pTemp = CRuleUtil::GetDescByActionId(pItem->ucAction);
    m_strAction = _T(pTemp);
    
    // Use        
    pTemp = pItem->bUse == 1 ? "Ê¹ÓÃ" : "½ûÓÃ";
    m_strUse = _T(pTemp);
    
    // Source Start IP
    DWORD dwStartIP = pItem->ulSrcStartIp;
    TCHAR szBuf[40];
    RtlZeroMemory(szBuf, 40);
    _stprintf(szBuf, "%d.%d.%d.%d", 
        (dwStartIP >> 24) & 0xFF, (dwStartIP >> 16) & 0xFF, (dwStartIP >> 8) & 0xFF, dwStartIP & 0xFF);
    m_strSrcStartIP = _T(szBuf);

    // Source End IP
    DWORD dwEndIP   = pItem->ulSrcEndIp;
    RtlZeroMemory(szBuf, 40);
    _stprintf(szBuf, "%d.%d.%d.%d", 
        (dwEndIP >> 24) & 0xFF, (dwEndIP >> 16) & 0xFF, (dwEndIP >> 8) & 0xFF, dwEndIP   & 0xFF);
    m_strSrcEndIP = _T(szBuf);
    
    // Source StartPort
    RtlZeroMemory(szBuf, 40);
    _stprintf(szBuf, "%d", pItem->usSrcStartPort);            
    m_strSrcStartPort = _T(szBuf);

    // Source End Port
    RtlZeroMemory(szBuf, 40);
    _stprintf(szBuf, "%d", pItem->usSrcEndPort);            
    m_strSrcEndPort = _T(szBuf);
    
    // Destination Start IP
    dwStartIP = pItem->ulDestStartIp;
    RtlZeroMemory(szBuf, 40);
    _stprintf(szBuf, "%d.%d.%d.%d", 
        (dwStartIP >> 24) & 0xFF, (dwStartIP >> 16) & 0xFF, (dwStartIP >> 8) & 0xFF, dwStartIP & 0xFF);
    m_strDstStartIP = _T(szBuf);

    // Destination End IP
    dwEndIP   = pItem->ulDestEndIp;
    RtlZeroMemory(szBuf, 40);
    _stprintf(szBuf, "%d.%d.%d.%d", 
        (dwEndIP >> 24) & 0xFF, (dwEndIP >> 16) & 0xFF, (dwEndIP >> 8) & 0xFF, dwEndIP   & 0xFF);
    m_strDstEndIP = _T(szBuf);
    
    // Destination Start Port
    RtlZeroMemory(szBuf, 40);
    _stprintf(szBuf, "%d", pItem->usDestStartPort);            
    m_strDstStartPort = _T(szBuf);

    // Destination End Port
    RtlZeroMemory(szBuf, 40);
    _stprintf(szBuf, "%d", pItem->usDestEndPort);            
    m_strDstEndPort = _T(szBuf);
    
    // Description
    m_strMemo = _T(pItem->chMsg);
    
    // Adapter Name
    m_strAdapterName = ((CMainFrame *)AfxGetMainWnd())->m_strFriendlyAdapterName;

    UpdateData(FALSE);

    return TRUE;
}
