//=============================================================================================
/*
	CheckAcl.h

	Project	: XFILTER 1.0 Personal Firewall
	Author	: Tony Zhu
	Create Date	: 2001/08/21
	Email	: xstudio@xfilt.com
	URL		: http://www.xfilt.com

	Copyright (c) 2001-2002 XStudio Technology.
	All Rights Reserved.

	WARNNING: 
*/
//=============================================================================================

#ifndef CHECKACL_H
#define CHECKACL_H

class CCheckAcl
{
public:
	CCheckAcl();
	virtual	~CCheckAcl();
public:
	int		CheckStartup	();
	void	CheckSocket		(SOCKET s, int af, int type, int protocol);
	void	CheckCloseSocket(SOCKET s);
	int		CheckBind		(SOCKET s, const struct sockaddr FAR *name);
	int		CheckListen		(SOCKET s);
	int		CheckConnect	(SOCKET s, const struct sockaddr FAR *name, int namelen);
	int		CheckAccept		(SOCKET s, SOCKET news);
	int		CheckSend		(SOCKET s, TCHAR *buf, int len, LPDWORD lpNumberOfBytesSent);
	int		CheckSendTo		(SOCKET s, const SOCKADDR *pTo, TCHAR *buf, int len, LPDWORD lpNumberOfBytesSent);
	int		CheckRecv		(SOCKET s, TCHAR *buf, int len, LPDWORD lpNumberOfBytesRecvd);
	int		CheckRecvFrom	(SOCKET s, SOCKADDR *pFrom, TCHAR *buf, int len, LPDWORD lpNumberOfBytesRecvd);

public:
	CProtocolInfo*	GetProtocolInfo(){return &m_ProtocolInfo;}
	CSession*		GetSession(){return &m_Session;}
	CMemoryFile*	GetMomoryFile(){ return &m_MemoryFile;}

private:
	CProtocolInfo	m_ProtocolInfo;
	CSession		m_Session;
	CMemoryFile		m_MemoryFile;
};

#endif