#include <windows.h>
#include <winioctl.h>
#include <tchar.h>
#include <stdio.h>
#include <psapi.h>


#include "_SYS_BIN.h"

#pragma comment (lib,"psapi.lib")

#define	IOCTL_PEDIY		(ULONG) CTL_CODE(FILE_DEVICE_UNKNOWN,0x00000100,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define	IOCTL_GETSTR	(ULONG) CTL_CODE(FILE_DEVICE_UNKNOWN,0x00000101,METHOD_BUFFERED,FILE_ANY_ACCESS)

typedef struct _INBUF
{
	DWORD dwDebugerPID;
	DWORD dwPID;
//	DWORD dwNum_NtDelayExecution;
	DWORD dwNum_NtGetContextThread;
	DWORD dwNum_NtSetContextThread;
//	DWORD dwNum_NtContinue;
}INBUF;


#define MAX_MODULE_COUNT		400
#define MAX_THREAD_COUNT		400
typedef struct _LoadedModule
{
	LPVOID lpModuleBase;			//模块的载入后的基地址
	char szPath[MAX_PATH];			//模块路径
	DWORD dwModuleSize;				//模块大小
}LoadedModule;

typedef struct _THREAD
{
	DWORD dwThreadID;
	HANDLE hThread;

	DWORD dwDRXSave[6];
}THREAD;



//DWORD gdwEOP = 0;					//exe  的 eop地址
//BYTE gcbEOPValue = 0;				//exe 的eop处的字节

BYTE* pKiUserExceptionDispatcher = NULL;	//KiUserExceptionDispatcher 附近处的断点地址
BYTE* pZwContinue = NULL;


DWORD g_dwPid = 0;
HANDLE g_hProcess = NULL;

DWORD g_dwCurrTid = 0;				//当前线程

DWORD gdwPEB = 0;

DWORD lpBaseOfImage = 0;
DWORD lpEndOfImage = 0;
LoadedModule* g_Modules = NULL;
THREAD* g_Threads = NULL;
int g_nModules = 0;
int g_nThreads = 0;

int int3 = 0;

BOOL gbAutodebugging = FALSE;


BOOL bExit = FALSE;
HANDLE g_hDrvMonitorThread = NULL;
HANDLE hDevice = NULL;


DWORD gdrx[4] = {0,0,0,0};			//dr1 = pKiUserExceptionDispatcher, dr2 = pZwContinue
DWORD gdr7 = 0;



const THREAD* GetThreadFromTID(DWORD dwThreadId);
//当前线程
const THREAD* GetThread(CONTEXT* con)
{
	memset(con, 0, sizeof(CONTEXT));
	con->ContextFlags = CONTEXT_FULL;

	const THREAD* p = GetThreadFromTID(g_dwCurrTid);
	
	if(p != NULL)
	{
		GetThreadContext(p->hThread, con);
	}
	return p;
}

//当前线程
void SetThread(CONTEXT* con)
{
	const THREAD* p = GetThreadFromTID(g_dwCurrTid);
	if(p != NULL)
	{
		SetThreadContext(p->hThread, con);
	}
}

void SetSingleDebug()
{
	CONTEXT con = {0};
	GetThread(&con);
	con.EFlags |= 0x100;
	SetThread(&con);
}

void CancelSingleDebug()
{
	CONTEXT con = {0};
	GetThread(&con);
	con.EFlags &= ~0x100;
	SetThread(&con);
}


BOOL ReadMemory(LPVOID pAddr, int nSize, LPVOID pSave)
{
	DWORD r = 0;
	if(!ReadProcessMemory(g_hProcess, pAddr, pSave, nSize, &r))
		return FALSE;
	return (int)r == nSize;
}

BOOL WriteMemory(LPVOID pAddr, LPVOID pData, int nSize)
{
	DWORD w = 0;
	return WriteProcessMemory(g_hProcess, pAddr, pData, nSize, &w);
}

void HardBPSetDR7(DWORD* dr7, 
				  int i,	 //0,1,2,3
				  int ewr, //0,1,3		-1 = clear
				  int len) //0,1,3
{
	if(ewr == 0)
		len = 0;
	
	DWORD v = *dr7;
	
	//先清除相应的位
	DWORD c = (0xf << (16+4*i)) | (3 << 2*i);
	c = ~c;
	v &= c;
	
	if(ewr != -1)
	{
		//重新设置
		int l_r_w = (len << 2)|ewr;
		c = ((l_r_w&0xf) << (16+4*i)) | (1/*L*/ << 2*i);
		v |= c;
	}
	
	*dr7 = v;
}

void SetHardBP(DWORD dwAddr, int i, int ewr, int len)
{	
	HardBPSetDR7(&gdr7, i, ewr, len);

	gdrx[i] = dwAddr;
	
	CONTEXT con;

	int c = 0;
	
	for(int j=0; j<MAX_THREAD_COUNT; j++)
	{
		if(g_Threads[j].hThread != NULL)
		{
			memset(&con, 0, sizeof(con));
			con.ContextFlags = CONTEXT_DEBUG_REGISTERS;
			GetThreadContext(g_Threads[j].hThread, &con);
			
			con.Dr0 = gdrx[0];
			con.Dr1 = gdrx[1];
			con.Dr2 = gdrx[2];
			con.Dr3 = gdrx[3];
			
			
			con.Dr7 = gdr7;
			SetThreadContext(g_Threads[j].hThread, &con);

			//保存
			memcpy(g_Threads[j].dwDRXSave, &con.Dr0, sizeof(g_Threads[j].dwDRXSave));


			c++;

			if(c >= g_nThreads)
				break;
		}
	}
	
}

void SetHardExeBP(DWORD dwAddr, int i)
{
	SetHardBP(dwAddr, i, 0, 0);
}

void DeleteHardBP(int i)
{
	HardBPSetDR7(&gdr7, i, -1, -1);

	gdrx[i] = 0;
	
	CONTEXT con;
	int c = 0;
	
	for(int j=0; j<MAX_THREAD_COUNT; j++)
	{
		if(g_Threads[j].hThread != NULL)
		{
			memset(&con, 0, sizeof(con));
			con.ContextFlags = CONTEXT_DEBUG_REGISTERS;
			GetThreadContext(g_Threads[j].hThread, &con);
			
			con.Dr0 = gdrx[0];
			con.Dr1 = gdrx[1];
			con.Dr2 = gdrx[2];
			con.Dr3 = gdrx[3];

			con.Dr7 = gdr7;
			SetThreadContext(g_Threads[j].hThread, &con);
		
			//保存
			memcpy(g_Threads[j].dwDRXSave, &con.Dr0, sizeof(g_Threads[j].dwDRXSave));

			c++;
			
			if(c >= g_nThreads)
				break;

		}
	}
}




/*
DWORD GetEntryPointAddrByHandle(IN const HANDLE hFile, IN const DWORD dwImgBase, OUT DWORD* pdwRawImgBase)
{
	DWORD dwPEHeaderStart = 0;
	DWORD dwReaded = 0;
	IMAGE_OPTIONAL_HEADER32 ImgOpt = {0};
		
	
	SetFilePointer(hFile, 0x3C, NULL, FILE_BEGIN);
	if(!ReadFile(hFile, &dwPEHeaderStart, 4, &dwReaded,NULL))
		return 0;
	
	SetFilePointer(hFile, dwPEHeaderStart + 4 + sizeof(IMAGE_FILE_HEADER), NULL, FILE_BEGIN);
	
	if(!ReadFile(hFile, &ImgOpt, sizeof(ImgOpt), &dwReaded, NULL))
		return 0;
	
	if(ImgOpt.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		return 0;
	}
	
	if(pdwRawImgBase != NULL)
	{
		*pdwRawImgBase = ImgOpt.ImageBase;
	}
	
	if(dwImgBase == 0)
	{
		return (ImgOpt.ImageBase + ImgOpt.AddressOfEntryPoint);
	}
	else
	{
		return (dwImgBase + ImgOpt.AddressOfEntryPoint);
	}
}

DWORD GetEntryPointAddr(LPCTSTR lpsExeName, const DWORD dwImgBase)
{
	HANDLE hFile;
	DWORD dwRet;
		
	hFile = CreateFile(lpsExeName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	
	dwRet = GetEntryPointAddrByHandle(hFile, dwImgBase, NULL);
	CloseHandle(hFile);
	
	return dwRet;
	
}
*/






void AddThread(DWORD dwThreadID, HANDLE hThreadHandle, BOOL bCreateProcess)
{
	for(int i=0; i<MAX_THREAD_COUNT; i++)
	{
		if(g_Threads[i].hThread == NULL)
		{
			g_Threads[i].dwThreadID = dwThreadID;
			memset(g_Threads[i].dwDRXSave, 0, sizeof(g_Threads[i].dwDRXSave));

			BOOL bSuc = DuplicateHandle(GetCurrentProcess(),
				hThreadHandle,
				GetCurrentProcess(),
				&(g_Threads[i].hThread),
				THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_ALL_ACCESS,
				FALSE,
				0);
			
			if(!bSuc)
			{
				bSuc = DuplicateHandle(GetCurrentProcess(),
					hThreadHandle,
					GetCurrentProcess(),
					&(g_Threads[i].hThread),
					THREAD_ALL_ACCESS,
					FALSE,
					DUPLICATE_SAME_ACCESS);
			}

			CONTEXT con;
			memset(&con, 0, sizeof(con));
			con.ContextFlags = CONTEXT_DEBUG_REGISTERS;
			GetThreadContext(g_Threads[i].hThread, &con);

			if(!bCreateProcess)
			{
				
				con.Dr0 = gdrx[0];
				con.Dr1 = gdrx[1];
				con.Dr2 = gdrx[2];
				con.Dr3 = gdrx[3];
				
				
				con.Dr7 = gdr7;
				SetThreadContext(g_Threads[i].hThread, &con);

				//保存 
				memcpy(g_Threads[i].dwDRXSave, &con.Dr0, sizeof(g_Threads[i].dwDRXSave));
			}

			g_nThreads++;
			break;
		}
	}
}

void DbgExitThread(DWORD dwThreadId)
{
	for(int i=0; i<MAX_THREAD_COUNT; i++)
	{
		if(g_Threads[i].hThread != NULL && g_Threads[i].dwThreadID == dwThreadId)
		{
			CloseHandle(g_Threads[i].hThread);
			g_Threads[i].hThread = NULL;
			g_Threads[i].dwThreadID = 0;
			g_nThreads--;
			break;
		}
	}
}

const THREAD* GetThreadFromTID(DWORD dwThreadId)
{
	for(int i=0; i<MAX_THREAD_COUNT; i++)
	{
		if(g_Threads[i].dwThreadID == dwThreadId)
		{
			return &g_Threads[i];
		}
	}
	return NULL;
}

void AddModule(LPVOID lpModBase, char* pszModPath, int nModsize)
{

	for(int i=0; i<MAX_MODULE_COUNT; i++)
	{
		if(g_Modules[i].lpModuleBase == NULL)
		{
			g_Modules[i].lpModuleBase = lpModBase;
			g_Modules[i].dwModuleSize = nModsize;
			strcpy(g_Modules[i].szPath, pszModPath);

			g_nModules++;
			break;
		}
	}
}

/*
由PE文件的 HANDLE 得到PE文件的模块大小
*/
DWORD GetModuleSize(HANDLE hFile)
{
	DWORD dwPEStart;
	DWORD dwReaded;
	IMAGE_OPTIONAL_HEADER Img;
	
	if(hFile == NULL)
	{
		return 0;
	}
	
	SetFilePointer(hFile, 0x3C, NULL, FILE_BEGIN);
	ReadFile(hFile, &dwPEStart, sizeof(DWORD), &dwReaded, NULL);
	if(dwReaded != sizeof(DWORD))
	{
		return 0;
	}
	
	SetFilePointer(hFile, dwPEStart+4+sizeof(IMAGE_FILE_HEADER), NULL, FILE_BEGIN);
	if(ReadFile(hFile, &Img, sizeof(Img), &dwReaded, NULL))
	{
		return Img.SizeOfImage;
	}
	return 0;
}

DWORD GetEXEModuleSize(HANDLE hFile, char* pszFilePath)
{
	if(hFile == NULL)
	{
		hFile = CreateFile(pszFilePath, 
			GENERIC_READ, 
			FILE_SHARE_READ|FILE_SHARE_WRITE, 
			NULL, 
			OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, 
			NULL);
	}
	
	return GetModuleSize(hFile);
}

/*
void SetBPAtEOP()
{
	//在eop处设置断点，防止在eop前调用F函数
	ReadMemory((LPVOID)gdwEOP, 1, &gcbEOPValue);
	BYTE cc = 0xcc;
	WriteMemory((LPVOID)gdwEOP, &cc, 1);
}

void ClearBPAtEOP()
{
	CONTEXT con;
	GetThread(&con);
	con.Eip = gdwEOP;
	SetThread(&con);

	WriteMemory((LPVOID)gdwEOP, &gcbEOPValue, 1);
	FlushInstructionCache(g_hProcess, (LPVOID)gdwEOP, 1);
}*/

//pKiUserExceptionDispatcher : 一种是运行时触发异常，一种是单步执行时触发异常
//单步执行时，停在第二条语句

void SetHardBPAt_KiUserExceptionDispatcher()
{
	BYTE* p = (BYTE*)GetProcAddress(GetModuleHandle("ntdll.dll"), "KiUserExceptionDispatcher");
	if(p != NULL)
	{
		// 找第2条指令

		/*
		770199E8  FC                    cld		//vista下有，2003下没有
		770199E9  8B 4C 24 04           mov         ecx,[esp+0x4]
		770199ED  8B 1C 24              mov         ebx,[esp]
		*/
		if(p[0] == 0xFC)
		{
			p+=1;
			//判断下一条
			if(*(DWORD*)p != 0x04244C8B)
			{
				return;
			}

		}
		else if(*(DWORD*)p == 0x04244C8B)
		{
			p+=4;
			//判断下一条
			if(p[0] != 0x8B && p[1] != 0x1C && p[2] != 0x24)
			{
				return;
			}
		}
		else
		{
			p = NULL;
		}
		if(p != NULL)
		{
			pKiUserExceptionDispatcher = p;
			
			SetHardExeBP((DWORD)pKiUserExceptionDispatcher, 1);

			//再hook ZwContinue;
			p = (BYTE*)GetProcAddress(GetModuleHandle("ntdll.dll"), "ZwContinue");
			if(p != NULL)
			{
				pZwContinue = p;
				SetHardExeBP((DWORD)pZwContinue, 2);
			}	
		}

	}


}

#define BUFSIZE 512
/*
	根据 FILE 的句柄得到文件的全路径
	如果成功，返回TRUE
*/
BOOL GetFileNameFromHandle(IN HANDLE hFile, OUT TCHAR* pszFilename) 
{
	BOOL bSuccess = FALSE;
    DWORD dwFileSizeHi = 0;
	DWORD dwFileSizeLo;
	HANDLE hFileMap;

	pszFilename[0] = 0;

	if(hFile == NULL || hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}


  // Get the file size.
  dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi); 

  // Create a file mapping object.
  hFileMap = CreateFileMapping(hFile, 
                          NULL, 
                          PAGE_READONLY,
                          0, 
                          dwFileSizeLo,
                          NULL);

  if (hFileMap) 
  {
    // Create a file mapping to get the file name.
    void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);

    if (pMem) 
    {
      if (GetMappedFileName (GetCurrentProcess(), 
                             pMem, 
                             pszFilename,
                             MAX_PATH)) 
      {

        // Translate path with device name to drive letters.
        TCHAR szTemp[BUFSIZE];
        szTemp[0] = '\0';

        if (GetLogicalDriveStrings(BUFSIZE-1, szTemp)) 
        {
          TCHAR szName[MAX_PATH];
          TCHAR szDrive[3] = TEXT(" :");
          BOOL bFound = FALSE;
          TCHAR* p = szTemp;

          do 
          {
            // Copy the drive letter to the template string
            *szDrive = *p;

            // Look up each device name
            if (QueryDosDevice(szDrive, szName, BUFSIZE))
            {
              UINT uNameLen = _tcslen(szName);

              if (uNameLen < MAX_PATH) 
              {
                bFound = _tcsnicmp(pszFilename, szName, 
                    uNameLen) == 0;

                if (bFound) 
                {
                  // Reconstruct pszFilename using szTemp
                  // Replace device path with DOS path
                  TCHAR szTempFile[MAX_PATH];
                  _stprintf(szTempFile,
                            TEXT("%s%s"),
                            szDrive,
                            pszFilename+uNameLen);
                  _tcsncpy(pszFilename, szTempFile, MAX_PATH);
                }
              }
            }

            // Go to the next NULL character.
            while (*p++);
          } while (!bFound && *p); // end of string
        }
      }
      bSuccess = TRUE;
      UnmapViewOfFile(pMem);
    } 

    CloseHandle(hFileMap);
  }

  if(!bSuccess)
  {
	  pszFilename[0] = 0;
  }
  return(bSuccess);
}

void DbgDllLoad(DEBUG_EVENT de, HANDLE hProcess)
{
	DWORD dwAddr = 0;
	ReadMemory(de.u.LoadDll.lpImageName, 4,  &dwAddr);

	char dststr[MAX_PATH*2] = {0};

	if(dwAddr == 0)
	{
		//装载NTDLL.DLL时DWADDR为0
		if(GetModuleHandle("NTDLL.DLL") == de.u.LoadDll.lpBaseOfDll)
		{
			//就是NTDLL.DLL了
			GetSystemDirectory(dststr, MAX_PATH);
			strcat(dststr, "\\NTDLL.DLL");
		}
		else
		{
			GetModuleFileNameEx(hProcess, (HMODULE)de.u.LoadDll.lpBaseOfDll, dststr, sizeof(dststr));
		}
	}
	else
	{
		char rawstr[MAX_PATH*2] = {0};
		ReadMemory((LPVOID)dwAddr, sizeof(rawstr), rawstr);

		if(de.u.LoadDll.fUnicode)
		{
			WideCharToMultiByte(CP_ACP, 0x400, rawstr, 
								-1, 
								dststr, 
								sizeof(dststr),
								NULL,NULL);

		}
		else
		{
			strcpy(dststr, rawstr);
		}
	}

	if(de.u.LoadDll.hFile == NULL && dststr[0] != 0)
	{
		HANDLE hFile = CreateFile(dststr, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile != INVALID_HANDLE_VALUE)
		{
			de.u.LoadDll.hFile = hFile;
		}
	}

	if(dststr[0] == 0 || strchr(dststr, '\\') == NULL)
	{
		//根据文件的句柄去取文件的路径
		GetFileNameFromHandle(de.u.LoadDll.hFile, dststr);
	}


	DWORD dwImgSize = GetModuleSize(de.u.LoadDll.hFile);

	AddModule(de.u.LoadDll.lpBaseOfDll, dststr, dwImgSize);


}


void DbgDllUnLoad(DEBUG_EVENT* de)
{
	for(int i=0; i<MAX_MODULE_COUNT; i++)
	{
		if((DWORD)g_Modules[i].lpModuleBase == (DWORD)de->u.UnloadDll.lpBaseOfDll)
		{			
			
			
			g_Modules[i].dwModuleSize = 0;
			g_Modules[i].lpModuleBase = NULL;
			g_Modules[i].szPath[0] = 0;

			g_nModules--;
			break;
		}
	}

}

//得到进程的PEB地址
DWORD GetProcessPEB(HANDLE hProcess)
{
	typedef LONG (__stdcall *NTQUERYINFOMATIONPROCESS)(
		HANDLE ProcessHandle,
		UINT ProcessInformationClass,
		PVOID ProcessInformation,
		ULONG ProcessInformationLength,
		PULONG ReturnLength
		);
	typedef struct _PROCESS_BASIC_INFORMATION { 
		LONG ExitStatus; 
		PVOID PebBaseAddress; 
		ULONG AffinityMask; 
		ULONG BasePriority; 
		ULONG UniqueProcessId; 
		ULONG InheritedFromUniqueProcessId; 
	} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION; 
	
	PROCESS_BASIC_INFORMATION proc_basic_info = {0};
	HMODULE hNTDLL = LoadLibrary("ntdll.dll");
	if(hNTDLL != NULL)
	{
		NTQUERYINFOMATIONPROCESS NtQueryInformationProcess = 
			(NTQUERYINFOMATIONPROCESS)GetProcAddress(hNTDLL, "NtQueryInformationProcess");
		if(NtQueryInformationProcess != NULL)
		{
			ULONG tret=0;
			if(NtQueryInformationProcess(hProcess, 0, &proc_basic_info, sizeof proc_basic_info, &tret) == 0)
			{
				return (DWORD)proc_basic_info.PebBaseAddress;
			}
		}
	}
	return 0;
}

DWORD GetPEBFromTEB(DWORD dwTEB)
{
	//取 PEB
	// fs:0x30 是 PEB
	DWORD dwPEB = 0;
	
	ReadMemory((LPVOID)(dwTEB + 0x30), 4, &dwPEB);
	if(dwPEB == 0)
	{
		dwPEB = GetProcessPEB(g_hProcess);
	}

	return dwPEB;
}

void Hide_DebugFlag()
{
	//去掉 PEB中被调试的标志位
	
	if(gdwPEB != 0)
	{
		bool dbgFlag;
		ReadMemory((LPVOID)(gdwPEB+2), 1, &dbgFlag);
		if(dbgFlag)
		{
			dbgFlag = false;
			WriteMemory((LPVOID)(gdwPEB+2), (BYTE*)&dbgFlag, 1);
		}

	}
}

BOOL IsVistaOrLater()
{
	DWORD v = GetVersion();
	return ((BYTE)v) >= 6;
}
 

void HideNtGlobalFlag()
{
	if(gdwPEB != 0)
	{
		//NtGlobalFlag（偏移0x68）

		DWORD d = 0;
		WriteMemory((LPVOID)(gdwPEB+0x68), (BYTE*)&d, 4);

		//Heap.HeapFlags, Heap.ForceFlags
		DWORD ProcessHeap = 0;
		ReadMemory((LPVOID)(gdwPEB+0x18), 4, &ProcessHeap);

		if(!IsVistaOrLater())	//vista的值不一样
		{
			if(ProcessHeap != 0)
			{
				d = 2;
				WriteMemory((LPVOID)(ProcessHeap+0xc), (BYTE*)&d, 4);	//PEB.ProcessHeap.Flags

				d = 0;
				WriteMemory((LPVOID)(ProcessHeap+0x10), (BYTE*)&d, 4);	//PEB.ProcessHeap.ForceFlags
			}
		}

	}
}





DWORD DbgExp(DEBUG_EVENT* pDebugEvt, DWORD dwExpCode, DWORD dwExpAddr)
{
	DWORD dwRet = DBG_CONTINUE;
	switch(dwExpCode)
	{
	case EXCEPTION_BREAKPOINT:
		if(int3 == 0)
		{

			Hide_DebugFlag();
			HideNtGlobalFlag();

			printf("DebugFlag and NtGlobalFlag hided\n");
			
		//	SetBPAtEOP();		

			SetHardBPAt_KiUserExceptionDispatcher();

		}
	/*	else if(dwExpAddr == gdwEOP)
		{
			ClearBPAtEOP();
			
			//给F函数设置硬件断点
			//此时F函数所在的模块可能还没有加载
			
		}*/
		else
		{
			printf("dwExpCode = 0x%08X, dwExpAddr = 0x%08X\n", dwExpCode, dwExpAddr);

			dwRet = DBG_EXCEPTION_NOT_HANDLED;
			if(pDebugEvt->u.Exception.dwFirstChance == 0)
			{
				dwRet = DBG_CONTINUE;
			}
		}
		int3++;
		break;
	case EXCEPTION_SINGLE_STEP:
		if(dwExpAddr != 0 && dwExpAddr == (DWORD)pKiUserExceptionDispatcher)
		{
		//	printf("KiUserExceptionDispatcher\n");

			//发生了异常,清掉drx
			CONTEXT con;
			THREAD* pThread = (THREAD*)GetThread(&con);

			//pKiUserExceptionDispatcher  KiUserExceptionDispatcher 函数的下一条指令
			//[esp+0x4] = context*
			DWORD dwESP[2];
			ReadMemory((LPVOID)(con.Esp), 
				8, 
				dwESP);

			//清除 dr0-dr7, 
			CONTEXT rcon;
			ReadMemory((LPVOID)(dwESP[1]), sizeof(rcon), &rcon);
			if(pThread != NULL)
			{
				memcpy(pThread->dwDRXSave, &rcon.Dr0, sizeof(pThread->dwDRXSave));
			}

			rcon.Dr0 = rcon.Dr1 = rcon.Dr2 = rcon.Dr3 = rcon.Dr6 = rcon.Dr7 = 0;
			//清除单步调试, 如果不清除单步调试，那么下一个运行的函数是 KiUserExceptionDispatcher
			if(gbAutodebugging)
			{
				rcon.EFlags &= (~0x100);
			}

			WriteMemory((LPVOID)(dwESP[1]), &rcon, sizeof(rcon));

			//eip设置到下一句，模拟执行本条语句。否则一直会出现这个异常
			//8B 4C 24 04           mov         ecx,[esp+0x4] 或 
			//8B 1C 24              mov         ebx,[esp]

			if(pKiUserExceptionDispatcher[1] == 0x4C)
			{
				con.Ecx = dwESP[1];
				con.Eip += 4;
			}
			else
			{
				con.Ebx = dwESP[0];
				con.Eip += 3;
			}

			SetThread(&con);

		}
		else if(dwExpAddr != 0 && dwExpAddr == (DWORD)pZwContinue)
		{
		//	printf("ZwContinue\n");
			
			// ZwContinue 的第一个参数是  context*
			CONTEXT con;
			const THREAD* pThread = GetThread(&con);

			//恢复context
			DWORD context = 0;
			ReadMemory((LPVOID)(con.Esp+4), 4, &context);

			CONTEXT rcon={0};
			ReadMemory((LPVOID)context, sizeof(rcon), &rcon);

			//如果是单步自动执行的时候出现异常，那么还要恢复单步调试!
			if(gbAutodebugging)
			{
				rcon.EFlags |= 0x100;
			}
				
			rcon.ContextFlags |= CONTEXT_DEBUG_REGISTERS;
			if(pThread != NULL)
			{
				memcpy(&rcon.Dr0, pThread->dwDRXSave, sizeof(pThread->dwDRXSave));
			}
			WriteMemory((LPVOID)(context), &rcon, sizeof(rcon));
			
			//eip设置到下一句，模拟执行本条语句。否则一直会出现这个异常
			//应该是 B8 XX XX XX XX        mov         eax,XXXXXXXX
			con.Eax = *(DWORD*)(pZwContinue+1);
			con.Eip += 5;
			SetThread(&con);

		}
		else
		{
			printf("EXCEPTION_SINGLE_STEP, addr = 0x%08X\n", dwExpAddr);
			dwRet = DBG_EXCEPTION_NOT_HANDLED;
			if(pDebugEvt->u.Exception.dwFirstChance == 0)
			{
				dwRet = DBG_CONTINUE;
			}

		}
		break;
	default:
	//	printf("dwExpCode = 0x%08X, dwExpAddr = 0x%08X\n", dwExpCode, dwExpAddr);
		dwRet = DBG_EXCEPTION_NOT_HANDLED;
		if(pDebugEvt->u.Exception.dwFirstChance == 0)
		{
			dwRet = DBG_CONTINUE;
		}
		break;
	}

	
	return dwRet;
}

void DeleteService(char* servicename)
{
	SC_HANDLE scm = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS); 
	if(scm != NULL)
	{
		SC_HANDLE newService = OpenService(scm, servicename, SERVICE_ALL_ACCESS);
		if(newService != NULL)
		{
			SERVICE_STATUS ss;
			ControlService(newService, SERVICE_CONTROL_STOP, &ss);

			DeleteService(newService);
			CloseServiceHandle(newService);
		}

		CloseServiceHandle(scm);
	}
}

BOOL InstallDriver(char* path, char* servicename)
{
	BOOL bSuc = FALSE;

	SC_HANDLE newService;
	SC_HANDLE scm; 
	
	scm = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS); 
	
	newService = CreateService( 
		scm, servicename,
		servicename,
		SERVICE_ALL_ACCESS, 
		SERVICE_KERNEL_DRIVER, 
		SERVICE_DEMAND_START, 
		SERVICE_ERROR_NORMAL, 
		path,
		0, 0, 0, 0, 0); 
	if(newService != NULL)
	{
		bSuc = StartService(newService, 0, NULL);

		CloseServiceHandle(newService); 
	}
	else
	{
		if(ERROR_SERVICE_EXISTS == GetLastError())
		{
			newService = OpenService(scm, servicename, SERVICE_ALL_ACCESS);
			if(newService != NULL)
			{
				bSuc = StartService(newService, 0, NULL);
				
				CloseServiceHandle(newService); 
			}
		}
	}
	
	if(scm != NULL)
	{
		CloseServiceHandle(scm); 
	}

	return bSuc;
} 

DWORD GetSSDTNum(const char* pszAPI)
{
	HMODULE hMod = GetModuleHandle("ntdll.dll");
	BYTE* p = (BYTE*)GetProcAddress(hMod, pszAPI);
	if(p == NULL)
		return 0;

	return *(DWORD*)(p+1);
}


ULONG __stdcall DriverMontorThread(LPVOID pParam)
{
	BYTE outbuff[0x200];
	DWORD dwRet;
	
	while(!bExit)
	{
		memset(outbuff, 0, sizeof(outbuff));
		DeviceIoControl((HANDLE)pParam,
			IOCTL_GETSTR,
			NULL,
			0,
			outbuff,
			sizeof(outbuff), 
			&dwRet,
			NULL);

		if(outbuff[0] != 0)
		{
			printf("%s\n", outbuff);
		}

		Sleep(50);
	}

	return 0;
}

void LoadDriver(DWORD dwPID)
{	
	hDevice = CreateFile("\\\\.\\accessd_at_pediy",
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if(hDevice == INVALID_HANDLE_VALUE)
	{
		//安装驱动
		char szTmpPath[MAX_PATH];
		GetTempPath(MAX_PATH, szTmpPath);
		strcat(szTmpPath, "accessd_at_pediy.sys");
		FILE* fp = fopen(szTmpPath, "wb");
		if(fp != NULL)
		{
			fwrite(gcbSYSData, SYSDATALEN, 1, fp);
			fclose(fp);
		}

		if(!InstallDriver(szTmpPath, "accessd_at_pediy"))
		{
		}

		hDevice = CreateFile("\\\\.\\accessd_at_pediy",
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
	}
	if(hDevice == INVALID_HANDLE_VALUE)
	{
		printf("load driver failed\n");
		return;
	}

	DWORD dwRet;

	INBUF inbuf;
	inbuf.dwDebugerPID = GetCurrentProcessId();
	inbuf.dwPID = dwPID;
//	inbuf.dwNum_NtDelayExecution   = GetSSDTNum("NtDelayExecution");
	inbuf.dwNum_NtGetContextThread = GetSSDTNum("NtGetContextThread");
	inbuf.dwNum_NtSetContextThread = GetSSDTNum("NtSetContextThread");
//	inbuf.dwNum_NtContinue         = GetSSDTNum("NtContinue");

	//参数
	
	DeviceIoControl(hDevice,
		IOCTL_PEDIY,
		&inbuf,
		sizeof(inbuf),
		NULL,
		0, 
		&dwRet,
		NULL);

	DWORD tid;
	g_hDrvMonitorThread = CreateThread(NULL, 0, DriverMontorThread, hDevice, 0, &tid);

//	CloseHandle(hDevice);

}



void RaisePrivilege(BOOL bRaise)
{
	TOKEN_PRIVILEGES tkp;
	HANDLE hToken;
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&hToken))
		return;
	LookupPrivilegeValue(NULL,SE_DEBUG_NAME,&tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = bRaise ? SE_PRIVILEGE_ENABLED : SE_PRIVILEGE_ENABLED_BY_DEFAULT;
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
	
	CloseHandle(hToken);
}

//#define _PRINT_TIME

int main(int argc, char* argv[])
{

	if(argc != 2)
	{
		printf("本工具用于监视目标程序的反调试\n");
		printf("usage: %s exe_path\n", argv[0]);
		return 0;
	}

	RaisePrivilege(TRUE);

	CreateEvent(NULL, FALSE, FALSE, "5ed66a99-5bad-4528-8bc3-f5ed067b2e19");
	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
		printf("already running.\n");
		return 0;
	}

	int i;

	g_Modules = new LoadedModule[MAX_MODULE_COUNT];
	for(i=0; i<MAX_MODULE_COUNT; i++)
	{
		g_Modules[i].lpModuleBase = NULL;
	}
	g_Threads = new THREAD[MAX_THREAD_COUNT];
	for(i=0; i<MAX_THREAD_COUNT; i++)
	{
		g_Threads[i].dwThreadID = 0;
		g_Threads[i].hThread = NULL;
	}


	//调试启动
	PROCESS_INFORMATION pinfo={0};
	STARTUPINFO startInfo = {0};
	startInfo.cb = sizeof(STARTUPINFO);
	startInfo.dwFlags = STARTF_USESHOWWINDOW;
	startInfo.wShowWindow = STARTF_USESHOWWINDOW;
	
	if(!CreateProcess(argv[1],
		NULL, 
		NULL,
		NULL,
		TRUE,
		DEBUG_ONLY_THIS_PROCESS|DEBUG_PROCESS,
		NULL,
		NULL,
		&startInfo,
		&pinfo))
	{
		printf("CreateProcess error\n");
		return 0;
	}

	
	DEBUG_EVENT dbgEvt = {0};
	while((!bExit) && WaitForDebugEvent(&dbgEvt, INFINITE))
	{
		DWORD dwContinue = DBG_CONTINUE;
		
		g_dwCurrTid = dbgEvt.dwThreadId;

		switch(dbgEvt.dwDebugEventCode)
		{
		case CREATE_PROCESS_DEBUG_EVENT:
			{
				g_hProcess = dbgEvt.u.CreateProcessInfo.hProcess;
				g_dwPid = dbgEvt.dwProcessId;
				lpBaseOfImage = (DWORD)dbgEvt.u.CreateProcessInfo.lpBaseOfImage;
				lpEndOfImage = lpBaseOfImage + GetEXEModuleSize(dbgEvt.u.CreateProcessInfo.hFile, argv[1]);
				AddThread(dbgEvt.dwThreadId, dbgEvt.u.CreateProcessInfo.hThread, TRUE);
			//	gdwEOP = GetEntryPointAddr(argv[1], (DWORD)lpBaseOfImage);
				DWORD dwTEB = (DWORD)dbgEvt.u.CreateProcessInfo.lpThreadLocalBase;
				gdwPEB = GetPEBFromTEB(dwTEB);
				
				HideNtGlobalFlag();

				LoadDriver(dbgEvt.dwProcessId);
			}
			break;
		case EXCEPTION_DEBUG_EVENT:
			dwContinue = DbgExp(&dbgEvt, dbgEvt.u.Exception.ExceptionRecord.ExceptionCode, (DWORD)dbgEvt.u.Exception.ExceptionRecord.ExceptionAddress);
			if(dwContinue == 0)
			{
				bExit = TRUE;
			}
			break;
		case EXIT_PROCESS_DEBUG_EVENT:
			bExit = TRUE;
			break;
		case LOAD_DLL_DEBUG_EVENT:
			DbgDllLoad(dbgEvt, pinfo.hProcess);
			break;
		case UNLOAD_DLL_DEBUG_EVENT:
			DbgDllUnLoad(&dbgEvt);
			break; 
		case CREATE_THREAD_DEBUG_EVENT:
			AddThread(dbgEvt.dwThreadId, dbgEvt.u.CreateThread.hThread, FALSE);
			break;
		case EXIT_THREAD_DEBUG_EVENT:
			DbgExitThread(dbgEvt.dwThreadId);
			break;
		default:
			break;
		}
		

		if(!bExit)
		{
			ContinueDebugEvent(dbgEvt.dwProcessId, dbgEvt.dwThreadId, dwContinue);
		}
		
	}

	bExit = TRUE;
	if(g_hDrvMonitorThread != NULL)
	{
		WaitForSingleObject(g_hDrvMonitorThread, INFINITE);
	}
	if(hDevice != NULL)
	{
		CloseHandle(hDevice);
	}

	DeleteService("accessd_at_pediy");
	return 0;
}