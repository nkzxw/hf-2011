#ifndef _SERVER_IOCP_H_
#define _SERVER_IOCP_H_

#include "..\Common\common.h"
#include "..\Common\lock.h"

#define OP_ACCEPT   0
#define OP_READ     1
#define OP_WRITE    2
#define OP_OTHER    3
#define PORT		12345

#define MAX_WORKER_THREADS 2

#define MAX_BUFFER_LEN 256

HANDLE	g_hShutdownEvent = NULL;
HANDLE	g_hWorkerThreads[MAX_WORKER_THREADS + 2];
HANDLE	g_hAcceptThread = NULL;
WSAEVENT	g_hAcceptEvent;
//WSAEVENT	g_hShutdownEvent;

CRITICAL_SECTION g_csConsole; 
CRITICAL_SECTION g_csClientList; 

HANDLE g_hIOCompletionPort = NULL;

//保存客户度相关信息
class CClientContext  
{
public:
	OVERLAPPED        m_ol;
	WSABUF            m_wbuf; //记录客户度发送的消息内容
	
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

/*
typedef struct _CONTEXT_LINK_NODE
{
	CClientContext *context; 
	struct _CONTEXT_LINK_NODE *next;
	struct _CONTEXT_LINK_NODE *prev;

	struct _CONTEXT_LINK_NODE ()
	{
		next = NULL;
		prev = NULL;
		context = NULL;
	}
}CONTEXT_LINK_NODE, *PCONTEXT_LINK_NODE;

PCONTEXT_LINK_NODE g_ClientContext;

void
InitContextNode ()
{
	g_ClientContext = (PCONTEXT_LINK_NODE)malloc (sizeof (CONTEXT_LINK_NODE));
	g_ClientContext->next = NULL;
	g_ClientContext->prev = NULL;
	g_ClientContext->context = NULL;	
}

PCONTEXT_LINK_NODE 
AllocContextNode (
	CClientContext *context
	)
{
	PCONTEXT_LINK_NODE pNode = (PCONTEXT_LINK_NODE)malloc (sizeof (CONTEXT_LINK_NODE));
	pNode->next = NULL;
	pNode->prev = NULL;
	pNode->context = context;

	return pNode;
}

void 
FreeContextNode (
	PCONTEXT_LINK_NODE node
	)
{
	if (NULL == node)
		return;

	if (NULL != node->context){
		delete node->context;
		node->context = NULL;
	}

	free(node);
	node = NULL;
}

void 
InsertContextNode (
	PCONTEXT_LINK_NODE node
	)
{
	if (NULL == g_ClientContext ||
		g_ClientContext->next == g_ClientContext->prev)
	{
		g_ClientContext->next = node;
		g_ClientContext->prev = node;
		node->prev = g_ClientContext;
		node->next = g_ClientContext;
	}

	g_ClientContext->next = node;
	node->next = g_ClientContext->next;
	node->prev = g_ClientContext;
}

void 
RemoveContextNode (
	CClientContext *context
	)
{
	if (NULL == g_ClientContext)
		return;

	if (g_ClientContext->next == g_ClientContext ||
		g_ClientContext->prev == g_ClientContext)
	{
		if (g_ClientContext->next = g_ClientContext)
		{
			FreeContextNode (g_ClientContext);
			g_ClientContext = NULL;
		}
		else {
			PCONTEXT_LINK_NODE tmpNode = g_ClientContext;
			g_ClientContext = g_ClientContext->next;
			FreeContextNode (tmpNode);
			tmpNode = NULL;
		}
	}
	
	PCONTEXT_LINK_NODE newNode = g_ClientContext->next;
	while (newNode != g_ClientContext)
	{
		if (newNode->context == context)
		{
			newNode->prev->next = newNode->next;
			newNode->next->prev = newNode->prev;
			FreeContextNode (newNode);
			newNode = NULL;
			break;
		}
	}
}

void 
ClearContextNode ()
{
	if (NULL == g_ClientContext)
		return;

	if (g_ClientContext->next == g_ClientContext ||
		g_ClientContext->prev == g_ClientContext)
	{
		FreeContextNode (g_ClientContext);
		g_ClientContext =	NULL;
	}

	PCONTEXT_LINK_NODE delNode = g_ClientContext;
	while (delNode)
	{
		PCONTEXT_LINK_NODE tmpNode = delNode;
		delNode = delNode->next;
		FreeContextNode (tmpNode);
		tmpNode = NULL;
	}
}
*/

std::vector<CClientContext *> g_ClientContext;

/*
* 初始化
*/
bool Initialize();

/*
* 卸载
*/
void DeInitialize();

/*
* 接收客户端连接线程
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
* 建立Socket 与 IOCP 的关联
* @param ListenSocket: 监听Socket
* Exception: 
* Return: 
*/
void 
AcceptConnection(
	SOCKET ListenSocket
	);

/*
* 处理完成端口的工作线程
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
* 保存客户端连接信息
* @param pClientContext: 客户端连接信息内容
* Exception: 
* Return:
*/
void 
AddToClientList(
	CClientContext	*pClientContext
	);

/*
* 移除客户端连接信息
* @param pClientContext: 客户端连接信息内容
* Exception: 
* Return: 
*/
void 
RemoveFromClientList(
	CClientContext	*pClientContext
	);

/*
* 清楚所有的客户连接信息
*/
void CleanClientList();


#endif //_SERVER_IOCP_H_