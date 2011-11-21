//=============================================================================================
/*
	ProtocolInfo.h

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

#ifndef PROTOCOLINFO_H
#define PROTOCOLINFO_H

class CProtocolInfo
{
private:
	int	GetFromSend		(SESSION *session, TCHAR *pBuf, int nBufLenth);
	int	GetFromRecv		(SESSION *session, TCHAR *pBuf, int nBufLenth);
	int	GetFtp			(SESSION *session, TCHAR *pBuf, int nBufLenth);
	int	GetHttp			(SESSION *session, TCHAR *pBuf, int nBufLenth);
	int	GetSmtp			(SESSION *session, TCHAR *pBuf, int nBufLenth);
	int	GetPop3BySend	(SESSION *session, TCHAR *pBuf, int nBufLenth);
	int	GetPop3			(SESSION *session, TCHAR *pBuf, int nBufLenth);

public:
	int	GetProtocolInfo	(SESSION *session, TCHAR *pBuf, int nBufLenth, BOOL IsSend);
};

#endif