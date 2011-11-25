//////////////////////////////////////////////////////////////////////
// GetNetworkParams.cpp�ļ�


#include <stdio.h>
#include <windows.h>
#include <Iphlpapi.h>

#pragma comment(lib, "Iphlpapi.lib")


int main()
{
	FIXED_INFO fi;
	ULONG ulOutBufLen = sizeof(fi);

	// ��ȡ���ص��Ե��������
	if(::GetNetworkParams(&fi, &ulOutBufLen) != ERROR_SUCCESS)
	{
		printf(" GetNetworkParams() failed \n");
		return -1;
	}

	// ��������
	printf(" Host Name: %s \n", fi.HostName);

	// ����ע�������
	printf(" Domain Name: %s \n", fi.DomainName);

	// ��ӡ�����е�DNS������
	printf(" DNS Servers: \n");
	printf(" \t%s \n", fi.DnsServerList.IpAddress.String);
	IP_ADDR_STRING *pIPAddr = fi.DnsServerList.Next;
	while(pIPAddr != NULL)
	{
		printf(" \t%s \n", pIPAddr->IpAddress.String);
		pIPAddr = pIPAddr->Next;
	}

	return 0;
}

