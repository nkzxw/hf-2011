//======================================================================
//
// FILEMON.h - include file for VxD FILEMON
//
// Copyright (C) 1996-1999 Mark Russinovich and Bryce Cogswell
//
//======================================================================

//----------------------------------------------------------------------
//                           D E F I N E S 
//----------------------------------------------------------------------
#define FILEMON_Major		1
#define FILEMON_Minor		0
#define FILEMON_DeviceID	UNDEFINED_DEVICE_ID
#define FILEMON_Init_Order	UNDEFINED_INIT_ORDER

// number of hash buckets
#define NUMHASH		0x100
#define HASHOBJECT(_filenumber)	(((ULONG)_filenumber)>>2)%NUMHASH

//
// Maximum seperate filter components 
//
#define MAXFILTERS      64

//
// Size of storage buffer in pages
//
#define LOGBUFPAGES    (LOGBUFSIZE/0x1000+1)

//
// Maximum path length
//
#define MAXPATHLEN      260


#define TIME_DIFF()  \
                VTD_Get_Real_Time( &timehi1, &timelo1 ); \
                timelo = timelo1 - timelo  

//
// This designates the operation for which
// pathname conversion takes place
//
typedef enum {
    CONVERT_STANDARD,
    CONVERT_RENAME_SOURCE,
    CONVERT_RENAME_TARGET,
    CONVERT_FINDOPEN,
} CONVERT_TYPE;

//----------------------------------------------------------------------
//                        S T R U C T U R E S 
//----------------------------------------------------------------------

//
// Structure for our name hash table
//
typedef struct _nameentry {
   fh_t                 filenumber;
   int                  drive;
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
    struct _log         *Next;
    char		        Data[ LOGBUFSIZE ];
} LOG_BUF, *PLOG_BUF;

//
// Partial undocumented IFSREQ structure
//
typedef struct {
    // embedded ioreq stucture
    ioreq            ifsir;
    // the structure isn't really defined this way, but
    // we take advantage of the layout for our purposes
    struct hndlfunc  *ifs_hndl;
    ULONG            reserved[10];
} ifsreq, *pifsreq;

 



