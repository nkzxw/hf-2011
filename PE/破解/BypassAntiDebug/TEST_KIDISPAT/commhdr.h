
#ifndef ___HDR____
#define ___HDR____

#ifdef __cplusplus
extern "C" {
#endif

#include <ntifs.h>
#include <wdm.h>
#include <windef.h>
#include <ntimage.h>
#include <stdio.h>


#include "newstruct.h"
#include "dbgk.h"
#include "dbgkp.h"
#include "ntos.h"
#include "ntdbg.h"
#include "ntpsapi.h"

#ifndef PORT_MAXIMUM_MESSAGE_LENGTH
#define PORT_MAXIMUM_MESSAGE_LENGTH 512
#endif



	typedef struct HX_DYNC_FUNCTION
	{
		char FunName[50];
		DWORD *FunVar;
		DWORD FunAddr;
		DWORD	bNeedHook;	//this DWORD is for the inline hook purpose
		DWORD	delta;
}HX_DYNC_FUNCTION,*PHX_DYNC_FUNCTION;	
	
typedef	struct	__REPLACEAPIINFO__
{
	char			*FunName;
	ULONG		uOriValue;
	PULONG		uOriLocation;
	ULONG		type;
}REPLACEAPIINFO,*PREPLACEAPIINFO;
#include "stdlib.h"
#include "stdio.h"
#include <stdarg.h>

extern	PVOID	g_DebugPort;
extern	KSEMAPHORE	*g_Ksmp;
extern	ULONG	g_TargetEP;


inline void kprintf(char *fmt,...)
{
	char mybuffer[2048]={0};
	char myformat[218]={0};
//	strcpy(myformat, fmt);
	sprintf(myformat, "[SUPPER-Dispatch]:%s",  fmt);
	va_list	val;
	va_start(val, fmt);
	int icount=_vsnprintf(mybuffer,2048,myformat, val);
	if (icount>2048)
	{
		DbgPrint("warning...buffer overflow....\r\n");
		va_end(val);
		return ;
	}

	DbgPrint(mybuffer);

	va_end(val);

}

/*
NTSTATUS
PsLookupProcessByProcessId(
    IN HANDLE ProcessId,
    OUT PEPROCESS *Process
    );
*/
	
typedef enum _SYSTEMINFOCLASS
{
SystemBasicInformation, // 0x002C
SystemProcessorInformation, // 0x000C
SystemPerformanceInformation, // 0x0138
SystemTimeInformation, // 0x0020
SystemPathInformation, // not implemented
SystemProcessInformation, // 0x00C8+ per process
SystemCallInformation, // 0x0018 + (n * 0x0004)
SystemConfigurationInformation, // 0x0018
SystemProcessorCounters, // 0x0030 per cpu
SystemGlobalFlag, // 0x0004 (fails if size != 4)
SystemCallTimeInformation, // not implemented
SystemModuleInformation, // 0x0004 + (n * 0x011C)
SystemLockInformation, // 0x0004 + (n * 0x0024)
SystemStackTraceInformation, // not implemented
SystemPagedPoolInformation, // checked build only
SystemNonPagedPoolInformation, // checked build only
SystemHandleInformation, // 0x0004 + (n * 0x0010)
SystemObjectTypeInformation, // 0x0038+ + (n * 0x0030+)
SystemPageFileInformation, // 0x0018+ per page file
SystemVdmInstemulInformation, // 0x0088
SystemVdmBopInformation, // invalid info class
SystemCacheInformation, // 0x0024
SystemPoolTagInformation, // 0x0004 + (n * 0x001C)
SystemInterruptInformation, // 0x0000, or 0x0018 per cpu
SystemDpcInformation, // 0x0014
SystemFullMemoryInformation, // checked build only
SystemLoadDriver, // 0x0018, set mode only
SystemUnloadDriver, // 0x0004, set mode only
SystemTimeAdjustmentInformation, // 0x000C, 0x0008 writeable
SystemSummaryMemoryInformation, // checked build only
SystemNextEventIdInformation, // checked build only
SystemEventIdsInformation, // checked build only
SystemCrashDumpInformation, // 0x0004
SystemExceptionInformation, // 0x0010
SystemCrashDumpStateInformation, // 0x0004
SystemDebuggerInformation, // 0x0002
SystemContextSwitchInformation, // 0x0030
SystemRegistryQuotaInformation, // 0x000C
SystemAddDriver, // 0x0008, set mode only
SystemPrioritySeparationInformation,// 0x0004, set mode only
SystemPlugPlayBusInformation, // not implemented
SystemDockInformation, // not implemented
SystemPowerInfo, // 0x0060 (XP only!)
SystemProcessorSpeedInformation, // 0x000C (XP only!)
SystemTimeZoneInformation, // 0x00AC
SystemLookasideInformation, // n * 0x0020
SystemSetTimeSlipEvent,
SystemCreateSession, // set mode only
SystemDeleteSession, // set mode only
SystemInvalidInfoClass1, // invalid info class
SystemRangeStartInformation, // 0x0004 (fails if size != 4)
SystemVerifierInformation,
SystemAddVerifier,
SystemSessionProcessesInformation, // checked build only
MaxSystemInfoClass
} SYSTEMINFOCLASS, *PSYSTEMINFOCLASS;

#ifdef __cplusplus
}
#endif
#include "WorkKit.h"

#endif
