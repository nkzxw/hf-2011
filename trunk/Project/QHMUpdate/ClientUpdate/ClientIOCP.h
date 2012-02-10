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
	char version[32]; //���°汾
	char name[FILE_COUNT][MAX_PATH]; //�����ļ��Ķ�ά����
	int size[FILE_COUNT]; //�����ļ��Ĵ�С
	WIN32_FILE_ATTRIBUTE_DATA data[FILE_COUNT];
	int count; //�����ļ�������
	
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

#endif //_CLIENT_IOCP_H_