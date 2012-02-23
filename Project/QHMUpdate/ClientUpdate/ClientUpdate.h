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
* 创建连接Server Socket.
* @param pSocket: 连接Socket
* @param szHost:  连接IP
* @param nPortNo: 连接端口
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
* 连接Server线程
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
* 等待所有线程退出 
*/
DWORD 
ClientWaitForMultipleObjects(
	DWORD nCount, 
	const HANDLE* lpHandles, 
	BOOL bWaitAll, 
	DWORD dwMilliseconds 
	) ;

/*
* 写INI配置文件
*/
void WriteIni ();

/*
* 读INI配置文件
*/ 
void ReadIni ();

#endif //_CLIENT_IOCP_H_