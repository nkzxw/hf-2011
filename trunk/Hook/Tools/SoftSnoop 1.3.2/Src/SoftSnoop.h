
#ifndef __SSincludes
  #define __SSincludes

  #include "OptionMenu.h"
  #include "SSPlugin.h"
  #include "lib.h"

  #define    SDIM                        SendDlgItemMessage
  #define    SDIT                        SetDlgItemText

  #define    TRAPAPIBUFFSIZE             10 * 50
  #define    MAX_INT3BPS                 30
  #define    MAX_BPRESTORENUM            20
  #define    MAX_THREADNUM				 30
  #define    MAX_OUTFILENAME_LENGTH      30
  #define    WM_APICALL                  WM_USER + 0x1000
  #define    CD_ID_APIRETURN             1
  #define    WM_CANTRAPDLL               WM_USER + 0x1002 // wParam -> IID index

  // add by ygq
  #pragma   pack(1) 

  #define    WM_APICALL_DYN              WM_USER + 0x1004
  #define    CD_ID_APIRETURN_DYN         2
  #define    CD_ID_DEBUG_MSG             3
  #define    MAX_TRAP_API_COUNT          100
  #define    MAX_NAME_LEN                100

  #define    MAX_DLL_COUNT               200
  #define    API_LIST_INC                100

  #define SS_CMD_GET_MODULE_LIST         1
  #define SS_CMD_GET_API_LIST            2
  #define SS_CMD_SET_OPTION              3
  #define SS_CMD_STOP                    4
  #define SS_CMD_CONTINUE                5

 // typedef struct
 // {
	//BYTE   byPushOpc;       // 0x68 push (dword)
	//DWORD  dwPushAddr;      // Api number
	//BYTE   byPushadOpc;     // 0x60 pushad
	//BYTE	byJmp0pc;		// 0xE9 jmp(dword)
	//DWORD	dwJmpAddr;		// jmp to addr
 // } sJmpOp;

  struct sShareMemHeader {
	  DWORD dwApiListAddr;
	  DWORD dwCmdAddr;
  };

typedef struct
{
#ifdef API_DBG
	BYTE   Int3;
#endif
	BYTE   byPushadOpc;   // 0x60 pushad

	BYTE   byPushEsp;     // 0x54 push esp
	BYTE   byPushOpc;     // 0x68 push (dword)
	DWORD  dwPushAddr;    // Api number
	BYTE   byCallOpc;     // 0xE8 call (dword)
	DWORD  dwCallAddr;    // address of "TrappedApiCall"

	BYTE   byPopadOpc;    // 0x61 popad
	BYTE   byJmpOpcApi;   // 0xE9 jmp (dword)
	DWORD  dwJmpApiAddr;  // jmp to the real Api function
} sApiStub;

  struct sModuleInfo {
	DWORD BaseAddr;
	BOOL  IsFiltered;
	DWORD Size;
	unsigned char ModuleName[MAX_NAME_LEN];
  };

  struct sApiInfo {
	DWORD ApiAddr;
	WORD IsHooked;
	WORD IsFiltered;
	DWORD ModuleIndex;
	DWORD ApiOrdinal;
	unsigned char ApiName[MAX_NAME_LEN];
  };

  struct sApiHookInfo {
	DWORD ApiAddr;
	WORD IsHooked;
	WORD IsFiltered;
	DWORD ModuleIndex;
	DWORD ApiOrdinal;
	unsigned char ApiName[MAX_NAME_LEN];
	unsigned char NewOps[sizeof(sApiStub)];
  };

  const DWORD  SHARE_MEM_SIZE = (sizeof(sApiInfo)*20000+8);
  const char SHARE_MEM_NAME[] = "SoftSnoopShareMem";
  const char SHARE_CMD_NAME[] = "SoftSnoopCmdEvent";
  const char SHARE_CMDFINISH_NAME[] = "SoftSnoopCmdFinishEvent";
  
  const char  SS_CLASS_NAME[]               = "SoftSnoopMainDialog";
  const char  SSDialogTitle[]               = "[ SoftSnoop 1.3.2 ] by yoda/f2f modified by ygq";

// end of add by ygq

  #define    NO_SPECIAL_APIS             0
  #define    DONT_CALL_SPECIAL_APIS      1
  #define    JUST_CALL_SPECIAL_APIS      2

  #define    NO_SPECIAL_DLLS             0
  #define    DONT_SPECIAL_DLL_CALLS      1
  #define    JUST_SPECIAL_DLL_CALLS      2

  #define    DLL_NO_TRAP_BUFF_SIZE       300

  // structs
#pragma pack(1)
  typedef struct
  {
	  BOOL      bScrollList;
	  BOOL      bShowDebugEvents;
	  BOOL      bHandleExceptions;
	  BOOL      bMoveWindow;
	  BOOL      bWinTopMost;
	  BOOL      bReportNameAPI;
	  BOOL      bReportOrdinalAPI;
	  BOOL      bStopAtDB;

	  // add by ygq
	  BOOL      bDebugProcess; // should we create process with debug falgs?
	  BOOL      bShowAPICall;
	  BOOL      bShowApiParams;
	  BOOL      bShowApiReturn;

	  DWORD     dwIgnoreAPIs;
	  WORD      wIgnoreAPINum;
	  CHAR      cIgnoreAPIs[MAX_IGNOREAPINUM][MAX_APILISTITEMLENGTH];

	  DWORD     dwIgnoreToDlls;
	  WORD      wIgnoreToDllNum;
	  CHAR      cIgnoreToDlls[MAX_IGNOREDLLNUM][MAX_DLLNAMELENGTHOPT];

	  DWORD     dwIgnoreFromDlls;
	  WORD      wIgnoreFromDllNum;
	  CHAR      cIgnoreFromDlls[MAX_IGNOREDLLNUM][MAX_DLLNAMELENGTHOPT];

	  DWORD     dwIgnoreRegions;
	  WORD      wIgnoreRegionNum;
	  DWORD     dwIgnoreRegionBegin[MAX_IGNOREREGIONNUM];
	  DWORD     dwIgnoreRegionEnd[MAX_IGNOREREGIONNUM];

	  BOOL      bAdditionalCmdLine;
	  CHAR      cCmdLine[MAX_PATH];

	  int       MainWndWidth;
	  int       MainWndHeight;

	  BOOL      bGUIOutPut;
	  BOOL      bFileOutPut;
	  CHAR      cOutFile[MAX_OUTFILENAME_LENGTH];

	  BOOL      bStopAtEntry;
	  BOOL      bDlgOnBPM;
	  BOOL      bShellExtension;
	  char      cLastFile1[MAX_PATH];
	  char      cLastFile2[MAX_PATH];
	  char      cLastFile3[MAX_PATH];
	  char      cLastFile4[MAX_PATH];
	  char      cLastFile5[MAX_PATH];

	  BOOL      bDllNoTrapList;
	  char      cDllNoTrapList[DLL_NO_TRAP_BUFF_SIZE];

	  char      cAppPath[MAX_PATH];

	  BOOL      bRestoreBP;
	  BOOL      bShowTIDs;

  } Options;

	typedef struct
	{
		DWORD   dwApi;
		DWORD   dwRetValue;
		void*   pRetValue;
	} APIRETURNINFO;

  typedef struct
  {
	  VOID*  pVA;
	  BYTE   byOrg;
  } sBPXInfo;

  typedef struct
  {
      DWORD    dwThreadID;
	  HANDLE   hThread;
  } ThreadInfo;

#pragma pack()

  extern Options                  Option;
  extern BOOL                     bDebugging;
  extern CHAR                     TrapApiNameBuff[TRAPAPIBUFFSIZE];
  extern sBPXInfo                 BPXInfo[MAX_INT3BPS];
  extern int                      iBPXCnt;
  extern char                     szFname[MAX_PATH];
  extern HWND                     hLBDbg;
  extern ThreadInfo               ThreadList[MAX_THREADNUM];
  extern DWORD                    dwThreadCount;
  extern BOOL                     bWinNT;
  extern PROCESS_INFORMATION      PI;
  extern HINSTANCE			      hInst;
  extern HWND				      hDlg_;
  extern HMENU                    hMenu;
  extern DWORD                    ImageBase, SizeOfImage, dwEntryPointVA;
  extern DWORD                    dwInjectedProcessID;

  BOOL   ProcessBPXList();
  CHAR*  ExtractFileName(CHAR* szFilePath);
  VOID   SSAPIPROC Add(PSTR szText);
  int    SSAPIPROC ShowError(PSTR MsgText);
  VOID   SSAPIPROC ResumeProcess();
  VOID   SSAPIPROC SuspendProcess();
  HANDLE GetThreadHandle(DWORD dwThreadID);
  VOID   UpdateStatus(PSTR StatusText);
  void   WaitForUser();
  void   DefApiToCombo(HWND hDlg, DWORD dwCBId);
  void   ModuleListToCombo(HWND hDlg, DWORD dwCBId);
  int    GetModuleIndex(char* cpModuleName);
  void   ApiListToCombo(HWND hDlg, DWORD dwCBId, int dwModuleIndex);
  void   NotifyOptionChange();
  BOOL   ShowProcessModules(HWND hDlg);
  BOOL   ShowProcessList(HWND hDlg);
  VOID   SSAPIPROC StartDebugThread();
  VOID   SetDebuggingStatus();

#endif