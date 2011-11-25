//////////////////////////////////////////////////////////////
// InstLSP.cpp�ļ�

// �����˰�װLSP�Ĵ���

// InstallProvider(WCHAR *pwszPathName) ��ָ��LSP�ṩ�߰�װ��TCP UDP ��ԭʼ�׽���֮��
// RemoveProvider()						�Ƴ�InstallProvider������װ��LSP

#define UNICODE
#define _UNICODE

#include <Ws2spi.h>
#include <Sporder.h>		// ������WSCWriteProviderOrder����

#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Rpcrt4.lib")	// ʵ����UuidCreate����


// Ҫ��װ��LSP��Ӳ���룬���Ƴ���ʱ��Ҫʹ����
GUID  ProviderGuid = {0xd3c21122, 0x85e1, 0x48f3, 
							{0x9a,0xb6,0x23,0xd9,0x0c,0x73,0x07,0xef}};



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

BOOL InstallProvider(WCHAR *pwszPathName)
{
	WCHAR wszLSPName[] = L"PhoenixLSP";
	LPWSAPROTOCOL_INFOW pProtoInfo;
	int nProtocols;
	WSAPROTOCOL_INFOW OriginalProtocolInfo[3];
	DWORD			 dwOrigCatalogId[3];
	int nArrayCount = 0;

	DWORD dwLayeredCatalogId;		// ���Ƿֲ�Э���Ŀ¼ID��

	int nError;
	
		// �ҵ����ǵ��²�Э�飬����Ϣ����������
	// ö�����з�������ṩ��
	pProtoInfo = GetProvider(&nProtocols);
	BOOL bFindUdp = FALSE;
	BOOL bFindTcp = FALSE;
	BOOL bFindRaw = FALSE;
	int i=0;
	for(i; i<nProtocols; i++)
	{
		if(pProtoInfo[i].iAddressFamily == AF_INET)
		{
		if(!bFindUdp && pProtoInfo[i].iProtocol == IPPROTO_UDP)
			{
				memcpy(&OriginalProtocolInfo[nArrayCount], &pProtoInfo[i], sizeof(WSAPROTOCOL_INFOW));
				OriginalProtocolInfo[nArrayCount].dwServiceFlags1 = 
					OriginalProtocolInfo[nArrayCount].dwServiceFlags1 & (~XP1_IFS_HANDLES); 
				
				dwOrigCatalogId[nArrayCount++] = pProtoInfo[i].dwCatalogEntryId;

				bFindUdp = TRUE;
			}

		if(!bFindTcp && pProtoInfo[i].iProtocol == IPPROTO_TCP)
			{
				memcpy(&OriginalProtocolInfo[nArrayCount], &pProtoInfo[i], sizeof(WSAPROTOCOL_INFOW));
				OriginalProtocolInfo[nArrayCount].dwServiceFlags1 = 
					OriginalProtocolInfo[nArrayCount].dwServiceFlags1 & (~XP1_IFS_HANDLES); 
				
				dwOrigCatalogId[nArrayCount++] = pProtoInfo[i].dwCatalogEntryId;

				bFindTcp = TRUE;
			} 
		if(!bFindRaw && pProtoInfo[i].iProtocol == IPPROTO_IP)
			{
				memcpy(&OriginalProtocolInfo[nArrayCount], &pProtoInfo[i], sizeof(WSAPROTOCOL_INFOW));
				OriginalProtocolInfo[nArrayCount].dwServiceFlags1 = 
					OriginalProtocolInfo[nArrayCount].dwServiceFlags1 & (~XP1_IFS_HANDLES); 
				
				dwOrigCatalogId[nArrayCount++] = pProtoInfo[i].dwCatalogEntryId;

				bFindRaw = TRUE;
			}
		}
	}  

		// ��װ���ǵķֲ�Э�飬��ȡһ��dwLayeredCatalogId
	// �����һ���²�Э��Ľṹ���ƹ�������
	WSAPROTOCOL_INFOW LayeredProtocolInfo;
	memcpy(&LayeredProtocolInfo, &OriginalProtocolInfo[0], sizeof(WSAPROTOCOL_INFOW));
	// �޸�Э�����ƣ����ͣ�����PFL_HIDDEN��־
	wcscpy(LayeredProtocolInfo.szProtocol, wszLSPName);
	LayeredProtocolInfo.ProtocolChain.ChainLen = LAYERED_PROTOCOL; // 0;
	LayeredProtocolInfo.dwProviderFlags |= PFL_HIDDEN;
	// ��װ
	if(::WSCInstallProvider(&ProviderGuid, 
					pwszPathName, &LayeredProtocolInfo, 1, &nError) == SOCKET_ERROR)
	{
		return FALSE;
	}
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
	for(i=0; i<nArrayCount; i++)
	{
		swprintf(wszChainName, L"%ws over %ws", wszLSPName, OriginalProtocolInfo[i].szProtocol);
		wcscpy(OriginalProtocolInfo[i].szProtocol, wszChainName);
		if(OriginalProtocolInfo[i].ProtocolChain.ChainLen == 1)
		{
			OriginalProtocolInfo[i].ProtocolChain.ChainEntries[1] = dwOrigCatalogId[i];
		}
		else
		{
			for(int j = OriginalProtocolInfo[i].ProtocolChain.ChainLen; j>0; j--)
			{
				OriginalProtocolInfo[i].ProtocolChain.ChainEntries[j] 
									= OriginalProtocolInfo[i].ProtocolChain.ChainEntries[j-1];
			}
		}
		OriginalProtocolInfo[i].ProtocolChain.ChainLen ++;
		OriginalProtocolInfo[i].ProtocolChain.ChainEntries[0] = dwLayeredCatalogId;	
	}
	// ��ȡһ��Guid����װ֮
	GUID ProviderChainGuid;
	if(::UuidCreate(&ProviderChainGuid) == RPC_S_OK)
	{
		if(::WSCInstallProvider(&ProviderChainGuid, 
					pwszPathName, OriginalProtocolInfo, nArrayCount, &nError) == SOCKET_ERROR)
		{
			return FALSE;	
		}
	}
	else
		return FALSE;

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
	if((nError = ::WSCWriteProviderOrder(dwIds, nIndex)) != ERROR_SUCCESS)
	{
		return FALSE;
	}
	FreeProvider(pProtoInfo);

	return TRUE;
}

BOOL RemoveProvider()
{
	LPWSAPROTOCOL_INFOW pProtoInfo;
	int nProtocols;
	DWORD dwLayeredCatalogId;

	// ����Guidȡ�÷ֲ�Э���Ŀ¼ID��
	pProtoInfo = GetProvider(&nProtocols);
	int nError;
	int i=0;
	for(i; i<nProtocols; i++)
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

	return TRUE;
}


void main(int argc, char *argv[])
{
	if(argc==2)
	{
		if(strcmp(argv[1], "-install")==0)   
		{
			TCHAR szPathName[256];
			TCHAR* p;

			if(::GetFullPathName(L"LSP.dll", 256, szPathName, &p) != 0)
			{
				if(InstallProvider(szPathName))
				{
					printf(" Install successully. \n");
					return;
				}
			}
			printf(" Install failed. \n");
			return;
		}
		else if(strcmp(argv[1],"-remove")==0)  
		{
			if(RemoveProvider())
				printf(" Deinstall successully. \n");
			else
				printf(" Deinstall failed. \n");
			return;
		}
	}
	
	printf(" Usage: Instlsp [ -install �� -remove ] \n");
}
