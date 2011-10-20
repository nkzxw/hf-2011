/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		hookproc.c
 *
 * Abstract:
 *
 *		This module implements various service operation (system call) hooking routines.
 *
 *		Since not all the Zw* services are exported, we use a hack to find the required information.
 *		ntdll.dll contains stubs for calling all the system services. All stubs start with
 *		"mov eax, function_index" instruction where function_index is an index into a global
 *		system call table. By parsing ntdll.dll and extracting all the function indeces
 *		we can map all the Zw* names to their appropriate system call table indeces.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 16-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */

#include <NTDDK.h>
#include "hookproc.h"
#include "sysinfo.h"
#include "ntproto.h"
#include "learn.h"
#include "file.h"
#include "registry.h"
#include "section.h"
#include "sysinfo.h"
#include "semaphore.h"
#include "dirobj.h"
#include "symlink.h"
#include "mutant.h"
#include "event.h"
#include "port.h"
#include "job.h"
#include "token.h"
#include "timer.h"
#include "driverobj.h"
#include "process.h"
#include "procname.h"
#include "time.h"
#include "atom.h"
#include "vdm.h"
#include "debug.h"
#include "i386.h"
#include "misc.h"
#include "log.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitSyscallsHooks)
#pragma alloc_text (INIT, InstallSyscallsHooks)
#pragma alloc_text (PAGE, RemoveSyscallsHooks)
#endif


#if DBG
int		HookedRoutineRunning = 0;
#endif

PCHAR	NTDLL_Base;

int ZwCallsNumber = 0;



/*
 * FindFunctionOffset()
 *
 * Description:
 *		Finds a Zw* system call offset in a system service table.
 *
 *		Implementation of all the Zw* system services (such as ZwOpenFile or ZwCreateProcess())
 *		start with a "mov eax, function_offset" instruction. Function offset is extracted from
 *		the first instruction.
 *
 * Parameters:
 *		Function - Pointer to the function code.
 *
 * Returns:
 *		Integer function offset (-1 in case of a failure).
 */

/* "MOV EAX,IMM32" opcode */
#define	OPCODE_MOV	0xB8

/* macro shortcut for bailing out of FindFunctionOffset in case of an error */

#define	ABORT_FindFunctionOffset(msg) {																\
		LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("Error occured in FindFunctionOffset():"));		\
		LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, msg);												\
		return -1;																					\
	}

int
FindFunctionOffset(PULONG_PTR Function)
{
	PUCHAR		Instruction;
	ULONG		num, ServiceIDNumber, ServiceTableIndex;


	/*
	 * Make sure that the service code starts with a MOV EAX,IMM32 instruction:
	 *
	 * lkd> u ZwCreateFile
	 * nt!ZwCreateFile:
	 * 804f86f4 b825000000       mov     eax,0x25
	 */

	Instruction = (PUCHAR) Function;

	if (*Instruction != OPCODE_MOV)

		ABORT_FindFunctionOffset(("Invalid opcode %x\n", *Instruction));


	/*
	 * Extract the Service Descriptor Table index (4 bytes following the mov opcode)
	 *
	 * The index format is as follows:
	 *
	 * Leading 18 bits are all zeroes
	 * Following 2 bits are system service table index (3 bits on Win64)
	 * Following 12 bits are service number
	 */

	num = * (PULONG) ++Instruction;


	/* only SERVICE_TABLE_INDEX_BITS LSB bits should be set */

	ServiceTableIndex = num >> SERVICE_ID_NUMBER_BITS;

	if (ServiceTableIndex >= NUMBER_SERVICE_TABLES)

		ABORT_FindFunctionOffset(("Invalid SSDT index: %x (%x)\n", ServiceTableIndex, num));


	/* XXX temporary? There exist 4 (8 on IA64) service tables. All the Zw* system services are in table 0 */
	if (ServiceTableIndex != 0)

		ABORT_FindFunctionOffset(("Invalid SSDT index2: %x (%x)\n", ServiceTableIndex, num));


	/* Verify Service ID Number is in range */

	ServiceIDNumber = num & SERVICE_ID_NUMBER_MASK;

//XXX shouldn't we be using the shadow table instead??
//shadow table Zw* base address is the same in addition to GUI table
	if (ServiceIDNumber > KeServiceDescriptorTable[ServiceTableIndex].NumberOfServices)

		ABORT_FindFunctionOffset(("Invalid service id number %d (max is %d)\n", ServiceIDNumber, KeServiceDescriptorTable[ServiceTableIndex].NumberOfServices));


	return ServiceIDNumber;
}



#if 0

/*
 * HookSystemService()
 *
 * Description:
 *		Replaces an existing sytem service pointer (n a global system service table) with another function pointer.
 *
 * Parameters:
 *		OldService - Pointer to the service code to mediate.
 *		NewService - Pointer to the new function code.
 *
 * Returns:
 *		Current OldService indexed system service function pointer.
 */

/* macro shortcut for bailing out of HookSystemService in case of an error */

#define	ABORT_HookSystemService(msg) {																\
		LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("Error occured in HookSystemService():"));		\
		LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, (msg));											\
		return NULL;																				\
	}

PVOID
HookSystemService(PVOID OldService, PVOID NewService)
{
	PULONG_PTR	ssdt;
	PULONG_PTR	retptr = NULL;
	ULONG		ServiceIDNumber;


	if (OldService == NULL || NewService == NULL)

		ABORT_HookSystemService(("NULL detected. OldService=%x NewService=%x", OldService, NewService));


	ServiceIDNumber = FindFunctionOffset(OldService);

	if (ServiceIDNumber == -1)

		ABORT_HookSystemService(("FindFunctionOffset(%x) failed", OldService));


	ssdt = KeServiceDescriptorTable[0].ServiceTableBase;


	retptr = (PULONG_PTR) ssdt[ServiceIDNumber];

	if (retptr == NULL)

		ABORT_HookSystemService(("ssdt[index] = NULL\n"));


	if (((ULONG) retptr & SystemAddressStart) == 0)

		ABORT_HookSystemService(("invalid code instruction specified\n"));


	retptr = ExchangeReadOnlyMemoryPointer((PVOID *) &ssdt[ServiceIDNumber], NewService);


	return retptr;
}

#endif



/*
 * HookSystemServiceByIndex()
 *
 * Description:
 *		Replaces an existing sytem service (n a global system service table) with another function pointer.
 *
 * Parameters:
 *		ServiceIDNumber - Index of a system service to mediate.
 *		NewService - Pointer to the new function code.
 *
 * Returns:
 *		Current ServiceIDNumber indexed system service function pointer.
 */

/* macro shortcut for bailing out of HookSystemServiceByIndex in case of an error */

#define	ABORT_HookSystemServiceByIndex(msg) {															\
		LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("Error occured in HookSystemServiceByIndex():"));		\
		LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, msg);													\
		return NULL;																					\
	}

PVOID
HookSystemServiceByIndex(ULONG ServiceIDNumber, PVOID NewService)
{
	PULONG_PTR	ssdt;
	PULONG_PTR	retptr = NULL;
	ULONG		ServiceTableIndex = 0;


	ssdt = KeServiceDescriptorTable[ServiceTableIndex].ServiceTableBase;


	/* Verify Service ID Number is in range */

//XXX shouldn't we be using the shadow table instead??
	if (ServiceIDNumber > KeServiceDescriptorTable[ServiceTableIndex].NumberOfServices)

		ABORT_HookSystemServiceByIndex(("Invalid service id number %d (max is %d)\n", ServiceIDNumber, KeServiceDescriptorTable[ServiceTableIndex].NumberOfServices));


	retptr = (PULONG_PTR) ssdt[ServiceIDNumber];

	if (retptr == NULL)

		ABORT_HookSystemServiceByIndex(("ssdt[index] = NULL\n"));


	if (((ULONG) retptr & SystemAddressStart) == 0)

		ABORT_HookSystemServiceByIndex(("invalid code instruction specified\n"));

	
	retptr = ExchangeReadOnlyMemoryPointer((PVOID *) &ssdt[ServiceIDNumber], NewService);


	return retptr;
}


#if 0
/*
 * FindSystemServiceByIndex()
 *
 * Description:XXX
 *		Replaces an existing sytem service (n a global system service table) with another function pointer.
 *
 * Parameters:
 *		ServiceIDNumber - Index of a system service to mediate.
 *		NewService - Pointer to the new function code.
 *
 * Returns:
 *		Current ServiceIDNumber indexed system service function pointer.
 */

/* macro shortcut for bailing out of HookSystemServiceByIndex in case of an error */

#define	ABORT_FindSystemServiceByIndex(msg) { LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("Error occured in FindSystemServiceByIndex():")); LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, msg); return NULL; }

PULONG
FindSystemServiceByIndex(ULONG ServiceIDNumber)
{
	PULONG_PTR	ssdt;
	PULONG_PTR	retptr = NULL;
	ULONG		ServiceTableIndex = 0;


	ssdt = KeServiceDescriptorTable[ServiceTableIndex].ServiceTableBase;


	/* Verify Service ID Number is in range */

//XXX shouldn't we be using the shadow table instead??
	if (ServiceIDNumber > KeServiceDescriptorTable[ServiceTableIndex].NumberOfServices)

		ABORT_FindSystemServiceByIndex(("Invalid service id number %d (max is %d)\n", ServiceIDNumber, KeServiceDescriptorTable[ServiceTableIndex].NumberOfServices));


	retptr = (PULONG_PTR) ssdt[ServiceIDNumber];

	if (retptr == NULL)

		ABORT_FindSystemServiceByIndex(("ssdt[index] = NULL\n"));


	if (((ULONG) retptr & SystemAddressStart) == 0)

		ABORT_FindSystemServiceByIndex(("invalid code instruction specified\n"));


	return (PULONG) ssdt[ServiceIDNumber];
}
#endif


/*
 * HookSystemServiceByName()
 *
 * Description:
 *		Replaces an existing sytem service (n a global system service table) with another function pointer.
 *
 * Parameters:
 *		ServiceName - Name of a Zw* system service to mediate.
 *		HookFunction - Pointer to the mediator function code.
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

BOOLEAN
HookSystemServiceByName(PCHAR ServiceName, PULONG_PTR HookFunction)
{
	int		i;


	for (i = 0; i < ZwCallsNumber; i++)
	{
		if (strlen(ServiceName) == ZwCalls[i].ZwNameLength && _stricmp(ServiceName, ZwCalls[i].ZwName + 2) == 0)
		{
//			LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("HookSystemServiceByName: Matched rule: %x\n", ZwCalls[i].ServiceIDNumber));

			// hijack the syscall if not hijacked already
			if (ZwCalls[i].Hijacked == FALSE && ZwCalls[i].ServiceIDNumber != -1)
			{
				if ((ZwCalls[i].OriginalFunction = HookSystemServiceByIndex(ZwCalls[i].ServiceIDNumber,
												HookFunction ? HookFunction : ZwCalls[i].HookFunction)) != NULL)
				{
					ZwCalls[i].Hijacked = TRUE;
				}
			}

			return TRUE;
		}
	}


	return FALSE;
}



#if 0
/*
 * FindSystemServiceIndex()
 *
 * Description:
 *		Find a system service index for a specified service name.
 *
 * Parameters:
 *		ServiceName - Name of a Zw* system service to find.
 *
 * Returns:
 *		System service index if found, -1 otherwise.
 */

ULONG
FindSystemServiceIndex(PCHAR ServiceName)
{
	int		i;


	for (i = 0; i < ZwCallsNumber; i++)
	{
		if (strlen(ServiceName) == ZwCalls[i].ZwNameLength && _stricmp(ServiceName, ZwCalls[i].ZwName + 2) == 0)
		{
//			LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("FindSystemServiceByName: Matched rule: %x\n", ZwCalls[i].ServiceIDNumber));

			return ZwCalls[i].ServiceIDNumber;
		}
	}


	return -1;
}
#endif


/*
 * FindSystemServiceNumber()
 *
 * Description:
 *		Find a system service number for a specified service name.
 *
 * Parameters:
 *		ServiceName - Name of a Zw* system service to find.
 *
 * Returns:
 *		System service number if found, -1 otherwise.
 */

ULONG
FindSystemServiceNumber(PCHAR ServiceName)
{
	int		i;


	for (i = 0; i < ZwCallsNumber; i++)
	{
		if (strlen(ServiceName) == ZwCalls[i].ZwNameLength && _stricmp(ServiceName, ZwCalls[i].ZwName + 2) == 0)
		{
//			LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("FindSystemServiceByName: Matched rule: %x\n", ZwCalls[i].ServiceIDNumber));

			return i;
		}
	}


	return -1;
}



/*
 * Find_NTDLL_Base()
 *
 * Description:
 *		Returns the base address of mapped ntdll.dll in a "System" process context.
 *
 *		NOTE: Based on "Windows NT/2000 Native API Reference" implementation.
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		ntdll.dll base address (NULL if not found).
 */

PVOID
Find_NTDLL_Base()
{
	ULONG						size, i;
	PVOID						ntdll = NULL;
	PULONG						SystemInfo;
	PSYSTEM_MODULE_INFORMATION	pSMI;
	NTSTATUS					status;


	/*
	 * The data returned to the SystemInformation buffer is a ULONG count of the number of
	 * modules followed immediately by an array of SYSTEM_MODULE_INFORMATION.
	 *
	 * The system modules are the Portable Executable (PE) format files loaded into the
	 * kernel address space (ntoskrnl.exe, hal.dll, device drivers, and so on) and ntdll.dll.
	 */


	/* first, find out the total amount of module information to be returned */

	status = ZwQuerySystemInformation(SystemModuleInformation, &size, 0, &size);
	if (size == 0 || size > 1024*1024)
	{
		LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("Find_NTDLL_Base: ZwQuerySystemInformation failed. status=%x size=%d\n", status, size));
		return NULL;
	}


	/* second, allocate the required amount of memory */
	
	SystemInfo = ExAllocatePoolWithTag(PagedPool, size + 4, _POOL_TAG);
	if (SystemInfo == NULL)
	{
		LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("Find_NTDLL_Base: out of memory (requested %d bytes)\n", size + 4));
		return NULL;
	}


	/* third, request the module information */

	ZwQuerySystemInformation(SystemModuleInformation, SystemInfo, size, &i);

	if (size != ((*SystemInfo * sizeof(SYSTEM_MODULE_INFORMATION)) + 4))
	{
		LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("Find_NTDLL_Base: inconsistent size (%d != %d * %d + 4)\n", size, *SystemInfo, sizeof(SYSTEM_MODULE_INFORMATION)));
		return NULL;
	}



	pSMI = (PSYSTEM_MODULE_INFORMATION) (SystemInfo + 1);

	for (i = 0; i < *SystemInfo; i++)
	{
//		LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("comparing '%s' (base 0x%x)\n", pSMI[i].ImageName + pSMI[i].ModuleNameOffset, pSMI[i].Base));

		if (_stricmp(pSMI[i].ImageName + pSMI[i].ModuleNameOffset, "ntdll.dll") == 0)
		{
			LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_VERBOSE, ("Find_NTDLL_Base: ntdll.dll base address is %x\n", pSMI[i].Base));
			ntdll = pSMI[i].Base;
			break;
		}
	}

	ExFreePoolWithTag(SystemInfo, _POOL_TAG);


	return ntdll;
}


#if 0
PVOID
Find_Kernel32_Base2()
{
	PVOID						Kernel32 = NULL;
	NTSTATUS					status;
	OBJECT_ATTRIBUTES			ObjectAttributes;
	IO_STATUS_BLOCK				isb;
	HANDLE						FileHandle;
	UNICODE_STRING				usName;
	PVOID						BaseAddress = NULL;
	SIZE_T						Size;
	CHAR						buffer[256];


//	RtlInitUnicodeString(&usName, L"\\SystemRoot\\System32\\kernel32.dll");
	RtlInitUnicodeString(&usName, L"\\??\\c:\\windows\\system32\\kernel32.dll");
	InitializeObjectAttributes(&ObjectAttributes, &usName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

	status = ZwCreateFile(&FileHandle, GENERIC_READ, &ObjectAttributes, &isb, NULL, 0, 0, 0, 0, NULL, 0);
	if (!NT_SUCCESS(status))
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("Find_Kernel32_Base: Failed to open file %S (%x)\n", usName.Buffer, status));
		return FALSE;
	}


	status = ZwReadFile(FileHandle, NULL, NULL, NULL, &isb, (PVOID) buffer, sizeof(buffer) - 1,	0, NULL);

	if (! NT_SUCCESS(status))
	{
		if (status != STATUS_END_OF_FILE)
		{
			LOG(LOG_SS_POLICY, LOG_PRIORITY_DEBUG, ("LoadSecurityPolicy: ZwReadFile failed rc=%x\n", status));
		}
	}

	ZwClose(FileHandle);


	return Kernel32;
}



PVOID
Find_Kernel32_Base()
{
	PVOID						Kernel32 = NULL;
	NTSTATUS					status;
	OBJECT_ATTRIBUTES			ObjectAttributes;
	HANDLE						SectionHandle;
	UNICODE_STRING				usName;
	PVOID						BaseAddress = NULL;
	SIZE_T						Size;
	NTSTATUS					q;


	RtlInitUnicodeString(&usName, L"\\KnownDlls\\kernel32.dll");
	InitializeObjectAttributes(&ObjectAttributes, &usName, OBJ_KERNEL_HANDLE, NULL, NULL);

	if (NT_SUCCESS( ZwOpenSection(&SectionHandle, SECTION_MAP_READ, &ObjectAttributes) ))
	{
		q = ZwMapViewOfSection(SectionHandle, NtCurrentProcess(), &BaseAddress, 0, 0, NULL, &Size, ViewShare, MEM_RESERVE, PAGE_READWRITE);
		if (NT_SUCCESS(q))
		{
			KdPrint(("XXX mapped kernel32.dll at BaseAddress %x\n", BaseAddress));

			if (!NT_SUCCESS( ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress) ))
			{
				KdPrint(("ZwUnmapViewOfSection failed"));
			}

			ZwClose(SectionHandle);
		}
		else
		{
			KdPrint(("ZwMapViewOfSection failed with status %x", q));
		}
	}
	else
	{
		KdPrint(("ZwOpenSection failed"));
	}

	return Kernel32;
}
#endif


/*
 * FindFunctionBase()
 *
 * Description:
 *		Finds the address where a function is mapped at.
 *
 *		NOTE: Based on "Windows NT/2000 Native API Reference" implementation.
 *
 * Parameters:
 *		Base - Mapped address of a DLL exporting the function.
 *		Name - Function name.
 *
 * Returns:
 *		Address where a function is mapped at (NULL if not found).
 */

/* macro shortcut for bailing out of FindFunctionBase in case of an error */

#define	ABORT_FindFunctionBase(msg) {																\
		LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("Error occured in FindFunctionBase():"));			\
		LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, msg);												\
		return NULL;																				\
	}

PVOID
FindFunctionBase(PCHAR ImageBase, PCSTR Name)
{
	PIMAGE_DOS_HEADER		DosHeader;
	PIMAGE_NT_HEADERS		PeHeader;
	PIMAGE_DATA_DIRECTORY	ImageExportDirectoryEntry;
	ULONG					ExportDirectorySize, ExportDirectoryOffset, i;
	PIMAGE_EXPORT_DIRECTORY	ExportDirectory;
	PULONG					ExportAddressTable;
	PSHORT					ExportOrdinalTable;
	PULONG					ExportNameTable;


	if ( (DosHeader = (PIMAGE_DOS_HEADER) ImageBase) == NULL)

		ABORT_FindFunctionBase(("Base pointer is NULL (name = %s)\n", Name));


	if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)

		ABORT_FindFunctionBase(("DOS Signature not found! (%x %x)\n", DosHeader, DosHeader->e_magic));


	if (DosHeader->e_lfanew > 1024*1024)

		ABORT_FindFunctionBase(("Invalid e_lfanew value %x\n", DosHeader->e_lfanew));


	if ( (PeHeader = (PIMAGE_NT_HEADERS) (ImageBase + DosHeader->e_lfanew)) == NULL)

		ABORT_FindFunctionBase(("NT header pointer is NULL (name = %s)\n", Name));


	if (PeHeader->Signature != IMAGE_PE_SIGNATURE)

		ABORT_FindFunctionBase(("PE Signature not found! (%x %x)\n", PeHeader, PeHeader->Signature));


	if ( (ImageExportDirectoryEntry = PeHeader->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_EXPORT) == NULL)

		ABORT_FindFunctionBase(("Export directory pointer is NULL (name = %s)\n", Name));


	ExportDirectorySize = ImageExportDirectoryEntry->Size;
	ExportDirectoryOffset = ImageExportDirectoryEntry->VirtualAddress;

	if ( (ExportDirectory = (PIMAGE_EXPORT_DIRECTORY) (ImageBase + ExportDirectoryOffset)) == NULL)

		ABORT_FindFunctionBase(("Exports pointer is NULL (name = %s)\n", Name));


	ExportAddressTable = (PULONG) (ImageBase + ExportDirectory->AddressOfFunctions);
	ExportOrdinalTable = (PSHORT) (ImageBase + ExportDirectory->AddressOfNameOrdinals);
	ExportNameTable = (PULONG) (ImageBase + ExportDirectory->AddressOfNames);

//XXX "AcceptConnectPort\0" \0 necessary to make sure ZwCreateProcess does not match ZwCreateProcessEx ?

	for (i = 0; i < ExportDirectory->NumberOfNames; i++)
	{
		ULONG ord = ExportOrdinalTable[i];

		if (ExportAddressTable[ord] < ExportDirectoryOffset ||
			ExportAddressTable[ord] >= ExportDirectoryOffset + ExportDirectorySize)
		{
			//XXX windows loader uses binary search?
			if (strcmp(ImageBase + ExportNameTable[i], Name) == 0)
			{
				return ImageBase + ExportAddressTable[ord];
			}
		}
	}

	return NULL;
}



/*
 * FindZwFunctionIndex()
 *
 * Description:
 *		Finds a system service table index of a Zw* system service.
 *
 *		NOTE: Based on "Windows NT/2000 Native API Reference" implementation.
 *
 * Parameters:
 *		Name - Zw* service name.
 *
 * Returns:
 *		Zw* service index in a system service table (-1 if not found).
 */

int
FindZwFunctionIndex(PCSTR Name)
{
	PULONG_PTR	FunctionBase;


	if (NTDLL_Base == NULL)
		if ( (NTDLL_Base = Find_NTDLL_Base()) == NULL)
			return -1;


	if ( (FunctionBase = FindFunctionBase(NTDLL_Base, Name)) == NULL)
	{
//		LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("FindZwFunctionIndex: FindZwFunctionBase(%s) returned NULL", Name));
		return -1;
	}


	return FindFunctionOffset(FunctionBase);
}



/*
 * ZwCalls structure describes all known system services.
 * Part of the structure (i.e. system call table offset and system call address)
 * is filled in at runtime.
 *
 * Automatically generated with
 * dumpbin /exports c:\windows2003\system32\ntdll.dll|grep Zw|
 *		perl -wnle "print qq{\t{ \"$1\", NULL, (PULONG_PTR) SystemCallHandler$i, NULL, FALSE },} if /(Zw.*)/; ++$i" > win2k3_syscalls
 *
 * perl -wne "if (/\"Zw(.*?)\"/) { $q=length $1; s/\"Zw(.*?)\"/\"Zw$1\", $q/} print" q.txt > q2.txt
 */

struct _ZwCalls /* {
	PCHAR			ZwName;				// System call name
	USHORT			ZwNameLength;		// System call name length
	USHORT			ServiceIDNumber;	// System call index (filled in at runtime)
	PULONG_PTR		HookFunction;		// Address of the hijacking function (function that will be called instead of the original system call)
	PULONG_PTR		OriginalFunction;	// PlaceHolder for the address of the original syscall address
	BOOLEAN			Hijacked;			// Flag indicating whether we already hijacked this system call
										// or whether this is a special system service that needs to be hijacked initially
} */ ZwCalls[] =
{
	{ "ZwAcceptConnectPort", 17, -1, (PULONG_PTR) SystemCallHandler0, NULL, FALSE },
	{ "ZwAccessCheck", 11, -1, (PULONG_PTR) SystemCallHandler1, NULL, FALSE },
	{ "ZwAccessCheckAndAuditAlarm", 24, -1, (PULONG_PTR) SystemCallHandler2, NULL, FALSE },
	{ "ZwAccessCheckByType", 17, -1, (PULONG_PTR) SystemCallHandler3, NULL, FALSE },
	{ "ZwAccessCheckByTypeAndAuditAlarm", 30, -1, (PULONG_PTR) SystemCallHandler4, NULL, FALSE },
	{ "ZwAccessCheckByTypeResultList", 27, -1, (PULONG_PTR) SystemCallHandler5, NULL, FALSE },
	{ "ZwAccessCheckByTypeResultListAndAuditAlarm", 40, -1, (PULONG_PTR) SystemCallHandler6, NULL, FALSE },
	{ "ZwAccessCheckByTypeResultListAndAuditAlarmByHandle", 48, -1, (PULONG_PTR) SystemCallHandler7, NULL, FALSE },


#if HOOK_ATOM
	{ "ZwAddAtom", 7, -1, (PULONG_PTR) HookedNtAddAtom, NULL, TRUE },
#else
	{ "ZwAddAtom", 7, -1, (PULONG_PTR) SystemCallHandler8, NULL, FALSE },
#endif


//XXX
	{ "ZwAddBootEntry", 12, -1, (PULONG_PTR) SystemCallHandler9, NULL, FALSE },
	{ "ZwAddDriverEntry", 14, -1, (PULONG_PTR) SystemCallHandler10, NULL, FALSE },


#if HOOK_TOKEN_ZZZ
	{ "ZwAdjustGroupsToken", 17, -1, (PULONG_PTR) HookedNtAdjustGroupsToken, NULL, TRUE },
#else
	{ "ZwAdjustGroupsToken", 17, -1, (PULONG_PTR) SystemCallHandler11, NULL, FALSE },
#endif

#if HOOK_TOKEN
	{ "ZwAdjustPrivilegesToken", 21, -1, (PULONG_PTR) HookedNtAdjustPrivilegesToken, NULL, TRUE },
#else
	{ "ZwAdjustPrivilegesToken", 21, -1, (PULONG_PTR) SystemCallHandler12, NULL, FALSE },
#endif


	{ "ZwAlertResumeThread", 17, -1, (PULONG_PTR) SystemCallHandler13, NULL, FALSE },
	{ "ZwAlertThread", 11, -1, (PULONG_PTR) SystemCallHandler14, NULL, FALSE },
	{ "ZwAllocateLocallyUniqueId", 23, -1, (PULONG_PTR) SystemCallHandler15, NULL, FALSE },
	{ "ZwAllocateUserPhysicalPages", 25, -1, (PULONG_PTR) SystemCallHandler16, NULL, FALSE },
	{ "ZwAllocateUuids", 13, -1, (PULONG_PTR) SystemCallHandler17, NULL, FALSE },
	{ "ZwAllocateVirtualMemory", 21, -1, (PULONG_PTR) SystemCallHandler18, NULL, FALSE },
	{ "ZwApphelpCacheControl", 19, -1, (PULONG_PTR) SystemCallHandler19, NULL, FALSE },
	{ "ZwAreMappedFilesTheSame", 21, -1, (PULONG_PTR) SystemCallHandler20, NULL, FALSE },
	{ "ZwAssignProcessToJobObject", 24, -1, (PULONG_PTR) SystemCallHandler21, NULL, FALSE },
	{ "ZwCallbackReturn", 14, -1, (PULONG_PTR) SystemCallHandler22, NULL, FALSE },
	{ "ZwCancelDeviceWakeupRequest", 25, -1, (PULONG_PTR) SystemCallHandler23, NULL, FALSE },
	{ "ZwCancelIoFile", 12, -1, (PULONG_PTR) SystemCallHandler24, NULL, FALSE },
	{ "ZwCancelTimer", 11, -1, (PULONG_PTR) SystemCallHandler25, NULL, FALSE },
	{ "ZwClearEvent", 10, -1, (PULONG_PTR) SystemCallHandler26, NULL, FALSE },

	/* don't mediate for performance reasons, requires a valid handle anyway  */
	{ "ZwClose", 5, -1, NULL, NULL, FALSE },

	{ "ZwCloseObjectAuditAlarm", 21, -1, (PULONG_PTR) SystemCallHandler28, NULL, FALSE },
	{ "ZwCompactKeys", 11, -1, (PULONG_PTR) SystemCallHandler29, NULL, FALSE },
	{ "ZwCompareTokens", 13, -1, (PULONG_PTR) SystemCallHandler30, NULL, FALSE },
	{ "ZwCompleteConnectPort", 19, -1, (PULONG_PTR) SystemCallHandler31, NULL, FALSE },
	{ "ZwCompressKey", 11, -1, (PULONG_PTR) SystemCallHandler32, NULL, FALSE },


#if HOOK_PORT
	{ "ZwConnectPort", 11, -1, (PULONG_PTR) HookedNtConnectPort, NULL, TRUE },
#else
	{ "ZwConnectPort", 11, -1, (PULONG_PTR) SystemCallHandler33, NULL, FALSE },
#endif


	{ "ZwContinue", 8, -1, (PULONG_PTR) SystemCallHandler34, NULL, FALSE },


#if HOOK_DEBUG_ZZZ
	{ "ZwCreateDebugObject", 17, -1, (PULONG_PTR) HookedNtCreateDebugObject, NULL, TRUE },
#else
	{ "ZwCreateDebugObject", 17, -1, (PULONG_PTR) SystemCallHandler35, NULL, FALSE },
#endif


#if HOOK_DIROBJ
	{ "ZwCreateDirectoryObject", 21, -1, (PULONG_PTR) HookedNtCreateDirectoryObject, NULL, TRUE },
#else
	{ "ZwCreateDirectoryObject", 21, -1, (PULONG_PTR) SystemCallHandler36, NULL, FALSE },
#endif


#if HOOK_EVENT
	{ "ZwCreateEvent", 11, -1, (PULONG_PTR) HookedNtCreateEvent, NULL, TRUE },
	{ "ZwCreateEventPair", 15, -1, (PULONG_PTR) HookedNtCreateEventPair, NULL, TRUE },
#else
	{ "ZwCreateEvent", 11, -1, (PULONG_PTR) SystemCallHandler37, NULL, FALSE },
	{ "ZwCreateEventPair", 15, -1, (PULONG_PTR) SystemCallHandler38, NULL, FALSE },
#endif


#if HOOK_FILE
	{ "ZwCreateFile", 10, -1, (PULONG_PTR) HookedNtCreateFile, NULL, TRUE },
#else
	{ "ZwCreateFile", 10, -1, (PULONG_PTR) SystemCallHandler39, NULL, FALSE },
#endif


	{ "ZwCreateIoCompletion", 18, -1, (PULONG_PTR) SystemCallHandler40, NULL, FALSE },


#if HOOK_JOB
	{ "ZwCreateJobObject", 15, -1, (PULONG_PTR) HookedNtCreateJobObject, NULL, TRUE },
#else
	{ "ZwCreateJobObject", 15, -1, (PULONG_PTR) SystemCallHandler41, NULL, FALSE },
#endif


//XXX ???
	{ "ZwCreateJobSet", 12, -1, (PULONG_PTR) SystemCallHandler42, NULL, FALSE },
	

#if HOOK_REGISTRY
	{ "ZwCreateKey", 9, -1, (PULONG_PTR) HookedNtCreateKey, NULL, TRUE },
#else
	{ "ZwCreateKey", 9, -1, (PULONG_PTR) SystemCallHandler43, NULL, FALSE },
#endif


	{ "ZwCreateKeyedEvent", 16, -1, (PULONG_PTR) SystemCallHandler44, NULL, FALSE },


#if HOOK_FILE
	{ "ZwCreateMailslotFile", 18, -1, (PULONG_PTR) HookedNtCreateMailslotFile, NULL, TRUE },
#else
	{ "ZwCreateMailslotFile", 18, -1, (PULONG_PTR) SystemCallHandler45, NULL, FALSE },
#endif


#if HOOK_MUTANT
	{ "ZwCreateMutant", 12, -1, (PULONG_PTR) HookedNtCreateMutant, NULL, TRUE },
#else
	{ "ZwCreateMutant", 12, -1, (PULONG_PTR) SystemCallHandler46, NULL, FALSE },
#endif


#if HOOK_FILE
	{ "ZwCreateNamedPipeFile", 19, -1, (PULONG_PTR) HookedNtCreateNamedPipeFile, NULL, TRUE },
#else
	{ "ZwCreateNamedPipeFile", 19, -1, (PULONG_PTR) SystemCallHandler47, NULL, FALSE },
#endif


	{ "ZwCreatePagingFile", 16, -1, (PULONG_PTR) SystemCallHandler48, NULL, FALSE },


#if HOOK_PORT
	{ "ZwCreatePort", 10, -1, (PULONG_PTR) HookedNtCreatePort, NULL, TRUE },
#else
	{ "ZwCreatePort", 10, -1, (PULONG_PTR) SystemCallHandler49, NULL, FALSE },
#endif


#if HOOK_PROCESS
	{ "ZwCreateProcess", 13, -1, (PULONG_PTR) HookedNtCreateProcess, NULL, TRUE },
	{ "ZwCreateProcessEx", 15, -1, (PULONG_PTR) HookedNtCreateProcessEx, NULL, TRUE },
#else
	{ "ZwCreateProcess", 13, -1, (PULONG_PTR) SystemCallHandler50, NULL, FALSE },
	{ "ZwCreateProcessEx", 15, -1, (PULONG_PTR) SystemCallHandler51, NULL, FALSE },
#endif


	{ "ZwCreateProfile", 13, -1, (PULONG_PTR) SystemCallHandler52, NULL, FALSE },


#if HOOK_SECTION
	{ "ZwCreateSection", 13, -1, (PULONG_PTR) HookedNtCreateSection, NULL, TRUE },
#else
	{ "ZwCreateSection", 13, -1, (PULONG_PTR) SystemCallHandler53, NULL, FALSE },
#endif


#if HOOK_SEMAPHORE
	{ "ZwCreateSemaphore", 15, -1, (PULONG_PTR) HookedNtCreateSemaphore, NULL, TRUE },
#else
	{ "ZwCreateSemaphore", 15, -1, (PULONG_PTR) SystemCallHandler54, NULL, FALSE },
#endif


#if HOOK_SYMLINK
	{ "ZwCreateSymbolicLinkObject", 24, -1, (PULONG_PTR) HookedNtCreateSymbolicLinkObject, NULL, TRUE },
#else
	{ "ZwCreateSymbolicLinkObject", 24, -1, (PULONG_PTR) SystemCallHandler55, NULL, FALSE },
#endif


#if HOOK_PROCESS
	{ "ZwCreateThread", 12, -1, (PULONG_PTR) HookedNtCreateThread, NULL, TRUE },
#else
	{ "ZwCreateThread", 12, -1, (PULONG_PTR) SystemCallHandler56, NULL, FALSE },
#endif


#if HOOK_TIMER
	{ "ZwCreateTimer", 11, -1, (PULONG_PTR) HookedNtCreateTimer, NULL, TRUE },
#else
	{ "ZwCreateTimer", 11, -1, (PULONG_PTR) SystemCallHandler57, NULL, FALSE },
#endif


#if HOOK_TOKEN_ZZZ
	{ "ZwCreateToken", 11, -1, (PULONG_PTR) HookedNtCreateToken, NULL, TRUE },
#else
	{ "ZwCreateToken", 11, -1, (PULONG_PTR) SystemCallHandler58, NULL, FALSE },
#endif


#if HOOK_PORT
	{ "ZwCreateWaitablePort", 18, -1, (PULONG_PTR) HookedNtCreateWaitablePort, NULL, TRUE },
#else
	{ "ZwCreateWaitablePort", 18, -1, (PULONG_PTR) SystemCallHandler59, NULL, FALSE },
#endif


#if HOOK_DEBUG
	{ "ZwDebugActiveProcess", 18, -1, (PULONG_PTR) HookedNtDebugActiveProcess, NULL, TRUE },
#else
	{ "ZwDebugActiveProcess", 18, -1, (PULONG_PTR) SystemCallHandler60, NULL, FALSE },
#endif


	{ "ZwDebugContinue", 13, -1, (PULONG_PTR) SystemCallHandler61, NULL, FALSE },
	{ "ZwDelayExecution", 14, -1, (PULONG_PTR) SystemCallHandler62, NULL, FALSE },
	{ "ZwDeleteAtom", 10, -1, (PULONG_PTR) SystemCallHandler63, NULL, FALSE },
	{ "ZwDeleteBootEntry", 15, -1, (PULONG_PTR) SystemCallHandler64, NULL, FALSE },
	{ "ZwDeleteDriverEntry", 17, -1, (PULONG_PTR) SystemCallHandler65, NULL, FALSE },


#if HOOK_FILE
	{ "ZwDeleteFile", 10, -1, (PULONG_PTR) HookedNtDeleteFile, NULL, TRUE },
#else
	{ "ZwDeleteFile", 10, -1, (PULONG_PTR) SystemCallHandler66, NULL, FALSE },
#endif


#if HOOK_REGISTRY
	{ "ZwDeleteKey", 9, -1, (PULONG_PTR) HookedNtDeleteKey, NULL, TRUE },
#else
	{ "ZwDeleteKey", 9, -1, (PULONG_PTR) SystemCallHandler67, NULL, FALSE },
#endif

	{ "ZwDeleteObjectAuditAlarm", 22, -1, (PULONG_PTR) SystemCallHandler68, NULL, FALSE },
	{ "ZwDeleteValueKey", 14, -1, (PULONG_PTR) SystemCallHandler69, NULL, FALSE },
//XXX

	{ "ZwDeviceIoControlFile", 19, -1, NULL, NULL, FALSE },
	{ "ZwDisplayString", 13, -1, (PULONG_PTR) SystemCallHandler71, NULL, FALSE },
	{ "ZwDuplicateObject", 15, -1, (PULONG_PTR) SystemCallHandler72, NULL, FALSE },
	{ "ZwDuplicateToken", 14, -1, (PULONG_PTR) SystemCallHandler73, NULL, FALSE },
	{ "ZwEnumerateBootEntries", 20, -1, (PULONG_PTR) SystemCallHandler74, NULL, FALSE },
	{ "ZwEnumerateDriverEntries", 22, -1, (PULONG_PTR) SystemCallHandler75, NULL, FALSE },
	{ "ZwEnumerateKey", 12, -1, (PULONG_PTR) SystemCallHandler76, NULL, FALSE },
	{ "ZwEnumerateSystemEnvironmentValuesEx", 34, -1, (PULONG_PTR) SystemCallHandler77, NULL, FALSE },
	{ "ZwEnumerateValueKey", 17, -1, (PULONG_PTR) SystemCallHandler78, NULL, FALSE },
//XXX


	{ "ZwExtendSection", 13, -1, (PULONG_PTR) SystemCallHandler79, NULL, FALSE },
	{ "ZwFilterToken", 11, -1, (PULONG_PTR) SystemCallHandler80, NULL, FALSE },


#if HOOK_ATOM
	{ "ZwFindAtom", 8, -1, (PULONG_PTR) HookedNtFindAtom, NULL, TRUE },
#else
	{ "ZwFindAtom", 8, -1, (PULONG_PTR) SystemCallHandler81, NULL, FALSE },
#endif


	{ "ZwFlushBuffersFile", 16, -1, (PULONG_PTR) SystemCallHandler82, NULL, FALSE },
	{ "ZwFlushInstructionCache", 21, -1, (PULONG_PTR) SystemCallHandler83, NULL, FALSE },
	{ "ZwFlushKey", 8, -1, (PULONG_PTR) SystemCallHandler84, NULL, FALSE },
	{ "ZwFlushVirtualMemory", 18, -1, (PULONG_PTR) SystemCallHandler85, NULL, FALSE },
	{ "ZwFlushWriteBuffer", 16, -1, (PULONG_PTR) SystemCallHandler86, NULL, FALSE },
	{ "ZwFreeUserPhysicalPages", 21, -1, (PULONG_PTR) SystemCallHandler87, NULL, FALSE },
	{ "ZwFreeVirtualMemory", 17, -1, (PULONG_PTR) SystemCallHandler88, NULL, FALSE },
	{ "ZwFsControlFile", 13, -1, (PULONG_PTR) SystemCallHandler89, NULL, FALSE },
	{ "ZwGetContextThread", 16, -1, (PULONG_PTR) SystemCallHandler90, NULL, FALSE },
	{ "ZwGetCurrentProcessorNumber", 25, -1, (PULONG_PTR) SystemCallHandler91, NULL, FALSE },
	{ "ZwGetDevicePowerState", 19, -1, (PULONG_PTR) SystemCallHandler92, NULL, FALSE },
	{ "ZwGetPlugPlayEvent", 16, -1, (PULONG_PTR) SystemCallHandler93, NULL, FALSE },
	{ "ZwGetWriteWatch", 13, -1, (PULONG_PTR) SystemCallHandler94, NULL, FALSE },

//XXX
	{ "ZwImpersonateAnonymousToken", 25, -1, (PULONG_PTR) SystemCallHandler95, NULL, FALSE },
	{ "ZwImpersonateClientOfPort", 23, -1, (PULONG_PTR) SystemCallHandler96, NULL, FALSE },
	{ "ZwImpersonateThread", 17, -1, (PULONG_PTR) SystemCallHandler97, NULL, FALSE },

	{ "ZwInitializeRegistry", 18, -1, (PULONG_PTR) SystemCallHandler98, NULL, FALSE },
	{ "ZwInitiatePowerAction", 19, -1, (PULONG_PTR) SystemCallHandler99, NULL, FALSE },
	{ "ZwIsProcessInJob", 14, -1, (PULONG_PTR) SystemCallHandler100, NULL, FALSE },
	{ "ZwIsSystemResumeAutomatic", 23, -1, (PULONG_PTR) SystemCallHandler101, NULL, FALSE },
	{ "ZwListenPort", 10, -1, (PULONG_PTR) SystemCallHandler102, NULL, FALSE },


#if HOOK_DRIVEROBJ
	{ "ZwLoadDriver", 10, -1, (PULONG_PTR) HookedNtLoadDriver, NULL, TRUE },
#else
	{ "ZwLoadDriver", 10, -1, (PULONG_PTR) SystemCallHandler103, NULL, FALSE },
#endif


	{ "ZwLoadKey", 7, -1, (PULONG_PTR) SystemCallHandler104, NULL, FALSE },
	{ "ZwLoadKey2", 8, -1, (PULONG_PTR) SystemCallHandler105, NULL, FALSE },
	{ "ZwLoadKeyEx", 9, -1, (PULONG_PTR) SystemCallHandler106, NULL, FALSE },
	{ "ZwLockFile", 8, -1, (PULONG_PTR) SystemCallHandler107, NULL, FALSE },
	{ "ZwLockProductActivationKeys", 25, -1, (PULONG_PTR) SystemCallHandler108, NULL, FALSE },
	{ "ZwLockRegistryKey", 15, -1, (PULONG_PTR) SystemCallHandler109, NULL, FALSE },
	{ "ZwLockVirtualMemory", 17, -1, (PULONG_PTR) SystemCallHandler110, NULL, FALSE },
	{ "ZwMakePermanentObject", 19, -1, (PULONG_PTR) SystemCallHandler111, NULL, FALSE },
	{ "ZwMakeTemporaryObject", 19, -1, (PULONG_PTR) SystemCallHandler112, NULL, FALSE },
	{ "ZwMapUserPhysicalPages", 20, -1, (PULONG_PTR) SystemCallHandler113, NULL, FALSE },
	{ "ZwMapUserPhysicalPagesScatter", 27, -1, (PULONG_PTR) SystemCallHandler114, NULL, FALSE },


#if HOOK_SECTION_ZZZ
	{ "ZwMapViewOfSection", 16, -1, (PULONG_PTR) HookedNtMapViewOfSection, NULL, TRUE },
#else
	{ "ZwMapViewOfSection", 16, -1, (PULONG_PTR) SystemCallHandler115, NULL, FALSE },
#endif


	{ "ZwModifyBootEntry", 15, -1, (PULONG_PTR) SystemCallHandler116, NULL, FALSE },
	{ "ZwModifyDriverEntry", 17, -1, (PULONG_PTR) SystemCallHandler117, NULL, FALSE },
	{ "ZwNotifyChangeDirectoryFile", 25, -1, (PULONG_PTR) SystemCallHandler118, NULL, FALSE },
	{ "ZwNotifyChangeKey", 15, -1, (PULONG_PTR) SystemCallHandler119, NULL, FALSE },
	{ "ZwNotifyChangeMultipleKeys", 24, -1, (PULONG_PTR) SystemCallHandler120, NULL, FALSE },


#if HOOK_DIROBJ
	{ "ZwOpenDirectoryObject", 19, -1, (PULONG_PTR) HookedNtOpenDirectoryObject, NULL, TRUE },
#else
	{ "ZwOpenDirectoryObject", 19, -1, (PULONG_PTR) SystemCallHandler121, NULL, FALSE },
#endif


#if HOOK_EVENT
	{ "ZwOpenEvent", 9, -1, (PULONG_PTR) HookedNtOpenEvent, NULL, TRUE },
	{ "ZwOpenEventPair", 13, -1, (PULONG_PTR) HookedNtOpenEventPair, NULL, TRUE },
#else
	{ "ZwOpenEvent", 9, -1, (PULONG_PTR) SystemCallHandler122, NULL, FALSE },
	{ "ZwOpenEventPair", 13, -1, (PULONG_PTR) SystemCallHandler123, NULL, FALSE },
#endif


#if HOOK_FILE
	{ "ZwOpenFile", 8, -1, (PULONG_PTR) HookedNtOpenFile, NULL, TRUE },
#else
	{ "ZwOpenFile", 8, -1, (PULONG_PTR) SystemCallHandler124, NULL, FALSE },
#endif

	{ "ZwOpenIoCompletion", 16, -1, (PULONG_PTR) SystemCallHandler125, NULL, FALSE },


#if HOOK_JOB
	{ "ZwOpenJobObject", 13, -1, (PULONG_PTR) HookedNtOpenJobObject, NULL, TRUE },
#else
	{ "ZwOpenJobObject", 13, -1, (PULONG_PTR) SystemCallHandler126, NULL, FALSE },
#endif


#if HOOK_REGISTRY
	{ "ZwOpenKey", 7, -1, (PULONG_PTR) HookedNtOpenKey, NULL, TRUE },
#else
	{ "ZwOpenKey", 7, -1, (PULONG_PTR) SystemCallHandler127, NULL, FALSE },
#endif


	{ "ZwOpenKeyedEvent", 14, -1, (PULONG_PTR) SystemCallHandler128, NULL, FALSE },


#if HOOK_MUTANT
	{ "ZwOpenMutant", 10, -1, (PULONG_PTR) HookedNtOpenMutant, NULL, TRUE },
#else
	{ "ZwOpenMutant", 10, -1, (PULONG_PTR) SystemCallHandler129, NULL, FALSE },
#endif


	{ "ZwOpenObjectAuditAlarm", 20, -1, (PULONG_PTR) SystemCallHandler130, NULL, FALSE },


#if HOOK_PROCESS
	{ "ZwOpenProcess", 11, -1, (PULONG_PTR) HookedNtOpenProcess, NULL, TRUE },
#else
	{ "ZwOpenProcess", 11, -1, (PULONG_PTR) SystemCallHandler131, NULL, FALSE },
#endif


#if HOOK_TOKEN_ZZZ
	{ "ZwOpenProcessToken", 16, -1, (PULONG_PTR) HookedNtOpenProcessToken, NULL, TRUE },
	{ "ZwOpenProcessTokenEx", 18, -1, (PULONG_PTR) HookedNtOpenProcessTokenEx, NULL, TRUE },
#else
	{ "ZwOpenProcessToken", 16, -1, (PULONG_PTR) SystemCallHandler132, NULL, FALSE },
	{ "ZwOpenProcessTokenEx", 18, -1, (PULONG_PTR) SystemCallHandler133, NULL, FALSE },
#endif


#if HOOK_SECTION
	{ "ZwOpenSection", 11, -1, (PULONG_PTR) HookedNtOpenSection, NULL, TRUE },
#else
	{ "ZwOpenSection", 11, -1, (PULONG_PTR) SystemCallHandler134, NULL, FALSE },
#endif


#if HOOK_SEMAPHORE
	{ "ZwOpenSemaphore", 13, -1, (PULONG_PTR) HookedNtOpenSemaphore, NULL, TRUE },
#else
	{ "ZwOpenSemaphore", 13, -1, (PULONG_PTR) SystemCallHandler135, NULL, FALSE },
#endif


#if HOOK_SYMLINK_ZZZ
	{ "ZwOpenSymbolicLinkObject", 22, -1, (PULONG_PTR) HookedNtOpenSymbolicLinkObject, NULL, TRUE },
#else
	{ "ZwOpenSymbolicLinkObject", 22, -1, (PULONG_PTR) SystemCallHandler136, NULL, FALSE },
#endif


#if HOOK_PROCESS
	{ "ZwOpenThread", 10, -1, (PULONG_PTR) HookedNtOpenThread, NULL, TRUE },
#else
	{ "ZwOpenThread", 10, -1, (PULONG_PTR) SystemCallHandler137, NULL, FALSE },
#endif


#if HOOK_TOKEN_ZZZ
	{ "ZwOpenThreadToken", 15, -1, (PULONG_PTR) HookedNtOpenThreadToken, NULL, TRUE },
	{ "ZwOpenThreadTokenEx", 17, -1, (PULONG_PTR) HookedNtOpenThreadTokenEx, NULL, TRUE },
#else
	{ "ZwOpenThreadToken", 15, -1, (PULONG_PTR) SystemCallHandler138, NULL, FALSE },
	{ "ZwOpenThreadTokenEx", 17, -1, (PULONG_PTR) SystemCallHandler139, NULL, FALSE },
#endif


#if HOOK_TIMER
	{ "ZwOpenTimer", 9, -1, (PULONG_PTR) HookedNtOpenTimer, NULL, TRUE },
#else
	{ "ZwOpenTimer", 9, -1, (PULONG_PTR) SystemCallHandler140, NULL, FALSE },
#endif


	{ "ZwPlugPlayControl", 15, -1, (PULONG_PTR) SystemCallHandler141, NULL, FALSE },
	{ "ZwPowerInformation", 16, -1, (PULONG_PTR) SystemCallHandler142, NULL, FALSE },
	{ "ZwPrivilegeCheck", 14, -1, (PULONG_PTR) SystemCallHandler143, NULL, FALSE },
	{ "ZwPrivilegeObjectAuditAlarm", 25, -1, (PULONG_PTR) SystemCallHandler144, NULL, FALSE },
	{ "ZwPrivilegedServiceAuditAlarm", 27, -1, (PULONG_PTR) SystemCallHandler145, NULL, FALSE },
	{ "ZwProtectVirtualMemory", 20, -1, (PULONG_PTR) SystemCallHandler146, NULL, FALSE },
	{ "ZwPulseEvent", 10, -1, (PULONG_PTR) SystemCallHandler147, NULL, FALSE },


#if HOOK_FILE
	{ "ZwQueryAttributesFile", 19, -1, (PULONG_PTR) HookedNtQueryAttributesFile, NULL, TRUE },
#else
	{ "ZwQueryAttributesFile", 19, -1, (PULONG_PTR) SystemCallHandler148, NULL, FALSE },
#endif


	{ "ZwQueryBootEntryOrder", 19, -1, (PULONG_PTR) SystemCallHandler149, NULL, FALSE },
	{ "ZwQueryBootOptions", 16, -1, (PULONG_PTR) SystemCallHandler150, NULL, FALSE },
	{ "ZwQueryDebugFilterState", 21, -1, (PULONG_PTR) SystemCallHandler151, NULL, FALSE },
	{ "ZwQueryDefaultLocale", 18, -1, (PULONG_PTR) SystemCallHandler152, NULL, FALSE },
	{ "ZwQueryDefaultUILanguage", 22, -1, (PULONG_PTR) SystemCallHandler153, NULL, FALSE },


#if FILE_HOOK_ZZZ
	{ "ZwQueryDirectoryFile", 18, -1, (PULONG_PTR) HookedNtQueryDirectoryFile, NULL, TRUE },
#else
	{ "ZwQueryDirectoryFile", 18, -1, (PULONG_PTR) SystemCallHandler154, NULL, FALSE },
#endif


	{ "ZwQueryDirectoryObject", 20, -1, (PULONG_PTR) SystemCallHandler155, NULL, FALSE },
	{ "ZwQueryDriverEntryOrder", 21, -1, (PULONG_PTR) SystemCallHandler156, NULL, FALSE },
	{ "ZwQueryEaFile", 11, -1, (PULONG_PTR) SystemCallHandler157, NULL, FALSE },
	{ "ZwQueryEvent", 10, -1, (PULONG_PTR) SystemCallHandler158, NULL, FALSE },


#if HOOK_FILE
	{ "ZwQueryFullAttributesFile", 23, -1, (PULONG_PTR) HookedNtQueryFullAttributesFile, NULL, TRUE },
#else
	{ "ZwQueryFullAttributesFile", 23, -1, (PULONG_PTR) SystemCallHandler159, NULL, FALSE },
#endif


	{ "ZwQueryInformationAtom", 20, -1, (PULONG_PTR) SystemCallHandler160, NULL, FALSE },
	{ "ZwQueryInformationFile", 20, -1, (PULONG_PTR) SystemCallHandler161, NULL, FALSE },
	{ "ZwQueryInformationJobObject", 25, -1, (PULONG_PTR) SystemCallHandler162, NULL, FALSE },
	{ "ZwQueryInformationPort", 20, -1, (PULONG_PTR) SystemCallHandler163, NULL, FALSE },
	{ "ZwQueryInformationProcess", 23, -1, (PULONG_PTR) SystemCallHandler164, NULL, FALSE },
	{ "ZwQueryInformationThread", 22, -1, (PULONG_PTR) SystemCallHandler165, NULL, FALSE },
	{ "ZwQueryInformationToken", 21, -1, (PULONG_PTR) SystemCallHandler166, NULL, FALSE },
	{ "ZwQueryInstallUILanguage", 22, -1, (PULONG_PTR) SystemCallHandler167, NULL, FALSE },
	{ "ZwQueryIntervalProfile", 20, -1, (PULONG_PTR) SystemCallHandler168, NULL, FALSE },
	{ "ZwQueryIoCompletion", 17, -1, (PULONG_PTR) SystemCallHandler169, NULL, FALSE },
	{ "ZwQueryKey", 8, -1, (PULONG_PTR) SystemCallHandler170, NULL, FALSE },
	{ "ZwQueryMultipleValueKey", 21, -1, (PULONG_PTR) SystemCallHandler171, NULL, FALSE },
	{ "ZwQueryMutant", 11, -1, (PULONG_PTR) SystemCallHandler172, NULL, FALSE },
	{ "ZwQueryObject", 11, -1, (PULONG_PTR) SystemCallHandler173, NULL, FALSE },
	{ "ZwQueryOpenSubKeys", 16, -1, (PULONG_PTR) SystemCallHandler174, NULL, FALSE },
	{ "ZwQueryOpenSubKeysEx", 18, -1, (PULONG_PTR) SystemCallHandler175, NULL, FALSE },
	{ "ZwQueryPerformanceCounter", 23, -1, (PULONG_PTR) SystemCallHandler176, NULL, FALSE },
	{ "ZwQueryPortInformationProcess", 27, -1, (PULONG_PTR) SystemCallHandler177, NULL, FALSE },
	{ "ZwQueryQuotaInformationFile", 25, -1, (PULONG_PTR) SystemCallHandler178, NULL, FALSE },
	{ "ZwQuerySection", 12, -1, (PULONG_PTR) SystemCallHandler179, NULL, FALSE },
	{ "ZwQuerySecurityObject", 19, -1, (PULONG_PTR) SystemCallHandler180, NULL, FALSE },
	{ "ZwQuerySemaphore", 14, -1, (PULONG_PTR) SystemCallHandler181, NULL, FALSE },
	{ "ZwQuerySymbolicLinkObject", 23, -1, (PULONG_PTR) SystemCallHandler182, NULL, FALSE },
	{ "ZwQuerySystemEnvironmentValue", 27, -1, (PULONG_PTR) SystemCallHandler183, NULL, FALSE },
	{ "ZwQuerySystemEnvironmentValueEx", 29, -1, (PULONG_PTR) SystemCallHandler184, NULL, FALSE },
	{ "ZwQuerySystemInformation", 22, -1, (PULONG_PTR) SystemCallHandler185, NULL, FALSE },
	{ "ZwQuerySystemTime", 15, -1, (PULONG_PTR) SystemCallHandler186, NULL, FALSE },
	{ "ZwQueryTimer", 10, -1, (PULONG_PTR) SystemCallHandler187, NULL, FALSE },
	{ "ZwQueryTimerResolution", 20, -1, (PULONG_PTR) SystemCallHandler188, NULL, FALSE },


#if HOOK_REGISTRY_ZZZ
	{ "ZwQueryValueKey", 13, -1, (PULONG_PTR) HookedNtQueryValueKey, NULL, TRUE },
#else
	{ "ZwQueryValueKey", 13, -1, (PULONG_PTR) SystemCallHandler189, NULL, FALSE },
#endif


	{ "ZwQueryVirtualMemory", 18, -1, (PULONG_PTR) SystemCallHandler190, NULL, FALSE },
	{ "ZwQueryVolumeInformationFile", 26, -1, (PULONG_PTR) SystemCallHandler191, NULL, FALSE },
	{ "ZwQueueApcThread", 14, -1, (PULONG_PTR) SystemCallHandler192, NULL, FALSE },


//XXX should we not mediate these calls? they are only raised during an error and we might
// not encounter them during "learning" phase? can these be abused otherwise?
	{ "ZwRaiseException", 14, -1, (PULONG_PTR) SystemCallHandler193, NULL, FALSE },
	{ "ZwRaiseHardError", 14, -1, (PULONG_PTR) SystemCallHandler194, NULL, FALSE },

	/* don't mediate for performance reasons, requires a valid handle anyway  */
	{ "ZwReadFile", 8, -1, NULL, NULL, FALSE },
	{ "ZwReadFileScatter", 15, -1, NULL, NULL, FALSE },
	{ "ZwReadRequestData", 15, -1, NULL, NULL, FALSE },
	{ "ZwReadVirtualMemory", 17, -1, NULL, NULL, FALSE },

	{ "ZwRegisterThreadTerminatePort", 27, -1, (PULONG_PTR) SystemCallHandler199, NULL, FALSE },
	{ "ZwReleaseKeyedEvent", 17, -1, (PULONG_PTR) SystemCallHandler200, NULL, FALSE },
	{ "ZwReleaseMutant", 13, -1, (PULONG_PTR) SystemCallHandler201, NULL, FALSE },
	{ "ZwReleaseSemaphore", 16, -1, (PULONG_PTR) SystemCallHandler202, NULL, FALSE },
	{ "ZwRemoveIoCompletion", 18, -1, (PULONG_PTR) SystemCallHandler203, NULL, FALSE },
	{ "ZwRemoveProcessDebug", 18, -1, (PULONG_PTR) SystemCallHandler204, NULL, FALSE },

//XXX
	{ "ZwRenameKey", 9, -1, (PULONG_PTR) SystemCallHandler205, NULL, FALSE },
	{ "ZwReplaceKey", 10, -1, (PULONG_PTR) SystemCallHandler206, NULL, FALSE },


	{ "ZwReplyPort", 9, -1, (PULONG_PTR) SystemCallHandler207, NULL, FALSE },
	{ "ZwReplyWaitReceivePort", 20, -1, (PULONG_PTR) SystemCallHandler208, NULL, FALSE },
	{ "ZwReplyWaitReceivePortEx", 22, -1, (PULONG_PTR) SystemCallHandler209, NULL, FALSE },
	{ "ZwReplyWaitReplyPort", 18, -1, (PULONG_PTR) SystemCallHandler210, NULL, FALSE },
	{ "ZwRequestDeviceWakeup", 19, -1, (PULONG_PTR) SystemCallHandler211, NULL, FALSE },
	{ "ZwRequestPort", 11, -1, (PULONG_PTR) SystemCallHandler212, NULL, FALSE },
	{ "ZwRequestWaitReplyPort", 20, -1, (PULONG_PTR) SystemCallHandler213, NULL, FALSE },
	{ "ZwRequestWakeupLatency", 20, -1, (PULONG_PTR) SystemCallHandler214, NULL, FALSE },
	{ "ZwResetEvent", 10, -1, (PULONG_PTR) SystemCallHandler215, NULL, FALSE },
	{ "ZwResetWriteWatch", 15, -1, (PULONG_PTR) SystemCallHandler216, NULL, FALSE },
	{ "ZwRestoreKey", 10, -1, (PULONG_PTR) SystemCallHandler217, NULL, FALSE },
	{ "ZwResumeProcess", 13, -1, (PULONG_PTR) SystemCallHandler218, NULL, FALSE },
	{ "ZwResumeThread", 12, -1, (PULONG_PTR) SystemCallHandler219, NULL, FALSE },
	{ "ZwSaveKey", 7, -1, (PULONG_PTR) SystemCallHandler220, NULL, FALSE },
	{ "ZwSaveKeyEx", 9, -1, (PULONG_PTR) SystemCallHandler221, NULL, FALSE },
	{ "ZwSaveMergedKeys", 14, -1, (PULONG_PTR) SystemCallHandler222, NULL, FALSE },


#if HOOK_PORT
	{ "ZwSecureConnectPort", 17, -1, (PULONG_PTR) HookedNtSecureConnectPort, NULL, TRUE },
#else
	{ "ZwSecureConnectPort", 17, -1, (PULONG_PTR) SystemCallHandler223, NULL, FALSE },
#endif


	{ "ZwSetBootEntryOrder", 17, -1, (PULONG_PTR) SystemCallHandler224, NULL, FALSE },
	{ "ZwSetBootOptions", 14, -1, (PULONG_PTR) SystemCallHandler225, NULL, FALSE },

//XXX
	{ "ZwSetContextThread", 16, -1, (PULONG_PTR) SystemCallHandler226, NULL, FALSE },

	{ "ZwSetDebugFilterState", 19, -1, (PULONG_PTR) SystemCallHandler227, NULL, FALSE },
	{ "ZwSetDefaultHardErrorPort", 23, -1, (PULONG_PTR) SystemCallHandler228, NULL, FALSE },
	{ "ZwSetDefaultLocale", 16, -1, (PULONG_PTR) SystemCallHandler229, NULL, FALSE },
	{ "ZwSetDefaultUILanguage", 20, -1, (PULONG_PTR) SystemCallHandler230, NULL, FALSE },
	{ "ZwSetDriverEntryOrder", 19, -1, (PULONG_PTR) SystemCallHandler231, NULL, FALSE },
	{ "ZwSetEaFile", 9, -1, (PULONG_PTR) SystemCallHandler232, NULL, FALSE },
	{ "ZwSetEvent", 8, -1, (PULONG_PTR) SystemCallHandler233, NULL, FALSE },
	{ "ZwSetEventBoostPriority", 21, -1, (PULONG_PTR) SystemCallHandler234, NULL, FALSE },
	{ "ZwSetHighEventPair", 16, -1, (PULONG_PTR) SystemCallHandler235, NULL, FALSE },
	{ "ZwSetHighWaitLowEventPair", 23, -1, (PULONG_PTR) SystemCallHandler236, NULL, FALSE },
	{ "ZwSetInformationDebugObject", 25, -1, (PULONG_PTR) SystemCallHandler237, NULL, FALSE },

#if HOOK_FILE
	{ "ZwSetInformationFile", 18, -1, (PULONG_PTR) HookedNtSetInformationFile, NULL, TRUE },
#else
	{ "ZwSetInformationFile", 18, -1, (PULONG_PTR) SystemCallHandler238, NULL, FALSE },
#endif

	{ "ZwSetInformationJobObject", 23, -1, (PULONG_PTR) SystemCallHandler239, NULL, FALSE },
	{ "ZwSetInformationKey", 17, -1, (PULONG_PTR) SystemCallHandler240, NULL, FALSE },
	{ "ZwSetInformationObject", 20, -1, (PULONG_PTR) SystemCallHandler241, NULL, FALSE },
	{ "ZwSetInformationProcess", 21, -1, (PULONG_PTR) SystemCallHandler242, NULL, FALSE },
	{ "ZwSetInformationThread", 20, -1, (PULONG_PTR) SystemCallHandler243, NULL, FALSE },


#if HOOK_TOKEN
	{ "ZwSetInformationToken", 19, -1, (PULONG_PTR) HookedNtSetInformationToken, NULL, TRUE },
#else
	{ "ZwSetInformationToken", 19, -1, (PULONG_PTR) SystemCallHandler244, NULL, FALSE },
#endif


	{ "ZwSetIntervalProfile", 18, -1, (PULONG_PTR) SystemCallHandler245, NULL, FALSE },
	{ "ZwSetIoCompletion", 15, -1, (PULONG_PTR) SystemCallHandler246, NULL, FALSE },


#if HOOK_VDM
	{ "ZwSetLdtEntries", 13, -1, (PULONG_PTR) HookedNtSetLdtEntries, NULL, TRUE },
#else
	{ "ZwSetLdtEntries", 13, -1, (PULONG_PTR) SystemCallHandler247, NULL, FALSE },
#endif


	{ "ZwSetLowEventPair", 15, -1, (PULONG_PTR) SystemCallHandler248, NULL, FALSE },
	{ "ZwSetLowWaitHighEventPair", 23, -1, (PULONG_PTR) SystemCallHandler249, NULL, FALSE },
	{ "ZwSetQuotaInformationFile", 23, -1, (PULONG_PTR) SystemCallHandler250, NULL, FALSE },
	{ "ZwSetSecurityObject", 17, -1, (PULONG_PTR) SystemCallHandler251, NULL, FALSE },
	{ "ZwSetSystemEnvironmentValue", 25, -1, (PULONG_PTR) SystemCallHandler252, NULL, FALSE },	// XXX intel hal supports only 1 variable LastKnownGood
	{ "ZwSetSystemEnvironmentValueEx", 27, -1, (PULONG_PTR) SystemCallHandler253, NULL, FALSE },


#if HOOK_SYSINFO
	{ "ZwSetSystemInformation", 20, -1, (PULONG_PTR) HookedNtSetSystemInformation, NULL, TRUE },
#else
	{ "ZwSetSystemInformation", 20, -1, (PULONG_PTR) SystemCallHandler254, NULL, FALSE },
#endif


	{ "ZwSetSystemPowerState", 19, -1, (PULONG_PTR) SystemCallHandler255, NULL, FALSE },


#if HOOK_TIME
	{ "ZwSetSystemTime", 13, -1, (PULONG_PTR) HookedNtSetSystemTime, NULL, TRUE },
#else
	{ "ZwSetSystemTime", 13, -1, (PULONG_PTR) SystemCallHandler256, NULL, FALSE },
#endif


	{ "ZwSetThreadExecutionState", 23, -1, (PULONG_PTR) SystemCallHandler257, NULL, FALSE },
	{ "ZwSetTimer", 8, -1, (PULONG_PTR) SystemCallHandler258, NULL, FALSE },


#if 0 //HOOK_TIME
	{ "ZwSetTimerResolution", 18, -1, (PULONG_PTR) HookedNtSetTimerResolution, NULL, TRUE },
#else
	{ "ZwSetTimerResolution", 18, -1, (PULONG_PTR) SystemCallHandler259, NULL, FALSE },
#endif


	{ "ZwSetUuidSeed", 11, -1, (PULONG_PTR) SystemCallHandler260, NULL, FALSE },


#if HOOK_REGISTRY_ZZZ
	{ "ZwSetValueKey", 11, -1, (PULONG_PTR) HookedNtSetValueKey, NULL, TRUE },
#else
	{ "ZwSetValueKey", 11, -1, (PULONG_PTR) SystemCallHandler261, NULL, FALSE },
#endif


	{ "ZwSetVolumeInformationFile", 24, -1, (PULONG_PTR) SystemCallHandler262, NULL, FALSE },

//XXX
	{ "ZwShutdownSystem", 14, -1, (PULONG_PTR) SystemCallHandler263, NULL, FALSE },

	{ "ZwSignalAndWaitForSingleObject", 28, -1, (PULONG_PTR) SystemCallHandler264, NULL, FALSE },
	{ "ZwStartProfile", 12, -1, (PULONG_PTR) SystemCallHandler265, NULL, FALSE },
	{ "ZwStopProfile", 11, -1, (PULONG_PTR) SystemCallHandler266, NULL, FALSE },
	{ "ZwSuspendProcess", 14, -1, (PULONG_PTR) SystemCallHandler267, NULL, FALSE },
	{ "ZwSuspendThread", 13, -1, (PULONG_PTR) SystemCallHandler268, NULL, FALSE },
	{ "ZwSystemDebugControl", 18, -1, (PULONG_PTR) SystemCallHandler269, NULL, FALSE },

//XXX
	{ "ZwTerminateJobObject", 18, -1, (PULONG_PTR) SystemCallHandler270, NULL, FALSE },
	{ "ZwTerminateProcess", 16, -1, (PULONG_PTR) SystemCallHandler271, NULL, FALSE },
	{ "ZwTerminateThread", 15, -1, (PULONG_PTR) SystemCallHandler272, NULL, FALSE },

	{ "ZwTestAlert", 9, -1, (PULONG_PTR) SystemCallHandler273, NULL, FALSE },
	{ "ZwTraceEvent", 10, -1, (PULONG_PTR) SystemCallHandler274, NULL, FALSE },
	{ "ZwTranslateFilePath", 17, -1, (PULONG_PTR) SystemCallHandler275, NULL, FALSE },


#if HOOK_DRIVEROBJ_ZZZ
	{ "ZwUnloadDriver", 12, -1, (PULONG_PTR) HookedNtUnloadDriver, NULL, TRUE },
#else
	{ "ZwUnloadDriver", 12, -1, (PULONG_PTR) SystemCallHandler276, NULL, FALSE },
#endif


	{ "ZwUnloadKey", 9, -1, (PULONG_PTR) SystemCallHandler277, NULL, FALSE },
	{ "ZwUnloadKey2", 10, -1, (PULONG_PTR) SystemCallHandler278, NULL, FALSE },
	{ "ZwUnloadKeyEx", 11, -1, (PULONG_PTR) SystemCallHandler279, NULL, FALSE },
	{ "ZwUnlockFile", 10, -1, (PULONG_PTR) SystemCallHandler280, NULL, FALSE },
	{ "ZwUnlockVirtualMemory", 19, -1, (PULONG_PTR) SystemCallHandler281, NULL, FALSE },
	{ "ZwUnmapViewOfSection", 18, -1, (PULONG_PTR) SystemCallHandler282, NULL, FALSE },


#if HOOK_VDM
	{ "ZwVdmControl", 10, -1, (PULONG_PTR) HookedNtVdmControl, NULL, TRUE },
#else
	{ "ZwVdmControl", 10, -1, (PULONG_PTR) SystemCallHandler283, NULL, FALSE },
#endif


	{ "ZwWaitForDebugEvent", 17, -1, (PULONG_PTR) SystemCallHandler284, NULL, FALSE },
	{ "ZwWaitForKeyedEvent", 17, -1, (PULONG_PTR) SystemCallHandler285, NULL, FALSE },
	{ "ZwWaitForMultipleObjects", 22, -1, (PULONG_PTR) SystemCallHandler286, NULL, FALSE },
	{ "ZwWaitForSingleObject", 19, -1, (PULONG_PTR) SystemCallHandler287, NULL, FALSE },
	{ "ZwWaitHighEventPair", 17, -1, (PULONG_PTR) SystemCallHandler288, NULL, FALSE },
	{ "ZwWaitLowEventPair", 16, -1, (PULONG_PTR) SystemCallHandler289, NULL, FALSE },

	/* don't mediate for performance reasons, requires a valid handle anyway  */
	{ "ZwWriteFile", 9, -1, NULL, NULL, FALSE },
	{ "ZwWriteFileGather", 15, -1, NULL, NULL, FALSE },
	{ "ZwWriteRequestData", 16, -1, NULL, NULL, FALSE },
	{ "ZwWriteVirtualMemory", 18, -1, NULL, NULL, FALSE },
	{ "ZwYieldExecution", 14, -1, NULL, NULL, FALSE },
};



ACTION_TYPE
VerifySystemServiceCall(USHORT SystemServiceNumber)
{
	ACTION_TYPE			Action = ACTION_NONE;
	PIMAGE_PID_ENTRY	p;
	PPOLICY_RULE		PolicyRule;


//	LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("system call %x %x %s. irql %d", ZwCalls[SystemServiceNumber].OriginalFunction, ZwCalls[SystemServiceNumber].ServiceIDNumber, ZwCalls[SystemServiceNumber].ZwName, KeGetCurrentIrql()));


	if (KeGetCurrentIrql() != PASSIVE_LEVEL)
		return ACTION_NONE;


	if (LearningMode)
	{
		AddRule(RULE_SYSCALL, ZwCalls[SystemServiceNumber].ZwName, 0);
		return ACTION_NONE;
	}

	
	/* verify policy and user return address? */

	p = FindImagePidEntry(CURRENT_PROCESS_PID, 0);

	if (p == NULL)
	{
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_VERBOSE, ("VerifySystemServiceCall: FindImagePidEntry(%d) failed\n", CURRENT_PROCESS_PID));
		return ACTION_NONE;
	}
//XXX p->SecPolicy spinlock is not acquired
	PolicyRule = p->SecPolicy.RuleList[ RULE_SYSCALL ];
	if (PolicyRule)
	{
		/* Calculate the bit array ULONG we need to read (bit array consists of a bunch of ULONGs) */
		ULONG	UlongCount = SystemServiceNumber >> UlongBitShift;

		/* Choose the correct bit array ULONG */
		PULONG	BitArray = &PolicyRule->ServiceBitArray[0] + UlongCount;

		ULONG	BitOffset = SystemServiceNumber - (UlongCount << UlongBitShift);

		/* read the bit */
		Action = *BitArray & ( 1 << BitOffset ) ? ACTION_PERMIT : ACTION_DENY;

		if (Action == ACTION_DENY)
		{
			LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("denying system call %x %x %s\n", ZwCalls[SystemServiceNumber].OriginalFunction, ZwCalls[SystemServiceNumber].ServiceIDNumber, ZwCalls[SystemServiceNumber].ZwName));
			Action = ACTION_PERMIT;
		}
	}

	return Action;
}


//XXX can we get rid of pushfd / popfd ?
//XXX move to i386.c?
#define	SYSCALL_HANDLER(num)							\
	__declspec(naked) void SystemCallHandler##num()		\
	{													\
		_asm pushad 									\
		_asm { pushfd }									\
		if (0 && VerifySystemServiceCall(num) == ACTION_DENY)\
		{												\
			_asm popfd									\
			_asm popad									\
			_asm mov eax, 0xC0000022 /*STATUS_ACCESS_DENIED*/				\
			_asm ret									\
		}												\
		_asm popfd										\
		_asm popad										\
		_asm jmp dword ptr [ ZwCalls + num*20 + 12 ]	\
/*		_asm jmp ZwCalls[num].OriginalFunction			*/\
	}

/*
//XXX can find out dynamically the amount of stack space to clean up by looking at Zw* ret instruction
			if (PolicyCheck(RULE_SYSCALL, NULL, 0) == ACTION_DENY)				\
			{											\
				_asm mov eax, 0xC0000022				\
				_asm ret								\
			}											\
*/


SYSCALL_HANDLER(0) SYSCALL_HANDLER(1) SYSCALL_HANDLER(2) SYSCALL_HANDLER(3) SYSCALL_HANDLER(4)
SYSCALL_HANDLER(5) SYSCALL_HANDLER(6) SYSCALL_HANDLER(7) SYSCALL_HANDLER(8) SYSCALL_HANDLER(9)
SYSCALL_HANDLER(10) SYSCALL_HANDLER(11) SYSCALL_HANDLER(12) SYSCALL_HANDLER(13) SYSCALL_HANDLER(14)
SYSCALL_HANDLER(15) SYSCALL_HANDLER(16) SYSCALL_HANDLER(17) SYSCALL_HANDLER(18) SYSCALL_HANDLER(19)
SYSCALL_HANDLER(20) SYSCALL_HANDLER(21) SYSCALL_HANDLER(22) SYSCALL_HANDLER(23) SYSCALL_HANDLER(24)
SYSCALL_HANDLER(25) SYSCALL_HANDLER(26) SYSCALL_HANDLER(27) SYSCALL_HANDLER(28) SYSCALL_HANDLER(29)
SYSCALL_HANDLER(30) SYSCALL_HANDLER(31) SYSCALL_HANDLER(32) SYSCALL_HANDLER(33) SYSCALL_HANDLER(34)
SYSCALL_HANDLER(35) SYSCALL_HANDLER(36) SYSCALL_HANDLER(37) SYSCALL_HANDLER(38) SYSCALL_HANDLER(39)
SYSCALL_HANDLER(40) SYSCALL_HANDLER(41) SYSCALL_HANDLER(42) SYSCALL_HANDLER(43) SYSCALL_HANDLER(44)
SYSCALL_HANDLER(45) SYSCALL_HANDLER(46) SYSCALL_HANDLER(47) SYSCALL_HANDLER(48) SYSCALL_HANDLER(49)
SYSCALL_HANDLER(50) SYSCALL_HANDLER(51) SYSCALL_HANDLER(52) SYSCALL_HANDLER(53) SYSCALL_HANDLER(54)
SYSCALL_HANDLER(55) SYSCALL_HANDLER(56) SYSCALL_HANDLER(57) SYSCALL_HANDLER(58) SYSCALL_HANDLER(59)
SYSCALL_HANDLER(60) SYSCALL_HANDLER(61) SYSCALL_HANDLER(62) SYSCALL_HANDLER(63) SYSCALL_HANDLER(64)
SYSCALL_HANDLER(65) SYSCALL_HANDLER(66) SYSCALL_HANDLER(67) SYSCALL_HANDLER(68) SYSCALL_HANDLER(69)
SYSCALL_HANDLER(70) SYSCALL_HANDLER(71) SYSCALL_HANDLER(72) SYSCALL_HANDLER(73) SYSCALL_HANDLER(74)
SYSCALL_HANDLER(75) SYSCALL_HANDLER(76) SYSCALL_HANDLER(77) SYSCALL_HANDLER(78) SYSCALL_HANDLER(79)
SYSCALL_HANDLER(80) SYSCALL_HANDLER(81) SYSCALL_HANDLER(82) SYSCALL_HANDLER(83) SYSCALL_HANDLER(84)
SYSCALL_HANDLER(85) SYSCALL_HANDLER(86) SYSCALL_HANDLER(87) SYSCALL_HANDLER(88) SYSCALL_HANDLER(89)
SYSCALL_HANDLER(90) SYSCALL_HANDLER(91) SYSCALL_HANDLER(92) SYSCALL_HANDLER(93) SYSCALL_HANDLER(94)
SYSCALL_HANDLER(95) SYSCALL_HANDLER(96) SYSCALL_HANDLER(97) SYSCALL_HANDLER(98) SYSCALL_HANDLER(99)
SYSCALL_HANDLER(100) SYSCALL_HANDLER(101) SYSCALL_HANDLER(102) SYSCALL_HANDLER(103) SYSCALL_HANDLER(104)
SYSCALL_HANDLER(105) SYSCALL_HANDLER(106) SYSCALL_HANDLER(107) SYSCALL_HANDLER(108) SYSCALL_HANDLER(109)
SYSCALL_HANDLER(110) SYSCALL_HANDLER(111) SYSCALL_HANDLER(112) SYSCALL_HANDLER(113) SYSCALL_HANDLER(114)
SYSCALL_HANDLER(115) SYSCALL_HANDLER(116) SYSCALL_HANDLER(117) SYSCALL_HANDLER(118) SYSCALL_HANDLER(119)
SYSCALL_HANDLER(120) SYSCALL_HANDLER(121) SYSCALL_HANDLER(122) SYSCALL_HANDLER(123) SYSCALL_HANDLER(124)
SYSCALL_HANDLER(125) SYSCALL_HANDLER(126) SYSCALL_HANDLER(127) SYSCALL_HANDLER(128) SYSCALL_HANDLER(129)
SYSCALL_HANDLER(130) SYSCALL_HANDLER(131) SYSCALL_HANDLER(132) SYSCALL_HANDLER(133) SYSCALL_HANDLER(134)
SYSCALL_HANDLER(135) SYSCALL_HANDLER(136) SYSCALL_HANDLER(137) SYSCALL_HANDLER(138) SYSCALL_HANDLER(139)
SYSCALL_HANDLER(140) SYSCALL_HANDLER(141) SYSCALL_HANDLER(142) SYSCALL_HANDLER(143) SYSCALL_HANDLER(144)
SYSCALL_HANDLER(145) SYSCALL_HANDLER(146) SYSCALL_HANDLER(147) SYSCALL_HANDLER(148) SYSCALL_HANDLER(149)
SYSCALL_HANDLER(150) SYSCALL_HANDLER(151) SYSCALL_HANDLER(152) SYSCALL_HANDLER(153) SYSCALL_HANDLER(154)
SYSCALL_HANDLER(155) SYSCALL_HANDLER(156) SYSCALL_HANDLER(157) SYSCALL_HANDLER(158) SYSCALL_HANDLER(159)
SYSCALL_HANDLER(160) SYSCALL_HANDLER(161) SYSCALL_HANDLER(162) SYSCALL_HANDLER(163) SYSCALL_HANDLER(164)
SYSCALL_HANDLER(165) SYSCALL_HANDLER(166) SYSCALL_HANDLER(167) SYSCALL_HANDLER(168) SYSCALL_HANDLER(169)
SYSCALL_HANDLER(170) SYSCALL_HANDLER(171) SYSCALL_HANDLER(172) SYSCALL_HANDLER(173) SYSCALL_HANDLER(174)
SYSCALL_HANDLER(175) SYSCALL_HANDLER(176) SYSCALL_HANDLER(177) SYSCALL_HANDLER(178) SYSCALL_HANDLER(179)
SYSCALL_HANDLER(180) SYSCALL_HANDLER(181) SYSCALL_HANDLER(182) SYSCALL_HANDLER(183) SYSCALL_HANDLER(184)
SYSCALL_HANDLER(185) SYSCALL_HANDLER(186) SYSCALL_HANDLER(187) SYSCALL_HANDLER(188) SYSCALL_HANDLER(189)
SYSCALL_HANDLER(190) SYSCALL_HANDLER(191) SYSCALL_HANDLER(192) SYSCALL_HANDLER(193) SYSCALL_HANDLER(194)
SYSCALL_HANDLER(195) SYSCALL_HANDLER(196) SYSCALL_HANDLER(197) SYSCALL_HANDLER(198) SYSCALL_HANDLER(199)
SYSCALL_HANDLER(200) SYSCALL_HANDLER(201) SYSCALL_HANDLER(202) SYSCALL_HANDLER(203) SYSCALL_HANDLER(204)
SYSCALL_HANDLER(205) SYSCALL_HANDLER(206) SYSCALL_HANDLER(207) SYSCALL_HANDLER(208) SYSCALL_HANDLER(209)
SYSCALL_HANDLER(210) SYSCALL_HANDLER(211) SYSCALL_HANDLER(212) SYSCALL_HANDLER(213) SYSCALL_HANDLER(214)
SYSCALL_HANDLER(215) SYSCALL_HANDLER(216) SYSCALL_HANDLER(217) SYSCALL_HANDLER(218) SYSCALL_HANDLER(219)
SYSCALL_HANDLER(220) SYSCALL_HANDLER(221) SYSCALL_HANDLER(222) SYSCALL_HANDLER(223) SYSCALL_HANDLER(224)
SYSCALL_HANDLER(225) SYSCALL_HANDLER(226) SYSCALL_HANDLER(227) SYSCALL_HANDLER(228) SYSCALL_HANDLER(229)
SYSCALL_HANDLER(230) SYSCALL_HANDLER(231) SYSCALL_HANDLER(232) SYSCALL_HANDLER(233) SYSCALL_HANDLER(234)
SYSCALL_HANDLER(235) SYSCALL_HANDLER(236) SYSCALL_HANDLER(237) SYSCALL_HANDLER(238) SYSCALL_HANDLER(239)
SYSCALL_HANDLER(240) SYSCALL_HANDLER(241) SYSCALL_HANDLER(242) SYSCALL_HANDLER(243) SYSCALL_HANDLER(244)
SYSCALL_HANDLER(245) SYSCALL_HANDLER(246) SYSCALL_HANDLER(247) SYSCALL_HANDLER(248) SYSCALL_HANDLER(249)
SYSCALL_HANDLER(250) SYSCALL_HANDLER(251) SYSCALL_HANDLER(252) SYSCALL_HANDLER(253) SYSCALL_HANDLER(254)
SYSCALL_HANDLER(255) SYSCALL_HANDLER(256) SYSCALL_HANDLER(257) SYSCALL_HANDLER(258) SYSCALL_HANDLER(259)
SYSCALL_HANDLER(260) SYSCALL_HANDLER(261) SYSCALL_HANDLER(262) SYSCALL_HANDLER(263) SYSCALL_HANDLER(264)
SYSCALL_HANDLER(265) SYSCALL_HANDLER(266) SYSCALL_HANDLER(267) SYSCALL_HANDLER(268) SYSCALL_HANDLER(269)
SYSCALL_HANDLER(270) SYSCALL_HANDLER(271) SYSCALL_HANDLER(272) SYSCALL_HANDLER(273) SYSCALL_HANDLER(274)
SYSCALL_HANDLER(275) SYSCALL_HANDLER(276) SYSCALL_HANDLER(277) SYSCALL_HANDLER(278) SYSCALL_HANDLER(279)
SYSCALL_HANDLER(280) SYSCALL_HANDLER(281) SYSCALL_HANDLER(282) SYSCALL_HANDLER(283) SYSCALL_HANDLER(284)
SYSCALL_HANDLER(285) SYSCALL_HANDLER(286) SYSCALL_HANDLER(287) SYSCALL_HANDLER(288) SYSCALL_HANDLER(289)
SYSCALL_HANDLER(290) SYSCALL_HANDLER(291) SYSCALL_HANDLER(292) SYSCALL_HANDLER(293) SYSCALL_HANDLER(294)



/*
 * InitSyscallsHooks()
 *
 * Description:
 *		Find the correct global syscall table indeces for all system calls
 *		(initialize "OriginalFunction" pointers). Must be called at PASSIVE_LEVEL IRQL
 *		in order to access pageable memory.
 *
 *		NOTE: Called once during driver initialization (DriverEntry()).
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

BOOLEAN
InitSyscallsHooks()
{
	int		i;


	ZwCallsNumber = sizeof(ZwCalls) / sizeof(ZwCalls[0]);

	for (i = 0; i < ZwCallsNumber; i++)
	{
		//XXX this can be optimized to return index directly?! this way when we restore syscalls back we don't have to do the same process again??!?!
		ZwCalls[i].ServiceIDNumber = (USHORT) FindZwFunctionIndex(ZwCalls[i].ZwName);
	}


	return TRUE;
}



/*
 * InstallSyscallsHooks()
 *
 * Description:
 *		Mediate various system services. Called at DISPATCH_LEVEL IRQL.
 *
 *		NOTE: Called once during driver initialization (DriverEntry()).
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

BOOLEAN
InstallSyscallsHooks()
{
	int		i;


	ZwCallsNumber = sizeof(ZwCalls) / sizeof(ZwCalls[0]);

	for (i = 0; i < ZwCallsNumber; i++)
	{
		if (ZwCalls[i].HookFunction != NULL &&
			(HOOK_SYSCALLS || LearningMode || ZwCalls[i].Hijacked == TRUE))
		{
			if (ZwCalls[i].ServiceIDNumber != (USHORT) -1)
			{
				ZwCalls[i].OriginalFunction = HookSystemServiceByIndex(ZwCalls[i].ServiceIDNumber,
																	   ZwCalls[i].HookFunction);
				ZwCalls[i].Hijacked = TRUE;
			}
		}
	}


	return TRUE;
}



/*
 * RemoveSyscallsHooks()
 *
 * Description:
 *		Restores all the original system service function pointers.
 *
 *		NOTE: Called once during driver unload (DriverUnload()).
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		Nothing.
 */

void
RemoveSyscallsHooks()
{
	int		i;


	for (i = 0; i < ZwCallsNumber; i++)
	{
		// restore hijacked system calls
		if (ZwCalls[i].Hijacked == TRUE && ZwCalls[i].OriginalFunction != NULL && ZwCalls[i].ServiceIDNumber != -1)
		{
//			LOG(LOG_SS_HOOKPROC, LOG_PRIORITY_DEBUG, ("Restoring syscall %d %x\n", i, i));

			HookSystemServiceByIndex(ZwCalls[i].ServiceIDNumber, ZwCalls[i].OriginalFunction);
		}
	}
}
