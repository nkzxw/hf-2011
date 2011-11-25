//////////////////////////////////////////////////////////
// RawEthernet.cpp�ļ�


#include <windows.h>
#include <winioctl.h>
#include <ntddndis.h>
#include <stdio.h>
#include "protoutils.h"

int main()
{
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
	
	// ����һ��ԭʼ���������ӦΪ16���ֽڳ���
	BYTE bytes[] =  {0xff,0xff,0xff,0xff,0xff,0xff,  // Ŀ��MAC��ַ
					 0x00,0x02,0x3e,0x4c,0x49,0xaa,  // ԴMAC��ַ
					 0x08,0x00,					     // Э��
					 0x01,0x02,0x03,0x04,0x05,0x06}; // ͨ������
	// ��ӡ��ÿ���²�����������Ϣ����������
	for(int i=0; i<adapters.m_nAdapters; i++)
	{
		char sz[256];
		wsprintf(sz, "\n\n Adapter:	%ws \n Symbolic Link: %ws \n\n ", 
								adapters.m_pwszAdapterName[i], adapters.m_pwszSymbolicLink[i]);
		printf(sz);

		CAdapter adapter;
		adapter.OpenAdapter(adapters.m_pwszSymbolicLink[i]);
		// �ڴ��������Ϸ���ԭʼ����
		int nSend = adapter.SendData(bytes, sizeof(bytes));
		if(nSend > 0)
			printf(" Packet sent: %d bytes \n", nSend);
		else
			printf(" Packet sent failed \n");
	}

	::CloseHandle(hControlDevice);
	ProtoStopService();
	return 0;
}



