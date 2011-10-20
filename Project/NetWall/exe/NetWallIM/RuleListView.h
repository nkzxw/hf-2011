#if !defined(AFX_RULELISTVIEW_H__5F913041_F377_4C2A_8B07_C0459F0E75B4__INCLUDED_)
#define AFX_RULELISTVIEW_H__5F913041_F377_4C2A_8B07_C0459F0E75B4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RuleListView.h : header file
//

class CAdapterInfo;

/////////////////////////////////////////////////////////////////////////////
// CRuleListView view

class CRuleListView : public CListView
{
protected:
	CRuleListView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CRuleListView)

// Attributes
public:
    UINT    m_nType;
    	
// Operations
public:		
	int RefreshRule(CAdapterInfo * pAdapterInfo);
	int RefreshAdapter(CAdapterInfo * pAdapterInfo);
	
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRuleListView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CRuleListView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CRuleListView)
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:	
	BOOL InitHeadForRule();
	BOOL InitHeadForAdapter();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RULELISTVIEW_H__5F913041_F377_4C2A_8B07_C0459F0E75B4__INCLUDED_)
