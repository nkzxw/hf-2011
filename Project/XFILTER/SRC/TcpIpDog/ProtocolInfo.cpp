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
//=============================================================================================

#include "stdafx.h"
#include "ProtocolInfo.h"
#include "CheckAcl.h"

extern	CCheckAcl			m_CheckAcl;

/****************************************************************************
* 供外部调用的公共函数													    *
****************************************************************************/

//
// 从封包缓冲区pBuf里分析出自己感兴趣的数据，并保存到Session的备注字段里
// 由于发送和接收的数据封包结构各不相同，所以增加参数IsSend来区分发送和接收
//
int	CProtocolInfo::GetProtocolInfo(SESSION *session, TCHAR *pBuf, int nBufLenth, BOOL IsSend)
{
	//
	// 如果没有数据缓冲区，或者缓冲区长度为0，则返回错误代码
	//
	if (pBuf == NULL || nBufLenth == 0)
		return XERR_PROTOCOL_NO_DATA;

	//
	// 根据是发送还是接受分别调用不同的处理函数
	//
	if(IsSend)
		return GetFromSend(session, pBuf, nBufLenth);
	else
		return GetFromRecv(session, pBuf, nBufLenth);

	return XERR_SUCCESS;
}

/****************************************************************************
* 私有函数																    *
****************************************************************************/

//
// 分析发送的数据包，并将结果保存到Session的备注字段里
//
int CProtocolInfo::GetFromSend(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	//
	// 根据不同的协议调用不用的协议分析函数
	//
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

//
// 分析接收的数据包，并将结果保存到session的备注字段里
//
int CProtocolInfo::GetFromRecv(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	//
	// 根据不同的协议调用不用的协议分析函数
	//
	if(session->bProtocol == ACL_SERVICE_TYPE_POP3)
		return GetPop3(session, pBuf, nBufLenth);

	return XERR_SUCCESS;
}

//
// 分析HTTP协议，这里主要是从包头中找出主机信息并保存到Session的备注字段
//
int CProtocolInfo::GetHttp(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	if(nBufLenth < 12 || pBuf[0] < 'A' || pBuf[0] > 'z' || _strnicmp(pBuf, "GET", 3) != 0)
		return XERR_SUCCESS;

	char* Buffer = new char[nBufLenth + 1];
	memcpy(Buffer, pBuf, nBufLenth);
	Buffer[nBufLenth] = 0;

	CString tmp, sHost = _T("");

	tmp = Buffer;

	//
	// 查找主机标识"Host: "
	//
	int i = tmp.Find(_T("Host: "));

	//
	// 如果找到，取出标识后一行数据，到回车符处结束
	//
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
			//
			// 设置字符串结束符
			//
			if(sHost.GetLength() >= MAX_PATH - 1)
				sHost.SetAt(MAX_PATH - 1, '\0');

			//
			// 保存主机信息到Session的备注字段
			//
			_tcscpy(session->sMemo, sHost);
		}
	}

	delete[] Buffer;

	return XERR_SUCCESS;
}

//
// 分析FTP协议，这里用来分析出登录FTP服务器传输的文件名信息
// 最后将分析出的数据保存到Session的备注字段
//
int CProtocolInfo::GetFtp(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	if(nBufLenth < 5 || pBuf[0] < 'A' || pBuf[0] > 'z')
		return XERR_SUCCESS;

	TCHAR tBuf[MAX_PATH];
	CString tmpStr;

	//
	// 分析下载的文件名，标识符为"RETR "
	//
	if(_tcsnicmp(pBuf, _T("RETR "), 5) == 0)
	{
		_stscanf(pBuf + 4, _T("%*[ ]%s"), tBuf);
		tmpStr.Format(_T("Get File: %s"), tBuf);

		if(tmpStr.GetLength() >= MAX_PATH - 1)
			tmpStr.SetAt(MAX_PATH - 1, '\0');

		_tcscpy(session->sMemo, tmpStr);

		m_CheckAcl.GetSession()->SendSessionToAppEx(session);
	}
	//
	// 分析上传的文件名，标识符为"STOR "
	//
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

//
// 分析SMTP协议，得到发送邮件的发件人和收件人后保存到session的备注字段
//
int CProtocolInfo::GetSmtp(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	if(nBufLenth < 9  || pBuf[0] < 'A' || pBuf[0] > 'z')
		return XERR_SUCCESS;

	static CString sEmail = _T("");

	//
	// 分析发件人邮件地址，标识符为"MAIL FROM: "
	//
	if(_tcsnicmp(pBuf, _T("MAIL FROM: "), 11) == 0)
		sEmail.Format(_T("%s"), pBuf);
	//
	// 分析收件人邮件地址，标识符为"RCPT TO: "
	//
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

//
// 分析发送时的POP3数据。保留函数。
//
int CProtocolInfo::GetPop3BySend(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	return XERR_SUCCESS;
}

//
// 分析POP3协议接收的数据，用来得到接收邮件的发件人和收件人后保存到session的
// 备注字段
//
int CProtocolInfo::GetPop3(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	if(nBufLenth < 4 || pBuf[0] < 'A' || pBuf[0] > 'z')
		return XERR_SUCCESS;

	CString		tmpStr;
	int			iStart, iOver, iTmp;
	
	//
	// 查找收件人地址的标识，如果找不到直接返回
	//
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

		//
		// 查找发件人地址的标识符，如果找不到直接返回，并保存索引iStart
		//
		tmpStr = Buffer;
		iStart = tmpStr.Find(_T("From: "));
		if(iStart == -1)
			break;

		//
		// 查找主题标识符，如果找不到直接返回，并保存索引iOver
		//
		iOver = tmpStr.Find(_T("Subject: "));
		if(iOver == -1)
			break;

		//
		// 取出主题和发件人之间的一段字符串
		//
		tmpStr = tmpStr.Mid(iStart, iOver - iStart);

		tmpStr.Replace(13, ';');
		tmpStr.Replace(10, ' ');

		int i = MAX_PATH - 1 - _tcslen(session->sMemo);

		//
		// 如果长度超过session备注字段允许的长度，则截断字符串并保存
		//
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
