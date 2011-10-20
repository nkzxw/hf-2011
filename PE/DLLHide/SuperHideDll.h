#ifndef _SUPERHIDEDLL_H_
#define _SUPERHIDEDLL_H_

#include <ntddk.h>

typedef ULONG DWORD;
typedef LONG BOOL;
typedef unsigned short WORD;
typedef unsigned char BYTE;

#define dprintf if (DBG) DbgPrint
#define nprintf DbgPrint

#define kmalloc(_s)	ExAllocatePoolWithTag(NonPagedPool, _s, 'SYSQ')
#define kfree(_p)	ExFreePool(_p)

//结构声明
typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    union {
        LIST_ENTRY HashLinks;
        struct {
            PVOID SectionPointer;
            ULONG CheckSum;
        };
    };
    union {
        struct {
            ULONG TimeDateStamp;
        };
        struct {
            PVOID LoadedImports;
        };
    };
    struct _ACTIVATION_CONTEXT * EntryPointActivationContext;

    PVOID PatchInformation;

} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _OBJECT_HEADER_CREATOR_INFO {
    LIST_ENTRY TypeList;
    HANDLE CreatorUniqueProcess;
    USHORT CreatorBackTraceIndex;
    USHORT Reserved;
} OBJECT_HEADER_CREATOR_INFO, *POBJECT_HEADER_CREATOR_INFO;

typedef struct _OBJECT_HEADER_NAME_INFO {
    PVOID Directory;
    UNICODE_STRING Name;
    ULONG QueryReferences;
} OBJECT_HEADER_NAME_INFO, *POBJECT_HEADER_NAME_INFO;

typedef struct _OBJECT_DIRECTORY_ENTRY {
    struct _OBJECT_DIRECTORY_ENTRY *ChainLink;
    PVOID Object;
    ULONG HashValue;
} OBJECT_DIRECTORY_ENTRY, *POBJECT_DIRECTORY_ENTRY;
//函数声明
ULONG g_uCr0;

extern POBJECT_TYPE NTSYSAPI PsProcessType;
extern POBJECT_TYPE NTSYSAPI PsThreadType;
_inline void WPOFF()
{
	
    ULONG uAttr;
	
    _asm
    {
        cli
		push eax
        mov eax, cr0
        mov g_uCr0, eax
        and eax, 0FFFEFFFFh // CR0 16 BIT = 0
        mov cr0, eax
        pop eax
        
    };
	
}

_inline void WPON()
{
	
    _asm
    {
        
		push eax
        mov eax, g_uCr0 //恢復原有 CR0 屬性
        mov cr0, eax
        pop eax
		sti
    };
	
}

typedef struct _PEB_LDR_DATA
{
		ULONG Length;
		BOOLEAN Initialized;
		PVOID SsHandle; 
		LIST_ENTRY InLoadOrderModuleList;
		LIST_ENTRY InMemoryOrderModuleList; 
		LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA,*PPEB_LDR_DATA; 

/*
lkd> dt _CONTROL_AREA
nt!_CONTROL_AREA
+0x000 Segment          : Ptr32 _SEGMENT
+0x004 DereferenceList  : _LIST_ENTRY
+0x00c NumberOfSectionReferences : Uint4B
+0x010 NumberOfPfnReferences : Uint4B
+0x014 NumberOfMappedViews : Uint4B
+0x018 NumberOfSubsections : Uint2B
+0x01a FlushInProgressCount : Uint2B
+0x01c NumberOfUserReferences : Uint4B
+0x020 u                : __unnamed
+0x024 FilePointer      : Ptr32 _FILE_OBJECT
+0x028 WaitingForDeletion : Ptr32 _EVENT_COUNTER
+0x02c ModifiedWriteCount : Uint2B
+0x02e NumberOfSystemCacheViews : Uint2B
*/
typedef struct _CONTROL_AREA {
    PVOID Segment; 											//PSEGMENT
    LIST_ENTRY DereferenceList;
    ULONG NumberOfSectionReferences;    // All section refs & image flushes
    ULONG NumberOfPfnReferences;        // valid + transition prototype PTEs
    ULONG NumberOfMappedViews;          // total # mapped views, including
    USHORT NumberOfSubsections;     		// system cache views only
		USHORT FlushInProgressCount;
    ULONG NumberOfUserReferences;       // user section & view references
    ULONG LongFlags;
    PFILE_OBJECT FilePointer;
    PVOID WaitingForDeletion; 					//PEVENT_COUNTER
    USHORT ModifiedWriteCount;
    USHORT NumberOfSystemCacheViews;
} CONTROL_AREA, *PCONTROL_AREA;


typedef struct _MMVAD {
		ULONG_PTR StartingVpn;
		ULONG_PTR EndingVpn;
		struct _MMVAD *Parent;
		struct _MMVAD *LeftChild;
		struct _MMVAD *RightChild;
		ULONG_PTR LongFlags;
		PCONTROL_AREA ControlArea;
		PVOID FirstPrototypePte; 	//PMMPTE
		PVOID LastContiguousPte;	//PMMPTE
		ULONG LongFlags2;
} MMVAD, *PMMVAD;

//一些函数的声明

NTSYSAPI
NTSTATUS
NTAPI
NtDeviceIoControlFile (
			HANDLE FileHandle,
			HANDLE Event,
			PIO_APC_ROUTINE ApcRoutine,
			PVOID ApcContext,
			PIO_STATUS_BLOCK IoStatusBlock,
			ULONG IoControlCode,
			PVOID InputBuffer,
			ULONG InputBufferLength,
			PVOID OutputBuffer,
			ULONG OutputBufferLength
    );

NTSYSAPI
NTSTATUS
NTAPI
NtFsControlFile(
		HANDLE FileHandle,
		HANDLE Event,
		PIO_APC_ROUTINE ApcRoutine,
		PVOID ApcContext,
		PIO_STATUS_BLOCK IoStatusBlock,
		ULONG IoControlCode,
		PVOID InputBuffer,
		ULONG InputBufferLength,
		PVOID OutputBuffer,
		ULONG OutputBufferLength
    );

//一些函数的声明
NTSYSAPI
NTSTATUS
NTAPI
NtWriteFile (
	IN HANDLE FileHandle,
	IN HANDLE Event,
	IN PIO_APC_ROUTINE ApcRoutine,
	IN PVOID ApcContext,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID Buffer,
	IN ULONG Length,
	IN PLARGE_INTEGER ByteOffset,
	IN PULONG Key
    );

NTSYSAPI
NTSTATUS
NTAPI
ObOpenObjectByName (
	POBJECT_ATTRIBUTES ObjectAttributes,
	POBJECT_TYPE ObjectType,
	KPROCESSOR_MODE AccessMode,
	PACCESS_STATE AccessState,
	ACCESS_MASK DesiredAccess,
	PVOID ParseContext,
	PHANDLE Handle
    );
    
NTSYSAPI
UCHAR *
NTAPI
PsGetProcessImageFileName(
		PEPROCESS Process
    );
    
NTKERNELAPI
VOID
KeAttachProcess (
    IN PEPROCESS Process
);

NTKERNELAPI
VOID
KeDetachProcess (
    VOID
);

#endif //_SUPERHIDEDLL_H_