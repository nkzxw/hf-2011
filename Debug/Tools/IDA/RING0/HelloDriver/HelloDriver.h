/***********************************************************************
HelloDriver.h

Purpose:
	Practise write base driver program.

Author:
	yhf	(hongfu830202@163.com)
CreateTime:
	2011-5-19 22:45:00
***********************************************************************/

#ifndef __HELLO_DRIVER_H__
#define __HELLO_DRIVER_H__

#pragma once 

#ifdef __cplusplus
{
	extern "C"
#endif 
	#include <ntddk.h>

#ifdef __cplusplus
}
#endif 

/**
* 分页内存和非分页内存
*
* 1. 当程序的中断请求级在大于等于DISPATCH_LEVEL时，程序只能使用非分页内存
* 2. #pragma PAGED_CODE  加载程序到分页内存
* 3. #pragma LOCKEDCODE 加载程序到非分页内存
* 4. #pragma INITCODE 程序初始化时加载到内存，然后就可以从内存中卸载掉。
*/
#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg ()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg ()
#define INITDATA data_seg ("INIT")

typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT pDeviceObject;
	UNICODE_STRING deviceName;
	UNICODE_STRING symbolLinkName;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION; 

typedef struct _MYDATASTRUCT 
{
	LIST_ENTRY ListEntry;
	ULONG number;
}MYDATASTRUCT, *PMYDATASTRUCT;

VOID PrintProcessName (PWCHAR pRoutine)
{
	PEPROCESS pEProcess = PsGetCurrentProcess ();
	PTSTR ProcessName = (PTSTR)((ULONG)pEProcess + 0x174);

	KdPrint (("%S Current Process %s.\n", pRoutine, ProcessName));
}

/**
* Routine Description:
* This routine looks to see if SubString is a substring of String.  This
* does a case insensitive test.

* Arguments:
* String - the string to search in
* SubString - the substring to find in String

* Return Value:
* Returns TRUE if the substring is found in string and FALSE otherwise.
**/
BOOLEAN
SpyFindSubString (
		__in PUNICODE_STRING String,
		__in PUNICODE_STRING SubString)
{
	
		ULONG index;

		//
		//  First, check to see if the strings are equal.
		//

		if (RtlEqualUnicodeString( String, SubString, TRUE )) {
				return TRUE;
		}

		//
		//  String and SubString aren't equal, so now see if SubString
		//  is in String any where.
		//		
		for (index = 0; index + (SubString->Length/sizeof(WCHAR)) <= (String->Length/sizeof(WCHAR));index++){
			if (_wcsnicmp( &String->Buffer[index],SubString->Buffer,(SubString->Length / sizeof(WCHAR)) ) == 0){
				//
				//  SubString is found in String, so return TRUE.
				//
				return TRUE;
			}
		}

	  return FALSE;
}

#define HELLO_DRIVER_DEVICE_NAME1  L"\\device\\HelloDriver_1"
#define HELLO_DRIVER_DEVICE_NAME2  L"\\device\\HelloDriver_2"
#define HELLO_DRIVER_DEVICE_NAME3  L"\\device\\HelloDriver_3"

#define HELLO_DRIVER_SYMBOL_NAME1  L"\\??\\HelloDriver_1"
#define HELLO_DRIVER_SYMBOL_NAME2  L"\\??\\HelloDriver_2"
#define HELLO_DRIVER_SYMBOL_NAME3  L"\\??\\HelloDriver_3"

#endif //__HELLO_DRIVER_H__
