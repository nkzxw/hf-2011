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
// Author & Create Date: Tony Zhu, 2002/03/18
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
// 简介：
//		提供SOCKET连线封包记录的缓冲区和操作函数
//		以下称SOCKET连线封包记录为SESSION
//
//

#include "stdafx.h"
#include "Session.h"

#pragma data_seg(".inidata")
	BOOL	m_gbIsInitialized = FALSE;
	HANDLE	m_ghSessionFile = NULL;
#pragma data_seg()

//
// 返回SESSION缓冲区的最大个数，XFILTER.EXE通过XfIoControl调用。
//
DWORD GetSessionCount()
{
	return SESSION_MAX_COUNT;
}

//
// 返回SESSION缓冲区的内存映射文件句柄，XFILTER.EXE通过XfIoControl调用。
//
HANDLE GetSessionFileHandle()
{
	return m_ghSessionFile;
}

//===============================================================================
// CSession 类
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
// 得到索引为dwIndex的SESSION记录指针
//
PSESSION CSession::GetSession(DWORD dwIndex)
{
	if(m_pSession == NULL) return NULL;
	if(dwIndex < 0 || dwIndex > SESSION_MAX_COUNT)
		return NULL;
	return &m_pSession[dwIndex];
}

//
// 初始化SESSION缓冲区
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
// 创建一个内存映射文件作为SESSION缓冲区
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
// 关闭内存映射文件并释放缓冲区
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
// 创建一条SESSION记录
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
// 删除一条SESSION记录
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
// 设置SESSION记录的部分字段的值
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
// 设置SESSION记录的部分字段的值
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
// 查找SESSION记录
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
// 应用程序退出时结束这个应用程序的所有SOCKET连接，将相关SESSION
// 设置为结束状态
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
// 将结束的SOCKET连线的SESSION记录标记为结束状态，由XFILTER.EXE保存日志
// 或者进行其他处理
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
// 当协议解析函数改变Session的Memo字段时，将状态设置为SESSION_STATUS_CHANGE
// 由XFILTER.EXE保存日志。
//
int CSession::SendSessionToAppEx(SESSION *session)
{
	session->tEndTime = CTime::GetCurrentTime();
	session->bStatus = SESSION_STATUS_CHANGE;

	return XERR_SUCCESS;
}

#pragma comment( exestr, "B9D3B8FD2A756775756B71702B")
