/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		misc.h
 *
 * Abstract:
 *
 *		This module definies various types used by miscellaneous routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 23-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __MISC_H__
#define __MISC_H__


#include <NTDDK.h>
#include "pathproc.h"
#include "log.h"


// win2k ddk does not know about ExFreePoolWithTag?
/*
#ifndef ExFreePoolWithTag
#define	ExFreePoolWithTag(p, tag)	ExFreePool(p)
#endif
*/

#define	_POOL_TAG			'nozO'


/* convert seconds into units of 100 nanoseconds for KeWaitFor* & KeDelayExecutionThread functions */
#define	SECONDS(s)	((s) * -10000000)


#define ProbeAndReadUnicodeString(Source)  \
    (((Source) >= (UNICODE_STRING * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile UNICODE_STRING * const)MM_USER_PROBE_ADDRESS) : (*(volatile UNICODE_STRING *)(Source)))


/* extern defines */

KPROCESSOR_MODE
KeGetPreviousMode(
	VOID
	);


int sprintf(char *buffer, const char *format, ...);
int _snprintf(char *buffer, size_t count, const char *format, ...);
int _snwprintf(wchar_t *buffer, size_t count, const wchar_t *format, ...);

BOOLEAN	LearningModePostBootup();


#define	CURRENT_PROCESS_PID		((ULONG) PsGetCurrentProcessId())


#define	ON		1
#define	OFF		0


/* internal defines */

extern BOOLEAN		BootingUp;


INT32	atoi(IN PCHAR buf);
PCHAR	itoa(int value, char *string, unsigned int radix);

/* XXX move to netmisc.c */
ULONG	ntohl(IN ULONG netlong);
USHORT	ntohs(IN USHORT netshort);

ULONG	inet_addr(IN PCCHAR cp);
VOID	inet_ntoa(ULONG ina, PCHAR buf);
PCHAR	inet_ntoa2(IN ULONG ina);

BOOLEAN VerifyUnicodeString(IN PUNICODE_STRING InputUnicodeString, OUT PUNICODE_STRING OutputUnicodeString);
BOOLEAN	VerifyPwstr(IN PWSTR InputString, IN ULONG InputStringLength);

BOOLEAN	ReadStringRegistryValueA(IN PWSTR RegistryPath, IN PWSTR KeyName, OUT PCHAR Buffer, IN USHORT BufferSize);
BOOLEAN	ReadStringRegistryValueW(IN PWSTR RegistryPath, IN PWSTR KeyName, OUT PWSTR Buffer, IN USHORT BufferSize);
BOOLEAN	ReadSymlinkValue(IN PWSTR SymlinkPath, OUT PCHAR Buffer, IN USHORT BufferSize);

VOID	InitPostBootup();
PCHAR	GetCurrentUserSid(PUSHORT Size);

PVOID	ExchangeReadOnlyMemoryPointer(IN OUT PVOID *Target, IN PVOID Value);


#endif	/* __MISC_H__ */