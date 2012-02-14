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

CIniFile IniFile;

inline 
void ScanUpdateDir(
    LPCSTR path,
	PIOCP_FILE_INFO c_FileInfo,
	PIOCP_FILE_INFO s_FileInfo,
	LPCSTR relativePath)
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
			char newPath[MAX_PATH];
			memset (newPath, MAX_PATH, 0);
			sprintf (newPath, "%s\\%s", path, fData.cFileName);

			char chRelativePath[MAX_PATH];
			memset (chRelativePath, 0, MAX_PATH);
			if (NULL == relativePath) {
				sprintf (chRelativePath, "%s", fData.cFileName);
			}
			else {
				sprintf (chRelativePath, "%s\\%s", relativePath, fData.cFileName);
			}
			ScanUpdateDir(newPath, c_FileInfo, s_FileInfo, chRelativePath);
		}
		else{
			bool bExits = false;
			for (int i = 0; i < c_FileInfo->count; i++)
			{
				if (stricmp (c_FileInfo->name[i], fData.cFileName) == 0)
				{
					bExits = true;

					//检查路径下的文件是否需要更新
					//比对文件的创建时间
					char chFile[MAX_PATH];
					memset (chFile, 0, MAX_PATH);
					sprintf (chFile, "%s\\%s", path, fData.cFileName);
					HANDLE hFile = CreateFile(chFile,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
					if (INVALID_HANDLE_VALUE != hFile){
						WIN32_FILE_ATTRIBUTE_DATA data; 
						GetFileAttributesExA (chFile, GetFileExInfoStandard, &data);
						if (c_FileInfo->data[i].ftLastWriteTime.dwHighDateTime == data.ftLastWriteTime.dwHighDateTime &&
							c_FileInfo->data[i].ftLastWriteTime.dwLowDateTime == data.ftLastWriteTime.dwLowDateTime)
						{
							CloseHandle (hFile);
							break;
						}
						else {
							DWORD dwFileSize = GetFileSize (hFile, NULL);
							s_FileInfo->size[i] = dwFileSize;
							
							//记录更新文件的相对根目录的相对路径
							memset (chFile, 0, MAX_PATH);
							if (NULL == relativePath) {
								sprintf (chFile, "%s", fData.cFileName);
							}
							else {
								sprintf (chFile, "%s\\%s", relativePath, fData.cFileName);
							}
							memcpy(s_FileInfo->name[s_FileInfo->count], chFile, strlen (chFile));
							s_FileInfo->count++;
						}
						CloseHandle (hFile);
					}
				}
			}
			
			//如果服务端存在文件，客户端不存在，则加入到更新结构中。
			if (!bExits)
			{
				char chFile[MAX_PATH];
				memset (chFile, 0, MAX_PATH);
				sprintf (chFile, "%s\\%s", path, fData.cFileName);
				HANDLE hFile = CreateFile(chFile,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
				if (INVALID_HANDLE_VALUE != hFile){
					GetFileAttributesExA (chFile, GetFileExInfoStandard, &s_FileInfo->data[s_FileInfo->count]);
					DWORD dwFileSize = GetFileSize (hFile, NULL);
					s_FileInfo->size[s_FileInfo->count] = dwFileSize;

					//记录更新文件的相对根目录的相对路径
					memset (chFile, 0, MAX_PATH);
					if (NULL == relativePath) {
						sprintf (chFile, "%s", fData.cFileName);
					}
					else {
						sprintf (chFile, "%s\\%s", relativePath, fData.cFileName);
					}
					memcpy(s_FileInfo->name[s_FileInfo->count], chFile, strlen (chFile));
					s_FileInfo->count++;

					CloseHandle (hFile);
				}
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
	//InitContextNode ();

	if (false == Initialize()){
		return 1;
	}

	char chPath[MAX_PATH];
	memset (chPath, 0, MAX_PATH);
	GetCurrentPath (chPath);
	char chVerFile[MAX_PATH];
	memset (chVerFile, 0, MAX_PATH);
	sprintf (chVerFile, "%s\%s", chPath, "update.ini");

	if (FALSE == IniFile.SetPath (chVerFile)){
		OutputDebugPrintf ("IniFile Error.\r\n");
		return 0;
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
		::Sleep(100);  //switch to some other thread
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

		pClientContext = (CClientContext *)lpContext;

		if ((FALSE == bReturn) || 
			((TRUE == bReturn) && (0 == dwBytesTransfered)))
		{
			RemoveFromClientList(pClientContext);
			continue;
		}

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
				IOCP_FILE_INFO NewFileInfo;
				pClientContext->GetBuffer((char *)&NewFileInfo);
			
				pClientContext->SetOpCode(OP_READ);		
				pClientContext->m_nTotalBytes = dwBytesTransfered;
				pClientContext->m_nSentBytes  = 0;
				pClientContext->m_wbuf.len  = dwBytesTransfered;
				
				dwFlags = 0;

				//检测客户端版本号,验证更新的文件
				char chVersion[32];
				memset (chVersion, 0, 32);
				memcpy (chVersion, NewFileInfo.version, 32);
				trim(chVersion, '.');
				int cVersion = atoi (chVersion);
				if (0 == cVersion)
					continue;

				char chPath[MAX_PATH];
				memset (chPath, 0, MAX_PATH);
				GetCurrentPath (chPath);
				char chUpdate[MAX_PATH];
				memset (chUpdate, 0, MAX_PATH);
				sprintf (chUpdate, "%s\\%s", chPath, "QHMUpdate");
				
				IniFile.GetKeyValue ("QHMUpdate", "version", FileInfo.version, 32);
				memset (chVersion, 0, 32);
				memcpy (chVersion, FileInfo.version, 32);
				trim(chVersion, '.');
				int sVersion = atoi(chVersion);
				
				if (sVersion > cVersion){		
					//发送更新文件信息
					ScanUpdateDir (chUpdate, &NewFileInfo, &FileInfo, NULL);

				}

				DWORD dwReadSize;
				WSABUF wsFileInfo;
				wsFileInfo.len = sizeof (IOCP_FILE_INFO);
				wsFileInfo.buf = new char[sizeof(IOCP_FILE_INFO)];
				memcpy (wsFileInfo.buf, &FileInfo, sizeof (IOCP_FILE_INFO));

				nBytesSent = WSASend(pClientContext->GetSocket(), &wsFileInfo, 1, 
					&dwReadSize, dwFlags, &pClientContext->m_ol, NULL);
				
				delete []wsFileInfo.buf;

				if ((SOCKET_ERROR == nBytesSent) && (WSA_IO_PENDING != WSAGetLastError()))
				{
					RemoveFromClientList(pClientContext);
				}

				//发送更新文件
				for (int i = 0; i < FileInfo.count; i++) 
				{
					char chTmpFile[MAX_PATH];
					memset (chTmpFile, 0, MAX_PATH);
					sprintf (chTmpFile, "%s\\%s", chUpdate, FileInfo.name[i]);
					HANDLE hFile = CreateFile(chTmpFile,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
					GetFileAttributesExA (chTmpFile, GetFileExInfoStandard, &FileInfo.data[i]);
					if (hFile != INVALID_HANDLE_VALUE) 
					{
						WSABUF wsFile;
						wsFile.len = FileInfo.size[i];
						wsFile.buf = new char[FileInfo.size[i]];
						
						ReadFile (hFile, wsFile.buf, FileInfo.size[i], &dwReadSize, 0);
						CloseHandle (hFile);

						nBytesSent = WSASend(pClientContext->GetSocket(), &wsFile, 1, 
							&dwReadSize, dwFlags, &pClientContext->m_ol, NULL);
						
						delete []wsFile.buf;

						if ((SOCKET_ERROR == nBytesSent) && (WSA_IO_PENDING != WSAGetLastError()))
						{
							RemoveFromClientList(pClientContext);
						}
					}
				}

				//更新配置文件
			
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
	
	//PCONTEXT_LINK_NODE newNode = AllocContextNode (pClientContext);
	//InsertContextNode (newNode);
	
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
			CancelIo((HANDLE)(pClientContext->GetSocket()));
			closesocket(pClientContext->GetSocket());

			g_ClientContext.erase(IterClientContext);
			delete pClientContext;
			pClientContext = NULL;
			break;
		}
	}

	//RemoveContextNode (pClientContext);
	
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

	//ClearContextNode ();
	
	LeaveCriticalSection(&g_csClientList);
}