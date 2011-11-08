
// version number for position settings
#define POSITION_VERSION	430

// Position settings data structure 
typedef struct {
	int			posversion;
	int			left;
	int			top;
	int			width;
	int			height;
	DWORD		column[NUMCOLUMNS];
	DWORD		historydepth;
	BOOLEAN		maximized;
	BOOLEAN		clocktime;
	BOOLEAN		showms;
	BOOLEAN		ontop;
	DWORD		highlightfg;
	DWORD		highlightbg;
	BOOLEAN		logerror;
	BOOLEAN		logsuccess;
	BOOLEAN		logreads;
	BOOLEAN		logwrites;
	BOOLEAN		logaux;
	LOGFONT		font;
} POSITION_SETTINGS;


//
// Externals
//
extern BOOLEAN		OnTop;
extern BOOLEAN		ClockTime;
extern HINSTANCE	hInst;
extern BOOLEAN		ClockTime;
extern BOOLEAN		ShowToolbar;
extern BOOLEAN		ShowMs;
extern POSITION_SETTINGS	PositionInfo;

VOID GetPositionSettings();
VOID SavePositionSettings( HWND hWnd );