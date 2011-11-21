//-----------------------------------------------------------
/*
	���̣�		�Ѷ����˷���ǽ
	��ַ��		http://www.xfilt.com
	�����ʼ���	xstudio@xfilt.com
	��Ȩ���� (c) 2002 ���޻�(�Ѷ���ȫʵ����)

	��Ȩ����:
	---------------------------------------------------
		�����Գ���������Ȩ���ı�����δ����Ȩ������ʹ��
	���޸ı����ȫ���򲿷�Դ���롣�����Ը��ơ����û�ɢ
	���˳���򲿷ֳ�������������κ�ԽȨ��Ϊ�����⵽��
	���⳥�����µĴ�������������������̷�����׷�ߡ�
	
		��ͨ���Ϸ�;�������Դ������(�����ڱ���)��Ĭ��
	��Ȩ�����Ķ������롢���ԡ������ҽ����ڵ��Ե���Ҫ��
	�����޸ı����룬���޸ĺ�Ĵ���Ҳ����ֱ��ʹ�á�δ��
	��Ȩ������������Ʒ��ȫ���򲿷ִ�������������Ʒ��
	������ת�����ˣ����������κη�ʽ���ƻ򴫲���������
	�����κη�ʽ����ҵ��Ϊ��	

    ---------------------------------------------------	
*/
//-----------------------------------------------------------
//
// ��飺
//		��SPI�������������ݺ�SOCKET���߽��кϷ�����֤
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
// ���WSPSocket�����������õ�Э�����Ͳ�����SOCKET���ݷ����¼(SESSION)
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
// ���WSPCloseSocket���ڹر�һ��SOCKET֮ǰ��Ҫ����ɾ�����
// SOCKET�ķ����¼(SESSION)
//
void CCheckAcl::CheckCloseSocket(SOCKET s)
{
	m_Session.DeleteSession(s);
}

//
// ���WSPBind����ΪSOCKET�󶨶˿�ʱ����ȷ��SESSION�ı��ض˿�
// ��ȷ���Ƿ���UDP����
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
// ���WSPListen��������SOCKETΪ����״̬ʱ������SESSION��
// �����߷���Ϊ��������ȷ�������˿ڡ�(��TCP)
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
// ���WSPConnect��ȷ��һ��TCP���������Ƿ���С�
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
// ���WSPAccept��ȷ���Ƿ����һ��TCP��������
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
// ���WSPSend����TCP���ӷ��͵����ݽ��кϷ�����֤��
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
// ���WSPSendTo����UDP���͵����ݽ��кϷ�����֤��
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
// ���WSPRecv����TCP���յ����ݽ��кϷ�����֤��
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
// ���WSPRecvFrom����UDP���յ����ݽ��кϷ�����֤��
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
