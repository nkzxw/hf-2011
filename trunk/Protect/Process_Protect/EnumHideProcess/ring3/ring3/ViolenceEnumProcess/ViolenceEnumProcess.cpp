#include <windows.h>
#include <Psapi.h>
#include <iostream.h>

#pragma comment(lib,"psapi.lib")

void RaisePrivileges()
{
	HANDLE hToken = NULL;
	bool bRes = OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken );
	if( bRes == 0 )
	{
		cout<<"OpenProcessToken false"<<endl;
		return;
	}
	TOKEN_PRIVILEGES tps = {0};
	bRes = LookupPrivilegeValue( NULL, SE_SECURITY_NAME, &tps.Privileges->Luid );
	if( bRes == 0 )
	{
		cout<<"LookupProvilegeValue false"<<endl;
		CloseHandle( hToken );
		return;
	}

	tps.PrivilegeCount = 1;
	tps.Privileges->Attributes = SE_PRIVILEGE_ENABLED;
	bRes = AdjustTokenPrivileges( hToken, false, &tps, sizeof(tps), NULL, NULL );
	if( bRes == 0 )
	{
		cout<<"AdjustTokenPrivileges false"<<endl;
		CloseHandle( hToken );
		return;
	}
	CloseHandle( hToken );
	return;
}


void main()
{
	//提升进程权限
	RaisePrivileges();
	for( int i = 0; i <0xffff; i++ )
	{
		HANDLE hProcess = OpenProcess( PROCESS_ALL_ACCESS, false, i );
		if( hProcess )
		{
			char ProcessName[MAX_PATH] = {0};
			GetProcessImageFileName( hProcess, ProcessName, MAX_PATH );
			cout<<"PID:"<<i<<"\t"<<"Path:"<<ProcessName<<endl;
		}
	}
}