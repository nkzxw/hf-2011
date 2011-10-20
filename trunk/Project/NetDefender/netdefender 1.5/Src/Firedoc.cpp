// fireDoc.cpp : implementation of the CFireDoc class
//
#include "stdafx.h"
#include "fire.h"

#include "fireDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFireDoc
IMPLEMENT_DYNCREATE(CFireDoc, CDocument)
BEGIN_MESSAGE_MAP(CFireDoc, CDocument)
//{{AFX_MSG_MAP(CFireDoc)
// NOTE - the ClassWizard will add and remove mapping macros here.
//    DO NOT EDIT what you see in these blocks of generated code!
//}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CFireDoc construction/destruction

/**
 * CFireDoc: Calls the base class CDocument constructor
 *
 * @return  
 */
CFireDoc::CFireDoc()
{
	m_bStarted = FALSE;
	m_bBlockAll = FALSE;
	m_bAllowAll = FALSE;
	m_bBlockPing = FALSE;
	m_bStoped = TRUE;
}

CFireDoc::~CFireDoc()
{
}

/**
 * OnNewDocument:Default implementation of OnNewDocument() .defines by MFC framework 
 *
 * @return BOOL 
 */
BOOL CFireDoc::OnNewDocument()
{
	if(!CDocument :: OnNewDocument())
	{
		return(FALSE);
	}

	// (SDI documents will reuse this document)
	return(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CFireDoc serialization

/**
 * Serialize: Default implementation of Serializetion 
 *
 * @param ar : Object of CArchive Class
 * @return void 
 */
void CFireDoc::Serialize(CArchive &ar)
{
	if(ar.IsStoring())
	{
	}
	else
	{
	}
}
BOOL CFireDoc::ISFirewallStarted(BOOL bMessage)
{
	if(m_bStarted != TRUE)
	{
		if(bMessage)
			AfxMessageBox(_T("Start the firewall to use this functionality"),MB_ICONERROR);
	   return FALSE;
	}
	else
	{
		return TRUE;
	}
}
void CFireDoc::setIsFirewallStart(BOOL bFlag )
{
	m_bStarted = bFlag;
	m_bStoped  = !bFlag;
	m_bBlockAll = !bFlag;
	m_bBlockPing =	!bFlag;

}

/////////////////////////////////////////////////////////////////////////////
// CFireDoc diagnostics
#ifdef _DEBUG
void CFireDoc::AssertValid() const
{
	CDocument :: AssertValid();
}

void CFireDoc::Dump(CDumpContext &dc) const
{
	CDocument :: Dump(dc);
}
#endif //_DEBUG
void CFireDoc::setAllowAll(BOOL bFlag )
{
	m_bAllowAll = bFlag;
	m_bBlockPing = !bFlag;
	m_bBlockAll = !bFlag;

}
BOOL CFireDoc::getAllowAll()
{
	return	m_bAllowAll;

}
void CFireDoc::setBlockAll(BOOL bFlag )
{
   m_bBlockAll = bFlag;
   m_bBlockPing = bFlag;
   m_bAllowAll = !bFlag;



}
BOOL CFireDoc::getBlockAll()
{
	return m_bBlockAll;

}
void CFireDoc::setBlockPing(BOOL bFlag)
{
	 m_bBlockPing = bFlag;
	 m_bAllowAll = !bFlag;
	 m_bBlockAll = !bFlag;
}
BOOL CFireDoc::getBlockPing()
{
	return m_bBlockPing;
}
void CFireDoc::setStopFirewall(BOOL bFlag)
{
	m_bStoped = bFlag;
	m_bStarted = !bFlag;
	m_bBlockPing = !bFlag;
	m_bAllowAll = !bFlag;
	m_bBlockAll = !bFlag;
}
BOOL CFireDoc::getStopFirewall()
{
	return m_bStoped;
}