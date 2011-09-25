/*++
Copyright (C) HF. 2011-2012

Abstract:
	Disk Filter	
Author:
	yhf (hongfu830202@163.com)
CreateTime:
	2011-9-25
--*/
#ifndef _HF_DISKFILTER_H_
#define _HF_DISKFILTER_H_

#define FS_OTHER	0
#define FS_FAT16	1
#define FS_FAT32	2
#define FS_NTFS		3

#define MY_REG_KEY_NAME L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\DiskFilter"
#define IOCTL_CLOSE_PROTECT  CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_OPEN_PROTECT  CTL_CODE(FILE_DEVICE_UNKNOWN,0x801,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define DEVICE_NAME 	L"\\Device\\HFDiskFilter"
#define DEVICE_LINK_NAME L"\\DosDevices\\HFDiskFilter"

#define _FileSystemNameLength	64
#define FAT16_SIG_OFFSET	54
#define FAT32_SIG_OFFSET	82
#define NTFS_SIG_OFFSET		3



typedef struct _DF_FILTER_DEV_EXTENSION_ 
{
	WCHAR					VolumeLetter;
	BOOL					Protect;
	LARGE_INTEGER			TotalSizeInByte;
	DWORD					ClusterSizeInByte;
	DWORD					SectorSizeInByte;
	PDEVICE_OBJECT			FltDevObj;
	PDEVICE_OBJECT			LowerDevObj;
	PDEVICE_OBJECT			PhyDevObj;
	BOOL					InitializeCompleted;
	PDP_BITMAP		Bitmap;	
	HANDLE					TempFile;
	LIST_ENTRY				ReqList;
	KSPIN_LOCK				ReqLock;
	KEVENT					ReqEvent;
	PVOID					ThreadHandle;
	BOOLEAN					ThreadTermFlag;
	KEVENT					PagingPathCountEvent;
	LONG					PagingPathCount;
} DF_FILTER_DEV_EXTENSION, *PDF_FILTER_DEV_EXTENSION;

typedef struct _VOLUME_ONLINE_CONTEXT_
{
	PDF_FILTER_DEV_EXTENSION	DevExt;
	PKEVENT						Event;
}VOLUME_ONLINE_CONTEXT, *PVOLUME_ONLINE_CONTEXT;

#pragma pack(1)
typedef struct _FAT16_BOOT_SECTOR
{
	UCHAR		JMPInstruction[3];
	UCHAR		OEM[8];
	USHORT		BytesPerSector;
	UCHAR		SectorsPerCluster;
	USHORT		ReservedSectors;
	UCHAR		NumberOfFATs;
	USHORT		RootEntries;
	USHORT		Sectors;
	UCHAR		MediaDescriptor;
	USHORT		SectorsPerFAT;
	USHORT		SectorsPerTrack;
	USHORT		Heads;
	DWORD		HiddenSectors;
	DWORD		LargeSectors;
	UCHAR		PhysicalDriveNumber;
	UCHAR		CurrentHead;
} FAT16_BOOT_SECTOR, *PFAT16_BOOT_SECTOR;

typedef struct _FAT32_BOOT_SECTOR
{
	UCHAR		JMPInstruction[3];
	UCHAR		OEM[8];
	USHORT		BytesPerSector;
	UCHAR		SectorsPerCluster;
	USHORT		ReservedSectors;
	UCHAR		NumberOfFATs;
	USHORT		RootEntries;
	USHORT		Sectors;
	UCHAR		MediaDescriptor;
	USHORT		SectorsPerFAT;
	USHORT		SectorsPerTrack;
	USHORT		Heads;
	DWORD		HiddenSectors;
	DWORD		LargeSectors;
	DWORD		LargeSectorsPerFAT;
	UCHAR		Data[24];
	UCHAR		PhysicalDriveNumber;
	UCHAR		CurrentHead;
} FAT32_BOOT_SECTOR, *PFAT32_BOOT_SECTOR;

typedef struct _NTFS_BOOT_SECTOR
{
	UCHAR		Jump[3];					
	UCHAR		FSID[8];					
	USHORT		BytesPerSector;				
	UCHAR		SectorsPerCluster;		
	USHORT		ReservedSectors;	
	UCHAR		Mbz1;							
	USHORT		Mbz2;						
	USHORT		Reserved1;		
	UCHAR		MediaDesc;				
	USHORT		Mbz3;					
	USHORT		SectorsPerTrack;		
	USHORT		Heads;					
	ULONG		HiddenSectors;		
	ULONG		Reserved2[2];			
	ULONGLONG	TotalSectors;			
	ULONGLONG	MftStartLcn;			
	ULONGLONG	Mft2StartLcn;				
}NTFS_BOOT_SECTOR, *PNTFS_BOOT_SECTOR;
#pragma pack()

NTSTATUS
dfCompleteRequest(
	IN	PIRP			Irp,
	IN	NTSTATUS		Status,
	IN	CCHAR			Priority
	);
	
NTSTATUS
dfSendToNextDriver(
	IN	PDEVICE_OBJECT	TgtDevObj,
	IN	PIRP			Irp
	);

NTSTATUS
DPSendToNextDriverSynchronous(
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	PDEVICE_OBJECT	TargetDeviceObject,
	IN	PIRP			Irp
	);

NTSTATUS DPDispatchShutdown(    
	IN PDEVICE_OBJECT  DeviceObject,
	IN PIRP  Irp
	);

NTSTATUS
HFAddDevice(
	IN	PDRIVER_OBJECT	DriverObject,
	IN	PDEVICE_OBJECT	PhysicalDeviceObject
	);

VOID
HFUnload(
	IN	PDRIVER_OBJECT	DriverObject
	);

NTSTATUS
HFDispatch(
    IN	PDEVICE_OBJECT	DeviceObject,
    IN	PIRP			Irp
    );
 
	
NTSTATUS
HFPower(
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	PIRP			Irp
	);

NTSTATUS	
HFPnp(
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	PIRP			Irp
	);

NTSTATUS
HFDeviceControl(
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	PIRP			Irp
	);

NTSTATUS
HFReadWrite(
    IN	PDEVICE_OBJECT	DeviceObject,
    IN	PIRP			Irp
    );

NTSTATUS    
HFReadDriverParameters (
		IN PUNICODE_STRING RegistryPath
		);

NTSTATUS
DriverEntry(
    IN	PDRIVER_OBJECT	DriverObject,
    IN	PUNICODE_STRING	RegistryPath
    );
    
#endif //_HF_DISKFILTER_H_