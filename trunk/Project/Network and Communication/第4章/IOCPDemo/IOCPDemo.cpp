/////////////////////////////////////////////////
// IOCPDemo.cpp�ļ�			����ͨ��


#include "../common/initsock.h"
#include <stdio.h>
#include <windows.h>

// ��ʼ��Winsock��
CInitSock theSock;

#define BUFFER_SIZE 1024

typedef struct _PER_HANDLE_DATA		// per-handle����
{
	SOCKET s;			// ��Ӧ���׽��־��
	sockaddr_in addr;	// �ͻ�����ַ
} PER_HANDLE_DATA, *PPER_HANDLE_DATA;


typedef struct _PER_IO_DATA			// per-I/O����
{
	OVERLAPPED ol;			// �ص��ṹ
	char buf[BUFFER_SIZE];	// ���ݻ�����
	int nOperationType;		// ��������
#define OP_READ   1
#define OP_WRITE  2
#define OP_ACCEPT 3
} PER_IO_DATA, *PPER_IO_DATA;


DWORD WINAPI ServerThread(LPVOID lpParam)
{
	// �õ���ɶ˿ڶ�����
	HANDLE hCompletion = (HANDLE)lpParam;

	DWORD dwTrans;
	PPER_HANDLE_DATA pPerHandle;
	PPER_IO_DATA pPerIO;
	while(TRUE)
	{
		// �ڹ���������ɶ˿ڵ������׽����ϵȴ�I/O���
		BOOL bOK = ::GetQueuedCompletionStatus(hCompletion, 
			&dwTrans, (LPDWORD)&pPerHandle, (LPOVERLAPPED*)&pPerIO, WSA_INFINITE);
		if(!bOK)						// �ڴ��׽������д�����
		{
			::closesocket(pPerHandle->s);
			::GlobalFree(pPerHandle);
			::GlobalFree(pPerIO);
			continue;
		}
		
		if(dwTrans == 0 &&				// �׽��ֱ��Է��ر�
			(pPerIO->nOperationType == OP_READ || pPerIO->nOperationType == OP_WRITE))	
			
		{
			::closesocket(pPerHandle->s);
			::GlobalFree(pPerHandle);
			::GlobalFree(pPerIO);
			continue;
		}

		switch(pPerIO->nOperationType)	// ͨ��per-I/O�����е�nOperationType��鿴ʲôI/O���������
		{
		case OP_READ:	// ���һ����������
			{
				pPerIO->buf[dwTrans] = '\0';
				printf(pPerIO -> buf);
				
				// ����Ͷ�ݽ���I/O����
				WSABUF buf;
				buf.buf = pPerIO->buf ;
				buf.len = BUFFER_SIZE;
				pPerIO->nOperationType = OP_READ;

				DWORD nFlags = 0;
				::WSARecv(pPerHandle->s, &buf, 1, &dwTrans, &nFlags, &pPerIO->ol, NULL);
			}
			break;
		case OP_WRITE: // ������û��Ͷ����Щ���͵�I/O����
		case OP_ACCEPT:
			break;
		}
	}
	return 0;
}


void main()
{
	int nPort = 4567;
	// ������ɶ˿ڶ��󣬴��������̴߳�����ɶ˿ڶ������¼�
	HANDLE hCompletion = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	::CreateThread(NULL, 0, ServerThread, (LPVOID)hCompletion, 0, 0);

	// ���������׽��֣��󶨵����ص�ַ����ʼ����
	SOCKET sListen = ::socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN si;
	si.sin_family = AF_INET;
	si.sin_port = ::ntohs(nPort);
	si.sin_addr.S_un.S_addr = INADDR_ANY;
	::bind(sListen, (sockaddr*)&si, sizeof(si));
	::listen(sListen, 5);

	// ѭ��������������
	while(TRUE)
	{
		// �ȴ�����δ������������
		SOCKADDR_IN saRemote;
		int nRemoteLen = sizeof(saRemote);
		SOCKET sNew = ::accept(sListen, (sockaddr*)&saRemote, &nRemoteLen);

		// ���ܵ�������֮��Ϊ������һ��per-handle���ݣ��������ǹ�������ɶ˿ڶ���
		PPER_HANDLE_DATA pPerHandle = 
							(PPER_HANDLE_DATA)::GlobalAlloc(GPTR, sizeof(PER_HANDLE_DATA));
		pPerHandle->s = sNew;
		memcpy(&pPerHandle->addr, &saRemote, nRemoteLen);
		::CreateIoCompletionPort((HANDLE)pPerHandle->s, hCompletion, (DWORD)pPerHandle, 0);
	
		// Ͷ��һ����������
		PPER_IO_DATA pPerIO = (PPER_IO_DATA)::GlobalAlloc(GPTR, sizeof(PER_IO_DATA));
		pPerIO->nOperationType = OP_READ;
		WSABUF buf;
		buf.buf = pPerIO->buf;
		buf.len = BUFFER_SIZE;	
		DWORD dwRecv;
		DWORD dwFlags = 0;
		::WSARecv(pPerHandle->s, &buf, 1, &dwRecv, &dwFlags, &pPerIO->ol, NULL);
	}
}
