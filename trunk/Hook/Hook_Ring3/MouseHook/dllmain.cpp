// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include "stdafx.h"
#include "MouseHook.h"

#pragma data_seg("mydata")

//
//�ϴ������ָ�Ĵ��ھ��
//
HWND glhPrevTarWnd = NULL;

//
//��ʾĿ�괰�ڱ���༭��ľ��
//
HWND glhDisplayWnd = NULL;

//
//��װ����깳�Ӿ��
//
HHOOK glhHook = NULL;

HINSTANCE	g_hInstance = NULL;

#pragma data_seg() 

LRESULT WINAPI MouseProc(
		int nCode,
		WPARAM wparam,
		LPARAM lparam)
{
	OutputDebugString (L"MouseProc Entry.\n");

	return CallNextHookEx(glhHook,nCode,wparam,lparam);

	LPMOUSEHOOKSTRUCT pMouseHook=(MOUSEHOOKSTRUCT FAR *) lparam;
	if ( nCode >= 0 )
	{
		//
		//ȡĿ�괰�ھ��
		//
		HWND glhTargetWnd=pMouseHook->hwnd;
		HWND ParentWnd=glhTargetWnd;
		while (ParentWnd !=NULL)
		{
			glhTargetWnd=ParentWnd;
			//
			//ȡӦ�ó��������ھ��
			//
			ParentWnd=GetParent(glhTargetWnd);	
		}

		if(glhTargetWnd!=glhPrevTarWnd)
		{
			WCHAR szCaption[100];
			GetWindowText(glhTargetWnd,szCaption,100);

			if(IsWindow(glhDisplayWnd)){
				SendMessage(glhDisplayWnd,WM_SETTEXT,0,(LPARAM)(LPCTSTR)szCaption);
			}

			//
			//����Ŀ�괰��
			//
			glhPrevTarWnd=glhTargetWnd;	
		}
	}
	return CallNextHookEx(glhHook,nCode,wparam,lparam);
} 

BOOL StartHook(HWND hWnd)
{
	OutputDebugString (L"StartHook Entry.\n");

	//
	//������ʾĿ�괰�ڱ���༭��ľ��
	//
	glhDisplayWnd = hWnd;	
	glhHook = SetWindowsHookEx( 
			WH_KEYBOARD, 
			MouseProc, 
			g_hInstance, 
			0);

	if( glhHook != NULL){
		return TRUE;
	}
	else{
		return FALSE; 
	}
}

BOOL StopHook()
{
	OutputDebugString (L"StopHook Entry.\n");

	BOOL bResult=FALSE;

	if(glhHook)
	{
		bResult = UnhookWindowsHookEx(glhHook);
		if(bResult)
		{
			glhPrevTarWnd = NULL;
			glhDisplayWnd = NULL;
			glhHook = NULL;
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

