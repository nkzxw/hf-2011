//======================================================================
//
// REGMON.h - include file for VxD REGMON
//
// Copyright (C) 1996 Mark Russinovich and Bryce Cogswell
//
//======================================================================
#include <vtoolsc.h>

//----------------------------------------------------------------------
//                           D E F I N E S 
//----------------------------------------------------------------------
#define REGMON_Major		1
#define REGMON_Minor		0
#define REGMON_DeviceID		UNDEFINED_DEVICE_ID
#define REGMON_Init_Order	UNDEFINED_INIT_ORDER

//
// Number of hash buckets
//
#define NUMHASH		        0x100
#define HASHOBJECT(_hkey)	(((ULONG)_hkey)>>2)%NUMHASH

//
// Size of storage buffer in pages
//
#define LOGBUFPAGES     (LOGBUFSIZE/0x1000+1)

//
// Maximum path length
//
#define MAXPATHLEN      1024

//
// Sizes of various buffers
//
#define STRINGLEN      240
#define DATASIZE       512
#define NAMELEN        256
#define PROCESSLEN      16
#define BINARYLEN        8

//
// Maximum seperate filter components 
//
#define MAXFILTERS      64

//
// Entries in VMM Win32 service table
//
#define VMMWIN32QUERYINFOKEY    (0x1E * 2 )

//----------------------------------------------------------------------
//                        S T R U C T U R E S 
//----------------------------------------------------------------------

//
// Structure for our name hash table
//
typedef struct _nameentry {

   HKEY                 hkey;
   struct _nameentry 	*Next;
   CHAR		            FullName[];
} HASH_ENTRY, *PHASH_ENTRY;

//
// Structure for keeping linked lists of output buffers.
// Note: if fields are added update the definition for 
// LOGBUFSIZE in gui\ioctlcmd.h
//
typedef struct _log {
    MEMHANDLE           Handle;
    ULONG		        Len;
    struct _log        *Next;
    char		        Data[ LOGBUFSIZE ];
} LOG_BUF, *PLOG_BUF;

 



