// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include "stdafx.h"
#include "stdio.h"


bool HookCode(
		void* TargetProc, 
		void* NewProc,
		void ** l_OldProc, 
		int bytescopy = 5)
{

	DWORD dwOldProtect;

	::VirtualProtect((LPVOID)TargetProc, bytescopy, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	//
	// Ϊ����ִ�б����ǵ�ָ������ռ�,����ԭָ��.
	//
	*l_OldProc = new unsigned char[bytescopy+5];    
	memcpy(*l_OldProc, TargetProc, bytescopy);       

	//
	// �ҵ��ڴ�Ĵ���ִ���� ����ԭ���� ���� + �ƻ��Ĵ���ĳ��� ��ȥ
	//
	*((unsigned char*)(*l_OldProc) + bytescopy) = 0xe9;     
	
	//
	//���ƻ�ָ��ĳ���      
	//E9 opcode����           
	//���ƫ�Ƶ�OPCODE = ��HOOK������ַ�ĵ�ַ + �ƻ�ָ��ĳ��� - �ҷ����ڴ�Ľ�����ַ                            
	// ���ڴ��������ԭ�������ϵ�ƫ��
	//
	*(unsigned int *)((unsigned char*)(*l_OldProc) + bytescopy + 1) = (unsigned int)(TargetProc) + bytescopy - ( (unsigned int)((*l_OldProc)) + 5 + bytescopy ) ;   
	
	//
	//��HOOK�ĺ���ͷ��Ϊjmp
	//
	*(unsigned char*)TargetProc =(unsigned char)0xe9;              

	//
	//���ƫ�Ƶ�OPCODE   = ��������ַ - ��HOOK������ַ
	//
	*(unsigned int*)((unsigned int)TargetProc +1) = (unsigned int)NewProc - ( (unsigned int)TargetProc + 5) ; //��HOOK�ĵط������ҵ��¹��� ���ܹ���


	::VirtualProtect((LPVOID)TargetProc, bytescopy, dwOldProtect, 0);
	
	return true;
}

bool UnHookCode(void* TargetAddress, void * l_SavedCode, unsigned int len)
{
     DWORD dwOldProtect;

     ::VirtualProtect((LPVOID)TargetAddress, len, PAGE_EXECUTE_READWRITE, &dwOldProtect);   

	 //
     // �ָ����ƻ�����ָ�� 
	 //
     memcpy(TargetAddress, l_SavedCode, len);

     ::VirtualProtect((LPVOID)TargetAddress, len, dwOldProtect, 0);

     return true;
}



unsigned int *   OldProc;
typedef 
int 
(__stdcall * PMESSAGEBOX)      
    (
     IN HWND hWnd,
     IN LPCSTR lpText,
     IN LPCSTR lpCaption,
     IN UINT uType);


int 
__stdcall        
MessageBoxProxy(
     IN HWND hWnd,
     IN LPCSTR lpText,
     IN LPCSTR lpCaption,
     IN UINT uType)
/*++
	��������stdcall�Ļ� ��������Ϊ��C������ʽ Ҫ�Լ�ƽ���ջ  
	����ִ��һЩ������Ϊ ����ı���� ���� ֱ��ģ�ⷵ����ȷ�Ľ��
   
--*/
{
	OutputDebugString  (L"MessageBoxProxy Entry.\n");
	if ( strcmp(lpText, "sample") == 0)
	{
		printf("filter");
		return 1;
	}

	return ( (PMESSAGEBOX) OldProc)(hWnd, lpText, lpCaption, uType);
}

void StartHook ()
{
	MessageBox (0, L"begin", L"begin", MB_OK);

	HookCode ((void *)MessageBox, (void *)MessageBoxProxy, (void **)&OldProc, 5);

	MessageBox (0, L"inlinehook", L"inlinehook", MB_OK);

	UnHookCode ((void*)MessageBox, OldProc, 5);

	MessageBox (0, L"over", L"over", MB_OK);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		StartHook ();
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

