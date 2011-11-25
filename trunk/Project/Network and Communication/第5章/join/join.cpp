////////////////////////////////////
// join.cpp�ļ�

#include "Initsock.h"

#include <stdio.h>
#include <windows.h>
#include <Ws2tcpip.h>



// ��ʼ��Winsock��
CInitSock theSock;

void main()
{
	SOCKET s = ::socket(AF_INET, SOCK_DGRAM, 0);

	// ������������ʹ�ð󶨵ĵ�ַ
	BOOL bReuse = TRUE;
	::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&bReuse, sizeof(BOOL));


	// �󶨵�4567�˿�
	sockaddr_in si;
	si.sin_family = AF_INET;
	si.sin_port = ::ntohs(4567);
	si.sin_addr.S_un.S_addr = INADDR_ANY;
	::bind(s, (sockaddr*)&si, sizeof(si));
	
	// ����ಥ��
	ip_mreq	mcast;
	mcast.imr_interface.S_un.S_addr = INADDR_ANY;
	mcast.imr_multiaddr.S_un.S_addr = ::inet_addr("234.5.6.7");  // �ಥ��ַΪ234.5.6.7
	::setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mcast, sizeof(mcast));


	// ���նಥ������
	printf(" ��ʼ���նಥ��234.5.6.7�ϵ�����... \n");
	char buf[1280];
	int nAddrLen = sizeof(si);
	while(TRUE)
	{
		int nRet = ::recvfrom(s, buf, strlen(buf), 0, (sockaddr*)&si, &nAddrLen);
		if(nRet != SOCKET_ERROR)
		{
			buf[nRet] = '\0';
			printf(buf);
		}
		else
		{
			int n = ::WSAGetLastError();
			break;
		}
	}
}



/*

	SOCKET	s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	// ���ؽӿ�
	SOCKADDR_IN	localif;
	localif.sin_family = AF_INET;
	localif.sin_port   = htons(5150);
	localif.sin_addr.s_addr = htonl(INADDR_ANY);
	::bind(s, (SOCKADDR *)&localif, sizeof(localif));
	
	// ����ip_mreq_source�ṹ
	struct ip_mreq_source mreqsrc;
	mreqsrc.imr_interface.s_addr = inet_addr("192.168.0.46");
	mreqsrc.imr_multiaddr.s_addr = inet_addr("234.5.6.7");
	
	
	// ���Դ��ַ218.12.255.113
	mreqsrc.imr_sourceaddr.s_addr = inet_addr("218.12.255.113");
	::setsockopt(s, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, (char *)&mreqsrc, sizeof(mreqsrc));
	// ���Դ��ַ
	mreqsrc.imr_sourceaddr.s_addr = inet_addr("218.12.174.222");
	::setsockopt(s, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, (char *)&mreqsrc, sizeof(mreqsrc));




  */