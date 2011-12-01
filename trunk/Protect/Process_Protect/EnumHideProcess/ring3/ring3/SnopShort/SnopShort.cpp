#include <windows.h>
#include <TLHELP32.H>
#include <iostream.h>

void main()
{
	HANDLE hSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( hSnap == INVALID_HANDLE_VALUE  )
	{
		cout<<"Create Toolhelp false"<<endl;
		return;
	}
	PROCESSENTRY32 pEntry32 = {0};
	pEntry32.dwSize = sizeof(PROCESSENTRY32);
	bool bRes = Process32First( hSnap, &pEntry32 );
	int pNums = 0;
	while( bRes )
	{
		pNums++;
		cout<<"PID:"<<pEntry32.th32ProcessID<<"\t"<<"Path:"<<pEntry32.szExeFile<<endl;
		bRes = Process32Next( hSnap, &pEntry32 );
	}
	CloseHandle( hSnap );
	cout<<"Process Nums:"<<pNums<<endl;


}