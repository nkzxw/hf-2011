#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <winsock2.h>

#include "ClientUpdate.h"

inline
void 
OutputDebugPrintf(
	TCHAR *szFormat, ...
	)
{
	EnterCriticalSection(&g_csConsole);

	va_list args;
	va_start(args, szFormat);

	vprintf(szFormat, args );

	va_end(args);

	LeaveCriticalSection(&g_csConsole);
}

int 
main(
	int argc, 
	char* argv[]
	)
{
     WSADATA wsaData;     
     int nResult = WSAStartup(MAKEWORD(2,2), &wsaData);
     if (NO_ERROR != nResult){
          return 1;
     }
     
     InitializeCriticalSection(&g_csConsole);
     
     char szBuffer[MAX_BUFFER_LEN];
     int nNoOfThreads = 0;
     int nNoOfSends = 0;
     
     strcpy(szBuffer, "科技型中小企业技术创新基金项目----申请资料（XML数据库）");
      
	 nNoOfThreads = 1;
     nNoOfSends = 1;
     
     HANDLE *p_hThreads = new HANDLE[nNoOfThreads];
     ThreadInfo *pThreadInfo = new ThreadInfo[nNoOfThreads];
	 if (NULL == pThreadInfo){
		 //TODO
	 }
     
     bool bConnectedSocketCreated = false;     
     DWORD nThreadID;
	 int ii = 0;
     for (ii; ii < nNoOfThreads; ii++)
     {
         bConnectedSocketCreated = CreateSocket(&(pThreadInfo[ii].m_Socket), ADDR, PORT); 
         if (!bConnectedSocketCreated)
         {
               delete[] p_hThreads;
			   p_hThreads = NULL;
               delete[] pThreadInfo;
			   pThreadInfo = NULL;
               return 1;
          }
          
          pThreadInfo[ii].m_nNoOfSends = nNoOfSends;
          pThreadInfo[ii].m_nThreadNo = ii+1;
          sprintf(pThreadInfo[ii].m_szBuffer, "Thread %d - %s", ii+1, szBuffer);
          p_hThreads[ii] = CreateThread(0, 0, WorkerThread, (void *)(&pThreadInfo[ii]), 0, &nThreadID);
     }
     
	 ClientWaitForMultipleObjects(nNoOfThreads, p_hThreads, TRUE, INFINITE);
     for (ii = 0; ii < nNoOfThreads; ii++)
     {
          closesocket(pThreadInfo[ii].m_Socket);
     }

	 delete[] p_hThreads;
     delete[] pThreadInfo;
	 DeleteCriticalSection(&g_csConsole);
     WSACleanup();
     return 0;
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
     char szConsole[MAX_BUFFER_LEN];

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

	char szConsole[MAX_BUFFER_LEN];
	char szTemp[MAX_BUFFER_LEN];
	IOCP_FILE_INFO FileInfo;

	int nBytesSent = 0;
	int nBytesRecv = 0;
     
	for (int ii = 0; ii < pThreadInfo->m_nNoOfSends; ii++)
	{
		//发送版本号到服务端
		memcpy(&(FileInfo.version), "152.0.1", strlen ("152.0.1"));
		nBytesSent = send(pThreadInfo->m_Socket, FileInfo.version, sizeof (FileInfo.version), 0);

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

		for (int i = 0; i < FileInfo.count; i++) 
		{
			ULONG ulTotalSize = FileInfo.size[i];
			char *szInfo = new char[ulTotalSize];
			memset (szInfo, 0, ulTotalSize);
			
			char *chFilePath = FileInfo.name[i];
			int iLen = strlen (chFilePath); 
			while(strchr(chFilePath,'\\'))
			{
				 chFilePath = strchr(chFilePath,'\\');
				 chFilePath++;
			}

			char newFile[MAX_PATH];
			memset (newFile, 0, MAX_PATH);
			sprintf (newFile, "%d_%s", pThreadInfo->m_nThreadNo, chFilePath);
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
		}
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