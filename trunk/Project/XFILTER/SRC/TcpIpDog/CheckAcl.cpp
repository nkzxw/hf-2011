//-----------------------------------------------------------
/*
	工程：		费尔个人防火墙
	网址：		http://www.xfilt.com
	电子邮件：	xstudio@xfilt.com
	版权所有 (c) 2002 朱艳辉(费尔安全实验室)

	版权声明:
	---------------------------------------------------
		本电脑程序受著作权法的保护。未经授权，不能使用
	和修改本软件全部或部分源代码。凡擅自复制、盗用或散
	布此程序或部分程序或者有其它任何越权行为，将遭到民
	事赔偿及刑事的处罚，并将依法以最高刑罚进行追诉。
	
		凡通过合法途径购买此源程序者(仅限于本人)，默认
	授权允许阅读、编译、调试。调试且仅限于调试的需要才
	可以修改本代码，且修改后的代码也不可直接使用。未经
	授权，不允许将本产品的全部或部分代码用于其它产品，
	不允许转阅他人，不允许以任何方式复制或传播，不允许
	用于任何方式的商业行为。	

    ---------------------------------------------------	
*/
//-----------------------------------------------------------
//
// 简介：
//		对SPI函数操作的数据和SOCKET连线进行合法性认证
//
//


#include "stdafx.h"
#include "CheckAcl.h"
#include "TcpIpDog.h"


//=============================================================================================
// extern globals variable
extern TCHAR	m_sProcessName[MAX_PATH];

//=============================================================================================
// initialize class function and pre-destroy class function.

CCheckAcl::CCheckAcl()
{
}

CCheckAcl::~CCheckAcl()
{
}

//=============================================================================================
// check the hook function, set session value and return access info.

//
// reserved, not use.
//
int CCheckAcl::CheckStartup()
{
	return XF_PASS;
}

//
// 检查WSPSocket。这里用来得到协议类型并创建SOCKET数据封包记录(SESSION)
//
void CCheckAcl::CheckSocket(SOCKET s, int af, int type, int protocol)
{
	if(af != AF_INET)
		return;

	WORD wProtocol = ACL_SERVICE_TYPE_ALL;	

	if(protocol == IPPROTO_IP)
	{
		if(type == SOCK_STREAM)
			wProtocol = ACL_SERVICE_TYPE_TCP;
		else if(type == SOCK_DGRAM)
			wProtocol = ACL_SERVICE_TYPE_UDP;
	}
	else if(protocol == IPPROTO_TCP)
		wProtocol = ACL_SERVICE_TYPE_TCP;
	else if(protocol == IPPROTO_UDP)
		wProtocol = ACL_SERVICE_TYPE_UDP;

	m_Session.CreateSession(s, wProtocol, m_sProcessName);
}

//
// 检查WSPCloseSocket。在关闭一个SOCKET之前需要首先删除这个
// SOCKET的封包记录(SESSION)
//
void CCheckAcl::CheckCloseSocket(SOCKET s)
{
	m_Session.DeleteSession(s);
}

//
// 检查WSPBind。在为SOCKET绑定端口时用来确定SESSION的本地端口
// 并确定是否是UDP侦听
//
int	CCheckAcl::CheckBind(SOCKET s, const struct sockaddr FAR *name)
{
	int	iIndex = m_Session.FindSession(s);
	if(iIndex == -1) return XF_PASS;

	PSESSION session = m_Session.GetSession(iIndex);
	if(session == NULL) return XF_PASS;
	if(session->bProtocol != ACL_SERVICE_TYPE_UDP)
		return XF_PASS;

	SOCKADDR_IN* pinAddr = (SOCKADDR_IN*)name;

	if(pinAddr->sin_port > 0)
	{
		m_Session.SetSession(iIndex
			, ACL_DIRECTION_LISTEN
			, 0
			, 0
			, TRUE
			);
	}
	else
	{
		m_Session.SetSession(iIndex
			, ACL_DIRECTION_NOT_SET
			, 0
			, 0
			, TRUE
			);
	}

	return m_MemoryFile.CheckAcl(session);
}

//
// 检查WSPListen。在设置SOCKET为侦听状态时，设置SESSION的
// 的连线方向为侦听，并确定侦听端口。(仅TCP)
//
int	CCheckAcl::CheckListen(SOCKET s)
{
	int	iIndex = m_Session.FindSession(s);
	if(iIndex == -1) return XF_PASS;

	PSESSION session = m_Session.GetSession(iIndex);
	if(session == NULL) return XF_PASS;
	if(session->bProtocol != ACL_SERVICE_TYPE_TCP)
		return XF_PASS;

	SOCKADDR_IN	 inAddr;
	int	nLength = sizeof(SOCKADDR_IN);
	getsockname(s, (SOCKADDR*)&inAddr, &nLength);
	
	m_Session.SetSession(iIndex
		, ACL_DIRECTION_LISTEN
		, ntohs(inAddr.sin_port)
		, 0
		, TRUE
		);

	return m_MemoryFile.CheckAcl(session);
}

//
// 检查WSPConnect。确认一个TCP连接请求是否放行。
//
int CCheckAcl::CheckConnect(SOCKET s, const struct sockaddr FAR *name, int namelen)
{
	int	iIndex = m_Session.FindSession(s);
	if(iIndex == -1) return XF_PASS;
	
	SOCKADDR_IN	*pinAddr	= (SOCKADDR_IN*)name;
	WORD		wPort		= ntohs(pinAddr->sin_port);
	DWORD		*pRemoteIp	= (DWORD*)&pinAddr->sin_addr;

	m_Session.SetSession(iIndex, ACL_DIRECTION_OUT, wPort, *pRemoteIp);

	PSESSION session = m_Session.GetSession(iIndex);
	if(session == NULL) return XF_PASS;
	return m_MemoryFile.CheckAcl(session);
}

//
// 检查WSPAccept。确认是否接受一个TCP连接请求。
//
int CCheckAcl::CheckAccept(SOCKET s, SOCKET news)
{
	int	iIndex = m_Session.FindSession(s);
	if(iIndex == -1) return XF_PASS;

	iIndex = m_Session.CreateSession(news, ACL_SERVICE_TYPE_TCP, m_sProcessName);
	if(iIndex == XERR_SESSION_ALREDAY_EXISTS)
		return XF_PASS;
	
	SOCKADDR_IN		addr;
	int				addrlen	= sizeof(addr);
	DWORD			ulRemoteIp;

	getpeername(news, (SOCKADDR*)&addr, &addrlen);
	memcpy(&ulRemoteIp, &addr.sin_addr, 4);
	getsockname(news, (SOCKADDR*)&addr, &addrlen);

	m_Session.SetSession(iIndex
		, ACL_DIRECTION_IN
		, ntohs(addr.sin_port)
		, ulRemoteIp
		);

	PSESSION session = m_Session.GetSession(iIndex);
	if(session == NULL) return XF_PASS;
	return m_MemoryFile.CheckAcl(session);
}

//
// 检查WSPSend。对TCP连接发送的数据进行合法性认证。
//
int CCheckAcl::CheckSend(SOCKET s, TCHAR *buf, int len, LPDWORD lpNumberOfBytesSent)
{
	int	iIndex = m_Session.FindSession(s);
	if(iIndex == -1) return XF_PASS;

	m_Session.SetSessionEx(iIndex, ACL_DIRECTION_NOT_SET, NULL , *lpNumberOfBytesSent, TRUE);

	PSESSION session = m_Session.GetSession(iIndex);
	if(session == NULL) return XF_PASS;

	m_ProtocolInfo.GetProtocolInfo(session, buf, len, TRUE);

	return m_MemoryFile.CheckAcl(session);
}

//
// 检查WSPSendTo。对UDP发送的数据进行合法性认证。
//
int	CCheckAcl::CheckSendTo(SOCKET s, const SOCKADDR *pTo, TCHAR *buf, int len, LPDWORD lpNumberOfBytesSent)
{
	int	iIndex = m_Session.FindSession(s);
	if(iIndex == -1) return XF_PASS;

	PSESSION session = m_Session.GetSession(iIndex);
	if(session == NULL) return XF_PASS;

	if (pTo != NULL && session->bProtocol == ACL_SERVICE_TYPE_UDP )
	{
		SOCKADDR_IN	*pAddr			= (SOCKADDR_IN*)pTo;
		DWORD		*pRemoteIp		= (DWORD*)&pAddr->sin_addr;

		m_Session.SetSession(iIndex, (*pRemoteIp == 0xFFFFFFFF ? ACL_DIRECTION_BROADCAST : ACL_DIRECTION_OUT), ntohs(pAddr->sin_port), *pRemoteIp);
	}

	m_Session.SetSessionEx(iIndex, ACL_DIRECTION_NOT_SET, NULL, *lpNumberOfBytesSent, TRUE);

	m_ProtocolInfo.GetProtocolInfo(session, buf, len, TRUE);

	return m_MemoryFile.CheckAcl(session);
}

//
// 检查WSPRecv。对TCP接收的数据进行合法性认证。
//
int CCheckAcl::CheckRecv(SOCKET s, TCHAR *buf, int len, LPDWORD lpNumberOfBytesRecvd)
{
	int	iIndex = m_Session.FindSession(s);
	if(iIndex == -1) return XF_PASS;

	m_Session.SetSessionEx(iIndex, ACL_DIRECTION_NOT_SET, NULL, *lpNumberOfBytesRecvd, FALSE);

	PSESSION session = m_Session.GetSession(iIndex);
	if(session == NULL) return XF_PASS;
	m_ProtocolInfo.GetProtocolInfo(session, buf, len, FALSE);

	return m_MemoryFile.CheckAcl(session);
}

//
// 检查WSPRecvFrom。对UDP接收的数据进行合法性认证。
//
int CCheckAcl::CheckRecvFrom(SOCKET s, SOCKADDR *pFrom, TCHAR *buf, int len, LPDWORD lpNumberOfBytesRecvd)
{
	int	iIndex = m_Session.FindSession(s);
	if(iIndex == -1) return XF_PASS;

	PSESSION session = m_Session.GetSession(iIndex);
	if(session == NULL) return XF_PASS;

	if (pFrom != NULL && session->bProtocol == ACL_SERVICE_TYPE_UDP)
	{
		SOCKADDR_IN	*pinAddr	= (SOCKADDR_IN*)pFrom;
		DWORD		*pRemoteIp	= (DWORD*)&pinAddr->sin_addr;

		m_Session.SetSession(iIndex, ACL_DIRECTION_IN, ntohs(pinAddr->sin_port), *pRemoteIp); 
	}

	m_Session.SetSessionEx(iIndex, ACL_DIRECTION_NOT_SET, NULL, *lpNumberOfBytesRecvd, FALSE);

	m_ProtocolInfo.GetProtocolInfo(session, buf, len, FALSE);

	return m_MemoryFile.CheckAcl(session);
}



#pragma comment( exestr, "B9D3B8FD2A656A67656D63656E2B")
