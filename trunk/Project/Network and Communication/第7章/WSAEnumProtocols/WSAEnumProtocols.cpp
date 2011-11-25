/////////////////////////////////////////////////////////
// WSAEnumProtocols.cpp�ļ�

#include "..\common\initsock.h"
#include <windows.h>
#include <stdio.h>

LPWSAPROTOCOL_INFO GetProvider(LPINT lpnTotalProtocols)
{
	DWORD dwSize = 0;
	LPWSAPROTOCOL_INFO pProtoInfo = NULL;
	
	// ȡ����Ҫ�Ļ���������
	if(::WSAEnumProtocols(NULL, pProtoInfo, &dwSize) == SOCKET_ERROR)
	{
		if(::WSAGetLastError() != WSAENOBUFS) 
			return NULL;
	}
	
	// ���뻺�������ٴε���WSAEnumProtocols����
	pProtoInfo = (LPWSAPROTOCOL_INFO)::GlobalAlloc(GPTR, dwSize);
	*lpnTotalProtocols = ::WSAEnumProtocols(NULL, pProtoInfo, &dwSize);
	return pProtoInfo;
}

void FreeProvider(LPWSAPROTOCOL_INFO pProtoInfo)
{
	::GlobalFree(pProtoInfo);
}

CInitSock theSock;

void main()
{
	int nTotalProtocols;
	LPWSAPROTOCOL_INFO pProtoInfo = GetProvider(&nTotalProtocols);
	if(pProtoInfo != NULL)
	{
		// ��ӡ�������ṩ�ߵ�Э����Ϣ
		for(int i=0; i<nTotalProtocols; i++)
		{
			printf(" Protocol: %s \n", pProtoInfo[i].szProtocol);
			printf(" CatalogEntryId: %d		ChainLen: %d \n\n", 
				pProtoInfo[i].dwCatalogEntryId, pProtoInfo[i].ProtocolChain.ChainLen);
		}
		FreeProvider(pProtoInfo);
	}

	getchar ();
	return;
}