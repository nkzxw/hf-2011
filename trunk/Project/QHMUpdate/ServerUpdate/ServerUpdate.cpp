#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <winsock2.h>
#include <vector>

#include "mswsock.h"

#include "ServerUpdate.h"
#include "..\Common\IniFile.h"

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

inline 
void ScanUpdateDir(
    LPCSTR path,
	PIOCP_FILE_INFO c_FileInfo,
	PIOCP_FILE_INFO s_FileInfo
	)
{
	HANDLE Handle;
	WIN32_FIND_DATAA fData;
	
	Handle = FindFirstFileA(path, &fData);

	if (Handle == INVALID_HANDLE_VALUE)
		return;

	do {
		if (fData.cFileName[0] == '.' )
			continue;

		if (fData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
			char chPath[MAX_PATH];
			memset (chPath, MAX_PATH, 0);
			sprintf (chPath, "%s\\%s", path, fData.cFileName);
			ScanUpdateDir(chPath, c_FileInfo, s_FileInfo);
		}
		else{
			bool bExits = false;
			for (int i = 0; i < c_FileInfo->count; i++)
			{
				if (stricmp (c_FileInfo->name[i], fData.cFileName) == 0)
				{
					//检查路径下的文件是否需要更新
					//比对文件的创建时间
					bExits = true;
				}
			}
			
			//如果服务端存在文件，客户端不存在，则加入到更新结构中。
			if (!bExits)
			{
			}
		}

	}while(FindNextFileA(Handle,&fData));

	if (Handle)
		FindClose(Handle);
}

int 
main(
	int argc, 
	char *argv[]
)
{     
	if (false == Initialize()){
		return 1;
	}
	
	SOCKET ListenSocket; 
	struct sockaddr_in ServerAddress;
	ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);	
	if (INVALID_SOCKET == ListenSocket) {
		goto error;
	}

	ZeroMemory((char *)&ServerAddress, sizeof(ServerAddress));
	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_addr.s_addr = INADDR_ANY; //WinSock will supply address
	ServerAddress.sin_port = htons(PORT);    //comes from commandline
	
	if (SOCKET_ERROR == bind(ListenSocket, (struct sockaddr *) &ServerAddress, sizeof(ServerAddress))) {
		closesocket(ListenSocket);
		goto error;
	}

	if (SOCKET_ERROR == listen(ListenSocket,SOMAXCONN)){
		closesocket(ListenSocket);
		goto error;
	}
	
	g_hAcceptEvent = WSACreateEvent();
	if (WSA_INVALID_EVENT == g_hAcceptEvent){
		goto error;
	}
	
	if (SOCKET_ERROR == WSAEventSelect(ListenSocket, g_hAcceptEvent, FD_ACCEPT)){
		WSACloseEvent(g_hAcceptEvent);
		goto error;
	}

	DWORD nThreadID;
	g_hAcceptThread = CreateThread(0, 0, AcceptThread, (void *)ListenSocket, 0, &nThreadID);
	while(!_kbhit())
	{
		::Sleep(30000);  //switch to some other thread
	}
	
	closesocket(ListenSocket);
	DeInitialize();
	
	return 0; 
	
error:
	closesocket(ListenSocket);
	DeInitialize();
	return 1;
}

bool 
Initialize(
	)
{
	InitializeCriticalSection(&g_csConsole);
	InitializeCriticalSection(&g_csClientList);
	g_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	int nResult;
	WSADATA wsaData;	
	nResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (NO_ERROR != nResult){
		return false;
	}

	g_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0 );	
	if ( NULL == g_hIOCompletionPort){
		return false;
	}
	
	DWORD nThreadID;
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	for (int j = 0; j < SystemInfo.dwNumberOfProcessors * 2; j++)
	{
		g_hWorkerThreads[j] = CreateThread(0, 0, WorkerThread, (void *)(j+1), 0, &nThreadID);
	}
	
	return true;
}

void 
DeInitialize(
	)
{
	SetEvent(g_hShutdownEvent);
	WaitForSingleObject(g_hAcceptThread, INFINITE);
	for (int i = 0; i < MAX_WORKER_THREADS; i++)
	{
		PostQueuedCompletionStatus(g_hIOCompletionPort, 0, (DWORD) NULL, NULL);
	}
	
	WaitForMultipleObjects(MAX_WORKER_THREADS, g_hWorkerThreads, TRUE, INFINITE);
	WSACloseEvent(g_hAcceptEvent);
	CleanClientList();

	DeleteCriticalSection(&g_csConsole);
	DeleteCriticalSection(&g_csClientList);
	CloseHandle(g_hIOCompletionPort);
	CloseHandle(g_hShutdownEvent);
	WSACleanup();
}

DWORD 
WINAPI 
AcceptThread(
	LPVOID lParam
	)
{
	SOCKET ListenSocket = (SOCKET)lParam;
	WSANETWORKEVENTS WSAEvents;
	while(WAIT_OBJECT_0 != WaitForSingleObject(g_hShutdownEvent, 0))
	{
		if (WSA_WAIT_TIMEOUT != WSAWaitForMultipleEvents(1, &g_hAcceptEvent, FALSE, 0, FALSE))
		{
			WSAEnumNetworkEvents(ListenSocket, g_hAcceptEvent, &WSAEvents);
			if ((WSAEvents.lNetworkEvents & FD_ACCEPT) && 
				(0 == WSAEvents.iErrorCode[FD_ACCEPT_BIT]))
			{
				AcceptConnection(ListenSocket);
			}
		}
	}
	
	return 0;
}

void 
AcceptConnection(
	SOCKET ListenSocket
	)
{
	sockaddr_in ClientAddress;
	int nClientLength = sizeof(ClientAddress);
	
	SOCKET Socket = accept(ListenSocket, (sockaddr*)&ClientAddress, &nClientLength);
	if (INVALID_SOCKET == Socket){
		//TODO
	}

	CClientContext  *pClientContext  = new CClientContext;	
	if (NULL == pClientContext){
		//TODO
	}
	pClientContext->SetOpCode(OP_READ);
	pClientContext->SetSocket(Socket);
	AddToClientList(pClientContext);
	

	HANDLE hPort = CreateIoCompletionPort((HANDLE)pClientContext->GetSocket(), 
							   g_hIOCompletionPort, 
							   (DWORD)pClientContext, 
							   0);	
	if (NULL == hPort)
	{
		RemoveFromClientList(pClientContext);		
		return;
	}
	
	pClientContext->SetOpCode(OP_WRITE);

	DWORD dwFlags = 0;
	DWORD dwBytes = 0;		
	int nBytesRecv = WSARecv(pClientContext->GetSocket(), &pClientContext->m_wbuf, 1, 
		&dwBytes, &dwFlags, &pClientContext->m_ol, NULL);
	if ((SOCKET_ERROR == nBytesRecv) && 
		(WSA_IO_PENDING != WSAGetLastError()))
	{
		//TODO
	}
}

DWORD 
WINAPI 
WorkerThread(
	LPVOID lpParam
	)
{    
	int nThreadNo = (int)lpParam;

	void	*lpContext = NULL;
	OVERLAPPED	*pOverlapped = NULL;
	CClientContext	*pClientContext = NULL;
	DWORD	dwBytesTransfered = 0;
	int nBytesRecv = 0;
	int nBytesSent = 0;
	DWORD	dwBytes = 0;
	DWORD	dwFlags = 0;

	IOCP_FILE_INFO FileInfo;

	while (WAIT_OBJECT_0 != WaitForSingleObject(g_hShutdownEvent, 0))
	{
		BOOL bReturn = GetQueuedCompletionStatus(g_hIOCompletionPort,
									&dwBytesTransfered,
									(PULONG_PTR)&lpContext,
									&pOverlapped,
									INFINITE);
		if (NULL == lpContext)
		{
			//TODO
			break;
		}

		if ((FALSE == bReturn) || 
			((TRUE == bReturn) && (0 == dwBytesTransfered)))
		{
			RemoveFromClientList(pClientContext);
			continue;
		}

		pClientContext = (CClientContext *)lpContext;

		switch (pClientContext->GetOpCode())
		{
		case OP_READ:
			{				
				pClientContext->SetOpCode(OP_WRITE);
				pClientContext->ResetWSABUF();
				
				dwFlags = 0;
				nBytesRecv = WSARecv(pClientContext->GetSocket(), &pClientContext->m_wbuf, 1, 
					&dwBytes, &dwFlags, &pClientContext->m_ol, NULL);				
				if ((SOCKET_ERROR == nBytesRecv) && 
					(WSA_IO_PENDING != WSAGetLastError()))
				{
					//TODO
					RemoveFromClientList(pClientContext);
				}
			}
			break;
			
		case OP_WRITE:
			{
				char szBuffer[MAX_BUFFER_LEN];
				pClientContext->GetBuffer(szBuffer);
			
				pClientContext->SetOpCode(OP_READ);		
				pClientContext->m_nTotalBytes = dwBytesTransfered;
				pClientContext->m_nSentBytes  = 0;
				pClientContext->m_wbuf.len  = dwBytesTransfered;
				
				dwFlags = 0;


				//检测客户端版本号,验证更新的文件

				//发送更新文件信息
				FileInfo.count = 1;
				memcpy (FileInfo.version, "153.0.0", strlen("153.0.0"));
				for (int i = 0; i < FileInfo.count; i++) 
				{
					memcpy(FileInfo.name[i], "E:\\IFS.c", strlen("E:\\IFS.c"));
					HANDLE hFile = CreateFile(FileInfo.name[i],GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
					if (hFile != INVALID_HANDLE_VALUE) 
					{
						DWORD dwFileSize = GetFileSize (hFile, NULL);
						FileInfo.size[i] = dwFileSize;
						CloseHandle (hFile);
					}
				}

				DWORD dwReadSize;
				WSABUF wsFileInfo;
				wsFileInfo.len = sizeof (IOCP_FILE_INFO);
				wsFileInfo.buf = new char[sizeof(IOCP_FILE_INFO)];
				memcpy (wsFileInfo.buf, &FileInfo, sizeof (IOCP_FILE_INFO));

				nBytesSent = WSASend(pClientContext->GetSocket(), &wsFileInfo, 1, 
					&dwReadSize, dwFlags, &pClientContext->m_ol, NULL);
				
				if ((SOCKET_ERROR == nBytesSent) && (WSA_IO_PENDING != WSAGetLastError()))
				{
					RemoveFromClientList(pClientContext);
				}

				//发送更新文件
				for (int i = 0; i < FileInfo.count; i++) 
				{
					HANDLE hFile = CreateFile(FileInfo.name[i],GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
					GetFileAttributesExA (FileInfo.name[i], GetFileExInfoStandard, &FileInfo.data[i]);
					if (hFile != INVALID_HANDLE_VALUE) 
					{
						WSABUF wsFile;
						wsFile.len = FileInfo.size[i];
						wsFile.buf = new char[FileInfo.size[i]];
						
						ReadFile (hFile, wsFile.buf, FileInfo.size[i], &dwReadSize, 0);
						CloseHandle (hFile);

						nBytesSent = WSASend(pClientContext->GetSocket(), &wsFile, 1, 
							&dwReadSize, dwFlags, &pClientContext->m_ol, NULL);
						
						if ((SOCKET_ERROR == nBytesSent) && (WSA_IO_PENDING != WSAGetLastError()))
						{
							RemoveFromClientList(pClientContext);
						}
					}
				}
			
			}
			break;
			
		default:
			break;
		} // switch
	} // while

	return 0;
}

void 
AddToClientList(
	CClientContext   *pClientContext
	)
{
	EnterCriticalSection(&g_csClientList);
	g_ClientContext.push_back(pClientContext);	
	LeaveCriticalSection(&g_csClientList);
}

void 
RemoveFromClientList(
	CClientContext   *pClientContext
	)
{
	EnterCriticalSection(&g_csClientList);

	std::vector <CClientContext *>::iterator IterClientContext;
	for (IterClientContext = g_ClientContext.begin(); IterClientContext != g_ClientContext.end(); IterClientContext++)
	{
		if (pClientContext == *IterClientContext)
		{
			g_ClientContext.erase(IterClientContext);
			CancelIo((HANDLE)(pClientContext->GetSocket()));
			closesocket(pClientContext->GetSocket());
			delete pClientContext;
			break;
		}
	}
	
	LeaveCriticalSection(&g_csClientList);
}

void CleanClientList(
	)
{
	EnterCriticalSection(&g_csClientList);

	std::vector <CClientContext *>::iterator IterClientContext;
	for (IterClientContext = g_ClientContext.begin(); IterClientContext != g_ClientContext.end( ); IterClientContext++)
	{
		CancelIo((HANDLE)(*IterClientContext)->GetSocket());
		closesocket((*IterClientContext)->GetSocket());
		delete *IterClientContext;
	}
	
	g_ClientContext.clear();
	
	LeaveCriticalSection(&g_csClientList);
}