#ifndef __HOOK_ENGINE_H__
#define __HOOK_ENGINE_H__

#define TrampolineLen	64

#ifdef _WIN64
 #define SUPPORT_64BIT_OFFSET
 typedef ULONG64 ULONG_PTR;
 #define JMP_MIN_LENGTH 14
#else
 typedef ULONG32 ULONG_PTR;
 #define JMP_MIN_LENGTH 5
#endif


//
//Opcode 结构
//
#pragma pack(push, 1)
typedef struct _JMP_REL
{
	unsigned char opcode;
#ifdef _WIN64
	unsigned char reserved[5];
#endif 	
	ULONG_PTR operand;
}JMP_REL, *PJMP_REL;

typedef struct _JMP_ABS
{
	unsigned char opcode1;
	unsigned char opcode2;
#ifdef _WIN64
	unsigned char reserved[4];
#endif 
	ULONG_PTR operand;
}JMP_ABS, *PJMP_ABS;
#pragma pack(pop)

//
//Hook 结构对象
//
typedef struct _HOOK_INFO
{
		ULONG ulReplaceLen;      //替换的字节长度
		ULONG_PTR pOrigFunction; //原始系统API函数地址
		ULONG_PTR pHookFunction; //HOOK函数地址
		ULONG_PTR pTramFunction; //Trampoline跳板函数地址
}HOOK_INFO, *PHOOK_INFO;

/*
* Hook interface function.
*/
VOID InstallHook (
		PHOOK_INFO pHookInfo
		);
		
/*
* UnHook interface function.
*/
VOID UnInstallHook (
		PHOOK_INFO pHookInfo
		);
		
#endif //__HOOK_ENGINE_H__