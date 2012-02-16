#ifndef __FILEFLT_FILTER_H__
#define __FILEFLT_FILTER_H__

#define VOLUME_DEVICE_NAME_SAFEDISKV  		 L"\\Device\\SafeDiskVVolume"
#define VOLUME_DEVICE_NAME_HGFS 		 	 L"\\Device\\HGFS"
#define VOLUME_DEVICE_NAME_LANMAN            L"\\Device\\LanmanRedirector"

#define FILE_DEVICE_REMOVE_FILE_SYSTEM   (FILE_DEVICE_PMI + 0x32)

#define CRYPT_MD5_LEN 		  16
#define CRYPT_MD5_CHAR_LEN  (CRYPT_MD5_LEN * 2)

typedef enum _FLT_TASK_SLOT_INDEX {

	FLT_TASK_SLOT_UK,
	FLT_TASK_SLOT_SI,
	FLT_TASK_SLOT_UE,
	FLT_TASK_SLOT_BC,
	FLT_TASK_SLOT_XS,
	FLT_TASK_SLOT_VC,
	FLT_TASK_SLOT_MAX	
	
}FLT_TASK_SLOT_INDEX, *PFLT_TASK_SLOT_INDEX;

#define FLT_TASK_SLOT_NUMBER (FLT_TASK_SLOT_MAX - 1)

typedef enum _FLT_VOLUME_TYPE {
	FLT_VLTYPE_PLAN,
	FLT_VLTYPE_CRYTP,
	FLT_VLTYPE_SYSTEM,
	FLT_VLTYPE_HGFS,
	FLT_VLTYPE_PASSTHRU
} FLT_VOLUME_TYPE, *PFLT_VOLUME_TYPE;

typedef enum _FLT_IO_RULE {
	FLT_IORULE_UNKNOWN,	//0x00
	FLT_IORULE_DISABLE,	//0x01
	FLT_IORULE_BROWSE,	//0x02
	FLT_IORULE_ONLYRD,	//0x03
	FLT_IORULE_ONLYWT,	//0x04
	FLT_IORULE_RDWT,		//0x05
} FLT_IO_RULE, *PFLT_IO_RULE;

typedef enum _FLT_TASK_STATE {
	FLT_TASK_STATE_UNKNOWN,				//0x00
	FLT_TASK_STATE_UNTRUST_UNHOOK,		//0x01
	FLT_TASK_STATE_TRUST_UNHOOK,		//0x02
	FLT_TASK_STATE_UNTRUST_HOOK,		//0x03
	FLT_TASK_STATE_TRUST_HOOK,			//0x04
	FLT_TASK_STATE_EXPLORE_UNHOOK,		//0x05
	FLT_TASK_STATE_EXPLORE_HOOK,		//0x06
	FLT_TASK_STATE_SYSTEM,				//0x07
	FLT_TASK_STATE_SHARE,					//0x08
	FLT_TASK_STATE_SERVICE,				//0x09
	FLT_TASK_STATE_PASSTHRU,				//0x0A
	FLT_TASK_STATE_NUMBER				//0x0B
}FLT_TASK_STATE, *PFLT_TASK_STATE;

typedef enum _FLT_VOLUME_DEVICE_TYPE {
	FLT_VOLUME_DEVICE_TYPE_DISK,
	FLT_VOLUME_DEVICE_TYPE_REMOVEABLE,
	FLT_VOLUME_DEVICE_TYPE_CDROM,
	FLT_VOLUME_DEVICE_TYPE_REMOTE,
	FLT_VOLUME_DEVICE_TYPE_SHARE,
	FLT_VOLUME_DEVICE_TYPE_NUMBER,
#define FLT_VOLUME_DEVICE_TYPE_PASSTHRU  FLT_VOLUME_DEVICE_TYPE_NUMBER
	FLT_VOLUME_DEVICE_TYPE_MAX
}FLT_VOLUME_DEVICE_TYPE, *PFLT_VOLUME_DEVICE_TYPE;	

typedef enum _FLT_MY_TASK_TYPE {
	FLT_MY_TASK_RMSERVER,
	FLT_MY_TASK_ASSISTANCE,
	FLT_MY_TASK_NUMBER
}FLT_MY_TASK_TYPE, *PFLT_MY_TASK_TYPE;

extern PKTHREAD g_RestartHostScanThreadHandle;

NTSTATUS
FltInstanceSetup(
   __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType,
    __inout PVOLUME_CONTEXT VolumeContext
    );

VOID
FltCleanupContext(
    __in PFLT_CONTEXT Context,
    __in FLT_CONTEXT_TYPE ContextType
    );

FLT_PREOP_CALLBACK_STATUS
FltPreCreate(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext,
    __in PVOLUME_CONTEXT VolumeContext
    );

FLT_POSTOP_CALLBACK_STATUS
FltPostCreate(
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FltPreCleanup(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
FltPreClose(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
FltPreRead(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext,
    __in PVOLUME_CONTEXT VolumeContext
    );

FLT_POSTOP_CALLBACK_STATUS
FltPostRead(
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_POSTOP_CALLBACK_STATUS
FltPostReadWhenSafe (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FltPreWrite(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext,
    __in PVOLUME_CONTEXT VolumeContext
    );

FLT_POSTOP_CALLBACK_STATUS
FltPostWrite(
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FltPreQueryInformation (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
FltPostQueryInformation (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FltPreSetInformation(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
FltPostSetInformation (    
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FltPreNetworkQueryOpen(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
FltPostNetworkQueryOpen (    
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FltPreDirCtrlBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext,
    __in PVOLUME_CONTEXT VolumeContext
    );

FLT_POSTOP_CALLBACK_STATUS
FltPostDirCtrlBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_POSTOP_CALLBACK_STATUS
FltPostDirCtrlBuffersWhenSafe (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );


int
TaskInit(
	VOID
	);

VOID
TaskExit(
	VOID
	);

FLT_TASK_SLOT_INDEX
TaskGetSlotIndex(
	__in HANDLE TaskId
	);

VOID
TaskGetState(
	__inout HANDLE Pid,
	__inout PFLT_TASK_STATE TaskState
	);


NTSTATUS
CtxFindOrCreateStreamContext (
    __in PFLT_CALLBACK_DATA Cbd,
    __in BOOLEAN CreateIfNotFound,
    __deref_out PSTREAM_CONTEXT *StreamContext,
    __out_opt PBOOLEAN ContextCreated
    );

NTSTATUS
CtxCreateStreamContext (
    __deref_out PSTREAM_CONTEXT *StreamContext
    );

VOID
CtxUpdateAttributeInStreamContext(
	__inout PSTREAM_CONTEXT StreamContext, 
	__in PVOLUME_CONTEXT VolumeContext
	);

NTSTATUS
CtxFindOrCreateStreamHandleContext (
    __in PFLT_CALLBACK_DATA Cbd,
    __in BOOLEAN CreateIfNotFound,
    __deref_out PSTREAMHANDLE_CONTEXT *StreamHandleContext,
    __out_opt PBOOLEAN ContextCreated
    );

NTSTATUS
CtxCreateOrReplaceStreamHandleContext (
    __in PFLT_CALLBACK_DATA Cbd,
    __in BOOLEAN ReplaceIfExists,
    __deref_out PSTREAMHANDLE_CONTEXT *StreamHandleContext,
    __out_opt PBOOLEAN ContextReplaced
    );

NTSTATUS
CtxCreateStreamHandleContext (
    __deref_out PSTREAMHANDLE_CONTEXT *StreamHandleContext
    );

#endif /* __FILEFLT_FILTER_H__ */
