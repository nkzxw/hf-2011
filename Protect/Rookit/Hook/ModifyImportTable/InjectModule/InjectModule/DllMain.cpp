
#include  <windows.h>

// 空函数，仅仅是个导出符号，在向导入表添加输入项时使用
VOID WINAPI Install ()
{
}

BOOL WINAPI DllMain ( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved )
{
	if ( fdwReason == DLL_PROCESS_ATTACH )
		OutputDebugStringA ( "模块被加载!" ) ;
    return TRUE;
}
