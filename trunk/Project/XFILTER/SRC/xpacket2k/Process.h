/*！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
	file:			process.h
	project:		xfilter personal firewall 2.0
	create date:	2002-01-28
	Comments:		get process info, header file
	author:			tony zhu
	email:			xstudio@xfilt.com or xstudio@371.net
	url:			http://www.xfilt.com
	warning:		...
	copyright (c) 2002-2003 xstudio.di All Right Reserved.
*///！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！

//
// Process base address from PsGetCurrentProcess()
//
#ifndef __PROCESS_H__
#define __PROCESS_H__


#define BASE_PROCESS_PEB_OFFSET					0x01B0
#define BASE_PEB_PROCESS_PARAMETER_OFFSET		0x0010
#define BASE_PROCESS_PARAMETER_FULL_IMAGE_NAME	0x003C

#define BASE_PROCESS_NAME_OFFSET				0x01FC
#define BASE_PROCESS_NAME_OFFSET_XP				0x0174

#define PS_SUCCESS					0
#define PS_BUFFER_SO_SMALL			-1
#define PS_INVALID_PARAMETER		-2

#define PS_SYSTEM_PROCESS			100
#define PS_USER_PROCESS				101

ULONG GetCurrentTime(unsigned char* Week, ULONG* pTime);
INT GetProcessFileName(char* buf, DWORD nSize, BOOL IsOnlyName);

PCWSTR PsGetModuleFileNameW();
char* PsGetProcessName();

#endif // __PROCESS_H__
