#ifndef __FILE_FILTER_H__
#define __FILE_FILTER_H__

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "control.h"

#define FLT_MEMORY_TAG			   	'mfTF'
#define VOLUME_CONTEXT_TAG  		'cvTF'
#define STREAM_CONTEXT_TAG  		'csTF'
#define STREAMHANDLE_CONTEXT_TAG 'hsTF'
#define PRE_2_POST_TAG      			'ppTF'
#define NAME_TAG					'tnTF'
#define RESOURCE_TAG                   	'trTF'
#define SESSION_LIST_TAG			'lsTF'
#define IMAGE_NAME_TAG			'niTF'
#define BUFFER_TAG                       		'tbTF'
#define TASK_POOL_TAG				'ptTF'
#define VOLUME_RULE_TAG             	'rvTF'
#define TASK_RULE_POOL_TAG		'rtTF'
#define SHARE_RULE_TAG               	'rsTF'
#define CRYPT_CONTEXT_TAG			'ccTF'
#define BUFFER_SWAP_TAG			'sbTF'
#define SIFS_METADATA_TAG               'msTF'

#define DEVICE_NAME_MAX_LENGTH 64
#define FILE_PATH_MAX_LENGTH  260

#define SIFS_SALT_SIZE 8
#define SIFS_SALT_SIZE_HEX (SIFS_SALT_SIZE*2)
#define SIFS_MAX_KEY_BYTES 64
#define SIFS_MAX_ENCRYPTED_KEY_BYTES 512
#define SIFS_DEFAULT_IV_BYTES 16

#define SIFS_MAX_IV_BYTES 16	/* 128 bits */
#define SIFS_SALT_BYTES 2

#define MAGIC_SIFS_MARKER 0x3c81b7f5
#define MAGIC_SIFS_MARKER_SIZE_BYTES 8	/* 4*2 */
#define SIFS_MINIMUM_HEADER_EXTENT_SIZE  	4096
#define SIFS_DEFAULT_EXTERN_SIZE       			512

//
//  MULTIVERSION NOTE: For this version of the driver, we need to know the
//  current OS version while we are running to make decisions regarding what
//  logic to use when the logic cannot be the same for all platforms.  We
//  will look up the OS version in DriverEntry and store the values
//  in these global variables.
//

//
//  Here is what the major and minor versions should be for the various OS versions:
//
//  OS Name                                 MajorVersion    MinorVersion
//  ---------------------------------------------------------------------
//  Windows 2000                             5                 0
//  Windows XP                               5                 1
//  Windows Server 2003                      5                 2
//

#define IS_WINDOWS2000() \
    ((g_FileFltContext.OsMajorVersion == 5) && (g_FileFltContext.OsMinorVersion == 0))

#define IS_WINDOWSXP() \
    ((g_FileFltContext.OsMajorVersion == 5) && (g_FileFltContext.OsMinorVersion == 1))

#define IS_WINDOWSXP_OR_LATER() \
    (((g_FileFltContext.OsMajorVersion == 5) && (g_FileFltContext.OsMinorVersion >= 1)) || \
     (g_FileFltContext.OsMajorVersion > 5))

#define IS_WINDOWSSRV2003_OR_LATER() \
    (((g_FileFltContext.OsMajorVersion == 5) && (g_FileFltContext.OsMinorVersion >= 2)) || \
     (g_FileFltContext.OsMajorVersion > 5))

#define IS_WINDOWSVISTA_OR_LATER() \
	(g_FileFltContext.OsMajorVersion >= 6)


typedef
NTSTATUS
(*PFS_GET_VERSION) (
    __inout PRTL_OSVERSIONINFOW VersionInformation
    );

typedef NTSTATUS (*PFS_ZWQUERYINFORMATIONPROCESS) (
    __in HANDLE ProcessHandle,
    __in PROCESSINFOCLASS ProcessInformationClass,
    __out PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out PULONG ReturnLength
    );

typedef struct _FS_DYNAMIC_FUNCTION_POINTERS {

    //
    //  The following routines should all be available on Windows XP (5.1) and
    //  later.
    //

    PFS_GET_VERSION GetVersion;

    PFS_ZWQUERYINFORMATIONPROCESS QueryInformationProcess;

} FS_DYNAMIC_FUNCTION_POINTERS, *PFS_DYNAMIC_FUNCTION_POINTERS;

#define FILEFLT_DRIVER_BOOT_START		0x00000001
#define FILEFLT_DRIVER_SYSTEM_START	0x00000002
#define FILEFLT_DRIVER_SERVICE_START 	0x00000004
#define FILEFLT_DRIVER_MANUAL_START   	0x00000008

#define FILEFLT_DRIVER_STATUS_MASK	0x0000000f
#define FILEFLT_DRIVER_STATUS_VALUE(x)  ((1 << x) & FILEFLT_DRIVER_STATUS_MASK)
#define FILEFLT_DRIVER_STATUS	(g_FileFltContext.Status & FILEFLT_DRIVER_STATUS_MASK)

#define FILEFLT_SYSTEM_INSTALL			0x00000010
#define FILEFLT_SYSTEM_UNLOAD			0x00000020
#define FILEFLT_SYSTEM_STATUS_MASK     	0x000000f0
#define FILEFLT_SYSTEM_STATUS_VALUE(x)   ((0x10 << x) & FILEFLT_SYSTEM_STATUS_MASK)
#define FILEFLT_SYSTEM_STATUS  (g_FileFltContext.Status & FILEFLT_SYSTEM_STATUS_MASK)

#define FILEFLT_WORK_RUNNING			0x00000100
#define FILEFLT_WORK_STOPPED			0x00000200
#define FILEFLT_WORK_STATUS_MASK        	0x00000f00
#define FILEFLT_WORK_STATUS_VALUE(x)   ((0x100 << x) & FILEFLT_WORK_STATUS_MASK)
#define FILEFLT_WORK_STATUS  (g_FileFltContext.Status & FILEFLT_WORK_STATUS_MASK)

#define FILEFLT_DRIVER_VALIDATA ((FILEFLT_DRIVER_STATUS & FILEFLT_DRIVER_BOOT_START) == FILEFLT_DRIVER_BOOT_START)
#define FILEFLT_SYSTEM_VALIDATA  ((FILEFLT_SYSTEM_STATUS & FILEFLT_SYSTEM_INSTALL) == FILEFLT_SYSTEM_INSTALL)
#define FILEFLT_WORK_VALIDATA ((FILEFLT_WORK_STATUS & FILEFLT_WORK_RUNNING) == FILEFLT_WORK_RUNNING)

#define FileFltStatusValidate  (FILEFLT_DRIVER_VALIDATA && FILEFLT_SYSTEM_VALIDATA && FILEFLT_WORK_VALIDATA)

typedef struct _FILEFLT_CONTEXT {

	ULONG OsMajorVersion;
	ULONG OsMinorVersion;

	FS_DYNAMIC_FUNCTION_POINTERS DynamicFunctions;
	
	PFLT_FILTER 	FileFltHandle;
	
	ULONG 		LoggingFlags;
	ULONG           Status;
	
	//
	//  Listens for incoming connections
	//

	PFLT_PORT ServerPort;

	//
	//  User process that connected to the port
	//

	PEPROCESS UserProcess;

	//
	//  Client port for a connection to user-mode
	//

	PFLT_PORT ClientPort;
	
}FILEFLT_CONTEXT;

extern FILEFLT_CONTEXT g_FileFltContext;

//
//  This is a volume context, one of these are attached to each volume
//  we monitor.  This is used to get a "DOS" name for debug display.
//

typedef struct _VOLUME_CONTEXT {

    //
    //  Holds the sector size for this volume.
    //

    ULONG SectorSize;

    ULONG DeviceType;

    FLT_FILESYSTEM_TYPE FileSystemType;

    NPAGED_LOOKASIDE_LIST Pre2PostContextList;

    WCHAR 			VolumeNameBuffer[DEVICE_NAME_MAX_LENGTH];
    UNICODE_STRING 	VolumeName;

} VOLUME_CONTEXT, *PVOLUME_CONTEXT;

#define VOLUME_CONTEXT_SIZE         sizeof( VOLUME_CONTEXT )

#define MIN_SECTOR_SIZE 0x200

typedef struct _CRYPT_CONTEXT{
	ULONG  ExternSize;
	ULONG  MetadataSize;

	unsigned char Key[SIFS_MAX_KEY_BYTES];
	unsigned char Root_iv[SIFS_MAX_IV_BYTES];
	
}CRYPT_CONTEXT, *PCRYPT_CONTEXT;

typedef struct _STREAM_CONTEXT {
	
	ULONG  FileSystemType;
	
	ULONG  VolumeDeviceType;

	BOOLEAN  CryptedFile;
       BOOLEAN  MetadataExist;

	PCRYPT_CONTEXT CryptContext;
       
	LARGE_INTEGER	FileSize;
    
	
	//
	//  Lock used to protect this context.
	//
	PERESOURCE Resource;
	
} STREAM_CONTEXT, *PSTREAM_CONTEXT;

#define STREAM_CONTEXT_SIZE         sizeof( STREAM_CONTEXT )

//
//  Stream handle context data structure
//

typedef struct _STREAMHANDLE_CONTEXT {

    //
    //  Name of the file associated with this context.
    //

    ULONG                     TaskState;

    ULONG                     IoRule;

    //
    //  Lock used to protect this context.
    //

    PERESOURCE Resource;

} STREAMHANDLE_CONTEXT, *PSTREAMHANDLE_CONTEXT;

#define STREAMHANDLE_CONTEXT_SIZE         sizeof( STREAMHANDLE_CONTEXT )

//
//  This is a context structure that is used to pass state from our
//  pre-operation callback to our post-operation callback.
//

typedef struct _PRE_2_POST_CONTEXT {

    ULONG 		TaskState;
    ULONG 		IoRule;

	
    BOOLEAN 		CryptedFile;

    PCRYPT_CONTEXT 			CryptContext;
    PSTREAM_CONTEXT                StreamContext;
    PSTREAMHANDLE_CONTEXT     StreamHandleContext;

  
    PVOLUME_CONTEXT 				VolCtx;    
    PFLT_FILE_NAME_INFORMATION 	NameInfo;
    
    PVOID							SwappedBuffer;
	
} PRE_2_POST_CONTEXT, *PPRE_2_POST_CONTEXT;

NTSTATUS
InstanceSetup (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
CleanupContext(
    __in PFLT_CONTEXT Context,
    __in FLT_CONTEXT_TYPE ContextType
    );

NTSTATUS
InstanceQueryTeardown (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    );

NTSTATUS
FilterUnload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
SwapPreCreate(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostCreate(
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
SwapPreCleanup(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );
    
FLT_PREOP_CALLBACK_STATUS
SwapPreClose(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );    
    
FLT_PREOP_CALLBACK_STATUS
SwapPreRead(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostRead(
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
SwapPreWrite(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostWrite(
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
SwapPreQueryInformation (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostQueryInformation (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
SwapPreSetInformation(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostSetInformation (
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
SwapPreNetworkQueryOpen(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostNetworkQueryOpen (
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
SwapPreDirCtrlBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
SwapPostDirCtrlBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

VOID
ReadDriverParameters (
    __in PUNICODE_STRING RegistryPath
    );

#include "debug.h"
#include "sifs.h"
#include "filter.h"
#include "help.h"

extern int module_init(void);
extern void module_exit(void);

#endif /* __FILE_FILTER_H__ */
