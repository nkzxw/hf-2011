// PhoenixLSP.cpp : Defines the entry point for the DLL application.
//

// ����Ҫʹ��UNICODE�ַ���
#define UNICODE
#define _UNICODE

#include <Winsock2.h>
#include <Ws2spi.h>
#include <Sporder.h>
#include <Windows.h>
#include <stdio.h>
#include <tchar.h>

#include "PhoenixLSP.h"

#include "../common/Debug.h"
#include "../common/PMacRes.h"
#include "Acl.h"

#pragma comment(lib, "Ws2_32.lib")


CAcl g_Acl;							// �����б��������Ự�ķ���Ȩ��

WSPUPCALLTABLE g_pUpCallTable;		// �ϲ㺯���б����LSP�������Լ���α�������ʹ����������б�
WSPPROC_TABLE g_NextProcTable;		// �²㺯���б�
TCHAR	g_szCurrentApp[MAX_PATH];	// ��ǰ���ñ�DLL�ĳ��������


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			// ȡ����ģ�������
			::GetModuleFileName(NULL, g_szCurrentApp, MAX_PATH);
		}
		break;
	}
	return TRUE;
}

int WSPAPI WSPStartup(
  WORD wVersionRequested,
  LPWSPDATA lpWSPData,
  LPWSAPROTOCOL_INFO lpProtocolInfo,
  WSPUPCALLTABLE UpcallTable,
  LPWSPPROC_TABLE lpProcTable
)
{
	ODS1(L"  WSPStartup...  %s \n", g_szCurrentApp);
	
	if(lpProtocolInfo->ProtocolChain.ChainLen <= 1)
	{	
		return WSAEPROVIDERFAILEDINIT;
	}
	
	// �������ϵ��õĺ�����ָ�루�������ǲ�ʹ������
	g_pUpCallTable = UpcallTable;

	// ö��Э�飬�ҵ��²�Э���WSAPROTOCOL_INFOW�ṹ	
	WSAPROTOCOL_INFOW	NextProtocolInfo;
	int nTotalProtos;
	LPWSAPROTOCOL_INFOW pProtoInfo = GetProvider(&nTotalProtos);
	// �²����ID	
	DWORD dwBaseEntryId = lpProtocolInfo->ProtocolChain.ChainEntries[1];
	int i=0;
	for(i; i<nTotalProtos; i++)
	{
		if(pProtoInfo[i].dwCatalogEntryId == dwBaseEntryId)
		{
			memcpy(&NextProtocolInfo, &pProtoInfo[i], sizeof(NextProtocolInfo));
			break;
		}
	}
	if(i >= nTotalProtos)
	{
		ODS(L" WSPStartup:	Can not find underlying protocol \n");
		return WSAEPROVIDERFAILEDINIT;
	}

	// �����²�Э���DLL
	int nError;
	TCHAR szBaseProviderDll[MAX_PATH];
	int nLen = MAX_PATH;
	// ȡ���²��ṩ����DLL·��
	if(::WSCGetProviderPath(&NextProtocolInfo.ProviderId, szBaseProviderDll, &nLen, &nError) == SOCKET_ERROR)
	{
		ODS1(L" WSPStartup: WSCGetProviderPath() failed %d \n", nError);
		return WSAEPROVIDERFAILEDINIT;
	}
	if(!::ExpandEnvironmentStrings(szBaseProviderDll, szBaseProviderDll, MAX_PATH))
	{
		ODS1(L" WSPStartup:  ExpandEnvironmentStrings() failed %d \n", ::GetLastError());
		return WSAEPROVIDERFAILEDINIT;
	}
	// �����²��ṩ����
	HMODULE hModule = ::LoadLibrary(szBaseProviderDll);
	if(hModule == NULL)
	{
		ODS1(L" WSPStartup:  LoadLibrary() failed %d \n", ::GetLastError());
		return WSAEPROVIDERFAILEDINIT;
	}

	// �����²��ṩ�����WSPStartup����
	LPWSPSTARTUP  pfnWSPStartup = NULL;
	pfnWSPStartup = (LPWSPSTARTUP)::GetProcAddress(hModule, "WSPStartup");
	if(pfnWSPStartup == NULL)
	{
		ODS1(L" WSPStartup:  GetProcAddress() failed %d \n", ::GetLastError());
		return WSAEPROVIDERFAILEDINIT;
	}

	// �����²��ṩ�����WSPStartup����
	LPWSAPROTOCOL_INFOW pInfo = lpProtocolInfo;
	if(NextProtocolInfo.ProtocolChain.ChainLen == BASE_PROTOCOL)
		pInfo = &NextProtocolInfo;

	int nRet = pfnWSPStartup(wVersionRequested, lpWSPData, pInfo, UpcallTable, lpProcTable);
	if(nRet != ERROR_SUCCESS)
	{
		ODS1(L" WSPStartup:  underlying provider's WSPStartup() failed %d \n", nRet);
		return nRet;
	}

	// �����²��ṩ�ߵĺ�����
	g_NextProcTable = *lpProcTable;

	// �����ϲ㣬�ػ�����º����ĵ���
	lpProcTable->lpWSPSocket = WSPSocket;
	lpProcTable->lpWSPCloseSocket = WSPCloseSocket;
	lpProcTable->lpWSPBind = WSPBind;
	lpProcTable->lpWSPAccept = WSPAccept;
	lpProcTable->lpWSPConnect = WSPConnect;
	lpProcTable->lpWSPSendTo = WSPSendTo;	
	lpProcTable->lpWSPRecvFrom = WSPRecvFrom; 

	FreeProvider(pProtoInfo);
	return nRet;
}



SOCKET WSPAPI WSPSocket(
	int			af,                               
	int			type,                             
	int			protocol,                         
	LPWSAPROTOCOL_INFOW lpProtocolInfo,   
	GROUP		g,                              
	DWORD		dwFlags,                        
	LPINT		lpErrno
)
{
	// ���ȵ����²㺯�������׽���
	SOCKET	s = g_NextProcTable.lpWSPSocket(af, type, protocol, lpProtocolInfo, g, dwFlags, lpErrno);
	if(s == INVALID_SOCKET)
		return s;

	// ����CAcl���CheckSocket���������ûỰ����
	if (af == FROM_PROTOCOL_INFO)
		af = lpProtocolInfo->iAddressFamily;
	if (type == FROM_PROTOCOL_INFO)
		type = lpProtocolInfo->iSocketType;
	if (protocol == FROM_PROTOCOL_INFO)
		protocol = lpProtocolInfo->iProtocol;

	g_Acl.CheckSocket(s, af, type, protocol);

	return s;
}

int WSPAPI WSPCloseSocket(
	SOCKET		s,
	LPINT		lpErrno
)
{
	// ����CAcl���CheckCloseSocket������ɾ����Ӧ�ĻỰ
	g_Acl.CheckCloseSocket(s);
	return g_NextProcTable.lpWSPCloseSocket(s, lpErrno);
}

int WSPAPI WSPBind(SOCKET s, const struct sockaddr* name, int namelen, LPINT lpErrno)
{
	// ����CAcl���CheckBind���������ûỰ����
	g_Acl.CheckBind(s, name);
	return g_NextProcTable.lpWSPBind(s, name, namelen, lpErrno);
}

int WSPAPI WSPConnect(
	SOCKET			s,
	const struct	sockaddr FAR * name,
	int				namelen,
	LPWSABUF		lpCallerData,
	LPWSABUF		lpCalleeData,
	LPQOS			lpSQOS,
	LPQOS			lpGQOS,
	LPINT			lpErrno
)
{
	ODS1(L" WSPConnect...	%s", g_szCurrentApp);

	// ����Ƿ��������ӵ�Զ������
	if(g_Acl.CheckConnect(s, name) != PF_PASS)
	{
		*lpErrno = WSAECONNREFUSED;
		ODS1(L" WSPConnect deny a query %s \n", g_szCurrentApp);
		return SOCKET_ERROR;
	} 

	return g_NextProcTable.lpWSPConnect(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS, lpErrno);
}

SOCKET WSPAPI WSPAccept(
	SOCKET			s,
	struct sockaddr FAR *addr,
	LPINT			addrlen,
	LPCONDITIONPROC	lpfnCondition,
	DWORD			dwCallbackData,
	LPINT			lpErrno
)
{
	ODS1(L"  PhoenixLSP:  WSPAccept  %s \n", g_szCurrentApp);

	// ���ȵ����²㺯�����յ���������
	SOCKET	sNew	= g_NextProcTable.lpWSPAccept(s, addr, addrlen, lpfnCondition, dwCallbackData, lpErrno);
	
	// ����Ƿ���������������ر��½��յ�����
	if (sNew != INVALID_SOCKET && g_Acl.CheckAccept(s, sNew, addr) != PF_PASS)
	{
		int iError;
		g_NextProcTable.lpWSPCloseSocket(sNew, &iError);
		*lpErrno = WSAECONNREFUSED;
		return SOCKET_ERROR;
	}

	return sNew;
}


int WSPAPI WSPSendTo(
	SOCKET			s,
	LPWSABUF		lpBuffers,
	DWORD			dwBufferCount,
	LPDWORD			lpNumberOfBytesSent,
	DWORD			dwFlags,
	const struct sockaddr FAR * lpTo,
	int				iTolen,
	LPWSAOVERLAPPED	lpOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
	LPWSATHREADID	lpThreadId,
	LPINT			lpErrno
)
{
	ODS1(L" query send to... %s \n", g_szCurrentApp);
	
	// ����Ƿ�����������
	if (g_Acl.CheckSendTo(s, lpTo) != PF_PASS)
	{
		int		iError;
		g_NextProcTable.lpWSPShutdown(s, SD_BOTH, &iError);
		*lpErrno = WSAECONNABORTED;

		ODS1(L" WSPSendTo deny query %s \n", g_szCurrentApp);

		return SOCKET_ERROR;
	}

	// �����²㷢�ͺ���
	return g_NextProcTable.lpWSPSendTo(s, lpBuffers, dwBufferCount, 
							lpNumberOfBytesSent, dwFlags, lpTo, iTolen, 
								lpOverlapped, lpCompletionRoutine, lpThreadId, lpErrno);
}

int WSPAPI WSPRecvFrom (
	SOCKET			s,
	LPWSABUF		lpBuffers,
	DWORD			dwBufferCount,
	LPDWORD			lpNumberOfBytesRecvd,
	LPDWORD			lpFlags,
	struct sockaddr FAR * lpFrom,
	LPINT			lpFromlen,
	LPWSAOVERLAPPED lpOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
	LPWSATHREADID	lpThreadId,
	LPINT			lpErrno
)
{
	ODS1(L"  PhoenixLSP:  WSPRecvFrom %s \n", g_szCurrentApp);
	
	// ���ȼ���Ƿ������������
	if(g_Acl.CheckRecvFrom(s, lpFrom) != PF_PASS)
	{
		int		iError;
		g_NextProcTable.lpWSPShutdown(s, SD_BOTH, &iError);
		*lpErrno = WSAECONNABORTED;

		ODS1(L" WSPRecvFrom deny query %s \n", g_szCurrentApp);
		return SOCKET_ERROR;
	}
	
	// �����²���պ���
	return g_NextProcTable.lpWSPRecvFrom(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, 
		lpFlags, lpFrom, lpFromlen, lpOverlapped, lpCompletionRoutine, lpThreadId, lpErrno);
}



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

/*

int WSPAPI WSPStartup(
  WORD wVersionRequested,
  LPWSPDATA lpWSPData,
  LPWSAPROTOCOL_INFO lpProtocolInfo,
  WSPUPCALLTABLE UpcallTable,
  LPWSPPROC_TABLE lpProcTable
)
{
	ODS1(L"  PhoenixLSP:  WSPStartup  %s \n", g_szCurrentApp);

	ODS1(L" %s", lpProtocolInfo->szProtocol);
	
	if(lpProtocolInfo->ProtocolChain.ChainLen <= 1)
	{	
		::OutputDebugString(L" Chain len <= 1 \n");
		return WSAEPROVIDERFAILEDINIT;
	}
	
	g_pUpCallTable = UpcallTable;
	int nTotalProtos;
	LPWSAPROTOCOL_INFOW pProtoInfo = GetProvider(&nTotalProtos);

	
	// �ҵ��²�Э��	
	WSAPROTOCOL_INFOW	NextProtocolInfo;
	// �²����ID	
	DWORD dwBaseEntryId = lpProtocolInfo->ProtocolChain.ChainEntries[1];
	for(int i=0; i<nTotalProtos; i++)
	{
		if(pProtoInfo[i].dwCatalogEntryId == dwBaseEntryId)
		{
			memcpy(&NextProtocolInfo, &pProtoInfo[i], sizeof(NextProtocolInfo));
			break;
		}
	}
	if(i >= nTotalProtos)
	{
		::OutputDebugString(L" Can not find next protocol <= 1 \n");
		return WSAEPROVIDERFAILEDINIT;
	}

	// �����²�Э���DLL
	int nError;
	TCHAR szBaseProviderDll[MAX_PATH];
	int nLen = MAX_PATH;
	if(::WSCGetProviderPath(&NextProtocolInfo.ProviderId, szBaseProviderDll, &nLen, &nError) == SOCKET_ERROR)
	{
		::OutputDebugString(L" WSCGetProviderPath() failed \n");
		return WSAEPROVIDERFAILEDINIT;
	}
	if(!::ExpandEnvironmentStrings(szBaseProviderDll, szBaseProviderDll, MAX_PATH))
	{
		::OutputDebugString(L" ExpandEnvironmentStrings() failed \n");
		return WSAEPROVIDERFAILEDINIT;
	}
	HMODULE hModule = ::LoadLibrary(szBaseProviderDll);
	if(hModule == NULL)
	{
		::OutputDebugString(L" LoadLibrary() failed \n");
		return WSAEPROVIDERFAILEDINIT;
	}

	// �����²�Э���WSPStartup����
	LPWSPSTARTUP  proWSPStartup = NULL;
	proWSPStartup = (LPWSPSTARTUP)::GetProcAddress(hModule, "WSPStartup");
	if(proWSPStartup == NULL)
	{
		::OutputDebugString(L" GetProcAddress() failed \n");
		return WSAEPROVIDERFAILEDINIT;
	}

	LPWSAPROTOCOL_INFOW pInfo = lpProtocolInfo;
	if(NextProtocolInfo.ProtocolChain.ChainLen == BASE_PROTOCOL)
		pInfo = &NextProtocolInfo;

	int nRet = proWSPStartup(wVersionRequested, lpWSPData, pInfo, UpcallTable, lpProcTable);
	if(nRet != ERROR_SUCCESS)
	{
		ODS1(L" next layer's WSPStartup() failed %d \n ", nRet);
		return nRet;
	}

	// �����²�Э��ĺ�����
	g_NextProcTable = *lpProcTable;

	// �����ϲ�
	lpProcTable->lpWSPSocket = WSPSocket;
	lpProcTable->lpWSPCloseSocket = WSPCloseSocket;

	lpProcTable->lpWSPBind = WSPBind;

	// tcp
	lpProcTable->lpWSPAccept = WSPAccept;
	lpProcTable->lpWSPConnect = WSPConnect;

	// udp raw
	lpProcTable->lpWSPSendTo = WSPSendTo;	
	lpProcTable->lpWSPRecvFrom = WSPRecvFrom; 



	FreeProvider(pProtoInfo);
	return nRet;
}

*/




////////////////////////////////////////////////////////////////////////////////

/*	lpProcTable->lpWSPSend = WSPSend;
	lpProcTable->lpWSPRecv = WSPRecv;
*/
/*

  int WSPAPI WSPSend(
	SOCKET			s,
	LPWSABUF		lpBuffers,
	DWORD			dwBufferCount,
	LPDWORD			lpNumberOfBytesSent,
	DWORD			dwFlags,
	LPWSAOVERLAPPED	lpOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
	LPWSATHREADID	lpThreadId,
	LPINT			lpErrno
)
{
//	ODS1(L"  PhoenixLSP:  WSPSend  %s \n", g_szCurrentApp);

	// ?? ���Buf��δ���
	if (g_Acl.CheckSend(s, lpBuffers[0].buf, *lpNumberOfBytesSent) != PF_PASS)
	{
		int		iError;
		g_NextProcTable.lpWSPShutdown(s, SD_BOTH, &iError);
		*lpErrno = WSAECONNABORTED;

		ODS(L" deny a send ");
		return SOCKET_ERROR;
	}

	return g_NextProcTable.lpWSPSend(s, lpBuffers, dwBufferCount, 
				lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine, lpThreadId, lpErrno);
}

int WSPAPI WSPRecv(
	SOCKET			s,
	LPWSABUF		lpBuffers,
	DWORD			dwBufferCount,
	LPDWORD			lpNumberOfBytesRecvd,
	LPDWORD			lpFlags,
	LPWSAOVERLAPPED	lpOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
	LPWSATHREADID	lpThreadId,
	LPINT			lpErrno
)
{
	ODS1(L"  PhoenixLSP:  WSPRecv  %s \n", g_szCurrentApp);

	if(lpOverlapped != NULL)
	{
		if(g_Acl.CheckRecv(s, NULL, 0) != PF_PASS)
		{
			int		iError;
			g_NextProcTable.lpWSPShutdown(s, SD_BOTH, &iError);
			*lpErrno = WSAECONNABORTED;

			ODS(L"deny a recv");
			return SOCKET_ERROR;
		}
		ODS(L" overlappped ");
	}

	int	iRet = g_NextProcTable.lpWSPRecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped
				, lpCompletionRoutine, lpThreadId, lpErrno);

	if(iRet != SOCKET_ERROR && lpOverlapped == NULL)
	{
		if(g_Acl.CheckRecv(s, lpBuffers[0].buf, *lpNumberOfBytesRecvd) != PF_PASS)
		{
			int		iError;
			g_NextProcTable.lpWSPShutdown(s, SD_BOTH, &iError);
			*lpErrno = WSAECONNABORTED;

			ODS(L"deny a recv");
			return SOCKET_ERROR;
		}
	}
	return iRet;
}

  */
