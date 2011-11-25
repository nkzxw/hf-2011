///////////////////////////////////////////
// rawudp.cpp�ļ�

#include "../common/initsock.h"
#include "../common/protoinfo.h"
#include "../common/comm.h"

#include <stdio.h>
#include <ws2tcpip.h>	// ������IP_HDRINCL

CInitSock theSock;


/* 
����UDPαͷУ��͡�UDPУ��ͻ������¼�����
	ԴIP��ַ
	Ŀ��IP��ַ
	8λ0��
	8λЭ����
	16λUDP����
	16λԴ�˿ں�
	16λĿ�Ķ˿ں�
	16λUDP�������
	16λUDPУ��ͣ�0��
	UDP����
 */
void ComputeUdpPseudoHeaderChecksum(
    IPHeader    *pIphdr,
    UDPHeader *pUdphdr,
    char    *payload,
    int      payloadlen
    )
{
	char buff[1024];
	char *ptr = buff;
	int chksumlen = 0;
	ULONG zero = 0;
	
	// ����ԴIP��ַ��Ŀ��IP��ַ
	memcpy(ptr, &pIphdr->ipSource, sizeof(pIphdr->ipSource));
	ptr += sizeof(pIphdr->ipSource);
	chksumlen += sizeof(pIphdr->ipSource);

	memcpy(ptr, &pIphdr->ipDestination, sizeof(pIphdr->ipDestination));
	ptr += sizeof(pIphdr->ipDestination);
	chksumlen += sizeof(pIphdr->ipDestination);

	// ����8λ0��
	memcpy(ptr, &zero, 1);
	ptr += 1;
	chksumlen += 1;

	// Э��
	memcpy(ptr, &pIphdr->ipProtocol, sizeof(pIphdr->ipProtocol));
	ptr += sizeof(pIphdr->ipProtocol);
	chksumlen += sizeof(pIphdr->ipProtocol);

	// UDP����
	memcpy(ptr, &pUdphdr->len, sizeof(pUdphdr->len));
	ptr += sizeof(pUdphdr->len);
	chksumlen += sizeof(pUdphdr->len);

	// UDPԴ�˿ں�
	memcpy(ptr, &pUdphdr->sourcePort, sizeof(pUdphdr->sourcePort));
	ptr += sizeof(pUdphdr->sourcePort);
	chksumlen += sizeof(pUdphdr->sourcePort);

	// UDPĿ�Ķ˿ں�
	memcpy(ptr, &pUdphdr->destinationPort, sizeof(pUdphdr->destinationPort));
	ptr += sizeof(pUdphdr->destinationPort);
	chksumlen += sizeof(pUdphdr->destinationPort);

	// ����UDP����
	memcpy(ptr, &pUdphdr->len, sizeof(pUdphdr->len));
	ptr += sizeof(pUdphdr->len);
	chksumlen += sizeof(pUdphdr->len);

	// 16λ��UDPУ��ͣ���Ϊ0
	memcpy(ptr, &zero, sizeof(USHORT));
	ptr += sizeof(USHORT);
	chksumlen += sizeof(USHORT);

	// ����
	memcpy(ptr, payload, payloadlen);
	ptr += payloadlen;
	chksumlen += payloadlen;

	// ���뵽��һ��16λ�߽�
	for(int i=0; i<payloadlen%2; i++)
	{
		*ptr = 0;
		ptr++;
		chksumlen++;
	}
	// �������У��ͣ��������䵽UDPͷ
	pUdphdr->checksum = checksum((USHORT*)buff, chksumlen);
}


int main()
{
	// ���������Ϣ
	char szDestIp[] = "10.16.115.88";	  //  <<== ��дĿ��IP��ַ
	char szSourceIp[] = "127.0.0.1";      //  <<== ��д���Լ���IP��ַ

	USHORT nDestPort = 4567;
	USHORT nSourcePort = 8888;
	char szMsg[] = "This is a test \r\n";
	int nMsgLen = strlen(szMsg);

	// ����ԭʼ�׽���
	SOCKET sRaw = ::socket(AF_INET, SOCK_RAW, IPPROTO_UDP);

	// ��ЧIPͷ����ѡ��
	BOOL bIncl = TRUE;
	::setsockopt(sRaw, IPPROTO_IP, IP_HDRINCL, (char *)&bIncl, sizeof(bIncl));

	char buff[1024] = { 0 };

	// IPͷ
	IPHeader *pIphdr = (IPHeader *)buff;
	pIphdr->iphVerLen = (4<<4 | (sizeof(IPHeader)/sizeof(ULONG)));

	pIphdr->ipLength = ::htons(sizeof(IPHeader) + sizeof(UDPHeader) + nMsgLen);
	pIphdr->ipTTL = 128;
	pIphdr->ipProtocol = IPPROTO_UDP;
	pIphdr->ipSource = ::inet_addr(szSourceIp);
	pIphdr->ipDestination = ::inet_addr(szDestIp);
	pIphdr->ipChecksum = checksum((USHORT*)pIphdr, sizeof(IPHeader));

	// UDPͷ
	UDPHeader *pUdphdr = (UDPHeader *)&buff[sizeof(IPHeader)];
	pUdphdr->sourcePort = htons(8888);
	pUdphdr->destinationPort = htons(nDestPort);
	pUdphdr->len = htons(sizeof(UDPHeader) + nMsgLen);
	pUdphdr->checksum = 0;

	char *pData = &buff[sizeof(IPHeader) + sizeof(UDPHeader)];
	memcpy(pData, szMsg, nMsgLen);

	ComputeUdpPseudoHeaderChecksum(pIphdr, pUdphdr, pData, nMsgLen);

	// ����Ŀ�ĵ�ַ
	SOCKADDR_IN destAddr = { 0 };
	destAddr.sin_family = AF_INET;
	destAddr.sin_port = htons(nDestPort);
	destAddr.sin_addr.S_un.S_addr = ::inet_addr(szDestIp);

	// ����ԭʼUDP���
	int nRet;
	for(int i=0; i<5; i++)
	{
		nRet = ::sendto(sRaw, buff, 
			sizeof(IPHeader) + sizeof(UDPHeader) + nMsgLen, 0, (sockaddr*)&destAddr, sizeof(destAddr));
		if(nRet == SOCKET_ERROR)
		{
			printf(" sendto() failed: %d \n", ::WSAGetLastError());
			break;
		}
		else
		{
			printf(" sent %d bytes \n", nRet);
		}
	}

	::closesocket(sRaw);

	getchar();
	return 0;
}



