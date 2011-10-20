// TheSocket.cpp : implementation file
//
#include "stdafx.h"

//#include "PortScan.h"
#include "TheSocket.h"
#include "PortScanDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

// CTheSocket
CTheSocket::CTheSocket()
{
}

CTheSocket::~CTheSocket()
{
}

// Do not edit the following lines, which are needed by ClassWizard.
#if 0
BEGIN_MESSAGE_MAP(CTheSocket, CSocket)
//{{AFX_MSG_MAP(CTheSocket)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif // 0
	
	/////////////////////////////////////////////////////////////////////////////

// CTheSocket member functions
void CTheSocket::OnAccept(int nErrorCode)
{
	CSocket :: OnAccept(nErrorCode);
}
