
#define SeLengthSid( Sid ) \
(8 + (4 * ((SID *)Sid)->SubAuthorityCount))



#define TOKEN_HAS_TRAVERSE_PRIVILEGE    0x01
#define TOKEN_HAS_BACKUP_PRIVILEGE      0x02
#define TOKEN_HAS_RESTORE_PRIVILEGE     0x04

// #define SECURITY_NT_AUTHORITY           {0,0,0,0,0,5}   // ntifs
// 
// #define SECURITY_DIALUP_RID             (0x00000001L)
// #define SECURITY_NETWORK_RID            (0x00000002L)
// #define SECURITY_BATCH_RID              (0x00000003L)
// #define SECURITY_INTERACTIVE_RID        (0x00000004L)
// #define SECURITY_SERVICE_RID            (0x00000006L)
// #define SECURITY_ANONYMOUS_LOGON_RID    (0x00000007L)
// #define SECURITY_PROXY_RID              (0x00000008L)
// #define SECURITY_SERVER_LOGON_RID       (0x00000009L)
// 
// #define SECURITY_LOGON_IDS_RID          (0x00000005L)
// #define SECURITY_LOGON_IDS_RID_COUNT    (3L)
// 
// #define SECURITY_LOCAL_SYSTEM_RID       (0x00000012L)
// 
// #define SECURITY_NT_NON_UNIQUE          (0x00000015L)
// 
// #define SECURITY_BUILTIN_DOMAIN_RID     (0x00000020L)



// well-known aliases ...
// 
// #define DOMAIN_ALIAS_RID_ADMINS        (0x00000220L)
// #define DOMAIN_ALIAS_RID_USERS         (0x00000221L)
// #define DOMAIN_ALIAS_RID_GUESTS        (0x00000222L)
// #define DOMAIN_ALIAS_RID_POWER_USERS   (0x00000223L)
// 
// #define DOMAIN_ALIAS_RID_ACCOUNT_OPS   (0x00000224L)
// #define DOMAIN_ALIAS_RID_SYSTEM_OPS    (0x00000225L)
// #define DOMAIN_ALIAS_RID_PRINT_OPS     (0x00000226L)
// #define DOMAIN_ALIAS_RID_BACKUP_OPS    (0x00000227L)
// 
// #define DOMAIN_ALIAS_RID_REPLICATOR    (0x00000228L)




typedef enum _MMFLUSH_TYPE {
    MmFlushForDelete,
	MmFlushForWrite
} MMFLUSH_TYPE;

typedef USHORT SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;

#define SID_IDENTIFIER_AUTHORITY_DEFINED
// typedef struct _SID_IDENTIFIER_AUTHORITY {
//     UCHAR Value[6];
// } SID_IDENTIFIER_AUTHORITY, *PSID_IDENTIFIER_AUTHORITY;
// 
// typedef struct  _SECURITY_DESCRIPTOR
// {
//     UCHAR Revision;
//     UCHAR Sbz1;
//     SECURITY_DESCRIPTOR_CONTROL Control;
//     PSID Owner;
//     PSID Group;
//     PACL Sacl;
//     PACL Dacl;
// }	SECURITY_DESCRIPTOR;

#pragma pack(push,4)
typedef struct _MyIO_STACK_LOCATION {
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    UCHAR Flags;
    UCHAR Control;

    //
    // The following user parameters are based on the service that is being
    // invoked.  Drivers and file systems can determine which set to use based
    // on the above major and minor function codes.
    //

    union {

        //
        // System service parameters for:  NtCreateFile
        //

        struct {
            PIO_SECURITY_CONTEXT SecurityContext;
            ULONG Options;
            USHORT FileAttributes;
            USHORT ShareAccess;
            ULONG EaLength;
        } Create;

// end_ntddk end_nthal




// begin_ntddk begin_nthal

        //
        // System service parameters for:  NtReadFile
        //

        struct {
            ULONG Length;
            ULONG Key;
            LARGE_INTEGER ByteOffset;
        } Read;

        //
        // System service parameters for:  NtWriteFile
        //

        struct {
            ULONG Length;
            ULONG Key;
            LARGE_INTEGER ByteOffset;
        } Write;

// end_ntddk end_nthal

        //
        // System service parameters for:  NtQueryDirectoryFile
        //

        struct {
            ULONG Length;
            PSTRING FileName;
            FILE_INFORMATION_CLASS FileInformationClass;
            ULONG FileIndex;
        } QueryDirectory;

        //
        // System service parameters for:  NtNotifyChangeDirectoryFile
        //

        struct {
            ULONG Length;
            ULONG CompletionFilter;
        } NotifyDirectory;

// begin_ntddk begin_nthal

        //
        // System service parameters for:  NtQueryInformationFile
        //

        struct {
            ULONG Length;
            FILE_INFORMATION_CLASS FileInformationClass;
        } QueryFile;

        //
        // System service parameters for:  NtSetInformationFile
        //

        struct {
            ULONG Length;
            FILE_INFORMATION_CLASS FileInformationClass;
            PFILE_OBJECT FileObject;
            union {
                struct {
                    BOOLEAN ReplaceIfExists;
                    BOOLEAN AdvanceOnly;
                };
                ULONG ClusterCount;
                HANDLE DeleteHandle;
            };
        } SetFile;

// end_ntddk end_nthal

        //
        // System service parameters for:  NtQueryEaFile
        //

        struct {
            ULONG Length;
            PVOID EaList;
            ULONG EaListLength;
            ULONG EaIndex;
        } QueryEa;

        //
        // System service parameters for:  NtSetEaFile
        //

        struct {
            ULONG Length;
        } SetEa;

// begin_ntddk begin_nthal

        //
        // System service parameters for:  NtQueryVolumeInformationFile
        //

        struct {
            ULONG Length;
            FS_INFORMATION_CLASS FsInformationClass;
        } QueryVolume;

// end_ntddk end_nthal

        //
        // System service parameters for:  NtSetVolumeInformationFile
        //

        struct {
            ULONG Length;
            FS_INFORMATION_CLASS FsInformationClass;
        } SetVolume;

        //
        // System service parameters for:  NtFsControlFile
        //
        // Note that the user's output buffer is stored in the UserBuffer field
        // and the user's input buffer is stored in the SystemBuffer field.
        //

        struct {
            ULONG OutputBufferLength;
            ULONG InputBufferLength;
            ULONG FsControlCode;
            PVOID Type3InputBuffer;
        } FileSystemControl;

        //
        // System service parameters for:  NtLockFile/NtUnlockFile
        //

        struct {
            PLARGE_INTEGER Length;
            ULONG Key;
            LARGE_INTEGER ByteOffset;
        } LockControl;

// begin_ntddk begin_nthal

        //
        // System service parameters for:  NtFlushBuffersFile
        //
        // No extra user-supplied parameters.
        //

// end_ntddk end_nthal

        //
        // System service parameters for:  NtCancelIoFile
        //
        // No extra user-supplied parameters.
        //

// begin_ntddk begin_nthal

        //
        // System service parameters for:  NtDeviceIoControlFile
        //
        // Note that the user's output buffer is stored in the UserBuffer field
        // and the user's input buffer is stored in the SystemBuffer field.
        //

        struct {
            ULONG OutputBufferLength;
            ULONG InputBufferLength;
            ULONG IoControlCode;
            PVOID Type3InputBuffer;
        } DeviceIoControl;

        //
        // System service parameters for:  NtQuerySecurityObject
        //

        struct {
            SECURITY_INFORMATION SecurityInformation;
            ULONG Length;
        } QuerySecurity;

        //
        // System service parameters for:  NtSetSecurityObject
        //

        struct {
            SECURITY_INFORMATION SecurityInformation;
            PSECURITY_DESCRIPTOR SecurityDescriptor;
        } SetSecurity;

        //
        // Non-system service parameters.
        //
        // Parameters for MountVolume
        //

        struct {
            PVPB Vpb;
            PDEVICE_OBJECT DeviceObject;
        } MountVolume;

        //
        // Parameters for VerifyVolume
        //

        struct {
            PVPB Vpb;
            PDEVICE_OBJECT DeviceObject;
        } VerifyVolume;

        //
        // Parameters for Scsi with internal device contorl.
        //

        struct {
            struct _SCSI_REQUEST_BLOCK *Srb;
        } Scsi;

#ifdef _PNP_POWER_
        //
        // Parameters for device power control.
        //

        struct {
            POWER_STATE PowerState;
            ULONG DevicePowerState;
        } SetPower;

        //
        // Parameters for device removal irp
        //

        struct {
            PDEVICE_OBJECT DeviceToRemove;
        } RemoveDevice;

#endif // _PNP_POWER_

        //
        // Parameters for StartDevice
        //

        struct {
            PCM_RESOURCE_LIST AllocatedResources;
        } StartDevice;

        //
        // Parameters for Cleanup
        //
        // No extra parameters supplied
        //

        //
        // Others - driver-specific
        //

        struct {
            PVOID Argument1;
            PVOID Argument2;
            PVOID Argument3;
            PVOID Argument4;
        } Others;

    } Parameters;

    //
    // Save a pointer to this device driver's device object for this request
    // so it can be passed to the completion routine if needed.
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // The following location contains a pointer to the file object for this
    //

    PFILE_OBJECT FileObject;

    //
    // The following routine is invoked depending on the flags in the above
    // flags field.
    //

    PIO_COMPLETION_ROUTINE CompletionRoutine;

    //
    // The following is used to store the address of the context parameter
    // that should be passed to the CompletionRoutine.
    //

    PVOID Context;

} MyIO_STACK_LOCATION, *PMyIO_STACK_LOCATION;
#pragma pack(pop)



BOOLEAN
MmSetAddressRangeModified (
						   IN PVOID Address,
						   IN ULONG Length
    );

BOOLEAN
MmCanFileBeTruncated (
					  IN PSECTION_OBJECT_POINTERS SectionPointer,
					  IN PLARGE_INTEGER NewFileSize
    );

PIRP
IoGetTopLevelIrp(
				 VOID
    );

VOID
RtlGenerate8dot3Name (
					  IN PUNICODE_STRING Name,
					  IN BOOLEAN AllowExtendedCharacters,
					  IN OUT PGENERATE_NAME_CONTEXT Context,
					  OUT PUNICODE_STRING Name8dot3
    );

PFILE_OBJECT
IoCreateStreamFileObject(
						 IN PFILE_OBJECT FileObject OPTIONAL,
						 IN PDEVICE_OBJECT DeviceObject OPTIONAL
    );

BOOLEAN
IoIsOperationSynchronous(
						 IN PIRP Irp
    );

VOID
IoSetTopLevelIrp(
				 IN PIRP Irp
				 );

PEPROCESS
IoGetRequestorProcess(
					  IN PIRP Irp
    );

BOOLEAN
MmFlushImageSection (
    IN PSECTION_OBJECT_POINTERS SectionPointer,
    IN MMFLUSH_TYPE FlushType
    );

NTKERNELAPI
NTSTATUS
IoCheckEaBufferValidity(
						IN PFILE_FULL_EA_INFORMATION EaBuffer,
						IN ULONG EaLength,
						OUT PULONG ErrorOffset
    );

NTSTATUS
IoSetInformation(
				 IN PFILE_OBJECT FileObject,
				 IN FILE_INFORMATION_CLASS FileInformationClass,
				 IN ULONG Length,
				 IN PVOID FileInformation
    );

NTSTATUS
RtlGetCompressionWorkSpaceSize (
								IN USHORT CompressionFormatAndEngine,
								OUT PULONG CompressBufferWorkSpaceSize,
								OUT PULONG CompressFragmentWorkSpaceSize
    );

NTSTATUS
RtlCompressBuffer (
				   IN USHORT CompressionFormatAndEngine,
				   IN PUCHAR UncompressedBuffer,
				   IN ULONG UncompressedBufferSize,
				   OUT PUCHAR CompressedBuffer,
				   IN ULONG CompressedBufferSize,
				   IN ULONG UncompressedChunkSize,
				   OUT PULONG FinalCompressedSize,
				   IN PVOID WorkSpace
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDescribeChunk (
				  IN USHORT CompressionFormat,
				  IN OUT PUCHAR *CompressedBuffer,
				  IN PUCHAR EndOfCompressedBufferPlus1,
				  OUT PUCHAR *ChunkBuffer,
				  OUT PULONG ChunkSize
    );

NTSTATUS
RtlDecompressFragment (
					   IN USHORT CompressionFormat,
					   OUT PUCHAR UncompressedFragment,
					   IN ULONG UncompressedFragmentSize,
					   IN PUCHAR CompressedBuffer,
					   IN ULONG CompressedBufferSize,
					   IN ULONG FragmentOffset,
					   OUT PULONG FinalUncompressedSize,
					   IN PVOID WorkSpace
    );

NTSTATUS
RtlDecompressBuffer (
					 IN USHORT CompressionFormat,
					 OUT PUCHAR UncompressedBuffer,
					 IN ULONG UncompressedBufferSize,
					 IN PUCHAR CompressedBuffer,
					 IN ULONG CompressedBufferSize,
					 OUT PULONG FinalUncompressedSize
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDescribeChunk (
				  IN USHORT CompressionFormat,
				  IN OUT PUCHAR *CompressedBuffer,
				  IN PUCHAR EndOfCompressedBufferPlus1,
				  OUT PUCHAR *ChunkBuffer,
				  OUT PULONG ChunkSize
    );

NTSTATUS
RtlDecompressFragment (
					   IN USHORT CompressionFormat,
					   OUT PUCHAR UncompressedFragment,
					   IN ULONG UncompressedFragmentSize,
					   IN PUCHAR CompressedBuffer,
					   IN ULONG CompressedBufferSize,
					   IN ULONG FragmentOffset,
					   OUT PULONG FinalUncompressedSize,
					   IN PVOID WorkSpace
    );

NTSTATUS
RtlDecompressBuffer (
					 IN USHORT CompressionFormat,
					 OUT PUCHAR UncompressedBuffer,
					 IN ULONG UncompressedBufferSize,
					 IN PUCHAR CompressedBuffer,
					 IN ULONG CompressedBufferSize,
					 OUT PULONG FinalUncompressedSize
    );

PVOID
RtlInsertElementGenericTable (
							  IN PRTL_GENERIC_TABLE Table,
							  IN PVOID Buffer,
							  IN CLONG BufferSize,
							  OUT PBOOLEAN NewElement OPTIONAL
    );

VOID
IoAcquireVpbSpinLock(
					 OUT PKIRQL Irql
    );

NTSTATUS
SeQuerySecurityDescriptorInfo (
							   IN PSECURITY_INFORMATION SecurityInformation,
							   OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
							   IN OUT PULONG Length,
							   IN PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor
							   );

BOOLEAN
SeAuditingFileEvents(
					 IN BOOLEAN AccessGranted,
					 IN PSECURITY_DESCRIPTOR SecurityDescriptor
					 );

VOID
SeLockSubjectContext(
					 IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    );

NTSTATUS
SeAppendPrivileges(
				   PACCESS_STATE AccessState,
				   PPRIVILEGE_SET Privileges
    );

VOID
SeFreePrivileges(
				 IN PPRIVILEGE_SET Privileges
				 );

VOID
SeUnlockSubjectContext(
					   IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    );



VOID
IoReleaseVpbSpinLock(
					 IN KIRQL Irql
    );

VOID
SeOpenObjectForDeleteAuditAlarm (
								 IN PUNICODE_STRING ObjectTypeName,
								 IN PVOID Object OPTIONAL,
								 IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
								 IN PSECURITY_DESCRIPTOR SecurityDescriptor,
								 IN PACCESS_STATE AccessState,
								 IN BOOLEAN ObjectCreated,
								 IN BOOLEAN AccessGranted,
								 IN KPROCESSOR_MODE AccessMode,
								 OUT PBOOLEAN GenerateOnClose
    );

VOID
SeOpenObjectAuditAlarm (
						IN PUNICODE_STRING ObjectTypeName,
						IN PVOID Object OPTIONAL,
						IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
						IN PSECURITY_DESCRIPTOR SecurityDescriptor,
						IN PACCESS_STATE AccessState,
						IN BOOLEAN ObjectCreated,
						IN BOOLEAN AccessGranted,
						IN KPROCESSOR_MODE AccessMode,
						OUT PBOOLEAN GenerateOnClose
    );

VOID
RtlInitializeGenericTable (
						   IN PRTL_GENERIC_TABLE Table,
						   IN PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
						   IN PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
						   IN PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
						   IN PVOID TableContext
    );

PVOID
RtlEnumerateGenericTableWithoutSplaying (
										 IN PRTL_GENERIC_TABLE Table,
										 IN PVOID *RestartKey
    );


NTSTATUS
SeSetSecurityDescriptorInfo (
							 IN PVOID Object OPTIONAL,
							 IN PSECURITY_INFORMATION SecurityInformation,
							 IN PSECURITY_DESCRIPTOR ModificationDescriptor,
							 IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
							 IN POOL_TYPE PoolType,
							 IN PGENERIC_MAPPING GenericMapping
    );

PRTL_SPLAY_LINKS
RtlSplay (
		  IN PRTL_SPLAY_LINKS Links
    );

PVOID
RtlLookupElementGenericTable (
							  IN PRTL_GENERIC_TABLE Table,
							  IN PVOID Buffer
    );

VOID
SeCaptureSubjectContext (
						 OUT PSECURITY_SUBJECT_CONTEXT SubjectContext
    );

NTSTATUS
ObQueryObjectAuditingByHandle(
							  IN HANDLE Handle,
							  OUT PBOOLEAN GenerateOnClose
    );

VOID
SeDeleteObjectAuditAlarm(
						 IN PVOID Object,
						 IN HANDLE Handle
						 );

BOOLEAN
RtlDeleteElementGenericTable (
							  IN PRTL_GENERIC_TABLE Table,
							  IN PVOID Buffer
    );

NTSTATUS
ObQueryNameString(
				  IN PVOID Object,
				  OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
				  IN ULONG Length,
				  OUT PULONG ReturnLength
    );

VOID
FsRtlIncrementCcFastReadWait(
							 VOID
    ); 

VOID 
FsRtlIncrementCcFastReadNotPossible(
										 VOID
    ); 

NTSTATUS
RtlUnicodeStringToCountedOemString(
								   OUT POEM_STRING DestinationString,
								   IN PUNICODE_STRING SourceString,
								   IN BOOLEAN AllocateDestinationString
    );

VOID
RtlFreeOemString(
				 IN OUT POEM_STRING OemString
    );

NTSTATUS
RtlUnicodeStringToOemString(
							OUT POEM_STRING DestinationString,
							IN PUNICODE_STRING SourceString,
							IN BOOLEAN AllocateDestinationString
    );

BOOLEAN
IoIsSystemThread(
				 IN PETHREAD Thread
				 );

VOID
IoRegisterFileSystem(
					 IN OUT PDEVICE_OBJECT DeviceObject
					 );



NTSTATUS
RtlOemStringToUnicodeString(
							OUT PUNICODE_STRING DestinationString,
							IN POEM_STRING SourceString,
							IN BOOLEAN AllocateDestinationString
							);

ULONG
RtlLengthRequiredSid (
					  IN ULONG SubAuthorityCount
    );

NTSTATUS
RtlInitializeSid(
				 IN PSID Sid,
				 IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
				 IN UCHAR SubAuthorityCount
    );

PULONG
RtlSubAuthoritySid(
				   IN PSID Sid,
				   IN ULONG SubAuthority
    );

NTSTATUS
RtlCreateAcl (
			  IN PACL Acl,
			  IN ULONG AclLength,
			  IN ULONG AclRevision
    );

NTSTATUS
RtlAddAccessAllowedAce (
						IN OUT PACL Acl,
						IN ULONG AceRevision,
						IN ACCESS_MASK AccessMask,
						IN PSID Sid
    );

VOID
SeReleaseSubjectContext (
						 IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    );

PRTL_SPLAY_LINKS
RtlDelete (
		   IN PRTL_SPLAY_LINKS Links
    );

VOID
RtlFillMemoryUlong(
				   IN PVOID  Destination,
				   IN SIZE_T  Length,
				   IN ULONG  Pattern
    ); 