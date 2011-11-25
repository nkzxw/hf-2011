/////////////////////////////////////////////////
// routetracer.cpp�ļ�



#include "../common/initsock.h"
#include "../common/protoinfo.h"
#include "../common/comm.h"

#include <stdio.h>


CInitSock theSock;

typedef struct icmp_hdr
{
    unsigned char   icmp_type;		// ��Ϣ����
    unsigned char   icmp_code;		// ����
    unsigned short  icmp_checksum;	// У���
	// �����ǻ���ͷ
    unsigned short  icmp_id;		// ����Ωһ��ʶ�������ID�ţ�ͨ������Ϊ����ID
    unsigned short  icmp_sequence;	// ���к�
    unsigned long   icmp_timestamp; // ʱ���
} ICMP_HDR, *PICMP_HDR;



void main()
{
	char *szDestIp = "10.16.115.178"; // 210.181.18.12910.16.115.25 61.55.66.30

	char recvBuf[1024] = { 0 };

	// �������ڽ���ICMP�����ԭʼ�׽��֣��󶨵����ض˿�
	SOCKET sRaw = ::socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	sockaddr_in in;
	in.sin_family = AF_INET;
	in.sin_port = 0;
	in.sin_addr.S_un.S_addr = INADDR_ANY;
	if(::bind(sRaw, (sockaddr*)&in, sizeof(in)) == SOCKET_ERROR)
	{
		printf(" bind() failed \n");
		return;
	}

	SetTimeout(sRaw, 5*1000);

	// �������ڷ���UDP������׽���
	SOCKET sSend = ::socket(AF_INET, SOCK_DGRAM, 0);	
	
	SOCKADDR_IN destAddr;
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = ::htons(22);
	destAddr.sin_addr.S_un.S_addr = ::inet_addr(szDestIp);


	int nTTL = 1;
	int nRet;
	ICMP_HDR *pICMPHdr;
	int nTick;	
	SOCKADDR_IN recvAddr;
	do
	{
		// ����UDP�����TTLֵ
		SetTTL(sSend, nTTL);
		nTick = ::GetTickCount();

		// �������UDP���
		nRet = ::sendto(sSend, "hello", 5, 0, (sockaddr*)&destAddr, sizeof(destAddr));
		if(nRet == SOCKET_ERROR)
		{
			printf(" sendto() failed \n");
			break;
		}


		// �ȴ�����·�������ص�ICMP����
		int nLen = sizeof(recvAddr);
		nRet = ::recvfrom(sRaw, recvBuf, 1024, 0, (sockaddr*)&recvAddr, &nLen);
		if(nRet == SOCKET_ERROR)
		{
			if(::WSAGetLastError() == WSAETIMEDOUT)
			{
				printf(" time out \n");
				break;
			}
			else
			{
				printf(" recvfrom() failed \n");
				break;
			}
		}

		// �������յ���ICMP����
		pICMPHdr = (ICMP_HDR*)&recvBuf[20]; // sizeof(IPHeader)

		if(pICMPHdr->icmp_type != 11 && pICMPHdr->icmp_type != 3 && pICMPHdr->icmp_code != 3)
		{
			printf(" Unexpected Type: %d , code: %d \n",
				pICMPHdr->icmp_type, pICMPHdr->icmp_code);
		}
		else
		{
			char *szIP = ::inet_ntoa(recvAddr.sin_addr);
			
			printf(" ��%d��·������IP��ַ��%s \n", nTTL, szIP);
			printf("      ��ʱ��%d���� \n", ::GetTickCount() - nTick);
		}
		
		if(destAddr.sin_addr.S_un.S_addr == recvAddr.sin_addr.S_un.S_addr)
		{	
			printf("Ŀ��ɴ� \n");
			break;
		}

		printf("//------------------------------------// \n");

	}while(nTTL++ < 20);

	::closesocket(sRaw);
	::closesocket(sSend);
}


/*
		in_addr addr;
		addr.S_un.S_addr = pIPHdr->ipSource;

		addr.S_un.S_addr = pIPHdr->ipDestination;
		printf(" dest addr : %s \n", ::inet_ntoa(addr));

			if(pIPHdr->ipSource != recvAddr.sin_addr.S_un.S_addr)
		{
			printf(" error ip addr \n");
		}
			
			
			
			
			
			
			
					pIPHdr = (IPHeader*)recvBuf;
		int nHdrLen = (pIPHdr->iphVerLen & 0x0F)*4;  // ���ֽ�
			
			*/