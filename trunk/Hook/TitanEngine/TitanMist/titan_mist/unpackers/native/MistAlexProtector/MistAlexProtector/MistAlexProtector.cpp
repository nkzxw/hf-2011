// MistAlexProtector.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "sdk\SDK.h"
#include <vector>

typedef struct MEMORY_COMPARE_HANDLER{
	union {
		BYTE bArrayEntry[1];		
		WORD wArrayEntry[1];
		DWORD dwArrayEntry[1];
		DWORD64 qwArrayEntry[1];
	} Array;
}MEMORY_COMPARE_HANDLER, *PMEMORY_COMPARE_HANDLER;

typedef struct AlexProt_SectionData{
	DWORD SectionVirtualOffset;
	DWORD SectionVirtualSize;
}AlexProt_SectionData, *PAlexProt_SectionData;

typedef struct AlexProt_API_Entry{
	char* szAPIName;
	DWORD EliminatedAddress;
}AlexProt_API_Entry, *PAlexProt_API_Entry;

// Unpacker Data:
std::vector<AlexProt_API_Entry> apiEntry;

__declspec(dllexport) bool __cdecl unpack(char* szInputFileName, char* szOutputFileName, void* reserved){

	int i;
	DWORD FileSize;
	HANDLE FileMap;
	HANDLE FileHandle;
	ULONG_PTR FileMapVA;
	DWORD ItemTableSize;
	DWORD NewSectionSize;
	DWORD ItemStringSize;
	DWORD ItemWriteAddress;
	DWORD AlexProtectorDelta;
	DWORD AlexProtectorEPAddress;
	void* AlexProtectorSectionData;
	DWORD AlexProtectorDecryptSize;
	DWORD AlexProtectorDecryptedSize;
	DWORD AlexProtectorVirtualIAT = -4;
	ULONG_PTR AlexProtectorCompressedData;
	PMEMORY_COMPARE_HANDLER myMemoryDecryptor;
	PAlexProt_SectionData myAlexSection;
	FILE_STATUS_INFO myFileStatus = {};
	AlexProt_API_Entry myAPIEntry = {};
	char LoggingBuffer[1024] = {};
	PE32Struct myPEData = {};

	apiEntry.clear();
	if(szInputFileName != NULL){
		if(IsPE32FileValidEx(szInputFileName, UE_DEPTH_DEEP, &myFileStatus)){
			if(StaticFileLoad(szInputFileName, UE_ACCESS_READ, true, &FileHandle, &FileSize, &FileMap, &FileMapVA) && GetPE32DataEx(szInputFileName, &myPEData)){
				if(myPEData.NtSizeOfImage % myPEData.SectionAligment != NULL){
					myPEData.NtSizeOfImage = (((myPEData.NtSizeOfImage / myPEData.SectionAligment) + 1) * myPEData.SectionAligment);
				}
				DumpMemory(GetCurrentProcess(), (void*)FileMapVA, myPEData.NtSizeOfImage, szOutputFileName);
				StaticFileUnload(szInputFileName, false, FileHandle, FileSize, FileMap, FileMapVA);
				if(StaticFileLoad(szOutputFileName, UE_ACCESS_ALL, false, &FileHandle, &FileSize, &FileMap, &FileMapVA)){
					SetPE32DataForMappedFile(FileMapVA, NULL, UE_SIZEOFIMAGE, myPEData.NtSizeOfImage);
					for(i = 0; i < myPEData.SectionNumber; i++){
						NewSectionSize = (DWORD)GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONVIRTUALSIZE);
						if(NewSectionSize % myPEData.SectionAligment != NULL){
							NewSectionSize = (((NewSectionSize / myPEData.SectionAligment) + 1) * myPEData.SectionAligment);
						}
						SetPE32DataForMappedFile(FileMapVA, i, UE_SECTIONRAWOFFSET, (ULONG_PTR)GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONVIRTUALOFFSET));
						SetPE32DataForMappedFile(FileMapVA, i, UE_SECTIONRAWSIZE, NewSectionSize);
					}
					__try{
						AlexProtectorDelta = (DWORD)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_OEP) - 0x1000;
						myAlexSection = (PAlexProt_SectionData)ConvertVAtoFileOffset(FileMapVA, AlexProtectorDelta + 0x4023C1, true);
						while(myAlexSection->SectionVirtualOffset != NULL){
							if(myAlexSection->SectionVirtualOffset < myPEData.NtSizeOfImage && myAlexSection->SectionVirtualSize < myPEData.NtSizeOfImage){
								AlexProtectorSectionData = VirtualAlloc(NULL, myAlexSection->SectionVirtualSize, MEM_COMMIT, PAGE_READWRITE);
								AlexProtectorCompressedData = (ULONG_PTR)GetPE32DataFromMappedFile(FileMapVA, GetPE32SectionNumberFromVA(FileMapVA, (ULONG_PTR)(myAlexSection->SectionVirtualOffset + myPEData.ImageBase)), UE_SECTIONRAWOFFSET) + FileMapVA;
								if(!StaticMemoryDecompress((void*)AlexProtectorCompressedData, myAlexSection->SectionVirtualSize, AlexProtectorSectionData, myAlexSection->SectionVirtualSize, UE_STATIC_APLIB)){
									StaticFileUnload(szOutputFileName, false, FileHandle, FileSize, FileMap, FileMapVA);
									DeleteFileA(szOutputFileName);
									return(false);
								}
								RtlMoveMemory((void*)AlexProtectorCompressedData, AlexProtectorSectionData, myAlexSection->SectionVirtualSize);
								VirtualFree(AlexProtectorSectionData, NULL, MEM_RELEASE);
								myAlexSection = (PAlexProt_SectionData)((ULONG_PTR)myAlexSection + sizeof AlexProt_SectionData);
							}else{
								StaticFileUnload(szOutputFileName, false, FileHandle, FileSize, FileMap, FileMapVA);
								DeleteFileA(szOutputFileName);
								return(false);
							}
						}
					}__except(1){
						StaticFileUnload(szOutputFileName, false, FileHandle, FileSize, FileMap, FileMapVA);
						DeleteFileA(szOutputFileName);
						return(false);
					}
					myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)ConvertVAtoFileOffset(FileMapVA, AlexProtectorDelta + 0x4023B9, true);
					AlexProtectorDecryptSize = myMemoryDecryptor->Array.dwArrayEntry[0];
					myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)ConvertVAtoFileOffset(FileMapVA, AlexProtectorDelta + 0x4023B5, true);
					myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)(ConvertVAtoFileOffset(FileMapVA, AlexProtectorDelta + 0x402531, true)) + myMemoryDecryptor->Array.dwArrayEntry[0]);
					AlexProtectorCompressedData = (ULONG_PTR)myMemoryDecryptor;
					i = AlexProtectorDecryptSize;
					while(i > NULL){
						myMemoryDecryptor->Array.bArrayEntry[0] = myMemoryDecryptor->Array.bArrayEntry[0] ^ 0x7D;
						myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)myMemoryDecryptor + 1);
						i--;
					}
					AlexProtectorSectionData = VirtualAlloc(NULL, AlexProtectorDecryptSize * 10, MEM_COMMIT, PAGE_READWRITE);
					if(StaticMemoryDecompress((void*)AlexProtectorCompressedData, AlexProtectorDecryptSize, AlexProtectorSectionData, AlexProtectorDecryptSize * 10, UE_STATIC_APLIB)){
						myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)AlexProtectorSectionData;
						ImporterInit(50 * 1024, myPEData.ImageBase);
						while(myMemoryDecryptor->Array.bArrayEntry[0] != 0x00){
							if(myMemoryDecryptor->Array.bArrayEntry[0] == 0xC3){
								myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)myMemoryDecryptor + 1);
								ItemStringSize = myMemoryDecryptor->Array.bArrayEntry[0];
								myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)myMemoryDecryptor + 1);
								ImporterAddNewDll((char*)myMemoryDecryptor, NULL);
								myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)myMemoryDecryptor + ItemStringSize + 1);
								AlexProtectorVirtualIAT = AlexProtectorVirtualIAT + 4;
							}else if(myMemoryDecryptor->Array.bArrayEntry[0] == 0xC4){
								myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)myMemoryDecryptor + 1);
								ImporterAddNewOrdinalAPI(myMemoryDecryptor->Array.dwArrayEntry[0], AlexProtectorVirtualIAT);
								myAPIEntry.szAPIName = (char*)myMemoryDecryptor->Array.dwArrayEntry[0];
								myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)myMemoryDecryptor + 4);
								ItemTableSize = myMemoryDecryptor->Array.bArrayEntry[0];
								myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)myMemoryDecryptor + 1);
								while((int)ItemTableSize > NULL){
									myAPIEntry.EliminatedAddress = myMemoryDecryptor->Array.dwArrayEntry[0];
									myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)myMemoryDecryptor + 4);
									apiEntry.push_back(myAPIEntry);
									ItemTableSize--;
								}
								AlexProtectorVirtualIAT = AlexProtectorVirtualIAT + 4;
							}else{
								ItemStringSize = myMemoryDecryptor->Array.bArrayEntry[0];
								myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)myMemoryDecryptor + 1);
								myAPIEntry.szAPIName = (char*)myMemoryDecryptor;
								ImporterAddNewAPI((char*)myMemoryDecryptor, AlexProtectorVirtualIAT);
								myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)myMemoryDecryptor + ItemStringSize + 1);
								ItemTableSize = myMemoryDecryptor->Array.bArrayEntry[0];
								myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)myMemoryDecryptor + 1);
								while((int)ItemTableSize > NULL){
									myAPIEntry.EliminatedAddress = myMemoryDecryptor->Array.dwArrayEntry[0];
									myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)myMemoryDecryptor + 4);
									apiEntry.push_back(myAPIEntry);
									ItemTableSize--;
								}
								AlexProtectorVirtualIAT = AlexProtectorVirtualIAT + 4;
							}
						}
						ImporterMoveIAT();
						ImporterRelocateWriteLocation(myPEData.NtSizeOfImage + myPEData.ImageBase);
						for(i = 0; i < (int)apiEntry.size(); i++){
							myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)ConvertVAtoFileOffset(FileMapVA, apiEntry[i].EliminatedAddress, true));
							ItemWriteAddress = (ULONG_PTR)ImporterFindAPIWriteLocation(apiEntry[i].szAPIName);
							myMemoryDecryptor->Array.dwArrayEntry[0] = ItemWriteAddress;
						}
						StaticFileUnload(szOutputFileName, false, FileHandle, FileSize, FileMap, FileMapVA);
						ImporterExportIATEx(szOutputFileName, ".alexIAT");
						VirtualFree(AlexProtectorSectionData, NULL, MEM_RELEASE);

						if(StaticFileLoad(szOutputFileName, UE_ACCESS_ALL, false, &FileHandle, &FileSize, &FileMap, &FileMapVA)){
							myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)ConvertVAtoFileOffset(FileMapVA, AlexProtectorDelta + 0x402385, true);
							AlexProtectorEPAddress = myMemoryDecryptor->Array.dwArrayEntry[0];
							myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)ConvertVAtoFileOffset(FileMapVA, AlexProtectorDelta + 0x402395, true);
							AlexProtectorDecryptSize = myMemoryDecryptor->Array.dwArrayEntry[0];
							myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)ConvertVAtoFileOffset(FileMapVA, AlexProtectorDelta + 0x402391, true);
							AlexProtectorDecryptedSize = myMemoryDecryptor->Array.dwArrayEntry[0];
							myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)ConvertVAtoFileOffset(FileMapVA, AlexProtectorDelta + 0x4023B9, true);
							ItemTableSize = myMemoryDecryptor->Array.dwArrayEntry[0];
							myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)ConvertVAtoFileOffset(FileMapVA, AlexProtectorDelta + 0x4023B5, true);
							myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)(ConvertVAtoFileOffset(FileMapVA, AlexProtectorDelta + 0x402531, true)) + myMemoryDecryptor->Array.dwArrayEntry[0] + ItemTableSize);
							AlexProtectorSectionData = VirtualAlloc(NULL, AlexProtectorDecryptedSize, MEM_COMMIT, PAGE_READWRITE);
							AlexProtectorCompressedData = (ULONG_PTR)myMemoryDecryptor;
							if(StaticMemoryDecompress((void*)AlexProtectorCompressedData, AlexProtectorDecryptSize, AlexProtectorSectionData, AlexProtectorDecryptedSize, UE_STATIC_APLIB)){
								myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)AlexProtectorSectionData + AlexProtectorDecryptedSize);
								myMemoryDecryptor->Array.bArrayEntry[0] = 0x68;
								myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)myMemoryDecryptor + 1);
								myMemoryDecryptor->Array.dwArrayEntry[0] = AlexProtectorEPAddress;
								myMemoryDecryptor = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)myMemoryDecryptor + 4);
								myMemoryDecryptor->Array.bArrayEntry[0] = 0xC3;
								StaticFileUnload(szOutputFileName, false, FileHandle, FileSize, FileMap, FileMapVA);
								SetPE32Data(szOutputFileName, NULL, UE_OEP, (ULONG_PTR)AddNewSectionEx(szOutputFileName, ".alexEP", AlexProtectorDecryptSize, NULL, AlexProtectorSectionData, AlexProtectorDecryptedSize + 6));
								if(StaticFileLoad(szOutputFileName, UE_ACCESS_ALL, false, &FileHandle, &FileSize, &FileMap, &FileMapVA)){
									FileSize = RealignPE(FileMapVA, FileSize, 2);
									StaticFileUnload(szOutputFileName, false, FileHandle, FileSize, FileMap, FileMapVA);
									return(true);
								}
							}else{
								StaticFileUnload(szOutputFileName, false, FileHandle, FileSize, FileMap, FileMapVA);
								DeleteFileA(szOutputFileName);
								return(false);
							}
							VirtualFree(AlexProtectorSectionData, NULL, MEM_RELEASE);
						}
					}else{
						StaticFileUnload(szOutputFileName, false, FileHandle, FileSize, FileMap, FileMapVA);
						DeleteFileA(szOutputFileName);
						return(false);
					}
				}
			}
		}
	}
	return(false);
}