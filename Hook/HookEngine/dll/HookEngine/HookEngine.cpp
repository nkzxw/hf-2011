// HookEngine.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "HookEngine.h"

#include "../../include/distorm.h"

ULONG CalcReplaceSize (ULONG_NEW pOrigFunction)
{
#define MAX_INSTRUCTIONS 15

	_DecodeResult res;
	ULONG ReplaceSize = 0;
	ULONG Index = 0;
	_DecodedInst decodedInstructions[MAX_INSTRUCTIONS];
	unsigned int decodedInstructionsCount = 0;

#ifndef _WIN64
	_DecodeType dt = Decode32Bits;
#else 
	_DecodeType dt = Decode64Bits;
#endif

	_OffsetType offset = 0;
	res = distorm_decode(offset,	
						(const BYTE *) pOrigFunction,
						45,							
						dt,							
						decodedInstructions,		
						MAX_INSTRUCTIONS,		
						&decodedInstructionsCount	
						);

	if (res == DECRES_INPUTERR)
		return 0;

	
	for (Index; Index < decodedInstructionsCount; Index++)
	{
		if (ReplaceSize >= JMP_MIN_LENGTH)
			break;
			
		ReplaceSize += decodedInstructions[Index].size;
	}	
	
	return ReplaceSize;
}

JMP_REL MakeRelativeJump (ULONG_NEW pFromAddr, ULONG_NEW pToAddr)
{
	JMP_REL JumpREL;
	JumpREL.opcode = 0xE9;  //跳转到相对地址
	
#ifdef _WIN64
	//JumpREL.reserved[5] = {0x90};
	//RtlFillMemory((PCHAR)JumpREL.reserved[5], 5, 0);
	JumpREL.reserved[0] = 0x0;
	JumpREL.reserved[1] = 0x0;
	JumpREL.reserved[2] = 0x0;
	JumpREL.reserved[3] = 0x0;
	JumpREL.reserved[4] = 0x0;
#endif //_WIN64

	JumpREL.operand = pToAddr - (pFromAddr + sizeof (JMP_REL));
	
	return JumpREL;
}

JMP_ABS MakeAbstractJump(ULONG_NEW pToAddr)
{
	JMP_ABS JumpABS;
	JumpABS.opcode1 = 0xff;  //跳转到绝对地址
	JumpABS.opcode2 = 0x25;
	
#ifdef _WIN64
	JumpABS.reserved = 0x0;
#else
	JumpABS.reserved = (ULONG32)pToAddr;
#endif //_WIN64

	JumpABS.operand = pToAddr;
	
	return JumpABS;
}

VOID UnInstallHook (
		PHOOK_INFO pHookInfo
		)
{
	//
	//还原替换函数字节
	//
	DWORD dwOldProtect = 0;
	VirtualProtect((LPVOID)(pHookInfo->pOrigFunction), pHookInfo->ulReplaceLen, PAGE_EXECUTE_READWRITE, &dwOldProtect);	
	
	memcpy((PCHAR)(pHookInfo->pOrigFunction), (PCHAR)(pHookInfo->pTramFunction), pHookInfo->ulReplaceLen);
	//ExFreePoolWithTag ((PVOID)(pHookInfo->pTramFunction), HOOK_ENGINE_TAG);
	unsigned char * pTrame = (unsigned char * )pHookInfo->pTramFunction;
	delete [] pTrame;
	
	DWORD dwBuf = 0;	
	VirtualProtect((LPVOID)(pHookInfo->pOrigFunction), pHookInfo->ulReplaceLen, dwOldProtect, &dwBuf);	
}

VOID InstallHook (
		PHOOK_INFO pHookInfo
		)
{
		ULONG_NEW ulTrampoline = 0;
		unsigned char *pTrampoline = NULL;
#ifdef _WIN64		
		JMP_ABS JmpABS;
#else
		JMP_REL JmpREL;
#endif//_WIN64	

		ULONG ulReplaceLen = 0;
		if (0 == pHookInfo->pOrigFunction ||
				0 == pHookInfo->pHookFunction) {
				return;
		}
		
		//
		//加入反汇编引擎，计算替换指令的字节长度。
		//
		ulReplaceLen = CalcReplaceSize (pHookInfo->pOrigFunction);
		
		DWORD dwOldProtect = 0;
		VirtualProtect((LPVOID)(pHookInfo->pOrigFunction), pHookInfo->ulReplaceLen, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		//
		//申请一块内存写入ShellCode.保存原始函数更改字节并跳转至原始函数位置
		//		
		//pTrampoline = (unsigned char *)ExAllocatePoolWithTag(NonPagedPool,TrampolineLen, HOOK_ENGINE_TAG);        
		pTrampoline = new unsigned char[TrampolineLen];        
		RtlFillMemory(pTrampoline, TrampolineLen, 0x90); 
		ulTrampoline = (ULONG_NEW)pTrampoline;
						
		memcpy((PCHAR)(ulTrampoline), (PCHAR)(pHookInfo->pOrigFunction), ulReplaceLen);		
#ifdef _WIN64		
		JmpABS = MakeAbstractJump (pHookInfo->pOrigFunction + ulReplaceLen);	
		memcpy((PCHAR)(ulTrampoline + ulReplaceLen), (PVOID)(&JmpABS), sizeof(JMP_ABS));
#else		
		JmpREL = MakeRelativeJump (ulTrampoline, pHookInfo->pOrigFunction);
		memcpy((PCHAR)(ulTrampoline + ulReplaceLen), (PCHAR)(&JmpREL), sizeof(JMP_REL));
#endif//_WIN64
		
		//
		//处理原始函数地址的内容，JMP到HOOK函数
		//
		RtlFillMemory((PCHAR)(pHookInfo->pOrigFunction), ulReplaceLen, 0x90); 
#ifdef _WIN64	
		JmpABS = MakeAbstractJump (pHookInfo->pHookFunction);			
		memcpy((PCHAR)(pHookInfo->pOrigFunction), (PVOID)(&JmpABS), sizeof(JMP_ABS)); 
#else		
		JmpREL = MakeRelativeJump (pHookInfo->pOrigFunction, pHookInfo->pHookFunction);			
		memcpy((PCHAR)(pHookInfo->pOrigFunction), (PCHAR)(&JmpREL), sizeof(JMP_REL)); 
#endif//_WIN64	
		
		DWORD dwBuf = 0;	
		VirtualProtect((LPVOID)(pHookInfo->pOrigFunction), pHookInfo->ulReplaceLen, dwOldProtect, &dwBuf);
						
		pHookInfo->ulReplaceLen = ulReplaceLen;
		pHookInfo->pTramFunction = (ULONG_NEW)pTrampoline;	
}

