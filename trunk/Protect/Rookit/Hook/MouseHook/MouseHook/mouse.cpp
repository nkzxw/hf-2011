

#include <windows.h>
#include "mouse.h"
#include "stdio.h"


HINSTANCE	hDllInst	= NULL ;	// DLLģ��ʵ�����

int WINAPI DllMain ( HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved )
{ 
	if ( fdwReason == DLL_PROCESS_ATTACH )
		hDllInst = hInstance ;		// ����DLLʵ�����
	return true ;
}

// ��깳����Ϣ�������
LRESULT CALLBACK MouseProc ( int nCode, WPARAM wParam, LPARAM lParam )
{
	if ( nCode == HC_ACTION )
	{
		WCHAR	szBuf[32] = {0} ;
		PMOUSEHOOKSTRUCT pMouseInfo = (PMOUSEHOOKSTRUCT)lParam ;
		switch ( wParam )
		{
		// ������ƶ���Ϣ�Ĵ���
		case WM_MOUSEMOVE:
			{
				// �������������Ϣ
				wsprintf ( szBuf, L"( %d, %d )", pMouseInfo->pt.x, pMouseInfo->pt.y ) ;
				SetWindowText  ( hTagWnd, szBuf ) ;
			}
			break ;
		// ������Ӷ����������Ϣ�Ĵ���
		case WM_LBUTTONDOWN:	break ;
		case WM_LBUTTONUP:		break ;
		case WM_LBUTTONDBLCLK:	break ;
		}
	}
	// ����������Ϣ
	return CallNextHookEx ( hMouse, nCode, wParam, lParam ) ;
}

// ���ӿ��ƺ������ܹ�����/ж�ع���
BOOL WINAPI SetHook ( HWND hwnd, BOOL isInstall ) 
{
	// ��Ҫ��װ������깳�Ӳ�����
	if ( isInstall && !hMouse )
	{
		// ���洰�����������
		hTagWnd = hwnd ;	

		// ����ȫ�ֹ���
		hMouse = SetWindowsHookEx ( WH_MOUSE, (HOOKPROC)MouseProc, hDllInst, 0 ) ;
		if ( hMouse == NULL )
			return FALSE ;
	}

	// ��Ҫж�أ�����깳�Ӵ���
	if ( !isInstall && hMouse )
	{
		// ж�ع���
		BOOL ret = UnhookWindowsHookEx ( hMouse ) ;
		hMouse		= NULL ;
		hTagWnd		= NULL ;
		return ret ;
	}

	return TRUE ;
}

	

