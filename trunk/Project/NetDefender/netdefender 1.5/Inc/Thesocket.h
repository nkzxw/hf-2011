#if !defined(AFX_THESOCKET_H__EFF53F01_FDFC_11D4_B48A_B065456AA632__INCLUDED_)
#define AFX_THESOCKET_H__EFF53F01_FDFC_11D4_B48A_B065456AA632__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// TheSocket.h : header file
//
#include <afxsock.h>

/////////////////////////////////////////////////////////////////////////////
// CTheSocket command target
class	CPortScanDlg;

/**
 * class CTheSocket:Class drived from CSocket class
 *
 * @author 
 */
class CTheSocket : public CSocket
{
// Attributes
public:
// Operations
public:
	CTheSocket();
	virtual ~CTheSocket();

// Overrides
public:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTheSocket)
public:
	virtual void	OnAccept(int nErrorCode);
	//}}AFX_VIRTUAL
	// Generated message map functions
	//{{AFX_MSG(CTheSocket)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
// Implementation
protected:
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#endif // !defined(AFX_THESOCKET_H__EFF53F01_FDFC_11D4_B48A_B065456AA632__INCLUDED_)
