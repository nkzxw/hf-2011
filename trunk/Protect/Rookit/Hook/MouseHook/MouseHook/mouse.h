
#pragma once

// ���干���
#pragma data_seg ( "Shared" )
HHOOK		hMouse		= NULL ;	// ��깳�Ӿ��
HWND		hTagWnd		= NULL ;	// Ŀ�괰����
#pragma data_seg ()
#pragma comment ( linker, "/SECTION:Shared,RWS" )

BOOL WINAPI SetHook ( HWND hwnd, BOOL isInstall ) ;