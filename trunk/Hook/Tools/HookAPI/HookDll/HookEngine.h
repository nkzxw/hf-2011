#if !defined(HOOKAPI_ENGINE)
#define HOOKAPI_ENGINE

//
//���������ƣ����ص�xxx����ִ��,ʵ�ʹ��ܺ�����ַ
//
#define NAKED_PROXY_FUNCTION(name, RealFuntion) \
	_declspec(naked) name() \
{ \
	__asm call RealFuntion \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
	__asm _emit 0x90 \
} 

ULONG JmpHook(ULONG src, ULONG nakedFun);

ULONG SearchHookEntry(ULONG BaseAddress, char *featureCode, ULONG  featureCodeLen, LONG offset = 0);

void DbgW(const WCHAR* format,...);

void Dbg(const TCHAR* format,...);

void OutputDebugPrintf(const char * strOutputString,...);

void OutputDebugPrintfW(const WCHAR * strOutputString,...);

#endif //HOOKAPI_ENGINE
