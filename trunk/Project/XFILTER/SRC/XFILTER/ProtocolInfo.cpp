//=============================================================================================
/*
	文件：		ProtocolInfo.cpp

	说明:
	---------------------------------------------------
		TCP/IP 封包分析，利用 TCP/IP 协议的封包结构分
	析截获的数据，提取需要的数据。
	---------------------------------------------------

	工程：		Xfilter 个人防火墙
	作者：		朱雁辉，朱雁冰
	创建日期：	2001/08/21
	网址：		http://www.xfilt.com
	电子邮件：	xstudio@xfilt.com
	版权所有 (c) 2001-2002 X 工作室

	警告:
	---------------------------------------------------
		本电脑程序受著作权法的保护。未经授权，不能使用
	和修改本软件全部或部分源代码。凡擅自复制、盗用或散
	布此程序或部分程序或者有其它任何越权行为，将遭到民
	事赔偿及刑事的处罚，并将依法以最高刑罚进行追诉。
	
		凡通过合法途径购买本软件源代码的用户被默认授权
	可以在自己的程序中使用本软件的部分代码，但作者不对
	代码产生的任何后果负责。
	
		使用了本软件代码的程序只能以可执行文件形式发布，
	未经特别许可，不能将含有本软件源代码的源程序以任何
	形式发布。
	---------------------------------------------------	
*/
//=============================================================================================

#include "stdafx.h"
#include "ProtocolInfo.h"
#include "CheckAcl.h"

extern	CCheckAcl			m_CheckAcl;

//=============================================================================================
// exported public function.

int	CProtocolInfo::GetProtocolInfo(SESSION *session, TCHAR *pBuf, int nBufLenth, BOOL IsSend)
{
	if (pBuf == NULL || nBufLenth == 0)
		return XERR_PROTOCOL_NO_DATA;

	if(IsSend)
		return GetFromSend(session, pBuf, nBufLenth);
	else
		return GetFromRecv(session, pBuf, nBufLenth);

	return XERR_SUCCESS;
}

//=============================================================================================
// private function.

int CProtocolInfo::GetFromSend(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	if(session->bProtocol == ACL_SERVICE_TYPE_HTTP)
		return GetHttp(session, pBuf, nBufLenth);
	else if(session->bProtocol == ACL_SERVICE_TYPE_FTP)
		return GetFtp(session, pBuf, nBufLenth);
	else if(session->bProtocol == ACL_SERVICE_TYPE_SMTP)
		return GetSmtp(session, pBuf, nBufLenth);
	else if(session->bProtocol == ACL_SERVICE_TYPE_POP3)
		return GetPop3BySend(session, pBuf, nBufLenth);

	return XERR_SUCCESS;
}

int CProtocolInfo::GetFromRecv(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	if(session->bProtocol == ACL_SERVICE_TYPE_POP3)
		return GetPop3(session, pBuf, nBufLenth);

	return XERR_SUCCESS;
}

int CProtocolInfo::GetHttp(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	if(nBufLenth < 12 || pBuf[0] < 'A' || pBuf[0] > 'z' || _strnicmp(pBuf, "GET", 3) != 0)
		return XERR_SUCCESS;

	char* Buffer = new char[nBufLenth + 1];
	memcpy(Buffer, pBuf, nBufLenth);
	Buffer[nBufLenth] = 0;

	CString tmp, sHost = _T("");

	tmp = Buffer;

	int i = tmp.Find(_T("Host: "));

	if(i != -1)
	{
		int j = i + 6;
		TCHAR c = tmp.GetAt(j);

		while(c != 13)
		{
			sHost += c;
			c = tmp.GetAt(++j);
		}

		if(session->sMemo[0] == '\0')
		{
			if(sHost.GetLength() >= MAX_PATH - 1)
				sHost.SetAt(MAX_PATH - 1, '\0');

			_tcscpy(session->sMemo, sHost);
		}
	}

	delete[] Buffer;

	return XERR_SUCCESS;
}

int CProtocolInfo::GetFtp(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	if(nBufLenth < 5 || pBuf[0] < 'A' || pBuf[0] > 'z')
		return XERR_SUCCESS;

	TCHAR tBuf[MAX_PATH];
	CString tmpStr;

	if(_tcsnicmp(pBuf, _T("RETR "), 5) == 0)
	{
		_stscanf(pBuf + 4, _T("%*[ ]%s"), tBuf);
		tmpStr.Format(_T("Get File: %s"), tBuf);

		if(tmpStr.GetLength() >= MAX_PATH - 1)
			tmpStr.SetAt(MAX_PATH - 1, '\0');

		_tcscpy(session->sMemo, tmpStr);

		m_CheckAcl.GetSession()->SendSessionToAppEx(session);
	}
	else if(_tcsnicmp(pBuf, _T("STOR "), 5) == 0)
	{
		_stscanf(pBuf + 4, _T("%*[ ]%s"), tBuf);
		tmpStr.Format(_T("Put File: %s"), tBuf);

		if(tmpStr.GetLength() >= MAX_PATH - 1)
			tmpStr.SetAt(MAX_PATH - 1, '\0');

		_tcscpy(session->sMemo, tmpStr);

		m_CheckAcl.GetSession()->SendSessionToAppEx(session);
	}

	return XERR_SUCCESS;
}

int CProtocolInfo::GetSmtp(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	if(nBufLenth < 9  || pBuf[0] < 'A' || pBuf[0] > 'z')
		return XERR_SUCCESS;

	static CString sEmail = _T("");

	if(_tcsnicmp(pBuf, _T("MAIL FROM: "), 11) == 0)
		sEmail.Format(_T("%s"), pBuf);
	else if(_tcsnicmp(pBuf, _T("RCPT TO: "), 9) == 0)
	{
		sEmail += pBuf;
		sEmail.Replace(13, ';');
		sEmail.Replace(10, ' ');
		if(sEmail.GetLength() >= MAX_PATH - 1)
			sEmail.SetAt(MAX_PATH - 1, '\0');

		_tcscpy(session->sMemo, sEmail);
	}

	return XERR_SUCCESS;
}

int CProtocolInfo::GetPop3BySend(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	return XERR_SUCCESS;
}

int CProtocolInfo::GetPop3(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	if(nBufLenth < 4 || pBuf[0] < 'A' || pBuf[0] > 'z')
		return XERR_SUCCESS;

	CString		tmpStr;
	int			iStart, iOver, iTmp;
	
	tmpStr = session->sMemo;
	iTmp = tmpStr.Find(_T("To: "));
	if(iTmp != -1)
		return XERR_SUCCESS;

	char *Buffer = NULL;
	do
	{
		Buffer = new char[nBufLenth + 1];
		memcpy(Buffer, pBuf, nBufLenth);
		Buffer[nBufLenth] = 0;

		tmpStr = Buffer;
		iStart = tmpStr.Find(_T("From: "));

		if(iStart == -1)
			break;

		iOver = tmpStr.Find(_T("Subject: "));
			
		if(iOver == -1)
			break;

		tmpStr = tmpStr.Mid(iStart, iOver - iStart);

		tmpStr.Replace(13, ';');
		tmpStr.Replace(10, ' ');

		int i = MAX_PATH - 1 - _tcslen(session->sMemo);

		if(tmpStr.GetLength() >= i)
			tmpStr.SetAt(i, '\0');

		_tcscat(session->sMemo, tmpStr);

		break;

	} while(true);

	if(Buffer != NULL)
		delete[] Buffer;

	return XERR_SUCCESS;
}

#pragma comment( exestr, "B9D3B8FD2A727471767165716E6B7068712B")
