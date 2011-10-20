#if !defined(ADDDEFINES__INCLUDED_)
#define ADDDEFINES__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//includes
#include "StdAfx.h"
#include "winsock2.h"

#ifndef WIN_NT
	#define WIN_NT 4
#endif
	
#ifndef WIN_XX
	#define WIN_XX 0
#endif

#ifndef EVENT_TIMER
	#define EVENT_TIMER WM_USER + 0x101
#endif

#ifndef EVENT_TIMER_NEW_TCP_CONN //used to track every second the tcptable stack changes
	#define EVENT_TIMER_NEW_TCP_CONN WM_USER + 0x102
#endif

#ifndef EVENT_TIMER_REFRESH_LIST
	#define EVENT_TIMER_REFRESH_LIST WM_USER + 0x103
#endif

#ifndef WM_ICON_NOTIFY
	#define WM_ICON_NOTIFY  WM_USER + 0x104
#endif
	
#ifndef LOGMSG
	#define LOGMSG WM_USER + 0x108
#endif


#ifndef QWORD
	#define QWORD 4 * sizeof(WORD)
#endif

typedef struct	_LOGPARAM
{
	DWORD	pdwLoggedEntries;
	DWORD	pdwLostEntries;
	DWORD	pdwSizeUsed;
} LOGPARAM;

//vars
typedef struct	_FILTER
{
	DWORD				dwProto;
	CString				strLocalAddress;
	u_short				ulLocalPort;
	CString				strRemoteAddress;
	u_short				ulRemotePort;
	PFFORWARD_ACTION	eAction;	//forward/drop
	eDirection			eInOut;
} FILTER;

typedef struct	TCPTABLE
{
	CString strLocalAddress;
	u_long	ulLocalPort;
	CString strRemoteAddress;
	u_long	ulRemotePort;
	CString strConnectionState;
	CString strProcessName;
	DWORD	dwPid;
} _TCPTABLE;

typedef struct	_MIB_TCPROW_EX
{
	DWORD	dwState;
	DWORD	dwLocalAddr;
	DWORD	dwLocalPort;
	DWORD	dwRemoteAddr;
	DWORD	dwRemotePort;
	DWORD	dwProcessId;
}
MIB_TCPROW_EX, *PMIB_TCPROW_EX;

typedef struct _MIB_TCPTABLE_EX
{
	DWORD			dwNumEntries;
	MIB_TCPROW_EX	table[ANY_SIZE];
}
MIB_TCPTABLE_EX, *PMIB_TCPTABLE_EX;

typedef struct UDPTABLE
{
	CString strLocalAddress;
	u_long	ulLocalPort;
	CString strProcessName;
	DWORD	dwPid;
	u_long	ulDataAmmount;
} _UDPTABLE;

typedef struct	_MIB_UDPROW_EX
{
	DWORD	dwLocalAddr;
	DWORD	dwLocalPort;
	DWORD	dwProcessId;
}
MIB_UDPROW_EX, *PMIB_UDPROW_EX;

typedef struct _MIB_UDPTABLE_EX
{
	DWORD			dwNumEntries;
	MIB_UDPROW_EX	table[ANY_SIZE];
}
MIB_UDPTABLE_EX, *PMIB_UDPTABLE_EX;

//methods
//imported function from iphlpapi.dll
typedef DWORD (WINAPI *pAllocateAndGetTcpExTableFromStack)
	(
		PMIB_TCPTABLE_EX * pTcpTableEx,
		BOOL,
		HANDLE,
		DWORD,						//0
		DWORD
	);			//2
typedef DWORD (WINAPI *pAllocateAndGetUdpExTableFromStack)
	(
		PMIB_UDPTABLE_EX * pUdpTableEx,
		BOOL,
		HANDLE,
		DWORD,	//0
		DWORD
	);			//2

//imported function from psapi.dll
//ToolHelp function used for all winver, but not for NT
#ifndef TH32CS_SNAPPROCESS_MY
#define TH32CS_SNAPPROCESS_MY	0x02
#endif
#ifndef ULONG_PTR
#define ULONG_PTR	DWORD
#endif
typedef struct	_PROCESSENTRY32_MY
{
	DWORD		dwSize;
	DWORD		cntUsage;
	DWORD		th32ProcessID;
	ULONG_PTR	th32DefaultHeapID;
	DWORD		th32ModuleID;
	DWORD		cntThreads;
	DWORD		th32ParentProcessID;
	LONG		pcPriClassBase;
	DWORD		dwFlags;
	CHAR		szExeFile[MAX_PATH];
}
PROCESSENTRY32_MY, *LPPROCESSENTRY32_MY;

typedef HANDLE (WINAPI *pCreateToolhelp32Snapshot) (DWORD, DWORD);

typedef BOOL (WINAPI *pProcess32First) (HANDLE, LPPROCESSENTRY32_MY);

typedef BOOL (WINAPI *pProcess32Next) (HANDLE, LPPROCESSENTRY32_MY);
#endif //(!defined ADDDEFINES__INCLUDED_)
