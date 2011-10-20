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

typedef struct __MY_IMAGE_SECTION_HEADER			//������Ϣ�ṹ��
{
	int SectionCount;
	IMAGE_SECTION_HEADER *pImageSectionHeader;
	
} MY_IMAGE_SECTION_HEADER;


typedef struct __MY_INT_TO_IAT			//������Ϣ�ṹ��
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


//-------------------------------����������
#pragma data_seg("Shared")


BREAK_POINT *pHeadBreakPoint = NULL;					//�ϵ�������ָ��
BREAK_POINT *pTailBreakPoint = NULL;					//�ϵ�����βָ��
BREAK_POINT BreakPointDataZero = {0};					//0ֵ�ϵ�ṹ��
BREAK_POINT BreakPointData[255] = {0};					//�ϵ�ռ�
STARTUPINFO StartupInfo = {0};							//���Գ���������Ϣ
PROCESS_INFORMATION ProcessInfo = {0};					//���Խ�����Ϣ
DEBUG_EVENT DebugEvent = {0};							//������Ϣ�ṹ
IMAGE_FILE_HEADER ImageFileHead = {0};					//PE�ļ�ͷ
IMAGE_OPTIONAL_HEADER ImageOptionalHead = {0};			//PEѡ��ͷ
MY_IMAGE_SECTION_HEADER ImageSectionHeader = {0};		//PE������Ϣ

int nLodCount = 0;
int nBreakNum = 0;										//�ϵ���(����)
char szFilePath[MAX_PATH] = {0};						//���Գ���·��
char szFileName[MAX_PATH] = {0};						//���Գ�������
char szErrStr[100] = {0};								//�����ַ���
BOOL isTmpSetup = FALSE;								//��ʱ����
char szOpCode[10] = {0};								//����ָ��
BOOL isCodePase = FALSE;								//������ͣ
DWORD dwBreakAddress = 0;								//�ϵ��ַ
DWORD dwGoAddress = 0;									//Go�����ַ
char szComment[255] = {0};								//�ϵ�ע���ַ�
int nBreakPointNum = 0;									//Ҫ�����Ķϵ���
BOOL isDr0 = FALSE;										//Dr0�Ƿ�ռ��
BOOL isDr1 = FALSE;										//Dr1�Ƿ�ռ��
BOOL isDr2 = FALSE;										//Dr2�Ƿ�ռ��
int nDrCount = 0;										//Dr������
int nBreakLends = 0;									//�ϵ㳤��
DWORD dwProtect = PAGE_EXECUTE_READWRITE;				//�ڴ汣������
BOOL isUserCommand = FALSE;								//�Ƿ���Ҫ�û�����
DWORD dwNextCommandAddr = NULL;							//��һ��ָ��ĵ�ַ
BOOL isSetup1Show = FALSE;								//������ʱ�����Ƿ���Ҫ��ʾ
LPVOID pShowMemAddr = (LPVOID)0x00401000;				//Ҫ��ʾ���ڴ��ַ
char szASM[1024] = {0};									//������ַ���
char szRegister[300] = {0};								//�Ĵ����ַ���
char szMem[1024] = {0};									//�ڴ���Ϣ�ַ���
char szStack[100] = {0};								//ջ��Ϣ�ַ���
char szBreak[1024] = {0};								//�ϵ���Ϣ�ַ���
char szNowAsm[255] = {0};								//��ǰָ��ķ�����ַ���
char szOpCommend[255] = {0};							//����ָ��
LPVOID CodeAddrss = 0;									//��ǰ�����ַ
LPVOID CodeAddrssLost = 0;								//���һ�������ָ���ַ
char szPreProcess[MAX_PATH] = {0};						//���ظ�����·��
PTHREAD_START_ROUTINE OpCommendAddress = 0;				//OpCommend��ַ
int nImportCount = 0;									//���뺯������
MY_INT_TO_IAT* pMyIATtoINT = NULL;						//IAT_IMT���ձ�ָ��
DWORD dwziSetupAddress = 0;								//Ҫ�ظ��Ķϵ��ַ
BREAK_POINT_TYPE ziSetupType = BREAK_TYPE_TEMP;			//Ҫ�ظ��Ķϵ�����
t_disasm Tdisasm = {0};									//�����ṹ��
HMODULE hR3DbgDll = 0;									//�����ں�DLL���ص������еľ��
BOOL isMemSetupShow = FALSE;							//��������ڴ�����쳣�Ƿ���Ҫ�û�����


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
	if((nLodCount == 0))		//���Ƽ��س���·��
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


//���̺߳���
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


//PEɨ�躯��
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
	if(INVALID_HANDLE_VALUE == hLoadFile)					//���ļ�
	{
		strcpy(szErrStr, "��ȡ�ļ�ʧ��!");
		return FALSE;
	}
	
	
	SetFilePointer(hLoadFile, 0, NULL, FILE_BEGIN);			//��ȡ�ļ�ͷ��־
	isRead = ReadFile(hLoadFile, szExeHead, 2, &nReadSize, NULL);
	if(2 != nReadSize && isRead == FALSE)
	{
		strcpy(szErrStr, "��ȡ�ļ�ͷ��Ϣʧ��!");
		CloseHandle(hLoadFile);
		return FALSE;
	}
	if(0 != memcmp(szExeHead, bExeHead, 2))
	{
		strcpy(szErrStr, "������Ч�Ŀ�ִ�г���!");
		CloseHandle(hLoadFile);
		return FALSE;
	}
	
	SetFilePointer(hLoadFile, 0x3C, NULL, FILE_BEGIN);						//��ȡPEͷƫ��
	isRead = ReadFile(hLoadFile, (char *)&nPEOffset, 4, &nReadSize, NULL);
	if(4 != nReadSize && isRead == FALSE)
	{
		strcpy(szErrStr, "��ȡPEͷλ��ʧ��!");
		CloseHandle(hLoadFile);
		return FALSE;
	}

	SetFilePointer(hLoadFile, nPEOffset, NULL, FILE_BEGIN);
	isRead = ReadFile(hLoadFile, (char *)&szPeStr, 4, &nReadSize, NULL);
	if(4 != nReadSize && isRead ==FALSE)
	{
		strcpy(szErrStr, "��ȡPE��־ʧ��!");
		CloseHandle(hLoadFile);
		return FALSE;
	}
	
	if(0 != memcmp(szPeStr, bPeStr, 4))
	{
		strcpy(szErrStr, "������Ч��PE��ʽ!");
		CloseHandle(hLoadFile);
		return FALSE;
	}

	isRead = ReadFile(hLoadFile, pImageFileHead, sizeof(IMAGE_FILE_HEADER), &nReadSize, NULL);
	if(sizeof(IMAGE_FILE_HEADER) != nReadSize && isRead == FALSE)	//��ȡFILEͷ��Ϣ
	{
		strcpy(szErrStr, "��ȡFileͷ���ݴ���!");
		CloseHandle(hLoadFile);
		return FALSE;
	}

	isRead = ReadFile(hLoadFile, pImageOptionalHead, pImageFileHead->SizeOfOptionalHeader, &nReadSize, NULL);
	if(pImageFileHead->SizeOfOptionalHeader != nReadSize && isRead == FALSE)
	{
		strcpy(szErrStr, "��ȡѡ�������ݴ���!");
		CloseHandle(hLoadFile);
		return FALSE;
	}
	
	
	pMyImageSectionHeader->SectionCount = pImageFileHead->NumberOfSections;
	pMyImageSectionHeader->pImageSectionHeader = new IMAGE_SECTION_HEADER[pMyImageSectionHeader->SectionCount];
	UINT SectionSize = sizeof(IMAGE_SECTION_HEADER) * pMyImageSectionHeader->SectionCount;

	isRead = ReadFile(hLoadFile, pMyImageSectionHeader->pImageSectionHeader, SectionSize, &nReadSize, NULL);
	if(SectionSize != nReadSize && isRead == FALSE)
	{
		strcpy(szErrStr, "��ȡ����Ϣ���ݴ���!");
		CloseHandle(hLoadFile);
		return FALSE;
	}
	CloseHandle(hLoadFile);																		//�ر��ļ�
	return TRUE;
}


//��ȫ�Ķ�ȡ�ڴ溯��
BOOL ReadProcessMemorySafe(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead)
{
	DWORD dwOldProtect;										//�޸�����Ϊ�ɶ���д
	if(!VirtualProtectEx(hProcess,
		(LPVOID)lpBaseAddress,
		nSize,
		PAGE_EXECUTE_READWRITE,
		&dwOldProtect))
	{
		strcpy(szErrStr, "��ȡ�ڴ�ʱ�����ڴ����Դ���!");
		return FALSE;
	}
	
	ReadProcessMemory(hProcess,					//��ȡ�ڴ�����
		lpBaseAddress,
		lpBuffer, nSize, lpNumberOfBytesRead);
	
	if(!VirtualProtectEx(hProcess,				//��ԭ�ڴ�ԭ����
		(LPVOID)lpBaseAddress,
		nSize,
		dwOldProtect,
		&dwOldProtect))
	{
		strcpy(szErrStr, "��ȡ�ڴ�ʱ��ԭ�ڴ����Դ���!");
		return FALSE;
	}
	return TRUE;
}


//��ȫ��д���ڴ溯��
BOOL WriteProcessMemorySafe(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead)
{
	DWORD dwOldProtect;										//�޸�����Ϊ�ɶ���д
	if(!VirtualProtectEx(hProcess,
						(LPVOID)lpBaseAddress,
						nSize,
						PAGE_EXECUTE_READWRITE,
						&dwOldProtect))
	{
		strcpy(szErrStr, "��ȡ�ڴ�ʱ�����ڴ����Դ���!");
		return FALSE;
	}
	
	WriteProcessMemory(hProcess,					//д���ڴ�����
					  (LPVOID)lpBaseAddress,
					  lpBuffer, nSize, lpNumberOfBytesRead);
	
	if(!VirtualProtectEx(hProcess,					//��ԭ�ڴ�ԭ����
						(LPVOID)lpBaseAddress,
						nSize,
						dwOldProtect,
						&dwOldProtect))
	{
		strcpy(szErrStr, "��ȡ�ڴ�ʱ��ԭ�ڴ����Դ���!");
		return FALSE;
	}
	return TRUE;
}


//�������жϵ���Ϣ
BOOL SetBreakPointAll()					
{
	BREAK_POINT *pTmp = NULL;
	pTmp = pHeadBreakPoint;
	while (pTmp)
	{
		if(pTmp->BreakType == BREAK_TYPE_SOFT && pTmp->isEffect == TRUE)	//����ϵ�
		{
			BYTE bBreakPoint = 0xCC;
			if(!WriteProcessMemorySafe(ProcessInfo.hProcess,//��ԭ����
										(LPVOID)pTmp->dwBreakAddress,
										&bBreakPoint, 1, NULL))
			{
				strcpy(szErrStr, "д����ϵ�ʧ��!");
				return FALSE;
			}
		}

		if(pTmp->BreakType == BREAK_TYPE_HARD && pTmp->isEffect == TRUE)	//Ӳ���ϵ�
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
					switch(pTmp->dwMemType)						//���öϵ�����
					{
					case 0:				//����
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFFCFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x30000;
						}
						break;
					case 1:				//д��
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFFCFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x10000;
						}
						break;
					case 2:				//ִ��
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFFCFFFF;
						}
						break;
					}

					switch(pTmp->dwBreakAddressEnd - pTmp->dwBreakAddress +1)	//���öϵ㳤��
					{
					case 1:				//����1
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFF3FFFF;
						}
						break;
					case 2:				//����2
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFF3FFFF;
							ConText.Dr7 = ConText.Dr7 | 0x40000;
						}
						break;
					case 4:				//����4
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

	//���öϵ�����
					{
					case 0:				//����
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFCFFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x300000;
						}
						break;
					case 1:				//д��
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFCFFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x100000;
						}
						break;
					case 4:				//ִ��
						{
							ConText.Dr7 = ConText.Dr7 & 0xFFCFFFFF;
						}
						break;
					}
					
					switch(pTmp->dwBreakAddressEnd - pTmp->dwBreakAddress +1)	//���öϵ㳤��
					{
					case 1:				//����1
						{
							ConText.Dr7 = ConText.Dr7 & 0xFF3FFFFF;
						}
						break;
					case 2:				//����2
						{
							ConText.Dr7 = ConText.Dr7 & 0xFF3FFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x400000;
						}
						break;
					case 4:				//����4
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
					switch(pTmp->dwMemType)						//���öϵ�����
					{
					case 0:				//����
						{
							ConText.Dr7 = ConText.Dr7 & 0xFCFFFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x3000000;
						}
						break;
					case 1:				//д��
						{
							ConText.Dr7 = ConText.Dr7 & 0xFCFFFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x1000000;
						}
						break;
					case 2:				//ִ��
						{
							ConText.Dr7 = ConText.Dr7 & 0xFCFFFFFF;
						}
						break;
					}
					
					switch(pTmp->dwBreakAddressEnd - pTmp->dwBreakAddress +1)	//���öϵ㳤��
					{
					case 1:				//����1
						{
							ConText.Dr7 = ConText.Dr7 & 0xF3FFFFFF;
						}
						break;
					case 2:				//����2
						{
							ConText.Dr7 = ConText.Dr7 & 0xF3FFFFFF;
							ConText.Dr7 = ConText.Dr7 | 0x4000000;
						}
						break;
					case 4:				//����4
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

		//�ڴ�ϵ�
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
					strcpy(szErrStr, "���öϵ��ʾ���ڴ����Դ���");
					return FALSE;
				}
			}
		}
		pTmp = pTmp->pNextPoint;
	}
	return 1;
}


//����IAT-IMT���ձ�
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
		
		int u = 0;											//��ȡģ������
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
			ReadProcessMemorySafe(ProcessInfo.hProcess,								//��ȡָ���OriginalFirstThunk�ṹ��
							  (LPVOID)(Idd[s].OriginalFirstThunk + pianyi),
							  &ImageThunkData,
							  4, NULL);
			if(ImageThunkData == 0)
			{
				break;
			}
			if((ImageThunkData & 0x80000000) == 0)								//�ж�����Ǻ���������
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
// 							  (LPVOID)(Idd[s].FirstThunk + pianyi),				//��ȡָ���FirstThunk������ַ
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


//��ȡ��������
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


//����ϵ�ռ�
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


//�ͷŶϵ�ռ�
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


//��ȡOp������ַ--����
PTHREAD_START_ROUTINE GetOpCommend()
{
	return OpCommendAddress;
}


//��ȡ������Ϣ����--����
char * GetErr()										
{
	return szErrStr;
}


//��ȡR3Dbg��ģ����
HMODULE GetR3DbgHandle()
{
	return hR3DbgDll;
}


//��ȡ���Խ��̾��
HANDLE GetR3ProcHandle()
{
	return 	ProcessInfo.hProcess;
}


//���ñ����Գ���·������--����
void SetDbgNamePath(char *szDbgName, char *szDbgPath)
{
	strcpy(szFileName, szDbgName);
	strcpy(szFilePath, szDbgPath);
}


//����Ҫ���Ӷϵ���Ϣ����--����
void SetBreakPointInfo(DWORD BreakAddress, char *Comment)
{
	dwBreakAddress = BreakAddress;
	strcpy(szComment, Comment);
}


//����Ҫ���ӵ��ڴ�ϵ㸽����Ϣ����--����
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
		strcpy(szErrStr, "���öϵ����ʹ���!");
		return FALSE;
	}
	return TRUE;
}


//����Ҫ�����Ķϵ��ź���--����
void SetBreakNum(int BreakNum)
{
	nBreakPointNum = BreakNum;
}


//���ò���ָ���--����
void SetOpCode(char *OpCode)
{
	strcpy(szOpCommend, OpCode);
}


//��ȡ�����ָ���--����
char *GetAsmStr()
{
	return szASM;
}


//���ó�������״̬--����
void SetIsPase(BOOL isPase)
{
	isCodePase = isPase;
}


//��ȡ��������״̬--����
BOOL GetIsPase()
{
	return isCodePase;
}


//��ȡ�Ĵ�����Ϣ�ַ���--����
char *GetRegisterStr()
{
	return szRegister;
}


//��ȡ�ڴ���Ϣ�ַ���--����
char *GetMemStr()
{
	return szMem;
}


//��ȡջ��Ϣ�ַ���--����
char *GetStackStr()
{
	return szStack;
}


//��ȡ�ϵ���Ϣ�ַ���--����
char *GetBreakStr()
{
	return szBreak;
}


//�������ؽ���·��
void SetPrProcess(char *Proprocess)
{
	strcpy(szPreProcess, Proprocess);
}


//���Ӷϵ�����ڵ㺯��
void ListAdd(BREAK_POINT *pNewPoint)
{
	if(pHeadBreakPoint)								//����ϵ�����
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


//ɾ���ϵ�����ڵ㺯��
void ListDel(int nBreakNum)								//ɾ���ϵ�����ڵ�
{
	BREAK_POINT *pTmp = pHeadBreakPoint;
	BREAK_POINT *pLast = NULL;
	while(pTmp)										//���Ҹ���ŵĽڵ��ַ
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

	if(pTmp == pHeadBreakPoint)						//Ҫɾ����Ϊͷ�ڵ�
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
	
	if(pTmp == pTailBreakPoint)						//Ҫɾ����Ϊβ�ڵ�
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

	pLast->pNextPoint = pTmp->pNextPoint;			//Ҫɾ�������м�ڵ�
	if(pTmp)
	{
		DelBk(pTmp);
		pTmp = NULL;
	}
	return ;
}


//���öϵ���Ϣ
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
			if(pTmp->BreakType == BREAK_TYPE_SOFT && pTmp->isEffect == TRUE && pTmp->dwBreakAddress == BkAddress)	//����ϵ�
			{
				BYTE bBreakPoint = 0xCC;
				if(!WriteProcessMemorySafe(ProcessInfo.hProcess,				//��ԭ����
										  (LPVOID)pTmp->dwBreakAddress,
										  &bBreakPoint, 1, NULL))
				{
					strcpy(szErrStr, "д����ϵ�ʧ��!");
					return FALSE;
				}
			}
			pTmp = pTmp->pNextPoint;
		}
	}

	if(BkTypt == BREAK_TYPE_HARD)							//Ӳ���ϵ�
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
						switch(pTmp->dwMemType)							//���öϵ�����
						{
						case 0:				//����
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFFCFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x30000;
							}
							break;
						case 1:				//д��
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFFCFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x10000;
							}
							break;
						case 2:				//ִ��
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFFCFFFF;
							}
							break;
						}
						
						switch(pTmp->dwBreakAddressEnd - pTmp->dwBreakAddress +1)			//���öϵ㳤��
						{
						case 1:				//����1
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFF3FFFF;
							}
							break;
						case 2:				//����2
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFF3FFFF;
								ConText.Dr7 = ConText.Dr7 | 0x40000;
							}
							break;
						case 4:				//����4
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
						switch(pTmp->dwMemType)							//���öϵ�����
						{
						case 0:				//����
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFCFFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x300000;
							}
							break;
						case 1:				//д��
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFCFFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x100000;
							}
							break;
						case 4:				//ִ��
							{
								ConText.Dr7 = ConText.Dr7 & 0xFFCFFFFF;
							}
							break;
						}
						
						switch(pTmp->dwBreakAddressEnd - pTmp->dwBreakAddress +1)			//���öϵ㳤��
						{
						case 1:				//����1
							{
								ConText.Dr7 = ConText.Dr7 & 0xFF3FFFFF;
							}
							break;
						case 2:				//����2
							{
								ConText.Dr7 = ConText.Dr7 & 0xFF3FFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x400000;
							}
							break;
						case 4:				//����4
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
						switch(pTmp->dwMemType)							//���öϵ�����
						{
						case 0:				//����
							{
								ConText.Dr7 = ConText.Dr7 & 0xFCFFFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x3000000;
							}
							break;
						case 1:				//д��
							{
								ConText.Dr7 = ConText.Dr7 & 0xFCFFFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x1000000;
							}
							break;
						case 2:				//ִ��
							{
								ConText.Dr7 = ConText.Dr7 & 0xFCFFFFFF;
							}
							break;
						}
						
						switch(pTmp->dwBreakAddressEnd - pTmp->dwBreakAddress +1)			//���öϵ㳤��
						{
						case 1:				//����1
							{
								ConText.Dr7 = ConText.Dr7 & 0xF3FFFFFF;
							}
							break;
						case 2:				//����2
							{
								ConText.Dr7 = ConText.Dr7 & 0xF3FFFFFF;
								ConText.Dr7 = ConText.Dr7 | 0x4000000;
							}
							break;
						case 4:				//����4
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
			if(pTmp->BreakType == BREAK_TYPE_MEM && pTmp->isEffect == TRUE)						//�ڴ�ϵ�
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
						strcpy(szErrStr, "���öϵ��ʾ���ڴ����Դ���");
						return FALSE;
					}
				}
			}
			pTmp = pTmp->pNextPoint;
		}
	}

	return 1;
}


//������ϵ㺯��--����
BOOL AddSoftBreak()
{
	BREAK_POINT *pTmp = NULL;
	pTmp = pHeadBreakPoint;
	while (pTmp)									//Ѱ�Ҹ����Ͷϵ��Ƿ����
	{
		if(pTmp->dwBreakAddress == dwBreakAddress && pTmp->BreakType == BREAK_TYPE_SOFT)
		{
			pTmp->isEffect = TRUE;
			strcpy(pTmp->szComment, szComment);
			return 1;
		}
		pTmp = pTmp->pNextPoint;
	}

	BREAK_POINT *pNewPoint = NewBk();		//�����ڸöϵ�,�����¶ϵ�
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
		strcpy(szErrStr, "��ȡ�ϵ�Դ����ʧ��!");
		return 0;
	}
	
	ListAdd(pNewPoint);								//��������
	SetBreakPoint(pNewPoint->dwBreakAddress, BREAK_TYPE_SOFT);
	dwBreakAddress = 0;
	strcpy(szComment, "");
	return 1;
}


//����Ӳ���ϵ㺯��--����
BOOL AddHeadBreak()
{
	BREAK_POINT *pTmp = NULL;
	pTmp = pHeadBreakPoint;
	while (pTmp)									//Ѱ�Ҹ�Ӳ���ϵ��Ƿ����
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
		strcpy(szErrStr, "Ӳ���ϵ�����,�޷�����!");
		return FALSE;
	}
	BREAK_POINT *pNewPoint = NewBk();		//�����ڸöϵ�,�����¶ϵ�
  	pNewPoint->BreakType = BREAK_TYPE_HARD;
	pNewPoint->dwBreakAddress = dwBreakAddress;
	pNewPoint->dwBreakAddressEnd = pNewPoint->dwBreakAddress + nBreakLends -1;
	pNewPoint->isEffect = TRUE;
	pNewPoint->pNextPoint = NULL;
	pNewPoint->nBreakNum = ++nBreakNum;
	pNewPoint->bSrcCode = 0x90;
	pNewPoint->dwMemType = dwProtect;
	strcpy(pNewPoint->szComment, szComment);
	if(!isDr0)								//Ѱ��û��ʹ�õ�Ӳ���ϵ�
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
	ListAdd(pNewPoint);								//��������
	SetBreakPoint(pNewPoint->dwBreakAddress, BREAK_TYPE_HARD);

	dwBreakAddress = 0;
	strcpy(szComment, "");
	return 1;
}


//�����ڴ�ϵ㺯��--����
BOOL AddMemBreak()
{
	BREAK_POINT *pTmp = NULL;
	pTmp = pHeadBreakPoint;
	while (pTmp)									//Ѱ�Ҹ��ڴ�ϵ��Ƿ������еĶϵ㷶Χ��
	{
		if(dwBreakAddress >= pTmp->dwBreakAddress &&
		   (dwBreakAddress + (DWORD)nBreakLends -1) <= pTmp->dwBreakAddressEnd &&
			pTmp->BreakType == BREAK_TYPE_MEM &&
			pTmp->dwMemType == dwProtect
		  )
		{
			pTmp->isEffect = TRUE;
			strcpy(pTmp->szComment, szComment);
			strcpy(szErrStr, "�ϵ㷶Χ�Ѵ���!");
			isCodePase = TRUE;
			return 0;
		}
		pTmp = pTmp->pNextPoint;
	}

	BREAK_POINT *pNewPoint = NewBk();		//�����ڸöϵ�,�����¶ϵ�
	pNewPoint->BreakType = BREAK_TYPE_MEM;
	pNewPoint->dwBreakAddress = dwBreakAddress;
	pNewPoint->dwBreakAddressEnd = pNewPoint->dwBreakAddress + nBreakLends -1;
	pNewPoint->isEffect = TRUE;
	pNewPoint->pNextPoint = NULL;
	pNewPoint->nBreakNum = ++nBreakNum;
	pNewPoint->bSrcCode = 0x90;
	pNewPoint->dwMemType = dwProtect;
													//���öϵ��ԭ�ڴ�����
	MEMORY_BASIC_INFORMATION MemInfoMation;
	for (DWORD i = dwBreakAddress, s = 0;
		i<= pNewPoint->dwBreakAddressEnd;
		i++,s++)
	{
		BREAK_POINT *pTmp = NULL;
		pTmp = pHeadBreakPoint;
		BOOL isFind = FALSE;
		while (pTmp)											//�жϸ��ڴ�ϵ��ַ���ڴ�ԭ�����Ƿ��޸�
		{
			if(pTmp->BreakType == BREAK_TYPE_MEM)
			{
				for (DWORD j=pTmp->dwBreakAddress, t = 0;
					j <= pTmp->dwBreakAddressEnd;
					j++, t++) //�����Ѵ��ڵĶϵ��а����ĵ�ַ
				{
					if((j & 0xFFFFF000) == (i & 0xFFFFF000))		//�������ͬһ��ҳ��
					{
						(pNewPoint->dwMemOldProtect)[s] = (pTmp->dwMemOldProtect)[t];
						isFind = TRUE;
						break;
					}
				}
			}
			pTmp = pTmp->pNextPoint;
		}
		
		if(!isFind)												//û���ҵ��ѱ��޸ĵ��ڴ��ҳ
		{
			VirtualQueryEx(ProcessInfo.hProcess,					//��ѯ�öϵ�λ�õ�ԭ�ڴ����� 
							(LPVOID)i,
							&MemInfoMation,
							sizeof(MEMORY_BASIC_INFORMATION)
							);
			(pNewPoint->dwMemOldProtect)[s] = MemInfoMation.Protect;
		}
	}

	strcpy(pNewPoint->szComment, szComment);
	ListAdd(pNewPoint);								//��������
	
	dwBreakAddress = 0;
	strcpy(szComment, "");
	nBreakLends = 0;
	dwProtect = PAGE_EXECUTE_READWRITE;
	
	SetBreakPoint(pNewPoint->dwBreakAddress, BREAK_TYPE_MEM);
	return 1;
}


//�����´δ��뵥��ִ�к���
BOOL SetNextSetup(DWORD ThreadId)
{
	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, ThreadId);	
	SuspendThread(hThread);
	CONTEXT ConText;											//�����´ε���
	ConText.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
	if(!GetThreadContext(hThread, &ConText))
	{
		strcpy(szErrStr, "��ȡCONTEXT����");
		ResumeThread(hThread);
		return FALSE;
	}
	ConText.EFlags = ConText.EFlags | 0x100;
	
	if(!SetThreadContext(hThread, &ConText))
	{
		strcpy(szErrStr, "����CONTEXT����");
		ResumeThread(hThread);
		return FALSE;
	}
	ResumeThread(hThread);
	CloseHandle(hThread);
	return TRUE;
}


//ִ���û�����ָ��
BOOL RunUserOrder()
{
	if(isUserCommand)
	{
		isCodePase = TRUE;
		strcpy(szOpCode, "");
		while (!strcmp(szOpCode,""))							//�ȴ�����ָ��
		{
			Sleep(1);
		}
		Sleep(300);

		if((!strcmp(szOpCode, "p") || !strcmp(szOpCode, "P")) && memcmp(szNowAsm,"CALL", 4))
		{
			strcpy(szOpCode,"t");
		}

		if(!strcmp(szOpCode, "t") || !strcmp(szOpCode, "T"))		//������������
		{
			SetNextSetup(DebugEvent.dwThreadId);						//�����´ε���
			isTmpSetup = 2;												//���Ϊ�û�����
			isSetup1Show = TRUE;
			isMemSetupShow = TRUE;
		}
		
		if(!strcmp(szOpCode, "p") || !strcmp(szOpCode, "P"))		//������������
		{
			BREAK_POINT *pNewPoint = NewBk();					//��Ӳ���ϵ�
			pNewPoint->BreakType = BREAK_TYPE_GO;
			pNewPoint->dwBreakAddress = dwNextCommandAddr;				//����ָ���ַ
			pNewPoint->isEffect = TRUE;
			pNewPoint->pNextPoint = NULL;
			pNewPoint->nBreakNum = ++nBreakNum;
			pNewPoint->bSrcCode = 0x90;
			pNewPoint->Drx = 3;
			strcpy(pNewPoint->szComment, "������������");
			ListAdd(pNewPoint);										//��������
			dwBreakAddress = 0;
			strcpy(szComment, "");
			isSetup1Show = FALSE;									//��ǻظ��ϵ�ʱ����ʾ
			isMemSetupShow = FALSE;
			isTmpSetup = 1;
			SetNextSetup(DebugEvent.dwThreadId);
			
		}


		if(!strcmp(szOpCode, "q") || !strcmp(szOpCode, "Q"))		//������˳�����
		{
			HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, DebugEvent.dwThreadId);	
			SuspendThread(hThread);								//�����߳�,�ȴ�ģ��ж��
		}


		if((!strcmp(szOpCode, "g") || !strcmp(szOpCode, "G")) && dwGoAddress)		//������������GO
		{
			BREAK_POINT *pNewPoint = NewBk();					//��Ӳ���ϵ�
			pNewPoint->BreakType = BREAK_TYPE_GO;
			pNewPoint->dwBreakAddress = dwGoAddress;					//ҪGO�ĵ�ַ
			pNewPoint->isEffect = TRUE;
			pNewPoint->pNextPoint = NULL;
			pNewPoint->nBreakNum = ++nBreakNum;
			pNewPoint->bSrcCode = 0x90;
			pNewPoint->Drx = 3;
			strcpy(pNewPoint->szComment, "������������");
			ListAdd(pNewPoint);										//��������
			dwBreakAddress = 0;
			strcpy(szComment, "");
			isSetup1Show = FALSE;									//��ǻظ��ϵ�ʱ����ʾ
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


//ɾ���ϵ㺯��--����
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
		strcpy(szErrStr, "û���ҵ��öϵ�!");
		return FALSE;
	}

	switch(pTmp->BreakType)
	{
		case BREAK_TYPE_SOFT:												//ɾ����ϵ�
		{
			if(!::WriteProcessMemorySafe(ProcessInfo.hProcess,				//��ԭ����
										(LPVOID)pTmp->dwBreakAddress,
										&(pTmp->bSrcCode), 1, NULL))
			{
				strcpy(szErrStr, "��ԭ�ϵ�������!");
				return 0;
			}

			ListDel(nBreakPointNum);									//ɾ���ϵ�����ڵ�
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
					strcpy(szErrStr, "�ָ��ϵ㴦ԭ���Դ���!");
					return FALSE;
				}
			}
			ListDel(nBreakPointNum);									//ɾ���ϵ�����ڵ�
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


//16���Ʒ��뺯��
void HexToText(BYTE * pShellCode, int nCodeLends, char *szCodeText)
{
	memset(szCodeText, 0, 8);
	for(int s=0; s< nCodeLends; s++)
	{
		if(*((BYTE *)((int)pShellCode +s)) >= 0xB0 &&					//�ж��Ƿ��Ǻ���
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
		else if(*((BYTE *)((int)pShellCode +s)) >= 0x20 && *((BYTE *)((int)pShellCode +s)) <= 0x7E)		//�ж��Ƿ���ASC��
		{
			char szTemp[2] = {0};
			szTemp[0] = *((BYTE *)((int)pShellCode +s));
			strcat(szCodeText, szTemp);
		}
		else										//���Ǹ�����
		{
			strcat(szCodeText,".");
		}
	}
}


void SetOutputText()
{
	//���÷������Ϣ
	char szCode[200];
	ReadProcessMemorySafe(ProcessInfo.hProcess,					//��ȡ�ϵ㴦����
					(LPVOID)CodeAddrss,
					szCode, 200, NULL);
	DWORD nCodeLends = Disasm(szCode,MAXCMDSIZE, 0, &Tdisasm, DISASM_CODE);	//�����ָ��
	char szProcName[255] = {0};
	if(!memcmp(Tdisasm.result, "CALL", 4))						//���ͺ�������
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
	
	//���üĴ�����Ϣ
	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, DebugEvent.dwThreadId);
	CONTEXT ConText;
	ConText.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
	GetThreadContext(hThread, &ConText);
	CloseHandle(hThread);
	sprintf(szRegister, "�� EAX %08X |�� EBX %08X |�� ECX %08X |�� EDX %08X|�� ESP %08X |�� EBP %08X |�� ESI %08X |�� EDI %08X|�� ������������ |�� EIP %08X |�� EFL %08X |�� ������������ |�� DR0 %08X |�� DR1 %08X |�� DR2 %08X |�� DR3 %08X |�� DR7 %08X ",
						 ConText.Eax, ConText.Ebx, ConText.Ecx, ConText.Edx, 
						 ConText.Esp, ConText.Ebp, ConText.Esi, ConText.Edi,
						 ConText.Eip, ConText.EFlags, ConText.Dr0, ConText.Dr1,
						 ConText.Dr2, ConText.Dr3, ConText.Dr7);

	//�����ڴ���Ϣ
	BYTE bMemData[40];
	DWORD szMemTmp = (int)szMem;
	DWORD dwMemData = (int)bMemData;
	DWORD dwShowMemAddr = (int)pShowMemAddr;
	if(ReadProcessMemorySafe(ProcessInfo.hProcess,					//��ȡҪ��ʾ���ڴ�����
						 pShowMemAddr,
					     bMemData, 40, NULL))
	{
		for(int s = 0; s<40; s++)
		{
			if(s == 32)
			{
				szMemTmp += sprintf((char *)szMemTmp, "��");
			}
			else
			{
				szMemTmp += sprintf((char *)szMemTmp, "��");
			}
		}
		szMemTmp += sprintf((char *)szMemTmp, "|");
		char szTextCode[18];
		for(int i = 0; i<5; i++)
		{
			HexToText((BYTE *)dwMemData, 16, szTextCode);
			szMemTmp += sprintf((char *)szMemTmp, "%08X��%02X %02X %02X %02X��%02X %02X %02X %02X��%02X %02X %02X %02X��%02X %02X %02X %02X��%s|",
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
	//����ջ��Ϣ�ַ���
	BYTE bStackData[20];
	DWORD dwStackData = (DWORD)bStackData;
	DWORD dwStack = (DWORD)szStack;
	if(ReadProcessMemorySafe(ProcessInfo.hProcess,					//��ȡҪ��ʾ���ڴ�����
						 (LPVOID)ConText.Esp,
					     (LPVOID)bStackData, 16, NULL)
						 )
	{
		int nTmp = *((int *)dwStackData);
		dwStack += sprintf((char *)dwStack, "$->��%08X|", nTmp);
		dwStackData+=4;
		for(int i = 1; i<4; i++)
		{
			nTmp = *((int *)dwStackData);
			dwStack += sprintf((char *)dwStack, "$+%X��%08X|",i*4, nTmp);
			dwStackData+=4;
		}
	}
	
	
	//���öϵ���Ϣ�ַ���
	DWORD dwBreak = (DWORD)szBreak;
	BREAK_POINT *pTmp = pHeadBreakPoint;
	memset(szBreak, 0, strlen(szBreak));
	pTmp = pHeadBreakPoint;
	for (int s=0; s<32; s++)
	{
		dwBreak+=sprintf((char *)dwBreak,"��");
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
					strcpy(szMemTtype, "Ӳ��");
				}
				break;
			case BREAK_TYPE_MEM:
				{
					strcpy(szMemTtype, "�ڴ�");
				}
				break;
			case BREAK_TYPE_SOFT:
				{
					strcpy(szMemTtype, "���");
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


//�����̻߳ص�����
DWORD WINAPI DebugThread(LPVOID lpParameter)
{

	if(!CreateProcess(szFileName, NULL, NULL, NULL, TRUE,			//�������Խ���
		DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS,
		NULL,szFilePath, &StartupInfo, &ProcessInfo))
	{
		strcpy(szErrStr, "�򿪵��Խ���ʧ��!");
		int i = ::GetLastError();
		return 0;
	}

	CreateIATtoIMT();
	//OEP�����öϵ�
	SetBreakPointInfo(ImageOptionalHead.AddressOfEntryPoint + ImageOptionalHead.ImageBase,"����ִ�����");
	AddSoftBreak();


	//��ʼѭ�����������Ϣ
	while(TRUE)
	{
		WaitForDebugEvent(&DebugEvent, INFINITE);
		
		if(DebugEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
		{
			strcpy(szErrStr, "�����Խ��̹ر�,Q(q)�����˳�!");
			isCodePase = TRUE;
		}
		if(DebugEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)	//������쳣��Ϣ
		{
			EXCEPTION_DEBUG_INFO ExceptionDebugInfo = {0};
			memcpy(&ExceptionDebugInfo, &(DebugEvent.u), sizeof(EXCEPTION_DEBUG_INFO));
			
			switch (ExceptionDebugInfo.ExceptionRecord.ExceptionCode)
			{
			case EXCEPTION_ACCESS_VIOLATION:						//�ڴ�ϵ�
				{
					BREAK_POINT *pTmp = NULL;
					pTmp = pHeadBreakPoint;
					while (pTmp)
					{
						if(pTmp->BreakType == BREAK_TYPE_MEM &&			//�ж��쳣��ַ�Ƿ��ڶϵ��ҳ��Χ��
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
					{																//�ָ��ϵ㴦ԭ����
						for (DWORD i = pTmp->dwBreakAddress, j=0; i<= pTmp->dwBreakAddressEnd; i++, j++)
						{
							DWORD dwOldProtect;
							if(!VirtualProtectEx(ProcessInfo.hProcess,
												(LPVOID)i,
												1,
												pTmp->dwMemOldProtect[j],
												&dwOldProtect))
							{
								strcpy(szErrStr, "�ָ��ϵ㴦ԭ���Դ���!");
								return FALSE;
							}
						}
						SetNextSetup(DebugEvent.dwThreadId);			//������ʱ�Ե���
						isTmpSetup = 1;
						if(isMemSetupShow)
						{
							isSetup1Show = TRUE;
							isUserCommand = TRUE;
							isMemSetupShow = FALSE;
						}


						pTmp = pHeadBreakPoint;							//�ж��Ƿ��Ƕϵ��еĵ�ַ
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
						if(pTmp)										//����Ƕϵ��б��еĶϵ�
						{
							CodeAddrss = ExceptionDebugInfo.ExceptionRecord.ExceptionAddress;
							SetOutputText();
							isUserCommand = TRUE;
							sprintf(szErrStr, "�ϵ����� %d ��", pTmp->nBreakNum);
							RunUserOrder();											//�ȴ��û�ָ��
							isTmpSetup = 1;											//������ʱ�������
							
						}
						::ContinueDebugEvent(DebugEvent.dwProcessId,DebugEvent.dwThreadId, DBG_CONTINUE);
					}
					else
					{
						::ContinueDebugEvent(DebugEvent.dwProcessId,DebugEvent.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);			//�����ҵ��쳣������
					}
				}
				break;
				
			case EXCEPTION_BREAKPOINT:								//�ϵ��쳣����
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
					if(pTmp)														//�ҵ��ϵ�
					{
						if(!WriteProcessMemorySafe(ProcessInfo.hProcess,			//��ԭ����
													(LPVOID)pTmp->dwBreakAddress,
													&(pTmp->bSrcCode), 1, NULL))
						{
							::MessageBox(NULL,"��ʱ�ظ���ϵ�Դ����ʧ��!", "����", MB_OK);
						}

						isTmpSetup = 1;											//���ñ�־�Ե���
						isUserCommand = TRUE;									//������Ҫ�û�����
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
						sprintf(szErrStr, "�ϵ����� %d ��", pTmp->nBreakNum);
						RunUserOrder();											//�ȴ��û�ָ��
						isTmpSetup = 1;											//������ʱ�������
						

					}
				}
				::ContinueDebugEvent(DebugEvent.dwProcessId,DebugEvent.dwThreadId, DBG_CONTINUE);
				break;
			case EXCEPTION_SINGLE_STEP:							//��������
				{
					HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, DebugEvent.dwThreadId);
					SuspendThread(hThread);
					CONTEXT ConText;
					ConText.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
					GetThreadContext(hThread, &ConText);
					if((ConText.Dr6 & 0x0000000F) != 0)								    //�ҵ�Ӳ���ϵ�
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
						strcpy(szErrStr, "�ϵ����� Ӳ��");
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
						RunUserOrder();											//�ȴ��û�ָ��
						isTmpSetup = 1;											//������ʱ�������
					}
					else														//û�ҵ�Ӳ���ϵ�
					{
						if(isTmpSetup == 1)								//�������ʱ������һ��
						{		
							if(isSetup1Show)
							{
								CodeAddrss = ExceptionDebugInfo.ExceptionRecord.ExceptionAddress;
								SetOutputText();
								isUserCommand = TRUE;
								RunUserOrder();
								isSetup1Show = FALSE;
							}
							SetBreakPoint(dwziSetupAddress, ziSetupType);			//�ָ��ϵ�
						}
						else if(isTmpSetup == 2)							//������û�����
						{
							CodeAddrss = ExceptionDebugInfo.ExceptionRecord.ExceptionAddress;
							SetOutputText();
							isUserCommand = TRUE;
 							RunUserOrder();										//�ȴ��û�����ָ��
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


//�������Խ��̺���--����
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


//��������������� --����;
BOOL OpCommend()
{
	strcpy(szErrStr,"");
	if(!strcmp(szOpCommend, ""))
	{
		return TRUE;
	}

	char szCommend[4][20] = {0};		//���Ѳ���ָ��
	int nAgvCount = 1;				//����ָ�����
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
			strcpy(szErrStr, "����ָ��,��������!");
			isCodePase = TRUE;
			return FALSE;
		}
		szCommend[nCommendIndex][j] = szOpCommend[i];
	}

	if(!strcmp(szCommend[0], "t") || !strcmp(szCommend[0], "T"))		//��������ָ��
	{
		strcpy(szOpCode, szCommend[0]);
		return TRUE;
	}
	else if(!strcmp(szCommend[0], "p") || !strcmp(szCommend[0], "P"))		//��������ָ��
	{
		strcpy(szOpCode, szCommend[0]);
		return TRUE;
	}
	else if(!strcmp(szCommend[0], "g") || !strcmp(szCommend[0], "G"))		//����ָ��
	{
		if(strcmp(szCommend[1], ""))
		{	
			if(strlen(szCommend[1]) >8)										//����ַ��������
			{
				strcpy(szErrStr, "����ָ��,��ַ�������ȴ���");
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
					strcpy(szErrStr, "����ָ��,��Ч�ĵ�ֵַ");
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
	else if(!strcmp(szCommend[0], "bps") || !strcmp(szCommend[0], "BPS"))			//��ϵ�ָ��
	{
		if(strcmp(szCommend[1], ""))
		{
			if(strlen(szCommend[1]) >8)										//����ַ��������
			{
				strcpy(szErrStr, "����ָ��,��ַ�������ȴ���");
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
					strcpy(szErrStr, "����ָ��,��Ч�ĵ�ֵַ");
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
			strcpy(szErrStr, "����ָ��,ȱ�ٲ���!");
			isCodePase = TRUE;
			return FALSE;
		}
	}
	else if(!strcmp(szCommend[0], "d") || !strcmp(szCommend[0], "D"))			//��ʾ�ڴ�ָ��
	{
		if(strcmp(szCommend[1], ""))
		{
			if(strlen(szCommend[1]) >8)										//����ַ��������
			{
				strcpy(szErrStr, "����ָ��,��ַ�������ȴ���");
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
					strcpy(szErrStr, "����ָ��,��Ч�ĵ�ֵַ");
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
			strcpy(szErrStr, "����ָ��,ȱ�ٲ���!");
			isCodePase = TRUE;
			return FALSE;
		}
	}
	else if(!strcmp(szCommend[0], "dbp") || !strcmp(szCommend[0], "DBP"))			//ɾ���ϵ�ָ��
	{
		if(strcmp(szCommend[1], ""))
		{
			if(strlen(szCommend[1]) >8)										//����ַ��������
			{
				strcpy(szErrStr, "����ָ��,�ϵ��Ų���ȷ");
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
					strcpy(szErrStr, "����ָ��,��Ч�Ķϵ���");
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
			strcpy(szErrStr, "����ָ��,ȱ�ٲ���!");
			isCodePase = TRUE;
			return FALSE;
		}
	}
	else if(!strcmp(szCommend[0], "bpm") || !strcmp(szCommend[0], "BPM"))			//�ڴ�ϵ�ָ��
	{
		DWORD MemBreakLends = 0;
		if(strcmp(szCommend[1], ""))
		{
			if(strlen(szCommend[1]) >8)										//����ַ��������
			{
				strcpy(szErrStr, "����ָ��,��ַ�������ȴ���");
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
					strcpy(szErrStr, "����ָ��,��Ч�ĵ�ֵַ");
					isCodePase = TRUE;
					return FALSE;
				}
			}
			sscanf(szCommend[1],"%X", &dwBreakAddress);
		}
		else
		{
			strcpy(szErrStr, "����ָ��,ȱ�ٵ�ַ����!");
			isCodePase = TRUE;
			return FALSE;
		}
		
		if(strcmp(szCommend[2], ""))
		{
			if(strlen(szCommend[2]) >2)										//����ַ��������
			{
				strcpy(szErrStr, "����ָ��,���Ȳ���������Χ");
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
					strcpy(szErrStr, "����ָ��,��Ч�ĳ��Ȳ���");
					isCodePase = TRUE;
					return FALSE;
				}
			}
			sscanf(szCommend[2],"%X", &MemBreakLends);
			if(MemBreakLends <=0 || MemBreakLends>= 255)
			{
				strcpy(szErrStr, "����ָ��,���Ȳ���������Χ");
				isCodePase = TRUE;
				return FALSE;
			}
		}
		else
		{
			strcpy(szErrStr, "����ָ��,ȱ�ٵ�ַ����!");
			isCodePase = TRUE;
			return FALSE;
		}
		
		if(!strcmp(szCommend[3], "read") || !strcmp(szCommend[3], "write"))		//������Ͳ���
		{
			SetMemBreakInfo(MemBreakLends, szCommend[3]);
		}
		else
		{
			strcpy(szErrStr, "����ָ��,����ʶ����ڴ�ϵ����Ͳ���!");
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
	else if(!strcmp(szCommend[0], "bph") || !strcmp(szCommend[0], "BPH"))		//Ӳ���ϵ�
	{
		DWORD MemBreakLends = 0;
		if(strcmp(szCommend[1], ""))
		{
			if(strlen(szCommend[1]) >8)										//����ַ��������
			{
				strcpy(szErrStr, "����ָ��,��ַ�������ȴ���");
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
					strcpy(szErrStr, "����ָ��,��Ч�ĵ�ֵַ");
					isCodePase = TRUE;
					return FALSE;
				}
			}
			sscanf(szCommend[1],"%X", &dwBreakAddress);
		}
		else
		{
			strcpy(szErrStr, "����ָ��,ȱ�ٵ�ַ����!");
			isCodePase = TRUE;
			return FALSE;
		}
		
		if(strcmp(szCommend[2], ""))
		{
			if(strlen(szCommend[2]) >2)										//����ַ��������
			{
				strcpy(szErrStr, "����ָ��,���Ȳ���������Χ");
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
					strcpy(szErrStr, "����ָ��,��Ч�ĳ��Ȳ���");
					isCodePase = TRUE;
					return FALSE;
				}
			}
			sscanf(szCommend[2],"%X", &MemBreakLends);
			if(MemBreakLends != 1 && MemBreakLends != 2 && MemBreakLends != 4)
			{
				strcpy(szErrStr, "����ָ��,���Ȳ���������Χ");
				isCodePase = TRUE;
				return FALSE;
			}
		}
		else
		{
			strcpy(szErrStr, "����ָ��,ȱ�ٵ�ַ����!");
			isCodePase = TRUE;
			return FALSE;
		}
		
		if(!strcmp(szCommend[3], "read") || !strcmp(szCommend[3], "write") || !strcmp(szCommend[3], "exec"))		//������Ͳ���
		{
			SetMemBreakInfo(MemBreakLends, szCommend[3]);
		}
		else
		{
			strcpy(szErrStr, "����ָ��,����ʶ���Ӳ���ϵ����Ͳ���!");
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
		TerminateProcess(ProcessInfo.hProcess, 0);													//���������Խ���
		strcpy(szOpCode, szCommend[0]);
		return TRUE;
	}
	else
	{
		strcpy(szErrStr, "ָ���޷�ʶ��!");
		isCodePase = TRUE;
		return FALSE;
	}
	return TRUE;
}
