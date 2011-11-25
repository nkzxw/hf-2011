//////////////////////////////////////////////////////////////////////
// GetInterfaceInfo.cpp�ļ�


#include <stdio.h>
#include <windows.h>
#include <Iphlpapi.h>

#pragma comment(lib, "Iphlpapi.lib")


int main()
{
	PIP_INTERFACE_INFO pInfo = 
			(PIP_INTERFACE_INFO)::GlobalAlloc(GPTR, sizeof(IP_INTERFACE_INFO));
	ULONG ulOutBufLen = sizeof(IP_INTERFACE_INFO);

	// �������������ڴ治���Ļ�������������
	if(::GetInterfaceInfo(pInfo, &ulOutBufLen) == ERROR_INSUFFICIENT_BUFFER)
	{
		::GlobalFree(pInfo);
		pInfo = (PIP_INTERFACE_INFO)::GlobalAlloc(GPTR, ulOutBufLen);
	}

	// �ٴε���GetInterfaceInfo����ȡ����ʵ����Ҫ������
	if(::GetInterfaceInfo(pInfo, &ulOutBufLen) == NO_ERROR)
	{
		printf(" \tAdapter Name: %ws\n", pInfo->Adapter[0].Name);
		printf(" \tAdapter Index: %ld\n", pInfo->Adapter[0].Index);
		printf(" \tNum Adapters: %ld\n", pInfo->NumAdapters);
	}
	else
	{
		printf(" GetInterfaceInfo() failed \n");
	}

	::GlobalFree(pInfo);
	return 0;
}