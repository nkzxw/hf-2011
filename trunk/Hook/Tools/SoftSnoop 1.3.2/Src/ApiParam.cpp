
#include <windows.h>
#include <string.h>
#include "SoftSnoop.h"

#define MAX_API_DEF_FILES        100
#define MAX_DEF_FILENAME_LENGTH  40
#define STRING_READ_SIZE         10

// structs
typedef struct
{
	CHAR   DefFileObject[MAX_DEF_FILENAME_LENGTH];
	VOID*  MapAddress;
	DWORD  MapSize;
} sDefFileInfo;

// functions
BOOL  MapSSFiles();
BOOL  DoMapFile(CHAR* szFname);
VOID  UnmapSSFiles();

//constants
CONST CHAR*      szFilesToFind         = "\\ApiDef\\*.ss";
CONST CHAR*      szApiDefDir           = "\\ApiDef\\";

// global variables
sDefFileInfo     DefFileInfo[MAX_API_DEF_FILES];
DWORD            dwDefFileNum = 0;
CHAR             cApiParam[300];


BOOL MapSSFiles()
{
	WIN32_FIND_DATA  FindData;
	HANDLE           hFind;

	// find all SS files
	wsprintf(cApiParam, "%s%s", Option.cAppPath, szFilesToFind);

	hFind = FindFirstFile(cApiParam,&FindData);
	if (hFind == INVALID_HANDLE_VALUE)
		return FALSE;

	DoMapFile(FindData.cFileName);

	while (FindNextFile(hFind,&FindData))
		DoMapFile(FindData.cFileName);

	// clean up
	FindClose(hFind);
	return TRUE;
}

BOOL DoMapFile(CHAR* szFname)
{
	CHAR    cFDir[MAX_PATH],*pCH;
	HANDLE  hFile;
	DWORD   dwFsize,dwBytesRead;
	WORD    wFnameSize;
	VOID*   pMem;

	if (dwDefFileNum == MAX_API_DEF_FILES)
		return FALSE;

	// get the file path
	wsprintf(cFDir, "%s%s%s", Option.cAppPath, szApiDefDir, szFname);

	// map the file
	hFile = CreateFile(cFDir,GENERIC_READ,FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	dwFsize = GetFileSize(hFile,NULL);
	pMem = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,dwFsize+1);
	if (!pMem)
	{
		CloseHandle(hFile);
		return FALSE;
	}
	ReadFile(hFile,pMem,dwFsize,&dwBytesRead,NULL);

	// fill DefFileInfo struct
	DefFileInfo[dwDefFileNum].MapAddress = pMem;
	DefFileInfo[dwDefFileNum].MapSize = dwFsize+1;
	pCH = strstr(szFname,".");
	wFnameSize = (WORD)((DWORD)pCH - (DWORD)szFname);
	if (wFnameSize > MAX_DEF_FILENAME_LENGTH-1)
		return FALSE;
	lstrcpyn(DefFileInfo[dwDefFileNum].DefFileObject,szFname,wFnameSize+1);
	++dwDefFileNum;

	// clean up
	CloseHandle(hFile);
	return TRUE;
}

VOID UnmapSSFiles()
{
	WORD w;

	for (w=0; w < dwDefFileNum; w++)
		GlobalFree(DefFileInfo[w].MapAddress);
	return;
}

BOOL ToNextLine(CHAR* &pCH)
{
	while (*pCH != 0xA)
	{
		if (!*pCH)
			return FALSE;
		++pCH;
	}
	while (*pCH < 0x20)
	{
		if (!*pCH)
			return FALSE;
		++pCH;
	}
	return TRUE;
}

CHAR* GetApiParam(CHAR* szDll,CHAR* szApi,HANDLE hProc,VOID* pStack)
{
	CHAR    *pCH,cBuff[100], *pWCH;
	WORD    w,wDllNameLength,wIndex,wBuff;
	DWORD   *pdwStack,dwStackVal,dwBytesRead,dwBuff;
	BYTE    byBuff;
	DWORD   i;

	memset(&cApiParam,0,sizeof(cApiParam));

	// get the size of the dll name without the extension
	wDllNameLength = lstrlen(szDll) - 4;

	// search the dll name in the DefFileInfo struct's
	wIndex = 0xFFFF;
	for (w=0; w < dwDefFileNum; w++)
		if (strnicmp(DefFileInfo[w].DefFileObject,szDll,wDllNameLength) == 0)
		{
			wIndex = w;
			break;
		}
	if (wIndex == 0xFFFF)
		return NULL;

	// search the function in the ss-file memory
	pCH = strstr(
		(CHAR*)DefFileInfo[wIndex].MapAddress,
		szApi);
	if (!pCH)
		return NULL;

	// get memory for process stack
	lstrcpy(cApiParam,"API: ");
	lstrcat(cApiParam,szApi);
	wsprintf(cBuff,"(%s)",szDll);
	lstrcat(cApiParam,cBuff);
	pdwStack = (DWORD*)pStack;
	++pdwStack; // skip the return value
	ToNextLine(pCH);
	while (*pCH != '-' && *pCH > 0x20)
	{
		lstrcat(cApiParam,", ");
		if (!ReadProcessMemory(  // read the value at the current stack position
			hProc,
			(VOID*)pdwStack,
			&dwStackVal,
			4,
			&dwBytesRead))
			return NULL;
		switch (*pCH)
		{
		case '0': // BOOL
			if (!dwStackVal)
				lstrcpy(cBuff, "FALSE");
			else
				lstrcpy(cBuff, "TRUE");
			lstrcat(cApiParam, cBuff);
			break;
		case '1': // DWORD
			wsprintf(cBuff,"%08lXh",dwStackVal);
			lstrcat(cApiParam,cBuff);
			break;

		case '2': // WORD
			wsprintf(cBuff,"%04lXh",(WORD)dwStackVal);
			lstrcat(cApiParam,cBuff);
			break;

		case '3': // BYTE
			wsprintf(cBuff,"%02lXh",(BYTE)dwStackVal);
			lstrcat(cApiParam,cBuff);
			break;

		case '4': // PSTR
			wsprintf(cBuff,"%08lXh",dwStackVal);
			lstrcat(cApiParam,cBuff);
			lstrcat(cApiParam,"=");
			if (HIWORD(dwStackVal)) // is it a string ?
			{
				memset(&cBuff,0,sizeof(cBuff));
				if (!ReadProcessMemory( // grab the string out of the target process memory
					hProc,
					(VOID*)dwStackVal,
					&cBuff,
					STRING_READ_SIZE,
					&dwBytesRead))
					lstrcat(cApiParam,"?");
				else
				{
					lstrcat(cApiParam,"\"");
					lstrcat(cApiParam,cBuff);

					// add by ygq, read more
					if (strlen(cBuff)>=STRING_READ_SIZE)
					{
						for (i=0; i<10; i++)
						{
							if (ReadProcessMemory( // grab the string out of the target process memory
								hProc,
								(VOID*)(dwStackVal+(i+1)*STRING_READ_SIZE),
								&cBuff,
								STRING_READ_SIZE,
								&dwBytesRead))
							{
								lstrcat(cApiParam,cBuff);
								if (strlen(cBuff)<STRING_READ_SIZE)
									break;
							}
							else
								break;
						}
					}
					// end add by ygq

					lstrcat(cApiParam,"\"");
				}
			}
			else
				lstrcat(cApiParam,"?");
			break;

		case '5': // LPDWORD
			wsprintf(cBuff,"%08lX",dwStackVal);
			lstrcat(cApiParam,cBuff);
			if (dwStackVal)
			{
				lstrcat(cApiParam,"=");
				if (!ReadProcessMemory(
					hProc,
					(VOID*)dwStackVal,
					&dwBuff,
					4,
					&dwBytesRead))
					lstrcat(cApiParam,"?");
				else
				{
					wsprintf(cBuff,"%08lXh",dwBuff);
					lstrcat(cApiParam,cBuff);
				}
			}
			break;

		case '6': // LPWORD
			wsprintf(cBuff,"%08lX",dwStackVal);
			lstrcat(cApiParam,cBuff);
			if (dwStackVal)
			{
				lstrcat(cApiParam,"=");
				if (!ReadProcessMemory(
					hProc,
					(VOID*)dwStackVal,
					&wBuff,
					2,
					&dwBytesRead))
					lstrcat(cApiParam,"?");
				else
				{
					wsprintf(cBuff,"%04lXh",wBuff);
					lstrcat(cApiParam,cBuff);
				}
			}
			break;

		case '7': // LPBYTE
			wsprintf(cBuff,"%08lX",dwStackVal);
			lstrcat(cApiParam,cBuff);
			if (dwStackVal)
			{
				lstrcat(cApiParam,"=");
				if (!ReadProcessMemory(
					hProc,
					(VOID*)dwStackVal,
					&byBuff,
					1,
					&dwBytesRead))
					lstrcat(cApiParam,"?");
				else
				{
					wsprintf(cBuff,"%02lXh",byBuff);
					lstrcat(cApiParam,cBuff);
				}
			}
			break;

		case '8': // LPWSTR
			wsprintf(cBuff,"%08lXh",dwStackVal);
			lstrcat(cApiParam,cBuff);
			lstrcat(cApiParam,"=");
			if (HIWORD(dwStackVal)) // is it a string ?
			{
				memset(&cBuff,0,sizeof(cBuff));
				if (!ReadProcessMemory( // grab the string out of the target process memory
					hProc,
					(VOID*)dwStackVal,
					&cBuff,
					2 * STRING_READ_SIZE,
					&dwBytesRead))
					lstrcat(cApiParam,"?");
				else
				{
					lstrcat(cApiParam,"\"");
					// convert UNICODE to ASCII string per byte
					pWCH = (CHAR*)&cBuff;
					dwBuff = STRING_READ_SIZE;
					while (*pWCH && dwBuff)
					{
						lstrcat(cApiParam,pWCH);
						pWCH += 2;
						--dwBuff;
					}
					lstrcat(cApiParam,"\"");
				}
			}
			else
				lstrcat(cApiParam,"?");
			break;

		default: // handle as DWORD
			wsprintf(cBuff,"%08lXh",dwStackVal);
			lstrcat(cApiParam,cBuff);
			break;
		}

		if (!ToNextLine(pCH)) // the ss file mem is at the end
			break;
		++pdwStack;
	}
	return cApiParam;
}