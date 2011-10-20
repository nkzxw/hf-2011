#ifndef _COMM_H_
#define _COMM_H_

#if defined(_WINDOWS) || defined (_CONSOLE)
#include <WinIoctl.h>   // Compiling Win32 Applications Or DLL's

#include "W32NDIS.H"
#endif // _WINDOWS


//
// Note
// ----
// This file contains constants and definitions that are shared between
// the kernel mode driver and Win32 components.
//
/////////////////////////////////////////////////////////////////////////////
//// NWIP Win32 API Version Information
//
// Make sure that this is coordinated with information in the VERSION
// resource.
//
#define NETWALL_API_VERSION             (ULONG)0x01000100

////////////////////////////////////////////////////////////////////////////
//// Device Naming String Definitions
//
//
// Driver WDM Device Object Name
// -----------------------------
// This is the name of the NWIM driver WDM device object.
//
#define NETWALL_WDM_DEVICE_NAME_W  L"\\Device\\NetWall"
#define NETWALL_WDM_DEVICE_NAME_A   "\\Device\\NetWall"

#ifdef _UNICODE
#define NETWALL_WDM_DEVICE_NAME NETWALL_WDM_DEVICE_NAME_W
#else
#define NETWALL_WDM_DEVICE_NAME NETWALL_WDM_DEVICE_NAME_A
#endif

//
// Driver Device WDM Symbolic Link
// -------------------------------
// This is the name of the NWIM driver device WDM symbolic link. This
// is a user-visible name that can be used by Win32 applications to access
// the NWIM driver WDM interface.
//
#define NETWALL_WDM_SYMBOLIC_LINK_W  L"\\DosDevices\\NetWall"
#define NETWALL_WDM_SYMBOLIC_LINK_A   "\\DosDevices\\NetWall"

#ifdef _UNICODE
#define NETWALL_WDM_SYMBOLIC_LINK  NETWALL_WDM_SYMBOLIC_LINK_W
#else
#define NETWALL_WDM_SYMBOLIC_LINK  NETWALL_WDM_SYMBOLIC_LINK_A
#endif

//
// Driver WDM Device Filename
// --------------------------
// This is the name that Win32 applications pass to CreateFile to open
// the NETWALL WDM symbolic link.
//
#define NETWALL_WDM_DEVICE_FILENAME_W  L"\\\\.\\NetWall"
#define NETWALL_WDM_DEVICE_FILENAME_A   "\\\\.\\NetWall"

#ifdef _UNICODE
#define NETWALL_WDM_DEVICE_FILENAME NETWALL_WDM_DEVICE_FILENAME_W
#else
#define NETWALL_WDM_DEVICE_FILENAME NETWALL_WDM_DEVICE_FILENAME_A
#endif


/////////////////////////////////////////////////////////////////////////////
//// IOCTL Code Definitions
//

/////////////////////////////////////////////////////////////////////////////
//// IOCTL Code Definitions
//

#define IOCTL_NETWALL_GET_API_VERSION           CTL_CODE(FILE_DEVICE_NETWORK, 0x000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NETWALL_GET_DRIVER_DESCRIPTION    CTL_CODE(FILE_DEVICE_NETWORK, 0x001, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NETWALL_ENUM_ADAPTERS             CTL_CODE(FILE_DEVICE_NETWORK, 0x002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NETWALL_OPEN_VIRTUAL_ADAPTER      CTL_CODE(FILE_DEVICE_NETWORK, 0x003, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NETWALL_OPEN_LOWER_ADAPTER        CTL_CODE(FILE_DEVICE_NETWORK, 0x004, METHOD_BUFFERED, FILE_ANY_ACCESS)

// IOCTL Code Definitions For NetWall Filter
#define NETWALL_FILTER_BASE                     (FILE_DEVICE_NETWORK + 0x01F)

#define IOCTL_NETWALL_SET_FILTER                CTL_CODE(NETWALL_FILTER_BASE, NETWALL_FILTER_BASE+0x1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NETWALL_CLEAR_FILTER              CTL_CODE(NETWALL_FILTER_BASE, NETWALL_FILTER_BASE+0x2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NETWALL_OPEN_LOGPRINT             CTL_CODE(NETWALL_FILTER_BASE, NETWALL_FILTER_BASE+0x3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_NETWALL_CLOSE_LOGPRINT            CTL_CODE(NETWALL_FILTER_BASE, NETWALL_FILTER_BASE+0x4, METHOD_BUFFERED, FILE_ANY_ACCESS)


/////////////////////////////////////////////////////////////////////////////
//// Structure Definitions
//
// Specify Structure Packing
//
#pragma pack(push,1)

/////////////////////////////////////////////////////////////////////////////
//// IM_OPEN_ADAPTER Structure Definition
//
// Used to pass wide character adapter name down to driver and return
// NDIS_STATUS of the completed operation back to the Win32 application.
//
typedef
struct _IM_OPEN_ADAPTER
{
    NDIS_STATUS     m_nOpenStatus;   // Open Completion Status
  /*NDIS_STRING     m_AdapterName;*/ // Wide Character Adapter Name Follows m_nOpenStatus...
} IM_OPEN_ADAPTER, *PIM_OPEN_ADAPTER;


/////////////////////////////////////////////////////////////////////////////
//// IM_FILTER Structure Definition

#define NW_MAX_PROTOCOL			    5
#define NW_MAX_ACTION				3
#define NW_MAX_DIRECTION			3

#define NETWALL_ACTION_PASS         0x0000
#define NETWALL_ACTION_DROP         0x0001
#define NETWALL_ACTION_LOG          0x0002

#define NETWALL_DIRECTION_IN        0x0001
#define NETWALL_DIRECTION_OUT       0x0002
#define NETWALL_DIRECTION_BOTH      0x0004

//
// Rule item.
//
typedef struct _RULE_ITEM 
{
    UINT	cbSize;
    UINT	bUse;
    UINT	iProto;
    
    ULONG	ulSrcStartIp;
    ULONG	ulSrcEndIp;
    USHORT	usSrcStartPort;
    USHORT	usSrcEndPort;
    
    UCHAR	ucDirection;
    
    ULONG	ulDestStartIp;
    ULONG	ulDestEndIp;
    USHORT	usDestStartPort;
    USHORT	usDestEndPort;
    
    UCHAR	ucAction;
    char	chMsg[1];
    
} RULE_ITEM, * PRULE_ITEM;

//
// Restore Default Structure Packing
//
#pragma pack(pop)


#endif /* _COMM_H_ */
