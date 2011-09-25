// dllmain.cpp : 定义 DLL 应用程序的入口点。
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
	// 为拷贝执行被覆盖的指令申请空间,保存原指令.
	//
	*l_OldProc = new unsigned char[bytescopy+5];    
	memcpy(*l_OldProc, TargetProc, bytescopy);       

	//
	// 我的内存的代码执行完 跳到原来的 代码 + 破坏的代码的长度 上去
	//
	*((unsigned char*)(*l_OldProc) + bytescopy) = 0xe9;     
	
	//
	//被破坏指令的长度      
	//E9 opcode长度           
	//算出偏移的OPCODE = 被HOOK函数地址的地址 + 破坏指令的长度 - 我分配内存的结束地址                            
	// 我内存代码跳到原来代码上的偏移
	//
	*(unsigned int *)((unsigned char*)(*l_OldProc) + bytescopy + 1) = (unsigned int)(TargetProc) + bytescopy - ( (unsigned int)((*l_OldProc)) + 5 + bytescopy ) ;   
	
	//
	//被HOOK的函数头改为jmp
	//
	*(unsigned char*)TargetProc =(unsigned char)0xe9;              

	//
	//算出偏移的OPCODE   = 代理函数地址 - 被HOOK函数地址
	//
	*(unsigned int*)((unsigned int)TargetProc +1) = (unsigned int)NewProc - ( (unsigned int)TargetProc + 5) ; //被HOOK的地方跳到我的新过程 接受过滤


	::VirtualProtect((LPVOID)TargetProc, bytescopy, dwOldProtect, 0);
	
	return true;
}

bool UnHookCode(void* TargetAddress, void * l_SavedCode, unsigned int len)
{
     DWORD dwOldProtect;

     ::VirtualProtect((LPVOID)TargetAddress, len, PAGE_EXECUTE_READWRITE, &dwOldProtect);   

	 //
     // 恢复被破坏处的指令 
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
	不声明成stdcall的话 编译器认为是C声明方式 要自己平衡堆栈  
	可以执行一些过滤行为 比如改变参数 或者 直接模拟返回正确的结果
   
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

