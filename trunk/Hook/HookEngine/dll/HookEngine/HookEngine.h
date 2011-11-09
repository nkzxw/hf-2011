#ifndef __HOOKENGINE_H__
#define __HOOKENGINE_H__

#define DllExport   __declspec( dllexport ) 

#define TrampolineLen	64
#define HOOK_ENGINE_TAG						'YHF'

#ifdef _WIN64
 #define SUPPORT_64BIT_OFFSET
 typedef ULONG64 ULONG_NEW;
 #define JMP_MIN_LENGTH 14
#else
 typedef ULONG32 ULONG_NEW;
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
	ULONG_NEW operand;
}JMP_REL, *PJMP_REL;

typedef struct _JMP_ABS
{
	unsigned char opcode1;
	unsigned char opcode2;
	ULONG32 reserved;
	ULONG_NEW operand;
}JMP_ABS, *PJMP_ABS;
#pragma pack(pop)

//
//Hook �ṹ����
//
typedef struct _HOOK_INFO
{
		ULONG ulReplaceLen;      //�滻���ֽڳ���
		ULONG_NEW pOrigFunction; //ԭʼϵͳAPI������ַ
		ULONG_NEW pHookFunction; //HOOK������ַ
		ULONG_NEW pTramFunction; //Trampoline���庯����ַ
}HOOK_INFO, *PHOOK_INFO;

/*
* Hook interface function.
*/
DllExport VOID InstallHook (
		PHOOK_INFO pHookInfo
		);
		
/*
* UnHook interface function.
*/
DllExport VOID UnInstallHook (
		PHOOK_INFO pHookInfo
		);
		
#endif //__HOOKENGINE_H__