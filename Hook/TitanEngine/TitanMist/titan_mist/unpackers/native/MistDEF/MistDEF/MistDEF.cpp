// MistDEF.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "SDK\SDK.h"

typedef struct MODULE_FILE_MAPPING_INFO{
	ULONG_PTR FileMapVA;
	HANDLE FileHandle;
	HANDLE FileMap;
	DWORD FileSize;
}MODULE_FILE_MAPPING_INFO, *LPMODULE_FILE_MAPPING_INFO;

typedef struct MEMORY_COMPARE_HANDLER{
	union {
		BYTE bArrayEntry[1];		
		WORD wArrayEntry[1];
		DWORD dwArrayEntry[1];
		DWORD64 qwArrayEntry[1];
	} Array;
}MEMORY_COMPARE_HANDLER, *PMEMORY_COMPARE_HANDLER;

__declspec(dllexport) bool __cdecl unpack(char* szInputFileName, char* szOutputFileName, void* reserved){

	DWORD DEFUnpackedOEP;
	int DefNumberOfSections;
	DWORD SizeOfDecryptionSection;
	PMEMORY_COMPARE_HANDLER ptrFileMemory;
	MODULE_FILE_MAPPING_INFO FileMappingInfo = {};
	char szOriginalFile[MAX_PATH] = {};
	char szSectionName[8] = {};

	if(CopyFileA(szInputFileName, szOutputFileName, FALSE)){
		if(StaticFileLoad(szOutputFileName, UE_ACCESS_ALL, false, &FileMappingInfo.FileHandle, &FileMappingInfo.FileSize, &FileMappingInfo.FileHandle, &FileMappingInfo.FileMapVA)){
			DefNumberOfSections = (int)GetPE32DataFromMappedFile(FileMappingInfo.FileMapVA, NULL, UE_SECTIONNUMBER);
			for(int i = 0; i < DefNumberOfSections; i++){
				RtlMoveMemory(&szSectionName[0], (char*)GetPE32DataFromMappedFile(FileMappingInfo.FileMapVA, i, UE_SECTIONNAME), sizeof szSectionName);
				if(szSectionName[sizeof szSectionName - 1] == 0x01){
					SizeOfDecryptionSection = (DWORD)GetPE32DataFromMappedFile(FileMappingInfo.FileMapVA, i, UE_SECTIONRAWSIZE);
					ptrFileMemory = (PMEMORY_COMPARE_HANDLER)(FileMappingInfo.FileMapVA + (ULONG_PTR)GetPE32DataFromMappedFile(FileMappingInfo.FileMapVA, i, UE_SECTIONRAWOFFSET));
					if(SizeOfDecryptionSection < FileMappingInfo.FileSize){
						__try{
							while(SizeOfDecryptionSection > NULL){
								ptrFileMemory->Array.bArrayEntry[0] = ptrFileMemory->Array.bArrayEntry[0] ^ (BYTE)SizeOfDecryptionSection;
								ptrFileMemory = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)ptrFileMemory + 1);
								SizeOfDecryptionSection--;
							}
						}__except(EXCEPTION_EXECUTE_HANDLER){
							StaticFileUnload(szOutputFileName, true, FileMappingInfo.FileHandle, FileMappingInfo.FileSize, FileMappingInfo.FileHandle, FileMappingInfo.FileMapVA);
							DeleteFileA(szOutputFileName);
							return(false);
						}
					}
				}
			}
			__try{
				RtlMoveMemory(&DEFUnpackedOEP, (void*)((ULONG_PTR)ConvertVAtoFileOffset(FileMappingInfo.FileMapVA, (ULONG_PTR)(GetPE32DataFromMappedFile(FileMappingInfo.FileMapVA, NULL, UE_IMAGEBASE) + GetPE32DataFromMappedFile(FileMappingInfo.FileMapVA, NULL, UE_OEP)), true) + 0x25), sizeof DEFUnpackedOEP);
			}__except(EXCEPTION_EXECUTE_HANDLER){
				StaticFileUnload(szOutputFileName, true, FileMappingInfo.FileHandle, FileMappingInfo.FileSize, FileMappingInfo.FileHandle, FileMappingInfo.FileMapVA);
				DeleteFileA(szOutputFileName);
				return(false);
			}
			SetPE32DataForMappedFile(FileMappingInfo.FileMapVA, NULL, UE_OEP, DEFUnpackedOEP - (DWORD)GetPE32DataFromMappedFile(FileMappingInfo.FileMapVA, NULL, UE_IMAGEBASE));
			StaticFileUnload(szOutputFileName, true, FileMappingInfo.FileHandle, FileMappingInfo.FileSize, FileMappingInfo.FileHandle, FileMappingInfo.FileMapVA);
			return(true);
		}
	}
	return(false);
}



