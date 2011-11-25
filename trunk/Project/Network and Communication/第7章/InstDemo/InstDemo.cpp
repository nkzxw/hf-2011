////////////////////////////////////////////////////////
// InstDemo.cpp

#include <Ws2spi.h>
#include <Sporder.h>				// ������WSCWriteProviderOrder����

#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Rpcrt4.lib")	// ʵ����UuidCreate����


// Ҫ��װ��LSP��Ӳ���룬���Ƴ���ʱ��Ҫʹ����
GUID  ProviderGuid = {0xd3c21122, 0x85e1, 0x48f3, {0x9a,0xb6,0x23,0xd9,0x0c,0x73,0x07,0xef}};


LPWSAPROTOCOL_INFOW GetProvider(LPINT lpnTotalProtocols)
{
	DWORD dwSize = 0;
	int nError;
	LPWSAPROTOCOL_INFOW pProtoInfo = NULL;
	
	// ȡ����Ҫ�ĳ���
	if(::WSCEnumProtocols(NULL, pProtoInfo, &dwSize, &nError) == SOCKET_ERROR)
	{
		if(nError != WSAENOBUFS)
			return NULL;
	}
	
	pProtoInfo = (LPWSAPROTOCOL_INFOW)::GlobalAlloc(GPTR, dwSize);
	*lpnTotalProtocols = ::WSCEnumProtocols(NULL, pProtoInfo, &dwSize, &nError);
	return pProtoInfo;
}

void FreeProvider(LPWSAPROTOCOL_INFOW pProtoInfo)
{
	::GlobalFree(pProtoInfo);
}


// ��LSP��װ��UDPЭ���ṩ��֮��
int InstallProvider(WCHAR *wszDllPath)
{
	WCHAR wszLSPName[] = L"TinyLSP";	// ���ǵ�LSP������
	int nError = NO_ERROR;

	LPWSAPROTOCOL_INFOW pProtoInfo;
	int nProtocols;
	WSAPROTOCOL_INFOW UDPLayeredInfo, UDPChainInfo; // ����Ҫ��װ��UDP�ֲ�Э���Э����
	DWORD dwUdpOrigCatalogId, dwLayeredCatalogId;

		// ��WinsockĿ¼���ҵ�ԭ����UDPЭ������ṩ�ߣ����ǵ�LSPҪ��װ����֮��
	// ö�����з�������ṩ��
	pProtoInfo = GetProvider(&nProtocols);
	for(int i=0; i<nProtocols; i++)
	{
		if(pProtoInfo[i].iAddressFamily == AF_INET && pProtoInfo[i].iProtocol == IPPROTO_UDP)
		{
			memcpy(&UDPChainInfo, &pProtoInfo[i], sizeof(UDPLayeredInfo));
			// 
			UDPChainInfo.dwServiceFlags1 = UDPChainInfo.dwServiceFlags1 & ~XP1_IFS_HANDLES;  
			// ����ԭ�������ID
			dwUdpOrigCatalogId = pProtoInfo[i].dwCatalogEntryId;
			break;
		}
	}  

		// ���Ȱ�װ�ֲ�Э�飬��ȡһ��Winsock�ⰲ�ŵ�Ŀ¼ID�ţ���dwLayeredCatalogId
	// ֱ��ʹ���²�Э���WSAPROTOCOL_INFOW�ṹ����
	memcpy(&UDPLayeredInfo, &UDPChainInfo, sizeof(UDPLayeredInfo));
	// �޸�Э�����ƣ����ͣ�����PFL_HIDDEN��־
	wcscpy(UDPLayeredInfo.szProtocol, wszLSPName);
	UDPLayeredInfo.ProtocolChain.ChainLen = LAYERED_PROTOCOL;		// LAYERED_PROTOCOL��0
	UDPLayeredInfo.dwProviderFlags |= PFL_HIDDEN;
	// ��װ
	if(::WSCInstallProvider(&ProviderGuid, 
					wszDllPath, &UDPLayeredInfo, 1, &nError) == SOCKET_ERROR)
		return nError;
	// ����ö��Э�飬��ȡ�ֲ�Э���Ŀ¼ID��
	FreeProvider(pProtoInfo);
	pProtoInfo = GetProvider(&nProtocols);
	for(i=0; i<nProtocols; i++)
	{
		if(memcmp(&pProtoInfo[i].ProviderId, &ProviderGuid, sizeof(ProviderGuid)) == 0)
		{
			dwLayeredCatalogId = pProtoInfo[i].dwCatalogEntryId;
			break;
		}
	}

		// ��װЭ����
	// �޸�Э�����ƣ�����
	WCHAR wszChainName[WSAPROTOCOL_LEN + 1];
	swprintf(wszChainName, L"%ws over %ws", wszLSPName, UDPChainInfo.szProtocol);
	wcscpy(UDPChainInfo.szProtocol, wszChainName);
	if(UDPChainInfo.ProtocolChain.ChainLen == 1)
	{
		UDPChainInfo.ProtocolChain.ChainEntries[1] = dwUdpOrigCatalogId;
	}
	else
	{
		for(i=UDPChainInfo.ProtocolChain.ChainLen; i>0 ; i--)
		{
			UDPChainInfo.ProtocolChain.ChainEntries[i] = UDPChainInfo.ProtocolChain.ChainEntries[i-1];
		}
	}
	UDPChainInfo.ProtocolChain.ChainLen ++;
	// �����ǵķֲ�Э�����ڴ�Э�����Ķ���
	UDPChainInfo.ProtocolChain.ChainEntries[0] = dwLayeredCatalogId; 
	// ��ȡһ��Guid����װ֮
	GUID ProviderChainGuid;
	if(::UuidCreate(&ProviderChainGuid) == RPC_S_OK)
	{
		if(::WSCInstallProvider(&ProviderChainGuid, 
					wszDllPath, &UDPChainInfo, 1, &nError) == SOCKET_ERROR)
					return nError;
	}
	else
		return GetLastError();



		// ��������WinsockĿ¼�������ǵ�Э������ǰ
	// ����ö�ٰ�װ��Э��
	FreeProvider(pProtoInfo);
	pProtoInfo = GetProvider(&nProtocols);

	DWORD dwIds[20];
	int nIndex = 0;
	// ������ǵ�Э����
	for(i=0; i<nProtocols; i++)
	{
		if((pProtoInfo[i].ProtocolChain.ChainLen > 1) &&
					(pProtoInfo[i].ProtocolChain.ChainEntries[0] == dwLayeredCatalogId))
			dwIds[nIndex++] = pProtoInfo[i].dwCatalogEntryId;
	}
	// �������Э��
	for(i=0; i<nProtocols; i++)
	{
		if((pProtoInfo[i].ProtocolChain.ChainLen <= 1) ||
				(pProtoInfo[i].ProtocolChain.ChainEntries[0] != dwLayeredCatalogId))
			dwIds[nIndex++] = pProtoInfo[i].dwCatalogEntryId;
	}
	// ��������WinsockĿ¼
	nError = ::WSCWriteProviderOrder(dwIds, nIndex);

	FreeProvider(pProtoInfo);
	return nError;
}

void RemoveProvider()
{	
	LPWSAPROTOCOL_INFOW pProtoInfo;
	int nProtocols;
	DWORD dwLayeredCatalogId;

	// ����Guidȡ�÷ֲ�Э���Ŀ¼ID��
	pProtoInfo = GetProvider(&nProtocols);
	int nError;
	for(int i=0; i<nProtocols; i++)
	{
		if(memcmp(&ProviderGuid, &pProtoInfo[i].ProviderId, sizeof(ProviderGuid)) == 0)
		{
			dwLayeredCatalogId = pProtoInfo[i].dwCatalogEntryId;
			break;
		}
	}

	if(i < nProtocols)
	{
		// �Ƴ�Э����
		for(i=0; i<nProtocols; i++)
		{
			if((pProtoInfo[i].ProtocolChain.ChainLen > 1) &&
					(pProtoInfo[i].ProtocolChain.ChainEntries[0] == dwLayeredCatalogId))
			{
				::WSCDeinstallProvider(&pProtoInfo[i].ProviderId, &nError);
			}
		}
		// �Ƴ��ֲ�Э��
		::WSCDeinstallProvider(&ProviderGuid, &nError);
	}
}



////////////////////////////////////////////////////

int binstall = 0;
void main()
{
	if(binstall)
	{
		if(InstallProvider(L"lsp.dll") == ERROR_SUCCESS)
		{
			printf(" Install successully \n");
		}
		else
		{
			printf(" Install failed \n");
		}
	}
	else
		RemoveProvider();
}