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
// Author & Create Date: Tony Zhu, 2002/03/18
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
// ��飺
//		�ṩSOCKET���߷����¼�Ļ������Ͳ�������
//		���³�SOCKET���߷����¼ΪSESSION
//
//

#include "stdafx.h"
#include "Session.h"

#pragma data_seg(".inidata")
	BOOL	m_gbIsInitialized = FALSE;
	HANDLE	m_ghSessionFile = NULL;
#pragma data_seg()

//
// ����SESSION����������������XFILTER.EXEͨ��XfIoControl���á�
//
DWORD GetSessionCount()
{
	return SESSION_MAX_COUNT;
}

//
// ����SESSION���������ڴ�ӳ���ļ������XFILTER.EXEͨ��XfIoControl���á�
//
HANDLE GetSessionFileHandle()
{
	return m_ghSessionFile;
}

//===============================================================================
// CSession ��
//

CSession::CSession()
{
	m_pMemFile = NULL;
	m_pSession = NULL;
	InitializeSessionBuffer();
}

CSession::~CSession()
{
	FinallySession();
	if(GetDllRefenceCount() == 0)
		CloseMemFile();

}

//
// �õ�����ΪdwIndex��SESSION��¼ָ��
//
PSESSION CSession::GetSession(DWORD dwIndex)
{
	if(m_pSession == NULL) return NULL;
	if(dwIndex < 0 || dwIndex > SESSION_MAX_COUNT)
		return NULL;
	return &m_pSession[dwIndex];
}

//
// ��ʼ��SESSION������
//
void CSession::InitializeSessionBuffer()
{
	if(!CreateMemFile()) return;

	if(!m_gbIsInitialized)
	{
		for(int i = 0; i < SESSION_MAX_COUNT; i++)
		{
			m_pSession[i].bStatus = SESSION_STATUS_FREE;
			m_pSession[i].dwIndex = i;
			m_pSession[i].s = SESSION_STATUS_FREE;
		}
		m_gbIsInitialized = TRUE;
	}
}

//
// ����һ���ڴ�ӳ���ļ���ΪSESSION������
//
BOOL CSession::CreateMemFile()
{
	if(m_pMemFile != NULL) 
		return TRUE;

	m_ghSessionFile = CreateFileMapping((HANDLE)INVALID_HANDLE_VALUE
		, GetSecurityAttributes()
		, PAGE_READWRITE
		, 0
		, SESSION_MEMORY_FILE_MAX_SIZE
		, SESSION_MEMORY_FILE_NAME);
	if(m_ghSessionFile == NULL) return FALSE;

	m_pMemFile = (char*)MapViewOfFile(m_ghSessionFile, FILE_MAP_WRITE, 0, 0, 0);
	if(m_pMemFile == NULL)
	{
		CloseHandle(m_ghSessionFile);
		m_ghSessionFile = NULL;
		return FALSE;
	}
	m_pSession = (PSESSION)m_pMemFile;
	return TRUE;
}

//
// �ر��ڴ�ӳ���ļ����ͷŻ�����
//
void CSession::CloseMemFile()
{
	if(m_pMemFile != NULL)
		UnmapViewOfFile(m_pMemFile);
	m_pMemFile = NULL;
}

//=============================================================================================
// session operation. session include the socket connection info.

//
// ����һ��SESSION��¼
//
int CSession::CreateSession(SOCKET s, int nProtocol, LPCTSTR sProcessName)
{
	if(m_pSession == NULL) return XERR_SESSION_BUFFER_NOT_EXISTS;
	if(FindSession(s) != -1) return XERR_SESSION_ALREDAY_EXISTS;

	int i = 0;
	for(i; i < SESSION_MAX_COUNT; i++)
	{
		if(m_pSession[i].s == 0)
			break;
	}
	if(i == SESSION_MAX_COUNT) return -1;

	memset(&m_pSession[i], 0, SESSION_LENTH);

	m_pSession[i].bAction	 = XF_PASS;
	m_pSession[i].dwIndex	 = i;
	m_pSession[i].s			 = s;
	m_pSession[i].bStatus	 = SESSION_STATUS_FREE;
	m_pSession[i].dwPid		 = GetCurrentProcessId();
	m_pSession[i].bDirection = ACL_DIRECTION_NOT_SET;
	m_pSession[i].bProtocol	 = nProtocol;
	m_pSession[i].tStartTime = CTime::GetCurrentTime();
	_tcscpy(m_pSession[i].sPathName, sProcessName);

	return i;
}

//
// ɾ��һ��SESSION��¼
//
int CSession::DeleteSession(SOCKET s)
{
	if(m_pSession == NULL) return XERR_SESSION_BUFFER_NOT_EXISTS;
	int iIndex = FindSession(s);
	if(iIndex >= 0)
		SendSessionToApp(&m_pSession[iIndex]);
	return iIndex;
}

//
// ����SESSION��¼�Ĳ����ֶε�ֵ
//
int CSession::SetSession(
	DWORD		dwIndex, 
	BYTE		bDirection, 
	WORD		wPort, 
	DWORD		dwRemoteIp,
	DWORD		IsListen
)
{
	if(m_pSession == NULL) 
		return XERR_SESSION_BUFFER_NOT_EXISTS;
	PSESSION session = &m_pSession[dwIndex];

	if(session->wLocalPort == 0 || session->dwRemoteIp == 0)
	{
		SOCKADDR_IN inAddr;
		int	nNameLength	= sizeof(inAddr);

		if(getsockname(session->s, (SOCKADDR*)&inAddr, &nNameLength) == 0)
		{
			session->wLocalPort = ntohs(inAddr.sin_port);
			memcpy(&session->dwLocalIp, &inAddr.sin_addr, 4);
			session->dwLocalIp	= htonl(session->dwLocalIp);
			if(inAddr.sin_port != 0)
				XF_AddSpiPort(session->wLocalPort);
		}
	}

	if(IsListen) wPort = session->wLocalPort;

	if(session->bProtocol == ACL_SERVICE_TYPE_TCP)
	{
		if(wPort == ACL_SERVICE_PORT_FTP)
			session->bProtocol = ACL_SERVICE_TYPE_FTP;
		else if(wPort == ACL_SERVICE_PORT_HTTP)
			session->bProtocol = ACL_SERVICE_TYPE_HTTP;
		else if(wPort == ACL_SERVICE_PORT_TELNET)
			session->bProtocol = ACL_SERVICE_TYPE_TELNET;
		else if(wPort == ACL_SERVICE_PORT_NNTP)
			session->bProtocol = ACL_SERVICE_TYPE_NNTP;
		else if(wPort == ACL_SERVICE_PORT_POP3)
			session->bProtocol = ACL_SERVICE_TYPE_POP3;
		else if(wPort == ACL_SERVICE_PORT_SMTP)
			session->bProtocol = ACL_SERVICE_TYPE_SMTP;
	}

	if(bDirection != ACL_DIRECTION_NOT_SET)
		session->bDirection	= bDirection;

	if(!IsListen)
	{
		session->wRemotePort = wPort;
		session->dwRemoteIp	 = htonl(dwRemoteIp);
	}

	return XERR_SUCCESS;
}

//
// ����SESSION��¼�Ĳ����ֶε�ֵ
//
int CSession::SetSessionEx(
	DWORD		dwIndex, 
	BYTE		bDirection, 
	TCHAR		*pMemo, 
	int			ByteCount, 
	BOOL		isSend
)
{
	if(m_pSession == NULL) return XERR_SESSION_BUFFER_NOT_EXISTS;
	PSESSION session = &m_pSession[dwIndex];

	if(session->wLocalPort == 0 || session->dwRemoteIp == 0)
	{
		SOCKADDR_IN		inAddr;
		int				nNameLength	= sizeof(inAddr);

		if(getsockname(session->s, (SOCKADDR*)&inAddr, &nNameLength) == 0)
		{
			session->wLocalPort = ntohs(inAddr.sin_port);

			memcpy(&session->dwLocalIp, &inAddr.sin_addr, 4);
			session->dwLocalIp	= htonl(session->dwLocalIp);
		}
	}

	if(bDirection != ACL_DIRECTION_NOT_SET && session->bDirection != bDirection)
		session->bDirection	= bDirection;

	if(pMemo != NULL && session->sMemo[0] == '\0' )
		_tcscpy(session->sMemo, pMemo);

	if(ByteCount > 0)
	{
		if(isSend)
			session->dwSendData += ByteCount;
		else
			session->dwRecvData += ByteCount;
	}

	return XERR_SUCCESS;
}

//
// ����SESSION��¼
//
int CSession::FindSession(SOCKET s)
{
	if(m_pSession == NULL) return XERR_SESSION_BUFFER_NOT_EXISTS;
	int i = 0;
	for(i; i < SESSION_MAX_COUNT; i++)
	{
		if(m_pSession[i].s == s)
			break;
	}
	if(i == SESSION_MAX_COUNT) return -1;

	return i;
}

//
// Ӧ�ó����˳�ʱ�������Ӧ�ó��������SOCKET���ӣ������SESSION
// ����Ϊ����״̬
//
int CSession::FinallySession()
{
	if(m_pSession == NULL) return XERR_SESSION_BUFFER_NOT_EXISTS;
	DWORD dwPid = GetCurrentProcessId();

	for(int i = 0; i < SESSION_MAX_COUNT; i ++)
	{
		if(m_pSession[i].s != 0 && m_pSession[i].dwPid == dwPid)
			SendSessionToApp(&m_pSession[i]);
	}

	return XERR_SUCCESS;
}

//
// ��������SOCKET���ߵ�SESSION��¼���Ϊ����״̬����XFILTER.EXE������־
// ���߽�����������
//
int CSession::SendSessionToApp(SESSION *session)
{
	if(session->wLocalPort != 0)
		XF_DeleteSpiPort(session->wLocalPort);
	session->tEndTime = CTime::GetCurrentTime();
	session->bStatus = SESSION_STATUS_OVER;

	return XERR_SUCCESS;
}

//
// ��Э����������ı�Session��Memo�ֶ�ʱ����״̬����ΪSESSION_STATUS_CHANGE
// ��XFILTER.EXE������־��
//
int CSession::SendSessionToAppEx(SESSION *session)
{
	session->tEndTime = CTime::GetCurrentTime();
	session->bStatus = SESSION_STATUS_CHANGE;

	return XERR_SUCCESS;
}

#pragma comment( exestr, "B9D3B8FD2A756775756B71702B")
