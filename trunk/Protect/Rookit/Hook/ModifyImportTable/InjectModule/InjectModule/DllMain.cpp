
#include  <windows.h>

// �պ����������Ǹ��������ţ�����������������ʱʹ��
VOID WINAPI Install ()
{
}

BOOL WINAPI DllMain ( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved )
{
	if ( fdwReason == DLL_PROCESS_ATTACH )
		OutputDebugStringA ( "ģ�鱻����!" ) ;
    return TRUE;
}
