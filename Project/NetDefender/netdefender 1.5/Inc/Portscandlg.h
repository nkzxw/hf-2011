#if !defined(AFX_PORTSCANDLG_H__0391AA01_67AD_11D7_B841_AB5862F4201F__INCLUDED_)
#define AFX_PORTSCANDLG_H__0391AA01_67AD_11D7_B841_AB5862F4201F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// PortScanDlg.h : header file
//
#include <afxtempl.h>	// CArray
#include "reportctrl.h"
#include "BtnST.h"
#include "Label.h"
#include "PPTooltip.h"
/////////////////////////////////////////////////////////////////////////////

// CPortScanDlg dialog

//This data structure registers the statuses of scanned ports:

/*********************************************
 *  This data structure registers the statuses of scanned ports:
 *
 * @author 
 ********************************************/
typedef struct
{
	int		nAttempts;
	CString IPAddress;
	CString port;
	BOOL	bStatus;	//1 = open , 0 = close
} DATA;

class	CScanData
{
public:
	CString m_strErrorlog;
	CString m_strIPAddress;
	UINT	m_nPortNo;
	bool	m_bOpen;
	bool	m_bFinished;

	CScanData()
	{ ;
		//Constructor
		m_strIPAddress = "";
		m_nPortNo = 0;
		m_strErrorlog = "";
		m_bOpen = false;
		m_bFinished = false;
	}

	~CScanData()
	{ ;
	}
};

/*********************************************
 * class  for the  Dialog box that is used for scanning ports
 *
 * @author 
 ********************************************/
class CPortScanDlg : public CDialog
{
// Construction
public:
	/*********************************************
	 * Test the connection 
	 * @return BOOL 
	 ********************************************/
	BOOL	TestConnection(CString IP, UINT nPort);

	/*********************************************
	 *  standard destructor
	 *
	 * @return virtual 
	 ********************************************/
	virtual ~CPortScanDlg();
public:
	/*********************************************
	 * Maximum attempts to connect a socket
	 ********************************************/
	UINT								m_nMaxAttempts; //Maximum attempts to connect a socket

	/*********************************************
	 * standard constructor
	 * @return  
	 ********************************************/
	CPortScanDlg(CWnd *pParent = NULL);					// standard constructor
	CString								IP;

	CArray<CScanData *, CScanData *>	arrScanData;
	UINT								status_timer_id;

	// Dialog Data
	//{{AFX_DATA(CPortScanDlg)
	enum
	{
		IDD = 134
	};
	CStatic m_static;
	CProgressCtrl m_cProgress;
	CReportCtrl m_cResult;
	CIPAddressCtrl m_cIP;
	CEdit m_cPortTo;
	CEdit m_cPortFrom;
	CEdit m_cSinglePort;
	CEdit m_cAttempts;
	CButtonST m_cBtnStop;
	CButtonST m_cBtnScan;
	CLabel	m_lIPAddress;
	CLabel	m_lScanResult;
	//}}AFX_DATA
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPortScanDlg)
protected:
	virtual void	DoDataExchange(CDataExchange *pDX); // DDX/DDV support
	//}}AFX_VIRTUAL
// Implementation
protected:
	/*********************************************
	 * Shows the headers of member variable List box 
	 * @return void 
	 ********************************************/
	void			ShowHeaders(void);		//Shows the headers of member variable m_cResult (See below)

	/*********************************************
	 * Adds some new headers to List box.
	 *
	 * @param hdr 
	 * @return void 
	 ********************************************/
	void			AddHeader(LPTSTR hdr);	//Adds some new headers to m_cResult.

	/*********************************************
	 * Adds a new item to List Box
	 * @return  
	 ********************************************/
	BOOL AddItem(int nItem, int nSubItem, LPCTSTR strItem, int nImageIndex = -1);	//Adds a new item to m_cResult

	/*********************************************
 * Adds a new column to List box
 * @return  
 ********************************************/
	BOOL AddColumn(LPCTSTR strItem, int nItem, int nSubItem = -1, int nMask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
			  int nFmt = LVCFMT_LEFT);		//Adds a new column to m_cResult

	//	CMainFrame* m_parent;
	BOOL			m_bSinglePort;

	/*********************************************
	 * Lower bound and upper bound of scanning ports range
	 ********************************************/
	UINT			m_minPort, m_maxPort;	//Lower bound and upper bound of scanning ports range
	UINT			m_nCounter;

	/*********************************************
	 * titles of columns of List box
	 ********************************************/
	CStringList		*m_pColumns;			//titles of columns of m_cResult

	//*******************************************************************************
	// Generated message map functions
	//{{AFX_MSG(CPortScanDlg)
	virtual BOOL	OnInitDialog();
	afx_msg void	OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void	OnRadioSingle();
	afx_msg void	OnRadioRange();
	afx_msg void	OnButtonScan();
	afx_msg void	OnButtonStop();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	/*********************************************
	 * Our link list. The nodes are in type DATA*
	 ********************************************/
	CPtrList	*m_pStatusList;				//Our link list. The nodes are in type DATA*. See the above structure...
public:
	afx_msg void	OnTimer(UINT nIDEvent);
	CString			GetIPAddress();
	//CPP Tooltips Object
	CPPToolTip				m_tooltip;
	void AddToolTips();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#endif // !defined(AFX_PORTSCANDLG_H__0391AA01_67AD_11D7_B841_AB5862F4201F__INCLUDED_)
