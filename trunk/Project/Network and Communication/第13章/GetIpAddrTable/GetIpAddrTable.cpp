//////////////////////////////////////////////////////////////////////
// GetIpAddrTable.cpp�ļ�


#include <stdio.h>
#include <windows.h>
#include <Iphlpapi.h>

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "WS2_32.lib")


int main()
{
		// ���ȵ���GetIpAddrTable������ȡһ��������
	PMIB_IPADDRTABLE pIPAddrTable;
	DWORD dwSize = 0;

	pIPAddrTable = (PMIB_IPADDRTABLE)::GlobalAlloc(GPTR, sizeof(MIB_IPADDRTABLE));

	// ��ȡ������ڴ�
	if(::GetIpAddrTable(pIPAddrTable, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER)
	{
		::GlobalFree(pIPAddrTable);
		pIPAddrTable = (PMIB_IPADDRTABLE)::GlobalAlloc(GPTR, dwSize);
	}

	// �ٴε���GetIpAddrTable��ȡʵ��������Ҫ������
	if(::GetIpAddrTable(pIPAddrTable, &dwSize, FALSE) == NO_ERROR)
	{
		// ��ӡ����������Ϣ
		printf(" Address: %ld\n", pIPAddrTable->table[0].dwAddr);
		printf(" Mask:    %ld\n", pIPAddrTable->table[0].dwMask);
		printf(" Index:   %ld\n", pIPAddrTable->table[0].dwIndex);
		printf(" BCast:   %ld\n", pIPAddrTable->table[0].dwBCastAddr);
		printf(" Reasm:   %ld\n", pIPAddrTable->table[0].dwReasmSize);
	}	
	else
	{
		printf(" GetIpAddrTable() failed \n");
	}
	
	::GlobalFree(pIPAddrTable);
	
	// ���ǽ�Ҫ��ӵ�IP��mast
	UINT iaIPAddress;
	UINT imIPMask;
	
	iaIPAddress = inet_addr("192.168.0.27");
	imIPMask = inet_addr("255.255.255.0");
	
	// ���صľ��
	ULONG NTEContext = 0;
	ULONG NTEInstance = 0;
	
	// ���һ�����������IP��ַ
	DWORD dwRet;
	dwRet = ::AddIPAddress(iaIPAddress, imIPMask, 
				pIPAddrTable->table[0].dwIndex, &NTEContext, &NTEInstance);
	if(dwRet == NO_ERROR) 
	{
		printf(" IP address added.\n");
	}
	else 
	{
		printf(" AddIPAddress failed. \n");
		LPVOID lpMsgBuf;
		// ����ʧ�ܣ���ӡ��Ϊʲôʧ��
		if (::FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dwRet,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL )) 
		{
			printf(" Error: %s ", lpMsgBuf);
		}
		::LocalFree(lpMsgBuf);
	}
	
	// ɾ�������ڵ�һ������������ӵ�IP��ַ
	dwRet = ::DeleteIPAddress(NTEContext);
	if(dwRet == NO_ERROR) 
	{
		printf(" IP Address Deleted.\n");
	}
	else 
	{		
		printf(" DeleteIPAddress failed.\n");
	}
	
	return 0;
}