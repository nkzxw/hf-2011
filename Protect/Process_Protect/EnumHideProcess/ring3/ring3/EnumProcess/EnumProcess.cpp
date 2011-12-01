#include <Windows.h>
#include <iostream.h>
#include <tchar.h>

#include "psapi.h"
#pragma comment( lib, "psapi.lib" )

bool RaisePrivilege()
{
	HANDLE hToken = NULL;
	bool bRes = OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken );
	if( !bRes )
	{
		cout<<"OpenProcessToken"<<endl;
		return false;
	}
	TOKEN_PRIVILEGES tps = {0};
	LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &tps.Privileges[0].Luid );
	tps.PrivilegeCount = 1;
	tps.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	bRes = AdjustTokenPrivileges( hToken, false, &tps, sizeof(tps), NULL, NULL );
	if( bRes == 0 )
	{
		cout<<"AdjustTokenPrivileges false"<<endl;
		return false;
	}

	CloseHandle( hToken );
	return true;
}

void GetProcessPathById( DWORD PId )
{
	TCHAR szProcessName[MAX_PATH] = _T("_Unknow_");
	bool bRes = RaisePrivilege();
	if( bRes )
	{
		HANDLE hProcess = OpenProcess( PROCESS_ALL_ACCESS, false, PId );
		DWORD dw = GetLastError();
		if( hProcess != NULL )
		{
			HMODULE hModule = NULL;
			//DWORD dw = 0;
			bool bGetModule = EnumProcessModules( hProcess, &hModule, sizeof(HMODULE), &dw );
			dw = GetLastError();
			if( bGetModule )
			{
				int len = GetModuleBaseName( hProcess, hModule, szProcessName, sizeof(szProcessName)/sizeof(TCHAR) );
			}
		}
		CloseHandle( hProcess );
	}
	cout<<"PId:"<<PId<<"\t"<<"PathNam:"<<szProcessName<<endl;

}

void main()
{
	DWORD dProcessIds[1024] = {0};
	DWORD dRet = 0;
	DWORD dRes = 0;

	dRes = EnumProcesses( dProcessIds, sizeof(dProcessIds), &dRet );
	if( dRes == 0 )
	{
		cout<<"EnumProcesses1 False"<<endl;
		return;
	}
	int ProcessNums = dRet/sizeof(DWORD);
	
	for( int i = 0; i < ProcessNums; i++ )
		GetProcessPathById( dProcessIds[i] );

	cout<<"Process Nums:"<<ProcessNums<<endl;

}