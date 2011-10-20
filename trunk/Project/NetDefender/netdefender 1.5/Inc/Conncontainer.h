// ConnContainer1.h: interface for the CConnContainer class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_CONNCONTAINER1_H__9B173F89_208C_4C48_853B_9B20BF1D1F1D__INCLUDED_)
#define AFX_CONNCONTAINER1_H__9B173F89_208C_4C48_853B_9B20BF1D1F1D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#pragma warning(disable : 4786) // get rid of stl warnings
#include "TCPTable.h"
#include <map>
#include <list>
#include <vector>
#include <algorithm>
using std:: map;
using std:: list;
using std:: vector;

#include "afxmt.h"

/************************************************************************/
/* Note: 
Outgoing connection: (connection state is set to "ESTABLISHED")
+ new local port (don't care about remote context, but store it) #ACT

		Incomming connection: (connection state is set to "ESTABLISHED")
		+ on existing local port
			- new remote IP (don't care about remote port, but store it) #ACT
			+ same remote IP
				- different remote port(store it)			 #ACT
*/
/************************************************************************/
typedef vector<MIB_TCPROW_EX>	vectTcpTable;

class							CConnContainer
{
public:
	CConnContainer();
	virtual ~CConnContainer();
public:
	static bool IsNewConnection();
private:
	static vectTcpTable		m_vTcpTable;
	static CCriticalSection __CriticalSection__;
};
#endif // !defined(AFX_CONNCONTAINER1_H__9B173F89_208C_4C48_853B_9B20BF1D1F1D__INCLUDED_)
