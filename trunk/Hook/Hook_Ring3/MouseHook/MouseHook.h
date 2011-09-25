/*++
Copyright (C) HF.  2011-2012.

Abstract :
	Mouse Hook Practice.
Author:
	yhf (hongfu830202@163.com)

CreateTime:
	2011-9-24
--*/

#ifndef __HF_MOUSEHOOK_H__
#define __HF_MOUSEHOOK_H__

extern "C" BOOL __declspec(dllexport) StartHook(HWND hWnd);

extern "C" BOOL __declspec(dllexport) UnHook();

#endif //__HF_MOUSEHOOK_H__