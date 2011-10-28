
#include "windows.h"

#include "stdio.h"

// 创建标识
// 创建线程之前设置为TRUE，表示需要创建一个合法的线程
// 只有合法线程才能正常创建
BOOL bStatus = FALSE ;


VOID MY_OutputDebugStringA ( const char* szFormat,...)
{
	char szBuf[1024] ;

	va_list argList ;
	va_start ( argList, szFormat ) ;
	vsprintf ( szBuf, szFormat, argList ) ;
	va_end ( argList ) ;

	OutputDebugStringA ( szBuf ) ;
}

// 设置线程创建标识
VOID WINAPI SetStatus ( BOOL status )
{
	bStatus = status ;
}

BOOL WINAPI DllMain ( HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved )
{
	switch ( nReason )
	{
	case DLL_PROCESS_ATTACH:
		break ;
	case DLL_PROCESS_DETACH:
		break ;
	case DLL_THREAD_ATTACH:
		MY_OutputDebugStringA ( "DLL_THREAD_ATTACH ThreadId=%d", GetCurrentThreadId() ) ;
		
		// 检测线程创建标识
		if ( bStatus == FALSE )
		{
			// 如果线程创建标识为FALSE，就直接结束线程
			MY_OutputDebugStringA ( "Terminate unvalid thread!" ) ;
			TerminateThread ( GetCurrentThread(), 0 ) ;
		}
		else
		{
			// 每次创建合法线程后，自动关闭创建标识
			// 每次设置合法线程标识只能使用一次
			SetStatus ( FALSE ) ;
		}
		break ;
	case DLL_THREAD_DETACH:
		MY_OutputDebugStringA ( "DLL_THREAD_DETACH ThreadId=%d", GetCurrentThreadId() ) ;
		break ;
	}

	return TRUE ;
}

