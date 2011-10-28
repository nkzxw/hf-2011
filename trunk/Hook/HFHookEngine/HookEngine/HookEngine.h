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
//Opcode �ṹ
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
//Hook �ṹ����
//
typedef struct _HOOK_INFO
{
		ULONG ulReplaceLen;      //�滻���ֽڳ���
		ULONG_PTR pOrigFunction; //ԭʼϵͳAPI������ַ
		ULONG_PTR pHookFunction; //HOOK������ַ
		ULONG_PTR pTramFunction; //Trampoline���庯����ַ
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