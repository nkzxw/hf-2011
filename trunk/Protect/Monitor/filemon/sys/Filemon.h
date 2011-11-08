//======================================================================
// 
// Filemon.h
//
// Sysinternals - www.sysinternals.com
// Copyright (C) 1996-2001 Mark Russinovich and Bryce Cogswell
//
//======================================================================
#include "wintypes.h"

//
// Print macro that only turns on when debugging is on
//
#if DBG
#define DbgPrint(arg)  DbgPrint( "%x: ", KeGetCurrentThread()); \
                       DbgPrint arg
#else
#define DbgPrint(arg) 
#endif

//
// The name of the System process, in which context we're called in our 
// DriverEntry
//
#define SYSNAME    "System"

//
// Final Build numbers
//
#define NT4FINAL        1381
#define WIN2KFINAL      2195

//
// Maximum amount of memory grabbed 
//
#define MAXMEMORY       1000000

//
// Maximum path length of pathname. This is larger than Win32 maxpath
// because network drives have leading paths
//
#define MAXPATHLEN      1024

//
// Length of process name (rounded up to next DWORD)
//
#define PROCNAMELEN     32

//
// Max length of NT process name
//
#define NT_PROCNAMELEN  16

//
// Maximum seperate filter components 
//
#define MAXFILTERS      64

//
// Length of buffer for error string
//
#define ERRORLEN        64

//
// Documented in the IFS Kit
//
typedef struct _FILE_FS_ATTRIBUTE_INFORMATION {
    ULONG FileSystemAttributes;
    LONG MaximumComponentNameLength;
    ULONG FileSystemNameLength;
    WCHAR FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;


//
// Structure for device specific data that keeps track of what
// drive and what filesystem device are hooked 
//
typedef struct {
    FILE_SYSTEM_TYPE Type;
    PDEVICE_OBJECT   FileSystem;
    unsigned         LogicalDrive;
    BOOLEAN          Hooked;
    PFILE_FS_ATTRIBUTE_INFORMATION FsAttributes;
} HOOK_EXTENSION, *PHOOK_EXTENSION;         


//
// Structure for the fileobject/name hash table
//
typedef struct _nameentry {
   PFILE_OBJECT		FileObject;
   struct _nameentry 	*Next;
   CHAR		FullPathName[];
} HASH_ENTRY, *PHASH_ENTRY;

//
// Structure for a completion routine work item
//
typedef struct _filemonwork {
    WORK_QUEUE_ITEM WorkItem;
    ULONG          Sequence;
    LARGE_INTEGER  TimeResult;
    CHAR           ErrString[ERRORLEN];
} FILEMON_WORK, *PFILEMON_WORK;


//
// Number of hash buckets in the hash table
//
#define NUMHASH		0x100

//
// Hash function. Basically chops the low few bits of the file object
//
#if defined(_IA64_) 
#define HASHOBJECT(_fileobject)		(((ULONG_PTR)_fileobject)>>5)%NUMHASH
#else
#define HASHOBJECT(_fileobject)		(((ULONG)_fileobject)>>5)%NUMHASH
#endif


//
// Structure for keeping linked lists of output buffers
//
typedef struct _log {
    ULONG           Len;
    struct _log   * Next;
    CHAR            Data[ LOGBUFSIZE ];
} LOG_BUF, *PLOG_BUF;


//
// A check to see if a fastio table extends to a specific entry
//
#if defined(_IA64_) 
#define FASTIOPRESENT( _hookExt, _call )                                                      \
    (_hookExt->Type != GUIINTERFACE &&                                                        \
     _hookExt->FileSystem->DriverObject->FastIoDispatch &&                                    \
     (((ULONG_PTR)&_hookExt->FileSystem->DriverObject->FastIoDispatch->_call -                    \
       (ULONG_PTR) &_hookExt->FileSystem->DriverObject->FastIoDispatch->SizeOfFastIoDispatch <    \
       (ULONG_PTR) _hookExt->FileSystem->DriverObject->FastIoDispatch->SizeOfFastIoDispatch )) && \
      _hookExt->FileSystem->DriverObject->FastIoDispatch->_call )
#else
#define FASTIOPRESENT( _hookExt, _call )                                                      \
    (_hookExt->Type != GUIINTERFACE &&                                                        \
     _hookExt->FileSystem->DriverObject->FastIoDispatch &&                                    \
     (((ULONG)&_hookExt->FileSystem->DriverObject->FastIoDispatch->_call -                    \
       (ULONG) &_hookExt->FileSystem->DriverObject->FastIoDispatch->SizeOfFastIoDispatch <    \
       (ULONG) _hookExt->FileSystem->DriverObject->FastIoDispatch->SizeOfFastIoDispatch )) && \
      _hookExt->FileSystem->DriverObject->FastIoDispatch->_call )
#endif

//
// Time stamp start macro
//
#define TIMESTAMPSTART()                                  \
        timeStampStart = KeQueryPerformanceCounter(NULL); \
        KeQuerySystemTime( &dateTime )

#define TIMESTAMPSTOP()                                      \
        timeStampComplete = KeQueryPerformanceCounter(NULL); \
        timeResult.QuadPart = timeStampComplete.QuadPart - timeStampStart.QuadPart; 

//
// Macro for getting the path name
//
#define GETPATHNAME(_IsCreate)                                                  \
        fullPathName = ExAllocateFromNPagedLookasideList( &FullPathLookaside ); \
        if( fullPathName ) {                                                    \
            FilemonGetFullPath( _IsCreate, FileObject, hookExt, fullPathName ); \
        } else {                                                                \
            fullPathName = InsufficientResources;                               \
        }                                                                                    

#define FREEPATHNAME()                                   \
        if ( fullPathName != InsufficientResources ) ExFreeToNPagedLookasideList( &FullPathLookaside, fullPathName )

