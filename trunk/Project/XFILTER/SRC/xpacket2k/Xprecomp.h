/*！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
	file:			xprecomp.h
	project:		xfilter personal firewall 2.0
	create date:	2002-01-22
	Comments:		packet filter driver precompiler header file
	author:			tony zhu
	email:			xstudio@xfilt.com or xstudio@371.net
	url:			http://www.xfilt.com
	warning:		...
	copyright (c) 2002-2003 xstudio.di All Right Reserved.
*///！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！

#ifndef _XPRECOMP_H
#define _XPRECOMP_H

#define USE_XFILTER
#define KERNEL_MODE

#define BYTE			unsigned char
#define DWORD			unsigned __int32
#define WORD			unsigned __int16
#define HMODULE			HANDLE
#define MAX_PATH		260
#define NT_PROCNAMELEN  16
#define BOOL			BOOLEAN

#define VWIN32_GetCurrentProcessHandle	PsGetCurrentProcess
#define dprintf				KdPrint

#define malloc(nSize)		ExAllocatePool(NonPagedPool, nSize)
#define free(pBuffer)		ExFreePool(pBuffer)

#define strnicmp			_strnicmp

#include <stdio.h>
#include <ndis.h>
#include "NtApi.h"
#include "NdisHook.h"
#include "xfilter.h"
#include "xpacket.h"
#include "..\Common\ControlCode.h"
#include "process.h"
#include "Packet.h"
#include "NetBios.h"
#include "PacketBuffer.h"
#include "MemoryAcl.h"

#endif // _XPRECOMP_H