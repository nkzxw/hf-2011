

#include <windows.h>
#include "mouse.h"
#include "stdio.h"


HINSTANCE	hDllInst	= NULL ;	// DLL模块实例句柄

int WINAPI DllMain ( HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved )
{ 
	if ( fdwReason == DLL_PROCESS_ATTACH )
		hDllInst = hInstance ;		// 保存DLL实例句柄
	return true ;
}

// 鼠标钩子消息处理过程
LRESULT CALLBACK MouseProc ( int nCode, WPARAM wParam, LPARAM lParam )
{
	if ( nCode == HC_ACTION )
	{
		WCHAR	szBuf[32] = {0} ;
		PMOUSEHOOKSTRUCT pMouseInfo = (PMOUSEHOOKSTRUCT)lParam ;
		switch ( wParam )
		{
		// 对鼠标移动消息的处理
		case WM_MOUSEMOVE:
			{
				// 设置鼠标坐标信息
				wsprintf ( szBuf, L"( %d, %d )", pMouseInfo->pt.x, pMouseInfo->pt.y ) ;
				SetWindowText  ( hTagWnd, szBuf ) ;
			}
			break ;
		// 可以添加对其他鼠标消息的处理
		case WM_LBUTTONDOWN:	break ;
		case WM_LBUTTONUP:		break ;
		case WM_LBUTTONDBLCLK:	break ;
		}
	}
	// 继续传递消息
	return CallNextHookEx ( hMouse, nCode, wParam, lParam ) ;
}

// 钩子控制函数，能够启动/卸载钩子
BOOL WINAPI SetHook ( HWND hwnd, BOOL isInstall ) 
{
	// 需要安装，且鼠标钩子不存在
	if ( isInstall && !hMouse )
	{
		// 保存窗体句柄到共享段
		hTagWnd = hwnd ;	

		// 设置全局钩子
		hMouse = SetWindowsHookEx ( WH_MOUSE, (HOOKPROC)MouseProc, hDllInst, 0 ) ;
		if ( hMouse == NULL )
			return FALSE ;
	}

	// 需要卸载，且鼠标钩子存在
	if ( !isInstall && hMouse )
	{
		// 卸载钩子
		BOOL ret = UnhookWindowsHookEx ( hMouse ) ;
		hMouse		= NULL ;
		hTagWnd		= NULL ;
		return ret ;
	}

	return TRUE ;
}

	

