////////////////////////////////////////////////
// ScannerDemo.cpp�ļ�

#include "../common/initsock.h"

#include <windows.h>
#include <stdio.h>
#include "ntddndis.h"
#include "protoutils.h"
#include "ProtoPacket.h"

#include "Iphlpapi.h"
#pragma comment(lib, "Iphlpapi.lib")

#include "../common/comm.h"


DWORD WINAPI SendThread(LPVOID lpParam);
BOOL GetGlobalData();

/////////////////////////////////////////
// ȫ������
u_char	g_ucLocalMac[6];	// ����MAC��ַ
DWORD	g_dwGatewayIP;		// ����IP��ַ
DWORD	g_dwLocalIP;		// ����IP��ַ
DWORD	g_dwMask;			// ��������

CInitSock theSock;




 /* 
����TCPαͷУ��͡�TCPУ��ͻ������¼�����
	ԴIP��ַ
	Ŀ��IP��ַ
	8λ0��
	8λЭ����
	16λTCP����
	TCPͷ
	TCP����
 */


void ComputeTcpPseudoHeaderChecksum(
    IPHeader    *pIphdr,
    TCPHeader *pTcphdr,
    char    *payload,
    int      payloadlen
    )
{
	char buff[1024];
	char *ptr = buff;
	int chksumlen = 0;
	ULONG zero = 0;
	
		// αͷ
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

	// TCP����
	USHORT tcp_len = htons(sizeof(TCPHeader) + payloadlen);
	memcpy(ptr, &tcp_len, sizeof(tcp_len));
	ptr += sizeof(tcp_len);
	chksumlen += sizeof(tcp_len);

		// TCPͷ
	memcpy(ptr, pTcphdr, sizeof(TCPHeader));
	ptr += sizeof(TCPHeader);
	chksumlen += sizeof(TCPHeader);

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

	// �������У��ͣ��������䵽TCPͷ
	pTcphdr->checksum = checksum((USHORT*)buff, chksumlen);
}

int main()
{
	// ��ȡȫ������
	GetGlobalData();
	// ��������
	if(!ProtoStartService())
	{
		printf(" ProtoStartService() failed %d \n", ::GetLastError());
		return -1;
	}
	// �򿪿����豸����
	HANDLE hControlDevice = ProtoOpenControlDevice();
	if(hControlDevice == INVALID_HANDLE_VALUE)
	{
		printf(" ProtoOpenControlDevice() failed() %d \n", ::GetLastError());
		ProtoStopService();
		return -1;
	}
	// ö�ٰ󶨵��²�������
	CPROTOAdapters adapters;
	if(!adapters.EnumAdapters(hControlDevice))
	{
		printf(" Enume adapter failed \n");
		ProtoStopService();
		return -1;
	}

	CAdapter adapter;
	// Ĭ��ʹ�õ�һ��������
	if(!adapter.OpenAdapter(adapters.m_pwszSymbolicLink[0], FALSE))
	{
		printf(" OpenAdapter failed \n");
		ProtoStopService();
		return -1;
	}
	
	adapter.SetFilter(	//  NDIS_PACKET_TYPE_PROMISCUOUS|
		NDIS_PACKET_TYPE_DIRECTED | 
			NDIS_PACKET_TYPE_MULTICAST | NDIS_PACKET_TYPE_BROADCAST);


	// Ŀ��IP��ַ��Ҫ̽��Ķ˿ں�
	char szDestIP[] = "219.238.168.74";  
	USHORT usDestPort = 80;

	DWORD dwLocalIP = g_dwLocalIP;  // ����������ʹ�üٵ�IP��ַ��MAC��ַ
	u_char *pLocalMac = g_ucLocalMac;


	// �õ����ص�MAC��ַ
	u_char arGatewayMac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	ULONG ulLen = 6;
	if(!::SendARP(g_dwGatewayIP, 0, (ULONG*)arGatewayMac, &ulLen) == NO_ERROR)
	{
		printf(" ȡ�����ص�MAC��ַ���� \n");
		return -1;
	}

	DWORD dwDestIP = ::inet_addr(szDestIP);

		// ����TCP������
	char frame[500] = { 0 };
	// ��̫ͷ
	ETHeader etHeader;
	memcpy(etHeader.dhost, arGatewayMac, 6);
	memcpy(etHeader.shost, pLocalMac, 6);
	etHeader.type = ::htons(ETHERTYPE_IP);
	memcpy(frame, &etHeader, sizeof(etHeader));

	// IPͷ
	IPHeader ipHeader = { 0 };
	ipHeader.iphVerLen = (4<<4 | (sizeof(ipHeader)/sizeof(ULONG)));

	ipHeader.ipLength = ::htons(sizeof(IPHeader) + sizeof(TCPHeader));

	ipHeader.ipID = 1;
	ipHeader.ipFlags = 0;
	ipHeader.ipTTL = 128;
	ipHeader.ipProtocol = IPPROTO_TCP;
	ipHeader.ipSource = dwLocalIP;
	ipHeader.ipDestination = dwDestIP;	
	ipHeader.ipChecksum = 0;
	ipHeader.ipChecksum = checksum((USHORT*)&ipHeader, sizeof(ipHeader));

	memcpy(&frame[sizeof(etHeader)], &ipHeader, sizeof(ipHeader));

	// TCPͷ
	TCPHeader tcpHeader = { 0 };
	tcpHeader.sourcePort = htons(6000);
	tcpHeader.destinationPort = htons(0);
	tcpHeader.sequenceNumber = htonl(55551);
	tcpHeader.acknowledgeNumber = 0;
	tcpHeader.dataoffset =  (sizeof(tcpHeader)/4<<4|0); 

	tcpHeader.flags = TCP_SYN;   // #define   TCP_SYN   0x02
	tcpHeader.urgentPointer = 0;

	tcpHeader.windows = htons(512);
	tcpHeader.checksum = 0;


	//	������̽����롣ע�⣬Ҫʵ��ɨ�軰��������ѭ��̽��˿ںż���
	{
		// �������
		tcpHeader.destinationPort = htons(usDestPort);
		ComputeTcpPseudoHeaderChecksum(&ipHeader, &tcpHeader, NULL, 0);
		memcpy(&frame[sizeof(etHeader) + sizeof(ipHeader)], &tcpHeader, sizeof(tcpHeader));

		printf(" ��ʼ̽�⡾%s:%d��... \n\n",  szDestIP, usDestPort);

		// ���ͷ��
		int nLen = sizeof(etHeader) + sizeof(ipHeader) + sizeof(tcpHeader);
		if(adapter.SendData(frame, nLen) != nLen)
		{
			printf(" SendData failed \n");
			return 0;
		}

		// ���շ��
		char buff[500] = { 0 };
		for(int i=0; i<5; i++)  // ע�⣬��Ӧ��ʹ���첽��ʽ�������ݡ�����ʹ��ѭ����Ϊ�˷���
		{
			adapter.RecieveData(buff, nLen);
			ETHeader *pEtherhdr = (ETHeader *)buff;
			if(pEtherhdr->type == ::htons(ETHERTYPE_IP))
			{
				IPHeader *pIphdr = (IPHeader *)&buff[sizeof(ETHeader)];
				if(pIphdr->ipProtocol == IPPROTO_TCP && pIphdr->ipSource == dwDestIP)
				{
					TCPHeader *pTcphdr = (TCPHeader *)&buff[sizeof(ETHeader) + sizeof(IPHeader)];
					
					if((pTcphdr->flags & TCP_SYN) && (pTcphdr->flags & TCP_ACK)) // #define   TCP_ACK   0x10
					{
						printf(" ��%s:%d��Open \n", szDestIP, usDestPort);
					}
					else
					{
						printf(" ��%s:%d��Closed \n", szDestIP, usDestPort);
					}
					break;
				}
			}
		}
	}

	ProtoStopService();

	return 0;
}

BOOL GetGlobalData()
{
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	ULONG ulLen = 0;

	// Ϊ�������ṹ�����ڴ�
	::GetAdaptersInfo(pAdapterInfo,&ulLen);
	pAdapterInfo = (PIP_ADAPTER_INFO)::GlobalAlloc(GPTR, ulLen);

	// ȡ�ñ����������ṹ��Ϣ
	if(::GetAdaptersInfo(pAdapterInfo,&ulLen) ==  ERROR_SUCCESS)
	{
		if(pAdapterInfo != NULL)
		{
			memcpy(g_ucLocalMac, pAdapterInfo->Address, 6);
			g_dwGatewayIP = ::inet_addr(pAdapterInfo->GatewayList.IpAddress.String);
			g_dwLocalIP = ::inet_addr(pAdapterInfo->IpAddressList.IpAddress.String);
			g_dwMask = ::inet_addr(pAdapterInfo->IpAddressList.IpMask.String);
		}
	}
	::GlobalFree(pAdapterInfo);

	printf(" \n -------------------- ����������Ϣ -----------------------\n\n");
	in_addr in;
	in.S_un.S_addr = g_dwLocalIP;
	printf("      IP Address : %s \n", ::inet_ntoa(in));

	in.S_un.S_addr = g_dwMask;
	printf("     Subnet Mask : %s \n", ::inet_ntoa(in));

	in.S_un.S_addr = g_dwGatewayIP;
	printf(" Default Gateway : %s \n", ::inet_ntoa(in));

	u_char *p = g_ucLocalMac;
	printf("     MAC Address : %02X-%02X-%02X-%02X-%02X-%02X \n", p[0], p[1], p[2], p[3], p[4], p[5]);

	printf(" \n \n ");



	return TRUE;
}



