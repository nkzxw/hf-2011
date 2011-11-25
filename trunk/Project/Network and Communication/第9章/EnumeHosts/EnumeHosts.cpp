////////////////////////////////////////////////
// EnumeHosts.cpp�ļ�

#include "../common/initsock.h"

#include <windows.h>
#include <stdio.h>
#include <Iphlpapi.h>

#include "protoutils.h"
#include "ProtoPacket.h"


#pragma comment(lib, "Iphlpapi.lib")


DWORD WINAPI SendThread(LPVOID lpParam);
BOOL GetGlobalData();

/////////////////////////////////////////
// ȫ������
u_char	g_ucLocalMac[6];	// ����MAC��ַ
DWORD	g_dwGatewayIP;		// ����IP��ַ
DWORD	g_dwLocalIP;		// ����IP��ַ
DWORD	g_dwMask;			// ��������

CInitSock theSock;

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
	if(!adapter.OpenAdapter(adapters.m_pwszSymbolicLink[0], TRUE))
	{
		printf(" OpenAdapter failed \n");
		ProtoStopService();
		return -1;
	}

	CArpPacket arp(&adapter);

	////////////////////////////////////////////////////////////////////////
	// ���濪ʼ����LANɨ��

	// �ṩ��ǰ�̣߳��������ݵ��̣߳������ȼ������ⶪʧ������������
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	// ���������̣߳���ʼ����ARP����
	::CloseHandle(::CreateThread(NULL, 0, SendThread, &arp, 0, NULL));

	// ����ARPӦ��
	u_char *p;
	PARPHeader pArph;
	printf(" ��ʼɨ��LAN�еļ����... \n");
	while(TRUE)
	{
		pArph = arp.WaitReply();
		if(pArph != NULL)
		{

			printf(" \n ------------------------------------------- \n");

			p = pArph->smac;
			printf("	Mac��ַ:	%02X-%02X-%02X-%02X-%02X-%02X \n", p[0], p[1], p[2], p[3], p[4], p[5]);
			p = (u_char*)&pArph->saddr;
			printf("	 IP��ַ:	%d.%d.%d.%d  \n ", p[0], p[1], p[2], p[3]);
	//		HOSTENT *pHost = ::gethostbyaddr((char*)&pArph->saddr, 4, AF_INET);
	//		if(pHost != NULL)
	//			printf("	 ������:	%s \n", pHost->h_name);
		}
		else
		{
			break;
		}
	}
	printf("\n\n ɨ����� \n");

	ProtoStopService();

	getchar();

	return 0;
}


DWORD WINAPI SendThread(LPVOID lpParam)
{
	CArpPacket *pArp = (CArpPacket *)lpParam;
	// ȡ��LAN��IP��ַ�ռ�Ĵ�С
	DWORD dwMask = ::ntohl(g_dwMask);
	int nMaxHosts = ~dwMask;

	// ������̫ͷ��Ŀ��MAC��ַ������ARP����ʱ��Ӧ�ý�����Ϊ�㲥��ַ
	u_char destmacEther[6];
	memset(destmacEther, 0xff, 6);
	// ARPͷ�е�Ŀ��MAC��ַ
	u_char destmacArp[6] = { 0 }; 
	// ���ַ�ռ��е�ÿ��IP��ַ����ARP����
	DWORD dwTemp = ::ntohl(g_dwGatewayIP & g_dwMask) + 1;
	for(int i=1; i<nMaxHosts; i++, dwTemp++)
	{
		DWORD dwIP = ::htonl(dwTemp);
		if(dwIP != g_dwLocalIP)
		{
			if(!pArp->SendPacket(destmacEther, g_ucLocalMac, ARPOP_REQUEST, 
										destmacArp, dwIP, g_ucLocalMac, g_dwLocalIP))
			{
				printf(" SendPacket() failed \n");
				break;
			}
		}
	}
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
