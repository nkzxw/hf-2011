// TCPTable.h: interface for the CTCPTable class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_TCPTABLE_H__79B29239_E26B_11D6_B451_0030052162DB__INCLUDED_)
#define AFX_TCPTABLE_H__79B29239_E26B_11D6_B451_0030052162DB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#include "StdAfx.h"
#include "Base.h"

class CTCPTable : public CBase
{
public:
	CTCPTable();
	virtual				~CTCPTable();

	//vars
	PMIB_TCPTABLE_EX	m_pBuffTcpTableEx;
	DWORD				m_dwLastError;
private:
//methods
public:
	//methods
	DWORD				GetTableEx(void);
	PMIB_TCPTABLE_EX	GetTableEx(DWORD *dw);

	// helper methods
	static CString		Convert2State(DWORD dwState);
	DWORD				GetNumberOfEntries(void);
	MIB_TCPROW_EX		GetElement(DWORD dwIndex);

	DWORD				GetState(DWORD dwIndex);
	DWORD				GetLocalAddress(DWORD dwIndex);
	DWORD				GetLocalPort(DWORD dwIndex);
	DWORD				GetRemoteAddress(DWORD dwIndex);
	DWORD				GetRemotePort(DWORD dwIndex);
	DWORD				GetProcessId(DWORD dwIndex);
};
#endif // !defined(AFX_TCPTABLE_H__79B29239_E26B_11D6_B451_0030052162DB__INCLUDED_)
