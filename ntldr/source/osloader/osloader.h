//********************************************************************
//	created:	16:8:2008   14:23
//	file:		osloader.h
//	author:		tiamo
//	purpose:	load kernel,hal,device driver and system registry
//********************************************************************

#pragma once

//
// profile status OK
//
#define HW_PROFILE_STATUS_SUCCESS						0x0000

//
// profile status failed
//
#define HW_PROFILE_STATUS_FAILURE						0xC001

//
// unsupported dock state
//
#define HW_PROFILE_DOCKSTATE_UNSUPPORTED				0

//
// undocked
//
#define HW_PROFILE_DOCKSTATE_UNDOCKED					1

//
// docked
//
#define HW_PROFILE_DOCKSTATE_DOCKED						2

//
// unknown
//
#define HW_PROFILE_DOCKSTATE_UNKNOWN					3

//
// nls data block
//
typedef struct _NLS_DATA_BLOCK
{
	//
	// ansi code page
	//
	PVOID												AnsiCodePageData;

	//
	// oem code page
	//
	PVOID												OemCodePageData;

	//
	// unicode case table
	//
	PVOID												UnicodeCaseTableData;
}NLS_DATA_BLOCK,*PNLS_DATA_BLOCK;

//
// x86 loader block
//
typedef struct _I386_LOADER_BLOCK
{
	//
	// common data,offset = 0,5c
	//
	PVOID												CommonDataArea;

	//
	// machine type,offset = 4,60
	//
	ULONG												MachineType;

	//
	// /3gb virtual bias,offset = 8,64
	//
	ULONG												VirtualBias;
}I386_LOADER_BLOCK,*PI386_LOADER_BLOCK;

//
// profile parameter block
//
typedef struct _PROFILE_PARAMETER_BLOCK
{
	//
	// status
	//
	USHORT												Status;

	//
	// padding
	//
	USHORT												Reserved;

	//
	// docking state
	//
	USHORT												DockingState;

	//
	// capabilities
	//
	USHORT												Capabilities;

	//
	// dock id
	//
	ULONG												DockID;

	//
	// serial number
	//
	ULONG												SerialNumber;

}PROFILE_PARAMETER_BLOCK,*PPROFILE_PARAMETER_BLOCK;

//
// extension,sizeof = 58
//
typedef struct _LOADER_PARAMETER_EXTENSION
{
	//
	// set to sizeof (struct _LOADER_PARAMETER_EXTENSION),offset = 0
	//
	ULONG												Size;

	//
	// profile,offset = 4
	//
	PROFILE_PARAMETER_BLOCK								Profile;

	//
	// major version,offset = 14
	//
	ULONG												MajorVersion;

	//
	// minor version,offset = 18
	//
	ULONG												MinorVersion;

	//
	// inf used to identify "broken" machines,offset = 1c
	//
	PVOID												InfFileImage;

	//
	// inf file size,offset = 20
	//
	ULONG												InfFileSize;

	//
	// pointer to the triage block, if present,offset = 24
	//
	PVOID												TriageDumpBlock;

	//
	// heighest page,offset = 28
	//
	ULONG												HighestPage;

	//
	// serial redirection info,offset = 2c
	//
	struct _LOADER_REDIRECTION_INFORMATION*				RedirectionInfo;

	//
	// offset 30
	//
	ULONG												Offset30;

	//
	// drvmain.sdb referenced as shim engine,,offset = 34
	//
	PVOID												DrvMainSdbFileImage;

	//
	// drvmain.sdb file size,offset = 38
	//
	ULONG												DrvMainSdbFileSize;

	//
	// network loading info,offset = 3c
	//
	PVOID												NetworkBootInfo;

	//
	// offset = 40
	//
	ULONG												Offset40;

	//
	// offset = 44
	//
	ULONG												Offset44;

	//
	// offset = 48
	//
	LIST_ENTRY											FirmwareTable64ListHead;

	//
	// acpitabl.dat,offset = 50
	//
	PVOID												AcpiTableDatFileImage;

	//
	// acpitabl.dat size,offset = 54
	//
	ULONG												AcpiTableDatFileSize;
}LOADER_PARAMETER_EXTENSION,*PLOADER_PARAMETER_EXTENSION;

//
// loader parameter block,sizeof = 68
//
typedef struct _LOADER_PARAMETER_BLOCK
{
	//
	// ordered modules list head,offset = 0
	//
	LIST_ENTRY											LoadOrderListHead;

	//
	// memory allocation descriptor list head,offset = 8
	//
	LIST_ENTRY											MemoryDescriptorListHead;

	//
	// boot driver list header,offset = 10
	//
	LIST_ENTRY											BootDriverListHead;

	//
	// keren stack,offset = 18
	//
	ULONG_PTR											KernelStack;

	//
	// process control block,offset = 1c
	//
	ULONG_PTR											Prcb;

	//
	// process,offset = 20
	//
	ULONG_PTR											Process;

	//
	// thread,offset = 24
	//
	ULONG_PTR											Thread;

	//
	// registry length,offset = 28
	//
	ULONG												RegistryLength;

	//
	// registry base,offset = 2c
	//
	PVOID												RegistryBase;

	//
	// configuration root,offset = 30
	//
	struct _CONFIGURATION_COMPONENT_DATA*				ConfigurationRoot;

	//
	// boot device name,offset = 34
	//
	PCHAR												ArcBootDeviceName;

	//
	// hal device name,offset = 38
	//
	PCHAR												ArcHalDeviceName;

	//
	// kernel path name,offset = 3c
	//
	PCHAR												NtBootPathName;

	//
	// hal path name,offset = 40
	//
	PCHAR												NtHalPathName;

	//
	// load options,offset = 44
	//
	PCHAR												LoadOptions;

	//
	// nls data,offset = 48
	//
	struct _NLS_DATA_BLOCK*								NlsData;

	//
	// disk info,offset = 4c
	//
	struct _ARC_DISK_INFORMATION*						ArcDiskInformation;

	//
	// font file,offset = 50
	//
	PVOID												OemFontFile;

	//
	// setu loader block,offset = 54
	//
	struct _SETUP_LOADER_BLOCK*							SetupLoaderBlock;

	//
	// extension,offset = 58
	//
	PLOADER_PARAMETER_EXTENSION							Extension;

	//
	// platform data,offset = 5c
	//
	union
	{
		//
		// x86 info
		//
		I386_LOADER_BLOCK								I386;
	}u;
}LOADER_PARAMETER_BLOCK,*PLOADER_PARAMETER_BLOCK;

//
// data cache fill size
//
extern ULONG											BlDcacheFillSize;

//
// loader parameter block
//
extern PLOADER_PARAMETER_BLOCK							BlLoaderBlock;

//
// load system files into memory and transfer control to kernel
//
ARC_STATUS BlOsLoader(__in ULONG Argc,__in PCHAR Argv[],__in PCHAR Envp[]);

//
// use PAE
//
extern BOOLEAN											BlUsePae;