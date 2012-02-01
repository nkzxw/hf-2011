
#include <windows.h>
#include <commctrl.h>
#include "toolbar.h"
#include "resource.h"

// functions
HWND BuildToolBar(HWND hTargetDlg);
BOOL HandleTBToolTips(LPARAM lParam);

// constants
const CHAR*      szTBBmpName  = "ToolBar";
// button styles
const TBBUTTON   TBbs[]       = 
{
	{0,ID_OPEN,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{10,ID_RESTARTDEBUG,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{11,ID_CONTINUEDEBUG,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{14,ID_SUSPENDALL,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{8,ID_STOPDEBUG,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	
	{0,NULL,TBSTATE_ENABLED,(BYTE)TBSTYLE_SEP,0,0},

	{7,ID_SAVETOFILE,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{6,ID_CLEARLIST,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{3,ID_AUTOSCROLL,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{15,ID_SEARCH,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{5,ID_TOPMOST,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},

	{0,NULL,TBSTATE_ENABLED,(BYTE)TBSTYLE_SEP,0,0},

	{9,ID_OPTION,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{13,ID_DUMP,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{12,ID_SETBPX,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{21,ID_SETBPM,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{16,ID_VIEWREGS,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{17,ID_MEMSERVER,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{19,ID_SHOWPEINFO,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{18,ID_SHOWMODULES,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},
	{20,ID_APISTACKMOD,TBSTATE_INDETERMINATE,(BYTE)TBSTYLE_BUTTON,0,0},
	{22,ID_ACTION_MAR,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0},

	{0,NULL,TBSTATE_ENABLED,(BYTE)TBSTYLE_SEP,0,0},
	{1,ID_EXIT,TBSTATE_ENABLED,(BYTE)TBSTYLE_BUTTON,0,0}
};

HWND BuildToolBar(HWND hTargetDlg)
{
	return CreateToolbarEx(
		hTargetDlg,
		WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT,
		ID_TB,
		23, // number of buttons in the bitmap
		NULL,
		(DWORD)LoadBitmap(GetModuleHandle(NULL),szTBBmpName),
		(LPTBBUTTON)&TBbs,
		24, // number of buttons to create
		16,
		16,
		16,
		16,
		sizeof(TBBUTTON));
}

BOOL HandleTBToolTips(LPARAM lParam)
{
	if (((LPNMHDR) lParam)->code == TTN_NEEDTEXT)
	{
		switch(((LPNMHDR) lParam)->idFrom)
		{
		case ID_OPEN:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Choose and debug a file...";
			return TRUE;

		case ID_RESTARTDEBUG:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Restart debugging";
			return TRUE;

		case ID_CONTINUEDEBUG:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Resume debugging";
			return TRUE;

		case ID_SUSPENDALL:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Suspend all threads";
			return TRUE;

		case ID_STOPDEBUG:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Stop debugging";
			return TRUE;

		case ID_SAVETOFILE:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Save the list...";
			return TRUE;

		case ID_CLEARLIST:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Clear list";
			return TRUE;

		case ID_AUTOSCROLL:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Autoscroll list";
			return TRUE;

		case ID_SEARCH:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Search in list...";
			return TRUE;

		case ID_TOPMOST:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Window stays on top or not";
			return TRUE;

		case ID_DUMP:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Dump process...";
			return TRUE;

		case ID_OPTION:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Set options...";
			return TRUE;

		case ID_SETBPX:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Set breakpoint on execution...";
			return TRUE;

		case ID_SETBPM:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Set breakpoint on execution/read/write...";
			return TRUE;

		case ID_VIEWREGS:
			((LPTOOLTIPTEXT)lParam)->lpszText = "View registers...";
			return TRUE;

		case ID_MEMSERVER:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Start Memory Server...";
			return TRUE;

		case ID_SHOWPEINFO:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Open PE Viewer...";
			return TRUE;

		case ID_SHOWMODULES:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Show Process Modules...";
			return TRUE;

		case ID_APISTACKMOD:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Modify API Parameters...";
			return TRUE;

		case ID_ACTION_MAR:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Modify API return values...";
			return TRUE;

		case ID_EXIT:
			((LPTOOLTIPTEXT)lParam)->lpszText = "Exit SoftSnoop";
			return TRUE;

		default:
			return FALSE;
		}
	}
	else
		return FALSE;
}
