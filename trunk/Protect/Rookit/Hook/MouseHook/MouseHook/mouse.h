
#pragma once

// 定义共享段
#pragma data_seg ( "Shared" )
HHOOK		hMouse		= NULL ;	// 鼠标钩子句柄
HWND		hTagWnd		= NULL ;	// 目标窗体句柄
#pragma data_seg ()
#pragma comment ( linker, "/SECTION:Shared,RWS" )

BOOL WINAPI SetHook ( HWND hwnd, BOOL isInstall ) ;