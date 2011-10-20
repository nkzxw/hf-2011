// ConnContainer1.cpp: implementation of the CConnContainer class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

//#include "ENetStatX.h"
#include "ConnContainer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

// init static members
vectTcpTable CConnContainer::		m_vTcpTable;
CCriticalSection CConnContainer::	__CriticalSection__;

//////////////////////////////////////////////////////////////////////

// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CConnContainer::CConnContainer()
{
}

//////////////////////////////////////////////////////////////////////

//
CConnContainer::~CConnContainer()
{
}

//////////////////////////////////////////////////////////////////////

//
bool CConnContainer::IsNewConnection()
{
	__CriticalSection__.Lock();

	CTCPTable	*m_pTcp = new CTCPTable();
	if(m_pTcp == NULL)
	{
		return(false);	//error
	}

	//fill buffer
	DWORD	dw = m_pTcp->GetTableEx();
	if(dw != NO_ERROR)
	{
		return(false);	//error
	}

	if(m_pTcp->m_pBuffTcpTableEx == NULL)
	{
		return(false);	//error
	}

	// fields
	vectTcpTable	vTcpTable;

	//build a reference table (a new one)
	for(int i = 0; i < (int)m_pTcp->GetNumberOfEntries(); i++)
	{
		//take the data in the bloody vector
		vTcpTable.push_back(m_pTcp->GetElement(i));
	}

	//compare the present vector list with the one we already have
	vectTcpTable :: iterator	itNew = vTcpTable.begin();
	vectTcpTable :: iterator	itOld = m_vTcpTable.begin();

	//we have gather the tcp table sorted, so just check the consistency
	if(m_vTcpTable.size() == vTcpTable.size())
	{
		for(itNew = vTcpTable.begin(); itNew != vTcpTable.end(); itNew++)
		{
			//check the consistency
			if((*itNew).dwLocalAddr == (*itOld).dwLocalAddr && (*itNew).dwLocalPort == (*itOld).dwLocalPort &&
			   (*itNew).dwProcessId == (*itOld).dwProcessId && (*itNew).dwRemoteAddr == (*itOld).dwRemoteAddr &&
			   (*itNew).dwRemotePort == (*itOld).dwRemotePort && (*itNew).dwState == (*itOld).dwState)
			{
				itOld++;
			}
			else
			{
				//update the tcp table
				m_vTcpTable.clear();

				for(int j = 0; j < (int)m_pTcp->GetNumberOfEntries(); j++)
				{
					//take the data in the bloody vector
					m_vTcpTable.push_back(m_pTcp->GetElement(j));
				}

				return(true);
			}
		}
	}
	else
	{
		//it's obviously that something has changed into the tcp table
		//update the tcp table
		m_vTcpTable.clear();

		for(int jj = 0; jj < (int)m_pTcp->GetNumberOfEntries(); jj++)
		{
			//take the data in the bloody vector
			m_vTcpTable.push_back(m_pTcp->GetElement(jj));
		}

		return(true);
	}
   __CriticalSection__.Unlock();
	return(false);

	
}
