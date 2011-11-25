///////////////////////////////////////////////////////////
// WSCEnumProtocols.cpp�ļ�


#include <Ws2spi.h>		//  SPI����������Ws2spi.h�ļ���
#include <windows.h>
#include <stdio.h>
#pragma comment(lib, "WS2_32")	// ���ӵ�WS2_32.lib

LPWSAPROTOCOL_INFOW GetProvider(LPINT lpnTotalProtocols)
{
	int nError;
	DWORD dwSize = 0;
	LPWSAPROTOCOL_INFOW pProtoInfo = NULL;
	
	//  ȡ����Ҫ�Ļ���������
	if(::WSCEnumProtocols(NULL, pProtoInfo, &dwSize, &nError) == SOCKET_ERROR)
	{
		if(nError != WSAENOBUFS)
			return NULL;
	}
	// ���뻺�������ٴε���WSCEnumProtocols����
	pProtoInfo = (LPWSAPROTOCOL_INFOW)::GlobalAlloc(GPTR, dwSize);
	*lpnTotalProtocols = ::WSCEnumProtocols(NULL, pProtoInfo, &dwSize, &nError);
	return pProtoInfo;
}

void FreeProvider(LPWSAPROTOCOL_INFOW pProtoInfo)
{
	::GlobalFree(pProtoInfo);
}

void main()
{
	LPWSAPROTOCOL_INFOW pProtoInfo;
	int nProtocols;
	pProtoInfo = GetProvider(&nProtocols);

	for(int i=0; i<nProtocols; i++)
	{
		
		printf(" Protocol: %ws \n", pProtoInfo[i].szProtocol);
		printf(" CatalogEntryId: %d		ChainLen: %d \n\n", 
			pProtoInfo[i].dwCatalogEntryId, pProtoInfo[i].ProtocolChain.ChainLen);
		
	}

	getchar ();
	return;
}