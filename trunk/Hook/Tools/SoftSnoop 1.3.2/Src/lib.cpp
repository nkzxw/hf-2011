
#include <math.h>
#include "lib.h"

LPVOID MapFileR(char * targetfile)
{
	HANDLE   hFile,hFileMap;
	LPVOID   pMappedFile;
	
	hFile = CreateFile ( targetfile, GENERIC_READ,FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}
	hFileMap = CreateFileMapping (hFile, NULL, PAGE_READONLY, 0, 0,NULL);
	if (!hFileMap)
	{
		CloseHandle (hFile);
		return NULL;
	}
	pMappedFile = MapViewOfFile ( hFileMap, FILE_MAP_READ, 0, 0, 0);
	if (!pMappedFile)
	{
		CloseHandle (hFileMap);
		CloseHandle (hFile);
		return NULL;
	}
	CloseHandle(hFileMap);
	CloseHandle(hFile);
	return pMappedFile;
}

LPVOID MapFileRW(char * targetfile)
{
	HANDLE   hFile,hMap;
	LPVOID   pMap;

	hFile = CreateFile ( targetfile, GENERIC_READ | GENERIC_WRITE, \
		FILE_SHARE_READ | FILE_SHARE_WRITE,	NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}
	hMap = CreateFileMapping (hFile, NULL, PAGE_READWRITE, 0, 0,NULL);
	if (!hMap)
	{
		CloseHandle (hFile);
		return NULL;
	}
	pMap = MapViewOfFile ( hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	CloseHandle(hMap);
	CloseHandle(hFile);
	if (!pMap)
		return NULL;
	return pMap;
}

BOOL IsPE (LPVOID MapAddress)
{
	PIMAGE_DOS_HEADER pDosh;
	PIMAGE_NT_HEADERS pPeh;

	if (MapAddress == NULL)
		return FALSE;
	__try // needed for some cruel dos files
	{
		pDosh = (PIMAGE_DOS_HEADER)MapAddress;
		if (pDosh->e_magic != IMAGE_DOS_SIGNATURE)
			return FALSE;
		pPeh = (PIMAGE_NT_HEADERS)((DWORD)pDosh + pDosh->e_lfanew);
		if (pPeh->Signature != IMAGE_NT_SIGNATURE)
			return FALSE;
	}
	__except(1)
	{
		return FALSE;
	}
	return TRUE;
}

DWORD GetFsize(PSTR szTargetFile)
{
	HANDLE   hFile;

	hFile = CreateFile (szTargetFile, GENERIC_READ,FILE_SHARE_READ, NULL, OPEN_EXISTING, \
		               FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return 0xFFFFFFFF;
	}
	return GetFileSize(hFile,0);
}

VOID ShowLastError()
{
	PSTR lpMsgBuf;
    
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | 
		          FORMAT_MESSAGE_IGNORE_INSERTS,NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL, \
				  SUBLANG_DEFAULT),(LPTSTR) &lpMsgBuf,0,NULL);
	MessageBox(0,(PSTR)lpMsgBuf,"LastError:",0);
	return;
}

VOID MakeOfn(OPENFILENAME &TMPofn)
{
	ZeroMemory(&TMPofn,sizeof(TMPofn));
	TMPofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	TMPofn.lpstrTitle = "Choose A file...";
	TMPofn.nMaxFile = 256;
	TMPofn.nFilterIndex = 1;
	TMPofn.lpstrInitialDir = ".";
	TMPofn.lpstrFilter = "any file\0*.*\0\0";
	TMPofn.lStructSize = sizeof(TMPofn);
	return;
}

int mb(char* Title,char* Text = "Info",int Style = 0)
{
	return MessageBox(0,Title,Text,Style);
}

BOOL Hexstr2Int(CHAR* szHexStr,DWORD &dwHexNum)
// returns false if the string isn't a valid hex number
{
	int    i,iStrLen;
	BYTE*  pBy;
	BYTE   byCharNum;
	DWORD  dwResult;
	CHAR   ch;

	iStrLen = strlen(szHexStr);
	pBy = (BYTE*)((DWORD)szHexStr + iStrLen - 1);
	dwResult = 0;
	for(i=0; i!=iStrLen; ++i)
	{
		ch = toupper(*pBy);
		// check if this is a valid hex string
		if ( (ch < '0' || ch > '9') && (ch < 'A' || ch > 'F') )
			return FALSE;
		// calculate...
		switch(ch)
		{
		case 'F':
			byCharNum = 15;
			break;

		case 'E':
			byCharNum = 14;
			break;

		case 'D':
			byCharNum = 13;
			break;

		case 'C':
			byCharNum = 12;
			break;

		case 'B':
			byCharNum = 11;
			break;

		case 'A':
			byCharNum = 10;
			break;

		default:
			byCharNum = *(BYTE*)(pBy) - 0x30;
		}
		dwResult += (DWORD)(pow(16.0,i) * byCharNum);
		--pBy;
	}
	dwHexNum = dwResult; // save the result to one of the args
	return TRUE;
}

BOOL Str2Int(CHAR* szStr,DWORD &dwNum)
// returns false if the string isn't a valid number
{
	int    i,iStrLen;
	BYTE   byCharNum;
	DWORD  dwResult;
	CHAR   ch;

	iStrLen = strlen(szStr);
	dwResult = 0;
	for(i=0; i!=iStrLen; ++i)
	{
		ch = szStr[i];
		// check if this is a valid string
		if (ch < '0' || ch > '9')
			return FALSE;
		// calculate...
		byCharNum = (BYTE)ch - 0x30;

		dwResult = dwResult * 10 + byCharNum;
	}
	dwNum = dwResult; // save the result to one of the args
	return TRUE;
}

BOOL IsRoundedTo(DWORD dwTarNum,DWORD dwRoundNum)
{
	div_t  d;

	d = div((int)dwTarNum,(int)dwRoundNum);
	return (d.rem == 0);
}

BOOL IsNT()
{
	DWORD dwVer;

	dwVer = GetVersion();
	dwVer >>= 31;

	return (dwVer == 0);
}