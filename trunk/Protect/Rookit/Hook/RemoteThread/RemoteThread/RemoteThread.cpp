// RemoteThread.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

#include <windows.h>
#include <string>
#include "stdio.h"
#include <iostream>
using namespace std;

#define DEF_BUF_SIZE 1024

// ���ڴ洢ע��ģ��DLL��·��ȫ��
char szDllPath[DEF_BUF_SIZE] = {0} ;

// ʹ��Զ���߳���ָ��ID�Ľ���ע��ģ��
BOOL InjectModuleToProcessById ( DWORD dwProcessId )
{
	if ( dwProcessId == 0 )
		return FALSE ;

	// �򿪽���
	HANDLE hProcess = OpenProcess ( PROCESS_ALL_ACCESS, FALSE, dwProcessId ) ;
	if ( hProcess == NULL )
		return FALSE ;

	//�������ļ����Ŀռ�
	UINT	nLen = (UINT)strlen ( szDllPath ) + 1;
	LPVOID	lpRemoteDllName = VirtualAllocEx ( hProcess, NULL, nLen, MEM_COMMIT, PAGE_READWRITE ) ;
	if ( lpRemoteDllName == NULL )
	{
		printf ( "[ERROR]VirtualAllocEx(%d)\n", GetLastError() ); 
		return FALSE ;
	}

	//��dll�ļ���д������Ŀռ�
	if ( WriteProcessMemory ( hProcess, lpRemoteDllName, szDllPath, nLen, NULL) == FALSE )
	{
		printf ( "[ERROR]WriteProcessMemory(%d)\n", GetLastError() ); 
		return FALSE ;
	}

	//��ȡ��̬���ӿ⺯����ַ
	HMODULE hModule = GetModuleHandle ( L"kernel32.dll" ) ;
	LPTHREAD_START_ROUTINE fnStartAddr = ( LPTHREAD_START_ROUTINE )GetProcAddress(hModule,"LoadLibraryA") ;
	if ( (DWORD)fnStartAddr == 0 )
	{
		printf ( "[ERROR]GetProcAddress(%d)\n", GetLastError() ); 
		return FALSE ;
	}

	//����Զ���߳�
	HANDLE hRemoteThread = CreateRemoteThread ( hProcess, NULL, 0,fnStartAddr, lpRemoteDllName, 0, NULL ) ;
	if ( hRemoteThread == NULL )
	{
		printf ( "[ERROR]CreateRemoteThread(%d)\n", GetLastError() ); 
		return FALSE ;
	}

	// �ȴ�Զ���߳̽���
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
	// ȡ�õ�ǰ����Ŀ¼·��
	GetCurrentDirectoryA ( DEF_BUF_SIZE, szDllPath ) ;

	// ����ע��ģ��DLL��·��ȫ��
	strcat ( szDllPath, "\\DLLSample.dll" ) ;

	DWORD dwProcessId = 0 ;
	// �����û������Ŀ�����ID
	while ( printf ( "������Ŀ�����ID��" ) && cin >> dwProcessId && dwProcessId > 0 ) 
	{
		BOOL bRet = InjectModuleToProcessById ( dwProcessId ) ;
		printf ( bRet ? "ע��ɹ���\n":"ע��ʧ�ܣ�\n") ;
	}
	return 0;
}
