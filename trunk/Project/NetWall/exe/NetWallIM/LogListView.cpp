// LogListView.cpp : implementation file
//

#include "stdafx.h"
#include "netwall.h"
#include "LogListView.h"
#include "MainFrm.h"
#include "RuleUtil.h"
#include "log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLogListView

ThreadParams *CLogListView::m_pThreadParams = NULL;

IMPLEMENT_DYNCREATE(CLogListView, CListView)

CLogListView::CLogListView()
{
    m_nType          = LOG;    
    m_bLogLoaded     = FALSE;
    m_bThreadStarted = FALSE;
    m_dwCurItem      = 0;
    m_dwLogCount     = 0;
}

CLogListView::~CLogListView()
{
    if (NULL != m_pThreadParams)
    {
        delete m_pThreadParams;
        m_pThreadParams = NULL;
    }

    m_bThreadStarted = FALSE;
}


BEGIN_MESSAGE_MAP(CLogListView, CListView)
	//{{AFX_MSG_MAP(CLogListView)
	ON_COMMAND(ID_LOG_PAUSE, OnLogPause)
	ON_UPDATE_COMMAND_UI(ID_LOG_PAUSE, OnUpdateLogPause)
	ON_COMMAND(ID_LOG_STOP_DISPLAY, OnLogStopDisplay)
	ON_UPDATE_COMMAND_UI(ID_LOG_STOP_DISPLAY, OnUpdateLogStopDisplay)
	ON_COMMAND(ID_LOG_CONTINUE, OnLogContinue)
	ON_UPDATE_COMMAND_UI(ID_LOG_CONTINUE, OnUpdateLogContinue)
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_USER_ADD_ITEM, OnAddLogItem)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLogListView drawing

void CLogListView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CLogListView diagnostics

#ifdef _DEBUG
void CLogListView::AssertValid() const
{
	CListView::AssertValid();
}

void CLogListView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLogListView message handlers

BOOL CLogListView::PreCreateWindow(CREATESTRUCT& cs) 
{
    cs.style = (cs.style & ~LVS_TYPEMASK) | LVS_REPORT | LVS_SHOWSELALWAYS;
    cs.style |= LVS_AUTOARRANGE;
    // Set full line select
    //ListView_SetExtendedListViewStyle(GetSafeHwnd(), LVS_EX_FULLROWSELECT);
    
    return CListView::PreCreateWindow(cs);
}

void CLogListView::OnInitialUpdate() 
{
    CListView::OnInitialUpdate();
    
    // this code only works for a report-mode list view
    VERIFY(GetStyle() & LVS_REPORT);
}

void CLogListView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
    m_nType = (UINT)lHint;
    
    switch (m_nType) 
    {
    case LOG:
        InitHeadForLog();
        RefreshLog((CAdapterInfo *)pHint);
        break;               
        
    default:
        break;
    }
    
    ((CMainFrame *)AfxGetMainWnd())->SendMessage(WM_SIZE);	
}

BOOL CLogListView::InitHeadForLog()
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
    listView.InsertColumn(0, _T("序号"),    LVCFMT_LEFT, 40, 0);  
    listView.InsertColumn(1, _T("时间"),    LVCFMT_LEFT, 140, 0);  
    listView.InsertColumn(2, _T("协议"),    LVCFMT_LEFT, 40, 0);
    listView.InsertColumn(3, _T("方向"),    LVCFMT_LEFT, 40, 0);
    listView.InsertColumn(4, _T("动作"),    LVCFMT_LEFT, 40, 0);
    
    listView.InsertColumn(5, _T("源IP地址"),    LVCFMT_LEFT, 100, 0);
    listView.InsertColumn(6, _T("源端口"),      LVCFMT_LEFT, 60, 0);
    
    listView.InsertColumn(7, _T("目的IP地址"),  LVCFMT_LEFT, 100, 0);
    listView.InsertColumn(8, _T("目的端口"),    LVCFMT_LEFT, 60, 0);
    
    listView.InsertColumn(9, _T("大小"),     LVCFMT_LEFT, 40, 0);
    
    return TRUE;
}


BOOL CLogListView::AddLogItem(LOG_ITEM *pItem, int nIndex, DWORD dwSerial, CListCtrl& listView)
{
    ASSERT(pItem && nIndex >= 0);
    
    static TCHAR szIndex[16];
    static TCHAR szTime[20];            
    
    // Order
    _stprintf(szIndex, _T("%d"), dwSerial);
    listView.InsertItem(nIndex, szIndex);
    
    // Time
    //_stprintf(szTime, _T("%d.%d.%d.%d.%d.%d.%d.%d"), pItem->Now.Year, pItem->Now.Month, pItem->Now.Day, pItem->Now.Hour, pItem->Now.Minute, pItem->Now.Second, pItem->Now.Milliseconds, pItem->Now.Weekday);
    _stprintf(szTime, _T("%d.%d.%d.%d.%d.%d.%d"), pItem->Now.Year, pItem->Now.Month, pItem->Now.Day, pItem->Now.Hour, pItem->Now.Minute, pItem->Now.Second, pItem->Now.Weekday);
    listView.SetItemText(nIndex, 1, szTime);
    
    // Protocol
    char * pTemp = CRuleUtil::GetProtocolDescById(pItem->iProto);
    listView.SetItemText(nIndex, 2, _T(pTemp));
    
    // Direction
    pTemp = CRuleUtil::GetDescByDirectionId(pItem->ucDirection);
    listView.SetItemText(nIndex, 3, _T(pTemp));
    
    // Action
    //pTemp = CRuleUtil::GetDescByActionId(pItem->ucAction);
    pTemp = "通过";
    listView.SetItemText(nIndex, 4, _T(pTemp));
    
    // Source IP
    TCHAR szBuf[20];
    DWORD dwIP = pItem->ulSrcAddress;
    _stprintf(szBuf, _T("%d.%d.%d.%d"), (dwIP >> 24) & 0xFF, (dwIP >> 16) & 0xFF, (dwIP >> 8) & 0xFF, dwIP & 0xFF);
    listView.SetItemText(nIndex, 5, szBuf);
    
    // Source Port
    _stprintf(szBuf, _T("%d"), pItem->usSrcPort);            
    listView.SetItemText(nIndex, 6, szBuf);
    
    // Destination IP
    dwIP = pItem->ulDestAddress;
    _stprintf(szBuf, _T("%d.%d.%d.%d"), (dwIP >> 24) & 0xFF, (dwIP >> 16) & 0xFF, (dwIP >> 8) & 0xFF, dwIP & 0xFF);
    listView.SetItemText(nIndex, 7, szBuf);
    
    // Destination Port
    _stprintf(szBuf, _T("%d"), pItem->usDestPort);            
    listView.SetItemText(nIndex, 8, szBuf);
    
    // Packet Size
    _stprintf(szBuf, _T("%d"), pItem->iSize);            
    listView.SetItemText(nIndex, 9, szBuf);
    
    return TRUE;
}

int CLogListView::RefreshLog(CAdapterInfo *pAdapterInfo)
{
    CListCtrl& listCtrl = GetListCtrl();            
    
    // 1. 装载日志到系统内存空间
    if (! (m_bLogLoaded = ((CMainFrame *)AfxGetMainWnd())->LoadLogFile()))
    {        
        return 0;
    }
    
    // 2. 计算总共有多少项日志以及一页可以显示的项数	
    DWORD dwLogSize		= ((CMainFrame *)AfxGetMainWnd())->m_dwLogFileSize;
    DWORD dwLogCount	= dwLogSize / sizeof(LOG_ITEM);//38
    int   nCountPerPage = listCtrl.GetCountPerPage();
    int   nPageCount	= (dwLogSize + nCountPerPage) / nCountPerPage;
    
    // 3. 如果数据超过10页，则首先显示前5页数据，否则显示所有数据
    int nIndex   = 0;   
    m_dwCurItem  = 0;
    m_dwLogCount = dwLogCount;
    LOG_ITEM * pItem = (PLOG_ITEM)(((CMainFrame *)AfxGetMainWnd())->m_lpMapAddr);
    
    // 4. 如果数据少于10页，则显示所有数据
    if (nPageCount < 10)
    {
        while (m_dwCurItem < dwLogCount)
        {
            AddLogItem(pItem++, nIndex++, ++m_dwCurItem, listCtrl);	
        }	
        
        return m_dwCurItem;
    }
    
    // 5. 如果数据超过10页，则首先显示前5页数据，
    while (m_dwCurItem < dwLogCount)
    {
        AddLogItem(pItem++, nIndex++, ++m_dwCurItem, listCtrl);                 
        
        if (m_dwCurItem == nCountPerPage * 5)
        {
            break;
        }
    }
    
    // 6. 如果执行到这里，需要创建一个线程继续添加后面的项
    m_pThreadParams = new ThreadParams;
    if (! m_pThreadParams) 
    {
        AfxMessageBox("不能创建日志装载线程!");
        return m_dwCurItem;
    }
    
    HANDLE hWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hWaitEvent == NULL)
    {
        AfxMessageBox("不能创建日志装载线程!");
        delete m_pThreadParams;
        m_pThreadParams = NULL;
        return m_dwCurItem;
    }
    
    m_pThreadParams->m_hwndProgress = AfxGetMainWnd()->m_hWnd;	
    m_pThreadParams->m_hwndView   = this->GetSafeHwnd();
    m_pThreadParams->m_lpLogBase  = ((CMainFrame *)AfxGetMainWnd())->m_lpMapAddr;
    m_pThreadParams->m_dwLogCount = m_dwLogCount; 
    m_pThreadParams->m_dwCurItem  = m_dwCurItem;
    m_pThreadParams->m_hWaitEvent = hWaitEvent;
    m_pThreadParams->m_bPaused    = FALSE;
    m_pThreadParams->m_bInterrupt = FALSE;
    m_pThreadParams->m_bDone	  = FALSE;
    
    AfxBeginThread(LogThreadFunction, m_pThreadParams);
    
    m_bThreadStarted = TRUE;

    SetEvent(m_pThreadParams->m_hWaitEvent);
    
    ::SendMessage(m_pThreadParams->m_hwndProgress,
        WM_USER_LOGTHREAD_STATUS, 0, (LPARAM)m_pThreadParams);

    ::SendMessage(m_pThreadParams->m_hwndProgress,
        WM_USER_LOGTHREAD_STATUS, 1, (LPARAM)m_pThreadParams);

    ::SendMessage(m_pThreadParams->m_hwndProgress,
        WM_USER_LOGTHREAD_STATUS, 2, (LPARAM)m_pThreadParams);

    ::SendMessage(m_pThreadParams->m_hwndProgress,
        WM_USER_LOGTHREAD_STATUS, 3, (LPARAM)m_pThreadParams);

    return m_dwCurItem;
}

UINT CLogListView::LogThreadFunction(LPVOID pParam)
{
    ThreadParams * lpThreadParams = (PThreadParams)pParam;
    if (lpThreadParams == NULL) 
    {
        return -1;
    }
    
    //DWORD dwCurItem  = lpThreadParams->m_dwCurItem;
    DWORD dwLogCount = lpThreadParams->m_dwLogCount;
    LOG_ITEM * pItem = (PLOG_ITEM)((LPBYTE)lpThreadParams->m_lpLogBase + lpThreadParams->m_dwCurItem * sizeof(LOG_ITEM));
    
    while (TRUE)
    {
        // Wait for user continue display LOG_ITEM of the rest.
        // (check for m_bInterrupt going TRUE every 100ms)
        while (WaitForSingleObject(lpThreadParams->m_hWaitEvent, 100) == WAIT_TIMEOUT)
        {
            if (lpThreadParams->m_bInterrupt)
            {                
                goto Exit;
            }
        }        
        
        while (lpThreadParams->m_dwCurItem < dwLogCount)
        {			
            lpThreadParams->m_dwCurItem = ::SendMessage(lpThreadParams->m_hwndView, WM_USER_ADD_ITEM, 0, (LPARAM)pItem);
            ::SendMessage(lpThreadParams->m_hwndProgress,
                WM_USER_LOGTHREAD_STATUS, 2, (LPARAM)lpThreadParams);

            Sleep(1);
            if (lpThreadParams->m_bInterrupt)
            {                                
                goto Exit;
            }
            else if (lpThreadParams->m_bPaused)
            {                
                break;
            }                          
        }
        
        if (lpThreadParams->m_dwCurItem == dwLogCount)
        {
            lpThreadParams->m_bDone = TRUE;            
            break;
        }
    }
    
    ::SendMessage(lpThreadParams->m_hwndProgress,
       WM_USER_LOGTHREAD_STATUS, 3, (LPARAM)lpThreadParams); 
    
Exit:    
    return 0;
}

LRESULT CLogListView::OnAddLogItem(WPARAM wParam, LPARAM lParam)
{
    CListCtrl& listCtrl = GetListCtrl();
    int nListEntries = listCtrl.GetItemCount();
    LOG_ITEM * pItem = (PLOG_ITEM)lParam;
    
    AddLogItem(pItem, nListEntries, ++m_dwCurItem, listCtrl);     	
    
    return m_dwCurItem;
}

void CLogListView::OnUpdateLogPause(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(m_bThreadStarted && ! m_pThreadParams->m_bPaused);    
}

void CLogListView::OnLogPause() 
{
    m_pThreadParams->m_bPaused = TRUE;
    ResetEvent(m_pThreadParams->m_hWaitEvent);
    
    ::SendMessage(m_pThreadParams->m_hwndProgress,
        WM_USER_LOGTHREAD_STATUS, 3, (LPARAM)m_pThreadParams); 
}

void CLogListView::OnUpdateLogContinue(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(m_bThreadStarted && m_pThreadParams->m_bPaused);    
}

void CLogListView::OnLogContinue() 
{
    m_pThreadParams->m_bPaused = FALSE;
    SetEvent(m_pThreadParams->m_hWaitEvent);

    ::SendMessage(m_pThreadParams->m_hwndProgress,
        WM_USER_LOGTHREAD_STATUS, 3, (LPARAM)m_pThreadParams);
}

void CLogListView::OnUpdateLogStopDisplay(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(m_bThreadStarted && ! m_pThreadParams->m_bDone); 
}

void CLogListView::OnLogStopDisplay() 
{
    m_pThreadParams->m_bPaused = TRUE;
    m_pThreadParams->m_bInterrupt = TRUE;
    ResetEvent(m_pThreadParams->m_hWaitEvent);

    ::SendMessage(m_pThreadParams->m_hwndProgress,
        WM_USER_LOGTHREAD_STATUS, 3, (LPARAM)m_pThreadParams);

    ((CMainFrame *)AfxGetMainWnd())->m_bLoadCompleted = TRUE;
    m_bThreadStarted = FALSE;

    if (NULL != m_pThreadParams)
    {
        delete m_pThreadParams;
        m_pThreadParams = NULL;
    }    
}

