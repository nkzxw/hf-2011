#ifndef  ___HOOK_SHADOWSSDT_IO_CONTROL___
#define	 ___HOOK_SHADOWSSDT_IO_CONTROL___

#define		HIDE_PROCESS_WIN32_DEV_NAME    L"\\Device\\HookShadowSSDT"
#define		HIDE_PROCESS_DEV_NAME	      L"\\DosDevices\\HookShadowSSDT"


#define		FILE_DEVICE_HIDE_PROCESS		 0x00008811

#define		IO_PROTECT          		(ULONG) CTL_CODE(FILE_DEVICE_HIDE_PROCESS, 0x808, METHOD_NEITHER, FILE_ANY_ACCESS)


/************************************************************************
*                                                                      *
*                             Struct Define                            *
*                                                                      *
************************************************************************/

#define ObjectNameInformation  1

#define SystemHandleInformation 0x10

typedef struct _SYSTEM_HANDLE_INFORMATION {
	ULONG ProcessId;
	UCHAR ObjectTypeNumber;
	UCHAR Flags;
	USHORT Handle;
	PVOID Object;
	ACCESS_MASK GrantedAccess;
} _SYSTEM_HANDLE_INFORMATION, *P_SYSTEM_HANDLE_INFORMATION;


typedef struct _SYSTEM_HANDLE_INformATION_EX {
	ULONG NumberOfHandles;
	_SYSTEM_HANDLE_INFORMATION Information[1];
} _SYSTEM_HANDLE_INFORMATION_EX, *PSYSTEM_HANDLE_INFORMATION_EX;

typedef struct ServiceDescriptorEntry {
	PVOID *ServiceTableBase;
	ULONG *ServiceCounterTableBase; //Used only in checked build
	ULONG NumberOfServices;
	PVOID *ParamTableBase;
} ServiceDescriptorTableEntry, *PServiceDescriptorTableEntry;

PServiceDescriptorTableEntry KeServiceDescriptorTableShadow;

#endif