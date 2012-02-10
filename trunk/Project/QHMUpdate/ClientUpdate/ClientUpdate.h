#ifndef _CLIENT_IOCP_H_
#define _CLIENT_IOCP_H_

#define MAX_BUFFER_LEN 256
#define PORT 12345
#define ADDR "192.168.30.141"

CRITICAL_SECTION g_csConsole; //When threads write to console we need mutual exclusion

#define TRANS_FILE_LENGTH 1024*1024

#define FILE_COUNT 32
typedef struct _IOCP_FILE_INFO
{
	char version[32]; //更新版本
	char name[FILE_COUNT][MAX_PATH]; //更新文件的二维数组
	int size[FILE_COUNT]; //更新文件的大小
	WIN32_FILE_ATTRIBUTE_DATA data[FILE_COUNT];
	int count; //更新文件的数量
	
	struct _IOCP_FILE_INFO ()
	{
		memset (version, 0, 32);
		for (int i = 0; i < FILE_COUNT; i++){
			memset (name[i], 0, MAX_PATH);
		}
		for (int j = 0; j < FILE_COUNT; j++){
			size[j] = 0;
		}

		count = 0;
	}
}IOCP_FILE_INFO, *PIOCP_FILE_INFO;

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

#endif //_CLIENT_IOCP_H_