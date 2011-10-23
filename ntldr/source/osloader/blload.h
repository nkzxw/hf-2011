//********************************************************************
//	created:	20:8:2008   3:03
//	file:		blload.h
//	author:		tiamo
//	purpose:	load system files
//********************************************************************

#pragma once

//
// boot driver list entry
//
typedef struct _BOOT_DRIVER_LIST_ENTRY
{
	//
	// link
	//
	LIST_ENTRY											Link;

	//
	// file path
	//
	UNICODE_STRING										FilePath;

	//
	// registry path
	//
	UNICODE_STRING										RegistryPath;

	//
	// loader entry
	//
	PLDR_DATA_TABLE_ENTRY								LdrEntry;
}BOOT_DRIVER_LIST_ENTRY,*PBOOT_DRIVER_LIST_ENTRY;

//
// boot driver node
//
typedef struct _BOOT_DRIVER_NODE
{
	//
	// boot driver list entry
	//
	BOOT_DRIVER_LIST_ENTRY								ListEntry;

	//
	// group name
	//
	UNICODE_STRING										Group;

	//
	// name
	//
	UNICODE_STRING										Name;

	//
	// tag
	//
	ULONG												Tag;

	//
	// error control
	//
	ULONG												ErrorControl;
}BOOT_DRIVER_NODE,*PBOOT_DRIVER_NODE;


//
// load and scan system hive
//
ARC_STATUS BlLoadAndScanSystemHive(__in ULONG DeviceId,__in PCHAR DeviceName,__in PCHAR DirectoryPath,__in PWCHAR BootFileSystem,
								   __inout PBOOLEAN UseLastKnownGood,__out PBOOLEAN LoadSacDriver,__out PCHAR BadFile);

//
// load boot device driver
//
ARC_STATUS BlLoadBootDrivers(__in PLDR_SCAN_IMPORT_SERACH_PATH_SET LoadDevicePath,__in PLIST_ENTRY ListHead,__out PCHAR BadFileName);

//
// add boot driver
//
ARC_STATUS BlAddToBootDriverList(__in PLIST_ENTRY ListHead,__in PWCHAR FileName,__in PWCHAR RegistryName,__in PWCHAR Group,
								 __in ULONG Tag,__in ULONG ErrorControl,__in BOOLEAN InsertHeadOrTail);

//
// load system hive
//
ARC_STATUS BlLoadSystemHive(__in ULONG DeviceId,__in PCHAR DeviceName,__in PCHAR DirectoryPath,__in PCHAR FileName);

//
// load system.log
//
ARC_STATUS BlLoadSystemHiveLog(__in ULONG DeviceId,__in PCHAR DeviceName,__in PCHAR DirectoryPath,__in PCHAR FileName,__out PVOID* FileBuffer);