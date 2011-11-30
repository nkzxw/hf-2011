#ifndef _DRIVER_H
#define _DRIVER_H

#define IOCTL_SET_FILTERSET	CTL_CODE (FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GET_PROCESSLIST	CTL_CODE (FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SET_SCANNER_PATH	CTL_CODE (FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SET_SCAN_FILTERS	CTL_CODE (FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_KILL_PROCESS	CTL_CODE (FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_LINK_APP2DRV	CTL_CODE (FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_LINK_DRV2APP	CTL_CODE (FILE_DEVICE_UNKNOWN, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IGNORE_PROCESS	CTL_CODE (FILE_DEVICE_UNKNOWN, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SYNC_CACHE	CTL_CODE (FILE_DEVICE_UNKNOWN, 0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ADD_FILE_TO_CACHE	CTL_CODE (FILE_DEVICE_UNKNOWN, 0x80A, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
  DWORD		dwCode ;
} SDNMHDR ;

typedef struct {
  SDNMHDR	hdr ;
  INT_PTR	nProcessAddress ;
  UINT		nDefReaction ;
  BYTE		data[0] ;
} SDNASK ;

typedef struct {
  SDNMHDR	hdr ;
  INT_PTR	nProcessAddress ;
  DWORD		dwReaction ;
  BYTE		data[0] ;
} SDNLOG, SDNALERT ;

typedef struct {
  SDNMHDR	hdr ;
  INT_PTR	nProcessAddress ;
  DWORD		nProcessId ;
  WCHAR		wszFilePath[0] ;
} SDNPROCESSCREATED ;

typedef struct {
  SDNMHDR	hdr ;
  INT_PTR	nProcessAddress ;
} SDNPROCESSTERMINATED ;

typedef struct {
  SDNMHDR	hdr ;
  INT_PTR	nProcessAddress ;
  LARGE_INTEGER	liFileTime ;
  WCHAR		wszFilePath[MAX_PATH] ;
} SDNSCANFILE ;

typedef struct {
  SDNMHDR	hdr ;
  INT_PTR	nProcessAddress ;
  DWORD		nNewProcessId ;
} SDNPIDCHANGED ;

#define SDN_LOG			1
#define SDN_ALERT		2
#define SDN_ASK			3
#define SDN_PROCESSCREATED	4
#define SDN_PROCESSTERMINATED	5
#define SDN_SCANFILE		6
#define SDN_PIDCHANGED		7


typedef struct {
  DWORD		nNextEntry ;
  INT_PTR	nProcessAddress ;
  DWORD		nProcessId ;
  WCHAR		wszFilePath[0] ;
} PROCESSLISTENTRY ;

typedef struct
{
  INT_PTR	nProcessAddress ;
  BOOL		bIgnore ;
} SDCIGNOREPROC ;

typedef struct 
{
  LARGE_INTEGER	liLastSyncTime ;
} SDCSYNCCACHE ;

typedef struct 
{
  DWORD		nNextEntry ;
  ULONG		nIdentifier ;
  DWORD		nScanResult ;
  LARGE_INTEGER	liScanTime ;
  WCHAR		wszFilePath[0] ;
} SCANCACHEENTRY ;

typedef struct 
{
  UINT		nMaxCacheLength ;
  ULONG		nFirstIdentifier ;
  ULONG		nLastIdentifier ;
} SCANCACHEHEADER ;

typedef struct
{
  DWORD		nScanResult ;
  LARGE_INTEGER liScanTime ;
  WCHAR		wszFilePath[0] ;
} SDCADDFILETOCACHE ;

#endif
