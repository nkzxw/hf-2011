// NetWallView.h : interface of the CNetWallView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_NETWALLVIEW_H__91639C0F_CCCF_4FD1_8381_A47B0065C6FA__INCLUDED_)
#define AFX_NETWALLVIEW_H__91639C0F_CCCF_4FD1_8381_A47B0065C6FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MainFrm.h"

class CNetWallView : public CTreeView
{
protected: // create from serialization only
	CNetWallView();
	DECLARE_DYNCREATE(CNetWallView)

// Attributes
public:
	CNetWallDoc* GetDocument();

    CAdapterInfo *  m_pCurrentAdapter;  // Current Selected Adapter
    CAdapterList *  m_pAdapterList;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetWallView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	//}}AFX_VIRTUAL

// Implementation
public:
	void Refresh(NETWALL_LIST_TYPE eType = ADAPTER, CAdapterInfo *pAdapterInfo = NULL);
	BOOL SetButtonState(HTREEITEM hItem, LPCTSTR lpszItem);
	int AddAdapterItems(HTREEITEM hItem, CString strItem);
	void DeleteFirstChild(HTREEITEM hItem);
	void DeleteAllChildren(HTREEITEM hItem);
	virtual ~CNetWallView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	int AddAdapters();
	CImageList m_ilAdapters;
	//{{AFX_MSG(CNetWallView)
	afx_msg void OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in NetWallView.cpp
inline CNetWallDoc* CNetWallView::GetDocument()
   { return (CNetWallDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NETWALLVIEW_H__91639C0F_CCCF_4FD1_8381_A47B0065C6FA__INCLUDED_)
