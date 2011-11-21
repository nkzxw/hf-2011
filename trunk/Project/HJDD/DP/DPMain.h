#ifndef _DPMAIN_H_
#define _DPMAIN_H_

#define FS_OTHER	0
#define FS_FAT16	1
#define FS_FAT32	2
#define FS_NTFS		3

//�����洢һ�������е������Ϣ�����ݽṹ�����ڹ����豸���豸��չ��
typedef struct _DP_FILTER_DEV_EXTENSION_ 
{
	//������֣�����"C:,D:"���е���ĸ����
	WCHAR					VolumeLetter;
	//������Ƿ��ڱ���״̬
	BOOL					Protect;
	//�������ܴ�С����byteΪ��λ
	LARGE_INTEGER			TotalSizeInByte;
	//��������ļ�ϵͳ��ÿ�ش�С����byteΪ��λ
	DWORD					ClusterSizeInByte;
	//������ÿ��������С����byteΪ��λ
	DWORD					SectorSizeInByte;
	//������豸��Ӧ�Ĺ����豸���豸����
	PDEVICE_OBJECT			FltDevObj;
	//������豸��Ӧ�Ĺ����豸���²��豸����
	PDEVICE_OBJECT			LowerDevObj;
	//������豸��Ӧ�������豸���豸����
	PDEVICE_OBJECT			PhyDevObj;
	//������ݽṹ�Ƿ��Ѿ�����ʼ�������
	BOOL					InitializeCompleted;
	//������ϵı���ϵͳʹ�õ�λͼ�ľ��
	PDP_BITMAP		Bitmap;	
	//����ת�����ļ����
	HANDLE					TempFile;
	//������ϵı���ϵͳʹ�õ��������
	LIST_ENTRY				ReqList;
	//������ϵı���ϵͳʹ�õ�������е���
	KSPIN_LOCK				ReqLock;
	//������ϵı���ϵͳʹ�õ�������е�ͬ���¼�
	KEVENT					ReqEvent;
	//������ϵı���ϵͳʹ�õ�������еĴ����߳�֮�߳̾��
	PVOID					ThreadHandle;
	//������ϵı���ϵͳʹ�õ�������еĴ����߳�֮������־
	BOOLEAN					ThreadTermFlag;
	//������ϵı���ϵͳ�Ĺػ���ҳ��Դ����ļ����¼�
	KEVENT					PagingPathCountEvent;
	//������ϵı���ϵͳ�Ĺػ���ҳ��Դ����ļ���
	LONG					PagingPathCount;
} DP_FILTER_DEV_EXTENSION, *PDP_FILTER_DEV_EXTENSION;

typedef struct _VOLUME_ONLINE_CONTEXT_
{
	//��volume_online��DeviceIoControl�д�����ɺ������豸��չ
	PDP_FILTER_DEV_EXTENSION	DevExt;
	//��volume_online��DeviceIoControl�д�����ɺ�����ͬ���¼�
	PKEVENT						Event;
}VOLUME_ONLINE_CONTEXT, *PVOLUME_ONLINE_CONTEXT;

#pragma pack(1)
typedef struct _DP_FAT16_BOOT_SECTOR
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
} DP_FAT16_BOOT_SECTOR, *PDP_FAT16_BOOT_SECTOR;

typedef struct _DP_FAT32_BOOT_SECTOR
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
} DP_FAT32_BOOT_SECTOR, *PDP_FAT32_BOOT_SECTOR;

typedef struct _DP_NTFS_BOOT_SECTOR
{
	UCHAR		Jump[3];					//0
	UCHAR		FSID[8];					//3
	USHORT		BytesPerSector;				//11
	UCHAR		SectorsPerCluster;			//13
	USHORT		ReservedSectors;			//14
	UCHAR		Mbz1;						//16		
	USHORT		Mbz2;						//17
	USHORT		Reserved1;					//19
	UCHAR		MediaDesc;					//21
	USHORT		Mbz3;						//22
	USHORT		SectorsPerTrack;			//24
	USHORT		Heads;						//26
	ULONG		HiddenSectors;				//28
	ULONG		Reserved2[2];				//32
	ULONGLONG	TotalSectors;				//40
	ULONGLONG	MftStartLcn;				//48
	ULONGLONG	Mft2StartLcn;				//56
}DP_NTFS_BOOT_SECTOR, *PDP_NTFS_BOOT_SECTOR;
#pragma pack()

NTSTATUS
DPCompleteRequest(
	IN	PIRP			Irp,
	IN	NTSTATUS		Status,
	IN	CCHAR			Priority
	);
	
NTSTATUS
DPSendToNextDriver(
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
DPAddDevice(
	IN	PDRIVER_OBJECT	DriverObject,
	IN	PDEVICE_OBJECT	PhysicalDeviceObject
	);

VOID
DPUnload(
	IN	PDRIVER_OBJECT	DriverObject
	);

NTSTATUS
DPDispatchAny(
    IN	PDEVICE_OBJECT	DeviceObject,
    IN	PIRP			Irp
    );
    
NTSTATUS
DPDispatchCreateClose(
    IN	PDEVICE_OBJECT	DeviceObject,
    IN	PIRP			Irp
    );
	
NTSTATUS
DPDispatchPower(
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	PIRP			Irp
	);

NTSTATUS	
DPDispatchPnp(
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	PIRP			Irp
	);

NTSTATUS
DPDispatchDeviceControl(
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	PIRP			Irp
	);

NTSTATUS
DPDispatchReadWrite(
    IN	PDEVICE_OBJECT	DeviceObject,
    IN	PIRP			Irp
    );

NTSTATUS
DiskShldForwardIrpSynchronous(
	IN PDEVICE_OBJECT DeviceObject,
	IN PDEVICE_OBJECT TargetDeviceObject,
	IN PIRP Irp
	);

#endif