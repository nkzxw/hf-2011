//
// This dll will be loaded into the target's process memory and
// it'll build the API stubs which will inform the GUI about API calls.
//
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stddef.h>
#include <tlhelp32.h>
#include "..\SoftSnoop.h"
#include "..\ApiParam.h"

#include ".\Detours\detours.h"

#pragma pack (1) // very important
#pragma comment(linker,"/FILEALIGN:0x200 /MERGE:.rdata=.text /MERGE:.data=.text /SECTION:.text,EWR /IGNORE:4078")

//#define API_DBG
//#define RET_DBG

// structs
typedef struct
{
	PIMAGE_DOS_HEADER     pDosh;
	PIMAGE_NT_HEADERS     pPeh;
	DWORD                 ImageBase;
	DWORD                 SizeOfImage;
} sPEInfo;

typedef struct
{
	CHAR DllName[8];
	int  iCheckSize;
} sDllSkipList;

typedef struct
{
	LPVOID  pRetAddr;
	DWORD   dwAPINum;
} sThRetStack;

// add by ygq
const PSTR          NEW_API_FUNCT_NAME  = "NewApiCall";

// alloc one page once
#define API_BLOCK_SIZE  0x10000
#define API_BLOCK_NUM  1000
DWORD dwApiBlockCount;
DWORD dwApiInBlockIndex;
sApiHookInfo* ApiBlockList[1000];

DWORD dwModuleListLen;
DWORD dwModuleCount;
sModuleInfo* ModuleList;

DWORD dwApiListLen;
DWORD dwApiHookCount;
DWORD* ApiHookList;

DWORD dwIsFirstIn;

HANDLE hCmdSignal;
HANDLE hCmdFinishSignal;
HANDLE hWaitCmdThread;
DWORD dwWaitCmdThreadID;

BOOL STOP = FALSE;
BOOL STOP_OUTPUT = FALSE;

HANDLE dwMapMemHandle;
DWORD* dpMappedMem;

DWORD  dwNewApiAddr;

DWORD TargetModuleBase;
DWORD TargetModuleEnd;
DWORD SelfModuleBase;
DWORD SelfModuleEnd;

struct sRegion{
	DWORD BeginAddr;
	DWORD EndAddr;
};

DWORD dwValidFromRegionCount;
DWORD dwInvalidFromRegionCount;
sRegion ValidFromRegion[MAX_IGNOREREGIONNUM + MAX_IGNOREDLLNUM];
sRegion InvalidFromRegion[MAX_IGNOREREGIONNUM + MAX_IGNOREDLLNUM];

Options             Option;
BOOL  KeepSilent;         // should we output message to the main window?

sApiHookInfo* AllocApiHookInfoBuf();
void ReleaseApiBlock();
int AddApiHookInfo(struct sApiHookInfo* aApiInfo);
int AddModuleInfo(DWORD, DWORD, char*);
void OutputMessage(char* msg);
void InitMapMem(DWORD);
void UnInitMapMem();
void UpdateShareMem();
DWORD WINAPI WaitHookCmd(PVOID);
void AddApiOfModule(DWORD ModuleIndex);
int HookApiInfo(DWORD);
void HookAllApi();
void UnHookAllApi();
void HookApiOfModule(DWORD dwModuleIndex);

void ScanModules();
void HookAllApi();
void FillModuleListToShareMem();
void FillApiListToShareMem();
void BuildFilterListFromOption();
BOOL IsFilteredApi(DWORD);
BOOL IsFilteredFromAddr(DWORD);

// end by ygq

// the functions
BOOL  CreateThreadRetStack();
BOOL  FreeThreadRetStack();
VOID __stdcall  PushRetStack(DWORD dwValue, DWORD dwApiNum);
DWORD __stdcall PopRetStack(void* pEax, DWORD dwApiRetValue);
extern "C" VOID __stdcall TrappedApiCall(DWORD dwApiNum,VOID* pStack);
extern "C" VOID __stdcall RetTrapProc();
BOOL  InitApiStubStruct(sApiStub *tmpApiStub);
BOOL  BuildApiStub(DWORD* dwApiAddr,DWORD dwApiNum);

// constants
#define             RET_STACK_SIZE       (sizeof(sThRetStack)*20)

const PSTR          TRAPFUNCTION_NAME    = "TrappedApiCall";
CONST PSTR          TRAP_RET_FUNCT_NAME  = "RetTrapProc";
const sDllSkipList  DllSkipList[]        = {{"MFC",3},{"MSVCRT",6}};
const int           DLLSKIPCOUNT         = 2;

// varialbes
//PVOID                        pApiStubs,pApiStubMem;
HANDLE                       hDll;
HWND                         hSSDlg;
//sPEInfo                      PEInfo;
//PIMAGE_IMPORT_DESCRIPTOR     pIID;
//DWORD                        *pDW;
//sApiStub                     ApiStub;
DWORD                        dwTrapFunctAddr, dwRetProcAddr, dwTlsInd,dwMemTlsInd;

char        buff[255];

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  fdwReason, 
                       LPVOID lpReserved
					 )
{

	switch(fdwReason) 
    { 
        case DLL_PROCESS_ATTACH:
			hDll = hModule;
			// get the handle of the SS dialog 
			hSSDlg = FindWindow(NULL,SSDialogTitle);
			if (!hSSDlg)
			{
				MessageBox(
					0,
					"Couldn't locate SoftSnoop Dialog :(",
					"APISnoop.dll - Error",
					MB_ICONERROR | MB_SYSTEMMODAL);
				return FALSE;
			}

			// add by ygq
			InitMapMem(GetCurrentProcessId());

			if (!STOP)
			{
				// create tls's
				dwTlsInd = TlsAlloc();
				dwMemTlsInd = TlsAlloc();
				if (dwTlsInd == 0xFFFFFFFF || dwMemTlsInd == 0xFFFFFFFF)
					return FALSE;
				
				// get the address of the API return handler
				dwRetProcAddr = (DWORD)GetProcAddress((HINSTANCE)hModule,TRAP_RET_FUNCT_NAME);
				// create RET stack for the main thread
				if (!CreateThreadRetStack())
					return FALSE;

				dwApiBlockCount = 0;
				dwApiInBlockIndex = 0;
				memset(ApiBlockList, 0, sizeof(ApiBlockList));

				ApiHookList = (DWORD*)VirtualAlloc(NULL, sizeof(DWORD) * API_LIST_INC, MEM_COMMIT, PAGE_READWRITE);
				dwApiListLen = API_LIST_INC;
				dwApiHookCount = 0;
				ModuleList = (sModuleInfo*)VirtualAlloc(NULL, sizeof(sModuleInfo) * API_LIST_INC, MEM_COMMIT, PAGE_READWRITE);
				dwModuleListLen = API_LIST_INC;
				dwModuleCount = 0;

				dwIsFirstIn = -1;
				dwValidFromRegionCount = 0;
				dwInvalidFromRegionCount = 0;

				STOP = FALSE;
				STOP_OUTPUT = FALSE;
				KeepSilent = FALSE;

				TargetModuleBase = (DWORD)GetModuleHandle(NULL);
				TargetModuleEnd = TargetModuleBase + ((PIMAGE_NT_HEADERS) (TargetModuleBase + ((IMAGE_DOS_HEADER*)TargetModuleBase)->e_lfanew))->OptionalHeader.SizeOfImage;
				
				SelfModuleBase = (DWORD)hModule;
				SelfModuleEnd = SelfModuleBase + ((PIMAGE_NT_HEADERS) (SelfModuleBase + ((IMAGE_DOS_HEADER*)SelfModuleBase)->e_lfanew))->OptionalHeader.SizeOfImage;

				MapSSFiles();

				dwNewApiAddr = (DWORD)GetProcAddress((HINSTANCE)hModule,NEW_API_FUNCT_NAME);

				ScanModules();

			}

			hWaitCmdThread = CreateThread(NULL,0,WaitHookCmd,NULL,0,&dwWaitCmdThreadID);

			// end by ygq

            break;

		case DLL_THREAD_ATTACH:
			if (!CreateThreadRetStack())
				return FALSE;
			break;

		case DLL_THREAD_DETACH:
			if (!FreeThreadRetStack())
				return FALSE;
			break;

        case DLL_PROCESS_DETACH:
				
			// add by ygq
			wsprintf(buff, "Unload ApiSnoop dll.");
			OutputMessage(buff);

			// tell the main window to stop debug
			SendMessage(hSSDlg, WM_COMMAND, 40007, 0);

			// end the thread
			STOP = TRUE;
			SetEvent(hCmdSignal);

			UnHookAllApi();

			//// free thread ret stack memory of the main thread
			FreeThreadRetStack();
			// free TLS's
			TlsFree(dwTlsInd);
			TlsFree(dwMemTlsInd);

			//// add by ygq
			CloseHandle(hWaitCmdThread);
			UnInitMapMem();

			UnmapSSFiles();

			ReleaseApiBlock();

			VirtualFree(ApiHookList, 0, MEM_RELEASE);

			VirtualFree(ModuleList, 0, MEM_RELEASE);

			// end by ygq
            break;
	}
	return TRUE;
}

// add by ygq

sApiHookInfo* AllocApiHookInfoBuf()
{
	if (dwApiBlockCount>0)
	{
		// current block still has enough space
		if ((dwApiInBlockIndex+sizeof(sApiHookInfo))<API_BLOCK_SIZE)
		{
			dwApiInBlockIndex += sizeof(sApiHookInfo);
			return (sApiHookInfo*)((DWORD)ApiBlockList[dwApiBlockCount-1] + dwApiInBlockIndex - sizeof(sApiHookInfo));
		}
	}

	// we need to alloc a new block
	ApiBlockList[dwApiBlockCount] = (sApiHookInfo*)VirtualAlloc(NULL, API_BLOCK_SIZE, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (ApiBlockList[dwApiBlockCount] == NULL)
		return NULL;
	dwApiInBlockIndex = sizeof(sApiHookInfo);
	dwApiBlockCount ++;
	return ApiBlockList[dwApiBlockCount-1];
}

BOOL IsAddrInApiBlock(DWORD dwAddr)
{
	DWORD i;
	for (i=0; i<dwApiBlockCount; i++)
		if ( dwAddr >= (DWORD)ApiBlockList[i] && (dwAddr < (DWORD)ApiBlockList[i]+API_BLOCK_SIZE) )
			return TRUE;
	return FALSE;
}

void ReleaseApiBlock()
{
	int i;
	for (i=0;i<dwApiBlockCount;i++)
		if (ApiBlockList[i]!=NULL)
			VirtualFree(ApiBlockList[i], 0, MEM_RELEASE);
}

int AddApiHookInfo(struct sApiHookInfo* aApiInfo)
{
	DWORD* tmpList;

	if (dwApiHookCount>=dwApiListLen)
	{
		// increase apilist lengh
		tmpList = (DWORD*)VirtualAlloc(NULL, sizeof(DWORD) * (dwApiListLen + API_LIST_INC), MEM_COMMIT, PAGE_READWRITE);
		if (tmpList == NULL)
			return -1;

		CopyMemory(tmpList, ApiHookList, sizeof(DWORD) * dwApiListLen );
		VirtualFree(ApiHookList, 0, MEM_RELEASE);
		ApiHookList = tmpList;
		dwApiListLen += API_LIST_INC;
		if (dwApiHookCount+1>dwApiListLen)
			return -1;
	}

	ApiHookList[dwApiHookCount] = (DWORD)aApiInfo;
	++ dwApiHookCount;
	return dwApiHookCount-1;

}

int AddModuleInfo(DWORD dwModuleBase, DWORD dwModuleSize, char* cpModuleName)
{
	DWORD* tmpList;
	DWORD  i;

	// already exist?
	for (i=0;i<dwModuleCount;i++)
		if (ModuleList[i].BaseAddr == dwModuleBase)
			return -1;

	if (dwModuleCount>=dwModuleListLen)
	{
		// increase modulelist lengh
		tmpList = (DWORD*)VirtualAlloc(NULL, sizeof(DWORD) * (dwModuleListLen + API_LIST_INC), MEM_COMMIT, PAGE_READWRITE);
		if (tmpList == NULL)
			return -1;

		CopyMemory(tmpList, ModuleList, sizeof(sModuleInfo) * dwModuleListLen );
		VirtualFree(ModuleList, 0, MEM_RELEASE);

		ModuleList = (sModuleInfo*)tmpList;
		dwModuleListLen += API_LIST_INC;
		if (dwModuleCount+1>dwModuleListLen)
			return -1;
	}

	// add to list
	ModuleList[dwModuleCount].BaseAddr = dwModuleBase;
	ModuleList[dwModuleCount].Size = dwModuleSize;
	if (cpModuleName!=NULL)
		strncpy((char*)ModuleList[dwModuleCount].ModuleName, cpModuleName, MAX_NAME_LEN-1);
	else
		ModuleList[dwModuleCount].ModuleName[0] = 0;

	++ dwModuleCount;
	return dwModuleCount-1;

}

void OutputMessage(char* msg)
{
	COPYDATASTRUCT CDS;

	if (KeepSilent || STOP_OUTPUT)
		return;

	CDS.lpData = msg;
	CDS.cbData = strlen(msg)+1;
	CDS.dwData = CD_ID_DEBUG_MSG;
	SendMessage(
		hSSDlg,
		WM_COPYDATA,
		(WPARAM)0,
		(LPARAM)&CDS);
}

void InitMapMem(DWORD dwPID)
{
	char fileName[200];
	DWORD dwMapExist = 1;

	wsprintf(fileName, "%s %d", SHARE_MEM_NAME, dwPID);
	dwMapMemHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, fileName);
	if (dwMapMemHandle==NULL)
	{
		dwMapExist = 0;
		dwMapMemHandle = CreateFileMapping(0, NULL, PAGE_READWRITE, 0, SHARE_MEM_SIZE, fileName);
		if (!dwMapMemHandle)
			return;
	}

	dpMappedMem = (DWORD*)MapViewOfFile(dwMapMemHandle, FILE_MAP_ALL_ACCESS, 0, 0, SHARE_MEM_SIZE);
	if (dpMappedMem==NULL)
		return;

	wsprintf(fileName, "%s %d", SHARE_CMD_NAME, dwPID);
	hCmdSignal = CreateEvent(NULL, FALSE, FALSE, fileName);
	wsprintf(fileName, "%s %d", SHARE_CMDFINISH_NAME, dwPID);
	hCmdFinishSignal = CreateEvent(NULL, FALSE, FALSE, fileName);

	if (dwMapExist)
	{
		// read option
		Option = *((Options*)(dpMappedMem+1));
		BuildFilterListFromOption();
		//wsprintf(buff, "Option readed %d", Option.wIgnoreAPINum);
		//OutputMessage(buff);
	} else
	{
		wsprintf(buff, "Option can not readed for no file mapping");
		OutputMessage(buff);
	}
}

void UnInitMapMem()
{
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

void UpdateShareMem()
{
}

DWORD WINAPI WaitHookCmd(PVOID)
{
	DWORD dwCmd;

	wsprintf(buff, "Start waiting thread.");
	OutputMessage(buff);

	if (dpMappedMem==NULL)
	{
		wsprintf(buff, "Memory map error.");
		OutputMessage(buff);

		return -1;
	}

	while (1) // DEBUG LOOP
	{
		WaitForSingleObject(hCmdSignal,INFINITE);
		if (dpMappedMem==NULL)
			break;
		if (STOP)
			break;

		// don't outputmessage, else the program will be locked
		KeepSilent = TRUE;

		dwCmd = *dpMappedMem;
		switch (dwCmd) {
			case SS_CMD_GET_MODULE_LIST:
				ScanModules();
				FillModuleListToShareMem();
				break;
			case SS_CMD_GET_API_LIST:
				FillApiListToShareMem();
				break;
			case SS_CMD_SET_OPTION:
				Option = *((Options*)(dpMappedMem+1));
				BuildFilterListFromOption();
				break;
			case SS_CMD_STOP:
				KeepSilent = FALSE;
				wsprintf(buff, "Stop output.");
				OutputMessage(buff);
				STOP_OUTPUT = TRUE;
				break;
			case SS_CMD_CONTINUE:
				KeepSilent = FALSE;
				STOP_OUTPUT = FALSE;
				Option = *((Options*)(dpMappedMem+1));
				BuildFilterListFromOption();
				wsprintf(buff, "Continue output.");
				OutputMessage(buff);
				break;
		}

		SetEvent(hCmdFinishSignal);
		KeepSilent = FALSE;

		if (STOP)
			break;
	}

	UnInitMapMem();

	KeepSilent = FALSE;
	STOP_OUTPUT = FALSE;
	wsprintf(buff, "Exit waiting thread.");
	OutputMessage(buff);

	return 0;
}

void AddApiOfModule(DWORD ModuleIndex)
{
	int ApiIndex;
	DWORD ImageBase;
	PIMAGE_NT_HEADERS peNTHeader;
	PIMAGE_EXPORT_DIRECTORY pImageExportDir;
	DWORD dwExportSecBase, dwExportSecEnd;
	DWORD *dpExportName, *dpExportAddress;
	WORD *dpExportOrdinal;
	sApiHookInfo *NewApiInfo;
	sApiStub ApiStub;
	MEMORY_BASIC_INFORMATION minfo;
	DWORD i;
	LONG lRet = 0;

	if (ModuleIndex>=dwModuleCount)
		return;

	//wsprintf(buff, "Start add api of %s base %08X.",  ModuleList[ModuleIndex].ModuleName, ModuleList[ModuleIndex].BaseAddr);
	//OutputMessage(buff);

	ImageBase = ModuleList[ModuleIndex].BaseAddr;
	peNTHeader = (PIMAGE_NT_HEADERS) (ImageBase + ((IMAGE_DOS_HEADER*)ImageBase)->e_lfanew);
	if(peNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress==0 || \
			peNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size==0)
	{
		return;
	}

	pImageExportDir = (PIMAGE_EXPORT_DIRECTORY)(ImageBase + peNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	dpExportName = (DWORD*)(ImageBase + pImageExportDir->AddressOfNames);
	dpExportOrdinal = (WORD*)(ImageBase + pImageExportDir->AddressOfNameOrdinals);
	dpExportAddress = (DWORD*)(ImageBase + pImageExportDir->AddressOfFunctions);


	dwExportSecBase = (DWORD)pImageExportDir;
	dwExportSecEnd = (DWORD)pImageExportDir + peNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	InitApiStubStruct(&ApiStub);

	for (i=0;i<pImageExportDir->NumberOfNames;i++)
	{
		NewApiInfo = AllocApiHookInfoBuf();
		NewApiInfo->ModuleIndex = ModuleIndex;
		NewApiInfo->ApiOrdinal = dpExportOrdinal[i];
		NewApiInfo->ApiAddr = ImageBase + dpExportAddress[NewApiInfo->ApiOrdinal];
		if (dpExportName[i] != 0)
			strncpy((char*)&NewApiInfo->ApiName[0], (char*)(ImageBase + dpExportName[i]), MAX_NAME_LEN-1);
		
		//wsprintf(buff, "Find export api: %s Ordinal: %d Addr: %08x.", NewApiInfo->ApiName, dpExportOrdinal[i], dpExportAddress[NewApiInfo->ApiOrdinal]);
		//OutputMessage(buff);

		if (NewApiInfo->ApiAddr>=dwExportSecBase && \
			NewApiInfo->ApiAddr<dwExportSecEnd)
		{
			//wsprintf(buff, "Api %s is an export forward to api: %s.", NewApiInfo->ApiName, NewApiInfo->ApiAddr);
			//OutputMessage(buff);
			continue;
		}

		// is the api addr excutable?
		if (VirtualQuery((VOID*)NewApiInfo->ApiAddr, &minfo, sizeof(minfo))!=0)
		{
			if ((minfo.Protect & 0xF0) == 0 || minfo.State!=MEM_COMMIT)
			{
				//wsprintf(buff, "Address property of Api %s is no executable: %08X state: %08X.", NewApiInfo->ApiName, minfo.Protect, minfo.State);
				//OutputMessage(buff);
				continue;
			}
		} else
		{
			continue;
		}

		ApiIndex = AddApiHookInfo(NewApiInfo);

		if (ApiIndex>=0)
		{
			ApiStub.dwPushAddr = ApiIndex;
			ApiStub.dwJmpApiAddr = NewApiInfo->ApiAddr - (DWORD)NewApiInfo->NewOps  - offsetof(sApiStub,dwJmpApiAddr) - 4;
			ApiStub.dwCallAddr = dwTrapFunctAddr - (DWORD)NewApiInfo->NewOps - offsetof(sApiStub,dwCallAddr) - 4;
			memcpy(NewApiInfo->NewOps, &ApiStub, sizeof(sApiStub));
		} else
		{
			wsprintf(buff, "Unable to add api %s total: %d.",  NewApiInfo->ApiName, pImageExportDir->NumberOfNames);
			OutputMessage(buff);

		}
	} // for

	//wsprintf(buff, "End add api of %s total name: %d total fun: %d.",  ModuleList[ModuleIndex].ModuleName, pImageExportDir->NumberOfNames, pImageExportDir->NumberOfFunctions);
	//OutputMessage(buff);

}

void UnHookAllApi()
{
	DWORD i;
	LONG  lRet;
	HANDLE hSnap;
	THREADENTRY32 thInfo;

	//wsprintf(buff, "Unhook all modules.");
	//OutputMessage(buff);

	DetourTransactionBegin();
	DetourSetIgnoreTooSmall(TRUE);

	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetCurrentProcessId());
	if (hSnap!=INVALID_HANDLE_VALUE)
	{
		if (Thread32First(hSnap, &thInfo))
		{
			DetourUpdateThread(OpenThread(THREAD_QUERY_INFORMATION, FALSE, thInfo.th32ThreadID));
			while (Thread32Next(hSnap, &thInfo))
			{
				DetourUpdateThread(OpenThread(THREAD_QUERY_INFORMATION, FALSE, thInfo.th32ThreadID));
			}
		}
	}

	for (i=0;i<dwApiHookCount;i++)
	{
		if (!((sApiHookInfo*)ApiHookList[i])->IsHooked)
			continue;

		lRet = DetourDetach((PVOID*)&((sApiHookInfo*)ApiHookList[i])->ApiAddr, ((sApiHookInfo*)ApiHookList[i])->NewOps);

		if (lRet!=NO_ERROR)
		{
			wsprintf(buff, "Failed to unhook api: %s. error : %d.", ((sApiHookInfo*)ApiHookList[i])->ApiName, lRet);
			OutputMessage(buff);
		} else
		{
			//wsprintf(buff, "Success to unhook api: %s. error : %d.", ((sApiHookInfo*)ApiHookList[i])->ApiName, lRet);
			//OutputMessage(buff);
		}
	}

	lRet = DetourTransactionCommit();

	if (lRet!=NO_ERROR)
	{
		wsprintf(buff, "Failed to commit unhook api total: %d. error: %d", dwApiHookCount, lRet);
		OutputMessage(buff);
	} else
	{
		wsprintf(buff, "Success to commit unhook api total: %d.", dwApiHookCount);
		OutputMessage(buff);
	}
}

void HookAllApi()
{
	DWORD i;
	LONG  lRet;
	DWORD newAddr;
	HANDLE hSnap;
	THREADENTRY32 thInfo;
	sApiHookInfo* theInfo;

	DetourTransactionBegin();
	DetourSetIgnoreTooSmall(TRUE);

	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetCurrentProcessId());
	if (hSnap!=INVALID_HANDLE_VALUE)
	{
		if (Thread32First(hSnap, &thInfo))
		{
			DetourUpdateThread(OpenThread(THREAD_QUERY_INFORMATION, FALSE, thInfo.th32ThreadID));
			while (Thread32Next(hSnap, &thInfo))
			{
				DetourUpdateThread(OpenThread(THREAD_QUERY_INFORMATION, FALSE, thInfo.th32ThreadID));
			}
		}
	}
	
	for (i=0;i<dwApiHookCount;i++)
	{

		theInfo = (sApiHookInfo*)ApiHookList[i];

		lRet = DetourAttachEx((PVOID*)&theInfo->ApiAddr, ((sApiHookInfo*)ApiHookList[i])->NewOps, (PDETOUR_TRAMPOLINE*)&newAddr, NULL, NULL);
			
		if (lRet!=NO_ERROR)
		{
			theInfo->IsHooked = FALSE;
			//wsprintf(buff, "Failed to hook api: %s. error: %d", ((sApiHookInfo*)ApiHookList[i])->ApiName, lRet);
			//OutputMessage(buff);
		} else
		{
			theInfo->IsHooked = TRUE;
			// change the jmp dest to new api address
			((sApiStub*)theInfo->NewOps)->dwJmpApiAddr = newAddr - (DWORD)(sApiStub*)theInfo->NewOps  - offsetof(sApiStub,dwJmpApiAddr) - 4;
		}
	}

	lRet = DetourTransactionCommit();

	if (lRet!=NO_ERROR)
	{
		wsprintf(buff, "Failed to commit hook api total: %d. error: %d", dwApiHookCount, lRet);
		OutputMessage(buff);
	} else
	{
		wsprintf(buff, "Success to commit hook api total: %d.", dwApiHookCount);
		OutputMessage(buff);
	}
}

void HookApiOfModule(DWORD dwModuleIndex)
{
	DWORD i;
	LONG  lRet;
	DWORD newAddr;
	sApiHookInfo* theInfo;

	for (i=0;i<dwApiHookCount;i++)
	{

		theInfo = (sApiHookInfo*)ApiHookList[i];
		if (theInfo->ModuleIndex!=dwModuleIndex)
			continue;

		lRet = DetourAttachEx((PVOID*)&theInfo->ApiAddr, ((sApiHookInfo*)ApiHookList[i])->NewOps, (PDETOUR_TRAMPOLINE*)&newAddr, NULL, NULL);
			
		if (lRet!=NO_ERROR)
		{
			theInfo->IsHooked = FALSE;
			//wsprintf(buff, "Failed to hook api: %s. error: %d", ((sApiHookInfo*)ApiHookList[i])->ApiName, lRet);
			//OutputMessage(buff);
		} else
		{
			theInfo->IsHooked = TRUE;
			// change the jmp dest to new api address
			((sApiStub*)theInfo->NewOps)->dwJmpApiAddr = newAddr - (DWORD)(sApiStub*)theInfo->NewOps  - offsetof(sApiStub,dwJmpApiAddr) - 4;
		}
	}
}

void ScanModules()
{
	DWORD pbLast, pbOld;
	int iNewModule;
    MEMORY_BASIC_INFORMATION mbi;
	char* cpModuleName = NULL;
	char ModuleName[MAX_PATH];
	HANDLE hSnap;
	THREADENTRY32 thInfo;
	DWORD lRet;

	STOP = TRUE;

__try
{

	wsprintf(buff, "Start scan modules.");
	OutputMessage(buff);


	DetourTransactionBegin();
	DetourSetIgnoreTooSmall(TRUE);

	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetCurrentProcessId());
	if (hSnap!=INVALID_HANDLE_VALUE)
	{
		if (Thread32First(hSnap, &thInfo))
		{
			DetourUpdateThread(OpenThread(THREAD_QUERY_INFORMATION, FALSE, thInfo.th32ThreadID));
			while (Thread32Next(hSnap, &thInfo))
			{
				DetourUpdateThread(OpenThread(THREAD_QUERY_INFORMATION, FALSE, thInfo.th32ThreadID));
			}
		}
	}

	pbLast = 0x10000;
	pbOld = pbLast;
    ZeroMemory(&mbi, sizeof(mbi));

	while (1)
	{
		if ((DWORD)pbLast<(DWORD)pbOld)
			break;
		pbLast = pbLast + 0x10000;
		pbOld = pbLast;

		for (;; pbLast = (DWORD)mbi.BaseAddress + mbi.RegionSize) 
		{
			if (VirtualQuery((PVOID)pbLast, &mbi, sizeof(mbi)) <= 0) {
				pbLast = 0;
				break;
			}

			// Skip uncommitted regions and guard pages.
			//
			if ((mbi.State != MEM_COMMIT) || (mbi.Protect & PAGE_GUARD)) {
				continue;
			}

			__try {
				PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pbLast;
				if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
					continue;
				}

				PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
																pDosHeader->e_lfanew);
				if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
					continue;
				}
				if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0) {
					continue;
				}

				cpModuleName = NULL;
				DWORD ExportTableBase = pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
				if (ExportTableBase!=0)
				{
					cpModuleName = (char*)((DWORD)pbLast + ((PIMAGE_EXPORT_DIRECTORY)((DWORD)pbLast +ExportTableBase))->Name);
				} else
				{
					if (GetModuleFileName((HMODULE)pbLast, ModuleName, sizeof(ModuleName))>0)
						cpModuleName = ModuleName;
				}

				// ignore self dll
				if ((DWORD)pbLast == (DWORD)hDll)
					continue;

				if (cpModuleName!=NULL)
				{
					iNewModule = AddModuleInfo((DWORD)pbLast, pNtHeader->OptionalHeader.SizeOfImage, cpModuleName);
					if (iNewModule>=0)
					{
						wsprintf(buff, "Find module base: 0x%08X end 0x%08X name: %s", pbLast, pbLast+pNtHeader->OptionalHeader.SizeOfImage, cpModuleName);
						OutputMessage(buff);

						AddApiOfModule(iNewModule);
						HookApiOfModule(iNewModule);
					}
				}
				break;
			}
			__except(EXCEPTION_EXECUTE_HANDLER) {
				break;
			}
		}
	}

	lRet = DetourTransactionCommit();

	wsprintf(buff, "Scan find %d modules.", dwModuleCount);
	OutputMessage(buff);

	if (lRet!=NO_ERROR)
	{
		wsprintf(buff, "Failed to commit hook api total: %d. error: %d", dwApiHookCount, lRet);
		OutputMessage(buff);
	} else
	{
		wsprintf(buff, "Success to commit hook api total: %d.", dwApiHookCount);
		OutputMessage(buff);
	}

	BuildFilterListFromOption();

	STOP = FALSE;

}
__except(EXCEPTION_EXECUTE_HANDLER) {
	STOP = FALSE;
}

    return ;

}

void FillModuleListToShareMem()
{
	DWORD dwCopyCount;

	if (dpMappedMem==NULL)
		return;
	
	if (dwModuleCount*sizeof(sModuleInfo)>=SHARE_MEM_SIZE-8)
		dwCopyCount = (SHARE_MEM_SIZE-8) / sizeof(sModuleInfo);
	else
		dwCopyCount = dwModuleCount;
	*(dpMappedMem+1) = dwCopyCount;
	CopyMemory(dpMappedMem+2, ModuleList, sizeof(sModuleInfo)*dwCopyCount);
}

void FillApiListToShareMem()
{
	sApiInfo* buf; 
	DWORD i;

	if (dpMappedMem==NULL)
		return;
	
	*(dpMappedMem+1) = dwApiHookCount;
	buf = (sApiInfo*)(dpMappedMem+2);

	for (i=0;i<dwApiHookCount;i++)
	{
		if ((DWORD)buf-(DWORD)dpMappedMem >= SHARE_MEM_SIZE )
		{
			*(dpMappedMem+1) = i;
			break;
		}
		*buf = *((sApiInfo*)ApiHookList[i]);
		buf++;
	}
}

void AddValidRegion(DWORD dwBegin, DWORD dwEnd)
{
	if (dwValidFromRegionCount>=MAX_IGNOREREGIONNUM+MAX_IGNOREDLLNUM)
		return;
	ValidFromRegion[dwValidFromRegionCount].BeginAddr = dwBegin;
	ValidFromRegion[dwValidFromRegionCount].EndAddr = dwEnd;
	dwValidFromRegionCount ++;
}

void AddInvalidRegion(DWORD dwBegin, DWORD dwEnd)
{
	if (dwInvalidFromRegionCount>=MAX_IGNOREREGIONNUM+MAX_IGNOREDLLNUM)
		return;
	InvalidFromRegion[dwInvalidFromRegionCount].BeginAddr = dwBegin;
	InvalidFromRegion[dwInvalidFromRegionCount].EndAddr = dwEnd;
	dwInvalidFromRegionCount ++;
}

void BuildFilterListFromOption()
{
	DWORD i, j;

	if (Option.dwIgnoreAPIs != NO_SPECIAL_APIS)
	{
		for (i=0;i<dwApiHookCount;i++)
		{
			if (Option.dwIgnoreAPIs==JUST_CALL_SPECIAL_APIS)
				((sApiInfo*)ApiHookList[i])->IsFiltered = TRUE;
			else
				((sApiInfo*)ApiHookList[i])->IsFiltered = FALSE;

			for (j=0;j<Option.wIgnoreAPINum;j++)
			{
				if (stricmp((char*)((sApiInfo*)ApiHookList[i])->ApiName, Option.cIgnoreAPIs[j])==0)
				{
					if (Option.dwIgnoreAPIs==DONT_CALL_SPECIAL_APIS)
						((sApiInfo*)ApiHookList[i])->IsFiltered = TRUE;
					else
						((sApiInfo*)ApiHookList[i])->IsFiltered = FALSE;
				}
			}
		}
	}
	if (Option.dwIgnoreToDlls != NO_SPECIAL_DLLS)
	{
		for (i=0;i<dwModuleCount;i++)
		{
			if (Option.dwIgnoreToDlls==JUST_SPECIAL_DLL_CALLS)
				ModuleList[i].IsFiltered = TRUE;
			else
				ModuleList[i].IsFiltered = FALSE;

			for (j=0;j<Option.wIgnoreToDllNum;j++)
			{
				if (stricmp((char*)ModuleList[i].ModuleName, Option.cIgnoreToDlls[j])==0)
				{
					if (Option.dwIgnoreToDlls==DONT_SPECIAL_DLL_CALLS)
						ModuleList[i].IsFiltered = TRUE;
					else
						ModuleList[i].IsFiltered = FALSE;
				}
			}
		}
	}

	dwValidFromRegionCount = 0;
	dwInvalidFromRegionCount = 0;

	// exe image is always valid
	AddValidRegion(TargetModuleBase, TargetModuleEnd);

	if (Option.dwIgnoreFromDlls != NO_SPECIAL_DLLS)
	{
		for (i=0;i<dwModuleCount;i++)
		{
			for (j=0;j<Option.wIgnoreFromDllNum;j++)
			{
				if (stricmp((char*)ModuleList[i].ModuleName, Option.cIgnoreFromDlls[j])==0)
				{
					if (Option.dwIgnoreFromDlls==DONT_SPECIAL_DLL_CALLS)
					{
						//AddInvalidRegion(ModuleList[i].BaseAddr, ModuleList[i].BaseAddr+ModuleList[i].Size);
					}
					else
					{
						AddValidRegion(ModuleList[i].BaseAddr, ModuleList[i].BaseAddr+ModuleList[i].Size);
					}
				}
			}
		}
	}
	//if (Option.dwIgnoreRegions != NO_SPECIAL_DLLS)
	//{
	//	for (i=0;i<dwModuleCount;i++)
	//	{
	//		for (j=0;j<Option.wIgnoreRegionNum;j++)
	//		{
	//			if (Option.dwIgnoreRegionBegin[j]<Option.dwIgnoreRegionEnd[j])
	//			{
	//				if (Option.dwIgnoreFromDlls==DONT_SPECIAL_DLL_CALLS)
	//				{
	//					AddInvalidRegion(Option.dwIgnoreRegionBegin[j], Option.dwIgnoreRegionEnd[j]);
	//				}
	//				else
	//				{
	//					AddValidRegion(Option.dwIgnoreRegionBegin[j], Option.dwIgnoreRegionEnd[j]);
	//				}
	//			}
	//		}
	//	}
	//}
}

BOOL IsFilteredApi(DWORD dwApiIndex)
{
	DWORD dwModuleIndex;
	if (dwApiIndex>=dwApiHookCount)
		return TRUE;

	if (((sApiInfo*)ApiHookList[dwApiIndex])->IsFiltered)
		return TRUE;
	dwModuleIndex =((sApiInfo*)ApiHookList[dwApiIndex])->ModuleIndex;
	if (ModuleList[dwModuleIndex].IsFiltered)
		return TRUE;
	return FALSE;
}

BOOL IsFilteredFromAddr(DWORD dwAddr)
{
	DWORD i;

	if (dwValidFromRegionCount>0)
	{
		for (i=0;i<dwValidFromRegionCount;i++)
			if (ValidFromRegion[i].BeginAddr <= dwAddr && ValidFromRegion[i].EndAddr > dwAddr )
				return FALSE;
		return TRUE;
	}
	for (i=0;i<dwValidFromRegionCount;i++)
		if (ValidFromRegion[i].BeginAddr <= dwAddr && ValidFromRegion[i].EndAddr > dwAddr )
			return TRUE;
	return FALSE;
}

// end add by ygq

BOOL CreateThreadRetStack()
{
	VOID* pMem;

	if (!(pMem = (VOID*)VirtualAlloc(NULL, RET_STACK_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE)))
	{
		wsprintf(buff, "Create tls error.");
		OutputMessage(buff);
		return FALSE;
	}
	// The second tls table (dwMemTlsInd) contains always the start addresses
	// of the allocated blocks !
	TlsSetValue(dwMemTlsInd, pMem);
	// the RET Stack pointers start at the end of the mem !
	pMem = LPVOID((DWORD)pMem + RET_STACK_SIZE);
	TlsSetValue(dwTlsInd,pMem);
	return TRUE;
}

BOOL FreeThreadRetStack()
{
	DWORD dwMemBase;

	dwMemBase = (DWORD)TlsGetValue(dwMemTlsInd);
	if (!dwMemBase)
		return FALSE;
	if (VirtualFree((VOID*)dwMemBase, 0, MEM_RELEASE) == 0)
		return FALSE;
	return TRUE;
}

VOID __stdcall PushRetStack(DWORD dwValue, DWORD dwApiNum)
{
	sThRetStack*   pRetStack;

	pRetStack = (sThRetStack*)TlsGetValue(dwTlsInd);
	if (pRetStack==0)
	{
		if (GetLastError()==NO_ERROR)
		{
			// the thread has not set tls
			pRetStack = 0;
			CreateThreadRetStack();
			pRetStack = (sThRetStack*)TlsGetValue(dwTlsInd);
			if (pRetStack==0)
				return;
		} else
			return;
	}
	--pRetStack;
	pRetStack->pRetAddr = (LPVOID)dwValue;
	pRetStack->dwAPINum = dwApiNum;
	TlsSetValue(dwTlsInd,(LPVOID)pRetStack);

	return;
}

DWORD __stdcall PopRetStack(void* pEax, DWORD dwApiRetValue)
{
	sThRetStack*   pRetStack;
	DWORD          dwRetValue;
	//APIRETURNINFO  ARI;
	//COPYDATASTRUCT CDS;
	DWORD          ApiIndex;

	pRetStack = (sThRetStack*)TlsGetValue(dwTlsInd);
	dwRetValue = (DWORD)pRetStack->pRetAddr;

	// report the return value to the GUI
	//ARI.pRetValue  = pEax;
	//ARI.dwRetValue = dwApiRetValue;
	//ARI.dwApi      = pRetStack->dwAPINum;
	//CDS.lpData = &ARI;
	//CDS.cbData = sizeof(ARI);
	//CDS.dwData = CD_ID_APIRETURN_DYN;
	//SendMessage(
	//	hSSDlg,
	//	WM_COPYDATA,
	//	(WPARAM)0,
	//	(LPARAM)&CDS);

	ApiIndex = pRetStack->dwAPINum;

	// handle stack pointer
	++pRetStack;
	TlsSetValue(dwTlsInd, pRetStack);

	if (Option.bShowApiReturn)
	//if ( dwRetValue>=TargetModuleBase && dwRetValue<=TargetModuleEnd )
	{
		wsprintf(buff, "Success to ret from api: %s with ret value: %x.", ((sApiHookInfo*)(ApiHookList[ApiIndex]))->ApiName, *(DWORD*)pEax);
		OutputMessage(buff);
	}

	return dwRetValue;
}

extern "C" VOID __declspec(naked) __stdcall RetTrapProc()
{
	// continue execution at the real return address
	__asm
	{
#ifdef RET_DBG
		INT   3
#endif			
	    SUB    ESP, 4           // get space for the new return value
        PUSHAD                  // save all registers

		PUSH   EAX              // push current return value
		LEA    EAX, [ESP + 020h]
		PUSH   EAX              // push address of return value
		CALL   PopRetStack      // get the real return value

	    MOV    [ESP+020h], EAX  // mov it to the free space
        POPAD                   // restore all registers
		RET                     // return to real return address
	}
}

extern "C" VOID __stdcall TrappedApiCall(DWORD dwApiNum,VOID* pStack)
{
	char *cParams;

	if (STOP || STOP_OUTPUT)
		return;

	pStack = (VOID*)((DWORD)pStack + 0x20);  // add the size of "pushad"

	// if called from this module, ignore it
	if ( *(DWORD*)pStack>=SelfModuleBase && *(DWORD*)pStack<=SelfModuleEnd )
		return; 
	//if (IsAddrInApiBlock(*(DWORD*)pStack))
	//	return;

	// ignore call not from target space
	//if ( *(DWORD*)pStack>=TargetModuleEnd || *(DWORD*)pStack<=TargetModuleBase )
	//	return;
	
	if (IsFilteredFromAddr(*(DWORD*)pStack))
		return;
	if (IsFilteredApi(dwApiNum))
		return;

	if (Option.bShowAPICall)
	{
		wsprintf(buff, "Trapped api: %s(%s). Called from %08X.", ((sApiHookInfo*)(ApiHookList[dwApiNum]))->ApiName, \
			(char*)ModuleList[((sApiHookInfo*)(ApiHookList[dwApiNum]))->ModuleIndex].ModuleName, *(DWORD*)pStack);
		OutputMessage(buff);
	}

	if (Option.bShowApiParams)
	{
		cParams = GetApiParam((char*)ModuleList[((sApiHookInfo*)(ApiHookList[dwApiNum]))->ModuleIndex].ModuleName,
				(char*)((sApiHookInfo*)(ApiHookList[dwApiNum]))->ApiName, NULL, (VOID*)pStack);
		if (cParams!=NULL)
		{
			wsprintf(buff, "Trapped api param: %s.", cParams);
			OutputMessage(buff);
		}
	}

	// change the return value >:-)
	PushRetStack(
		*(DWORD*)pStack,
		dwApiNum);
	*(DWORD*)pStack = dwRetProcAddr;

	// send the GUI a message with the index of the called API
	//SendMessage(hSSDlg,WM_APICALL_DYN,dwApiNum,(LPARAM)pStack);

	return;
}

BOOL InitApiStubStruct(sApiStub *tmpApiStub)
{
	// paste the values which are always the same
#ifdef API_DBG
	tmpApiStub->Int3        = 0xCC;
#endif
	tmpApiStub->byPushadOpc = 0x60;
	tmpApiStub->byPushEsp   = 0x54;
	tmpApiStub->byPushOpc   = 0x68;
	tmpApiStub->byCallOpc   = 0xE8;
    tmpApiStub->byJmpOpcApi = 0xE9;
	tmpApiStub->byPopadOpc  = 0x61;
	dwTrapFunctAddr = (DWORD)GetProcAddress((HINSTANCE)hDll,TRAPFUNCTION_NAME);
	if (!dwTrapFunctAddr)
		return FALSE;
	else
		return TRUE;
}

