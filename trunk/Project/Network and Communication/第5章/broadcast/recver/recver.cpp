/////////////////////////////////////////////////////
// recver.cpp�ļ�

#include "initsock.h"
#include "stdio.h"
#include <windows.h>

CInitSock theSock;

void main()
{
	SOCKET s = ::socket(AF_INET, SOCK_DGRAM, 0);

	// ����Ҫ��һ�����ص�ַ��ָ���㲥�˿ں�
	SOCKADDR_IN sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.S_un.S_addr = INADDR_ANY;	
	sin.sin_port = ::ntohs(4567);
	if(::bind(s, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		printf(" bind() failed \n");
		return;
	}

	// ���չ㲥
	printf(" ��ʼ��4567�˿ڽ��չ㲥����... \n\n");
	SOCKADDR_IN addrRemote;	
	int nLen = sizeof(addrRemote);
	char sz[256];
	while(TRUE)
	{
		int nRet = ::recvfrom(s, sz, 256, 0, (sockaddr*)&addrRemote, &nLen);
		if(nRet > 0)
		{
			sz[nRet] = '\0';
			printf(sz);
		}
	}
}



