#ifndef _CLIENT_IOCP_H_
#define _CLIENT_IOCP_H_

#include "..\Common\common.h"
#include "..\Common\lock.h"

#define MAX_BUFFER_LEN 256
#define PORT 12345
#define ADDR "192.168.30.141"

#define UPDATE "update"

CRITICAL_SECTION g_csConsole; //When threads write to console we need mutual exclusion

#define TRANS_FILE_LENGTH 1024*1024

struct ThreadInfo
{
     int m_nThreadNo;
     int m_nNoOfSends;
     SOCKET m_Socket;
     char m_szBuffer[MAX_BUFFER_LEN];
};

/*
* ��������Server Socket.
* @param pSocket: ����Socket
* @param szHost:  ����IP
* @param nPortNo: ���Ӷ˿�
* Exception:
* Return:
*/
bool 
CreateSocket(
	SOCKET *pSocket, 
	char *szHost, 
	int nPortNo
	);

/*
* ����Server�߳�
* @param lpParam:
* Exception:
* Return:
*/
DWORD 
WINAPI 
WorkerThread(
	LPVOID lpParam
	);

/*
* �ȴ������߳��˳� 
*/
DWORD 
ClientWaitForMultipleObjects(
	DWORD nCount, 
	const HANDLE* lpHandles, 
	BOOL bWaitAll, 
	DWORD dwMilliseconds 
	) ;

/*
* дINI�����ļ�
*/
void WriteIni ();

/*
* ��INI�����ļ�
*/ 
void ReadIni ();

#endif //_CLIENT_IOCP_H_