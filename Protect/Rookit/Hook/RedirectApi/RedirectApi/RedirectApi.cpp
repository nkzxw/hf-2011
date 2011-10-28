
#include <windows.h>

// 定义API挂接项结构
typedef struct _HOOK_ITEM {
	DWORD	dwAddr ;			// IAT项所在地址
	DWORD	dwOldValue ;		// IAT项的原始函数地址
	DWORD	dwNewValue ;		// IAT项的新函数地址
} HOOK_ITEM, *PHOOK_ITEM ;
HOOK_ITEM	HookItem = {0} ;	// 定义IAT项，用于保存MessageBoxA的IAT项信息

// 定义MessageBoxA函数原型
typedef int (WINAPI* PFNMessageBoxA)( HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType ) ;

// 定义重定向API的实现函数
BOOL WINAPI RedirectApi ( PCHAR pDllName, PCHAR pFunName, DWORD dwNewProc, PHOOK_ITEM pItem ) ;

// 自定义的MessageBoxA函数
// 实现对原始MessageBoxA的输入、输出参数的监控，甚至是取消调用
int WINAPI NEW_MessageBoxA( HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType )
{
	// 此处可以观察/修改调用参数，甚至可以取消调用直接返回。
	// ……

	// 取得原函数地址
	PFNMessageBoxA pfnMessageBoxA = (PFNMessageBoxA)HookItem.dwOldValue ;

	// 输出测试信息，
	// 如果这里直接调用MessageBoxA，就进入无限循环
	pfnMessageBoxA ( hWnd, "这是API重定向过程的消息框", "测试", 0 ) ;

	// 调用原函数
	int ret = pfnMessageBoxA ( hWnd, lpText, lpCaption, uType ) ;

	// 此处可以查看/修改调用原函数的返回值
	// ……

	return ret ;
}

int WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	// 重定向API
	if ( !RedirectApi ( "USER32.dll", "MessageBoxA", (DWORD)NEW_MessageBoxA, &HookItem ) )
		OutputDebugStringA ( "RedirectApi failed!" ) ;
	else
		OutputDebugStringA ( "RedirectApi success!" ) ;
	
	MessageBoxA ( 0, "正常消息框", "测试", 0 ) ;
	return 0 ;
}

// 实现重定向API
// 参数pDllName:目标API所在的DLL名称
// 参数pFunName:目标API名称
// 参数dwNewProc:自定义的函数地址
// 参数pItem:用于保存IAT项信息
BOOL WINAPI RedirectApi ( PCHAR pDllName, PCHAR pFunName, DWORD dwNewProc, PHOOK_ITEM pItem )
{
	// 检查参数是否合法
	if ( pDllName == NULL || pFunName == NULL || !dwNewProc || !pItem )
		return FALSE ;

	// 检测目标模块是否存在
	char	szTempDllName[256] = {0} ;
	DWORD	dwBaseImage = (DWORD)GetModuleHandle(NULL) ;
	if ( dwBaseImage == 0 )
		return FALSE ;

	// 取得PE文件头信息指针
	PIMAGE_DOS_HEADER			pDosHeader	= (PIMAGE_DOS_HEADER)dwBaseImage ;
	PIMAGE_NT_HEADERS			pNtHeader	= (PIMAGE_NT_HEADERS)(dwBaseImage + (pDosHeader->e_lfanew)) ;
	PIMAGE_OPTIONAL_HEADER32	pOptionalHeader = &(pNtHeader->OptionalHeader) ;
	PIMAGE_SECTION_HEADER		pSectionHeader	= (PIMAGE_SECTION_HEADER)((DWORD)pNtHeader + 0x18 + pNtHeader->FileHeader.SizeOfOptionalHeader ) ;

	// 遍历导入表
	PIMAGE_THUNK_DATA pThunk, pIAT ;
	PIMAGE_IMPORT_DESCRIPTOR pIID = (PIMAGE_IMPORT_DESCRIPTOR)(dwBaseImage+pOptionalHeader->DataDirectory[1].VirtualAddress ) ;
	while ( pIID->FirstThunk )
	{
		// 检测是否目标模块
		if ( strcmp ( (PCHAR)(dwBaseImage+pIID->Name), pDllName ) )
		{
			pIID++ ;
			continue ;
		}

		pIAT = (PIMAGE_THUNK_DATA)( dwBaseImage + pIID->FirstThunk ) ;
		if ( pIID->OriginalFirstThunk )
			pThunk = (PIMAGE_THUNK_DATA)( dwBaseImage + pIID->OriginalFirstThunk ) ;
		else
			pThunk = pIAT ;

		// 遍历IAT
		DWORD	dwThunkValue = 0 ;
		while ( ( dwThunkValue = *((DWORD*)pThunk) ) != 0 )
		{
			if ( ( dwThunkValue & IMAGE_ORDINAL_FLAG32 ) == 0 )
			{
				// 检测是否目标函数
				if ( strcmp ( (PCHAR)(dwBaseImage+dwThunkValue+2), pFunName ) == 0 )
				{
					// 填充函数重定向信息
					pItem->dwAddr		= (DWORD)pIAT ;
					pItem->dwOldValue	= *((DWORD*)pIAT) ;
					pItem->dwNewValue = dwNewProc;

					// 修改IAT项
					DWORD dwOldProtect = 0 ;
					VirtualProtect ( pIAT, 4, PAGE_READWRITE, &dwOldProtect ) ;
					*((DWORD*)pIAT) = dwNewProc ;
					VirtualProtect ( pIAT, 4, PAGE_READWRITE, &dwOldProtect ) ;
					return TRUE ;
				}
			}

			pThunk ++ ;
			pIAT ++ ;
		}

		pIID ++ ;
	}

	return FALSE ;
}
