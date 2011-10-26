// MistAHPack.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "sdk\SDK.h"

// Unpacker Data:
bool MapFileEx(char* szFileName, DWORD ReadOrWrite, LPHANDLE FileHandle, LPDWORD FileSize, LPHANDLE FileMap, LPDWORD FileMapVA, DWORD SizeModifier){

	HANDLE hFile = 0;
	DWORD FileAccess = 0;
	DWORD FileMapType = 0;
	DWORD FileMapViewType = 0;
	DWORD mfFileSize = 0;
	HANDLE mfFileMap = 0;
	LPVOID mfFileMapVA = 0;

	if(ReadOrWrite == UE_ACCESS_READ){
		FileAccess = GENERIC_READ;
		FileMapType = 2;
		FileMapViewType = 4;
	}else if(ReadOrWrite == UE_ACCESS_WRITE){
		FileAccess = GENERIC_WRITE;
		FileMapType = 4;
		FileMapViewType = 2;
	}else if(ReadOrWrite == UE_ACCESS_ALL){
		FileAccess = GENERIC_READ+GENERIC_WRITE;
		FileMapType = 4;
		FileMapViewType = 2;
	}else{
		FileAccess = GENERIC_READ+GENERIC_WRITE;
		FileMapType = 4;
		FileMapViewType = 2;
	}

	hFile = CreateFileA(szFileName, FileAccess, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE){
		*FileHandle = hFile;
		mfFileSize = GetFileSize(hFile,NULL);
		mfFileSize = mfFileSize + SizeModifier;
		*FileSize = mfFileSize;
		mfFileMap = CreateFileMappingA(hFile, NULL, FileMapType, NULL, mfFileSize, NULL);
		if(mfFileMap != NULL){
			*FileMap = mfFileMap;
			mfFileMapVA = MapViewOfFile(mfFileMap, FileMapViewType, NULL, NULL, NULL);
			if(mfFileMapVA != NULL){
				*FileMapVA = (DWORD)mfFileMapVA;
				return(true);
			}
		}
		*FileMapVA = NULL;
		*FileHandle = NULL;
		*FileSize = NULL;
		CloseHandle(hFile);
	}else{
		*FileMapVA = NULL;
	}
	return(false);
}
void UnMapFileEx(HANDLE FileHandle, DWORD FileSize, HANDLE FileMap, DWORD FileMapVA){

	LPVOID ufFileMapVA = (LPDWORD)FileMapVA;

	UnmapViewOfFile(ufFileMapVA);
	CloseHandle(FileMap);
	SetFilePointer(FileHandle,FileSize,NULL,FILE_BEGIN);
	SetEndOfFile(FileHandle);
	CloseHandle(FileHandle);
}
__declspec(dllexport) bool __cdecl unpack(char* szInputFileName, char* szOutputFileName, void* reserved){

	int i = NULL;
	int j = NULL;
	int k = NULL;
	bool Critical = false;
	long uSectionNumber;
	long OriginalFileSize;
	long uFirstSectionDelta;
	long uFirstSectionSize;
	long uFirstSectionRawSize;
	long uFileSectionAlignment;
	long uFirstSectionDataPointer;
	PIMAGE_IMPORT_DESCRIPTOR ImportIID;
	FILE_STATUS_INFO inFileStatus = {};
	long fdImageBase = NULL;
	long fdLoadedBase = NULL;
	long fdEntryPoint = NULL;
	long fdSizeOfImage = NULL;
	void* DecompressedMemory;
	void* ResizeFileMemory;
	ULONG_PTR uReadLocation;
	ULONG_PTR uEPLocation;
	DWORD uIATLocation;
	DWORD uOEPLocation;
	ULONG_PTR FileMapVA;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;

	BYTE uIATPattern[] = {0x61,0xB9,0x00,0x00,0x00,0x00,0x8B,0x1C,0x08,0x89,0x99,0x00,0x00,0x00,0x00,0xE2,0xF5,0x90,0x90,0xBA,0x00,0x00,0x00,0x00,0xBE,0x00,0x00,0x00,0x00,0x01,0xD6};
	BYTE uOEPPattern[] = {0x8B,0x15,0x00,0x00,0x00,0x00,0x52,0xFF,0xD0,0x61};

	if(szInputFileName != NULL && szOutputFileName != NULL){
		if(IsPE32FileValidEx(szInputFileName, UE_DEPTH_DEEP, &inFileStatus)){
			fdImageBase = (long)GetPE32Data(szInputFileName, NULL, UE_IMAGEBASE);
			fdEntryPoint = (long)GetPE32Data(szInputFileName, NULL, UE_OEP);
			fdSizeOfImage = (long)GetPE32Data(szInputFileName, NULL, UE_SIZEOFIMAGE);		
			if(CopyFileA(szInputFileName, szOutputFileName, false)){
				if(StaticFileLoad(szOutputFileName, UE_ACCESS_READ, false, &FileHandle, &FileSize, &FileMap, &FileMapVA)){
					uSectionNumber = (long)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_SECTIONNUMBER);
					uFirstSectionSize = (long)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_SECTIONVIRTUALSIZE);
					uFirstSectionRawSize = (long)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_SECTIONRAWSIZE);
					uFirstSectionDataPointer = (long)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_SECTIONRAWOFFSET);
					uFileSectionAlignment = (long)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_SECTIONALIGNMENT);
					if(uFirstSectionSize % uFileSectionAlignment != 0){
						uFirstSectionSize = ((uFirstSectionSize / uFileSectionAlignment) + 1) * uFileSectionAlignment;
					}
					OriginalFileSize = FileSize;
					uFirstSectionDelta = uFirstSectionSize - uFirstSectionRawSize;
					DecompressedMemory = VirtualAlloc(NULL, (SIZE_T)uFirstSectionSize, MEM_COMMIT, PAGE_READWRITE);
					if(DecompressedMemory != NULL){
						if(StaticMemoryDecompress((void*)(uFirstSectionDataPointer + FileMapVA), (DWORD)uFirstSectionRawSize, DecompressedMemory, (DWORD)uFirstSectionSize, UE_STATIC_APLIB)){
							StaticFileUnload(szOutputFileName, false, FileHandle, FileSize, FileMap, FileMapVA);
							if(MapFileEx(szOutputFileName, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, uFirstSectionDelta)){
								ResizeFileMemory = VirtualAlloc(NULL, (SIZE_T)FileSize, MEM_COMMIT, PAGE_READWRITE);
								if(ResizeFileMemory != NULL){
									RtlMoveMemory(ResizeFileMemory, (void*)(FileMapVA + uFirstSectionDataPointer + uFirstSectionRawSize), OriginalFileSize - (uFirstSectionDataPointer + uFirstSectionRawSize));
									RtlZeroMemory((void*)(FileMapVA + uFirstSectionDataPointer + uFirstSectionRawSize), OriginalFileSize - (uFirstSectionDataPointer + uFirstSectionRawSize));
									RtlMoveMemory((void*)(FileMapVA + uFirstSectionDataPointer + uFirstSectionRawSize + uFirstSectionDelta), ResizeFileMemory, OriginalFileSize - (uFirstSectionDataPointer + uFirstSectionRawSize));
									RtlMoveMemory((void*)(FileMapVA + uFirstSectionDataPointer), DecompressedMemory, uFirstSectionSize);
									for(int n = 0; n < uSectionNumber; n++){
										if(n == 0){
											SetPE32DataForMappedFile(FileMapVA, n, UE_SECTIONRAWSIZE, uFirstSectionSize);
										}else{
											SetPE32DataForMappedFile(FileMapVA, n, UE_SECTIONRAWOFFSET, (ULONG_PTR)(GetPE32DataFromMappedFile(FileMapVA, n, UE_SECTIONRAWOFFSET) + uFirstSectionDelta));
										}
									}
									VirtualFree(ResizeFileMemory, NULL, MEM_RELEASE);
									uEPLocation = (ULONG_PTR)ConvertVAtoFileOffset(FileMapVA, fdEntryPoint + fdImageBase, true);
									if(uEPLocation != NULL){
										uReadLocation = (ULONG_PTR)FindEx(GetCurrentProcess(), (void*)uEPLocation, 0x200, &uIATPattern[0], sizeof uIATPattern / sizeof uIATPattern[0], NULL);
										if(uReadLocation != NULL){
											k++;
											uReadLocation = uReadLocation + 0x19;
											RtlMoveMemory(&uIATLocation, (void*)uReadLocation, sizeof uIATLocation);
											ImportIID = (PIMAGE_IMPORT_DESCRIPTOR)ConvertVAtoFileOffset(FileMapVA, uIATLocation + fdImageBase, true);
											while(ImportIID->FirstThunk != NULL){
												ImportIID = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG_PTR)ImportIID + sizeof IMAGE_IMPORT_DESCRIPTOR);
												k = k + sizeof IMAGE_IMPORT_DESCRIPTOR;
											}
											SetPE32DataForMappedFile(FileMapVA, NULL, UE_IMPORTTABLEADDRESS, uIATLocation);
											SetPE32DataForMappedFile(FileMapVA, NULL, UE_IMPORTTABLESIZE, k);
										}else{
											Critical = true;
										}
										uReadLocation = (ULONG_PTR)FindEx(GetCurrentProcess(), (void*)uEPLocation, 0x200, &uOEPPattern[0], sizeof uOEPPattern / sizeof uOEPPattern[0], NULL);
										if(uReadLocation != NULL){
											uReadLocation = uReadLocation + (sizeof uOEPPattern / sizeof uOEPPattern[0]) + 1;
											RtlMoveMemory(&uOEPLocation, (void*)uReadLocation, sizeof uOEPLocation);
											uOEPLocation = uOEPLocation - fdImageBase;
											SetPE32DataForMappedFile(FileMapVA, NULL, UE_OEP, uOEPLocation);
										}else{
											Critical = true;
										}
									}else{
										Critical = true;
									}
								}else{
									Critical = true;
								}
								VirtualFree(DecompressedMemory, NULL, MEM_RELEASE);
								UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
								if(!DeleteLastSection(szOutputFileName)){
									Critical = true;
								}else if(!Critical){
									return(true);
								}
							}
						}else{
							Critical = true;
						}
					}else{
						StaticFileUnload(szOutputFileName, false, FileHandle, FileSize, FileMap, FileMapVA);
						Critical = true;
					}
				}
			}
		}
		if(Critical){
			DeleteFileA(szOutputFileName);
		}
	}
	return(false);
}