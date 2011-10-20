// fireView.h : interface of the CFireView class
//
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_FIREVIEW_H__BFC04DAD_65C6_11D7_9C14_CE97B9E6B96A__INCLUDED_)
#define AFX_FIREVIEW_H__BFC04DAD_65C6_11D7_9C14_CE97B9E6B96A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "Addruledlg.h"
#include "tdriver.h"
#include "MainFrm.h"
#include "reportctrl.h"
#include "BtnST.h"
#include "afxwin.h"
#include "Label.h"
#include "PPTooltip.h"

#define StyleTile 0
#define StyleCenter 1
#define StyleStretch 2

/**
 * class CFireView: This is the main class that define the view area of the application.
 *In a typical MFC application it is called an View class.
 *
 * @author 
 */
class CFireView : public CFormView
{
protected:	// create from serialization only
	/*********************************************
	 * default constructor of the class.
	 *
	 * @return  
	 ********************************************/
	CFireView();

	DECLARE_DYNCREATE(CFireView)
public:
	//{{AFX_DATA(CFireView)
	enum
	{
		IDD = IDD_FIRE_FORM
	};
	CReportCtrl m_cResult;
	CButtonST m_cEditRule;
	CButtonST m_cRemoveRule;
	CButtonST m_btnAddRule;
	CButtonST m_btnReInitializeRule;
	CButton m_cping;
	CButton m_cblockall;
	CButton m_cstart;
	CLabel	m_lFirewallRule;
	//}}AFX_DATA
// Attributes
public:
	CFireDoc	*GetDocument();

	//**********************************************************
	HANDLE		hFile;	// handle to the rule file

	/**
	 * m_filterDriver: instance of filter hook driver
	 */
	TDriver		m_filterDriver;
	TDriver		m_ipFltDrv;

	/*********************************************
	 *Create instance of ADD Rule Dialog box Class
	 ********************************************/
	CAddRuleDlg m_Addrule;

	//	CMainFrame* m_parent;
	/**
	 * ImplementRule: implement predefined rules
	 *
	 * @param  
	 * @return BOOL 
	 */
	BOOL		ImplementRule(void);	// to implement predefined rules
protected:
	/*********************************************
		 * keep track of if the Start button is pressed
		 ********************************************/
	BOOL		start;

	/*********************************************
		 * keep track of if the Block ALL button is pressed
		 ********************************************/
	BOOL		block;

	/*********************************************
		 * keep track of if the Allow All button is pressed
		 ********************************************/
	BOOL		allow;

	/*********************************************
		 * keep track of if the Allow all button is pressed
		 ********************************************/
	BOOL		ping;
	CMainFrame	*m_parent;
	HICON		m_hIcon;

	/**
	 * ParseToIp: Function to convert CString to IP address
	 *
	 * @param str :CString
	 * @return void 
	 */
	//void		ParseToIp(CString str);

	/**
	 * _rows : keep the track of the no of rows in the grid
	 */
	int			_rows;					// keep the track of the no of rows in the grid

	//************************************************
	//list Control
	/**
	 * ShowHeaders:Shows the headers of list cdontrol on the main screen of firewall.
	 * Where added rule are shown
	 *
	 * @param  : void
	 * @return void 
	 */
	void		ShowHeaders(void);		//Shows the headers of member variable m_cResult (See below)

	/**
	 * AddHeader:Adds some new headers to List control (on main window of firewall).
	 *
	 * @param hdr : text of header to be added
	 * @return void 
	 */
	void		AddHeader(LPTSTR hdr);	//Adds some new headers to m_cResult.

	/**
	 * AddItem:Adds a new item to List control (on main window of firewall)
	 *
	 * @param nItem :type int
	 * @param nSubItem :type int
	 * @param strItem :type LPCTSTR
	 * @param nImageIndex :type int
	 * @return  
	 */
	BOOL AddItem(int nItem, int nSubItem, LPCTSTR strItem, int nImageIndex = -1,LPARAM lParam = NULL);	//

	/**
	 * AddColumn:Adds a new column to List control (on main window of firewall)
	 *
	 * @param strItem :text of column to be added
	 * @param nItem :type int
	 * @param nSubItem :type int
	 * @param nMask :type int
	 * @param nFmt :type int
	 * @return  
	 */
	BOOL AddColumn(LPCTSTR strItem, int nItem, int nSubItem = -1, int nMask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			  int nFmt = LVCFMT_LEFT);	//Adds a new column to m_cResult
	CStringList *m_pColumns;			//titles of columns of m_cResult

// Operations
public:
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFireView)
public:
	virtual BOOL	PreCreateWindow(CREATESTRUCT &cs);
	virtual BOOL	Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT &rect,
						   CWnd *pParentWnd, UINT nID, CCreateContext *pContext = NULL);
protected:
	virtual void	DoDataExchange(CDataExchange *pDX); // DDX/DDV support
	virtual void	OnInitialUpdate();					// called first time after construct
	//}}AFX_VIRTUAL
// Implementation
public:
	/**
	 * ~CFireView:Deafult Implementation of Destructor
	 *
	 * @return virtual 
	 */
	virtual			~CFireView();
#ifdef _DEBUG
	virtual void	AssertValid() const;
	virtual void	Dump(CDumpContext &dc) const;
#endif
protected:
	//*************************************************************
	COLORREF	m_clrBk, m_clrText;

	///////////////////////////////////////////
	
	///////////////////////////////////////////

// Generated message map functions
protected:
	//{{AFX_MSG(CFireView)
	afx_msg void	OnAddrule();
	afx_msg void	OnStart();
	afx_msg void	OnBlockping();
	afx_msg void	OnBlockall();
	afx_msg void	OnAllowall();
	afx_msg HBRUSH	OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);
	afx_msg void	OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void	OnUpdateStart(CCmdUI *pCmdUI);
	afx_msg void	OnStop();
	afx_msg void	OnUpdateStop(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateAllowall(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateBlockall(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateBlockping(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateEditRule(CCmdUI *pCmdUI);
	
	//}}AFX_MSG
	afx_msg void NotifyDisplayTooltip(NMHDR * pNMHDR, LRESULT * result);
	DECLARE_MESSAGE_MAP()
private:
	CBrush	*m_pBrush;
public:
	
	//CPP Tooltips Object
	CPPToolTip				m_tooltip;
	void AddToolTips();
	void RegisterRulesFromFile();
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

public:
	afx_msg void OnLvnItemchangedListResult(NMHDR *pNMHDR, LRESULT *pResult);
public:
	afx_msg void OnBnClickedButRemoveRule();
	CNetDefenderRules* GetSelectedItem();
	BOOL ReInitializeRules();
public:
	afx_msg void OnBnClickedButEditRule();
public:
	afx_msg void OnBnClickedReregisterRule();
	//Added for background Image
	int SetBitmap(UINT nIDResource);
	void SetBitmapStyle(int style);
protected:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
private:
	CBitmap m_bitmap;
	int m_style;
};

#ifndef _DEBUG	// debug version in fireView.cpp
inline CFireDoc *CFireView::GetDocument()
{
	return (CFireDoc *) m_pDocument;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#endif // !defined(AFX_FIREVIEW_H__BFC04DAD_65C6_11D7_9C14_CE97B9E6B96A__INCLUDED_)
