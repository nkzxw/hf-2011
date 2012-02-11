#ifndef _SERVER_IOCP_H_
#define _SERVER_IOCP_H_

#include "..\Common\common.h"

#define OP_READ     0
#define OP_WRITE    1
#define PORT		12345

#define MAX_WORKER_THREADS 2

#define MAX_BUFFER_LEN 256

HANDLE	g_hShutdownEvent = NULL;
HANDLE	g_hWorkerThreads[MAX_WORKER_THREADS];
HANDLE	g_hAcceptThread = NULL;
WSAEVENT	g_hAcceptEvent;

CRITICAL_SECTION g_csConsole; 
CRITICAL_SECTION g_csClientList; 

HANDLE g_hIOCompletionPort = NULL;

//����ͻ��������Ϣ
class CClientContext  
{
public:
	OVERLAPPED        m_ol;
	WSABUF            m_wbuf; //��¼�ͻ��ȷ��͵���Ϣ����
	
	int               m_nTotalBytes;
	int               m_nSentBytes;
	
	SOCKET            m_Socket;  
	char              m_szBuffer[sizeof (IOCP_FILE_INFO)];
	int               m_nOpCode; 
	
	void SetOpCode(int n){
		m_nOpCode = n;
	}
	
	int GetOpCode(){
		return m_nOpCode;
	}
	
	void SetSocket(SOCKET s){
		m_Socket = s;
	}
	
	SOCKET GetSocket(){
		return m_Socket;
	}
	
	void SetBuffer(char *szBuffer){
		memcpy(m_szBuffer, szBuffer, sizeof (IOCP_FILE_INFO));
	}
	
	void GetBuffer(char *szBuffer){
		memcpy(szBuffer, m_szBuffer, sizeof (IOCP_FILE_INFO));
	}
	
	void ZeroBuffer(){
		ZeroMemory(m_szBuffer, sizeof (IOCP_FILE_INFO));
	}
	
	void SetWSABUFLength(){
		m_wbuf.len = sizeof (IOCP_FILE_INFO);
	}
	
	int GetWSABUFLength(){
		return m_wbuf.len;
	}
	
	void ResetWSABUF(){
		ZeroBuffer();
		m_wbuf.buf = m_szBuffer;
		m_wbuf.len = sizeof (IOCP_FILE_INFO);
	}

	CClientContext()
	{
		ZeroMemory(&m_ol, sizeof(OVERLAPPED));
		m_Socket =  SOCKET_ERROR;
		
		ZeroMemory(m_szBuffer, sizeof (IOCP_FILE_INFO));
		
		m_wbuf.buf = m_szBuffer;
		m_wbuf.len = sizeof (IOCP_FILE_INFO);
		
		m_nOpCode = 0;
		m_nTotalBytes = 0;
		m_nSentBytes = 0;
	}
};

std::vector<CClientContext *> g_ClientContext;

/*
* ��ʼ��
*/
bool Initialize();

/*
* ж��
*/
void DeInitialize();

/*
* ���տͻ��������߳�
* @param lParam: 
* Exception: 
* Return: 
*/
DWORD 
WINAPI 
AcceptThread(
	LPVOID lParam
	);

/*
* ����Socket �� IOCP �Ĺ���
* @param ListenSocket: ����Socket
* Exception: 
* Return: 
*/
void 
AcceptConnection(
	SOCKET ListenSocket
	);

/*
* ������ɶ˿ڵĹ����߳�
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
* ����ͻ���������Ϣ
* @param pClientContext: �ͻ���������Ϣ����
* Exception: 
* Return:
*/
void 
AddToClientList(
	CClientContext	*pClientContext
	);

/*
* �Ƴ��ͻ���������Ϣ
* @param pClientContext: �ͻ���������Ϣ����
* Exception: 
* Return: 
*/
void 
RemoveFromClientList(
	CClientContext	*pClientContext
	);

/*
* ������еĿͻ�������Ϣ
*/
void CleanClientList();


#endif //_SERVER_IOCP_H_