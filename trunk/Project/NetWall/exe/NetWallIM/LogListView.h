#if !defined(AFX_LOGLISTVIEW_H__F3DE3ADF_65CE_41C2_B65D_B185ACC211B6__INCLUDED_)
#define AFX_LOGLISTVIEW_H__F3DE3ADF_65CE_41C2_B65D_B185ACC211B6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LogListView.h : header file
//

#include "log.h"

class CAdapterInfo;

#define WM_USER_ADD_ITEM	    	(WM_USER + 1)
#define WM_USER_LOGTHREAD_STATUS	(WM_USER + 2)


/////////////////////////////////////////////////////////////////////////////
// CLogListView view

typedef struct tagThreadParams
{
    HWND		m_hwndProgress;
    HWND		m_hwndView;
    LPVOID		m_lpLogBase;
    DWORD		m_dwLogCount;
    DWORD		m_dwCurItem;
    HANDLE      m_hWaitEvent;
    BOOL        m_bPaused;
    BOOL        m_bInterrupt;   
	BOOL		m_bDone;
    
} ThreadParams, *PThreadParams; 

class CLogListView : public CListView
{
protected:
    CLogListView();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CLogListView)

// Attributes
public:
    UINT    m_nType;
    BOOL    m_bLogLoaded;
    BOOL    m_bThreadStarted;
    DWORD   m_dwCurItem;
    DWORD   m_dwLogCount;
    
    static ThreadParams *m_pThreadParams;
    static UINT LogThreadFunction(LPVOID pParam);
	
// Operations
public:
    BOOL AddLogItem(LOG_ITEM * pItem, int nIndex, DWORD dwSerial, CListCtrl& listView);
    int RefreshLog(CAdapterInfo * pAdapterInfo);
	
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLogListView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CLogListView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
protected:
    //{{AFX_MSG(CLogListView)
	afx_msg void OnLogPause();
	afx_msg void OnUpdateLogPause(CCmdUI* pCmdUI);
	afx_msg void OnLogStopDisplay();
	afx_msg void OnUpdateLogStopDisplay(CCmdUI* pCmdUI);
	afx_msg void OnLogContinue();
	afx_msg void OnUpdateLogContinue(CCmdUI* pCmdUI);
	//}}AFX_MSG
    
    LRESULT OnAddLogItem(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
    	
private:	
    BOOL InitHeadForLog();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGLISTVIEW_H__F3DE3ADF_65CE_41C2_B65D_B185ACC211B6__INCLUDED_)
