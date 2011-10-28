// RemoteThread.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <windows.h>
#include <string>
#include "stdio.h"
#include <iostream>
using namespace std;

#define DEF_BUF_SIZE 1024

// 用于存储注入模块DLL的路径全名
char szDllPath[DEF_BUF_SIZE] = {0} ;

// 使用远程线程向指定ID的进程注入模块
BOOL InjectModuleToProcessById ( DWORD dwProcessId )
{
	if ( dwProcessId == 0 )
		return FALSE ;

	// 打开进程
	HANDLE hProcess = OpenProcess ( PROCESS_ALL_ACCESS, FALSE, dwProcessId ) ;
	if ( hProcess == NULL )
		return FALSE ;

	//申请存放文件名的空间
	UINT	nLen = (UINT)strlen ( szDllPath ) + 1;
	LPVOID	lpRemoteDllName = VirtualAllocEx ( hProcess, NULL, nLen, MEM_COMMIT, PAGE_READWRITE ) ;
	if ( lpRemoteDllName == NULL )
	{
		printf ( "[ERROR]VirtualAllocEx(%d)\n", GetLastError() ); 
		return FALSE ;
	}

	//把dll文件名写入申请的空间
	if ( WriteProcessMemory ( hProcess, lpRemoteDllName, szDllPath, nLen, NULL) == FALSE )
	{
		printf ( "[ERROR]WriteProcessMemory(%d)\n", GetLastError() ); 
		return FALSE ;
	}

	//获取动态链接库函数地址
	HMODULE hModule = GetModuleHandle ( L"kernel32.dll" ) ;
	LPTHREAD_START_ROUTINE fnStartAddr = ( LPTHREAD_START_ROUTINE )GetProcAddress(hModule,"LoadLibraryA") ;
	if ( (DWORD)fnStartAddr == 0 )
	{
		printf ( "[ERROR]GetProcAddress(%d)\n", GetLastError() ); 
		return FALSE ;
	}

	//创建远程线程
	HANDLE hRemoteThread = CreateRemoteThread ( hProcess, NULL, 0,fnStartAddr, lpRemoteDllName, 0, NULL ) ;
	if ( hRemoteThread == NULL )
	{
		printf ( "[ERROR]CreateRemoteThread(%d)\n", GetLastError() ); 
		return FALSE ;
	}

	// 等待远程线程结束
	if ( WaitForSingleObject ( hRemoteThread, INFINITE ) != WAIT_OBJECT_0 )
	{
		printf ( "[ERROR]WaitForSingleObject(%d)\n", GetLastError() ); 
		return FALSE ;
	}

	CloseHandle ( hRemoteThread ) ;
	CloseHandle ( hModule ) ;
	CloseHandle ( hProcess ) ;
	return TRUE ;
}

int _tmain(int argc, _TCHAR* argv[])
{
	// 取得当前工作目录路径
	GetCurrentDirectoryA ( DEF_BUF_SIZE, szDllPath ) ;

	// 生成注入模块DLL的路径全名
	strcat ( szDllPath, "\\DLLSample.dll" ) ;

	DWORD dwProcessId = 0 ;
	// 接收用户输入的目标进程ID
	while ( printf ( "请输入目标进程ID：" ) && cin >> dwProcessId && dwProcessId > 0 ) 
	{
		BOOL bRet = InjectModuleToProcessById ( dwProcessId ) ;
		printf ( bRet ? "注入成功！\n":"注入失败！\n") ;
	}
	return 0;
}
