#include "config.h"
#include "../include/inlinehook.h"
//#include <intrin.h>

#ifndef USER_MODE
#define KERNEL_MODE
#endif

#ifndef KERNEL_MODE
#include <windows.h>
#endif

#pragma pack(push, 1)

typedef struct _JMPER
{
	//jmp old function
#ifdef _M_IX86
	unsigned char jmp; void *offsetaddr;
	//char dbgnop[2];
#else ifdef _M_AMD64
	unsigned short jmp;//\xff\x25
	long offset; //=0
	void* addr;//
#endif

}JMPER, *PJMPER;

#define MAX_TrampolineInfo_JMPADDR 4

typedef struct _TrampolineInfo
{
	unsigned char *this_addr;
	void *jmpaddr[MAX_TrampolineInfo_JMPADDR];
	char addr_count;
	int bakoffset;
	int baklen;
	unsigned char *target;
	void *new_handler;
}TrampolineInfo, *PTrampolineInfo;

#define TRAMPOLINE_INFO_OFFSET (TRAMPOLINE_SIZE-sizeof(TrampolineInfo))

unsigned char *GetTrampolineInfo_jmpaddr(PTrampolineInfo pTI, int i)
{
	return pTI->this_addr + (int)&((PTrampolineInfo)0)->jmpaddr[i];
}

#pragma pack(pop)

#ifdef SUPPORT_64BIT_OFFSET
_DecodeResult distorm_decompose64(_CodeInfo* ci, _DInst result[], unsigned int maxInstructions, unsigned int* usedInstructionsCount);
#define distorm_decompose distorm_decompose64
#else
_DecodeResult distorm_decode32(_OffsetType codeOffset, const unsigned char* code, int codeLen, _DecodeType dt, _DecodedInst result[], unsigned int maxInstructions, unsigned int* usedInstructionsCount);
#define distorm_decompose distorm_decompose32
#endif

#ifdef _M_IX86
#define constDecodeType Decode32Bits
#else ifdef _M_AMD64
#define constDecodeType Decode64Bits
#endif

#define MAX_INSTRUCTIONS 32

int bakcode(uint32_t InsertCodeLen, _DecodeType dt, unsigned char*pCode, int codeLen, unsigned char*pBakBuf, uint32_t *pBackupLen)
{

	_DecodeResult res;
	uint32_t decodedInstructionsCount = 0;
	_DInst di[MAX_INSTRUCTIONS];
	_CodeInfo ci;

	uint32_t InstrOffset = 0;
	uint32_t x;


	ci.codeOffset = 0;
	ci.code = pCode;
	ci.codeLen = codeLen;
	ci.dt = dt;
	ci.features = DF_NONE;
	if (dt == Decode16Bits) ci.features = DF_MAXIMUM_ADDR16;
	else if (dt == Decode32Bits) ci.features = DF_MAXIMUM_ADDR32;

	res = distorm_decompose(&ci,	
		(_DInst*)&di, 
		MAX_INSTRUCTIONS,							 
		&decodedInstructionsCount	
		);

	if (res == DECRES_INPUTERR)
		return -1;

	for (x = 0; x < decodedInstructionsCount; x++)
	{
		unsigned char *op;

		op = pCode + InstrOffset;

		InstrOffset += di[x].size;

		if (InstrOffset >= InsertCodeLen)
			break;

		//TODO:
		if (0
			|| op[0] == 0xEB //jmp short
			|| META_GET_FC(di[x].meta) == FC_RET
			)
			return -1;


	}

	if (x == decodedInstructionsCount)
		return -1;

	if (pBakBuf)
	{
		memcpy(pBakBuf, pCode, InstrOffset);
	}
	if (pBackupLen)
	{
		*pBackupLen = InstrOffset;
	}
	return 0;
}


void *get_final_addr(unsigned char *code, int onlyOnce)
{
	do
	{
#ifdef _M_AMD64
		int added = 0;

		if (code[0] == 0x48)
		{
			added = 1;
			code++;
		}
#endif	

		if (code[0] == 0xE9)
		{
			code = code + 5 + *((long*)&code[1]);
		}else if (code[0] == 0xEB)
		{
			code = code + 2 + *((char*)&code[1]);
		}else if (code[0] == 0xff && code[1] == 0x25)
		{
#ifdef _M_IX86
			code = (unsigned char *)*((unsigned long*)&code[2]);
			code = (unsigned char *)*((unsigned long*)code);
#else ifdef _M_AMD64
			code = code + 6 + *((long*)&code[2]);
			code = (unsigned char *)*(unsigned __int64*)code;
#endif	
		}else
		{
#ifdef _M_AMD64
			if (added)
				code--;
#endif	

			break;
		}
	}while(!onlyOnce);


	return code;
}

void init_jmper(PJMPER pjmp, void *from, void *to)
{
#ifdef _M_IX86
	pjmp->jmp = 0xe9;
	pjmp->offsetaddr = (void*)((char*)to - (char*)from - 5);

#else ifdef _M_AMD64
	pjmp->jmp = 0x25ff;
	pjmp->offset = 0;
	pjmp->addr = to;
#endif
}

int fix_rva(unsigned char *code, int codeLen, unsigned char *newcode, int *newlen, unsigned char *from, unsigned char *to, PTrampolineInfo pTI)
{
	memcpy(newcode, code, codeLen);
	*newlen = codeLen;

#ifdef _M_IX86
#else ifdef _M_AMD64
	if (code[0] == 0x48)
	{
		code++;
		codeLen--;

		newcode++;

		from++;
		to++;
	}
#endif	

	if ((code[0] == 0xE9 || code[0] == 0xE8)&& codeLen == 5)
	{
#ifdef _M_IX86

		long offset = *((long*)&code[1]);
		unsigned char *real = from + 5 + offset;

		offset = (long)((char*)real - (char*)to - 5);

		*((long*)&newcode[1]) = offset;

#else ifdef _M_AMD64
		long offset = *((unsigned long*)&code[1]);
		unsigned char *addr = (unsigned char *)(from + 5 + offset);

		//to+5 + X = addr
		__int64 offset64 = (__int64)(addr - (to + 5));

		if (offset64 < -0x7FFFFFFF || offset64 > 0x7FFFFFFF)
		{
			//trampoline距离源函数地址太远了，用64位的方式跳，
			//这样原来的jmp offset是5字节，被改为ff 25 [offset]是6字节，而[offset]指向的64位地址放在trampoline预留的空间

			unsigned __int64 real = *(unsigned __int64*)addr;

			newcode[0] = 0xff;
			newcode[1] = (code[0] == 0xE8) ? 0x15 : 0x25;

			if (pTI->addr_count >= MAX_TrampolineInfo_JMPADDR)
				return -1;
			else
			{
				//设置[offset]的64位地址
				addr = GetTrampolineInfo_jmpaddr(pTI, pTI->addr_count);
				pTI->jmpaddr[pTI->addr_count] = (void*)real;
				pTI->addr_count++;

				offset64 = (__int64)(addr - (to + 6));
				if (offset64 < -0x7FFFFFFF || offset64 > 0x7FFFFFFF)
				{
					return -1;
				}else
				{
					*((long*)&newcode[2]) = (long)offset64;
					(*newlen) ++;
				}
			}

		}else
		{
			*((long*)&newcode[1]) = (long)offset64;
		}

#endif	

	}else if (code[0] == 0xff && code[1] == 0x25 && codeLen == 6)
	{
#ifdef _M_IX86

#else ifdef _M_AMD64
		long offset = *((unsigned long*)&code[2]);
		unsigned char *addr = (unsigned char *)(from + 6 + offset);

		//to+6 + X = addr
		__int64 offset64 = (__int64)(addr - (to + 6));

		if (offset64 < -0x7FFFFFFF || offset64 > 0x7FFFFFFF)
		{
			unsigned __int64 real = *(unsigned __int64*)addr;

			if (pTI->addr_count >= MAX_TrampolineInfo_JMPADDR)
				return -1;
			else
			{
				addr = GetTrampolineInfo_jmpaddr(pTI, pTI->addr_count);
				pTI->jmpaddr[pTI->addr_count] = (void*)real;
				pTI->addr_count++;

				offset64 = (__int64)(addr - (to + 6));
				if (offset64 < -0x7FFFFFFF || offset64 > 0x7FFFFFFF)
				{
					return -1;
				}else
				{
					*((long*)&newcode[2]) = (long)offset64;
				}
			}

		}else
		{
			*((long*)&newcode[2]) = (long)offset64;
		}
#endif	
	}

	return 0;
}

int fix_rvas(unsigned char *code,      int codeLen,
			 unsigned char *fixedcode, int *fixedcodeLen,
			 unsigned char *from,      unsigned char *to,
			 PTrampolineInfo pTI)
{

	_DecodeResult res;
	uint32_t decodedInstructionsCount = 0;
	_DInst di[MAX_INSTRUCTIONS];
	_CodeInfo ci;

	uint32_t x;

	ci.codeOffset = 0;
	ci.code = code;
	ci.codeLen = codeLen;
	ci.dt = constDecodeType;
	ci.features = DF_NONE;
	if (ci.dt == Decode16Bits) ci.features = DF_MAXIMUM_ADDR16;
	else if (ci.dt == Decode32Bits) ci.features = DF_MAXIMUM_ADDR32;

	res = distorm_decompose(&ci,	
		(_DInst*)&di, 
		MAX_INSTRUCTIONS,							 
		&decodedInstructionsCount	
		);

	if (res == DECRES_INPUTERR)
		return -1;

	*fixedcodeLen = 0;

	for (x = 0; x < decodedInstructionsCount; x++)
	{
		int n = di[x].size;
		int n2 = 0;

		if (0 != fix_rva(code, n, fixedcode, &n2, from, to, pTI))
			return -1;

		from += n;
		code += n;
		fixedcode += n2;
		to += n2;
		*fixedcodeLen += n2;
	}

	return 0;
}

#ifdef KERNEL_MODE

#ifdef _M_IX86

unsigned long __readcr0(void);
void __writecr0(unsigned);

#ifndef LONG_PTR
typedef long LONG_PTR;
#endif

#else ifdef _M_AMD64

unsigned __int64 __readcr0(void);
void __writecr0(unsigned __int64);

#ifndef LONG_PTR
typedef __int64 LONG_PTR;
#endif

#endif

#pragma intrinsic(__readcr0, __writecr0)


LONG_PTR SetCR0(LONG_PTR v)
{
	LONG_PTR prev = __readcr0();

	__writecr0(v);

	return prev;
}

void _SetCR0(LONG_PTR v)
{
	__writecr0(v);
}

LONG_PTR GetCR0()
{
	LONG_PTR ret = __readcr0();
	return ret;
}

LONG_PTR WriteProtectOff()
{
	LONG_PTR curr = GetCR0();
	return SetCR0(curr&(~0x10000));
}

LONG_PTR WriteProtectOn()
{
	LONG_PTR curr = GetCR0();
	return SetCR0(curr|0x10000);
}

#endif//KERNEL_MODE

unsigned char *findInsertable(unsigned char *target, int needLen)
{
	unsigned char *curr = target;
	unsigned char *prev = target;

	curr = (unsigned char *)get_final_addr(target, 0);

	if (0 == bakcode(needLen, constDecodeType, curr, 50, 0, 0))
		return curr;
	else
	{
		curr = target;
		for (;;)
		{
			if (0 == bakcode(needLen, constDecodeType, curr, 50, 0, 0))
				return curr;

			prev = curr;
			curr = (unsigned char *)get_final_addr(curr, 1);

			if (curr == prev)
				break;
		}

		//not found
		return 0;
	}
}

int hook_set(unsigned char *target, unsigned char *trampoline, void *new_handler)
{
	char jmpToBuf[sizeof(JMPER)+32];
	PJMPER pJmpTo = (PJMPER)&jmpToBuf[0];
	JMPER jmpBack;
	char bakins[128];
	char bakins_fixed[128];
	uint32_t bakLen = 0, bakLen_fixed = 0;
	LONG_PTR wp = 0;
	PTrampolineInfo pTI;
	TrampolineInfo ti;

	target = findInsertable(target, sizeof(JMPER));
	trampoline = (unsigned char *)get_final_addr(trampoline, 1);
	//new_handler = (unsigned char *)get_final_code(new_handler, 1);

	if (target == NULL)
		return -1;

	//pTI指向trampoline空间靠后的位置，这儿专门保留来保存hook的一些信息，
	//这空间就是DEFINE_TRAMPOLINE宏那里一堆__debugbreak();的指令空间
	pTI = (PTrampolineInfo)(trampoline+TRAMPOLINE_INFO_OFFSET);

	memset(&ti, 0, sizeof(ti));
	ti.this_addr = (unsigned char*)pTI;

	memset(jmpToBuf, 0x90, sizeof(jmpToBuf));
	init_jmper(pJmpTo, target, new_handler);

	if (0 != bakcode(sizeof(JMPER), constDecodeType, target, 50, (unsigned char*)&bakins[0], &bakLen))
		return -1;

	if (0 != fix_rvas((unsigned char*)&bakins[0], bakLen, (unsigned char*)&bakins_fixed[0], &bakLen_fixed, target, trampoline, &ti))
		return -1;

	init_jmper(&jmpBack, trampoline+bakLen_fixed, target+bakLen);
	memcpy(bakins_fixed+bakLen_fixed, &jmpBack, sizeof(JMPER));
	memcpy(bakins_fixed+bakLen_fixed+sizeof(JMPER), bakins, bakLen);
	/*
	trampoline 数据结构:
	    ----
		fixed original code,
		jmp back code
		backuped original code
		----
		有效长度 = bakLen_fixed+sizeof(JMPER)+bakLen;
	*/

	//填写trampoline相关信息
	ti.bakoffset = bakLen_fixed+sizeof(JMPER);
	ti.baklen = bakLen;
	ti.new_handler = new_handler;
	ti.target = target;

#ifdef KERNEL_MODE

	_disable();
	//wp off
	wp = WriteProtectOff();

	//将bakins_fixed中的有效长度写入到trampoline
	memcpy(trampoline, bakins_fixed, bakLen_fixed+sizeof(jmpBack)+bakLen);
	//目标函数写入跳转代码
	memcpy(target, pJmpTo, bakLen);
	//将trampoline相关信息写到trampoline空间末尾， 以便卸载hook
	memcpy(pTI, &ti, sizeof(ti));

	//restore wp
	_SetCR0(wp);
	_enable();

#else
	wp = wp;
	WriteProcessMemory(GetCurrentProcess(), trampoline, bakins_fixed, bakLen_fixed+sizeof(jmpBack)+bakLen, NULL);
	WriteProcessMemory(GetCurrentProcess(), target, pJmpTo, bakLen, NULL);
	WriteProcessMemory(GetCurrentProcess(), pTI, &ti, sizeof(ti), NULL);

#endif

	return 0;
}

int hook_remove(unsigned char *trampoline, void *new_handler)
{
	LONG_PTR wp = 0;
	PTrampolineInfo pTI;
	TrampolineInfo ti = {0};
	char bakins[128];

	trampoline = (unsigned char *)get_final_addr(trampoline, 1);
	//new_handler = (unsigned char *)get_final_code(new_handler, 0);

	//从trampoline空间取得相关信息
	pTI = (PTrampolineInfo)(trampoline+TRAMPOLINE_INFO_OFFSET);

	if (pTI->new_handler == new_handler)
	{
		//从trampoline空间取得hook前备份的代码
		memcpy(bakins, trampoline+pTI->bakoffset, (char)pTI->baklen);

#ifdef KERNEL_MODE
		//wp off
		_disable();
		wp = WriteProtectOff();

		memcpy(pTI->target, bakins, pTI->baklen);
		memcpy(pTI, &ti, sizeof(ti));

		//restore wp
		_SetCR0(wp);
		_enable();

#else
		wp = wp;
		WriteProcessMemory(GetCurrentProcess(), pTI->target, bakins, pTI->baklen, NULL);
		WriteProcessMemory(GetCurrentProcess(), pTI, &ti, sizeof(ti), NULL);

#endif

		return 0;
	}else
	{
		return -1;
	}

}
