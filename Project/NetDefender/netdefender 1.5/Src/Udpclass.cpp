// UDPClass.cpp: implementation of the CUDPClass class.
//
//////////////////////////////////////////////////////////////////////
#include "UDPClass.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
//init static members
DWORD CUDPClass::				m_dwLastError = 0;	//default
PMIB_UDPTABLE_EX CUDPClass::	m_pBuffUdpTableEx = NULL;

//////////////////////////////////////////////////////////////////////

// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CUDPClass::CUDPClass()
{
	Initialize();
}

CUDPClass::~CUDPClass()
{
	//free used mem
	if(m_pBuffUdpTableEx != NULL)
	{
		m_dwLastError = HeapFree(GetProcessHeap(), 0, m_pBuffUdpTableEx);
	}

	CBase :: Unitialize();
}

//////////////////////////////////////////////////////////////////////

//
DWORD CUDPClass::GetTableEx(void)
{
	//free used mem
	if(m_pBuffUdpTableEx != NULL)
	{
		m_dwLastError = HeapFree(GetProcessHeap(), 0, m_pBuffUdpTableEx);
	}

	if(CBase :: m_hModuleUdp != NULL)
	{
		//gathering info
		(CBase :: m_pGetUdpTableEx) (&m_pBuffUdpTableEx, TRUE, GetProcessHeap(), 0, 2);
	}
	else
	{
		return(0);
	}

	return(1);
}

//////////////////////////////////////////////////////////////////////
//
