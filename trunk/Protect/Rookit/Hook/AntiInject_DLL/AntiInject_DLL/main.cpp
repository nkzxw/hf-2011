
#include "windows.h"

#include "stdio.h"

// ������ʶ
// �����߳�֮ǰ����ΪTRUE����ʾ��Ҫ����һ���Ϸ����߳�
// ֻ�кϷ��̲߳�����������
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

// �����̴߳�����ʶ
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
		
		// ����̴߳�����ʶ
		if ( bStatus == FALSE )
		{
			// ����̴߳�����ʶΪFALSE����ֱ�ӽ����߳�
			MY_OutputDebugStringA ( "Terminate unvalid thread!" ) ;
			TerminateThread ( GetCurrentThread(), 0 ) ;
		}
		else
		{
			// ÿ�δ����Ϸ��̺߳��Զ��رմ�����ʶ
			// ÿ�����úϷ��̱߳�ʶֻ��ʹ��һ��
			SetStatus ( FALSE ) ;
		}
		break ;
	case DLL_THREAD_DETACH:
		MY_OutputDebugStringA ( "DLL_THREAD_DETACH ThreadId=%d", GetCurrentThreadId() ) ;
		break ;
	}

	return TRUE ;
}

