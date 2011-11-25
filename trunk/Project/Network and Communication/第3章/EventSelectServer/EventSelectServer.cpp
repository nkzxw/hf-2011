///////////////////////////////////////////
// EventSelectServer.cpp�ļ�

#include "../common/initsock.h"

#include <stdio.h>
#include <windows.h>

#include "EventSelectServer.h"

// ��ʼ��Winsock��
CInitSock theSock;

int main()
{
	USHORT nPort = 4567;	// �˷����������Ķ˿ں�

	// ���������׽���
	SOCKET sListen = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(nPort);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	if(::bind(sListen, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		printf(" Failed bind() \n");
		return -1;
	}
	::listen(sListen, 200);

	// �����¼����󣬲��������������׽���
	WSAEVENT event = ::WSACreateEvent();
	::WSAEventSelect(sListen, event, FD_ACCEPT|FD_CLOSE);

	::InitializeCriticalSection(&g_cs);

	// ����ͻ��������󣬴�ӡ״̬��Ϣ
	while(TRUE)
	{
		int nRet = ::WaitForSingleObject(event, 5*1000);
		if(nRet == WAIT_FAILED)
		{
			printf(" Failed WaitForSingleObject() \n");
			break;
		}
		else if(nRet == WSA_WAIT_TIMEOUT)	// ��ʱ��ʽ״̬��Ϣ
		{
			printf(" \n");
			printf("   TatolConnections: %d \n", g_nTatolConnections);
			printf(" CurrentConnections: %d \n", g_nCurrentConnections);
			continue;
		}
		else								// ���µ�����δ��
		{
			::ResetEvent(event);
			// ѭ����������δ������������
			while(TRUE)
			{
				sockaddr_in si;
				int nLen = sizeof(si);
				SOCKET sNew = ::accept(sListen, (sockaddr*)&si, &nLen);
				if(sNew == SOCKET_ERROR)
					break;
				PSOCKET_OBJ pSocket = GetSocketObj(sNew);
				pSocket->addrRemote = si;
				::WSAEventSelect(pSocket->s, pSocket->event, FD_READ|FD_CLOSE|FD_WRITE);
				AssignToFreeThread(pSocket);
			}
		}
	}
	::DeleteCriticalSection(&g_cs);
	return 0;
}

