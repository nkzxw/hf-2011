// NetWallDoc.h : interface of the CNetWallDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_NETWALLDOC_H__C5285194_CDD9_495B_8D38_9985AB030D66__INCLUDED_)
#define AFX_NETWALLDOC_H__C5285194_CDD9_495B_8D38_9985AB030D66__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CNetWallDoc : public CDocument
{
protected: // create from serialization only
	CNetWallDoc();
	DECLARE_DYNCREATE(CNetWallDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetWallDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	BOOL RouteCmdToAllViews(CView *pView, UINT nID, int nCode, void *pExtra, AFX_CMDHANDLERINFO *pHandlerInfo);
	virtual ~CNetWallDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CNetWallDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NETWALLDOC_H__C5285194_CDD9_495B_8D38_9985AB030D66__INCLUDED_)
