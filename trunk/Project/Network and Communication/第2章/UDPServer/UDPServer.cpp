//////////////////////////////////////////////////////////
// UDPServer.cpp�ļ�


#include "../common/InitSock.h"
#include <stdio.h>
CInitSock initSock;		// ��ʼ��Winsock��

int main()
{
	// �����׽���
	SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(s == INVALID_SOCKET)
	{
		printf("Failed socket() \n");
		return 0;
	}
	
	// ���sockaddr_in�ṹ
	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(4567);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;
	
	// ������׽��ֵ�һ�����ص�ַ
	if(::bind(s, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		printf("Failed bind() \n");
		return 0;
	}

	// ��������
	char buff[1024];
	sockaddr_in addr;
	int nLen = sizeof(addr);
	while(TRUE)
	{
		int nRecv = ::recvfrom(s, buff, 1024, 0, (sockaddr*)&addr, &nLen); 
		if(nRecv > 0)
		{
			buff[nRecv] = '\0';
			printf(" ���յ����ݣ�%s����%s", ::inet_ntoa(addr.sin_addr), buff);
		}
	}
	::closesocket(s);
}