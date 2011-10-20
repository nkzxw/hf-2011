// TCPTable.cpp: implementation of the CTCPTable class.
//
//////////////////////////////////////////////////////////////////////
#include "TCPTable.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////

// Construction/Destruction
//////////////////////////////////////////////////////////////////////								
CTCPTable::CTCPTable()
{
	m_dwLastError = 0;	//default
	m_pBuffTcpTableEx = NULL;

	CBase :: Initialize();
}

//////////////////////////////////////////////////////////////////////

//
CTCPTable::~CTCPTable()
{
	//free used mem
	if(m_pBuffTcpTableEx != NULL)
	{
		m_dwLastError = HeapFree(GetProcessHeap(), 0, m_pBuffTcpTableEx);
	}

	CBase :: Unitialize();
}

//////////////////////////////////////////////////////////////////////

//
DWORD CTCPTable::GetTableEx(void)
{
	//free used mem
	if(m_pBuffTcpTableEx != NULL)
	{
		m_dwLastError = HeapFree(GetProcessHeap(), 0, m_pBuffTcpTableEx);
	}

	if(CBase :: m_hModuleTcp != NULL)
	{
		//gathering info
		(CBase :: m_pGetTcpTableEx) (&m_pBuffTcpTableEx, TRUE, //sorted list
		GetProcessHeap(), 0, 2);
	}
	else
	{
		return(1);	//error
	}

	return(0);
}

//////////////////////////////////////////////////////////////////////

//
CString CTCPTable::Convert2State(DWORD dwState)
{
	switch(dwState)
	{
		case MIB_TCP_STATE_CLOSED:
			return(_T("CLOSED"));

		case MIB_TCP_STATE_LISTEN:
			return(_T("LISTENING"));

		case MIB_TCP_STATE_SYN_SENT:
			return(_T("SYN_SENT"));

		case MIB_TCP_STATE_SYN_RCVD:
			return(_T("SYN_RECEIVED"));

		case MIB_TCP_STATE_ESTAB:
			return(_T("ESTABLISHED"));

		case MIB_TCP_STATE_FIN_WAIT1:
			return(_T("FIN_WAIT1"));

		case MIB_TCP_STATE_FIN_WAIT2:
			return(_T("FIN_WAIT2"));

		case MIB_TCP_STATE_CLOSE_WAIT:
			return(_T("CLOSE_WAIT"));

		case MIB_TCP_STATE_CLOSING:
			return(_T("CLOSING"));

		case MIB_TCP_STATE_LAST_ACK:
			return(_T("LAST_ACK"));

		case MIB_TCP_STATE_TIME_WAIT:
			return(_T("TIME_WAIT"));

		case MIB_TCP_STATE_DELETE_TCB:
			return(_T("DELETE_TCB"));

		default:
			return(_T("UNKNOWN"));
	}
}

//////////////////////////////////////////////////////////////////////

//
PMIB_TCPTABLE_EX CTCPTable::GetTableEx(DWORD *dw)
{
	if(0 == (*dw = GetTableEx()))
	{
		return(m_pBuffTcpTableEx);
	}
	else
	{
		return(NULL);
	}
}

//////////////////////////////////////////////////////////////////////

//
DWORD CTCPTable::GetNumberOfEntries(void)
{
	if(m_pBuffTcpTableEx != NULL)
	{
		return(m_pBuffTcpTableEx->dwNumEntries);
	}
	else
	{
		return(0);
	}
}

//////////////////////////////////////////////////////////////////////

//
MIB_TCPROW_EX CTCPTable::GetElement(DWORD dwIndex)
{
	MIB_TCPROW_EX	sRow = { 0, 0, 0, 0, 0 };

	if(m_pBuffTcpTableEx != NULL)
	{
		if(dwIndex < GetNumberOfEntries())
		{
			return(m_pBuffTcpTableEx->table[dwIndex]);
		}
	}

	return(sRow);
}

//////////////////////////////////////////////////////////////////////

//
DWORD CTCPTable::GetState(DWORD dwIndex)
{
	if(m_pBuffTcpTableEx != NULL)
	{
		if(dwIndex < GetNumberOfEntries())
		{
			return(m_pBuffTcpTableEx->table[dwIndex].dwState);
		}
	}

	return(0);
}

//////////////////////////////////////////////////////////////////////

//
DWORD CTCPTable::GetLocalAddress(DWORD dwIndex)
{
	if(m_pBuffTcpTableEx != NULL)
	{
		if(dwIndex < GetNumberOfEntries())
		{
			return(m_pBuffTcpTableEx->table[dwIndex].dwLocalAddr);
		}
	}

	return(0);
}

//////////////////////////////////////////////////////////////////////

//
DWORD CTCPTable::GetLocalPort(DWORD dwIndex)
{
	if(m_pBuffTcpTableEx != NULL)
	{
		if(dwIndex < GetNumberOfEntries())
		{
			return(m_pBuffTcpTableEx->table[dwIndex].dwLocalPort);
		}
	}

	return(0);
}

//////////////////////////////////////////////////////////////////////

//
DWORD CTCPTable::GetRemoteAddress(DWORD dwIndex)
{
	if(m_pBuffTcpTableEx != NULL)
	{
		if(dwIndex < GetNumberOfEntries())
		{
			return(m_pBuffTcpTableEx->table[dwIndex].dwRemoteAddr);
		}
	}

	return(0);
}

//////////////////////////////////////////////////////////////////////

//
DWORD CTCPTable::GetRemotePort(DWORD dwIndex)
{
	if(m_pBuffTcpTableEx != NULL)
	{
		if(dwIndex < GetNumberOfEntries())
		{
			return(m_pBuffTcpTableEx->table[dwIndex].dwRemotePort);
		}
	}

	return(0);
}

//////////////////////////////////////////////////////////////////////

//
DWORD CTCPTable::GetProcessId(DWORD dwIndex)
{
	if(m_pBuffTcpTableEx != NULL)
	{
		if(dwIndex < GetNumberOfEntries())
		{
			return(m_pBuffTcpTableEx->table[dwIndex].dwProcessId);
		}
	}

	return(0);
}

//////////////////////////////////////////////////////////////////////
//
