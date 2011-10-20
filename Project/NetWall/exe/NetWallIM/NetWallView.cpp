/**************************************************************************/
/*  NetWallView.cpp : implementation of the CNetWallView class            */
/*                                                                        */
/*  这个文件实现一个树视图，在树上面显示适配器的相关信息，规则，日志等。  */
/*  当程序启动的时候，主框架从NetWall中间层驱动读取适配器的相关信息，然后 */
/*  主框架创建这个树视图，视图利用取回的适配器信息刷新树。                */ 
/*                                                                        */        
/*	当用户点击该适配器项本身时，将发消息给主框架刷新右视图，列表显示适配  */
/*  的每一项信息。                                                        */
/*                                                                        */                                                             
/*	当用户点击该适配器的规则项时，将发消息给主框架将适配器的规则信息显示  */
/*  在树上，并且刷新右视图，列表显示每一项规则信息。                      */
/*                                                                        */
/*	当用户点击该适配器的日志项时，将发消息给主框架刷新右视图，列表显示该  */
/*  适配器的当天日志信息。                                                */
/*    */
/*	  */
/*    */
/*    */
/*	  */
/*    */
/**************************************************************************/
#include "stdafx.h"
#include "NetWall.h"

#include "NetWallDoc.h"
#include "NetWallView.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Image indexes
#define ILI_HARD_DISK       0
#define ILI_FLOPPY          1
#define ILI_CD_ROM          2
#define ILI_NET_DRIVE       3
#define ILI_CLOSED_FOLDER   4
#define ILI_OPEN_FOLDER     5

const LPCTSTR  ADAPTER_NAME = _T("适配器");
const LPCTSTR  RULE_NAME    = _T("规则");
const LPCTSTR  LOG_NAME     = _T("日志");

/////////////////////////////////////////////////////////////////////////////
// CNetWallView

IMPLEMENT_DYNCREATE(CNetWallView, CTreeView)

BEGIN_MESSAGE_MAP(CNetWallView, CTreeView)
	//{{AFX_MSG_MAP(CNetWallView)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemexpanding)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_WM_CREATE()
	ON_NOTIFY_REFLECT(TVN_DELETEITEM, OnDeleteitem)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CTreeView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNetWallView construction/destruction

CNetWallView::CNetWallView()
{
    m_pCurrentAdapter = NULL;
	m_pAdapterList    = NULL;
}

CNetWallView::~CNetWallView()
{
    m_pCurrentAdapter = NULL;
    m_pAdapterList    = NULL;
}

BOOL CNetWallView::PreCreateWindow(CREATESTRUCT& cs)
{     	
    if (! CTreeView::PreCreateWindow(cs))
    {
		return FALSE;
    }

    //  Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
    cs.style |= TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS;
	
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CNetWallView drawing

void CNetWallView::OnDraw(CDC* pDC)
{
	CNetWallDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);	
}

/////////////////////////////////////////////////////////////////////////////
// CNetWallView diagnostics

#ifdef _DEBUG
void CNetWallView::AssertValid() const
{
    CTreeView::AssertValid();
}

void CNetWallView::Dump(CDumpContext& dc) const
{
    CTreeView::Dump(dc);
}

CNetWallDoc* CNetWallView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CNetWallDoc)));
    return (CNetWallDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNetWallView message handlers

int CNetWallView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CTreeView::OnCreate(lpCreateStruct) == -1)
    {
        return -1;
    }
    
    //
    // Initialize the image list.
    //
    m_ilAdapters.Create(IDB_NETWALL_IMAGES, 16, 1, RGB(255, 0, 255));
    m_ilAdapters.SetBkColor(GetSysColor(COLOR_WINDOW));
    GetTreeCtrl().SetImageList(&m_ilAdapters, TVSIL_NORMAL);
        
    return 0;
}

void CNetWallView::OnInitialUpdate() 
{
    CTreeView::OnInitialUpdate();
    
    Refresh(ADAPTER);
}

void CNetWallView::Refresh(NETWALL_LIST_TYPE eType, CAdapterInfo *pAdapterInfo)
{
    GetTreeCtrl().DeleteAllItems();

    //
    // Populate the tree view with adapters items.
    //
    m_pAdapterList = ((CMainFrame *)AfxGetMainWnd())->GetAdapters();
    if (NULL != m_pAdapterList)
    {
        AddAdapters();
    }    
    
    if (ADAPTER == eType)
    {
        //
        // Show the items on the first adapter.
        //	
        HTREEITEM hItem = GetTreeCtrl().GetNextItem(NULL, TVGN_ROOT);	
        if (hItem != NULL) 
        {
            GetTreeCtrl().Expand(hItem, TVE_EXPAND);
            GetTreeCtrl().Select(hItem, TVGN_CARET);
        }
    
        //
        // Initialize the list view.
        //
        m_pCurrentAdapter = (CAdapterInfo *)GetTreeCtrl().GetItemData(GetTreeCtrl().GetSelectedItem());
        ((CMainFrame *)AfxGetMainWnd())->SetCurrentAdapter(m_pCurrentAdapter);
        GetDocument()->UpdateAllViews(this, ADAPTER, (CObject *)m_pCurrentAdapter);
    }
    else if (RULE == eType)
    {
        //
        // Show the rule items on the first adapter.
        //	
        HTREEITEM hItem = GetTreeCtrl().GetNextItem(NULL, TVGN_ROOT);	
        while (hItem != NULL) 
        {
            CAdapterInfo * pAdapter = (CAdapterInfo *)GetTreeCtrl().GetItemData(hItem);
            if (pAdapter == pAdapterInfo)
            {
                GetTreeCtrl().Expand(hItem, TVE_EXPAND);
                GetTreeCtrl().Select(hItem, TVGN_CARET);
                                
                HTREEITEM  hChildItem = GetTreeCtrl().GetChildItem(hItem);
                
                while (hChildItem != NULL)
                {
                    CString strItem = _T("");
                    strItem = GetTreeCtrl().GetItemText(hChildItem);
                    if (! strItem.IsEmpty() && ! _tcscmp(strItem, RULE_NAME))
                    {
                        GetTreeCtrl().Expand(hChildItem, TVE_EXPAND);
                        GetTreeCtrl().Select(hChildItem, TVGN_CARET);
                        break;
                    } 
                    
                    hChildItem = GetTreeCtrl().GetNextSiblingItem(hChildItem);
                }
                
                break;
            }
            
            hItem = GetTreeCtrl().GetNextSiblingItem(hItem);                       
        }
    }
    else if (LOG == eType)
    {
        //
        // Show the log items on the adapter.
        //	
        HTREEITEM hItem = GetTreeCtrl().GetNextItem(NULL, TVGN_ROOT);	
        while (hItem != NULL) 
        {
            CAdapterInfo * pAdapter = (CAdapterInfo *)GetTreeCtrl().GetItemData(hItem);
            if (pAdapter == pAdapterInfo)
            {
                GetTreeCtrl().Expand(hItem, TVE_EXPAND);
                GetTreeCtrl().Select(hItem, TVGN_CARET);
                
                HTREEITEM  hChildItem = GetTreeCtrl().GetChildItem(hItem);
                
                while (hChildItem != NULL)
                {
                    CString strItem = _T("");
                    strItem = GetTreeCtrl().GetItemText(hChildItem);
                    if (! strItem.IsEmpty() && ! _tcscmp(strItem, LOG_NAME))
                    {
                        GetTreeCtrl().Expand(hChildItem, TVE_EXPAND);
                        GetTreeCtrl().Select(hChildItem, TVGN_CARET);
                        break;
                    } 
                    
                    hChildItem = GetTreeCtrl().GetNextSiblingItem(hChildItem);
                }
                
                break;
            }
            
            hItem = GetTreeCtrl().GetNextSiblingItem(hItem);                       
        }        
    }
}

int CNetWallView::AddAdapters()
{
    int nAdaptersAdded = 0;
    CString strItem = _T("");
    
    for (CAdapterListIterator it = m_pAdapterList->begin();
         it != m_pAdapterList->end();
         it++)
    {
        HTREEITEM hItem;
        strItem.Format(_T("适配器 %d"), ++nAdaptersAdded);
        
        hItem = GetTreeCtrl().InsertItem(strItem, ILI_NET_DRIVE, ILI_NET_DRIVE);
        
        GetTreeCtrl().SetItemData(hItem, (DWORD)(*it));
        
        GetTreeCtrl().InsertItem(_T(""), ILI_CLOSED_FOLDER, ILI_CLOSED_FOLDER, hItem);
    }
    
    return nAdaptersAdded;
}

BOOL CNetWallView::SetButtonState(HTREEITEM hItem, LPCTSTR lpszItem)
{
    if (! _tcsnicmp(lpszItem, RULE_NAME, _tcslen(RULE_NAME)))
    {
        HTREEITEM hParentItem = GetTreeCtrl().GetParentItem(hItem);
        if (NULL != hParentItem)
        {
            CAdapterInfo * pAdaterInfo = (CAdapterInfo *)GetTreeCtrl().GetItemData(hParentItem);
            ASSERT(pAdaterInfo);

            if (((CMainFrame *)AfxGetMainWnd())->HasRuleForAdapter(pAdaterInfo))
            {                
                HTREEITEM  hNewItem = GetTreeCtrl().InsertItem(_T(""), ILI_CLOSED_FOLDER, ILI_CLOSED_FOLDER, hItem);
                GetTreeCtrl().SetItemData(hNewItem, (DWORD)(0));
            }
        }                
    }

    // Don't support logged now.
    else if (! _tcsnicmp(lpszItem, LOG_NAME, _tcslen(LOG_NAME)))
    {
        // HTREEITEM  hNewItem = GetTreeCtrl().InsertItem(_T (""), ILI_CLOSED_FOLDER, ILI_CLOSED_FOLDER, hItem);
        // GetTreeCtrl().SetItemData(hNewItem, (DWORD)(0));        
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

void CNetWallView::OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    HTREEITEM hItem = pNMTreeView->itemNew.hItem;
    CString strItem = GetTreeCtrl().GetItemText(hItem);
    
    *pResult = FALSE;
    
    if (pNMTreeView->action == TVE_EXPAND) 
    {
        DeleteFirstChild(hItem);
        if (AddAdapterItems(hItem, strItem) == 0)
        {
            *pResult = TRUE;
        }
    }
    else // pNMTreeView->action == TVE_COLLAPSE
    { 
        DeleteAllChildren(hItem);
        if (GetTreeCtrl().GetParentItem(hItem) == NULL)
        {
            HTREEITEM  hNewItem = GetTreeCtrl().InsertItem(_T(""), ILI_CLOSED_FOLDER, ILI_CLOSED_FOLDER, hItem);
            GetTreeCtrl().SetItemData(hNewItem, (DWORD)(0));
        }
        else
        {
            SetButtonState(hItem, strItem);
        }
    }	
}

void CNetWallView::DeleteFirstChild(HTREEITEM hItem)
{
    HTREEITEM   hChildItem;

    if ((hChildItem = GetTreeCtrl().GetChildItem(hItem)) != NULL)
    {
        GetTreeCtrl().DeleteItem(hChildItem);
    }
}

void CNetWallView::DeleteAllChildren(HTREEITEM hItem)
{
    HTREEITEM   hChildItem;

    if ((hChildItem = GetTreeCtrl().GetChildItem(hItem)) == NULL)
    {
        return;
    }
    
    do 
    {
        HTREEITEM hNextItem = GetTreeCtrl ().GetNextSiblingItem(hChildItem);
        GetTreeCtrl().DeleteItem(hChildItem);
        hChildItem = hNextItem;
        
    } while (hChildItem != NULL);
}

int CNetWallView::AddAdapterItems(HTREEITEM hItem, CString strItem)
{
    HTREEITEM   hNewItem;
    int         nCount = 0;
    
    try
    {    
        // Add Adapter's sub item
        if (! _tcsnicmp(strItem, ADAPTER_NAME, _tcslen(ADAPTER_NAME)))
        {        
            hNewItem = GetTreeCtrl().InsertItem(RULE_NAME, ILI_CLOSED_FOLDER, ILI_OPEN_FOLDER, hItem);
            GetTreeCtrl().SetItemData(hNewItem, (DWORD)(0));
            SetButtonState(hNewItem, RULE_NAME);
            nCount++;
        
            hNewItem = GetTreeCtrl().InsertItem(LOG_NAME, ILI_CLOSED_FOLDER, ILI_OPEN_FOLDER, hItem);
            GetTreeCtrl().SetItemData(hNewItem, (DWORD)(0));
            SetButtonState(hNewItem, LOG_NAME);
            nCount++;
        }
    
        else if (! _tcsnicmp(strItem, RULE_NAME, _tcslen(RULE_NAME))) 
        {
            CString strItem = _T("");
        
            if (0 < m_pCurrentAdapter->m_dwNumberOfRules)
            {
                RULE_ITEM * pItem = ((CMainFrame *)AfxGetMainWnd())->GetAdapterRuleItem();

                for (nCount = 0; nCount < m_pCurrentAdapter->m_dwNumberOfRules; nCount++)        
                {
                    strItem.Format(_T("规则 %d"), nCount + 1);
                    
                    hNewItem = GetTreeCtrl().InsertItem(strItem, ILI_NET_DRIVE, ILI_NET_DRIVE, hItem);  
                                        
                    RULE_ITEM * pNode = (PRULE_ITEM)new BYTE[pItem->cbSize];
                    ASSERT(pItem && pNode);                          
                    
                    RtlCopyMemory(pNode, pItem, pItem->cbSize);
                    
                    GetTreeCtrl().SetItemData(hNewItem, (DWORD)(pNode));  
                    
                    pItem = (RULE_ITEM *)((LPBYTE)pItem + pItem->cbSize);
                }    
            }                
        }        

        else if (! _tcsnicmp(strItem, LOG_NAME, _tcslen(LOG_NAME))) 
        {
            // Don't support Logged now.
        }
    }
    catch (...)
    {
        AfxMessageBox(_T("刷新树视图失败!"));
        throw;
    }

    return nCount;
}

/************************************************************************/
/* 当用户选择树上某项时，发消息给MainFrame更新右视图，并刷新右视图。    */
/************************************************************************/
void CNetWallView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;	

    ((CMainFrame *)AfxGetMainWnd())->m_nIndex = -1;
    
    HTREEITEM  hItem = pNMTreeView->itemNew.hItem;    
    if (NULL == hItem)
    {
        return;
    }

    // Get Adapter 
    HTREEITEM hParent, hTempItem;
    hTempItem = hItem;
    while ((hParent = GetTreeCtrl().GetParentItem(hTempItem)) != NULL) 
    {        
        hTempItem = hParent;
    }

    m_pCurrentAdapter = (CAdapterInfo *)GetTreeCtrl().GetItemData(hParent == NULL ? hTempItem : hParent);
    
    ((CMainFrame *)AfxGetMainWnd())->SetCurrentAdapter(m_pCurrentAdapter);
    
    // Set Data
    CString strItem = GetTreeCtrl().GetItemText(pNMTreeView->itemNew.hItem);

    // 在 RuleListView 中显示适配器的信息
    if (! _tcsnicmp(strItem, ADAPTER_NAME, _tcslen(ADAPTER_NAME)))
    {
        AfxGetMainWnd()->SendMessage(WM_USER_DISPLAY_RULEITEM, ADAPTER, 0);
        GetDocument()->UpdateAllViews(this, ADAPTER, (CObject *)m_pCurrentAdapter);
        ((CMainFrame *)AfxGetMainWnd())->UpdateStatusBar(ADAPTER);
    }
    else if (! _tcsnicmp(strItem, RULE_NAME, _tcslen(RULE_NAME))) 
    {
        // 在 RuleListView 中显示当前适配器的所有规则信息
        if (strItem == RULE_NAME)
        {
            AfxGetMainWnd()->SendMessage(WM_USER_DISPLAY_RULEITEM, RULE, 0);
            GetDocument()->UpdateAllViews(this, RULE, (CObject *)m_pCurrentAdapter);
            ((CMainFrame *)AfxGetMainWnd())->UpdateStatusBar(RULE);
        }
        
        // 在 DisplayRuleView 中显示当前适配器的一条规则的详细信息
        else
        {
            RULE_ITEM  *pNode = (PRULE_ITEM)GetTreeCtrl().GetItemData(hItem);
            ASSERT(pNode);
            if (NULL != pNode)
            {           
                ((CMainFrame *)AfxGetMainWnd())->m_nIndex = atoi(strItem.Mid(strItem.Find(_T(' ')) + 1)) - 1;
                
                AfxGetMainWnd()->SendMessage(WM_USER_DISPLAY_RULEITEM, RULEITEM, (LPARAM)pNode);
                GetDocument()->UpdateAllViews(this, RULEITEM, (CObject *)pNode);
                ((CMainFrame *)AfxGetMainWnd())->UpdateStatusBar(RULEITEM);
            }
        }
    }

    // 在 RuleListView 中显示当前适配器当天的日志信息
    else if (! _tcsnicmp(strItem, LOG_NAME, _tcslen(LOG_NAME))) 
    {
        AfxGetMainWnd()->SendMessage(WM_USER_DISPLAY_RULEITEM, LOG, 0);
        GetDocument()->UpdateAllViews(this, LOG, (CObject *)m_pCurrentAdapter);
    }
    
    *pResult = 0;
}

void CNetWallView::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	HTREEITEM hItem = (pNMTreeView->itemOld).hItem;

    CString strItem = GetTreeCtrl().GetItemText(hItem);
    if (! strItem.IsEmpty() 
        && strItem.GetLength() > _tcslen(RULE_NAME) 
        && ! _tcsnicmp(strItem, RULE_NAME, _tcslen(RULE_NAME)))
    {    
        RULE_ITEM  *pNode = (PRULE_ITEM)GetTreeCtrl().GetItemData(hItem);
        if (NULL != pNode)
        {
            delete	[](LPBYTE)pNode;
            pNode = NULL;
        }
    }
    
    *pResult = 0;
}


void CNetWallView::OnContextMenu(CWnd* pWnd, CPoint point) 
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
    VERIFY(menu.LoadMenu(IDR_POPUP_ADAPTER_VIEW));
    
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

void CNetWallView::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{	   
    CPoint curPoint;
    GetCursorPos(&curPoint);

    ScreenToClient(&curPoint);
    
    // Select the item that is at the point myPoint.
    UINT uFlags;
    HTREEITEM hItem = GetTreeCtrl().HitTest(curPoint, &uFlags);
    
    if ((hItem != NULL) && (TVHT_ONITEM & uFlags))
    {
        GetTreeCtrl().Select(hItem, TVGN_CARET);
        SendMessage(WM_CONTEXTMENU, 0, MAKELPARAM(curPoint.x, curPoint.y));
    }
    
	*pResult = 0;
}
