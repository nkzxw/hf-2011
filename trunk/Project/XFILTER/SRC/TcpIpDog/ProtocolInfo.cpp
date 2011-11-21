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
//=============================================================================================

#include "stdafx.h"
#include "ProtocolInfo.h"
#include "CheckAcl.h"

extern	CCheckAcl			m_CheckAcl;

/****************************************************************************
* ���ⲿ���õĹ�������													    *
****************************************************************************/

//
// �ӷ��������pBuf��������Լ�����Ȥ�����ݣ������浽Session�ı�ע�ֶ���
// ���ڷ��ͺͽ��յ����ݷ���ṹ������ͬ���������Ӳ���IsSend�����ַ��ͺͽ���
//
int	CProtocolInfo::GetProtocolInfo(SESSION *session, TCHAR *pBuf, int nBufLenth, BOOL IsSend)
{
	//
	// ���û�����ݻ����������߻���������Ϊ0���򷵻ش������
	//
	if (pBuf == NULL || nBufLenth == 0)
		return XERR_PROTOCOL_NO_DATA;

	//
	// �����Ƿ��ͻ��ǽ��ֱܷ���ò�ͬ�Ĵ�����
	//
	if(IsSend)
		return GetFromSend(session, pBuf, nBufLenth);
	else
		return GetFromRecv(session, pBuf, nBufLenth);

	return XERR_SUCCESS;
}

/****************************************************************************
* ˽�к���																    *
****************************************************************************/

//
// �������͵����ݰ�������������浽Session�ı�ע�ֶ���
//
int CProtocolInfo::GetFromSend(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	//
	// ���ݲ�ͬ��Э����ò��õ�Э���������
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
// �������յ����ݰ�������������浽session�ı�ע�ֶ���
//
int CProtocolInfo::GetFromRecv(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	//
	// ���ݲ�ͬ��Э����ò��õ�Э���������
	//
	if(session->bProtocol == ACL_SERVICE_TYPE_POP3)
		return GetPop3(session, pBuf, nBufLenth);

	return XERR_SUCCESS;
}

//
// ����HTTPЭ�飬������Ҫ�ǴӰ�ͷ���ҳ�������Ϣ�����浽Session�ı�ע�ֶ�
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
	// ����������ʶ"Host: "
	//
	int i = tmp.Find(_T("Host: "));

	//
	// ����ҵ���ȡ����ʶ��һ�����ݣ����س���������
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
			// �����ַ���������
			//
			if(sHost.GetLength() >= MAX_PATH - 1)
				sHost.SetAt(MAX_PATH - 1, '\0');

			//
			// ����������Ϣ��Session�ı�ע�ֶ�
			//
			_tcscpy(session->sMemo, sHost);
		}
	}

	delete[] Buffer;

	return XERR_SUCCESS;
}

//
// ����FTPЭ�飬����������������¼FTP������������ļ�����Ϣ
// ��󽫷����������ݱ��浽Session�ı�ע�ֶ�
//
int CProtocolInfo::GetFtp(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	if(nBufLenth < 5 || pBuf[0] < 'A' || pBuf[0] > 'z')
		return XERR_SUCCESS;

	TCHAR tBuf[MAX_PATH];
	CString tmpStr;

	//
	// �������ص��ļ�������ʶ��Ϊ"RETR "
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
	// �����ϴ����ļ�������ʶ��Ϊ"STOR "
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
// ����SMTPЭ�飬�õ������ʼ��ķ����˺��ռ��˺󱣴浽session�ı�ע�ֶ�
//
int CProtocolInfo::GetSmtp(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	if(nBufLenth < 9  || pBuf[0] < 'A' || pBuf[0] > 'z')
		return XERR_SUCCESS;

	static CString sEmail = _T("");

	//
	// �����������ʼ���ַ����ʶ��Ϊ"MAIL FROM: "
	//
	if(_tcsnicmp(pBuf, _T("MAIL FROM: "), 11) == 0)
		sEmail.Format(_T("%s"), pBuf);
	//
	// �����ռ����ʼ���ַ����ʶ��Ϊ"RCPT TO: "
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
// ��������ʱ��POP3���ݡ�����������
//
int CProtocolInfo::GetPop3BySend(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	return XERR_SUCCESS;
}

//
// ����POP3Э����յ����ݣ������õ������ʼ��ķ����˺��ռ��˺󱣴浽session��
// ��ע�ֶ�
//
int CProtocolInfo::GetPop3(SESSION *session, TCHAR *pBuf, int nBufLenth)
{
	if(nBufLenth < 4 || pBuf[0] < 'A' || pBuf[0] > 'z')
		return XERR_SUCCESS;

	CString		tmpStr;
	int			iStart, iOver, iTmp;
	
	//
	// �����ռ��˵�ַ�ı�ʶ������Ҳ���ֱ�ӷ���
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
		// ���ҷ����˵�ַ�ı�ʶ��������Ҳ���ֱ�ӷ��أ�����������iStart
		//
		tmpStr = Buffer;
		iStart = tmpStr.Find(_T("From: "));
		if(iStart == -1)
			break;

		//
		// ���������ʶ��������Ҳ���ֱ�ӷ��أ�����������iOver
		//
		iOver = tmpStr.Find(_T("Subject: "));
		if(iOver == -1)
			break;

		//
		// ȡ������ͷ�����֮���һ���ַ���
		//
		tmpStr = tmpStr.Mid(iStart, iOver - iStart);

		tmpStr.Replace(13, ';');
		tmpStr.Replace(10, ' ');

		int i = MAX_PATH - 1 - _tcslen(session->sMemo);

		//
		// ������ȳ���session��ע�ֶ�����ĳ��ȣ���ض��ַ���������
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
