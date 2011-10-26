// MistMEW5.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "SDK\SDK.h"

typedef struct MODULE_FILE_MAPPING_INFO{
	ULONG_PTR FileMapVA;
	HANDLE FileHandle;
	HANDLE FileMap;
	DWORD FileSize;
}MODULE_FILE_MAPPING_INFO, *LPMODULE_FILE_MAPPING_INFO;

bool moduleMEW5StaticCallBack(void* MemoryStart, int KeySize){

	BYTE mewProcessByte;

	__try{
		RtlMoveMemory(&mewProcessByte, MemoryStart, KeySize);
		//mewProcessByte = _rotl8((BYTE)mewProcessByte, 0x29);
		//mewProcessByte = mewProcessByte + 0xBA;
		//mewProcessByte = _rotr8((BYTE)mewProcessByte, 0x50);
		__asm{
			PUSH EAX
			MOVZX EAX,mewProcessByte
   		    ROL AL,0x29
		    ADD AL,0xBA
		    ROR AL,0x50
			MOV mewProcessByte,AL
			POP EAX
		};
		RtlMoveMemory(MemoryStart, &mewProcessByte, KeySize);
	}__except(EXCEPTION_EXECUTE_HANDLER){
		return(false);
	}
	return(true);
}

__declspec(dllexport) bool __cdecl unpack(char* szInputFileName, char* szOutputFileName, void* reserved){

	DWORD mew5UnpackedOEP;
	DWORD mew5DataPointer;
	DWORD mew5DecryptStart;
	DWORD mew5DecryptSize;
	MODULE_FILE_MAPPING_INFO FileMappingInfo = {};

	if(CopyFileA(szInputFileName, szOutputFileName, FALSE)){
		if(StaticFileLoad(szInputFileName, UE_ACCESS_ALL, false, &FileMappingInfo.FileHandle, &FileMappingInfo.FileSize, &FileMappingInfo.FileHandle, &FileMappingInfo.FileMapVA)){
			RtlMoveMemory(&mew5DataPointer, (void*)((ULONG_PTR)ConvertVAtoFileOffset(FileMappingInfo.FileMapVA, (ULONG_PTR)GetPE32DataFromMappedFile(FileMappingInfo.FileMapVA, NULL, UE_OEP) + (ULONG_PTR)GetPE32DataFromMappedFile(FileMappingInfo.FileMapVA, NULL, UE_IMAGEBASE) + 1, true)), sizeof mew5DataPointer);
			mew5DataPointer = (DWORD)ConvertVAtoFileOffset(FileMappingInfo.FileMapVA, mew5DataPointer, true);
			RtlMoveMemory(&mew5DecryptSize, (void*)(mew5DataPointer), sizeof mew5DecryptSize);
			RtlMoveMemory(&mew5UnpackedOEP, (void*)(mew5DataPointer + 4), sizeof mew5UnpackedOEP);
			RtlMoveMemory(&mew5DecryptStart, (void*)(mew5DataPointer + 8), sizeof mew5DecryptStart);
			mew5DecryptStart = (DWORD)ConvertVAtoFileOffset(FileMappingInfo.FileMapVA, mew5DecryptStart, true);
			StaticMemoryDecryptEx((void*)mew5DecryptStart, mew5DecryptSize, UE_STATIC_KEY_SIZE_1, moduleMEW5StaticCallBack);
			mew5UnpackedOEP = mew5UnpackedOEP - (DWORD)GetPE32DataFromMappedFile(FileMappingInfo.FileMapVA, NULL, UE_IMAGEBASE);
			SetPE32DataForMappedFile(FileMappingInfo.FileMapVA, NULL, UE_OEP, mew5UnpackedOEP);
			StaticFileUnload(szOutputFileName, true, FileMappingInfo.FileHandle, FileMappingInfo.FileSize, FileMappingInfo.FileHandle, FileMappingInfo.FileMapVA);
			return(true);
		}
	}
	return(false);
}
