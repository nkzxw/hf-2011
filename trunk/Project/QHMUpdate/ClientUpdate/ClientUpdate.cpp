#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <winsock2.h>
#include <ShellAPI.h>

#include "ClientUpdate.h"
#include "..\Common\IniFile.h"
#include "..\Common\installService.h"

CIniFile IniFile;

inline 
void CreateRelativeDir(
    LPCSTR parentPath,
	LPSTR relativePath
	)
{
	char *tmpPath = relativePath;
	tmpPath = strchr (tmpPath, '\\');
	if (NULL == tmpPath)
		return;

	char chPath[MAX_PATH];
	memset (chPath, 0, MAX_PATH);
	memcpy (chPath, relativePath, strlen (relativePath) - strlen (tmpPath));
	
	char newDir[MAX_PATH];
	memset (newDir, 0, MAX_PATH);
	sprintf (newDir, "%s\\%s", parentPath, chPath);

	WIN32_FIND_DATAA fData;
	HANDLE hHandle = FindFirstFileA (newDir, &fData);
	if (INVALID_HANDLE_VALUE == hHandle)
	{
		CreateDirectory (newDir, NULL);
	}
	else {
		FindClose (hHandle);
	}
	
	CreateRelativeDir (newDir, ++tmpPath);
}

inline 
BOOL DeleteDirectory  (
	LPCSTR path
	)
{
	BOOL result;
	HANDLE Handle;
	WIN32_FIND_DATAA fData;
		
	char chPath[MAX_PATH];
	memset (chPath, 0, MAX_PATH);
	sprintf (chPath, "%s\\*.*", path);

	Handle = FindFirstFileA(chPath, &fData);
	if (Handle == INVALID_HANDLE_VALUE)
		return FALSE;

	do {
		if (fData.cFileName[0] == '.' )
			continue;

		if (fData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
			memset (chPath, MAX_PATH, 0);
			sprintf (chPath, "%s\\%s", path, fData.cFileName);
			DeleteDirectory(chPath);
		}
		else{
			memset (chPath, 0, MAX_PATH);
			sprintf (chPath, "%s\\%s", path, fData.cFileName);

			SetFileAttributesA(chPath, ~FILE_ATTRIBUTE_READONLY);
			if (!DeleteFileA(chPath)){
				OutputDebugPrintf ("[ClientUpdate]: RemoveDirectoryA Error = %d\r\n", GetLastError ());
				result = FALSE;
			}
			else{
				result = TRUE;
			}
		}

	}while(FindNextFileA(Handle,&fData));

	if (Handle)
		FindClose(Handle);

	SetFileAttributesA(path, ~FILE_ATTRIBUTE_READONLY);
	if (RemoveDirectoryA(path) == 0)
	{
		int iError = GetLastError ();
		OutputDebugPrintf ("[ClientUpdate]: RemoveDirectoryA Error = %d\r\n", GetLastError ());
	}
	
	return result;
}

inline 
void CopyDirectory (
	LPCSTR lpSource, 
	LPCSTR lpDest
	)
{
	if (NULL == lpSource ||
		NULL == lpDest)
	{
		return;
	}

	WIN32_FIND_DATAA fData;
	HANDLE handle = FindFirstFileA (lpDest, &fData);
	if (INVALID_HANDLE_VALUE == handle){
		CreateDirectory (lpDest, NULL);
	}
	else{
		FindClose (handle);
	}

	char chSource[MAX_PATH];
	memset (chSource, 0, MAX_PATH);

	char chDest[MAX_PATH];
	memset (chDest, 0, MAX_PATH);
	
	sprintf (chSource, "%s\\*.*", lpSource);
	HANDLE hHandle = FindFirstFileA (chSource, &fData);
	do {
		if ('.' == fData.cFileName[0])
			continue;
		
		if (FILE_ATTRIBUTE_DIRECTORY == fData.dwFileAttributes)
		{
			char newSource[MAX_PATH];
			memset (newSource, MAX_PATH, 0);
			sprintf (newSource, "%s\\%s", lpSource, fData.cFileName);

			char newDest[MAX_PATH];
			memset (newDest, 0, MAX_PATH);
			sprintf (newDest, "%s\\%s", lpDest, fData.cFileName);

			CopyDirectory(newSource, newDest);
		}
		else {
			char newSFile[MAX_PATH];
			memset (newSFile, 0, MAX_PATH);
			sprintf (newSFile, "%s\\%s", lpSource, fData.cFileName);

			char newDFile[MAX_PATH];
			memset (newDFile, 0, MAX_PATH);
			sprintf (newDFile, "%s\\%s", lpDest, fData.cFileName);
			
			if(CopyFile (newSFile, newDFile, FALSE) == 0){
				OutputDebugPrintf ("[ClientUpdate]: source = %s, dest = %s (Error = %d)\r\n", newSFile, newDFile, GetLastError ());
			}
		}
	}while (FindNextFileA (hHandle, &fData));

	if (hHandle)
		FindClose (hHandle);
}

inline 
BOOL FindFilePath (
	LPCSTR path, 
	LPCSTR fname, 
	LPSTR fullPath
	)
{
	HANDLE Handle;
	WIN32_FIND_DATAA fData;
	bool bResult = false;

	char chPath[MAX_PATH];
	memset (chPath, 0, MAX_PATH);
	sprintf (chPath, "%s\\*.*", path);
	Handle = FindFirstFileA(chPath, &fData);

	if (Handle == INVALID_HANDLE_VALUE)
		return false;

	do {
		if (fData.cFileName[0] == '.' )
			continue;

		if (fData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
			//排除更新文件的拷贝路径
			if (stricmp (fData.cFileName, "update")  == 0)
				continue;

			char newPath[MAX_PATH];
			memset (newPath, MAX_PATH, 0);
			sprintf (newPath, "%s\\%s", path, fData.cFileName);
			bResult = FindFilePath(newPath, fname, fullPath);
			if (true == bResult)
				break;
		}
		else{
			bool bExits = false;
			if (stricmp(fData.cFileName, fname) == 0) {
				sprintf (fullPath, "%s\\%s", path, fData.cFileName);
				bResult = true;
				break;
			}
		}
	}while(FindNextFileA(Handle,&fData));

	if (Handle)
		FindClose(Handle);	
}

inline 
void ScanInstallDir(
    LPCSTR path,
	PIOCP_FILE_INFO c_FileInfo
	)
{
	HANDLE Handle;
	WIN32_FIND_DATAA fData;
	
	char chPath[MAX_PATH];
	memset (chPath, 0, MAX_PATH);
	sprintf (chPath, "%s\\*.*", path);
	Handle = FindFirstFileA(chPath, &fData);

	if (Handle == INVALID_HANDLE_VALUE)
		return;

	do {
		if (fData.cFileName[0] == '.' )
			continue;

		if (fData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
			if (stricmp ("QHMUpdate", fData.cFileName) == 0)
				continue;

			char newPath[MAX_PATH];
			memset (newPath, MAX_PATH, 0);
			sprintf (newPath, "%s\\%s", path, fData.cFileName);
			ScanInstallDir(newPath, c_FileInfo);
		}
		else{
			bool bExits = false;
	
			//fData.cFileName;
			char chFile[MAX_PATH];
			memset (chFile, 0, MAX_PATH);
			sprintf (chFile, "%s\\%s", path, fData.cFileName);
			HANDLE hFile = CreateFile(chFile,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
			if (INVALID_HANDLE_VALUE != hFile){
				DWORD dwFileSize = GetFileSize (hFile, NULL);
				c_FileInfo->size[c_FileInfo->count] = dwFileSize;
				GetFileAttributesExA (chFile, GetFileExInfoStandard, &c_FileInfo->data[c_FileInfo->count]);

				memcpy(c_FileInfo->name[c_FileInfo->count], fData.cFileName, strlen (fData.cFileName));
				c_FileInfo->count++;

				CloseHandle (hFile);
			}	
		}
	}while(FindNextFileA(Handle,&fData));

	if (Handle)
		FindClose(Handle);
}

/*
* 解析更新下来的文件，分发到指定的位置
*/
void 
ParseUpdateFile (
	PIOCP_FILE_INFO c_FileInfo
	)
{
	char chPath[MAX_PATH];
	memset (chPath, 0, MAX_PATH);
	GetCurrentPath (chPath);

	char chUpdate[MAX_PATH];
	memset (chUpdate, 0, MAX_PATH);
	sprintf (chUpdate, "%s\\%s", chPath, UPDATE);
	ScanInstallDir (chUpdate, c_FileInfo);

	char chWinDir[MAX_PATH];
	memset (chWinDir, 0, MAX_PATH);
	GetWindowsDirectoryA (chWinDir, MAX_PATH);

	char chDest[MAX_PATH];
	char chSource[MAX_PATH];

	for (int i = 0; i < c_FileInfo->count; i++)
	{
		//判断文件是否为sys 文件
		char *chExt = GetFileExtName (c_FileInfo->name[i]);
		if (stricmp (chExt, "sys")){
			memset (chSource, 0, MAX_PATH);
			sprintf (chSource, "%s\\%s", chUpdate, c_FileInfo->name[i]);

			memset (chDest, 0, MAX_PATH);
			sprintf (chDest, "%s\\system32\\drivers\\%s", chWinDir, c_FileInfo->name[i]);

			if(CopyFile (chSource, chDest, FALSE) == true)
			{
				OutputDebugPrintf ("[ClientUpdate]: ParseUpdateFile CopyFile1 (error = %d)\r\n", GetLastError ());
			}

			char chFilePath[MAX_PATH];
			memset (chFilePath, 0, MAX_PATH);
			if(FindFilePath (chPath, c_FileInfo->name[i], chFilePath) == true)
			{
				if(CopyFile (chSource, chFilePath, FALSE) == FALSE)
				{
					OutputDebugPrintf ("[ClientUpdate]: ParseUpdateFile CopyFile2 (error = %d)\r\n", GetLastError ());
				}
			}
		}
		else {
			memset (chSource, 0, MAX_PATH);
			sprintf (chSource, "%s\\%s", chUpdate, c_FileInfo->name[i]);

			memset (chDest, 0, MAX_PATH);
			sprintf (chDest, "%s\\%s", chPath, c_FileInfo->name[i]);

			if(CopyFile (chSource, chDest, FALSE) == FALSE)
			{
				OutputDebugPrintf ("[ClientUpdate]: ParseUpdateFile CopyFile3 (error = %d)\r\n", GetLastError ());
			}

		}
	}
}

/*
* 更新自检函数
* 确保更新文件没有替换成功的重新处理。
*/
void AutoCheckLocal ()
{
}

//版本更新，执行安装程序。
void 
ExecuteInstallProcess(
	LPCSTR ExeName
	)
{
	//m_ExePath：程序安装后可执行文件的名字，一般以.exe结尾，比如：TTPlayer.exe
	//m_Path：此可执行文件所在的目录，比如：C:\Program Files\TTPlayer

	char chPath[MAX_PATH];
	memset (chPath, 0, MAX_PATH);
	GetCurrentPath (chPath);

	char chUpdate[MAX_PATH];
	memset (chUpdate, 0, MAX_PATH);
	sprintf (chUpdate, "%s\\%s", chPath, "update");
	ShellExecute( NULL, NULL, ExeName, NULL, chUpdate, SW_SHOW );
}

bool 
CreateSocket(
	SOCKET *pSocket, 
	char *szHost, 
	int nPortNo
	)
{
     struct sockaddr_in ServerAddress;
     struct hostent *Server;

     *pSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);     
     if (INVALID_SOCKET == *pSocket) {
		//TODO
		return false; 
     }
     
     Server = gethostbyname(szHost);
     if (Server == NULL) {
          closesocket(*pSocket);
          return false;
     }
    
     ZeroMemory((char *) &ServerAddress, sizeof(ServerAddress)); 
     ServerAddress.sin_family = AF_INET;
     CopyMemory((char *)&ServerAddress.sin_addr.s_addr, 
				(char *)Server->h_addr,
				Server->h_length);     
     ServerAddress.sin_port = htons(nPortNo);
     if (SOCKET_ERROR == connect(*pSocket, reinterpret_cast<const struct sockaddr *>(&ServerAddress),sizeof(ServerAddress))) {
		  //TODO
          closesocket(*pSocket);
          return false; 
     }
     
     return true;
}

DWORD 
WINAPI 
WorkerThread(
	LPVOID lpParam
	)
{
	ThreadInfo *pThreadInfo = (ThreadInfo*)lpParam;

	IOCP_FILE_INFO FileInfo;

	int nBytesSent = 0;
	int nBytesRecv = 0;
     
	for (int ii = 0; ii < pThreadInfo->m_nNoOfSends; ii++)
	{
		char chVersion[32];
		memset (chVersion, 0, 32);
		IniFile.GetKeyValue ("QHMUpdate", "version", chVersion, 32);

		//发送版本号到服务端
		memcpy(&(FileInfo.version), chVersion, strlen (chVersion));

		char chPath[MAX_PATH];
		memset (chPath, 0, MAX_PATH);
		GetCurrentPath (chPath);
		//char chClient[MAX_PATH];
		//memset (chClient, 0, MAX_PATH);
		//sprintf (chClient, "%s\\%s", chPath, "QHMClient");
		ScanInstallDir (chPath, &FileInfo);

		nBytesSent = send(pThreadInfo->m_Socket, (char *)&FileInfo, sizeof (IOCP_FILE_INFO), 0);
		if (SOCKET_ERROR == nBytesSent) 
		{
		   //TODO
		   return 1; 
		}

		//接收更新的版本号和更新的文件
		memset ((PVOID)&FileInfo, 0, sizeof (IOCP_FILE_INFO));
		nBytesRecv = recv(pThreadInfo->m_Socket, (char *)&FileInfo, sizeof(IOCP_FILE_INFO), 0);
		if (SOCKET_ERROR == nBytesRecv) 
		{
		   return 1;
		}

		//下载更新文件
		for (int i = 0; i < FileInfo.count; i++) 
		{
			ULONG ulTotalSize = FileInfo.size[i];
			char *szInfo = new char[ulTotalSize];
			memset (szInfo, 0, ulTotalSize);
			
			char chCurrentPath[MAX_PATH];
			memset (chCurrentPath, 0, MAX_PATH);
			GetCurrentPath (chCurrentPath);

			char chUpdatePath[MAX_PATH];
			memset (chUpdatePath, 0, MAX_PATH);
			sprintf (chUpdatePath, "%s\\%s", chCurrentPath, UPDATE);

			WIN32_FIND_DATAA fData;
			HANDLE hHandle = FindFirstFileA (chUpdatePath, &fData);
			if (INVALID_HANDLE_VALUE == hHandle){
				CreateDirectory (chUpdatePath, NULL);
			}
			else {
				FindClose (hHandle);
			}
	
			CreateRelativeDir (chUpdatePath, FileInfo.name[i]);

			//char *chFilePath = FileInfo.name[i];
			//int iLen = strlen (chFilePath); 
			//while(strchr(chFilePath,'\\'))
			//{
			//	 chFilePath = strchr(chFilePath,'\\');
			//	 chFilePath++;
			//}

			char newFile[MAX_PATH];
			memset (newFile, 0, MAX_PATH);
			//sprintf (newFile, "%s\\%d_%s", UPDATE, pThreadInfo->m_nThreadNo, chFilePath);
			sprintf (newFile, "%s\\%s", chUpdatePath, FileInfo.name[i]);
			HANDLE hFile = CreateFile(newFile,GENERIC_WRITE,FILE_SHARE_WRITE,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0); 
			if(hFile != INVALID_HANDLE_VALUE) 
			{ 
				ULONG ulReadSize = 0;
				while (ulReadSize != ulTotalSize)
				{
					ULONG lReadSize = 0;
					int iSize = recv(pThreadInfo->m_Socket,szInfo,(ulTotalSize - ulReadSize),0); 
					if(iSize == SOCKET_ERROR || iSize == 0) { 
						CloseHandle(hFile); 
						break;   
					} 

					//写入文件 
					ulReadSize += iSize;
					WriteFile(hFile,szInfo,iSize,&lReadSize,NULL); 
				}
			}
			SetFileTime (hFile, &(FileInfo.data[i].ftCreationTime), &(FileInfo.data[i].ftLastAccessTime), &(FileInfo.data[i].ftLastWriteTime));
			SetFileAttributes (newFile, FileInfo.data[i].dwFileAttributes);
			CloseHandle (hFile);
			delete []szInfo;
		}

		//更新配置文件
		IniFile.SetKeyValue ("QHMUpdate", "version", FileInfo.version);

		//更新文件替换本地文件
		char chCurrentPath[MAX_PATH];
		memset (chCurrentPath, 0, MAX_PATH);
		GetCurrentPath (chCurrentPath);

		char chUpdatePath[MAX_PATH];
		memset (chUpdatePath, 0, MAX_PATH);
		sprintf (chUpdatePath, "%s\\%s", chCurrentPath, UPDATE);

		CopyDirectory (chUpdatePath, chCurrentPath);

		DeleteDirectory (chUpdatePath);

		RemoveDirectory (chUpdatePath);
	}
	
	return 0; 
}

DWORD 
ClientWaitForMultipleObjects( 
	DWORD nCount, 
	const HANDLE* lpHandles, 
	BOOL bWaitAll, 
	DWORD dwMilliseconds 
	) 
{ 
	DWORD dwResult = WAIT_TIMEOUT; // 存储等待结果 
	DWORD dwSignaled = 0; // 记录信号量通过的句柄数 
	DWORD dwTickCnt = GetTickCount(); // 累计超时 
	PHANDLE lpInner = (PHANDLE)VirtualAlloc( 0, // 分柄句柄内存 
		nCount * sizeof(HANDLE), MEM_COMMIT, PAGE_READWRITE ); 
	if ( lpInner == NULL ) // 检验内存分配失败 
	{ 
		dwResult = WAIT_FAILED; 
	} 
	else 
	{ 
		CopyMemory( lpInner, lpHandles, nCount * sizeof(HANDLE) ); // 复制句柄 
		for ( int dwIdx = 0; dwSignaled != nCount; ++dwIdx ) 
		{ // 循环等待所有的信号量 
			if ( dwIdx == nCount ) 
			{ // 如果访问到末尾，则从头一个重新开始 
				dwIdx = 0; 
			} 
			if ( lpInner[dwIdx] ) 
			{ // 等待当前的信号 
				switch( WaitForSingleObject( lpInner[dwIdx], 0 ) ) 
				{ 
				case WAIT_ABANDONED: 
					dwResult = WAIT_ABANDONED_0 + dwIdx; 
					dwSignaled = nCount; 
					break; 
				case WAIT_OBJECT_0: 
					lpInner[dwIdx] = NULL; 
					dwSignaled = bWaitAll ? dwSignaled + 1 : nCount; 
					dwResult = WAIT_OBJECT_0 + dwIdx; 
					break; 
				case WAIT_TIMEOUT: 
					break; 
				case WAIT_FAILED: 
				default: 
					dwResult = WAIT_FAILED; 
					dwSignaled = nCount; 
					break; 
				} 
			} 
			if ( GetTickCount() - dwTickCnt > dwMilliseconds ) 
			{ 
				break; 
			} 
			Sleep( 1 ); // 空闲，以减少CPU使用 
		} 
	} 
	VirtualFree( lpInner, 0, MEM_RELEASE ); 
	return dwResult; 
} 

void 
StartUpdate ()
{
     WSADATA wsaData;     
     int nResult = WSAStartup(MAKEWORD(2,2), &wsaData);
     if (NO_ERROR != nResult){
         return;
     }
     
     InitializeCriticalSection(&g_csConsole);
     
     int nNoOfThreads = 0;
     int nNoOfSends = 0;
           
	 nNoOfThreads = 1;
     nNoOfSends = 1;
     
     HANDLE *p_hThreads = new HANDLE[nNoOfThreads];
     ThreadInfo *pThreadInfo = new ThreadInfo[nNoOfThreads];
	 if (NULL == pThreadInfo){
		 //TODO
	 }

	//检查更新路径是否存在
	WIN32_FIND_DATA fData;
	HANDLE hHandle = FindFirstFile ("update", &fData);
	if (INVALID_HANDLE_VALUE == hHandle){
		BOOL bRet = CreateDirectory (UPDATE, NULL);
		if (!bRet)
		{
			OutputDebugPrintf ("CreateDirectory Error = %d.\r\n", GetLastError ());
		}
	}
	FindClose (hHandle);

	//初始化Version.ini文件
	char chPath[MAX_PATH];
	memset (chPath, 0, MAX_PATH);
	GetCurrentPath (chPath);
	char chVerFile[MAX_PATH];
	memset (chVerFile, 0, MAX_PATH);
	sprintf (chVerFile, "%s\\%s", chPath, "version.ini");

	if (FALSE == IniFile.SetPath (chVerFile)){
		OutputDebugPrintf ("IniFile Error.\r\n");
		return;
	}
     
     bool bConnectedSocketCreated = false;     
     DWORD nThreadID;
	 int i = 0;
     for (i; i < nNoOfThreads; i++)
     {
         bConnectedSocketCreated = CreateSocket(&(pThreadInfo[i].m_Socket), ADDR, PORT); 
         if (!bConnectedSocketCreated)
         {
               delete[] p_hThreads;
			   p_hThreads = NULL;
               delete[] pThreadInfo;
			   pThreadInfo = NULL;
               return;
          }
          
          pThreadInfo[i].m_nNoOfSends = nNoOfSends;
          pThreadInfo[i].m_nThreadNo = i+1;
          p_hThreads[i] = CreateThread(0, 0, WorkerThread, (void *)(&pThreadInfo[i]), 0, &nThreadID);
     }
     
	 ClientWaitForMultipleObjects(nNoOfThreads, p_hThreads, TRUE, INFINITE);
     for (i = 0; i < nNoOfThreads; i++)
     {
          closesocket(pThreadInfo[i].m_Socket);
     }

	 delete[] p_hThreads;
     delete[] pThreadInfo;
	 DeleteCriticalSection(&g_csConsole);
     WSACleanup();
}

void 
DeInstance ()
{
	//TODO
}

int 
main(
	int argc, 
	char* argv[]
	)
{
	if (argc > 2){
	   return 0; 
	}

	if (2 == argc){
		char *param = argv[1];
		if (stricmp (param, "-install") == 0)
		{
			Install ("QHMClientUpdate", "QHMClientUpdate");
			return 1;
		}
		else if (stricmp (param, "-uninstall") == 0){
			Uninstall ("QHMClientUpdate");
			return 1;
		}
		else {
			return 0;
		}
	}

	//服务入口
	StartServer ("QHMClientUpdate",
			StartUpdate, 
			DeInstance
			);
}