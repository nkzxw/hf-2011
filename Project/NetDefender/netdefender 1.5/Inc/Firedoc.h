// fireDoc.h : interface of the CFireDoc class
//
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_FIREDOC_H__BFC04DAB_65C6_11D7_9C14_CE97B9E6B96A__INCLUDED_)
#define AFX_FIREDOC_H__BFC04DAB_65C6_11D7_9C14_CE97B9E6B96A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/**
 * class CFireDoc: This is the main class that store the application data.
 *In a typical MFC application it is called an Document class.
 *
 * @author Sudhir mangla
 */
class CFireDoc : public CDocument
{
protected:	// create from serialization only
	/**
	 * CFireDoc:  default constructor of the class.
	 *
	 * @return  
	 */
	CFireDoc();
	DECLARE_DYNCREATE(CFireDoc)
// Attributes
public:
	BOOL ISFirewallStarted(BOOL bMessage=TRUE);
	void setIsFirewallStart(BOOL bFlag = TRUE);
	void setAllowAll(BOOL bFlag = TRUE);
	BOOL getAllowAll();
	void setBlockAll(BOOL bFlag = TRUE);
	BOOL getBlockAll();
	void setBlockPing(BOOL bFlag = TRUE);
	BOOL getBlockPing();
	void setStopFirewall(BOOL bFlag = TRUE);
	BOOL getStopFirewall();
	
public:
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFireDoc)
public:
	virtual BOOL	OnNewDocument();
	virtual void	Serialize(CArchive &ar);
	//}}AFX_VIRTUAL
// Implementation
public:
	virtual			~CFireDoc();
#ifdef _DEBUG
	virtual void	AssertValid() const;
	virtual void	Dump(CDumpContext &dc) const;
#endif
protected:
	BOOL m_bStarted;	//Set to true if Firewall is started
	BOOL m_bStoped;		//Set to true if Firewall is Stopped
	BOOL m_bBlockAll;	//Set to true if Blocked All button is clicked
	BOOL m_bAllowAll;	//Set to true if Allow All button is clicked
	BOOL m_bBlockPing;	//Set to true if Block Ping button is clicked
// Generated message map functions
protected:
	//{{AFX_MSG(CFireDoc)
	// NOTE - the ClassWizard will add and remove member functions here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#endif // !defined(AFX_FIREDOC_H__BFC04DAB_65C6_11D7_9C14_CE97B9E6B96A__INCLUDED_)
