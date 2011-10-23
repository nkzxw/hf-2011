//********************************************************************
//	created:	16:8:2008   14:26
//	file:		sumodule.h
//	author:		tiamo
//	purpose:	startup module defined structures
//********************************************************************

#pragma once

//
// fs context record
//
typedef struct _FSCONTEXT_RECORD
{
	//
	// boot drive,0x8? = hdd,0x41=ramdisk,0x40=netboot
	//
	ULONG												BootDrive	: 8;

	//
	// partition
	//
	ULONG												Partition	: 8;

	//
	// reserved
	//
	ULONG												Reserved	: 16;

}FSCONTEXT_RECORD,*PFSCONTEXT_RECORD;

//
// bios 820 frame
//
#include <pshpack4.h>
typedef struct _E820FRAME
{
	//
	// error flag
	//
	ULONG												ErrorFlag;

	//
	// key
	//
	ULONG												Key;

	//
	// size
	//
	ULONG												Size;

	struct _E820FRAME_DESCRIPTOR
	{
		//
		// base address
		//
		ULARGE_INTEGER									BaseAddress;

		//
		// size
		//
		ULARGE_INTEGER									Size;

		//
		// memory type
		//
		ULONG											MemoryType;
	}Descriptor;
}E820FRAME,*PE820FRAME;
#include <poppack.h>

//
// reboot process
//
typedef VOID (__cdecl* SU_REBOOT_PROCESSOR)();

//
// disk io system
//
typedef NTSTATUS (__cdecl* SU_DISK_IO_SYSTEM)(__in ULONG Fn,__in ULONG DriveId,__in ULONG Track,__in ULONG Head,__in ULONG Sector,__in ULONG Count,__in PVOID Buffer);

//
// read key
//
typedef ULONG (__cdecl* SU_GET_KEY)();

//
// get counter
//
typedef ULONG (__cdecl* SU_GET_COUNTER)();

//
// reboot
//	0 = fat,1 = HPFS,2 = NTFS
//
typedef VOID (__cdecl* SU_REBOOT)(__in ULONG RebootType);

//
// detect hardware
//
typedef VOID (__cdecl* SU_DETECT_HARDWARE)(__in ULONG Heap,__in ULONG HeapSize,__out PVOID FwCfgTree,__out PULONG HeapUsed,__in PCHAR Options,__in ULONG OptionLength);

//
// hardware cursor
//
typedef VOID (__cdecl* SU_HARDWARE_CURSOR)(__in ULONG PositionY,__in ULONG PositionX);

//
// get date time
//
typedef VOID (__cdecl* SU_GET_DATE_TIME)(__out PULONG Date,__out PULONG Time);

//
// com port
//
typedef VOID (__cdecl *SU_COM_PORT)(__in LONG  Port,__in ULONG Function,__in UCHAR Arg);

//
// get stall count
//
typedef ULONG (__cdecl* SU_GET_STALL_COUNT)();

//
// initialize display
//
typedef VOID (__cdecl* SU_INITIALIZE_DISPLAY_FOR_NT)();

//
// get memory discriptor
//
typedef VOID (__cdecl* SU_GET_MEMORY_DESCRIPTOR)(__inout PE820FRAME Frame);

//
// access disk using LBA
//
typedef NTSTATUS (__cdecl* SU_EXTENDED_DISK_IO_SYSTEM)(__in ULONG DriveId,__in ULONGLONG LBA,__in ULONG SectorCount,__in PVOID Buffer,__in ULONG Fn);

//
// get eltoritor status
//
typedef NTSTATUS (__cdecl* SU_GET_ELTORITO_STATUS)(__in PVOID Buffer,__in ULONG DriveNumber);

//
// check int13 extension status
//
typedef UCHAR (__cdecl* SU_CHECK_INT13_EXTENSION_SUPPORTED)(__in ULONG DriveId,__out PVOID DiskParameters);

//
// pxe service
//
typedef NTSTATUS (__cdecl* SU_PXE_SERVICE)(__in ULONG ServiceCode,__in PVOID ServiceParam);

//
// init APM bios
//
typedef VOID (__cdecl* SU_INITIALIZE_APM_BIOS)();

//
// external service table
//
typedef struct _EXTERNAL_SERVICES_TABLE
{
	//
	// reboot processor
	//
	SU_REBOOT_PROCESSOR									RebootProcessor;

	//
	// disk io system
	//
	SU_DISK_IO_SYSTEM									DiskIOSystem;

	//
	// read key
	//
	SU_GET_KEY											GetKey;

	//
	// get counter
	//
	SU_GET_COUNTER										GetCounter;

	//
	// reboot
	//
	SU_REBOOT											Reboot;

	//
	// detect hardware
	//
	SU_DETECT_HARDWARE									DetectHardware;

	//
	// hardware cursor
	//
	SU_HARDWARE_CURSOR									HardwareCursor;

	//
	// get date time
	//
	SU_GET_DATE_TIME									GetDateTime;

	//
	// com port
	//
	SU_COM_PORT											ComPort;

	//
	// get stall count
	//
	SU_GET_STALL_COUNT									GetStallCount;

	//
	// initialize display for net
	//
	SU_INITIALIZE_DISPLAY_FOR_NT						InitializeDisplayForNt;

	//
	// get memory descriptor
	//
	SU_GET_MEMORY_DESCRIPTOR							GetMemoryDescriptor;

	//
	// LBA access disk
	//
	SU_EXTENDED_DISK_IO_SYSTEM							ExtendedDiskIOSystem;

	//
	// get eltorito status
	//
	SU_GET_ELTORITO_STATUS								GetElToritoStatus;

	//
	// check int13 extension
	//
	SU_CHECK_INT13_EXTENSION_SUPPORTED					CheckInt13ExtensionSupported;

	//
	// pxe service
	//
	SU_PXE_SERVICE										PxeService;

	//
	// init apm bios
	//
	SU_INITIALIZE_APM_BIOS								InitializeAPMBios;
}EXTERNAL_SERVICES_TABLE,*PEXTERNAL_SERVICES_TABLE;

//
// memory descriptor
//
typedef struct _SU_MEMORY_DESCRIPTOR
{
	//
	// base page
	//
	ULONG												BlockBase;

	//
	// block size
	//
	ULONG												BlockSize;
}SU_MEMORY_DESCRIPTOR,*PSU_MEMORY_DESCRIPTOR;

//
// boot context,passed to the OS loader by the SU module
//
typedef struct _BOOT_CONTEXT
{
	//
	// fs context record,containing boot drive info
	//
	PFSCONTEXT_RECORD									FSContextPointer;

	//
	// service table provided by SU module
	//
	PEXTERNAL_SERVICES_TABLE							ExternalServicesTable;

	//
	// memory descriptor list
	//
	PSU_MEMORY_DESCRIPTOR								MemoryDescriptorList;

	//
	// machine type
	//
	ULONG												MachineType;

	//
	// osloader start address
	//
	ULONG												OsLoaderStart;

	//
	// osloader end address
	//
	ULONG												OsLoaderEnd;

	//
	// osloader's resource directory
	//
	ULONG												ResourceDirectory;

	//
	// osloader's resource offset
	//
	ULONG												ResourceOffset;

	//
	// osloader's base address
	//
	ULONG												OsLoaderBase;

	//
	// osloader's export table
	//
	ULONG												OsLoaderExports;

	//
	// boot flags
	//
	ULONG												BootFlags;

	//
	// ntdetect start
	//
	ULONG												NtDetectStart;

	//
	// ntdetect end
	//
	ULONG												NtDetectEnd;

	//
	// ramdisk address
	//
	ULONG												SdiAddress;
}BOOT_CONTEXT,*PBOOT_CONTEXT;

//
// external service table
//
extern PEXTERNAL_SERVICES_TABLE							ExternalServicesTable;

//
// machine type
//
extern ULONG											MachineType;

//
// osloader start address
//
extern ULONG											OsLoaderStart;

//
// osloader end address
//
extern ULONG											OsLoaderEnd;

//
// osloader's resource directory
//
extern ULONG											BlpResourceDirectory;

//
// osloader's resource offset
//
extern ULONG											BlpResourceFileOffset;

//
// osloader's base address
//
extern ULONG											OsLoaderBase;

//
// osloader's export table
//
extern ULONG											OsLoaderExports;

//
// boot flags
//
extern ULONG											BootFlags;

//
// ntdetect start
//
extern ULONG											NtDetectStart;

//
// ntdetect end
//
extern ULONG											NtDetectEnd;

//
// ramdisk address
//
extern ULONG											SdiAddress;

//
// boot from net
//
extern BOOLEAN											BlBootingFromNet;

//
// boot from cd
//
extern BOOLEAN											ElToritoCDBoot;

//
// net boot path
//
extern CHAR												NetBootPath[0x100];

//
// build number
//
extern ULONG											NtBuildNumber;

//
// command line from su module
//
extern CHAR												BlSuCmdLine[0x101];