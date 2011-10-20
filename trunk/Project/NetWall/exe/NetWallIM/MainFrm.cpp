// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include <afxpriv.h>
#include "NetWall.h"
#include "MainFrm.h"
#include "NetWallDoc.h"
#include "NetWallView.h"
#include "RuleListView.h"
#include "DisplayRuleView.h"
#include "RuleUtil.h"
#include "LogListView.h"
#include "AddRuleDlg.h"
#include "initguid.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const DWORD ADAPTER_BUFFER_SIZE = 10 * 1024;

/////////////////////////////////////////////////////////////////////////////
// CAdapterInfo Class Implementation

IMPLEMENT_DYNAMIC(CAdapterInfo, CObject)

CAdapterInfo::CAdapterInfo() : CObject()
{
    m_hAdapter               = INVALID_HANDLE_VALUE;
    m_bRuleSeted             = FALSE;
    m_bRuleSetedNoFresh      = FALSE;

    m_dwNumberOfRules        = 0;
    m_dwVirtualAddress       = 0;   // There is No Section Header for this adapter
    
    m_strVirtualAdapterName  = _T("Unknown");
    m_strLowerAdapterName    = _T("Unknown");
    m_strFriendlyAdapterName = _T("Unknown");    
}


CAdapterInfo::~CAdapterInfo()
{
    if (INVALID_HANDLE_VALUE != m_hAdapter)
    {
        CloseHandle(m_hAdapter);
        m_hAdapter = INVALID_HANDLE_VALUE;
    }    
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_ADAPTER_OPEN, OnAdapterOpen)
	ON_COMMAND(ID_ADAPTER_CLOSE, OnAdapterClose)
	ON_WM_SIZE()
	ON_COMMAND(ID_RULE_ADD, OnRuleAdd)
	ON_COMMAND(ID_RULE_DELETE, OnRuleDelete)
	ON_UPDATE_COMMAND_UI(ID_RULE_DELETE, OnUpdateRuleDelete)
	ON_UPDATE_COMMAND_UI(ID_ADAPTER_OPEN, OnUpdateAdapterOpen)
	ON_UPDATE_COMMAND_UI(ID_ADAPTER_CLOSE, OnUpdateAdapterClose)
	ON_COMMAND(ID_RULE_DELETEALL, OnRuleDeleteall)
	ON_UPDATE_COMMAND_UI(ID_RULE_DELETEALL, OnUpdateRuleDeleteall)
	ON_COMMAND(ID_RULE_MODIFY, OnRuleModify)
	ON_UPDATE_COMMAND_UI(ID_RULE_MODIFY, OnUpdateRuleModify)
	ON_COMMAND(ID_ADAPTER_SETRULE, OnAdapterSetrule)
	ON_UPDATE_COMMAND_UI(ID_ADAPTER_SETRULE, OnUpdateAdapterSetrule)
	ON_COMMAND(ID_ADAPTER_CLEARRULE, OnAdapterClearrule)
	ON_UPDATE_COMMAND_UI(ID_ADAPTER_CLEARRULE, OnUpdateAdapterClearrule)
	ON_COMMAND(ID_LOG_START, OnLogStart)
	ON_COMMAND(ID_LOG_STOP, OnLogStop)
	ON_UPDATE_COMMAND_UI(ID_LOG_START, OnUpdateLogStart)
	ON_UPDATE_COMMAND_UI(ID_LOG_STOP, OnUpdateLogStop)
	ON_COMMAND(ID_LOG_LOAD, OnLogLoad)
	ON_UPDATE_COMMAND_UI(ID_LOG_LOAD, OnUpdateLogLoad)
	//}}AFX_MSG_MAP
    
    ON_MESSAGE(WM_USER_DISPLAY_RULEITEM, OnView)
    ON_MESSAGE(WM_USER_LOGTHREAD_STATUS, OnLogThreadStatus)

END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_SEPARATOR,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{	
    m_AdapterList.clear();
       
    m_nAdapterCount         = 0;
    m_nOpenAdapterCount     = 0;
    
    m_pCurrentAdapter       = NULL;
    m_strVirtualAdapterName = _T("");
    m_strLowerAdapterName   = _T("");
    m_strFriendlyAdapterName= _T("");

    m_bSplitterCreated      = FALSE;

    m_pFile                 = NULL;
    m_pBuffer               = NULL;
    m_dwBufferLength        = 0;    

    m_nIndex                = -1;
    
    m_bLogStarted           = FALSE;

    RtlZeroMemory(m_pFileName, MAX_PATH);

    m_bLogLoaded            = FALSE;
    m_bLoadCompleted        = FALSE;
    m_hLogFile              = INVALID_HANDLE_VALUE;
    m_hMapFile              = INVALID_HANDLE_VALUE;
    m_lpMapAddr             = NULL;
    m_dwLogFileSize         = 0;

    InitializeCriticalSection(&m_CritSect);
}

CMainFrame::~CMainFrame()
{
    // Close Rule File
    if (NULL != m_pFile)
    {
        if (NULL != m_pBuffer && 0 < m_dwBufferLength)
        {
            m_pFile->SeekToBegin();
            m_pFile->SetLength(0);
            m_pFile->Write(m_pBuffer, m_dwBufferLength);
            delete []m_pBuffer;
            m_pBuffer = NULL;
        }
        m_pFile->Close();
        delete m_pFile;
        m_pFile = NULL;
    }
    
    // Close Adapter And Free Adapter List 
    if (0 < m_AdapterList.size())
    {
        CAdapterListIterator it;        

        for (it = m_AdapterList.begin(); 
             it != m_AdapterList.end(); 
             ++it)
        {
            delete *it;
            *it = NULL;
        }        

        m_AdapterList.clear();
    }

    // Unmap Log memory from the process's address space.
    if (NULL != m_lpMapAddr)
    {
        UnmapViewOfFile(m_lpMapAddr);
        m_lpMapAddr = NULL;
    }
    
    // Close the Log File's handle to the file-mapping object.    
    if (INVALID_HANDLE_VALUE != m_hMapFile) 
    { 
        CloseHandle(m_hMapFile);   
        m_hMapFile = INVALID_HANDLE_VALUE;
    } 
    
    if (INVALID_HANDLE_VALUE != m_hLogFile)
    {
        CloseHandle(m_hLogFile);            
        m_hLogFile = INVALID_HANDLE_VALUE;
    }

    m_bLogLoaded = FALSE;

    DeleteCriticalSection(&m_CritSect);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
    {    
		return -1;
    }
	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators, 4))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
    
    // Get Adapters
    m_nAdapterCount = GetAdapterList();
    if (0 == m_nAdapterCount)
    {
        AfxMessageBox(_T("There is no driver, or driver is not running!"));
        return -1;
    }
    
    // Set Status Bar
    TEXTMETRIC tm;
    CClientDC dc(this);
    CFont* pFont    = m_wndStatusBar.GetFont();
    CFont* pOldFont = dc.SelectObject(pFont);
    dc.GetTextMetrics(&tm);
    dc.SelectObject(pOldFont);
    
    int cxWidth;
    UINT nID, nStyle;
    m_wndStatusBar.GetPaneInfo(1, nID, nStyle, cxWidth);
    m_wndStatusBar.SetPaneInfo(1, nID, nStyle, tm.tmAveCharWidth * 24);
    m_wndStatusBar.SetPaneInfo(2, nID, nStyle, tm.tmAveCharWidth * 24);
    m_wndStatusBar.SetPaneInfo(3, nID, nStyle, tm.tmAveCharWidth * 24);    

    //UpdateStatusBar();
    
	// Delete these three lines if you don't want the toolbar to
	// be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

    // Get Rule File name        
    GetSystemDirectory(m_pFileName, MAX_PATH);
    _tcscat(m_pFileName, _T("\\NetWall\\"));
    CreateDirectory(m_pFileName, NULL);    
    _tcscat(m_pFileName, "NetWall.dat");
    
    // First Check the file exists or not.
    BOOL bExist = TRUE;
    TRY
    {
        m_pFile = new CFile(m_pFileName, CFile::modeRead);
    }
    CATCH (CFileException, e)
    {
        TRACE("%s Not Exists!\n", m_pFileName);
        bExist = FALSE;
    }
    END_CATCH

    // If Exists, free it.
    if (m_pFile)
    {
        delete m_pFile;
        m_pFile = NULL;
    }

    // Open Rule File
    TRY
    {
        if (bExist)
        {
            m_pFile = new CFile(m_pFileName, CFile::modeReadWrite | 
                                             CFile::shareDenyWrite);    
            m_pFile->SeekToBegin();
            m_dwBufferLength = m_pFile->GetLength();
            if (0 < m_dwBufferLength)
            {
                m_pBuffer = new BYTE[m_dwBufferLength];

                m_pFile->Read(m_pBuffer, m_dwBufferLength);

                PRULE_FILE_HEADER pFileHead = (PRULE_FILE_HEADER)m_pBuffer;
                
                if (! ((NETWALL_RULE_SIGNATURE == pFileHead->Signature || NETWALL_RULE_SIGNATURE1 == pFileHead->Signature) 
                    && (pFileHead->dwVersion == ((CNetWallApp * )AfxGetApp())->m_nAppAPIVersion)
                    && RULE_FILE_HEADER_SIZE <= pFileHead->dwTotal))
                {
                    AfxMessageBox(IDS_API_VER_MISMATCH);
                    
                    RULE_FILE_HEADER ruleHdr;
                    
                    CRuleUtil::CreateRuleFileHead(&ruleHdr);
                    m_dwBufferLength = RULE_FILE_HEADER_SIZE;
                    RtlCopyMemory(m_pBuffer, &ruleHdr, m_dwBufferLength);
                    m_pFile->Write(m_pBuffer, m_dwBufferLength);                    
                }   
            }
            else
            {
                AfxMessageBox(_T("There is no rule for this adapter!"));  
                RULE_FILE_HEADER ruleHdr;
                
                CRuleUtil::CreateRuleFileHead(&ruleHdr);
                m_dwBufferLength = RULE_FILE_HEADER_SIZE;

                m_pBuffer = new BYTE[m_dwBufferLength];

                RtlCopyMemory(m_pBuffer, &ruleHdr, m_dwBufferLength);
                m_pFile->Write(m_pBuffer, m_dwBufferLength);
            }
        }
        else
        {
            m_pFile = new CFile(m_pFileName, CFile::modeCreate     | 
                                             CFile::modeNoTruncate | 
                                             CFile::modeReadWrite  | 
                                             CFile::shareDenyWrite |
                                             CFile::typeBinary   );
            RULE_FILE_HEADER ruleHdr;
            CRuleUtil::CreateRuleFileHead(&ruleHdr);

            m_dwBufferLength = RULE_FILE_HEADER_SIZE;
            m_pBuffer = new BYTE[m_dwBufferLength];
            
            RtlCopyMemory(m_pBuffer, &ruleHdr, m_dwBufferLength);
            m_pFile->Write(m_pBuffer, m_dwBufferLength);
        }
    }
    CATCH (CFileException, pEx)
    {
        CString strOut;
        strOut.Format("File %s Open Failed!", m_pFileName);
        AfxMessageBox(strOut, MB_OK | MB_ICONHAND);
        if (m_pFile)
        {
            delete m_pFile;
            m_pFile = NULL;
        }
    }    
    AND_CATCH (CMemoryException, pEx)
    {        
        AfxMessageBox(IDS_MEMORY_NOT_ENOUGH);
        AfxAbort();
    }
    END_CATCH      
            
    // Set Frame's Title
    CString strTitle;
    strTitle.LoadString(AFX_IDS_APP_TITLE);
    SetWindowText(strTitle);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (! CFrameWnd::PreCreateWindow(cs))
    {
		return FALSE;
    }

	// Modify the Window styles 
    cs.style &= ~FWS_ADDTOTITLE;

	return TRUE;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	//
	// Create the CRuleListView first so the CNetWallView's OnInitialUpdate
	// function can call OnUpdate on the CRuleListView.
	//
    if (!m_wndSplitter.CreateStatic(this, 1, 2) ||
        !m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CRuleListView), CSize(0, 0), pContext) ||
        !m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CNetWallView),  CSize(0, 0), pContext))
    {
        return FALSE;
    }

    m_bSplitterCreated = TRUE;

    return m_bSplitterCreated;	
}

void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
	
	CRect rect;
    GetWindowRect(&rect);
    if (m_bSplitterCreated)  // m_bSplitterCreated set in OnCreateClient
    {
        m_wndSplitter.SetColumnInfo(0, rect.Width() / 6, 10);
        m_wndSplitter.SetColumnInfo(1, rect.Width() / 6 * 5, 10);
        m_wndSplitter.RecalcLayout();
    }
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	//
    // Route to standard command targets first.
	//
    if (CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
    {
        return TRUE;
    }

	//
    // Route to inactive views second.
	//
    CNetWallDoc* pDoc = (CNetWallDoc *)GetActiveDocument();
    if (NULL != pDoc) // Important!
    { 
		return pDoc->RouteCmdToAllViews(GetActiveView(), nID, nCode, pExtra, pHandlerInfo);
	}

    return FALSE;
}

LRESULT CMainFrame::OnView(WPARAM wParam, LPARAM lParam) 
{
    UINT nType  = wParam;

    /*m_bLogLoaded = FALSE;

    // 如果先前装载了日志，现在卸载它
    if (m_bLogLoaded)
    {
        // Unmap Log memory from the process's address space.
        if (NULL != m_lpMapAddr)
        {
            UnmapViewOfFile(m_lpMapAddr);
            m_lpMapAddr = NULL;
        }
        
        // Close the Log File's handle to the file-mapping object.    
        if (INVALID_HANDLE_VALUE != m_hMapFile) 
        { 
            CloseHandle(m_hMapFile);   
            m_hMapFile = INVALID_HANDLE_VALUE;
        } 
        
        if (INVALID_HANDLE_VALUE != m_hLogFile)
        {
            CloseHandle(m_hLogFile);            
            m_hLogFile = INVALID_HANDLE_VALUE;
        }
        
        m_bLogLoaded = FALSE;
    }*/

    switch (nType)
    {
        // 在一个FormView中显示规则详细信息
        case RULEITEM:	
        {        
            RULE_ITEM  *pNode = (PRULE_ITEM)lParam;
            ReplaceView(RUNTIME_CLASS(CDisplayRuleView));	
        }
        break;

        // 在 CLogListView 列表视图中显示日志详细信息
        case LOG:
        {        
            ReplaceView(RUNTIME_CLASS(CLogListView));	
        }
        break;
        
        // 在 CRuleListView 列表视图中显示适配器、规则的信息
        case ADAPTER:
        case RULE:	        
        default: 
        {
            ReplaceView(RUNTIME_CLASS(CRuleListView));
        }
        
        break;        
    }
    
    return TRUE;
}

BOOL CMainFrame::ReplaceView(CRuntimeClass *pViewClass)
{
    CCreateContext context;
    CView * pCurrentView = NULL;    
    
    // if no active view for the frame, return FALSE because this 
    // function retrieves the current document from the active view
    if ((pCurrentView = (CView *)m_wndSplitter.GetPane(0, 1)) == NULL)
    {
        return FALSE;               
    }
    
    // If we're already displaying this kind of view, no need to go 
    // further. 
    if ((pCurrentView->IsKindOf(pViewClass)) == TRUE)
    {
        return TRUE;
    }
            
    // Get pointer to CDocument object so that it can be used in the creation 
    // process of the new view
    CDocument * pDoc= pCurrentView->GetDocument();
    
    // set flag so that document will not be deleted when view is destroyed
    BOOL bAutoDelete    = pDoc->m_bAutoDelete;
    pDoc->m_bAutoDelete = FALSE; 
    
    // Delete existing view 
    m_wndSplitter.DeleteView(0, 1);

    // restore flag  
    pDoc->m_bAutoDelete = bAutoDelete;
        
    // Create new view and redraw    
    context.m_pNewViewClass   = pViewClass;
    context.m_pCurrentDoc     = pDoc;
    context.m_pNewDocTemplate = NULL;
    context.m_pLastView       = NULL;
    context.m_pCurrentFrame   = this;
    
    if (! m_wndSplitter.CreateView(0, 1, pViewClass, CSize(0, 0), &context))
    {
        TRACE0("Warning: couldn't create view for frame\n");
        return FALSE;  // programmer can assume FALSE return value 
        // from this function means that there 
        //isn't a view
    }

    // WM_INITIALUPDATE is define in AFXPRIV.H		
    m_wndSplitter.GetPane(0, 1)->SendMessage(WM_INITIALUPDATE, 0, 0); 

    RecalcLayout();   

    m_wndSplitter.GetPane(0, 1)->UpdateWindow();
    
    SetActiveView((CView *)m_wndSplitter.GetPane(0, 1));
    
    return TRUE;
}

BOOL CMainFrame::SetCurrentAdapter(CAdapterInfo *pAdapterinfo)
{
    m_pCurrentAdapter = pAdapterinfo;

    HasRuleForAdapter(pAdapterinfo);

    m_strVirtualAdapterName  = m_pCurrentAdapter->m_strVirtualAdapterName;
    m_strLowerAdapterName    = m_pCurrentAdapter->m_strLowerAdapterName;
    m_strFriendlyAdapterName = m_pCurrentAdapter->m_strFriendlyAdapterName;
    
    UpdateStatusBar(ADAPTER);
    /*
    CString strCount;
    if (INVALID_HANDLE_VALUE == m_pCurrentAdapter->m_hAdapter)
    {
        strCount.Format("适配器状态: 关闭");
    }
    else
    {
        strCount.Format("适配器状态: 打开");
    }
    
    m_wndStatusBar.SetPaneText(1, (LPCSTR)strCount, TRUE);
*/
    return TRUE;
}

DWORD CMainFrame::GetAdapterList()
{
    CNetWallApp *pApp = (CNetWallApp *)AfxGetApp();

    int   nAdapterCount = 0;
    
    //
    // Open The Device Handle
    //
    HANDLE hIMHandle = INVALID_HANDLE_VALUE;
    hIMHandle = CreateFile(NETWALL_WDM_DEVICE_FILENAME,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL
                           );
    
    if (INVALID_HANDLE_VALUE == hIMHandle)
    {
        AfxMessageBox(_T("Driver is not loaded. Try reloading the app."));
        return nAdapterCount;
    }
    
    //
    // Allocate A Buffer Of The Required Size
    //
    PWCHAR pwszAdapterList = NULL;
    pwszAdapterList = new WCHAR[ADAPTER_BUFFER_SIZE];
        
    if (NULL == pwszAdapterList)
    {
        //
        // Memory Allocation Failed
        //
        CloseHandle(hIMHandle);
            
        return nAdapterCount;        
    }

    try
    {   
        //
        // Make Second Query To Actually Retrieve Binding Information
        //
        UINT nBufferSize = ADAPTER_BUFFER_SIZE;
        BOOL bResult = pApp->GetAPI()->EnumerateBindings(hIMHandle, pwszAdapterList, &nBufferSize);  
    
        CloseHandle(hIMHandle);

        if (! bResult)
        {
            //
            // I/O Operation Failed
            //
            delete [] pwszAdapterList;
            
            AfxMessageBox(_T("Enum Adapter Failed"));
        
            return nAdapterCount;
        }
    }
    catch (...)
    {
        delete [] pwszAdapterList;
        return nAdapterCount;
    }

    //
    // Fill The AdapterListBox
    //
    nAdapterCount = *(PULONG)pwszAdapterList;
    PWCHAR pWStr = (PWCHAR)((PCHAR)pwszAdapterList + sizeof(ULONG));
    
    while (*pWStr)
    {
        CAdapterInfo *pAdapterInfo = new CAdapterInfo();

        TCHAR szMac[4];     
        RtlZeroMemory(szMac, sizeof(szMac));

        CString strTmp = _T("");

        for (INT i = 0; i < ETHER_ADDR_LENGTH / 2; i++)
        {
            WCHAR ch = *pWStr++;
            _stprintf(szMac, _T("%02x-"), ch & 0xFF);
            strTmp += szMac;
            if (i == 2)
            {
                _stprintf(szMac, _T("%02x"), (ch >> 8) & 0xFF);
            }
            else
            {
                _stprintf(szMac, _T("%02x-"), (ch >> 8) & 0xFF);
            }
            
            strTmp += szMac;
        }

        pAdapterInfo->m_strAdapterMacAddress = strTmp;

        ++pWStr;

        pAdapterInfo->m_strVirtualAdapterName = pWStr;
        
        pWStr += wcslen(pWStr);
        ++pWStr;
        
        if (*pWStr)
        {
            pAdapterInfo->m_strLowerAdapterName = pWStr;
            
            pWStr += wcslen(pWStr);
            ++pWStr;
        }
        else
        {
            break;
        }
        
        if (*pWStr)
        {
            pAdapterInfo->m_strFriendlyAdapterName = pWStr;
            
            pWStr += wcslen(pWStr);
            ++pWStr;
        }
        else
        {
            break;
        }
        
        m_AdapterList.push_back(pAdapterInfo);                
    }   
    
    delete [] pwszAdapterList;
    
    return nAdapterCount;
}

CAdapterList * CMainFrame::GetAdapters()
{
    return &m_AdapterList;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnUpdateAdapterOpen(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(NULL != m_pCurrentAdapter 
        && INVALID_HANDLE_VALUE == m_pCurrentAdapter->m_hAdapter);
}

void CMainFrame::OnAdapterOpen() 
{
    CNetWallApp *pApp = (CNetWallApp * )AfxGetApp();

    //
    // Open The Adapter
    //
    if (NULL == m_pCurrentAdapter)
    {
        AfxMessageBox(_T("Please Select A Virtual Adapter!"));
        return;
    }

    m_pCurrentAdapter->m_hAdapter = pApp->GetAPI()->OpenVirtualAdapter((LPSTR)((LPCTSTR)m_strVirtualAdapterName));
        
    if (INVALID_HANDLE_VALUE == m_pCurrentAdapter->m_hAdapter)
    {
        AfxMessageBox(_T("Open Virtual Adapter Failed!"));
        DWORD error = GetLastError();
        return;
    }

    m_nOpenAdapterCount++;

    UpdateStatusBar(ADAPTER);    
}

void CMainFrame::OnUpdateAdapterClose(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(NULL != m_pCurrentAdapter 
        && INVALID_HANDLE_VALUE != m_pCurrentAdapter->m_hAdapter);
}

void CMainFrame::OnAdapterClose() 
{
    if (NULL != m_pCurrentAdapter 
        && INVALID_HANDLE_VALUE != m_pCurrentAdapter->m_hAdapter)
    {
        CloseHandle(m_pCurrentAdapter->m_hAdapter);
        m_pCurrentAdapter->m_hAdapter = INVALID_HANDLE_VALUE;
    }

	m_nOpenAdapterCount--;

    UpdateStatusBar(ADAPTER);    
}

RULE_ITEM * CMainFrame::GetAdapterRuleItem()
{
    ASSERT(m_pCurrentAdapter);
    
    RULE_ITEM * pItem = (PRULE_ITEM)(m_pBuffer + m_pCurrentAdapter->m_dwVirtualAddress);
    
    return pItem;
}

BOOL CMainFrame::HasRuleForAdapter(CAdapterInfo * pAdaterInfo)
{
    PRULE_ITEM  pRuleItem  = NULL;
    DWORD       dwRuleSize = 0;
    BOOL        bHave      = FALSE;
    
    ASSERT(pAdaterInfo);
    
    PRULE_FILE_HEADER pFileHead = (PRULE_FILE_HEADER)m_pBuffer;
    
    if (0 < pFileHead->NumberOfSections)
    {    
        PRULE_SECTION_HEADER pSecHdr = (PRULE_SECTION_HEADER)(m_pBuffer + RULE_FILE_HEADER_SIZE);
        
        for (UINT i = 0; i < (UINT)pFileHead->NumberOfSections; i++) 
        {
            if (! _tcscmp((LPCTSTR)pSecHdr->Name, pAdaterInfo->m_strLowerAdapterName))
            {
                if (0 < pSecHdr->NumberOfRules)
                {
                    bHave = TRUE;
                    
                    if (pAdaterInfo == m_pCurrentAdapter)
                    {
                        m_pCurrentAdapter->m_dwNumberOfRules  = pSecHdr->NumberOfRules;
                        m_pCurrentAdapter->m_dwVirtualAddress = pSecHdr->VirtualAddress;
                    }
                    break;                    
                }
            }
            
            pSecHdr++;
        }
    }
    
    return bHave;
}

void CMainFrame::OnRuleAdd() 
{
    CAddRuleDlg  dlg;
    
    if (IDOK != dlg.DoModal() || NULL == dlg.m_pBuffer)
    {
        return;
    }

    try
    {
        RULE_ITEM * pItem = (PRULE_ITEM)dlg.m_pBuffer;
        
        DWORD dwHeaderSectionSize = 0;
        DWORD dwPreviousRuleSize  = 0;

        //
        // 第一种情况，当前适配器从未设置过规则, 即不存在该适配器的SECTION_HEADER和规则:
        //  . 修改规则文件的头部
        //  . 为这个适配器添加一项SECTION_HEADER
        //  . 将这个规则添加到后面
        //
        if (0 == m_pCurrentAdapter->m_dwVirtualAddress)
        {
            time_t  ltime;
            time(&ltime);
            
            RULE_FILE_HEADER * pFileHead  = (PRULE_FILE_HEADER)m_pBuffer;
            
            // Modify Rule File Header
            pFileHead->TimeDateStamp    =  ltime;
            pFileHead->NumberOfSections += 1;                
            pFileHead->dwTotal          += RULE_SECTION_HEADER_SIZE + pItem->cbSize;
            
            m_dwBufferLength = pFileHead->dwTotal;            
            
            // Count HeaderSectionSize and PreviousRuleSize, Modify Previous SECTION_HEADER's VirtualAddress
            dwHeaderSectionSize = RULE_FILE_HEADER_SIZE + pFileHead->NumberOfSections * RULE_SECTION_HEADER_SIZE;
            dwPreviousRuleSize = 0;
            PRULE_SECTION_HEADER pSecHdr = (PRULE_SECTION_HEADER)(m_pBuffer + RULE_FILE_HEADER_SIZE);
            for (int i = 0; i < pFileHead->NumberOfSections - 1; i++)
            {
                pSecHdr->VirtualAddress += RULE_SECTION_HEADER_SIZE;
                dwPreviousRuleSize += pSecHdr->VirtualSize;
                pSecHdr++;
            }

            // ReAllocate Memory 
            LPBYTE m_pTmpBuffer = NULL;
            m_pTmpBuffer = new BYTE[m_dwBufferLength];
            ASSERT(m_pTmpBuffer);
            RtlZeroMemory(m_pTmpBuffer, m_dwBufferLength);
            
            // Copy Rule File Header and Previous Section header
            RtlMoveMemory(m_pTmpBuffer, pFileHead, RULE_FILE_HEADER_SIZE);
            if (1 < pFileHead->NumberOfSections)
            {                
                RtlMoveMemory(m_pTmpBuffer + RULE_FILE_HEADER_SIZE, 
                              m_pBuffer + RULE_FILE_HEADER_SIZE,
                              (pFileHead->NumberOfSections - 1) * RULE_SECTION_HEADER_SIZE
                             );
            }                                 
            
            // Construct This Adapter's Section Header
            RULE_SECTION_HEADER secHdr;
            
            RtlZeroMemory(&secHdr, RULE_SECTION_HEADER_SIZE);
            
            _tcscpy((LPTSTR)secHdr.Name, m_strLowerAdapterName);
            secHdr.NumberOfRules  = 1;
            secHdr.TimeDateStamp  = ltime;
            secHdr.VirtualSize    = pItem->cbSize;
            secHdr.VirtualAddress = dwHeaderSectionSize + dwPreviousRuleSize;
            
            // Copy This Section Header to Rear of Previous Section Header
            RtlMoveMemory(m_pTmpBuffer + dwHeaderSectionSize - RULE_SECTION_HEADER_SIZE,
                          &secHdr, 
                          RULE_SECTION_HEADER_SIZE
                         );
            
            // Copy Previous Rule Item to Rear of This Section Header                
            RtlMoveMemory(m_pTmpBuffer + dwHeaderSectionSize,
                          m_pBuffer + dwHeaderSectionSize - RULE_SECTION_HEADER_SIZE,
                          dwPreviousRuleSize
                         );
            
            // Copy This Rule Item to Rear of Previous Rule Items
            RtlMoveMemory(m_pTmpBuffer + dwHeaderSectionSize + dwPreviousRuleSize,
                          pItem,
                          pItem->cbSize
                         );
            
            // Set Buffer 
            delete []m_pBuffer;
            m_pBuffer = NULL;
            
            m_pBuffer = m_pTmpBuffer;                
            
            // Write Buffer into File
            m_pFile->SeekToBegin();
            m_pFile->SetLength(0);
            m_pFile->Write(m_pBuffer, m_dwBufferLength);
            
            m_pCurrentAdapter->m_dwNumberOfRules += 1;
            m_pCurrentAdapter->m_dwVirtualAddress = dwHeaderSectionSize + dwPreviousRuleSize;
            
            // Refresh CNetWallView
            CNetWallView * pView = (CNetWallView *)m_wndSplitter.GetPane(0, 0);
            ASSERT(pView);
            ASSERT_KINDOF(CNetWallView, pView);
            
            pView->Refresh(RULE, m_pCurrentAdapter);
        }

        //
        // 第二种情况，当前适配器以前设置过规则，但全部删除了, 即存在该适配器的SECTION_HEADER, 但没有规则:
        //  . 需要修改规则文件的头部
        //  . 需要修改这个适配器SECTION的头部,
        //  . 并将这个规则添加到后面
        //
        else if (0 == m_pCurrentAdapter->m_dwNumberOfRules)
        {
            time_t  ltime;
            time(&ltime);
            
            RULE_FILE_HEADER * pFileHead  = (PRULE_FILE_HEADER)m_pBuffer;
            
            // Modify Rule File Header
            pFileHead->TimeDateStamp    =  ltime;
            pFileHead->dwTotal          += pItem->cbSize;
            
            DWORD dwPrevBufferLength = m_dwBufferLength;
            m_dwBufferLength         = pFileHead->dwTotal;                       
            
            dwHeaderSectionSize = RULE_FILE_HEADER_SIZE + pFileHead->NumberOfSections * RULE_SECTION_HEADER_SIZE;
            dwPreviousRuleSize  = 0;

            // Modify Adapter's Section Header
            PRULE_SECTION_HEADER pSecHdr = (PRULE_SECTION_HEADER)(m_pBuffer + RULE_FILE_HEADER_SIZE);
            
            for (UINT i = 0; i < (UINT)pFileHead->NumberOfSections; i++) 
            {
                dwPreviousRuleSize += pSecHdr->VirtualSize;

                if (! _tcscmp((LPCTSTR)pSecHdr->Name, m_pCurrentAdapter->m_strLowerAdapterName))
                {            
                    pSecHdr->TimeDateStamp = ltime;
                    pSecHdr->NumberOfRules++;          
                    pSecHdr->VirtualSize  = pItem->cbSize;
                }
                
                pSecHdr++;
            }

            // ReAllocate Memory 
            LPBYTE m_pTmpBuffer = NULL;
            m_pTmpBuffer = new BYTE[m_dwBufferLength];
            ASSERT(m_pTmpBuffer);
            RtlZeroMemory(m_pTmpBuffer, m_dwBufferLength);
            
            // Copy Rule File Header, Previous Section header and Rule Items
            RtlMoveMemory(m_pTmpBuffer, m_pBuffer, dwPrevBufferLength);
                                    
            // Copy This Rule Item to Rear of Previous Rule Items
            RtlMoveMemory(m_pTmpBuffer + dwPrevBufferLength,
                          pItem,
                          pItem->cbSize
                         );
            
            // Set Buffer 
            delete []m_pBuffer;
            m_pBuffer = NULL;
            
            m_pBuffer = m_pTmpBuffer;                
            
            // Write Buffer into File
            m_pFile->SeekToBegin();
            m_pFile->SetLength(0);
            m_pFile->Write(m_pBuffer, m_dwBufferLength);
            
            m_pCurrentAdapter->m_dwNumberOfRules += 1;
            m_pCurrentAdapter->m_dwVirtualAddress = dwPrevBufferLength;
            
            // Refresh CNetWallView
            CNetWallView * pView = (CNetWallView *)m_wndSplitter.GetPane(0, 0);
            ASSERT(pView);
            ASSERT_KINDOF(CNetWallView, pView);
            
            pView->Refresh(RULE, m_pCurrentAdapter);
        }
        
        //
        // 第三种情况，当前适配器以前设置过规则, 即存在该适配器的SECTION_HEADER, 并且有规则:
        //  . 需要修改规则文件的头部
        //  . 需要修改这个适配器SECTION的头部,
        //  . 并将这个规则添加到以前规则的后面
        //
        else
        {
            time_t  ltime;
            time(&ltime);
            
            RULE_FILE_HEADER * pFileHead  = (PRULE_FILE_HEADER)m_pBuffer;
            
            // Modify Rule File Header
            pFileHead->TimeDateStamp    =  ltime;
            pFileHead->dwTotal          += pItem->cbSize;
            
            m_dwBufferLength = pFileHead->dwTotal;
            
            dwHeaderSectionSize = RULE_FILE_HEADER_SIZE + pFileHead->NumberOfSections * RULE_SECTION_HEADER_SIZE;
            dwPreviousRuleSize  = 0;

            // Modify Adapter's Section Header and Save its VitualAddress/VitualSize
            PRULE_SECTION_HEADER pSecHdr = (PRULE_SECTION_HEADER)(m_pBuffer + RULE_FILE_HEADER_SIZE);
            
            DWORD dwPreVirtualAddress = 0;
            DWORD dwPreVirtualSize = 0;

            for (UINT i = 0; i < (UINT)pFileHead->NumberOfSections; i++) 
            {
                dwPreviousRuleSize += pSecHdr->VirtualSize;
                
                if (! _tcscmp((LPCTSTR)pSecHdr->Name, m_pCurrentAdapter->m_strLowerAdapterName))
                {     
                    dwPreVirtualAddress = pSecHdr->VirtualAddress;
                    dwPreVirtualSize    = pSecHdr->VirtualSize;

                    pSecHdr->NumberOfRules++;
                    pSecHdr->TimeDateStamp = ltime;                    
                    pSecHdr->VirtualSize  += pItem->cbSize;                                                          
                }                
                
                pSecHdr++;
            }
            
            // ReAllocate Memory 
            LPBYTE m_pTmpBuffer = NULL;
            m_pTmpBuffer = new BYTE[m_dwBufferLength];
            ASSERT(m_pTmpBuffer);
            RtlZeroMemory(m_pTmpBuffer, m_dwBufferLength);
            
            // Copy Rule File Header and Previous Section header
            DWORD dwOffset = RULE_FILE_HEADER_SIZE + pFileHead->NumberOfSections * RULE_SECTION_HEADER_SIZE;
            RtlMoveMemory(m_pTmpBuffer, m_pBuffer, dwOffset);
            
            // 1. 第一种情况，只有一个SECTION，只需将这个规则添加在后面就可以了。
            if (1 == pFileHead->NumberOfSections)
            {
                // Copy Previous Rule Items 
                RtlMoveMemory(m_pTmpBuffer + dwOffset, m_pBuffer + dwOffset, dwPreVirtualSize);

                // Copy This Rule Item to Rear of Previous Rule Items
                dwOffset += dwPreVirtualSize;
                RtlMoveMemory(m_pTmpBuffer + dwOffset,
                              pItem,
                              pItem->cbSize
                             );
            }

            // 2. 第二种情况，有多个SECTION，要修改其它每个SECTION的头部，将其规则添加到
            //    SECTION头部的后面，并将这个SECTION的规则添加到其它SECTION的后面，最后，
            //    添加这个规则。
            //    
            else
            {                
                // 先修改其它SECTION的头部并添加其它SECTION的规则
                PRULE_SECTION_HEADER pOthSecHdr = (PRULE_SECTION_HEADER)(m_pTmpBuffer + RULE_FILE_HEADER_SIZE);
                PRULE_SECTION_HEADER pPreSecHdr = (PRULE_SECTION_HEADER)(m_pBuffer + RULE_FILE_HEADER_SIZE);
                
                for (UINT i = 0; i < (UINT)pFileHead->NumberOfSections; i++) 
                {                    
                    if (_tcscmp((LPCTSTR)pOthSecHdr->Name, m_pCurrentAdapter->m_strLowerAdapterName))
                    {
                        pOthSecHdr->VirtualAddress = dwOffset;    
                        RtlMoveMemory(m_pTmpBuffer + dwOffset, 
                                      m_pBuffer + pPreSecHdr->VirtualAddress,
                                      pPreSecHdr->VirtualSize
                                     );
                        dwOffset += pOthSecHdr->VirtualSize;
                    }
                    else
                    {
                        pSecHdr = pOthSecHdr;
                    }
                    
                    pOthSecHdr++;
                    pPreSecHdr++;
                }

                // 修改这个SECTION的头部并添加这个SECTIon以前的规则，并添加新加的规则
                pSecHdr->VirtualAddress = dwOffset;
                RtlMoveMemory(m_pTmpBuffer + dwOffset, 
                              m_pBuffer + dwPreVirtualAddress,
                              dwPreVirtualSize
                             );

                dwOffset += dwPreVirtualSize;
                RtlMoveMemory(m_pTmpBuffer + dwOffset,
                              pItem,
                              pItem->cbSize
                             );
            }
            
            // Set Buffer 
            delete []m_pBuffer;
            m_pBuffer = NULL;
            
            m_pBuffer = m_pTmpBuffer;                
            
            // Write Buffer into File
            m_pFile->SeekToBegin();
            m_pFile->SetLength(0);
            m_pFile->Write(m_pBuffer, m_dwBufferLength);
            
            m_pCurrentAdapter->m_dwNumberOfRules += 1;
            m_pCurrentAdapter->m_dwVirtualAddress = dwHeaderSectionSize + dwPreviousRuleSize;
            
            // Refresh CNetWallView
            CNetWallView * pView = (CNetWallView *)m_wndSplitter.GetPane(0, 0);
            ASSERT(pView);
            ASSERT_KINDOF(CNetWallView, pView);
            
            pView->Refresh(RULE, m_pCurrentAdapter);
        }
        
        delete []dlg.m_pBuffer;
    }
    catch (...)
    {
        delete []dlg.m_pBuffer;
        throw;
    }    

    m_pCurrentAdapter->m_bRuleSetedNoFresh = TRUE;
}

void CMainFrame::OnUpdateRuleDelete(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(NULL != m_pCurrentAdapter && m_nIndex != -1);
}

void CMainFrame::OnRuleDelete() 
{	
    ASSERT(m_nIndex > -1);

    ASSERT(m_pCurrentAdapter && m_pCurrentAdapter->m_dwVirtualAddress > 0 && m_pCurrentAdapter->m_dwNumberOfRules > 0);

    try
    {        
        //
        // 首先找到要删除的规则的适配器 SECTION_HEADER
        //
        RULE_SECTION_HEADER * pCurSecHdr = NULL;
        RULE_ITEM           * pCurItem   = NULL;
        
        RULE_FILE_HEADER * pFileHead = (PRULE_FILE_HEADER)m_pBuffer;
        
        pCurSecHdr = (PRULE_SECTION_HEADER)(m_pBuffer + RULE_FILE_HEADER_SIZE);
        
        for (UINT i = 0; i < (UINT)pFileHead->NumberOfSections; i++) 
        {
            if (! _tcscmp((LPCTSTR)pCurSecHdr->Name, (LPCTSTR)m_pCurrentAdapter->m_strLowerAdapterName))
            {
                pCurItem   = (PRULE_ITEM)(m_pBuffer + pCurSecHdr->VirtualAddress);                
                break;
            }
            
            pCurSecHdr++;
        }

        ASSERT(pCurSecHdr);

        //
        // 找到要删除的 RULE_ITEM
        //
        for (i = 0; i < pCurSecHdr->NumberOfRules; i++)
        {
            if (i == m_nIndex)
            {
                break;
            }
            pCurItem = (RULE_ITEM *)((LPBYTE)pCurItem + pCurItem->cbSize);
        }

        ASSERT(pCurItem);

        // Modify Rule File Header
        time_t  ltime;
        time(&ltime);

        pFileHead->TimeDateStamp   =  ltime;
        pFileHead->dwTotal        -= pCurItem->cbSize;

        m_dwBufferLength           = pFileHead->dwTotal;   
    
        // ReAllocate Memory 
        LPBYTE m_pTmpBuffer = NULL;
        m_pTmpBuffer = new BYTE[m_dwBufferLength];
        ASSERT(m_pTmpBuffer);
        RtlZeroMemory(m_pTmpBuffer, m_dwBufferLength);

        // Copy Rule File Header and Previous Section header
        DWORD dwOffset = RULE_FILE_HEADER_SIZE + pFileHead->NumberOfSections * RULE_SECTION_HEADER_SIZE;
        RtlMoveMemory(m_pTmpBuffer, m_pBuffer, dwOffset);
        
        //
        //  要修改其它每个SECTION的头部，将其规则添加到 SECTION 头部的后面，
        //  并将这个SECTION的规则添加到其它SECTION的后面，当然，删除这个规则。
        // 

        // 先修改其它SECTION的头部并添加其它SECTION的规则
        RULE_SECTION_HEADER * pTheSecHdr = NULL;
        RULE_SECTION_HEADER * pOthSecHdr = (PRULE_SECTION_HEADER)(m_pTmpBuffer + RULE_FILE_HEADER_SIZE);
        RULE_SECTION_HEADER * pPreSecHdr = (PRULE_SECTION_HEADER)(m_pBuffer + RULE_FILE_HEADER_SIZE);
        
        for (i = 0; i < (UINT)pFileHead->NumberOfSections; i++) 
        {                    
            if (_tcscmp((LPCTSTR)pOthSecHdr->Name, m_pCurrentAdapter->m_strLowerAdapterName))
            {
                pOthSecHdr->VirtualAddress = dwOffset;    
                RtlMoveMemory(m_pTmpBuffer + dwOffset, 
                              m_pBuffer + pPreSecHdr->VirtualAddress,
                              pPreSecHdr->VirtualSize
                             );
                dwOffset += pOthSecHdr->VirtualSize;
            }
            else
            {
                pTheSecHdr = pOthSecHdr;
            }
            
            pOthSecHdr++;
            pPreSecHdr++;
        }
        
        ASSERT(pTheSecHdr);
        
        // 修改这个 SECTION_HEADER
        pTheSecHdr->NumberOfRules--;
        pTheSecHdr->VirtualAddress = dwOffset;
        pTheSecHdr->TimeDateStamp  = ltime;
        pTheSecHdr->VirtualSize   -= pCurItem->cbSize;
        
        // 如果还有别的规则，则添加到最后
        if (0 < pTheSecHdr->NumberOfRules)
        {            
            //
            RULE_ITEM * pTmpItem = (PRULE_ITEM)(m_pBuffer + pCurSecHdr->VirtualAddress);
            
            // Copy All Rule Items Except This Item
            //DWORD dwPreOffset = dwOffset;
            for (UINT i = 0; i < pCurSecHdr->NumberOfRules; i++)
            {                
                if (i != m_nIndex)
                {                    
                    RtlMoveMemory(m_pTmpBuffer + dwOffset, (LPBYTE)pTmpItem/*m_pBuffer + dwPreOffset*/, pTmpItem->cbSize);                    
                    dwOffset += pTmpItem->cbSize;
                }
                //dwPreOffset += pItem->cbSize;
                pTmpItem = (RULE_ITEM *)((LPBYTE)pTmpItem + pTmpItem->cbSize);
            }
        }
                
        // Set Buffer 
        delete []m_pBuffer;
        m_pBuffer = NULL;
        
        m_pBuffer = m_pTmpBuffer;                
        
        // Write Buffer into File
        m_pFile->SeekToBegin();
        m_pFile->SetLength(0);
        m_pFile->Write(m_pBuffer, m_dwBufferLength);
        
        m_pCurrentAdapter->m_dwNumberOfRules -= 1;
        m_pCurrentAdapter->m_dwVirtualAddress = pTheSecHdr->VirtualAddress;
        
        // Refresh CNetWallView
        CNetWallView * pView = (CNetWallView *)m_wndSplitter.GetPane(0, 0);
        ASSERT(pView);
        ASSERT_KINDOF(CNetWallView, pView);
        
        pView->Refresh(RULE, m_pCurrentAdapter);
    }
    catch(...)
    {
        throw;
    }

    m_pCurrentAdapter->m_bRuleSetedNoFresh = TRUE;
}

void CMainFrame::OnUpdateRuleDeleteall(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(NULL != m_pCurrentAdapter 
        && 0 < m_pCurrentAdapter->m_dwNumberOfRules);
}

void CMainFrame::OnRuleDeleteall() 
{
	ASSERT(m_pCurrentAdapter && m_pCurrentAdapter->m_dwVirtualAddress > 0 && m_pCurrentAdapter->m_dwNumberOfRules > 0);

    try
    {                
        //
        // 首先找到要删除的规则的适配器 SECTION_HEADER
        //        
        RULE_FILE_HEADER * pFileHead = (PRULE_FILE_HEADER)m_pBuffer;
        
        RULE_SECTION_HEADER * pCurSecHdr = (PRULE_SECTION_HEADER)(m_pBuffer + RULE_FILE_HEADER_SIZE);
        
        for (UINT i = 0; i < (UINT)pFileHead->NumberOfSections; i++) 
        {
            if (! _tcscmp((LPCTSTR)pCurSecHdr->Name, (LPCTSTR)m_pCurrentAdapter->m_strLowerAdapterName))
            {
                break;
            }
            
            pCurSecHdr++;
        }

        ASSERT(pCurSecHdr);

        // Modify Rule File Header
        time_t  ltime;
        time(&ltime);

        pFileHead->TimeDateStamp   = ltime;
        pFileHead->dwTotal        -= pCurSecHdr->VirtualSize;

        m_dwBufferLength           = pFileHead->dwTotal;   
    
        // ReAllocate Memory 
        LPBYTE m_pTmpBuffer = NULL;
        m_pTmpBuffer = new BYTE[m_dwBufferLength];
        ASSERT(m_pTmpBuffer);
        RtlZeroMemory(m_pTmpBuffer, m_dwBufferLength);

        // Copy Rule File Header and Previous Section header
        DWORD dwOffset = RULE_FILE_HEADER_SIZE + pFileHead->NumberOfSections * RULE_SECTION_HEADER_SIZE;
        RtlMoveMemory(m_pTmpBuffer, m_pBuffer, dwOffset);
        
        //
        //  要修改其它每个SECTION的头部，将其规则添加到 SECTION 头部的后面，
        //  

        // 先修改其它SECTION的头部并添加其它SECTION的规则
        RULE_SECTION_HEADER * pTheSecHdr = NULL;
        RULE_SECTION_HEADER * pOthSecHdr = (PRULE_SECTION_HEADER)(m_pTmpBuffer + RULE_FILE_HEADER_SIZE);
        RULE_SECTION_HEADER * pPreSecHdr = (PRULE_SECTION_HEADER)(m_pBuffer + RULE_FILE_HEADER_SIZE);
        
        for (i = 0; i < (UINT)pFileHead->NumberOfSections; i++) 
        {                    
            if (_tcscmp((LPCTSTR)pOthSecHdr->Name, m_pCurrentAdapter->m_strLowerAdapterName))
            {
                pOthSecHdr->VirtualAddress = dwOffset;    
                RtlMoveMemory(m_pTmpBuffer + dwOffset, 
                              m_pBuffer + pPreSecHdr->VirtualAddress,
                              pPreSecHdr->VirtualSize
                             );
                dwOffset += pOthSecHdr->VirtualSize;
            }
            else
            {
                pTheSecHdr = pOthSecHdr;
            }
            
            pOthSecHdr++;
            pPreSecHdr++;
        }
        
        ASSERT(pTheSecHdr);
        
        // 修改这个 SECTION_HEADER
        pTheSecHdr->TimeDateStamp  = ltime;
        pTheSecHdr->NumberOfRules  = 0;
        pTheSecHdr->VirtualSize    = 0;
        pTheSecHdr->VirtualAddress = dwOffset;                
                        
        // Set Buffer 
        delete []m_pBuffer;
        m_pBuffer = NULL;
        
        m_pBuffer = m_pTmpBuffer;                
        
        // Write Buffer into File
        m_pFile->SeekToBegin();
        m_pFile->SetLength(0);
        m_pFile->Write(m_pBuffer, m_dwBufferLength);
        
        m_pCurrentAdapter->m_dwNumberOfRules  = 0;
        m_pCurrentAdapter->m_dwVirtualAddress = pTheSecHdr->VirtualAddress;
        
        // Refresh CNetWallView
        CNetWallView * pView = (CNetWallView *)m_wndSplitter.GetPane(0, 0);
        ASSERT(pView);
        ASSERT_KINDOF(CNetWallView, pView);
        
        pView->Refresh(RULE, m_pCurrentAdapter);
    }
    catch(...)
    {
        throw;
    }

    m_pCurrentAdapter->m_bRuleSetedNoFresh = TRUE;
}

void CMainFrame::OnUpdateRuleModify(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(NULL != m_pCurrentAdapter && m_nIndex != -1);
}

void CMainFrame::OnRuleModify() 
{
	ASSERT(m_nIndex > -1);

    ASSERT(m_pCurrentAdapter && m_pCurrentAdapter->m_dwVirtualAddress > 0 && m_pCurrentAdapter->m_dwNumberOfRules > 0);    

    //
    // 首先找到要修改规则的适配器 SECTION_HEADER
    //
    RULE_SECTION_HEADER * pCurSecHdr = NULL;
    RULE_ITEM           * pCurItem   = NULL;
    
    RULE_FILE_HEADER * pFileHead = (PRULE_FILE_HEADER)m_pBuffer;
    
    pCurSecHdr = (PRULE_SECTION_HEADER)(m_pBuffer + RULE_FILE_HEADER_SIZE);
    
    for (UINT i = 0; i < (UINT)pFileHead->NumberOfSections; i++) 
    {
        if (! _tcscmp((LPCTSTR)pCurSecHdr->Name, (LPCTSTR)m_pCurrentAdapter->m_strLowerAdapterName))
        {
            pCurItem   = (PRULE_ITEM)(m_pBuffer + pCurSecHdr->VirtualAddress);                
            break;
        }
        
        pCurSecHdr++;
    }
    
    ASSERT(pCurSecHdr);
    
    //
    // 找到要修改的 RULE_ITEM
    //
    for (i = 0; i < pCurSecHdr->NumberOfRules; i++)
    {
        if (i == m_nIndex)
        {
            break;
        }
        pCurItem = (RULE_ITEM *)((LPBYTE)pCurItem + pCurItem->cbSize);
    }
    
    ASSERT(pCurItem);
    
    CAddRuleDlg  dlg;
    dlg.m_nType = MODIFY_RULE;
    dlg.m_pItem = pCurItem;
    
    if (IDOK != dlg.DoModal() || NULL == dlg.m_pBuffer)
    {
        return;
    }
    
    try
    {                      
        RULE_ITEM * pItem = (PRULE_ITEM)dlg.m_pBuffer;
        time_t  ltime;
        time(&ltime);
        
        // 如果修改后的规则大小跟以前的相同，即没有更改描述，则只需
        // 修改这个SECTION_HEADER的时间戳，并更新这个规则项即可。
        if (pCurItem->cbSize == pItem->cbSize)
        {
            pCurSecHdr->TimeDateStamp = ltime;
            RtlZeroMemory(pCurItem, pCurItem->cbSize);
            RtlMoveMemory(pCurItem, pItem, pItem->cbSize);            
        }
        
        // 否则，要先删除以前的规则，再添加这个规则。
        else
        {
            // Modify Rule File Header
            pFileHead->TimeDateStamp = ltime;
            pFileHead->dwTotal       = pFileHead->dwTotal - pCurItem->cbSize + pItem->cbSize;
            
            m_dwBufferLength         = pFileHead->dwTotal;   
            
            // ReAllocate Memory 
            LPBYTE m_pTmpBuffer = NULL;
            m_pTmpBuffer = new BYTE[m_dwBufferLength];
            ASSERT(m_pTmpBuffer);
            RtlZeroMemory(m_pTmpBuffer, m_dwBufferLength);
            
            // Copy Rule File Header and Previous Section header
            DWORD dwOffset = RULE_FILE_HEADER_SIZE + pFileHead->NumberOfSections * RULE_SECTION_HEADER_SIZE;
            RtlMoveMemory(m_pTmpBuffer, m_pBuffer, dwOffset);

            //
            //  要修改其它每个SECTION的头部，将其规则添加到 SECTION 头部的后面，
            //  并将这个SECTION的规则添加到其它SECTION的后面，当然，删除这个规则。
            // 
            
            // 先修改其它SECTION的头部并添加其它SECTION的规则
            RULE_SECTION_HEADER * pTheSecHdr = NULL;
            RULE_SECTION_HEADER * pOthSecHdr = (PRULE_SECTION_HEADER)(m_pTmpBuffer + RULE_FILE_HEADER_SIZE);
            RULE_SECTION_HEADER * pPreSecHdr = (PRULE_SECTION_HEADER)(m_pBuffer + RULE_FILE_HEADER_SIZE);
            
            for (i = 0; i < (UINT)pFileHead->NumberOfSections; i++) 
            {                    
                if (_tcscmp((LPCTSTR)pOthSecHdr->Name, m_pCurrentAdapter->m_strLowerAdapterName))
                {
                    pOthSecHdr->VirtualAddress = dwOffset;    
                    RtlMoveMemory(m_pTmpBuffer + dwOffset, 
                                  m_pBuffer + pPreSecHdr->VirtualAddress,
                                  pPreSecHdr->VirtualSize
                                 );
                    dwOffset += pOthSecHdr->VirtualSize;
                }
                else
                {
                    pTheSecHdr = pOthSecHdr;
                }
                
                pOthSecHdr++;
                pPreSecHdr++;
            }
            
            ASSERT(pTheSecHdr);
            
            // 修改这个 SECTION_HEADER
            pTheSecHdr->VirtualAddress = dwOffset;
            pTheSecHdr->TimeDateStamp  = ltime;
            pTheSecHdr->VirtualSize    = pTheSecHdr->VirtualSize - pCurItem->cbSize + pItem->cbSize;
            
            // 如果还有别的规则，则添加到最后
            if (1 < pTheSecHdr->NumberOfRules)
            {            
                //
                RULE_ITEM * pTmpItem = (PRULE_ITEM)(m_pBuffer + pCurSecHdr->VirtualAddress);
                
                // Copy All Rule Items Except This Item
                for (UINT i = 0; i < pCurSecHdr->NumberOfRules; i++)
                {                
                    if (i != m_nIndex)
                    {                    
                        RtlMoveMemory(m_pTmpBuffer + dwOffset, (LPBYTE)pTmpItem, pTmpItem->cbSize);                    
                        dwOffset += pTmpItem->cbSize;
                    }
                    pTmpItem = (RULE_ITEM *)((LPBYTE)pTmpItem + pTmpItem->cbSize);
                }
            }

            // 最后添加修改的规则
            RtlMoveMemory(m_pTmpBuffer + dwOffset, pItem, pItem->cbSize);

            // Set Buffer 
            delete []m_pBuffer;
            m_pBuffer = NULL;
            
            m_pBuffer = m_pTmpBuffer;           
                        
            m_pCurrentAdapter->m_dwVirtualAddress = pTheSecHdr->VirtualAddress;            
        }

        delete []dlg.m_pBuffer;
                                  
        // Refresh Rule File
        m_pFile->SeekToBegin();
        m_pFile->SetLength(0);
        m_pFile->Write(m_pBuffer, m_dwBufferLength);
        
        // Refresh CNetWallView and CRuleListView
        CNetWallView * pView = (CNetWallView *)m_wndSplitter.GetPane(0, 0);
        ASSERT(pView);
        ASSERT_KINDOF(CNetWallView, pView);
        
        pView->Refresh(RULE, m_pCurrentAdapter);
    }
    catch(...)
    {
        delete []dlg.m_pBuffer;
        throw;
    }

    m_pCurrentAdapter->m_bRuleSetedNoFresh = TRUE;
}

void CMainFrame::OnUpdateAdapterSetrule(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(NULL != m_pCurrentAdapter 
        && INVALID_HANDLE_VALUE != m_pCurrentAdapter->m_hAdapter);
}

void CMainFrame::OnAdapterSetrule() 
{
	ASSERT(INVALID_HANDLE_VALUE != m_pCurrentAdapter->m_hAdapter);

    if (m_pCurrentAdapter->m_dwNumberOfRules == 0)
    {
        AfxMessageBox(_T("当前适配器没有设置规则!"));
        return;
    }

    DWORD  dwRuleSize = 0;

    PRULE_FILE_HEADER pFileHead = (PRULE_FILE_HEADER)m_pBuffer;
    
    if (0 < pFileHead->NumberOfSections)
    {    
        PRULE_SECTION_HEADER pSecHdr = (PRULE_SECTION_HEADER)(m_pBuffer + RULE_FILE_HEADER_SIZE);
        
        for (UINT i = 0; i < (UINT)pFileHead->NumberOfSections; i++) 
        {
            if (! _tcscmp((LPCTSTR)pSecHdr->Name, m_pCurrentAdapter->m_strLowerAdapterName))
            {
                if (0 < pSecHdr->NumberOfRules)
                {
                    dwRuleSize = pSecHdr->VirtualSize;                    
                    break;                    
                }
            }
            
            pSecHdr++;
        }
    }
    
    ASSERT(0 != dwRuleSize);

    m_pCurrentAdapter->m_bRuleSeted = 
        ((CNetWallApp *)AfxGetApp())->GetAPI()->SetRule(m_pCurrentAdapter->m_hAdapter,
                           (LPVOID)(m_pBuffer + m_pCurrentAdapter->m_dwVirtualAddress),
                           dwRuleSize
                          );    

    if (! m_pCurrentAdapter->m_bRuleSeted)
    {
        AfxMessageBox(_T("设置规则失败!"));
        return;
    }    

    UpdateStatusBar(RULE);
}

void CMainFrame::OnUpdateAdapterClearrule(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(NULL != m_pCurrentAdapter 
        && INVALID_HANDLE_VALUE != m_pCurrentAdapter->m_hAdapter);
}

void CMainFrame::OnAdapterClearrule() 
{
    ASSERT(INVALID_HANDLE_VALUE != m_pCurrentAdapter->m_hAdapter);
    
    if (m_pCurrentAdapter->m_dwNumberOfRules == 0)
    {
        AfxMessageBox(_T("当前适配器没有设置规则!"));
        return;
    }
        
    m_pCurrentAdapter->m_bRuleSeted = ((CNetWallApp *)AfxGetApp())->GetAPI()->ClearRule(m_pCurrentAdapter->m_hAdapter);
    
    if (! m_pCurrentAdapter->m_bRuleSeted)
    {
        AfxMessageBox(_T("清除规则失败!"));
        return;
    }    
    
    m_pCurrentAdapter->m_bRuleSeted = FALSE;

    UpdateStatusBar(RULE);
}

void CMainFrame::OnUpdateLogStart(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(! m_bLogStarted);
}

void CMainFrame::OnLogStart() 
{
    //
    // Open The NetWall WDM Device Handle
    //
    HANDLE hIMHandle = INVALID_HANDLE_VALUE;
    hIMHandle = CreateFile(NETWALL_WDM_DEVICE_FILENAME,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL
                          );
    
    if (INVALID_HANDLE_VALUE == hIMHandle)
    {
        AfxMessageBox(_T("Driver is not loaded. Try reloading the app."));
        return;
    }

    //
    // Send Message to NetWall WDM Driver to initialize LogPrint
    //
    m_bLogStarted = ((CNetWallApp *)AfxGetApp())->GetAPI()->StartLogPrint(hIMHandle);
    
    if (! m_bLogStarted)
    {
        AfxMessageBox(_T("打开日志服务失败!"));
    }    

    CloseHandle(hIMHandle);
}

void CMainFrame::OnUpdateLogStop(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(m_bLogStarted);
}

void CMainFrame::OnLogStop() 
{	    
    //
    // Open The NetWall WDM Device Handle
    //
    HANDLE hIMHandle = INVALID_HANDLE_VALUE;
    hIMHandle = CreateFile(NETWALL_WDM_DEVICE_FILENAME,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL
                          );
    
    if (INVALID_HANDLE_VALUE == hIMHandle)
    {
        AfxMessageBox(_T("Driver is not loaded. Try reloading the app."));
        return;
    }
    
    //
    // Send Message to NetWall WDM Driver to close LogPrint
    //
    BOOL bRet = ((CNetWallApp *)AfxGetApp())->GetAPI()->CloseLogPrint(hIMHandle);
    
    if (! bRet)
    {
        AfxMessageBox(_T("关闭日志服务失败!"));
    }    
    
    CloseHandle(hIMHandle);

    m_bLogStarted = FALSE;        
}

BOOL CMainFrame::LoadLogFile(BOOL bReLoad)
{
    static const TCHAR * LOG_FILE = _T("NetWall.log");
    TCHAR  szLogFile[MAX_PATH];
    
    // 1. 首先检查是否已经装载了，如果已经装载并且不重新装载，直接返回，
    //    否则释放以前的日志资源，并进行装载。
    if (m_bLogLoaded)
    {
        if (! bReLoad)
        {
            return m_bLogLoaded;
        }
        else
        {
            // Unmap Log memory from the process's address space.
            if (NULL != m_lpMapAddr)
            {
                UnmapViewOfFile(m_lpMapAddr);
                m_lpMapAddr = NULL;
            }
            
            // Close the Log File's handle to the file-mapping object.    
            if (INVALID_HANDLE_VALUE != m_hMapFile) 
            { 
                CloseHandle(m_hMapFile);   
                m_hMapFile = INVALID_HANDLE_VALUE;
            } 
            
            if (INVALID_HANDLE_VALUE != m_hLogFile)
            {
                CloseHandle(m_hLogFile);            
                m_hLogFile = INVALID_HANDLE_VALUE;
            }
            
            m_bLogLoaded = FALSE;
        }
    }   

    // 1. 获取 Windows 目录
    if (0 == GetWindowsDirectory(szLogFile, MAX_PATH)) // C:\WINNT
    {
        AfxMessageBox(_T("Could not find Windows directory."));        
        return FALSE;
    }
    
    // 2. 检测是否存在日志文件
    _stprintf(szLogFile + _tcslen(szLogFile), _T("\\%s"), LOG_FILE);
    
    WIN32_FIND_DATA  findData;
    HANDLE findHandle = FindFirstFile(szLogFile, &findData);
    if (INVALID_HANDLE_VALUE == findHandle) 
    {                    
        TCHAR   * pFile;
        
        if (! SearchPath(NULL, LOG_FILE, NULL, sizeof(szLogFile), szLogFile, &pFile))
        {                        
            CString strTmp;
            strTmp.Format(_T("日志文件 %s 没有找到."), LOG_FILE);
            AfxMessageBox(strTmp);
            return FALSE;
        }                    
    } 
    else 
    {
        FindClose(findHandle);
    }
    
    // 3. 装载日志文件到系统内存
    TCHAR lpszError[MAX_PATH];
    RtlZeroMemory(lpszError, MAX_PATH);
    m_bLogLoaded = ((CNetWallApp *)AfxGetApp())->GetAPI()->LoadLogFile((LPCTSTR)szLogFile,
                                                                    &m_lpMapAddr,
                                                                    &m_dwLogFileSize,
                                                                    &m_hLogFile,
                                                                    &m_hMapFile,
                                                                    lpszError
                                                                   );
    
    if (! m_bLogLoaded)
    {
        AfxMessageBox((LPCTSTR)lpszError);        
    }    

    return m_bLogLoaded;
}

void CMainFrame::OnUpdateLogLoad(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(! m_bLogLoaded || m_bLoadCompleted);
}

void CMainFrame::OnLogLoad() 
{
    CNetWallView * pView = (CNetWallView *)m_wndSplitter.GetPane(0, 0);
    pView->Refresh(LOG, m_pCurrentAdapter);
}

LRESULT CMainFrame::OnLogThreadStatus(WPARAM wParam, LPARAM lParam) 
{
    EnterCriticalSection(&m_CritSect);
 
    ThreadParams * lpThreadParams = (PThreadParams)lParam;

    UINT nIndex = (UINT)wParam;
    CString  string = _T("");

    switch (nIndex)
    {
    case 0 :
        string = "正在装载日志...";            
        break;

    case 1:
        string.Format(_T("共有日志数: %u 项"), lpThreadParams->m_dwLogCount);        
        break;

    case 2:
        string.Format(_T("目前打开数: %u 项"), lpThreadParams->m_dwCurItem);        
        break;

    case 3:
        {
            if (lpThreadParams->m_bPaused && ! lpThreadParams->m_bInterrupt)
            {
                m_bLoadCompleted = FALSE;
                string = _T("当前状态: 暂停");
            }
            else if (lpThreadParams->m_bInterrupt)
            {
                m_bLoadCompleted = TRUE;
                string = _T("当前状态: 中断");
            }
            else if (lpThreadParams->m_bDone)
            {
                m_bLoadCompleted = TRUE;
                string = _T("当前状态: 完成");   
            }
            else
            {
                m_bLoadCompleted = FALSE;
                string = _T("当前状态: 正在装载");      
            }
        }        
        break;

    default:
        string = _T("Ready");
    }

    m_wndStatusBar.SetPaneText(nIndex, (LPCTSTR)string, TRUE);
        
    LeaveCriticalSection(&m_CritSect);
    
    return 0;
}

BOOL CMainFrame::UpdateStatusBar(NETWALL_LIST_TYPE nType) 
{
    switch (nType)
    {    
    case RULE:
        {        
            CString  string = "就绪";
            m_wndStatusBar.SetPaneText(0, (LPCTSTR) string, TRUE);
           
            string.Format(_T("适配器状态: %s"), (m_pCurrentAdapter->m_hAdapter != INVALID_HANDLE_VALUE) ? _T("打开") : _T("关闭"));
            m_wndStatusBar.SetPaneText(1, (LPCTSTR) string, TRUE);    
            
            string.Format(_T("适配器规则数 : %d"), m_pCurrentAdapter->m_dwNumberOfRules);
            m_wndStatusBar.SetPaneText(2, (LPCTSTR) string, TRUE);
            
            // 规则是否指定给适配器
            if (m_pCurrentAdapter->m_bRuleSeted)
            {
                if (m_pCurrentAdapter->m_bRuleSetedNoFresh)
                {
                    string = _T("规则应用状态: 设置未更新");
                }
                else
                {
                    string = _T("规则应用状态: 设置");
                }
            }
            else
            {
                string = _T("规则应用状态: 未设置");
            }
                        
            m_wndStatusBar.SetPaneText(3, (LPCTSTR) string, TRUE);
        }
        break;

    case ADAPTER:
    case RULEITEM:    
    default:
        {        
            CString  string = "就绪";
            m_wndStatusBar.SetPaneText(0, (LPCTSTR) string, TRUE);
            
            string.Format(_T("适配器状态: %s"), (m_pCurrentAdapter->m_hAdapter != INVALID_HANDLE_VALUE) ? _T("打开") : _T("关闭"));
            m_wndStatusBar.SetPaneText(1, (LPCTSTR) string, TRUE);  
            
            string.Format(_T("适配器数 : %d"), m_nAdapterCount);
            m_wndStatusBar.SetPaneText(2, (LPCTSTR) string, TRUE);
            
            string.Format(_T("打开适配器数 : %d"), m_nOpenAdapterCount);
            m_wndStatusBar.SetPaneText(3, (LPCTSTR) string, TRUE);
        }
        break;
    }

    return TRUE;
}