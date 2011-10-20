// R3Dbg.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "Disasm\disasm.h"
#include <stdio.h>
typedef enum __BREAK_POINT_TYPE
{
	BREAK_TYPE_SOFT, 
	BREAK_TYPE_MEM,
	BREAK_TYPE_HARD,
	BREAK_TYPE_GO,
	BREAK_TYPE_TEMP
} BREAK_POINT_TYPE;

typedef HANDLE (APIENTRY *OPENTHREAD)(  DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwThreadId);

typedef struct __MY_IMAGE_SECTION_HEADER			//节区信息结构体
{
	int SectionCount;
	IMAGE_SECTION_HEADER *pImageSectionHeader;
	
} MY_IMAGE_SECTION_HEADER;


typedef struct __MY_INT_TO_IAT			//节区信息结构体
{
	DWORD ProcAddr;
	char ProcName[255];

} MY_INT_TO_IAT;

typedef struct __BREAK_POINT
{
	__BREAK_POINT *pNextPoint;
	int nBreakNum;
	DWORD dwBreakAddress;
	DWORD dwBreakAddressEnd;
	BOOL isEffect;
	BREAK_POINT_TYPE BreakType;
	BYTE bSrcCode;
	DWORD dwMemOldProtect[255];
	char szComment[255];
	BYTE Drx;
	DWORD dwMemType;
} BREAK_POINT;


//-------------------------------共享数据区
#pragma data_seg("Shared")


BREAK_POINT *pHeadBreakPoint = NULL;					//断点链表首指针
BREAK_POINT *pTailBreakPoint = NULL;					//断点链表尾指针
BREAK_POINT BreakPointDataZero = {0};					//0值断点结构体
BREAK_POINT BreakPointData[255] = {0};					//断点空间
STARTUPINFO StartupInfo = {0};							//调试程序启动信息
PROCESS_INFORMATION ProcessInfo = {0};					//调试进程信息
DEBUG_EVENT DebugEvent = {0};							//调试信息结构
IMAGE_FILE_HEADER ImageFileHead = {0};					//PE文件头
IMAGE_OPTIONAL_HEADER ImageOptionalHead = {0};			//PE选项头
MY_IMAGE_SECTION_HEADER ImageSectionHeader = {0};		//PE节区信息

int nLodCount = 0;
int nBreakNum = 0;										//断点编号(递增)
char szFilePath[MAX_PATH] = {0};						//调试程序路径
char szFileName[MAX_PATH] = {0};						//调试程序名称
char szErrStr[100] = {0};								//错误字符串
BOOL isTmpSetup = FALSE;								//临时单步
char szOpCode[10] = {0};								//操作指令
BOOL isCodePase = FALSE;								//程序暂停
DWORD dwBreakAddress = 0;								//断点地址
DWORD dwGoAddress = 0;									//Go命令地址
char szComment[255] = {0};								//断点注释字符
int nBreakPointNum = 0;									//要操作的断点编号
BOOL isDr0 = FALSE;										//Dr0是否占用
BOOL isDr1 = FALSE;										//Dr1是否占用
BOOL isDr2 = FALSE;										//Dr2是否占用
int nDrCount = 0;										//Dr计数器
int nBreakLends = 0;									//断点长度
DWORD dwProtect = PAGE_EXECUTE_READWRITE;				//内存保护属性
BOOL isUserCommand = FALSE;								//是否需要用户输入
DWORD dwNextCommandAddr = NULL;							//下一条指令的地址
BOOL isSetup1Show = FALSE;								//设置临时单步是否需要显示
LPVOID pShowMemAddr = (LPVOID)0x00401000;				//要显示的内存地址
char szASM[1024] = {0};									//反汇编字符串
char szRegister[300] = {0};								//寄存器字符串
char szMem[1024] = {0};									//内存信息字符串
char szStack[100] = {0};								//栈信息字符串
char szBreak[1024] = {0};								//断点信息字符串
char szNowAsm[255] = {0};								//当前指令的反汇编字符串
char szOpCommend[255] = {0};							//操作指令
LPVOID CodeAddrss = 0;									//当前代码地址
LPVOID CodeAddrssLost = 0;								//最后一条反汇编指令地址
char szPreProcess[MAX_PATH] = {0};						//主控父进程路径
PTHREAD_START_ROUTINE OpCommendAddress = 0;				//OpCommend地址
int nImportCount = 0;									//导入函数个数
MY_INT_TO_IAT* pMyIATtoINT = NULL;						//IAT_IMT对照表指针
DWORD dwziSetupAddress = 0;								//要回复的断点地址
BREAK_POINT_TYPE ziSetupType = BREAK_TYPE_TEMP;			//要回复的断点类型
t_disasm Tdisasm = {0};									//反汇编结构体
HMODULE hR3DbgDll = 0;									//调试内核DLL加载到宿主中的句柄
BOOL isMemSetupShow = FALSE;							//标记遇到内存访问异常是否需要用户输入


#pragma data_seg() 
#pragma comment(linker, "/SECTION:Shared,RWS") 

//-------------------------------
BOOL CreateDbgProc();


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	char szNum[MAX_PATH] = {0};
	GetModuleFileName(NULL, szNum, MAX_PATH);
	int nLend = strlen(szNum);
	if((nLodCount == 0))		//控制加载程序路径
	{
		strcpy(szFileName, "D:\\Test.exe");
		strcpy(szFilePath, "D:");
		
		while(!strcmp(szFileName,""))
		{
			Sleep(100);
		}

		if(!CreateDbgProc())
		{
			strcpy(szASM, szErrStr);
			isCodePase = TRUE;
			return FALSE;
		}
		nLodCount++;
		pMyIATtoINT = new MY_INT_TO_IAT[2048];
	}
    return TRUE;
}


//打开线程函数
HANDLE OpenThread(DWORD dwDesiredAccess, DWORD bInheritHandle, DWORD dwThreadId)
{
	OPENTHREAD pOpenThread = NULL;
	HMODULE hMoudule =  ::LoadLibrary("Kernel32");
	if(!hMoudule)
	{
		return 0;
	}
	pOpenThread = (OPENTHREAD)GetProcAddress(hMoudule, "OpenThread");
	if(!pOpenThread)
	{
		return 0;
	}
	return pOpenThread(dwDesiredAccess, bInheritHandle, dwThreadId);
}


//PE扫描函数
BOOL PeScan(char *szPath,						
			char *szErrStr,
			IMAGE_FILE_HEADER *pImageFileHead,
			IMAGE_OPTIONAL_HEADER *pImageOptionalHead,
			MY_IMAGE_SECTION_HEADER *pMyImageSectionHeader)
{
	LONG nPEOffset;
	HANDLE hLoadFile;
	char szExeHead[2];
	char szPeStr[4];
	BYTE bExeHead[2] = {0x4D, 0x5A};				//MZ
	BYTE bPeStr[4] = {0x50, 0x45, 0x00, 0x00};		//PE
	DWORD nReadSize = 0;
	BOOL isRead = TRUE;
	
	hLoadFile = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(INVALID_HANDLE_VALUE == hLoadFile)					//打开文件
	{
		strcpy(szErrStr, "读取文件失败!");
		return FALSE;
	}
	
	
	SetFilePointer(hLoadFile, 0, NULL, FILE_BEGIN);			//读取文件头标志
	isRead = ReadFile(hLoadFile, szExeHead, 2, &nReadSize, NULL);
	if(2 != nReadSize && isRead == FALSE)
	{
		strcpy(szErrStr, "读取文件头信息失败!");
		CloseHandle(hLoadFile);
		return FALSE;
	}
	if(0 != memcmp(szExeHead, bExeHead, 2))
	{
		strcpy(szErrStr, "不是有效的可执行程序!");
		CloseHandle(hLoadFile);
		return FALSE;
	}
	
	SetFilePointer(hLoadFile, 0x3C, NULL, FILE_BEGIN);						//读取PE头偏移
	isRead = ReadFile(hLoadFile, (char *)&nPEOffset, 4, &nReadSize, NULL);
	if(4 != nReadSize && isRead == FALSE)
	{
		strcpy(szErrStr, "读取PE头位置失败!");
		CloseHandle(hLoadFile);
		return FALSE;
	}

	SetFilePointer(hLoadFile, nPEOffset, NULL, FILE_BEGIN);
	isRead = ReadFile(hLoadFile, (char *)&szPeStr, 4, &nReadSize, NULL);
	if(4 != nReadSize && isRead ==FALSE)
	{
		strcpy(szErrStr, "读取PE标志失败!");
		CloseHandle(hLoadFile);
		return FALSE;
	}
	
	if(0 != memcmp(szPeStr, bPeStr, 4))
	{
		strcpy(szErrStr, "不是有效的PE格式!");
		CloseHandle(hLoadFile);
		return FALSE;
	}

	isRead = ReadFile(hLoadFile, pImageFileHead, sizeof(IMAGE_FILE_HEADER), &nReadSize, NULL);
	if(sizeof(IMAGE_FILE_HEADER) != nReadSize && isRead == FALSE)	//读取FILE头信息
	{
		strcpy(szErrStr, "读取File头数据错误!");
		CloseHandle(hLoadFile);
		return FALSE;
	}

	isRead = ReadFile(hLoadFile, pImageOptionalHead, pImageFileHead->SizeOfOptionalHeader, &nReadSize, NULL);
	if(pImageFileHead->SizeOfOptionalHeader != nReadSize && isRead == FALSE)
	{
		strcpy(szErrStr, "读取选项区数据错误!");
		CloseHandle(hLoadFile);
		return FALSE;
	}
	
	
	pMyImageSectionHeader->SectionCount = pImageFileHead->NumberOfSections;
	pMyImageSectionHeader->pImageSectionHeader = new IMAGE_SECTION_HEADER[pMyImageSectionHeader->SectionCount];
	UINT SectionSize = sizeof(IMAGE_SECTION_HEADER) * pMyImageSectionHeader->SectionCount;

	isRead = ReadFile(hLoadFile, pMyImageSectionHeader->pImageSectionHeader, SectionSize, &nReadSize, NULL);
	if(SectionSize != nReadSize && isRead == FALSE)
	{
		strcpy(szErrStr, "读取节信息数据错误!");
		CloseHandle(hLoadFile);
		return FALSE;
	}
	CloseHandle(hLoadFile);																		//关闭文件
	return TRUE;
}


//安全的读取内存函数
BOOL ReadProcessMemorySafe(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead)
{
	DWORD dwOldProtect;										//修改属性为可读可写
	if(!VirtualProtectEx(hProcess,
		(LPVOID)lpBaseAddress,
		nSize,
		PAGE_EXECUTE_READWRITE,
		&dwOldProtect))
	{
		strcpy(szErrStr, "读取内存时设置内存属性错误!");
		return FALSE;
	}
	
	ReadProcessMemory(hProcess,					//读取内存数据
		lpBaseAddress,
		lpBuffer, nSize, lpNumberOfBytesRead);
	
	if(!VirtualProtectEx(hProcess,				//还原内存原属性
		(LPVOID)lpBaseAddress,
		nSize,
		dwOldProtect,
		&dwOldProtect))
	{
		strcpy(szErrStr, "读取内存时还原内存属性错误!");
		return FALSE;
	}
	return TRUE;
}


//安全的写入内存函数
BOOL WriteProcessMemorySafe(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead)
{
	DWORD dwOldProtect;										//修改属性为可读可写
	if(!VirtualProtectEx(hProcess,
						(LPVOID)lpBaseAddress,
						nSize,
						PAGE_EXECUTE_READWRITE,
						&dwOldProtect))
	{
		strcpy(szErrStr, "读取内存时设置内存属性错误!");
		return FALSE;
	}
	
	WriteProcessMemory(hProcess,					//写入内存数据
					  (LPVOID)lpBaseAddress,
					  lpBuffer, nSize, lpNumberOfBytesRead);
	
	if(!VirtualProtectEx(hProcess,					//还原内存原属性
						(LPVOID)lpBaseAddress,
						nSize,
						dwOldProtect,
						&dwOldProtect))
	{
		strcpy(szErrStr, "读取内存时还原内存属性错误!");
		return FALSE;
	}
	return TRUE;
}


//重置所有断点信息
BOOL SetBreakPointAll()					
{
	BREAK_POINT *pTmp = NULL;
	pTmp = pHeadBreakPoint;
	while (pTmp)
	{
		if(pTmp->BreakType == BREAK_TYPE_SOFT && pTmp->isEffect == TRUE)	//软件断点
		{
			BYTE bBreakPoint = 0xCC;
			if(!WriteProcessMemorySafe(ProcessInfo.hProcess,//还原代码
										(LPVOID)pTmp->dwBreakAddress,
										&bBreakPoint, 1, NULL))
			{
				strcpy(szErrStr, "写入软断点失败!");
				return FALSE;
			}
		}

		if(pTmp->BreakType == BREAK_TYPE_HARD && pTmp->isEffect == TRUE)	//硬件断点
		{
			HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, DebugEvent.dwThreadId);
			SuspendThread(hThread);
			CONTEXT ConText;
			ConText.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
			GetThreadContext(hThread, &ConText);
			switch(pTmp->Drx)
			{
			case 0:
				{
					ConText.Dr0 = pTmp->dwBreakAddress;
					ConText.Dr7 = ConText.Dr7 | 0x101;
					switch(pTmp->dwMemType)						//设置断点类型
					{
					case 0:				//访问
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFFCFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x30000;
						}
						break;
					case 1:				//写入
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFFCFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x10000;
						}
						break;
					case 2:				//执行
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFFCFFFF;
						}
						break;
					}

					switch(pTmp->dwBreakAddressEnd - pTmp->dwBreakAddress +1)	//设置断点长度
					{
					case 1:				//长度1
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFF3FFFF;
						}
						break;
					case 2:				//长度2
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFF3FFFF;
							ConText.Dr7 = ConText.Dr7 | 0x40000;
						}
						break;
					case 4:				//长度4
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFF3FFFF;
							ConText.Dr7 = ConText.Dr7 | 0xC0000;
						}
						break;     //01 00 00 0 001 01 00 00 00 01
					}
					
				}
			break;

			case 1:
				{
					ConText.Dr1 = pTmp->dwBreakAddress;
					ConText.Dr7 = ConText.Dr7 | 0x104;
					switch(pTmp->dwMemType)						

	//设置断点类型
					{
					case 0:				//访问
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFCFFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x300000;
						}
						break;
					case 1:				//写入
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFCFFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x100000;
						}
						break;
					case 4:				//执行
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFCFFFFF;
						}
						break;
					}
					
					switch(pTmp->dwBreakAddressEnd - pTmp->dwBreakAddress +1)	//设置断点长度
					{
					case 1:				//长度1
						{
							ConText.Dr7 = ConText.Dr7 & 0xFF3FFFFF;
						}
						break;
					case 2:				//长度2
						{
							ConText.Dr7 = ConText.Dr7 & 0xFF3FFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x400000;
						}
						break;
					case 4:				//长度4
						{
							ConText.Dr7 = ConText.Dr7 & 0xFF3FFFFF;
							ConText.Dr7 = ConText.Dr7 | 0xC00000;
						}
						break;
					}
				}
			break;

			case 2:
				{
					ConText.Dr2 = pTmp->dwBreakAddress;
					ConText.Dr7 = ConText.Dr7 | 0x110;
					switch(pTmp->dwMemType)						//设置断点类型
					{
					case 0:				//访问
						{
							ConText.Dr7 = ConText.Dr7 & 0xFCFFFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x3000000;
						}
						break;
					case 1:				//写入
						{
							ConText.Dr7 = ConText.Dr7 & 0xFCFFFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x1000000;
						}
						break;
					case 2:				//执行
						{
							ConText.Dr7 = ConText.Dr7 & 0xFCFFFFFF;
						}
						break;
					}
					
					switch(pTmp->dwBreakAddressEnd - pTmp->dwBreakAddress +1)	//设置断点长度
					{
					case 1:				//长度1
						{
							ConText.Dr7 = ConText.Dr7 & 0xF3FFFFFF;
						}
						break;
					case 2:				//长度2
						{
							ConText.Dr7 = ConText.Dr7 & 0xF3FFFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x4000000;
						}
						break;
					case 4:				//长度4
						{
							ConText.Dr7 = ConText.Dr7 & 0xF3FFFFFF;
							ConText.Dr7 = ConText.Dr7 | 0xC000000;
						}
						break;
					}
				}
			break;
			}
			SetThreadContext(hThread, &ConText);
			ResumeThread(hThread);
			CloseHandle(hThread);
		}

		if(pTmp->BreakType == BREAK_TYPE_GO && pTmp->isEffect == TRUE)
		{
			HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, DebugEvent.dwThreadId);
			SuspendThread(hThread);
			CONTEXT ConText;
			ConText.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
			GetThreadContext(hThread, &ConText);
			ConText.Dr3 = pTmp->dwBreakAddress;
			ConText.Dr7 = ConText.Dr7 | 0x140;
			SetThreadContext(hThread, &ConText);
			ResumeThread(hThread);
			CloseHandle(hThread);

		}

		if(pTmp->BreakType == BREAK_TYPE_MEM && pTmp->isEffect == TRUE)				

		//内存断点
		{
			for (DWORD i = pTmp->dwBreakAddress; i<= pTmp->dwBreakAddressEnd; i++)
			{
				DWORD dwOldProtect;
				if(!VirtualProtectEx(ProcessInfo.hProcess,
									(LPVOID)i,
									1,
									PAGE_NOACCESS,
									&dwOldProtect))
				{
					strcpy(szErrStr, "设置断点表示的内存属性错误");
					return FALSE;
				}
			}
		}
		pTmp = pTmp->pNextPoint;
	}
	return 1;
}


//构建IAT-IMT对照表
void CreateIATtoIMT()
{
	IMAGE_IMPORT_DESCRIPTOR *Idd = new IMAGE_IMPORT_DESCRIPTOR[100];
	IMAGE_IMPORT_DESCRIPTOR IddZero = {0};
	int nIddCount = 0;
	DWORD IddStar = ImageOptionalHead.DataDirectory[1].VirtualAddress + ImageOptionalHead.ImageBase;
	do 
	{
		DWORD pianyi = sizeof(IMAGE_IMPORT_DESCRIPTOR) *nIddCount;

		ReadProcessMemorySafe(ProcessInfo.hProcess,
						  (LPVOID)(IddStar + pianyi),
						  &(Idd[nIddCount]),
						  sizeof(IMAGE_IMPORT_DESCRIPTOR),
						  NULL);
		nIddCount++;

	} while (memcmp(&(Idd[nIddCount-1]), &IddZero, sizeof(IMAGE_IMPORT_DESCRIPTOR)));
	nIddCount-=2;


	int nIatToIntCount = 0;
	for (int s = 0; s<nIddCount; s++)
	{
		int nXiangPianyi = 0;
		char szMoudName[255] = {0};
		
		int u = 0;											//获取模块名称
		do 
		{
			ReadProcessMemorySafe(ProcessInfo.hProcess,
				(LPVOID)(Idd[s].Name + ImageOptionalHead.ImageBase + u),
				szMoudName+u,
				1,NULL
				);
			u++;
			
		} while (szMoudName[u-1] != '\0');
		szMoudName[strlen(szMoudName)-4] = '\0';
		strcat(szMoudName,".");

		do 
		{
			DWORD ImageThunkData = 0;
			DWORD pianyi = (4 * nXiangPianyi) + ImageOptionalHead.ImageBase;
			ReadProcessMemorySafe(ProcessInfo.hProcess,								//读取指向的OriginalFirstThunk结构体
							  (LPVOID)(Idd[s].OriginalFirstThunk + pianyi),
							  &ImageThunkData,
							  4, NULL);
			if(ImageThunkData == 0)
			{
				break;
			}
			if((ImageThunkData & 0x80000000) == 0)								//判断如果是函数名导出
			{
				int q = 0;
				do 
				{
					ReadProcessMemorySafe(ProcessInfo.hProcess,
									  (LPVOID)(ImageThunkData +2+q + ImageOptionalHead.ImageBase),
								      &(pMyIATtoINT[nIatToIntCount].ProcName[q]),
									  1,NULL
									  );
					q++;
					
				} while (pMyIATtoINT[nIatToIntCount].ProcName[q-1] != '\0');

				char szTmp[255] = {0};
				strcpy(szTmp, szMoudName);
 				strcat(szMoudName ,pMyIATtoINT[nIatToIntCount].ProcName);
 				strcpy(pMyIATtoINT[nIatToIntCount].ProcName, szMoudName);
				strcpy( szMoudName, szTmp);
			}
			else
			{
				sprintf(pMyIATtoINT[nIatToIntCount].ProcName, "%d", (ImageThunkData & 0x7FFFFFFF));
			}
			
// 			ReadProcessMemory(ProcessInfo.hProcess,
// 							  (LPVOID)(Idd[s].FirstThunk + pianyi),				//读取指向的FirstThunk函数地址
// 							  &(pMyIATtoINT[nIatToIntCount].ProcAddr),
// 							  4, NULL);
			pMyIATtoINT[nIatToIntCount].ProcAddr = Idd[s].FirstThunk + pianyi;

			nXiangPianyi++;
			nIatToIntCount++;
		} while (TRUE);
		int y = s;
	}
	delete []Idd;
	nImportCount = nIatToIntCount;
}


//获取函数名称
char *GetProcName(DWORD ProcAddress)
{
	for (int i = 0; i < nImportCount; i++)
	{
		if(pMyIATtoINT[i].ProcAddr == ProcAddress)
		{
			return pMyIATtoINT[i].ProcName;
		}
	}
	return "";
}


//分配断点空间
BREAK_POINT *NewBk()
{
	for (int i =0; i<255; i++)
	{
		if(!memcmp(&(BreakPointData[i]), &BreakPointDataZero, sizeof(BREAK_POINT)))
		{
			return &(BreakPointData[i]);
		}
	}
	return NULL;
}


//释放断点空间
BOOL DelBk(BREAK_POINT *pBreakPint)
{
	for (int i =0; i<255; i++)
	{
		if(&(BreakPointData[i]) == pBreakPint)
		{
			memset(&(BreakPointData[i]), 0, sizeof(BREAK_POINT));
			return TRUE;
		}
	}
	return FALSE;
}


//获取Op函数地址--导出
PTHREAD_START_ROUTINE GetOpCommend()
{
	return OpCommendAddress;
}


//获取错误信息函数--导出
char * GetErr()										
{
	return szErrStr;
}


//获取R3Dbg的模块句柄
HMODULE GetR3DbgHandle()
{
	return hR3DbgDll;
}


//获取调试进程句柄
HANDLE GetR3ProcHandle()
{
	return 	ProcessInfo.hProcess;
}


//设置被调试程序路径函数--导出
void SetDbgNamePath(char *szDbgName, char *szDbgPath)
{
	strcpy(szFileName, szDbgName);
	strcpy(szFilePath, szDbgPath);
}


//设置要增加断点信息函数--导出
void SetBreakPointInfo(DWORD BreakAddress, char *Comment)
{
	dwBreakAddress = BreakAddress;
	strcpy(szComment, Comment);
}


//设置要增加的内存断点附加信息函数--导出
BOOL SetMemBreakInfo(int BreakLends, char* Protect)
{
	nBreakLends = BreakLends;
	
	if(!strcmp(Protect, "read"))
	{
		dwProtect = 0;
	}
	else if(!strcmp(Protect, "write"))
	{
		dwProtect = 1;
	}
	else if(!strcmp(Protect, "exec"))
	{
		dwProtect = 2;
	}
	else
	{
		strcpy(szErrStr, "设置断点类型错误!");
		return FALSE;
	}
	return TRUE;
}


//设置要操作的断点编号函数--导出
void SetBreakNum(int BreakNum)
{
	nBreakPointNum = BreakNum;
}


//设置操作指令函数--导出
void SetOpCode(char *OpCode)
{
	strcpy(szOpCommend, OpCode);
}


//获取反汇编指令函数--导出
char *GetAsmStr()
{
	return szASM;
}


//设置程序运行状态--导出
void SetIsPase(BOOL isPase)
{
	isCodePase = isPase;
}


//获取程序运行状态--导出
BOOL GetIsPase()
{
	return isCodePase;
}


//获取寄存器信息字符串--导出
char *GetRegisterStr()
{
	return szRegister;
}


//获取内存信息字符串--导出
char *GetMemStr()
{
	return szMem;
}


//获取栈信息字符串--导出
char *GetStackStr()
{
	return szStack;
}


//获取断点信息字符串--导出
char *GetBreakStr()
{
	return szBreak;
}


//设置主控进程路径
void SetPrProcess(char *Proprocess)
{
	strcpy(szPreProcess, Proprocess);
}


//增加断点链表节点函数
void ListAdd(BREAK_POINT *pNewPoint)
{
	if(pHeadBreakPoint)								//加入断点链表
	{
		pTailBreakPoint->pNextPoint = pNewPoint;
		pTailBreakPoint = pNewPoint;
	}
	else
	{
		pHeadBreakPoint = pNewPoint;
		pTailBreakPoint = pNewPoint;
	}
}


//删除断点链表节点函数
void ListDel(int nBreakNum)								//删除断点链表节点
{
	BREAK_POINT *pTmp = pHeadBreakPoint;
	BREAK_POINT *pLast = NULL;
	while(pTmp)										//查找该序号的节点地址
	{
		if(pTmp->nBreakNum == nBreakNum)
		{
			break;
		}
		pLast = pTmp;
		pTmp = pTmp->pNextPoint;
	}

	if(!pTmp)
	{
		return;
	}

	if(pTmp == pHeadBreakPoint)						//要删除的为头节点
	{
		BREAK_POINT *pOldHead = pHeadBreakPoint;
		pHeadBreakPoint = pHeadBreakPoint->pNextPoint;
		if(pOldHead)
		{
			DelBk(pOldHead);
			pOldHead = NULL;
		}
		return;
	}
	
	if(pTmp == pTailBreakPoint)						//要删除的为尾节点
	{
		BREAK_POINT *pOldTail = pTailBreakPoint;
		pTailBreakPoint = pLast;
		pTailBreakPoint->pNextPoint = NULL;
		if (pOldTail)
		{
			DelBk(pOldTail);
			pOldTail = NULL;
		}
		return;
	}

	pLast->pNextPoint = pTmp->pNextPoint;			//要删除的是中间节点
	if(pTmp)
	{
		DelBk(pTmp);
		pTmp = NULL;
	}
	return ;
}


//重置断点信息
BOOL SetBreakPoint(DWORD BkAddress, BREAK_POINT_TYPE BkTypt)											
{

	if(BkAddress == 0 && BkTypt == BREAK_TYPE_TEMP)
	{
		return SetBreakPointAll();
	}
	if(BkTypt == BREAK_TYPE_SOFT)
	{
		BREAK_POINT *pTmp = NULL;
		pTmp = pHeadBreakPoint;
		while (pTmp)
		{
			if(pTmp->BreakType == BREAK_TYPE_SOFT && pTmp->isEffect == TRUE && pTmp->dwBreakAddress == BkAddress)	//软件断点
			{
				BYTE bBreakPoint = 0xCC;
				if(!WriteProcessMemorySafe(ProcessInfo.hProcess,				//还原代码
										  (LPVOID)pTmp->dwBreakAddress,
										  &bBreakPoint, 1, NULL))
				{
					strcpy(szErrStr, "写入软断点失败!");
					return FALSE;
				}
			}
			pTmp = pTmp->pNextPoint;
		}
	}

	if(BkTypt == BREAK_TYPE_HARD)							//硬件断点
	{
		BREAK_POINT *pTmp = NULL;
		pTmp = pHeadBreakPoint;
		while (pTmp)
		{
			if(pTmp->BreakType == BREAK_TYPE_HARD && pTmp->isEffect == TRUE && pTmp->dwBreakAddress == BkAddress)
			{
				HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, DebugEvent.dwThreadId);
				SuspendThread(hThread);
				CONTEXT ConText;
				ConText.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
				GetThreadContext(hThread, &ConText);
				switch(pTmp->Drx)
				{
				case 0:
					{
						ConText.Dr0 = pTmp->dwBreakAddress;
						ConText.Dr7 = ConText.Dr7 | 0x101;
						switch(pTmp->dwMemType)							//设置断点类型
						{
						case 0:				//访问
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFFCFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x30000;
							}
							break;
						case 1:				//写入
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFFCFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x10000;
							}
							break;
						case 2:				//执行
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFFCFFFF;
							}
							break;
						}
						
						switch(pTmp->dwBreakAddressEnd - pTmp->dwBreakAddress +1)			//设置断点长度
						{
						case 1:				//长度1
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFF3FFFF;
							}
							break;
						case 2:				//长度2
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFF3FFFF;
								ConText.Dr7 = ConText.Dr7 | 0x40000;
							}
							break;
						case 4:				//长度4
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFF3FFFF;
								ConText.Dr7 = ConText.Dr7 | 0xC0000;
							}
							break;     //01 00 00 0 001 01 00 00 00 01
						}
						
					}
					break;
					
				case 1:
					{
						ConText.Dr1 = pTmp->dwBreakAddress;
						ConText.Dr7 = ConText.Dr7 | 0x104;
						switch(pTmp->dwMemType)							//设置断点类型
						{
						case 0:				//访问
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFCFFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x300000;
							}
							break;
						case 1:				//写入
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFCFFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x100000;
							}
							break;
						case 4:				//执行
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFCFFFFF;
							}
							break;
						}
						
						switch(pTmp->dwBreakAddressEnd - pTmp->dwBreakAddress +1)			//设置断点长度
						{
						case 1:				//长度1
							{
								ConText.Dr7 = ConText.Dr7 & 0xFF3FFFFF;
							}
							break;
						case 2:				//长度2
							{
								ConText.Dr7 = ConText.Dr7 & 0xFF3FFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x400000;
							}
							break;
						case 4:				//长度4
							{
								ConText.Dr7 = ConText.Dr7 & 0xFF3FFFFF;
								ConText.Dr7 = ConText.Dr7 | 0xC00000;
							}
							break;
						}
					}
					break;
					
				case 2:
					{
						ConText.Dr2 = pTmp->dwBreakAddress;
						ConText.Dr7 = ConText.Dr7 | 0x110;
						switch(pTmp->dwMemType)							//设置断点类型
						{
						case 0:				//访问
							{
								ConText.Dr7 = ConText.Dr7 & 0xFCFFFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x3000000;
							}
							break;
						case 1:				//写入
							{
								ConText.Dr7 = ConText.Dr7 & 0xFCFFFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x1000000;
							}
							break;
						case 2:				//执行
							{
								ConText.Dr7 = ConText.Dr7 & 0xFCFFFFFF;
							}
							break;
						}
						
						switch(pTmp->dwBreakAddressEnd - pTmp->dwBreakAddress +1)			//设置断点长度
						{
						case 1:				//长度1
							{
								ConText.Dr7 = ConText.Dr7 & 0xF3FFFFFF;
							}
							break;
						case 2:				//长度2
							{
								ConText.Dr7 = ConText.Dr7 & 0xF3FFFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x4000000;
							}
							break;
						case 4:				//长度4
							{
								ConText.Dr7 = ConText.Dr7 & 0xF3FFFFFF;
								ConText.Dr7 = ConText.Dr7 | 0xC000000;
							}
							break;
						}
					}
					break;
				}
				SetThreadContext(hThread, &ConText);
				ResumeThread(hThread);
				CloseHandle(hThread);
			}
			pTmp = pTmp->pNextPoint;
		}
	}

	if(BkTypt == BREAK_TYPE_GO)
	{
		BREAK_POINT *pTmp = NULL;
		pTmp = pHeadBreakPoint;
		while (pTmp)
		{
			if(pTmp->BreakType == BREAK_TYPE_GO && pTmp->isEffect == TRUE && pTmp->dwBreakAddress == BkAddress)
			{
				HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, DebugEvent.dwThreadId);
				SuspendThread(hThread);
				CONTEXT ConText;
				ConText.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
				GetThreadContext(hThread, &ConText);
				ConText.Dr3 = pTmp->dwBreakAddress;
				ConText.Dr7 = ConText.Dr7 | 0x140;
				SetThreadContext(hThread, &ConText);
				ResumeThread(hThread);
				CloseHandle(hThread);
			}
			pTmp = pTmp->pNextPoint;
		}
	}

	if(BkTypt == BREAK_TYPE_MEM)
	{
		BREAK_POINT *pTmp = NULL;
		pTmp = pHeadBreakPoint;
		while (pTmp)
		{
			if(pTmp->BreakType == BREAK_TYPE_MEM && pTmp->isEffect == TRUE)						//内存断点
			{
				for (DWORD i = pTmp->dwBreakAddress; i<= pTmp->dwBreakAddressEnd; i++)
				{
					DWORD dwOldProtect;
					if(!VirtualProtectEx(ProcessInfo.hProcess,
						(LPVOID)i,
						1,
						PAGE_NOACCESS,
						&dwOldProtect))
					{
						strcpy(szErrStr, "设置断点表示的内存属性错误");
						return FALSE;
					}
				}
			}
			pTmp = pTmp->pNextPoint;
		}
	}

	return 1;
}


//增加软断点函数--导出
BOOL AddSoftBreak()
{
	BREAK_POINT *pTmp = NULL;
	pTmp = pHeadBreakPoint;
	while (pTmp)									//寻找该类型断点是否存在
	{
		if(pTmp->dwBreakAddress == dwBreakAddress && pTmp->BreakType == BREAK_TYPE_SOFT)
		{
			pTmp->isEffect = TRUE;
			strcpy(pTmp->szComment, szComment);
			return 1;
		}
		pTmp = pTmp->pNextPoint;
	}

	BREAK_POINT *pNewPoint = NewBk();		//不存在该断点,建立新断点
	pNewPoint->BreakType = BREAK_TYPE_SOFT;
	pNewPoint->dwBreakAddress = dwBreakAddress;
	pNewPoint->isEffect = TRUE;
	pNewPoint->pNextPoint = NULL;
	pNewPoint->nBreakNum = ++nBreakNum;
	pNewPoint->dwMemType = 4;
	strcpy(pNewPoint->szComment, szComment);
	if(!ReadProcessMemorySafe(ProcessInfo.hProcess,
					  (LPCVOID)dwBreakAddress,
					  &(pNewPoint->bSrcCode),
					  1, NULL))
	{
		strcpy(szErrStr, "读取断点源代码失败!");
		return 0;
	}
	
	ListAdd(pNewPoint);								//加入链表
	SetBreakPoint(pNewPoint->dwBreakAddress, BREAK_TYPE_SOFT);
	dwBreakAddress = 0;
	strcpy(szComment, "");
	return 1;
}


//增加硬件断点函数--导出
BOOL AddHeadBreak()
{
	BREAK_POINT *pTmp = NULL;
	pTmp = pHeadBreakPoint;
	while (pTmp)									//寻找该硬件断点是否存在
	{
		if(pTmp->dwBreakAddress == dwBreakAddress && pTmp->BreakType == BREAK_TYPE_HARD)
		{
			pTmp->isEffect = TRUE;
			strcpy(pTmp->szComment, szComment);
			return 1;
		}
		pTmp = pTmp->pNextPoint;
	}
	if(nDrCount >= 3)
	{
		strcpy(szErrStr, "硬件断点已满,无法增加!");
		return FALSE;
	}
	BREAK_POINT *pNewPoint = NewBk();		//不存在该断点,建立新断点
  	pNewPoint->BreakType = BREAK_TYPE_HARD;
	pNewPoint->dwBreakAddress = dwBreakAddress;
	pNewPoint->dwBreakAddressEnd = pNewPoint->dwBreakAddress + nBreakLends -1;
	pNewPoint->isEffect = TRUE;
	pNewPoint->pNextPoint = NULL;
	pNewPoint->nBreakNum = ++nBreakNum;
	pNewPoint->bSrcCode = 0x90;
	pNewPoint->dwMemType = dwProtect;
	strcpy(pNewPoint->szComment, szComment);
	if(!isDr0)								//寻找没有使用的硬件断点
	{
		pNewPoint->Drx = 0;
		isDr0 = TRUE;
		nDrCount++;
	}
	else if(!isDr1)
	{
		pNewPoint->Drx = 1;
		isDr1 = TRUE;
		nDrCount++;
	}
	else if(!isDr2)
	{
		pNewPoint->Drx = 2;
		isDr2 = TRUE;
		nDrCount++;
	}
	ListAdd(pNewPoint);								//加入链表
	SetBreakPoint(pNewPoint->dwBreakAddress, BREAK_TYPE_HARD);

	dwBreakAddress = 0;
	strcpy(szComment, "");
	return 1;
}


//增加内存断点函数--导出
BOOL AddMemBreak()
{
	BREAK_POINT *pTmp = NULL;
	pTmp = pHeadBreakPoint;
	while (pTmp)									//寻找该内存断点是否在已有的断点范围内
	{
		if(dwBreakAddress >= pTmp->dwBreakAddress &&
		   (dwBreakAddress + (DWORD)nBreakLends -1) <= pTmp->dwBreakAddressEnd &&
			pTmp->BreakType == BREAK_TYPE_MEM &&
			pTmp->dwMemType == dwProtect
		  )
		{
			pTmp->isEffect = TRUE;
			strcpy(pTmp->szComment, szComment);
			strcpy(szErrStr, "断点范围已存在!");
			isCodePase = TRUE;
			return 0;
		}
		pTmp = pTmp->pNextPoint;
	}

	BREAK_POINT *pNewPoint = NewBk();		//不存在该断点,建立新断点
	pNewPoint->BreakType = BREAK_TYPE_MEM;
	pNewPoint->dwBreakAddress = dwBreakAddress;
	pNewPoint->dwBreakAddressEnd = pNewPoint->dwBreakAddress + nBreakLends -1;
	pNewPoint->isEffect = TRUE;
	pNewPoint->pNextPoint = NULL;
	pNewPoint->nBreakNum = ++nBreakNum;
	pNewPoint->bSrcCode = 0x90;
	pNewPoint->dwMemType = dwProtect;
													//设置断点的原内存属性
	MEMORY_BASIC_INFORMATION MemInfoMation;
	for (DWORD i = dwBreakAddress, s = 0;
		i<= pNewPoint->dwBreakAddressEnd;
		i++,s++)
	{
		BREAK_POINT *pTmp = NULL;
		pTmp = pHeadBreakPoint;
		BOOL isFind = FALSE;
		while (pTmp)											//判断该内存断点地址的内存原属性是否被修改
		{
			if(pTmp->BreakType == BREAK_TYPE_MEM)
			{
				for (DWORD j=pTmp->dwBreakAddress, t = 0;
					j <= pTmp->dwBreakAddressEnd;
					j++, t++) //遍历已存在的断点中包含的地址
				{
					if((j & 0xFFFFF000) == (i & 0xFFFFF000))		//如果处在同一分页内
					{
						(pNewPoint->dwMemOldProtect)[s] = (pTmp->dwMemOldProtect)[t];
						isFind = TRUE;
						break;
					}
				}
			}
			pTmp = pTmp->pNextPoint;
		}
		
		if(!isFind)												//没有找到已被修改的内存分页
		{
			VirtualQueryEx(ProcessInfo.hProcess,					//查询该断点位置的原内存属性 
							(LPVOID)i,
							&MemInfoMation,
							sizeof(MEMORY_BASIC_INFORMATION)
							);
			(pNewPoint->dwMemOldProtect)[s] = MemInfoMation.Protect;
		}
	}

	strcpy(pNewPoint->szComment, szComment);
	ListAdd(pNewPoint);								//加入链表
	
	dwBreakAddress = 0;
	strcpy(szComment, "");
	nBreakLends = 0;
	dwProtect = PAGE_EXECUTE_READWRITE;
	
	SetBreakPoint(pNewPoint->dwBreakAddress, BREAK_TYPE_MEM);
	return 1;
}


//设置下次代码单步执行函数
BOOL SetNextSetup(DWORD ThreadId)
{
	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, ThreadId);	
	SuspendThread(hThread);
	CONTEXT ConText;											//设置下次单步
	ConText.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
	if(!GetThreadContext(hThread, &ConText))
	{
		strcpy(szErrStr, "读取CONTEXT错误");
		ResumeThread(hThread);
		return FALSE;
	}
	ConText.EFlags = ConText.EFlags | 0x100;
	
	if(!SetThreadContext(hThread, &ConText))
	{
		strcpy(szErrStr, "设置CONTEXT错误");
		ResumeThread(hThread);
		return FALSE;
	}
	ResumeThread(hThread);
	CloseHandle(hThread);
	return TRUE;
}


//执行用户操作指令
BOOL RunUserOrder()
{
	if(isUserCommand)
	{
		isCodePase = TRUE;
		strcpy(szOpCode, "");
		while (!strcmp(szOpCode,""))							//等待操作指令
		{
			Sleep(1);
		}
		Sleep(300);

		if((!strcmp(szOpCode, "p") || !strcmp(szOpCode, "P")) && memcmp(szNowAsm,"CALL", 4))
		{
			strcpy(szOpCode,"t");
		}

		if(!strcmp(szOpCode, "t") || !strcmp(szOpCode, "T"))		//如果命令单步步入
		{
			SetNextSetup(DebugEvent.dwThreadId);						//设置下次单步
			isTmpSetup = 2;												//标记为用户单步
			isSetup1Show = TRUE;
			isMemSetupShow = TRUE;
		}
		
		if(!strcmp(szOpCode, "p") || !strcmp(szOpCode, "P"))		//如果命令单步步过
		{
			BREAK_POINT *pNewPoint = NewBk();					//新硬件断点
			pNewPoint->BreakType = BREAK_TYPE_GO;
			pNewPoint->dwBreakAddress = dwNextCommandAddr;				//下条指令地址
			pNewPoint->isEffect = TRUE;
			pNewPoint->pNextPoint = NULL;
			pNewPoint->nBreakNum = ++nBreakNum;
			pNewPoint->bSrcCode = 0x90;
			pNewPoint->Drx = 3;
			strcpy(pNewPoint->szComment, "单步步过到达");
			ListAdd(pNewPoint);										//加入链表
			dwBreakAddress = 0;
			strcpy(szComment, "");
			isSetup1Show = FALSE;									//标记回复断点时不显示
			isMemSetupShow = FALSE;
			isTmpSetup = 1;
			SetNextSetup(DebugEvent.dwThreadId);
			
		}


		if(!strcmp(szOpCode, "q") || !strcmp(szOpCode, "Q"))		//如果是退出命令
		{
			HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, DebugEvent.dwThreadId);	
			SuspendThread(hThread);								//挂起线程,等待模块卸载
		}


		if((!strcmp(szOpCode, "g") || !strcmp(szOpCode, "G")) && dwGoAddress)		//如果命令单步步过GO
		{
			BREAK_POINT *pNewPoint = NewBk();					//新硬件断点
			pNewPoint->BreakType = BREAK_TYPE_GO;
			pNewPoint->dwBreakAddress = dwGoAddress;					//要GO的地址
			pNewPoint->isEffect = TRUE;
			pNewPoint->pNextPoint = NULL;
			pNewPoint->nBreakNum = ++nBreakNum;
			pNewPoint->bSrcCode = 0x90;
			pNewPoint->Drx = 3;
			strcpy(pNewPoint->szComment, "单步步过到达");
			ListAdd(pNewPoint);										//加入链表
			dwBreakAddress = 0;
			strcpy(szComment, "");
			isSetup1Show = FALSE;									//标记回复断点时不显示
			isMemSetupShow = FALSE;
			isTmpSetup = 1;
			dwGoAddress = 0;
			SetNextSetup(DebugEvent.dwThreadId);
			
		}
		dwNextCommandAddr = 0;
		strcpy(szOpCode, ""); 
	}
	return 1;
}


//删除断点函数--导出
BOOL DelBreakPoint()
{
	BREAK_POINT *pTmp = NULL;
	BREAK_POINT *pLast = NULL;
	pTmp = pHeadBreakPoint;
	while (pTmp)
	{
		if(pTmp->nBreakNum == nBreakPointNum)
		{
			break;
		}
		pTmp = pTmp->pNextPoint;
	}

	if(!pTmp)
	{
		strcpy(szErrStr, "没有找到该断点!");
		return FALSE;
	}

	switch(pTmp->BreakType)
	{
		case BREAK_TYPE_SOFT:												//删除软断点
		{
			if(!::WriteProcessMemorySafe(ProcessInfo.hProcess,				//还原代码
										(LPVOID)pTmp->dwBreakAddress,
										&(pTmp->bSrcCode), 1, NULL))
			{
				strcpy(szErrStr, "还原断点代码错误!");
				return 0;
			}

			ListDel(nBreakPointNum);									//删除断点链表节点
			nBreakPointNum = 0;
		}
		break;

		case BREAK_TYPE_HARD:
		{
			HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, DebugEvent.dwThreadId);
			SuspendThread(hThread);
			CONTEXT ConText;
			ConText.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
			GetThreadContext(hThread, &ConText);
			switch(pTmp->Drx)
			{
			case 0:
				{
					ConText.Dr0 = 0;
					ConText.Dr7 = ConText.Dr7 & 0xFFFFFFFE;
					isDr0 = FALSE;
					nDrCount--;
				}
				break;
				
			case 1:
				{
					ConText.Dr1 = 0;
					ConText.Dr7 = ConText.Dr7 & 0xFFFFFFFB;
					isDr1 = FALSE;
					nDrCount--;
				}
				break;
				
			case 2:
				{
					ConText.Dr2 = 0;
					ConText.Dr7 = ConText.Dr7 & 0xFFFFFFEF;
					isDr2 = FALSE;
					nDrCount--;
				}
				break;
			}
			if(nDrCount == 0)
			{
				ConText.Dr7 = ConText.Dr7 & 0xFFFFFEFF;
			}
			SetThreadContext(hThread, &ConText);
			ResumeThread(hThread);
			ListDel(nBreakPointNum);
			nBreakPointNum = 0;
			CloseHandle(hThread);
		}
		break;

		case BREAK_TYPE_MEM:
		{
			for (DWORD i = pTmp->dwBreakAddress, j=0; i<= pTmp->dwBreakAddressEnd; i++, j++)
			{
				DWORD dwOldProtect;
				if(!VirtualProtectEx(ProcessInfo.hProcess,
					(LPVOID)i,
					1,
					pTmp->dwMemOldProtect[j],
					&dwOldProtect))
				{
					strcpy(szErrStr, "恢复断点处原属性错误!");
					return FALSE;
				}
			}
			ListDel(nBreakPointNum);									//删除断点链表节点
			SetBreakPoint(pTmp->dwBreakAddress, BREAK_TYPE_MEM);
			nBreakPointNum = 0;	
		}
		break;

		case BREAK_TYPE_GO:
		{
			HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, DebugEvent.dwThreadId);
			SuspendThread(hThread);
			CONTEXT ConText;
			ConText.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
			GetThreadContext(hThread, &ConText);
			ConText.Dr3 = pTmp->dwBreakAddress;
			ConText.Dr7 = ConText.Dr7 & 0xFFFFFFBF;
			if(nDrCount == 0)
			{
				ConText.Dr7 = ConText.Dr7 & 0xFFFFFEFF;
				ConText.Dr3 = 0;
			}
			SetThreadContext(hThread, &ConText);
			ResumeThread(hThread);
			ListDel(nBreakPointNum);
			CloseHandle(hThread);
			nBreakPointNum = 0;
		}
		break;

		case BREAK_TYPE_TEMP:
		{
				
		}
		break;
		
	}
	nBreakPointNum = 0;
	return 1;
};


//16进制翻译函数
void HexToText(BYTE * pShellCode, int nCodeLends, char *szCodeText)
{
	memset(szCodeText, 0, 8);
	for(int s=0; s< nCodeLends; s++)
	{
		if(*((BYTE *)((int)pShellCode +s)) >= 0xB0 &&					//判断是否是汉字
			*((BYTE *)((int)pShellCode +s)) <= 0xF7 &&
			*((BYTE *)((int)pShellCode +s+1)) >= 0xA1 &&
			*((BYTE *)((int)pShellCode +s+1)) <= 0xFE
			)
		{
			char szTemp[3] = {0};
			memcpy(szTemp, (BYTE *)((int)pShellCode +s), 2);
			strcat(szCodeText, szTemp);
			s++;
		}
		else if(*((BYTE *)((int)pShellCode +s)) >= 0x20 && *((BYTE *)((int)pShellCode +s)) <= 0x7E)		//判断是否是ASC码
		{
			char szTemp[2] = {0};
			szTemp[0] = *((BYTE *)((int)pShellCode +s));
			strcat(szCodeText, szTemp);
		}
		else										//不是个东西
		{
			strcat(szCodeText,".");
		}
	}
}


void SetOutputText()
{
	//设置反汇编信息
	char szCode[200];
	ReadProcessMemorySafe(ProcessInfo.hProcess,					//读取断点处代码
					(LPVOID)CodeAddrss,
					szCode, 200, NULL);
	DWORD nCodeLends = Disasm(szCode,MAXCMDSIZE, 0, &Tdisasm, DISASM_CODE);	//反汇编指令
	char szProcName[255] = {0};
	if(!memcmp(Tdisasm.result, "CALL", 4))						//解释函数名称
	{
		DWORD nProcAddress = 0;
		char szProcAddr[12] = {0};
		int t = 4;
		int h = 0;
		while (Tdisasm.result[t] != '\0')
		{
			if((Tdisasm.result[t] >= '0' && 
			   Tdisasm.result[t] <= '9') ||
			   (Tdisasm.result[t] >= 'A' &&
			    Tdisasm.result[t] <= 'F'))
			{
				szProcAddr[h++] = Tdisasm.result[t];
			}
			t++;
		}
		sscanf(szProcAddr,"%X", &nProcAddress);
		strcpy(szProcName, GetProcName(nProcAddress));
	}

	int AsmTextCount = sprintf(szASM, "%08X  %-16s  %s  %s\r\n", CodeAddrss, Tdisasm.dump, Tdisasm.result, szProcName);
	strcpy(szNowAsm, Tdisasm.result);
	dwNextCommandAddr = (DWORD)CodeAddrss + nCodeLends;
	DWORD TmpCodeCount = nCodeLends;
	for(int i=0; i<11; i++)
	{
		int nOldCodeCount = TmpCodeCount;
		TmpCodeCount += Disasm(szCode+TmpCodeCount,MAXCMDSIZE, 0, &Tdisasm, DISASM_CODE);

		char szProcName[255] = {0};
		if(!memcmp(Tdisasm.result, "CALL", 4))
		{
			DWORD nProcAddress = 0;
			char szProcAddr[12] = {0};
			int t = 4;
			int h = 0;
			while (Tdisasm.result[t] != '\0')
			{
				if((Tdisasm.result[t] >= '0' && 
					Tdisasm.result[t] <= '9') ||
					(Tdisasm.result[t] >= 'A' &&
					Tdisasm.result[t] <= 'F'))
				{
					szProcAddr[h++] = Tdisasm.result[t];
				}
				t++;
			}
			sscanf(szProcAddr,"%X", &nProcAddress);
			strcpy(szProcName, GetProcName(nProcAddress));
		}
		CodeAddrssLost = (LPVOID)((DWORD)TmpCodeCount+(DWORD)CodeAddrss);

		AsmTextCount += sprintf(szASM+AsmTextCount, "%08X  %-16s  %s  %s\r\n", (DWORD)CodeAddrss+nOldCodeCount, Tdisasm.dump, Tdisasm.result, szProcName);
	}
	
	//设置寄存器信息
	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, DebugEvent.dwThreadId);
	CONTEXT ConText;
	ConText.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
	GetThreadContext(hThread, &ConText);
	CloseHandle(hThread);
	sprintf(szRegister, "┃ EAX %08X |┃ EBX %08X |┃ ECX %08X |┃ EDX %08X|┃ ESP %08X |┃ EBP %08X |┃ ESI %08X |┃ EDI %08X|┃ ────── |┃ EIP %08X |┃ EFL %08X |┃ ────── |┫ DR0 %08X |┃ DR1 %08X |┃ DR2 %08X |┃ DR3 %08X |┃ DR7 %08X ",
						 ConText.Eax, ConText.Ebx, ConText.Ecx, ConText.Edx, 
						 ConText.Esp, ConText.Ebp, ConText.Esi, ConText.Edi,
						 ConText.Eip, ConText.EFlags, ConText.Dr0, ConText.Dr1,
						 ConText.Dr2, ConText.Dr3, ConText.Dr7);

	//设置内存信息
	BYTE bMemData[40];
	DWORD szMemTmp = (int)szMem;
	DWORD dwMemData = (int)bMemData;
	DWORD dwShowMemAddr = (int)pShowMemAddr;
	if(ReadProcessMemorySafe(ProcessInfo.hProcess,					//读取要显示的内存数据
						 pShowMemAddr,
					     bMemData, 40, NULL))
	{
		for(int s = 0; s<40; s++)
		{
			if(s == 32)
			{
				szMemTmp += sprintf((char *)szMemTmp, "┸");
			}
			else
			{
				szMemTmp += sprintf((char *)szMemTmp, "─");
			}
		}
		szMemTmp += sprintf((char *)szMemTmp, "|");
		char szTextCode[18];
		for(int i = 0; i<5; i++)
		{
			HexToText((BYTE *)dwMemData, 16, szTextCode);
			szMemTmp += sprintf((char *)szMemTmp, "%08X│%02X %02X %02X %02X┊%02X %02X %02X %02X┊%02X %02X %02X %02X┊%02X %02X %02X %02X│%s|",
							dwShowMemAddr, *((BYTE *)dwMemData), *((BYTE *)(dwMemData)+1),
							*((BYTE *)(dwMemData)+2), *((BYTE *)(dwMemData)+3), *((BYTE *)(dwMemData)+4),
							*((BYTE *)(dwMemData)+5), *((BYTE *)(dwMemData)+6), *((BYTE *)(dwMemData)+7),
							*((BYTE *)(dwMemData)+8), *((BYTE *)(dwMemData)+9), *((BYTE *)(dwMemData)+10),
							*((BYTE *)(dwMemData)+11), *((BYTE *)(dwMemData)+12), *((BYTE *)(dwMemData)+13),
							*((BYTE *)(dwMemData)+14), *((BYTE *)(dwMemData)+15),
							 szTextCode);

 			dwMemData +=16;
 			dwShowMemAddr +=16;
		}
	}
	//设置栈信息字符串
	BYTE bStackData[20];
	DWORD dwStackData = (DWORD)bStackData;
	DWORD dwStack = (DWORD)szStack;
	if(ReadProcessMemorySafe(ProcessInfo.hProcess,					//读取要显示的内存数据
						 (LPVOID)ConText.Esp,
					     (LPVOID)bStackData, 16, NULL)
						 )
	{
		int nTmp = *((int *)dwStackData);
		dwStack += sprintf((char *)dwStack, "$->│%08X|", nTmp);
		dwStackData+=4;
		for(int i = 1; i<4; i++)
		{
			nTmp = *((int *)dwStackData);
			dwStack += sprintf((char *)dwStack, "$+%X│%08X|",i*4, nTmp);
			dwStackData+=4;
		}
	}
	
	
	//设置断点信息字符串
	DWORD dwBreak = (DWORD)szBreak;
	BREAK_POINT *pTmp = pHeadBreakPoint;
	memset(szBreak, 0, strlen(szBreak));
	pTmp = pHeadBreakPoint;
	for (int s=0; s<32; s++)
	{
		dwBreak+=sprintf((char *)dwBreak,"━");
	}
	dwBreak+=sprintf((char *)dwBreak,"|");
	while (pTmp)
	{
		if(pTmp->BreakType != BREAK_TYPE_GO && pTmp->BreakType != BREAK_TYPE_TEMP)
		{
			char szMemTtype[5];
			char szIsEff[5];
			switch(pTmp->BreakType)
			{
			case BREAK_TYPE_HARD:
				{
					strcpy(szMemTtype, "硬件");
				}
				break;
			case BREAK_TYPE_MEM:
				{
					strcpy(szMemTtype, "内存");
				}
				break;
			case BREAK_TYPE_SOFT:
				{
					strcpy(szMemTtype, "软断");
				}
				break;
			}
			
			if(pTmp->dwMemType == 0)
			{
				strcpy(szIsEff, "Read");
			}
			else if(pTmp->dwMemType == 1)
			{
				strcpy(szIsEff, "Write");
			}
			else if(pTmp->dwMemType == 2)
			{
				strcpy(szIsEff, "Exec");
			}
			else
			{
				strcpy(szIsEff, "");
			}
			
			dwBreak+=sprintf((char *)dwBreak,
							"%d %08X %s % 5s|",
							pTmp->nBreakNum,
							pTmp->dwBreakAddress,
							szMemTtype,
							szIsEff
							);
		}
		pTmp = pTmp->pNextPoint;
	}
	
}


//调试线程回调函数
DWORD WINAPI DebugThread(LPVOID lpParameter)
{

	if(!CreateProcess(szFileName, NULL, NULL, NULL, TRUE,			//创建调试进程
		DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS,
		NULL,szFilePath, &StartupInfo, &ProcessInfo))
	{
		strcpy(szErrStr, "打开调试进程失败!");
		int i = ::GetLastError();
		return 0;
	}

	CreateIATtoIMT();
	//OEP处设置断点
	SetBreakPointInfo(ImageOptionalHead.AddressOfEntryPoint + ImageOptionalHead.ImageBase,"程序执行入口");
	AddSoftBreak();


	//开始循环捕获调试信息
	while(TRUE)
	{
		WaitForDebugEvent(&DebugEvent, INFINITE);
		
		if(DebugEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
		{
			strcpy(szErrStr, "被调试进程关闭,Q(q)命令退出!");
			isCodePase = TRUE;
		}
		if(DebugEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)	//如果是异常信息
		{
			EXCEPTION_DEBUG_INFO ExceptionDebugInfo = {0};
			memcpy(&ExceptionDebugInfo, &(DebugEvent.u), sizeof(EXCEPTION_DEBUG_INFO));
			
			switch (ExceptionDebugInfo.ExceptionRecord.ExceptionCode)
			{
			case EXCEPTION_ACCESS_VIOLATION:						//内存断点
				{
					BREAK_POINT *pTmp = NULL;
					pTmp = pHeadBreakPoint;
					while (pTmp)
					{
						if(pTmp->BreakType == BREAK_TYPE_MEM &&			//判断异常地址是否在断点的页范围内
						   pTmp->isEffect == TRUE &&			
						   ((ExceptionDebugInfo.ExceptionRecord.ExceptionInformation)[1] & 0xFFFFF000) == ((pTmp->dwBreakAddress) & 0xFFFFF000) ||
						   ((ExceptionDebugInfo.ExceptionRecord.ExceptionInformation)[1] & 0xFFFFF000) == ((pTmp->dwBreakAddressEnd) & 0xFFFFF000)
						  )
						{
							break;
						}
						pTmp = pTmp->pNextPoint;
					}
					if(pTmp)
					{																//恢复断点处原属性
						for (DWORD i = pTmp->dwBreakAddress, j=0; i<= pTmp->dwBreakAddressEnd; i++, j++)
						{
							DWORD dwOldProtect;
							if(!VirtualProtectEx(ProcessInfo.hProcess,
												(LPVOID)i,
												1,
												pTmp->dwMemOldProtect[j],
												&dwOldProtect))
							{
								strcpy(szErrStr, "恢复断点处原属性错误!");
								return FALSE;
							}
						}
						SetNextSetup(DebugEvent.dwThreadId);			//设置临时自单步
						isTmpSetup = 1;
						if(isMemSetupShow)
						{
							isSetup1Show = TRUE;
							isUserCommand = TRUE;
							isMemSetupShow = FALSE;
						}


						pTmp = pHeadBreakPoint;							//判断是否是断点中的地址
						while (pTmp)
						{
							if(pTmp->BreakType == BREAK_TYPE_MEM &&	
								pTmp->isEffect == TRUE &&			
								pTmp->dwMemType == (ExceptionDebugInfo.ExceptionRecord.ExceptionInformation)[0] &&
								(ExceptionDebugInfo.ExceptionRecord.ExceptionInformation)[1] >= pTmp->dwBreakAddress &&
								(ExceptionDebugInfo.ExceptionRecord.ExceptionInformation)[1] <= pTmp->dwBreakAddressEnd
							  )
							{
								break;
							}
							pTmp = pTmp->pNextPoint;
						}
						if(pTmp)										//如果是断点列表中的断点
						{
							CodeAddrss = ExceptionDebugInfo.ExceptionRecord.ExceptionAddress;
							SetOutputText();
							isUserCommand = TRUE;
							sprintf(szErrStr, "断点命中 %d 号", pTmp->nBreakNum);
							RunUserOrder();											//等待用户指令
							isTmpSetup = 1;											//设置临时单步标记
							
						}
						::ContinueDebugEvent(DebugEvent.dwProcessId,DebugEvent.dwThreadId, DBG_CONTINUE);
					}
					else
					{
						::ContinueDebugEvent(DebugEvent.dwProcessId,DebugEvent.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);			//不是我的异常产生的
					}
				}
				break;
				
			case EXCEPTION_BREAKPOINT:								//断点异常处理
				{
					BREAK_POINT *pTmp = NULL;
					pTmp = pHeadBreakPoint;
					while (pTmp)
					{
						if(pTmp->BreakType == BREAK_TYPE_SOFT && pTmp->isEffect == TRUE && (PVOID)(pTmp->dwBreakAddress) == ExceptionDebugInfo.ExceptionRecord.ExceptionAddress)
						{
							break;
						}
						pTmp = pTmp->pNextPoint;
					}
					if(pTmp)														//找到断点
					{
						if(!WriteProcessMemorySafe(ProcessInfo.hProcess,			//还原代码
													(LPVOID)pTmp->dwBreakAddress,
													&(pTmp->bSrcCode), 1, NULL))
						{
							::MessageBox(NULL,"临时回复软断点源代码失败!", "错误", MB_OK);
						}

						isTmpSetup = 1;											//设置标志自单步
						isUserCommand = TRUE;									//设置需要用户输入
						CodeAddrss = ExceptionDebugInfo.ExceptionRecord.ExceptionAddress;
						SetOutputText();
						
						HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, DebugEvent.dwThreadId);
						SuspendThread(hThread);
						CONTEXT ConText;
						ConText.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
						GetThreadContext(hThread, &ConText);
						ConText.Eip--;
						SetThreadContext(hThread, &ConText);
						ResumeThread(hThread);
						CloseHandle(hThread);
						sprintf(szErrStr, "断点命中 %d 号", pTmp->nBreakNum);
						RunUserOrder();											//等待用户指令
						isTmpSetup = 1;											//设置临时单步标记
						

					}
				}
				::ContinueDebugEvent(DebugEvent.dwProcessId,DebugEvent.dwThreadId, DBG_CONTINUE);
				break;
			case EXCEPTION_SINGLE_STEP:							//单步处理
				{
					HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, DebugEvent.dwThreadId);
					SuspendThread(hThread);
					CONTEXT ConText;
					ConText.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
					GetThreadContext(hThread, &ConText);
					if((ConText.Dr6 & 0x0000000F) != 0)								    //找到硬件断点
					{
						ConText.EFlags = ConText.EFlags | 0x100;
						ConText.Dr7 = ConText.Dr7 & 0xFFFFFF00;
						SetThreadContext(hThread, &ConText);
						ResumeThread(hThread);
						CloseHandle(hThread);
						isTmpSetup = 1;	
						isUserCommand = TRUE;

						BREAK_POINT *pTmp = NULL;
						pTmp = pHeadBreakPoint;
						strcpy(szErrStr, "断点命中 硬件");
						while (pTmp)
						{
							if((pTmp->BreakType == BREAK_TYPE_GO) && ((ConText.Dr6 & 0x00000008) != 0))
							{
								nBreakPointNum = pTmp->nBreakNum;
								DelBreakPoint();
								strcpy(szErrStr, "");
								break;
							}
							pTmp = pTmp->pNextPoint;
						}
						CodeAddrss = ExceptionDebugInfo.ExceptionRecord.ExceptionAddress;
						SetOutputText();
						RunUserOrder();											//等待用户指令
						isTmpSetup = 1;											//设置临时单步标记
					}
					else														//没找到硬件断点
					{
						if(isTmpSetup == 1)								//如果是临时单步第一次
						{		
							if(isSetup1Show)
							{
								CodeAddrss = ExceptionDebugInfo.ExceptionRecord.ExceptionAddress;
								SetOutputText();
								isUserCommand = TRUE;
								RunUserOrder();
								isSetup1Show = FALSE;
							}
							SetBreakPoint(dwziSetupAddress, ziSetupType);			//恢复断点
						}
						else if(isTmpSetup == 2)							//如果是用户单步
						{
							CodeAddrss = ExceptionDebugInfo.ExceptionRecord.ExceptionAddress;
							SetOutputText();
							isUserCommand = TRUE;
 							RunUserOrder();										//等待用户输入指令
						}
						ResumeThread(hThread);
					}
				}
				::ContinueDebugEvent(DebugEvent.dwProcessId,DebugEvent.dwThreadId, DBG_CONTINUE);
				break;
			}
		}
		else
		{
			::ContinueDebugEvent(DebugEvent.dwProcessId,DebugEvent.dwThreadId, DBG_CONTINUE);
		}
	}
}


//创建调试进程函数--导出
BOOL CreateDbgProc()
{
	if(!PeScan(szFileName, szErrStr, &ImageFileHead, &ImageOptionalHead, &ImageSectionHeader))
	{
		return FALSE;
	}
	DWORD dwThread = 0;
	HANDLE hDebugThread = CreateThread(NULL,
		0,
		(LPTHREAD_START_ROUTINE)DebugThread,
		NULL,
		0,
		&dwThread);
	return TRUE;
};


//操作命令解析函数 --导出;
BOOL OpCommend()
{
	strcpy(szErrStr,"");
	if(!strcmp(szOpCommend, ""))
	{
		return TRUE;
	}

	char szCommend[4][20] = {0};		//分裂操作指令
	int nAgvCount = 1;				//操作指令个数
	int nCommendIndex = 0;
	for (int i=0,j=0; szOpCommend[i] != '\0'; i++,j++)
	{
		if(szOpCommend[i] == ' ')
		{
			nCommendIndex++;
			j = 0;
			nAgvCount++;
			i++;
		}
		if(nCommendIndex >=4)
		{
			strcpy(szErrStr, "错误指令,参数过多!");
			isCodePase = TRUE;
			return FALSE;
		}
		szCommend[nCommendIndex][j] = szOpCommend[i];
	}

	if(!strcmp(szCommend[0], "t") || !strcmp(szCommend[0], "T"))		//单步步入指令
	{
		strcpy(szOpCode, szCommend[0]);
		return TRUE;
	}
	else if(!strcmp(szCommend[0], "p") || !strcmp(szCommend[0], "P"))		//单步步过指令
	{
		strcpy(szOpCode, szCommend[0]);
		return TRUE;
	}
	else if(!strcmp(szCommend[0], "g") || !strcmp(szCommend[0], "G"))		//运行指令
	{
		if(strcmp(szCommend[1], ""))
		{	
			if(strlen(szCommend[1]) >8)										//检查地址参数长度
			{
				strcpy(szErrStr, "错误指令,地址参数长度错误");
				isCodePase = TRUE;
				return FALSE;
			}
			for (UINT i = 0; i < strlen(szCommend[1]); i++)
			{
				if(
				   !((szCommend[1][i] >= '0' && szCommend[1][i] <= '9') ||
				   (szCommend[1][i] >= 'a' && szCommend[1][i] <= 'f') ||
				   (szCommend[1][i] >= 'A' && szCommend[1][i] <= 'F'))
				  )
				{
					strcpy(szErrStr, "错误指令,无效的地址值");
					isCodePase = TRUE;
					return FALSE;
				}
			}
			sscanf(szCommend[1],"%X", &dwGoAddress);
			
		}
		else
		{
			dwGoAddress = 0;
		}
		strcpy(szOpCode, szCommend[0]);
	}
	else if(!strcmp(szCommend[0], "bps") || !strcmp(szCommend[0], "BPS"))			//软断点指令
	{
		if(strcmp(szCommend[1], ""))
		{
			if(strlen(szCommend[1]) >8)										//检查地址参数长度
			{
				strcpy(szErrStr, "错误指令,地址参数长度错误");
				isCodePase = TRUE;
				return FALSE;
			}
			for (UINT i = 0; i < strlen(szCommend[1]); i++)
			{
				if(
					!((szCommend[1][i] >= '0' && szCommend[1][i] <= '9') ||
					(szCommend[1][i] >= 'a' && szCommend[1][i] <= 'f') ||
					(szCommend[1][i] >= 'A' && szCommend[1][i] <= 'F'))
				  )
				{
					strcpy(szErrStr, "错误指令,无效的地址值");
					isCodePase = TRUE;
					return FALSE;
				}
			}
			sscanf(szCommend[1],"%X", &dwBreakAddress);
			
			if(!AddSoftBreak())
			{
				return FALSE;
			}
			SetOutputText();
			isCodePase = TRUE;
		}
		else
		{
			strcpy(szErrStr, "错误指令,缺少参数!");
			isCodePase = TRUE;
			return FALSE;
		}
	}
	else if(!strcmp(szCommend[0], "d") || !strcmp(szCommend[0], "D"))			//显示内存指令
	{
		if(strcmp(szCommend[1], ""))
		{
			if(strlen(szCommend[1]) >8)										//检查地址参数长度
			{
				strcpy(szErrStr, "错误指令,地址参数长度错误");
				isCodePase = TRUE;
				return FALSE;
			}
			for (UINT i = 0; i < strlen(szCommend[1]); i++)
			{
				if(
					!((szCommend[1][i] >= '0' && szCommend[1][i] <= '9') ||
					(szCommend[1][i] >= 'a' && szCommend[1][i] <= 'f') ||
					(szCommend[1][i] >= 'A' && szCommend[1][i] <= 'F'))
					)
				{
					strcpy(szErrStr, "错误指令,无效的地址值");
					isCodePase = TRUE;
					return FALSE;
				}
			}
			sscanf(szCommend[1],"%X", &pShowMemAddr);
			
			SetOutputText();
			isCodePase = TRUE;
		}
		else
		{
			strcpy(szErrStr, "错误指令,缺少参数!");
			isCodePase = TRUE;
			return FALSE;
		}
	}
	else if(!strcmp(szCommend[0], "dbp") || !strcmp(szCommend[0], "DBP"))			//删除断点指令
	{
		if(strcmp(szCommend[1], ""))
		{
			if(strlen(szCommend[1]) >8)										//检查地址参数长度
			{
				strcpy(szErrStr, "错误指令,断点编号不正确");
				isCodePase = TRUE;
				return FALSE;
			}
			for (UINT i = 0; i < strlen(szCommend[1]); i++)
			{
				if(
					!((szCommend[1][i] >= '0' && szCommend[1][i] <= '9') ||
					(szCommend[1][i] >= 'a' && szCommend[1][i] <= 'f') ||
					(szCommend[1][i] >= 'A' && szCommend[1][i] <= 'F'))
					)
				{
					strcpy(szErrStr, "错误指令,无效的断点编号");
					isCodePase = TRUE;
					return FALSE;
				}
			}
			sscanf(szCommend[1],"%X", &nBreakPointNum);
			if(!DelBreakPoint())
			{
				return FALSE;
			}
			SetOutputText();
			isCodePase = TRUE;
		}
		else
		{
			strcpy(szErrStr, "错误指令,缺少参数!");
			isCodePase = TRUE;
			return FALSE;
		}
	}
	else if(!strcmp(szCommend[0], "bpm") || !strcmp(szCommend[0], "BPM"))			//内存断点指令
	{
		DWORD MemBreakLends = 0;
		if(strcmp(szCommend[1], ""))
		{
			if(strlen(szCommend[1]) >8)										//检查地址参数长度
			{
				strcpy(szErrStr, "错误指令,地址参数长度错误");
				isCodePase = TRUE;
				return FALSE;
			}
			for (UINT i = 0; i < strlen(szCommend[1]); i++)
			{
				if(
					!((szCommend[1][i] >= '0' && szCommend[1][i] <= '9') ||
					(szCommend[1][i] >= 'a' && szCommend[1][i] <= 'f') ||
					(szCommend[1][i] >= 'A' && szCommend[1][i] <= 'F'))
					)
				{
					strcpy(szErrStr, "错误指令,无效的地址值");
					isCodePase = TRUE;
					return FALSE;
				}
			}
			sscanf(szCommend[1],"%X", &dwBreakAddress);
		}
		else
		{
			strcpy(szErrStr, "错误指令,缺少地址参数!");
			isCodePase = TRUE;
			return FALSE;
		}
		
		if(strcmp(szCommend[2], ""))
		{
			if(strlen(szCommend[2]) >2)										//检查地址参数长度
			{
				strcpy(szErrStr, "错误指令,长度参数超出范围");
				isCodePase = TRUE;
				return FALSE;
			}
			for (UINT i = 0; i < strlen(szCommend[2]); i++)
			{
				if(
					!((szCommend[2][i] >= '0' && szCommend[2][i] <= '9') ||
					(szCommend[2][i] >= 'a' && szCommend[2][i] <= 'f') ||
					(szCommend[2][i] >= 'A' && szCommend[2][i] <= 'F'))
					)
				{
					strcpy(szErrStr, "错误指令,无效的长度参数");
					isCodePase = TRUE;
					return FALSE;
				}
			}
			sscanf(szCommend[2],"%X", &MemBreakLends);
			if(MemBreakLends <=0 || MemBreakLends>= 255)
			{
				strcpy(szErrStr, "错误指令,长度参数超出范围");
				isCodePase = TRUE;
				return FALSE;
			}
		}
		else
		{
			strcpy(szErrStr, "错误指令,缺少地址参数!");
			isCodePase = TRUE;
			return FALSE;
		}
		
		if(!strcmp(szCommend[3], "read") || !strcmp(szCommend[3], "write"))		//检查类型参数
		{
			SetMemBreakInfo(MemBreakLends, szCommend[3]);
		}
		else
		{
			strcpy(szErrStr, "错误指令,不可识别的内存断点类型参数!");
			isCodePase = TRUE;
			return FALSE;
		}

		if(!AddMemBreak())
		{
			return FALSE;
		}
		SetOutputText();
		isCodePase = TRUE;
	}
	else if(!strcmp(szCommend[0], "bph") || !strcmp(szCommend[0], "BPH"))		//硬件断点
	{
		DWORD MemBreakLends = 0;
		if(strcmp(szCommend[1], ""))
		{
			if(strlen(szCommend[1]) >8)										//检查地址参数长度
			{
				strcpy(szErrStr, "错误指令,地址参数长度错误");
				isCodePase = TRUE;
				return FALSE;
			}
			for (UINT i = 0; i < strlen(szCommend[1]); i++)
			{
				if(
					!((szCommend[1][i] >= '0' && szCommend[1][i] <= '9') ||
					(szCommend[1][i] >= 'a' && szCommend[1][i] <= 'f') ||
					(szCommend[1][i] >= 'A' && szCommend[1][i] <= 'F'))
					)
				{
					strcpy(szErrStr, "错误指令,无效的地址值");
					isCodePase = TRUE;
					return FALSE;
				}
			}
			sscanf(szCommend[1],"%X", &dwBreakAddress);
		}
		else
		{
			strcpy(szErrStr, "错误指令,缺少地址参数!");
			isCodePase = TRUE;
			return FALSE;
		}
		
		if(strcmp(szCommend[2], ""))
		{
			if(strlen(szCommend[2]) >2)										//检查地址参数长度
			{
				strcpy(szErrStr, "错误指令,长度参数超出范围");
				isCodePase = TRUE;
				return FALSE;
			}
			for (UINT i = 0; i < strlen(szCommend[2]); i++)
			{
				if(
					!((szCommend[2][i] >= '0' && szCommend[2][i] <= '9') ||
					(szCommend[2][i] >= 'a' && szCommend[2][i] <= 'f') ||
					(szCommend[2][i] >= 'A' && szCommend[2][i] <= 'F'))
					)
				{
					strcpy(szErrStr, "错误指令,无效的长度参数");
					isCodePase = TRUE;
					return FALSE;
				}
			}
			sscanf(szCommend[2],"%X", &MemBreakLends);
			if(MemBreakLends != 1 && MemBreakLends != 2 && MemBreakLends != 4)
			{
				strcpy(szErrStr, "错误指令,长度参数超出范围");
				isCodePase = TRUE;
				return FALSE;
			}
		}
		else
		{
			strcpy(szErrStr, "错误指令,缺少地址参数!");
			isCodePase = TRUE;
			return FALSE;
		}
		
		if(!strcmp(szCommend[3], "read") || !strcmp(szCommend[3], "write") || !strcmp(szCommend[3], "exec"))		//检查类型参数
		{
			SetMemBreakInfo(MemBreakLends, szCommend[3]);
		}
		else
		{
			strcpy(szErrStr, "错误指令,不可识别的硬件断点类型参数!");
			isCodePase = TRUE;
			return FALSE;
		}
		
		if(!AddHeadBreak())
		{
			return FALSE;
		}
		SetOutputText();
		isCodePase = TRUE;
	}
	else if(!strcmp(szCommend[0], "u") || !strcmp(szCommend[0], "U"))
	{
		CodeAddrss = CodeAddrssLost;
		SetOutputText();
		isCodePase = TRUE;
	}
	else if(!strcmp(szCommend[0], "q") || !strcmp(szCommend[0], "Q"))
	{
		TerminateProcess(ProcessInfo.hProcess, 0);													//结束被调试进程
		strcpy(szOpCode, szCommend[0]);
		return TRUE;
	}
	else
	{
		strcpy(szErrStr, "指令无法识别!");
		isCodePase = TRUE;
		return FALSE;
	}
	return TRUE;
}
