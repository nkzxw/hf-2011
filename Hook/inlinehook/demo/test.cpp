#include <Windows.h>
#include <stdio.h>
#include <assert.h>

#if (_MSC_VER > 1200)
	#include <intrin.h>
#endif

#include "../include/inlinehook.h"
#ifdef _DEBUG
#pragma comment(lib, "../debug/inlineHook.lib")
#else
#pragma comment(lib, "../release/inlineHook.lib")
#endif

// MyDetourTrampoline(
// 			 VOID, WINAPI, OutputDebugStringW, ( LPCWSTR lpOutputString )
// 			 );
// 
// MyDetourTrampoline(
// 				   int, WINAPI, myMsgBoxW,(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, __in UINT uType)
// 				   );

//申明自己的钩子函数

//hook的函数返回类型是VOID的话，宏第一个参数写no_Return
MyDetourProc(no_Return,
			 VOID, WINAPI, OutputDebugStringW, ( LPCWSTR lpOutputString )
			 )
{
	CallTrampoline(OutputDebugStringW)(lpOutputString);
}


//hook的函数不管是什么类型返回值，宏第一个参数写has_Return
MyDetourProc(has_Return, 
			 int, WINAPI, myMsgBoxW,( HWND hWnd,  LPCWSTR lpText,  LPCWSTR lpCaption,  UINT uType)
			 )
{
	CallTrampoline(OutputDebugStringW)(lpText);
	CallTrampoline(OutputDebugStringW)(lpText);

	return CallTrampoline(myMsgBoxW)(hWnd, L"changed", lpCaption, uType);
}

void __stdcall test2(LPCWSTR p1)
{
	OutputDebugStringW(p1);
}

__declspec(naked) void __stdcall test1(LPCWSTR p1)
{
	//hook这种函数时需要备份覆盖两条指令，且call指令要修正地址偏移
	_asm
	{
		push DWORD PTR [esp+4];  4 bytes
		call __start          ;  5 bytes
		retn 0x4
__start:
		push ebp
		mov  ebp, esp
		push DWORD PTR [ebp+8]
		call test2
		push DWORD PTR [ebp+8]
		call test2
		pop  ebp
		retn 0x4;
	}
}

MyDetourProc(no_Return, 
			 void, __stdcall, test1,( LPCWSTR p1)
			 )
{
	CallTrampoline(test1)(L"my_test1: ");
	CallTrampoline(test1)(p1);
}

void* getModFunc(char*dll, char *name)
{
	HMODULE h = LoadLibraryA(dll);
	if (!h) return NULL;
	return GetProcAddress(h, name);
}

int main(int argc, char **argv)
{
	int ret;

//	__debugbreak();

	ret = ENABLE_HOOK(test1);
	assert(ret == 0);

	test1(L"test");

	//用法1
  	ret = ENABLE_HOOK(OutputDebugStringW);
	assert(ret == 0);


	//用法2
	ENABLE_HOOK2(myMsgBoxW, getModFunc("user32", "MessageBoxW"));

	MessageBoxW(0, L"hello", L"title", 0);


	DISABLE_HOOK(myMsgBoxW);
 	DISABLE_HOOK(OutputDebugStringW);

	return 0;
}