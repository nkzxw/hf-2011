// QueueApc.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"


#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <TlHelp32.h>

#include <iostream>
#include <string>
using namespace std;

#define DEF_BUF_SIZE 1024

// ���ڴ洢ע��ģ��DLL��·��ȫ��
char szDllPath[DEF_BUF_SIZE] = {0} ;

// ʹ��APC������ָ��ID�Ľ���ע��ģ��
BOOL InjectModuleToProcessById ( DWORD dwProcessId )
{
	DWORD	dwRet = 0 ;
	BOOL	bStatus = FALSE ;
	LPVOID	lpData = NULL ;
	UINT	uLen = strlen(szDllPath) + 1;
	// ��Ŀ�����
	HANDLE hProcess = OpenProcess ( PROCESS_ALL_ACCESS, FALSE, dwProcessId ) ;
	if ( hProcess )
	{
		// ����ռ�
		lpData = VirtualAllocEx ( hProcess, NULL, uLen, MEM_COMMIT, PAGE_EXECUTE_READWRITE ) ;
		if ( lpData )
		{
			// д����Ҫע���ģ��·��ȫ��
			bStatus = WriteProcessMemory ( hProcess, lpData, szDllPath, uLen, &dwRet ) ;
		}
		CloseHandle ( hProcess ) ;
	}

	if ( bStatus == FALSE )
		return FALSE ;

	// �����߳̿���
	THREADENTRY32 te32 = { sizeof(THREADENTRY32) } ;
	HANDLE hThreadSnap = CreateToolhelp32Snapshot ( TH32CS_SNAPTHREAD, 0 ) ;
	if ( hThreadSnap == INVALID_HANDLE_VALUE ) 
		return FALSE ; 

	bStatus = FALSE ;
	// ö�������߳�
	if ( Thread32First ( hThreadSnap, &te32 ) )
	{
		do{
			// �ж��Ƿ�Ŀ������е��߳�
			if ( te32.th32OwnerProcessID == dwProcessId )
			{
				// ���߳�
				HANDLE hThread = OpenThread ( THREAD_ALL_ACCESS, FALSE, te32.th32ThreadID ) ;
				if ( hThread )
				{
					// ��ָ���߳����APC
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
	// ȡ�õ�ǰ����Ŀ¼·��
	GetCurrentDirectoryA ( DEF_BUF_SIZE, szDllPath ) ;

	// ����ע��ģ��DLL��·��ȫ��
	strcat ( szDllPath, "\\DLLSample.dll" ) ;

	DWORD dwProcessId = 0 ;
	// �����û������Ŀ�����ID
	while ( cout << "������Ŀ�����ID��" && cin >> dwProcessId && dwProcessId > 0 ) 
	{
		BOOL bRet = InjectModuleToProcessById ( dwProcessId ) ;
		cout << (bRet ? "ע��ɹ���":"ע��ʧ�ܣ�") << endl ;
	}
	return 0;
}
