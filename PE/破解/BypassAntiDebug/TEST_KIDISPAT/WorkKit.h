#ifndef __WORKKIT__
#define __WORKKIT__



//////////////////////////////////////////////////////////////////////////

#include <ntddk.h>
#include "TEST_KIDISPAT.h"
BOOLEAN	IdentiyTarget(PPROTECT_INFO pProInfo);
BOOLEAN GetGargetEPbyName(UNICODE_STRING	*unitmi);
void GetProcessNameOffset() ;
void CreateProcessNotifyRoutine(IN HANDLE ParentId, IN HANDLE ProcessId, IN BOOLEAN Create);
typedef struct _DRIVER_INFO 
{
	ULONG   Unknown1;
	PVOID	BaseAddress;
	ULONG	Size;
	ULONG   Unknown3;
	ULONG	Index;
	ULONG   Unknown4;
	CHAR	PathName[0x104];
}DRIVER_INFO,*PDRIVER_INFO;

typedef struct _SYSTEM_INFO_DRIVERS 
{
	ULONG	NumberOfDrivers;
	ULONG	Reserved;
	DRIVER_INFO Drivers[0x100];
}SYSTEM_INFO_DRIVERS,*PSYSTEM_INFO_DRIVERS;

typedef	struct	_HOOKINFO
{
	LIST_ENTRY		Next;
	char				szFunName[128];
	ULONG			OriAddress;
	char				szOldCode[24];
	ULONG			OldCodeSize;
	ULONG			NewAddress;

}HOOKINFO ,*PHOOKINFO;

#define kmalloc(X)	ExAllocatePoolWithTag(NonPagedPool, (X), ' kid')
#define kfree(x)		ExFreePool((x))


extern LIST_ENTRY	g_HookInfoListHead;


extern "C"
PVOID	myReturnDebugPort(EPROCESS*);
ULONG GetMoudleBase(char* ModuleName );
VOID WPOFF();
VOID WPON();
NTSTATUS	WithdrawWhenUnload();
NTSTATUS	PatchDebugPortCheckInKiDispatch(PUCHAR osNewBaseAddress=NULL);
ULONG CheckDebugPort();
ULONG GetOsMoudleInfo(PULONG	pModuleSize, char* ModuleName=NULL, ULONG uLen =NULL);
extern "C"
NTSYSAPI
NTSTATUS
NTAPI ZwQuerySystemInformation(
							   IN ULONG SystemInformationClass,
							   IN PVOID SystemInformation,
							   IN ULONG SystemInformationLength,
							   OUT PULONG ReturnLength);
#endif