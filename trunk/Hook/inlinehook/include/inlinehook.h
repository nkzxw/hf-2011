#ifndef INLINEHOOK_2E489178_H
#define INLINEHOOK_2E489178_H

//
//2012.04.19   by cherryEx. 83617686@qq.com
//

#ifdef   __cplusplus
extern "C"{
#endif

int hook_set(unsigned char *target, unsigned char *trampoline, void *new_handler);
int hook_remove(unsigned char *trampoline, void *new_handler);


#define TRAMPOLINE_SIZE 134

#define no_Return ;
#define has_Return return 0;

#if defined (_MSC_VER) && (_MSC_VER <= 1200)
#define __debugbreak() { __asm int 3};
#endif

/*
����һ��TRAMPOLINE
----
һ�ѵ�__debugbreak();���������亯�����볤�ȣ���Щ����ռ����������hookǰ��ָ��ȵ�

����volatile int i = 0; memset(0, 0, 0); �ǹ����������ű�����ĳЩ�Ż���Ϊ

���磬1������volatile��ʱ�򣬶�����xxxA��xxxW����������trampoline��
      �������󣬷�������trampoline�ĺ�����ַ��Ȼ��ͬһ������Ϊ����������������������һ���ľ��Ż�Ϊһ���ˣ�
	  �������ڼӸ�volatile���ֿ��Ա��������Ż�

	  2���������ڼӸ����������ĵ��ã�����ѡ��memset�����ţ��������ʱ���������Ż�
	  ��ô�������������ε���
	  trampoline_xxxA(arg1);
	  trampoline_xxxA(arg1); ��ʱ��
	  ���Ӧ���ǣ�
	  lea rcx, arg1  x64���õ�һ������
	  call trampoline_xxxA
	  lea rcx, arg1  �ٴε�������ҪҪ�ٴ����õ�
	  call trampoline_xxxA
	  //�������memset�����ţ�����trampoline�Ĵ���ʱ��������Ϊ�ú���û�κΡ������塱������ȫ�����޸��κμĴ������ᱻ�Ż�Ϊ
	  lea rcx, arg1
	  call trampoline_xxxA
	  call trampoline_xxxA
	  ����ڶ��ε��õ�ʱ����������ò���Ҫ�ٴ�����rcx��
-----------------
*/

#define DEFINE_TRAMPOLINE(name, ret)\
	name {\
	volatile int i = 0;\
	memset(0, 0, 0);\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	__debugbreak();__debugbreak();__debugbreak();__debugbreak();\
	##ret;\
}



#define DETOUR_METHOD(rettype, calltype, method) rettype calltype method
#define MyDetourHook(rettype, calltype, method, args) DETOUR_METHOD(rettype, calltype, hook_##method)##args
#define MyDetourTrampoline(rettype, calltype, method, args) DETOUR_METHOD(rettype, calltype, Trampoline_##method)##args

#define MyDetourProc(returnVal,rettype, calltype, method, args)\
	DEFINE_TRAMPOLINE(MyDetourTrampoline(rettype, calltype, method, args), returnVal);\
	MyDetourHook(rettype, calltype, method, args)

#define CallTrampoline(method) Trampoline_##method

#define ENABLE_HOOK(fn) hook_set((unsigned char*)fn, (unsigned char*)Trampoline_##fn, hook_##fn)
#define DISABLE_HOOK(fn) hook_remove((unsigned char*)Trampoline_##fn, hook_##fn)
#define ENABLE_HOOK2(fn, addr) hook_set((unsigned char*)addr, (unsigned char*)Trampoline_##fn, (unsigned char*)hook_##fn)

#ifdef   __cplusplus
}
#endif


#endif //INLINEHOOK_2E489178_H