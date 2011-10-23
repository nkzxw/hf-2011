//********************************************************************
//	created:	14:8:2008   14:38
//	file:		peloader.h
//	author:		tiamo
//	purpose:	pe file loader
//********************************************************************

#pragma once

#include <ntimage.h>

//
// this entry has been processed
//
#define LDRP_ENTRY_PROCESSED                    0x00004000

//
// this is a dll used by other drivers
//
#define LDRP_DRIVER_DEPENDENT_DLL               0x04000000

//
// loader data table entry
//
typedef struct _LDR_DATA_TABLE_ENTRY
{
	//
	// in load order link list entry,offset = 0
	//
	LIST_ENTRY											InLoadOrderLinks;

	//
	// in memory order link list entry,offset = 8
	//
	LIST_ENTRY											InMemoryOrderLinks;

	//
	// in initialization order list entry,offset = 10
	//
	LIST_ENTRY											InInitializationOrderLinks;

	//
	// image base,offset = 18
	//
	PVOID												DllBase;

	//
	// entry point,offset = 1c
	//
	PVOID												EntryPoint;

	//
	// size of image,offset = 20
	//
	ULONG												SizeOfImage;

	//
	// full dll name,offset = 24
	//
	UNICODE_STRING										FullDllName;

	//
	// base dll name,offset = 2c
	//
	UNICODE_STRING										BaseDllName;

	//
	// flags,offset = 34
	//
	ULONG												Flags;

	//
	// reference count,offset = 38
	//
	USHORT												LoadCount;

	//
	// thread local storage index,offset = 3a
	//
	USHORT												TlsIndex;

	union
	{
		//
		// hash links,offset = 3c
		//
		LIST_ENTRY										HashLinks;

		//
		// section and checksum
		//
		struct
		{
			//
			// section pointer,offset = 3c
			//
			PVOID										SectionPointer;

			//
			// checksum,offset = 40
			//
			ULONG										CheckSum;
		};
	};

	union
	{
		//
		// time stamp,offset = 44
		//
		ULONG											TimeDateStamp;

		//
		// loaded imports,offset = 44
		//
		PVOID											LoadedImports;
	};

	//
	// entry point activation context,offset = 48
	//
	PVOID												EntryPointActivationContext;
}LDR_DATA_TABLE_ENTRY,*PLDR_DATA_TABLE_ENTRY;

//
// search path set
//
typedef struct _LDR_SCAN_IMPORT_SERACH_PATH_SET
{
	//
	// search path count
	//
	ULONG												SearchPathCount;

	//
	// system root
	//
	PCHAR												SystemRootPath;

	//
	// prefix path
	//
	CHAR												PrefixPath[0x100];

	//
	// search path
	//
	struct
	{
		//
		// device id
		//
		ULONG											DeviceId;

		//
		// device path
		//
		PCHAR											DevicePath;

		//
		// search path
		//
		PCHAR											Path;
	}SearchPath[1];
}LDR_SCAN_IMPORT_SERACH_PATH_SET,*PLDR_SCAN_IMPORT_SERACH_PATH_SET;

//
// get image nt header
//
PIMAGE_NT_HEADERS RtlImageNtHeader(__in PVOID ImageBase);

//
// load image
//
ARC_STATUS BlLoadImageEx(__in ULONG DeviceId,__in TYPE_OF_MEMORY MemoryType,__in PCHAR Path,__in USHORT ImageMachineType,
						 __in_opt ULONG ForceLoadBasePage,__in_opt ULONG Alignment,__out PVOID* LoadedImageBase);

//
// allocate a data table entry
//
ARC_STATUS BlAllocateDataTableEntry(__in PCHAR BaseDllName,__in PCHAR FullDllName,__in PVOID Base,__out PLDR_DATA_TABLE_ENTRY *AllocatedEntry);

//
// scan import table
//
ARC_STATUS BlScanImportDescriptorTable(__in PLDR_SCAN_IMPORT_SERACH_PATH_SET SearchPathSet,__in PLDR_DATA_TABLE_ENTRY ScanEntry,__in TYPE_OF_MEMORY MemoryType);

//
// reference data entry by name
//
BOOLEAN BlCheckForLoadedDll (__in PCHAR DllName,__out PLDR_DATA_TABLE_ENTRY *FoundEntry);