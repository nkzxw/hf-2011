/////////////////////////////////////////
// sender.cpp�ļ�

#include "initsock.h"
#include "stdio.h"
#include <windows.h>

CInitSock theSock;

void main()
{
	SOCKET s = ::socket(AF_INET, SOCK_DGRAM, 0);
	// ��ЧSO_BROADCASTѡ��
	BOOL bBroadcast = TRUE;
	::setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&bBroadcast, sizeof(BOOL));

	// ���ù㲥��ַ������Ĺ㲥�˿ںţ���̨����4567
	SOCKADDR_IN bcast;
	bcast.sin_family = AF_INET;
	bcast.sin_addr.s_addr = INADDR_BROADCAST; // ::inet_addr("255.255.255.255");
	bcast.sin_port = htons(4567);

	// ���͹㲥
	printf(" ��ʼ��4567�˿ڷ��͹㲥����... \n \n");
	char sz[] = "This is just a test. \r\n";
	while(TRUE)
	{
		::sendto(s, sz, strlen(sz), 0, (sockaddr*)&bcast, sizeof(bcast));
		::Sleep(5000);
	}
}


/*

	::send(s, sz, strlen(sz), 0);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = ntohs(3333);
	addr.sin_addr.S_un.S_addr  = ADDR_ANY;

	if(::bind(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		printf(" bind() failed \n");
		return;
	}





  */