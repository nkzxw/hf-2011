//********************************************************************
//	created:	17:8:2008   19:30
//	file:		arcdisk.h
//	author:		tiamo
//	purpose:	arc disk
//********************************************************************

#pragma once

typedef struct _ARC_DISK_SIGNATURE
{
	//
	// list entry to arc_disk_info
	//
	LIST_ENTRY											ListEntry;

	//
	// signature
	//
	ULONG												Signature;

	//
	// arc name
	//
	PCHAR												ArcName;

	//
	// checksum
	//
	ULONG												CheckSum;

	//
	// valid partition table
	//
	BOOLEAN												ValidPartitionTable;

	//
	// support int13 extension
	//
	BOOLEAN												SupportInt13Extension;

	//
	// is gpt disk
	//
	BOOLEAN												IsGptDisk;

	//
	// efi disk guid
	//
	GUID												EfiDiskGuid;
}ARC_DISK_SIGNATURE,*PARC_DISK_SIGNATURE;

//
// arc disk info
//
typedef struct _ARC_DISK_INFORMATION
{
	//
	// list head
	//
	LIST_ENTRY											DiskSignatures;
}ARC_DISK_INFORMATION,*PARC_DISK_INFORMATION;

//
// get arc disk information
//
ARC_STATUS BlGetArcDiskInformation(__in BOOLEAN SetupInt13ExtensionInfo);

//
// open MBR partition and update file table info
//
ARC_STATUS HardDiskPartitionOpen(__in ULONG FileId,__in ULONG DiskId,__in UCHAR PartitionNumber);

//
// open EFI partition and update file table info
//
ARC_STATUS BlOpenGPTDiskPartition(__in ULONG FileId,__in ULONG DiskId,__in UCHAR PartitionNumber);