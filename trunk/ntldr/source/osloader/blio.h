//********************************************************************
//	created:	12:8:2008   5:31
//	file:		blio.h
//	author:		tiamo
//	purpose:	io lib
//********************************************************************

#pragma once

//
// rename
//
typedef ARC_STATUS (*PRENAME_ROUTINE)(__in ULONG FileId,__in PCHAR NewName);

//
// close notification
//
typedef VOID (*PARC_DEVICE_CLOSE_NOTIFICATION_ROUTINE)(__in ULONG DeviceId);

//
// file flags
//
typedef struct _BL_FILE_FLAGS
{
	//
	// opened
	//
	ULONG												Open : 1;

	//
	// read access
	//
	ULONG												Read : 1;

	//
	// write access
	//
	ULONG												Write : 1;

	//
	// double space
	//
	ULONG												DoubleSpace : 1;
}BL_FILE_FLAGS,*PBL_FILE_FLAGS;

//
// bootfs info
//
typedef struct _BOOTFS_INFO
{
	//
	// driver name in unicode format
	//
	PWSTR												DriverName;
}BOOTFS_INFO,*PBOOTFS_INFO;

//
// device functions
//
typedef struct _BL_DEVICE_ENTRY_TABLE
{
	//
	// close
	//
	PARC_CLOSE_ROUTINE									Close;

	//
	// mount
	//
	PARC_MOUNT_ROUTINE									Mount;

	//
	// open
	//
	PARC_OPEN_ROUTINE									Open;

	//
	// read
	//
	PARC_READ_ROUTINE									Read;

	//
	// get read status
	//
	PARC_READ_STATUS_ROUTINE							GetReadStatus;

	//
	// seek
	//
	PARC_SEEK_ROUTINE									Seek;

	//
	// write
	//
	PARC_WRITE_ROUTINE									Write;

	//
	// get info
	//
	PARC_GET_FILE_INFO_ROUTINE							GetFileInformation;

	//
	// set info
	//
	PARC_SET_FILE_INFO_ROUTINE							SetFileInformation;

	//
	// rename file
	//
	PRENAME_ROUTINE										Rename;

	//
	// get dirent
	//
	PARC_GET_DIRECTORY_ENTRY_ROUTINE					GetDirectoryEntry;

	//
	// bootfs info
	//
	PBOOTFS_INFO										BootFsInfo;
}BL_DEVICE_ENTRY_TABLE, *PBL_DEVICE_ENTRY_TABLE;

//
// file system structure context
//
typedef union _FS_STRUCTURE_CONTEXT
{
#if _CDFS_SUPPORT_
	//
	// cdfs
	//
	CDFS_STRUCTURE_CONTEXT								CdfsStructure;
#endif

#if _FAT_SUPPORT_
	//
	// fastfat
	//
	FAT_STRUCTURE_CONTEXT								FatStructure;
#endif

#if _NET_SUPPORT_
	//
	// net
	//
	NET_STRUCTURE_CONTEXT								NetStructure;
#endif

#if _NTFS_SUPPORT_
	//
	// ntfs
	//
	NTFS_STRUCTURE_CONTEXT								NtfsStructure;
#endif
}FS_STRUCTURE_CONTEXT,*PFS_STRUCTURE_CONTEXT;

//
// partition context
//
typedef struct _PARTITION_CONTEXT
{
	//
	// total length
	//
	LARGE_INTEGER										PartitionLength;

	//
	// starting sector
	//
	ULONG												StartingSector;

	//
	// ending sector
	//
	ULONG												EndingSector;

	//
	// disk id
	//
	UCHAR												DiskId;

	//
	// device unit
	//
	UCHAR												DeviceUnit;

	//
	// target id
	//
	UCHAR												TargetId;

	//
	// path id
	//
	UCHAR												PathId;

	//
	// byte to sector shift
	//
	ULONG												SectorShift;

	//
	// size
	//
	ULONG												Size;

	//
	// port device object
	//
	PVOID												PortDeviceObject;
}PARTITION_CONTEXT,*PPARTITION_CONTEXT;

//
// serial port context structure
//
typedef struct _SERIAL_CONTEXT
{
	//
	// base address
	//
	ULONG												PortBase;

	//
	// number
	//
	ULONG												PortNumber;
}SERIAL_CONTEXT,*PSERIAL_CONTEXT;

//
// drive context structure (used for x86 BIOS)
//
typedef struct _DRIVE_CONTEXT
{
	//
	// is cdrom
	//
	BOOLEAN												IsCdRom;

	//
	// drive id
	//
	UCHAR												DriveId;

	//
	// sectors
	//
	USHORT												Sectors;

	//
	// cylinder count
	//
	USHORT												Cylinders;

	//
	// head count
	//
	USHORT												Heads;

	//
	// offset 8
	//
	BOOLEAN												Int13Ext;
}DRIVE_CONTEXT,*PDRIVE_CONTEXT;

//
// Define Floppy context structure
//
typedef struct _FLOPPY_CONTEXT
{
	//
	// type
	//
	ULONG												DriveType;

	//
	// sector per track
	//
	ULONG												SectorsPerTrack;

	//
	// disk id
	//
	UCHAR												DiskId;
}FLOPPY_CONTEXT,*PFLOPPY_CONTEXT;

//
// keyboard context structure
//
typedef struct _KEYBOARD_CONTEXT
{
	//
	// scan codes
	//
	BOOLEAN												ScanCodes;
}KEYBOARD_CONTEXT,*PKEYBOARD_CONTEXT;

//
// console context
//
typedef struct _CONSOLE_CONTEXT
{
	//
	// number
	//
	ULONG												ConsoleNumber;
}CONSOLE_CONTEXT,*PCONSOLE_CONTEXT;

//
// file table entry
//
typedef struct _BL_FILE_TABLE
{
	//
	// flags
	//
	BL_FILE_FLAGS										Flags;

	//
	// device id,offset = 4
	//
	ULONG												DeviceId;

	//
	// current position,offset = 8
	//
	LARGE_INTEGER										Position;

	//
	// structure context,ofsset = 10
	//
	PVOID												StructureContext;

	//
	// device entry table,offset = 0x14
	//
	PBL_DEVICE_ENTRY_TABLE								DeviceEntryTable;

	//
	// file name length,offset = 0x18
	//
	UCHAR												FileNameLength;

	//
	// file name buffer offset = 0x1c
	//
	CHAR												FileName[32];

	//
	// context,offset = 0x40(padding required)
	//
	union
	{
		//
		// net file
		//
		NET_FILE_CONTEXT								NetFileContext;

	#if _FAT_SUPPORT_
		//
		// fat
		//
		FAT_FILE_CONTEXT								FatFileContext;
	#endif

	#if _CDFS_SUPPORT_
		//
		// cdfs
		//
		CDFS_FILE_CONTEXT								CdfsFileContext;
	#endif

	#if _ETFS_SUPPORT_
		//
		// etfs
		//
		ETFS_FILE_CONTEXT								EtfsFileContext;
	#endif

	#if _UDFS_SUPPORT_
		//
		// udfs
		//
		UDFS_FILE_CONTEXT								UdfsFileContext;
	#endif

	#if _NTFS_SUPPORT_
		//
		// ntfs file
		//
		NTFS_FILE_CONTEXT								NtfsFileContext;
	#endif

		//
		// partition
		//
		PARTITION_CONTEXT								PartitionContext;

		//
		// serial
		//
		SERIAL_CONTEXT									SerialContext;

		//
		// drive
		//
		DRIVE_CONTEXT									DriveContext;

		//
		// floppy
		//
		FLOPPY_CONTEXT									FloppyContext;

		//
		// keyboard
		//
		KEYBOARD_CONTEXT								KeyboardContext;

		//
		// console
		//
		CONSOLE_CONTEXT									ConsoleContext;
	}u;
}BL_FILE_TABLE,*PBL_FILE_TABLE;

//
// file system cache
//
typedef	struct _BL_FILE_SYSTEM_CACHE
{
	//
	// device id
	//
	ULONG												DeviceId;

	//
	// structure context
	//
	PVOID												StructureContext;

	//
	// device entry table
	//
	PBL_DEVICE_ENTRY_TABLE								DeviceEntryTable;
}BL_FILE_SYSTEM_CACHE,*PBL_FILE_SYSTEM_CACHE;

//
// file table
//
extern BL_FILE_TABLE									BlFileTable[48];

//
// initialize io system
//
ARC_STATUS BlIoInitialize();

//
// cache close
//
VOID ArcCacheClose(__in ULONG DeviceId);

//
// register for device close
//
ARC_STATUS ArcRegisterForDeviceClose(__in PARC_DEVICE_CLOSE_NOTIFICATION_ROUTINE Routine);

//
// get fs info
//
PBOOTFS_INFO BlGetFsInfo(IN ULONG DeviceId);

//
// close
//
ARC_STATUS BlClose(__in ULONG FileId);

//
// open
//
ARC_STATUS BlOpen(__in ULONG DeviceId,__in PCHAR OpenPath,__in OPEN_MODE OpenMode,__out PULONG FileId);

//
// read
//
ARC_STATUS BlRead(__in ULONG FileId,__out PVOID Buffer,__in ULONG Length,__out PULONG Count);

//
// read status
//
ARC_STATUS BlGetReadStatus(__in ULONG FileId);

//
// seek
//
ARC_STATUS BlSeek(__in ULONG FileId,__in PLARGE_INTEGER Offset,__in SEEK_MODE SeekMode);

//
// write
//
ARC_STATUS BlWrite(__in ULONG FileId,__in PVOID Buffer,__in ULONG Length,__out PULONG Count);

//
// get file info
//
ARC_STATUS BlGetFileInformation(__in ULONG FileId,__in PFILE_INFORMATION FileInformation);

//
// rename
//
ARC_STATUS BlRename(__in ULONG FileId,__in PCHAR NewName);

//
// set file info
//
ARC_STATUS BlSetFileInformation(__in ULONG FileId,__in ULONG AttributeFlags,__in ULONG AttributeMask);

//
// read at offset
//
ARC_STATUS BlReadAtOffset(__in ULONG FileId,__in ULONG Offset,__in ULONG Length,__in PVOID Buffer);