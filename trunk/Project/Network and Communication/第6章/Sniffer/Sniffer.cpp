////////////////////////////////////////////
// Sniffer.cpp�ļ�

#include "../common/initsock.h"
#include "../common/protoinfo.h" 

#include <stdio.h>
#include <mstcpip.h>

#pragma comment(lib, "Advapi32.lib")

CInitSock theSock;

void DecodeTCPPacket(char *pData)
{
	TCPHeader *pTCPHdr = (TCPHeader *)pData;

	printf(" Port: %d -> %d \n", ntohs(pTCPHdr->sourcePort), ntohs(pTCPHdr->destinationPort));
	
	// ���滹���Ը���Ŀ�Ķ˿ںŽ�һ������Ӧ�ò�Э��
	switch(::ntohs(pTCPHdr->destinationPort))
	{
	case 21:
		break;
	case 80:
	case 8080:
		break;
	}
}

void DecodeIPPacket(char *pData)
{
	IPHeader *pIPHdr = (IPHeader*)pData;	
	in_addr source, dest;
	char szSourceIp[32], szDestIp[32]; 

	printf("\n\n-------------------------------\n");

	// ��IPͷ��ȡ��ԴIP��ַ��Ŀ��IP��ַ
	source.S_un.S_addr = pIPHdr->ipSource;
	dest.S_un.S_addr = pIPHdr->ipDestination;
	strcpy(szSourceIp, ::inet_ntoa(source));
	strcpy(szDestIp, ::inet_ntoa(dest));

	printf("	%s -> %s \n", szSourceIp, szDestIp);
	// IPͷ����
	int nHeaderLen = (pIPHdr->iphVerLen & 0xf) * sizeof(ULONG);

	switch(pIPHdr->ipProtocol)
	{
	case IPPROTO_TCP: // TCPЭ��
		DecodeTCPPacket(pData + nHeaderLen);
		break;
	case IPPROTO_UDP:
		break;
	case IPPROTO_ICMP:
		break; 
	}
}


void main()
{
	// ����ԭʼ�׽���
	SOCKET sRaw = socket(AF_INET, SOCK_RAW, IPPROTO_IP);

	// ��ȡ����IP��ַ
	char szHostName[56];
	SOCKADDR_IN addr_in;
	struct  hostent *pHost;
	gethostname(szHostName, 56);
	if((pHost = gethostbyname((char*)szHostName)) == NULL)	
		return ;

	// �ڵ���ioctl֮ǰ���׽��ֱ����
	addr_in.sin_family  = AF_INET;
	addr_in.sin_port    = htons(0);
	memcpy(&addr_in.sin_addr.S_un.S_addr, pHost->h_addr_list[0], pHost->h_length);

	printf(" Binding to interface : %s \n", ::inet_ntoa(addr_in.sin_addr));
	if(bind(sRaw, (PSOCKADDR)&addr_in, sizeof(addr_in)) == SOCKET_ERROR)
		return;

	// ����SIO_RCVALL���ƴ��룬�Ա�������е�IP��	
	DWORD dwValue = 1;
	if(ioctlsocket(sRaw, SIO_RCVALL, &dwValue) != 0)	
		return ;
	
	// ��ʼ���շ��
	char buff[1024];
	int nRet;
	while(TRUE)
	{
		nRet = recv(sRaw, buff, 1024, 0);
		if(nRet > 0)
		{
			DecodeIPPacket(buff);
		}
	}
	closesocket(sRaw);
}

