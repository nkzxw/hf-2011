/*																			*\

  SoftSnoop by yoda/FReAK2FReAK

\*																			*/

#define  WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <shellapi.h>
#include <imagehlp.h>
#include "resource.h"
#include "SoftSnoop.h"
#include "optionmenu.h"
#include "ToolBar.h"
#include "DumpMenu.h"
#include "BPMenu.h"
#include "SearchMenu.h"
#include "RegsMenu.h"
#include "MemServer.h"
#include "ApiParam.h"
#include "ModuleMenu.h"
#include "PEEditor.h"
#include "StackMenu.h"
#include "plugins.h"
#include "BPMMenu.h"
#include "ARMMenu.h"

#include ".\ApiSnoop\Detours\detours.h"

#pragma comment(linker,"/FILEALIGN:0x200")

// add by ygq

// constants
#define MOD_NAME_COLUMN_SIZE            306
#define MOD_IMAGESIZE_COLUMN_SIZE       78
#define MOD_IMAGEBASE_COLUMN_SIZE       78
#define MAX_MODULE_NUM                  300

DWORD dwModuleCount = 0;
struct sModuleInfo *ModuleList = NULL;
DWORD dwApiCount = 0;
struct sApiInfo *ApiList = NULL;

BOOL isDebuggingProcess = FALSE;

DWORD dwInjectedProcessID;

HANDLE hCmdSignal, hCmdFinishSignal;
HANDLE dwMapMemHandle;
DWORD* dpMappedMem;
void InitMapMem(DWORD);
void UnInitMapMem();
void RefreshModuleApiList();
void NotifyOptionChange();
void NotifyContinueOutput();
void UpdateShareMem();
char* GetModuleName(DWORD);
void ProcessInject(DWORD);

// end by ygq

// the functions
VOID     SSAPIPROC Add(PSTR szText);
VOID     ProcessOptions();
void     DefApiToCombo(HWND hDlg, DWORD dwCBId);
int      SSAPIPROC ShowError(PSTR MsgText);
VOID     UpdateStatus(PSTR StatusText);
VOID     SetDebuggingStatus();
void     ExtractDirectory(char* szPath, char* cDir);
BOOL     IsOrdinal(DWORD TmpOrdinal);
DWORD    RVA2Offset(DWORD aRVA);
//DWORD   Offset2RVA(DWORD aOffset);
VOID     SSAPIPROC SuspendProcess();
VOID     SSAPIPROC ResumeProcess();
DWORD    EspToTID(DWORD dwESP);
HANDLE   GetThreadHandle(DWORD dwThreadID);
VOID     SetDebugWindowPos();
VOID     SetNormalWindowPos();
VOID     SetWindowTop(HWND hDlg);
VOID     SetWindowNonTop(HWND hDlg);
BOOL     CanTrapDll(DWORD iDll);
BOOL     ShouldIgnoreAPI(int iArrayIndex, BOOL bCheckBPX);
CHAR*    ExtractFileName(CHAR* szFilePath);
void     SetLastFilesMenu();
BOOL     SetNewestLastFile(char* szPath);
BOOL     HandleCmdLine();
VOID     Handle_ID_SAVETOFILE(DWORD dwListItemNum);
VOID     SSAPIPROC StartDebugThread();
BOOL     DoAPIStuff();
BOOL     ProcessBPXList();
BOOL     WriteProcessByte(HANDLE hProc, void* pAddr, BYTE bToWrite, BYTE* pbOldByte);
BOOL     SetThreadEip(HANDLE hThread, DWORD dwEip);
BOOL     GetFilehandleDllName(HANDLE HFILE,CHAR* szLibName,int iBuffSize);
DWORD    HandleInt3(DWORD PID,HANDLE hThread,DWORD dwInt3VA);
BOOL     RestoreBPX();
VOID     PerformDebugCleanup();
void     CleanupThreadList();
void     WaitForUser();
VOID     StartDebug();
VOID     HandleSearch();
LRESULT  CALLBACK WndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void     UpdateGUI(HWND hW, BOOL bDebugging);
DWORD    WINAPI ProcessFile(PVOID);

// constants
#define      MAX_IMPORTS                   2000
#define      MAX_DLLNAMELENGTH			   50
#define      MAX_DLLNUM					   40
#define      ID_STATUSBAR                  0x9000
#define      THREAD_CONTROL_TIMEOUT        200
#define      MAX_LISTITEMCAPTIONLENGTH     500
#define      ID_LASTFILE_1                 0x8000
#define      ID_LASTFILE_2                 ID_LASTFILE_1 + 1
#define      ID_LASTFILE_3                 ID_LASTFILE_1 + 2
#define      ID_LASTFILE_4                 ID_LASTFILE_1 + 3
#define      ID_LASTFILE_5                 ID_LASTFILE_1 + 4

const int    DllHeaderBuffSize			 = 0x1000;
CONST int    MINUS_ONE                   = -1;
const BYTE   Int3                        = 0xCC;
const char   INI_FILE_NAME []			 = "SoftSnoop.ini";
const char   APISNOOPDLLNAME []			 = "APISnoop.dll";
CONST int    DEFAULT_MAINWND_WIDTH       = 600;
CONST int    DEFAULT_MAINWND_HEIGHT      = 420;
CONST int    MIN_MAINWND_WIDTH           = 300;
CONST int    MIN_MAINWND_HEIGHT          = 210;
CONST int    TB_HEIGHT                   = 30;
CONST int    STATUS_HEIGHT               = 30;
const WORD   wLineEnd                    = 0x0A0D;
const char*  szDefApiSec                 = "ApiDefault";
const PSTR   ABOUT_TEXT        = "           -=[  SoftSnoop 1.3.2  ]=-\n\n"\
								 "a small debugger for Win 9X/ME/NT/2K/XP\n\n"\
								 "Greetz:\n"\
								 "tHeRain, Perfx, r!sc, ultraschall,\n"\
								 "avlis, TiBeRiUm, D@niel, Henrik Nordhaus,\n"\
								 "ELiCZ (continue you're great work !),\n"\
								 "Daedalus, aVATAr, Snow Panther, MackT,\n"\
								 "Stone and all who know me...\n\n"\
								 "For any comments, bugreports or suggestions\n"\
								 "contact me at yoda_f2f@gmx.net\n\n"\
								 "Visit: y0da.cjb.net\n\n" \
								 "modified by ygq, add dynamic api hook support\n"\
								 "hmilyyangguoqiang@tom.com";
																	
// global variables
HWND				      hDlg_;
HINSTANCE			      hInst;
OPENFILENAME		      ofn,ofn_save;
char                      szFname[MAX_PATH],cSaveFileBuff[MAX_PATH],cInitDir[MAX_PATH];
HFONT                     hDbgFont;
HACCEL                    hAccel;

STARTUPINFO               SI;
PROCESS_INFORMATION       PI;
PIMAGE_DOS_HEADER		  pDosh;
PIMAGE_NT_HEADERS         pPeh;
PIMAGE_SECTION_HEADER     pSectionh;
PIMAGE_IMPORT_DESCRIPTOR  pIID;

PVOID                    pMap,pMem;
DWORD                    dwMemBase,dwDllNameRVA;

DEBUG_EVENT         dbevent;
DWORD               ImageBase,dwThreadID,SizeOfImage,dwBytesRead,dwBytesWritten,
                    dwAPINum,dwThreadCtrlThreadID, dwEntryPointVA, dwCurEventTID, dwLastEventTID;
HANDLE              hDebugThread;
HWND                hTB,hLBDbg,hStatus;
DWORD *             pDW;
BOOL                STOP;
char                buff[512],DllNameBuff[MAX_DLLNAMELENGTH],DebugStringBuff[1024],
                    IniFilePath[MAX_PATH],ApiSnoopDllPath[MAX_PATH],StatusBuff[80], *szCmd;
CONTEXT             Regs;
BOOL                bDebugging;
HANDLE              hCurrThread,hOutPutFile;
RECT                rOrgWindowPos,rect;
UINT                uiMenuState;
HMENU               hMenu, hmLastFiles;
Options             Option;
CRITICAL_SECTION    csThreadControl;
// BP related global variables
CHAR                TrapApiNameBuff[TRAPAPIBUFFSIZE];
sBPXInfo            BPXInfo[MAX_INT3BPS];
int                 iBPXCnt = 0, icBPRest = 0;
void*               BPRestAddr[MAX_BPRESTORENUM];
// list of all threads
DWORD               dwThreadCount;
ThreadInfo          ThreadList[MAX_THREADNUM];
//Import Data arrays
DWORD               dwFunctNames[MAX_IMPORTS];
BYTE                DllName[MAX_IMPORTS];
char                DllNames[MAX_DLLNUM][MAX_DLLNAMELENGTH];
DWORD               dwItemNum;
BOOL                bWinNT;


int WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine,
				   int nShowCmd)
{
	HANDLE       hDll;
	WNDCLASS     wc;
	int	         cx,cy,iWndWeidth,iWndHeight,iShow;
	MSG          msg;
	char         cBuff[MAX_PATH];

	hInst = hInstance;
	szCmd = lpCmdLine;
	
	// set current directory to exe path
	GetModuleFileName(NULL, cBuff, sizeof(cBuff));
	ExtractDirectory(cBuff, cBuff);
	SetCurrentDirectory(cBuff);

	GetCurrentDirectory(sizeof(cInitDir),cInitDir);

	// save the OS type
	bWinNT = IsNT();

	// add by ygq
	dwMapMemHandle = 0;
	dpMappedMem = NULL;
	hCmdSignal = 0;
	hCmdFinishSignal = 0;
	ModuleList = NULL;
	ApiList = NULL;
	dwModuleCount = 0;
	dwApiCount = 0;
	isDebuggingProcess = FALSE;

	// do some int stuff
	szFname[0] = 0;
	memset(&TrapApiNameBuff,0,sizeof(TrapApiNameBuff));
	memset(&ThreadList,0,sizeof(ThreadList));
	SearchOptions.SearchBuff[0] = 0;
	SearchOptions.CaseSensitive = FALSE;
	SearchOptions.SearchInWholeList = FALSE;

	// do some ini-file stuff
	GetCurrentDirectory(sizeof(IniFilePath),IniFilePath);
	if (IniFilePath[strlen(IniFilePath)-1] != '\\')
		strcat(IniFilePath,"\\");
	strcat(IniFilePath,(char*)&INI_FILE_NAME);

	// do some APISnoop.dll path stuff
	GetCurrentDirectory(sizeof(ApiSnoopDllPath),ApiSnoopDllPath);
	if (ApiSnoopDllPath[strlen(ApiSnoopDllPath)-1] != '\\')
		strcat(ApiSnoopDllPath,"\\");
	strcat(ApiSnoopDllPath,APISNOOPDLLNAME);
	hDll = CreateFile(
		(PSTR)ApiSnoopDllPath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);
	if (hDll == INVALID_HANDLE_VALUE)
	{
		wsprintf(buff,"Couldn't find %s :(",APISNOOPDLLNAME);
		ShowError(buff);
		return -1;
	}
	CloseHandle(hDll);

	// load forcelibrary.dll
	//if (!LoadFL())
	//	return -1;

	// do some other stuff
	ProcessOptions();
	InitializeCriticalSection(&csThreadControl);
	MapSSFiles();

	// create the windows
	ZeroMemory(&wc,sizeof(wc));
	wc.lpfnWndProc = DefDlgProc;
	wc.style = CS_HREDRAW|CS_VREDRAW;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = SS_CLASS_NAME;
	wc.lpfnWndProc = WndProc;
	RegisterClass(&wc);

	iWndWeidth = Option.MainWndWidth;
	iWndHeight = Option.MainWndHeight;
	iShow = SW_SHOWNORMAL;

	cx = (GetSystemMetrics(SM_CXFULLSCREEN) - iWndWeidth) / 2;
	if (cx < 0)
	{
		cx = 0;
		iWndWeidth = DEFAULT_MAINWND_WIDTH;
		iShow = SW_SHOWMAXIMIZED;
	}
	cy = (GetSystemMetrics(SM_CYFULLSCREEN) - iWndHeight) / 2;
	if (cy < 0)
	{
		cy = 0;
		iWndHeight = DEFAULT_MAINWND_HEIGHT;
		iShow = SW_SHOWMAXIMIZED;
	}

	hDlg_ = CreateWindow(
		SS_CLASS_NAME,
		SSDialogTitle,
		WS_OVERLAPPEDWINDOW,
		cx,
		cy,
		iWndWeidth,
		iWndHeight,
		0,
		NULL,
		hInstance,
		NULL);

	hLBDbg = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		"LISTBOX",
		NULL,
		WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_HASSTRINGS,
		0,
		TB_HEIGHT,
		iWndWeidth - 8,
		iWndHeight - STATUS_HEIGHT - 66,
		hDlg_,
		NULL,
		hInstance,
		NULL);

	hStatus = CreateStatusWindow(SBS_SIZEGRIP | WS_VISIBLE | WS_CHILD,NULL,hDlg_,ID_STATUSBAR);
	SendMessage(hStatus,SB_SETPARTS,1,(LPARAM)&MINUS_ONE);

	// set font for the debug list
	hDbgFont = CreateFont(14,9,0,0,200,0,0,0,OEM_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,FF_SCRIPT,"TERMINAL");
	SendMessage(hLBDbg,WM_SETFONT,(WPARAM)hDbgFont,0);

	// load accelerator
	hAccel = LoadAccelerators(hInst,MAKEINTRESOURCE(IDR_ACCEL));

	// show the window
	ShowWindow(hDlg_, iShow);
	UpdateWindow(hDlg_);

	UpdateStatus("Ready");

	// MSG-LOOP
	while(GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(hDlg_,hAccel,&msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}

VOID ProcessOptions()
{
	UINT         i;
	char         cBuff[10];
	const char*  szDefApis[] = {"LoadLibraryA", "GetModuleHandleA", "GetProcAddress", "CallWindowProcA",
								"GetMessageA", "DefWindowProcA", "IsDialogMessage",
								"TranslateMessage", "DispatchMessageA", "ReadProcessMemory", "WriteProcessMemory"};

#define DEFAULT_API_STRING_NUM (sizeof(szDefApis) / sizeof(char*))

	// set Option struct to default
	Option.bScrollList        = TRUE;
	Option.dwIgnoreAPIs       = NO_SPECIAL_APIS;
	Option.dwIgnoreToDlls     = NO_SPECIAL_DLLS;
	Option.dwIgnoreFromDlls   = NO_SPECIAL_DLLS;
	Option.dwIgnoreRegions    = NO_SPECIAL_DLLS;
	Option.wIgnoreAPINum      = 0;
	Option.wIgnoreToDllNum    = 0;
	Option.wIgnoreFromDllNum  = 0;
	Option.wIgnoreRegionNum   = 0;

	Option.bDebugProcess      = FALSE;
	Option.bShowAPICall       = TRUE;
	Option.bShowApiParams     = TRUE;
	Option.bShowApiReturn     = FALSE;

	Option.bMoveWindow        = TRUE;
	Option.bShowDebugEvents   = TRUE;
	Option.bWinTopMost        = FALSE;
	Option.bReportNameAPI     = TRUE;
	Option.bReportOrdinalAPI  = TRUE;
	Option.bHandleExceptions  = FALSE;
	Option.bStopAtDB	      = FALSE;
	Option.bFileOutPut        = FALSE;
	Option.bGUIOutPut         = TRUE;
	Option.bAdditionalCmdLine = FALSE;
	Option.MainWndWidth       = DEFAULT_MAINWND_WIDTH;
	Option.MainWndHeight      = DEFAULT_MAINWND_HEIGHT;
	Option.bShowApiParams     = TRUE;
	Option.bShowApiReturn     = TRUE;
	Option.bStopAtEntry       = FALSE;
	Option.bDlgOnBPM          = FALSE;
	Option.bShellExtension    = TRUE;
	Option.bDllNoTrapList     = FALSE;
	Option.bRestoreBP         = TRUE;
	Option.bShowTIDs          = FALSE;

	ZeroMemory(&Option.cLastFile1, sizeof(Option.cLastFile1));
	ZeroMemory(&Option.cLastFile2, sizeof(Option.cLastFile2));
	ZeroMemory(&Option.cLastFile3, sizeof(Option.cLastFile3));
	ZeroMemory(&Option.cLastFile4, sizeof(Option.cLastFile4));
	ZeroMemory(&Option.cLastFile5, sizeof(Option.cLastFile5));
	ZeroMemory(&Option.cCmdLine, sizeof(Option.cCmdLine));
	ZeroMemory(&Option.cOutFile, sizeof(Option.cOutFile));
	ZeroMemory(&Option.cDllNoTrapList, sizeof(Option.cDllNoTrapList));

	// set default APIs
	for (i=0; i < DEFAULT_API_STRING_NUM; i++)
	{
		wsprintf(cBuff, "%u", i);
		WritePrivateProfileString(szDefApiSec, cBuff, szDefApis[i], IniFilePath);
	}

	// grab option out of ini if present
	GetPrivateProfileStruct("options","current",&Option,sizeof(Option),(PSTR)IniFilePath);

	// fill current path
	ExtractDirectory(IniFilePath, Option.cAppPath);

	return;
}

void DefApiToCombo(HWND hDlg, DWORD dwCBId)
{
	const char* szBad = "APILISTEND";
	char cBuff[10], cApi[100];
	UINT i            = 0;

	while (TRUE)
	{
			wsprintf(cBuff, "%u", i++);
			GetPrivateProfileString(szDefApiSec, cBuff, szBad, cApi, sizeof(cApi), IniFilePath);
			if (lstrcmp(cApi, szBad) == 0)
				break;
			SDIM(hDlg, dwCBId, CB_ADDSTRING, 0, (LPARAM)cApi);
	}
	return;
}

void ModuleListToCombo(HWND hDlg, DWORD dwCBId)
{
	UINT i            = 0;

	for (i=0; i<dwModuleCount; i++)
	{
		if (ModuleList[i].ModuleName!=NULL)
			SDIM(hDlg, dwCBId, CB_ADDSTRING, 0, (LPARAM)ModuleList[i].ModuleName);
	}
	return;
}

int GetModuleIndex(char* cpModuleName)
{
	UINT i            = 0;

	if (cpModuleName==NULL)
		return -1;

	for (i=0; i<dwModuleCount; i++)
	{
		if (strcmp((char*)ModuleList[i].ModuleName, cpModuleName)==0)
			return i;
	}
	return -1;
}

void ApiListToCombo(HWND hDlg, DWORD dwCBId, int dwModuleIndex)
{
	UINT i            = 0;

	for (i=0; i<dwApiCount; i++)
	{
		if (ApiList[i].ModuleIndex==dwModuleIndex)
		{
			if (ApiList[i].ApiName[0]==0)
				wsprintf((char*)ApiList[i].ApiName, "%d", ApiList[i].ApiOrdinal);
			SDIM(hDlg, dwCBId, CB_ADDSTRING, 0, (LPARAM)ApiList[i].ApiName);
		}
	}
	return;
}

int SSAPIPROC ShowError(PSTR MsgText)
{
	return MessageBox(hDlg_,MsgText,"Error",MB_ICONERROR);
}

VOID UpdateStatus(PSTR StatusText)
{
	SendMessage(hStatus,SB_SETTEXT,0,(LPARAM)StatusText);
	return;
}

VOID SetDebuggingStatus()
{
	wsprintf((CHAR*)&StatusBuff,"Debugging: %s...",ExtractFileName(szFname));
	UpdateStatus(StatusBuff);
	return;
}

// cDir has to be MAX_PATH big
// no error checking !!!
void ExtractDirectory(char* szPath, char* cDir)
{
	char* pCH;

	lstrcpy(cDir, szPath);
	pCH = (char*)((DWORD)cDir + lstrlen(cDir) - 1 );
	while (*pCH != '\\')
		--pCH;
	*pCH = 0;
	return;
}

void SetLastFilesMenu()
{
	const char*  szLastFileFmt            = "&%u %s";
	MENUITEMINFO MII;
	char         cBuff[MAX_PATH + 10];

	if (*(char*)Option.cLastFile1)
	{
		// there's at least one path logged

		// enable "reopen" menu item
		EnableMenuItem(hMenu, ID_FILE_REOPEN, MF_ENABLED);
		memset(&MII, 0, sizeof(MII));
		MII.cbSize   = sizeof(MII);
		MII.fMask    = MIIM_SUBMENU;
		MII.hSubMenu = hmLastFiles;
		SetMenuItemInfo(hMenu, ID_FILE_REOPEN, FALSE, &MII);

		// clear existing entries
		DeleteMenu(hmLastFiles, ID_LASTFILE_1, MF_BYCOMMAND);
		DeleteMenu(hmLastFiles, ID_LASTFILE_2, MF_BYCOMMAND);
		DeleteMenu(hmLastFiles, ID_LASTFILE_3, MF_BYCOMMAND);
		DeleteMenu(hmLastFiles, ID_LASTFILE_4, MF_BYCOMMAND);
		DeleteMenu(hmLastFiles, ID_LASTFILE_5, MF_BYCOMMAND);

		// add new entries
		wsprintf(cBuff, szLastFileFmt, 1, Option.cLastFile1);
		AppendMenu(hmLastFiles, MF_STRING, ID_LASTFILE_1, cBuff);
		if (*(char*)Option.cLastFile2)
		{
			wsprintf(cBuff, szLastFileFmt, 2, Option.cLastFile2);
			AppendMenu(hmLastFiles, MF_STRING, ID_LASTFILE_2, cBuff);
		}
		if (*(char*)Option.cLastFile3)
		{
			wsprintf(cBuff, szLastFileFmt, 3, Option.cLastFile3);
			AppendMenu(hmLastFiles, MF_STRING, ID_LASTFILE_3, cBuff);
		}
		if (*(char*)Option.cLastFile4)
		{
			wsprintf(cBuff, szLastFileFmt, 4, Option.cLastFile4);
			AppendMenu(hmLastFiles, MF_STRING, ID_LASTFILE_4, cBuff);
		}
		if (*(char*)Option.cLastFile5)
		{
			wsprintf(cBuff, szLastFileFmt, 5, Option.cLastFile5);
			AppendMenu(hmLastFiles, MF_STRING, ID_LASTFILE_5, cBuff);
		}
	}
	return;
}

BOOL SetNewestLastFile(char* szPath)
{
	// check whether we've this path already
	if (lstrcmpi(szPath, Option.cLastFile1) == 0)
		return FALSE;
	if (lstrcmpi(szPath, Option.cLastFile2) == 0)
		return FALSE;
	if (lstrcmpi(szPath, Option.cLastFile3) == 0)
		return FALSE;
	if (lstrcmpi(szPath, Option.cLastFile4) == 0)
		return FALSE;
	if (lstrcmpi(szPath, Option.cLastFile5) == 0)
		return FALSE;

	// reorganize char chains
	lstrcpy(Option.cLastFile5, Option.cLastFile4);
	lstrcpy(Option.cLastFile4, Option.cLastFile3);
	lstrcpy(Option.cLastFile3, Option.cLastFile2);
	lstrcpy(Option.cLastFile2, Option.cLastFile1);
	lstrcpy(Option.cLastFile1, szPath);
	SetLastFilesMenu();
	return TRUE;
}

BOOL HandleCmdLine()
{
	if (*(char*)szCmd)
	{
		lstrcpy(szFname, szCmd);
		StartDebugThread();
		return TRUE;
	}
	return FALSE;
}

VOID SSAPIPROC Add(PSTR szText)
{
	DWORD  dwBytesWritten;
	char   cBuff[50];

	// new TID ?
	if (dwLastEventTID != dwCurEventTID)
	{
		dwLastEventTID = dwCurEventTID;
		if (Option.bShowTIDs) // report TID change
		{
			wsprintf(cBuff, "TID: %08lXh:", dwCurEventTID);
			Add(cBuff);
		}
	}

	// add the string to the listbox
	if (Option.bGUIOutPut)
		if (SendMessage(hLBDbg,LB_ADDSTRING,NULL,(LPARAM)szText) < 0)
		{
			SuspendProcess();
			MessageBox(
				0,
				"The list is full !\nClick on OK to clear it.",
				"ERROR",
				MB_ICONERROR);
			SendMessage(hLBDbg,LB_RESETCONTENT,NULL,NULL);
			ResumeProcess();
		}
	// write the string to the output file
	if (Option.bFileOutPut)
	{
		WriteFile(hOutPutFile,szText,lstrlen(szText),&dwBytesWritten,NULL);
		WriteFile(hOutPutFile,&wLineEnd,sizeof(wLineEnd),&dwBytesWritten,NULL);
	}
	// scroll down
	if (Option.bScrollList)
		SendMessage(hLBDbg,WM_VSCROLL,SB_LINEDOWN,NULL);
	// inform the plugin windows
	PostPluginWindowMessage(SS_PRINTTEXT, (WPARAM)szText, 0);
	return;
}

// add by ygq

void InitMapMem(DWORD dwPID)
{
	char fileName[200];
	DWORD dwMapExist = 0;

	wsprintf(fileName, "%s %d", SHARE_MEM_NAME, dwPID);
	dwMapMemHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, fileName);
	if (dwMapMemHandle!=NULL)
	{
		dwMapExist = 1;
	} else
	{
		dwMapMemHandle = CreateFileMapping(0, NULL, PAGE_READWRITE, 0, SHARE_MEM_SIZE, fileName);
		if (!dwMapMemHandle)
			return;
	}

	dpMappedMem = (DWORD*)MapViewOfFile(dwMapMemHandle, FILE_MAP_ALL_ACCESS, 0, 0, SHARE_MEM_SIZE);
	if (!dpMappedMem)
		return;

	wsprintf(fileName, "%s %d", SHARE_CMD_NAME, dwPID);
	hCmdSignal = CreateEvent(NULL, FALSE, FALSE, fileName);

	wsprintf(fileName, "%s %d", SHARE_CMDFINISH_NAME, dwPID);
	hCmdFinishSignal = CreateEvent(NULL, FALSE, FALSE, fileName);

	if (dwMapExist)
		NotifyContinueOutput();
	else
	{
		// write option in the mapped mem
		*dpMappedMem = SS_CMD_SET_OPTION;
		*((Options*)(dpMappedMem+1)) = Option;
	}

}

void UnInitMapMem()
{
	if (ModuleList!=NULL)
	{
		VirtualFree(ModuleList, 0, MEM_RELEASE);
		ModuleList = NULL;
	}
	if (ApiList!=NULL)
	{
		VirtualFree(ApiList, 0, MEM_RELEASE);
		ApiList = NULL;
	}
	dwModuleCount = 0;
	dwApiCount = 0;
	if (dpMappedMem)
	{
		UnmapViewOfFile(dpMappedMem);
		dpMappedMem = NULL;
	}
	if (dwMapMemHandle)
	{
		CloseHandle(dwMapMemHandle);
		dwMapMemHandle = 0;
	}
	if (hCmdSignal)
	{
		CloseHandle(hCmdSignal);
		hCmdSignal = 0;
	}
	if (hCmdFinishSignal)
	{
		CloseHandle(hCmdFinishSignal);
		hCmdFinishSignal = 0;
	}
}

void RefreshModuleApiList()
{
	DWORD dwOldCount;

	if (dpMappedMem==NULL)
		return;

	// Get Module List
	if (ResetEvent(hCmdFinishSignal)==0)
		return;
	*dpMappedMem = SS_CMD_GET_MODULE_LIST;
	if (SetEvent(hCmdSignal)==0)
		return;
	if (WaitForSingleObject(hCmdFinishSignal, 1000)==WAIT_OBJECT_0)
	{
		dwOldCount = dwModuleCount;
		dwModuleCount = *(dpMappedMem+1);
		if (dwModuleCount*sizeof(sModuleInfo) >= SHARE_MEM_SIZE)
			dwModuleCount = (SHARE_MEM_SIZE-8) / sizeof(sModuleInfo);
		if (dwModuleCount>dwOldCount)
		{
			if (ModuleList!=NULL)
				VirtualFree(ModuleList, 0, MEM_RELEASE);
			ModuleList = (sModuleInfo*)VirtualAlloc(NULL, dwModuleCount*sizeof(sModuleInfo), MEM_COMMIT, PAGE_READWRITE);
			if (ModuleList==NULL)
				return;
		}
		CopyMemory(ModuleList, dpMappedMem+2, dwModuleCount*sizeof(sModuleInfo));
	}

	// Get Api List
	if (ResetEvent(hCmdFinishSignal)==0)
		return;
	*dpMappedMem = SS_CMD_GET_API_LIST;
	if (SetEvent(hCmdSignal)==0)
		return;
	if (WaitForSingleObject(hCmdFinishSignal, 2000)==WAIT_OBJECT_0)
	{
		dwOldCount = dwApiCount;
		dwApiCount = *(dpMappedMem+1);
		if (dwApiCount*sizeof(sApiInfo) >= SHARE_MEM_SIZE)
			dwApiCount = (SHARE_MEM_SIZE-8) / sizeof(sApiInfo);
		if (dwApiCount>dwOldCount)
		{
			if (ApiList!=NULL)
				VirtualFree(ApiList, 0, MEM_RELEASE);
			ApiList = (sApiInfo*)VirtualAlloc(NULL, dwApiCount*sizeof(sApiInfo), MEM_COMMIT, PAGE_READWRITE);
			if (ApiList==NULL)
				return;
		}
		CopyMemory(ApiList, dpMappedMem+2, dwApiCount*sizeof(sApiInfo));
	}
}

void NotifyOptionChange()
{
	if (dpMappedMem==NULL)
		return;

	*dpMappedMem = SS_CMD_SET_OPTION;
	*((Options*)(dpMappedMem+1)) = Option;
	if (SetEvent(hCmdSignal)==0)
		return;
}

void NotifyContinueOutput()
{
	if (dpMappedMem==NULL)
		return;

	*dpMappedMem = SS_CMD_CONTINUE;
	*((Options*)(dpMappedMem+1)) = Option;
	if (SetEvent(hCmdSignal)==0)
		return;
}

void UpdateShareMem()
{
	//CHAR *pCH = (CHAR*)TrapApiNameBuff;
	//struct sApiToHook *pBuf = (sApiToHook*)((DWORD)dpMappedMem + sizeof(DWORD));
	//DWORD TrappedApiCount = 0, splitPos;

	//if (dpMappedMem==NULL)
	//	return;
	//while (*pCH)
	//{
	//	splitPos = strstr(pCH, "@")-pCH;
	//	//strncpy((char*)pBuf->ApiName, pCH, splitPos);
	//	//pBuf->ApiName[splitPos] = 0;
	//	//strcpy((char*)pBuf->DllName, pCH+splitPos+1);

	//	pBuf ++;
	//	TrappedApiCount ++;
	//	pCH += strlen(pCH);
	//	pCH ++;
	//}
	//*dpMappedMem = TrappedApiCount;
}

char* GetModuleName(DWORD dwModuleIndex)
{
	if (ModuleList==NULL)
		return NULL;
	if (dwModuleIndex>=dwModuleCount)
		return NULL;
	return (char*)ModuleList[dwModuleIndex].ModuleName;
}

void AddModuleInfoItem(HWND hDlg,
					   CHAR* szPath,
					   DWORD dwImageBase,
					   DWORD dwSizeOfImage,
					   int   iIndex)
{
	LVITEM        ColItem;
	CHAR          Buff[10];

	// insert the path
	memset(&ColItem, 0, sizeof(ColItem));
	ColItem.mask       = LVIF_TEXT;
	ColItem.iItem      = iIndex;
	ColItem.pszText    = szPath;
	SDIM(hDlg, IDC_MODULELIST, LVM_INSERTITEM, 0, (LPARAM)&ColItem);
	// insert the base
	ColItem.iSubItem   = 1;
	wsprintf(Buff, "%08lXh", dwImageBase);
	ColItem.pszText    = Buff;
	SDIM(hDlg, IDC_MODULELIST, LVM_SETITEM, 0, (LPARAM)&ColItem);
	// insert SizeOfImage
	ColItem.iSubItem   = 2;
	wsprintf(Buff, "%08lXh", dwSizeOfImage);
	ColItem.pszText    = Buff;
	SDIM(hDlg, IDC_MODULELIST, LVM_SETITEM, 0, (LPARAM)&ColItem);
	
	++iIndex;
	return;
}

BOOL ShowProcessModules(HWND hDlg)
{
	LVCOLUMN             Col;
	DWORD                i;
	
	// make listview columns
	memset(&Col, 0, sizeof(Col));
	Col.mask      = LVCF_TEXT | LVCF_WIDTH;
	Col.cx        = MOD_NAME_COLUMN_SIZE;
	Col.pszText   = "Module name";
	SDIM(hDlg, IDC_MODULELIST, LVM_INSERTCOLUMN, 0, (LPARAM)&Col);
	Col.cx        = MOD_IMAGEBASE_COLUMN_SIZE;
	Col.pszText   = "ImageBase";
	SDIM(hDlg, IDC_MODULELIST, LVM_INSERTCOLUMN, 1, (LPARAM)&Col);
	Col.cx        = MOD_IMAGESIZE_COLUMN_SIZE;
	Col.pszText   = "SizeOfImage";
	SDIM(hDlg, IDC_MODULELIST, LVM_INSERTCOLUMN, 2, (LPARAM)&Col);

	for (i=0;i<dwModuleCount;i++)
	{
		AddModuleInfoItem(hDlg, (char*)ModuleList[i].ModuleName, ModuleList[i].BaseAddr,
			ModuleList[i].Size, i);
	}

	return TRUE;
}

BOOL ShowProcessList(HWND hDlg)
{
	LVCOLUMN             Col;
	LVITEM               ColItem;
	HANDLE               hSnapshot;
	PROCESSENTRY32       pe;
	DWORD                iIndex;
	
	// make listview columns
	memset(&Col, 0, sizeof(Col));
	Col.mask      = LVCF_TEXT | LVCF_WIDTH;
	Col.cx        = MOD_IMAGEBASE_COLUMN_SIZE;
	Col.pszText   = "PID";
	SDIM(hDlg, IDC_MODULELIST, LVM_INSERTCOLUMN, 0, (LPARAM)&Col);
	Col.cx        = MOD_NAME_COLUMN_SIZE;
	Col.pszText   = "Process name";
	SDIM(hDlg, IDC_MODULELIST, LVM_INSERTCOLUMN, 1, (LPARAM)&Col);

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,NULL);
	if (hSnapshot==NULL)
		return FALSE;
	pe.dwSize = sizeof(PROCESSENTRY32);

	iIndex = 0;
	if (Process32First(hSnapshot,&pe))
	{
		do
		{
			// ingnore self...
			if (pe.th32ProcessID==GetCurrentProcessId())
				continue;

			// insert the id
			memset(&ColItem, 0, sizeof(ColItem));
			ColItem.mask       = LVIF_TEXT;
			ColItem.iItem      = iIndex;
			wsprintf(buff, "%d", pe.th32ProcessID);
			ColItem.pszText    = buff;
			SDIM(hDlg, IDC_MODULELIST, LVM_INSERTITEM, 0, (LPARAM)&ColItem);
			// insert the name
			ColItem.iSubItem   = 1;
			ColItem.pszText    = pe.szExeFile;
			SDIM(hDlg, IDC_MODULELIST, LVM_SETITEM, 0, (LPARAM)&ColItem);

			iIndex++;
		}
		while(Process32Next(hSnapshot,&pe)==TRUE);
	}

	CloseHandle (hSnapshot);

	return TRUE;
}

BOOL SetPrivilege()
{
	TOKEN_PRIVILEGES tp;
	LUID luid;
	HANDLE hToken;

	if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ALL_ACCESS,&hToken))
	{
		return FALSE;
	}

	if(!LookupPrivilegeValue(NULL,SE_DEBUG_NAME,&luid))
	{
		return FALSE; 
	}
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	//if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	//else
	//	tp.Privileges[0].Attributes = 0;
	// Enable the privilege or disable all privileges.
	AdjustTokenPrivileges(
		   hToken, 
		   FALSE, 
		   &tp, 
		   sizeof(TOKEN_PRIVILEGES), 
		   (PTOKEN_PRIVILEGES) NULL, 
		   (PDWORD) NULL); 
 	// Call GetLastError to determine whether the function succeeded.
	if (GetLastError() != ERROR_SUCCESS) 
	{ 
		return FALSE; 
	} 
	return TRUE;
}

int Inject(DWORD pid, char *dll)
 
{
	PWSTR pszLibFileRemote = NULL;
	HANDLE hRemoteProcess = NULL,hRemoteThread = NULL;
	char CurPath[256];

	hRemoteProcess = OpenProcess(
		 PROCESS_QUERY_INFORMATION |   // Required by Alpha
         PROCESS_CREATE_THREAD     |   // For CreateRemoteThread
         PROCESS_VM_OPERATION      |   // For VirtualAllocEx/VirtualFreeEx
         PROCESS_VM_WRITE,             // For WriteProcessMemory
         FALSE, pid);

	if (hRemoteProcess==NULL)
		return -1;
	
	//GetCurrentDirectory(256,CurPath);
	//strcat(CurPath,"\\");
	memset(CurPath, 0, sizeof(CurPath));
	strcat(CurPath, dll);

	int len = (strlen(CurPath)+1)*2;
	WCHAR wCurPath[256];
	MultiByteToWideChar(CP_ACP,0,CurPath,-1,wCurPath,256);

	pszLibFileRemote = (PWSTR) 
		VirtualAllocEx(hRemoteProcess, NULL, len, MEM_COMMIT, PAGE_READWRITE);

	WriteProcessMemory(hRemoteProcess, pszLibFileRemote, 
		(PVOID) wCurPath, len, NULL);

	PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)
         GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibraryW");

	hRemoteThread = CreateRemoteThread(hRemoteProcess, NULL, 0, 
		pfnThreadRtn, pszLibFileRemote, 0, NULL);

	if(hRemoteThread == NULL)
		return -1;

	return 0;
}

// end add by ygq

BOOL IsOrdinal(DWORD TmpOrdinal)
{
	return (TmpOrdinal >= 0x80000000 && TmpOrdinal <= 0x8000FFFF) ? TRUE : FALSE;
}

DWORD RVA2Offset(DWORD aRVA)
{
	int                     i;
	PIMAGE_SECTION_HEADER   pSectionh2;

	pSectionh2 = pSectionh;
	i = 0;
    do
	{
		if (aRVA >= pSectionh2->VirtualAddress && 
			aRVA <= (pSectionh2->VirtualAddress + pSectionh2->Misc.VirtualSize - 1))
			break;	
		++i;
		++pSectionh2;
	} while (i < pPeh->FileHeader.NumberOfSections);

	return aRVA - pSectionh2->VirtualAddress + pSectionh2->PointerToRawData;
}

/*
DWORD Offset2RVA(DWORD aOffset)
{
	int                     i;
	PIMAGE_SECTION_HEADER   pSectionh2;

	pSectionh2 = pSectionh;
	i = 0;
    do
	{
		if (aOffset >= pSectionh2->PointerToRawData && 
			aOffset <= (pSectionh2->PointerToRawData + pSectionh2->SizeOfRawData - 1))
			break;	
		++i;
		++pSectionh2;
	} while (i < pPeh->FileHeader.NumberOfSections);

	return aOffset - pSectionh2->PointerToRawData + pSectionh2->VirtualAddress;
}
*/

VOID SSAPIPROC SuspendProcess()
{
	UINT i;

	// suspend all thread of the target process
	for (i=0; i < dwThreadCount; i++)
		SuspendThread(ThreadList[i].hThread);

	// update the GUI
	EnableMenuItem(hMenu,ID_CONTINUEDEBUG,MF_ENABLED);
	EnableMenuItem(hMenu,ID_SUSPENDALL,MF_GRAYED);
	SendMessage(
		(HWND)hTB,
		TB_HIDEBUTTON,
		(WPARAM)ID_CONTINUEDEBUG,
		(LPARAM)MAKELONG(FALSE,0));
	SendMessage(
		(HWND)hTB,
		TB_HIDEBUTTON,
		(WPARAM)ID_SUSPENDALL,
		(LPARAM)MAKELONG(TRUE,0));
	// focus -> SS
	SetForegroundWindow(hDlg_);
	return;
}

VOID SSAPIPROC ResumeProcess()
{
	UINT i;

	// resume all thread of the target process
	for (i=0; i < dwThreadCount; i++)
		ResumeThread(ThreadList[i].hThread);

	// update the GUI
	EnableMenuItem(hMenu, ID_CONTINUEDEBUG, MF_GRAYED);
	EnableMenuItem(hMenu, ID_SUSPENDALL, MF_ENABLED);
	EnableMenuItem(hMenu, ID_APISTACKMOD, MF_GRAYED);
	SendMessage(
		(HWND)hTB,
		TB_HIDEBUTTON,
		(WPARAM)ID_CONTINUEDEBUG,
		(LPARAM)MAKELONG(TRUE,0));
	SendMessage(
		(HWND)hTB,
		TB_HIDEBUTTON,
		(WPARAM)ID_SUSPENDALL,
		(LPARAM)MAKELONG(FALSE,0));
	SendMessage(
		(HWND)hTB,
		TB_SETSTATE,
		ID_APISTACKMOD,
		(LPARAM)MAKELONG(TBSTATE_INDETERMINATE,0));

	// resume debug thread if needed
	ResumeThread(hDebugThread);
	return;
}

DWORD EspToTID(DWORD dwESP)
{
#define MAX_ESP_DIFFERENCE 0x100
	DWORD   i;
	CONTEXT c    = {CONTEXT_CONTROL};

	for (i=0; i < dwThreadCount; i++)
	{
		if (GetThreadContext(ThreadList[i].hThread, &c))
			if (c.Esp > dwESP - MAX_ESP_DIFFERENCE &&
				c.Esp < dwESP + MAX_ESP_DIFFERENCE
				)
				return ThreadList[i].dwThreadID;
	}

	return 0;
}

HANDLE GetThreadHandle(DWORD dwThreadID)
{
	int       iCurrentThread;

	for (iCurrentThread=0; (DWORD)iCurrentThread < dwThreadCount; iCurrentThread++)
		if ((DWORD)ThreadList[iCurrentThread].dwThreadID == dwThreadID)
			break;

	return ThreadList[iCurrentThread].hThread;
}

BOOL CanTrapDll(DWORD iDll)
{
	char cDllName[MAX_DLLNAMELENGTH];

	if (!Option.bDllNoTrapList)
		return TRUE;

	// check dll bad list
	lstrcpy(cDllName, DllNames[iDll]);
	CharUpper(cDllName);
	if (strstr(Option.cDllNoTrapList, cDllName))
		return FALSE;

	return TRUE; // ApiSnoop.dll can trap the dll
}

BOOL ShouldIgnoreAPI(int   iArrayIndex,
					 BOOL  bCheckBPX)    // should get checked if API returns
{
	//int       i;
	CHAR*     pCH;
	DWORD     TargetAPI;

	TargetAPI = dwFunctNames[iArrayIndex];

	// check whether there's a "Breakpoint" :) on this API
	if (bCheckBPX)
		if (!IsOrdinal(TargetAPI))
		{
			pCH = (CHAR*)TrapApiNameBuff;
			while (*pCH != 0)
			{
				if(lstrcmpi(pCH,(PSTR)(TargetAPI + dwMemBase)) == 0)
				{
					SuspendProcess();
					// enable the continue debug buttons
					SendMessage(
						(HWND)hTB,
						TB_HIDEBUTTON,
						(WPARAM)ID_CONTINUEDEBUG,
						(LPARAM)MAKELONG(FALSE,0));
					EnableMenuItem(hMenu, ID_CONTINUEDEBUG, MF_ENABLED);
					// enable the stack mod buttons
					EnableMenuItem(hMenu, ID_APISTACKMOD, MF_ENABLED);
					SendMessage(
						(HWND)hTB,
						TB_SETSTATE,
						ID_APISTACKMOD,
						(LPARAM)MAKELONG(TBSTATE_ENABLED,0));
					// report the BP occurrence
					wsprintf(buff,"BPX on \"%s\" reached...",pCH);
					UpdateStatus(buff);
				}
				pCH += strlen(pCH);
				++pCH;
			}
		}

	//// check the dll of the Api
	//if (Option.dwToIgnoreDlls != NO_SPECIAL_DLLS)
	//	for (i=0 ;i<Option.wIgnoreDllNum; i++)
	//		if (lstrcmpi((PSTR)DllNames[DllName[iArrayIndex]],(PSTR)Option.cIgnoreDlls[i]) == 0)
	//			if (Option.dwIgnoreDlls == NO_SPECIAL_DLL_CALLS)
	//				return TRUE;
	//			else
	//				return FALSE;
	//if (Option.dwIgnoreDlls == JUST_SPECIAL_DLL_CALLS)
	//	return TRUE;

	//// get to know whether this API call should be listed
	//// check a ordinal import
	//if (IsOrdinal(TargetAPI))
	//	if (Option.bReportOrdinalAPI && Option.dwIgnoreAPIs != JUST_CALL_SPECIAL_APIS)
	//		return FALSE;
	//	else
	//		return TRUE;

	//// check a name import
	//if (Option.bReportNameAPI == FALSE)
	//	return TRUE;
	//if (Option.dwIgnoreAPIs == NO_SPECIAL_APIS)
	//	return FALSE;

	//for (i=0; i<Option.wIgnoreAPINum; i++)
	//	if (lstrcmpi((PSTR)(TargetAPI + dwMemBase),(PSTR)Option.cIgnoreAPIs[i]) == 0)
	//		if (Option.dwIgnoreAPIs == DONT_CALL_SPECIAL_APIS)
	//			return TRUE;
	//		else
	//			return FALSE;

	//if (Option.dwIgnoreAPIs == JUST_CALL_SPECIAL_APIS)
	//	return TRUE;

	return FALSE;
}

VOID SetDebugWindowPos()
{
	RECT  r;

	GetWindowRect(hDlg_, &r);
	rOrgWindowPos = r;
	if (IsZoomed(hDlg_))
		return;
	MoveWindow(hDlg_,0,0,r.right-r.left,r.bottom-r.top,TRUE);
	return;
}

VOID SetNormalWindowPos()
{
	MoveWindow(
		hDlg_,
		rOrgWindowPos.left,
		rOrgWindowPos.top,
		rOrgWindowPos.right - rOrgWindowPos.left,
		rOrgWindowPos.bottom - rOrgWindowPos.top,
		TRUE);
	return;
}

VOID SetWindowTop(HWND hDlg)
{
	GetWindowRect(hDlg,&rect);
	SetWindowPos(
		hDlg,
		HWND_TOPMOST,
		rect.left,
		rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		SWP_SHOWWINDOW);
	return;
}

VOID SetWindowNonTop(HWND hDlg)
{
	GetWindowRect(hDlg,&rect);
	SetWindowPos(
		hDlg,
		HWND_NOTOPMOST,
		rect.left,
		rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		SWP_SHOWWINDOW);
	return;
}

CHAR* ExtractFileName(CHAR* szFilePath)
{
	BYTE*  pBy;

	pBy = (BYTE*)((DWORD)szFilePath + lstrlen(szFilePath) - 1);
	while (*pBy != 0 && *pBy != '\\')
		--pBy;
	++pBy;
	return (CHAR*)pBy;
}

VOID Handle_ID_SAVETOFILE(DWORD dwListItemNum)
{
	char         cItemCaption[MAX_LISTITEMCAPTIONLENGTH];
	HANDLE       hSaveFile;
	int          iCaptionLength;
	DWORD        dwListCount;

	// let the user selected a file
	MakeOfn(ofn_save);
	ofn_save.hInstance = hInst;
	ofn_save.hwndOwner = hDlg_;
	ofn_save.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	strcpy(cSaveFileBuff,"debug.txt");
	ofn_save.lpstrFile = cSaveFileBuff;
	ofn_save.lpstrTitle = "Save list to...";
	ofn_save.lpstrFilter = "txt files\0*.txt\0all files\0*.*\0";
	if (!GetSaveFileName(&ofn_save))
		return;
	// open the file
	hSaveFile = CreateFile(cSaveFileBuff,GENERIC_READ | GENERIC_WRITE, \
						   0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
	if (!hSaveFile)
	{
		ShowError("File I/O - Error !");
		return;
	}
	SetFilePointer(hSaveFile,0,0,FILE_BEGIN);
	// write the listbox strings into the file
	for (dwListCount=0; dwListCount < dwListItemNum; dwListCount++)
	{
		// check the item length
		iCaptionLength = SendMessage(
			hLBDbg,
			LB_GETTEXTLEN,
			dwListCount,
			0);
		if (iCaptionLength > MAX_LISTITEMCAPTIONLENGTH-1)
			continue;
		// get the text and write it into the file
		iCaptionLength = SendMessage(
			hLBDbg,
			LB_GETTEXT,
			MAKEWPARAM(dwListCount,0),
			(LPARAM)(LPCTSTR)cItemCaption);
		WriteFile(hSaveFile,&cItemCaption,iCaptionLength,&dwBytesWritten, \
			NULL);
		// write the end of the line
		WriteFile(hSaveFile,&wLineEnd,2,&dwBytesWritten,NULL);
	}
	MessageBox(hDlg_,"File saved successfully !",":)",MB_ICONINFORMATION);
	CloseHandle(hSaveFile); // clean up
	return;
}

VOID SSAPIPROC StartDebugThread()
{
	hDebugThread = CreateThread(NULL,0,ProcessFile,NULL,0,&dwThreadID);
	return;
}

BOOL DoAPIStuff() /* get the function-pointers/oridnals of the Import Table (RAW MODE) */
{
	__try
	{
		UpdateStatus("Collecting Import datas...");
		pSectionh = (PIMAGE_SECTION_HEADER)((DWORD)pMap + 0xF8 + pDosh->e_lfanew);
		if (!pPeh->OptionalHeader.DataDirectory[1].VirtualAddress)
			return TRUE; // return TRUE if no IT !
		pIID = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)pMap + 
			   RVA2Offset(pPeh->OptionalHeader.DataDirectory[1].VirtualAddress));
		dwItemNum = 0;
		int iDllCount = 0;
		while (pIID->FirstThunk != 0) // for each Image Import Descriptor
		{
			strcpy(DllNames[iDllCount],(char*)(RVA2Offset(pIID->Name) + (DWORD)pMap));
			if (pIID->OriginalFirstThunk == 0)
				pDW = (DWORD*)(RVA2Offset(pIID->FirstThunk) + (DWORD)pMap);
			else
				pDW = (DWORD*)(RVA2Offset(pIID->OriginalFirstThunk) + (DWORD)pMap);
			while (*pDW != 0) // for each First Thunk member
			{
				if (IsOrdinal(*pDW))
					dwFunctNames[dwItemNum] = *pDW;
				else
					dwFunctNames[dwItemNum] = *pDW + 2;
				DllName[dwItemNum] = iDllCount;
				++dwItemNum;
				++pDW;
			}
			++pIID;
			++iDllCount;
		}
		dwAPINum = dwItemNum;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL ProcessBPXList()
{
	HANDLE   hProc;
	INT      i;
	BOOL     bSuccess,bAllDone = TRUE;
	DWORD    dwBytesWritten,dwBytesRead,dwOldProt,dwNewProt;
	CHAR     cErrorMsg[9 * 30 + 20] = "Couldn't set BP(s) to:", cHexNumBuff[10];

	hProc = OpenProcess(
		PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_VM_OPERATION,
		FALSE,
		PI.dwProcessId);
	for(i=0; i<iBPXCnt; i++)
	{
		bSuccess = TRUE;
		// get page access
		VirtualProtectEx(
			hProc,
			BPXInfo[i].pVA,
			1,
			PAGE_READWRITE,
			&dwOldProt);
		// save the original byte
		bSuccess &= ReadProcessMemory(
			hProc,
			BPXInfo[i].pVA,
			(VOID*)&BPXInfo[i].byOrg,
			1,
			&dwBytesRead);
		// write a CC
		if (bSuccess)
			bSuccess &= WriteProcessMemory(
			hProc,
			BPXInfo[i].pVA,
			(VOID*)&Int3,
			1,
			&dwBytesWritten);
		// restore page access
		VirtualProtectEx(
			hProc,
			BPXInfo[i].pVA,
			1,
			dwOldProt,
			&dwNewProt);
		if (!bSuccess)
		{
			bAllDone = FALSE;
			wsprintf((PSTR)cHexNumBuff,"\n%X",(DWORD)BPXInfo[i].pVA);
			strcat((PSTR)cErrorMsg,(PSTR)cHexNumBuff);			
		}
	}
	// show errors
	if (!bAllDone)
		MessageBox(hDlg_,(PSTR)cErrorMsg,"Access error",MB_ICONWARNING);
	// clean up
	CloseHandle(hProc);
	if (bAllDone)
		return TRUE;
	else
		return FALSE;
}

BOOL WriteProcessByte(HANDLE hProc, void* pAddr, BYTE bToWrite, BYTE* pbOldByte)
{
	DWORD                     dwOldProt, dwBuff;
	BOOL                      bRet;

	// get write access
	if (!VirtualProtectEx(hProc, pAddr, 1, PAGE_EXECUTE_READWRITE, &dwOldProt))
		return FALSE;

	// read byte
	if (!ReadProcessMemory(hProc, pAddr, pbOldByte, 1, &dwBuff))
		return FALSE;
	// write byte
	bRet = WriteProcessMemory(hProc, pAddr, &bToWrite, 1, &dwBuff);

	// restore page access
	VirtualProtectEx(hProc, pAddr, 1, dwOldProt, &dwBuff);

	return bRet;
}

BOOL SetThreadEip(HANDLE hThread, DWORD dwEip)
{
	CONTEXT c = {CONTEXT_CONTROL};

	if (!GetThreadContext(hThread, &c))
		return FALSE;
	c.Eip = dwEip;
	if (!SetThreadContext(hThread, &c))
		return FALSE;
	return TRUE;
}

BOOL GetFilehandleDllName(HANDLE HFILE,CHAR* szLibName,int iBuffSize)
{
	PIMAGE_DOS_HEADER          pDosh;
	PIMAGE_NT_HEADERS          pPeh;
	PIMAGE_EXPORT_DIRECTORY    pExp;
	PIMAGE_SECTION_HEADER      pSec,pSearchSec;
	HANDLE                     hMap;
	VOID                       *pMap;
	CHAR*                      szDllName;
	int                        i;

	// map the file
	hMap = CreateFileMapping (HFILE,NULL,PAGE_READONLY,0,0,NULL);
	if (!hMap)
		return FALSE;
	pMap = MapViewOfFile (hMap,FILE_MAP_READ,0,0,0);
	CloseHandle(hMap);
	if (!pMap)
		return FALSE;

	// start the action :)
	__try
	{
		// get the header pointers
		pDosh = (PIMAGE_DOS_HEADER)pMap;
		if (pDosh->e_magic != IMAGE_DOS_SIGNATURE)
			goto BadExit;
		pPeh = (PIMAGE_NT_HEADERS)((DWORD)pMap + pDosh->e_lfanew);
		if (pPeh->Signature != IMAGE_NT_SIGNATURE)
			goto BadExit;
		pSec = (PIMAGE_SECTION_HEADER)((DWORD)pMap + pDosh->e_lfanew + 0xF8);
		// is there an export directory
		if (pPeh->OptionalHeader.DataDirectory[0].VirtualAddress == 0)
			goto BadExit;
		// get the dll name from the export dir
		pSearchSec = ImageRvaToSection(
			pPeh,
			pMap,
			pPeh->OptionalHeader.DataDirectory[0].VirtualAddress);
		if (!pSearchSec)
			goto BadExit;
		pExp = (PIMAGE_EXPORT_DIRECTORY)(
			(DWORD)pMap +
			pPeh->OptionalHeader.DataDirectory[0].VirtualAddress -
			pSearchSec->VirtualAddress +
			pSearchSec->PointerToRawData);
		pSearchSec = ImageRvaToSection(
			pPeh,
			pMap,
			pExp->Name);
		if (!pSearchSec)
			goto BadExit;
		szDllName = (CHAR*)(
			(DWORD)pMap +
			pExp->Name -
			pSearchSec->VirtualAddress +
			pSearchSec->PointerToRawData);
		// copy the name into the buffer
		i = iBuffSize;
		ZeroMemory(szLibName,i);
		--i;
		for(; i>0; i--)
		{
			*szLibName = *szDllName;
			++szLibName;
			++szDllName;
			if (*szDllName == 0)
				break;
		}		
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		goto BadExit;
	}
	UnmapViewOfFile(pMap);
	return TRUE;

BadExit:
	UnmapViewOfFile(pMap);
	return FALSE;
}

// return values:
// 1  - success
// 0  - not our Int3
// -1 - couldn't restore original byte
DWORD HandleInt3(DWORD PID,HANDLE hThread,DWORD dwInt3VA)
{
	int           i;
	HANDLE        hProc;
	DWORD         dwBytesWritten,dwOldProt,dwNewProt;
	CONTEXT       Regs;

	ZeroMemory(&Regs,sizeof(Regs));
	Regs.ContextFlags = CONTEXT_FULL;

	// get a valid process handle
	hProc = OpenProcess(
		PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_VM_OPERATION,
		FALSE,
		PI.dwProcessId);
	if (!hProc)
		return 0;

	// is this a custom BP ?
	for(i=0; i<iBPXCnt; i++)
		if((DWORD)BPXInfo[i].pVA == dwInt3VA)
		{
			// get page access
			VirtualProtectEx(
				hProc,
				BPXInfo[i].pVA,
				1,
				PAGE_READWRITE,
				&dwOldProt);
			// restore the orginal byte
			if (!WriteProcessMemory(
				hProc,
				BPXInfo[i].pVA,
				&BPXInfo[i].byOrg,
				1,
				&dwBytesWritten))
			{
				CloseHandle(hProc);
				return -1;
			}
			// restore page access
			VirtualProtectEx(
				hProc,
				BPXInfo[i].pVA,
				1,
				dwOldProt,
				&dwNewProt);
			// correct Eip
			GetThreadContext(hThread,&Regs);
			if (Option.bRestoreBP) // add to restore list and set trap flag
			{
				if (icBPRest != MAX_BPRESTORENUM)
				{
					BPRestAddr[icBPRest++] = BPXInfo[i].pVA;
					Regs.EFlags |= 0x100; // set TF
				}
			}			
			Regs.Eip = dwInt3VA;
			if (!SetThreadContext(hThread,&Regs))
			{
				CloseHandle(hProc);
				return -1;
			}
			// report about the BP
			SuspendProcess();
			wsprintf(StatusBuff,"BPX at %X reached...",(DWORD)BPXInfo[i].pVA);
			UpdateStatus((PSTR)StatusBuff);
			// quit
			CloseHandle(hProc);
			WaitForUser();
			return 1;
		}
	// clean up
	CloseHandle(hProc);
	return 0;
}

BOOL RestoreBPX()
{
	BOOL   bRet    = TRUE;
	DWORD  dwBytesWritten, dwOldProt, dwBuff, i;
	HANDLE hProc;

	hProc = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_VM_OPERATION,
		FALSE, PI.dwProcessId);
	for (i=0; i < (DWORD)icBPRest; i++)
	{
		// reset CCh
		VirtualProtectEx(hProc, BPRestAddr[i], 1, PAGE_EXECUTE_READWRITE, &dwOldProt);
		bRet &= WriteProcessMemory(hProc, BPRestAddr[i], (void*)&Int3, 1, &dwBytesWritten);
		VirtualProtectEx(hProc, BPRestAddr[i], 1, dwOldProt, &dwBuff);
	}
	icBPRest = 0;

	// clean up
	CloseHandle(hProc);
	return bRet;
}

VOID PerformDebugCleanup()
{
	icBPRest = 0;

	// update the GUI
	SendMessage(
		(HWND)hTB,
		TB_HIDEBUTTON,
		(WPARAM)ID_CONTINUEDEBUG,
		(LPARAM)MAKELONG(TRUE,0));
	SendMessage(
		(HWND)hTB,
		TB_HIDEBUTTON,
		(WPARAM)ID_SUSPENDALL,
		(LPARAM)MAKELONG(FALSE,0));
	EnableMenuItem(hMenu,ID_CONTINUEDEBUG,MF_GRAYED);
	EnableMenuItem(hMenu,ID_SUSPENDALL,MF_ENABLED);
	UpdateGUI(hDlg_, FALSE);

	// output file
	if (Option.bFileOutPut && hOutPutFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hOutPutFile);
		hOutPutFile = NULL;
	}

	// close handles
	if (PI.hProcess!=NULL)
	{
		CloseHandle(PI.hProcess);
		CloseHandle(PI.hThread);
		CloseHandle(hDebugThread);
		PI.hProcess = NULL;
		PI.hThread = NULL;
		hDebugThread = NULL;
	}

	// add by ygq
	if (dpMappedMem!=NULL)
	{
		*dpMappedMem = SS_CMD_STOP;
		SetEvent(hCmdSignal);
		Sleep(100);
	}
	UnInitMapMem();

	// inform plugin windows
	PostPluginWindowMessage(SS_DEBUGSTOP, 0, 0);
	return;
}

void CleanupThreadList()
{
	DWORD  i,iIndex,dwExitCode;
	CHAR   cBuff[80];

	// clean dead threads out of the ThreadList
	iIndex = 0;
	for (i=0; i < dwThreadCount; i++)
	{
		GetExitCodeThread(ThreadList[i].hThread,&dwExitCode);
		if (i==0 ||                      // main thread seems to be always already dead :)
			dwExitCode == STILL_ACTIVE)
		{
			ThreadList[iIndex].dwThreadID = ThreadList[i].dwThreadID;
			ThreadList[iIndex].hThread = ThreadList[i].hThread;
			++iIndex;
		}
		else
			if (Option.bShowDebugEvents)  // report about the killed thread
			{
				wsprintf(
					cBuff,
					"Thread exited...ID: %Xh  ExitCode: %u (%Xh)",
					ThreadList[i].dwThreadID,
					dwExitCode,
					dwExitCode);
				Add(cBuff);
			}
	}
	dwThreadCount = iIndex;
	return;
}

void WaitForUser()
{
	// It's needed to suspend the debug thread so that "ContinueDebugEvent" is
	// not called until the user edited registers,... !!!
	SuspendThread(hDebugThread);
	return;
}

VOID StartDebug()
{
	static int    iBPCount;
	static BOOL   bEntryTrap, bLogTID;
	static BYTE   byOrgEntryByte;
	DWORD         dwContFlag;
	BYTE          byBuff;
	char          cBuff[200];

	iBPCount = 0;
	bEntryTrap = FALSE;
	dwLastEventTID = dwCurEventTID = 0;
	bLogTID = FALSE;
	while (WaitForDebugEvent(&dbevent,INFINITE)) // DEBUG LOOP
	{
		// process plugin window messages
		PostPluginWindowMessage(SS_DEBUGEVENT, (WPARAM)&dbevent, 0);
		if (bPluginHandler)
			ContinueDebugEvent(dbevent.dwProcessId, dbevent.dwThreadId, DBG_CONTINUE);

		// set default continue flag
		dwContFlag = DBG_EXCEPTION_NOT_HANDLED;

		// save TID of event
		if (bLogTID)
			dwCurEventTID = dbevent.dwThreadId;

		switch(dbevent.dwDebugEventCode)
		{
		case CREATE_PROCESS_DEBUG_EVENT:
			dwEntryPointVA = (DWORD)dbevent.u.CreateProcessInfo.lpStartAddress;
			// save the infos of the main thread
			ThreadList[dwThreadCount].dwThreadID = dbevent.dwThreadId;
			ThreadList[dwThreadCount].hThread    = PI.hThread;
			++dwThreadCount;
			// show some infos about the new process
			if (Option.bShowDebugEvents)
			{
				wsprintf(buff,"Debug process created...");
				Add(buff);
				wsprintf(buff,"-PID: %Xh  process handle: %Xh",PI.dwProcessId, \
						dbevent.u.CreateProcessInfo.hProcess);
				Add(buff);
				wsprintf(buff,"-TID: %Xh  thread handle %Xh",PI.dwThreadId, \
						 dbevent.u.CreateProcessInfo.hThread);
				Add(buff);
			}
			// set Int3 breakpoints
			ProcessBPXList();
			// set TID variables
			dwCurEventTID = dbevent.dwThreadId;
			dwLastEventTID = 0xFFFFFFFF;
			bLogTID = TRUE;

			// Handle Int3 at EntryPoint
			if (Option.bStopAtEntry)
			{
				bEntryTrap = WriteProcessByte(PI.hProcess, (void*)dwEntryPointVA,
					Int3, &byOrgEntryByte);
				if (!bEntryTrap)
					ShowError("Failed to trap EntryPoint !");
			}
			break;

		case EXCEPTION_DEBUG_EVENT:
			switch(dbevent.u.Exception.ExceptionRecord.ExceptionCode)
			{
			case EXCEPTION_BREAKPOINT: // an int3 (0xCC) was found
				{
					dwContFlag = DBG_CONTINUE;

					if ((DWORD)dbevent.u.Exception.ExceptionRecord.ExceptionAddress == dwEntryPointVA)
					{
						// breakpoint at entry point
						if (Option.bStopAtEntry && bEntryTrap)
						{
							if (!SetThreadEip(PI.hThread, dwEntryPointVA) ||
								!WriteProcessByte(PI.hProcess, (void*)dwEntryPointVA, byOrgEntryByte,
								&byBuff))
							{
								ShowError("Failed to restore byte at EntryPoint !!!");
							}
							else
							{
								// report EntryPoint reached
								SuspendProcess();
								UpdateStatus("EntryPoint reached...");
								PostPluginWindowMessage(SS_ENTRYREACHED, 0, 0);
								if (!bPluginHandler)
									WaitForUser();
							}
							break;
						}
					} else
					{
						// get the right thread handle out of our list
						hCurrThread = GetThreadHandle(dbevent.dwThreadId);

						Regs.ContextFlags = CONTEXT_CONTROL;
						GetThreadContext(hCurrThread,&Regs);
						Regs.Eip -= 1;

						switch(HandleInt3(PI.dwProcessId,hCurrThread,Regs.Eip))
						{
						case 0:
							// continue anyway ?
							wsprintf(
								buff,
								"User define BreakPoint found at %Xh !\nContinue anyway ?",
								Regs.Eip);
							if (MessageBox(
								0,
								buff,
								"BP found",
								MB_ICONWARNING | MB_YESNO | MB_TOPMOST) == IDNO)
								SendMessage(hDlg_,WM_COMMAND,MAKEWPARAM(ID_STOPDEBUG,0),NULL);
							break;

						case -1:
							SuspendProcess();
							ShowError("Couldn't clean up BPX !!!");
							WaitForUser();
							break;

						}
						break;
					}
				}
				break;

			case EXCEPTION_SINGLE_STEP:
				if (Option.bRestoreBP)
					if (!RestoreBPX())
						ShowError("Couldn't restore at least one BPX !");
				HandleBPM(dbevent.dwThreadId);
				dwContFlag = DBG_CONTINUE;		// a must for win2k
				break;

			case EXCEPTION_ACCESS_VIOLATION:
				if (Option.bHandleExceptions)
					dwContFlag = DBG_CONTINUE;
				hCurrThread = GetThreadHandle(dbevent.dwThreadId);
				Regs.ContextFlags = CONTEXT_CONTROL;
				GetThreadContext(hCurrThread,&Regs);
				wsprintf(buff,"!!! Access violation at address %08Xh",Regs.Eip);
				Add(buff);
				break;

			// some other exception events (ripped from an debugger example 
			// of the MSDN library)
			case EXCEPTION_DATATYPE_MISALIGNMENT:
				if (Option.bHandleExceptions)
					dwContFlag = DBG_CONTINUE;
				if (Option.bShowDebugEvents)
					Add("!!! Exception: Datatype Misalignment");
				break;

			case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
				if (Option.bHandleExceptions)
					dwContFlag = DBG_CONTINUE;
				if (Option.bShowDebugEvents)
					Add("!!! Exception: Array Bound Exceeded");
				break;

			case EXCEPTION_FLT_DENORMAL_OPERAND:
				if (Option.bHandleExceptions)
					dwContFlag = DBG_CONTINUE;
				if (Option.bShowDebugEvents)
					Add("!!! Exception: Floating Point - Denormal Operand");
				break;

			case EXCEPTION_FLT_DIVIDE_BY_ZERO:
				if (Option.bHandleExceptions)
					dwContFlag = DBG_CONTINUE;
				if (Option.bShowDebugEvents)
					Add("!!! Exception: Floating Point - Divide By Zero");
				break;

			case EXCEPTION_FLT_INEXACT_RESULT:
				if (Option.bHandleExceptions)
					dwContFlag = DBG_CONTINUE;
				if (Option.bShowDebugEvents)
					Add("!!! Exception: Floating Point - Inexact Result");
				break;

			case EXCEPTION_FLT_INVALID_OPERATION:
				if (Option.bHandleExceptions)
					dwContFlag = DBG_CONTINUE;
				if (Option.bShowDebugEvents)
					Add("!!! Exception: Floating Point - Invalid Operation");
				break;

			case EXCEPTION_FLT_OVERFLOW:
				if (Option.bHandleExceptions)
					dwContFlag = DBG_CONTINUE;
				if (Option.bShowDebugEvents)
					Add("!!! Exception: Floating Point - Overflow");
				break;

			case EXCEPTION_FLT_STACK_CHECK:
				if (Option.bHandleExceptions)
					dwContFlag = DBG_CONTINUE;
				if (Option.bShowDebugEvents)
					Add("!!! Exception: Floating Point - Stack Check");
				break;

			case EXCEPTION_FLT_UNDERFLOW:
				if (Option.bHandleExceptions)
					dwContFlag = DBG_CONTINUE;
				if (Option.bShowDebugEvents)
					Add("!!! Exception: Floating Point - Underflow");
				break;

			case EXCEPTION_INT_DIVIDE_BY_ZERO:
				if (Option.bHandleExceptions)
					dwContFlag = DBG_CONTINUE;
				if (Option.bShowDebugEvents)
					Add("!!! Exception: Integer - Divide By Zero");
				break;

			case EXCEPTION_INT_OVERFLOW:
				if (Option.bHandleExceptions)
					dwContFlag = DBG_CONTINUE;
				if (Option.bShowDebugEvents)
					Add("!!! Exception: Integer - Overflow");
				break;

			case EXCEPTION_PRIV_INSTRUCTION:
				if (Option.bHandleExceptions)
					dwContFlag = DBG_CONTINUE;
				if (Option.bShowDebugEvents)
					Add("!!! Exception: Privileged Instruction"); 
				break;

			case EXCEPTION_IN_PAGE_ERROR:
				if (Option.bHandleExceptions)
					dwContFlag = DBG_CONTINUE;
				if (Option.bShowDebugEvents)
					Add("!!! Exception: In Page Error");
				break;
			}
			break;

		case OUTPUT_DEBUG_STRING_EVENT:
			if (!Option.bShowDebugEvents)
				break;
			// read the string in the process address space
			if (ReadProcessMemory(PI.hProcess,(PSTR)dbevent.u.DebugString.lpDebugStringData, \
				DebugStringBuff,dbevent.u.DebugString.nDebugStringLength,&dwBytesRead))
			{
			    // 0 terminate the msg
			    DebugStringBuff[dbevent.u.DebugString.nDebugStringLength] = 0;
				// format and show it
				wsprintf(buff,"Debug msg: \"%s\"",DebugStringBuff);
				Add(buff);
			}
			break;

		case EXIT_PROCESS_DEBUG_EVENT: // the debuggee exits
			wsprintf(
				buff,
				"!!! Debuggee exited...TID: %Xh  ExitCode: %u (%Xh)",
				PI.dwThreadId,
				dbevent.u.ExitProcess.dwExitCode,
				dbevent.u.ExitProcess.dwExitCode);
			Add(buff);
			STOP = TRUE;
			break;

		// Dll events
		case LOAD_DLL_DEBUG_EVENT:
			if (!Option.bShowDebugEvents)
				break;
			// show the dll name
			if (dbevent.u.LoadDll.lpImageName != 0 && !IsBadReadPtr(dbevent.u.LoadDll.lpImageName, 4) &&
				*(DWORD*)dbevent.u.LoadDll.lpImageName != 0)
			{
				lstrcpy(cBuff, (char*)*(DWORD*)dbevent.u.LoadDll.lpImageName);
				CharLower(cBuff);
				wsprintf(
					buff,
					"Dll loaded...%s  Base: %Xh",
					&cBuff,
					dbevent.u.LoadDll.lpBaseOfDll);
				Add(buff);
				break;
			}
			// ELSE dump the header of the dll and trace its export table to get the dll name
			if (!GetFilehandleDllName(
				dbevent.u.LoadDll.hFile,
				(CHAR*)&DllNameBuff,
				sizeof(DllNameBuff)))
				wsprintf(buff,"Dll loaded...Base: %Xh",dbevent.u.LoadDll.lpBaseOfDll);
			else
				wsprintf(buff,"Dll loaded...%s  Base: %Xh",DllNameBuff,dbevent.u.LoadDll.lpBaseOfDll);
			Add(buff);
			break;

		case UNLOAD_DLL_DEBUG_EVENT:
			if (!Option.bShowDebugEvents)
				break;
			wsprintf(buff,"Dll unloaded...Base: %Xh",dbevent.u.UnloadDll.lpBaseOfDll);
			Add(buff);
			break;

		// Thread events
		case CREATE_THREAD_DEBUG_EVENT:
			// the the infos of the new thread
			ThreadList[dwThreadCount].dwThreadID = dbevent.dwThreadId;
			ThreadList[dwThreadCount].hThread    = dbevent.u.CreateThread.hThread;
			++dwThreadCount;
			// show some infos
			if (!Option.bShowDebugEvents)
				break;
			wsprintf(buff,"Thread created...ID: %Xh  Handle: %Xh  StartAddress: %Xh", \
				     dbevent.dwThreadId,dbevent.u.CreateThread.hThread, \
					 dbevent.u.CreateThread.lpStartAddress);
			Add(buff);
			break;

		case EXIT_THREAD_DEBUG_EVENT:
			CleanupThreadList();
			break;
			}
			
		if (!STOP)
			ContinueDebugEvent(dbevent.dwProcessId,dbevent.dwThreadId,dwContFlag);
		else 
			break;
	} // end of the Debug loop
	return;
}

void UpdateGUI(HWND hW, BOOL bDebugging)
{
	TBBUTTONINFO BI, myBI;
	BOOL         bOn;

	// init structs
	memset(&BI, 0, sizeof(BI));
	BI.cbSize        = sizeof(BI);
	BI.dwMask        = TBIF_STATE;
	BI.fsState       = bDebugging ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE;
	memset(&myBI, 0, sizeof(myBI));
	myBI.cbSize      = sizeof(myBI);
	myBI.dwMask      = TBIF_STATE;

#define SM  SendMessage
#define EMI EnableMenuItem
#define bD  bDebugging
	// open
	EMI(hMenu, ID_OPEN, bDebugging ? MF_GRAYED : MF_ENABLED);
	myBI.fsState = bD ? TBSTATE_INDETERMINATE : TBSTATE_ENABLED;
	SM((HWND)hTB, TB_SETBUTTONINFO, ID_OPEN, (LPARAM)&myBI);
	if (*(char*)Option.cLastFile1)
		EMI(hMenu, ID_FILE_REOPEN, bDebugging ? MF_GRAYED : MF_ENABLED);

	// restart dbg
	bOn = !bDebugging && szFname[0];
	EMI(hMenu, ID_RESTARTDEBUG,
		bOn ? MF_ENABLED : MF_GRAYED);
	myBI.fsState = bOn ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE;
	SM((HWND)hTB, TB_SETBUTTONINFO, ID_RESTARTDEBUG, (LPARAM)&myBI);

	// stop dbg
	EMI(hMenu, ID_STOPDEBUG, bD ? MF_ENABLED : MF_GRAYED);
	SM((HWND)hTB, TB_SETBUTTONINFO, ID_STOPDEBUG, (LPARAM)&BI);

	// freeze process
	EMI(hMenu, ID_SUSPENDALL, bD ? MF_ENABLED : MF_GRAYED);
	SM((HWND)hTB, TB_SETBUTTONINFO, ID_SUSPENDALL, (LPARAM)&BI);

	//--- actions.... ---
	// dump
	EMI(hMenu, ID_DUMP, bD ? MF_ENABLED : MF_GRAYED);
	SM((HWND)hTB, TB_SETBUTTONINFO, ID_DUMP, (LPARAM)&BI);

	// BPX
	//EMI(hMenu, ID_SETBPX, bD ? MF_ENABLED : MF_GRAYED);
	//SM((HWND)hTB, TB_SETBUTTONINFO, ID_SETBPX, (LPARAM)&BI);

	// BPM
	EMI(hMenu, ID_SETBPM, bD ? MF_ENABLED : MF_GRAYED);
	SM((HWND)hTB, TB_SETBUTTONINFO, ID_SETBPM, (LPARAM)&BI);

	// Set Regs
	// Memory Server
	EMI(hMenu, ID_VIEWREGS, bD ? MF_ENABLED : MF_GRAYED);
	SM((HWND)hTB, TB_SETBUTTONINFO, ID_VIEWREGS, (LPARAM)&BI);
	
	// Memory Server
	EMI(hMenu, ID_MEMSERVER, bD ? MF_ENABLED : MF_GRAYED);
	SM((HWND)hTB, TB_SETBUTTONINFO, ID_MEMSERVER, (LPARAM)&BI);

	// PE Editor
	bOn = (*(char*)szFname != 0) ? TRUE : FALSE;
	myBI.fsState = bOn ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE;
	EMI(hMenu, ID_SHOWPEINFO, bOn ? MF_ENABLED : MF_GRAYED);
	SM((HWND)hTB, TB_SETBUTTONINFO, ID_SHOWPEINFO, (LPARAM)&myBI);

	// Module viewer
	EMI(hMenu, ID_SHOWMODULES, bD ? MF_ENABLED : MF_GRAYED);
	SM((HWND)hTB, TB_SETBUTTONINFO, ID_SHOWMODULES, (LPARAM)&BI);

#undef bD
#undef EMI
#undef SM
	return;
}


LRESULT CALLBACK WndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static DWORD        dwLastApiStackBase   = 0;
	static BOOL         bPlugInsLoaded       = FALSE;
	BOOL                bMod;
	DWORD               dwListItemNum, dwTID, dwNewRetVal;
	HDROP               hDrop;
	int                 i,iTBHeight,iStatusHeight,iWndHeight,iWndWidth;
	RECT                rect;
	MSGBOXPARAMS        MsgParams;
	CHAR                *szApiWithParams;
	APIINFO             ApiInfo;
	APIRETURNINFO*      pARI;
	PCOPYDATASTRUCT     pCDS;
	APIRETURNVALUEINFO  ARVI;

	switch (uMsg)
	{
	    case WM_NOTIFY:
			HandleTBToolTips(lParam);
			break;
		
		case WM_CREATE:
			hDlg_ = hDlg;
			DragAcceptFiles(hDlg,TRUE);
			// load the menu
			hMenu = LoadMenu(hInst,MAKEINTRESOURCE(IDR_MENU2));
			hmLastFiles = CreateMenu();
			SetMenu(hDlg,hMenu);
			EnableMenuItem(hMenu,ID_CONTINUEDEBUG,MF_GRAYED);
			// load toolbar
			hTB = BuildToolBar(hDlg);
			SendMessage(
				(HWND)hTB,
				TB_HIDEBUTTON,
				(WPARAM)ID_CONTINUEDEBUG,
				(LPARAM)MAKELONG(TRUE,0));
			// some option stuff
			if (Option.bScrollList)
				SendMessage(hDlg_,WM_COMMAND,MAKEWPARAM(ID_AUTOSCROLL,0),NULL);
			// initialize Plugin related stuff
			InitPluginStruct(&SSApi);

			if (!SetPrivilege())
				MessageBox(hDlg,"Can't raise process privilege.","Error",MB_ICONERROR);

			HandleCmdLine();
			SetLastFilesMenu();
			UpdateGUI(hDlg, bDebugging);
			break;

		case WM_SHOWWINDOW:
			// must be here else WM_SHOWWINDOW won't be reached for some reasons
			if (Option.bWinTopMost)
				SendMessage(hDlg_,WM_COMMAND,MAKEWPARAM(ID_TOPMOST,0),NULL);
			// load plugins
			if (!bPlugInsLoaded)
			{
				LoadPlugins();
				bPlugInsLoaded = TRUE;
			}
			break;

		case WM_SIZE:
			// resize toolbar
			SendMessage(hTB,TB_AUTOSIZE,0,0);
			// statusbar
			MoveWindow(hStatus,0,0,0,0,TRUE);
			// resize debug list
			GetClientRect(hTB,&rect);
			iTBHeight = rect.bottom;
			GetClientRect(hStatus,&rect);
			iStatusHeight = rect.bottom - rect.top;
			GetClientRect(hDlg_,&rect);
			iWndHeight = rect.bottom;
			iWndHeight -= (iStatusHeight + iTBHeight + 2);
			iWndWidth = rect.right;
			MoveWindow(
				hLBDbg,
				0,
				iTBHeight,
				iWndWidth,
				iWndHeight + 2,
				TRUE);
			break;

		case WM_CLOSE:
			// are we already in a debug session ?
			if (bDebugging)
				if (MessageBox(
					hDlg,
					"You're already in a debugging session !\nAre you sure you want to exit ?",
					"agreement",
					MB_ICONWARNING | MB_DEFBUTTON2 | MB_YESNO) == IDNO)
					return 0;
			// save	the settings in an ini-file
			GetWindowRect(hDlg_,&rect);
			Option.MainWndHeight = rect.bottom - rect.top;
			Option.MainWndWidth = rect.right - rect.left;
			WritePrivateProfileStruct(
				"options",
				"current",
				&Option,
				sizeof(Option),
				(PSTR)IniFilePath);
			// clean up
			DestroyMenu(hMenu);
			DestroyMenu(hmLastFiles);
			DeleteObject(hDbgFont);
			DeleteCriticalSection(&csThreadControl);
			UnmapSSFiles();
			// quite
			PostQuitMessage(0);
			break;

		case WM_DROPFILES: // drag'n'drop support
			hDrop = (HDROP)wParam;
			DragQueryFile(hDrop,0,szFname,sizeof(szFname));
			StartDebugThread();
			DragFinish(hDrop);
			break;

		case WM_APICALL: // APISnoop.dll sent us a nice message
			dwLastApiStackBase = lParam;
			i = wParam;

			// set last event-TID
			dwTID = EspToTID(lParam);
			if (dwTID)
				dwCurEventTID = dwTID;

			// inform plugins
			if (dwPluginWndNum)
			{
				ApiInfo.szDllName = (char*)DllNames[DllName[i]];
				if (IsOrdinal(dwFunctNames[i]))
					ApiInfo.szApiName = (char*)(dwFunctNames[i] - 0x80000000);
				else
					ApiInfo.szApiName = (char*)(dwFunctNames[i] + dwMemBase);
				PostPluginWindowMessage(SS_APICALL, (WPARAM)&ApiInfo, lParam);
			}

			// handle it
			if (ShouldIgnoreAPI(i, TRUE))
				break;

			if (IsOrdinal(dwFunctNames[i]))
				wsprintf(buff,"API: Ordinal %Xh(%s)",(PSTR)dwFunctNames[i] - 0x80000000, \
			            DllNames[DllName[i]]);
			else
			{
				if (Option.bShowApiParams)
				{
					szApiWithParams = GetApiParam(
						DllNames[DllName[i]],
						(CHAR*)dwFunctNames[i] + dwMemBase,
						PI.hProcess,
						(VOID*)lParam);
					if (szApiWithParams)
					{
						Add(szApiWithParams);
						break;
					}
				}
				wsprintf(buff,"API: %s(%s)",(PSTR)dwFunctNames[i] + dwMemBase, \
			             DllNames[DllName[i]]);
			}
			Add(buff);
			return TRUE;

		case WM_APICALL_DYN: // APISnoop.dll sent us a nice message with dynamic api call
			dwLastApiStackBase = lParam;
			i = wParam;

			// set last event-TID
			dwTID = EspToTID(lParam);
			if (dwTID)
				dwCurEventTID = dwTID;

			// inform plugins
			if (dwPluginWndNum)
			{
				ApiInfo.szDllName = (char*)GetModuleName(ApiList[i].ModuleIndex);
				ApiInfo.szApiName = (char*)ApiList[i].ApiName;
				PostPluginWindowMessage(SS_APICALL, (WPARAM)&ApiInfo, lParam);
			}

			if (Option.bShowApiParams)
			{
				szApiWithParams = GetApiParam(
					(char*)GetModuleName(ApiList[i].ModuleIndex),
					(char*)ApiList[i].ApiName,
					PI.hProcess,
					(VOID*)lParam);
				if (szApiWithParams)
				{
					Add(szApiWithParams);
				} else
				{
					DWORD par[5], dwBytesRead;
					DWORD* pPar = (DWORD*)lParam;
					pPar++;
					if (ReadProcessMemory(  // read the value at the current stack position
						PI.hProcess,
						(VOID*)pPar,
						&par,
						5*sizeof(DWORD),
						&dwBytesRead))
					{
						wsprintf(buff,"API: %s(%s) call with params: %x, %x, %x, %x, %x.", ApiList[i].ApiName, \
								GetModuleName(ApiList[i].ModuleIndex), par[0], par[1], par[2], par[3], par[4]);
						Add(buff);
					}
				}
			}
			return TRUE;

		case WM_COPYDATA: // APISnoop.dll sent us a nice message
			pCDS = (COPYDATASTRUCT*)lParam;
			switch(pCDS->dwData)
			{
			case CD_ID_APIRETURN:
				// A API IS RETURNING
				pARI = (APIRETURNINFO*)pCDS->lpData;
				i = pARI->dwApi;

				// inform plugins
				if (dwPluginWndNum)
				{
					ApiInfo.szDllName = (char*)DllNames[DllName[i]];
					if (IsOrdinal(dwFunctNames[i]))
						ApiInfo.szApiName = (char*)(dwFunctNames[i] - 0x80000000);
					else
						ApiInfo.szApiName = (char*)(dwFunctNames[i] + dwMemBase);
					PostPluginWindowMessage(SS_APIRETURN, (WPARAM)&ApiInfo, pARI->dwRetValue);

					ARVI.dwRetVal = pARI->dwRetValue;
					ARVI.pRetVal  = pARI->pRetValue;
					PostPluginWindowMessage(SS_APIRETURNEX, (WPARAM)&ApiInfo, (LPARAM)&ARVI);
				}
				
				// should we show return values and this API ?
				if (!Option.bShowApiReturn)
					break;

				// modify return value ?
				if (!IsOrdinal(dwFunctNames[i]) && !bPluginHandler)
				{
					bMod = HandleRetValMod(
						(char*)(dwFunctNames[pARI->dwApi] + dwMemBase),
						pARI->pRetValue,
						&dwNewRetVal);
				}
				else
					bMod = FALSE;

				// process a modified return value (by plugin)
				if (bPluginHandler)
				{
					bMod = ReadProcessMemory(
						PI.hProcess,
						pARI->pRetValue,
						&dwNewRetVal,
						4,
						&dwBytesRead);
				}

				if (ShouldIgnoreAPI(i, FALSE))
					break;

				// show it
				if (IsOrdinal(dwFunctNames[i]))
				{
					// Ordinal
					wsprintf(
					buff,
					"API: Ordinal %Xh returned: %08lXh",
					(PSTR)dwFunctNames[i] - 0x80000000,
					pARI->dwRetValue);
				}
				else
				{
					// NAME API
					if (bMod)
					{
						wsprintf(
							buff,
							"API: %s returned: %08lXh -> %08lXh",
							(char*)(dwFunctNames[i] + dwMemBase),
							pARI->dwRetValue,
							dwNewRetVal);
					}
					else
					{
						wsprintf(
						buff,
						"API: %s returned: %08lXh",
						(PSTR)dwFunctNames[i] + dwMemBase,
						pARI->dwRetValue);
					}
				}
				Add(buff);
				break;
			case CD_ID_APIRETURN_DYN:
				// A DYNAMIC API IS RETURNING
				pARI = (APIRETURNINFO*)pCDS->lpData;
				i = pARI->dwApi;

				// inform plugins
				if (dwPluginWndNum)
				{
					ApiInfo.szDllName = (char*)GetModuleName(ApiList[i].ModuleIndex);
					ApiInfo.szApiName = (char*)ApiList[i].ApiName;
					PostPluginWindowMessage(SS_APIRETURN, (WPARAM)&ApiInfo, pARI->dwRetValue);

					ARVI.dwRetVal = pARI->dwRetValue;
					ARVI.pRetVal  = pARI->pRetValue;
					PostPluginWindowMessage(SS_APIRETURNEX, (WPARAM)&ApiInfo, (LPARAM)&ARVI);
				}
				
				if (ShouldIgnoreAPI(i, FALSE))
					break;

				// should we show return values and this API ?
				if (!Option.bShowApiReturn)
					break;

				// modify return value ?
				if (!bPluginHandler)
				{
					bMod = HandleRetValMod(
						(char*)ApiList[i].ApiName,
						pARI->pRetValue,
						&dwNewRetVal);
				}
				else
					bMod = FALSE;

				// process a modified return value (by plugin)
				if (bPluginHandler)
				{
					bMod = ReadProcessMemory(
						PI.hProcess,
						pARI->pRetValue,
						&dwNewRetVal,
						4,
						&dwBytesRead);
				}

				// show it
				if (bMod)
				{
					wsprintf(
						buff,
						"API: %s returned: %08lXh -> %08lXh",
						ApiList[i].ApiName,
						pARI->dwRetValue,
						dwNewRetVal);
				}
				else
				{
					wsprintf(
					buff,
					"API: %s returned: %08lXh",
					ApiList[i].ApiName,
					pARI->dwRetValue);
				}
				Add(buff);
				break;
			case CD_ID_DEBUG_MSG:
				Add((char*)pCDS->lpData);
				break;
			}
			return TRUE;

		case WM_CANTRAPDLL:
			return CanTrapDll(wParam);

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case ID_OPEN:
					if (bDebugging)
					{
						MessageBeep(MB_ICONERROR);
						break;
					}
					// let the user choose a file
					MakeOfn(ofn);
					ofn.hwndOwner = hDlg;
					ofn.lpstrFile = szFname;
					ofn.lpstrTitle = "Choose an exe file...";
					ofn.lpstrFilter = "ExE files\0*.exe\0\0";
					if (GetOpenFileName(&ofn))
						StartDebugThread();
					return TRUE;

				case ID_ATTACH:
					dwInjectedProcessID = 0;
					CreateModuleDlg(hInst, hDlg, FALSE);

					if (dwInjectedProcessID!=0)
						ProcessInject(dwInjectedProcessID);
					return TRUE;

				// REOPEN stuff
				case ID_LASTFILE_1:
				case ID_LASTFILE_2:
				case ID_LASTFILE_3:
				case ID_LASTFILE_4:
				case ID_LASTFILE_5:
					if (bDebugging)
					{
						MessageBeep(MB_ICONERROR);
						break;
					}
					switch(LOWORD(wParam))
					{
						case ID_LASTFILE_1:
							lstrcpy(szFname, Option.cLastFile1);
							break;
						case ID_LASTFILE_2:
							lstrcpy(szFname, Option.cLastFile2);
							break;
						case ID_LASTFILE_3:
							lstrcpy(szFname, Option.cLastFile3);
							break;
						case ID_LASTFILE_4:
							lstrcpy(szFname, Option.cLastFile4);
							break;
						case ID_LASTFILE_5:
							lstrcpy(szFname, Option.cLastFile5);
							break;
					}
					StartDebugThread();
					break;

				case ID_SCAN_MODULES:
					RefreshModuleApiList();

					break;

				case ID_RESTARTDEBUG:
					if (bDebugging || szFname[0] == 0)
					{
						MessageBeep(MB_ICONERROR);
						break;
					}
					StartDebugThread();
					break;

				case ID_STOPDEBUG:
					if (!bDebugging)
					{
						MessageBeep(0);
						break;
					}

					bDebugging = FALSE;

					if (isDebuggingProcess)
					{
						// terminate our debug thread and the target process
						TerminateProcess(PI.hProcess, -1);
						TerminateThread(hDebugThread, -1);
						isDebuggingProcess = FALSE;
						Add("!!! process terminated by SoftSnoop");
					} 

					// clean up
					PerformDebugCleanup();

					bDebugging = FALSE;
					UpdateStatus("Ready");
					if (Option.bMoveWindow)
						SetNormalWindowPos();
					SetForegroundWindow(hDlg_);

					// clean up
					GlobalFree(pMem);
					if (Option.bMoveWindow)
						SetNormalWindowPos();
					SetForegroundWindow(hDlg_);
					PerformDebugCleanup();
					UpdateStatus("Ready");
					break;

				case ID_CLEARLIST:
					SendMessage(hLBDbg,LB_RESETCONTENT,NULL,NULL);
					break;

				case ID_AUTOSCROLL:
					uiMenuState = GetMenuState(hMenu,ID_AUTOSCROLL,MF_BYCOMMAND);
					if (uiMenuState != MF_CHECKED)
					{
						Option.bScrollList = TRUE;
						CheckMenuItem(hMenu,ID_AUTOSCROLL,MF_CHECKED);
						SendMessage((HWND)hTB,TB_CHANGEBITMAP,ID_AUTOSCROLL,2);
					}
					else
					{
						Option.bScrollList = FALSE;
						CheckMenuItem(hMenu,ID_AUTOSCROLL,MF_UNCHECKED);
						SendMessage((HWND)hTB,TB_CHANGEBITMAP,ID_AUTOSCROLL,3);
					}
					break;

				case ID_SAVETOFILE:
					dwListItemNum = SendMessage(hLBDbg,LB_GETCOUNT,NULL,NULL);
					if (dwListItemNum == 0)
					{
						MessageBeep(0);
						break;
					}
					Handle_ID_SAVETOFILE(dwListItemNum);
					break;

				case ID_SEARCH:
					CreateSearchDlg(hInst,hDlg_);
					break;

				case ID_SEARCHAGAIN:
					if (!DoListSearch(hLBDbg))
						MessageBeep(MB_ICONERROR);
					break;

				case ID_TOPMOST:
					uiMenuState = GetMenuState(hMenu, ID_TOPMOST, MF_BYCOMMAND);
					if (uiMenuState != MF_CHECKED)
					{
						Option.bWinTopMost = TRUE;
						SetWindowTop(hDlg);
						CheckMenuItem(hMenu,ID_TOPMOST,MF_CHECKED);
						SendMessage((HWND)hTB,TB_CHANGEBITMAP,ID_TOPMOST,4);
					}
					else
					{
						Option.bWinTopMost = FALSE;
						SetWindowNonTop(hDlg);
						CheckMenuItem(hMenu,ID_TOPMOST,MF_UNCHECKED);
						SendMessage((HWND)hTB,TB_CHANGEBITMAP,ID_TOPMOST,5);
					}
					break;

				case ID_DUMP:
					if (!bDebugging)
					{
						MessageBeep(MB_ICONERROR);
						break;
					}
					if (isDebuggingProcess)
						CreateDumpDlg(hInst,hDlg,PI.dwProcessId,ImageBase,SizeOfImage);
					else
						CreateDumpDlg(hInst,hDlg,dwInjectedProcessID,ImageBase,SizeOfImage);
					break;

				case ID_SETBPX:
					if (CreateBPDlg(hInst,hDlg)!=0)
					{
						//UpdateShareMem();
						//if (dpMappedMem!=NULL)
						//	SetEvent(hShareSignal);
					}
					break;

				case ID_SETBPM:
					if (!bDebugging)
					{
						MessageBeep(MB_ICONERROR);
						break;
					}
					CreateBPMDlg(hInst, hDlg);
					break;

				case ID_VIEWREGS:
					if (!bDebugging)
					{
						MessageBeep(MB_ICONERROR);
						break;
					}
					CreateRegsDlg(hInst,hDlg);
					break;

				case ID_MEMSERVER:
					if (!bDebugging)
					{
						MessageBeep(MB_ICONERROR);
						break;
					}
					if (hMSWnd)
					{
						SetForegroundWindow(hMSWnd);
						break;
					}
					CreateMemServerDlg(hInst,hDlg,PI.dwProcessId);
					break;

				case ID_SHOWPEINFO:
					if (szFname[0] == 0)
					{
						MessageBeep(MB_ICONERROR);
						break;
					}
					CreatePEEditorDlg(hInst, hDlg);
					break;

				case ID_SHOWMODULES:
					RefreshModuleApiList();

					if (ModuleList==NULL)
					{
						MessageBeep(MB_ICONERROR);
						break;
					}
					CreateModuleDlg(hInst, hDlg, TRUE);
					break;

				case ID_APISTACKMOD:
					CreateStackDlg(hDlg, dwLastApiStackBase);
					break;

				case ID_ACTION_MAR:
					CreateMARDlg(hInst, hDlg_);
					break;

				case ID_CONTINUEDEBUG:
					ResumeProcess();
					SetDebuggingStatus();
					break;

				case ID_SUSPENDALL:
					if (!bDebugging)
					{
						MessageBeep(MB_ICONERROR);
						break;
					}
					SuspendProcess();
					UpdateStatus("All threads are suspended...");
					WaitForUser();
					break;

				case ID_OPTION:
					RefreshModuleApiList();
					CreateOptionDlg(hInst,hDlg);
					break;

				case ID_ABOUT:
					ZeroMemory(&MsgParams,sizeof(MsgParams));
					MsgParams.cbSize = sizeof(MsgParams);
					MsgParams.dwStyle = MB_USERICON;
					MsgParams.lpszIcon = MAKEINTRESOURCE(IDI_ICON1);
					MsgParams.hInstance = hInst;
					MsgParams.hwndOwner = hDlg;
					MsgParams.lpszText = ABOUT_TEXT;
					MsgParams.lpszCaption = "About";
					MessageBoxIndirect(&MsgParams);
					break;

				case ID_EXIT:
					SendMessage(hDlg,WM_CLOSE,NULL,NULL);
					break;

				default:
					ProcessPluginMenu(LOWORD(wParam));
					break;
			}
	}
	return DefWindowProc(hDlg,uMsg,wParam,lParam);
}

DWORD WINAPI ProcessFile(PVOID)
{
	CHAR  cCmdLine[sizeof(Option.cCmdLine)];
	char  cDir[MAX_PATH];
	DWORD dwCreateFlag;

	bDebugging  = FALSE;

	UnInitMapMem();

	// clear the listbox
	SendMessage(hLBDbg,LB_RESETCONTENT,NULL,NULL);

	// build commandline
	//strcpy((PSTR)cCmdLine,(PSTR)szFname);
	wsprintf(cCmdLine, "\"%s\"", szFname);
	if(Option.bAdditionalCmdLine)
	{
		strcat((PSTR)cCmdLine," ");
		strcat((PSTR)cCmdLine,(PSTR)Option.cCmdLine);
	}

	// process the output file
	if (Option.bFileOutPut)
	{
		wsprintf(cDir, "%s", cInitDir);
		lstrcat(cDir, "\\");
		lstrcat(cDir, Option.cOutFile);
		hOutPutFile = CreateFile(cDir, GENERIC_READ | GENERIC_WRITE, \
		FILE_SHARE_READ | FILE_SHARE_WRITE,	NULL,CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (hOutPutFile == INVALID_HANDLE_VALUE)
			ShowError("File for the debug output could not be created :(");
	}

	// build current directory for debuggee
	ExtractDirectory(szFname, cDir);

	// create the process
	UpdateStatus("Creating process...");
	ZeroMemory(&SI,sizeof(STARTUPINFO));
	ZeroMemory(&PI,sizeof(PROCESS_INFORMATION));
	SI.cb = sizeof(STARTUPINFO);

	// add by ygq

	dwCreateFlag = CREATE_SUSPENDED;
	if (Option.bDebugProcess)
		dwCreateFlag = dwCreateFlag | DEBUG_ONLY_THIS_PROCESS | DEBUG_PROCESS;
	if (!DetourCreateProcessWithDll(
		NULL,
		(PSTR)cCmdLine,
		NULL,
		NULL,
		FALSE,
		dwCreateFlag, 
		NULL,
		cDir,
		&SI,
		&PI,
		NULL,
		ApiSnoopDllPath,
		CreateProcess))
	{
		ShowError("Couldn't create process !");
		return -1;
	}
	Sleep(100);

	InitMapMem(PI.dwProcessId);
	UpdateShareMem();

	//LoadLibrary(ApiSnoopDllPath);

	// end by ygq

	// ----------- debug the process ------------
	STOP = FALSE;

	// save path
	SetNewestLastFile(szFname);

	// report the memory server about the new process
	if (hMSWnd)
		SendMessage(hMSWnd,WM_MS_NEWPROCESS,(WPARAM)PI.dwProcessId,0);

	ResumeThread(PI.hThread); // let the main thread run
	dwThreadCount = 0;

	SetDebuggingStatus();

	// inform plugin windows
	PostPluginWindowMessage(SS_DEBUGSTART, 0, 0);
	
	// START the debug loop
	if (Option.bMoveWindow)
		SetDebugWindowPos();
	bDebugging = TRUE;

	// update GUI
	UpdateGUI(hDlg_, bDebugging);	

	if (Option.bDebugProcess)
	{
		// start debug thread
		isDebuggingProcess = TRUE;
		StartDebug();

		SendMessage(hDlg_, WM_COMMAND, ID_STOPDEBUG, 0);
	}

	return 0;
}

void ProcessInject(DWORD dwProcessID)
{
	char  cDir[MAX_PATH];

	bDebugging  = FALSE;

	UnInitMapMem();

	InitMapMem(dwProcessID);
	UpdateShareMem();

	if (Inject(dwProcessID, ApiSnoopDllPath)<0)
	{
		UnInitMapMem();
		ShowError("Can't inject to the selected process.");
		return;
	}

	// clear the listbox
	SendMessage(hLBDbg,LB_RESETCONTENT,NULL,NULL);

	// process the output file
	if (Option.bFileOutPut)
	{
		wsprintf(cDir, "%s", cInitDir);
		lstrcat(cDir, "\\");
		lstrcat(cDir, Option.cOutFile);
		hOutPutFile = CreateFile(cDir, GENERIC_READ | GENERIC_WRITE, \
		FILE_SHARE_READ | FILE_SHARE_WRITE,	NULL,CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (hOutPutFile == INVALID_HANDLE_VALUE)
			ShowError("File for the debug output could not be created :(");
	}

	// ----------- debug the process ------------
	STOP = FALSE;

	// report the memory server about the new process
	if (hMSWnd)
		SendMessage(hMSWnd,WM_MS_NEWPROCESS,(WPARAM)dwProcessID,0);

	dwThreadCount = 0;

	SetDebuggingStatus();

	// inform plugin windows
	PostPluginWindowMessage(SS_DEBUGSTART, 0, 0);
	
	// START the debug loop
	if (Option.bMoveWindow)
		SetDebugWindowPos();
	bDebugging = TRUE;

	// update GUI
	UpdateGUI(hDlg_, bDebugging);	

	return ;
}