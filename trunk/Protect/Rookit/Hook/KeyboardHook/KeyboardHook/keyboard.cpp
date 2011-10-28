

#include <windows.h>
#include "keyboard.h"
#include "stdio.h"

#define _WIN32_WINNT 0x0501

HINSTANCE	hDllInst	= NULL ;	// DLLģ��ʵ�����

int WINAPI DllMain ( HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved )
{ 
	if ( fdwReason == DLL_PROCESS_ATTACH )
		hDllInst = hInstance ;		// ����DLLʵ�����
	return true ;
}

// ���̹�����Ϣ�������
LRESULT CALLBACK KeyboardProc ( int nCode, WPARAM wParam, LPARAM lParam )
{
	// ���λΪ0(lParam>0)����ʾVM_KEYDOWN��Ϣ
	if( ( nCode == HC_ACTION ) && ( lParam > 0 ) )
	{
		WCHAR KeyName[10] = {0} ;
		GetKeyNameText ( (LONG)lParam, KeyName, 50 ) ;
		MessageBox ( NULL, KeyName, L"ȫ�ּ��̹���", MB_OK ) ;
	}

	// ����������Ϣ
	return CallNextHookEx ( hKeyboard, nCode, wParam, lParam ) ;
}

// ���ӿ��ƺ������ܹ�����/ж�ع���
BOOL WINAPI SetHook ( BOOL isInstall ) 
{
	// ��Ҫ��װ���Ҽ��̹��Ӳ�����
	if ( isInstall && !hKeyboard )
	{
		// ����ȫ�ֹ���
		hKeyboard = SetWindowsHookEx ( WH_KEYBOARD, (HOOKPROC)KeyboardProc, hDllInst, 0 ) ;
		if ( hKeyboard == NULL )
		{
			DWORD dwErrorCode = GetLastError () ;
			return FALSE ;
		}
	}

	// ��Ҫж�أ��Ҽ��̹��Ӵ���
	if ( !isInstall && hKeyboard )
	{
		// ж�ع���
		BOOL ret = UnhookWindowsHookEx ( hKeyboard ) ;
		hKeyboard	= NULL ;
		return ret ;
	}

	return TRUE ;
}

	

