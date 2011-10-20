/**************************************************************************/
/*  RuleListView.cpp : implementation of the CRuleListView class          */
/*                                                                        */
/*  ����ļ�ʵ��һ���б���ͼ��ͨ������ͼ��������Ϣ����ͼ�зֱ���ʾ������  */
/*  ��������־�ȵ������Ϣ��                                            */
/*                                                                        */        
/*	����ʾ�����б�ʱ���û�����ʹ�ÿ�ݲ˵��������еĹ��������            */
/*    */
/*    */
/*	  */
/*    */
/**************************************************************************/
#include "stdafx.h"
#include "netwall.h"
#include "RuleListView.h"
#include "MainFrm.h"
#include "RuleUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRuleListView

IMPLEMENT_DYNCREATE(CRuleListView, CListView)

CRuleListView::CRuleListView()
{
    m_nType  = ADAPTER;        
}

CRuleListView::~CRuleListView()
{
}

BEGIN_MESSAGE_MAP(CRuleListView, CListView)
	//{{AFX_MSG_MAP(CRuleListView)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemchanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CRuleListView::PreCreateWindow(CREATESTRUCT& cs) 
{
    // TODO: Add your specialized code here and/or call the base class
    cs.style = (cs.style & ~LVS_TYPEMASK) | LVS_REPORT | LVS_SHOWSELALWAYS;
    cs.style |= LVS_AUTOARRANGE;
    // Set full line select
    //ListView_SetExtendedListViewStyle(GetSafeHwnd(), LVS_EX_FULLROWSELECT);
    
    return CListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CRuleListView drawing

void CRuleListView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CRuleListView diagnostics

#ifdef _DEBUG
void CRuleListView::AssertValid() const
{
	CListView::AssertValid();
}

void CRuleListView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRuleListView message handlers

void CRuleListView::OnInitialUpdate() 
{
	CListView::OnInitialUpdate();
	
    // this code only works for a report-mode list view
    VERIFY(GetStyle() & LVS_REPORT);

    // Set full line select
    //ListView_SetExtendedListViewStyle(GetSafeHwnd(), LVS_EX_FULLROWSELECT);
}

void CRuleListView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    // CG: This block was added by the Pop-up Menu component
    if (RULE == m_nType)
    {
        if (point.x == -1 && point.y == -1)
        {
            //keystroke invocation
            CRect rect;
            GetClientRect(rect);
            ClientToScreen(rect);
            
            point = rect.TopLeft();
            point.Offset(5, 5);
        }
        
        CMenu menu;
        VERIFY(menu.LoadMenu(IDR_POPUP_RULE_LIST));
        
        CMenu* pPopup = menu.GetSubMenu(0);
        ASSERT(pPopup != NULL);
        CWnd* pWndPopupOwner = this;
        
        while (pWndPopupOwner->GetStyle() & WS_CHILD)
        {
            pWndPopupOwner = pWndPopupOwner->GetParent();
        }
        
        pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
            pWndPopupOwner);
    }
}

void CRuleListView::OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    
    if (ADAPTER == m_nType)
    {
        return;
    }
    
    int pos = GetListCtrl().GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
    if (-1 == pos)      //δѡ��
    {
        ((CMainFrame *)AfxGetMainWnd())->m_nIndex = -1;                                                        
    }
    else                //��ȡѡ���������
    {
        ((CMainFrame *)AfxGetMainWnd())->m_nIndex = GetListCtrl().GetItemData(pos);                  
    }
    
    *pResult = 0;
}

void CRuleListView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
    m_nType = (UINT)lHint;
    
    switch (m_nType) 
    {
    case ADAPTER:
        InitHeadForAdapter();
        RefreshAdapter((CAdapterInfo *)pHint);
        break;
        
    case RULE:
        InitHeadForRule();
        RefreshRule((CAdapterInfo *)pHint);
        break;            
        
    default:
        break;
    }
    
    ((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_SIZE);
}

BOOL CRuleListView::InitHeadForAdapter()
{
    // Gain a reference to the list control itself
    CListCtrl& listView = GetListCtrl();
    listView.SetBkColor(RGB(255,255,255));  // set bk color to white

    // Set full line select
    ListView_SetExtendedListViewStyle(GetSafeHwnd(), LVS_EX_FULLROWSELECT);
    
    // Delete all of the items
    listView.DeleteAllItems();
    
    // Delete all of the columns    
    int nColumnCount = listView.GetHeaderCtrl()->GetItemCount();
    
    for (int i = 0; i < nColumnCount; i++)
    {
        listView.DeleteColumn(0);
    }
    
    // Insert Columns
    listView.InsertColumn(0, _T("���"),   LVCFMT_LEFT, 50, 0);
    listView.InsertColumn(1, _T("��Ŀ"),   LVCFMT_LEFT, 150, 0);
    listView.InsertColumn(2, _T("ֵ"),     LVCFMT_LEFT, 450, 0);
    
    return TRUE;
}

int CRuleListView::RefreshAdapter(CAdapterInfo *pAdapterInfo)
{
    CListCtrl& listView = GetListCtrl();
    
    int nIndex = 0;
    CHAR szIndex[10];
    RtlZeroMemory(szIndex, sizeof(szIndex));
    
    // VirtualAdapterName
    _stprintf(szIndex, _T("%d"), nIndex + 1);
    
    listView.InsertItem(nIndex, szIndex);
    
    listView.SetItemText(nIndex, 1, _T("VirtualAdapterName"));
    listView.SetItemText(nIndex, 2, pAdapterInfo->m_strVirtualAdapterName);
    
    // LowerAdapterName
    _stprintf(szIndex, _T("%d"), ++nIndex + 1);
    
    listView.InsertItem(nIndex, szIndex);
    
    listView.SetItemText(nIndex, 1, _T("LowerAdapterName"));
    listView.SetItemText(nIndex, 2, pAdapterInfo->m_strLowerAdapterName);
    
    // FriendlyAdapterName
    _stprintf(szIndex, _T("%d"), ++nIndex + 1);
    
    listView.InsertItem(nIndex, szIndex);
    
    listView.SetItemText(nIndex, 1, _T("FriendlyAdapterName"));
    listView.SetItemText(nIndex, 2, pAdapterInfo->m_strFriendlyAdapterName);

    // FriendlyAdapterName
    _stprintf(szIndex, _T("%d"), ++nIndex + 1);
    
    listView.InsertItem(nIndex, szIndex);
    
    listView.SetItemText(nIndex, 1, _T("PhysicalAddress"));
    listView.SetItemText(nIndex, 2, pAdapterInfo->m_strAdapterMacAddress);
    
    return nIndex;
}

BOOL CRuleListView::InitHeadForRule()
{
    // Gain a reference to the list control itself
    CListCtrl& listView = GetListCtrl();
    listView.SetBkColor(RGB(255,255,255));  // set bk color to white
    
    // Set full line select
    ListView_SetExtendedListViewStyle(GetSafeHwnd(), LVS_EX_FULLROWSELECT);
    
    // Delete all of the items
    listView.DeleteAllItems();
    
    // Delete all of the columns    
    int nColumnCount = listView.GetHeaderCtrl()->GetItemCount();
    
    for (int i = 0; i < nColumnCount; i++)
    {
        listView.DeleteColumn(0);
    }
    
    // Insert Columns
    listView.InsertColumn(0, _T("���"),        LVCFMT_LEFT, 40, 0);    
    listView.InsertColumn(1, _T("Э��"),        LVCFMT_LEFT, 40, 0);
    listView.InsertColumn(2, _T("����"),        LVCFMT_LEFT, 40, 0);
    listView.InsertColumn(3, _T("����"),        LVCFMT_LEFT, 40, 0);
    listView.InsertColumn(4, _T("ʹ��"),        LVCFMT_LEFT, 40, 0);
    
    listView.InsertColumn(5, _T("Դ��ֹIP"),     LVCFMT_LEFT, 180, 0);
    listView.InsertColumn(6, _T("Դ��ֹPort"),   LVCFMT_LEFT, 80, 0);
    
    listView.InsertColumn(7, _T("Ŀ����ֹIP"),   LVCFMT_LEFT, 180, 0);
    listView.InsertColumn(8, _T("Ŀ����ֹPort"), LVCFMT_LEFT, 80, 0);
    
    listView.InsertColumn(9, _T("����"),        LVCFMT_LEFT, 256, 0);
    
    return TRUE;
}

int CRuleListView::RefreshRule(CAdapterInfo *pAdapterInfo)
{
    CListCtrl& listView = GetListCtrl();
    
    int nIndex = 0;
    CHAR szIndex[10];
    RtlZeroMemory(szIndex, sizeof(szIndex));
    
    RULE_ITEM * pItem = ((CMainFrame *)AfxGetMainWnd())->GetAdapterRuleItem();
    ASSERT(pItem);

    for (nIndex = 0; nIndex < pAdapterInfo->m_dwNumberOfRules; nIndex++)
    {
        // Order
        _stprintf(szIndex, _T("%d"), nIndex + 1);
        
        listView.InsertItem(nIndex, szIndex);
        listView.SetItemData(nIndex, DWORD(nIndex));

        // Protocol
        char * pTemp = CRuleUtil::GetProtocolDescById(pItem->iProto);
        listView.SetItemText(nIndex, 1, _T(pTemp));

        // Direction
        pTemp = CRuleUtil::GetDescByDirectionId(pItem->ucDirection);
        listView.SetItemText(nIndex, 2, _T(pTemp));

        // Action
        pTemp = CRuleUtil::GetDescByActionId(pItem->ucAction);
        listView.SetItemText(nIndex, 3, _T(pTemp));

        // Use        
        pTemp = pItem->bUse == 1 ? "ʹ��" : "����";
        listView.SetItemText(nIndex, 4, _T(pTemp));

        // Source Start - End IP
        DWORD dwStartIP = pItem->ulSrcStartIp;
        DWORD dwEndIP   = pItem->ulSrcStartIp;
        TCHAR szBuf[40];
        RtlZeroMemory(szBuf, 40);
        _stprintf(szBuf, "%d.%d.%d.%d : %d.%d.%d.%d", 
            (dwStartIP >> 24) & 0xFF, (dwStartIP >> 16) & 0xFF, (dwStartIP >> 8) & 0xFF, dwStartIP & 0xFF,
            (dwEndIP   >> 24) & 0xFF, (dwEndIP   >> 16) & 0xFF, (dwEndIP   >> 8) & 0xFF, dwEndIP   & 0xFF);
        listView.SetItemText(nIndex, 5, _T(szBuf));

        // Source Start - End Port
        RtlZeroMemory(szBuf, 40);
        _stprintf(szBuf, "%d : %d", pItem->usSrcStartPort, pItem->usSrcEndPort);            
        listView.SetItemText(nIndex, 6, _T(szBuf));

        // Destination Start - End IP
        dwStartIP = pItem->ulDestStartIp;
        dwEndIP   = pItem->ulDestEndIp;
        RtlZeroMemory(szBuf, 40);
        _stprintf(szBuf, "%d.%d.%d.%d : %d.%d.%d.%d", 
            (dwStartIP >> 24) & 0xFF, (dwStartIP >> 16) & 0xFF, (dwStartIP >> 8) & 0xFF, dwStartIP & 0xFF,
            (dwEndIP   >> 24) & 0xFF, (dwEndIP   >> 16) & 0xFF, (dwEndIP   >> 8) & 0xFF, dwEndIP   & 0xFF);
        listView.SetItemText(nIndex, 7, _T(szBuf));

        // Source Start - End Port
        RtlZeroMemory(szBuf, 40);
        _stprintf(szBuf, "%d : %d", pItem->usDestStartPort, pItem->usDestEndPort);            
        listView.SetItemText(nIndex, 8, _T(szBuf));

        // Description
        listView.SetItemText(nIndex, 9, _T(pItem->chMsg));
        
        pItem = (RULE_ITEM *)((LPBYTE)pItem + pItem->cbSize);
    }

    return nIndex;
}



