//======================================================================
// 
// Ioctlcmd.h
//
// Copyright (C) 1996, 1997 Mark Russinovich and Bryce Cogswell
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
#define FILE_DEVICE_REGMON      0x00008305

//
// Version #
//
#define REGMON_VERSION          430

//
// Commands that the GUI can send the device driver
// 
#define IOCTL_REGMON_HOOK      (ULONG) CTL_CODE( FILE_DEVICE_REGMON, 0x00, METHOD_BUFFERED, FILE_WRITE_ACCESS )
#define IOCTL_REGMON_UNHOOK    (ULONG) CTL_CODE( FILE_DEVICE_REGMON, 0x01, METHOD_BUFFERED, FILE_WRITE_ACCESS )
#define IOCTL_REGMON_ZEROSTATS (ULONG) CTL_CODE( FILE_DEVICE_REGMON, 0x02, METHOD_BUFFERED, FILE_WRITE_ACCESS )
#define IOCTL_REGMON_GETSTATS  (ULONG) CTL_CODE( FILE_DEVICE_REGMON, 0x03, METHOD_NEITHER, FILE_WRITE_ACCESS )
#define IOCTL_REGMON_SETFILTER (ULONG) CTL_CODE( FILE_DEVICE_REGMON, 0x04, METHOD_BUFFERED, FILE_WRITE_ACCESS )
#define IOCTL_REGMON_VERSION   (ULONG) CTL_CODE( FILE_DEVICE_REGMON, 0x05, METHOD_BUFFERED, FILE_WRITE_ACCESS )

//
// An allocation unit size. We size this so that
// a log buffer is a multiple of the page size
//
#define LOGBUFSIZE       ((ULONG)(64*0x400-(3*sizeof(ULONG)+1)))

#pragma pack(1)
//
// Log record entry
//
typedef struct {
	ULONG	        seq;
    LARGE_INTEGER   time;
    LARGE_INTEGER	perftime;
	char	        text[0];
} ENTRY, *PENTRY;
#pragma pack()

//
// Length of a filter definition string
//
#define MAXFILTERLEN   128

//
// Filter definition
//
typedef struct {
        char     includefilter[MAXFILTERLEN];
        char     excludefilter[MAXFILTERLEN];
        BOOLEAN  logsuccess;
        BOOLEAN  logerror;
        BOOLEAN  logreads;
        BOOLEAN  logwrites;
		BOOLEAN  logaux;	// Auxilliary operations, open, close, flush
} FILTER, *PFILTER;


