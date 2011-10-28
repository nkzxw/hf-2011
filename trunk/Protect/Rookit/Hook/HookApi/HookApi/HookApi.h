#ifndef _HOOKAPI_H
#define _HOOKAPI_H


class CHOOKAPI {
public:
	LPVOID	pOldFunEntry, pNewFunEntry ;	// 初始函数地址、HOOK后的函数地址
	BYTE	bOldByte[5], bNewByte[5] ;		// 原始字节、目标字节

public:
	CHOOKAPI () {}
	~CHOOKAPI() {}
	// 实现HOOK API
	void Hook ( PSTR szModuleName, PSTR szFunName, FARPROC pFun )
	{	
		HMODULE	hMod = ::GetModuleHandleA ( szModuleName ) ;
		if ( hMod != NULL )
		{
			pNewFunEntry	= (LPVOID)pFun ;
			pOldFunEntry	= (LPVOID)GetProcAddress ( hMod, szFunName ) ;
			bNewByte[0]		= 0xE9 ;
			*((PDWORD)(&(bNewByte[1])))	= (DWORD)pNewFunEntry - (DWORD)pOldFunEntry - 5 ; 

			DWORD   dwProtect, dwWriteByte, dwReadByte ; 
			VirtualProtect ( (LPVOID)pOldFunEntry, 5, PAGE_READWRITE, &dwProtect );
			ReadProcessMemory	( GetCurrentProcess(), (LPVOID)pOldFunEntry, bOldByte, 5, &dwReadByte ) ;		
			WriteProcessMemory	( GetCurrentProcess(), (LPVOID)pOldFunEntry, bNewByte, 5, &dwWriteByte ) ;
			VirtualProtect ( (LPVOID)pOldFunEntry, 5, dwProtect, NULL ) ;
		}
	}
	// 重新HOOK
	void ReHook ()
	{
		DWORD	dwProtect, dwWriteByte ;
		VirtualProtect ( pOldFunEntry, 5, PAGE_READWRITE, &dwProtect );
		WriteProcessMemory ( GetCurrentProcess(), pOldFunEntry, bNewByte, 5, &dwWriteByte ) ;
		VirtualProtect ( pOldFunEntry, 5, dwProtect, NULL ) ;
	}
	// 撤消HOOK
	void UnHook ()
	{
		DWORD	dwProtect, dwWriteByte ;
		VirtualProtect ( pOldFunEntry, 5, PAGE_READWRITE, &dwProtect );
		WriteProcessMemory ( GetCurrentProcess(), pOldFunEntry, bOldByte, 5, &dwWriteByte ) ;
		VirtualProtect ( pOldFunEntry, 5, dwProtect, NULL ) ;
	}
} ;

#endif