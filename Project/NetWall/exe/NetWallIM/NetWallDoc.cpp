// NetWallDoc.cpp : implementation of the CNetWallDoc class
//

#include "stdafx.h"
#include "NetWall.h"

#include "NetWallDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetWallDoc

IMPLEMENT_DYNCREATE(CNetWallDoc, CDocument)

BEGIN_MESSAGE_MAP(CNetWallDoc, CDocument)
	//{{AFX_MSG_MAP(CNetWallDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNetWallDoc construction/destruction

CNetWallDoc::CNetWallDoc()
{
	// TODO: add one-time construction code here

}

CNetWallDoc::~CNetWallDoc()
{
}

BOOL CNetWallDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CNetWallDoc serialization

void CNetWallDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNetWallDoc diagnostics

#ifdef _DEBUG
void CNetWallDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CNetWallDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNetWallDoc commands

BOOL CNetWallDoc::RouteCmdToAllViews(CView *pView, UINT nID, int nCode, void *pExtra, AFX_CMDHANDLERINFO *pHandlerInfo)
{
    POSITION pos = GetFirstViewPosition();

    while (NULL != pos) 
    {
        CView* pNextView = GetNextView(pos);
        if (pNextView != pView) 
        {
            if (pNextView->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}
