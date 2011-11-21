/*=============================================================================================
/*  
	TcpIpDog.h

	Project	: XFILTER 1.0 Personal Firewall
	Author	: Tony Zhu
	Create Date	: 2001/08/21
	Email	: xstudio@xfilt.com
	URL		: http://www.xfilt.com

	Copyright (c) 2001-2002 XStudio Technology.
	All Rights Reserved.

	WARNNING: 
*/

#ifndef TCPIPDOG_H
#define TCPIPDOG_H

extern int GetDllRefenceCount();

//=============================================================================================
//DllMain Procedure

BOOL WINAPI DllMain(
	HINSTANCE	hModule, 
    DWORD		ul_reason_for_call, 
    LPVOID		lpReserved
);

//=============================================================================================
//Export Functions

int WSPAPI WSPStartup (
	WORD				wVersionRequested,               
	LPWSPDATA			lpWSPData,                 
	LPWSAPROTOCOL_INFOW lpProtocolInfo,   
	WSPUPCALLTABLE		UpcallTable,           
	LPWSPPROC_TABLE		lpProcTable           
);

int WINAPI XfIoControl(
	int					iControlType, 
	XFILTER_IO_CONTROL	*ioControl
);

//=============================================================================================
//Socket Private functions

BOOL QueryAccess();

BOOL AddAcl(
	TCHAR	*sPathName, 
	BYTE	bAction		= ACL_ACTION_PASS
);

BOOL GetHookProvider(
	IN	WSAPROTOCOL_INFOW	*pProtocolInfo, 
	OUT	TCHAR				*sPathName
);

void GetRightEntryIdItem(
	IN	WSAPROTOCOL_INFOW	*pProtocolInfo, 
	OUT	TCHAR				*sItem
);

//=============================================================================================
//Winsock 2 service provider hook functions

int WSPAPI WSPCleanup(
	LPINT		lpErrno
);

SOCKET WSPAPI WSPSocket(
	int			af,                               
	int			type,                             
	int			protocol,                         
	LPWSAPROTOCOL_INFOW lpProtocolInfo,   
	GROUP		g,                              
	DWORD		dwFlags,                        
	LPINT		lpErrno
);

int WSPAPI WSPCloseSocket(
	SOCKET		s,
	LPINT		lpErrno
);

int WSPAPI WSPBind (
	SOCKET	s,
	const struct sockaddr FAR *name,
	int		namelen,
	LPINT	lpErrno 
);

int WSPAPI WSPListen (
	SOCKET	s,
	int		backlog,
	LPINT	lpErrno 
);
 
int WSPAPI WSPConnect(
	SOCKET			s,
	const struct	sockaddr FAR * name,
	int				namelen,
	LPWSABUF		lpCallerData,
	LPWSABUF		lpCalleeData,
	LPQOS			lpSQOS,
	LPQOS			lpGQOS,
	LPINT			lpErrno
);

SOCKET WSPAPI WSPAccept(
	SOCKET			s,
	struct sockaddr FAR *addr,
	LPINT			addrlen,
	LPCONDITIONPROC	lpfnCondition,
	DWORD			dwCallbackData,
	LPINT			lpErrno
);
 
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
);

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
);

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
);
 
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
);

void CALLBACK CompletionRoutine (
	IN    DWORD				dwError, 
	IN    DWORD				cbTransferred, 
	IN    LPWSAOVERLAPPED	lpOverlapped, 
	IN    DWORD				dwFlags 
);

#endif