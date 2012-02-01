
#include "plugins.h"
#include "resource.h"

// function prototypes
LPSSAPI  __stdcall GetSSApi();
void     InitPluginStruct(LPSSAPI pSSApi);
BOOL     WriteLogFile(HANDLE hFile, char* szText);
BOOL     LoadPlugins();
BOOL     ProcessPlugin(char* szDllName);
DWORD    __stdcall PlgnCallStub(void* pParam);
BOOL     ProcessPluginMenu(WORD wID);
BOOL     SSAPIPROC AddPluginFunction(char* szPName, fStartSSPlugin pFunctAddr);
BOOL     SSAPIPROC RegisterSSPluginWindow(HWND hPlginWnd);
BOOL     SSAPIPROC UnregisterSSPluginWindow(HWND hPluginWnd);
void     PostPluginWindowMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
void     SSAPIPROC SetSSOnTop(BOOL bTop);
DWORD    SSAPIPROC GetSSApiVersion();
void     SSAPIPROC PrintStatus(char* szText);
BOOL     SSAPIPROC EndDebugSession();
void     SSAPIPROC ExitSS();
DWORD    SSAPIPROC ProcAddrToMenuID(fStartSSPlugin pProc);

// constants
#define PLUGIN_MENU_BASE_ID         8500
#define PLUGIN_SUBMENU_INDEX        4

const char*     szPlginSrch         = ".\\Plugins\\*.dll";
const char*     szPlginDir          = "\\Plugins\\";
const char*     szStartPlginFn      = "StartSSPlugin";
const char*     szPlginLogFname     = "PluginInit.log";

// global variables
SSAPI           SSApi;
fStartSSPlugin  PluginFuncts[MAX_PLUGIN_NUM];
DWORD           dwPluginNum         = 0;
HWND            PluginWnds[MAX_PLUGIN_WND_NUM];
DWORD           dwPluginWndNum      = 0;
HANDLE          hLogFile;
BOOL            bLogFile            = FALSE;
BOOL            bPluginHandler      = FALSE;


void InitPluginStruct(LPSSAPI pSSApi)
{
	HMENU hMenu;

	// fill the struct
	pSSApi->szDebuggeePath           = szFname;
	pSSApi->pPI						 = &PI;
	pSSApi->Print					 = Add;
	pSSApi->lpbDebugging             = &bDebugging;
	pSSApi->lpbWinNT                 = &bWinNT;
	pSSApi->lpdwImageBase            = &ImageBase;
	pSSApi->lpdwSizeOfImage          = &SizeOfImage;
	pSSApi->ShowError                = ShowError;
	pSSApi->AddPluginFunction        = AddPluginFunction;
	pSSApi->hSSWnd                   = hDlg_;
	pSSApi->lpbGUIOutPut             = &Option.bGUIOutPut;
	pSSApi->lpbHandleExceptions      = &Option.bHandleExceptions;
	pSSApi->lpbStopAtDB              = &Option.bStopAtDB;
	pSSApi->lpbGUIOnTop              = &Option.bWinTopMost;
	pSSApi->hSSInst                  = hInst;
	pSSApi->SuspendProcess           = SuspendProcess;
	pSSApi->ResumeProcess            = ResumeProcess;
	pSSApi->RegisterPluginWindow     = RegisterSSPluginWindow;
	pSSApi->hLBDbg                   = hLBDbg;
	pSSApi->UnregisterPluginWindow   = UnregisterSSPluginWindow;
	pSSApi->SetSSOnTop               = SetSSOnTop;
	pSSApi->GetSSApiVersion          = GetSSApiVersion;
	// 1.1
	pSSApi->lpbStopAtEntry           = &Option.bStopAtEntry;
	pSSApi->lpbPluginHandled         = &bPluginHandler;
	pSSApi->StartDebugSession        = StartDebugThread;
	pSSApi->PrintStatus              = PrintStatus;
	pSSApi->lpdwEntryPointVA         = &dwEntryPointVA;
	pSSApi->EndDebugSession          = EndDebugSession;
	pSSApi->ExitSS                   = ExitSS;
	pSSApi->lpbCmdArgs               = &Option.bAdditionalCmdLine;
	pSSApi->szCmdArgs                = Option.cCmdLine;
	pSSApi->lpbNoTrapDlls            = &Option.bDllNoTrapList;
	pSSApi->szNoTrapDlls             = Option.cDllNoTrapList;
	pSSApi->lpbShowTIDs              = &Option.bShowTIDs;
	hMenu = GetMenu(hDlg_);
	pSSApi->hPluginMenu              = GetSubMenu(hMenu, PLUGIN_SUBMENU_INDEX);
	pSSApi->ProcAddrToMenuID         = ProcAddrToMenuID;

	return;
}

LPSSAPI __stdcall GetSSApi()
{
	return &SSApi;
}

BOOL WriteLogFile(HANDLE hFile, char* szText)
{
	DWORD dwBytesWritten;

	if (!bLogFile)
		return FALSE;

	return WriteFile(
		hFile,
		(LPCVOID)szText, 
		lstrlen(szText),
		&dwBytesWritten,
		NULL);
}


// returns:
// TRUE - at least one plugin was loaded
BOOL LoadPlugins()
{
	WIN32_FIND_DATA  Find;
	HANDLE           hFind;
	char             cBuff[MAX_PATH];

	// create plugin load log file
	GetCurrentDirectory(sizeof(cBuff), cBuff);
	lstrcat(cBuff, szPlginDir);
	lstrcat(cBuff, szPlginLogFname);
	hLogFile = CreateFile(
		cBuff,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		0);
	if (hLogFile != INVALID_HANDLE_VALUE)
		bLogFile = TRUE;

	// search plugins
	hFind = FindFirstFile(szPlginSrch, &Find);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		if (bLogFile)
			CloseHandle(hLogFile);
		return FALSE;
	}

	do
	{
		ProcessPlugin(Find.cFileName);
	} while (FindNextFile(hFind, &Find));

	// clean up
	FindClose(hFind);
	if (bLogFile)
		CloseHandle(hLogFile);

	return TRUE;
}

BOOL ProcessPlugin(char* szDllName)
{
	char         cDir[MAX_PATH];
	HINSTANCE    hDll;
	FARPROC      pFn;
	char         *pCH;
	HMENU        hMenu;
	char         cBuff[MAX_PATH];

	// get a full path name and load the library
	GetCurrentDirectory(sizeof(cDir), cDir);
	lstrcat(cDir, szPlginDir);
	lstrcat(cDir, szDllName);
	wsprintf(cBuff, "\r\n->%s:\r\n", szDllName);
	WriteLogFile(hLogFile, cBuff);
	hDll = LoadLibrary(cDir);
	if (!hDll)
	{
		WriteLogFile(hLogFile, "\"LoadLibrary\" failed !\r\n");
		return FALSE;
	}

	WriteLogFile(hLogFile, "Plugin loaded successfully.\r\n");

	pFn = GetProcAddress(hDll, szStartPlginFn);
	if (pFn)
	{
		if (dwPluginNum == MAX_PLUGIN_NUM)
		{
			WriteLogFile(hLogFile, "Maximum number of plugins reached !\r\n");
			return FALSE;
		}

		// save the start routine address
		PluginFuncts[dwPluginNum] = (fStartSSPlugin)pFn;		

		// add the plugin in the menu
		pCH = strstr(szDllName, ".");
		*pCH = 0;
		hMenu = GetMenu(hDlg_);
		hMenu = GetSubMenu(hMenu, PLUGIN_SUBMENU_INDEX);
		if (!dwPluginNum)
			RemoveMenu(hMenu, ID_NOPLUGINS, MF_BYCOMMAND);
		AppendMenu(hMenu, MF_STRING, PLUGIN_MENU_BASE_ID + dwPluginNum, szDllName);

		wsprintf(cBuff, "\"%s\" added in the plugin menu.\r\n", szDllName);
		WriteLogFile(hLogFile, cBuff);

		++dwPluginNum;
	}
	else
	{
		// no start function exported !
		wsprintf(cBuff, "\"%s\" export not found.\r\n", szStartPlginFn);
		WriteLogFile(hLogFile, cBuff);
	}

	return TRUE;
}

DWORD __stdcall PlgnCallStub(void* pParam)
{
	if (!PluginFuncts[(DWORD)pParam]())
		ShowError("Plugin returned an error !");
	return 0;
}

BOOL ProcessPluginMenu(WORD wID)
{
	DWORD  i, dwThId;
	HANDLE hTh;

	for (i=0; i < dwPluginNum; i++)
		if (i + PLUGIN_MENU_BASE_ID == wID)
		{
			hTh = CreateThread(NULL, 0, PlgnCallStub, (void*)i, 0, &dwThId);
			if (!hTh)
				ShowError("Couldn't create thread for plugin !");
			else
				CloseHandle(hTh); // close the plugin thread handle
			return TRUE;
		}

	return FALSE;
}

BOOL SSAPIPROC AddPluginFunction(char* szPName, fStartSSPlugin pFunctAddr)
{
	HMENU        hMenu;

	if (dwPluginNum == MAX_PLUGIN_NUM)
		return FALSE;

	// add the new plugin function in the plugin menu
	hMenu = GetMenu(hDlg_);
	hMenu = GetSubMenu(hMenu, 4);
	if (!dwPluginNum)
		RemoveMenu(hMenu, ID_NOPLUGINS, MF_BYCOMMAND);
	AppendMenu(hMenu, MF_STRING, PLUGIN_MENU_BASE_ID + dwPluginNum, szPName);
	PluginFuncts[dwPluginNum++] = pFunctAddr;
	return TRUE;
}

BOOL SSAPIPROC RegisterSSPluginWindow(HWND hPluginWnd)
{
	if (dwPluginWndNum == MAX_PLUGIN_WND_NUM)
		return FALSE;

	PluginWnds[dwPluginWndNum++] = hPluginWnd;
	return TRUE;
}

BOOL SSAPIPROC UnregisterSSPluginWindow(HWND hPluginWnd)
{
	UINT   i, iArray;
	BOOL   bFound = FALSE;
	DWORD  dwNewPlginWndCnt;

	// free the "PluginWnds" array from the specified HWND value
	dwNewPlginWndCnt = dwPluginWndNum;
	for (i=0,iArray=0; i < dwPluginWndNum; i++)
	{
		if (PluginWnds[i] == hPluginWnd)
		{
			--dwNewPlginWndCnt;
			bFound = TRUE;
			continue; // skip this hWnd value
		}
		PluginWnds[iArray++] = PluginWnds[i]; // reset the hWnd value in the array
	}
	dwPluginWndNum = dwNewPlginWndCnt;

	return bFound;
}

void PostPluginWindowMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	UINT i;

	// pass the stuff to all registered windows
	for (i=0; i < dwPluginWndNum; i++)
		SendMessage(PluginWnds[i], Msg, wParam, lParam);
	return;
}

void SSAPIPROC SetSSOnTop(BOOL bTop)
{
	BOOL bCurTop;

	bCurTop = (GetMenuState(hMenu, ID_TOPMOST, MF_BYCOMMAND) == MF_CHECKED);
	if (bCurTop != bTop)
		SendMessage(hDlg_, WM_COMMAND, MAKELONG(ID_TOPMOST,0), 0);
	return;
}

DWORD SSAPIPROC GetSSApiVersion()
{
	return PLUGIN_INTERFACE_VERSION;
}

void SSAPIPROC PrintStatus(char* szText)
{
	if (!szText)
		// default debugging-status wanted
		SetDebuggingStatus();
	else
		UpdateStatus(szText);
	return;
}

BOOL SSAPIPROC EndDebugSession()
{
	if (!bDebugging)
		return FALSE;

	SendMessage(hDlg_,WM_COMMAND,MAKEWPARAM(ID_STOPDEBUG,0),NULL);
	return TRUE;
}

void SSAPIPROC ExitSS()
{
		SendMessage(hDlg_, WM_CLOSE, 0, 0);
		return;
}

DWORD SSAPIPROC ProcAddrToMenuID(fStartSSPlugin pProc)
{
	DWORD i;

	for (i=0; i < dwPluginNum; i++)
		if (PluginFuncts[i] == pProc)
			return PLUGIN_MENU_BASE_ID + i;
	return 0;
}