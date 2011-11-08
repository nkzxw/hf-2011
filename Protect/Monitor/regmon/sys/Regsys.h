//======================================================================
// 
// Regmon.h
//
// Copyright (C) 1996-2002 Mark Russinovich and Bryce Cogswell
// Sysinternals - wwww.sysinternals.com
//
// This program is protected by copyright. You may not use
// this code or derivatives in commerical or freeware applications
// without a license. Contact licensing@sysinternals.com for inquiries.
//
// Typedefs and defines.
// 
//======================================================================

#if defined(_M_IA64) 

//
// IA64 SYSTEM CALL HOOK/UNHOOK
//

//
// On the IA64 the Zw function has an embedded immediate that is the system call number
//
#define SYSCALL_INDEX(_Function) (((*(PULONG)((PUCHAR)(*(PULONG_PTR)_Function+4)) & 0x3) << 7) + (*(PULONG)((PUCHAR)*(PULONG_PTR)_Function) >> 18))

#define HOOK_SYSCALL(_Function, _Hook, _Orig )  \
    if( !HookDescriptors[ SYSCALL_INDEX(_Function) ].Hooked ) { \
            ULONG syscallIndex = SYSCALL_INDEX(_Function ); \
            if( !stubsPatched ) PatchStub( gpReg, (PVOID) *(PULONG_PTR *) Stub##_Hook ); \
            HookDescriptors[ syscallIndex ].FuncDesc.EntryPoint = \
                     (ULONGLONG) InterlockedExchangePointer( (PVOID) &KeServiceTablePointers[ syscallIndex ], *(PULONG_PTR *) Stub##_Hook ); \
            HookDescriptors[ syscallIndex ].FuncDesc.GlobalPointer = ((PLABEL_DESCRIPTOR *)&_Function)->GlobalPointer; \
            _Orig = (PVOID) &HookDescriptors[ syscallIndex ].FuncDesc.EntryPoint; \
            HookDescriptors[ syscallIndex ].Hooked = TRUE; \
    }

//
// NOTE: We can't unhook if someone else has hooked on top of us. Note that the
// unhook code below still has a window of vulnerability where someone can hook between
// our test and unhook.
//
#define UNHOOK_SYSCALL(_Function, _Hook, _Orig )  \
    if( HookDescriptors[ SYSCALL_INDEX(_Function)].Hooked && KeServiceTablePointers[ SYSCALL_INDEX(_Function) ] == (PVOID) _Hook ) { \
            InterlockedExchangePointer( (PVOID) &KeServiceTablePointers[ SYSCALL_INDEX(_Function) ], (PVOID) _Orig ); \
            HookDescriptors[ SYSCALL_INDEX(_Function) ].Hooked = FALSE; \
    }

#else

//
// X86 SYSTEM CALL HOOK/UNHOOK
//

//
// Define this because we build with the NT4DDK for 32-bit systems, where
// ULONG_PTR isn't defined and is a ULONG anyway
//
typedef ULONG   ULONG_PTR;

//
// On X86 implementations of Zw* functions, the DWORD
// following the first byte is the system call number, so we reach into the Zw function
// passed as a parameter, and pull the number out. This makes system call hooking
//
#define SYSCALL_INDEX(_Function) *(PULONG)((PUCHAR)_Function+1)

#define HOOK_SYSCALL(_Function, _Hook, _Orig )  \
    if( !HookDescriptors[ SYSCALL_INDEX(_Function) ].Hooked ) { \
            _Orig = (PVOID) InterlockedExchange( (PLONG) &KeServiceTablePointers[ SYSCALL_INDEX(_Function) ], (LONG) _Hook ); \
            HookDescriptors[ SYSCALL_INDEX(_Function) ].Hooked = TRUE; \
    }

//
// NOTE: We can't unhook if someone else has hooked on top of us. Note that the
// unhook code below still has a window of vulnerability where someone can hook between
// our test and unhook.
//
#define UNHOOK_SYSCALL(_Function, _Hook, _Orig )  \
    if( HookDescriptors[ SYSCALL_INDEX(_Function)].Hooked && KeServiceTablePointers[ SYSCALL_INDEX(_Function) ] == (PVOID) _Hook ) { \
            InterlockedExchange( (PLONG) &KeServiceTablePointers[ SYSCALL_INDEX(_Function) ], (LONG) _Orig ); \
            HookDescriptors[ SYSCALL_INDEX(_Function) ].Hooked = FALSE; \
    }
#endif

//
// Number of predefined rootkeys
//
#define NUMROOTKEYS     4

//
// The name of the System process, in which context we're called in our DriverEntry
//
#define SYSNAME         "System"

//
// The maximum length of Registry values that will be copied
//
#define MAXVALLEN      256

//
// Maximum seperate filter components 
//
#define MAXFILTERS     64

//
// The maximum registry path length that will be copied
//
#define MAXPATHLEN     1024

//
// Max len of any error string
//
#define MAXERRORLEN    32

//
// Maximum length of data that will be copied to the "other" field in the display
//
#define MAXDATALEN     64

//
// Length of process name buffer. Process names are at most 16 characters so
// we take into account a trailing NULL.
//
#define MAXPROCNAMELEN  32

//
// Maximum length of NT process name
//
#define NT_PROCNAMELEN  16

//
// Maximum length of root keys that we will produce abbreviations for
//
#define MAXROOTLEN      128

//
// Maximum amount of memory that will be grabbed
//
#define MAXMEM          1000000

//
// Invalid handle
//
#define INVALID_HANDLE_VALUE  ((HANDLE) -1)

//
// Per-user classe key suffix
//
#define USER_CLASSES    "_CLASSES"

//
// System account LUID
//
#define SYSTEMACCOUNT_LOW 999
#define SYSTEMACCOUNT_HIGH 0 


//
// Convenient mutex macros
//
#define MUTEX_INIT(v)      KeInitializeMutex( &v, 0 )
#define MUTEX_ACQUIRE(v)   KeWaitForMutexObject( &v, Executive, KernelMode, FALSE, NULL )
#define MUTEX_RELEASE(v)   KeReleaseMutex( &v, FALSE )

//
// Basic types
//
typedef unsigned int    UINT;
typedef char            CHAR;
typedef char *          PCHAR;
typedef PVOID           POBJECT;

//
// Structure for our name hash table
//
typedef struct _nameentry {
   POBJECT              Object;
   struct _nameentry    *Next;
   CHAR                 FullPathName[];
} HASH_ENTRY, *PHASH_ENTRY;

//
// Structure for keeping linked lists of output buffers
//
typedef struct _log {
    ULONG               Len;
    struct _log *       Next;
    char                Data[ LOGBUFSIZE ];
} LOG_BUF, *PLOG_BUF;

//
// Rootkey name translation structure
//
typedef struct _rootkey {
    CHAR                RootName[256];
    CHAR                RootShort[32];
    ULONG               RootNameLen;
} ROOTKEY, *PROOTKEY;

#if WNET
//
// Pre-to-Post handler tracking structure
//
typedef struct _POST_CONTEXT {
    PVOID               Argument;
    PVOID               Thread;
    PCHAR               FullName;
    LIST_ENTRY          NextContext;
} POST_CONTEXT, *PPOST_CONTEXT;
#endif


//
// Our work item for getting user information
//
typedef struct {
    WORK_QUEUE_ITEM  Item;
    KEVENT           Event;
    LUID             LogonId;
    NTSTATUS         Status;
    PCHAR            Output;
    SecurityUserData UserInformation;
} GETSECINFO_WORK_ITEM, *PGETSECINFO_WORK_ITEM;


//
// Number of hash buckets
//
#define NUMHASH         0x100
#define HASHOBJECT(_regobject)          (ULONG)(((ULONG_PTR)_regobject)>>2)%NUMHASH

//
// Definition for Registry function prototypes not included in NTDDK.H
//
NTSYSAPI
NTSTATUS
NTAPI 
ZwDeleteValueKey( 
    IN HANDLE, 
    IN PUNICODE_STRING 
    );
NTSYSAPI
NTSTATUS
NTAPI 
ZwLoadKey( 
    IN POBJECT_ATTRIBUTES, 
    IN POBJECT_ATTRIBUTES 
    );
NTSYSAPI
NTSTATUS
NTAPI 
ZwUnloadKey( 
    IN POBJECT_ATTRIBUTES 
    );

//
// Definition for ObQueryNameString call
//

NTSTATUS
NTAPI 
ObQueryNameString( 
    POBJECT Object, 
    PUNICODE_STRING Name, 
    ULONG MaximumLength, 
    PULONG ActualLength 
    );

NTSYSAPI
NTSTATUS
NTAPI 
ObOpenObjectByPointer( 
    POBJECT Object, 
    ULONG HandleAttributes, 
    PACCESS_STATE PassedAccessState, 
    ACCESS_MASK DesiredAccess, 
    POBJECT_TYPE ObjectType, 
    KPROCESSOR_MODE AccessMode, 
    HANDLE *Handle 
    );


//
// For displaying messages to the Blue Screen
//
NTSYSAPI
NTSTATUS
NTAPI 
ZwDisplayString( 
    PUNICODE_STRING Text 
    );

//
// Definition for a call we get passed the index for from user space
//
NTSTATUS 
NTAPI
ZwQueryInformationToken (
    HANDLE TokenHandle,
    TOKEN_INFORMATION_CLASS TokenInformationClass,
    PVOID TokenInformation,
    ULONG TokenInformationLength,
    PULONG ReturnLength 
    );

#if !defined(_M_IA64) && !defined(WNET)


//
// Undocumented ntoskrnl function for checking user buffer validity
//
VOID 
NTAPI 
ProbeForWrite(
    PVOID Address, 
    ULONG Length, 
    ULONG Alignment 
    );

//
// This are Win2K definitions that are included only in the Win2K
// version of NTDDK.H. So that it works on NT 4, Regmon is built with
// the NT 4 DDK and we have to include these definitions ourselves
//
enum {
    KeyNameInformation = 3,
    KeyCachedInformation,
    KeyFlagsInformation
};

enum {
    KeyUserFlagsInformation = 1
};

typedef struct _KEY_USER_FLAGS_INFORMATION {
    ULONG   UserFlags;
} KEY_USER_FLAGS_INFORMATION, *PKEY_USER_FLAGS_INFORMATION;

typedef struct _KEY_NAME_INFORMATION {
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable length string
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

typedef struct _KEY_CACHED_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG   TitleIndex;
    ULONG   SubKeys;
    ULONG   MaxNameLen;
    ULONG   Values;
    ULONG   MaxValueNameLen;
    ULONG   MaxValueDataLen;
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable length string
} KEY_CACHED_INFORMATION, *PKEY_CACHED_INFORMATION;

typedef struct _KEY_FLAGS_INFORMATION {
    ULONG   UserFlags;
} KEY_FLAGS_INFORMATION, *PKEY_FLAGS_INFORMATION;

#endif

//
// Extract transfer type
//
#define IOCTL_TRANSFER_TYPE( _iocontrol)   (_iocontrol & 0x3)
