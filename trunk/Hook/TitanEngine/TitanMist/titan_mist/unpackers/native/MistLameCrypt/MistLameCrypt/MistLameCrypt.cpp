// MistLameCrypt.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "SDK\SDK.h"

__declspec(dllexport) bool __cdecl unpack(char* szInputFileName, char* szOutputFileName, void* reserved){

	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	DWORD UnpackedOEP;

	if(CopyFileA(szInputFileName, szOutputFileName, false)){
		if(StaticFileLoad(szOutputFileName, UE_ACCESS_ALL, false, &FileHandle, &FileSize, &FileMap, &FileMapVA)){
			RtlMoveMemory(&UnpackedOEP, (void*)(ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_OEP) + (ULONG_PTR)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_IMAGEBASE), true) + 0x19), 4);
			UnpackedOEP = UnpackedOEP - (DWORD)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_IMAGEBASE);
			StaticSectionDecrypt(FileMapVA, NULL, false, UE_STATIC_DECRYPTOR_XOR, UE_STATIC_KEY_SIZE_1, 0x90);
			SetPE32DataForMappedFile(FileMapVA, NULL, UE_OEP, (ULONG_PTR)UnpackedOEP);
			StaticFileUnload(szOutputFileName, true, FileHandle, FileSize, FileMap, FileMapVA);
			DeleteLastSection(szOutputFileName);
			return(true);
		}
	}
	return(false);
}