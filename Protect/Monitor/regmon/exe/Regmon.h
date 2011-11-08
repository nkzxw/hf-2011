/******************************************************************************
*
*       Regmon - Registry Monitor for Windows NT and Windows 9x
*		
*		Copyright (c) 1996-1999 Mark Russinovich and Bryce Cogswell
*
*		See readme.txt for terms and conditions.
*
*    	PROGRAM: Regmon.h
*
*    	PURPOSE: Typedefs and definitions.
*
******************************************************************************/

// delay for listview subitem tooltip
#define BALLOONDELAY		10

// toolbar height plus the borders
#define TOOLBARHEIGHT		28

// Number of buttons in the toolbar bitmap
#define NUMTOOLBUTTONS		13

// Number of columns in the listview
#define NUMCOLUMNS			7

// Application name
#define APPNAME				_T("Regmon")

// Variables/definitions for the driver that performs the actual monitoring.
#define	SYS_FILE			_T("REGSYS.SYS")
#define SYS_FILE_WNET		_T("REGSWNET.SYS")
#define	SYS_NAME			_T("REGMON")
#define	VXD_FILE			"\\\\.\\REGVXD.VXD"
#define	VXD_NAME			"REGVXD"

// length in ms we wait for Regedit to update its display 
#define	REGEDITSLOWWAIT		750

// Number of recent filters we keep
#define NUMRECENTFILTERS	5

// Offset for listview dots
#define DOTOFFSET			0

// Registry paths
#define REGMON_SETTINGS_KEY				TEXT("Software\\Sysinternals\\Regmon")
#define REGMON_DRIVER_KEY				TEXT("System\\CurrentControlSet\\Services\\Regmon")
#define REGMON_SETTINGS_VALUE			TEXT("Settings")
#define REGMON_RECENT_CONNECT_VALUE		TEXT("Recent")
#define REGMON_RECENT_INFILTER_VALUE	TEXT("InFilters")
#define REGMON_RECENT_EXFILTER_VALUE	TEXT("ExFilters")
#define REGMON_RECENT_HIFILTER_VALUE	TEXT("HiFilters")

// toolbar constants
#define ID_TOOLBAR			1

// defined for comtl32.dll version 4.7
#define TOOLBAR_FLAT		0x800

// windows .NET build number
#define WNET_BUILD_NUMBER	3663

// maximum length of an entier listview line
#define MAXITEMLENGTH		0x1000

// typedef for balloon popup
typedef struct {
	CHAR	itemText[1024];
	RECT	itemPosition;
} ITEM_CLICK, *PITEM_CLICK;

// this typedef, present in newer include files,
// supports the building regmon on older systems
typedef struct 
{
    DWORD cbSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformID;
} DLLVERSIONINFO_, *PDLLVERSIONINFO_;



extern HINSTANCE		hInst;
extern HWND				hWndMain;