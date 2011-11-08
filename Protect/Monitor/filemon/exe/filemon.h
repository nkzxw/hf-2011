/******************************************************************************
*
*       FileMon - File System Monitor for Windows NT/9x
*		
*		Copyright (c) 1996-1998 Mark Russinovich and Bryce Cogswell
*
*		See readme.txt for terms and conditions.
*
*    	PROGRAM: FileMon.h
*
*    	PURPOSE: Definitions for filemon.
*
******************************************************************************/

// Application name for message boxes
#define APPNAME				_T("Filemon")

// Registry paths
#define FILEMON_SETTINGS_KEY			TEXT("Software\\Sysinternals\\Filemon")
#define FILEMON_SETTINGS_VALUE			TEXT("Settings")
#define FILEMON_RECENT_CONNECT_VALUE	TEXT("Recent")
#define FILEMON_RECENT_INFILTER_VALUE	TEXT("InFilters")
#define FILEMON_RECENT_EXFILTER_VALUE	TEXT("ExFilters")
#define FILEMON_RECENT_HIFILTER_VALUE	TEXT("HiFilters")

// Delay for popping listview subitem tooltips
#define BALLOONDELAY		10

// toolbar height plus the borders
#define TOOLBARHEIGHT		28

// number of toolbar buttons
#define NUMTOOLBARBUTTONS	13

// Number of columns in the listview
#define NUMCOLUMNS			7

// Number of filters to remember
#define	NUMRECENTFILTERS	5

// Max length of string for a column in the listview
#define MAXITEMLENGTH		0x1000

// All drives and no drives 'drive letter' entries (integer)
#define ALLDRIVES			27
#define NODRIVES			28

// Variables/definitions for the driver that performs the actual monitoring.
#define	SYS_FILE			_T("FILEM.SYS")
#define	SYS_NAME			_T("FILEMON")

#define	VXD_FILE			"\\\\.\\FILEVXD.VXD"
#define	VXD_NAME			"FILEVXD"

// Drive type names
#define DRVUNKNOWN			0
#define DRVFIXED			1
#define DRVREMOTE			2
#define DRVRAM				3
#define DRVCD				4
#define DRVREMOVE			5

// must be > 26 (max drive letters)
#define PIPE_DRIVE			64
#define SLOT_DRIVE			65

// Offset for dots in listview
#define DOTOFFSET			0

// Position settings data structure 
#define POSITION_VERSION	410
typedef struct {
	int			posversion;
	int			left;
	int			top;
	int			width;
	int			height;
	DWORD		column[NUMCOLUMNS];
	DWORD		curdriveset;
	DWORD		historydepth;
	BOOLEAN		maximized;
	BOOLEAN		timeduration;
	BOOLEAN		showms;
	BOOLEAN		logreads;
	BOOLEAN		logwrites;
	BOOLEAN		ontop;
	BOOLEAN		hookpipes;
	BOOLEAN		hookslots;
	BOOLEAN		showtoolbar;
	LOGFONT		font;
	DWORD		highlightfg;
	DWORD		highlightbg;
} POSITION_SETTINGS;

// typedef for balloon popup
typedef struct {
	TCHAR	itemText[1024];
	RECT	itemPosition;
} ITEM_CLICK, *PITEM_CLICK;

// toolbar constants
#define ID_TOOLBAR        1

// defined for comtl32.dll version 4.7
#define TOOLBAR_FLAT		0x800

// this typedef, present in newer system include files,
// supports the building filemon on older systems
typedef struct 
{
    DWORD cbSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformID;
} DLLVERSIONINFO_, *PDLLVERSIONINFO_;

// Instdrv.c
BOOL LoadDeviceDriver( const TCHAR * Name, const TCHAR * Path, 
					  HANDLE * lphDevice, PDWORD Error );
BOOL UnloadDeviceDriver( const TCHAR * Name );
