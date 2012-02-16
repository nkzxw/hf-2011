#ifndef __FILEFLT_HELP_H__
#define __FILEFLT_HELP_H__

#define IsNetworkVolumeDevice(DeviceType, FileSystemType)  ((DeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) && (FileSystemType == FLT_FSTYPE_LANMAN))
//----------------------------------------------------------------------------------------
#define u8 	unsigned char
#define u16 	unsigned short int
#define u32 	unsigned int
#define u64   unsigned long long

void put_unaligned_be16(u16 val, u8 *p);

u16 get_unaligned_be16(u8 *p);

void put_unaligned_be32(u32 val, u8 *p);

u32 get_unaligned_be32(u8 *p);

void put_unaligned_be64(u64 val, u8 *p);

u64 get_unaligned_be64(u8 *p);

VOID
FsGetRandBytes(
	__out PVOID Data,
	__in LONG Size
	);
//----------------------------------------------------------------------------------------
//字符串相关

PCHAR 
FsLowerString( 
	__inout PCHAR Source, 
	__in ULONG Length 
	);

int
FsCompareStringWithPatten(
	  __in PUNICODE_STRING FileName,
	  __in WCHAR *StringList[],
	  __in ULONG Count
	  );


int
FsReverseCompareString(
	  __in PUNICODE_STRING FileName,
	  __in WCHAR *StringList[],
	  __in ULONG Count
	  );

VOID
FsFreeUnicodeString (
    __inout PUNICODE_STRING String
    );

NTSTATUS
FsAllocateUnicodeString (
    __inout PUNICODE_STRING String
    );

PWCHAR
FsWcsstrExtern(
    __in PWCHAR Source,
	__in LONG	SourceLength,
	__in LONG   SourceMaxLength,
	__in PWCHAR StrSearch //must be string
	);

//----------------------------------------------------------------------------------------
//系统相关

VOID
FsGetCurrentVersion (
	VOID
    );

VOID
FsLoadDynamicFunctions (
	VOID
    );

BOOLEAN
FsReadDriverParameters(
	 __in HANDLE 				DriverRegKey,
	 __in PUNICODE_STRING 	ValueName,
	 __out UCHAR  			*Buffer,
	 __in  ULONG  			BufferLength
	 );

NTSTATUS
FsIsShadowCopyVolume (
    __in PDEVICE_OBJECT StorageStackDeviceObject,
    __out PBOOLEAN IsShadowCopy
    );

NTSTATUS 
FsGetStorageDeviceBusType(
	IN PDEVICE_OBJECT StorageStackDeviceObject, 
	ULONG* puType
	);

BOOLEAN
FsIsNetworkAccess(
    __in PFLT_CALLBACK_DATA Data
    );

VOID
FsKernelSleep(
	__in ULONG MicroSecond
	);

//----------------------------------------------------------------------------------------
//文件相关

NTSTATUS
FsCreateFile(
       __in PFLT_INSTANCE        Instance,  	
       __in ULONG                     DesiredAccess,
       __in ULONG                     CreateDisposition,
       __in ULONG                     CreateOptions,
       __in ULONG                     ShareAccess,
       __in ULONG                     FileAttribute,
	__in PUNICODE_STRING  FileName,
	__out HANDLE *		  FileHandle,
	__out PFILE_OBJECT *	  FileObject
    );

NTSTATUS
FsOpenFile(
	__in PFLT_INSTANCE                  Instance,  	
	__in PUNICODE_STRING 		FileName,
	__out HANDLE *				FileHandle,
	__out PFILE_OBJECT *			FileObjectt,
	__out PULONG					FileStatus
	);

VOID
FsCloseFile(
	__in HANDLE 			FileHandle,
	__in PFILE_OBJECT 	FileObject
	);

int
FsDeleteFile(
        __in PFLT_INSTANCE    Instance,
        __in PFILE_OBJECT       FileObject
        );

NTSTATUS
FsReadFile (
    __in PFLT_INSTANCE  Instance,
    __in PFILE_OBJECT   FileObject,
    __in LONGLONG ByteOffset,
    __out PVOID Buffer,
    __in ULONG  Length,
    __in FLT_IO_OPERATION_FLAGS Flags,
    __out PULONG  BytesRead OPTIONAL,
    __in PFLT_COMPLETED_ASYNC_IO_CALLBACK CallbackRoutine OPTIONAL,
    __in PVOID  CallbackContext OPTIONAL 
    );

NTSTATUS
FsWriteFile (
    __in PFLT_INSTANCE  Instance,
    __in PFILE_OBJECT  FileObject,
    __in LONGLONG  ByteOffset,
    __in ULONG  Length,
    __in PVOID  Buffer,
    __in FLT_IO_OPERATION_FLAGS  Flags,
    __out PULONG  BytesWrite OPTIONAL,
    __out PFLT_COMPLETED_ASYNC_IO_CALLBACK  CallbackRoutine OPTIONAL,
    __in PVOID  CallbackContext OPTIONAL 
    );

BOOLEAN
FsCheckFileIsDirectoryByObject(
       __in PFLT_INSTANCE    Instance,
       __in PFILE_OBJECT  	FileObject
	);

int
FsCheckFileExistAndDirectoryByFileName(
       __inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in PFLT_FILE_NAME_INFORMATION NameInfo,
	__out BOOLEAN *Exist,
	__out BOOLEAN *Directory
	);

NTSTATUS
FsCalcFileHash(
	__in PUNICODE_STRING FileName,
	__out UCHAR *FileHash
	);

NTSTATUS
FsQueryInformationFile(
	__in       PFLT_INSTANCE Instance,
  	__in       PFILE_OBJECT FileObject,
  	__out      PVOID FileInformation,
  	__in       ULONG Length,
  	__in       FILE_INFORMATION_CLASS FileInformationClass,
  	__out_opt  PULONG LengthReturned
	 );

NTSTATUS
FsSetInformationFile (
	__in  PFLT_INSTANCE Instance,
	__in  PFILE_OBJECT FileObject,
	__in  PVOID FileInformation,
	__in  ULONG Length,
	__in  FILE_INFORMATION_CLASS FileInformationClass
	);
//----------------------------------------------------------------------------------------
//名字相关

ULONG 
FsGetTaskNameOffset(
	VOID
	);

NTSTATUS
FsGetObjectName(
    __in 		PVOID Object,
    __inout 	PUNICODE_STRING Name
    );

NTSTATUS
SfGetDeviceDosName(
    __in WCHAR *VolumeDeviceName,
    __out WCHAR *DosLetter,
    __in  ULONG *Offset
    );


NTSTATUS     
FsGetProcessImageName(    
    __in  HANDLE   ProcessId,    
    __out PUNICODE_STRING *ProcessImageName    
    );

int
FsGetFileNameWithoutStreamName(
	__in PFLT_FILE_NAME_INFORMATION NameInfo,
	__inout PUNICODE_STRING FileName
	);

//----------------------------------------------------------------------------------------
//资源相关

FORCEINLINE
PERESOURCE
FsAllocateResource (
    VOID
    )
{

    return ExAllocatePoolWithTag( NonPagedPool,
                                  sizeof( ERESOURCE ),
                                  RESOURCE_TAG );
}

FORCEINLINE
VOID
FsFreeResource (
    __in PERESOURCE Resource
    )
{

    ExFreePoolWithTag( Resource,
                       RESOURCE_TAG );
}


FORCEINLINE
VOID
__drv_acquiresCriticalRegion
FsAcquireResourceExclusive (
    __inout PERESOURCE Resource
    )
{
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) ||
           !ExIsResourceAcquiredSharedLite(Resource));

    KeEnterCriticalRegion();
    (VOID)ExAcquireResourceExclusiveLite( Resource, TRUE );
}

FORCEINLINE
VOID
__drv_acquiresCriticalRegion
FsAcquireResourceShared (
    __inout PERESOURCE Resource
    )
{
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    KeEnterCriticalRegion();
    (VOID)ExAcquireResourceSharedLite( Resource, TRUE );
}

FORCEINLINE
VOID
__drv_releasesCriticalRegion
__drv_mustHoldCriticalRegion
FsReleaseResource (
    __inout PERESOURCE Resource
    )
{
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    ASSERT(ExIsResourceAcquiredExclusiveLite(Resource) ||
           ExIsResourceAcquiredSharedLite(Resource));

    ExReleaseResourceLite(Resource);
    KeLeaveCriticalRegion();
}

//----------------------------------------------------------------------------
//thread
NTSTATUS 
FsStartThread (
	__in PKSTART_ROUTINE threadProc, 
	__in PVOID threadArg, 
	__out PKTHREAD *kThread
	);

VOID FsStopThread (
	__in PKTHREAD kThread
	);

int
FsCheckCryptFile(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects
	);
#endif /* __FILEFLT_HELP_H__ */
