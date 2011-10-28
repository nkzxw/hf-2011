// QueueApc.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <TlHelp32.h>

#include <iostream>
#include <string>
using namespace std;

#define DEF_BUF_SIZE 1024

// 用于存储注入模块DLL的路径全名
char szDllPath[DEF_BUF_SIZE] = {0} ;

// 使用APC机制向指定ID的进程注入模块
BOOL InjectModuleToProcessById ( DWORD dwProcessId )
{
	DWORD	dwRet = 0 ;
	BOOL	bStatus = FALSE ;
	LPVOID	lpData = NULL ;
	UINT	uLen = strlen(szDllPath) + 1;
	// 打开目标进程
	HANDLE hProcess = OpenProcess ( PROCESS_ALL_ACCESS, FALSE, dwProcessId ) ;
	if ( hProcess )
	{
		// 分配空间
		lpData = VirtualAllocEx ( hProcess, NULL, uLen, MEM_COMMIT, PAGE_EXECUTE_READWRITE ) ;
		if ( lpData )
		{
			// 写入需要注入的模块路径全名
			bStatus = WriteProcessMemory ( hProcess, lpData, szDllPath, uLen, &dwRet ) ;
		}
		CloseHandle ( hProcess ) ;
	}

	if ( bStatus == FALSE )
		return FALSE ;

	// 创建线程快照
	THREADENTRY32 te32 = { sizeof(THREADENTRY32) } ;
	HANDLE hThreadSnap = CreateToolhelp32Snapshot ( TH32CS_SNAPTHREAD, 0 ) ;
	if ( hThreadSnap == INVALID_HANDLE_VALUE ) 
		return FALSE ; 

	bStatus = FALSE ;
	// 枚举所有线程
	if ( Thread32First ( hThreadSnap, &te32 ) )
	{
		do{
			// 判断是否目标进程中的线程
			if ( te32.th32OwnerProcessID == dwProcessId )
			{
				// 打开线程
				HANDLE hThread = OpenThread ( THREAD_ALL_ACCESS, FALSE, te32.th32ThreadID ) ;
				if ( hThread )
				{
					// 向指定线程添加APC
					DWORD dwRet = QueueUserAPC ( (PAPCFUNC)LoadLibraryA, hThread, (ULONG_PTR)lpData ) ;
					if ( dwRet > 0 )
						bStatus = TRUE ;
					CloseHandle ( hThread ) ;
				}
			} 

		}while ( Thread32Next ( hThreadSnap, &te32 ) ) ;
	}

	CloseHandle ( hThreadSnap ) ;
	return bStatus;
}

int _tmain(int argc, _TCHAR* argv[])
{
	// 取得当前工作目录路径
	GetCurrentDirectoryA ( DEF_BUF_SIZE, szDllPath ) ;

	// 生成注入模块DLL的路径全名
	strcat ( szDllPath, "\\DLLSample.dll" ) ;

	DWORD dwProcessId = 0 ;
	// 接收用户输入的目标进程ID
	while ( cout << "请输入目标进程ID：" && cin >> dwProcessId && dwProcessId > 0 ) 
	{
		BOOL bRet = InjectModuleToProcessById ( dwProcessId ) ;
		cout << (bRet ? "注入成功！":"注入失败！") << endl ;
	}
	return 0;
}
