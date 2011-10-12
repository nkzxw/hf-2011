#include "stdafx.h"
#include <tchar.h>
#include <stdio.h>   
#include <stdarg.h>
#include <Psapi.h>
#include "HookEngine.h"
#include "Disasm.h"

void OutputDebugPrintf(const char * strOutputString,...)
{
	char strBuffer[4096]={0};
	va_list vlArgs;
	va_start(vlArgs,strOutputString);
	_vsnprintf(strBuffer,sizeof(strBuffer)-1,strOutputString,vlArgs);
	va_end(vlArgs);
	OutputDebugStringA(strBuffer);
}

void OutputDebugPrintfW(const WCHAR * strOutputString,...)
{
	WCHAR strBuffer[4096]={0};
	va_list vlArgs;
	va_start(vlArgs,strOutputString);
	_vsnwprintf (strBuffer,sizeof(strBuffer)-1,strOutputString,vlArgs);
	va_end(vlArgs);
	OutputDebugStringW(strBuffer);
}

void DbgW(const WCHAR* format,...)
{
	static const int BufferLen = 2000;
	va_list pNextArg;
	WCHAR szMessageBuffer[BufferLen];
	szMessageBuffer[BufferLen-1] = _T('\0');
	va_start(pNextArg,format);
	_vsnwprintf(szMessageBuffer,BufferLen-1,format,pNextArg);
	va_end(pNextArg);

	OutputDebugStringW(szMessageBuffer);
	//SaveChatInfoW(szMessageBuffer);
	//SaveChatInfoA("\n");
}
void Dbg(const TCHAR* format,...)
{
	static const int BufferLen = 2000;
	va_list pNextArg;
	TCHAR szMessageBuffer[BufferLen];
	szMessageBuffer[BufferLen-1] = _T('\0');
	va_start(pNextArg,format);
	_vsntprintf(szMessageBuffer,BufferLen-1,format,pNextArg);
	va_end(pNextArg);
	OutputDebugString(szMessageBuffer);
	//SaveChatInfoA(szMessageBuffer);
	//SaveChatInfoA("\n");
}


#ifndef _WIN64

ULONG SearchHookEntry(
	ULONG BaseAddress, 
	char *  featureCode, 
	ULONG  featureCodeLen, 
	LONG offset)
{
	MODULEINFO moduleInfo;
	memset(&moduleInfo, 0, sizeof(MODULEINFO));
	if(!GetModuleInformation(GetCurrentProcess(), (HMODULE)(BaseAddress - 0x1000), &moduleInfo, sizeof(MODULEINFO)))
	{
		return 0;
	}

	ULONG index = 0;
	DISASSEMBLY Disasm;
	FlushDecoded(&Disasm);
	Disasm.Address = BaseAddress;
	while(index < (moduleInfo.SizeOfImage-0x1000 - featureCodeLen))
	{
		if(memcmp((char *)Disasm.Address, featureCode, featureCodeLen) == 0)
		{
			return Disasm.Address - offset;
		}
		ZeroMemory(Disasm.Assembly,255);
		Decode(&Disasm, (char *)BaseAddress, &index);
		
		Disasm.Address += Disasm.OpcodeSize + Disasm.PrefixSize;
		FlushDecoded(&Disasm);
		index ++;
	}
	return 0;
}

ULONG JmpHook(ULONG src, ULONG nakedFun)
{
	DWORD wLen = 0;
	ULONG index = 0;
	DISASSEMBLY Disasm;
	FlushDecoded(&Disasm);
	Disasm.Address = src;
	
	while(index < 5)
	{
		Decode(&Disasm, (char *)src, &index);
		Disasm.Address += Disasm.OpcodeSize + Disasm.PrefixSize;
		FlushDecoded(&Disasm);
		index ++;
	}

	//
	//申请index大小的buffer.(写入一段跳转指令跳入到我们自定义的代理函数)
	//
	char * newCodeBuf =new char[index];
	memset(newCodeBuf, 0x90, index);

	//
	//申请index大小的buffer.(保存替换原函数的指令码)
	//
	char * oldCodeBuf = new char[index];
	ReadProcessMemory(GetCurrentProcess(), (char *)src, oldCodeBuf, index, &wLen);

	//
	//跳转到代理函数  
	//目标 == 相对跳转偏移量 = (当前位置 - 5(jmp占用))
	//
	newCodeBuf[0] = 0xe9; //jmp opcode.
	*((ULONG *)&newCodeBuf[1]) = (nakedFun - src) - 5 ;

	//
	//写入跳转指令码到原函数地址
	//
	WriteProcessMemory(GetCurrentProcess(), (char *)src, newCodeBuf, index, &wLen);

	//
	//将被覆盖的代码填充到代理函数中call真实函数之后
	//
	WriteProcessMemory(GetCurrentProcess(), (char *)(nakedFun+5), oldCodeBuf, index, &wLen);

	//
	//返回到被hook的下一条指令
	//
	char retCode[5];
	retCode[0] = 0xe9;
	*((ULONG *)&retCode[1]) = (src + 5) - (nakedFun + 5 + index) - 5;

	//
	//跳转到目标（原指令被替换jmp的下一条）- 当前位置（裸函数 + call 自己的真实处理函数 + 补缺指令）- 5（jmp占用）
	//
	WriteProcessMemory(GetCurrentProcess(), (char *)(nakedFun + 5 + index), retCode, 5, &wLen);

	delete newCodeBuf;

	//
	//函数返回被修补了多少字节代码
	//
	return index;
}

ULONG NewJwtServiceHook(ULONG src, ULONG nakedFun)
{
	DWORD wLen = 0;
	ULONG index = 0;
	DISASSEMBLY Disasm;
	FlushDecoded(&Disasm);
	Disasm.Address = src;
	
	while(index < 5)
	{
		Decode(&Disasm, (char *)src, &index);
		Disasm.Address += Disasm.OpcodeSize + Disasm.PrefixSize;
		FlushDecoded(&Disasm);
		index ++;
	}

	//
	//申请index大小的buffer.(写入一段跳转指令跳入到我们自定义的代理函数)
	//
	char * newCodeBuf =new char[index];
	memset(newCodeBuf, 0x90, index);

	//
	//申请index大小的buffer.(保存替换原函数的指令码)
	//
	char * oldCodeBuf = new char[index];
	ReadProcessMemory(GetCurrentProcess(), (char *)src, oldCodeBuf, index, &wLen);

	//
	//跳转到代理函数  
	//目标 == 相对跳转偏移量 = (当前位置 - 5(jmp占用))
	//
	newCodeBuf[0] = 0xe9; //jmp opcode.
	*((ULONG *)&newCodeBuf[1]) = (nakedFun - src) - 5 ;

	//
	//写入跳转指令码到原函数地址
	//
	WriteProcessMemory(GetCurrentProcess(), (char *)src, newCodeBuf, index, &wLen);

	//
	//将被覆盖的代码填充到代理函数中call真实函数之后
	//
	WriteProcessMemory(GetCurrentProcess(), (char *)(nakedFun+5), oldCodeBuf, index, &wLen);

	//
	//返回到被hook的下一条指令
	//
	char retCode[5];
	retCode[0] = 0xe9;
	*((ULONG *)&retCode[1]) = (src + 5) - (nakedFun + 5 + index) - 5;

	//
	//跳转到目标（原指令被替换jmp的下一条）- 当前位置（裸函数 + call 自己的真实处理函数 + 补缺指令）- 5（jmp占用）
	//
	WriteProcessMemory(GetCurrentProcess(), (char *)(nakedFun + 5 + index), retCode, 5, &wLen);

	delete newCodeBuf;

	//
	//函数返回被修补了多少字节代码
	//
	return index;
}

#endif