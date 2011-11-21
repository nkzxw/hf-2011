//-----------------------------------------------------------
/*
	工程：		费尔个人防火墙
	网址：		http://www.xfilt.com
	电子邮件：	xstudio@xfilt.com
	版权所有 (c) 2002 朱艳辉(费尔安全实验室)

	版权声明:
	---------------------------------------------------
		本电脑程序受著作权法的保护。未经授权，不能使用
	和修改本软件全部或部分源代码。凡擅自复制、盗用或散
	布此程序或部分程序或者有其它任何越权行为，将遭到民
	事赔偿及刑事的处罚，并将依法以最高刑罚进行追诉。
	
		凡通过合法途径购买此源程序者(仅限于本人)，默认
	授权允许阅读、编译、调试。调试且仅限于调试的需要才
	可以修改本代码，且修改后的代码也不可直接使用。未经
	授权，不允许将本产品的全部或部分代码用于其它产品，
	不允许转阅他人，不允许以任何方式复制或传播，不允许
	用于任何方式的商业行为。	

    ---------------------------------------------------	
*/
//-----------------------------------------------------------
// 简介：
//		DLL 入口模块。包括Dll的入口函数DllMain和SPI入口函数
//		WSPStartup。这部分模块主要完成两部分功能。
//		1. HOOK SPI 函数
//		2. 与XFILTER.EXE的接口XfIoControl
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
// DLL 入口函数，对DLL调用次数进行统计。
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
// SPI入口函数，任何使用SOCKET的程序必须使用WSAStartup进行初始化
// 其实WSAStartup会调用WSPStartup来得到30个SPI服务函数的指针
// 30个服务函数的指针就保存在lpProcTable结构中，所以我们利用
// 这个函数完成30个服务函数的HOOK，这里没有HOOK所有的函数，只
// 截获了我们所需要的几个。
// 
// 为了使系统调用我们的WSPStartup而不是系统的，需要对SPI进行安装
// 安装的代码在CXInstall类中。
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
		// 加载系统SPI模块
		//
		if (!GetHookProvider(lpProtocolInfo, sLibraryPath)
			|| (hLibraryHandle = LoadLibrary(sLibraryPath)) == NULL
			|| (WSPStartupFunc = (LPWSPSTARTUP)GetProcAddress(hLibraryHandle, "WSPStartup")) == NULL
			)
			return WSAEPROVIDERFAILEDINIT;

		//
		// 调用系统WSPStartup得到系统的30个服务函数指针
		//
		if ((ErrorCode = WSPStartupFunc(wVersionRequested, lpWSPData, lpProtocolInfo, upcallTable, lpProcTable)) != ERROR_SUCCESS)
			return ErrorCode;
		
		//
		// Don't filter QOS
		//
		if (XP1_QOS_SUPPORTED == (lpProtocolInfo->dwServiceFlags1 & XP1_QOS_SUPPORTED) )
			return 0;

		//
		// 保存系统服务函数列表
		//
		NextProcTable = *lpProcTable;

		//
		// HOOK SPI服务函数
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
// 为XFILTER.EXE提供的接口
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
		// 保存XFILTER.EXE的进程ID，以便区分是否是XFILTER.EXE文件在调用
		//
		SetXfilterProcessId(ioControl->DWord, (char*)ioControl->DWord2);
		break;
	case IO_CONTROL_SET_WORK_MODE:
		//
		// 设置总工作模式(过滤模式)
		//
		SetWorkMode(ioControl->Byte);
		break;
	case IO_CONTROL_SET_ACL_IS_REFRESH:
		//
		// 通知XFITLER.DLL控管规则正在更新，此时不能再进行控管规则的比较
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
		// 增加版本更新计数
		//
		RefenceUpdateVersion();
		break;
	case IO_CONTROL_SET_ACL_MEMORY_FILE_HANDLE:
		//
		// 设置控管规则内存映射文件的句柄、大小和基地址
		//
		SetMemoryFile(ioControl->DWord);
		SetMaxSize((DWORD)ioControl->Pointer);
		SetBaseAddress(ioControl->DWord2);
		break;
	case IO_CONTROL_GET_SESSION_FILE_HANDLE:
		//
		// 得到SOCKET连线记录的内存映射文件句柄
		//
		ioControl->DWord = (DWORD)GetSessionFileHandle();
		break;
	case IO_CONTROL_GET_SESSION_COUNT:
		//
		// 得到SOCKET连线记录的最大个数
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
// 得到系统SPI的路径
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
	// 2001-12-24 add, 如果是重叠操作且设置了回调函数，则首先用自定义函数
	// AddOverlapped保存原来的参数信息，然后用自己的回调函数接管原来的回
	// 调函数，并设置标志为IsSetCompletionRoutine
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
	// 2001-12-22 修改，这里有一个BUG，原来参数dwBufferCount为 1
	// 现在重新更改为dwBufferCount
	//
	int	iRet = NextProcTable.lpWSPRecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped
				, lpCompletionRoutine, lpThreadId, lpErrno);

	//
	// 返回SOCKET_ERROR有两种情况：
	// 1. 有错误发生
	// 2. 没有立即返回：没有立即返回可能是由于重叠操作造成，如果lpErrno为
	//	  WSA_IO_PENDING 则表示重叠操作成功，稍后操作才能完成。
	// 如果返回SOCKET_ERROR或者设置了自己的回调函数，直接返回由回调函数
	// 处理操作完成后的工作。
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
	// 2001-12-24 add, 如果时重叠操作且设置了回调函数，则首先用自定义函数
	// AddOverlapped保存原来的参数信息，然后用自己的回调函数接管原来的回
	// 调函数，并设置标志为IsSetCompletionRoutine
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
	// 2001-12-22 修改，这里有一个BUG，原来参数dwBufferCount为 1
	// 现在重新更改为dwBufferCount
	//
	int	iRet= NextProcTable.lpWSPRecvFrom(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpFrom
			, lpFromlen, lpOverlapped, lpCompletionRoutine, lpThreadId, lpErrno);

	//
	// 返回SOCKET_ERROR有两种情况：
	// 1. 有错误发生
	// 2. 没有立即返回：没有立即返回可能是由于重叠操作造成，如果lpErrno为
	//	  WSA_IO_PENDING 则表示重叠操作成功，稍后操作才能完成。
	// 如果返回SOCKET_ERROR或者设置了自己的回调函数，直接返回由回调函数
	// 处理操作完成后的工作。
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
