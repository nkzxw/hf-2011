/*
 * WehnTrust
 *
 * Copyright (c) 2005, Wehnus.
 */
#ifndef _WEHNTRUST_NRER_NRER_H
#define _WEHNTRUST_NRER_NRER_H

#define  USER_MODE
#include "../Common/Common.h"
#include "native.h"
#include "resource.h"
#include "SEH.h"
#include "format.h"

void nrememcpy(void *dst, void *src, int len);
void nrememset(void *dst, int val, int len);

typedef struct _EXCEPTION_REGISTRATION_RECORD
{
	struct _EXCEPTION_REGISTRATION_RECORD *Next;
	LPVOID                                Handler;
} EXCEPTION_REGISTRATION_RECORD, *PEXCEPTION_REGISTRATION_RECORD;

////
//
// Globalized variables.
//
////
extern NRER_NT_DISPATCH_TABLE DispatchTable;
extern ULONG                  NreExecutionFlags;

#define IsNreExecutionFlag(Flag) ((NreExecutionFlags & Flag) == Flag)

#ifndef NtCurrentProcess
#define NtCurrentProcess() ((HANDLE)(LONG_PTR)-1)
#endif

#ifndef NtCurrentThread
#define NtCurrentThread() ((HANDLE)(LONG_PTR)-2)
#endif

#endif
