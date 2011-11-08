//======================================================================
// 
// Ioctlcmd.h
//
// Copyright (C) 1996 Mark Russinovich and Bryce Cogswell
//
// Common header file for device driver and GUI. Contains common
// data structure definitions and IOCTL command codes.
//
//======================================================================

//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//
#define FILE_DEVICE_FILEMON	0x00008300

//
// Version #
//
#define FILEMONVERSION    430

//
// commands that the GUI can send the device driver
// 
#define IOCTL_FILEMON_SETDRIVES   (ULONG) CTL_CODE( FILE_DEVICE_FILEMON, 0x00, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FILEMON_ZEROSTATS   (ULONG) CTL_CODE( FILE_DEVICE_FILEMON, 0x01, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FILEMON_GETSTATS    (ULONG) CTL_CODE( FILE_DEVICE_FILEMON, 0x02, METHOD_NEITHER, FILE_ANY_ACCESS )
#define IOCTL_FILEMON_UNLOADQUERY (ULONG) CTL_CODE( FILE_DEVICE_FILEMON, 0x03, METHOD_NEITHER, FILE_ANY_ACCESS )
#define IOCTL_FILEMON_STOPFILTER  (ULONG) CTL_CODE( FILE_DEVICE_FILEMON, 0x04, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FILEMON_STARTFILTER (ULONG) CTL_CODE( FILE_DEVICE_FILEMON, 0x05, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FILEMON_SETFILTER   (ULONG) CTL_CODE( FILE_DEVICE_FILEMON, 0x06, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FILEMON_VERSION     (ULONG) CTL_CODE( FILE_DEVICE_FILEMON, 0x07, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FILEMON_HOOKSPECIAL (ULONG) CTL_CODE( FILE_DEVICE_FILEMON, 0x08, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define IOCTL_FILEMON_UNHOOKSPECIAL (ULONG) CTL_CODE( FILE_DEVICE_FILEMON, 0x09, METHOD_BUFFERED, FILE_ANY_ACCESS ) 

//
// format of a data entry
//
#pragma pack(1)
typedef struct {
	ULONG	          seq;
    LARGE_INTEGER     perftime;
    LARGE_INTEGER     datetime;
	char	          text[0];
} ENTRY, *PENTRY;
#pragma pack()

//
// Length of a filter definition string
//
#define MAXFILTERLEN	128

//
// Filter definition
//
typedef struct {
    char     includefilter[MAXFILTERLEN];
    char     excludefilter[MAXFILTERLEN];
    BOOLEAN  logreads;
    BOOLEAN  logwrites;
} FILTER, *PFILTER;


//
// Our different file system types
//
typedef enum {
    GUIINTERFACE,
    STANDARD,
    NPFS,
    MSFS
} FILE_SYSTEM_TYPE, *PFILE_SYSTEM_TYPE;


//
// Define page size for use by GUI
//

#ifndef PAGE_SIZE
#if defined(_ALPHA_)
#define PAGE_SIZE 0x2000  // 8K
#else
#define PAGE_SIZE 0x1000  // 4K
#endif
#endif

//
// Size of a storage buffer in bytes - keep it just below 64K so that
// the total length of the buffer including the header is just below
// a page boundary, thus not wasting much space since buffer allocation
// is page-granular.
//
#define LOGBUFSIZE       ((ULONG)(64*0x400-(3*sizeof(ULONG)+1)))


