//-----------------------------------------------------------
/*
	���̣�		�Ѷ����˷���ǽ
	��ַ��		http://www.xfilt.com
	�����ʼ���	xstudio@xfilt.com
	��Ȩ���� (c) 2002 ���޻�(�Ѷ���ȫʵ����)

	��Ȩ����:
	---------------------------------------------------
		�����Գ���������Ȩ���ı�����δ����Ȩ������ʹ��
	���޸ı����ȫ���򲿷�Դ���롣�����Ը��ơ����û�ɢ
	���˳���򲿷ֳ�������������κ�ԽȨ��Ϊ�����⵽��
	���⳥�����µĴ�������������������̷�����׷�ߡ�
	
		��ͨ���Ϸ�;�������Դ������(�����ڱ���)��Ĭ��
	��Ȩ�����Ķ������롢���ԡ������ҽ����ڵ��Ե���Ҫ��
	�����޸ı����룬���޸ĺ�Ĵ���Ҳ����ֱ��ʹ�á�δ��
	��Ȩ������������Ʒ��ȫ���򲿷ִ�������������Ʒ��
	������ת�����ˣ����������κη�ʽ���ƻ򴫲���������
	�����κη�ʽ����ҵ��Ϊ��	

    ---------------------------------------------------	
*/
//-----------------------------------------------------------
// ��飺
//		DLL ���ģ�顣����Dll����ں���DllMain��SPI��ں���
//		WSPStartup���ⲿ��ģ����Ҫ��������ֹ��ܡ�
//		1. HOOK SPI ����
//		2. ��XFILTER.EXE�Ľӿ�XfIoControl
//
//
//

//
// include header file and global variables
//
#include "stdafx.h"
#include "TcpIpDog.h"
#include "CheckAcl.h"
#include "MemoryFile.h"

//
// QQ Test
//
//#include "..\qq\qq.h"

// v1.0.2 2001-12-24 add for overlapped io
#include "Overlapped.h"

#pragma data_seg(".inidata")
	int		m_iDllCount		= 0;
#pragma data_seg()

int GetDllRefenceCount()
{
	return m_iDllCount;
}

CRITICAL_SECTION	gCriticalSection;
CCheckAcl			m_CheckAcl;
WSPPROC_TABLE		NextProcTable   ;
TCHAR				m_sProcessName[MAX_PATH];

// v1.0.2 2001-12-24 add for overlapped io
COverlapped			m_Overlapped;

CMemoryFile			m_MemoryFile;	
//=============================================================================================
//DllMain Procedure

//
// DLL ��ں�������DLL���ô�������ͳ�ơ�
//
BOOL WINAPI DllMain(
	HINSTANCE	hModule, 
    DWORD		ul_reason_for_call, 
    LPVOID		lpReserved
)
{
	if(ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		XF_OpenDriver();

 		GetModuleFileName(NULL, m_sProcessName, MAX_PATH);
		InitializeCriticalSection(&gCriticalSection);

		EnterCriticalSection(&gCriticalSection);
		{
			m_iDllCount ++;
			//if(m_iDllCount == 1)
			//{
			//	m_CheckAcl.GetSession()->InitializeSessionBuffer();
			//}
		}
		LeaveCriticalSection(&gCriticalSection);

		ODS2(m_sProcessName,_T(" Loading ..."));
	}
	else if(ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		//
		// 2002/05/24 add
		//
		if(m_MemoryFile.IsXfilter())
		{
			SetWorkMode(XF_PASS_ALL);
		}

		EnterCriticalSection(&gCriticalSection);
		{
			m_iDllCount -- ;
		}
		LeaveCriticalSection(&gCriticalSection);

		ODS2(m_sProcessName,_T(" Exit ..."));
	}

	return TRUE;
}

//=============================================================================================
//Exported Functions

//
// SPI��ں������κ�ʹ��SOCKET�ĳ������ʹ��WSAStartup���г�ʼ��
// ��ʵWSAStartup�����WSPStartup���õ�30��SPI��������ָ��
// 30����������ָ��ͱ�����lpProcTable�ṹ�У�������������
// ����������30����������HOOK������û��HOOK���еĺ�����ֻ
// �ػ�����������Ҫ�ļ�����
// 
// Ϊ��ʹϵͳ�������ǵ�WSPStartup������ϵͳ�ģ���Ҫ��SPI���а�װ
// ��װ�Ĵ�����CXInstall���С�
//
//
int WSPAPI WSPStartup(
	WORD				wVersionRequested,
	LPWSPDATA			lpWSPData,
	LPWSAPROTOCOL_INFOW	lpProtocolInfo,
	WSPUPCALLTABLE		upcallTable,
	LPWSPPROC_TABLE		lpProcTable
)
{
	EnterCriticalSection(&gCriticalSection);

	__try
	{
		ODS(_T("WSPStartup..."));
    
		TCHAR				sLibraryPath[512];
		LPWSPSTARTUP        WSPStartupFunc      = NULL;
		HMODULE				hLibraryHandle		= NULL;
		INT                 ErrorCode           = 0; 

		//
		// ����ϵͳSPIģ��
		//
		if (!GetHookProvider(lpProtocolInfo, sLibraryPath)
			|| (hLibraryHandle = LoadLibrary(sLibraryPath)) == NULL
			|| (WSPStartupFunc = (LPWSPSTARTUP)GetProcAddress(hLibraryHandle, "WSPStartup")) == NULL
			)
			return WSAEPROVIDERFAILEDINIT;

		//
		// ����ϵͳWSPStartup�õ�ϵͳ��30��������ָ��
		//
		if ((ErrorCode = WSPStartupFunc(wVersionRequested, lpWSPData, lpProtocolInfo, upcallTable, lpProcTable)) != ERROR_SUCCESS)
			return ErrorCode;
		
		//
		// Don't filter QOS
		//
		if (XP1_QOS_SUPPORTED == (lpProtocolInfo->dwServiceFlags1 & XP1_QOS_SUPPORTED) )
			return 0;

		//
		// ����ϵͳ�������б�
		//
		NextProcTable = *lpProcTable;

		//
		// HOOK SPI������
		//
		lpProcTable->lpWSPCleanup		= WSPCleanup;
		lpProcTable->lpWSPSocket		= WSPSocket;
		lpProcTable->lpWSPCloseSocket	= WSPCloseSocket;
		lpProcTable->lpWSPBind			= WSPBind;
		lpProcTable->lpWSPListen		= WSPListen;
		lpProcTable->lpWSPConnect		= WSPConnect;
		lpProcTable->lpWSPAccept		= WSPAccept;
		lpProcTable->lpWSPSend			= WSPSend;
		lpProcTable->lpWSPSendTo		= WSPSendTo;
		lpProcTable->lpWSPRecv			= WSPRecv;
		lpProcTable->lpWSPRecvFrom		= WSPRecvFrom;
		return 0;
	}
	__finally
	{
		LeaveCriticalSection(&gCriticalSection);
	}

	return 0;
}

//
// ΪXFILTER.EXE�ṩ�Ľӿ�
//
int WINAPI XfIoControl(
	int					iControlType, 
	XFILTER_IO_CONTROL	*ioControl
)
{
	switch(iControlType)
	{
	case IO_CONTROL_SET_XFILTER_PROCESS_ID:
		//
		// ����XFILTER.EXE�Ľ���ID���Ա������Ƿ���XFILTER.EXE�ļ��ڵ���
		//
		SetXfilterProcessId(ioControl->DWord, (char*)ioControl->DWord2);
		break;
	case IO_CONTROL_SET_WORK_MODE:
		//
		// �����ܹ���ģʽ(����ģʽ)
		//
		SetWorkMode(ioControl->Byte);
		break;
	case IO_CONTROL_SET_ACL_IS_REFRESH:
		//
		// ֪ͨXFITLER.DLL�عܹ������ڸ��£���ʱ�����ٽ��пعܹ���ıȽ�
		//
		if(!ioControl->Byte)
		{
			//SetMemoryFile(ioControl->DWord);
			RefenceUpdateVersion();
		}
		SetRefresh(ioControl->Byte);
		break;
	case IO_CONTROL_REFENCE_UPDATE_VERSION:
		//
		// ���Ӱ汾���¼���
		//
		RefenceUpdateVersion();
		break;
	case IO_CONTROL_SET_ACL_MEMORY_FILE_HANDLE:
		//
		// ���ÿعܹ����ڴ�ӳ���ļ��ľ������С�ͻ���ַ
		//
		SetMemoryFile(ioControl->DWord);
		SetMaxSize((DWORD)ioControl->Pointer);
		SetBaseAddress(ioControl->DWord2);
		break;
	case IO_CONTROL_GET_SESSION_FILE_HANDLE:
		//
		// �õ�SOCKET���߼�¼���ڴ�ӳ���ļ����
		//
		ioControl->DWord = (DWORD)GetSessionFileHandle();
		break;
	case IO_CONTROL_GET_SESSION_COUNT:
		//
		// �õ�SOCKET���߼�¼��������
		//
		ioControl->DWord = GetSessionCount();
		break;
	default:
		break;
	}

	return XERR_SUCCESS;
}

//=============================================================================================
//Socket Private functions

//
// �õ�ϵͳSPI��·��
//
BOOL GetHookProvider(
	IN	WSAPROTOCOL_INFOW	*pProtocolInfo, 
	OUT	TCHAR				*sPathName
)
{
	TCHAR sItem[21];
	GetRightEntryIdItem(pProtocolInfo, sItem);

	HKEY	hSubkey;
	DWORD	ulDateLenth	= MAX_PATH;
	TCHAR	sTemp[MAX_PATH];


	//
	// 2002/08/18 changed KEY_ALL_ACCESS to KEY_READ
	//
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_INSTALL_KEY, 0, KEY_READ, &hSubkey) != ERROR_SUCCESS)
		return FALSE;

	if (RegQueryValueEx(hSubkey, sItem, 0, NULL, (BYTE*)sTemp, &ulDateLenth)
		|| ExpandEnvironmentStrings(sTemp, sPathName, ulDateLenth) == 0)
		return FALSE;

	if(sPathName[0] == '\0' && sTemp[0] != '\0')
		_tcscpy(sPathName, sTemp);

	RegCloseKey(hSubkey);

	return TRUE;
}

void GetRightEntryIdItem(
	IN	WSAPROTOCOL_INFOW	*pProtocolInfo, 
	OUT	TCHAR				*sItem
)
{
	if(pProtocolInfo->ProtocolChain.ChainLen <= 1)
		_stprintf(sItem, _T("%u"), pProtocolInfo->dwCatalogEntryId);
	else
		_stprintf(sItem, _T("%u"), pProtocolInfo->ProtocolChain.ChainEntries[pProtocolInfo->ProtocolChain.ChainLen - 1]);
}

//=============================================================================================
//Winsock 2 service provider hook functions

//
// Our WSPCleanup
//
int WSPAPI WSPCleanup(
	LPINT		lpErrno
)
{
	return NextProcTable.lpWSPCleanup(lpErrno);
}

//
// Our WSPSocket
//
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
	ODS(_T("XFILTER.DLL: WSPSocket ..."));
	SOCKET	s = NextProcTable.lpWSPSocket(af, type, protocol, lpProtocolInfo, g, dwFlags, lpErrno);

	if(s == INVALID_SOCKET)
		return s;

	if (af == FROM_PROTOCOL_INFO)
		af = lpProtocolInfo->iAddressFamily;
	if (type == FROM_PROTOCOL_INFO)
		type = lpProtocolInfo->iSocketType;
	if (protocol == FROM_PROTOCOL_INFO)
		protocol = lpProtocolInfo->iProtocol;
	m_CheckAcl.CheckSocket(s, af, type, protocol);

	return s;
}

//
// Our WSPCloseSocket
//
int WSPAPI WSPCloseSocket(
	SOCKET		s,
	LPINT		lpErrno
)
{
	ODS(_T("XFILTER.DLL: WSPCloseSocket ..."));

	int iRet = NextProcTable.lpWSPCloseSocket(s, lpErrno);

	if(iRet != 0) 
		WSASetLastError(*lpErrno);
	else
		m_CheckAcl.CheckCloseSocket(s);

	return iRet;
}

//
// Our WSPBind
//
int WSPAPI WSPBind (
	SOCKET	s,
	const struct sockaddr FAR *name,
	int		namelen,
	LPINT	lpErrno 
)
{
	ODS(_T("XFILTER.DLL: WSPBind ..."));

	int iRet = NextProcTable.lpWSPBind(s, name, namelen, lpErrno);
	if(iRet == 0 && m_CheckAcl.CheckBind(s, name) != XF_PASS)
	{
		WSPCloseSocket(s, lpErrno);
		*lpErrno = WSAENOTSOCK;
		return SOCKET_ERROR;
	}
	return iRet;
}

//
// Our WSPListen
//
int WSPAPI WSPListen (
	SOCKET	s,
	int		backlog,
	LPINT	lpErrno 
)
{
	ODS(_T("XFILTER.DLL: WSPListen ..."));
	int iRet = NextProcTable.lpWSPListen(s, backlog, lpErrno);
	if(iRet == 0 && m_CheckAcl.CheckListen(s) != XF_PASS)
	{
		WSPCloseSocket(s, lpErrno);
		*lpErrno = WSAENOTSOCK;
		return SOCKET_ERROR;
	}
	return iRet;
}

//
// Our WSPConnect
//
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
	ODS(_T("XFILTER.DLL: WSPConnect ..."));

	if(m_CheckAcl.CheckConnect(s, name, namelen) != XF_PASS)
	{
		ODS2(_T("Deny the application "), m_sProcessName);
		*lpErrno = WSAECONNREFUSED;
		return SOCKET_ERROR;
	}
 
	return NextProcTable.lpWSPConnect(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS, lpErrno);
}

//
// Our WSPAccept
//
SOCKET WSPAPI WSPAccept(
	SOCKET			s,
	struct sockaddr FAR *addr,
	LPINT			addrlen,
	LPCONDITIONPROC	lpfnCondition,
	DWORD			dwCallbackData,
	LPINT			lpErrno
)
{
	ODS(_T("XFILTER.DLL: WSPAccept ..."));

	SOCKET	news	= NextProcTable.lpWSPAccept(s, addr, addrlen, lpfnCondition, dwCallbackData, lpErrno);

	if (news != INVALID_SOCKET && m_CheckAcl.CheckAccept(s, news) != XF_PASS)
	{
		WSPCloseSocket(news, lpErrno);
		// v1.0.2 add 2001-12-22
		*lpErrno = 0;
	}

	return news;
}

//
// Our WSPSend
//
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
	ODS(_T("XFILTER.DLL: WSPSend ..."));

	if (m_CheckAcl.CheckSend(s, lpBuffers[0].buf, lpBuffers[0].len, lpNumberOfBytesSent) != XF_PASS)
	{
		WSPCloseSocket(s, lpErrno);
		*lpErrno = WSAECONNABORTED;
		return SOCKET_ERROR;
	}

	return NextProcTable.lpWSPSend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped
				, lpCompletionRoutine, lpThreadId, lpErrno);
}

//
// Our WSPSendTo
//
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
	ODS(_T("XFILTER.DLL: WSPSendTo ..."));

	if (m_CheckAcl.CheckSendTo(s, lpTo, lpBuffers[0].buf, lpBuffers[0].len, lpNumberOfBytesSent) != XF_PASS)
	{
		WSPCloseSocket(s, lpErrno);
		*lpErrno = WSAECONNABORTED;
		return SOCKET_ERROR;
	}

	return NextProcTable.lpWSPSendTo(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpTo
			, iTolen, lpOverlapped, lpCompletionRoutine, lpThreadId, lpErrno);
}

//
// Our WSPRecv
//
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
	ODS(_T("XFILTER.DLL: WSPRecv ..."));

	//
	// 2001-12-24 add, ������ص������������˻ص����������������Զ��庯��
	// AddOverlapped����ԭ���Ĳ�����Ϣ��Ȼ�����Լ��Ļص������ӹ�ԭ���Ļ�
	// �������������ñ�־ΪIsSetCompletionRoutine
	//
	BOOL IsSetCompletionRoutine = FALSE;
	if(lpOverlapped && lpCompletionRoutine && m_Overlapped.AddOverlapped(s
		, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags
		, lpOverlapped, lpCompletionRoutine, NULL, NULL, 0)//WSPRecv
		)
	{
		lpCompletionRoutine		= CompletionRoutine;
		IsSetCompletionRoutine	= TRUE;
	}

	//
	// 2001-12-22 �޸ģ�������һ��BUG��ԭ������dwBufferCountΪ 1
	// �������¸���ΪdwBufferCount
	//
	int	iRet = NextProcTable.lpWSPRecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped
				, lpCompletionRoutine, lpThreadId, lpErrno);

	//
	// ����SOCKET_ERROR�����������
	// 1. �д�����
	// 2. û���������أ�û���������ؿ����������ص�������ɣ����lpErrnoΪ
	//	  WSA_IO_PENDING ���ʾ�ص������ɹ����Ժ����������ɡ�
	// �������SOCKET_ERROR�����������Լ��Ļص�������ֱ�ӷ����ɻص�����
	// ���������ɺ�Ĺ�����
	//
	if(iRet == SOCKET_ERROR || IsSetCompletionRoutine == TRUE)
	{
		return iRet;
	}

	if (m_CheckAcl.CheckRecv(s, lpBuffers[0].buf, lpBuffers[0].len, lpNumberOfBytesRecvd) != XF_PASS)
	{
		WSPCloseSocket(s, lpErrno);
		*lpErrno = WSAECONNABORTED;
		return SOCKET_ERROR;
	}

	return iRet;

}

//
// Our WSPRecvFrom
//
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
	ODS(_T("XFILTER.DLL: WSPRecvFrom ..."));

	//
	// 2001-12-24 add, ���ʱ�ص������������˻ص����������������Զ��庯��
	// AddOverlapped����ԭ���Ĳ�����Ϣ��Ȼ�����Լ��Ļص������ӹ�ԭ���Ļ�
	// �������������ñ�־ΪIsSetCompletionRoutine
	//
	BOOL IsSetCompletionRoutine = FALSE;
	if(lpOverlapped && lpCompletionRoutine && m_Overlapped.AddOverlapped(s
		, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags
		, lpOverlapped, lpCompletionRoutine, lpFrom, lpFromlen, 1)//WSPRecvFrom
		)
	{
		lpCompletionRoutine		= CompletionRoutine;
		IsSetCompletionRoutine	= TRUE;
	}

	//
	// 2001-12-22 �޸ģ�������һ��BUG��ԭ������dwBufferCountΪ 1
	// �������¸���ΪdwBufferCount
	//
	int	iRet= NextProcTable.lpWSPRecvFrom(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpFrom
			, lpFromlen, lpOverlapped, lpCompletionRoutine, lpThreadId, lpErrno);

	//
	// ����SOCKET_ERROR�����������
	// 1. �д�����
	// 2. û���������أ�û���������ؿ����������ص�������ɣ����lpErrnoΪ
	//	  WSA_IO_PENDING ���ʾ�ص������ɹ����Ժ����������ɡ�
	// �������SOCKET_ERROR�����������Լ��Ļص�������ֱ�ӷ����ɻص�����
	// ���������ɺ�Ĺ�����
	//
	if(iRet == SOCKET_ERROR || IsSetCompletionRoutine == TRUE)
	{
		return iRet;
	}

	if (m_CheckAcl.CheckRecvFrom(s, lpFrom, lpBuffers[0].buf, lpBuffers[0].len, lpNumberOfBytesRecvd) != XF_PASS)
	{
		WSPCloseSocket(s, lpErrno);
		*lpErrno = WSAECONNABORTED;
		return SOCKET_ERROR;
	}

	return iRet;
}

//
// v1.0.2 2001-12-24 add for overlapped io
//
void CALLBACK CompletionRoutine (
	IN    DWORD				dwError, 
	IN    DWORD				cbTransferred, 
	IN    LPWSAOVERLAPPED	lpOverlapped, 
	IN    DWORD				dwFlags 
)
{
	ODS(_T("XFILTER.DLL: CompletionRoutine ..."));
	int iIndex = m_Overlapped.FindOverlapped(lpOverlapped);
	if(iIndex < 0)
		return;

	if(m_Overlapped.m_OverlappedRecorder[iIndex].FunctionType == 1)//WSPRecvFrom
	{
		if( m_CheckAcl.CheckRecvFrom(
				m_Overlapped.m_OverlappedRecorder[iIndex].s
				, m_Overlapped.m_OverlappedRecorder[iIndex].lpFrom
				, m_Overlapped.m_OverlappedRecorder[iIndex].lpBuffers[0].buf
				, m_Overlapped.m_OverlappedRecorder[iIndex].lpBuffers[0].len
				, &cbTransferred
				) != XF_PASS
		)
		{
			int iErrno;
			WSPCloseSocket(m_Overlapped.m_OverlappedRecorder[iIndex].s, &iErrno);
			dwError = WSAECONNABORTED;
		}
	}
	else if(m_Overlapped.m_OverlappedRecorder[iIndex].FunctionType == 0) //WSPRecv
	{
		if( m_CheckAcl.CheckRecv(
				m_Overlapped.m_OverlappedRecorder[iIndex].s
				, m_Overlapped.m_OverlappedRecorder[iIndex].lpBuffers[0].buf
				, m_Overlapped.m_OverlappedRecorder[iIndex].lpBuffers[0].len
				, &cbTransferred
				) != XF_PASS
		)
		{
			int iErrno;
			WSPCloseSocket(m_Overlapped.m_OverlappedRecorder[iIndex].s, &iErrno);
			dwError = WSAECONNABORTED;
		}
	}

	if(m_Overlapped.m_OverlappedRecorder[iIndex].lpCompletionRoutine != NULL)
	{
		m_Overlapped.m_OverlappedRecorder[iIndex].lpCompletionRoutine(dwError
			, cbTransferred, lpOverlapped, dwFlags);
	}
	m_Overlapped.DeleteOverlapped(iIndex);
}
 
#pragma comment( exestr, "B9D3B8FD2A7665726B726671692B")
