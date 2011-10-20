// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__CF1E0650_F141_4D09_BEBC_2BBEFD544989__INCLUDED_)
#define AFX_MAINFRM_H__CF1E0650_F141_4D09_BEBC_2BBEFD544989__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <list>

using namespace std;

#define	ETHER_ADDR_LENGTH		6

class CAdapterInfo : public CObject
{
    DECLARE_DYNAMIC(CAdapterInfo)
        
    // Construction
public:
    CAdapterInfo();
    ~CAdapterInfo();
    
public:    
    HANDLE      m_hAdapter;
    BOOL        m_bRuleSeted;       // 是否将规则指定给了适配器
    BOOL        m_bRuleSetedNoFresh;// 规则是否修改过但没有指定给适配器
    //
    // VirtualAddress  NumberOfRules     SECTION_HEADER   RULE_ITEM
    //    = 0            = 0        -->        NO            NO
    //    > 0            = 0        -->        YES           NO
    //    > 0            > 0        -->        YES           YES
    //
    DWORD       m_dwNumberOfRules;
    DWORD       m_dwVirtualAddress; 

    CString     m_strAdapterMacAddress;
    CString     m_strVirtualAdapterName;
    CString     m_strLowerAdapterName;
    CString     m_strFriendlyAdapterName;    
};


typedef list<CAdapterInfo *>            CAdapterList;
typedef list<CAdapterInfo *>::iterator  CAdapterListIterator;

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:
    BOOL            m_bSplitterCreated;     // Create Splitter Success ot not
    UINT            m_nOpenAdapterCount;    // Count of opened adapter
	UINT            m_nAdapterCount;        // Count of all binded adapter
	
    CString         m_strVirtualAdapterName; // Current Opened Adapter's name
    CString         m_strLowerAdapterName;   // Current Opened Adapter's name
    CString         m_strFriendlyAdapterName;// Current Opened Adapter's name
	CAdapterInfo *  m_pCurrentAdapter;       // Current Selected Adapter

    CFile        *  m_pFile;                // Rule File
    TCHAR           m_pFileName[MAX_PATH];  // Rule File name
    LPBYTE          m_pBuffer;              // Buffer of Rule File Data
    DWORD           m_dwBufferLength;       // Length of Buffer

    CAdapterList    m_AdapterList;          // All Binding Adapter List

    INT             m_nIndex;

    BOOL            m_bLogStarted;          // NetWall IM LogPrint Started or not
    BOOL            m_bLoadCompleted;       // Display completed in the CLogListView
    BOOL            m_bLogLoaded;           // Log File loaded into memory or not
    HANDLE          m_hLogFile;             // Handle of Log File
    HANDLE          m_hMapFile;             // Handle of Log Map File
    LPVOID          m_lpMapAddr;            // Start Address of Log File
    DWORD           m_dwLogFileSize;        // Size of Log File

// Operations
public:
    BOOL UpdateStatusBar(NETWALL_LIST_TYPE nType = INIT);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:		
	BOOL LoadLogFile(BOOL bReLoad = FALSE);
	BOOL ReplaceView(CRuntimeClass * pViewClass);
	RULE_ITEM * GetAdapterRuleItem();
	BOOL HasRuleForAdapter(CAdapterInfo * pAdaterInfo);
	BOOL SetCurrentAdapter(CAdapterInfo * pAdapterinfo);
	CAdapterList * GetAdapters();
	DWORD GetAdapterList();

	virtual ~CMainFrame();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar      m_wndStatusBar;
	CToolBar        m_wndToolBar;
    CSplitterWnd    m_wndSplitter;

// Generated message map functions
protected:	
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnAdapterOpen();
	afx_msg void OnAdapterClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnRuleAdd();
	afx_msg void OnRuleDelete();
	afx_msg void OnUpdateRuleDelete(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAdapterOpen(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAdapterClose(CCmdUI* pCmdUI);
	afx_msg void OnRuleDeleteall();
	afx_msg void OnUpdateRuleDeleteall(CCmdUI* pCmdUI);
	afx_msg void OnRuleModify();
	afx_msg void OnUpdateRuleModify(CCmdUI* pCmdUI);
	afx_msg void OnAdapterSetrule();
	afx_msg void OnUpdateAdapterSetrule(CCmdUI* pCmdUI);
	afx_msg void OnAdapterClearrule();
	afx_msg void OnUpdateAdapterClearrule(CCmdUI* pCmdUI);
	afx_msg void OnLogStart();
	afx_msg void OnLogStop();
	afx_msg void OnUpdateLogStart(CCmdUI* pCmdUI);
	afx_msg void OnUpdateLogStop(CCmdUI* pCmdUI);
	afx_msg void OnLogLoad();
	afx_msg void OnUpdateLogLoad(CCmdUI* pCmdUI);
	//}}AFX_MSG

    LRESULT OnView(WPARAM wParam, LPARAM lParam);
    LRESULT OnLogThreadStatus(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
        
private:
    CRITICAL_SECTION  m_CritSect; 
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__CF1E0650_F141_4D09_BEBC_2BBEFD544989__INCLUDED_)
