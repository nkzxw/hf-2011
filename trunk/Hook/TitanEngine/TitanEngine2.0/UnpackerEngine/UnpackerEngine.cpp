//
// TitanEngine SDK 2.0
// UnpackerEngine.cpp : Defines the exported functions for the DLL application.
//

// Global.constants
#include "stdafx.h"
// Disassembler.engine
#include "distorm.h"
// Windows libs
#include <Imagehlp.h>
#include <stdlib.h>
#include <psapi.h>
#include <time.h>
// Global.Engine:
#include "definitions.h"

// Global.variables:
LPSTR szSharedOverlay = 0;
STARTUPINFOA dbgStartupInfo = {};
PROCESS_INFORMATION dbgProcessInformation = {};
DWORD DBGCode = DBG_CONTINUE;
DWORD CurrentExceptionsNumber = 0;
int BreakPointSetCount = 0;
BreakPointDetail BreakPointBuffer[MAXIMUM_BREAKPOINTS] = {};
BYTE INT3BreakPoint = 0xCC;
BYTE INT3LongBreakPoint[2] = {0xCD, 0x03};
BYTE UD2BreakPoint[2] = {0x0F, 0x0B};
CustomHandler myDBGCustomHandler = {};
PCustomHandler DBGCustomHandler = &myDBGCustomHandler;
DEBUG_EVENT DBGEvent = {};
DEBUG_EVENT TerminateDBGEvent = {};
CONTEXT DBGContext = {};
HANDLE DBGFileHandle;
DWORD ProcessExitCode = 0;
LPVOID hListProcess = 0;
LPVOID hListThread = 0;
LPVOID hListLibrary = 0;
ULONG_PTR impDeltaStart = NULL;
ULONG_PTR impDeltaCurrent = NULL;
ULONG_PTR impImageBase = 0;
DWORD impAllocSize = 20 * 1024;
DWORD impDLLNumber = 0;
bool impMoveIAT = false;
ULONG_PTR impDLLDataList[1000][2];
ULONG_PTR impDLLStringList[1000][2];
ULONG_PTR impOrdinalList[1000][2];
LPVOID expTableData = NULL;
LPVOID expTableDataCWP = NULL;
ULONG_PTR expImageBase = 0;
DWORD expExportNumber = 0;
bool expNamePresent = false;
DWORD expExportAddress[1000];
DWORD expNamePointers[1000];
DWORD expNameHashes[1000];
WORD expOrdinals[1000];
IMAGE_EXPORT_DIRECTORY expExportData;
ULONG_PTR tlsCallBackList[100];
int engineCurrentPlatform = UE_PLATFORM_x86;
ULONG_PTR engineTLSBreakOnCallBackAddress;
bool engineBackupTLSx64 = false;
bool engineTLSBreakOnCallBack = false;
LPVOID engineBackupArrayOfCallBacks = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
DWORD engineBackupNumberOfCallBacks = NULL;
DWORD engineBackupTLSAddress = NULL;
IMAGE_TLS_DIRECTORY32 engineBackupTLSDataX86 = {};
IMAGE_TLS_DIRECTORY64 engineBackupTLSDataX64 = {};
bool engineAlowModuleLoading = false;
bool engineCheckForwarders = true;
bool enginePassAllExceptions = true;
bool engineRemoveConsoleForDebugee = false;
bool engineBackupForCriticalFunctions = false;
DWORD engineWaitForDebugEventTimeOut = INFINITE;
LPVOID engineStepCallBack = NULL;
int engineStepCount = INFINITE;
bool engineStepActive = false;
bool engineAttachedToProcess = false;
bool engineProcessIsNowDetached = false;
ULONG_PTR engineAttachedProcessCallBack = NULL;
LPVOID engineAttachedProcessDebugInfo = NULL;
LPVOID engineExitThreadOneShootCallBack = NULL;
bool engineResumeProcessIfNoThreadIsActive = false;
bool engineAutoHideFromDebugger = true;
long engineDefaultBreakPointType = UE_BREAKPOINT_INT3;
bool engineDebuggingDLL = false;
char* engineDebuggingDLLFullFileName;
char* engineDebuggingDLLFileName;
char* engineDebuggingDLLReserveFileName;
ULONG_PTR engineDebuggingDLLBase = NULL;
ULONG_PTR engineDebuggingMainModuleBase = NULL;
ULONG_PTR engineFakeDLLHandle = NULL;
char engineDisassembledInstruction[128];
ExpertDebug engineExpertDebug = {};
ULONG_PTR engineReservedMemoryLeft[UE_MAX_RESERVED_MEMORY_LEFT];
HANDLE engineReservedMemoryProcess = NULL;
void* engineFindOEPCallBack = NULL;
void* engineFindOEPUserCallBack = NULL;
ULONG_PTR DebugModuleEntryPoint;
ULONG_PTR DebugModuleImageBase;
LPVOID DebugModuleEntryPointCallBack;
LPVOID DebugExeFileEntryPointCallBack;
HMODULE engineHandle;
HARDWARE_DATA DebugRegister0 = {};
HARDWARE_DATA DebugRegister1 = {};
HARDWARE_DATA DebugRegister2 = {};
HARDWARE_DATA DebugRegister3 = {};
LPVOID RelocationData = NULL;
LPVOID RelocationLastPage = NULL;
LPVOID RelocationStartPosition = NULL;
LPVOID RelocationWritePosition = NULL;
ULONG_PTR RelocationOldImageBase;
ULONG_PTR RelocationNewImageBase;
char engineExtractedFolderName[512];
char engineExtractedFileName[512];
char engineFoundAPIName[512];
char engineFoundDLLName[512];
char szReserveModuleName[512];
char szDebuggerName[512];
char szExportFileName[512];
char szParameterString[512];
// Global.Engine.Strings:
char engineSzEngineFile[MAX_PATH];
char engineSzEngineFolder[MAX_PATH];
char engineSzEngineGarbageFolder[MAX_PATH];
// Global.Engine.Librarian:
LPVOID LibrarianData = VirtualAlloc(NULL, MAX_LIBRARY_BPX * sizeof LIBRARY_BREAK_DATA, MEM_COMMIT, PAGE_READWRITE);
// Global.Engine.UnpackData:
BreakPointManagerData glbBreakPointManagerData = {};
// Global.Engine.TraceOEP:
GenericOEPTracerData glbEntryTracerData = {};

// Global.Engine.Constants:
#define UE_MODULEx86 0xB990;
#define UE_MODULEx64 0xC120;

// Global.Handle.functions:
bool EngineCloseHandle(HANDLE myHandle){

	DWORD HandleFlags;

	if(GetHandleInformation(myHandle, &HandleFlags)){
		if(CloseHandle(myHandle)){
			return(true);
		}else{
			return(false);
		}
	}else{
		return(false);
	}
}
// Global.Mapping.functions:
bool MapFileEx(char* szFileName, DWORD ReadOrWrite, LPHANDLE FileHandle, LPDWORD FileSize, LPHANDLE FileMap, LPVOID FileMapVA, DWORD SizeModifier){

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
				RtlMoveMemory(FileMapVA, &mfFileMapVA, sizeof ULONG_PTR);
				return(true);
			}
		}
		RtlZeroMemory(FileMapVA, sizeof ULONG_PTR);
		*FileHandle = NULL;
		*FileSize = NULL;
		EngineCloseHandle(hFile);
	}else{
		RtlZeroMemory(FileMapVA, sizeof ULONG_PTR);
	}
	return(false);
}

void UnMapFileEx(HANDLE FileHandle, DWORD FileSize, HANDLE FileMap, ULONG_PTR FileMapVA){

	LPVOID ufFileMapVA = (void*)FileMapVA;

	if(UnmapViewOfFile(ufFileMapVA)){
		EngineCloseHandle(FileMap);
		SetFilePointer(FileHandle,FileSize,NULL,FILE_BEGIN);
		SetEndOfFile(FileHandle);
		EngineCloseHandle(FileHandle);
	}
}
// Global.Engine.functions:
void EngineGlobalTestFunction(){
	MessageBoxA(NULL, "TitanEngine test message!", "TitanEngine2:", 0x40);
}
bool EngineIsThereFreeHardwareBreakSlot(){

	if(DebugRegister0.DrxEnabled == false || DebugRegister1.DrxEnabled == false || DebugRegister2.DrxEnabled == false || DebugRegister3.DrxEnabled == false){
		return(true);
	}
	return(false);
}
char* EngineExtractPath(char* szFileName){

	int i;

	RtlZeroMemory(&engineExtractedFolderName, 512);
	lstrcpyA(engineExtractedFolderName, szFileName);
	i = lstrlenA(engineExtractedFolderName);
	while(i > 0 && engineExtractedFolderName[i] != 0x5C){
		engineExtractedFolderName[i] = 0x00;
		i--;
	}
	return(engineExtractedFolderName);
}
char* EngineExtractFileName(char* szFileName){

	int i;
	int j;
	int x = 0;

	i = lstrlenA(szFileName);
	RtlZeroMemory(&engineExtractedFileName, 512);
	while(i > 0 && szFileName[i] != 0x5C){
		i--;
	}
	if(szFileName[i] == 0x5C){
		for(j = i + 1; j <= lstrlenA(szFileName); j++){
			engineExtractedFileName[x] = szFileName[j];
			x++;
		}
	}else{
		return(szFileName);
	}
	return(engineExtractedFileName);
}
bool EngineIsPointedMemoryString(ULONG_PTR PossibleStringPtr){
	
	bool StringIsValid = true;
	unsigned int i = 512;
	MEMORY_BASIC_INFORMATION MemInfo;
	DWORD MaxDisassmSize;
	BYTE TestChar;

	VirtualQueryEx(GetCurrentProcess(), (LPVOID)PossibleStringPtr, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
	if(MemInfo.State == MEM_COMMIT){
		if((ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)MemInfo.RegionSize - PossibleStringPtr <= 512){
			MaxDisassmSize = (DWORD)((ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)MemInfo.RegionSize - PossibleStringPtr - 1);
			VirtualQueryEx(GetCurrentProcess(), (LPVOID)(PossibleStringPtr + (ULONG_PTR)MemInfo.RegionSize), &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
			if(MemInfo.State != MEM_COMMIT){
				i = MaxDisassmSize;
			}else{
				MaxDisassmSize = 512;
			}
		}else{
			MaxDisassmSize = 512;
		}
		RtlMoveMemory(&TestChar, (LPVOID)PossibleStringPtr, 1);
		while(i > NULL && StringIsValid == true && TestChar != 0x00){
			RtlMoveMemory(&TestChar, (LPVOID)PossibleStringPtr, 1);
			if(TestChar < 32 || TestChar > 126){
				if(TestChar != 0x00){
					StringIsValid = false;
				}
			}
			PossibleStringPtr++;
			i--;
		}
		if(StringIsValid == true && MaxDisassmSize - i > 4){
			return(true);
		}else{
			return(false);
		}
	}
	return(false);
}
int EnginePointedMemoryStringLength(ULONG_PTR PossibleStringPtr){
	
	bool StringIsValid = true;
	unsigned int i = 512;
	MEMORY_BASIC_INFORMATION MemInfo;
	DWORD MaxDisassmSize;
	BYTE TestChar;

	VirtualQueryEx(GetCurrentProcess(), (LPVOID)PossibleStringPtr, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
	if(MemInfo.State == MEM_COMMIT){
		if((ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)MemInfo.RegionSize - PossibleStringPtr <= 512){
			MaxDisassmSize = (DWORD)((ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)MemInfo.RegionSize - PossibleStringPtr - 1);
			VirtualQueryEx(GetCurrentProcess(), (LPVOID)(PossibleStringPtr + (ULONG_PTR)MemInfo.RegionSize), &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
			if(MemInfo.State != MEM_COMMIT){
				i = MaxDisassmSize;
			}else{
				MaxDisassmSize = 512;
			}
		}else{
			MaxDisassmSize = 512;
		}
		RtlMoveMemory(&TestChar, (LPVOID)PossibleStringPtr, 1);
		while(i > NULL && StringIsValid == true && TestChar != 0x00){
			RtlMoveMemory(&TestChar, (LPVOID)PossibleStringPtr, 1);
			if(TestChar < 32 || TestChar > 126){
				if(TestChar != 0x00){
					StringIsValid = false;
				}
			}
			PossibleStringPtr++;
			i--;
		}
		if(StringIsValid == true && 512 - i > 4){
			i = 512 - i;
			return(i);
		}else{
			return(NULL);
		}
	}
	return(NULL);
}
long long EngineEstimateNewSectionRVA(ULONG_PTR FileMapVA){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD NewSectionVirtualOffset = 0;
	DWORD SectionNumber = 0;
	BOOL FileIs64;

	if(FileMapVA != NULL){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
		PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
		if(PEHeader32->OptionalHeader.Magic == 0x10B){
			FileIs64 = false;
		}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
			FileIs64 = true;
		}else{
			return(0);
		}
		if(!FileIs64){
			PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
			SectionNumber = PEHeader32->FileHeader.NumberOfSections;
			__try{
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + (SectionNumber - 1) * IMAGE_SIZEOF_SECTION_HEADER);
				NewSectionVirtualOffset = PESections->VirtualAddress + (PESections->Misc.VirtualSize / PEHeader32->OptionalHeader.SectionAlignment) * PEHeader32->OptionalHeader.SectionAlignment;
				if(NewSectionVirtualOffset < PESections->VirtualAddress + PESections->Misc.VirtualSize){
					NewSectionVirtualOffset = NewSectionVirtualOffset + PEHeader32->OptionalHeader.SectionAlignment;
				}
				return((ULONG_PTR)(NewSectionVirtualOffset + PEHeader32->OptionalHeader.ImageBase));
			}__except(EXCEPTION_EXECUTE_HANDLER){
				return(0);
			}
		}else{
			PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
			SectionNumber = PEHeader64->FileHeader.NumberOfSections;
			__try{
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + (SectionNumber - 1) * IMAGE_SIZEOF_SECTION_HEADER);
				NewSectionVirtualOffset = PESections->VirtualAddress + (PESections->Misc.VirtualSize / PEHeader64->OptionalHeader.SectionAlignment) * PEHeader64->OptionalHeader.SectionAlignment;
				if(NewSectionVirtualOffset < PESections->VirtualAddress + PESections->Misc.VirtualSize){
					NewSectionVirtualOffset = NewSectionVirtualOffset + PEHeader32->OptionalHeader.SectionAlignment;
				}
				return((ULONG_PTR)(NewSectionVirtualOffset + PEHeader64->OptionalHeader.ImageBase));
			}__except(EXCEPTION_EXECUTE_HANDLER){
				return(0);
			}
		}
	}
	return(0);
}
bool EngineExtractForwarderData(ULONG_PTR PossibleStringPtr, LPVOID szFwdDLLName, LPVOID szFwdAPIName){
	
	LPVOID lpPossibleStringPtr = (LPVOID)PossibleStringPtr;
	BYTE TestChar;
	
	RtlMoveMemory(&TestChar, (LPVOID)PossibleStringPtr, 1);
	while(TestChar != 0x2E && TestChar != 0x00){
		RtlMoveMemory(&TestChar, (LPVOID)PossibleStringPtr, 1);
		PossibleStringPtr++;
	}
	if(TestChar == 0x00){
		return(false);
	}
	PossibleStringPtr--;
	RtlMoveMemory(szFwdDLLName, lpPossibleStringPtr, PossibleStringPtr - (ULONG_PTR)lpPossibleStringPtr);
	lstrcatA((LPSTR)szFwdDLLName, ".dll");
	lpPossibleStringPtr = (LPVOID)(PossibleStringPtr + 1);
	RtlMoveMemory(&TestChar, (LPVOID)PossibleStringPtr, 1);
	if(TestChar == 0x23){
		lpPossibleStringPtr = (LPVOID)(PossibleStringPtr + 1);
	}
	while(TestChar != 0x00){
		RtlMoveMemory(&TestChar, (LPVOID)PossibleStringPtr, 1);
		PossibleStringPtr++;
	}
	RtlMoveMemory(szFwdAPIName, lpPossibleStringPtr, PossibleStringPtr - (ULONG_PTR)lpPossibleStringPtr);
	return(true);
}
bool EngineGrabDataFromMappedFile(HANDLE hFile, ULONG_PTR FileMapVA, ULONG_PTR FileOffset, DWORD CopySize, LPVOID CopyToMemory){

	DWORD rfNumberOfBytesRead = NULL;

	RtlZeroMemory(CopyToMemory, CopySize);
	SetFilePointer(hFile, (DWORD)(FileOffset - FileMapVA), NULL, FILE_BEGIN);
	if(ReadFile(hFile, CopyToMemory, CopySize, &rfNumberOfBytesRead, NULL)){
		return(true);
	}else{
		return(false);
	}
}
bool EngineExtractResource(char* szResourceName, char* szExtractedFileName){

	HRSRC hResource;
	HGLOBAL hResourceGlobal;
	DWORD ResourceSize;
	LPVOID ResourceData;
	DWORD NumberOfBytesWritten;
	HANDLE hFile;

	hResource = FindResourceA(engineHandle, (LPCSTR)szResourceName, "BINARY");
	if(hResource != NULL){
		hResourceGlobal = LoadResource(engineHandle, hResource);
		if(hResourceGlobal != NULL){
			ResourceSize = SizeofResource(engineHandle, hResource);
			ResourceData = LockResource(hResourceGlobal);
			hFile = CreateFileA(szExtractedFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hFile != INVALID_HANDLE_VALUE){
				WriteFile(hFile, ResourceData, ResourceSize, &NumberOfBytesWritten, NULL);
				EngineCloseHandle(hFile);
			}else{
				return(false);
			}
		}
		return(true);
	}
	return(false);
}
bool EngineIsDependencyPresent(char* szFileName, char* szDependencyForFile, char* szPresentInFolder){

	int i,j;
	HANDLE hFile;
	char szTryFileName[512];

	if(szPresentInFolder != NULL){
		RtlZeroMemory(&szTryFileName, 512);
		lstrcpyA(szTryFileName, szPresentInFolder);
		if(szTryFileName[lstrlenA(szTryFileName)-1] != 0x5C){
			szTryFileName[lstrlenA(szTryFileName)] = 0x5C;
		}
		lstrcatA(szTryFileName, szFileName);
		hFile = CreateFileA(szTryFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile != INVALID_HANDLE_VALUE){
			EngineCloseHandle(hFile);
			return(true);
		}
	}
	if(szFileName != NULL){
		hFile = CreateFileA(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile != INVALID_HANDLE_VALUE){
			EngineCloseHandle(hFile);
			return(true);
		}
		if(GetSystemDirectoryA(szTryFileName, 512) > NULL){
			lstrcatA(szTryFileName, "\\");
			lstrcatA(szTryFileName, szFileName);
			hFile = CreateFileA(szTryFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hFile != INVALID_HANDLE_VALUE){
				EngineCloseHandle(hFile);
				return(true);
			}
		}
		if(GetWindowsDirectoryA(szTryFileName, 512) > NULL){
			lstrcatA(szTryFileName, "\\");
			lstrcatA(szTryFileName, szFileName);
			hFile = CreateFileA(szTryFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hFile != INVALID_HANDLE_VALUE){
				EngineCloseHandle(hFile);
				return(true);
			}
		}
		if(szDependencyForFile != NULL){
			RtlZeroMemory(&szTryFileName, 512);
			i = lstrlenA(szDependencyForFile);
			while(i > 0 && szDependencyForFile[i] != 0x5C){
				i--;
			}
			for(j = 0; j <= i; j++){
				szTryFileName[j] = szDependencyForFile[j];
			}
			lstrcatA(szTryFileName, szFileName);
			hFile = CreateFileA(szTryFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hFile != INVALID_HANDLE_VALUE){
				EngineCloseHandle(hFile);
				return(true);
			}
		}
	}
	return(false);
}
bool EngineGetDependencyLocation(char* szFileName, char* szDependencyForFile, void* szLocationOfTheFile, int MaxStringSize){

	int i,j;
	HANDLE hFile;
	char szTryFileName[512];

	if(szFileName != NULL){
		hFile = CreateFileA(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile != INVALID_HANDLE_VALUE){
			RtlZeroMemory(szLocationOfTheFile, MaxStringSize);
			if(lstrlenA(szFileName) <= MaxStringSize){
				RtlMoveMemory(szLocationOfTheFile, szFileName, lstrlenA(szFileName));
			}
			EngineCloseHandle(hFile);
			return(true);
		}
		if(GetSystemDirectoryA(szTryFileName, 512) > NULL){
			lstrcatA(szTryFileName, "\\");
			lstrcatA(szTryFileName, szFileName);
			hFile = CreateFileA(szTryFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hFile != INVALID_HANDLE_VALUE){
				RtlZeroMemory(szLocationOfTheFile, MaxStringSize);
				if(lstrlenA(szTryFileName) <= MaxStringSize){
					RtlMoveMemory(szLocationOfTheFile, &szTryFileName, lstrlenA(szTryFileName));
				}
				EngineCloseHandle(hFile);
				return(true);
			}
		}
		if(GetWindowsDirectoryA(szTryFileName, 512) > NULL){
			lstrcatA(szTryFileName, "\\");
			lstrcatA(szTryFileName, szFileName);
			hFile = CreateFileA(szTryFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hFile != INVALID_HANDLE_VALUE){
				RtlZeroMemory(szLocationOfTheFile, MaxStringSize);
				if(lstrlenA(szTryFileName) <= MaxStringSize){
					RtlMoveMemory(szLocationOfTheFile, &szTryFileName, lstrlenA(szTryFileName));
				}
				EngineCloseHandle(hFile);
				return(true);
			}
		}
		if(szDependencyForFile != NULL){
			RtlZeroMemory(&szTryFileName, 512);
			i = lstrlenA(szDependencyForFile);
			while(i > 0 && szDependencyForFile[i] != 0x5C){
				i--;
			}
			for(j = 0; j <= i; j++){
				szTryFileName[j] = szDependencyForFile[j];
			}
			lstrcatA(szTryFileName, szFileName);
			hFile = CreateFileA(szTryFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hFile != INVALID_HANDLE_VALUE){
				RtlZeroMemory(szLocationOfTheFile, MaxStringSize);
				if(lstrlenA(szTryFileName) <= MaxStringSize){
					RtlMoveMemory(szLocationOfTheFile, &szTryFileName, lstrlenA(szTryFileName));
				}
				EngineCloseHandle(hFile);
				return(true);
			}
		}
	}
	return(false);
}
long EngineHashString(char* szStringToHash){

	int i = NULL;
	DWORD HashValue = NULL;

	for(i = 0; i < lstrlenA(szStringToHash); i++){
		HashValue = (((HashValue << 7) | (HashValue >> (32 - 7))) ^ szStringToHash[i]);
	}
	return(HashValue);
}
long EngineHashMemory(char* MemoryAddress, int MemorySize, DWORD InitialHashValue){

	int i = NULL;
	DWORD HashValue = InitialHashValue;

	for(i = 0; i < MemorySize; i++){
		if(MemoryAddress[i] != NULL){
			HashValue = (((HashValue << 7) | (HashValue >> (32 - 7))) ^ MemoryAddress[i]);
		}
	}
	return(HashValue);
}
bool EngineIsBadReadPtrEx(LPVOID DataPointer, DWORD DataSize){

	MEMORY_BASIC_INFORMATION MemInfo;

	while(DataSize > NULL){
		VirtualQuery(DataPointer, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		if(MemInfo.AllocationProtect == MEM_FREE || MemInfo.AllocationProtect == MEM_PRIVATE){
			return(false);
		}
		DataPointer = (LPVOID)((ULONG_PTR)DataPointer + MemInfo.RegionSize);
		if(MemInfo.RegionSize > DataSize){
			DataSize = NULL;
		}else{
			DataSize = DataSize - (DWORD)MemInfo.RegionSize;
		}
	}
	return(true);
}
bool EngineValidateResource(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam){

	HRSRC hResource;
	HGLOBAL hResourceGlobal;
	DWORD ResourceSize;
	LPVOID ResourceData;
	BYTE ReturnData = UE_FILED_FIXABLE_CRITICAL;

	hResource = FindResourceA(hModule, (LPCSTR)lpszName, (LPCSTR)lpszType);
	if(hResource != NULL){
		hResourceGlobal = LoadResource(hModule, hResource);
		if(hResourceGlobal != NULL){
			ResourceSize = SizeofResource(hModule, hResource);
			ResourceData = LockResource(hResourceGlobal);
			if(ResourceData != NULL){
				if(!EngineIsBadReadPtrEx(ResourceData, ResourceSize)){
					RtlMoveMemory((LPVOID)lParam, &ReturnData, 1);
					return(false);
				}
			}else{
				RtlMoveMemory((LPVOID)lParam, &ReturnData, 1);
				return(false);
			}
		}
		return(true);
	}
	RtlMoveMemory((LPVOID)lParam, &ReturnData, 1);
	return(false);
}
bool EngineValidateHeader(ULONG_PTR FileMapVA, HANDLE hFileProc, LPVOID ImageBase, PIMAGE_DOS_HEADER DOSHeader, bool IsFile){

	MODULEINFO ModuleInfo;
	DWORD MemorySize = NULL;
	PIMAGE_NT_HEADERS32 PEHeader32;
	IMAGE_NT_HEADERS32 RemotePEHeader32;
	MEMORY_BASIC_INFORMATION MemoryInfo;
	ULONG_PTR NumberOfBytesRW = NULL;

	if(IsFile){
		if(hFileProc == NULL){
			VirtualQueryEx(GetCurrentProcess(), (LPVOID)FileMapVA, &MemoryInfo, sizeof MEMORY_BASIC_INFORMATION);
			VirtualQueryEx(GetCurrentProcess(), MemoryInfo.AllocationBase, &MemoryInfo, sizeof MEMORY_BASIC_INFORMATION);
			MemorySize = (DWORD)((ULONG_PTR)MemoryInfo.AllocationBase + (ULONG_PTR)MemoryInfo.RegionSize - (ULONG_PTR)FileMapVA);
		}else{
			MemorySize = GetFileSize(hFileProc, NULL);
		}
		__try{
			if(DOSHeader->e_magic == 0x5A4D){
				if(DOSHeader->e_lfanew + sizeof IMAGE_DOS_HEADER + sizeof IMAGE_NT_HEADERS64 < MemorySize){
					PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
					if(PEHeader32->Signature != 0x4550){
						return(false);
					}else{
						return(true);
					}
				}else{
					return(false);
				}
			}else{
				return(false);
			}
		}__except(EXCEPTION_EXECUTE_HANDLER){
			return(false);
		}
	}else{
		RtlZeroMemory(&ModuleInfo, sizeof MODULEINFO);
		GetModuleInformation(hFileProc, (HMODULE)ImageBase, &ModuleInfo, sizeof MODULEINFO);
		__try{
			if(DOSHeader->e_magic == 0x5A4D){
				if(DOSHeader->e_lfanew + sizeof IMAGE_DOS_HEADER + sizeof IMAGE_NT_HEADERS64 < ModuleInfo.SizeOfImage){
					if(ReadProcessMemory(hFileProc, (LPVOID)((ULONG_PTR)ImageBase + DOSHeader->e_lfanew), &RemotePEHeader32, sizeof IMAGE_NT_HEADERS32, &NumberOfBytesRW)){
						PEHeader32 = (PIMAGE_NT_HEADERS32)(&RemotePEHeader32);
						if(PEHeader32->Signature != 0x4550){
							return(false);
						}else{
							return(true);
						}
					}else{
						return(false);
					}
				}else{
					return(false);
				}
			}else{
				return(false);
			}
		}__except(EXCEPTION_EXECUTE_HANDLER){
			return(false);
		}
	}
}
long long EngineSimulateNtLoader(char* szFileName){

	DWORD PeHeaderSize;
	LPVOID AllocatedFile;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD SectionNumber = 0;
	DWORD SectionRawOffset = 0;
	DWORD SectionRawSize = 0;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(NULL);
			}
			if(!FileIs64){
				AllocatedFile = VirtualAlloc(NULL, PEHeader32->OptionalHeader.SizeOfImage, MEM_COMMIT, PAGE_READWRITE);
				__try{
					PeHeaderSize = PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_SECTION_HEADER) * PEHeader32->FileHeader.NumberOfSections;
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
					SectionNumber = PEHeader32->FileHeader.NumberOfSections;
					RtlMoveMemory(AllocatedFile, (LPVOID)FileMapVA, PeHeaderSize);
					while(SectionNumber > 0){
						RtlMoveMemory((LPVOID)((ULONG_PTR)AllocatedFile + PESections->VirtualAddress), (LPVOID)(FileMapVA + PESections->PointerToRawData), PESections->SizeOfRawData);
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					VirtualFree(AllocatedFile, NULL, MEM_RELEASE);
					AllocatedFile = NULL;
				}
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return((ULONG_PTR)AllocatedFile);
			}else{
				AllocatedFile = VirtualAlloc(NULL, PEHeader64->OptionalHeader.SizeOfImage, MEM_COMMIT, PAGE_READWRITE);
				__try{
					PeHeaderSize = PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_SECTION_HEADER) * PEHeader64->FileHeader.NumberOfSections;
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
					SectionNumber = PEHeader64->FileHeader.NumberOfSections;
					RtlMoveMemory(AllocatedFile, (LPVOID)FileMapVA, PeHeaderSize);
					while(SectionNumber > 0){
						RtlMoveMemory((LPVOID)((ULONG_PTR)AllocatedFile + PESections->VirtualAddress), (LPVOID)(FileMapVA + PESections->PointerToRawData), PESections->SizeOfRawData);
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					VirtualFree(AllocatedFile, NULL, MEM_RELEASE);
					AllocatedFile = NULL;
				}
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return((ULONG_PTR)AllocatedFile);
			}
		}else{
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			return(NULL);		
		}
	}
	return(NULL);
}
long long EngineSimulateDllLoader(HANDLE hProcess, char* szFileName){

	int n;
	BOOL FileIs64;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	HANDLE FileHandle;
	LPVOID DLLMemory = NULL;
	DWORD ExportDelta = NULL;
	DWORD PEHeaderSize = NULL;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_EXPORT_DIRECTORY PEExports;
	PEXPORTED_DATA ExportedFunctionNames;
	ULONG_PTR ConvertedExport = NULL;
	char szFileRemoteProc[1024];
	char szDLLFileLocation[512];
	char* szTranslatedProcName;

	GetProcessImageFileNameA(hProcess, szFileRemoteProc, 1024);
	szTranslatedProcName = (char*)TranslateNativeName(szFileRemoteProc);
	if(EngineIsDependencyPresent(szFileName, NULL, NULL)){
		if(EngineGetDependencyLocation(szFileName, szTranslatedProcName, &szDLLFileLocation, 512)){
			VirtualFree((void*)szTranslatedProcName, NULL, MEM_RELEASE);
			if(MapFileEx(szDLLFileLocation, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
				DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
				if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
					PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
					PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
					if(PEHeader32->OptionalHeader.Magic == 0x10B){
						PEHeaderSize = PEHeader32->FileHeader.NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4;
						FileIs64 = false;
					}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
						PEHeaderSize = PEHeader64->FileHeader.NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4;
						FileIs64 = true;
					}else{
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(NULL);
					}
					if(!FileIs64){
						if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress != NULL){
							DLLMemory = VirtualAlloc(NULL, DOSHeader->e_lfanew + PEHeaderSize + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size + 0x1000, MEM_COMMIT, PAGE_READWRITE);
							if(DLLMemory != NULL){
								__try{
									if((DOSHeader->e_lfanew + PEHeaderSize) % 0x1000 != 0){
										ExportDelta = (((DOSHeader->e_lfanew + PEHeaderSize) / 0x1000) + 1) * 0x1000;
									}else{
										ExportDelta = ((DOSHeader->e_lfanew + PEHeaderSize) / 0x1000) * 0x1000;
									}
									ConvertedExport = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, PEHeader32->OptionalHeader.ImageBase, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress, true, true);
									if(ConvertedExport != NULL){
										PEExports = (PIMAGE_EXPORT_DIRECTORY)((ULONG_PTR)DLLMemory + ExportDelta);
										RtlMoveMemory(DLLMemory, (LPVOID)FileMapVA, PEHeaderSize + DOSHeader->e_lfanew);
										RtlMoveMemory((LPVOID)((ULONG_PTR)DLLMemory + ExportDelta), (LPVOID)ConvertedExport, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size);
										PEExports->AddressOfFunctions = PEExports->AddressOfFunctions - PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + ExportDelta;
										PEExports->AddressOfNameOrdinals = PEExports->AddressOfNameOrdinals - PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + ExportDelta;
										PEExports->AddressOfNames = PEExports->AddressOfNames - PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + ExportDelta;
										PEExports->Name = PEExports->Name - PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + ExportDelta;
										ExportedFunctionNames = (PEXPORTED_DATA)(PEExports->AddressOfNames + (ULONG_PTR)DLLMemory);
										for(n = 0; n < (int)PEExports->NumberOfNames; n++){
											ExportedFunctionNames->ExportedItem = ExportedFunctionNames->ExportedItem - PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + ExportDelta;
											ExportedFunctionNames = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctionNames + 4);
										}
										DOSHeader = (PIMAGE_DOS_HEADER)DLLMemory;
										PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
										PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = ExportDelta;
										UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
										return((ULONG_PTR)DLLMemory);
									}else{
										VirtualFree(DLLMemory, NULL, MEM_RELEASE);
									}
								}__except(EXCEPTION_EXECUTE_HANDLER){
									VirtualFree(DLLMemory, NULL, MEM_RELEASE);
								}
							}
						}
					}else{
						if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress != NULL){
							DLLMemory = VirtualAlloc(NULL, DOSHeader->e_lfanew + PEHeaderSize + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size + 0x1000, MEM_COMMIT, PAGE_READWRITE);
							if(DLLMemory != NULL){
								__try{
									if((DOSHeader->e_lfanew + PEHeaderSize) % 0x1000 != 0){
										ExportDelta = (((DOSHeader->e_lfanew + PEHeaderSize) % 0x1000) + 1) * 0x1000;
									}else{
										ExportDelta = ((DOSHeader->e_lfanew + PEHeaderSize) % 0x1000) * 0x1000;
									}
									ConvertedExport = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress, true, true);
									if(ConvertedExport != NULL){
										PEExports = (PIMAGE_EXPORT_DIRECTORY)((ULONG_PTR)DLLMemory + ExportDelta);
										RtlMoveMemory(DLLMemory, (LPVOID)FileMapVA, PEHeaderSize + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size);
										RtlMoveMemory((LPVOID)((ULONG_PTR)DLLMemory + ExportDelta), (LPVOID)ConvertedExport, PEHeaderSize + DOSHeader->e_lfanew);
										PEExports->AddressOfFunctions = PEExports->AddressOfFunctions - PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + ExportDelta;
										PEExports->AddressOfNameOrdinals = PEExports->AddressOfNameOrdinals - PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + ExportDelta;
										PEExports->AddressOfNames = PEExports->AddressOfNames - PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + ExportDelta;
										PEExports->Name = PEExports->Name - PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + ExportDelta;
										ExportedFunctionNames = (PEXPORTED_DATA)(PEExports->AddressOfNames + (ULONG_PTR)DLLMemory);
										for(n = 0; n < (int)PEExports->NumberOfNames; n++){
											ExportedFunctionNames->ExportedItem = ExportedFunctionNames->ExportedItem - PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + ExportDelta;
											ExportedFunctionNames = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctionNames + 4);
										}
										DOSHeader = (PIMAGE_DOS_HEADER)DLLMemory;
										PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
										PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = ExportDelta;
										UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
										return((ULONG_PTR)DLLMemory);
									}else{
										VirtualFree(DLLMemory, NULL, MEM_RELEASE);
									}
								}__except(EXCEPTION_EXECUTE_HANDLER){
									VirtualFree(DLLMemory, NULL, MEM_RELEASE);
								}
							}
						}
					}
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				}
			}
		}
	}
	VirtualFree((void*)szTranslatedProcName, NULL, MEM_RELEASE);
	return(NULL);
}
long long EngineGetProcAddress(ULONG_PTR ModuleBase, char* szAPIName){

	int i = 0;
	int j = 0;
	ULONG_PTR APIFoundAddress = 0;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_EXPORT_DIRECTORY PEExports;
	PEXPORTED_DATA ExportedFunctions;
	PEXPORTED_DATA ExportedFunctionNames;
	PEXPORTED_DATA_WORD ExportedFunctionOrdinals;
	char szModuleName[MAX_PATH] = {};
	bool FileIs64 = false;

	if(GetModuleFileNameA((HMODULE)ModuleBase, szModuleName, MAX_PATH) == NULL){
		__try{
			DOSHeader = (PIMAGE_DOS_HEADER)ModuleBase;
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				return(NULL);
			}
			if(!FileIs64){
				PEExports = (PIMAGE_EXPORT_DIRECTORY)(ModuleBase + (ULONG_PTR)PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
				ExportedFunctions = (PEXPORTED_DATA)(ModuleBase + (ULONG_PTR)PEExports->AddressOfFunctions);
				ExportedFunctionNames = (PEXPORTED_DATA)(ModuleBase + (ULONG_PTR)PEExports->AddressOfNames);
				ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)(ModuleBase + (ULONG_PTR)PEExports->AddressOfNameOrdinals);
			}else{
				PEExports = (PIMAGE_EXPORT_DIRECTORY)(ModuleBase + (ULONG_PTR)PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
				ExportedFunctions = (PEXPORTED_DATA)(ModuleBase + (ULONG_PTR)PEExports->AddressOfFunctions);
				ExportedFunctionNames = (PEXPORTED_DATA)(ModuleBase + (ULONG_PTR)PEExports->AddressOfNames);
				ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)(ModuleBase + (ULONG_PTR)PEExports->AddressOfNameOrdinals);
			}
			for(j = 0; j < (int)PEExports->NumberOfNames; j++){
				if(!FileIs64){
					if(lstrcmpiA((LPCSTR)szAPIName, (LPCSTR)(ModuleBase + (ULONG_PTR)ExportedFunctionNames->ExportedItem)) == NULL){
						ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)((ULONG_PTR)ExportedFunctionOrdinals + j * 2);
						ExportedFunctions = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctions + (ExportedFunctionOrdinals->OrdinalNumber) * 4);
						APIFoundAddress = ExportedFunctions->ExportedItem + (ULONG_PTR)ModuleBase;
						return((ULONG_PTR)APIFoundAddress);
					}
				}else{
					if(lstrcmpiA((LPCSTR)szAPIName, (LPCSTR)(ModuleBase + (ULONG_PTR)ExportedFunctionNames->ExportedItem)) == NULL){
						ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)((ULONG_PTR)ExportedFunctionOrdinals + j * 2);
						ExportedFunctions = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctions + (ExportedFunctionOrdinals->OrdinalNumber) * 4);
						APIFoundAddress = ExportedFunctions->ExportedItem + (ULONG_PTR)ModuleBase;
						return((ULONG_PTR)APIFoundAddress);
					}
				}
				ExportedFunctionNames = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctionNames + 4);
			}
			return(NULL);
		}__except(EXCEPTION_EXECUTE_HANDLER){
			return(NULL);
		}
	}else{
		return((ULONG_PTR)GetProcAddress((HMODULE)ModuleBase, szAPIName));
	}
}
bool EngineGetLibraryOrdinalData(ULONG_PTR ModuleBase, LPDWORD ptrOrdinalBase, LPDWORD ptrOrdinalCount){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_EXPORT_DIRECTORY PEExports;
	bool FileIs64 = false;

	__try{
		DOSHeader = (PIMAGE_DOS_HEADER)ModuleBase;
		PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
		PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
		if(PEHeader32->OptionalHeader.Magic == 0x10B){
			FileIs64 = false;
		}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
			FileIs64 = true;
		}else{
			return(false);
		}
		if(!FileIs64){
			PEExports = (PIMAGE_EXPORT_DIRECTORY)(ModuleBase + (ULONG_PTR)PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
			*ptrOrdinalBase = PEExports->Base;
			*ptrOrdinalCount = PEExports->NumberOfNames;
		}else{
			PEExports = (PIMAGE_EXPORT_DIRECTORY)(ModuleBase + (ULONG_PTR)PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
			*ptrOrdinalBase = PEExports->Base;
			*ptrOrdinalCount = PEExports->NumberOfNames;
		}
		return(true);
	}__except(EXCEPTION_EXECUTE_HANDLER){
		return(false);
	}
	return(false);
}
long long EngineGlobalAPIHandler(HANDLE handleProcess, ULONG_PTR EnumedModulesBases, ULONG_PTR APIAddress, char* szAPIName, DWORD ReturnType){

	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int n = 0;
	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int z = 0;
	DWORD Dummy = NULL;
	HANDLE hProcess = NULL;
	ULONG_PTR EnumeratedModules[0x2000];
	ULONG_PTR LoadedModules[1000][4];
	char RemoteDLLName[MAX_PATH];
	char FullRemoteDLLName[MAX_PATH];
	HANDLE hLoadedModule = NULL;
	HANDLE ModuleHandle = NULL;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_EXPORT_DIRECTORY PEExports;
	PEXPORTED_DATA ExportedFunctions;
	PEXPORTED_DATA ExportedFunctionNames;
	PEXPORTED_DATA_WORD ExportedFunctionOrdinals;
	ULONG_PTR APIFoundAddress = NULL;
	MODULEINFO RemoteModuleInfo;
	bool ValidateHeader = false;
	bool FileIs64 = false;
	bool APINameFound = false;
	bool SkipModule = false;
	unsigned int FoundIndex = 0;
	ULONG_PTR FileMapVA;
	char szFwdDLLName[512];
	char szFwdAPIName[512];
	ULONG_PTR RealignedAPIAddress;
	ULONG_PTR ForwarderData = NULL;
	unsigned int ClosestAPI = 0x1000;

	RtlZeroMemory(&engineFoundDLLName, 512);
	RtlZeroMemory(&EnumeratedModules, 0x2000 * sizeof ULONG_PTR);
	RtlZeroMemory(&LoadedModules, 1000 * 4 * sizeof ULONG_PTR);
	if(EnumedModulesBases != NULL){
		RtlMoveMemory(&EnumeratedModules, (LPVOID)EnumedModulesBases, 0x1000);
		i--;
	}
	if(handleProcess == NULL){
		if(dbgProcessInformation.hProcess == NULL){
			hProcess = GetCurrentProcess();
		}else{
			hProcess = dbgProcessInformation.hProcess;
		}
	}else{
		hProcess = handleProcess;
	}
	if(EnumedModulesBases != NULL || EnumProcessModules(hProcess, (HMODULE*)EnumeratedModules, 0x2000, &Dummy)){
		i++;
		z = i;
		y = i;
		while(EnumeratedModules[y] != NULL){
			y++;
		}
		while(APINameFound == false && EnumeratedModules[i] != NULL){
			ValidateHeader = false;
			RtlZeroMemory(&RemoteDLLName, MAX_PATH);
			GetModuleFileNameExA(hProcess, (HMODULE)EnumeratedModules[i], (LPSTR)RemoteDLLName, MAX_PATH);
			lstrcpyA(FullRemoteDLLName, RemoteDLLName);
			if(GetModuleHandleA(RemoteDLLName) == NULL){
				RtlZeroMemory(&RemoteDLLName, MAX_PATH);
				GetModuleBaseNameA(hProcess, (HMODULE)EnumeratedModules[i], (LPSTR)RemoteDLLName, MAX_PATH);
				if(GetModuleHandleA(RemoteDLLName) == NULL){
					if(engineAlowModuleLoading){
						hLoadedModule = LoadLibraryA(FullRemoteDLLName);
						if(hLoadedModule != NULL){
							LoadedModules[i][0] = EnumeratedModules[i];
							LoadedModules[i][1] = (ULONG_PTR)hLoadedModule;
							LoadedModules[i][2] = 1;
						}
					}else{
						hLoadedModule = (HANDLE)EngineSimulateDllLoader(hProcess, FullRemoteDLLName);
						if(hLoadedModule != NULL){
							LoadedModules[i][0] = EnumeratedModules[i];
							LoadedModules[i][1] = (ULONG_PTR)hLoadedModule;
							LoadedModules[i][2] = 1;
							ValidateHeader = true;
						}
					}
				}else{
					LoadedModules[i][0] = EnumeratedModules[i];
					LoadedModules[i][1] = (ULONG_PTR)GetModuleHandleA(RemoteDLLName);
					LoadedModules[i][2] = 0;
				}
			}else{
				LoadedModules[i][0] = EnumeratedModules[i];
				LoadedModules[i][1] = (ULONG_PTR)GetModuleHandleA(RemoteDLLName);
				LoadedModules[i][2] = 0;
			}

			if(ReturnType != UE_OPTION_IMPORTER_RETURN_FORWARDER_DLLNAME && ReturnType != UE_OPTION_IMPORTER_RETURN_FORWARDER_DLLINDEX && ReturnType != UE_OPTION_IMPORTER_RETURN_FORWARDER_APINAME){
				if(szAPIName == NULL && ReturnType == UE_OPTION_IMPORTER_REALIGN_APIADDRESS){
					RtlZeroMemory(&RemoteModuleInfo, sizeof MODULEINFO);
					GetModuleInformation(GetCurrentProcess(), (HMODULE)LoadedModules[i][1], &RemoteModuleInfo, sizeof MODULEINFO);
					if(APIAddress >= LoadedModules[i][1] && APIAddress <= LoadedModules[i][1] + RemoteModuleInfo.SizeOfImage){
						GetModuleBaseNameA(hProcess, (HMODULE)LoadedModules[i][0], (LPSTR)engineFoundDLLName, 512);
						APIFoundAddress = (ULONG_PTR)(APIAddress - LoadedModules[i][1] + LoadedModules[i][0]);
						APINameFound = true;
						FoundIndex = i;
						break;
					}
				}else if(szAPIName == NULL && ReturnType == UE_OPTION_IMPORTER_REALIGN_LOCAL_APIADDRESS){
					RtlZeroMemory(&RemoteModuleInfo, sizeof MODULEINFO);
					GetModuleInformation(hProcess, (HMODULE)LoadedModules[i][0], &RemoteModuleInfo, sizeof MODULEINFO);
					if(APIAddress >= LoadedModules[i][0] && APIAddress <= LoadedModules[i][0] + RemoteModuleInfo.SizeOfImage){
						GetModuleBaseNameA(hProcess, (HMODULE)LoadedModules[i][0], (LPSTR)engineFoundDLLName, 512);
						APIFoundAddress = (ULONG_PTR)(APIAddress - LoadedModules[i][0] + LoadedModules[i][1]);
						APINameFound = true;
						FoundIndex = i;
						break;
					}
				}else if(szAPIName == NULL && ReturnType == UE_OPTION_IMPORTER_RETURN_DLLBASE){
					if(APIAddress == LoadedModules[i][1]){
						APIFoundAddress = LoadedModules[i][0];
						APINameFound = true;
						FoundIndex = i;
						break;
					}
				}else if(ReturnType == UE_OPTION_IMPORTER_RETURN_NEAREST_APIADDRESS || ReturnType == UE_OPTION_IMPORTER_RETURN_NEAREST_APINAME){
					RtlZeroMemory(&RemoteModuleInfo, sizeof MODULEINFO);
					GetModuleInformation(hProcess, (HMODULE)LoadedModules[i][0], &RemoteModuleInfo, sizeof MODULEINFO);
					if(APIAddress >= LoadedModules[i][0] && APIAddress <= LoadedModules[i][0] + RemoteModuleInfo.SizeOfImage){
						DOSHeader = (PIMAGE_DOS_HEADER)LoadedModules[i][1];
						if(ValidateHeader || EngineValidateHeader((ULONG_PTR)LoadedModules[i][1], GetCurrentProcess(), RemoteModuleInfo.lpBaseOfDll, DOSHeader, false)){
							__try{
								PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
								PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
								if(PEHeader32->OptionalHeader.Magic == 0x10B){
									FileIs64 = false;
								}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
									FileIs64 = true;
								}else{
									return(NULL);
								}
								if(!FileIs64){
									PEExports = (PIMAGE_EXPORT_DIRECTORY)(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + LoadedModules[i][1]);
									ExportedFunctions = (PEXPORTED_DATA)(PEExports->AddressOfFunctions + LoadedModules[i][1]);
								}else{
									PEExports = (PIMAGE_EXPORT_DIRECTORY)(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + LoadedModules[i][1]);				
									ExportedFunctions = (PEXPORTED_DATA)(PEExports->AddressOfFunctions + LoadedModules[i][1]);
								}
								for(n = 0; n < PEExports->NumberOfFunctions; n++){ //NumberOfNames
									if(APIAddress - (ExportedFunctions->ExportedItem + LoadedModules[i][0]) < ClosestAPI){
										ClosestAPI = (unsigned int)(APIAddress - (ExportedFunctions->ExportedItem + LoadedModules[i][0]));
										if(!FileIs64){
											ExportedFunctionNames = (PEXPORTED_DATA)(PEExports->AddressOfNames + LoadedModules[i][1]);
											ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)(PEExports->AddressOfNameOrdinals + LoadedModules[i][1]);
										}else{
											ExportedFunctionNames = (PEXPORTED_DATA)(PEExports->AddressOfNames + LoadedModules[i][1]);
											ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)(PEExports->AddressOfNameOrdinals + LoadedModules[i][1]);
										}
										GetModuleBaseNameA(hProcess, (HMODULE)LoadedModules[i][0], (LPSTR)engineFoundDLLName, 512);
										RtlZeroMemory(&engineFoundAPIName, 512);
										x = n;
										for(j = 0; j < PEExports->NumberOfNames; j++){
											if(ExportedFunctionOrdinals->OrdinalNumber != x){
												ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)((ULONG_PTR)ExportedFunctionOrdinals + 2);
											}else{
												break;
											}
										}
										ExportedFunctionNames = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctionNames + j * 4);
										lstrcpyA((LPSTR)engineFoundAPIName, (LPCSTR)(ExportedFunctionNames->ExportedItem + LoadedModules[i][1]));
										APIFoundAddress = ExportedFunctions->ExportedItem + LoadedModules[i][0];
										APINameFound = true;
										FoundIndex = i;
									}
									ExportedFunctions = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctions + 4);
								}
							}__except(EXCEPTION_EXECUTE_HANDLER){
								ClosestAPI = 0x1000;
								APINameFound = false;
							}
						}
					}
				}

				if(ReturnType > UE_OPTION_IMPORTER_REALIGN_APIADDRESS && ReturnType < UE_OPTION_IMPORTER_RETURN_FORWARDER_DLLNAME && ReturnType != UE_OPTION_IMPORTER_RETURN_DLLBASE && LoadedModules[i][1] != NULL){
					RtlZeroMemory(&RemoteModuleInfo, sizeof MODULEINFO);
					DOSHeader = (PIMAGE_DOS_HEADER)LoadedModules[i][1];
					GetModuleInformation(GetCurrentProcess(), (HMODULE)LoadedModules[i][1], &RemoteModuleInfo, sizeof MODULEINFO);
					if(ValidateHeader || EngineValidateHeader((ULONG_PTR)LoadedModules[i][1], GetCurrentProcess(), RemoteModuleInfo.lpBaseOfDll, DOSHeader, false)){
						__try{
							PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
							PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
							if(PEHeader32->OptionalHeader.Magic == 0x10B){
								FileIs64 = false;
							}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
								FileIs64 = true;
							}else{
								return(NULL);
							}
							if(!FileIs64){
								PEExports = (PIMAGE_EXPORT_DIRECTORY)(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + LoadedModules[i][1]);
								ExportedFunctions = (PEXPORTED_DATA)(PEExports->AddressOfFunctions + LoadedModules[i][1]);
								ExportedFunctionNames = (PEXPORTED_DATA)(PEExports->AddressOfNames + LoadedModules[i][1]);
								ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)(PEExports->AddressOfNameOrdinals + LoadedModules[i][1]);
							}else{
								PEExports = (PIMAGE_EXPORT_DIRECTORY)(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + LoadedModules[i][1]);				
								ExportedFunctions = (PEXPORTED_DATA)(PEExports->AddressOfFunctions + LoadedModules[i][1]);
								ExportedFunctionNames = (PEXPORTED_DATA)(PEExports->AddressOfNames + LoadedModules[i][1]);
								ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)(PEExports->AddressOfNameOrdinals + LoadedModules[i][1]);
							}
							if(ReturnType == UE_OPTION_IMPORTER_RETURN_APINAME || ReturnType == UE_OPTION_IMPORTER_RETURN_DLLNAME || ReturnType == UE_OPTION_IMPORTER_RETURN_DLLINDEX){
								for(j = 0; j < PEExports->NumberOfFunctions; j++){ //NumberOfNames
									if(ExportedFunctions->ExportedItem + LoadedModules[i][0] == APIAddress){
										GetModuleBaseNameA(hProcess, (HMODULE)LoadedModules[i][0], (LPSTR)engineFoundDLLName, 512);
										RtlZeroMemory(&engineFoundAPIName, 512);
										x = j;
										for(j = 0; j < PEExports->NumberOfNames; j++){
											if(ExportedFunctionOrdinals->OrdinalNumber != x){
												ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)((ULONG_PTR)ExportedFunctionOrdinals + 2);
											}else{
												break;
											}
										}
										ExportedFunctionNames = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctionNames + j * 4);
										lstrcpyA((LPSTR)engineFoundAPIName, (LPCSTR)(ExportedFunctionNames->ExportedItem + LoadedModules[i][1]));
										APINameFound = true;
										FoundIndex = i;
										break;
									}
									ExportedFunctions = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctions + 4);
								}
							}else if(ReturnType == UE_OPTION_IMPORTER_RETURN_APIADDRESS){
								for(j = 0; j < PEExports->NumberOfFunctions; j++){ //NumberOfNames
									if(lstrcmpiA((LPCSTR)szAPIName, (LPCSTR)(ExportedFunctionNames->ExportedItem + LoadedModules[i][1])) == NULL){
										ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)((ULONG_PTR)ExportedFunctionOrdinals + j * 2);
										ExportedFunctions = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctions + (ExportedFunctionOrdinals->OrdinalNumber) * 4);
										GetModuleBaseNameA(hProcess, (HMODULE)LoadedModules[i][0], (LPSTR)engineFoundDLLName, 512);
										RtlZeroMemory(&engineFoundAPIName, 512);
										ExportedFunctions = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctions + (j + PEExports->Base) * 4);
										APIFoundAddress = ExportedFunctions->ExportedItem + LoadedModules[i][0];
										APINameFound = true;
										FoundIndex = i;
										break;
									}
									ExportedFunctionNames = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctionNames + 4);
								}					
							}
						}__except(EXCEPTION_EXECUTE_HANDLER){
							RtlZeroMemory(&engineFoundAPIName, 512);
							APINameFound = false;
						}
					}
				}
			}
			i++;
		}
		
		if(ReturnType == UE_OPTION_IMPORTER_RETURN_FORWARDER_DLLNAME || ReturnType == UE_OPTION_IMPORTER_RETURN_FORWARDER_APINAME || ReturnType == UE_OPTION_IMPORTER_RETURN_FORWARDER_DLLINDEX){
			RealignedAPIAddress = (ULONG_PTR)EngineGlobalAPIHandler(hProcess, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_REALIGN_APIADDRESS);
			if(z <= 1){
				z = 2;
			}
			for(i = y; i >= z; i--){
				FileMapVA = LoadedModules[i][1];
				if(FileMapVA != NULL){
					DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
					RtlZeroMemory(&RemoteModuleInfo, sizeof MODULEINFO);
					GetModuleInformation(GetCurrentProcess(), (HMODULE)LoadedModules[i][1], &RemoteModuleInfo, sizeof MODULEINFO);
					if(ValidateHeader || EngineValidateHeader((ULONG_PTR)LoadedModules[i][1], GetCurrentProcess(), RemoteModuleInfo.lpBaseOfDll, DOSHeader, false)){
						__try{
							PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
							PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
							if(PEHeader32->OptionalHeader.Magic == 0x10B){
								FileIs64 = false;
							}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
								FileIs64 = true;
							}else{
								SkipModule = true;
							}
							if(!SkipModule){
								if(!FileIs64){
									PEExports = (PIMAGE_EXPORT_DIRECTORY)(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + LoadedModules[i][1]);
									ExportedFunctionNames = (PEXPORTED_DATA)(PEExports->AddressOfNames + LoadedModules[i][1]);
									ExportedFunctions = (PEXPORTED_DATA)(PEExports->AddressOfFunctions + LoadedModules[i][1]);
									ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)(PEExports->AddressOfNameOrdinals + LoadedModules[i][1]);
								}else{
									PEExports = (PIMAGE_EXPORT_DIRECTORY)(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + LoadedModules[i][1]);				
									ExportedFunctionNames = (PEXPORTED_DATA)(PEExports->AddressOfNames + LoadedModules[i][1]);
									ExportedFunctions = (PEXPORTED_DATA)(PEExports->AddressOfFunctions + LoadedModules[i][1]);
									ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)(PEExports->AddressOfNameOrdinals + LoadedModules[i][1]);
								}
								for(j = 0; j < PEExports->NumberOfFunctions; j++){
									if(EngineIsPointedMemoryString((ULONG_PTR)ExportedFunctions->ExportedItem + LoadedModules[i][1])){
										RtlZeroMemory(&szFwdAPIName, 512);
										RtlZeroMemory(&szFwdDLLName, 512);
										if(EngineExtractForwarderData((ULONG_PTR)ExportedFunctions->ExportedItem + LoadedModules[i][1], &szFwdDLLName, &szFwdAPIName)){
											if((ULONG_PTR)GetProcAddress(GetModuleHandleA(szFwdDLLName), szFwdAPIName) == RealignedAPIAddress){
												GetModuleBaseNameA(hProcess, (HMODULE)LoadedModules[i][0], (LPSTR)engineFoundDLLName, 512);
												RtlZeroMemory(&engineFoundAPIName, 512);
												x = j;
												for(j = 0; j < PEExports->NumberOfNames; j++){
													if(ExportedFunctionOrdinals->OrdinalNumber != x){
														ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)((ULONG_PTR)ExportedFunctionOrdinals + 2);
													}else{
														break;
													}
												}
												ExportedFunctionNames = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctionNames + j * 4);
												lstrcpyA((LPSTR)engineFoundAPIName, (LPCSTR)(ExportedFunctionNames->ExportedItem + LoadedModules[i][1]));
												APINameFound = true;
												FoundIndex = i;
												break;
											}
										}
									}
									ExportedFunctions = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctions + 4);
								}
							}
						}__except(EXCEPTION_EXECUTE_HANDLER){
							RtlZeroMemory(&szFwdAPIName, 512);
							RtlZeroMemory(&szFwdDLLName, 512);
							APINameFound = false;
						}
					}
				}
				if(APINameFound){
					break;
				}
			}
		}
		i = 1;
		while(EnumeratedModules[i] != NULL){
			if(engineAlowModuleLoading){
				if(LoadedModules[i][2] == 1){
					FreeLibrary((HMODULE)LoadedModules[i][1]);
				}
			}else{
				if(LoadedModules[i][2] == 1){
					VirtualFree((void*)LoadedModules[i][1], NULL, MEM_RELEASE);
				}
			}
			i++;
		}
		if(APINameFound){
			if(ReturnType == UE_OPTION_IMPORTER_RETURN_APINAME || ReturnType == UE_OPTION_IMPORTER_RETURN_FORWARDER_APINAME){
				if(ReturnType == UE_OPTION_IMPORTER_RETURN_APINAME && engineCheckForwarders == true){
					if(engineAlowModuleLoading == true || (engineAlowModuleLoading == false && LoadedModules[FoundIndex][2] != 1)){
						if(lstrcmpiA(engineFoundDLLName, "ntdll.dll") == NULL){
							ForwarderData = (ULONG_PTR)EngineGlobalAPIHandler(handleProcess, EnumedModulesBases, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_FORWARDER_APINAME);
						}else{
							ForwarderData = NULL;
						}
						if(ForwarderData != NULL){
							return(ForwarderData);
						}else{
							return((ULONG_PTR)engineFoundAPIName);
						}
					}else{
						return((ULONG_PTR)engineFoundAPIName);
					}
				}else{
					return((ULONG_PTR)engineFoundAPIName);
				}
			}else if(ReturnType == UE_OPTION_IMPORTER_RETURN_APIADDRESS){
				return(APIFoundAddress);
			}else if(ReturnType == UE_OPTION_IMPORTER_RETURN_DLLNAME || ReturnType == UE_OPTION_IMPORTER_RETURN_FORWARDER_DLLNAME){
				if(ReturnType == UE_OPTION_IMPORTER_RETURN_DLLNAME && engineCheckForwarders == true){
					if(engineAlowModuleLoading == true || (engineAlowModuleLoading == false && LoadedModules[FoundIndex][2] != 1)){
						if(lstrcmpiA(engineFoundDLLName, "ntdll.dll") == NULL){
							ForwarderData = (ULONG_PTR)EngineGlobalAPIHandler(handleProcess, EnumedModulesBases, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_FORWARDER_DLLNAME);
						}else{
							ForwarderData = NULL;
						}
						if(ForwarderData != NULL){
							return(ForwarderData);
						}else{
							return((ULONG_PTR)engineFoundDLLName);
						}
					}else{
						return((ULONG_PTR)engineFoundDLLName);
					}
				}else{
					return((ULONG_PTR)engineFoundDLLName);
				}
			}else if(ReturnType == UE_OPTION_IMPORTER_RETURN_DLLINDEX || ReturnType == UE_OPTION_IMPORTER_RETURN_FORWARDER_DLLINDEX){
				if(ReturnType == UE_OPTION_IMPORTER_RETURN_DLLINDEX && engineCheckForwarders == true){
					if(engineAlowModuleLoading == true || (engineAlowModuleLoading == false && LoadedModules[FoundIndex][2] != 1)){
						if(lstrcmpiA(engineFoundDLLName, "ntdll.dll") == NULL){
							ForwarderData = (ULONG_PTR)EngineGlobalAPIHandler(handleProcess, EnumedModulesBases, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_FORWARDER_DLLINDEX);
						}else{
							ForwarderData = NULL;
						}
						if(ForwarderData != NULL){
							return(ForwarderData);
						}else{
							return(FoundIndex);
						}
					}else{
						return(FoundIndex);
					}
				}else{
					return(FoundIndex);
				}
			}else if(ReturnType == UE_OPTION_IMPORTER_RETURN_DLLBASE){
				return(APIFoundAddress);
			}else if(ReturnType == UE_OPTION_IMPORTER_RETURN_NEAREST_APIADDRESS){
				return(APIFoundAddress);
			}else if(ReturnType == UE_OPTION_IMPORTER_RETURN_NEAREST_APINAME){
				if(engineCheckForwarders){
					if(engineAlowModuleLoading == true || (engineAlowModuleLoading == false && LoadedModules[FoundIndex][2] != 1)){
						if(lstrcmpiA(engineFoundDLLName, "ntdll.dll") == NULL){
							ForwarderData = (ULONG_PTR)EngineGlobalAPIHandler(handleProcess, EnumedModulesBases, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_FORWARDER_APINAME);
						}else{
							ForwarderData = NULL;
						}
						if(ForwarderData != NULL){
							return(ForwarderData);
						}else{
							return((ULONG_PTR)engineFoundAPIName);
						}
					}else{
						return((ULONG_PTR)engineFoundAPIName);
					}
				}else{
					return((ULONG_PTR)engineFoundAPIName);
				}
			}else{
				return(APIFoundAddress);
			}
		}else{
			return(NULL);
		}
	}else{
		return(NULL);	
	}
	return(NULL);
}
// UnpackEngine.Dumper.functions:
__declspec(dllexport) bool __stdcall DumpProcess(HANDLE hProcess, LPVOID ImageBase, char* szDumpFileName, ULONG_PTR EntryPoint){
	
	int i = 0;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_DOS_HEADER DOSFixHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_NT_HEADERS32 PEFixHeader32;
	PIMAGE_NT_HEADERS64 PEFixHeader64;
	PIMAGE_SECTION_HEADER PESections;
	PIMAGE_SECTION_HEADER PEFixSection;
	ULONG_PTR ueNumberOfBytesRead = 0;
	DWORD uedNumberOfBytesRead = 0;
	DWORD SizeOfImageDump = 0;
	int NumberOfSections = 0;
	BOOL FileIs64 = false;
	HANDLE hFile = 0;
	DWORD RealignedVirtualSize = 0;
	ULONG_PTR ProcReadBase = 0;
	LPVOID ReadBase = ImageBase;
	SIZE_T CalculatedHeaderSize = NULL;
	SIZE_T AlignedHeaderSize = NULL;
	LPVOID ueReadBuffer = VirtualAlloc(NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE);
	LPVOID ueCopyBuffer = VirtualAlloc(NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE);

	if(ReadProcessMemory(hProcess, ImageBase, ueReadBuffer, 0x1000, &ueNumberOfBytesRead)){
		DOSHeader = (PIMAGE_DOS_HEADER)ueReadBuffer;
		CalculatedHeaderSize = DOSHeader->e_lfanew + sizeof IMAGE_DOS_HEADER + sizeof IMAGE_NT_HEADERS64;
		if(CalculatedHeaderSize > 0x1000){
			if(CalculatedHeaderSize % 0x1000 == NULL){
				AlignedHeaderSize = 0x1000;
			}else{
				AlignedHeaderSize = ((CalculatedHeaderSize / 0x1000) + 1) * 0x1000;
			}
			VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
			VirtualFree(ueCopyBuffer, NULL, MEM_RELEASE);
			ueReadBuffer = VirtualAlloc(NULL, AlignedHeaderSize, MEM_COMMIT, PAGE_READWRITE);
			ueCopyBuffer = VirtualAlloc(NULL, AlignedHeaderSize, MEM_COMMIT, PAGE_READWRITE);
			if(!ReadProcessMemory(hProcess, ImageBase, ueReadBuffer, AlignedHeaderSize, &ueNumberOfBytesRead)){
				VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
				VirtualFree(ueCopyBuffer, NULL, MEM_RELEASE);
				return(false);
			}else{
				DOSHeader = (PIMAGE_DOS_HEADER)ueReadBuffer;
			}
		}else{
			CalculatedHeaderSize = 0x1000;
			AlignedHeaderSize = 0x1000;
		}
		if(EngineValidateHeader((ULONG_PTR)ueReadBuffer, hProcess, ImageBase, DOSHeader, false)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
				VirtualFree(ueCopyBuffer, NULL, MEM_RELEASE);
				return(false);
			}
			if(!FileIs64){
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				NumberOfSections = PEHeader32->FileHeader.NumberOfSections;
				NumberOfSections++;
				if(PEHeader32->OptionalHeader.SizeOfImage % PEHeader32->OptionalHeader.SectionAlignment == NULL){
					SizeOfImageDump = ((PEHeader32->OptionalHeader.SizeOfImage / PEHeader32->OptionalHeader.SectionAlignment)) * PEHeader32->OptionalHeader.SectionAlignment;
				}else{
					SizeOfImageDump = ((PEHeader32->OptionalHeader.SizeOfImage / PEHeader32->OptionalHeader.SectionAlignment) + 1) * PEHeader32->OptionalHeader.SectionAlignment;				
				}
				SizeOfImageDump = SizeOfImageDump - PEHeader32->OptionalHeader.SectionAlignment;
				hFile = CreateFileA(szDumpFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 
				if(hFile != INVALID_HANDLE_VALUE){
					if(ReadProcessMemory(hProcess, ImageBase, ueCopyBuffer, AlignedHeaderSize, &ueNumberOfBytesRead)){
						__try{
							DOSFixHeader = (PIMAGE_DOS_HEADER)ueCopyBuffer;
							PEFixHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSFixHeader + DOSFixHeader->e_lfanew);
							PEFixSection = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEFixHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
							if(PEFixHeader32->OptionalHeader.FileAlignment > 0x200){
								PEFixHeader32->OptionalHeader.FileAlignment = PEHeader32->OptionalHeader.SectionAlignment;
							}
							PEFixHeader32->OptionalHeader.AddressOfEntryPoint = (DWORD)(EntryPoint - (ULONG_PTR)ImageBase);
							PEFixHeader32->OptionalHeader.ImageBase = (DWORD)((ULONG_PTR)ImageBase);
							i = NumberOfSections;
							while(i >= 1){
								PEFixSection->PointerToRawData = PEFixSection->VirtualAddress;
								RealignedVirtualSize = (PEFixSection->Misc.VirtualSize / PEHeader32->OptionalHeader.SectionAlignment) * PEHeader32->OptionalHeader.SectionAlignment;
								if(RealignedVirtualSize < PEFixSection->Misc.VirtualSize){
									RealignedVirtualSize = RealignedVirtualSize + PEHeader32->OptionalHeader.SectionAlignment;
								}
								PEFixSection->SizeOfRawData = RealignedVirtualSize;
								PEFixSection->Misc.VirtualSize = RealignedVirtualSize;
								PEFixSection = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEFixSection + IMAGE_SIZEOF_SECTION_HEADER);
								i--;
							}
							WriteFile(hFile, ueCopyBuffer, (DWORD)AlignedHeaderSize, &uedNumberOfBytesRead, NULL);
							ReadBase = (LPVOID)((ULONG_PTR)ReadBase + AlignedHeaderSize - PEHeader32->OptionalHeader.SectionAlignment);
							while(SizeOfImageDump > NULL){
								ProcReadBase = (ULONG_PTR)ReadBase + PEHeader32->OptionalHeader.SectionAlignment;
								ReadBase = (LPVOID)ProcReadBase;
								if(SizeOfImageDump >= PEHeader32->OptionalHeader.SectionAlignment){ 
									RtlZeroMemory(ueCopyBuffer, 0x2000);
									ReadProcessMemory(hProcess, ReadBase, ueCopyBuffer, PEHeader32->OptionalHeader.SectionAlignment, &ueNumberOfBytesRead);
									WriteFile(hFile, ueCopyBuffer, PEHeader32->OptionalHeader.SectionAlignment, &uedNumberOfBytesRead, NULL);
									SizeOfImageDump = SizeOfImageDump - PEHeader32->OptionalHeader.SectionAlignment;
								}else{
									RtlZeroMemory(ueCopyBuffer, 0x2000);							
									ReadProcessMemory(hProcess, ReadBase, ueCopyBuffer, SizeOfImageDump, &ueNumberOfBytesRead);						
									WriteFile(hFile, ueCopyBuffer, SizeOfImageDump, &uedNumberOfBytesRead, NULL);
									SizeOfImageDump = NULL;
								}
							}
							EngineCloseHandle(hFile);
							VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
							VirtualFree(ueCopyBuffer, NULL, MEM_RELEASE);
							return(true);
						}__except(EXCEPTION_EXECUTE_HANDLER){
							EngineCloseHandle(hFile);
							VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
							VirtualFree(ueCopyBuffer, NULL, MEM_RELEASE);
							return(false);
						}
					}else{
						EngineCloseHandle(hFile);
						VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
						VirtualFree(ueCopyBuffer, NULL, MEM_RELEASE);
						return(false);
					}
				}else{
					VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
					VirtualFree(ueCopyBuffer, NULL, MEM_RELEASE);
					return(false);
				}
			}else{
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				NumberOfSections = PEHeader64->FileHeader.NumberOfSections;
				NumberOfSections++;
				if(PEHeader64->OptionalHeader.SizeOfImage % PEHeader64->OptionalHeader.SectionAlignment == NULL){
					SizeOfImageDump = ((PEHeader64->OptionalHeader.SizeOfImage / PEHeader64->OptionalHeader.SectionAlignment)) * PEHeader64->OptionalHeader.SectionAlignment;
				}else{
					SizeOfImageDump = ((PEHeader64->OptionalHeader.SizeOfImage / PEHeader64->OptionalHeader.SectionAlignment) + 1) * PEHeader64->OptionalHeader.SectionAlignment;				
				}
				SizeOfImageDump = SizeOfImageDump - PEHeader64->OptionalHeader.SectionAlignment;
				hFile = CreateFileA(szDumpFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 
				if(hFile != INVALID_HANDLE_VALUE){
					if(ReadProcessMemory(hProcess, ImageBase, ueCopyBuffer, AlignedHeaderSize, &ueNumberOfBytesRead)){
						__try{
							DOSFixHeader = (PIMAGE_DOS_HEADER)ueCopyBuffer;
							PEFixHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSFixHeader + DOSFixHeader->e_lfanew);
							PEFixSection = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEFixHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
							if(PEFixHeader64->OptionalHeader.FileAlignment > 0x200){
								PEFixHeader64->OptionalHeader.FileAlignment = PEHeader64->OptionalHeader.SectionAlignment;
							}
							PEFixHeader64->OptionalHeader.AddressOfEntryPoint = (DWORD)(EntryPoint - (ULONG_PTR)ImageBase);
							PEFixHeader64->OptionalHeader.ImageBase = (DWORD64)((ULONG_PTR)ImageBase);
							i = NumberOfSections;
							while(i >= 1){
								PEFixSection->PointerToRawData = PEFixSection->VirtualAddress;
								RealignedVirtualSize = (PEFixSection->Misc.VirtualSize / PEHeader64->OptionalHeader.SectionAlignment) * PEHeader64->OptionalHeader.SectionAlignment;
								if(RealignedVirtualSize < PEFixSection->Misc.VirtualSize){
									RealignedVirtualSize = RealignedVirtualSize + PEHeader64->OptionalHeader.SectionAlignment;
								}
								PEFixSection->SizeOfRawData = RealignedVirtualSize;
								PEFixSection->Misc.VirtualSize = RealignedVirtualSize;
								PEFixSection = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEFixSection + IMAGE_SIZEOF_SECTION_HEADER);
								i--;
							}
							WriteFile(hFile,ueCopyBuffer, (DWORD)AlignedHeaderSize, &uedNumberOfBytesRead, NULL);
							ReadBase = (LPVOID)((ULONG_PTR)ReadBase + (DWORD)AlignedHeaderSize - PEHeader32->OptionalHeader.SectionAlignment);
							while(SizeOfImageDump > NULL){
								ProcReadBase = (ULONG_PTR)ReadBase + PEHeader64->OptionalHeader.SectionAlignment;
								ReadBase = (LPVOID)ProcReadBase;
								if(SizeOfImageDump >= PEHeader64->OptionalHeader.SectionAlignment){ 
									RtlZeroMemory(ueCopyBuffer, 0x2000);
									ReadProcessMemory(hProcess, ReadBase, ueCopyBuffer, PEHeader32->OptionalHeader.SectionAlignment, &ueNumberOfBytesRead);
									WriteFile(hFile, ueCopyBuffer, PEHeader64->OptionalHeader.SectionAlignment, &uedNumberOfBytesRead, NULL);
									SizeOfImageDump = SizeOfImageDump - PEHeader64->OptionalHeader.SectionAlignment;
								}else{
									RtlZeroMemory(ueCopyBuffer, 0x2000);							
									ReadProcessMemory(hProcess, ReadBase, ueCopyBuffer, SizeOfImageDump, &ueNumberOfBytesRead);						
									WriteFile(hFile, ueCopyBuffer, SizeOfImageDump, &uedNumberOfBytesRead, NULL);
									SizeOfImageDump = NULL;
								}
							}
							EngineCloseHandle(hFile);
							VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
							VirtualFree(ueCopyBuffer, NULL, MEM_RELEASE);
							return(true);
						}__except(EXCEPTION_EXECUTE_HANDLER){
							VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
							VirtualFree(ueCopyBuffer, NULL, MEM_RELEASE);
							return(false);
						}
					}else{
						EngineCloseHandle(hFile);
						VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
						VirtualFree(ueCopyBuffer, NULL, MEM_RELEASE);
						return(false);
					}
				}else{
					EngineCloseHandle(hFile);
					VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
					VirtualFree(ueCopyBuffer, NULL, MEM_RELEASE);
					return(false);
				}
			}
		}else{
			VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
			VirtualFree(ueCopyBuffer, NULL, MEM_RELEASE);
			return(false);
		}
	}else{
		VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
		VirtualFree(ueCopyBuffer, NULL, MEM_RELEASE);		
		return(false);
	}
	return(false);
}

__declspec(dllexport) bool __stdcall DumpProcessEx(DWORD ProcessId, LPVOID ImageBase, char* szDumpFileName, ULONG_PTR EntryPoint){
	
	HANDLE hProcess = 0;
	BOOL ReturnValue = false;

	hProcess = OpenProcess(PROCESS_VM_READ, FALSE, ProcessId);
	if(hProcess != INVALID_HANDLE_VALUE){
		ReturnValue = DumpProcess(hProcess, ImageBase, szDumpFileName, EntryPoint);
		EngineCloseHandle(hProcess);
		if(ReturnValue){
			return(true);
		}else{
			return(false);
		}
	}else{
		return(false);
	}
}
__declspec(dllexport) bool __stdcall DumpMemory(HANDLE hProcess, LPVOID MemoryStart, ULONG_PTR MemorySize, char* szDumpFileName){

	ULONG_PTR ueNumberOfBytesRead = 0;
	DWORD uedNumberOfBytesRead = 0;
	HANDLE hFile = 0;
	LPVOID ReadBase = MemoryStart;
	ULONG_PTR ProcReadBase = (ULONG_PTR)ReadBase;
	LPVOID ueCopyBuffer = VirtualAlloc(NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE);

	hFile = CreateFileA(szDumpFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 
	if(hFile != INVALID_HANDLE_VALUE){
		while(MemorySize > NULL){
			ReadBase = (LPVOID)ProcReadBase;
			if(MemorySize >= 0x1000){ 
				RtlZeroMemory(ueCopyBuffer,0x2000);
				ReadProcessMemory(hProcess, ReadBase, ueCopyBuffer, 0x1000, &ueNumberOfBytesRead);
				WriteFile(hFile,ueCopyBuffer, 0x1000, &uedNumberOfBytesRead, NULL);
				MemorySize = MemorySize - 0x1000;
			}else{
				RtlZeroMemory(ueCopyBuffer,0x2000);							
				ReadProcessMemory(hProcess, ReadBase, ueCopyBuffer, MemorySize, &ueNumberOfBytesRead);						
				WriteFile(hFile, ueCopyBuffer, (DWORD)MemorySize, &uedNumberOfBytesRead, NULL);
				MemorySize = NULL;
			}
			ProcReadBase = (ULONG_PTR)ReadBase + 0x1000;
		}
		EngineCloseHandle(hFile);
		VirtualFree(ueCopyBuffer, NULL, MEM_RELEASE);
		return(true);
	}else{
		VirtualFree(ueCopyBuffer, NULL, MEM_RELEASE);
		return(false);
	}
	return(true);
}
__declspec(dllexport) bool __stdcall DumpMemoryEx(DWORD ProcessId, LPVOID MemoryStart, ULONG_PTR MemorySize, char* szDumpFileName){
	
	HANDLE hProcess = 0;
	BOOL ReturnValue = false;

	hProcess = OpenProcess(PROCESS_VM_READ, FALSE, ProcessId);
	if(hProcess != INVALID_HANDLE_VALUE){
		ReturnValue = DumpMemory(hProcess, MemoryStart, MemorySize, szDumpFileName);
		EngineCloseHandle(hProcess);
		if(ReturnValue){
			return(true);
		}else{
			return(false);
		}
	}else{
		return(false);
	}
}
__declspec(dllexport) bool __stdcall DumpRegions(HANDLE hProcess, char* szDumpFolder, bool DumpAboveImageBaseOnly){

	int i;
	DWORD Dummy = NULL;
	char szDumpName[512];
	char szDumpFileName[512];
	MEMORY_BASIC_INFORMATION MemInfo; 
	ULONG_PTR DumpAddress = NULL;
	ULONG_PTR EnumeratedModules[0x2000];
	bool AddressIsModuleBase = false;

	if(hProcess != NULL){
		EnumProcessModules(hProcess, (HMODULE*)EnumeratedModules, 0x2000, &Dummy);
		while(VirtualQueryEx(hProcess, (LPVOID)DumpAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION) != NULL){
			AddressIsModuleBase = false;
			for(i = 0; i < 0x2000; i++){
				if(EnumeratedModules[i] == (ULONG_PTR)MemInfo.AllocationBase){
					AddressIsModuleBase = true;
					i = 0x2000;
				}else if(EnumeratedModules[i] == 0){
					i = 0x2000;
				}
			}
			if(!(MemInfo.Protect & PAGE_NOACCESS) && AddressIsModuleBase == false){
				if(DumpAboveImageBaseOnly == false || (DumpAboveImageBaseOnly == true && EnumeratedModules[0] < (ULONG_PTR)MemInfo.BaseAddress)){
					RtlZeroMemory(&szDumpName, 512);
					RtlZeroMemory(&szDumpFileName, 512);
					lstrcpyA(szDumpFileName, szDumpFolder);
					if(szDumpFileName[lstrlenA(szDumpFileName)-1] != 0x5C){
						szDumpFileName[lstrlenA(szDumpFileName)] = 0x5C;
					}
					wsprintfA(szDumpName, "Dump-%x_%x.dmp", (ULONG_PTR)MemInfo.BaseAddress, (ULONG_PTR)MemInfo.RegionSize);
					lstrcatA(szDumpFileName, szDumpName);
					DumpMemory(hProcess, (LPVOID)MemInfo.BaseAddress, (ULONG_PTR)MemInfo.RegionSize, szDumpFileName);
				}
			}
			DumpAddress = DumpAddress + (ULONG_PTR)MemInfo.RegionSize;
		}
		return(true);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall DumpRegionsEx(DWORD ProcessId, char* szDumpFolder, bool DumpAboveImageBaseOnly){

	HANDLE hProcess = 0;
	BOOL ReturnValue = false;

	hProcess = OpenProcess(PROCESS_VM_READ, FALSE, ProcessId);
	if(hProcess != INVALID_HANDLE_VALUE){
		ReturnValue = DumpRegions(hProcess, szDumpFolder, DumpAboveImageBaseOnly);
		EngineCloseHandle(hProcess);
		if(ReturnValue){
			return(true);
		}else{
			return(false);
		}
	}else{
		return(false);
	}
}
__declspec(dllexport) bool __stdcall DumpModule(HANDLE hProcess, LPVOID ModuleBase, char* szDumpFileName){

	int i;
	DWORD Dummy = NULL;
	MODULEINFO RemoteModuleInfo;
	ULONG_PTR EnumeratedModules[0x2000];

	if(EnumProcessModules(hProcess, (HMODULE*)EnumeratedModules, 0x2000, &Dummy)){
		for(i = 0; i < 512; i++){
			if(EnumeratedModules[i] == (ULONG_PTR)ModuleBase){
				GetModuleInformation(hProcess, (HMODULE)EnumeratedModules[i], &RemoteModuleInfo, sizeof MODULEINFO);
				return(DumpMemory(hProcess, (LPVOID)EnumeratedModules[i], RemoteModuleInfo.SizeOfImage, szDumpFileName));
			}
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall DumpModuleEx(DWORD ProcessId, LPVOID ModuleBase, char* szDumpFileName){

	HANDLE hProcess = 0;
	BOOL ReturnValue = false;

	hProcess = OpenProcess(PROCESS_VM_READ, FALSE, ProcessId);
	if(hProcess != INVALID_HANDLE_VALUE){
		ReturnValue = DumpModule(hProcess, ModuleBase, szDumpFileName);
		EngineCloseHandle(hProcess);
		if(ReturnValue){
			return(true);
		}else{
			return(false);
		}
	}else{
		return(false);
	}
}
__declspec(dllexport) bool __stdcall PastePEHeader(HANDLE hProcess, LPVOID ImageBase, char* szDebuggedFileName){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	IMAGE_NT_HEADERS32 RemotePEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	IMAGE_NT_HEADERS64 RemotePEHeader64;
	ULONG_PTR ueNumberOfBytesRead = 0;
	DWORD uedNumberOfBytesRead = 0;
	DWORD FileSize = 0;
	DWORD PEHeaderSize = 0;
	ULONG_PTR dwImageBase = (ULONG_PTR)ImageBase;
	BOOL FileIs64 = false;
	HANDLE hFile = 0;
	SIZE_T CalculatedHeaderSize = NULL;
	LPVOID ueReadBuffer = VirtualAlloc(NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE);

	hFile = CreateFileA(szDebuggedFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
	if(hFile != INVALID_HANDLE_VALUE){
		FileSize = GetFileSize(hFile, NULL);
		if(FileSize < 0x1000){
			ReadFile(hFile, ueReadBuffer, FileSize, &uedNumberOfBytesRead, NULL);
		}else{
			ReadFile(hFile, ueReadBuffer, 0x1000, &uedNumberOfBytesRead, NULL);
		}
		if(FileSize > 0x200){
			DOSHeader = (PIMAGE_DOS_HEADER)ueReadBuffer;
			if(EngineValidateHeader((ULONG_PTR)ueReadBuffer, hProcess, ImageBase, DOSHeader, false)){
				PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
				PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
				CalculatedHeaderSize = DOSHeader->e_lfanew + sizeof IMAGE_DOS_HEADER + sizeof IMAGE_NT_HEADERS64;
				if(CalculatedHeaderSize > 0x1000){
					SetFilePointer(hFile, NULL, NULL, FILE_BEGIN);
					VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
					ueReadBuffer = VirtualAlloc(NULL, CalculatedHeaderSize, MEM_COMMIT, PAGE_READWRITE);
					if(!ReadFile(hFile, ueReadBuffer, (DWORD)CalculatedHeaderSize, &uedNumberOfBytesRead, NULL)){
						EngineCloseHandle(hFile);
						VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
						return(false);
					}
				}
				if(PEHeader32->OptionalHeader.Magic == 0x10B){
					if(ReadProcessMemory(hProcess, (LPVOID)((ULONG_PTR)ImageBase + DOSHeader->e_lfanew), &RemotePEHeader32, sizeof IMAGE_NT_HEADERS32, &ueNumberOfBytesRead)){
						PEHeaderSize = PEHeader32->FileHeader.NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4;
						FileIs64 = false;
					}
				}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
					if(ReadProcessMemory(hProcess, (LPVOID)((ULONG_PTR)ImageBase + DOSHeader->e_lfanew), &RemotePEHeader64, sizeof IMAGE_NT_HEADERS32, &ueNumberOfBytesRead)){
						PEHeaderSize = PEHeader64->FileHeader.NumberOfSections * IMAGE_SIZEOF_SECTION_HEADER + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4;
						FileIs64 = false;
					}
				}else{
					EngineCloseHandle(hFile);
					VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
					return(false);
				}
				if(!FileIs64){
					PEHeader32->OptionalHeader.ImageBase = (DWORD)(dwImageBase);
					if(WriteProcessMemory(hProcess, ImageBase, ueReadBuffer, PEHeaderSize, &ueNumberOfBytesRead)){
						EngineCloseHandle(hFile);
						VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
						return(true);
					}else{
						EngineCloseHandle(hFile);
						VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
						return(false);
					}
				}else{
					PEHeader64->OptionalHeader.ImageBase = dwImageBase;
					if(WriteProcessMemory(hProcess, ImageBase, ueReadBuffer, PEHeaderSize, &ueNumberOfBytesRead)){
						EngineCloseHandle(hFile);
						VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
						return(true);
					}else{
						EngineCloseHandle(hFile);
						VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
						return(false);
					}
				}
			}else{
				EngineCloseHandle(hFile);
				VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
				return(false);
			}
		}else{
			EngineCloseHandle(hFile);
			VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
			return(false);
		}
	}else{
		EngineCloseHandle(hFile);
		VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);		
		return(false);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall ExtractSection(char* szFileName, char* szDumpFileName, DWORD SectionNumber){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD NumberOfBytesWritten;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	HANDLE hFile;

	if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(false);
			}
			if(!FileIs64){
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				if(SectionNumber <= PEHeader32->FileHeader.NumberOfSections){
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + SectionNumber * IMAGE_SIZEOF_SECTION_HEADER);
					hFile = CreateFileA(szDumpFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					if(hFile != INVALID_HANDLE_VALUE){
						__try{
							WriteFile(hFile, (LPCVOID)(FileMapVA + PESections->PointerToRawData), PESections->SizeOfRawData, &NumberOfBytesWritten, NULL);
							EngineCloseHandle(hFile);
							UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
							return(true);
						}__except(EXCEPTION_EXECUTE_HANDLER){
							EngineCloseHandle(hFile);
							UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);		
							DeleteFileA(szDumpFileName);
							return(false);
						}
					}
				}
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(false);
			}else{
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
				if(SectionNumber <= PEHeader64->FileHeader.NumberOfSections){
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + SectionNumber * IMAGE_SIZEOF_SECTION_HEADER);
					hFile = CreateFileA(szDumpFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					if(hFile != INVALID_HANDLE_VALUE){
						__try{
							WriteFile(hFile, (LPCVOID)(FileMapVA + PESections->PointerToRawData), PESections->SizeOfRawData, &NumberOfBytesWritten, NULL);
							EngineCloseHandle(hFile);
							UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
							return(true);
						}__except(EXCEPTION_EXECUTE_HANDLER){
							EngineCloseHandle(hFile);
							UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);		
							DeleteFileA(szDumpFileName);
							return(false);
						}
					}
				}
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(false);
			}
		}else{
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			return(false);		
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall ResortFileSections(char* szFileName){

	int i = 0;
	int j = 0;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD SectionNumber = 0;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	char szBackupFile[512] = {};
	char szBackupItem[512] = {};
	ULONG_PTR fileSectionData[MAXIMUM_SECTION_NUMBER][3];
	ULONG_PTR fileSectionTemp;
	LPVOID sortedFileName;

	if(engineBackupForCriticalFunctions && CreateGarbageItem(&szBackupItem, 512)){
		if(!FillGarbageItem(szBackupItem, szFileName, &szBackupFile, 512)){
			RtlZeroMemory(&szBackupItem, 512);
			lstrcpyA(szBackupFile, szFileName);
		}
	}else{
		RtlZeroMemory(&szBackupItem, 512);
		lstrcpyA(szBackupFile, szFileName);
	}
	if(MapFileEx(szBackupFile, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				RemoveGarbageItem(szBackupItem, true);
				return(false);
			}
			if(!FileIs64){
				sortedFileName = VirtualAlloc(NULL, FileSize, MEM_COMMIT, PAGE_READWRITE);
				__try{
					RtlMoveMemory(sortedFileName, (LPVOID)FileMapVA, FileSize);
					SectionNumber = PEHeader32->FileHeader.NumberOfSections;
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
					while(SectionNumber > 0){
						fileSectionData[i][0] = (ULONG_PTR)(PESections->PointerToRawData);
						fileSectionData[i][1] = PESections->SizeOfRawData;
						fileSectionData[i][2] = PEHeader32->FileHeader.NumberOfSections - SectionNumber;
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
						i++;
					}
					for(j = 0; j < PEHeader32->FileHeader.NumberOfSections; j++){
						for(i = 0; i < PEHeader32->FileHeader.NumberOfSections; i++){
							if(fileSectionData[i][0] > fileSectionData[j][0]){
								fileSectionTemp = fileSectionData[j][0];
								fileSectionData[j][0] = fileSectionData[i][0];
								fileSectionData[i][0] = fileSectionTemp;
								fileSectionTemp = fileSectionData[j][1];
								fileSectionData[j][1] = fileSectionData[i][1];
								fileSectionData[i][1] = fileSectionTemp;
							}
						}
					}
					for(i = 0; i < PEHeader32->FileHeader.NumberOfSections; i++){
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 - FileMapVA + (ULONG_PTR)sortedFileName + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + fileSectionData[i][2] * IMAGE_SIZEOF_SECTION_HEADER);
						RtlMoveMemory((LPVOID)((ULONG_PTR)sortedFileName + fileSectionData[i][0]), (LPVOID)((ULONG_PTR)FileMapVA + PESections->PointerToRawData), fileSectionData[i][1]);
						PESections->PointerToRawData = (DWORD)fileSectionData[i][0];
						PESections->SizeOfRawData = (DWORD)fileSectionData[i][1];
					}
					RtlMoveMemory((LPVOID)FileMapVA, sortedFileName, FileSize);
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					VirtualFree(sortedFileName, NULL, MEM_RELEASE);
					if(szBackupItem[0] != NULL){
						if(CopyFileA(szBackupFile, szFileName, false)){
							RemoveGarbageItem(szBackupItem, true);
							return(true);
						}else{
							RemoveGarbageItem(szBackupItem, true);
							return(false);
						}
					}else{
						return(true);
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					VirtualFree(sortedFileName, NULL, MEM_RELEASE);
					RemoveGarbageItem(szBackupItem, true);
					return(false);
				}
			}else{
				sortedFileName = VirtualAlloc(NULL, FileSize, MEM_COMMIT, PAGE_READWRITE);
				__try{
					RtlMoveMemory(sortedFileName, (LPVOID)FileMapVA, FileSize);
					SectionNumber = PEHeader64->FileHeader.NumberOfSections;
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
					while(SectionNumber > 0){
						fileSectionData[i][0] = (ULONG_PTR)(PESections->PointerToRawData);
						fileSectionData[i][1] = PESections->SizeOfRawData;
						fileSectionData[i][2] = PEHeader64->FileHeader.NumberOfSections - SectionNumber;
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
						i++;
					}
					for(j = 0; j < PEHeader64->FileHeader.NumberOfSections; j++){
						for(i = 0; i < PEHeader64->FileHeader.NumberOfSections; i++){
							if(fileSectionData[i][0] > fileSectionData[j][0]){
								fileSectionTemp = fileSectionData[j][0];
								fileSectionData[j][0] = fileSectionData[i][0];
								fileSectionData[i][0] = fileSectionTemp;
								fileSectionTemp = fileSectionData[j][1];
								fileSectionData[j][1] = fileSectionData[i][1];
								fileSectionData[i][1] = fileSectionTemp;
							}
						}
					}
					for(i = 0; i < PEHeader64->FileHeader.NumberOfSections; i++){
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 - FileMapVA + (ULONG_PTR)sortedFileName + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + fileSectionData[i][2] * IMAGE_SIZEOF_SECTION_HEADER);
						RtlMoveMemory((LPVOID)((ULONG_PTR)sortedFileName + fileSectionData[i][0]), (LPVOID)((ULONG_PTR)FileMapVA + PESections->PointerToRawData), fileSectionData[i][1]);
						PESections->PointerToRawData = (DWORD)fileSectionData[i][0];
						PESections->SizeOfRawData = (DWORD)fileSectionData[i][1];
					}
					RtlMoveMemory((LPVOID)FileMapVA, sortedFileName, FileSize);
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					VirtualFree(sortedFileName, NULL, MEM_RELEASE);
					if(szBackupItem[0] != NULL){
						if(CopyFileA(szBackupFile, szFileName, false)){
							RemoveGarbageItem(szBackupItem, true);
							return(true);
						}else{
							RemoveGarbageItem(szBackupItem, true);
							return(false);
						}
					}else{
						return(true);
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					VirtualFree(sortedFileName, NULL, MEM_RELEASE);
					RemoveGarbageItem(szBackupItem, true);
					return(false);
				}
			}
		}else{
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			RemoveGarbageItem(szBackupItem, true);
			return(false);
		}
	}
	RemoveGarbageItem(szBackupItem, true);
	return(false);
}
__declspec(dllexport) bool __stdcall FindOverlay(char* szFileName, LPDWORD OverlayStart, LPDWORD OverlaySize){	

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD SectionNumber = 0;
	DWORD SectionRawOffset = 0;
	DWORD SectionRawSize = 0;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(false);
			}
			if(!FileIs64){
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				SectionNumber = PEHeader32->FileHeader.NumberOfSections;
				__try{
					while(SectionNumber > 0){
						if(PESections->PointerToRawData >= SectionRawOffset){
							if(PESections->SizeOfRawData != NULL || (SectionRawOffset != PESections->PointerToRawData)){
								SectionRawSize = PESections->SizeOfRawData;
							}
							SectionRawOffset = PESections->PointerToRawData;
						}
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
					}
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					if(SectionRawOffset + SectionRawSize < FileSize){
						if(OverlayStart != NULL && OverlaySize != NULL){
							*OverlayStart = (DWORD)(SectionRawOffset + SectionRawSize);
							*OverlaySize = (DWORD)(FileSize - SectionRawOffset - SectionRawSize);
						}
						return(true);
					}else{
						return(false);
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);
				}
			}else{
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
				SectionNumber = PEHeader64->FileHeader.NumberOfSections;
				__try{
					while(SectionNumber > 0){
						if(PESections->PointerToRawData >= SectionRawOffset){
							if(PESections->SizeOfRawData != NULL || (SectionRawOffset != PESections->PointerToRawData)){
								SectionRawSize = PESections->SizeOfRawData;
							}
							SectionRawOffset = PESections->PointerToRawData;
						}
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
					}
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					if(SectionRawOffset + SectionRawSize < FileSize){
						if(OverlayStart != NULL && OverlaySize != NULL){
							*OverlayStart = (DWORD)(SectionRawOffset + SectionRawSize);
							*OverlaySize = (DWORD)(FileSize - SectionRawOffset - SectionRawSize);
						}
						return(true);
					}else{
						return(false);
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);
				}
			}
		}else{
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			return(false);		
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall ExtractOverlay(char* szFileName, char* szExtactedFileName){

	HANDLE hFile = 0;	
	HANDLE hFileWrite = 0;
	BOOL Return = false;
	DWORD OverlayStart = 0;
	DWORD OverlaySize = 0;
	DWORD ueNumberOfBytesRead = 0;
	LPVOID ueReadBuffer = VirtualAlloc(NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE);

	Return = FindOverlay(szFileName, &OverlayStart, &OverlaySize);
	if(Return){
		hFile = CreateFileA(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
		if(hFile != INVALID_HANDLE_VALUE){
			hFileWrite = CreateFileA(szExtactedFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 
			if(hFileWrite != INVALID_HANDLE_VALUE){
				SetFilePointer(hFile, OverlayStart, NULL, FILE_BEGIN);
				while(OverlaySize > 0){
					if(OverlaySize > 0x1000){
						RtlZeroMemory(ueReadBuffer, 0x2000);
						ReadFile(hFile, ueReadBuffer, 0x1000, &ueNumberOfBytesRead, NULL);
						WriteFile(hFileWrite, ueReadBuffer, 0x1000, &ueNumberOfBytesRead, NULL);
						OverlaySize = OverlaySize - 0x1000;
					}else{
						RtlZeroMemory(ueReadBuffer, 0x2000);
						ReadFile(hFile, ueReadBuffer, OverlaySize, &ueNumberOfBytesRead, NULL);					
						WriteFile(hFileWrite, ueReadBuffer, OverlaySize, &ueNumberOfBytesRead, NULL);
						OverlaySize = 0;
					}
				}
				VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
				EngineCloseHandle(hFile);
				EngineCloseHandle(hFileWrite);
				return(true);
			}else{
				VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
				EngineCloseHandle(hFile);
				return(false);			
			}
		}
	}
	VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
	return(false);
}
__declspec(dllexport) bool __stdcall AddOverlay(char* szFileName, char* szOverlayFileName){

	HANDLE hFile = 0;	
	HANDLE hFileRead = 0;
	DWORD FileSize = 0;
	DWORD OverlaySize = 0;
	ULONG_PTR ueNumberOfBytesRead = 0;
	DWORD uedNumberOfBytesRead = 0;
	LPVOID ueReadBuffer = VirtualAlloc(NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE);

	hFile = CreateFileA(szFileName, GENERIC_READ+GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
	if(hFile != INVALID_HANDLE_VALUE){
		hFileRead = CreateFileA(szOverlayFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); 
		if(hFileRead != INVALID_HANDLE_VALUE){
			FileSize = GetFileSize(hFile, NULL);
			OverlaySize = GetFileSize(hFileRead, NULL); 
			SetFilePointer(hFile, FileSize, NULL, FILE_BEGIN);
			while(OverlaySize > 0){
				if(OverlaySize > 0x1000){
					RtlZeroMemory(ueReadBuffer, 0x2000);
					ReadFile(hFileRead, ueReadBuffer, 0x1000, &uedNumberOfBytesRead, NULL);
					WriteFile(hFile, ueReadBuffer, 0x1000, &uedNumberOfBytesRead, NULL);
					OverlaySize = OverlaySize - 0x1000;
				}else{
					RtlZeroMemory(ueReadBuffer, 0x2000);
					ReadFile(hFileRead, ueReadBuffer, OverlaySize, &uedNumberOfBytesRead, NULL);					
					WriteFile(hFile, ueReadBuffer, OverlaySize, &uedNumberOfBytesRead, NULL);
					OverlaySize = 0;
				}
			}
			EngineCloseHandle(hFile);
			EngineCloseHandle(hFileRead);
			return(true);
		}else{
			EngineCloseHandle(hFile);
			return(false);			
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall CopyOverlay(char* szInFileName, char* szOutFileName){

	char szTempName[MAX_PATH]; 
	char szTempFolder[MAX_PATH]; 

	RtlZeroMemory(&szTempName, MAX_PATH);
	RtlZeroMemory(&szTempFolder, MAX_PATH);
	if(GetTempPathA(MAX_PATH, szTempFolder) < MAX_PATH){
		if(GetTempFileNameA(szTempFolder, "OverlayTemp", NULL, szTempName)){
			if(ExtractOverlay(szInFileName, szTempName)){
				AddOverlay(szOutFileName, szTempName);
				DeleteFileA(szTempName);
				return(true);
			}
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall RemoveOverlay(char* szFileName){

	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	DWORD OverlayStart = 0;
	DWORD OverlaySize = 0;

	if(FindOverlay(szFileName, &OverlayStart, &OverlaySize)){	
		if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
			FileSize = FileSize - OverlaySize;
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			return(true);
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall MakeAllSectionsRWE(char* szFileName){

	char szBackupFile[512] = {};
	char szBackupItem[512] = {};
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD SectionNumber = 0;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(engineBackupForCriticalFunctions && CreateGarbageItem(&szBackupItem, 512)){
		if(!FillGarbageItem(szBackupItem, szFileName, &szBackupFile, 512)){
			RtlZeroMemory(&szBackupItem, 512);
			lstrcpyA(szBackupFile, szFileName);
		}
	}else{
		RtlZeroMemory(&szBackupItem, 512);
		lstrcpyA(szBackupFile, szFileName);
	}
	if(MapFileEx(szBackupFile, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);	
				RemoveGarbageItem(szBackupItem, true);
				return(false);
			}
			if(!FileIs64){
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				SectionNumber = PEHeader32->FileHeader.NumberOfSections;
				__try{
					while(SectionNumber > 0){
						PESections->Characteristics = 0xE0000020;
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
					}
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					if(szBackupItem[0] != NULL){
						if(CopyFileA(szBackupFile, szFileName, false)){
							RemoveGarbageItem(szBackupItem, true);
							return(true);
						}else{
							RemoveGarbageItem(szBackupItem, true);
							return(false);
						}
					}else{
						return(true);
					}
					return(true);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					RemoveGarbageItem(szBackupItem, true);
					return(false);
				}
			}else{
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
				SectionNumber = PEHeader64->FileHeader.NumberOfSections;
				__try{
					while(SectionNumber > 0){
						PESections->Characteristics = 0xE0000020;
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
					}
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					if(szBackupItem[0] != NULL){
						if(CopyFileA(szBackupFile, szFileName, false)){
							RemoveGarbageItem(szBackupItem, true);
							return(true);
						}else{
							RemoveGarbageItem(szBackupItem, true);
							return(false);
						}
					}else{
						return(true);
					}
					return(true);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					RemoveGarbageItem(szBackupItem, true);
					return(false);
				}
			}
		}else{
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			RemoveGarbageItem(szBackupItem, true);
			return(false);		
		}
	}
	RemoveGarbageItem(szBackupItem, true);
	return(false);
}

__declspec(dllexport) long __stdcall AddNewSectionEx(char* szFileName, char* szSectionName, DWORD SectionSize, DWORD SectionAttributes, LPVOID SectionContent, DWORD ContentSize){

	char szBackupFile[512] = {};
	char szBackupItem[512] = {};
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD SectionNameLength = 0;
	DWORD NewSectionVirtualOffset = 0;
	DWORD FileResizeValue = 0;
	DWORD LastSectionRawSize = 0;	
	DWORD alignedSectionSize = 0;
	DWORD NtSizeOfImage = 0;
	DWORD SectionNumber = 0;
	DWORD SpaceLeft = 0;
	LPVOID NameOffset;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	DWORD OldFileSize = 0;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(ContentSize < SectionSize && ContentSize != 0){
		ContentSize = SectionSize;
	}else if(ContentSize > SectionSize){
		SectionSize = ContentSize;	
	}

	if(engineBackupForCriticalFunctions && CreateGarbageItem(&szBackupItem, 512)){
		if(!FillGarbageItem(szBackupItem, szFileName, &szBackupFile, 512)){
			RtlZeroMemory(&szBackupItem, 512);
			lstrcpyA(szBackupFile, szFileName);
		}
	}else{
		RtlZeroMemory(&szBackupItem, 512);
		lstrcpyA(szBackupFile, szFileName);
	}
	if(MapFileEx(szBackupFile, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		OldFileSize = FileSize;
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);	
				RemoveGarbageItem(szBackupItem, true);
				return(0);
			}
			if(!FileIs64){
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				SectionNumber = PEHeader32->FileHeader.NumberOfSections;
				__try{
					SpaceLeft = PESections->PointerToRawData - (SectionNumber * IMAGE_SIZEOF_SECTION_HEADER) - DOSHeader->e_lfanew - sizeof IMAGE_NT_HEADERS32;
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + (SectionNumber - 1) * IMAGE_SIZEOF_SECTION_HEADER);
					LastSectionRawSize = (PESections->SizeOfRawData / PEHeader32->OptionalHeader.FileAlignment) * PEHeader32->OptionalHeader.FileAlignment;
					if(LastSectionRawSize < PESections->SizeOfRawData){
						LastSectionRawSize = LastSectionRawSize + PEHeader32->OptionalHeader.FileAlignment;
					}
					LastSectionRawSize = LastSectionRawSize - PESections->SizeOfRawData;
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);	
					FileResizeValue = LastSectionRawSize + (DWORD)SectionSize;
				}__except(EXCEPTION_EXECUTE_HANDLER){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					RemoveGarbageItem(szBackupItem, true);
					return(0);
				}
			}else{
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
				SectionNumber = PEHeader64->FileHeader.NumberOfSections;
				__try{
					SpaceLeft = PESections->PointerToRawData - (SectionNumber * IMAGE_SIZEOF_SECTION_HEADER) - DOSHeader->e_lfanew - sizeof IMAGE_NT_HEADERS64;
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + (SectionNumber - 1) * IMAGE_SIZEOF_SECTION_HEADER);
					LastSectionRawSize = (PESections->SizeOfRawData / PEHeader64->OptionalHeader.FileAlignment) * PEHeader64->OptionalHeader.FileAlignment;
					if(LastSectionRawSize < PESections->SizeOfRawData){
						LastSectionRawSize = LastSectionRawSize + PEHeader64->OptionalHeader.FileAlignment;
					}
					LastSectionRawSize = LastSectionRawSize - PESections->SizeOfRawData;
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);	
					FileResizeValue = LastSectionRawSize + (DWORD)SectionSize;
				}__except(EXCEPTION_EXECUTE_HANDLER){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					RemoveGarbageItem(szBackupItem, true);
					return(0);
				}
			}
		}else{
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			RemoveGarbageItem(szBackupItem, true);
			return(0);
		}
	}
	if(SpaceLeft > IMAGE_SIZEOF_SECTION_HEADER){
		if(MapFileEx(szBackupFile, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, FileResizeValue)){
			DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
			if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
				PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
				PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
				if(PEHeader32->OptionalHeader.Magic == 0x10B){
					FileIs64 = false;
				}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
					FileIs64 = true;
				}else{
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					RemoveGarbageItem(szBackupItem, true);
					return(0);
				}
				if(!FileIs64){
					__try{
						if(SectionSize == 0){
							SectionSize = PEHeader32->OptionalHeader.FileAlignment;
						}
						alignedSectionSize = ((DWORD)SectionSize / PEHeader32->OptionalHeader.SectionAlignment) * PEHeader32->OptionalHeader.SectionAlignment;
						if(alignedSectionSize < SectionSize){
							alignedSectionSize = alignedSectionSize + PEHeader32->OptionalHeader.SectionAlignment;
						}
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
						SectionNumber = PEHeader32->FileHeader.NumberOfSections;
						PEHeader32->FileHeader.NumberOfSections = PEHeader32->FileHeader.NumberOfSections + 1;
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + (SectionNumber - 1)* IMAGE_SIZEOF_SECTION_HEADER);
						NewSectionVirtualOffset = PESections->VirtualAddress + (PESections->Misc.VirtualSize / PEHeader32->OptionalHeader.SectionAlignment) * PEHeader32->OptionalHeader.SectionAlignment;
						if(NewSectionVirtualOffset < PESections->VirtualAddress + PESections->Misc.VirtualSize){
							NewSectionVirtualOffset = NewSectionVirtualOffset + PEHeader32->OptionalHeader.SectionAlignment;
						}
						PESections->SizeOfRawData = PESections->SizeOfRawData + LastSectionRawSize;
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						PEHeader32->OptionalHeader.SizeOfImage = NewSectionVirtualOffset + alignedSectionSize;				
						NameOffset = &PESections->Name;
						if(lstrlenA(szSectionName) >= 8){
							SectionNameLength = 8;
						}else{
							SectionNameLength = lstrlenA(szSectionName);
						}
						RtlMoveMemory(NameOffset, szSectionName, SectionNameLength);
						if(SectionAttributes == 0){
							PESections->Characteristics = 0xE0000020;
						}else{
							PESections->Characteristics = (DWORD)(SectionAttributes);
						}
						PESections->Misc.VirtualSize = alignedSectionSize;
						PESections->SizeOfRawData = (DWORD)(SectionSize);
						PESections->VirtualAddress = NewSectionVirtualOffset;
						PESections->PointerToRawData = OldFileSize + LastSectionRawSize;
						if(SectionContent != NULL){
							RtlMoveMemory((LPVOID)(FileMapVA + OldFileSize), SectionContent, ContentSize);
						}
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						if(szBackupItem[0] != NULL){
							if(CopyFileA(szBackupFile, szFileName, false)){
								RemoveGarbageItem(szBackupItem, true);
								return(NewSectionVirtualOffset);
							}else{
								RemoveGarbageItem(szBackupItem, true);
								return(0);
							}
						}else{
							return(NewSectionVirtualOffset);
						}
					}__except(EXCEPTION_EXECUTE_HANDLER){
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						RemoveGarbageItem(szBackupItem, true);
						return(0);
					}
				}else{
					__try{
						if(SectionSize == 0){
							SectionSize = PEHeader64->OptionalHeader.FileAlignment;
						}
						alignedSectionSize = ((DWORD)SectionSize / PEHeader64->OptionalHeader.SectionAlignment) * PEHeader64->OptionalHeader.SectionAlignment;
						if(alignedSectionSize < SectionSize){
							alignedSectionSize = alignedSectionSize + PEHeader64->OptionalHeader.SectionAlignment;
						}
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
						SectionNumber = PEHeader64->FileHeader.NumberOfSections;
						PEHeader32->FileHeader.NumberOfSections = PEHeader32->FileHeader.NumberOfSections + 1;
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + (SectionNumber - 1)* IMAGE_SIZEOF_SECTION_HEADER);
						NewSectionVirtualOffset = PESections->VirtualAddress + (PESections->Misc.VirtualSize / PEHeader64->OptionalHeader.SectionAlignment) * PEHeader64->OptionalHeader.SectionAlignment;
						if(NewSectionVirtualOffset < PESections->VirtualAddress + PESections->Misc.VirtualSize){
							NewSectionVirtualOffset = NewSectionVirtualOffset + PEHeader64->OptionalHeader.SectionAlignment;
						}
						PESections->SizeOfRawData = PESections->SizeOfRawData + LastSectionRawSize;
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						PEHeader64->OptionalHeader.SizeOfImage = NewSectionVirtualOffset + alignedSectionSize;
						NameOffset = &PESections->Name;
						if(lstrlenA(szSectionName) >= 8){
							SectionNameLength = 8;
						}else{
							SectionNameLength = lstrlenA(szSectionName);
						}
						RtlMoveMemory(NameOffset, szSectionName, SectionNameLength);
						if(SectionAttributes == 0){
							PESections->Characteristics = 0xE0000020;
						}else{
							PESections->Characteristics = (DWORD)(SectionAttributes);				
						}
						PESections->Misc.VirtualSize = alignedSectionSize;
						PESections->SizeOfRawData = (DWORD)(SectionSize);
						PESections->VirtualAddress = NewSectionVirtualOffset;
						PESections->PointerToRawData = OldFileSize + LastSectionRawSize;
						if(SectionContent != NULL){
							RtlMoveMemory((LPVOID)(FileMapVA + OldFileSize), SectionContent, ContentSize);
						}
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						if(szBackupItem[0] != NULL){
							if(CopyFileA(szBackupFile, szFileName, false)){
								RemoveGarbageItem(szBackupItem, true);
								return(NewSectionVirtualOffset);
							}else{
								RemoveGarbageItem(szBackupItem, true);
								return(0);
							}
						}else{
							return(NewSectionVirtualOffset);
						}
					}__except(EXCEPTION_EXECUTE_HANDLER){
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						RemoveGarbageItem(szBackupItem, true);
						return(0);
					}
				}
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				RemoveGarbageItem(szBackupItem, true);
				return(0);		
			}
		}
	}
	RemoveGarbageItem(szBackupItem, true);
	return(0);
}
__declspec(dllexport) long __stdcall AddNewSection(char* szFileName, char* szSectionName, DWORD SectionSize){
	return(AddNewSectionEx(szFileName, szSectionName, SectionSize, NULL, NULL, NULL));
}
__declspec(dllexport) bool __stdcall ResizeLastSection(char* szFileName, DWORD NumberOfExpandBytes, bool AlignResizeData){

	char szBackupFile[512] = {};
	char szBackupItem[512] = {};
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD SectionNumber = 0;
	DWORD SectionRawSize = 0;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(engineBackupForCriticalFunctions && CreateGarbageItem(&szBackupItem, 512)){
		if(!FillGarbageItem(szBackupItem, szFileName, &szBackupFile, 512)){
			RtlZeroMemory(&szBackupItem, 512);
			lstrcpyA(szBackupFile, szFileName);
		}
	}else{
		RtlZeroMemory(&szBackupItem, 512);
		lstrcpyA(szBackupFile, szFileName);
	}
	if(MapFileEx(szBackupFile, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NumberOfExpandBytes)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				FileSize = FileSize - NumberOfExpandBytes;
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				RemoveGarbageItem(szBackupItem, true);
				return(false);
			}
			if(!FileIs64){
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				SectionNumber = PEHeader32->FileHeader.NumberOfSections;
				SectionNumber--;
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + SectionNumber * IMAGE_SIZEOF_SECTION_HEADER);
				__try{
					if(AlignResizeData){
						SectionRawSize = PESections->SizeOfRawData;
						if((PESections->SizeOfRawData + NumberOfExpandBytes) % PEHeader32->OptionalHeader.FileAlignment == NULL){
							PESections->SizeOfRawData = (((PESections->SizeOfRawData + NumberOfExpandBytes) / PEHeader32->OptionalHeader.FileAlignment)) * PEHeader32->OptionalHeader.FileAlignment;
						}else{
							PESections->SizeOfRawData = (((PESections->SizeOfRawData + NumberOfExpandBytes) / PEHeader32->OptionalHeader.FileAlignment) + 1) * PEHeader32->OptionalHeader.FileAlignment;
						}
						if(SectionRawSize < NULL){
							SectionRawSize = NULL;
						}
						SectionRawSize = PESections->SizeOfRawData - SectionRawSize - NumberOfExpandBytes;
						PEHeader32->OptionalHeader.SizeOfImage = PEHeader32->OptionalHeader.SizeOfImage - PESections->Misc.VirtualSize;
						if((PESections->Misc.VirtualSize + NumberOfExpandBytes + SectionRawSize) % PEHeader32->OptionalHeader.SectionAlignment == NULL){
							PESections->Misc.VirtualSize = (((PESections->Misc.VirtualSize + NumberOfExpandBytes + SectionRawSize) / PEHeader32->OptionalHeader.SectionAlignment)) * PEHeader32->OptionalHeader.SectionAlignment;
						}else{
							PESections->Misc.VirtualSize = (((PESections->Misc.VirtualSize + NumberOfExpandBytes + SectionRawSize) / PEHeader32->OptionalHeader.SectionAlignment) + 1) * PEHeader32->OptionalHeader.SectionAlignment;				
						}
						PEHeader32->OptionalHeader.SizeOfImage = PEHeader32->OptionalHeader.SizeOfImage + PESections->Misc.VirtualSize;
						if(SectionRawSize > NULL){
							UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
							MapFileEx(szFileName, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, SectionRawSize);
						}
					}else{
						PESections->SizeOfRawData = PESections->SizeOfRawData + NumberOfExpandBytes;
					}
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					if(szBackupItem[0] != NULL){
						if(CopyFileA(szBackupFile, szFileName, false)){
							RemoveGarbageItem(szBackupItem, true);
							return(true);
						}else{
							RemoveGarbageItem(szBackupItem, true);
							return(false);
						}
					}else{
						return(true);
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					RemoveGarbageItem(szBackupItem, true);
					return(false);
				}
			}else{
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
				SectionNumber = PEHeader64->FileHeader.NumberOfSections;
				SectionNumber--;
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + SectionNumber * IMAGE_SIZEOF_SECTION_HEADER);
				__try{
					if(AlignResizeData){
						SectionRawSize = PESections->SizeOfRawData;
						if((PESections->SizeOfRawData + NumberOfExpandBytes) % PEHeader64->OptionalHeader.FileAlignment == NULL){
							PESections->SizeOfRawData = (((PESections->SizeOfRawData + NumberOfExpandBytes) / PEHeader64->OptionalHeader.FileAlignment)) * PEHeader64->OptionalHeader.FileAlignment;
						}else{
							PESections->SizeOfRawData = (((PESections->SizeOfRawData + NumberOfExpandBytes) / PEHeader64->OptionalHeader.FileAlignment) + 1) * PEHeader64->OptionalHeader.FileAlignment;
						}
						if(SectionRawSize < NULL){
							SectionRawSize = NULL;
						}
						SectionRawSize = PESections->SizeOfRawData - SectionRawSize - NumberOfExpandBytes;
						PEHeader64->OptionalHeader.SizeOfImage = PEHeader64->OptionalHeader.SizeOfImage - PESections->Misc.VirtualSize;
						if((PESections->Misc.VirtualSize + NumberOfExpandBytes) % PEHeader64->OptionalHeader.SectionAlignment == NULL){
							PESections->Misc.VirtualSize = (((PESections->Misc.VirtualSize + NumberOfExpandBytes + SectionRawSize) / PEHeader32->OptionalHeader.SectionAlignment)) * PEHeader64->OptionalHeader.SectionAlignment;
						}else{
							PESections->Misc.VirtualSize = (((PESections->Misc.VirtualSize + NumberOfExpandBytes + SectionRawSize) / PEHeader32->OptionalHeader.SectionAlignment) + 1) * PEHeader64->OptionalHeader.SectionAlignment;				
						}
						PEHeader64->OptionalHeader.SizeOfImage = PEHeader64->OptionalHeader.SizeOfImage + PESections->Misc.VirtualSize;
						if(SectionRawSize > NULL){
							UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
							MapFileEx(szFileName, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, SectionRawSize);
						}
					}else{
						PESections->SizeOfRawData = PESections->SizeOfRawData + NumberOfExpandBytes;
					}
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					if(szBackupItem[0] != NULL){
						if(CopyFileA(szBackupFile, szFileName, false)){
							RemoveGarbageItem(szBackupItem, true);
							return(true);
						}else{
							RemoveGarbageItem(szBackupItem, true);
							return(false);
						}
					}else{
						return(true);
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					RemoveGarbageItem(szBackupItem, true);
					return(false);
				}
			}
		}else{
			FileSize = FileSize - NumberOfExpandBytes;
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			RemoveGarbageItem(szBackupItem, true);
			return(false);		
		}
	}
	RemoveGarbageItem(szBackupItem, true);
	return(false);
}
__declspec(dllexport) void __stdcall SetSharedOverlay(char* szFileName){
	szSharedOverlay = szFileName;
}
__declspec(dllexport) char* __stdcall GetSharedOverlay(){
	return(szSharedOverlay);
}
__declspec(dllexport) bool __stdcall DeleteLastSection(char* szFileName){

	char szBackupFile[512] = {};
	char szBackupItem[512] = {};
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD SectionNumber = 0;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(engineBackupForCriticalFunctions && CreateGarbageItem(&szBackupItem, 512)){
		if(!FillGarbageItem(szBackupItem, szFileName, &szBackupFile, 512)){
			RtlZeroMemory(&szBackupItem, 512);
			lstrcpyA(szBackupFile, szFileName);
		}
	}else{
		RtlZeroMemory(&szBackupItem, 512);
		lstrcpyA(szBackupFile, szFileName);
	}
	if(MapFileEx(szBackupFile, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);	
				RemoveGarbageItem(szBackupItem, true);
				return(false);
			}
			if(!FileIs64){
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				SectionNumber = PEHeader32->FileHeader.NumberOfSections;
				__try{
					if(SectionNumber > 1){
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + (SectionNumber - 1) * IMAGE_SIZEOF_SECTION_HEADER);
						PEHeader32->OptionalHeader.SizeOfImage = PEHeader32->OptionalHeader.SizeOfImage - PESections->Misc.VirtualSize;
						FileSize = PESections->PointerToRawData;
						RtlZeroMemory(PESections, IMAGE_SIZEOF_SECTION_HEADER);
						PEHeader32->FileHeader.NumberOfSections--;
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						if(szBackupItem[0] != NULL){
							if(CopyFileA(szBackupFile, szFileName, false)){
								RemoveGarbageItem(szBackupItem, true);
								return(true);
							}else{
								RemoveGarbageItem(szBackupItem, true);
								return(false);
							}
						}else{
							return(true);
						}
					}else{
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						RemoveGarbageItem(szBackupItem, true);
						return(false);
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					RemoveGarbageItem(szBackupItem, true);
					return(false);
				}
			}else{
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
				SectionNumber = PEHeader64->FileHeader.NumberOfSections;
				__try{
					if(SectionNumber > 1){
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + (SectionNumber - 1) * IMAGE_SIZEOF_SECTION_HEADER);
						PEHeader32->OptionalHeader.SizeOfImage = PEHeader32->OptionalHeader.SizeOfImage - PESections->Misc.VirtualSize;
						FileSize = PESections->PointerToRawData;
						RtlZeroMemory(PESections, IMAGE_SIZEOF_SECTION_HEADER);
						PEHeader64->FileHeader.NumberOfSections--;
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						if(szBackupItem[0] != NULL){
							if(CopyFileA(szBackupFile, szFileName, false)){
								RemoveGarbageItem(szBackupItem, true);
								return(true);
							}else{
								RemoveGarbageItem(szBackupItem, true);
								return(false);
							}
						}else{
							return(true);
						}
					}else{
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						RemoveGarbageItem(szBackupItem, true);
						return(false);
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					RemoveGarbageItem(szBackupItem, true);
					return(false);
				}
			}
		}else{
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			RemoveGarbageItem(szBackupItem, true);
			return(false);		
		}
	}
	RemoveGarbageItem(szBackupItem, true);
	return(false);
}
__declspec(dllexport) bool __stdcall DeleteLastSectionEx(char* szFileName, DWORD NumberOfSections){
	while(NumberOfSections > 0){
		DeleteLastSection(szFileName);
		NumberOfSections--;
	}
	return(true);
}
__declspec(dllexport) long long __stdcall GetPE32DataFromMappedFile(ULONG_PTR FileMapVA, DWORD WhichSection, DWORD WhichData){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD SectionNumber = 0;
	BOOL FileIs64;

	if(FileMapVA != NULL){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, NULL, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				return(0);
			}
			if(!FileIs64){
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				SectionNumber = PEHeader32->FileHeader.NumberOfSections;
				if(WhichData < UE_SECTIONNAME){
					if(WhichData == UE_PE_OFFSET){
						return(DOSHeader->e_lfanew);					
					}else if(WhichData == UE_IMAGEBASE){
						return(PEHeader32->OptionalHeader.ImageBase);
					}else if(WhichData == UE_OEP){
						return(PEHeader32->OptionalHeader.AddressOfEntryPoint);
					}else if(WhichData == UE_SIZEOFIMAGE){
						return(PEHeader32->OptionalHeader.SizeOfImage);
					}else if(WhichData == UE_SIZEOFHEADERS){
						return(PEHeader32->OptionalHeader.SizeOfHeaders);
					}else if(WhichData == UE_SIZEOFOPTIONALHEADER){
						return(PEHeader32->FileHeader.SizeOfOptionalHeader);
					}else if(WhichData == UE_SECTIONALIGNMENT){
						return(PEHeader32->OptionalHeader.SectionAlignment);
					}else if(WhichData == UE_IMPORTTABLEADDRESS){
						return(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
					}else if(WhichData == UE_IMPORTTABLESIZE){
						return(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size);
					}else if(WhichData == UE_RESOURCETABLEADDRESS){
						return(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress);
					}else if(WhichData == UE_RESOURCETABLESIZE){
						return(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size);
					}else if(WhichData == UE_EXPORTTABLEADDRESS){
						return(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
					}else if(WhichData == UE_EXPORTTABLESIZE){
						return(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size);
					}else if(WhichData == UE_TLSTABLEADDRESS){
						return(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
					}else if(WhichData == UE_TLSTABLESIZE){
						return(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size);
					}else if(WhichData == UE_RELOCATIONTABLEADDRESS){
						return(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
					}else if(WhichData == UE_RELOCATIONTABLESIZE){
						return(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);
					}else if(WhichData == UE_TIMEDATESTAMP){
						return(PEHeader32->FileHeader.TimeDateStamp);
					}else if(WhichData == UE_SECTIONNUMBER){
						return(PEHeader32->FileHeader.NumberOfSections);
					}else if(WhichData == UE_CHECKSUM){
						return(PEHeader32->OptionalHeader.CheckSum);
					}else if(WhichData == UE_SUBSYSTEM){
						return(PEHeader32->OptionalHeader.Subsystem);
					}else if(WhichData == UE_CHARACTERISTICS){
						return(PEHeader32->FileHeader.Characteristics);
					}else if(WhichData == UE_NUMBEROFRVAANDSIZES){
						return(PEHeader32->OptionalHeader.NumberOfRvaAndSizes);
					}else{
						return(0);
					}
				}else{
					if(SectionNumber >= WhichSection){
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + WhichSection * IMAGE_SIZEOF_SECTION_HEADER);
						if(WhichData == UE_SECTIONNAME){
							return((ULONG_PTR)PESections->Name);
						}else if(WhichData == UE_SECTIONVIRTUALOFFSET){
							return(PESections->VirtualAddress);
						}else if(WhichData == UE_SECTIONVIRTUALSIZE){
							return(PESections->Misc.VirtualSize);
						}else if(WhichData == UE_SECTIONRAWOFFSET){
							return(PESections->PointerToRawData);
						}else if(WhichData == UE_SECTIONRAWSIZE){
							return(PESections->SizeOfRawData);
						}else if(WhichData == UE_SECTIONFLAGS){
							return(PESections->Characteristics);
						}else{
							return(0);
						}
					}
				}
				return(0);
			}else{
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
				SectionNumber = PEHeader64->FileHeader.NumberOfSections;
				if(WhichData < UE_SECTIONNAME){
					if(WhichData == UE_PE_OFFSET){
						return(DOSHeader->e_lfanew);					
					}else if(WhichData == UE_IMAGEBASE){
						return(PEHeader64->OptionalHeader.ImageBase);
					}else if(WhichData == UE_OEP){
						return(PEHeader64->OptionalHeader.AddressOfEntryPoint);
					}else if(WhichData == UE_SIZEOFIMAGE){
						return(PEHeader64->OptionalHeader.SizeOfImage);
					}else if(WhichData == UE_SIZEOFHEADERS){
						return(PEHeader64->OptionalHeader.SizeOfHeaders);
					}else if(WhichData == UE_SIZEOFOPTIONALHEADER){
						return(PEHeader64->FileHeader.SizeOfOptionalHeader);
					}else if(WhichData == UE_SECTIONALIGNMENT){
						return(PEHeader64->OptionalHeader.SectionAlignment);
					}else if(WhichData == UE_IMPORTTABLEADDRESS){
						return(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
					}else if(WhichData == UE_IMPORTTABLESIZE){
						return(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size);
					}else if(WhichData == UE_RESOURCETABLEADDRESS){
						return(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress);
					}else if(WhichData == UE_RESOURCETABLESIZE){
						return(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size);
					}else if(WhichData == UE_EXPORTTABLEADDRESS){
						return(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
					}else if(WhichData == UE_EXPORTTABLESIZE){
						return(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size);
					}else if(WhichData == UE_TLSTABLEADDRESS){
						return(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
					}else if(WhichData == UE_TLSTABLESIZE){
						return(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size);
					}else if(WhichData == UE_RELOCATIONTABLEADDRESS){
						return(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
					}else if(WhichData == UE_RELOCATIONTABLESIZE){
						return(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);
					}else if(WhichData == UE_TIMEDATESTAMP){
						return(PEHeader64->FileHeader.TimeDateStamp);
					}else if(WhichData == UE_SECTIONNUMBER){
						return(PEHeader64->FileHeader.NumberOfSections);
					}else if(WhichData == UE_CHECKSUM){
						return(PEHeader64->OptionalHeader.CheckSum);
					}else if(WhichData == UE_SUBSYSTEM){
						return(PEHeader64->OptionalHeader.Subsystem);
					}else if(WhichData == UE_CHARACTERISTICS){
						return(PEHeader64->FileHeader.Characteristics);
					}else if(WhichData == UE_NUMBEROFRVAANDSIZES){
						return(PEHeader64->OptionalHeader.NumberOfRvaAndSizes);
					}else{
						return(0);
					}
				}else{
					if(SectionNumber >= WhichSection){
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + WhichSection * IMAGE_SIZEOF_SECTION_HEADER);
						if(WhichData == UE_SECTIONNAME){
							return((ULONG_PTR)PESections->Name);
						}else if(WhichData == UE_SECTIONVIRTUALOFFSET){
							return(PESections->VirtualAddress);
						}else if(WhichData == UE_SECTIONVIRTUALSIZE){
							return(PESections->Misc.VirtualSize);
						}else if(WhichData == UE_SECTIONRAWOFFSET){
							return(PESections->PointerToRawData);
						}else if(WhichData == UE_SECTIONRAWSIZE){
							return(PESections->SizeOfRawData);
						}else if(WhichData == UE_SECTIONFLAGS){
							return(PESections->Characteristics);
						}else{
							return(0);
						}
					}
				}
				return(0);
			}
		}else{
			return(0);		
		}
	}
	return(0);
}
__declspec(dllexport) long long __stdcall GetPE32Data(char* szFileName, DWORD WhichSection, DWORD WhichData){

	long long ReturnValue = 0;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		ReturnValue = GetPE32DataFromMappedFile(FileMapVA, WhichSection, WhichData);
		UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
		return(ReturnValue);
	}else{
		return(0);
	}
}
__declspec(dllexport) bool __stdcall GetPE32DataFromMappedFileEx(ULONG_PTR FileMapVA, LPVOID DataStorage){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	BOOL FileIs64;
	PPE32Struct PE32Structure = (PPE32Struct)DataStorage;
	PPE64Struct PE64Structure = (PPE64Struct)DataStorage;
	
	if(FileMapVA != NULL){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, NULL, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				return(false);
			}
			if(!FileIs64){
				PE32Structure->PE32Offset = DOSHeader->e_lfanew;
				PE32Structure->ImageBase = PEHeader32->OptionalHeader.ImageBase;
				PE32Structure->OriginalEntryPoint = PEHeader32->OptionalHeader.AddressOfEntryPoint;
				PE32Structure->NtSizeOfImage = PEHeader32->OptionalHeader.SizeOfImage;
				PE32Structure->NtSizeOfHeaders = PEHeader32->OptionalHeader.SizeOfHeaders;
				PE32Structure->SizeOfOptionalHeaders = PEHeader32->FileHeader.SizeOfOptionalHeader;
				PE32Structure->FileAlignment = PEHeader32->OptionalHeader.FileAlignment;
				PE32Structure->SectionAligment = PEHeader32->OptionalHeader.SectionAlignment;
				PE32Structure->ImportTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
				PE32Structure->ImportTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
				PE32Structure->ResourceTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
				PE32Structure->ResourceTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
				PE32Structure->ExportTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
				PE32Structure->ExportTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
				PE32Structure->TLSTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
				PE32Structure->TLSTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size;
				PE32Structure->RelocationTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
				PE32Structure->RelocationTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
				PE32Structure->TimeDateStamp = PEHeader32->FileHeader.TimeDateStamp;
				PE32Structure->SectionNumber = PEHeader32->FileHeader.NumberOfSections;
				PE32Structure->CheckSum = PEHeader32->OptionalHeader.CheckSum;
				PE32Structure->SubSystem = PEHeader32->OptionalHeader.Subsystem;
				PE32Structure->Characteristics = PEHeader32->FileHeader.Characteristics;
				PE32Structure->NumberOfRvaAndSizes = PEHeader32->OptionalHeader.NumberOfRvaAndSizes;
				return(true);
			}else{
				PE64Structure->PE64Offset = DOSHeader->e_lfanew;
				PE64Structure->ImageBase = PEHeader64->OptionalHeader.ImageBase;
				PE64Structure->OriginalEntryPoint = PEHeader64->OptionalHeader.AddressOfEntryPoint;
				PE64Structure->NtSizeOfImage = PEHeader64->OptionalHeader.SizeOfImage;
				PE64Structure->NtSizeOfHeaders = PEHeader64->OptionalHeader.SizeOfHeaders;
				PE64Structure->SizeOfOptionalHeaders = PEHeader64->FileHeader.SizeOfOptionalHeader;
				PE64Structure->FileAlignment = PEHeader64->OptionalHeader.FileAlignment;
				PE64Structure->SectionAligment = PEHeader64->OptionalHeader.SectionAlignment;
				PE64Structure->ImportTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
				PE64Structure->ImportTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
				PE64Structure->ResourceTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
				PE64Structure->ResourceTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
				PE64Structure->ExportTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
				PE64Structure->ExportTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
				PE64Structure->TLSTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
				PE64Structure->TLSTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size;
				PE64Structure->RelocationTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
				PE64Structure->RelocationTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
				PE64Structure->TimeDateStamp = PEHeader64->FileHeader.TimeDateStamp;
				PE64Structure->SectionNumber = PEHeader64->FileHeader.NumberOfSections;
				PE64Structure->CheckSum = PEHeader64->OptionalHeader.CheckSum;	
				PE64Structure->SubSystem = PEHeader32->OptionalHeader.Subsystem;
				PE64Structure->Characteristics = PEHeader32->FileHeader.Characteristics;
				PE64Structure->NumberOfRvaAndSizes = PEHeader32->OptionalHeader.NumberOfRvaAndSizes;
				return(true);
			}
		}else{
			return(false);		
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall GetPE32DataEx(char* szFileName, LPVOID DataStorage){

	long long ReturnValue = 0;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		ReturnValue = GetPE32DataFromMappedFileEx(FileMapVA, DataStorage);
		UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
		if(ReturnValue){
			return(true);
		}else{
			return(false);
		}
	}else{
		return(false);
	}
}
__declspec(dllexport) bool __stdcall SetPE32DataForMappedFile(ULONG_PTR FileMapVA, DWORD WhichSection, DWORD WhichData, ULONG_PTR NewDataValue){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD SectionNumber = 0;
	BOOL FileIs64;

	if(FileMapVA != NULL){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, NULL, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				return(false);
			}
			if(!FileIs64){
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				SectionNumber = PEHeader32->FileHeader.NumberOfSections;
				__try{
					if(WhichData < UE_SECTIONNAME){
						if(WhichData == UE_PE_OFFSET){
							DOSHeader->e_lfanew = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_IMAGEBASE){
							PEHeader32->OptionalHeader.ImageBase = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_OEP){
							PEHeader32->OptionalHeader.AddressOfEntryPoint = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_SIZEOFIMAGE){
							PEHeader32->OptionalHeader.SizeOfImage = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_SIZEOFHEADERS){
							PEHeader32->OptionalHeader.SizeOfHeaders = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_SIZEOFOPTIONALHEADER){
							PEHeader32->FileHeader.SizeOfOptionalHeader = (WORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_SECTIONALIGNMENT){
							PEHeader32->OptionalHeader.SectionAlignment = (WORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_IMPORTTABLEADDRESS){
							PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_IMPORTTABLESIZE){
							PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_RESOURCETABLEADDRESS){
							PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_RESOURCETABLESIZE){
							PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_EXPORTTABLEADDRESS){
							PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_EXPORTTABLESIZE){
							PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_TLSTABLEADDRESS){
							PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_TLSTABLESIZE){
							PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_RELOCATIONTABLEADDRESS){
							PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_RELOCATIONTABLESIZE){
							PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_TIMEDATESTAMP){
							PEHeader32->FileHeader.TimeDateStamp = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_SECTIONNUMBER){
							PEHeader32->FileHeader.NumberOfSections = (WORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_CHECKSUM){
							PEHeader32->OptionalHeader.CheckSum = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_SUBSYSTEM){
							PEHeader32->OptionalHeader.Subsystem = (WORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_CHARACTERISTICS){
							PEHeader32->FileHeader.Characteristics = (WORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_NUMBEROFRVAANDSIZES){
							PEHeader32->OptionalHeader.NumberOfRvaAndSizes = (DWORD)NewDataValue;
							return(true);
						}else{
							return(false);
						}
					}else{
						if(SectionNumber <= WhichSection){
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + WhichSection * IMAGE_SIZEOF_SECTION_HEADER);
							if(WhichData == UE_SECTIONNAME){
								return(false);
							}else if(WhichData == UE_SECTIONVIRTUALOFFSET){
								PESections->VirtualAddress = (DWORD)NewDataValue;
								return(true);
							}else if(WhichData == UE_SECTIONVIRTUALSIZE){
								PESections->Misc.VirtualSize = (DWORD)NewDataValue;
								return(true);
							}else if(WhichData == UE_SECTIONRAWOFFSET){
								PESections->PointerToRawData = (DWORD)NewDataValue;
								return(true);
							}else if(WhichData == UE_SECTIONRAWSIZE){
								PESections->SizeOfRawData = (DWORD)NewDataValue;
								return(true);
							}else if(WhichData == UE_SECTIONFLAGS){
								PESections->Characteristics = (DWORD)NewDataValue;
								return(true);
							}else{
								return(false);
							}
						}
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(false);
				}
				return(false);
			}else{
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
				SectionNumber = PEHeader64->FileHeader.NumberOfSections;
				__try{
					if(WhichData < UE_SECTIONNAME){
						if(WhichData == UE_PE_OFFSET){
							DOSHeader->e_lfanew = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_IMAGEBASE){
							PEHeader64->OptionalHeader.ImageBase = NewDataValue;
							return(true);
						}else if(WhichData == UE_OEP){
							PEHeader64->OptionalHeader.AddressOfEntryPoint = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_SIZEOFIMAGE){
							PEHeader64->OptionalHeader.SizeOfImage = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_SIZEOFHEADERS){
							PEHeader64->OptionalHeader.SizeOfHeaders = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_SIZEOFOPTIONALHEADER){
							PEHeader64->FileHeader.SizeOfOptionalHeader = (WORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_SECTIONALIGNMENT){
							PEHeader64->OptionalHeader.SectionAlignment = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_IMPORTTABLEADDRESS){
							PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_IMPORTTABLESIZE){
							PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_RESOURCETABLEADDRESS){
							PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_RESOURCETABLESIZE){
							PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_EXPORTTABLEADDRESS){
							PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_EXPORTTABLESIZE){
							PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_TLSTABLEADDRESS){
							PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_TLSTABLESIZE){
							PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_RELOCATIONTABLEADDRESS){
							PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_RELOCATIONTABLESIZE){
							PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_TIMEDATESTAMP){
							PEHeader64->FileHeader.TimeDateStamp = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_SECTIONNUMBER){
							PEHeader64->FileHeader.NumberOfSections = (WORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_CHECKSUM){
							PEHeader64->OptionalHeader.CheckSum = (DWORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_SUBSYSTEM){
							PEHeader64->OptionalHeader.Subsystem = (WORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_CHARACTERISTICS){
							PEHeader64->FileHeader.Characteristics = (WORD)NewDataValue;
							return(true);
						}else if(WhichData == UE_NUMBEROFRVAANDSIZES){
							PEHeader64->OptionalHeader.NumberOfRvaAndSizes = (DWORD)NewDataValue;
							return(true);
						}else{
							return(0);
						}
					}else{
						if(SectionNumber <= WhichSection){
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + WhichSection * IMAGE_SIZEOF_SECTION_HEADER);
							if(WhichData == UE_SECTIONNAME){
								return(false);
							}else if(WhichData == UE_SECTIONVIRTUALOFFSET){
								PESections->VirtualAddress = (DWORD)NewDataValue;
								return(true);
							}else if(WhichData == UE_SECTIONVIRTUALSIZE){
								PESections->Misc.VirtualSize = (DWORD)NewDataValue;
								return(true);
							}else if(WhichData == UE_SECTIONRAWOFFSET){
								PESections->PointerToRawData = (DWORD)NewDataValue;
								return(true);
							}else if(WhichData == UE_SECTIONRAWSIZE){
								PESections->SizeOfRawData = (DWORD)NewDataValue;
								return(true);
							}else if(WhichData == UE_SECTIONFLAGS){
								PESections->Characteristics = (DWORD)NewDataValue;
								return(true);
							}else{
								return(false);
							}
						}
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(false);
				}
				return(false);
			}
		}else{
			return(false);
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall SetPE32Data(char* szFileName, DWORD WhichSection, DWORD WhichData, ULONG_PTR NewDataValue){

	long long ReturnValue = 0;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		ReturnValue = SetPE32DataForMappedFile(FileMapVA, WhichSection, WhichData, NewDataValue);
		UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
		if(ReturnValue){
			return(true);
		}else{
			return(false);
		}
	}else{
		return(false);
	}
}
__declspec(dllexport) bool __stdcall SetPE32DataForMappedFileEx(ULONG_PTR FileMapVA, LPVOID DataStorage){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	BOOL FileIs64;
	PPE32Struct PE32Structure = (PPE32Struct)DataStorage;
	PPE64Struct PE64Structure = (PPE64Struct)DataStorage;
	
	if(FileMapVA != NULL){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, NULL, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				return(false);
			}
			if(!FileIs64){
				__try{
					DOSHeader->e_lfanew = PE32Structure->PE32Offset;
					PEHeader32->OptionalHeader.ImageBase = PE32Structure->ImageBase;
					PEHeader32->OptionalHeader.AddressOfEntryPoint = PE32Structure->OriginalEntryPoint;
					PEHeader32->OptionalHeader.SizeOfImage = PE32Structure->NtSizeOfImage;
					PEHeader32->OptionalHeader.SizeOfHeaders = PE32Structure->NtSizeOfHeaders;
					PEHeader32->FileHeader.SizeOfOptionalHeader = PE32Structure->SizeOfOptionalHeaders;
					PEHeader32->OptionalHeader.FileAlignment = PE32Structure->FileAlignment;
					PEHeader32->OptionalHeader.SectionAlignment = PE32Structure->SectionAligment;
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = PE32Structure->ImportTableAddress;
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = PE32Structure->ImportTableSize;
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = PE32Structure->ResourceTableAddress;
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = PE32Structure->ResourceTableSize;
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = PE32Structure->ExportTableAddress;
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = PE32Structure->ExportTableSize;
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = PE32Structure->TLSTableAddress;
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = PE32Structure->TLSTableSize;
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = PE32Structure->RelocationTableAddress;
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = PE32Structure->RelocationTableSize;
					PEHeader32->FileHeader.TimeDateStamp = PE32Structure->TimeDateStamp;
					PEHeader32->FileHeader.NumberOfSections = PE32Structure->SectionNumber;
					PEHeader32->OptionalHeader.CheckSum = PE32Structure->CheckSum;
					PEHeader32->OptionalHeader.Subsystem = PE32Structure->SubSystem;
					PEHeader32->FileHeader.Characteristics = PE32Structure->Characteristics;
					PEHeader32->OptionalHeader.NumberOfRvaAndSizes = PE32Structure->NumberOfRvaAndSizes;
					return(true);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(false);
				}
			}else{
				__try{
					DOSHeader->e_lfanew = PE64Structure->PE64Offset;
					PEHeader64->OptionalHeader.ImageBase = PE64Structure->ImageBase;
					PEHeader64->OptionalHeader.AddressOfEntryPoint = PE64Structure->OriginalEntryPoint;
					PEHeader64->OptionalHeader.SizeOfImage = PE64Structure->NtSizeOfImage;
					PEHeader64->OptionalHeader.SizeOfHeaders = PE64Structure->NtSizeOfHeaders;
					PEHeader64->FileHeader.SizeOfOptionalHeader = PE64Structure->SizeOfOptionalHeaders;
					PEHeader64->OptionalHeader.FileAlignment = PE64Structure->FileAlignment;
					PEHeader64->OptionalHeader.SectionAlignment = PE64Structure->SectionAligment;
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = PE64Structure->ImportTableAddress;
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = PE64Structure->ImportTableSize;
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = PE64Structure->ResourceTableAddress;
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = PE64Structure->ResourceTableSize;
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = PE64Structure->ExportTableAddress;
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = PE64Structure->ExportTableSize;
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = PE64Structure->TLSTableAddress;
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = PE64Structure->TLSTableSize;
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = PE64Structure->RelocationTableAddress;
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = PE64Structure->RelocationTableSize;
					PEHeader64->FileHeader.TimeDateStamp = PE64Structure->TimeDateStamp;
					PEHeader64->FileHeader.NumberOfSections = PE64Structure->SectionNumber;
					PEHeader64->OptionalHeader.CheckSum = PE64Structure->CheckSum;
					PEHeader64->OptionalHeader.Subsystem = PE64Structure->SubSystem;
					PEHeader64->FileHeader.Characteristics = PE64Structure->Characteristics;
					PEHeader64->OptionalHeader.NumberOfRvaAndSizes = PE64Structure->NumberOfRvaAndSizes;
					return(true);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(false);
				}
			}
		}else{
			return(false);
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall SetPE32DataEx(char* szFileName, LPVOID DataStorage){

	long long ReturnValue = 0;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(MapFileEx(szFileName, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		ReturnValue = SetPE32DataForMappedFileEx(FileMapVA, DataStorage);
		UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
		if(ReturnValue){
			return(true);
		}else{
			return(false);
		}
	}else{
		return(false);
	}
}

__declspec(dllexport) long __stdcall GetPE32SectionNumberFromVA(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	ULONG_PTR FoundInSection = -1;
	DWORD SectionNumber = 0;
	DWORD ConvertAddress = 0;
	BOOL FileIs64;

	if(FileMapVA != NULL){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, NULL, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				return(-2);
			}
			if(!FileIs64){
				__try{
					ConvertAddress = (DWORD)((DWORD)AddressToConvert - PEHeader32->OptionalHeader.ImageBase);
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
					SectionNumber = PEHeader32->FileHeader.NumberOfSections;
					while(SectionNumber > 0){
						if(PESections->VirtualAddress <= ConvertAddress && ConvertAddress < PESections->VirtualAddress + PESections->Misc.VirtualSize){
							FoundInSection = PEHeader32->FileHeader.NumberOfSections - SectionNumber;
						}
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
					}
					return((DWORD)FoundInSection);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(-2);
				}
			}else{
				__try{
					ConvertAddress = (DWORD)(AddressToConvert - PEHeader64->OptionalHeader.ImageBase);
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
					SectionNumber = PEHeader64->FileHeader.NumberOfSections;
					while(SectionNumber > 0){
						if(PESections->VirtualAddress <= ConvertAddress && ConvertAddress < PESections->VirtualAddress + PESections->Misc.VirtualSize){
							FoundInSection = PEHeader64->FileHeader.NumberOfSections - SectionNumber;
						}
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
					}
					return((DWORD)FoundInSection);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(-2);
				}
			}
		}else{
			return(-2);		
		}
	}
	return(-2);
}
__declspec(dllexport) long long __stdcall ConvertVAtoFileOffset(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert, bool ReturnType){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD SectionNumber = 0;
	ULONG_PTR ConvertedAddress = 0;
	ULONG_PTR ConvertAddress = 0;
	BOOL FileIs64;

	if(FileMapVA != NULL){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, NULL, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				return(0);
			}
			if(!FileIs64){
				ConvertAddress = (DWORD)((DWORD)AddressToConvert - PEHeader32->OptionalHeader.ImageBase);
				if(ConvertAddress < PEHeader32->OptionalHeader.SectionAlignment){
					ConvertedAddress = ConvertAddress;
				}
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				SectionNumber = PEHeader32->FileHeader.NumberOfSections;
				__try{
					while(SectionNumber > 0){
						if(PESections->VirtualAddress <= ConvertAddress && ConvertAddress <= PESections->VirtualAddress + PESections->Misc.VirtualSize){
							ConvertedAddress = PESections->PointerToRawData + (ConvertAddress - PESections->VirtualAddress);
						}
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
					}
					if(ReturnType){
						if(ConvertedAddress != NULL){
							ConvertedAddress = ConvertedAddress + FileMapVA;
						}
					}
					return(ConvertedAddress);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(0);
				}
			}else{
				ConvertAddress = (DWORD)(AddressToConvert - PEHeader64->OptionalHeader.ImageBase);
				if(ConvertAddress < PEHeader64->OptionalHeader.SectionAlignment){
					ConvertedAddress = ConvertAddress;
				}
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
				SectionNumber = PEHeader64->FileHeader.NumberOfSections;
				__try{
					while(SectionNumber > 0){
						if(PESections->VirtualAddress <= ConvertAddress && ConvertAddress <= PESections->VirtualAddress + PESections->Misc.VirtualSize){
							ConvertedAddress = PESections->PointerToRawData + (ConvertAddress - PESections->VirtualAddress);
						}
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
					}
					if(ReturnType){
						if(ConvertedAddress != NULL){
							ConvertedAddress = ConvertedAddress + FileMapVA;
						}
					}
					return(ConvertedAddress);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(0);
				}
			}
		}else{
			return(0);		
		}
	}
	return(0);
}
__declspec(dllexport) long long __stdcall ConvertVAtoFileOffsetEx(ULONG_PTR FileMapVA, DWORD FileSize, ULONG_PTR ImageBase, ULONG_PTR AddressToConvert, bool AddressIsRVA, bool ReturnType){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD SectionNumber = 0;
	ULONG_PTR ConvertedAddress = 0;
	ULONG_PTR ConvertAddress = 0;
	BOOL FileIs64;

	if(FileMapVA != NULL){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, NULL, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				return(0);
			}
			if(!FileIs64){
				if(!AddressIsRVA){
					if(ImageBase == NULL){
						ConvertAddress = (DWORD)((DWORD)AddressToConvert - PEHeader32->OptionalHeader.ImageBase);
					}else{
						ConvertAddress = (DWORD)((DWORD)AddressToConvert - ImageBase);
					}
				}else{
					ConvertAddress = (DWORD)AddressToConvert;
				}
				if(ConvertAddress < PEHeader32->OptionalHeader.SectionAlignment){
					ConvertedAddress = ConvertAddress;
				}
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				SectionNumber = PEHeader32->FileHeader.NumberOfSections;
				__try{
					while(SectionNumber > 0){
						if(PESections->VirtualAddress <= ConvertAddress && ConvertAddress <= PESections->VirtualAddress + PESections->Misc.VirtualSize){
							ConvertedAddress = PESections->PointerToRawData + (ConvertAddress - PESections->VirtualAddress);
						}
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
					}
					if(ReturnType){
						if(ConvertedAddress != NULL){
							ConvertedAddress = ConvertedAddress + FileMapVA;
						}
					}
					if(ReturnType){
						if(ConvertedAddress >= FileMapVA && ConvertedAddress <= FileMapVA + FileSize){
							return((ULONG_PTR)ConvertedAddress);
						}else{
							return(NULL);
						}
					}else{
						if(ConvertedAddress > NULL && ConvertedAddress <= FileSize){
							return((ULONG_PTR)ConvertedAddress);
						}else{
							return(NULL);
						}
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(NULL);
				}
			}else{
				if(!AddressIsRVA){
					if(ImageBase == NULL){
						ConvertAddress = (DWORD)(AddressToConvert - PEHeader64->OptionalHeader.ImageBase);
					}else{
						ConvertAddress = (DWORD)(AddressToConvert - ImageBase);
					}
				}else{
					ConvertAddress = (DWORD)AddressToConvert;
				}
				if(ConvertAddress < PEHeader64->OptionalHeader.SectionAlignment){
					ConvertedAddress = ConvertAddress;
				}
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
				SectionNumber = PEHeader64->FileHeader.NumberOfSections;
				__try{
					while(SectionNumber > 0){
						if(PESections->VirtualAddress <= ConvertAddress && ConvertAddress <= PESections->VirtualAddress + PESections->Misc.VirtualSize){
							ConvertedAddress = PESections->PointerToRawData + (ConvertAddress - PESections->VirtualAddress);
						}
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
					}
					if(ReturnType){
						if(ConvertedAddress != NULL){
							ConvertedAddress = ConvertedAddress + FileMapVA;
						}
					}
					if(ReturnType){
						if(ConvertedAddress >= FileMapVA && ConvertedAddress <= FileMapVA + FileSize){
							return((ULONG_PTR)ConvertedAddress);
						}else{
							return(NULL);
						}
					}else{
						if(ConvertedAddress > NULL && ConvertedAddress <= FileSize){
							return((ULONG_PTR)ConvertedAddress);
						}else{
							return(NULL);
						}
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(NULL);
				}
			}
		}else{
			return(0);		
		}
	}
	return(0);
}
__declspec(dllexport) long long __stdcall ConvertFileOffsetToVA(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert, bool ReturnType){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD SectionNumber = 0;
	ULONG_PTR ConvertedAddress = 0;
	ULONG_PTR ConvertAddress = 0;
	BOOL FileIs64;

	if(FileMapVA != NULL){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, NULL, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				return(0);
			}
			if(!FileIs64){
				ConvertAddress = (DWORD)((DWORD)AddressToConvert - FileMapVA);
				if(ConvertAddress < PEHeader32->OptionalHeader.FileAlignment){
					ConvertedAddress = ConvertAddress;
				}
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				SectionNumber = PEHeader32->FileHeader.NumberOfSections;
				__try{
					while(SectionNumber > 0){
						if(PESections->PointerToRawData <= ConvertAddress && ConvertAddress <= PESections->PointerToRawData + PESections->SizeOfRawData){
							ConvertedAddress = PESections->VirtualAddress + (ConvertAddress - PESections->PointerToRawData);
						}
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
					}
					if(ReturnType){
						if(ConvertedAddress != NULL){
							ConvertedAddress = ConvertedAddress + PEHeader32->OptionalHeader.ImageBase;
						}
					}
					return(ConvertedAddress);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(0);
				}
			}else{
				ConvertAddress = (DWORD)(AddressToConvert - FileMapVA);
				if(ConvertAddress < PEHeader64->OptionalHeader.FileAlignment){
					ConvertedAddress = ConvertAddress;
				}
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
				SectionNumber = PEHeader64->FileHeader.NumberOfSections;
				__try{
					while(SectionNumber > 0){
						if(PESections->PointerToRawData <= ConvertAddress && ConvertAddress <= PESections->PointerToRawData + PESections->SizeOfRawData){
							ConvertedAddress = PESections->VirtualAddress + (ConvertAddress - PESections->PointerToRawData);
						}
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						SectionNumber--;
					}
					if(ReturnType){
						if(ConvertedAddress != NULL){
							ConvertedAddress = ConvertedAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase;
						}
					}
					return(ConvertedAddress);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(0);
				}
			}
		}else{
			return(0);		
		}
	}
	return(0);
}
__declspec(dllexport) long long __stdcall ConvertFileOffsetToVAEx(ULONG_PTR FileMapVA, DWORD FileSize, ULONG_PTR ImageBase, ULONG_PTR AddressToConvert, bool ReturnType){

	ULONG_PTR ConvertedAddress = NULL;
	DWORD cnvSectionAlignment = NULL;
	ULONG_PTR cnvImageBase = NULL;
	DWORD cnvSizeOfImage = NULL;

	if(FileMapVA != NULL){
		if(ImageBase == NULL){
			cnvImageBase = (ULONG_PTR)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_IMAGEBASE);
		}else{
			cnvImageBase = ImageBase;
		}
		cnvSizeOfImage = (DWORD)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_SIZEOFIMAGE);
		cnvSectionAlignment = (DWORD)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_SECTIONALIGNMENT);
		ConvertedAddress = (ULONG_PTR)ConvertFileOffsetToVA(FileMapVA, AddressToConvert, ReturnType);
		if(ReturnType){
			if(ConvertedAddress >= cnvImageBase + cnvSectionAlignment && ConvertedAddress <= cnvImageBase + cnvSizeOfImage){
				return((ULONG_PTR)ConvertedAddress);
			}else{
				return(NULL);
			}
		}else{
			if(ConvertedAddress >= cnvSectionAlignment && ConvertedAddress <= cnvSizeOfImage){
				return((ULONG_PTR)ConvertedAddress);
			}else{
				return(NULL);
			}
		}
	}
	return(NULL);
}
// Global.Realigner.functions:
void SetOverallFileStatus(PFILE_STATUS_INFO myFileInfo, BYTE FiledStatus, bool FiledCritical){

	if(myFileInfo->OveralEvaluation == UE_RESULT_FILE_OK || myFileInfo->OveralEvaluation == UE_RESULT_FILE_INVALID_BUT_FIXABLE){
		if(FiledStatus == UE_FILED_FIXABLE_CRITICAL || FiledStatus == UE_FIELD_BROKEN_FIXABLE_FOR_STATIC_USE || FiledStatus == UE_FIELD_BROKEN_BUT_CAN_BE_EMULATED){
			myFileInfo->OveralEvaluation = UE_RESULT_FILE_INVALID_BUT_FIXABLE;
		}else if(FiledStatus == UE_FIELD_BROKEN_NON_FIXABLE && FiledCritical == true){
			myFileInfo->OveralEvaluation = UE_RESULT_FILE_INVALID_AND_NON_FIXABLE;
		}else if(FiledStatus == UE_FIELD_BROKEN_FIXABLE_FOR_STATIC_USE){
			myFileInfo->OveralEvaluation = UE_RESULT_FILE_INVALID_BUT_FIXABLE;
		}
	}
}
// UnpackEngine.Realigner.functions:
__declspec(dllexport) bool __stdcall FixHeaderCheckSum(char* szFileName){

	DWORD HeaderSum = NULL;
	DWORD CheckSum  = NULL;

	if(MapFileAndCheckSumA(szFileName, &HeaderSum, &CheckSum) == NULL){
		SetPE32Data(szFileName, NULL, UE_CHECKSUM, (ULONG_PTR)CheckSum);
		return(true);
	}else{
		return(false);
	}
}
__declspec(dllexport) long __stdcall RealignPE(ULONG_PTR FileMapVA, DWORD FileSize, DWORD RealingMode){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD NewVirtualSectionSize = 0;
	DWORD NewSectionRawPointer = 0;
	DWORD OldSectionDataRawPtr = 0;
	DWORD OldSectionDataPtr = 0;	
	DWORD SectionDataPtr = 0;
	DWORD SectionNumber = 0;
	DWORD CurrentSection = 0;
	DWORD FileAlignment = 0;
	BOOL FileIs64;

	if(FileMapVA != NULL){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, NULL, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				return(-1);
			}
			if(!FileIs64){
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				SectionNumber = PEHeader32->FileHeader.NumberOfSections;
				FileAlignment = PEHeader32->OptionalHeader.FileAlignment;
				if(FileAlignment == 0x1000){
					FileAlignment = 0x200;
				}
				__try{
					PEHeader32->OptionalHeader.FileAlignment = FileAlignment;
					while(SectionNumber > 0){
						SectionDataPtr = PESections->PointerToRawData + PESections->SizeOfRawData;
						SectionDataPtr--;
						while(*(PUCHAR)(FileMapVA + SectionDataPtr) == 0x00 && SectionDataPtr > PESections->PointerToRawData){
							SectionDataPtr--;
						}
						SectionDataPtr = SectionDataPtr - PESections->PointerToRawData;
						OldSectionDataPtr = SectionDataPtr;
						SectionDataPtr = (SectionDataPtr / FileAlignment) * FileAlignment;
						if(SectionDataPtr < OldSectionDataPtr){
							SectionDataPtr = SectionDataPtr + FileAlignment;
						}
						if(CurrentSection == NULL){
							PEHeader32->OptionalHeader.SizeOfHeaders = PESections->PointerToRawData;
							if(PESections->VirtualAddress % 0x1000 == NULL){
								PEHeader32->OptionalHeader.SectionAlignment = 0x1000;
							}else{
								PEHeader32->OptionalHeader.SectionAlignment = PESections->VirtualAddress;
							}
							PESections->SizeOfRawData = SectionDataPtr;
						}else{
							OldSectionDataRawPtr = PESections->PointerToRawData;
							PESections->SizeOfRawData = SectionDataPtr;	
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections - IMAGE_SIZEOF_SECTION_HEADER);
							NewSectionRawPointer = PESections->PointerToRawData + PESections->SizeOfRawData;
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
							PESections->PointerToRawData = NewSectionRawPointer;
							RtlMoveMemory((LPVOID)((ULONG_PTR)FileMapVA + NewSectionRawPointer), (LPVOID)((ULONG_PTR)FileMapVA + OldSectionDataRawPtr), SectionDataPtr);
						}
						NewVirtualSectionSize = (PESections->Misc.VirtualSize / PEHeader32->OptionalHeader.SectionAlignment) * PEHeader32->OptionalHeader.SectionAlignment;
						if(NewVirtualSectionSize < PESections->Misc.VirtualSize){
							NewVirtualSectionSize = NewVirtualSectionSize + PEHeader32->OptionalHeader.SectionAlignment;
						}
						PESections->Misc.VirtualSize = NewVirtualSectionSize;
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						CurrentSection++;
						SectionNumber--;
					}
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections - IMAGE_SIZEOF_SECTION_HEADER);
					return(PESections->PointerToRawData + PESections->SizeOfRawData);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(-1);
				}
			}else{
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
				SectionNumber = PEHeader64->FileHeader.NumberOfSections;
				FileAlignment = PEHeader64->OptionalHeader.FileAlignment;
				if(FileAlignment == 0x1000){
					FileAlignment = 0x200;
				}
				__try{
					PEHeader64->OptionalHeader.FileAlignment = FileAlignment;				
					while(SectionNumber > 0){
						SectionDataPtr = PESections->PointerToRawData + PESections->SizeOfRawData;
						SectionDataPtr--;
						while(*(PUCHAR)(FileMapVA + SectionDataPtr) == 0x00 && SectionDataPtr > PESections->PointerToRawData){
							SectionDataPtr--;
						}
						SectionDataPtr = SectionDataPtr - PESections->PointerToRawData;
						OldSectionDataPtr = SectionDataPtr;
						SectionDataPtr = (SectionDataPtr / FileAlignment) * FileAlignment;
						if(SectionDataPtr < OldSectionDataPtr){
							SectionDataPtr = SectionDataPtr + FileAlignment;
						}
						if(CurrentSection == NULL){
							PEHeader64->OptionalHeader.SizeOfHeaders = PESections->PointerToRawData;
							if(PESections->VirtualAddress % 0x1000 == NULL){
								PEHeader64->OptionalHeader.SectionAlignment = 0x1000;
							}else{
								PEHeader64->OptionalHeader.SectionAlignment = PESections->VirtualAddress;
							}
							PESections->SizeOfRawData = SectionDataPtr;
						}else{
							OldSectionDataRawPtr = PESections->PointerToRawData;
							PESections->SizeOfRawData = SectionDataPtr;	
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections - IMAGE_SIZEOF_SECTION_HEADER);
							NewSectionRawPointer = PESections->PointerToRawData + PESections->SizeOfRawData;
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
							PESections->PointerToRawData = NewSectionRawPointer;
							RtlMoveMemory((LPVOID)((ULONG_PTR)FileMapVA + NewSectionRawPointer), (LPVOID)((ULONG_PTR)FileMapVA + OldSectionDataRawPtr), SectionDataPtr);
						}
						NewVirtualSectionSize = (PESections->Misc.VirtualSize / PEHeader64->OptionalHeader.SectionAlignment) * PEHeader64->OptionalHeader.SectionAlignment;
						if(NewVirtualSectionSize < PESections->Misc.VirtualSize){
							NewVirtualSectionSize = NewVirtualSectionSize + PEHeader64->OptionalHeader.SectionAlignment;
						}
						PESections->Misc.VirtualSize = NewVirtualSectionSize;
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						CurrentSection++;
						SectionNumber--;
					}
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections - IMAGE_SIZEOF_SECTION_HEADER);
					return(PESections->PointerToRawData + PESections->SizeOfRawData);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(-1);
				}
			}
		}else{
			return(-1);		
		}
	}
	return(-1);
}
__declspec(dllexport) long __stdcall RealignPEEx(char* szFileName, DWORD RealingFileSize, DWORD ForcedFileAlignment){

	char szBackupFile[512] = {};
	char szBackupItem[512] = {};
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD NewVirtualSectionSize = 0;
	DWORD NewSectionRawPointer = 0;
	DWORD OldSectionDataRawPtr = 0;
	DWORD OldSectionDataPtr = 0;	
	DWORD SectionDataPtr = 0;
	DWORD SectionNumber = 0;
	DWORD CurrentSection = 0;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(engineBackupForCriticalFunctions && CreateGarbageItem(&szBackupItem, 512)){
		if(!FillGarbageItem(szBackupItem, szFileName, &szBackupFile, 512)){
			RtlZeroMemory(&szBackupItem, 512);
			lstrcpyA(szBackupFile, szFileName);
		}
	}else{
		RtlZeroMemory(&szBackupItem, 512);
		lstrcpyA(szBackupFile, szFileName);
	}
	if(MapFileEx(szBackupFile, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				RemoveGarbageItem(szBackupItem, true);
				return(-1);
			}
			if(!FileIs64){
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
				SectionNumber = PEHeader32->FileHeader.NumberOfSections;
				if(ForcedFileAlignment == 0x0){
					ForcedFileAlignment = 0x200;
				}
				__try{
					PEHeader32->OptionalHeader.FileAlignment = ForcedFileAlignment;
					while(SectionNumber > 0){
						SectionDataPtr = PESections->PointerToRawData + PESections->SizeOfRawData;
						SectionDataPtr--;
						while(*(PUCHAR)(FileMapVA + SectionDataPtr) == 0x00 && SectionDataPtr > PESections->PointerToRawData){
							SectionDataPtr--;
						}
						SectionDataPtr = SectionDataPtr - PESections->PointerToRawData;
						OldSectionDataPtr = SectionDataPtr;
						SectionDataPtr = (SectionDataPtr / ForcedFileAlignment) * ForcedFileAlignment;
						if(SectionDataPtr < OldSectionDataPtr){
							SectionDataPtr = SectionDataPtr + ForcedFileAlignment;
						}
						if(CurrentSection == NULL){
							PEHeader32->OptionalHeader.SizeOfHeaders = PESections->PointerToRawData;
							if(PESections->VirtualAddress % 0x1000 == NULL){
								PEHeader32->OptionalHeader.SectionAlignment = 0x1000;
							}else{
								PEHeader32->OptionalHeader.SectionAlignment = PESections->VirtualAddress;
							}
							PESections->SizeOfRawData = SectionDataPtr;
						}else{
							OldSectionDataRawPtr = PESections->PointerToRawData;
							PESections->SizeOfRawData = SectionDataPtr;	
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections - IMAGE_SIZEOF_SECTION_HEADER);
							NewSectionRawPointer = PESections->PointerToRawData + PESections->SizeOfRawData;
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
							PESections->PointerToRawData = NewSectionRawPointer;
							RtlMoveMemory((LPVOID)((ULONG_PTR)FileMapVA + NewSectionRawPointer), (LPVOID)((ULONG_PTR)FileMapVA + OldSectionDataRawPtr), SectionDataPtr);
						}
						NewVirtualSectionSize = (PESections->Misc.VirtualSize / PEHeader32->OptionalHeader.SectionAlignment) * PEHeader32->OptionalHeader.SectionAlignment;
						if(NewVirtualSectionSize < PESections->Misc.VirtualSize){
							NewVirtualSectionSize = NewVirtualSectionSize + PEHeader32->OptionalHeader.SectionAlignment;
						}
						PESections->Misc.VirtualSize = NewVirtualSectionSize;
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						CurrentSection++;
						SectionNumber--;
					}
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections - IMAGE_SIZEOF_SECTION_HEADER);
					if(RealingFileSize == NULL){
						FileSize = PESections->PointerToRawData + PESections->SizeOfRawData;
					}else{
						FileSize = RealingFileSize;
					}
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					if(szBackupItem[0] != NULL){
						if(CopyFileA(szBackupFile, szFileName, false)){
							RemoveGarbageItem(szBackupItem, true);
							return(FileSize);
						}else{
							RemoveGarbageItem(szBackupItem, true);
							return(-1);
						}
					}else{
						return(FileSize);
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					RemoveGarbageItem(szBackupItem, true);
					return(-1);
				}
			}else{
				PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
				SectionNumber = PEHeader64->FileHeader.NumberOfSections;
				if(ForcedFileAlignment == 0x0){
					ForcedFileAlignment = 0x200;
				}
				__try{
					PEHeader64->OptionalHeader.FileAlignment = ForcedFileAlignment;				
					while(SectionNumber > 0){
						SectionDataPtr = PESections->PointerToRawData + PESections->SizeOfRawData;
						SectionDataPtr--;
						while(*(PUCHAR)(FileMapVA + SectionDataPtr) == 0x00 && SectionDataPtr > PESections->PointerToRawData){
							SectionDataPtr--;
						}
						SectionDataPtr = SectionDataPtr - PESections->PointerToRawData;
						OldSectionDataPtr = SectionDataPtr;
						SectionDataPtr = (SectionDataPtr / ForcedFileAlignment) * ForcedFileAlignment;
						if(SectionDataPtr < OldSectionDataPtr){
							SectionDataPtr = SectionDataPtr + ForcedFileAlignment;
						}
						if(CurrentSection == NULL){
							PEHeader64->OptionalHeader.SizeOfHeaders = PESections->PointerToRawData;
							if(PESections->VirtualAddress % 0x1000 == NULL){
								PEHeader64->OptionalHeader.SectionAlignment = 0x1000;
							}else{
								PEHeader64->OptionalHeader.SectionAlignment = PESections->VirtualAddress;
							}
							PESections->SizeOfRawData = SectionDataPtr;
						}else{
							OldSectionDataRawPtr = PESections->PointerToRawData;
							PESections->SizeOfRawData = SectionDataPtr;	
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections - IMAGE_SIZEOF_SECTION_HEADER);
							NewSectionRawPointer = PESections->PointerToRawData + PESections->SizeOfRawData;
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
							PESections->PointerToRawData = NewSectionRawPointer;
							RtlMoveMemory((LPVOID)((ULONG_PTR)FileMapVA + NewSectionRawPointer), (LPVOID)((ULONG_PTR)FileMapVA + OldSectionDataRawPtr), SectionDataPtr);
						}
						NewVirtualSectionSize = (PESections->Misc.VirtualSize / PEHeader64->OptionalHeader.SectionAlignment) * PEHeader64->OptionalHeader.SectionAlignment;
						if(NewVirtualSectionSize < PESections->Misc.VirtualSize){
							NewVirtualSectionSize = NewVirtualSectionSize + PEHeader64->OptionalHeader.SectionAlignment;
						}
						PESections->Misc.VirtualSize = NewVirtualSectionSize;
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
						CurrentSection++;
						SectionNumber--;
					}
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections - IMAGE_SIZEOF_SECTION_HEADER);
					if(RealingFileSize == NULL){
						FileSize = PESections->PointerToRawData + PESections->SizeOfRawData;
					}else{
						FileSize = RealingFileSize;
					}
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					if(szBackupItem[0] != NULL){
						if(CopyFileA(szBackupFile, szFileName, false)){
							RemoveGarbageItem(szBackupItem, true);
							return(FileSize);
						}else{
							RemoveGarbageItem(szBackupItem, true);
							return(-1);
						}
					}else{
						return(FileSize);
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					RemoveGarbageItem(szBackupItem, true);
					return(-1);
				}
			}
		}else{
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			RemoveGarbageItem(szBackupItem, true);
			return(-1);		
		}
	}
	UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
	RemoveGarbageItem(szBackupItem, true);
	return(-1);
}
__declspec(dllexport) bool __stdcall WipeSection(char* szFileName, int WipeSectionNumber, bool RemovePhysically){

	char szBackupFile[512] = {};
	char szBackupItem[512] = {};
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD NewVirtualSectionSize = 0;
	DWORD NewSectionRawPointer = 0;
	DWORD OldSectionDataRawPtr = 0;
	DWORD OldSectionDataPtr = 0;	
	DWORD CurrentSectionPSize = 0;
	DWORD WipeSectionVirSize = 0;
	DWORD WipeSectionSize = 0;
	DWORD SectionDataPtr = 0;
	DWORD FileAlignment = 0;
	int SectionNumber = 0;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(engineBackupForCriticalFunctions && CreateGarbageItem(&szBackupItem, 512)){
		if(!FillGarbageItem(szBackupItem, szFileName, &szBackupFile, 512)){
			RtlZeroMemory(&szBackupItem, 512);
			lstrcpyA(szBackupFile, szFileName);
		}
	}else{
		RtlZeroMemory(&szBackupItem, 512);
		lstrcpyA(szBackupFile, szFileName);
	}
	if(MapFileEx(szBackupFile, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				RemoveGarbageItem(szBackupItem, true);
				return(false);
			}
			if(!FileIs64){
				if(WipeSectionNumber != -1 && WipeSectionNumber <= PEHeader32->FileHeader.NumberOfSections){
					WipeSectionVirSize = (DWORD)GetPE32DataFromMappedFile(FileMapVA, WipeSectionNumber, UE_SECTIONVIRTUALSIZE);
					WipeSectionSize = (DWORD)GetPE32DataFromMappedFile(FileMapVA, WipeSectionNumber, UE_SECTIONRAWSIZE);
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
					FileAlignment = PEHeader32->OptionalHeader.FileAlignment;
					__try{
						while(SectionNumber < PEHeader32->FileHeader.NumberOfSections){
							if(SectionNumber == WipeSectionNumber - 1){
								CurrentSectionPSize = PESections->SizeOfRawData;
								if(CurrentSectionPSize % FileAlignment == NULL){
									CurrentSectionPSize = ((CurrentSectionPSize / FileAlignment)) * FileAlignment;
								}else{
									CurrentSectionPSize = ((CurrentSectionPSize / FileAlignment) + 1) * FileAlignment;
								}
								PESections->SizeOfRawData = CurrentSectionPSize;
								WipeSectionVirSize = WipeSectionVirSize + PESections->Misc.VirtualSize;
								if(WipeSectionVirSize % PEHeader32->OptionalHeader.SectionAlignment == NULL){
									WipeSectionVirSize = ((WipeSectionVirSize / PEHeader32->OptionalHeader.SectionAlignment)) * PEHeader32->OptionalHeader.SectionAlignment;
								}else{
									WipeSectionVirSize = ((WipeSectionVirSize / PEHeader32->OptionalHeader.SectionAlignment) + 1) * PEHeader32->OptionalHeader.SectionAlignment;				
								}
								PESections->Misc.VirtualSize = WipeSectionVirSize;
								CurrentSectionPSize = CurrentSectionPSize - PESections->SizeOfRawData;
								WipeSectionSize = WipeSectionSize - CurrentSectionPSize;
							}else if(SectionNumber > WipeSectionNumber){
								RtlMoveMemory((LPVOID)((ULONG_PTR)PESections - IMAGE_SIZEOF_SECTION_HEADER), (LPVOID)PESections, IMAGE_SIZEOF_SECTION_HEADER);
							}
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
							SectionNumber++;
						}
						RtlZeroMemory((LPVOID)PESections, IMAGE_SIZEOF_SECTION_HEADER);
						PEHeader32->FileHeader.NumberOfSections--;
						if(RemovePhysically){
							FileSize = RealignPE(FileMapVA, FileSize, NULL);
						}
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						if(szBackupItem[0] != NULL){
							if(CopyFileA(szBackupFile, szFileName, false)){
								RemoveGarbageItem(szBackupItem, true);
								return(true);
							}else{
								RemoveGarbageItem(szBackupItem, true);
								return(false);
							}
						}else{
							return(true);
						}
					}__except(EXCEPTION_EXECUTE_HANDLER){
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						RemoveGarbageItem(szBackupItem, true);
						return(false);
					}
				}
			}else{
				if(WipeSectionNumber != -1 && WipeSectionNumber <= PEHeader64->FileHeader.NumberOfSections){
					WipeSectionVirSize = (DWORD)GetPE32DataFromMappedFile(FileMapVA, WipeSectionNumber, UE_SECTIONVIRTUALOFFSET);
					WipeSectionVirSize = WipeSectionVirSize + (DWORD)GetPE32DataFromMappedFile(FileMapVA, WipeSectionNumber, UE_SECTIONVIRTUALSIZE);
					if(WipeSectionVirSize % PEHeader32->OptionalHeader.SectionAlignment == NULL){
						WipeSectionVirSize = ((WipeSectionVirSize / PEHeader32->OptionalHeader.SectionAlignment)) * PEHeader32->OptionalHeader.SectionAlignment;
					}else{
						WipeSectionVirSize = ((WipeSectionVirSize / PEHeader32->OptionalHeader.SectionAlignment) + 1) * PEHeader32->OptionalHeader.SectionAlignment;				
					}
					WipeSectionSize = (DWORD)GetPE32DataFromMappedFile(FileMapVA, WipeSectionNumber, UE_SECTIONRAWSIZE);
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
					FileAlignment = PEHeader64->OptionalHeader.FileAlignment;
					__try{
						while(SectionNumber < PEHeader64->FileHeader.NumberOfSections){
							if(SectionNumber == WipeSectionNumber - 1){
								CurrentSectionPSize = PESections->SizeOfRawData;
								if(CurrentSectionPSize % FileAlignment == NULL){
									CurrentSectionPSize = ((CurrentSectionPSize / FileAlignment)) * FileAlignment;
								}else{
									CurrentSectionPSize = ((CurrentSectionPSize / FileAlignment) + 1) * FileAlignment;
								}
								PESections->SizeOfRawData = CurrentSectionPSize;
								WipeSectionVirSize = WipeSectionVirSize + PESections->Misc.VirtualSize;
								if(WipeSectionVirSize % PEHeader64->OptionalHeader.SectionAlignment == NULL){
									WipeSectionVirSize = ((WipeSectionVirSize / PEHeader64->OptionalHeader.SectionAlignment)) * PEHeader64->OptionalHeader.SectionAlignment;
								}else{
									WipeSectionVirSize = ((WipeSectionVirSize / PEHeader64->OptionalHeader.SectionAlignment) + 1) * PEHeader64->OptionalHeader.SectionAlignment;
								}
								PESections->Misc.VirtualSize = WipeSectionVirSize;
								CurrentSectionPSize = CurrentSectionPSize - PESections->SizeOfRawData;
								WipeSectionSize = WipeSectionSize - CurrentSectionPSize;
							}else if(SectionNumber > WipeSectionNumber){
								RtlMoveMemory((LPVOID)((ULONG_PTR)PESections - IMAGE_SIZEOF_SECTION_HEADER), (LPVOID)PESections, IMAGE_SIZEOF_SECTION_HEADER);
							}
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
							SectionNumber++;
						}
						RtlZeroMemory((LPVOID)PESections, IMAGE_SIZEOF_SECTION_HEADER);
						PEHeader64->FileHeader.NumberOfSections--;
						if(RemovePhysically){
							FileSize = RealignPE(FileMapVA, FileSize, NULL);
						}
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						if(szBackupItem[0] != NULL){
							if(CopyFileA(szBackupFile, szFileName, false)){
								RemoveGarbageItem(szBackupItem, true);
								return(true);
							}else{
								RemoveGarbageItem(szBackupItem, true);
								return(false);
							}
						}else{
							return(true);
						}
					}__except(EXCEPTION_EXECUTE_HANDLER){
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						RemoveGarbageItem(szBackupItem, true);
						return(false);
					}
				}
			}
		}else{
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			RemoveGarbageItem(szBackupItem, true);
			return(false);		
		}
	}
	UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
	RemoveGarbageItem(szBackupItem, true);
	return(false);
}
__declspec(dllexport) bool __stdcall IsPE32FileValidEx(char* szFileName, DWORD CheckDepth, LPVOID FileStatusInfo){

	unsigned int i;
	DWORD ReadData = NULL;
	DWORD ReadSize = NULL;
	WORD ReadDataWORD = NULL;
	ULONG_PTR hSimulatedFileLoad;
	DWORD SectionAttributes = NULL;
	ULONG_PTR ConvertedAddress = NULL;
	DWORD CorrectedImageSize = NULL;
	DWORD SectionVirtualSize = NULL;	
	DWORD SectionVirtualSizeFixed = NULL;
	DWORD NumberOfSections = NULL;
	FILE_STATUS_INFO myFileStatusInfo;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	PIMAGE_EXPORT_DIRECTORY PEExports;
	PIMAGE_TLS_DIRECTORY32 PETls32;
	PIMAGE_TLS_DIRECTORY64 PETls64;
	PIMAGE_IMPORT_DESCRIPTOR ImportIID;
	PIMAGE_THUNK_DATA32 ThunkData32;
	PIMAGE_THUNK_DATA64 ThunkData64;
	bool hLoadedModuleSimulated = false;
	HMODULE hLoadedModule;
	ULONG_PTR ImportNamePtr;
	ULONG_PTR CurrentThunk;
	BOOL FileIsDLL = false;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	WORD ResourceNamesTable[22] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, 20, 21, 22, 23, 24};

	RtlZeroMemory(&myFileStatusInfo, sizeof FILE_STATUS_INFO);
	if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		myFileStatusInfo.OveralEvaluation = UE_RESULT_FILE_OK;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->Signature == 0x4550 && PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->Signature == 0x4550 && PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
				myFileStatusInfo.FileIs64Bit = true;
			}else{
				myFileStatusInfo.OveralEvaluation = UE_RESULT_FILE_INVALID_FORMAT;
				myFileStatusInfo.SignaturePE = UE_FIELD_BROKEN_NON_FIXABLE;
				if(FileStatusInfo != NULL){
					RtlMoveMemory(FileStatusInfo, &myFileStatusInfo, sizeof FILE_STATUS_INFO);
				}
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(false);
			}
			if(!FileIs64){
				/*
					x86 Surface check
				*/
				__try{
					if(PEHeader32->OptionalHeader.SizeOfImage % PEHeader32->OptionalHeader.SectionAlignment == NULL){
						CorrectedImageSize = ((PEHeader32->OptionalHeader.SizeOfImage / PEHeader32->OptionalHeader.SectionAlignment)) * PEHeader32->OptionalHeader.SectionAlignment;
					}else{
						CorrectedImageSize = ((PEHeader32->OptionalHeader.SizeOfImage / PEHeader32->OptionalHeader.SectionAlignment) + 1) * PEHeader32->OptionalHeader.SectionAlignment;				
					}
					if(PEHeader32->OptionalHeader.SectionAlignment != NULL && PEHeader32->OptionalHeader.SectionAlignment >= PEHeader32->OptionalHeader.FileAlignment){
						myFileStatusInfo.SectionAlignment = UE_FIELD_OK;
						if(PEHeader32->OptionalHeader.SizeOfImage % PEHeader32->OptionalHeader.SectionAlignment == NULL){
							myFileStatusInfo.SizeOfImage = UE_FIELD_OK;
						}else{
							if(CorrectedImageSize < PEHeader32->OptionalHeader.AddressOfEntryPoint){
								myFileStatusInfo.SizeOfImage = UE_FIELD_BROKEN_NON_FIXABLE;
							}else{
								myFileStatusInfo.SizeOfImage = UE_FILED_FIXABLE_NON_CRITICAL;
							}
						}
					}else{
						myFileStatusInfo.SectionAlignment = UE_FILED_FIXABLE_CRITICAL;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.SectionAlignment, true);
					if(PEHeader32->OptionalHeader.ImageBase % 0x1000 == NULL){
						myFileStatusInfo.ImageBase = UE_FIELD_OK;
					}else{
						myFileStatusInfo.ImageBase = UE_FIELD_BROKEN_NON_FIXABLE;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.ImageBase, true);
					if(PEHeader32->OptionalHeader.FileAlignment % 2 == NULL){
						myFileStatusInfo.FileAlignment = UE_FIELD_OK;
					}else{
						myFileStatusInfo.FileAlignment = UE_FILED_FIXABLE_CRITICAL;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.FileAlignment, false);
					/*
						Get the console flag
					*/
					if(PEHeader32->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI){
						myFileStatusInfo.FileIsConsole = true;
					}
					/*
						Export and relocation checks [for DLL and EXE]
					*/
					if(PEHeader32->FileHeader.Characteristics & 0x2000){
						/*
							Export table check
						*/
						FileIsDLL = true;
						myFileStatusInfo.FileIsDLL = true;
						if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXPORT && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress != NULL){
							if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size > CorrectedImageSize){
								myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
							}else{
								ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
								if(ConvertedAddress != NULL){
									if(EngineIsBadReadPtrEx((LPVOID)ConvertedAddress, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)){
										PEExports = (PIMAGE_EXPORT_DIRECTORY)ConvertedAddress;
										if(PEExports->AddressOfFunctions > CorrectedImageSize || PEExports->AddressOfFunctions + 4 * PEExports->NumberOfFunctions > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
										}else if(PEExports->AddressOfNameOrdinals > CorrectedImageSize || PEExports->AddressOfNameOrdinals + 4 * PEExports->NumberOfNames > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
										}else if(PEExports->AddressOfNames > CorrectedImageSize || PEExports->AddressOfNames + 4 * PEExports->NumberOfNames > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
										}else if(PEExports->Name > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
										}else{
											if(CheckDepth == UE_DEPTH_DEEP){
												ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEExports->AddressOfFunctions + PEHeader32->OptionalHeader.ImageBase, false, true);
												if(ConvertedAddress != NULL){
													for(i = 0; i < PEExports->NumberOfFunctions; i++){
														RtlMoveMemory(&ReadData, (LPVOID)ConvertedAddress, 4);
														if(ReadData > CorrectedImageSize || ReadData < PEHeader32->OptionalHeader.SectionAlignment){
															myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
															i = PEExports->NumberOfFunctions;
														}else{
															ConvertedAddress = ConvertedAddress + 4;
														}
													}
												}else{
													myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
												}
												ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEExports->AddressOfNames + PEHeader32->OptionalHeader.ImageBase, false, true);
												if(ConvertedAddress != NULL){
													for(i = 0; i < PEExports->NumberOfNames; i++){
														RtlMoveMemory(&ReadData, (LPVOID)ConvertedAddress, 4);
														if(ReadData > CorrectedImageSize || ReadData < PEHeader32->OptionalHeader.SectionAlignment){
															myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
															i = PEExports->NumberOfNames;
														}else{
															ConvertedAddress = ConvertedAddress + 4;
														}
													}
												}else{
													myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
												}
											}
										}
									}else{
										myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
									}
								}else{
									myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
								}
							}
							SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.ExportTable, true);
						}else{
							myFileStatusInfo.ExportTable = UE_FIELD_NOT_PRESET;
						}
						/*
							Relocation table check
						*/
						if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BASERELOC && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != NULL){
							if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > CorrectedImageSize){
								myFileStatusInfo.RelocationTable = UE_FIELD_BROKEN_NON_FIXABLE;
							}else{
								ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
								if(ConvertedAddress != NULL){
									if(EngineIsBadReadPtrEx((LPVOID)ConvertedAddress, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)){
										RtlMoveMemory(&ReadData, (LPVOID)ConvertedAddress, 4);
										RtlMoveMemory(&ReadSize, (LPVOID)(ConvertedAddress + 4), 4);
										while(ReadData != NULL){
											ReadSize = ReadSize - 8;
											ConvertedAddress = ConvertedAddress + 8;
											while(ReadSize > NULL){
												RtlMoveMemory(&ReadDataWORD, (LPVOID)ConvertedAddress, 2);
												if(ReadDataWORD > 0xCFFF){
													myFileStatusInfo.RelocationTable = UE_FIELD_BROKEN_FIXABLE_FOR_STATIC_USE;
												}
												ConvertedAddress = ConvertedAddress + 2;
												ReadSize = ReadSize - 2;
											}
											RtlMoveMemory(&ReadData, (LPVOID)ConvertedAddress, 4);
											RtlMoveMemory(&ReadSize, (LPVOID)(ConvertedAddress + 4), 4);
										}
									}else{
										myFileStatusInfo.RelocationTable = UE_FIELD_BROKEN_FIXABLE_FOR_STATIC_USE;
									}
								}else{
									myFileStatusInfo.RelocationTable = UE_FIELD_BROKEN_FIXABLE_FOR_STATIC_USE;
								}
							}
							SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.RelocationTable, true);
						}else{
							myFileStatusInfo.RelocationTable = UE_FIELD_NOT_PRESET_WARNING;
						}
					}else{
						/*
							Export table check
						*/
						if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXPORT && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress != NULL){
							if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size > CorrectedImageSize){
								myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_CRITICAL;
							}else{
								ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
								if(ConvertedAddress != NULL){
									if(EngineIsBadReadPtrEx((LPVOID)ConvertedAddress, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)){
										PEExports = (PIMAGE_EXPORT_DIRECTORY)ConvertedAddress;
										if(PEExports->AddressOfFunctions > CorrectedImageSize || PEExports->AddressOfFunctions + 4 * PEExports->NumberOfFunctions > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_CRITICAL;
										}else if(PEExports->AddressOfNameOrdinals > CorrectedImageSize || PEExports->AddressOfNameOrdinals + 4 * PEExports->NumberOfNames > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_CRITICAL;
										}else if(PEExports->AddressOfNames > CorrectedImageSize || PEExports->AddressOfNames + 4 * PEExports->NumberOfNames > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_CRITICAL;
										}else if(PEExports->Name > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_CRITICAL;
										}
									}else{
										myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_CRITICAL;
									}
								}
							}
							SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.ExportTable, false);
						}else{
							myFileStatusInfo.ExportTable = UE_FIELD_NOT_PRESET;
						}
						/*
							Relocation table check
						*/
						if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BASERELOC && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != NULL){
							if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > CorrectedImageSize){
								myFileStatusInfo.RelocationTable = UE_FIELD_BROKEN_NON_FIXABLE;
							}else{
								ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
								if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
									myFileStatusInfo.RelocationTable = UE_FILED_FIXABLE_NON_CRITICAL;
								}
							}
							SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.RelocationTable, false);
						}else{
							myFileStatusInfo.RelocationTable = UE_FIELD_NOT_PRESET;
						}
					}
					/*
						Import table check
					*/
					if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IMPORT && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress != NULL){
						if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress > CorrectedImageSize){
							myFileStatusInfo.ImportTable = UE_FIELD_BROKEN_NON_FIXABLE;
						}else{
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
							if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
								myFileStatusInfo.ImportTable = UE_FIELD_BROKEN_NON_FIXABLE;
							}else{
								SectionAttributes = GetPE32SectionNumberFromVA(FileMapVA, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase);
								if(SectionAttributes >= NULL){
									SectionAttributes = (DWORD)GetPE32DataFromMappedFile(FileMapVA, SectionAttributes, UE_SECTIONFLAGS);
									if(SectionAttributes & IMAGE_SCN_MEM_EXECUTE || SectionAttributes & IMAGE_SCN_CNT_CODE || SectionAttributes & IMAGE_SCN_MEM_WRITE || SectionAttributes & IMAGE_SCN_CNT_INITIALIZED_DATA){
										myFileStatusInfo.ImportTableSection = UE_FIELD_OK;								
									}else{
										myFileStatusInfo.ImportTableSection = UE_FILED_FIXABLE_CRITICAL;
									}
									if(CheckDepth == UE_DEPTH_DEEP){
										if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress != NULL){
											ImportIID = (PIMAGE_IMPORT_DESCRIPTOR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase, (ULONG_PTR)(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase), false, true);
											while(myFileStatusInfo.ImportTableData == UE_FIELD_OK && ImportIID->FirstThunk != NULL){
												hLoadedModule = NULL;
												ImportNamePtr = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase, (ULONG_PTR)((ULONG_PTR)ImportIID->Name + PEHeader32->OptionalHeader.ImageBase), false, true);
												if(ImportNamePtr != NULL){
													if(!EngineIsDependencyPresent((char*)ImportNamePtr, NULL, NULL)){
														myFileStatusInfo.MissingDependencies = true;
														hLoadedModuleSimulated = false;
													}else{
														hLoadedModuleSimulated = false;
														hLoadedModule = GetModuleHandleA((char*)ImportNamePtr);
														if(hLoadedModule == NULL){
															hLoadedModule = (HMODULE)EngineSimulateDllLoader(GetCurrentProcess(), (char*)ImportNamePtr);
															hLoadedModuleSimulated = true;
														}
													}
												}else{
													myFileStatusInfo.ImportTableData = UE_FIELD_BROKEN_NON_FIXABLE;
												}
												ThunkData32 = (PIMAGE_THUNK_DATA32)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase, (ULONG_PTR)((ULONG_PTR)ImportIID->FirstThunk + PEHeader32->OptionalHeader.ImageBase), false, true);
												if(ThunkData32 != NULL){
													CurrentThunk = (ULONG_PTR)ImportIID->FirstThunk;
													while(myFileStatusInfo.ImportTableData == UE_FIELD_OK && ThunkData32->u1.AddressOfData != NULL){
														if(ThunkData32->u1.Ordinal & IMAGE_ORDINAL_FLAG32){
															if((int)(ThunkData32->u1.Ordinal ^ IMAGE_ORDINAL_FLAG32) >= 0x10000){
																myFileStatusInfo.ImportTableData = UE_FIELD_BROKEN_NON_FIXABLE;
															}
														}else{
															ImportNamePtr = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase, (ULONG_PTR)((ULONG_PTR)ThunkData32->u1.AddressOfData + 2 + PEHeader32->OptionalHeader.ImageBase), false, true);
															if(ImportNamePtr != NULL){
																if(!EngineIsBadReadPtrEx((LPVOID)ImportNamePtr, 8)){
																	myFileStatusInfo.ImportTableData = UE_FIELD_BROKEN_NON_FIXABLE;
																}else{
																	if(hLoadedModule != NULL){
																		if(EngineGetProcAddress((ULONG_PTR)hLoadedModule, (char*)ImportNamePtr) == NULL){
																			myFileStatusInfo.MissingDeclaredAPIs = true;
																			SetOverallFileStatus(&myFileStatusInfo, UE_FILED_FIXABLE_CRITICAL, true);
																		}
																	}
																}
															}else{
																myFileStatusInfo.ImportTableData = UE_FIELD_BROKEN_NON_FIXABLE;
															}														
														}
														CurrentThunk = CurrentThunk + 4;
														ThunkData32 = (PIMAGE_THUNK_DATA32)((ULONG_PTR)ThunkData32 + sizeof IMAGE_THUNK_DATA32);
													}
												}else{
													myFileStatusInfo.ImportTableData = UE_FIELD_BROKEN_NON_FIXABLE;
												}
												if(hLoadedModuleSimulated){
													VirtualFree((LPVOID)hLoadedModule, NULL, MEM_RELEASE);
												}
												ImportIID = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG_PTR)ImportIID + sizeof IMAGE_IMPORT_DESCRIPTOR);
											}
										}
									}
								}else{
									myFileStatusInfo.ImportTable = UE_FIELD_BROKEN_NON_FIXABLE;
								}
							}
						}
						SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.ImportTable, true);
						SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.ImportTableSection, true);
					}else{
						myFileStatusInfo.ImportTable = UE_FIELD_NOT_PRESET;
					}
					/*
						TLS table check
					*/
					if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_TLS && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress != NULL){
						if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size > CorrectedImageSize){
							myFileStatusInfo.TLSTable = UE_FILED_FIXABLE_CRITICAL;
						}else{
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
							if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
								myFileStatusInfo.TLSTable = UE_FILED_FIXABLE_CRITICAL;
							}else{
								PETls32 = (PIMAGE_TLS_DIRECTORY32)ConvertedAddress;
								if(PETls32->StartAddressOfRawData != NULL && (PETls32->StartAddressOfRawData < PEHeader32->OptionalHeader.ImageBase || PETls32->StartAddressOfRawData > CorrectedImageSize + PEHeader32->OptionalHeader.ImageBase)){
									myFileStatusInfo.TLSTable = UE_FILED_FIXABLE_CRITICAL;
								}else if(PETls32->EndAddressOfRawData != NULL && (PETls32->EndAddressOfRawData < PEHeader32->OptionalHeader.ImageBase || PETls32->EndAddressOfRawData > CorrectedImageSize + PEHeader32->OptionalHeader.ImageBase)){
									myFileStatusInfo.TLSTable = UE_FILED_FIXABLE_CRITICAL;
								}else if(PETls32->AddressOfIndex != NULL && (PETls32->AddressOfIndex < PEHeader32->OptionalHeader.ImageBase || PETls32->AddressOfIndex > CorrectedImageSize + PEHeader32->OptionalHeader.ImageBase)){
									myFileStatusInfo.TLSTable = UE_FILED_FIXABLE_CRITICAL;
								}else if(PETls32->AddressOfCallBacks != NULL && (PETls32->AddressOfCallBacks < PEHeader32->OptionalHeader.ImageBase || PETls32->AddressOfCallBacks > CorrectedImageSize + PEHeader32->OptionalHeader.ImageBase)){
									myFileStatusInfo.TLSTable = UE_FILED_FIXABLE_CRITICAL;
								}
								if(PETls32->AddressOfCallBacks != NULL && CheckDepth == UE_DEPTH_DEEP){
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PETls32->AddressOfCallBacks + PEHeader32->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress != NULL){
										while(ReadData != NULL){
											RtlMoveMemory(&ReadData, (LPVOID)ConvertedAddress, 4);
											if(ReadData < PEHeader32->OptionalHeader.ImageBase || ReadData > CorrectedImageSize + PEHeader32->OptionalHeader.ImageBase){
												myFileStatusInfo.TLSTable = UE_FILED_FIXABLE_CRITICAL;
											}
											ConvertedAddress = ConvertedAddress + 4;
										}
									}
								}
							}
						}
						SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.TLSTable, false);
					}else{
						myFileStatusInfo.TLSTable = UE_FIELD_NOT_PRESET;
					}
					/*
						Load config table check
					*/
					if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress != NULL){
						if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size > CorrectedImageSize){
							myFileStatusInfo.LoadConfigTable = UE_FILED_FIXABLE_CRITICAL;
						}else{
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
							if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
								myFileStatusInfo.LoadConfigTable = UE_FILED_FIXABLE_CRITICAL;
							}
						}
					}else{
						myFileStatusInfo.LoadConfigTable = UE_FIELD_NOT_PRESET;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.LoadConfigTable, false);
					/*
						Bound import table check
					*/
					if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress != NULL){
						if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size > CorrectedImageSize){
							myFileStatusInfo.BoundImportTable = UE_FILED_FIXABLE_CRITICAL;
						}else{
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
							if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
								myFileStatusInfo.BoundImportTable = UE_FILED_FIXABLE_CRITICAL;
							}
						}
					}else{
						myFileStatusInfo.BoundImportTable = UE_FIELD_NOT_PRESET;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.BoundImportTable, false);
					/*
						IAT check
					*/
					if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IAT && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress != NULL){
						if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size > CorrectedImageSize){
							myFileStatusInfo.IATTable = UE_FILED_FIXABLE_NON_CRITICAL;
						}else{
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
							if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
								myFileStatusInfo.IATTable = UE_FILED_FIXABLE_NON_CRITICAL;
							}
						}
					}else{
						myFileStatusInfo.IATTable = UE_FIELD_NOT_PRESET;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.IATTable, false);
					/*
						COM header check
					*/
					if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress != NULL){
						if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size > CorrectedImageSize){
							myFileStatusInfo.COMHeaderTable = UE_FILED_FIXABLE_NON_CRITICAL;
						}else{
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
							if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
								myFileStatusInfo.COMHeaderTable = UE_FILED_FIXABLE_NON_CRITICAL;
							}
						}
					}else{
						myFileStatusInfo.COMHeaderTable = UE_FIELD_NOT_PRESET;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.COMHeaderTable, false);
					/*
						Resource header check
					*/
					if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_RESOURCE && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress != NULL){
						if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size > CorrectedImageSize){
							myFileStatusInfo.ResourceTable = UE_FILED_FIXABLE_NON_CRITICAL;
						}else{
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
							if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize || ConvertedAddress - FileMapVA + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size > FileSize){
								myFileStatusInfo.ResourceTable = UE_FIELD_BROKEN_BUT_CAN_BE_EMULATED;
							}
							if(CheckDepth == UE_DEPTH_DEEP){
								hSimulatedFileLoad = (ULONG_PTR)EngineSimulateNtLoader(szFileName);
								if(hSimulatedFileLoad != NULL){								
									for(i = 0; i < 22; i++){
										if(myFileStatusInfo.ResourceData == UE_FIELD_OK){
											EnumResourceNamesA((HMODULE)hSimulatedFileLoad, MAKEINTRESOURCEA(ResourceNamesTable[i]), (ENUMRESNAMEPROCA)EngineValidateResource, (ULONG_PTR)&myFileStatusInfo.ResourceData);
										}else{
											i = 22;
										}
									}
									VirtualFree((LPVOID)hSimulatedFileLoad, NULL, MEM_RELEASE);
								}
							}
						}
						SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.ResourceTable, true);
					}else{
						myFileStatusInfo.ResourceTable = UE_FIELD_NOT_PRESET;
					}
					/*
						Section check
					*/
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
					NumberOfSections = PEHeader32->FileHeader.NumberOfSections;
					while(NumberOfSections > NULL){
						SectionVirtualSize = PESections->VirtualAddress + PESections->Misc.VirtualSize;
						if(PESections->Misc.VirtualSize % PEHeader32->OptionalHeader.SectionAlignment == NULL){
							SectionVirtualSizeFixed = SectionVirtualSize;
						}else{
							SectionVirtualSizeFixed = PESections->VirtualAddress + (((PESections->Misc.VirtualSize / PEHeader32->OptionalHeader.SectionAlignment) + 1) * PEHeader32->OptionalHeader.SectionAlignment);
						}
						if(NumberOfSections > 1){
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + sizeof IMAGE_SECTION_HEADER);
							if(SectionVirtualSize > PESections->VirtualAddress || SectionVirtualSizeFixed > PESections->VirtualAddress){
								myFileStatusInfo.SectionTable = UE_FILED_FIXABLE_CRITICAL;
							}
						}
						NumberOfSections--;
					}
					if(PESections->PointerToRawData + PESections->SizeOfRawData > FileSize && PESections->SizeOfRawData != NULL){
						myFileStatusInfo.SectionTable = UE_FIELD_BROKEN_NON_FIXABLE;
					}
					SectionVirtualSizeFixed = SectionVirtualSizeFixed + 0xF000;
					if(PEHeader32->OptionalHeader.SizeOfImage > SectionVirtualSizeFixed){
						myFileStatusInfo.SizeOfImage = UE_FILED_FIXABLE_CRITICAL;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.SizeOfImage, true);
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.SectionTable, true);
					/*
						Entry point check
					*/
					SectionAttributes = GetPE32SectionNumberFromVA(FileMapVA, PEHeader32->OptionalHeader.AddressOfEntryPoint + PEHeader32->OptionalHeader.ImageBase);
					if(SectionAttributes != -1){
						SectionAttributes = (DWORD)GetPE32DataFromMappedFile(FileMapVA, SectionAttributes, UE_SECTIONFLAGS);
						if(SectionAttributes & IMAGE_SCN_MEM_EXECUTE || SectionAttributes & IMAGE_SCN_CNT_CODE){
							myFileStatusInfo.EntryPoint = UE_FIELD_OK;								
						}else{
							myFileStatusInfo.EntryPoint = UE_FILED_FIXABLE_CRITICAL;
						}
					}
					ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.AddressOfEntryPoint + PEHeader32->OptionalHeader.ImageBase, false, true);
					if(ConvertedAddress == NULL){
						myFileStatusInfo.EntryPoint = UE_FIELD_BROKEN_NON_FIXABLE;
					}else{
						ReadData = NULL;
						if(memcmp(&ReadData, (LPVOID)ConvertedAddress, 4) == NULL){
							myFileStatusInfo.EntryPoint = UE_FIELD_BROKEN_NON_FIXABLE;
						}
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.EntryPoint, true);
					/*
						Return data
					*/
					if(FileStatusInfo != NULL){
						RtlMoveMemory(FileStatusInfo, &myFileStatusInfo, sizeof FILE_STATUS_INFO);
					}
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(true);	
				}__except(EXCEPTION_EXECUTE_HANDLER){
					myFileStatusInfo.EvaluationTerminatedByException = true;
					myFileStatusInfo.OveralEvaluation = UE_RESULT_FILE_INVALID_FORMAT;
					myFileStatusInfo.SignaturePE = UE_FIELD_BROKEN_NON_FIXABLE;
					if(FileStatusInfo != NULL){
						RtlMoveMemory(FileStatusInfo, &myFileStatusInfo, sizeof FILE_STATUS_INFO);
					}
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);
				}
			}else{
				/*
					x64 Surface check
				*/
				__try{
					if(PEHeader64->OptionalHeader.SizeOfImage % PEHeader64->OptionalHeader.SectionAlignment == NULL){
						CorrectedImageSize = ((PEHeader64->OptionalHeader.SizeOfImage / PEHeader64->OptionalHeader.SectionAlignment)) * PEHeader64->OptionalHeader.SectionAlignment;
					}else{
						CorrectedImageSize = ((PEHeader64->OptionalHeader.SizeOfImage / PEHeader64->OptionalHeader.SectionAlignment) + 1) * PEHeader64->OptionalHeader.SectionAlignment;				
					}
					if(PEHeader64->OptionalHeader.SectionAlignment != NULL && PEHeader64->OptionalHeader.SectionAlignment >= PEHeader64->OptionalHeader.FileAlignment){
						myFileStatusInfo.SectionAlignment = UE_FIELD_OK;
						if(PEHeader64->OptionalHeader.SizeOfImage % PEHeader64->OptionalHeader.SectionAlignment == NULL){
							myFileStatusInfo.SizeOfImage = UE_FIELD_OK;
						}else{
							if(CorrectedImageSize < PEHeader64->OptionalHeader.AddressOfEntryPoint){
								myFileStatusInfo.SizeOfImage = UE_FIELD_BROKEN_NON_FIXABLE;
							}else{
								myFileStatusInfo.SizeOfImage = UE_FILED_FIXABLE_NON_CRITICAL;
							}
						}
					}else{
						myFileStatusInfo.SectionAlignment = UE_FILED_FIXABLE_CRITICAL;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.SectionAlignment, true);
					if((ULONG_PTR)PEHeader64->OptionalHeader.ImageBase % 0x1000 == NULL){
						myFileStatusInfo.ImageBase = UE_FIELD_OK;
					}else{
						myFileStatusInfo.ImageBase = UE_FIELD_BROKEN_NON_FIXABLE;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.ImageBase, true);
					if(PEHeader64->OptionalHeader.FileAlignment % 2 == NULL){
						myFileStatusInfo.FileAlignment = UE_FIELD_OK;
					}else{
						myFileStatusInfo.FileAlignment = UE_FILED_FIXABLE_CRITICAL;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.FileAlignment, false);
					/*
						Get the console flag
					*/
					if(PEHeader64->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI){
						myFileStatusInfo.FileIsConsole = true;
					}
					/*
						Export and relocation checks [for DLL and EXE]
					*/
					if(PEHeader64->FileHeader.Characteristics & 0x2000){
						/*
							Export table check
						*/
						FileIsDLL = true;
						myFileStatusInfo.FileIsDLL = true;
						if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXPORT && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress != NULL){
							if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size > CorrectedImageSize){
								myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
							}else{
								ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
								if(ConvertedAddress != NULL){
									if(EngineIsBadReadPtrEx((LPVOID)ConvertedAddress, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)){
										PEExports = (PIMAGE_EXPORT_DIRECTORY)ConvertedAddress;
										if(PEExports->AddressOfFunctions > CorrectedImageSize || PEExports->AddressOfFunctions + 4 * PEExports->NumberOfFunctions > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
										}else if(PEExports->AddressOfNameOrdinals > CorrectedImageSize || PEExports->AddressOfNameOrdinals + 4 * PEExports->NumberOfNames > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
										}else if(PEExports->AddressOfNames > CorrectedImageSize || PEExports->AddressOfNames + 4 * PEExports->NumberOfNames > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
										}else if(PEExports->Name > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
										}else{
											if(CheckDepth == UE_DEPTH_DEEP){
												ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEExports->AddressOfFunctions + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
												if(ConvertedAddress != NULL){
													for(i = 0; i < PEExports->NumberOfFunctions; i++){
														RtlMoveMemory(&ReadData, (LPVOID)ConvertedAddress, 4);
														if(ReadData > CorrectedImageSize || ReadData < PEHeader64->OptionalHeader.SectionAlignment){
															myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
															i = PEExports->NumberOfFunctions;
														}else{
															ConvertedAddress = ConvertedAddress + 4;
														}
													}
												}else{
													myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
												}
												ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEExports->AddressOfNames + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
												if(ConvertedAddress != NULL){
													for(i = 0; i < PEExports->NumberOfNames; i++){
														RtlMoveMemory(&ReadData, (LPVOID)ConvertedAddress, 4);
														if(ReadData > CorrectedImageSize || ReadData < PEHeader64->OptionalHeader.SectionAlignment){
															myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
															i = PEExports->NumberOfNames;
														}else{
															ConvertedAddress = ConvertedAddress + 4;
														}
													}
												}else{
													myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
												}
											}
										}
									}else{
										myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
									}
								}else{
									myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_FIXABLE;
								}
							}
							SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.ExportTable, true);
						}else{
							myFileStatusInfo.ExportTable = UE_FIELD_NOT_PRESET;
						}
						/*
							Relocation table check
						*/
						if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BASERELOC && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != NULL){
							if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > CorrectedImageSize){
								myFileStatusInfo.RelocationTable = UE_FIELD_BROKEN_NON_FIXABLE;
							}else{
								ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
								if(ConvertedAddress != NULL){
									if(EngineIsBadReadPtrEx((LPVOID)ConvertedAddress, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)){
										RtlMoveMemory(&ReadData, (LPVOID)ConvertedAddress, 4);
										RtlMoveMemory(&ReadSize, (LPVOID)(ConvertedAddress + 4), 4);
										while(ReadData != NULL){
											ReadSize = ReadSize - 8;
											ConvertedAddress = ConvertedAddress + 8;
											while(ReadSize > NULL){
												RtlMoveMemory(&ReadDataWORD, (LPVOID)ConvertedAddress, 2);
												if(ReadDataWORD > 0xCFFF){
													myFileStatusInfo.RelocationTable = UE_FIELD_BROKEN_FIXABLE_FOR_STATIC_USE;
												}
												ConvertedAddress = ConvertedAddress + 2;
												ReadSize = ReadSize - 2;
											}
											RtlMoveMemory(&ReadData, (LPVOID)ConvertedAddress, 4);
											RtlMoveMemory(&ReadSize, (LPVOID)(ConvertedAddress + 4), 4);
										}
									}else{
										myFileStatusInfo.RelocationTable = UE_FIELD_BROKEN_FIXABLE_FOR_STATIC_USE;
									}
								}else{
									myFileStatusInfo.RelocationTable = UE_FIELD_BROKEN_FIXABLE_FOR_STATIC_USE;
								}
							}
							SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.RelocationTable, true);
						}else{
							myFileStatusInfo.RelocationTable = UE_FIELD_NOT_PRESET_WARNING;
						}
					}else{
						/*
							Export table check
						*/
						if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXPORT && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress != NULL){
							if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size > CorrectedImageSize){
								myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_CRITICAL;
							}else{
								ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
								if(ConvertedAddress != NULL){
									if(EngineIsBadReadPtrEx((LPVOID)ConvertedAddress, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)){
										PEExports = (PIMAGE_EXPORT_DIRECTORY)ConvertedAddress;
										if(PEExports->AddressOfFunctions > CorrectedImageSize || PEExports->AddressOfFunctions + 4 * PEExports->NumberOfFunctions > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_CRITICAL;
										}else if(PEExports->AddressOfNameOrdinals > CorrectedImageSize || PEExports->AddressOfNameOrdinals + 4 * PEExports->NumberOfNames > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_CRITICAL;
										}else if(PEExports->AddressOfNames > CorrectedImageSize || PEExports->AddressOfNames + 4 * PEExports->NumberOfNames > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_CRITICAL;
										}else if(PEExports->Name > CorrectedImageSize){
											myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_CRITICAL;
										}
									}else{
										myFileStatusInfo.ExportTable = UE_FIELD_BROKEN_NON_CRITICAL;
									}
								}
							}
							SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.ExportTable, false);
						}else{
							myFileStatusInfo.ExportTable = UE_FIELD_NOT_PRESET;
						}
						/*
							Relocation table check
						*/
						if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BASERELOC && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != NULL){
							if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > CorrectedImageSize){
								myFileStatusInfo.RelocationTable = UE_FIELD_BROKEN_NON_FIXABLE;
							}else{
								ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
								if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
									myFileStatusInfo.RelocationTable = UE_FILED_FIXABLE_NON_CRITICAL;
								}
							}
							SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.RelocationTable, false);
						}else{
							myFileStatusInfo.RelocationTable = UE_FIELD_NOT_PRESET;
						}
					}
					/*
						Import table check
					*/
					if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IMPORT && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress != NULL){
						if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress > CorrectedImageSize){
							myFileStatusInfo.ImportTable = UE_FIELD_BROKEN_NON_FIXABLE;
						}else{
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
							if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
								myFileStatusInfo.ImportTable = UE_FIELD_BROKEN_NON_FIXABLE;
							}else{
								SectionAttributes = GetPE32SectionNumberFromVA(FileMapVA, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase);
								if(SectionAttributes >= NULL){
									SectionAttributes = (DWORD)GetPE32DataFromMappedFile(FileMapVA, SectionAttributes, UE_SECTIONFLAGS);
									if(SectionAttributes & IMAGE_SCN_MEM_EXECUTE || SectionAttributes & IMAGE_SCN_CNT_CODE || SectionAttributes & IMAGE_SCN_MEM_WRITE || SectionAttributes & IMAGE_SCN_CNT_INITIALIZED_DATA){
										myFileStatusInfo.ImportTableSection = UE_FIELD_OK;								
									}else{
										myFileStatusInfo.ImportTableSection = UE_FILED_FIXABLE_CRITICAL;
									}
									if(CheckDepth == UE_DEPTH_DEEP){
										if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress != NULL){
											ImportIID = (PIMAGE_IMPORT_DESCRIPTOR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)(ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, (ULONG_PTR)(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase), false, true);
											while(myFileStatusInfo.ImportTableData == UE_FIELD_OK && ImportIID->FirstThunk != NULL){
												hLoadedModule = NULL;
												ImportNamePtr = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)(ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, (ULONG_PTR)((ULONG_PTR)ImportIID->Name + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase), false, true);
												if(ImportNamePtr != NULL){
													if(!EngineIsDependencyPresent((char*)ImportNamePtr, NULL, NULL)){
														myFileStatusInfo.MissingDependencies = true;
														hLoadedModuleSimulated = false;
													}else{
														hLoadedModuleSimulated = false;
														hLoadedModule = GetModuleHandleA((char*)ImportNamePtr);
														if(hLoadedModule == NULL){
															hLoadedModule = (HMODULE)EngineSimulateDllLoader(GetCurrentProcess(), (char*)ImportNamePtr);
															hLoadedModuleSimulated = true;
														}
													}
												}else{
													myFileStatusInfo.ImportTableData = UE_FIELD_BROKEN_NON_FIXABLE;
												}
												ThunkData64 = (PIMAGE_THUNK_DATA64)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)(ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, (ULONG_PTR)((ULONG_PTR)ImportIID->FirstThunk + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase), false, true);
												if(ThunkData64 != NULL){
													CurrentThunk = (ULONG_PTR)ImportIID->FirstThunk;
													while(myFileStatusInfo.ImportTableData == UE_FIELD_OK && ThunkData64->u1.AddressOfData != NULL){
														if(ThunkData64->u1.Ordinal & IMAGE_ORDINAL_FLAG64){
															if((int)(ThunkData64->u1.Ordinal ^ IMAGE_ORDINAL_FLAG64) >= 0x10000){
																myFileStatusInfo.ImportTableData = UE_FIELD_BROKEN_NON_FIXABLE;
															}
														}else{
															ImportNamePtr = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)(ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, (ULONG_PTR)((ULONG_PTR)ThunkData64->u1.AddressOfData + 2 + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase), false, true);
															if(ImportNamePtr != NULL){
																if(!EngineIsBadReadPtrEx((LPVOID)ImportNamePtr, 8)){
																	myFileStatusInfo.ImportTableData = UE_FIELD_BROKEN_NON_FIXABLE;
																}else{
																	if(hLoadedModule != NULL){
																		if(EngineGetProcAddress((ULONG_PTR)hLoadedModule, (char*)ImportNamePtr) == NULL){
																			myFileStatusInfo.MissingDeclaredAPIs = true;
																			SetOverallFileStatus(&myFileStatusInfo, UE_FILED_FIXABLE_CRITICAL, true);
																		}
																	}
																}
															}else{
																myFileStatusInfo.ImportTableData = UE_FIELD_BROKEN_NON_FIXABLE;
															}														
														}
														CurrentThunk = CurrentThunk + 8;
														ThunkData64 = (PIMAGE_THUNK_DATA64)((ULONG_PTR)ThunkData64 + sizeof IMAGE_THUNK_DATA64);
													}
												}else{
													myFileStatusInfo.ImportTableData = UE_FIELD_BROKEN_NON_FIXABLE;
												}
												if(hLoadedModuleSimulated){
													VirtualFree((LPVOID)hLoadedModule, NULL, MEM_RELEASE);
												}
												ImportIID = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG_PTR)ImportIID + sizeof IMAGE_IMPORT_DESCRIPTOR);
											}
										}
									}
								}else{
									myFileStatusInfo.ImportTable = UE_FIELD_BROKEN_NON_FIXABLE;
								}
							}
						}
						SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.ImportTable, true);
						SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.ImportTableSection, true);
					}else{
						myFileStatusInfo.ImportTable = UE_FIELD_NOT_PRESET;
					}
					/*
						TLS table check
					*/
					if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_TLS && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress != NULL){
						if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size > CorrectedImageSize){
							myFileStatusInfo.TLSTable = UE_FILED_FIXABLE_CRITICAL;
						}else{
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
							if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
								myFileStatusInfo.TLSTable = UE_FILED_FIXABLE_CRITICAL;
							}else{
								PETls64 = (PIMAGE_TLS_DIRECTORY64)ConvertedAddress;
								if(PETls64->StartAddressOfRawData != NULL && (PETls64->StartAddressOfRawData < (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase || PETls64->StartAddressOfRawData > CorrectedImageSize + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase)){
									myFileStatusInfo.TLSTable = UE_FILED_FIXABLE_CRITICAL;
								}else if(PETls64->EndAddressOfRawData != NULL && (PETls64->EndAddressOfRawData < (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase || PETls64->EndAddressOfRawData > CorrectedImageSize + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase)){
									myFileStatusInfo.TLSTable = UE_FILED_FIXABLE_CRITICAL;
								}else if(PETls64->AddressOfIndex != NULL && (PETls64->AddressOfIndex < (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase || PETls64->AddressOfIndex > CorrectedImageSize + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase)){
									myFileStatusInfo.TLSTable = UE_FILED_FIXABLE_CRITICAL;
								}else if(PETls64->AddressOfCallBacks != NULL && (PETls64->AddressOfCallBacks < (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase || PETls64->AddressOfCallBacks > CorrectedImageSize + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase)){
									myFileStatusInfo.TLSTable = UE_FILED_FIXABLE_CRITICAL;
								}
								if(PETls64->AddressOfCallBacks != NULL && CheckDepth == UE_DEPTH_DEEP){
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, (ULONG_PTR)PETls64->AddressOfCallBacks + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress != NULL){
										while(ReadData != NULL){
											RtlMoveMemory(&ReadData, (LPVOID)ConvertedAddress, 8);
											if(ReadData < (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase || ReadData > CorrectedImageSize + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase){
												myFileStatusInfo.TLSTable = UE_FILED_FIXABLE_CRITICAL;
											}
											ConvertedAddress = ConvertedAddress + 8;
										}
									}
								}
							}
						}
						SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.TLSTable, false);
					}else{
						myFileStatusInfo.TLSTable = UE_FIELD_NOT_PRESET;
					}
					/*
						Load config table check
					*/
					if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress != NULL){
						if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size > CorrectedImageSize){
							myFileStatusInfo.LoadConfigTable = UE_FILED_FIXABLE_CRITICAL;
						}else{
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
							if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
								myFileStatusInfo.LoadConfigTable = UE_FILED_FIXABLE_CRITICAL;
							}
						}
					}else{
						myFileStatusInfo.LoadConfigTable = UE_FIELD_NOT_PRESET;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.LoadConfigTable, false);
					/*
						Bound import table check
					*/
					if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress != NULL){
						if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size > CorrectedImageSize){
							myFileStatusInfo.BoundImportTable = UE_FILED_FIXABLE_CRITICAL;
						}else{
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
							if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
								myFileStatusInfo.BoundImportTable = UE_FILED_FIXABLE_CRITICAL;
							}
						}
					}else{
						myFileStatusInfo.BoundImportTable = UE_FIELD_NOT_PRESET;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.BoundImportTable, false);
					/*
						IAT check
					*/
					if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IAT && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress != NULL){
						if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size > CorrectedImageSize){
							myFileStatusInfo.IATTable = UE_FILED_FIXABLE_NON_CRITICAL;
						}else{
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
							if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
								myFileStatusInfo.IATTable = UE_FILED_FIXABLE_NON_CRITICAL;
							}
						}
					}else{
						myFileStatusInfo.IATTable = UE_FIELD_NOT_PRESET;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.IATTable, false);
					/*
						COM header check
					*/
					if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress != NULL){
						if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size > CorrectedImageSize){
							myFileStatusInfo.COMHeaderTable = UE_FILED_FIXABLE_NON_CRITICAL;
						}else{
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
							if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
								myFileStatusInfo.COMHeaderTable = UE_FILED_FIXABLE_NON_CRITICAL;
							}
						}
					}else{
						myFileStatusInfo.COMHeaderTable = UE_FIELD_NOT_PRESET;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.COMHeaderTable, false);
					/*
						Resource header check
					*/
					if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_RESOURCE && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress != NULL){
						if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size > CorrectedImageSize){
							myFileStatusInfo.ResourceTable = UE_FILED_FIXABLE_NON_CRITICAL;
						}else{
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
							if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize || ConvertedAddress - FileMapVA + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size > FileSize){
								myFileStatusInfo.ResourceTable = UE_FIELD_BROKEN_BUT_CAN_BE_EMULATED;
							}
							if(CheckDepth == UE_DEPTH_DEEP){
								hSimulatedFileLoad = (ULONG_PTR)EngineSimulateNtLoader(szFileName);
								if(hSimulatedFileLoad != NULL){								
									for(i = 0; i < 22; i++){
										if(myFileStatusInfo.ResourceData == UE_FIELD_OK){
											EnumResourceNamesA((HMODULE)hSimulatedFileLoad, MAKEINTRESOURCEA(ResourceNamesTable[i]), (ENUMRESNAMEPROCA)EngineValidateResource, (ULONG_PTR)&myFileStatusInfo.ResourceData);
										}else{
											i = 22;
										}
									}
									VirtualFree((LPVOID)hSimulatedFileLoad, NULL, MEM_RELEASE);
								}
							}
						}
						SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.ResourceTable, true);
					}else{
						myFileStatusInfo.ResourceTable = UE_FIELD_NOT_PRESET;
					}
					/*
						Section check
					*/
					PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
					NumberOfSections = PEHeader64->FileHeader.NumberOfSections;
					while(NumberOfSections > NULL){
						SectionVirtualSize = PESections->VirtualAddress + PESections->Misc.VirtualSize;
						if(PESections->Misc.VirtualSize % PEHeader64->OptionalHeader.SectionAlignment == NULL){
							SectionVirtualSizeFixed = SectionVirtualSize;
						}else{
							SectionVirtualSizeFixed = PESections->VirtualAddress + (((PESections->Misc.VirtualSize / PEHeader64->OptionalHeader.SectionAlignment) + 1) * PEHeader64->OptionalHeader.SectionAlignment);
						}
						if(NumberOfSections > 1){
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + sizeof IMAGE_SECTION_HEADER);
							if(SectionVirtualSize > PESections->VirtualAddress || SectionVirtualSizeFixed > PESections->VirtualAddress){
								myFileStatusInfo.SectionTable = UE_FILED_FIXABLE_CRITICAL;
							}
						}
						NumberOfSections--;
					}
					if(PESections->PointerToRawData + PESections->SizeOfRawData > FileSize && PESections->SizeOfRawData != NULL){
						myFileStatusInfo.SectionTable = UE_FIELD_BROKEN_NON_FIXABLE;
					}
					SectionVirtualSizeFixed = SectionVirtualSizeFixed + 0xF000;
					if(PEHeader64->OptionalHeader.SizeOfImage > SectionVirtualSizeFixed){
						myFileStatusInfo.SizeOfImage = UE_FILED_FIXABLE_CRITICAL;
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.SizeOfImage, true);
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.SectionTable, true);
					/*
						Entry point check
					*/
					SectionAttributes = GetPE32SectionNumberFromVA(FileMapVA, PEHeader64->OptionalHeader.AddressOfEntryPoint + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase);
					if(SectionAttributes != -1){
						SectionAttributes = (DWORD)GetPE32DataFromMappedFile(FileMapVA, SectionAttributes, UE_SECTIONFLAGS);
						if(SectionAttributes & IMAGE_SCN_MEM_EXECUTE || SectionAttributes & IMAGE_SCN_CNT_CODE){
							myFileStatusInfo.EntryPoint = UE_FIELD_OK;								
						}else{
							myFileStatusInfo.EntryPoint = UE_FILED_FIXABLE_CRITICAL;
						}
					}
					ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.AddressOfEntryPoint + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
					if(ConvertedAddress == NULL){
						myFileStatusInfo.EntryPoint = UE_FIELD_BROKEN_NON_FIXABLE;
					}else{
						ReadData = NULL;
						if(memcmp(&ReadData, (LPVOID)ConvertedAddress, 4) == NULL){
							myFileStatusInfo.EntryPoint = UE_FIELD_BROKEN_NON_FIXABLE;
						}
					}
					SetOverallFileStatus(&myFileStatusInfo, myFileStatusInfo.EntryPoint, true);
					/*
						Return data
					*/
					if(FileStatusInfo != NULL){
						RtlMoveMemory(FileStatusInfo, &myFileStatusInfo, sizeof FILE_STATUS_INFO);
					}
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(true);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					myFileStatusInfo.EvaluationTerminatedByException = true;
					myFileStatusInfo.OveralEvaluation = UE_RESULT_FILE_INVALID_FORMAT;
					myFileStatusInfo.SignaturePE = UE_FIELD_BROKEN_NON_FIXABLE;
					if(FileStatusInfo != NULL){
						RtlMoveMemory(FileStatusInfo, &myFileStatusInfo, sizeof FILE_STATUS_INFO);
					}
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);
				}
			}
		}else{
			myFileStatusInfo.OveralEvaluation = UE_RESULT_FILE_INVALID_FORMAT;
			myFileStatusInfo.SignatureMZ = UE_FIELD_BROKEN_NON_FIXABLE;
			if(FileStatusInfo != NULL){
				RtlMoveMemory(FileStatusInfo, &myFileStatusInfo, sizeof FILE_STATUS_INFO);
			}
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			return(false);	
		}
	}
	if(FileStatusInfo != NULL){
		RtlMoveMemory(FileStatusInfo, &myFileStatusInfo, sizeof FILE_STATUS_INFO);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall FixBrokenPE32FileEx(char* szFileName, LPVOID FileStatusInfo, LPVOID FileFixInfo){

	DWORD ReadData = NULL;
	DWORD ReadSize = NULL;
	WORD ReadDataWORD = NULL;
	ULONG_PTR ReadDataQWORD = NULL;
	DWORD OrdinalBase = NULL;
	DWORD OrdinalCount = NULL;
	DWORD SectionAttributes = NULL;
	ULONG_PTR ConvertedAddress = NULL;
	DWORD CorrectedImageSize = NULL;
	DWORD SectionVirtualSize = NULL;	
	DWORD SectionVirtualSizeFixed = NULL;
	DWORD NumberOfSections = NULL;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	PIMAGE_EXPORT_DIRECTORY PEExports;
	PIMAGE_TLS_DIRECTORY32 PETls32;
	PIMAGE_TLS_DIRECTORY64 PETls64;
	PIMAGE_IMPORT_DESCRIPTOR ImportIID;
	PIMAGE_THUNK_DATA32 ThunkData32;
	PIMAGE_THUNK_DATA64 ThunkData64;
	PFILE_STATUS_INFO myFileStatusInfo = (PFILE_STATUS_INFO)FileStatusInfo;
	PFILE_FIX_INFO myFileFixInfo = (PFILE_FIX_INFO)FileFixInfo;
	bool hLoadedModuleSimulated = false;
	HMODULE hLoadedModule;
	ULONG_PTR ImportNamePtr;
	ULONG_PTR CurrentThunk;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	bool FileFixed = true;
	bool FeatureFixed = false;

	if(myFileStatusInfo == NULL){
		IsPE32FileValidEx(szFileName, UE_DEPTH_DEEP, FileStatusInfo);
	}
	if(myFileStatusInfo->OveralEvaluation == UE_RESULT_FILE_INVALID_BUT_FIXABLE){
		if(MapFileEx(szFileName, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
			myFileFixInfo->OveralEvaluation = UE_RESULT_FILE_INVALID_AND_NON_FIXABLE;
			DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
			if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
				PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
				PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
				if(PEHeader32->Signature == 0x4550 && PEHeader32->OptionalHeader.Magic == 0x10B){
					FileIs64 = false;
				}else if(PEHeader32->Signature == 0x4550 && PEHeader32->OptionalHeader.Magic == 0x20B){
					FileIs64 = true;
				}else{
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);
				}
				if(myFileStatusInfo->SignatureMZ != UE_FIELD_OK){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);
				}else if(myFileStatusInfo->SignaturePE != UE_FIELD_OK){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);
				}else if(myFileStatusInfo->SectionAlignment != UE_FIELD_OK){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);
				}else if(myFileStatusInfo->FileAlignment != UE_FIELD_OK){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);
				}else if(myFileStatusInfo->ImportTable != UE_FIELD_OK){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);
				}else if(myFileStatusInfo->ImportTableData != UE_FIELD_OK){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);
				}
				if(!FileIs64){
					/*
						x86 Surface check
					*/
					__try{
						if(PEHeader32->OptionalHeader.SizeOfImage % PEHeader32->OptionalHeader.SectionAlignment == NULL){
							CorrectedImageSize = (PEHeader32->OptionalHeader.SizeOfImage / PEHeader32->OptionalHeader.SectionAlignment) * PEHeader32->OptionalHeader.SectionAlignment;
						}else{
							CorrectedImageSize = ((PEHeader32->OptionalHeader.SizeOfImage / PEHeader32->OptionalHeader.SectionAlignment) + 1) * PEHeader32->OptionalHeader.SectionAlignment;				
						}
						/*
							Fixing import table
						*/
						if(myFileStatusInfo->MissingDeclaredAPIs){
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
							SectionAttributes = GetPE32SectionNumberFromVA(FileMapVA, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase);
							if(SectionAttributes >= NULL){
								SectionAttributes = (DWORD)GetPE32DataFromMappedFile(FileMapVA, SectionAttributes, UE_SECTIONFLAGS);
								if(SectionAttributes & IMAGE_SCN_MEM_EXECUTE || SectionAttributes & IMAGE_SCN_CNT_CODE || SectionAttributes & IMAGE_SCN_MEM_WRITE || SectionAttributes & IMAGE_SCN_CNT_INITIALIZED_DATA){
									// Should not execute!
								}else{
									if(!SetPE32DataForMappedFile(FileMapVA, SectionAttributes, UE_SECTIONFLAGS, 0xE0000020)){
										UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
										return(false);
									}
								}
								if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress != NULL){
									ImportIID = (PIMAGE_IMPORT_DESCRIPTOR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase, (ULONG_PTR)(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase), false, true);
									while(ImportIID->FirstThunk != NULL){
										hLoadedModule = NULL;
										ImportNamePtr = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase, (ULONG_PTR)((ULONG_PTR)ImportIID->Name + PEHeader32->OptionalHeader.ImageBase), false, true);
										if(ImportNamePtr != NULL){
											if(!EngineIsDependencyPresent((char*)ImportNamePtr, NULL, NULL)){
												hLoadedModuleSimulated = false;
											}else{
												hLoadedModuleSimulated = false;
												hLoadedModule = GetModuleHandleA((char*)ImportNamePtr);
												if(hLoadedModule == NULL){
													hLoadedModule = (HMODULE)EngineSimulateDllLoader(GetCurrentProcess(), (char*)ImportNamePtr);
													hLoadedModuleSimulated = true;
												}
											}
										}
										ThunkData32 = (PIMAGE_THUNK_DATA32)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase, (ULONG_PTR)((ULONG_PTR)ImportIID->FirstThunk + PEHeader32->OptionalHeader.ImageBase), false, true);
										if(ThunkData32 != NULL){
											CurrentThunk = (ULONG_PTR)ImportIID->FirstThunk;
											while(ThunkData32->u1.AddressOfData != NULL){
												if(ThunkData32->u1.Ordinal & IMAGE_ORDINAL_FLAG32){
													if((int)(ThunkData32->u1.Ordinal ^ IMAGE_ORDINAL_FLAG32) >= 0x10000){
														FileFixed = false;
													}
												}else{
													ImportNamePtr = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase, (ULONG_PTR)((ULONG_PTR)ThunkData32->u1.AddressOfData + 2 + PEHeader32->OptionalHeader.ImageBase), false, true);
													if(ImportNamePtr != NULL){
														if(EngineIsBadReadPtrEx((LPVOID)ImportNamePtr, 8)){
															if(hLoadedModule != NULL){
																if(EngineGetProcAddress((ULONG_PTR)hLoadedModule, (char*)ImportNamePtr) == NULL){
																	OrdinalBase = NULL;
																	OrdinalCount = NULL;
																	if(EngineGetLibraryOrdinalData((ULONG_PTR)hLoadedModule, &OrdinalBase, &OrdinalCount)){
																		if(OrdinalBase != NULL && OrdinalCount != NULL){
																			ThunkData32->u1.Ordinal = (OrdinalBase + 1) ^ IMAGE_ORDINAL_FLAG32;
																		}else{
																			FileFixed = false;
																		}
																	}
																}
															}
														}
													}														
												}
												CurrentThunk = CurrentThunk + 4;
												ThunkData32 = (PIMAGE_THUNK_DATA32)((ULONG_PTR)ThunkData32 + sizeof IMAGE_THUNK_DATA32);
											}
										}
										if(hLoadedModuleSimulated){
											VirtualFree((LPVOID)hLoadedModule, NULL, MEM_RELEASE);
										}
										ImportIID = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG_PTR)ImportIID + sizeof IMAGE_IMPORT_DESCRIPTOR);
									}
								}
							}
						}
						/*
							Fixing Export table
						*/
						if(myFileStatusInfo->ExportTable == UE_FIELD_NOT_PRESET_WARNING){
							FileFixed = false;
						}else if(myFileFixInfo->DontFixExports == false && myFileStatusInfo->ExportTable != UE_FIELD_OK && myFileStatusInfo->ExportTable != UE_FIELD_NOT_PRESET){
							if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXPORT && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress != NULL){
								if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size > CorrectedImageSize){
									myFileFixInfo->StrippedExports = true;
									myFileFixInfo->OriginalExportTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
									myFileFixInfo->OriginalExportTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = NULL;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = NULL;
								}else{
									FeatureFixed = true;
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress != NULL){
										if(EngineIsBadReadPtrEx((LPVOID)ConvertedAddress, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)){
											PEExports = (PIMAGE_EXPORT_DIRECTORY)ConvertedAddress;
											if(PEExports->AddressOfFunctions > CorrectedImageSize || PEExports->AddressOfFunctions + 4 * PEExports->NumberOfFunctions > CorrectedImageSize){
												FeatureFixed = false;
											}else if(PEExports->AddressOfNameOrdinals > CorrectedImageSize || PEExports->AddressOfNameOrdinals + 4 * PEExports->NumberOfNames > CorrectedImageSize){
												FeatureFixed = false;
											}else if(PEExports->AddressOfNames > CorrectedImageSize || PEExports->AddressOfNames + 4 * PEExports->NumberOfNames > CorrectedImageSize){
												FeatureFixed = false;
											}else if(PEExports->Name > CorrectedImageSize){
												FeatureFixed = false;
											}
											if(!FeatureFixed){
												myFileFixInfo->StrippedExports = true;
												myFileFixInfo->OriginalExportTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
												myFileFixInfo->OriginalExportTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
												PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = NULL;
												PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = NULL;
											}
										}else{
											myFileFixInfo->StrippedExports = true;
											myFileFixInfo->OriginalExportTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
											myFileFixInfo->OriginalExportTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
											PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = NULL;
											PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = NULL;
										}
									}
								}
							}
						}
						/*
							Fixing Relocation table
						*/
						if(myFileStatusInfo->FileIsDLL == true && myFileStatusInfo->RelocationTable == UE_FIELD_BROKEN_NON_FIXABLE){
							FileFixed = false;
						}else if(myFileFixInfo->DontFixRelocations == false && myFileStatusInfo->RelocationTable != UE_FIELD_OK){
							if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > CorrectedImageSize){
								if(myFileStatusInfo->FileIsDLL){
									FileFixed = false;
								}else{
									myFileFixInfo->StrippedRelocation = true;
									myFileFixInfo->OriginalRelocationTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
									myFileFixInfo->OriginalRelocationTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = NULL;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = NULL;
								}
							}else{
								ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
								if(ConvertedAddress != NULL){
									if(EngineIsBadReadPtrEx((LPVOID)ConvertedAddress, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)){
										RtlMoveMemory(&ReadData, (LPVOID)ConvertedAddress, 4);
										RtlMoveMemory(&ReadSize, (LPVOID)(ConvertedAddress + 4), 4);
										while(ReadData != NULL){
											ReadSize = ReadSize - 8;
											ConvertedAddress = ConvertedAddress + 8;
											while(ReadSize > NULL){
												RtlMoveMemory(&ReadDataWORD, (LPVOID)ConvertedAddress, 2);
												if(ReadDataWORD > 0xCFFF){
													RtlZeroMemory((LPVOID)ConvertedAddress, 2);
												}
												ConvertedAddress = ConvertedAddress + 2;
												ReadSize = ReadSize - 2;
											}
											RtlMoveMemory(&ReadData, (LPVOID)ConvertedAddress, 4);
											RtlMoveMemory(&ReadSize, (LPVOID)(ConvertedAddress + 4), 4);
										}
									}else{
										if(myFileStatusInfo->FileIsDLL){
											FileFixed = false;
										}else{
											myFileFixInfo->StrippedRelocation = true;
											myFileFixInfo->OriginalRelocationTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
											myFileFixInfo->OriginalRelocationTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
											PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = NULL;
											PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = NULL;
										}
									}
								}else{
									if(myFileStatusInfo->FileIsDLL){
										FileFixed = false;
									}else{
										myFileFixInfo->StrippedRelocation = true;
										myFileFixInfo->OriginalRelocationTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
										myFileFixInfo->OriginalRelocationTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
										PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = NULL;
										PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = NULL;
									}
								}
							}
						}else if(myFileStatusInfo->RelocationTable == UE_FIELD_OK){
							// Filter case!
						}else{
							FileFixed = false;
						}
						/*
							Fixing Resource table
						*/
						if(myFileFixInfo->DontFixResources == false && myFileStatusInfo->ResourceData != UE_FIELD_OK && myFileStatusInfo->ResourceData != UE_FIELD_NOT_PRESET){
							myFileFixInfo->StrippedResources = true;
							myFileFixInfo->OriginalResourceTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
							myFileFixInfo->OriginalResourceTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
							PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = NULL;
							PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = NULL;
						}else if(myFileFixInfo->DontFixResources == false && myFileStatusInfo->ResourceTable != UE_FIELD_OK && myFileStatusInfo->ResourceTable != UE_FIELD_NOT_PRESET){
							if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_RESOURCE && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress != NULL){
								if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size > CorrectedImageSize){
									myFileFixInfo->StrippedResources = true;
									myFileFixInfo->OriginalResourceTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
									myFileFixInfo->OriginalResourceTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = NULL;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = NULL;
								}else{
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize || ConvertedAddress - FileMapVA + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size > FileSize){
										myFileFixInfo->StrippedResources = true;
										myFileFixInfo->OriginalResourceTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
										myFileFixInfo->OriginalResourceTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
										PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = NULL;
										PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = NULL;							
									}
								}
							}
						}
						/*
							Fixing TLS table
						*/
						if(myFileFixInfo->DontFixTLS == false && myFileStatusInfo->TLSTable != UE_FIELD_OK && myFileStatusInfo->TLSTable != UE_FIELD_NOT_PRESET){
							if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_TLS && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress != NULL){
								if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size > CorrectedImageSize){
									myFileFixInfo->StrippedTLS = true;
									myFileFixInfo->OriginalTLSTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
									myFileFixInfo->OriginalTLSTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = NULL;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = NULL;							
								}else{
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
										myFileFixInfo->StrippedTLS = true;
										myFileFixInfo->OriginalTLSTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
										myFileFixInfo->OriginalTLSTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size;
										PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = NULL;
										PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = NULL;	
									}else{
										FeatureFixed = true;
										PETls32 = (PIMAGE_TLS_DIRECTORY32)ConvertedAddress;
										if(PETls32->StartAddressOfRawData != NULL && (PETls32->StartAddressOfRawData < PEHeader32->OptionalHeader.ImageBase || PETls32->StartAddressOfRawData > CorrectedImageSize + PEHeader32->OptionalHeader.ImageBase)){
											FeatureFixed = false;
										}else if(PETls32->EndAddressOfRawData != NULL && (PETls32->EndAddressOfRawData < PEHeader32->OptionalHeader.ImageBase || PETls32->EndAddressOfRawData > CorrectedImageSize + PEHeader32->OptionalHeader.ImageBase)){
											FeatureFixed = false;
										}else if(PETls32->AddressOfIndex != NULL && (PETls32->AddressOfIndex < PEHeader32->OptionalHeader.ImageBase || PETls32->AddressOfIndex > CorrectedImageSize + PEHeader32->OptionalHeader.ImageBase)){
											FeatureFixed = false;
										}else if(PETls32->AddressOfCallBacks != NULL && (PETls32->AddressOfCallBacks < PEHeader32->OptionalHeader.ImageBase || PETls32->AddressOfCallBacks > CorrectedImageSize + PEHeader32->OptionalHeader.ImageBase)){
											FeatureFixed = false;
										}
										if(!FeatureFixed){
											myFileFixInfo->StrippedTLS = true;
											myFileFixInfo->OriginalTLSTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
											myFileFixInfo->OriginalTLSTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size;
											PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = NULL;
											PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = NULL;	
										}else{
											if(PETls32->AddressOfCallBacks != NULL){
												ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PETls32->AddressOfCallBacks + PEHeader32->OptionalHeader.ImageBase, false, true);
												if(ConvertedAddress != NULL){
													while(ReadData != NULL){
														RtlMoveMemory(&ReadData, (LPVOID)ConvertedAddress, 4);
														if(ReadData < PEHeader32->OptionalHeader.ImageBase || ReadData > CorrectedImageSize + PEHeader32->OptionalHeader.ImageBase){
															RtlZeroMemory((LPVOID)ConvertedAddress, 4);
														}
														ConvertedAddress = ConvertedAddress + 4;
													}
												}
											}
										}
									}
								}
							}
						}
						/*
							Fix Load config table
						*/
						if(myFileFixInfo->DontFixLoadConfig == false && myFileStatusInfo->LoadConfigTable != UE_FIELD_OK && myFileStatusInfo->LoadConfigTable != UE_FIELD_NOT_PRESET){
							if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress != NULL){
								if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size > CorrectedImageSize){
									myFileFixInfo->StrippedLoadConfig = true;
									myFileFixInfo->OriginalLoadConfigTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress;
									myFileFixInfo->OriginalLoadConfigTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress = NULL;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size = NULL;	
								}else{
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
										myFileFixInfo->StrippedLoadConfig = true;
										myFileFixInfo->OriginalLoadConfigTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress;
										myFileFixInfo->OriginalLoadConfigTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size;
										PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress = NULL;
										PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size = NULL;
									}
								}
							}
						}
						/*
							Fix Bound import table
						*/
						if(myFileFixInfo->DontFixBoundImports == false && myFileStatusInfo->BoundImportTable != UE_FIELD_OK && myFileStatusInfo->BoundImportTable != UE_FIELD_NOT_PRESET){
							if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress != NULL){
								if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size > CorrectedImageSize){
									myFileFixInfo->StrippedBoundImports = true;
									myFileFixInfo->OriginalBoundImportTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress;
									myFileFixInfo->OriginalBoundImportTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = NULL;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = NULL;
								}else{
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
										myFileFixInfo->StrippedBoundImports = true;
										myFileFixInfo->OriginalBoundImportTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress;
										myFileFixInfo->OriginalBoundImportTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size;
										PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = NULL;
										PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = NULL;
									}
								}
							}
						}
						/*
							Fix IAT
						*/
						if(myFileFixInfo->DontFixIAT == false && myFileStatusInfo->IATTable != UE_FIELD_OK && myFileStatusInfo->IATTable != UE_FIELD_NOT_PRESET){
							if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IAT && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress != NULL){
								if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size > CorrectedImageSize){
									myFileFixInfo->StrippedIAT = true;
									myFileFixInfo->OriginalImportAddressTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress;
									myFileFixInfo->OriginalImportAddressTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = NULL;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = NULL;
								}else{
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
										myFileFixInfo->StrippedIAT = true;
										myFileFixInfo->OriginalImportAddressTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress;
										myFileFixInfo->OriginalImportAddressTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size;
										PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = NULL;
										PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = NULL;
									}
								}
							}
						}
						/*
							Fix COM header
						*/
						if(myFileFixInfo->DontFixCOM == false && myFileStatusInfo->COMHeaderTable != UE_FIELD_OK && myFileStatusInfo->COMHeaderTable != UE_FIELD_NOT_PRESET){
							if(PEHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR && PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress != NULL){
								if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress > CorrectedImageSize || PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size > CorrectedImageSize){
									myFileFixInfo->StrippedCOM = true;
									myFileFixInfo->OriginalCOMTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;
									myFileFixInfo->OriginalCOMTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = NULL;
									PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = NULL;
								}else{
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress + PEHeader32->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
										myFileFixInfo->StrippedCOM = true;
										myFileFixInfo->OriginalCOMTableAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;
										myFileFixInfo->OriginalCOMTableSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size;
										PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = NULL;
										PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = NULL;
									}
								}
							}
						}
						/*
							Fix sections and SizeOfImage
						*/
						if(myFileStatusInfo->SectionTable != UE_FIELD_OK || myFileStatusInfo->SizeOfImage != UE_FIELD_OK){
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
							NumberOfSections = PEHeader32->FileHeader.NumberOfSections;
							while(NumberOfSections > NULL){
								SectionVirtualSize = PESections->VirtualAddress + PESections->Misc.VirtualSize;
								if(PESections->Misc.VirtualSize % PEHeader32->OptionalHeader.SectionAlignment == NULL){
									SectionVirtualSizeFixed = SectionVirtualSize;
								}else{
									SectionVirtualSizeFixed = PESections->VirtualAddress + (((PESections->Misc.VirtualSize / PEHeader32->OptionalHeader.SectionAlignment) + 1) * PEHeader32->OptionalHeader.SectionAlignment);
								}
								if(NumberOfSections > 1){
									PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + sizeof IMAGE_SECTION_HEADER);
									if(SectionVirtualSize > PESections->VirtualAddress || SectionVirtualSizeFixed > PESections->VirtualAddress){
										PESections->Misc.VirtualSize = SectionVirtualSizeFixed;
									}
								}
								NumberOfSections--;
							}
							if(PESections->PointerToRawData + PESections->SizeOfRawData > FileSize && PESections->SizeOfRawData != NULL){
								PESections->SizeOfRawData = FileSize - PESections->PointerToRawData;
							}
							if(myFileStatusInfo->SizeOfImage != UE_FIELD_OK){
								SectionVirtualSizeFixed = SectionVirtualSizeFixed + 0xF000;
								if(PEHeader32->OptionalHeader.SizeOfImage > SectionVirtualSizeFixed){
									PEHeader32->OptionalHeader.SizeOfImage = SectionVirtualSizeFixed - 0xF000;
								}
							}
						}
						/*
							Entry point check
						*/
						if(myFileStatusInfo->EntryPoint != UE_FIELD_OK){
							SectionAttributes = GetPE32SectionNumberFromVA(FileMapVA, PEHeader32->OptionalHeader.AddressOfEntryPoint + (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase);
							if(SectionAttributes != -1){
								SectionAttributes = (DWORD)GetPE32DataFromMappedFile(FileMapVA, SectionAttributes, UE_SECTIONFLAGS);
								if(SectionAttributes & IMAGE_SCN_MEM_EXECUTE || SectionAttributes & IMAGE_SCN_CNT_CODE){
									// Should never execute
								}else{
									if(!SetPE32DataForMappedFile(FileMapVA, SectionAttributes, UE_SECTIONFLAGS, 0xE0000020)){
										UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
										return(false);
									}
								}
							}
						}
						/*
							Fix end
						*/
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						if(FileFixed){
							myFileFixInfo->OveralEvaluation = UE_RESULT_FILE_OK;
							myFileFixInfo->FileFixPerformed = FileFixed;
						}
						return(true);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						myFileFixInfo->FixingTerminatedByException = true;
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(false);
					}
				}else{
					/*
						x64 Surface check
					*/
					__try{
						if(PEHeader64->OptionalHeader.SizeOfImage % PEHeader64->OptionalHeader.SectionAlignment == NULL){
							CorrectedImageSize = (PEHeader64->OptionalHeader.SizeOfImage / PEHeader64->OptionalHeader.SectionAlignment) * PEHeader64->OptionalHeader.SectionAlignment;
						}else{
							CorrectedImageSize = ((PEHeader64->OptionalHeader.SizeOfImage / PEHeader64->OptionalHeader.SectionAlignment) + 1) * PEHeader64->OptionalHeader.SectionAlignment;				
						}
						/*
							Fixing import table
						*/
						if(myFileStatusInfo->MissingDeclaredAPIs){
							ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
							SectionAttributes = GetPE32SectionNumberFromVA(FileMapVA, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase);
							if(SectionAttributes >= NULL){
								SectionAttributes = (DWORD)GetPE32DataFromMappedFile(FileMapVA, SectionAttributes, UE_SECTIONFLAGS);
								if(SectionAttributes & IMAGE_SCN_MEM_EXECUTE || SectionAttributes & IMAGE_SCN_CNT_CODE || SectionAttributes & IMAGE_SCN_MEM_WRITE || SectionAttributes & IMAGE_SCN_CNT_INITIALIZED_DATA){
									// Should not execute!
								}else{
									if(!SetPE32DataForMappedFile(FileMapVA, SectionAttributes, UE_SECTIONFLAGS, 0xE0000020)){
										UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
										return(false);
									}
								}
								if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress != NULL){
									ImportIID = (PIMAGE_IMPORT_DESCRIPTOR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, (ULONG_PTR)(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase), false, true);
									while(ImportIID->FirstThunk != NULL){
										hLoadedModule = NULL;
										ImportNamePtr = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, (ULONG_PTR)((ULONG_PTR)ImportIID->Name + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase), false, true);
										if(ImportNamePtr != NULL){
											if(!EngineIsDependencyPresent((char*)ImportNamePtr, NULL, NULL)){
												hLoadedModuleSimulated = false;
											}else{
												hLoadedModuleSimulated = false;
												hLoadedModule = GetModuleHandleA((char*)ImportNamePtr);
												if(hLoadedModule == NULL){
													hLoadedModule = (HMODULE)EngineSimulateDllLoader(GetCurrentProcess(), (char*)ImportNamePtr);
													hLoadedModuleSimulated = true;
												}
											}
										}
										ThunkData64 = (PIMAGE_THUNK_DATA64)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, (ULONG_PTR)((ULONG_PTR)ImportIID->FirstThunk + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase), false, true);
										if(ThunkData64 != NULL){
											CurrentThunk = (ULONG_PTR)ImportIID->FirstThunk;
											while(ThunkData64->u1.AddressOfData != NULL){
												if(ThunkData64->u1.Ordinal & IMAGE_ORDINAL_FLAG64){
													if((int)(ThunkData64->u1.Ordinal ^ IMAGE_ORDINAL_FLAG64) >= 0x10000){
														FileFixed = false;
													}
												}else{
													ImportNamePtr = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, (ULONG_PTR)((ULONG_PTR)ThunkData64->u1.AddressOfData + 2 + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase), false, true);
													if(ImportNamePtr != NULL){
														if(EngineIsBadReadPtrEx((LPVOID)ImportNamePtr, 8)){
															if(hLoadedModule != NULL){
																if(EngineGetProcAddress((ULONG_PTR)hLoadedModule, (char*)ImportNamePtr) == NULL){
																	OrdinalBase = NULL;
																	OrdinalCount = NULL;
																	if(EngineGetLibraryOrdinalData((ULONG_PTR)hLoadedModule, &OrdinalBase, &OrdinalCount)){
																		if(OrdinalBase != NULL && OrdinalCount != NULL){
																			ThunkData64->u1.Ordinal = (OrdinalBase + 1) ^ IMAGE_ORDINAL_FLAG64;
																		}else{
																			FileFixed = false;
																		}
																	}
																}
															}
														}
													}														
												}
												CurrentThunk = CurrentThunk + 8;
												ThunkData64 = (PIMAGE_THUNK_DATA64)((ULONG_PTR)ThunkData64 + sizeof IMAGE_THUNK_DATA64);
											}
										}
										if(hLoadedModuleSimulated){
											VirtualFree((LPVOID)hLoadedModule, NULL, MEM_RELEASE);
										}
										ImportIID = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG_PTR)ImportIID + sizeof IMAGE_IMPORT_DESCRIPTOR);
									}
								}
							}
						}
						/*
							Fixing Export table
						*/
						if(myFileStatusInfo->ExportTable == UE_FIELD_NOT_PRESET_WARNING){
							FileFixed = false;
						}else if(myFileFixInfo->DontFixExports == false && myFileStatusInfo->ExportTable != UE_FIELD_OK && myFileStatusInfo->ExportTable != UE_FIELD_NOT_PRESET){
							if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXPORT && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress != NULL){
								if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size > CorrectedImageSize){
									myFileFixInfo->StrippedExports = true;
									myFileFixInfo->OriginalExportTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
									myFileFixInfo->OriginalExportTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = NULL;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = NULL;
								}else{
									FeatureFixed = true;
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress != NULL){
										if(EngineIsBadReadPtrEx((LPVOID)ConvertedAddress, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)){
											PEExports = (PIMAGE_EXPORT_DIRECTORY)ConvertedAddress;
											if(PEExports->AddressOfFunctions > CorrectedImageSize || PEExports->AddressOfFunctions + 4 * PEExports->NumberOfFunctions > CorrectedImageSize){
												FeatureFixed = false;
											}else if(PEExports->AddressOfNameOrdinals > CorrectedImageSize || PEExports->AddressOfNameOrdinals + 4 * PEExports->NumberOfNames > CorrectedImageSize){
												FeatureFixed = false;
											}else if(PEExports->AddressOfNames > CorrectedImageSize || PEExports->AddressOfNames + 4 * PEExports->NumberOfNames > CorrectedImageSize){
												FeatureFixed = false;
											}else if(PEExports->Name > CorrectedImageSize){
												FeatureFixed = false;
											}
											if(!FeatureFixed){
												myFileFixInfo->StrippedExports = true;
												myFileFixInfo->OriginalExportTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
												myFileFixInfo->OriginalExportTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
												PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = NULL;
												PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = NULL;
											}
										}else{
											myFileFixInfo->StrippedExports = true;
											myFileFixInfo->OriginalExportTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
											myFileFixInfo->OriginalExportTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
											PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = NULL;
											PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = NULL;
										}
									}
								}
							}
						}
						/*
							Fixing Relocation table
						*/
						if(myFileStatusInfo->FileIsDLL == true && myFileStatusInfo->RelocationTable == UE_FIELD_BROKEN_NON_FIXABLE){
							FileFixed = false;
						}else if(myFileFixInfo->DontFixRelocations == false && myFileStatusInfo->RelocationTable != UE_FIELD_OK){
							if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > CorrectedImageSize){
								if(myFileStatusInfo->FileIsDLL){
									FileFixed = false;
								}else{
									myFileFixInfo->StrippedRelocation = true;
									myFileFixInfo->OriginalRelocationTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
									myFileFixInfo->OriginalRelocationTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = NULL;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = NULL;
								}
							}else{
								ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
								if(ConvertedAddress != NULL){
									if(EngineIsBadReadPtrEx((LPVOID)ConvertedAddress, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)){
										RtlMoveMemory(&ReadData, (LPVOID)ConvertedAddress, 4);
										RtlMoveMemory(&ReadSize, (LPVOID)(ConvertedAddress + 4), 4);
										while(ReadData != NULL){
											ReadSize = ReadSize - 8;
											ConvertedAddress = ConvertedAddress + 8;
											while(ReadSize > NULL){
												RtlMoveMemory(&ReadDataWORD, (LPVOID)ConvertedAddress, 2);
												if(ReadDataWORD > 0xCFFF){
													RtlZeroMemory((LPVOID)ConvertedAddress, 2);
												}
												ConvertedAddress = ConvertedAddress + 2;
												ReadSize = ReadSize - 2;
											}
											RtlMoveMemory(&ReadData, (LPVOID)ConvertedAddress, 4);
											RtlMoveMemory(&ReadSize, (LPVOID)(ConvertedAddress + 4), 4);
										}
									}else{
										if(myFileStatusInfo->FileIsDLL){
											FileFixed = false;
										}else{
											myFileFixInfo->StrippedRelocation = true;
											myFileFixInfo->OriginalRelocationTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
											myFileFixInfo->OriginalRelocationTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
											PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = NULL;
											PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = NULL;
										}
									}
								}else{
									if(myFileStatusInfo->FileIsDLL){
										FileFixed = false;
									}else{
										myFileFixInfo->StrippedRelocation = true;
										myFileFixInfo->OriginalRelocationTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
										myFileFixInfo->OriginalRelocationTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
										PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = NULL;
										PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = NULL;
									}
								}
							}
						}else if(myFileStatusInfo->RelocationTable == UE_FIELD_OK){
							// Filter case!
						}else{
							FileFixed = false;
						}
						/*
							Fixing Resource table
						*/
						if(myFileFixInfo->DontFixResources == false && myFileStatusInfo->ResourceData != UE_FIELD_OK && myFileStatusInfo->ResourceData != UE_FIELD_NOT_PRESET){
							myFileFixInfo->StrippedResources = true;
							myFileFixInfo->OriginalResourceTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
							myFileFixInfo->OriginalResourceTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
							PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = NULL;
							PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = NULL;
						}else if(myFileFixInfo->DontFixResources == false && myFileStatusInfo->ResourceTable != UE_FIELD_OK && myFileStatusInfo->ResourceTable != UE_FIELD_NOT_PRESET){
							if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_RESOURCE && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress != NULL){
								if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size > CorrectedImageSize){
									myFileFixInfo->StrippedResources = true;
									myFileFixInfo->OriginalResourceTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
									myFileFixInfo->OriginalResourceTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = NULL;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = NULL;
								}else{
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize || ConvertedAddress - FileMapVA + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size > FileSize){
										myFileFixInfo->StrippedResources = true;
										myFileFixInfo->OriginalResourceTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
										myFileFixInfo->OriginalResourceTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
										PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = NULL;
										PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = NULL;							
									}
								}
							}
						}
						/*
							Fixing TLS table
						*/
						if(myFileFixInfo->DontFixTLS == false && myFileStatusInfo->TLSTable != UE_FIELD_OK && myFileStatusInfo->TLSTable != UE_FIELD_NOT_PRESET){
							if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_TLS && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress != NULL){
								if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size > CorrectedImageSize){
									myFileFixInfo->StrippedTLS = true;
									myFileFixInfo->OriginalTLSTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
									myFileFixInfo->OriginalTLSTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = NULL;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = NULL;							
								}else{
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
										myFileFixInfo->StrippedTLS = true;
										myFileFixInfo->OriginalTLSTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
										myFileFixInfo->OriginalTLSTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size;
										PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = NULL;
										PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = NULL;	
									}else{
										FeatureFixed = true;
										PETls64 = (PIMAGE_TLS_DIRECTORY64)ConvertedAddress;
										if(PETls64->StartAddressOfRawData != NULL && (PETls64->StartAddressOfRawData < (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase || PETls64->StartAddressOfRawData > CorrectedImageSize + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase)){
											FeatureFixed = false;
										}else if(PETls64->EndAddressOfRawData != NULL && (PETls64->EndAddressOfRawData < (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase || PETls64->EndAddressOfRawData > CorrectedImageSize + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase)){
											FeatureFixed = false;
										}else if(PETls64->AddressOfIndex != NULL && (PETls64->AddressOfIndex < (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase || PETls64->AddressOfIndex > CorrectedImageSize + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase)){
											FeatureFixed = false;
										}else if(PETls64->AddressOfCallBacks != NULL && (PETls64->AddressOfCallBacks < (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase || PETls64->AddressOfCallBacks > CorrectedImageSize + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase)){
											FeatureFixed = false;
										}
										if(!FeatureFixed){
											myFileFixInfo->StrippedTLS = true;
											myFileFixInfo->OriginalTLSTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
											myFileFixInfo->OriginalTLSTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size;
											PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = NULL;
											PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = NULL;	
										}else{
											if(PETls64->AddressOfCallBacks != NULL){
												ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, (ULONG_PTR)PETls64->AddressOfCallBacks + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
												if(ConvertedAddress != NULL){
													while(ReadData != NULL){
														RtlMoveMemory(&ReadDataQWORD, (LPVOID)ConvertedAddress, 8);
														if(ReadDataQWORD < (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase || ReadDataQWORD > CorrectedImageSize + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase){
															RtlZeroMemory((LPVOID)ConvertedAddress, 8);
														}
														ConvertedAddress = ConvertedAddress + 8;
													}
												}
											}
										}
									}
								}
							}
						}
						/*
							Fix Load config table
						*/
						if(myFileFixInfo->DontFixLoadConfig == false && myFileStatusInfo->LoadConfigTable != UE_FIELD_OK && myFileStatusInfo->LoadConfigTable != UE_FIELD_NOT_PRESET){
							if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress != NULL){
								if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size > CorrectedImageSize){
									myFileFixInfo->StrippedLoadConfig = true;
									myFileFixInfo->OriginalLoadConfigTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress;
									myFileFixInfo->OriginalLoadConfigTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress = NULL;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size = NULL;	
								}else{
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
										myFileFixInfo->StrippedLoadConfig = true;
										myFileFixInfo->OriginalLoadConfigTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress;
										myFileFixInfo->OriginalLoadConfigTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size;
										PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress = NULL;
										PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size = NULL;
									}
								}
							}
						}
						/*
							Fix Bound import table
						*/
						if(myFileFixInfo->DontFixBoundImports == false && myFileStatusInfo->BoundImportTable != UE_FIELD_OK && myFileStatusInfo->BoundImportTable != UE_FIELD_NOT_PRESET){
							if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress != NULL){
								if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size > CorrectedImageSize){
									myFileFixInfo->StrippedBoundImports = true;
									myFileFixInfo->OriginalBoundImportTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress;
									myFileFixInfo->OriginalBoundImportTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = NULL;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = NULL;
								}else{
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
										myFileFixInfo->StrippedBoundImports = true;
										myFileFixInfo->OriginalBoundImportTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress;
										myFileFixInfo->OriginalBoundImportTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size;
										PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = NULL;
										PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = NULL;
									}
								}
							}
						}
						/*
							Fix IAT
						*/
						if(myFileFixInfo->DontFixIAT == false && myFileStatusInfo->IATTable != UE_FIELD_OK && myFileStatusInfo->IATTable != UE_FIELD_NOT_PRESET){
							if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_IAT && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress != NULL){
								if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size > CorrectedImageSize){
									myFileFixInfo->StrippedIAT = true;
									myFileFixInfo->OriginalImportAddressTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress;
									myFileFixInfo->OriginalImportAddressTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = NULL;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = NULL;
								}else{
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
										myFileFixInfo->StrippedIAT = true;
										myFileFixInfo->OriginalImportAddressTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress;
										myFileFixInfo->OriginalImportAddressTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size;
										PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = NULL;
										PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = NULL;
									}
								}
							}
						}
						/*
							Fix COM header
						*/
						if(myFileFixInfo->DontFixCOM == false && myFileStatusInfo->COMHeaderTable != UE_FIELD_OK && myFileStatusInfo->COMHeaderTable != UE_FIELD_NOT_PRESET){
							if(PEHeader64->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR && PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress != NULL){
								if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress > CorrectedImageSize || PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size > CorrectedImageSize){
									myFileFixInfo->StrippedCOM = true;
									myFileFixInfo->OriginalCOMTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;
									myFileFixInfo->OriginalCOMTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = NULL;
									PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = NULL;
								}else{
									ConvertedAddress = (ULONG_PTR)ConvertVAtoFileOffsetEx(FileMapVA, FileSize, NULL, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, false, true);
									if(ConvertedAddress == NULL || ConvertedAddress - FileMapVA > FileSize){
										myFileFixInfo->StrippedCOM = true;
										myFileFixInfo->OriginalCOMTableAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;
										myFileFixInfo->OriginalCOMTableSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size;
										PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress = NULL;
										PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size = NULL;
									}
								}
							}
						}
						/*
							Fix sections and SizeOfImage
						*/
						if(myFileStatusInfo->SectionTable != UE_FIELD_OK || myFileStatusInfo->SizeOfImage != UE_FIELD_OK){
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
							NumberOfSections = PEHeader64->FileHeader.NumberOfSections;
							while(NumberOfSections > NULL){
								SectionVirtualSize = PESections->VirtualAddress + PESections->Misc.VirtualSize;
								if(PESections->Misc.VirtualSize % PEHeader64->OptionalHeader.SectionAlignment == NULL){
									SectionVirtualSizeFixed = SectionVirtualSize;
								}else{
									SectionVirtualSizeFixed = PESections->VirtualAddress + (((PESections->Misc.VirtualSize / PEHeader64->OptionalHeader.SectionAlignment) + 1) * PEHeader64->OptionalHeader.SectionAlignment);
								}
								if(NumberOfSections > 1){
									PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + sizeof IMAGE_SECTION_HEADER);
									if(SectionVirtualSize > PESections->VirtualAddress || SectionVirtualSizeFixed > PESections->VirtualAddress){
										PESections->Misc.VirtualSize = SectionVirtualSizeFixed;
									}
								}
								NumberOfSections--;
							}
							if(PESections->PointerToRawData + PESections->SizeOfRawData > FileSize && PESections->SizeOfRawData != NULL){
								PESections->SizeOfRawData = FileSize - PESections->PointerToRawData;
							}
							if(myFileStatusInfo->SizeOfImage != UE_FIELD_OK){
								SectionVirtualSizeFixed = SectionVirtualSizeFixed + 0xF000;
								if(PEHeader64->OptionalHeader.SizeOfImage > SectionVirtualSizeFixed){
									PEHeader64->OptionalHeader.SizeOfImage = SectionVirtualSizeFixed - 0xF000;
								}
							}
						}
						/*
							Entry point check
						*/
						if(myFileStatusInfo->EntryPoint != UE_FIELD_OK){
							SectionAttributes = GetPE32SectionNumberFromVA(FileMapVA, PEHeader64->OptionalHeader.AddressOfEntryPoint + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase);
							if(SectionAttributes != -1){
								SectionAttributes = (DWORD)GetPE32DataFromMappedFile(FileMapVA, SectionAttributes, UE_SECTIONFLAGS);
								if(SectionAttributes & IMAGE_SCN_MEM_EXECUTE || SectionAttributes & IMAGE_SCN_CNT_CODE){
									// Should never execute
								}else{
									if(!SetPE32DataForMappedFile(FileMapVA, SectionAttributes, UE_SECTIONFLAGS, 0xE0000020)){
										UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
										return(false);
									}
								}
							}
						}
						/*
							Fix end
						*/
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						if(FileFixed){
							myFileFixInfo->OveralEvaluation = UE_RESULT_FILE_OK;
							myFileFixInfo->FileFixPerformed = FileFixed;
						}
						return(true);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						myFileFixInfo->FixingTerminatedByException = true;
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(false);
					}
				}
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(false);
			}
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall IsFileDLL(char* szFileName, ULONG_PTR FileMapVA){
	
	if(szFileName != NULL){
		if((DWORD)GetPE32Data(szFileName, NULL, UE_CHARACTERISTICS) & 0x2000){
			return(true);
		}
	}else if(FileMapVA != NULL){
		if((DWORD)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_CHARACTERISTICS) & 0x2000){
			return(true);
		}
	}
	return(false);
}
// UnpackEngine.Hider.functions:
__declspec(dllexport) bool __stdcall HideDebugger(HANDLE hThread, HANDLE hProcess, DWORD PatchAPILevel){

	CONTEXT myDBGContext;
	LDT_ENTRY threadSelector;
	DWORD WriteDummy = NULL;
	DWORD AddressOfFS = NULL;
	DWORD AddressOfPEB = NULL;
	DWORD AddressOfProcessHeap = NULL;
	ULONG_PTR ueNumberOfBytesRead = NULL;
	BYTE patchCheckRemoteDebuggerPresent[5] = {0x33, 0xC0, 0xC2, 0x08, 0x00};
	BYTE patchGetTickCount[3] = {0x33, 0xC0, 0xC3};
	MEMORY_BASIC_INFORMATION MemInfo; 
	ULONG_PTR APIPatchAddress = NULL;
	DWORD OldProtect;

	if(engineCurrentPlatform == UE_PLATFORM_x86 && hThread != NULL && hProcess != NULL){
		RtlZeroMemory(&myDBGContext, sizeof CONTEXT);
		myDBGContext.ContextFlags = CONTEXT_SEGMENTS;
		GetThreadContext(hThread, &myDBGContext);
		GetThreadSelectorEntry(hThread, myDBGContext.SegFs, &threadSelector);
		AddressOfFS = (threadSelector.HighWord.Bytes.BaseHi << 24) + (threadSelector.HighWord.Bytes.BaseMid << 16) + threadSelector.BaseLow;
		if(ReadProcessMemory(hProcess, (LPVOID)(AddressOfFS + 0x30), &AddressOfPEB, 4, &ueNumberOfBytesRead)){
			if(WriteProcessMemory(hProcess, (LPVOID)(AddressOfPEB + 2), &WriteDummy, 2, &ueNumberOfBytesRead)){
				WriteProcessMemory(hProcess, (LPVOID)(AddressOfPEB + 0x68), &WriteDummy, 4, &ueNumberOfBytesRead);
				ReadProcessMemory(hProcess, (LPVOID)(AddressOfPEB + 0x18), &AddressOfProcessHeap, 4, &ueNumberOfBytesRead);
				WriteProcessMemory(hProcess, (LPVOID)(AddressOfProcessHeap + 0x10), &WriteDummy, 4, &ueNumberOfBytesRead);
				if(PatchAPILevel >= 1){
					APIPatchAddress = (ULONG_PTR)EngineGlobalAPIHandler(hProcess, NULL, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"),"CheckRemoteDebuggerPresent"), NULL, UE_OPTION_IMPORTER_REALIGN_APIADDRESS);
					VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)APIPatchAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
					OldProtect = MemInfo.AllocationProtect;
					VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)APIPatchAddress, 5, PAGE_EXECUTE_READWRITE, &OldProtect);
					WriteProcessMemory(hProcess, (LPVOID)(APIPatchAddress), &patchCheckRemoteDebuggerPresent, 5, &ueNumberOfBytesRead);

					APIPatchAddress = (ULONG_PTR)EngineGlobalAPIHandler(hProcess, NULL, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"),"GetTickCount"), NULL, UE_OPTION_IMPORTER_REALIGN_APIADDRESS);
					VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)APIPatchAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
					OldProtect = MemInfo.AllocationProtect;
					VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)APIPatchAddress, 3, PAGE_EXECUTE_READWRITE, &OldProtect);
					WriteProcessMemory(hProcess, (LPVOID)(APIPatchAddress), &patchGetTickCount, 3, &ueNumberOfBytesRead);
				}
				return(true);
			}else{
				return(false);
			}
		}else{
			return(false);
		}
	}else{
		return(false);
	}
	return(false);
}
// UnpackEngine.Relocater.functions:
__declspec(dllexport) void __stdcall RelocaterCleanup(){

	if(RelocationData != NULL){
		VirtualFree(RelocationData, NULL, MEM_RELEASE);
		RelocationLastPage = NULL;
		RelocationStartPosition = NULL;
		RelocationWritePosition = NULL;
		RelocationOldImageBase = NULL;
		RelocationNewImageBase = NULL;
	}
}
__declspec(dllexport) void __stdcall RelocaterInit(DWORD MemorySize, ULONG_PTR OldImageBase, ULONG_PTR NewImageBase){

	if(RelocationData != NULL){
		VirtualFree(RelocationData, NULL, MEM_RELEASE);
	}
	RelocationData = VirtualAlloc(NULL, MemorySize, MEM_COMMIT, PAGE_READWRITE);
	RelocationLastPage = NULL;
	RelocationStartPosition = RelocationData;
	RelocationWritePosition = (LPVOID)((ULONG_PTR)RelocationData + 8);
	RelocationOldImageBase = OldImageBase;
	RelocationNewImageBase = NewImageBase;
}
__declspec(dllexport) void __stdcall RelocaterAddNewRelocation(HANDLE hProcess, ULONG_PTR RelocateAddress, DWORD RelocateState){

	MEMORY_BASIC_INFORMATION MemInfo; 
	DWORD CompareDummy = NULL;
	DWORD CopyDummy = NULL;

	VirtualQueryEx(hProcess, (LPVOID)RelocateAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
	if(MemInfo.BaseAddress != RelocationLastPage || RelocationLastPage == NULL){
		RelocationLastPage = MemInfo.BaseAddress;
		if(memcmp(RelocationStartPosition, &CompareDummy, 4) == NULL){
			CopyDummy = (DWORD)((ULONG_PTR)MemInfo.BaseAddress - (ULONG_PTR)RelocationNewImageBase);
			RtlMoveMemory(RelocationStartPosition, &CopyDummy, 4);
		}else{
			CopyDummy = (DWORD)((ULONG_PTR)RelocationWritePosition - (ULONG_PTR)RelocationStartPosition);
			if(CopyDummy % 4 == NULL){
				RtlMoveMemory((LPVOID)((ULONG_PTR)RelocationStartPosition + 4), &CopyDummy, 4);
			}else{
				RelocationWritePosition = (LPVOID)((ULONG_PTR)RelocationWritePosition + 2);
				CopyDummy = (DWORD)((ULONG_PTR)RelocationWritePosition - (ULONG_PTR)RelocationStartPosition);
				if(CopyDummy % 4 == NULL){
					RtlMoveMemory((LPVOID)((ULONG_PTR)RelocationStartPosition + 4), &CopyDummy, 4);
				}else{
					RelocationWritePosition = (LPVOID)((ULONG_PTR)RelocationWritePosition + 2);
					CopyDummy = (DWORD)((ULONG_PTR)RelocationWritePosition - (ULONG_PTR)RelocationStartPosition);
					RtlMoveMemory((LPVOID)((ULONG_PTR)RelocationStartPosition + 4), &CopyDummy, 4);
				}
			}
			RelocationStartPosition = RelocationWritePosition;
			CopyDummy = (DWORD)((ULONG_PTR)RelocationLastPage - (ULONG_PTR)RelocationNewImageBase);
			RtlMoveMemory(RelocationWritePosition, &CopyDummy, 4);
			RelocationWritePosition = (LPVOID)((ULONG_PTR)RelocationWritePosition + 8);
		}
	}
	#if !defined(_WIN64)
		CopyDummy = (DWORD)((RelocateAddress - (ULONG_PTR)RelocationLastPage) ^ 0x3000);
	#else
		CopyDummy = (DWORD)((RelocateAddress - (ULONG_PTR)RelocationLastPage) ^ 0x8000);
	#endif
	RtlMoveMemory(RelocationWritePosition, &CopyDummy, 2);	
	RelocationWritePosition = (LPVOID)((ULONG_PTR)RelocationWritePosition + 2);
}
__declspec(dllexport) long __stdcall RelocaterEstimatedSize(){
	return((DWORD)((ULONG_PTR)RelocationWritePosition - (ULONG_PTR)RelocationData + 8));
}
__declspec(dllexport) bool __stdcall RelocaterExportRelocation(ULONG_PTR StorePlace, DWORD StorePlaceRVA, ULONG_PTR FileMapVA){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	BOOL FileIs64 = false;
	DWORD CopyDummy = NULL;

	__try{
		if((ULONG_PTR)RelocationStartPosition != -1){
			CopyDummy = (DWORD)((ULONG_PTR)RelocationWritePosition - (ULONG_PTR)RelocationStartPosition);
			if(CopyDummy % 4 == NULL){
				RtlMoveMemory((LPVOID)((ULONG_PTR)RelocationStartPosition + 4), &CopyDummy, 4);
			}else{
				RelocationWritePosition = (LPVOID)((ULONG_PTR)RelocationWritePosition + 2);
				CopyDummy = (DWORD)((ULONG_PTR)RelocationWritePosition - (ULONG_PTR)RelocationStartPosition);
				if(CopyDummy % 4 == NULL){
					RtlMoveMemory((LPVOID)((ULONG_PTR)RelocationStartPosition + 4), &CopyDummy, 4);
				}else{
					RelocationWritePosition = (LPVOID)((ULONG_PTR)RelocationWritePosition + 2);
					CopyDummy = (DWORD)((ULONG_PTR)RelocationWritePosition - (ULONG_PTR)RelocationStartPosition);
					RtlMoveMemory((LPVOID)((ULONG_PTR)RelocationStartPosition + 4), &CopyDummy, 4);
				}
			}
		}
		RtlMoveMemory((LPVOID)StorePlace, RelocationData, (DWORD)((ULONG_PTR)RelocationWritePosition - (ULONG_PTR)RelocationData));
		VirtualFree(RelocationData, NULL, MEM_RELEASE);
	}__except(EXCEPTION_EXECUTE_HANDLER){
		return(false);
	}

	DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
	if(EngineValidateHeader(FileMapVA, NULL, NULL, DOSHeader, true)){
		PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
		PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
		if(PEHeader32->OptionalHeader.Magic == 0x10B){
			FileIs64 = false;
		}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
			FileIs64 = true;
		}else{
			RelocationData = NULL;
			return(false);
		}
		if(!FileIs64){
			PEHeader32->OptionalHeader.ImageBase = (DWORD)RelocationNewImageBase;
			PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = StorePlaceRVA;
			PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = (DWORD)((ULONG_PTR)RelocationWritePosition - (ULONG_PTR)RelocationData);
		}else{
			PEHeader64->OptionalHeader.ImageBase = (ULONG_PTR)RelocationNewImageBase;
			PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = StorePlaceRVA;
			PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = (DWORD)((ULONG_PTR)RelocationWritePosition - (ULONG_PTR)RelocationData);
		}
		RelocationData = NULL;
		return(true);
	}
	RelocationData = NULL;
	return(false);
}
__declspec(dllexport) bool __stdcall RelocaterExportRelocationEx(char* szFileName, char* szSectionName){
	
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	DWORD NewSectionVO = NULL;
	DWORD NewSectionFO = NULL;
	bool ReturnValue = false;
	
	if(RelocaterEstimatedSize() > NULL){
		NewSectionVO = AddNewSection(szFileName, szSectionName, RelocaterEstimatedSize());
		if(MapFileEx(szFileName, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
			NewSectionFO = (DWORD)ConvertVAtoFileOffset(FileMapVA, NewSectionVO + (ULONG_PTR)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_IMAGEBASE), true);
			ReturnValue = RelocaterExportRelocation(NewSectionFO, NewSectionVO, FileMapVA);
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			if(ReturnValue){
				return(true);
			}else{
				return(false);
			}
		}else{
			return(false);
		}
	}else{
		return(false);
	}
}
__declspec(dllexport) bool __stdcall RelocaterGrabRelocationTable(HANDLE hProcess, ULONG_PTR MemoryStart, DWORD MemorySize){

	MEMORY_BASIC_INFORMATION MemInfo; 
	ULONG_PTR ueNumberOfBytesRead = NULL;
	DWORD OldProtect;

	if(RelocationData != NULL){
		VirtualQueryEx(hProcess, (LPVOID)MemoryStart, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		OldProtect = MemInfo.AllocationProtect;
		VirtualProtectEx(hProcess, (LPVOID)MemoryStart, MemorySize, PAGE_EXECUTE_READWRITE, &OldProtect);
		if(ReadProcessMemory(hProcess, (LPVOID)MemoryStart, RelocationData, MemorySize, &ueNumberOfBytesRead)){
			RelocationWritePosition = (LPVOID)((ULONG_PTR)RelocationData + MemorySize);
			RelocationStartPosition = (LPVOID)(-1);
			return(true);
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall RelocaterGrabRelocationTableEx(HANDLE hProcess, ULONG_PTR MemoryStart, ULONG_PTR MemorySize, DWORD NtSizeOfImage){

	MEMORY_BASIC_INFORMATION MemInfo; 
	LPVOID ReadMemoryStorage = NULL;
	LPVOID mReadMemoryStorage = NULL;
	ULONG_PTR ueNumberOfBytesRead = NULL;
	DWORD CompareDummy = NULL;
	DWORD RelocationBase = NULL;
	DWORD RelocationSize = NULL;
	DWORD OldProtect;

	if(RelocationData != NULL){
		VirtualQueryEx(hProcess, (LPVOID)MemoryStart, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		OldProtect = MemInfo.AllocationProtect;
		VirtualQueryEx(hProcess, (LPVOID)MemInfo.AllocationBase, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		if(MemInfo.RegionSize < MemorySize || MemorySize == NULL){
			MemorySize = MemInfo.RegionSize;
		}
		VirtualProtectEx(hProcess, (LPVOID)MemoryStart, MemorySize, PAGE_EXECUTE_READWRITE, &OldProtect);
		ReadMemoryStorage = VirtualAlloc(NULL, MemorySize, MEM_COMMIT, PAGE_READWRITE);
		mReadMemoryStorage = ReadMemoryStorage;
		if(ReadProcessMemory(hProcess, (LPVOID)MemoryStart, ReadMemoryStorage, MemorySize, &ueNumberOfBytesRead)){
			RtlMoveMemory(&RelocationBase, ReadMemoryStorage, 4);
			RtlMoveMemory(&RelocationSize, (LPVOID)((ULONG_PTR)ReadMemoryStorage + 4), 4);
			while(memcmp(ReadMemoryStorage, &CompareDummy, 4) == NULL && RelocationBase < NtSizeOfImage && RelocationSize < 0x2000){
				ReadMemoryStorage = (LPVOID)((ULONG_PTR)ReadMemoryStorage + RelocationSize);
				RtlMoveMemory(&RelocationBase, ReadMemoryStorage, 4);
				RtlMoveMemory(&RelocationSize, (LPVOID)((ULONG_PTR)ReadMemoryStorage + 4), 4);
			}
			VirtualFree(ReadMemoryStorage, NULL, MEM_RELEASE);
			return(RelocaterGrabRelocationTable(hProcess, MemoryStart, (DWORD)((ULONG_PTR)ReadMemoryStorage - (ULONG_PTR)mReadMemoryStorage)));
		}else{
			VirtualFree(ReadMemoryStorage, NULL, MEM_RELEASE);
			return(false);
		}
	}
	return(false);
}

__declspec(dllexport) bool __stdcall RelocaterMakeSnapshot(HANDLE hProcess, char* szSaveFileName, LPVOID MemoryStart, ULONG_PTR MemorySize){
	return(DumpMemory(hProcess, MemoryStart, MemorySize, szSaveFileName));
}
__declspec(dllexport) bool __stdcall RelocaterCompareTwoSnapshots(HANDLE hProcess, ULONG_PTR LoadedImageBase, ULONG_PTR NtSizeOfImage, char* szDumpFile1, char* szDumpFile2, ULONG_PTR MemStart){

	int i = NULL;
	int RelativeBase = NULL;
	ULONG_PTR ReadData = NULL;
	HANDLE FileHandle1;
	DWORD FileSize1;
	HANDLE FileMap1;
	DWORD FileMapVA1;
	HANDLE FileHandle2;
	DWORD FileSize2;
	HANDLE FileMap2;
	DWORD FileMapVA2;
	DWORD SearchSize;
	LPVOID Search1;
	LPVOID Search2;
	DWORD bkSearchSize;
	LPVOID bkSearch1;
	LPVOID bkSearch2;

	if(MapFileEx(szDumpFile1, UE_ACCESS_READ, &FileHandle1, &FileSize1, &FileMap1, &FileMapVA1, NULL)){
		if(MapFileEx(szDumpFile2, UE_ACCESS_READ, &FileHandle2, &FileSize2, &FileMap2, &FileMapVA2, NULL)){
			__try{
				Search1 = (LPVOID)FileMapVA1;
				Search2 = (LPVOID)FileMapVA2;
				NtSizeOfImage = NtSizeOfImage + LoadedImageBase;
				SearchSize = FileSize2;
				SearchSize--;
				while((int)SearchSize > NULL){
					if(memcmp(Search1, Search2, 1) != 0){
						bkSearch1 = Search1;
						bkSearch2 = Search2;
						bkSearchSize = SearchSize;
						i = sizeof HANDLE;
						RelativeBase = NULL;
						Search1 = (LPVOID)((ULONG_PTR)Search1 - 1);
						Search2 = (LPVOID)((ULONG_PTR)Search2 - 1);
						SearchSize = SearchSize + 1;
						while(i > NULL && RelativeBase == NULL){
							RtlMoveMemory(&ReadData, Search2, sizeof HANDLE);
							if(ReadData >= LoadedImageBase && ReadData <= NtSizeOfImage){
								RelativeBase++;
							}else{
								Search1 = (LPVOID)((ULONG_PTR)Search1 - 1);
								Search2 = (LPVOID)((ULONG_PTR)Search2 - 1);
								SearchSize = SearchSize + 1;
								i--;
							}
						}
						if(RelativeBase == NULL){
							Search1 = bkSearch1;
							Search2 = bkSearch2;
							SearchSize = bkSearchSize;
						}else{
							RelocaterAddNewRelocation(hProcess, MemStart + ((ULONG_PTR)Search2 - (ULONG_PTR)FileMapVA2), NULL);
							Search1 = (LPVOID)((ULONG_PTR)Search1 + sizeof HANDLE - 1);
							Search2 = (LPVOID)((ULONG_PTR)Search2 + sizeof HANDLE - 1);
							SearchSize = SearchSize - sizeof HANDLE + 1;
						}
					}
					Search1 = (LPVOID)((ULONG_PTR)Search1 + 1);
					Search2 = (LPVOID)((ULONG_PTR)Search2 + 1);
					SearchSize = SearchSize - 1;
				}
			}__except(EXCEPTION_EXECUTE_HANDLER){
				RelocaterCleanup();
				UnMapFileEx(FileHandle2, FileSize2, FileMap2, FileMapVA2);
				UnMapFileEx(FileHandle1, FileSize1, FileMap1, FileMapVA1);
				return(false);
			}
			UnMapFileEx(FileHandle2, FileSize2, FileMap2, FileMapVA2);
		}
		UnMapFileEx(FileHandle1, FileSize1, FileMap1, FileMapVA1);
		return(true);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall RelocaterChangeFileBase(char* szFileName, ULONG_PTR NewImageBase){

	DWORD RelocSize;
	ULONG_PTR RelocData;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	DWORD CompareDummy = NULL;
	DWORD RelocDelta = NULL;
	DWORD RelocDeltaSize = NULL;
	WORD RelocAddressData = NULL;
	ULONG_PTR RelocWriteAddress = NULL;
	ULONG_PTR RelocWriteData = NULL;
	DWORD64 RelocWriteData64 = NULL;
	char szBackupFile[512] = {};
	char szBackupItem[512] = {};

	if(engineBackupForCriticalFunctions && CreateGarbageItem(&szBackupItem, 512)){
		if(!FillGarbageItem(szBackupItem, szFileName, &szBackupFile, 512)){
			RtlZeroMemory(&szBackupItem, 512);
			lstrcpyA(szBackupFile, szFileName);
		}
	}else{
		RtlZeroMemory(&szBackupItem, 512);
		lstrcpyA(szBackupFile, szFileName);
	}
	if(MapFileEx(szBackupFile, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				RemoveGarbageItem(szBackupItem, true);
				return(false);
			}
			if(!FileIs64){
				if(PEHeader32->OptionalHeader.ImageBase == (DWORD)NewImageBase){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					RemoveGarbageItem(szBackupItem, true);
					return(true);				
				}
				RelocData = (ULONG_PTR)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + PEHeader32->OptionalHeader.ImageBase), true);
				RelocSize = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
			}else{
				if((ULONG_PTR)PEHeader64->OptionalHeader.ImageBase == NewImageBase){
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					RemoveGarbageItem(szBackupItem, true);
					return(true);				
				}
				RelocData = (ULONG_PTR)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + PEHeader64->OptionalHeader.ImageBase), true);
				RelocSize = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
			}
			__try{
				while(memcmp((LPVOID)RelocData, &CompareDummy, 4)){
					RtlMoveMemory(&RelocDelta, (LPVOID)RelocData, 4);
					RtlMoveMemory(&RelocDeltaSize, (LPVOID)((ULONG_PTR)RelocData + 4), 4);
					RelocDeltaSize = RelocDeltaSize - 8;
					RelocData = RelocData + 8;
					while(RelocDeltaSize > NULL){
						RtlMoveMemory(&RelocAddressData, (LPVOID)RelocData, 2);
						if(RelocAddressData != NULL){
							if(RelocAddressData & 0x8000){
								RelocAddressData = RelocAddressData ^ 0x8000;
								RelocWriteAddress = (ULONG_PTR)(RelocAddressData + RelocDelta);
								RelocWriteAddress = (ULONG_PTR)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)((DWORD64)PEHeader64->OptionalHeader.ImageBase + RelocWriteAddress), true);
								RtlMoveMemory(&RelocWriteData64, (LPVOID)RelocWriteAddress, 8);
								RelocWriteData64 = RelocWriteData64 - (DWORD64)PEHeader64->OptionalHeader.ImageBase + (DWORD64)NewImageBase;
								RtlMoveMemory((LPVOID)RelocWriteAddress, &RelocWriteData64, 8);
							}else if(RelocAddressData & 0x3000){
								RelocAddressData = RelocAddressData ^ 0x3000;
								RelocWriteAddress = (ULONG_PTR)(RelocAddressData + RelocDelta);
								RelocWriteAddress = (ULONG_PTR)ConvertVAtoFileOffset(FileMapVA, PEHeader32->OptionalHeader.ImageBase + RelocWriteAddress, true);
								RtlMoveMemory(&RelocWriteData, (LPVOID)RelocWriteAddress, 4);
								RelocWriteData = RelocWriteData - PEHeader32->OptionalHeader.ImageBase + NewImageBase;
								RtlMoveMemory((LPVOID)RelocWriteAddress, &RelocWriteData, 4);
							}
						}
						RelocDeltaSize = RelocDeltaSize - 2;
						RelocData = RelocData + 2;
					}
				}
				if(!FileIs64){
					PEHeader32->OptionalHeader.ImageBase = (DWORD)NewImageBase;
				}else{
					PEHeader64->OptionalHeader.ImageBase = (ULONG_PTR)NewImageBase;
				}
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				if(szBackupItem[0] != NULL){
					if(CopyFileA(szBackupFile, szFileName, false)){
						RemoveGarbageItem(szBackupItem, true);
						return(true);
					}else{
						RemoveGarbageItem(szBackupItem, true);
						return(false);
					}
				}else{
					return(true);
				}
			}__except(EXCEPTION_EXECUTE_HANDLER){
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				RemoveGarbageItem(szBackupItem, true);
				return(false);
			}
		}else{
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			RemoveGarbageItem(szBackupItem, true);
			return(false);
		}
	}
	RemoveGarbageItem(szBackupItem, true);
	return(false);
}
__declspec(dllexport) bool __stdcall RelocaterWipeRelocationTable(char* szFileName){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	DWORD WipeSectionNumber = NULL;
	ULONG_PTR Characteristics;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(false);
			}
			if(!FileIs64){
				if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != NULL){
					Characteristics = (ULONG_PTR)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_CHARACTERISTICS) ^ 1;
					SetPE32DataForMappedFile(FileMapVA, NULL, UE_CHARACTERISTICS, Characteristics);
					WipeSectionNumber = GetPE32SectionNumberFromVA(FileMapVA, (ULONG_PTR)((ULONG_PTR)PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase));
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(WipeSection(szFileName, (int)WipeSectionNumber, true));
				}
			}else{
				if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != NULL){
					Characteristics = (ULONG_PTR)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_CHARACTERISTICS) ^ 1;
					SetPE32DataForMappedFile(FileMapVA, NULL, UE_CHARACTERISTICS, Characteristics);
					WipeSectionNumber = GetPE32SectionNumberFromVA(FileMapVA, (ULONG_PTR)((ULONG_PTR)PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase));
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(WipeSection(szFileName, (int)WipeSectionNumber, true));
				}
			}
		}
	}
	return(false);
}
// UnpackEngine.Resourcer.functions:
__declspec(dllexport) long long __stdcall ResourcerLoadFileForResourceUse(char* szFileName){
	return((ULONG_PTR)EngineSimulateNtLoader(szFileName));
}
__declspec(dllexport) bool __stdcall ResourcerFreeLoadedFile(LPVOID LoadedFileBase){
	if(VirtualFree(LoadedFileBase, NULL, MEM_RELEASE)){
		return(true);
	}else{
		return(false);
	}
}
__declspec(dllexport) bool __stdcall ResourcerExtractResourceFromFileEx(ULONG_PTR FileMapVA, char* szResourceType, char* szResourceName, char* szExtractedFileName){

	HRSRC hResource;
	HGLOBAL hResourceGlobal;
	DWORD ResourceSize;
	LPVOID ResourceData;
	DWORD NumberOfBytesWritten;
	HANDLE hFile;

	hResource = FindResourceA((HMODULE)FileMapVA, (LPCSTR)szResourceName, (LPCSTR)szResourceType);
	if(hResource != NULL){
		hResourceGlobal = LoadResource((HMODULE)FileMapVA, hResource);
		if(hResourceGlobal != NULL){
			ResourceSize = SizeofResource((HMODULE)FileMapVA, hResource);
			ResourceData = LockResource(hResourceGlobal);
			hFile = CreateFileA(szExtractedFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hFile != INVALID_HANDLE_VALUE){
				WriteFile(hFile, ResourceData, ResourceSize, &NumberOfBytesWritten, NULL);
				EngineCloseHandle(hFile);
			}else{
				return(false);
			}
		}
		return(true);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall ResourcerExtractResourceFromFile(char* szFileName, char* szResourceType, char* szResourceName, char* szExtractedFileName){

	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	bool bReturn;

	if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		bReturn = ResourcerExtractResourceFromFileEx(FileMapVA, szResourceType, szResourceName, szExtractedFileName);
		UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
		if(bReturn){
			return(true);
		}
	}
	return(false);
}
// UnpackEngine.Threader.functions:
__declspec(dllexport) void* __stdcall ThreaderGetThreadInfo(HANDLE hThread, DWORD ThreadId){

	PTHREAD_ITEM_DATA hListThreadPtr = (PTHREAD_ITEM_DATA)hListThread;

	if(hListThreadPtr != NULL){
		if(hThread != NULL){
			while(hListThreadPtr->hThread != NULL && hListThreadPtr->hThread != hThread){
				hListThreadPtr = (PTHREAD_ITEM_DATA)((ULONG_PTR)hListThreadPtr + sizeof THREAD_ITEM_DATA);
			}
			if(hListThreadPtr->hThread == hThread){
				return((void*)hListThreadPtr);
			}
		}else if(ThreadId != NULL){
			while(hListThreadPtr->hThread != NULL && hListThreadPtr->dwThreadId != ThreadId){
				hListThreadPtr = (PTHREAD_ITEM_DATA)((ULONG_PTR)hListThreadPtr + sizeof THREAD_ITEM_DATA);
			}
			if(hListThreadPtr->dwThreadId == ThreadId){
				return((void*)hListThreadPtr);
			}
		}
	}
	return(NULL);
}
__declspec(dllexport) void __stdcall ThreaderEnumThreadInfo(void* EnumCallBack){

	PTHREAD_ITEM_DATA hListThreadPtr = (PTHREAD_ITEM_DATA)hListThread;
	typedef void(__stdcall *fEnumCallBack)(LPVOID fThreadDetail);
	fEnumCallBack myEnumCallBack = (fEnumCallBack)EnumCallBack;

	if(hListThreadPtr != NULL){
		while(EnumCallBack != NULL && hListThreadPtr->hThread != NULL){
			if(hListThreadPtr->hThread != NULL){
				__try{
					myEnumCallBack((void*)hListThreadPtr);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					EnumCallBack = NULL;
				}
			}
			hListThreadPtr = (PTHREAD_ITEM_DATA)((ULONG_PTR)hListThreadPtr + sizeof THREAD_ITEM_DATA);
		}
	}
}
__declspec(dllexport) bool __stdcall ThreaderPauseThread(HANDLE hThread){

	PTHREAD_ITEM_DATA hListThreadPtr = (PTHREAD_ITEM_DATA)hListThread;

	if(hListThreadPtr != NULL){
		if(hThread != NULL){
			while(hListThreadPtr->hThread != NULL && hListThreadPtr->hThread != hThread){
				hListThreadPtr = (PTHREAD_ITEM_DATA)((ULONG_PTR)hListThreadPtr + sizeof THREAD_ITEM_DATA);
			}
			if(hListThreadPtr->hThread == hThread){
				if(SuspendThread(hThread) != -1){
					return(true);
				}else{
					return(false);
				}
			}else{
				return(false);
			}
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall ThreaderResumeThread(HANDLE hThread){

	PTHREAD_ITEM_DATA hListThreadPtr = (PTHREAD_ITEM_DATA)hListThread;

	if(hListThreadPtr != NULL){
		if(hThread != NULL){
			while(hListThreadPtr->hThread != NULL && hListThreadPtr->hThread != hThread){
				hListThreadPtr = (PTHREAD_ITEM_DATA)((ULONG_PTR)hListThreadPtr + sizeof THREAD_ITEM_DATA);
			}
			if(hListThreadPtr->hThread == hThread){
				if(ResumeThread(hThread) != -1){
					return(true);
				}else{
					return(false);
				}
			}else{
				return(false);
			}
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall ThreaderTerminateThread(HANDLE hThread, DWORD ThreadExitCode){

	PTHREAD_ITEM_DATA hListThreadPtr = (PTHREAD_ITEM_DATA)hListThread;

	if(hListThreadPtr != NULL){
		if(hThread != NULL){
			while(hListThreadPtr->hThread != NULL && hListThreadPtr->hThread != hThread){
				hListThreadPtr = (PTHREAD_ITEM_DATA)((ULONG_PTR)hListThreadPtr + sizeof THREAD_ITEM_DATA);
			}
			if(hListThreadPtr->hThread == hThread){
				if(TerminateThread(hThread, ThreadExitCode) != NULL){
					hListThreadPtr->hThread = (HANDLE)-1;
					hListThreadPtr->dwThreadId = NULL;
					hListThreadPtr->ThreadLocalBase = NULL;
					hListThreadPtr->ThreadStartAddress = NULL;
					return(true);
				}else{
					return(false);
				}
			}else{
				return(false);
			}
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall ThreaderPauseAllThreads(bool LeaveMainRunning){

	PTHREAD_ITEM_DATA hListThreadPtr = (PTHREAD_ITEM_DATA)hListThread;

	if(hListThreadPtr != NULL){
		while(hListThreadPtr->hThread != NULL){
			if(LeaveMainRunning){
				if(hListThreadPtr->hThread != dbgProcessInformation.hThread){
					SuspendThread((HANDLE)hListThreadPtr->hThread);
				}
			}else{
				SuspendThread(hListThreadPtr->hThread);
			}
			hListThreadPtr = (PTHREAD_ITEM_DATA)((ULONG_PTR)hListThreadPtr + sizeof THREAD_ITEM_DATA);
		}
		if(!LeaveMainRunning){
			SuspendThread(dbgProcessInformation.hThread);
		}
		return(true);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall ThreaderResumeAllThreads(bool LeaveMainPaused){

	PTHREAD_ITEM_DATA hListThreadPtr = (PTHREAD_ITEM_DATA)hListThread;

	if(hListThreadPtr != NULL){
		while(hListThreadPtr->hThread != NULL){
			if(LeaveMainPaused){
				if(hListThreadPtr->hThread != dbgProcessInformation.hThread){
					ResumeThread(hListThreadPtr->hThread);
				}
			}else{
				ResumeThread(hListThreadPtr->hThread);
			}
			hListThreadPtr = (PTHREAD_ITEM_DATA)((ULONG_PTR)hListThreadPtr + sizeof THREAD_ITEM_DATA);
		}
		if(!LeaveMainPaused){
			ResumeThread(dbgProcessInformation.hThread);
		}
		return(true);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall ThreaderPauseProcess(){
	return(ThreaderPauseAllThreads(false));
}
__declspec(dllexport) bool __stdcall ThreaderResumeProcess(){
	return(ThreaderResumeAllThreads(false));
}
__declspec(dllexport) long long __stdcall ThreaderCreateRemoteThread(ULONG_PTR ThreadStartAddress, bool AutoCloseTheHandle, LPVOID ThreadPassParameter, LPDWORD ThreadId){
	
	HANDLE myThread;

	if(dbgProcessInformation.hProcess != NULL){
		if(!AutoCloseTheHandle){
			return((ULONG_PTR)CreateRemoteThread(dbgProcessInformation.hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)ThreadStartAddress, ThreadPassParameter, NULL, ThreadId));
		}else{
			myThread = CreateRemoteThread(dbgProcessInformation.hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)ThreadStartAddress, ThreadPassParameter, NULL, ThreadId);
			EngineCloseHandle(myThread);
			return(NULL);
		}
	}
	return(NULL);
}
__declspec(dllexport) bool __stdcall ThreaderInjectAndExecuteCode(LPVOID InjectCode, DWORD StartDelta, DWORD InjectSize){

	LPVOID ThreadBase = 0;
	ULONG_PTR ueNumberOfBytesRead = 0;

	if(dbgProcessInformation.hProcess != NULL){
		ThreadBase = VirtualAllocEx(dbgProcessInformation.hProcess, NULL, InjectSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if(WriteProcessMemory(dbgProcessInformation.hProcess, ThreadBase, InjectCode, InjectSize, &ueNumberOfBytesRead)){
			ThreaderCreateRemoteThread((ULONG_PTR)((ULONG_PTR)InjectCode + StartDelta), true, NULL, NULL);
			return(true);
		}else{
			return(false);
		}
	}
	return(false);
}
__declspec(dllexport) long long __stdcall ThreaderCreateRemoteThreadEx(HANDLE hProcess, ULONG_PTR ThreadStartAddress, bool AutoCloseTheHandle, LPVOID ThreadPassParameter, LPDWORD ThreadId){
	
	HANDLE myThread;

	if(hProcess != NULL){
		if(!AutoCloseTheHandle){
			return((ULONG_PTR)CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)ThreadStartAddress, ThreadPassParameter, NULL, ThreadId));
		}else{
			myThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)ThreadStartAddress, ThreadPassParameter, NULL, ThreadId);
			EngineCloseHandle(myThread);
			return(NULL);
		}
	}
	return(NULL);
}
__declspec(dllexport) bool __stdcall ThreaderInjectAndExecuteCodeEx(HANDLE hProcess, LPVOID InjectCode, DWORD StartDelta, DWORD InjectSize){

	LPVOID ThreadBase = 0;
	ULONG_PTR ueNumberOfBytesRead = 0;

	if(hProcess != NULL){
		ThreadBase = VirtualAllocEx(hProcess, NULL, InjectSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if(WriteProcessMemory(hProcess, ThreadBase, InjectCode, InjectSize, &ueNumberOfBytesRead)){
			ThreaderCreateRemoteThread((ULONG_PTR)((ULONG_PTR)InjectCode + StartDelta), true, NULL, NULL);
			return(true);
		}else{
			return(false);
		}
	}
	return(false);
}
__declspec(dllexport) void __stdcall ThreaderSetCallBackForNextExitThreadEvent(LPVOID exitThreadCallBack){
	engineExitThreadOneShootCallBack = exitThreadCallBack;
}
__declspec(dllexport) bool __stdcall ThreaderIsThreadStillRunning(HANDLE hThread){

	CONTEXT myDBGContext;

	RtlZeroMemory(&myDBGContext, sizeof CONTEXT);
	myDBGContext.ContextFlags = CONTEXT_ALL;
	if(GetThreadContext(hThread, &myDBGContext)){
		return(true);
	}else{
		return(false);
	}
}
__declspec(dllexport) bool __stdcall ThreaderIsThreadActive(HANDLE hThread){

	if(SuspendThread(hThread) < 0){
		ResumeThread(hThread);
		return(true);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall ThreaderIsAnyThreadActive(){

	PTHREAD_ITEM_DATA hListThreadPtr = (PTHREAD_ITEM_DATA)hListThread;

	if(hListThreadPtr != NULL){
		while(hListThreadPtr->hThread != NULL){
			if(hListThreadPtr->hThread != (HANDLE)-1){
				if(ThreaderIsThreadActive(hListThreadPtr->hThread)){
					return(true);
				}
			}
			hListThreadPtr = (PTHREAD_ITEM_DATA)((ULONG_PTR)hListThreadPtr + sizeof THREAD_ITEM_DATA);
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall ThreaderExecuteOnlyInjectedThreads(){

	if(ThreaderPauseProcess()){
		engineResumeProcessIfNoThreadIsActive = true;
		return(true);
	}
	return(false);
}
__declspec(dllexport) long long __stdcall ThreaderGetOpenHandleForThread(DWORD ThreadId){

	PTHREAD_ITEM_DATA hListThreadPtr = (PTHREAD_ITEM_DATA)hListThread;

	if(hListThread != NULL){
		while(hListThreadPtr->hThread != NULL){
			if(hListThreadPtr->hThread != (HANDLE)-1 && hListThreadPtr->dwThreadId == ThreadId){
				return((ULONG_PTR)hListThreadPtr->hThread);
			}
			hListThreadPtr = (PTHREAD_ITEM_DATA)((ULONG_PTR)hListThreadPtr + sizeof THREAD_ITEM_DATA);
		}
	}
	return(NULL);
}
__declspec(dllexport) void* __stdcall ThreaderGetThreadData(){
	return(hListThread);
}
__declspec(dllexport) bool __stdcall ThreaderIsExceptionInMainThread(){
	
	LPDEBUG_EVENT myDBGEvent;

	myDBGEvent = (LPDEBUG_EVENT)GetDebugData();
	if(myDBGEvent->dwThreadId == dbgProcessInformation.dwThreadId){
		return(true);
	}
	return(false);
}
// Global.Debugger.functions:
long DebugLoopInSecondThread(LPVOID InputParameter){
	__try{
		if(InputParameter == NULL){
			InitDebugEx(engineExpertDebug.szFileName, engineExpertDebug.szCommandLine, engineExpertDebug.szCurrentFolder, engineExpertDebug.EntryCallBack);
		}else{
			InitDLLDebug(engineExpertDebug.szFileName, engineExpertDebug.ReserveModuleBase, engineExpertDebug.szCommandLine, engineExpertDebug.szCurrentFolder, engineExpertDebug.EntryCallBack);
		}
		DebugLoop();
		return(NULL);
	}__except(EXCEPTION_EXECUTE_HANDLER){
		return(-1);
	}
}
// UnpackEngine.Debugger.functions:
__declspec(dllexport) void* __stdcall StaticDisassembleEx(ULONG_PTR DisassmStart, LPVOID DisassmAddress){
	
	_DecodeResult DecodingResult;
	_DecodedInst engineDecodedInstructions[MAX_INSTRUCTIONS];	
	unsigned int DecodedInstructionsCount = 0;
#if !defined(_WIN64)
	_DecodeType DecodingType = Decode32Bits;
#else
	_DecodeType DecodingType = Decode64Bits;
#endif
	MEMORY_BASIC_INFORMATION MemInfo;
	DWORD MaxDisassmSize;

	VirtualQueryEx(GetCurrentProcess(), DisassmAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
	if(MemInfo.State == MEM_COMMIT){
		if((ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)MemInfo.RegionSize - (ULONG_PTR)DisassmAddress <= MAXIMUM_INSTRUCTION_SIZE){
			MaxDisassmSize = (DWORD)((ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)MemInfo.RegionSize - (ULONG_PTR)DisassmAddress - 1);
			VirtualQueryEx(GetCurrentProcess(), (LPVOID)((ULONG_PTR)DisassmAddress + (ULONG_PTR)MemInfo.RegionSize), &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
			if(MemInfo.State == MEM_COMMIT){
				MaxDisassmSize = MAXIMUM_INSTRUCTION_SIZE;
			}
		}else{
			MaxDisassmSize = MAXIMUM_INSTRUCTION_SIZE;
		}
		DecodingResult = distorm_decode((ULONG_PTR)DisassmStart, (const unsigned char*)DisassmAddress, MaxDisassmSize, DecodingType, engineDecodedInstructions, MAX_INSTRUCTIONS, &DecodedInstructionsCount);
		RtlZeroMemory(&engineDisassembledInstruction, 128);
		lstrcpyA(engineDisassembledInstruction, (LPCSTR)engineDecodedInstructions[0].mnemonic.p);
		if(engineDecodedInstructions[0].size != NULL){
			lstrcatA(engineDisassembledInstruction, " ");
		}
		lstrcatA(engineDisassembledInstruction, (LPCSTR)engineDecodedInstructions[0].operands.p);
		return((char*)engineDisassembledInstruction);
	}else{
		return(NULL);
	}
}
__declspec(dllexport) void* __stdcall StaticDisassemble(LPVOID DisassmAddress){
	return(StaticDisassembleEx((ULONG_PTR)DisassmAddress, DisassmAddress));
}
__declspec(dllexport) void* __stdcall DisassembleEx(HANDLE hProcess, LPVOID DisassmAddress, bool ReturnInstructionType){
	
	_DecodeResult DecodingResult;
	_DecodedInst engineDecodedInstructions[MAX_INSTRUCTIONS];	
	unsigned int DecodedInstructionsCount = 0;
#if !defined(_WIN64)
	_DecodeType DecodingType = Decode32Bits;
#else
	_DecodeType DecodingType = Decode64Bits;
#endif
	ULONG_PTR ueNumberOfBytesRead = 0;
	LPVOID ueReadBuffer = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	MEMORY_BASIC_INFORMATION MemInfo;
	DWORD MaxDisassmSize;

	if(hProcess != NULL){
		VirtualQueryEx(hProcess, DisassmAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		if(MemInfo.State == MEM_COMMIT){
			if((ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)MemInfo.RegionSize - (ULONG_PTR)DisassmAddress <= MAXIMUM_INSTRUCTION_SIZE){
				MaxDisassmSize = (DWORD)((ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)MemInfo.RegionSize - (ULONG_PTR)DisassmAddress - 1);
				VirtualQueryEx(hProcess, (LPVOID)((ULONG_PTR)DisassmAddress + (ULONG_PTR)MemInfo.RegionSize), &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
				if(MemInfo.State == MEM_COMMIT){
					MaxDisassmSize = MAXIMUM_INSTRUCTION_SIZE;
				}
			}else{
				MaxDisassmSize = MAXIMUM_INSTRUCTION_SIZE;
			}
			if(ReadProcessMemory(hProcess, (LPVOID)DisassmAddress, ueReadBuffer, MaxDisassmSize, &ueNumberOfBytesRead)){
				DecodingResult = distorm_decode((ULONG_PTR)DisassmAddress, (const unsigned char*)ueReadBuffer, MaxDisassmSize, DecodingType, engineDecodedInstructions, MAX_INSTRUCTIONS, &DecodedInstructionsCount);
				VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
				RtlZeroMemory(&engineDisassembledInstruction, 128);
				lstrcpyA(engineDisassembledInstruction, (LPCSTR)engineDecodedInstructions[0].mnemonic.p);
				if(!ReturnInstructionType){
					if(engineDecodedInstructions[0].size != NULL){
						lstrcatA(engineDisassembledInstruction, " ");
					}
					lstrcatA(engineDisassembledInstruction, (LPCSTR)engineDecodedInstructions[0].operands.p);
				}
				return((char*)engineDisassembledInstruction);
			}else{
				VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
				return(NULL);
			}
		}else{
			return(NULL);
		}
	}else{
		VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
		return(NULL);
	}
}
__declspec(dllexport) void* __stdcall Disassemble(LPVOID DisassmAddress){
	return(DisassembleEx(dbgProcessInformation.hProcess, DisassmAddress, false));
}
__declspec(dllexport) long __stdcall StaticLengthDisassemble(LPVOID DisassmAddress){
	
	_DecodeResult DecodingResult;
	_DecodedInst DecodedInstructions[MAX_INSTRUCTIONS];	
	unsigned int DecodedInstructionsCount = 0;
#if !defined(_WIN64)
	_DecodeType DecodingType = Decode32Bits;
#else
	_DecodeType DecodingType = Decode64Bits;
#endif
	MEMORY_BASIC_INFORMATION MemInfo;
	DWORD MaxDisassmSize;

	VirtualQueryEx(GetCurrentProcess(), DisassmAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
	if(MemInfo.State == MEM_COMMIT){
		if((ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)MemInfo.RegionSize - (ULONG_PTR)DisassmAddress <= MAXIMUM_INSTRUCTION_SIZE){
			MaxDisassmSize = (DWORD)((ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)MemInfo.RegionSize - (ULONG_PTR)DisassmAddress - 1);
			VirtualQueryEx(GetCurrentProcess(), (LPVOID)((ULONG_PTR)DisassmAddress + (ULONG_PTR)MemInfo.RegionSize), &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
			if(MemInfo.State == MEM_COMMIT){
				MaxDisassmSize = MAXIMUM_INSTRUCTION_SIZE;
			}
		}else{
			MaxDisassmSize = MAXIMUM_INSTRUCTION_SIZE;
		}
		DecodingResult = distorm_decode(NULL, (const unsigned char*)DisassmAddress, MaxDisassmSize, DecodingType, DecodedInstructions, MAX_INSTRUCTIONS, &DecodedInstructionsCount);
		return(DecodedInstructions[0].size);
	}else{
		return(NULL);
	}
}
__declspec(dllexport) long __stdcall LengthDisassembleEx(HANDLE hProcess, LPVOID DisassmAddress){
	
	_DecodeResult DecodingResult;
	_DecodedInst DecodedInstructions[MAX_INSTRUCTIONS];	
	unsigned int DecodedInstructionsCount = 0;
#if !defined(_WIN64)
	_DecodeType DecodingType = Decode32Bits;
#else
	_DecodeType DecodingType = Decode64Bits;
#endif
	ULONG_PTR ueNumberOfBytesRead = 0;
	LPVOID ueReadBuffer = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	MEMORY_BASIC_INFORMATION MemInfo;
	DWORD MaxDisassmSize;

	if(hProcess != NULL){
		VirtualQueryEx(GetCurrentProcess(), DisassmAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		if(MemInfo.State == MEM_COMMIT){
			if((ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)MemInfo.RegionSize - (ULONG_PTR)DisassmAddress <= MAXIMUM_INSTRUCTION_SIZE){
				MaxDisassmSize = (DWORD)((ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)MemInfo.RegionSize - (ULONG_PTR)DisassmAddress - 1);
				VirtualQueryEx(GetCurrentProcess(), (LPVOID)((ULONG_PTR)DisassmAddress + (ULONG_PTR)MemInfo.RegionSize), &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
				if(MemInfo.State == MEM_COMMIT){
					MaxDisassmSize = MAXIMUM_INSTRUCTION_SIZE;
				}
			}else{
				MaxDisassmSize = MAXIMUM_INSTRUCTION_SIZE;
			}
			if(ReadProcessMemory(hProcess, (LPVOID)DisassmAddress, ueReadBuffer, MaxDisassmSize, &ueNumberOfBytesRead)){
				DecodingResult = distorm_decode(NULL, (const unsigned char*)ueReadBuffer, MaxDisassmSize, DecodingType, DecodedInstructions, MAX_INSTRUCTIONS, &DecodedInstructionsCount);
				VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
				return(DecodedInstructions[0].size);
			}else{
				VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
				return(-1);
			}
		}else{
			return(NULL);
		}
	}else{
		VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
		return(-1);
	}
}
__declspec(dllexport) long __stdcall LengthDisassemble(LPVOID DisassmAddress){
	return(LengthDisassembleEx(dbgProcessInformation.hProcess, DisassmAddress));
}
__declspec(dllexport) void* __stdcall InitDebug(char* szFileName, char* szCommandLine, char* szCurrentFolder){

	char szCreateWithCmdLine[1024];
	int DebugConsoleFlag = NULL;

	if(engineRemoveConsoleForDebugee){
		DebugConsoleFlag = CREATE_NO_WINDOW;
	}
	BreakPointSetCount = 0;
	RtlZeroMemory(&BreakPointBuffer, sizeof BreakPointBuffer);
	if(szCommandLine == NULL){
		if(CreateProcessA(szFileName, NULL, NULL, NULL, false, DEBUG_PROCESS+DEBUG_ONLY_THIS_PROCESS+DebugConsoleFlag, NULL, szCurrentFolder, &dbgStartupInfo, &dbgProcessInformation)){
			engineAttachedToProcess = false;
			engineAttachedProcessCallBack = NULL;
			RtlZeroMemory(&BreakPointBuffer, sizeof BreakPointBuffer);
			return(&dbgProcessInformation);
		}else{
			RtlZeroMemory(&dbgProcessInformation,sizeof PROCESS_INFORMATION);
			return(0);	
		}
	}else{
		wsprintfA(szCreateWithCmdLine, "\"%s\" %s", szFileName, szCommandLine);
		if(CreateProcessA(NULL, szCreateWithCmdLine, NULL, NULL, false, DEBUG_PROCESS+DEBUG_ONLY_THIS_PROCESS+DebugConsoleFlag, NULL, szCurrentFolder, &dbgStartupInfo, &dbgProcessInformation)){
			engineAttachedToProcess = false;
			engineAttachedProcessCallBack = NULL;
			RtlZeroMemory(&BreakPointBuffer, sizeof BreakPointBuffer);
			return(&dbgProcessInformation);
		}else{
			RtlZeroMemory(&dbgProcessInformation,sizeof PROCESS_INFORMATION);
			return(0);	
		}
	}
}
__declspec(dllexport) void* __stdcall InitDebugEx(char* szFileName, char* szCommandLine, char* szCurrentFolder, LPVOID EntryCallBack){
	DebugExeFileEntryPointCallBack = EntryCallBack;
	return(InitDebug(szFileName, szCommandLine, szCurrentFolder));
}
__declspec(dllexport) void* __stdcall InitDLLDebug(char* szFileName, bool ReserveModuleBase, char* szCommandLine, char* szCurrentFolder, LPVOID EntryCallBack){

	int i = NULL;
	int j = NULL;
	bool ReturnData = false;

	RtlZeroMemory(&szDebuggerName, 512);
	lstrcpyA(szDebuggerName, szFileName);
	i = lstrlenA(szDebuggerName);
	while(szDebuggerName[i] != 0x5C){
		i--;
	}
	szDebuggerName[i+1] = 0x00;
	lstrcatA(szDebuggerName, "DLLLoader.exe");
	RtlZeroMemory(&szReserveModuleName, 512);
	lstrcpyA(szReserveModuleName, szFileName);
	lstrcatA(szReserveModuleName, ".module");
#if defined(_WIN64)
	ReturnData = EngineExtractResource("LOADERx64", szDebuggerName);
	if(ReserveModuleBase){
		EngineExtractResource("MODULEx64", szReserveModuleName);
	}
#else
	ReturnData = EngineExtractResource("LOADERx86", szDebuggerName);
	if(ReserveModuleBase){
		EngineExtractResource("MODULEx86", szReserveModuleName);
	}
#endif
	if(ReturnData){
		engineDebuggingDLL = true;
		i = lstrlenA(szFileName);
		while(szFileName[i] != 0x5C && i >= NULL){
			i--;
		}
		j = lstrlenA(szReserveModuleName);
		while(szReserveModuleName[j] != 0x5C && j >= NULL){
			j--;
		}
		engineDebuggingDLLBase = NULL;
		engineDebuggingDLLFullFileName = szFileName;
		engineDebuggingDLLFileName = &szFileName[i+1];
		engineDebuggingDLLReserveFileName = &szReserveModuleName[j+1];
		DebugModuleImageBase = (ULONG_PTR)GetPE32Data(szFileName, NULL, UE_IMAGEBASE);
		DebugModuleEntryPoint = (ULONG_PTR)GetPE32Data(szFileName, NULL, UE_OEP);
		DebugModuleEntryPointCallBack = EntryCallBack;
		if(ReserveModuleBase){
			RelocaterChangeFileBase(szReserveModuleName, DebugModuleImageBase);
		}
		return(InitDebug(szDebuggerName, szCommandLine, szCurrentFolder));
	}else{
		return(NULL);
	}
	return(NULL);
}
__declspec(dllexport) bool __stdcall StopDebug(){
	if(dbgProcessInformation.hProcess != NULL){
		TerminateThread(dbgProcessInformation.hThread, NULL);
		TerminateProcess(dbgProcessInformation.hProcess, NULL);
		return(true);
	}else{
		return(false);
	}
}
__declspec(dllexport) void __stdcall SetBPXOptions(long DefaultBreakPointType){
	engineDefaultBreakPointType = DefaultBreakPointType;
}
__declspec(dllexport) bool __stdcall IsBPXEnabled(ULONG_PTR bpxAddress){

	int i;
	ULONG_PTR NumberOfBytesReadWritten = 0;	
	DWORD MaximumBreakPoints = 0;
	BYTE ReadData[10] = {};

	for(i = 0; i < BreakPointSetCount; i++){
		if(BreakPointBuffer[i].BreakPointAddress == bpxAddress){
			if(BreakPointBuffer[i].BreakPointActive != UE_BPXINACTIVE && BreakPointBuffer[i].BreakPointActive != UE_BPXREMOVED){
				if(ReadProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &ReadData[0], UE_MAX_BREAKPOINT_SIZE, &NumberOfBytesReadWritten)){
					if(BreakPointBuffer[i].AdvancedBreakPointType == UE_BREAKPOINT_INT3 && ReadData[0] == INT3BreakPoint){
						return(true);
					}else if(BreakPointBuffer[i].AdvancedBreakPointType == UE_BREAKPOINT_LONG_INT3 && ReadData[0] == INT3LongBreakPoint[0] && ReadData[1] == INT3LongBreakPoint[1]){
						return(true);
					}else if(BreakPointBuffer[i].AdvancedBreakPointType == UE_BREAKPOINT_UD2 && ReadData[0] == UD2BreakPoint[0] && ReadData[1] == UD2BreakPoint[1]){
						return(true);
					}else{
						return(false);
					}
				}else{
					return(false);
				}
			}else{
				return(false);
			}
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall EnableBPX(ULONG_PTR bpxAddress){

	int i;
	MEMORY_BASIC_INFORMATION MemInfo; 
	ULONG_PTR NumberOfBytesReadWritten = 0;
	DWORD MaximumBreakPoints = 0;
	bool testWrite = false;
	DWORD OldProtect;

	for(i = 0; i < BreakPointSetCount; i++){
		if(BreakPointBuffer[i].BreakPointAddress == bpxAddress){
			VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
			OldProtect = MemInfo.AllocationProtect;
			VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, PAGE_EXECUTE_READWRITE, &OldProtect);
			if(BreakPointBuffer[i].BreakPointActive == UE_BPXINACTIVE && (BreakPointBuffer[i].BreakPointType == UE_BREAKPOINT || BreakPointBuffer[i].BreakPointType == UE_SINGLESHOOT)){
				if(BreakPointBuffer[i].AdvancedBreakPointType == UE_BREAKPOINT_INT3){
					if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &INT3BreakPoint, 1, &NumberOfBytesReadWritten)){
						testWrite = true;
					}
				}else if(BreakPointBuffer[i].AdvancedBreakPointType == UE_BREAKPOINT_LONG_INT3){
					if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &INT3LongBreakPoint, 2, &NumberOfBytesReadWritten)){
						testWrite = true;
					}
				}else if(BreakPointBuffer[i].AdvancedBreakPointType == UE_BREAKPOINT_UD2){
					if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &UD2BreakPoint, 2, &NumberOfBytesReadWritten)){
						testWrite = true;
					}
				}
				if(testWrite){
					BreakPointBuffer[i].BreakPointActive = UE_BPXACTIVE;
					VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
					return(true);
				}else{
					VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
					return(false);
				}
			}else{
				VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
				return(false);
			}
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall DisableBPX(ULONG_PTR bpxAddress){

	int i;
	MEMORY_BASIC_INFORMATION MemInfo; 
	ULONG_PTR NumberOfBytesReadWritten = 0;
	DWORD MaximumBreakPoints = 0;
	DWORD OldProtect;

	for(i = 0; i < BreakPointSetCount; i++){
		if(BreakPointBuffer[i].BreakPointAddress == bpxAddress){
			VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
			OldProtect = MemInfo.AllocationProtect;
			VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, PAGE_EXECUTE_READWRITE, &OldProtect);
			if(BreakPointBuffer[i].BreakPointActive == UE_BPXACTIVE && (BreakPointBuffer[i].BreakPointType == UE_BREAKPOINT || BreakPointBuffer[i].BreakPointType == UE_SINGLESHOOT)){
				if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &BreakPointBuffer[i].OriginalByte[0], BreakPointBuffer[i].BreakPointSize, &NumberOfBytesReadWritten)){
					BreakPointBuffer[i].BreakPointActive = UE_BPXINACTIVE;
					VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
					return(true);
				}else{
					VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
					return(false);
				}
			}else{
				VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
				return(false);
			}
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall SetBPX(ULONG_PTR bpxAddress, DWORD bpxType, LPVOID bpxCallBack){

	int i = 0;
	int j = -1;
	void* bpxDataPrt;
	PMEMORY_COMPARE_HANDLER bpxDataCmpPtr;
	MEMORY_BASIC_INFORMATION MemInfo; 
	ULONG_PTR NumberOfBytesReadWritten = 0;
	DWORD OldProtect;

	if(bpxCallBack == NULL){
		return(false);
	}
	for(i = 0; i < BreakPointSetCount; i++){
		if(BreakPointBuffer[0].BreakPointAddress == bpxAddress && BreakPointBuffer[i].BreakPointActive == UE_BPXACTIVE){
			return(true);
		}else if(BreakPointBuffer[i].BreakPointAddress == bpxAddress && BreakPointBuffer[i].BreakPointActive == UE_BPXINACTIVE){
			return(EnableBPX(bpxAddress));
		}else if(j == -1 && BreakPointBuffer[i].BreakPointActive == UE_BPXREMOVED){
			j = i;
		}
	}
	if(j == -1){
		BreakPointSetCount++;
	}else{
		i = j;
	}
	if(i < MAXIMUM_BREAKPOINTS){
		RtlZeroMemory(&BreakPointBuffer[i], sizeof BreakPointDetail);
		if(engineDefaultBreakPointType == UE_BREAKPOINT_INT3){
			BreakPointBuffer[i].BreakPointSize = 1;
			bpxDataPrt = &INT3BreakPoint;
		}else if(engineDefaultBreakPointType == UE_BREAKPOINT_LONG_INT3){
			BreakPointBuffer[i].BreakPointSize = 2;
			bpxDataPrt = &INT3LongBreakPoint;
		}else if(engineDefaultBreakPointType == UE_BREAKPOINT_UD2){
			BreakPointBuffer[i].BreakPointSize = 2;
			bpxDataPrt = &UD2BreakPoint;
		}
		bpxDataCmpPtr = (PMEMORY_COMPARE_HANDLER)bpxDataPrt;
		VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		OldProtect = MemInfo.AllocationProtect;
		VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, PAGE_EXECUTE_READWRITE, &OldProtect);
		if(ReadProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &BreakPointBuffer[i].OriginalByte[0], BreakPointBuffer[i].BreakPointSize, &NumberOfBytesReadWritten)){
			if(BreakPointBuffer[i].OriginalByte[0] != bpxDataCmpPtr->Array.bArrayEntry[0]){
				if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, bpxDataPrt, BreakPointBuffer[i].BreakPointSize, &NumberOfBytesReadWritten)){
					BreakPointBuffer[i].AdvancedBreakPointType = (BYTE)engineDefaultBreakPointType;
					BreakPointBuffer[i].BreakPointActive = UE_BPXACTIVE;
					BreakPointBuffer[i].BreakPointAddress = bpxAddress;
					BreakPointBuffer[i].BreakPointType = (BYTE)bpxType;
					BreakPointBuffer[i].NumberOfExecutions = -1;
					BreakPointBuffer[i].ExecuteCallBack = (ULONG_PTR)bpxCallBack;
					VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
					return(true);
				}else{
					VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
					return(false);
				}
			}else{
				VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
				return(false);
			}
		}else{
			VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
			return(false);
		}
	}else{
		BreakPointSetCount--;
		return(false);
	}
}
__declspec(dllexport) bool __stdcall SetBPXEx(ULONG_PTR bpxAddress, DWORD bpxType, DWORD NumberOfExecution, DWORD CmpRegister, DWORD CmpCondition, ULONG_PTR CmpValue, LPVOID bpxCallBack, LPVOID bpxCompareCallBack, LPVOID bpxRemoveCallBack){

	int i = 0;
	int j = -1;
	void* bpxDataPrt;
	PMEMORY_COMPARE_HANDLER bpxDataCmpPtr;
	MEMORY_BASIC_INFORMATION MemInfo; 
	ULONG_PTR NumberOfBytesReadWritten = 0;
	DWORD OldProtect;

	if(bpxCallBack == NULL){
		return(false);
	}
	for(i = 0; i < BreakPointSetCount; i++){
		if(BreakPointBuffer[i].BreakPointAddress == bpxAddress && BreakPointBuffer[i].BreakPointActive == UE_BPXACTIVE){
			return(true);
		}else if(BreakPointBuffer[i].BreakPointAddress == bpxAddress && BreakPointBuffer[i].BreakPointActive == UE_BPXINACTIVE){
			return(EnableBPX(bpxAddress));
		}else if(j == -1 && BreakPointBuffer[i].BreakPointActive == UE_BPXREMOVED){
			j = i;
		}
	}
	if(j == -1){
		BreakPointSetCount++;
	}else{
		i = j;
	}
	if(i < MAXIMUM_BREAKPOINTS){
		RtlZeroMemory(&BreakPointBuffer[i], sizeof BreakPointDetail);
		if(engineDefaultBreakPointType == UE_BREAKPOINT_INT3){
			BreakPointBuffer[i].BreakPointSize = 1;
			bpxDataPrt = &INT3BreakPoint;
		}else if(engineDefaultBreakPointType == UE_BREAKPOINT_LONG_INT3){
			BreakPointBuffer[i].BreakPointSize = 2;
			bpxDataPrt = &INT3LongBreakPoint;
		}else if(engineDefaultBreakPointType == UE_BREAKPOINT_UD2){
			BreakPointBuffer[i].BreakPointSize = 2;
			bpxDataPrt = &UD2BreakPoint;
		}
		bpxDataCmpPtr = (PMEMORY_COMPARE_HANDLER)bpxDataPrt;
		VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		OldProtect = MemInfo.AllocationProtect;
		VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, PAGE_EXECUTE_READWRITE, &OldProtect);
		if(ReadProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &BreakPointBuffer[i].OriginalByte[0], BreakPointBuffer[i].BreakPointSize, &NumberOfBytesReadWritten)){
			if(BreakPointBuffer[i].OriginalByte[0] != bpxDataCmpPtr->Array.bArrayEntry[0]){
				if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, bpxDataPrt, BreakPointBuffer[i].BreakPointSize, &NumberOfBytesReadWritten)){
					BreakPointBuffer[i].AdvancedBreakPointType = (BYTE)engineDefaultBreakPointType;
					BreakPointBuffer[i].BreakPointActive = UE_BPXACTIVE;
					BreakPointBuffer[i].BreakPointAddress = bpxAddress;
					BreakPointBuffer[i].BreakPointType = (BYTE)bpxType;
					BreakPointBuffer[i].NumberOfExecutions = NumberOfExecution;
					BreakPointBuffer[i].CmpRegister = CmpRegister;
					BreakPointBuffer[i].CmpCondition = (BYTE)CmpCondition;
					BreakPointBuffer[i].CmpValue = CmpValue;
					BreakPointBuffer[i].ExecuteCallBack = (ULONG_PTR)bpxCallBack;
					BreakPointBuffer[i].RemoveCallBack = (ULONG_PTR)bpxRemoveCallBack;
					BreakPointBuffer[i].CompareCallBack = (ULONG_PTR)bpxCompareCallBack;
					VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
					return(true);
				}else{
					VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
					return(false);
				}
			}else{
				VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
				return(false);
			}
		}else{
			VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
			return(false);
		}
	}else{
		BreakPointSetCount--;		
		return(false);
	}
}
__declspec(dllexport) bool __stdcall DeleteBPX(ULONG_PTR bpxAddress){

	int i;
	typedef void(__stdcall *fCustomBreakPoint)(void* myBreakPointAddress);
	fCustomBreakPoint myCustomBreakPoint;
	MEMORY_BASIC_INFORMATION MemInfo; 
	ULONG_PTR NumberOfBytesReadWritten = 0;
	DWORD OldProtect;

	for(i = 0; i < BreakPointSetCount; i++){
		if(BreakPointBuffer[i].BreakPointAddress == bpxAddress){
			if(i - 1 == BreakPointSetCount){
				BreakPointSetCount--;
			}
			break;
		}
	}
	if(BreakPointBuffer[i].BreakPointAddress == bpxAddress){
		VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		OldProtect = MemInfo.AllocationProtect;
		VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, PAGE_EXECUTE_READWRITE, &OldProtect);
		if(BreakPointBuffer[i].BreakPointType == UE_BREAKPOINT || BreakPointBuffer[i].BreakPointType == UE_SINGLESHOOT){
			if(BreakPointBuffer[i].BreakPointActive == UE_BPXACTIVE){
				if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &BreakPointBuffer[i].OriginalByte[0], BreakPointBuffer[i].BreakPointSize, &NumberOfBytesReadWritten)){
					VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
					if(BreakPointBuffer[i].RemoveCallBack != NULL){
						__try{
							myCustomBreakPoint = (fCustomBreakPoint)((LPVOID)BreakPointBuffer[i].RemoveCallBack);
							myCustomBreakPoint((void*)BreakPointBuffer[i].BreakPointAddress);
							RtlZeroMemory(&BreakPointBuffer[i], sizeof BreakPointDetail);
						}__except(EXCEPTION_EXECUTE_HANDLER){
							RtlZeroMemory(&BreakPointBuffer[i], sizeof BreakPointDetail);
							return(true);
						}
					}else{
						RtlZeroMemory(&BreakPointBuffer[i], sizeof BreakPointDetail);
					}
					return(true);
				}else{
					VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
					return(false);
				}
			}else{
				RtlZeroMemory(&BreakPointBuffer[i], sizeof BreakPointDetail);
				return(true);
			}
		}else{
			VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer[i].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
			return(false);
		}
	}else{
		return(false);
	}
}
__declspec(dllexport) bool __stdcall SafeDeleteBPX(ULONG_PTR bpxAddress){
	return(DeleteBPX(bpxAddress));
}
__declspec(dllexport) bool __stdcall SetAPIBreakPoint(char* szDLLName, char* szAPIName, DWORD bpxType, DWORD bpxPlace, LPVOID bpxCallBack){

	BYTE ReadByte = NULL;
	HMODULE hModule = NULL;
	DWORD ReadMemSize = NULL;
	ULONG_PTR APIAddress = NULL;
	ULONG_PTR tryAPIAddress = NULL;
	ULONG_PTR QueryAPIAddress = NULL;
	int i = MAX_RET_SEARCH_INSTRUCTIONS;
	ULONG_PTR ueNumberOfReadWrite = NULL;
	int currentInstructionLen = NULL;
	bool ModuleLoaded = false;
	void* CmdBuffer = NULL;
	bool RemovedBpx = false;

	if(szDLLName != NULL && szAPIName != NULL){
		hModule = GetModuleHandleA(szDLLName);
		if(hModule == NULL){
			if(engineAlowModuleLoading){
				hModule = LoadLibraryA(szDLLName);
				ModuleLoaded = true;
			}else{
				ReadMemSize = MAX_RET_SEARCH_INSTRUCTIONS * MAXIMUM_INSTRUCTION_SIZE;
				APIAddress = (ULONG_PTR)EngineGlobalAPIHandler(dbgProcessInformation.hProcess, NULL, NULL, szAPIName, UE_OPTION_IMPORTER_RETURN_APIADDRESS);
				if(APIAddress != NULL){
					CmdBuffer = VirtualAlloc(NULL, ReadMemSize, MEM_COMMIT, PAGE_READWRITE);
					while(ReadProcessMemory(dbgProcessInformation.hProcess, (void*)APIAddress, CmdBuffer, ReadMemSize, &ueNumberOfReadWrite) == false && ReadMemSize > NULL){
						ReadMemSize = ReadMemSize - (MAXIMUM_INSTRUCTION_SIZE * 10);
					}
					if(ReadMemSize == NULL){
						VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
						APIAddress = NULL;
					}else{
						tryAPIAddress = (ULONG_PTR)CmdBuffer;
					}
				}
			}
		}
		if(hModule != NULL || APIAddress != NULL){
			if(hModule != NULL){
				APIAddress = (ULONG_PTR)GetProcAddress(hModule, szAPIName);
			}
			if(bpxPlace == UE_APIEND){
				if(tryAPIAddress == NULL){
					tryAPIAddress = APIAddress;
				}
				QueryAPIAddress = APIAddress;
				RtlMoveMemory(&ReadByte, (LPVOID)tryAPIAddress, 1);
				while(i > 0 && ReadByte != 0xC3 && ReadByte != 0xC2){
					if(engineAlowModuleLoading == false && CmdBuffer != NULL){
						if(IsBPXEnabled(QueryAPIAddress)){
							DisableBPX(QueryAPIAddress);
							ReadProcessMemory(dbgProcessInformation.hProcess, (void*)APIAddress, CmdBuffer, ReadMemSize, &ueNumberOfReadWrite);
							RemovedBpx = true;
						}
					}
					currentInstructionLen = StaticLengthDisassemble((LPVOID)tryAPIAddress);
					tryAPIAddress = tryAPIAddress + currentInstructionLen;
					RtlMoveMemory(&ReadByte, (LPVOID)tryAPIAddress, 1);
					QueryAPIAddress = QueryAPIAddress + currentInstructionLen;
					if(!engineAlowModuleLoading){
						if(RemovedBpx){
							EnableBPX(QueryAPIAddress - currentInstructionLen);
						}
					}
					RemovedBpx = false;
					i--;
				}
				if(i != NULL){
					if((engineAlowModuleLoading == true && ModuleLoaded == true) || (engineAlowModuleLoading == true && ModuleLoaded == false)){
						APIAddress = tryAPIAddress;
					}else if(!engineAlowModuleLoading){
						if(CmdBuffer != NULL){
							APIAddress = tryAPIAddress - (ULONG_PTR)CmdBuffer + APIAddress;	
						}else{
							APIAddress = tryAPIAddress;
						}
					}
				}else{
					if(ModuleLoaded){
						FreeLibrary(hModule);
					}
					if(CmdBuffer != NULL){
						VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
					}
					return(false);
				}
			}
			if(engineAlowModuleLoading){
				APIAddress = (ULONG_PTR)EngineGlobalAPIHandler(dbgProcessInformation.hProcess, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_REALIGN_APIADDRESS);
				if(ModuleLoaded){
					FreeLibrary(hModule);
				}
			}else{
				if(CmdBuffer != NULL){
					VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
				}
			}
			return(SetBPX(APIAddress, bpxType, bpxCallBack));
		}else{
			if(engineAlowModuleLoading){
				if(ModuleLoaded){
					FreeLibrary(hModule);
				}
			}else{
				if(CmdBuffer != NULL){
					VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
				}
			}
			return(false);
		}
	}else{
		return(false);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall DeleteAPIBreakPoint(char* szDLLName, char* szAPIName, DWORD bpxPlace){

	BYTE ReadByte = NULL;
	HMODULE hModule = NULL;
	DWORD ReadMemSize = NULL;
	ULONG_PTR APIAddress = NULL;
	ULONG_PTR tryAPIAddress = NULL;
	ULONG_PTR QueryAPIAddress = NULL;
	int i = MAX_RET_SEARCH_INSTRUCTIONS;
	ULONG_PTR ueNumberOfReadWrite = NULL;
	int currentInstructionLen = NULL;
	bool ModuleLoaded = false;
	void* CmdBuffer = NULL;
	bool RemovedBpx = false;

	if(szDLLName != NULL && szAPIName != NULL){
		hModule = GetModuleHandleA(szDLLName);
		if(hModule == NULL){
			if(engineAlowModuleLoading){
				hModule = LoadLibraryA(szDLLName);
				ModuleLoaded = true;
			}else{
				ReadMemSize = MAX_RET_SEARCH_INSTRUCTIONS * MAXIMUM_INSTRUCTION_SIZE;
				APIAddress = (ULONG_PTR)EngineGlobalAPIHandler(dbgProcessInformation.hProcess, NULL, NULL, szAPIName, UE_OPTION_IMPORTER_RETURN_APIADDRESS);
				if(APIAddress != NULL){
					CmdBuffer = VirtualAlloc(NULL, ReadMemSize, MEM_COMMIT, PAGE_READWRITE);
					while(ReadProcessMemory(dbgProcessInformation.hProcess, (void*)APIAddress, CmdBuffer, ReadMemSize, &ueNumberOfReadWrite) == false && ReadMemSize > NULL){
						ReadMemSize = ReadMemSize - (MAXIMUM_INSTRUCTION_SIZE * 10);
					}
					if(ReadMemSize == NULL){
						VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
						APIAddress = NULL;
					}else{
						tryAPIAddress = (ULONG_PTR)CmdBuffer;
					}
				}
			}
		}
		if(hModule != NULL || APIAddress != NULL){
			if(hModule != NULL){
				APIAddress = (ULONG_PTR)GetProcAddress(hModule, szAPIName);
			}
			if(bpxPlace == UE_APIEND){
				if(tryAPIAddress == NULL){
					tryAPIAddress = APIAddress;
				}
				QueryAPIAddress = APIAddress;
				RtlMoveMemory(&ReadByte, (LPVOID)tryAPIAddress, 1);
				while(i > 0 && ReadByte != 0xC3 && ReadByte != 0xC2){
					if(engineAlowModuleLoading == false && CmdBuffer != NULL){
						if(IsBPXEnabled(QueryAPIAddress)){
							DisableBPX(QueryAPIAddress);
							ReadProcessMemory(dbgProcessInformation.hProcess, (void*)APIAddress, CmdBuffer, ReadMemSize, &ueNumberOfReadWrite);
							RemovedBpx = true;
						}
					}
					currentInstructionLen = StaticLengthDisassemble((LPVOID)tryAPIAddress);
					tryAPIAddress = tryAPIAddress + currentInstructionLen;
					RtlMoveMemory(&ReadByte, (LPVOID)tryAPIAddress, 1);
					QueryAPIAddress = QueryAPIAddress + currentInstructionLen;
					if(!engineAlowModuleLoading){
						if(RemovedBpx){
							EnableBPX(QueryAPIAddress - currentInstructionLen);
						}
					}
					RemovedBpx = false;
					i--;
				}
				if(i != NULL){
					if((engineAlowModuleLoading == true && ModuleLoaded == true) || (engineAlowModuleLoading == true && ModuleLoaded == false)){
						APIAddress = tryAPIAddress;
					}else if(!engineAlowModuleLoading){
						if(CmdBuffer != NULL){
							APIAddress = tryAPIAddress - (ULONG_PTR)CmdBuffer + APIAddress;	
						}else{
							APIAddress = tryAPIAddress;
						}
					}
				}else{
					if(ModuleLoaded){
						FreeLibrary(hModule);
					}
					if(CmdBuffer != NULL){
						VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
					}
					return(false);
				}
			}
			if(engineAlowModuleLoading){
				APIAddress = (ULONG_PTR)EngineGlobalAPIHandler(dbgProcessInformation.hProcess, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_REALIGN_APIADDRESS);
				if(ModuleLoaded){
					FreeLibrary(hModule);
				}
			}else{
				if(CmdBuffer != NULL){
					VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
				}
			}
			return(DeleteBPX(APIAddress));
		}else{
			if(engineAlowModuleLoading){
				if(ModuleLoaded){
					FreeLibrary(hModule);
				}
			}else{
				if(CmdBuffer != NULL){
					VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
				}
			}
			return(false);
		}
	}else{
		return(false);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall SafeDeleteAPIBreakPoint(char* szDLLName, char* szAPIName, DWORD bpxPlace){
	return(DeleteAPIBreakPoint(szDLLName, szAPIName, bpxPlace));
}
__declspec(dllexport) bool __stdcall SetMemoryBPX(ULONG_PTR MemoryStart, DWORD SizeOfMemory, LPVOID bpxCallBack){

	int i = 0;
	int j = -1;
	MEMORY_BASIC_INFORMATION MemInfo; 
	ULONG_PTR NumberOfBytesReadWritten = 0;
	DWORD NewProtect = 0;
	DWORD OldProtect = 0;

	for(i = 0; i < BreakPointSetCount; i++){
		if(BreakPointBuffer[i].BreakPointAddress == MemoryStart){
			if(BreakPointBuffer[i].BreakPointActive == UE_BPXACTIVE){
				RemoveMemoryBPX(BreakPointBuffer[i].BreakPointAddress, BreakPointBuffer[i].BreakPointSize);
			}
			j = i;
			break;
		}else if(j == -1 && BreakPointBuffer[i].BreakPointActive == UE_BPXREMOVED){
			j = i;
		}
	}
	if(BreakPointBuffer[i].BreakPointAddress != MemoryStart){
		if(j != -1){
			i = j;
		}else{
			BreakPointSetCount++;
		}
	}
	if(i < MAXIMUM_BREAKPOINTS){
		RtlZeroMemory(&BreakPointBuffer[i], sizeof BreakPointDetail);
		VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)MemoryStart, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		OldProtect = MemInfo.Protect;
		if(!(OldProtect & PAGE_GUARD)){
			NewProtect = OldProtect ^ PAGE_GUARD;
			VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)MemoryStart, SizeOfMemory, NewProtect, &OldProtect);
			BreakPointBuffer[i].BreakPointActive = UE_BPXACTIVE;
			BreakPointBuffer[i].BreakPointAddress = MemoryStart;
			BreakPointBuffer[i].BreakPointType = UE_MEMORY;
			BreakPointBuffer[i].BreakPointSize = SizeOfMemory;
			BreakPointBuffer[i].NumberOfExecutions = -1;
			BreakPointBuffer[i].ExecuteCallBack = (ULONG_PTR)bpxCallBack;
		}
		return(true);
	}else{
		return(false);
	}
}
__declspec(dllexport) bool __stdcall SetMemoryBPXEx(ULONG_PTR MemoryStart, DWORD SizeOfMemory, DWORD BreakPointType, bool RestoreOnHit, LPVOID bpxCallBack){

	int i = 0;
	int j = -1;
	MEMORY_BASIC_INFORMATION MemInfo; 
	ULONG_PTR NumberOfBytesReadWritten = 0;
	DWORD NewProtect = 0;
	DWORD OldProtect = 0;

	for(i = 0; i < BreakPointSetCount; i++){
		if(BreakPointBuffer[i].BreakPointAddress == MemoryStart){
			if(BreakPointBuffer[i].BreakPointActive == UE_BPXACTIVE){
				RemoveMemoryBPX(BreakPointBuffer[i].BreakPointAddress, BreakPointBuffer[i].BreakPointSize);
			}
			j = i;
			break;
		}else if(j == -1 && BreakPointBuffer[i].BreakPointActive == UE_BPXREMOVED){
			j = i;
		}
	}
	if(BreakPointBuffer[i].BreakPointAddress != MemoryStart){
		if(j != -1){
			i = j;
		}else{
			BreakPointSetCount++;
		}
	}
	if(i < MAXIMUM_BREAKPOINTS){
		RtlZeroMemory(&BreakPointBuffer[i], sizeof BreakPointDetail);
		VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)MemoryStart, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		OldProtect = MemInfo.Protect;
		if(!(OldProtect & PAGE_GUARD)){
			NewProtect = OldProtect ^ PAGE_GUARD;
			VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)MemoryStart, SizeOfMemory, NewProtect, &OldProtect);
			BreakPointBuffer[i].BreakPointActive = UE_BPXACTIVE;
			BreakPointBuffer[i].BreakPointAddress = MemoryStart;
			BreakPointBuffer[i].BreakPointType = (BYTE)BreakPointType;
			BreakPointBuffer[i].BreakPointSize = SizeOfMemory;
			BreakPointBuffer[i].NumberOfExecutions = -1;
			BreakPointBuffer[i].MemoryBpxRestoreOnHit = (BYTE)RestoreOnHit;
			BreakPointBuffer[i].ExecuteCallBack = (ULONG_PTR)bpxCallBack;
		}
		return(true);
	}else{
		return(false);
	}
}
__declspec(dllexport) bool __stdcall RemoveMemoryBPX(ULONG_PTR MemoryStart, DWORD SizeOfMemory){

	int i = 0;
	MEMORY_BASIC_INFORMATION MemInfo; 
	ULONG_PTR NumberOfBytesReadWritten = 0;
	DWORD NewProtect = 0;
	DWORD OldProtect = 0;

	for(i = 0; i < BreakPointSetCount; i++){
		if(BreakPointBuffer[i].BreakPointAddress == MemoryStart && BreakPointBuffer[i].BreakPointType == UE_MEMORY){
			if(i - 1 == BreakPointSetCount){
				BreakPointSetCount--;
			}
			break;
		}
	}
	if(BreakPointBuffer[i].BreakPointAddress == MemoryStart){
		VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)MemoryStart, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		OldProtect = MemInfo.AllocationProtect;
		if(OldProtect & PAGE_GUARD){
			NewProtect = OldProtect ^ PAGE_GUARD;
		}else{
			NewProtect = OldProtect;
		}
		if(SizeOfMemory != NULL){
			VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)MemoryStart, SizeOfMemory, NewProtect, &OldProtect);
		}else{
			VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)MemoryStart, BreakPointBuffer[i].BreakPointSize, NewProtect, &OldProtect);
		}
		RtlZeroMemory(&BreakPointBuffer[i], sizeof BreakPointDetail);
		return(true);
	}else{
		return(false);
	}
}
__declspec(dllexport) long long __stdcall GetContextDataEx(HANDLE hActiveThread, DWORD IndexOfRegister){

	RtlZeroMemory(&DBGContext, sizeof CONTEXT);
	DBGContext.ContextFlags = CONTEXT_ALL | CONTEXT_DEBUG_REGISTERS;
#if defined(_WIN64)	
	GetThreadContext(hActiveThread, &DBGContext);
	if(IndexOfRegister == UE_EAX){
		return((DWORD)DBGContext.Rax);
	}else if(IndexOfRegister == UE_EBX){
		return((DWORD)DBGContext.Rbx);
	}else if(IndexOfRegister == UE_ECX){
		return((DWORD)DBGContext.Rcx);
	}else if(IndexOfRegister == UE_EDX){
		return((DWORD)DBGContext.Rdx);
	}else if(IndexOfRegister == UE_EDI){
		return((DWORD)DBGContext.Rdi);
	}else if(IndexOfRegister == UE_ESI){
		return((DWORD)DBGContext.Rsi);
	}else if(IndexOfRegister == UE_EBP){
		return((DWORD)DBGContext.Rbp);
	}else if(IndexOfRegister == UE_ESP){
		return((DWORD)DBGContext.Rsp);
	}else if(IndexOfRegister == UE_EIP){
		return((DWORD)DBGContext.Rip);
	}else if(IndexOfRegister == UE_EFLAGS){
		return((DWORD)DBGContext.EFlags);
	}else if(IndexOfRegister == UE_RAX){
		return(DBGContext.Rax);
	}else if(IndexOfRegister == UE_RBX){
		return(DBGContext.Rbx);
	}else if(IndexOfRegister == UE_RCX){
		return(DBGContext.Rcx);
	}else if(IndexOfRegister == UE_RDX){
		return(DBGContext.Rdx);
	}else if(IndexOfRegister == UE_RDI){
		return(DBGContext.Rdi);
	}else if(IndexOfRegister == UE_RSI){
		return(DBGContext.Rsi);
	}else if(IndexOfRegister == UE_RBP){
		return(DBGContext.Rbp);
	}else if(IndexOfRegister == UE_RSP){
		return(DBGContext.Rsp);
	}else if(IndexOfRegister == UE_RIP){
		return(DBGContext.Rip);
	}else if(IndexOfRegister == UE_RFLAGS){
		return(DBGContext.EFlags);
	}else if(IndexOfRegister == UE_DR0){
		return(DBGContext.Dr0);
	}else if(IndexOfRegister == UE_DR1){
		return(DBGContext.Dr1);
	}else if(IndexOfRegister == UE_DR2){
		return(DBGContext.Dr2);
	}else if(IndexOfRegister == UE_DR3){
		return(DBGContext.Dr3);
	}else if(IndexOfRegister == UE_DR6){
		return(DBGContext.Dr6);
	}else if(IndexOfRegister == UE_DR7){
		return(DBGContext.Dr7);
	}else if(IndexOfRegister == UE_R8){
		return(DBGContext.R8);
	}else if(IndexOfRegister == UE_R9){
		return(DBGContext.R9);
	}else if(IndexOfRegister == UE_R10){
		return(DBGContext.R10);
	}else if(IndexOfRegister == UE_R11){
		return(DBGContext.R11);
	}else if(IndexOfRegister == UE_R12){
		return(DBGContext.R12);
	}else if(IndexOfRegister == UE_R13){
		return(DBGContext.R13);
	}else if(IndexOfRegister == UE_R14){
		return(DBGContext.R14);
	}else if(IndexOfRegister == UE_R15){
		return(DBGContext.R15);
	}else if(IndexOfRegister == UE_CIP){
		return(DBGContext.Rip);
	}else if(IndexOfRegister == UE_CSP){
		return(DBGContext.Rsp);
	}
#else
	GetThreadContext(hActiveThread, &DBGContext);
	if(IndexOfRegister == UE_EAX){
		return(DBGContext.Eax);
	}else if(IndexOfRegister == UE_EBX){
		return(DBGContext.Ebx);
	}else if(IndexOfRegister == UE_ECX){
		return(DBGContext.Ecx);
	}else if(IndexOfRegister == UE_EDX){
		return(DBGContext.Edx);
	}else if(IndexOfRegister == UE_EDI){
		return(DBGContext.Edi);
	}else if(IndexOfRegister == UE_ESI){
		return(DBGContext.Esi);
	}else if(IndexOfRegister == UE_EBP){
		return(DBGContext.Ebp);
	}else if(IndexOfRegister == UE_ESP){
		return(DBGContext.Esp);
	}else if(IndexOfRegister == UE_EIP){
		return(DBGContext.Eip);
	}else if(IndexOfRegister == UE_EFLAGS){
		return(DBGContext.EFlags);
	}else if(IndexOfRegister == UE_DR0){
		return(DBGContext.Dr0);
	}else if(IndexOfRegister == UE_DR1){
		return(DBGContext.Dr1);
	}else if(IndexOfRegister == UE_DR2){
		return(DBGContext.Dr2);
	}else if(IndexOfRegister == UE_DR3){
		return(DBGContext.Dr3);
	}else if(IndexOfRegister == UE_DR6){
		return(DBGContext.Dr6);
	}else if(IndexOfRegister == UE_DR7){
		return(DBGContext.Dr7);
	}else if(IndexOfRegister == UE_CIP){
		return(DBGContext.Eip);
	}else if(IndexOfRegister == UE_CSP){
		return(DBGContext.Esp);
	}
#endif
	return(-1);
}
__declspec(dllexport) long long __stdcall GetContextData(DWORD IndexOfRegister){

	HANDLE hActiveThread = 0;
	long long ContextReturn;

	hActiveThread = OpenThread(THREAD_GET_CONTEXT+THREAD_SET_CONTEXT+THREAD_QUERY_INFORMATION, false, DBGEvent.dwThreadId);
	ContextReturn = GetContextDataEx(hActiveThread, IndexOfRegister);
	EngineCloseHandle(hActiveThread);
	return(ContextReturn);
}
__declspec(dllexport) bool __stdcall SetContextDataEx(HANDLE hActiveThread, DWORD IndexOfRegister, ULONG_PTR NewRegisterValue){

	RtlZeroMemory(&DBGContext, sizeof CONTEXT);
	DBGContext.ContextFlags = CONTEXT_ALL | CONTEXT_DEBUG_REGISTERS;
#if defined(_WIN64)	
	GetThreadContext(hActiveThread, &DBGContext);
	if(IndexOfRegister == UE_EAX){
		NewRegisterValue = DBGContext.Rax - (DWORD)DBGContext.Rax + NewRegisterValue; 
		DBGContext.Rax = NewRegisterValue;
	}else if(IndexOfRegister == UE_EBX){
		NewRegisterValue = DBGContext.Rbx - (DWORD)DBGContext.Rbx + NewRegisterValue;
		DBGContext.Rbx = NewRegisterValue;
	}else if(IndexOfRegister == UE_ECX){
		NewRegisterValue = DBGContext.Rcx - (DWORD)DBGContext.Rcx + NewRegisterValue;
		DBGContext.Rcx = NewRegisterValue;
	}else if(IndexOfRegister == UE_EDX){
		NewRegisterValue = DBGContext.Rdx - (DWORD)DBGContext.Rdx + NewRegisterValue;
		DBGContext.Rdx = NewRegisterValue;
	}else if(IndexOfRegister == UE_EDI){
		NewRegisterValue = DBGContext.Rdi - (DWORD)DBGContext.Rdi + NewRegisterValue;
		DBGContext.Rdi = NewRegisterValue;
	}else if(IndexOfRegister == UE_ESI){
		NewRegisterValue = DBGContext.Rsi - (DWORD)DBGContext.Rsi + NewRegisterValue;
		DBGContext.Rsi = NewRegisterValue;
	}else if(IndexOfRegister == UE_EBP){
		NewRegisterValue = DBGContext.Rbp - (DWORD)DBGContext.Rbp + NewRegisterValue;
		DBGContext.Rbp = NewRegisterValue;
	}else if(IndexOfRegister == UE_ESP){
		NewRegisterValue = DBGContext.Rsp - (DWORD)DBGContext.Rsp + NewRegisterValue;
		DBGContext.Rsp = NewRegisterValue;
	}else if(IndexOfRegister == UE_EIP){
		NewRegisterValue = DBGContext.Rip - (DWORD)DBGContext.Rip + NewRegisterValue;
		DBGContext.Rip = NewRegisterValue;
	}else if(IndexOfRegister == UE_EFLAGS){
		DBGContext.EFlags = (DWORD)NewRegisterValue;
	}else if(IndexOfRegister == UE_RAX){
		DBGContext.Rax = NewRegisterValue;
	}else if(IndexOfRegister == UE_RBX){
		DBGContext.Rbx = NewRegisterValue;
	}else if(IndexOfRegister == UE_RCX){
		DBGContext.Rcx = NewRegisterValue;
	}else if(IndexOfRegister == UE_RDX){
		DBGContext.Rdx = NewRegisterValue;
	}else if(IndexOfRegister == UE_RDI){
		DBGContext.Rdi = NewRegisterValue;
	}else if(IndexOfRegister == UE_RSI){
		DBGContext.Rsi = NewRegisterValue;
	}else if(IndexOfRegister == UE_RBP){
		DBGContext.Rbp = NewRegisterValue;
	}else if(IndexOfRegister == UE_RSP){
		DBGContext.Rsp = NewRegisterValue;
	}else if(IndexOfRegister == UE_RIP){
		DBGContext.Rip = NewRegisterValue;
	}else if(IndexOfRegister == UE_RFLAGS){
		DBGContext.EFlags = (DWORD)NewRegisterValue;
	}else if(IndexOfRegister == UE_DR0){
		DBGContext.Dr0 = NewRegisterValue;
	}else if(IndexOfRegister == UE_DR1){
		DBGContext.Dr1 = NewRegisterValue;
	}else if(IndexOfRegister == UE_DR2){
		DBGContext.Dr2 = NewRegisterValue;
	}else if(IndexOfRegister == UE_DR3){
		DBGContext.Dr3 = NewRegisterValue;
	}else if(IndexOfRegister == UE_DR6){
		DBGContext.Dr6 = NewRegisterValue;
	}else if(IndexOfRegister == UE_DR7){
		DBGContext.Dr7 = NewRegisterValue;
	}else if(IndexOfRegister == UE_R8){
		DBGContext.R8 = NewRegisterValue;
	}else if(IndexOfRegister == UE_R9){
		DBGContext.R9 = NewRegisterValue;
	}else if(IndexOfRegister == UE_R10){
		DBGContext.R10 = NewRegisterValue;
	}else if(IndexOfRegister == UE_R11){
		DBGContext.R11 = NewRegisterValue;
	}else if(IndexOfRegister == UE_R12){
		DBGContext.R12 = NewRegisterValue;
	}else if(IndexOfRegister == UE_R13){
		DBGContext.R13 = NewRegisterValue;
	}else if(IndexOfRegister == UE_R14){
		DBGContext.R14 = NewRegisterValue;
	}else if(IndexOfRegister == UE_R15){
		DBGContext.R15 = NewRegisterValue;
	}else if(IndexOfRegister == UE_CIP){
		DBGContext.Rip = NewRegisterValue;
	}else if(IndexOfRegister == UE_CSP){
		DBGContext.Rsp = NewRegisterValue;
	}else{
		return(false);	
	}
	SetThreadContext(hActiveThread, &DBGContext);
	return(true);
#else
	GetThreadContext(hActiveThread, &DBGContext);
	if(IndexOfRegister == UE_EAX){
		DBGContext.Eax = NewRegisterValue;
	}else if(IndexOfRegister == UE_EBX){
		DBGContext.Ebx = NewRegisterValue;
	}else if(IndexOfRegister == UE_ECX){
		DBGContext.Ecx = NewRegisterValue;
	}else if(IndexOfRegister == UE_EDX){
		DBGContext.Edx = NewRegisterValue;
	}else if(IndexOfRegister == UE_EDI){
		DBGContext.Edi = NewRegisterValue;
	}else if(IndexOfRegister == UE_ESI){
		DBGContext.Esi = NewRegisterValue;
	}else if(IndexOfRegister == UE_EBP){
		DBGContext.Ebp = NewRegisterValue;
	}else if(IndexOfRegister == UE_ESP){
		DBGContext.Esp = NewRegisterValue;
	}else if(IndexOfRegister == UE_EIP){
		DBGContext.Eip = NewRegisterValue;
	}else if(IndexOfRegister == UE_EFLAGS){
		DBGContext.EFlags = NewRegisterValue;
	}else if(IndexOfRegister == UE_DR0){
		DBGContext.Dr0 = NewRegisterValue;
	}else if(IndexOfRegister == UE_DR1){
		DBGContext.Dr1 = NewRegisterValue;
	}else if(IndexOfRegister == UE_DR2){
		DBGContext.Dr2 = NewRegisterValue;
	}else if(IndexOfRegister == UE_DR3){
		DBGContext.Dr3 = NewRegisterValue;
	}else if(IndexOfRegister == UE_DR6){
		DBGContext.Dr6 = NewRegisterValue;
	}else if(IndexOfRegister == UE_DR7){
		DBGContext.Dr7 = NewRegisterValue;
	}else if(IndexOfRegister == UE_CIP){
		DBGContext.Eip = NewRegisterValue;
	}else if(IndexOfRegister == UE_CSP){
		DBGContext.Esp = NewRegisterValue;
	}else{
		return(false);	
	}
	SetThreadContext(hActiveThread, &DBGContext);
	return(true);
#endif
	return(false);
}
__declspec(dllexport) bool __stdcall SetContextData(DWORD IndexOfRegister, ULONG_PTR NewRegisterValue){

	HANDLE hActiveThread = 0;
	bool ContextReturn;

	hActiveThread = OpenThread(THREAD_GET_CONTEXT+THREAD_SET_CONTEXT+THREAD_QUERY_INFORMATION, false, DBGEvent.dwThreadId);
	ContextReturn = SetContextDataEx(hActiveThread, IndexOfRegister, NewRegisterValue);
	EngineCloseHandle(hActiveThread);
	return(ContextReturn);
}
__declspec(dllexport) void __stdcall ClearExceptionNumber(){
	CurrentExceptionsNumber = 0;
}
__declspec(dllexport) long __stdcall CurrentExceptionNumber(){
	return(CurrentExceptionsNumber);
}
__declspec(dllexport) long long __stdcall FindEx(HANDLE hProcess, LPVOID MemoryStart, DWORD MemorySize, LPVOID SearchPattern, DWORD PatternSize, LPBYTE WildCard){

	DWORD i = 0;
	DWORD j = 0;
	ULONG_PTR ueNumberOfBytesRead = 0;
	LPVOID ueReadBuffer = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	LPVOID SearchBuffer = ueReadBuffer;
	DWORD CurrentPatternSize = PatternSize;
	MEMORY_BASIC_INFORMATION MemoryInfo; 
	LPVOID currentSearchPosition = NULL;
	LPVOID startSearchPosition = NULL;
	DWORD currentSizeOfSearch = NULL;

	if(hProcess != NULL){
		VirtualQueryEx(hProcess, MemoryStart, &MemoryInfo, sizeof MEMORY_BASIC_INFORMATION);
		if(MemorySize == NULL || (ULONG_PTR)MemoryInfo.AllocationBase + MemoryInfo.RegionSize - (ULONG_PTR)MemoryStart < MemorySize){
			if(hProcess == GetCurrentProcess()){
				VirtualQueryEx(hProcess, MemoryInfo.AllocationBase, &MemoryInfo, sizeof MEMORY_BASIC_INFORMATION);
				MemorySize = (DWORD)((ULONG_PTR)MemoryInfo.AllocationBase + (ULONG_PTR)MemoryInfo.RegionSize - (ULONG_PTR)MemoryStart);
			}else{
				currentSearchPosition = (void*)((ULONG_PTR)MemoryInfo.BaseAddress + MemoryInfo.RegionSize);
				currentSizeOfSearch = (DWORD)((ULONG_PTR)currentSearchPosition - (ULONG_PTR)MemoryStart);
				if(MemorySize == NULL || MemorySize > currentSizeOfSearch){
					MemorySize = currentSizeOfSearch;
				}
			}
		}
		if(ReadProcessMemory(hProcess, MemoryStart, ueReadBuffer, MemorySize, &ueNumberOfBytesRead)){
			__try{
				while(MemorySize > PatternSize - 1){
					CurrentPatternSize = PatternSize;
					while(CurrentPatternSize > NULL && (*(PUCHAR)((ULONG_PTR)SearchPattern + j) == *(PUCHAR)WildCard || *(PUCHAR)((ULONG_PTR)SearchBuffer + i) == *(PUCHAR)((ULONG_PTR)SearchPattern + j))){
						CurrentPatternSize--;
						MemorySize--;
						i++;
						j++;
					}
					if(CurrentPatternSize == NULL){
						VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
						return((ULONG_PTR)MemoryStart + i - PatternSize);
					}else{
						if(*(PUCHAR)((ULONG_PTR)SearchBuffer + i) != *(PUCHAR)((ULONG_PTR)SearchPattern)){
							i++;
						}
						j = 0;
					}
					MemorySize--;
				}
			}__except(EXCEPTION_EXECUTE_HANDLER){
				VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
				return(0);
			}
			VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
			return(0);
		}else{
			VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
			return(0);
		}	
	}else{
		VirtualFree(ueReadBuffer, NULL, MEM_RELEASE);
		return(0);
	}
}
__declspec(dllexport) long long __stdcall Find(LPVOID MemoryStart, DWORD MemorySize, LPVOID SearchPattern, DWORD PatternSize, LPBYTE WildCard){
	return(FindEx(dbgProcessInformation.hProcess, MemoryStart, MemorySize, SearchPattern, PatternSize, WildCard));
}
__declspec(dllexport) bool __stdcall FillEx(HANDLE hProcess, LPVOID MemoryStart, DWORD MemorySize, PBYTE FillByte){

	unsigned int i;
	MEMORY_BASIC_INFORMATION MemInfo;
	ULONG_PTR ueNumberOfBytesRead;
	DWORD OldProtect;

	if(hProcess != NULL){
		VirtualQueryEx(hProcess, MemoryStart, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		OldProtect = MemInfo.AllocationProtect;
		VirtualProtectEx(hProcess, MemoryStart, MemorySize, PAGE_EXECUTE_READWRITE, &OldProtect);
		for(i = 0; i < MemorySize; i++){
			WriteProcessMemory(hProcess, MemoryStart, FillByte, 1, &ueNumberOfBytesRead);
			MemoryStart = (LPVOID)((ULONG_PTR)MemoryStart + 1);
		}
		VirtualProtectEx(hProcess, MemoryStart, MemorySize, MemInfo.AllocationProtect, &OldProtect);
		return(true);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall Fill(LPVOID MemoryStart, DWORD MemorySize, PBYTE FillByte){
	return(FillEx(dbgProcessInformation.hProcess, MemoryStart, MemorySize, FillByte));
}
__declspec(dllexport) bool __stdcall PatchEx(HANDLE hProcess, LPVOID MemoryStart, DWORD MemorySize, LPVOID ReplacePattern, DWORD ReplaceSize, bool AppendNOP, bool PrependNOP){

	unsigned int i,recalcSize;
	LPVOID lpMemoryStart = MemoryStart;
	MEMORY_BASIC_INFORMATION MemInfo;
	ULONG_PTR ueNumberOfBytesRead;
	BYTE FillByte = 0x90;	
	DWORD OldProtect;

	if(hProcess != NULL){
		VirtualQueryEx(hProcess, MemoryStart, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		OldProtect = MemInfo.AllocationProtect;
		VirtualProtectEx(hProcess, MemoryStart, MemorySize, PAGE_EXECUTE_READWRITE, &OldProtect);

		if(MemorySize - ReplaceSize != NULL){
			recalcSize = abs(MemorySize - ReplaceSize);
			if(AppendNOP){
				WriteProcessMemory(hProcess, MemoryStart, ReplacePattern, ReplaceSize, &ueNumberOfBytesRead);
				lpMemoryStart = (LPVOID)((ULONG_PTR)MemoryStart + ReplaceSize);
				for(i = 0; i < recalcSize; i++){
					WriteProcessMemory(hProcess, lpMemoryStart, &FillByte, 1, &ueNumberOfBytesRead);
					lpMemoryStart = (LPVOID)((ULONG_PTR)lpMemoryStart + 1);
				}
			}else if(PrependNOP){
				lpMemoryStart = MemoryStart;
				for(i = 0; i < recalcSize; i++){
					WriteProcessMemory(hProcess, lpMemoryStart, &FillByte, 1, &ueNumberOfBytesRead);
					lpMemoryStart = (LPVOID)((ULONG_PTR)lpMemoryStart + 1);
				}
				WriteProcessMemory(hProcess, lpMemoryStart, ReplacePattern, ReplaceSize, &ueNumberOfBytesRead);
			}else{
				WriteProcessMemory(hProcess, MemoryStart, ReplacePattern, ReplaceSize, &ueNumberOfBytesRead);
			}
		}else{
			WriteProcessMemory(hProcess, MemoryStart, ReplacePattern, ReplaceSize, &ueNumberOfBytesRead);
		}
		VirtualProtectEx(hProcess, MemoryStart, MemorySize, MemInfo.AllocationProtect, &OldProtect);
		return(true);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall Patch(LPVOID MemoryStart, DWORD MemorySize, LPVOID ReplacePattern, DWORD ReplaceSize, bool AppendNOP, bool PrependNOP){
	return(PatchEx(dbgProcessInformation.hProcess, MemoryStart, MemorySize, ReplacePattern, ReplaceSize, AppendNOP, PrependNOP));
}
__declspec(dllexport) bool __stdcall ReplaceEx(HANDLE hProcess, LPVOID MemoryStart, DWORD MemorySize, LPVOID SearchPattern, DWORD PatternSize, DWORD NumberOfRepetitions, LPVOID ReplacePattern, DWORD ReplaceSize, PBYTE WildCard){

	unsigned int i;
	ULONG_PTR ueNumberOfBytesRead;
	ULONG_PTR CurrentFoundPattern;
	LPVOID cMemoryStart = MemoryStart;
	DWORD cMemorySize = MemorySize;
	LPVOID lpReadMemory = VirtualAlloc(NULL, PatternSize, MEM_COMMIT, PAGE_READWRITE);

	CurrentFoundPattern = (ULONG_PTR)FindEx(hProcess, cMemoryStart, cMemorySize, SearchPattern, PatternSize, WildCard);
	NumberOfRepetitions--;
	while(CurrentFoundPattern != NULL && NumberOfRepetitions != NULL){
		if(ReadProcessMemory(hProcess, (LPVOID)CurrentFoundPattern, lpReadMemory, PatternSize, &ueNumberOfBytesRead)){
			for(i = 0; i < ReplaceSize; i++){
				if(memcmp((LPVOID)((ULONG_PTR)ReplacePattern + i), WildCard, 1) != NULL){
					RtlMoveMemory((LPVOID)((ULONG_PTR)lpReadMemory + i), (LPVOID)((ULONG_PTR)ReplacePattern + i), 1);
				}
			}
			PatchEx(hProcess, (LPVOID)CurrentFoundPattern, PatternSize, lpReadMemory, ReplaceSize, true, false);
		}
		cMemoryStart = (LPVOID)(CurrentFoundPattern + PatternSize);
		cMemorySize = (DWORD)((ULONG_PTR)MemoryStart + MemorySize - CurrentFoundPattern);
		CurrentFoundPattern = (ULONG_PTR)FindEx(hProcess, cMemoryStart, cMemorySize, SearchPattern, PatternSize, WildCard);
		NumberOfRepetitions--;
	}
	VirtualFree(lpReadMemory, NULL, MEM_RELEASE);
	if(NumberOfRepetitions != NULL){
		return(false);
	}else{
		return(true);
	}
}
__declspec(dllexport) bool __stdcall Replace(LPVOID MemoryStart, DWORD MemorySize, LPVOID SearchPattern, DWORD PatternSize, DWORD NumberOfRepetitions, LPVOID ReplacePattern, DWORD ReplaceSize, PBYTE WildCard){
	return(ReplaceEx(dbgProcessInformation.hProcess, MemoryStart, MemorySize, SearchPattern, PatternSize, NumberOfRepetitions, ReplacePattern, ReplaceSize, WildCard));
}
__declspec(dllexport) void* __stdcall GetDebugData(){
	return(&DBGEvent);
}
__declspec(dllexport) void* __stdcall GetTerminationData(){
	return(&TerminateDBGEvent);
}
__declspec(dllexport) long __stdcall GetExitCode(){
	return(ProcessExitCode);
}
__declspec(dllexport) long long __stdcall GetDebuggedDLLBaseAddress(){
	return((ULONG_PTR)engineDebuggingDLLBase);
}
__declspec(dllexport) long long __stdcall GetDebuggedFileBaseAddress(){
	return((ULONG_PTR)engineDebuggingMainModuleBase);
}
__declspec(dllexport) bool __stdcall GetRemoteString(HANDLE hProcess, LPVOID StringAddress, LPVOID StringStorage, int MaximumStringSize){

	MEMORY_BASIC_INFORMATION MemInfo; 
	ULONG_PTR ueNumberOfBytesRW = NULL;
	DWORD StringReadSize = NULL;

	if(MaximumStringSize == NULL){
		MaximumStringSize = 512;
	}
	VirtualQueryEx(hProcess, (LPVOID)StringAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
	if((int)((ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)MemInfo.RegionSize - (ULONG_PTR)StringAddress) < MaximumStringSize){
		StringReadSize = (DWORD)((ULONG_PTR)StringAddress - (ULONG_PTR)MemInfo.BaseAddress);
		VirtualQueryEx(hProcess, (LPVOID)((ULONG_PTR)StringAddress + (ULONG_PTR)MemInfo.RegionSize), &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		if(MemInfo.State == MEM_COMMIT){
			StringReadSize = MaximumStringSize;
		}
	}else{
		StringReadSize = MaximumStringSize;
	}
	RtlZeroMemory(StringStorage, MaximumStringSize);
	if(ReadProcessMemory(hProcess, (LPVOID)StringAddress, StringStorage, StringReadSize, &ueNumberOfBytesRW)){
		return(true);
	}else{
		return(false);
	}
}
__declspec(dllexport) long long __stdcall GetFunctionParameter(HANDLE hProcess, DWORD FunctionType, DWORD ParameterNumber, DWORD ParameterType){

	MEMORY_BASIC_INFORMATION MemInfo; 
	ULONG_PTR ueNumberOfBytesRW = NULL;
	ULONG_PTR StackReadBuffer = NULL;
	ULONG_PTR StackFinalBuffer = NULL;
	ULONG_PTR StackReadAddress = NULL;
	DWORD StackSecondReadSize = NULL;
	DWORD StackReadSize = 512;
	DWORD StringReadSize = NULL;
	bool ValueIsPointer = false;

	if(ParameterType == UE_PARAMETER_BYTE){
		StackReadSize = 1;
	}else if(ParameterType == UE_PARAMETER_WORD){
		StackReadSize = 2;
	}else if(ParameterType == UE_PARAMETER_DWORD){
		StackReadSize = 4;
	}else if(ParameterType == UE_PARAMETER_QWORD){
		StackReadSize = 8;
	}else{
		if(ParameterType >= UE_PARAMETER_PTR_BYTE && ParameterType <= UE_PARAMETER_UNICODE){
			ValueIsPointer = true;
		}
		if(ParameterType == UE_PARAMETER_PTR_BYTE){
			StackSecondReadSize = 1;
		}else if(ParameterType == UE_PARAMETER_PTR_WORD){
			StackSecondReadSize = 2;
		}else if(ParameterType == UE_PARAMETER_PTR_DWORD){
			StackSecondReadSize = 4;
		}else if(ParameterType == UE_PARAMETER_PTR_QWORD){
			StackSecondReadSize = 8;
		}else{
			StackSecondReadSize = 0;
		}
		StackReadSize = sizeof ULONG_PTR;
	}
	if(FunctionType >= UE_FUNCTION_STDCALL && FunctionType <= UE_FUNCTION_CCALL_CALL && FunctionType != UE_FUNCTION_FASTCALL_RET){
		StackReadAddress = (ULONG_PTR)GetContextData(UE_CSP);
		if(FunctionType != UE_FUNCTION_FASTCALL_CALL){
			StackReadAddress = StackReadAddress + (ParameterNumber * sizeof ULONG_PTR);
			if(FunctionType >= UE_FUNCTION_STDCALL_CALL){
				StackReadAddress = StackReadAddress - sizeof ULONG_PTR;
			}
		}else{
			if(ParameterNumber <= 4){
				if(!ValueIsPointer){
					if(ParameterNumber == 1){
						return((ULONG_PTR)GetContextData(UE_RCX));
					}else if(ParameterNumber == 2){
						return((ULONG_PTR)GetContextData(UE_RDX));
					}else if(ParameterNumber == 3){
						return((ULONG_PTR)GetContextData(UE_R8));
					}else if(ParameterNumber == 4){
						return((ULONG_PTR)GetContextData(UE_R9));
					}
				}else{
					if(ParameterNumber == 1){
						StackReadAddress = (ULONG_PTR)GetContextData(UE_RCX);
					}else if(ParameterNumber == 2){
						StackReadAddress = (ULONG_PTR)GetContextData(UE_RDX);
					}else if(ParameterNumber == 3){
						StackReadAddress = (ULONG_PTR)GetContextData(UE_R8);
					}else if(ParameterNumber == 4){
						StackReadAddress = (ULONG_PTR)GetContextData(UE_R9);
					}
				}
			}else{
				StackReadAddress = StackReadAddress + 0x20 + ((ParameterNumber - 4) * sizeof ULONG_PTR) - sizeof ULONG_PTR;
			}
		}
		if(ReadProcessMemory(hProcess, (LPVOID)StackReadAddress, &StackReadBuffer, sizeof ULONG_PTR, &ueNumberOfBytesRW)){
			if(!ValueIsPointer){
				RtlMoveMemory((LPVOID)((ULONG_PTR)&StackFinalBuffer + sizeof ULONG_PTR - StackReadSize), (LPVOID)((ULONG_PTR)&StackReadBuffer + sizeof ULONG_PTR - StackReadSize), StackReadSize);
			}else{
				StackReadAddress = StackReadBuffer;
				if(StackSecondReadSize > NULL){
					if(ReadProcessMemory(hProcess, (LPVOID)StackReadAddress, &StackReadBuffer, sizeof ULONG_PTR, &ueNumberOfBytesRW)){
						RtlMoveMemory((LPVOID)((ULONG_PTR)&StackFinalBuffer + sizeof ULONG_PTR - StackSecondReadSize), (LPVOID)((ULONG_PTR)&StackReadBuffer + sizeof ULONG_PTR - StackSecondReadSize), StackSecondReadSize);
					}else{
						return(-1);
					}
				}else{
					VirtualQueryEx(hProcess, (LPVOID)StackReadAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
					if((ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)MemInfo.RegionSize - StackReadAddress < 512){
						StringReadSize = (DWORD)((ULONG_PTR)StackReadAddress - (ULONG_PTR)MemInfo.BaseAddress);
						VirtualQueryEx(hProcess, (LPVOID)(StackReadAddress + (ULONG_PTR)MemInfo.RegionSize), &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
						if(MemInfo.State == MEM_COMMIT){
							StringReadSize = 512;
						}
					}
					RtlZeroMemory(&szParameterString, 512);
					if(ReadProcessMemory(hProcess, (LPVOID)StackReadAddress, &szParameterString, StringReadSize, &ueNumberOfBytesRW)){
						return((ULONG_PTR)&szParameterString);
					}else{
						return(-1);
					}
				}
			}
			return(StackFinalBuffer);
		}else{
			return(-1);
		}
	}
	return(-1);
}
__declspec(dllexport) long long __stdcall GetJumpDestinationEx(HANDLE hProcess, ULONG_PTR InstructionAddress, bool JustJumps){

	LPVOID ReadMemory;
	MEMORY_BASIC_INFORMATION MemInfo;
	ULONG_PTR ueNumberOfBytesRead = NULL;
	PMEMORY_CMP_HANDLER CompareMemory;
	ULONG_PTR TargetedAddress = NULL;
	DWORD CurrentInstructionSize;
	int ReadMemData = NULL;
	BYTE ReadByteData = NULL;

	if(hProcess != NULL){
		VirtualQueryEx(hProcess, (LPVOID)InstructionAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
		if(MemInfo.RegionSize > NULL){
			ReadMemory = VirtualAlloc(NULL, MAXIMUM_INSTRUCTION_SIZE, MEM_COMMIT, PAGE_READWRITE);
			if(ReadProcessMemory(hProcess, (LPVOID)InstructionAddress, ReadMemory, MAXIMUM_INSTRUCTION_SIZE, &ueNumberOfBytesRead)){
				CompareMemory = (PMEMORY_CMP_HANDLER)ReadMemory;
				CurrentInstructionSize = StaticLengthDisassemble(ReadMemory);
				if(CompareMemory->DataByte[0] == 0xE9 && CurrentInstructionSize == 5){
					RtlMoveMemory(&ReadMemData, (LPVOID)((ULONG_PTR)ReadMemory + 1), 4);
					TargetedAddress = ReadMemData + InstructionAddress + CurrentInstructionSize;
				}else if(CompareMemory->DataByte[0] == 0xEB && CurrentInstructionSize == 2){
					RtlMoveMemory(&ReadByteData, (LPVOID)((ULONG_PTR)ReadMemory + 1), 1);
					if(ReadByteData > 0x7F){
						ReadByteData = 0xFF - ReadByteData;
						ReadMemData = NULL - ReadByteData - CurrentInstructionSize + 1;
					}else{
						ReadMemData = ReadByteData;
					}
					TargetedAddress = InstructionAddress + ReadMemData + CurrentInstructionSize;
				}else if(CompareMemory->DataByte[0] == 0xE3 && CurrentInstructionSize == 2){
					RtlMoveMemory(&ReadByteData, (LPVOID)((ULONG_PTR)ReadMemory + 1), 1);
					if(ReadByteData > 0x7F){
						ReadByteData = 0xFF - ReadByteData;
						ReadMemData = NULL - ReadByteData - CurrentInstructionSize + 1;
					}else{
						ReadMemData = ReadByteData;
					}
					TargetedAddress = InstructionAddress + ReadMemData + CurrentInstructionSize;
				}else if(CompareMemory->DataByte[0] >= 0x71 && CompareMemory->DataByte[0] <= 0x7F && CurrentInstructionSize == 2){
					RtlMoveMemory(&ReadByteData, (LPVOID)((ULONG_PTR)ReadMemory + 1), 1);
					if(ReadByteData > 0x7F){
						ReadByteData = 0xFF - ReadByteData;
						ReadMemData = NULL - ReadByteData - CurrentInstructionSize + 1;
					}
					TargetedAddress = InstructionAddress + ReadMemData + CurrentInstructionSize;
				}else if(CompareMemory->DataByte[0] >= 0xE0 && CompareMemory->DataByte[0] <= 0xE2 && CurrentInstructionSize == 2){
					RtlMoveMemory(&ReadByteData, (LPVOID)((ULONG_PTR)ReadMemory + 1), 1);
					if(ReadByteData > 0x7F){
						ReadByteData = 0xFF - ReadByteData;
						ReadMemData = NULL - ReadByteData - CurrentInstructionSize + 1;
					}else{
						ReadMemData = ReadByteData;
					}
					TargetedAddress = InstructionAddress + ReadMemData + CurrentInstructionSize;
				}else if(CompareMemory->DataByte[0] == 0x0F && CompareMemory->DataByte[1] >= 0x81 && CompareMemory->DataByte[1] <= 0x8F && CurrentInstructionSize == 6){
					RtlMoveMemory(&ReadMemData, (LPVOID)((ULONG_PTR)ReadMemory + 2), 4);
					TargetedAddress = ReadMemData + InstructionAddress + CurrentInstructionSize;
				}else if(CompareMemory->DataByte[0] == 0x0F && CompareMemory->DataByte[1] >= 0x81 && CompareMemory->DataByte[1] <= 0x8F && CurrentInstructionSize == 4){
					RtlMoveMemory(&ReadMemData, (LPVOID)((ULONG_PTR)ReadMemory + 2), 2);
					TargetedAddress = ReadMemData + InstructionAddress + CurrentInstructionSize;
				}else if(CompareMemory->DataByte[0] == 0xE8 && CurrentInstructionSize == 5 && JustJumps == false){
					RtlMoveMemory(&ReadMemData, (LPVOID)((ULONG_PTR)ReadMemory + 1), 4);
					TargetedAddress = ReadMemData + InstructionAddress + CurrentInstructionSize;
				}else if(CompareMemory->DataByte[0] == 0xFF && CompareMemory->DataByte[1] == 0x25 && CurrentInstructionSize == 6 && JustJumps == false){
					RtlMoveMemory(&ReadMemData, (LPVOID)((ULONG_PTR)ReadMemory + 2), 4);
					TargetedAddress = ReadMemData;
					if(sizeof HANDLE == 8){
						TargetedAddress = TargetedAddress + InstructionAddress;
					}
				}else if(CompareMemory->DataByte[0] == 0xFF && CompareMemory->DataByte[1] == 0x15 && CurrentInstructionSize == 6 && JustJumps == false){
					RtlMoveMemory(&ReadMemData, (LPVOID)((ULONG_PTR)ReadMemory + 2), 4);
					TargetedAddress = ReadMemData;
					if(sizeof HANDLE == 8){
						TargetedAddress = TargetedAddress + InstructionAddress;
					}
				}
			}
			VirtualFree(ReadMemory, NULL, MEM_RELEASE);
			return((ULONG_PTR)TargetedAddress);
		}
		return(NULL);
	}else{
		CompareMemory = (PMEMORY_CMP_HANDLER)InstructionAddress;
		CurrentInstructionSize = StaticLengthDisassemble((LPVOID)InstructionAddress);
		if(CompareMemory->DataByte[0] == 0xE9 && CurrentInstructionSize == 5){
			RtlMoveMemory(&ReadMemData, (LPVOID)((ULONG_PTR)InstructionAddress + 1), 4);
			TargetedAddress = ReadMemData + InstructionAddress + CurrentInstructionSize;
		}else if(CompareMemory->DataByte[0] == 0xEB && CurrentInstructionSize == 2){
			RtlMoveMemory(&ReadByteData, (LPVOID)((ULONG_PTR)InstructionAddress + 1), 1);
			if(ReadByteData > 0x7F){
				ReadByteData = 0xFF - ReadByteData;
				ReadMemData = NULL - ReadByteData - CurrentInstructionSize + 1;
			}else{
				ReadMemData = ReadByteData;
			}
			TargetedAddress = InstructionAddress + ReadMemData + CurrentInstructionSize;
		}else if(CompareMemory->DataByte[0] == 0xE3 && CurrentInstructionSize == 2){
			RtlMoveMemory(&ReadByteData, (LPVOID)((ULONG_PTR)InstructionAddress + 1), 1);
			if(ReadByteData > 0x7F){
				ReadByteData = 0xFF - ReadByteData;
				ReadMemData = NULL - ReadByteData - CurrentInstructionSize + 1;
			}else{
				ReadMemData = ReadByteData;
			}
			TargetedAddress = InstructionAddress + ReadMemData + CurrentInstructionSize;
		}else if(CompareMemory->DataByte[0] >= 0x71 && CompareMemory->DataByte[0] <= 0x7F && CurrentInstructionSize == 2){
			RtlMoveMemory(&ReadByteData, (LPVOID)((ULONG_PTR)InstructionAddress + 1), 1);
			if(ReadByteData > 0x7F){
				ReadByteData = 0xFF - ReadByteData;
				ReadMemData = NULL - ReadByteData - CurrentInstructionSize + 1;
			}
			TargetedAddress = InstructionAddress + ReadMemData + CurrentInstructionSize;
		}else if(CompareMemory->DataByte[0] >= 0xE0 && CompareMemory->DataByte[0] <= 0xE2 && CurrentInstructionSize == 2){
			RtlMoveMemory(&ReadByteData, (LPVOID)((ULONG_PTR)InstructionAddress + 1), 1);
			if(ReadByteData > 0x7F){
				ReadByteData = 0xFF - ReadByteData;
				ReadMemData = NULL - ReadByteData - CurrentInstructionSize + 1;
			}else{
				ReadMemData = ReadByteData;
			}
			TargetedAddress = InstructionAddress + ReadMemData + CurrentInstructionSize;
		}else if(CompareMemory->DataByte[0] == 0x0F && CompareMemory->DataByte[1] >= 0x81 && CompareMemory->DataByte[1] <= 0x8F && CurrentInstructionSize == 6){
			RtlMoveMemory(&ReadMemData, (LPVOID)((ULONG_PTR)InstructionAddress + 2), 4);
			TargetedAddress = ReadMemData + InstructionAddress + CurrentInstructionSize;
		}else if(CompareMemory->DataByte[0] == 0x0F && CompareMemory->DataByte[1] >= 0x81 && CompareMemory->DataByte[1] <= 0x8F && CurrentInstructionSize == 4){
			RtlMoveMemory(&ReadMemData, (LPVOID)((ULONG_PTR)InstructionAddress + 2), 2);
			TargetedAddress = ReadMemData + InstructionAddress + CurrentInstructionSize;
		}else if(CompareMemory->DataByte[0] == 0xE8 && CurrentInstructionSize == 5 && JustJumps == false){
			RtlMoveMemory(&ReadMemData, (LPVOID)((ULONG_PTR)InstructionAddress + 1), 4);
			TargetedAddress = ReadMemData + InstructionAddress + CurrentInstructionSize;
		}else if(CompareMemory->DataByte[0] == 0xFF && CompareMemory->DataByte[1] == 0x25 && CurrentInstructionSize == 6 && JustJumps == false){
			RtlMoveMemory(&ReadMemData, (LPVOID)((ULONG_PTR)InstructionAddress + 2), 4);
			TargetedAddress = ReadMemData;
			if(sizeof HANDLE == 8){
				TargetedAddress = TargetedAddress + InstructionAddress;
			}
		}else if(CompareMemory->DataByte[0] == 0xFF && CompareMemory->DataByte[1] == 0x15 && CurrentInstructionSize == 6 && JustJumps == false){
			RtlMoveMemory(&ReadMemData, (LPVOID)((ULONG_PTR)InstructionAddress + 2), 4);
			TargetedAddress = ReadMemData;
			if(sizeof HANDLE == 8){
				TargetedAddress = TargetedAddress + InstructionAddress;
			}
		}
		return((ULONG_PTR)TargetedAddress);
	}
	return(NULL);
}
__declspec(dllexport) long long __stdcall GetJumpDestination(HANDLE hProcess, ULONG_PTR InstructionAddress){
	return((ULONG_PTR)GetJumpDestinationEx(hProcess, InstructionAddress, false));
}
__declspec(dllexport) bool __stdcall IsJumpGoingToExecuteEx(HANDLE hProcess, HANDLE hThread, ULONG_PTR InstructionAddress, ULONG_PTR RegFlags){

	ULONG_PTR ThreadCIP = NULL;
	DWORD ThreadEflags = NULL;
	char* DisassembledString;
	bool bCF = false;
	bool bPF = false;
	bool bAF = false;
	bool bZF = false;
	bool bSF = false;
	bool bTF = false;
	bool bIF = false;
	bool bDF = false;
	bool bOF = false;

	if(hProcess != NULL && hThread != NULL){
		if(InstructionAddress == NULL){
			ThreadCIP = (ULONG_PTR)GetContextDataEx(hThread, UE_CIP);
		}else{
			ThreadCIP = InstructionAddress;
		}
		if(RegFlags == NULL){
			ThreadEflags = (DWORD)GetContextDataEx(hThread, UE_EFLAGS);
		}else{
			ThreadEflags = (DWORD)RegFlags;
		}
		DisassembledString = (char*)DisassembleEx(hProcess, (LPVOID)ThreadCIP, true);
		if(DisassembledString != NULL){
			if(ThreadEflags & (1 << 0)){
				bCF = true;
			}
			if(ThreadEflags & (1 << 2)){
				bPF = true;
			}
			if(ThreadEflags & (1 << 4)){
				bAF = true;
			}
			if(ThreadEflags & (1 << 6)){
				bZF = true;
			}
			if(ThreadEflags & (1 << 7)){
				bSF = true;
			}
			if(ThreadEflags & (1 << 8)){
				bTF = true;
			}
			if(ThreadEflags & (1 << 9)){
				bIF = true;
			}
			if(ThreadEflags & (1 << 10)){
				bDF = true;
			}
			if(ThreadEflags & (1 << 11)){
				bOF = true;
			}
			if(lstrcmpiA(DisassembledString, "JMP") == NULL){
				return(true);
			}else if(lstrcmpiA(DisassembledString, "JA") == NULL){
				if(bCF == false && bZF == false){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JAE") == NULL){
				if(!bCF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JB") == NULL){
				if(bCF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JBE") == NULL){
				if(bCF == true || bZF == true){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JC") == NULL){
				if(bCF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JCXZ") == NULL){
				if((WORD)GetContextDataEx(hThread, UE_ECX) == NULL){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JECXZ") == NULL){
				if((DWORD)GetContextDataEx(hThread, UE_ECX) == NULL){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JRCXZ") == NULL){
				if((ULONG_PTR)GetContextDataEx(hThread, UE_RCX) == NULL){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JZ") == NULL){
				if(bZF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JNZ") == NULL){
				if(!bZF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JE") == NULL){
				if(bZF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JNE") == NULL){
				if(!bZF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JG") == NULL){
				if(bZF == false && bSF == bOF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JGE") == NULL){
				if(bSF == bOF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JL") == NULL){
				if(bSF != bOF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JLE") == NULL){
				if(bZF == true || bSF != bOF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JNA") == NULL){
				if(bCF == true || bZF == true){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JNAE") == NULL){
				if(bCF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JNB") == NULL){
				if(!bCF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JNBE") == NULL){
				if(bCF == false && bZF == false){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JNC") == NULL){
				if(!bCF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JNG") == NULL){
				if(bZF == true || bSF != bOF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JNGE") == NULL){
				if(bSF != bOF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JNL") == NULL){
				if(bSF == bOF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JNLE") == NULL){
				if(bZF == false && bSF == bOF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JNO") == NULL){
				if(!bOF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JNP") == NULL){
				if(!bPF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JNS") == NULL){
				if(!bSF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JO") == NULL){
				if(bOF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JP") == NULL){
				if(bPF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JPE") == NULL){
				if(bPF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JPO") == NULL){
				if(!bPF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JS") == NULL){
				if(bSF){
					return(true);
				}
			}else if(lstrcmpiA(DisassembledString, "JC") == NULL){
				if(bCF){
					return(true);
				}
			}
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall IsJumpGoingToExecute(){
	return(IsJumpGoingToExecuteEx(dbgProcessInformation.hProcess, dbgProcessInformation.hThread, NULL, NULL));
}
__declspec(dllexport) void __stdcall SetCustomHandler(DWORD ExceptionId, LPVOID CallBack){
	
	if(ExceptionId == UE_CH_BREAKPOINT){
		DBGCustomHandler->chBreakPoint = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_SINGLESTEP){
		DBGCustomHandler->chSingleStep = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_ACCESSVIOLATION){
		DBGCustomHandler->chAccessViolation = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_ILLEGALINSTRUCTION){
		DBGCustomHandler->chIllegalInstruction = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_NONCONTINUABLEEXCEPTION){
		DBGCustomHandler->chNonContinuableException = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_ARRAYBOUNDSEXCEPTION){
		DBGCustomHandler->chArrayBoundsException = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_FLOATDENORMALOPERAND){
		DBGCustomHandler->chFloatDenormalOperand = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_FLOATDEVIDEBYZERO){
		DBGCustomHandler->chFloatDevideByZero = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_INTEGERDEVIDEBYZERO){
		DBGCustomHandler->chIntegerDevideByZero = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_INTEGEROVERFLOW){
		DBGCustomHandler->chIntegerOverflow = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_PRIVILEGEDINSTRUCTION){
		DBGCustomHandler->chPrivilegedInstruction = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_PAGEGUARD){
		DBGCustomHandler->chPageGuard = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_EVERYTHINGELSE){
		DBGCustomHandler->chEverythingElse = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_CREATETHREAD){
		DBGCustomHandler->chCreateThread = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_EXITTHREAD){
		DBGCustomHandler->chExitThread = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_CREATEPROCESS){
		DBGCustomHandler->chCreateProcess = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_EXITPROCESS){
		DBGCustomHandler->chExitProcess = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_LOADDLL){
		DBGCustomHandler->chLoadDll = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_UNLOADDLL){
		DBGCustomHandler->chUnloadDll = (ULONG_PTR)CallBack;
	}else if(ExceptionId == UE_CH_OUTPUTDEBUGSTRING){
		DBGCustomHandler->chOutputDebugString = (ULONG_PTR)CallBack;
	}
}
__declspec(dllexport) void __stdcall ForceClose(){

	PPROCESS_ITEM_DATA hListProcessPtr = NULL;
	PTHREAD_ITEM_DATA hListThreadPtr = NULL;
	PLIBRARY_ITEM_DATA hListLibraryPtr = NULL;

	if(hListProcess != NULL){
		hListProcessPtr = (PPROCESS_ITEM_DATA)hListProcess;
		while(hListProcessPtr->hProcess != NULL){
			__try{
				EngineCloseHandle(hListProcessPtr->hFile);
				EngineCloseHandle(hListProcessPtr->hProcess);
			}__except(EXCEPTION_EXECUTE_HANDLER){

			}
			hListProcessPtr = (PPROCESS_ITEM_DATA)((ULONG_PTR)hListProcessPtr + sizeof PROCESS_ITEM_DATA);
		}
		RtlZeroMemory(hListProcess, MAX_DEBUG_DATA * sizeof PROCESS_ITEM_DATA);
	}
	if(hListThread != NULL){
		hListThreadPtr = (PTHREAD_ITEM_DATA)hListThread;
		while(hListThreadPtr->hThread != NULL){
			if(hListThreadPtr->hThread != (HANDLE)-1){
				__try{
					if(EngineCloseHandle(hListThreadPtr->hThread)){
						hListThreadPtr->hThread = NULL;
						hListThreadPtr->dwThreadId = NULL;
						hListThreadPtr->ThreadLocalBase = NULL;
						hListThreadPtr->ThreadStartAddress = NULL;
					}
				}__except(EXCEPTION_EXECUTE_HANDLER){
					hListThreadPtr->hThread = NULL;
					hListThreadPtr->dwThreadId = NULL;
					hListThreadPtr->ThreadLocalBase = NULL;
					hListThreadPtr->ThreadStartAddress = NULL;
				}
			}
			hListThreadPtr = (PTHREAD_ITEM_DATA)((ULONG_PTR)hListThreadPtr + sizeof THREAD_ITEM_DATA);
		}
		RtlZeroMemory(hListThread, MAX_DEBUG_DATA * sizeof THREAD_ITEM_DATA);
	}
	if(hListLibrary != NULL){
		hListLibraryPtr = (PLIBRARY_ITEM_DATA)hListLibrary;
		while(hListLibraryPtr->hFile != NULL){
			if(hListLibraryPtr->hFile != (HANDLE)-1){
				if(hListLibraryPtr->hFileMappingView != NULL){
					UnmapViewOfFile(hListLibraryPtr->hFileMappingView);
					__try{
						EngineCloseHandle(hListLibraryPtr->hFileMapping);
					}__except(EXCEPTION_EXECUTE_HANDLER){

					}
				}
				__try{
					EngineCloseHandle(hListLibraryPtr->hFile);
				}__except(EXCEPTION_EXECUTE_HANDLER){

				}
			}
			hListLibraryPtr = (PLIBRARY_ITEM_DATA)((ULONG_PTR)hListLibraryPtr + sizeof LIBRARY_ITEM_DATA);
		}
		RtlZeroMemory(hListLibrary, MAX_DEBUG_DATA * sizeof LIBRARY_ITEM_DATA);
	}
	if(!engineProcessIsNowDetached){
		StopDebug();
	}
	RtlZeroMemory(&dbgProcessInformation, sizeof PROCESS_INFORMATION);
	if(engineDebuggingDLL){
		DeleteFileA(szDebuggerName);
		DeleteFileA(szReserveModuleName);
	}
	engineDebuggingDLL = false;
	DebugExeFileEntryPointCallBack = NULL;
}
__declspec(dllexport) void __stdcall StepInto(LPVOID StepCallBack){

	ULONG_PTR ueContext = NULL;

	ueContext = (ULONG_PTR)GetContextData(UE_EFLAGS);
	if(!(ueContext & 0x100)){
		ueContext = ueContext ^ 0x100;
	}
	SetContextData(UE_EFLAGS, ueContext);
	engineStepActive = true;
	engineStepCallBack = StepCallBack;
}
__declspec(dllexport) void __stdcall StepOver(LPVOID StepCallBack){

	ULONG_PTR ueCurrentPosition = NULL;

#if !defined(_WIN64)
	ueCurrentPosition = (ULONG_PTR)GetContextData(UE_EIP);
#else
	ueCurrentPosition = GetContextData(UE_RIP);
#endif
	ueCurrentPosition = ueCurrentPosition + LengthDisassemble((LPVOID)ueCurrentPosition);
	SetBPX(ueCurrentPosition, UE_SINGLESHOOT, StepCallBack);
}

__declspec(dllexport) void __stdcall SingleStep(DWORD StepCount, LPVOID StepCallBack){

	ULONG_PTR ueContext = NULL;

	ueContext = (ULONG_PTR)GetContextData(UE_EFLAGS);
	if(!(ueContext & 0x100)){
		ueContext = ueContext ^ 0x100;
	}
	SetContextData(UE_EFLAGS, ueContext);
	engineStepActive = true;
	engineStepCount = (int)StepCount;
	engineStepCallBack = StepCallBack;
	engineStepCount--;
}
__declspec(dllexport) bool __stdcall SetHardwareBreakPoint(ULONG_PTR bpxAddress, DWORD IndexOfRegister, DWORD bpxType, DWORD bpxSize, LPVOID bpxCallBack){

	ULONG_PTR HardwareBPX = NULL;

	if(bpxSize == UE_HARDWARE_SIZE_2){
		if(bpxAddress % 2 != 0){
			return(false);
		}
	}else if(bpxSize == UE_HARDWARE_SIZE_4){
		if(bpxAddress % 4 != 0){
			return(false);
		}
	}

	if(IndexOfRegister == NULL){
		if(!DebugRegister0.DrxEnabled){
			IndexOfRegister = UE_DR0;
		}else if(!DebugRegister1.DrxEnabled){
			IndexOfRegister = UE_DR1;
		}else if(!DebugRegister2.DrxEnabled){
			IndexOfRegister = UE_DR2;		
		}else if(!DebugRegister3.DrxEnabled){
			IndexOfRegister = UE_DR3;
		}else{
			IndexOfRegister = UE_DR3;
		}
	}

	if(IndexOfRegister == UE_DR0){
		DebugRegister0.DrxExecution = false;
		DebugRegister0.DrxBreakPointType = bpxType;
		DebugRegister0.DrxBreakPointSize = bpxSize;
		HardwareBPX = (ULONG_PTR)GetContextData(UE_DR7);
		HardwareBPX = HardwareBPX | (1 << 0);
		HardwareBPX = HardwareBPX &~ (1 << 1);
		if(bpxType == UE_HARDWARE_EXECUTE){
			DebugRegister0.DrxExecution = true;
			HardwareBPX = HardwareBPX &~ (1 << 17);
			HardwareBPX = HardwareBPX &~ (1 << 16);
		}else if(bpxType == UE_HARDWARE_WRITE){
			HardwareBPX = HardwareBPX &~ (1 << 17);
			HardwareBPX = HardwareBPX | (1 << 16);
		}else if(bpxType == UE_HARDWARE_READWRITE){
			HardwareBPX = HardwareBPX | (1 << 17);
			HardwareBPX = HardwareBPX | (1 << 16);
		}
		if(bpxSize == UE_HARDWARE_SIZE_1 || bpxType == UE_HARDWARE_EXECUTE){
			HardwareBPX = HardwareBPX &~ (1 << 19);
			HardwareBPX = HardwareBPX &~ (1 << 18);
		}else if(bpxSize == UE_HARDWARE_SIZE_2){
			HardwareBPX = HardwareBPX &~ (1 << 19);
			HardwareBPX = HardwareBPX | (1 << 18);
		}else if(bpxSize == UE_HARDWARE_SIZE_4){
			HardwareBPX = HardwareBPX | (1 << 19);
			HardwareBPX = HardwareBPX | (1 << 18);
		}
		HardwareBPX = HardwareBPX | (1 << 10);
		HardwareBPX = HardwareBPX &~ (1 << 11);
		HardwareBPX = HardwareBPX &~ (1 << 12);
		HardwareBPX = HardwareBPX &~ (1 << 14);
		HardwareBPX = HardwareBPX &~ (1 << 15);
		SetContextData(UE_DR0, (ULONG_PTR)bpxAddress);
		SetContextData(UE_DR7, HardwareBPX);
		DebugRegister0.DrxEnabled = true;
		DebugRegister0.DrxBreakAddress = (ULONG_PTR)bpxAddress;
		DebugRegister0.DrxCallBack = (ULONG_PTR)bpxCallBack;
		return(true);
	}else if(IndexOfRegister == UE_DR1){
		DebugRegister1.DrxExecution = false;
		DebugRegister1.DrxBreakPointType = bpxType;
		DebugRegister1.DrxBreakPointSize = bpxSize;
		HardwareBPX = (ULONG_PTR)GetContextData(UE_DR7);
		HardwareBPX = HardwareBPX | (1 << 2);
		HardwareBPX = HardwareBPX &~ (1 << 3);
		if(bpxType == UE_HARDWARE_EXECUTE){
			DebugRegister1.DrxExecution = true;
			HardwareBPX = HardwareBPX &~ (1 << 21);
			HardwareBPX = HardwareBPX &~ (1 << 20);
		}else if(bpxType == UE_HARDWARE_WRITE){
			HardwareBPX = HardwareBPX &~ (1 << 21);
			HardwareBPX = HardwareBPX | (1 << 20);
		}else if(bpxType == UE_HARDWARE_READWRITE){
			HardwareBPX = HardwareBPX | (1 << 21);
			HardwareBPX = HardwareBPX | (1 << 20);
		}
		if(bpxSize == UE_HARDWARE_SIZE_1 || bpxType == UE_HARDWARE_EXECUTE){
			HardwareBPX = HardwareBPX &~ (1 << 23);
			HardwareBPX = HardwareBPX &~ (1 << 22);
		}else if(bpxSize == UE_HARDWARE_SIZE_2){
			HardwareBPX = HardwareBPX &~ (1 << 23);
			HardwareBPX = HardwareBPX | (1 << 22);
		}else if(bpxSize == UE_HARDWARE_SIZE_4){
			HardwareBPX = HardwareBPX | (1 << 23);
			HardwareBPX = HardwareBPX | (1 << 22);
		}
		HardwareBPX = HardwareBPX | (1 << 10);
		HardwareBPX = HardwareBPX &~ (1 << 11);
		HardwareBPX = HardwareBPX &~ (1 << 12);
		HardwareBPX = HardwareBPX &~ (1 << 14);
		HardwareBPX = HardwareBPX &~ (1 << 15);
		SetContextData(UE_DR1, (ULONG_PTR)bpxAddress);
		SetContextData(UE_DR7, HardwareBPX);
		DebugRegister1.DrxEnabled = true;
		DebugRegister1.DrxBreakAddress = (ULONG_PTR)bpxAddress;
		DebugRegister1.DrxCallBack = (ULONG_PTR)bpxCallBack;
		return(true);
	}else if(IndexOfRegister == UE_DR2){
		DebugRegister2.DrxExecution = false;
		DebugRegister2.DrxBreakPointType = bpxType;
		DebugRegister2.DrxBreakPointSize = bpxSize;
		HardwareBPX = (ULONG_PTR)GetContextData(UE_DR7);
		HardwareBPX = HardwareBPX | (1 << 4);
		HardwareBPX = HardwareBPX &~ (1 << 5);
		if(bpxType == UE_HARDWARE_EXECUTE){
			DebugRegister2.DrxExecution = true;
			HardwareBPX = HardwareBPX &~ (1 << 25);
			HardwareBPX = HardwareBPX &~ (1 << 24);
		}else if(bpxType == UE_HARDWARE_WRITE){
			HardwareBPX = HardwareBPX &~ (1 << 25);
			HardwareBPX = HardwareBPX | (1 << 24);
		}else if(bpxType == UE_HARDWARE_READWRITE){
			HardwareBPX = HardwareBPX | (1 << 25);
			HardwareBPX = HardwareBPX | (1 << 24);
		}
		if(bpxSize == UE_HARDWARE_SIZE_1 || bpxType == UE_HARDWARE_EXECUTE){
			HardwareBPX = HardwareBPX &~ (1 << 27);
			HardwareBPX = HardwareBPX &~ (1 << 26);
		}else if(bpxSize == UE_HARDWARE_SIZE_2){
			HardwareBPX = HardwareBPX &~ (1 << 27);
			HardwareBPX = HardwareBPX | (1 << 26);
		}else if(bpxSize == UE_HARDWARE_SIZE_4){
			HardwareBPX = HardwareBPX | (1 << 27);
			HardwareBPX = HardwareBPX | (1 << 26);
		}
		HardwareBPX = HardwareBPX | (1 << 10);
		HardwareBPX = HardwareBPX &~ (1 << 11);
		HardwareBPX = HardwareBPX &~ (1 << 12);
		HardwareBPX = HardwareBPX &~ (1 << 14);
		HardwareBPX = HardwareBPX &~ (1 << 15);
		SetContextData(UE_DR2, (ULONG_PTR)bpxAddress);
		SetContextData(UE_DR7, HardwareBPX);
		DebugRegister2.DrxEnabled = true;
		DebugRegister2.DrxBreakAddress = (ULONG_PTR)bpxAddress;
		DebugRegister2.DrxCallBack = (ULONG_PTR)bpxCallBack;
		return(true);
	}else if(IndexOfRegister == UE_DR3){
		DebugRegister3.DrxExecution = false;
		DebugRegister3.DrxBreakPointType = bpxType;
		DebugRegister3.DrxBreakPointSize = bpxSize;
		HardwareBPX = (ULONG_PTR)GetContextData(UE_DR7);
		HardwareBPX = HardwareBPX | (1 << 6);
		HardwareBPX = HardwareBPX &~ (1 << 7);
		if(bpxType == UE_HARDWARE_EXECUTE){
			DebugRegister3.DrxExecution = true;
			HardwareBPX = HardwareBPX &~ (1 << 29);
			HardwareBPX = HardwareBPX &~ (1 << 28);
		}else if(bpxType == UE_HARDWARE_WRITE){
			HardwareBPX = HardwareBPX &~ (1 << 29);
			HardwareBPX = HardwareBPX | (1 << 28);
		}else if(bpxType == UE_HARDWARE_READWRITE){
			HardwareBPX = HardwareBPX | (1 << 29);
			HardwareBPX = HardwareBPX | (1 << 28);
		}
		if(bpxSize == UE_HARDWARE_SIZE_1 || bpxType == UE_HARDWARE_EXECUTE){
			HardwareBPX = HardwareBPX &~ (1 << 31);
			HardwareBPX = HardwareBPX &~ (1 << 30);
		}else if(bpxSize == UE_HARDWARE_SIZE_2){
			HardwareBPX = HardwareBPX &~ (1 << 31);
			HardwareBPX = HardwareBPX | (1 << 30);
		}else if(bpxSize == UE_HARDWARE_SIZE_4){
			HardwareBPX = HardwareBPX | (1 << 31);
			HardwareBPX = HardwareBPX | (1 << 30);
		}
		HardwareBPX = HardwareBPX | (1 << 10);
		HardwareBPX = HardwareBPX &~ (1 << 11);
		HardwareBPX = HardwareBPX &~ (1 << 12);
		HardwareBPX = HardwareBPX &~ (1 << 14);
		HardwareBPX = HardwareBPX &~ (1 << 15);
		SetContextData(UE_DR3, (ULONG_PTR)bpxAddress);
		SetContextData(UE_DR7, HardwareBPX);
		DebugRegister3.DrxEnabled = true;
		DebugRegister3.DrxBreakAddress = (ULONG_PTR)bpxAddress;
		DebugRegister3.DrxCallBack = (ULONG_PTR)bpxCallBack;
		return(true);
	}else{
		return(false);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall DeleteHardwareBreakPoint(DWORD IndexOfRegister){

	ULONG_PTR HardwareBPX = NULL;
	ULONG_PTR bpxAddress = NULL;

	if(IndexOfRegister == UE_DR0){
		HardwareBPX = (ULONG_PTR)GetContextData(UE_DR7);
		HardwareBPX = HardwareBPX &~ (1 << 0);
		HardwareBPX = HardwareBPX &~ (1 << 1);
		SetContextData(UE_DR0, (ULONG_PTR)bpxAddress);
		SetContextData(UE_DR7, HardwareBPX);
		DebugRegister0.DrxEnabled = false;
		DebugRegister0.DrxBreakAddress = NULL;
		DebugRegister0.DrxCallBack = NULL;
		return(true);
	}else if(IndexOfRegister == UE_DR1){
		HardwareBPX = (ULONG_PTR)GetContextData(UE_DR7);
		HardwareBPX = HardwareBPX &~ (1 << 2);
		HardwareBPX = HardwareBPX &~ (1 << 3);
		SetContextData(UE_DR1, (ULONG_PTR)bpxAddress);
		SetContextData(UE_DR7, HardwareBPX);
		DebugRegister1.DrxEnabled = false;
		DebugRegister1.DrxBreakAddress = NULL;
		DebugRegister1.DrxCallBack = NULL;
		return(true);
	}else if(IndexOfRegister == UE_DR2){
		HardwareBPX = (ULONG_PTR)GetContextData(UE_DR7);
		HardwareBPX = HardwareBPX &~ (1 << 4);
		HardwareBPX = HardwareBPX &~ (1 << 5);
		SetContextData(UE_DR2, (ULONG_PTR)bpxAddress);
		SetContextData(UE_DR7, HardwareBPX);
		DebugRegister2.DrxEnabled = false;
		DebugRegister2.DrxBreakAddress = NULL;
		DebugRegister2.DrxCallBack = NULL;
		return(true);
	}else if(IndexOfRegister == UE_DR3){
		HardwareBPX = (ULONG_PTR)GetContextData(UE_DR7);
		HardwareBPX = HardwareBPX &~ (1 << 6);
		HardwareBPX = HardwareBPX &~ (1 << 7);
		SetContextData(UE_DR3, (ULONG_PTR)bpxAddress);
		SetContextData(UE_DR7, HardwareBPX);
		DebugRegister3.DrxEnabled = false;
		DebugRegister3.DrxBreakAddress = NULL;
		DebugRegister3.DrxCallBack = NULL;
		return(true);
	}else{
		return(false);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall RemoveAllBreakPoints(DWORD RemoveOption){

	int i = 0;
	int CurrentBreakPointSetCount = -1;

	if(RemoveOption == UE_OPTION_REMOVEALL){
		for(i = BreakPointSetCount - 1; i >= 0; i--){
			if(BreakPointBuffer[i].BreakPointType == UE_BREAKPOINT){
				DeleteBPX((ULONG_PTR)BreakPointBuffer[i].BreakPointAddress);
			}else if(BreakPointBuffer[i].BreakPointType >= UE_MEMORY && BreakPointBuffer[i].BreakPointType <= UE_MEMORY_WRITE){
				RemoveMemoryBPX((ULONG_PTR)BreakPointBuffer[i].BreakPointAddress, BreakPointBuffer[i].BreakPointSize);
			}else if(CurrentBreakPointSetCount == -1 && BreakPointBuffer[i].BreakPointActive != UE_BPXREMOVED){
				CurrentBreakPointSetCount = BreakPointSetCount;
			}
			RtlZeroMemory(&BreakPointBuffer[i], sizeof BreakPointDetail);
		}
		DeleteHardwareBreakPoint(UE_DR0);
		DeleteHardwareBreakPoint(UE_DR1);
		DeleteHardwareBreakPoint(UE_DR2);
		DeleteHardwareBreakPoint(UE_DR3);
		BreakPointSetCount = 0;
		return(true);
	}else if(RemoveOption == UE_OPTION_DISABLEALL){
		for(i = BreakPointSetCount - 1; i >= 0; i--){
			if(BreakPointBuffer[i].BreakPointType == UE_BREAKPOINT && BreakPointBuffer[i].BreakPointActive == UE_BPXACTIVE){
				DisableBPX((ULONG_PTR)BreakPointBuffer[i].BreakPointAddress);
			}else if(BreakPointBuffer[i].BreakPointType >= UE_MEMORY && BreakPointBuffer[i].BreakPointType <= UE_MEMORY_WRITE){
				RemoveMemoryBPX((ULONG_PTR)BreakPointBuffer[i].BreakPointAddress, BreakPointBuffer[i].BreakPointSize);
				RtlZeroMemory(&BreakPointBuffer[i], sizeof BreakPointDetail);
			}
		}
		return(true);
	}else if(RemoveOption == UE_OPTION_REMOVEALLDISABLED){
		for(i = BreakPointSetCount - 1; i >= 0; i--){
			if(BreakPointBuffer[i].BreakPointType == UE_BREAKPOINT && BreakPointBuffer[i].BreakPointActive == UE_BPXINACTIVE){
				DeleteBPX((ULONG_PTR)BreakPointBuffer[i].BreakPointAddress);
			}else if(CurrentBreakPointSetCount == -1 && BreakPointBuffer[i].BreakPointActive != UE_BPXREMOVED){
				CurrentBreakPointSetCount = BreakPointSetCount;
			}
		}
		if(CurrentBreakPointSetCount == -1){
			BreakPointSetCount = 0;
		}else{
			BreakPointSetCount = CurrentBreakPointSetCount;
		}
		return(true);
	}else if(RemoveOption == UE_OPTION_REMOVEALLENABLED){
		for(i = BreakPointSetCount - 1; i >= 0; i--){
			if(BreakPointBuffer[i].BreakPointType == UE_BREAKPOINT && BreakPointBuffer[i].BreakPointActive == UE_BPXACTIVE){
				DeleteBPX((ULONG_PTR)BreakPointBuffer[i].BreakPointAddress);
			}else if(CurrentBreakPointSetCount == -1 && BreakPointBuffer[i].BreakPointActive != UE_BPXREMOVED){
				CurrentBreakPointSetCount = BreakPointSetCount;
			}
		}
		if(CurrentBreakPointSetCount == -1){
			BreakPointSetCount = 0;
		}else{
			BreakPointSetCount = CurrentBreakPointSetCount;
		}
		return(true);
	}
	return(false);
}
__declspec(dllexport) void* __stdcall GetProcessInformation(){
	return(&dbgProcessInformation);
}
__declspec(dllexport) void* __stdcall GetStartupInformation(){
	return(&dbgStartupInfo);
}
__declspec(dllexport) void __stdcall DebugLoop(){

	int i = NULL;
	int j = NULL;
	int k = NULL;
	bool FirstBPX = true;
	bool ResetBPX = false;
	bool BreakDBG = false;
	bool ResetHwBPX = false;
	bool CompareResult = false;
	bool SecondChance = false;
	ULONG_PTR CmpValue1 = NULL;
	ULONG_PTR CmpValue2 = NULL;
	bool hListProcessFirst = true;
	bool hListThreadFirst = true;
	bool hListLibraryFirst = true;
	PPROCESS_ITEM_DATA hListProcessPtr = NULL;
	PTHREAD_ITEM_DATA hListThreadPtr = NULL;
	PLIBRARY_ITEM_DATA hListLibraryPtr = NULL;
	PLIBRARY_ITEM_DATA hLoadedLibData = NULL;
	PLIBRARY_BREAK_DATA ptrLibrarianData = NULL;
	typedef void(__stdcall *fCustomHandler)(void* SpecialDBG);
	typedef void(__stdcall *fCustomBreakPoint)(void);
	typedef void(__stdcall *fFindOEPHandler)(LPPROCESS_INFORMATION fProcessInfo, LPVOID fCallBack);
	fCustomHandler myCustomHandler;
	fCustomBreakPoint myCustomBreakPoint;
	fFindOEPHandler myFindOEPHandler;
	ULONG_PTR MemoryBpxCallBack = 0;
	DWORD ResetBPXSize = 0;
	ULONG_PTR ResetBPXAddressTo =  0;
	int MaximumBreakPoints = 0;
	ULONG_PTR NumberOfBytesReadWritten = 0;
	MEMORY_BASIC_INFORMATION MemInfo; 
	HANDLE hActiveThread;
	CONTEXT myDBGContext;
	DWORD OldProtect;
	DWORD NewProtect;
	DWORD DebugRegisterXId = NULL;
	HARDWARE_DATA DebugRegisterX;
	char DLLDebugFileName[512];
	ULONG_PTR DLLPatchAddress;
	HANDLE hFileMapping;
	LPVOID hFileMappingView;
	LPVOID DBGEntryPoint;
	bool MemoryBpxFound = false;
	char* szTranslatedNativeName;

	DBGFileHandle = NULL;
	DBGCode = DBG_CONTINUE;
	engineFakeDLLHandle = NULL;
	DebugRegister0.DrxEnabled = false;
	DebugRegister1.DrxEnabled = false;
	DebugRegister2.DrxEnabled = false;
	DebugRegister3.DrxEnabled = false;
	engineProcessIsNowDetached = false;
	engineResumeProcessIfNoThreadIsActive = false;
	RtlZeroMemory(&DBGEvent, sizeof DEBUG_EVENT);
	RtlZeroMemory(&TerminateDBGEvent, sizeof DEBUG_EVENT);
	RtlZeroMemory(&DLLDebugFileName, 512);
	while(!BreakDBG){
		WaitForDebugEvent(&DBGEvent, engineWaitForDebugEventTimeOut);
		if(engineFindOEPCallBack != NULL){
			myFindOEPHandler = (fFindOEPHandler)engineFindOEPCallBack;
			engineFindOEPCallBack = NULL;
			__try{
				myFindOEPHandler(&dbgProcessInformation, engineFindOEPUserCallBack);
			}__except(EXCEPTION_EXECUTE_HANDLER){

			}
		}
		if(DBGEvent.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT){
			if(DBGFileHandle == NULL){
				DBGEntryPoint = DBGEvent.u.CreateProcessInfo.lpStartAddress;
				DBGFileHandle = DBGEvent.u.CreateProcessInfo.hFile;
				engineDebuggingMainModuleBase = (ULONG_PTR)DBGEvent.u.CreateProcessInfo.lpBaseOfImage;
				if(engineAttachedToProcess){
					dbgProcessInformation.hProcess = DBGEvent.u.CreateProcessInfo.hProcess;
					dbgProcessInformation.hThread = DBGEvent.u.CreateProcessInfo.hThread;
					dbgProcessInformation.dwThreadId = NULL;
					if(engineAttachedProcessDebugInfo != NULL){
						RtlMoveMemory(engineAttachedProcessDebugInfo, &dbgProcessInformation, sizeof PROCESS_INFORMATION);
					}
				}
				if(engineDebuggingDLL){
					#if defined(_WIN64)
						DLLPatchAddress = (ULONG_PTR)DBGEvent.u.CreateProcessInfo.lpBaseOfImage;
						DLLPatchAddress = (ULONG_PTR)DLLPatchAddress + UE_MODULEx64;
					#else
						DLLPatchAddress = (ULONG_PTR)DBGEvent.u.CreateProcessInfo.lpBaseOfImage;
						DLLPatchAddress = (ULONG_PTR)DLLPatchAddress + UE_MODULEx86;						
					#endif
					if(!WriteProcessMemory(DBGEvent.u.CreateProcessInfo.hProcess, (LPVOID)DLLPatchAddress, engineDebuggingDLLFullFileName, lstrlenA(engineDebuggingDLLFullFileName), &NumberOfBytesReadWritten)){
						StopDebug();
					}
				}
				if(hListProcess == NULL){
					hListProcess = VirtualAlloc(NULL, MAX_DEBUG_DATA * sizeof PROCESS_ITEM_DATA, MEM_COMMIT, PAGE_READWRITE);
				}else{
					if(hListProcessFirst == true){
						RtlZeroMemory(hListProcess, MAX_DEBUG_DATA * sizeof PROCESS_ITEM_DATA);
					}
				}
				if(hListThread == NULL){
					hListThread = VirtualAlloc(NULL, MAX_DEBUG_DATA * sizeof THREAD_ITEM_DATA, MEM_COMMIT, PAGE_READWRITE);
				}else{
					if(hListThreadFirst == true){
						RtlZeroMemory(hListThread, MAX_DEBUG_DATA * sizeof THREAD_ITEM_DATA);
					}
				}
				hListProcessPtr = (PPROCESS_ITEM_DATA)hListProcess;
				hListProcessPtr->hFile = DBGEvent.u.CreateProcessInfo.hFile;
				hListProcessPtr->hProcess = DBGEvent.u.CreateProcessInfo.hProcess;
				hListProcessPtr->hThread = DBGEvent.u.CreateProcessInfo.hThread;
				hListProcessPtr->dwProcessId = DBGEvent.dwProcessId;
				hListProcessPtr->dwThreadId = DBGEvent.dwThreadId;
				hListProcessPtr->BaseOfImage = (void*)DBGEvent.u.CreateProcessInfo.lpBaseOfImage;
				hListProcessPtr->ThreadStartAddress = (void*)DBGEvent.u.CreateProcessInfo.lpStartAddress;
				hListProcessPtr->ThreadLocalBase = (void*)DBGEvent.u.CreateProcessInfo.lpThreadLocalBase;

				hListThreadPtr = (PTHREAD_ITEM_DATA)hListThread;
				hListThreadPtr->dwThreadId = DBGEvent.dwThreadId;
				hListThreadPtr->hThread = DBGEvent.u.CreateThread.hThread;
				hListThreadPtr->ThreadStartAddress = (void*)DBGEvent.u.CreateThread.lpStartAddress;
				hListThreadPtr->ThreadLocalBase = (void*)DBGEvent.u.CreateThread.lpThreadLocalBase;
				hListThreadFirst = false;
			}else{
				hListProcessPtr = (PPROCESS_ITEM_DATA)hListProcess;
				while(hListProcessPtr->hProcess != NULL){
					hListProcessPtr = (PPROCESS_ITEM_DATA)((ULONG_PTR)hListProcessPtr + sizeof PROCESS_ITEM_DATA);
				}
				if(hListProcessPtr->hProcess == NULL){
					hListProcessPtr->hFile = DBGEvent.u.CreateProcessInfo.hFile;
					hListProcessPtr->hProcess = DBGEvent.u.CreateProcessInfo.hProcess;
					hListProcessPtr->hThread = DBGEvent.u.CreateProcessInfo.hThread;
					hListProcessPtr->dwProcessId = DBGEvent.dwProcessId;
					hListProcessPtr->dwThreadId = DBGEvent.dwThreadId;
					hListProcessPtr->BaseOfImage = (void*)DBGEvent.u.CreateProcessInfo.lpBaseOfImage;
					hListProcessPtr->ThreadStartAddress = (void*)DBGEvent.u.CreateProcessInfo.lpStartAddress;
					hListProcessPtr->ThreadLocalBase = (void*)DBGEvent.u.CreateProcessInfo.lpThreadLocalBase;
					hListProcessFirst = false;
				}
			}
			if(DBGCustomHandler->chCreateProcess != NULL){
				myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chCreateProcess);
				__try{
					myCustomHandler(&DBGEvent.u.CreateProcessInfo);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					DBGCustomHandler->chCreateProcess = NULL;
				}
			}
		}else if(DBGEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT){
			ProcessExitCode = DBGEvent.u.ExitProcess.dwExitCode;	
			if(DBGEvent.dwProcessId == dbgProcessInformation.dwProcessId){
				DBGCode = DBG_CONTINUE;
				BreakDBG = true;
			}else{
				DBGCode = DBG_CONTINUE;
			}
			if(DBGCustomHandler->chExitProcess != NULL){
				myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chExitProcess);
				__try{
					myCustomHandler(&DBGEvent.u.ExitProcess);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					DBGCustomHandler->chExitProcess = NULL;
				}
			}
		}else if(DBGEvent.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT){
			if(hListThread == NULL){
				hListThread = VirtualAlloc(NULL, MAX_DEBUG_DATA * sizeof THREAD_ITEM_DATA, MEM_COMMIT, PAGE_READWRITE);
			}
			hListThreadPtr = (PTHREAD_ITEM_DATA)hListThread;
			__try{
				while(hListThreadPtr->hThread != NULL){
					hListThreadPtr = (PTHREAD_ITEM_DATA)((ULONG_PTR)hListThreadPtr + sizeof THREAD_ITEM_DATA);
				}
				hListThreadPtr->dwThreadId = DBGEvent.dwThreadId;
				hListThreadPtr->hThread = DBGEvent.u.CreateThread.hThread;
				hListThreadPtr->ThreadStartAddress = (void*)DBGEvent.u.CreateThread.lpStartAddress;
				hListThreadPtr->ThreadLocalBase = (void*)DBGEvent.u.CreateThread.lpThreadLocalBase;
			}__except(EXCEPTION_EXECUTE_HANDLER){

			}
			if(DBGCustomHandler->chCreateThread != NULL){
				myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chCreateThread);
				__try{
					myCustomHandler(&DBGEvent.u.CreateThread);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					DBGCustomHandler->chCreateThread = NULL;
				}
			}
		}else if(DBGEvent.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT){
			if(DBGCustomHandler->chExitThread != NULL){
				myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chExitThread);
				__try{
					myCustomHandler(&DBGEvent.u.ExitThread);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					DBGCustomHandler->chExitThread = NULL;
				}
			}
			if(engineExitThreadOneShootCallBack != NULL){
				myCustomHandler = (fCustomHandler)(engineExitThreadOneShootCallBack);
				__try{
					myCustomHandler(&DBGEvent.u.ExitThread);
				}__except(EXCEPTION_EXECUTE_HANDLER){

				}
				engineExitThreadOneShootCallBack = NULL;
			}
			hListThreadPtr = (PTHREAD_ITEM_DATA)hListThread;
			while(hListThreadPtr->hThread != NULL && hListThreadPtr->dwThreadId != DBGEvent.dwThreadId){
				hListThreadPtr = (PTHREAD_ITEM_DATA)((ULONG_PTR)hListThreadPtr + sizeof THREAD_ITEM_DATA);
			}
			if(hListThreadPtr->dwThreadId == DBGEvent.dwThreadId){
				hListThreadPtr->hThread = (HANDLE)-1;
				hListThreadPtr->dwThreadId = NULL;
				hListThreadPtr->ThreadLocalBase = NULL;
				hListThreadPtr->ThreadStartAddress = NULL;
			}
		}else if(DBGEvent.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT){
			if(hListLibrary == NULL){
				hListLibrary = VirtualAlloc(NULL, MAX_DEBUG_DATA * sizeof LIBRARY_ITEM_DATA, MEM_COMMIT, PAGE_READWRITE);
			}else{
				if(hListLibraryFirst == true){
					RtlZeroMemory(hListLibrary, MAX_DEBUG_DATA * sizeof LIBRARY_ITEM_DATA);
				}
			}
			hListLibraryFirst = false;
			hListLibraryPtr = (PLIBRARY_ITEM_DATA)hListLibrary;
			while(hListLibraryPtr->hFile != NULL){
				hListLibraryPtr = (PLIBRARY_ITEM_DATA)((ULONG_PTR)hListLibraryPtr + sizeof LIBRARY_ITEM_DATA);
			}
			hListLibraryPtr->hFile = DBGEvent.u.LoadDll.hFile;
			hListLibraryPtr->BaseOfDll = DBGEvent.u.LoadDll.lpBaseOfDll;
			hFileMapping = CreateFileMappingA(DBGEvent.u.LoadDll.hFile, NULL, PAGE_READONLY, NULL, GetFileSize(DBGEvent.u.LoadDll.hFile, NULL), NULL);
			if(hFileMapping != NULL){
				hFileMappingView = MapViewOfFile(hFileMapping, FILE_MAP_READ, NULL, NULL, NULL);
				if(hFileMappingView != NULL){
					hListLibraryPtr->hFileMapping = hFileMapping;
					hListLibraryPtr->hFileMappingView = hFileMappingView;
					if(GetMappedFileNameA(GetCurrentProcess(), hFileMappingView, DLLDebugFileName, 512) > NULL){
						i = lstrlenA(DLLDebugFileName);
						while(DLLDebugFileName[i] != 0x5C && i >= NULL){
							i--;
						}
						if(engineDebuggingDLL){
							if(lstrcmpiA(&DLLDebugFileName[i+1], engineDebuggingDLLReserveFileName) == NULL){
								if((ULONG_PTR)DBGEvent.u.LoadDll.lpBaseOfDll != DebugModuleImageBase){
									VirtualAllocEx(dbgProcessInformation.hProcess, (void*)DebugModuleImageBase, 0x1000, MEM_RESERVE, PAGE_READWRITE);
								}
							}else if(lstrcmpiA(&DLLDebugFileName[i+1], engineDebuggingDLLFileName) == NULL){
								SetBPX(DebugModuleEntryPoint + (ULONG_PTR)DBGEvent.u.LoadDll.lpBaseOfDll, UE_SINGLESHOOT, DebugModuleEntryPointCallBack);
								engineDebuggingDLLBase = (ULONG_PTR)DBGEvent.u.LoadDll.lpBaseOfDll;
							}
						}
						if(engineFakeDLLHandle == NULL){
							if(lstrcmpiA(&DLLDebugFileName[i+1], "kernel32.dll") == NULL){
								engineFakeDLLHandle = (ULONG_PTR)DBGEvent.u.LoadDll.lpBaseOfDll;
							}
						}
						lstrcpyA(hListLibraryPtr->szLibraryName, &DLLDebugFileName[i+1]);
						szTranslatedNativeName = (char*)TranslateNativeName(DLLDebugFileName);
						lstrcpyA(hListLibraryPtr->szLibraryPath, szTranslatedNativeName);
						VirtualFree((void*)szTranslatedNativeName, NULL, MEM_RELEASE);
						k = NULL;
						ptrLibrarianData = (PLIBRARY_BREAK_DATA)LibrarianData;
						if(ptrLibrarianData != NULL){
							while(k < MAX_LIBRARY_BPX){
								if(ptrLibrarianData->szLibraryName[0] != 0x00){
									if(lstrcmpiA(ptrLibrarianData->szLibraryName, &DLLDebugFileName[i+1]) == NULL){
										if(ptrLibrarianData->bpxType == UE_ON_LIB_LOAD || ptrLibrarianData->bpxType == UE_ON_LIB_ALL){
											myCustomHandler = (fCustomHandler)(ptrLibrarianData->bpxCallBack);
											__try{
												myCustomHandler(&DBGEvent.u.LoadDll);
											}__except(EXCEPTION_EXECUTE_HANDLER){
												LibrarianRemoveBreakPoint(ptrLibrarianData->szLibraryName, ptrLibrarianData->bpxType);
											}
											if(ptrLibrarianData->bpxSingleShoot){
												LibrarianRemoveBreakPoint(ptrLibrarianData->szLibraryName, ptrLibrarianData->bpxType);
											}
										}
									}
								}
								ptrLibrarianData = (PLIBRARY_BREAK_DATA)((ULONG_PTR)ptrLibrarianData + sizeof LIBRARY_BREAK_DATA);
								k++;
							}
						}
					}
				}
			}
			if(DBGCustomHandler->chLoadDll != NULL){
				myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chLoadDll);
				__try{
					myCustomHandler(&DBGEvent.u.LoadDll);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					DBGCustomHandler->chLoadDll = NULL;
				}
			}
		}else if(DBGEvent.dwDebugEventCode == UNLOAD_DLL_DEBUG_EVENT){
			if(DBGCustomHandler->chUnloadDll != NULL){
				myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chUnloadDll);
				__try{
					myCustomHandler(&DBGEvent.u.UnloadDll);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					DBGCustomHandler->chUnloadDll = NULL;
				}
			}
			k = NULL;
			ptrLibrarianData = (PLIBRARY_BREAK_DATA)LibrarianData;
			hLoadedLibData = (PLIBRARY_ITEM_DATA)LibrarianGetLibraryInfoEx(DBGEvent.u.UnloadDll.lpBaseOfDll);
			if(ptrLibrarianData != NULL){
				while(k < MAX_LIBRARY_BPX){
					if(ptrLibrarianData->szLibraryName[0] != 0x00){
						if(lstrcmpiA(ptrLibrarianData->szLibraryName, hLoadedLibData->szLibraryName) == NULL){
							if(ptrLibrarianData->bpxType == UE_ON_LIB_UNLOAD || ptrLibrarianData->bpxType == UE_ON_LIB_ALL){
								myCustomHandler = (fCustomHandler)(ptrLibrarianData->bpxCallBack);
								__try{
									myCustomHandler(&DBGEvent.u.UnloadDll);
								}__except(EXCEPTION_EXECUTE_HANDLER){
									LibrarianRemoveBreakPoint(ptrLibrarianData->szLibraryName, ptrLibrarianData->bpxType);
								}
								if(ptrLibrarianData->bpxSingleShoot){
									LibrarianRemoveBreakPoint(ptrLibrarianData->szLibraryName, ptrLibrarianData->bpxType);
								}
							}
						}
					}
					ptrLibrarianData = (PLIBRARY_BREAK_DATA)((ULONG_PTR)ptrLibrarianData + sizeof LIBRARY_BREAK_DATA);
					k++;
				}
			}
			hListLibraryPtr = (PLIBRARY_ITEM_DATA)hListLibrary;
			while(hListLibraryPtr->hFile != NULL){
				if(hListLibraryPtr->BaseOfDll == DBGEvent.u.UnloadDll.lpBaseOfDll){
					if(hListLibraryPtr->hFile != (HANDLE)-1){
						if(hListLibraryPtr->hFileMappingView != NULL){
							UnmapViewOfFile(hListLibraryPtr->hFileMappingView);
							EngineCloseHandle(hListLibraryPtr->hFileMapping);
						}
						EngineCloseHandle(hListLibraryPtr->hFile);
						RtlZeroMemory(hListLibraryPtr, sizeof LIBRARY_ITEM_DATA);
						hListLibraryPtr->hFile = (HANDLE)-1;
					}
				}
				hListLibraryPtr = (PLIBRARY_ITEM_DATA)((ULONG_PTR)hListLibraryPtr + sizeof LIBRARY_ITEM_DATA);
			}
		}else if(DBGEvent.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT){
			if(DBGCustomHandler->chOutputDebugString != NULL){
				myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chOutputDebugString);
				__try{
					myCustomHandler(&DBGEvent.u.DebugString);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					DBGCustomHandler->chOutputDebugString = NULL;
				}
			}
		}else if(DBGEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT){
			if(DBGCustomHandler->chEverythingElse != NULL){
				myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chEverythingElse);
				__try{
					myCustomHandler(&DBGEvent.u.Exception.ExceptionRecord);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					DBGCustomHandler->chEverythingElse = NULL;
				}
			}
			if(DBGEvent.u.Exception.dwFirstChance == FALSE){
				if(!enginePassAllExceptions){
					DBGCode = DBG_CONTINUE;
				}else{
					DBGCode = DBG_EXCEPTION_NOT_HANDLED;
				}
				RtlMoveMemory(&TerminateDBGEvent, &DBGEvent, sizeof DEBUG_EVENT);
			}
			if(DBGEvent.u.Exception.ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT){
				if(DBGCustomHandler->chBreakPoint != NULL){
					myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chBreakPoint);
					__try{
						myCustomHandler(&DBGEvent.u.Exception.ExceptionRecord);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						DBGCustomHandler->chBreakPoint = NULL;
					}
				}
				MaximumBreakPoints = 0;
				for(MaximumBreakPoints = 0; MaximumBreakPoints < BreakPointSetCount; MaximumBreakPoints++){
					if(BreakPointBuffer[MaximumBreakPoints].BreakPointAddress == (ULONG_PTR)DBGEvent.u.Exception.ExceptionRecord.ExceptionAddress - (BreakPointBuffer[MaximumBreakPoints].BreakPointSize - 1)){
						break;
					}
				}
				if(BreakPointBuffer[MaximumBreakPoints].BreakPointActive == UE_BPXACTIVE && MaximumBreakPoints < MAXIMUM_BREAKPOINTS){
					VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
					OldProtect = MemInfo.AllocationProtect;
					VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, BreakPointBuffer[MaximumBreakPoints].BreakPointSize, PAGE_EXECUTE_READWRITE, &OldProtect);
					if(BreakPointBuffer[MaximumBreakPoints].BreakPointActive == UE_BPXACTIVE && (BreakPointBuffer[MaximumBreakPoints].BreakPointType == UE_BREAKPOINT || BreakPointBuffer[MaximumBreakPoints].BreakPointType == UE_SINGLESHOOT) && (BreakPointBuffer[MaximumBreakPoints].NumberOfExecutions == -1 || BreakPointBuffer[MaximumBreakPoints].NumberOfExecutions > 0)){
						if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, &BreakPointBuffer[MaximumBreakPoints].OriginalByte[0], BreakPointBuffer[MaximumBreakPoints].BreakPointSize, &NumberOfBytesReadWritten)){
							DBGCode = DBG_CONTINUE;
							hActiveThread = OpenThread(THREAD_GET_CONTEXT+THREAD_SET_CONTEXT+THREAD_QUERY_INFORMATION, false, DBGEvent.dwThreadId);
							myDBGContext.ContextFlags = CONTEXT_ALL | CONTEXT_DEBUG_REGISTERS;
							GetThreadContext(hActiveThread, &myDBGContext);
							if(BreakPointBuffer[MaximumBreakPoints].BreakPointType != UE_SINGLESHOOT){
								if(!(myDBGContext.EFlags & 0x100)){
									myDBGContext.EFlags = myDBGContext.EFlags ^ 0x100;
								}
							}
						#if defined(_WIN64)
							myDBGContext.Rip = myDBGContext.Rip - BreakPointBuffer[MaximumBreakPoints].BreakPointSize;
						#else
							myDBGContext.Eip = myDBGContext.Eip - BreakPointBuffer[MaximumBreakPoints].BreakPointSize;
						#endif
							SetThreadContext(hActiveThread, &myDBGContext);
							EngineCloseHandle(hActiveThread);							
							VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, BreakPointBuffer[MaximumBreakPoints].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
							myCustomBreakPoint = (fCustomBreakPoint)((LPVOID)BreakPointBuffer[MaximumBreakPoints].ExecuteCallBack);
							if(BreakPointBuffer[MaximumBreakPoints].NumberOfExecutions != -1 && BreakPointBuffer[MaximumBreakPoints].NumberOfExecutions != 0){
								BreakPointBuffer[MaximumBreakPoints].NumberOfExecutions--;
							}
							if(BreakPointBuffer[MaximumBreakPoints].CmpCondition != UE_CMP_NOCONDITION){
								CompareResult = false;
								CmpValue1 = (ULONG_PTR)GetContextData((DWORD)BreakPointBuffer[MaximumBreakPoints].CmpRegister);
								if(BreakPointBuffer[MaximumBreakPoints].CmpCondition == UE_CMP_EQUAL){
									CmpValue2 = BreakPointBuffer[MaximumBreakPoints].CmpValue;
									if(CmpValue1 == CmpValue2){
										CompareResult = true;
									}
								}else if(BreakPointBuffer[MaximumBreakPoints].CmpCondition == UE_CMP_NOTEQUAL){
									CmpValue2 = BreakPointBuffer[MaximumBreakPoints].CmpValue;
									if(CmpValue1 != CmpValue2){
										CompareResult = true;
									}
								}else if(BreakPointBuffer[MaximumBreakPoints].CmpCondition == UE_CMP_GREATER){
									CmpValue2 = BreakPointBuffer[MaximumBreakPoints].CmpValue;
									if(CmpValue1 > CmpValue2){
										CompareResult = true;
									}
								}else if(BreakPointBuffer[MaximumBreakPoints].CmpCondition == UE_CMP_GREATEROREQUAL){
									CmpValue2 = BreakPointBuffer[MaximumBreakPoints].CmpValue;
									if(CmpValue1 >= CmpValue2){
										CompareResult = true;
									}
								}else if(BreakPointBuffer[MaximumBreakPoints].CmpCondition == UE_CMP_LOWER){
									CmpValue2 = BreakPointBuffer[MaximumBreakPoints].CmpValue;
									if(CmpValue1 < CmpValue2){
										CompareResult = true;
									}
								}else if(BreakPointBuffer[MaximumBreakPoints].CmpCondition == UE_CMP_LOWEROREQUAL){
									CmpValue2 = BreakPointBuffer[MaximumBreakPoints].CmpValue;
									if(CmpValue1 <= CmpValue2){
										CompareResult = true;
									}
								}else if(BreakPointBuffer[MaximumBreakPoints].CmpCondition == UE_CMP_REG_EQUAL){
									CmpValue2 = (ULONG_PTR)GetContextData((DWORD)BreakPointBuffer[MaximumBreakPoints].CmpValue);
									if(CmpValue1 == CmpValue2){
										CompareResult = true;
									}
								}else if(BreakPointBuffer[MaximumBreakPoints].CmpCondition == UE_CMP_REG_NOTEQUAL){
									CmpValue2 = (ULONG_PTR)GetContextData((DWORD)BreakPointBuffer[MaximumBreakPoints].CmpValue);
									if(CmpValue1 != CmpValue2){
										CompareResult = true;
									}
								}else if(BreakPointBuffer[MaximumBreakPoints].CmpCondition == UE_CMP_REG_GREATER){
									CmpValue2 = (ULONG_PTR)GetContextData((DWORD)BreakPointBuffer[MaximumBreakPoints].CmpValue);
									if(CmpValue1 > CmpValue2){
										CompareResult = true;
									}
								}else if(BreakPointBuffer[MaximumBreakPoints].CmpCondition == UE_CMP_REG_GREATEROREQUAL){
									CmpValue2 = (ULONG_PTR)GetContextData((DWORD)BreakPointBuffer[MaximumBreakPoints].CmpValue);
									if(CmpValue1 >= CmpValue2){
										CompareResult = true;
									}
								}else if(BreakPointBuffer[MaximumBreakPoints].CmpCondition == UE_CMP_REG_LOWER){
									CmpValue2 = (ULONG_PTR)GetContextData((DWORD)BreakPointBuffer[MaximumBreakPoints].CmpValue);
									if(CmpValue1 < CmpValue2){
										CompareResult = true;
									}
								}else if(BreakPointBuffer[MaximumBreakPoints].CmpCondition == UE_CMP_REG_LOWEROREQUAL){
									CmpValue2 = (ULONG_PTR)GetContextData((DWORD)BreakPointBuffer[MaximumBreakPoints].CmpValue);
									if(CmpValue1 <= CmpValue2){
										CompareResult = true;
									}
								}
								if(CompareResult){
									__try{
										myCustomBreakPoint();
									}__except(EXCEPTION_EXECUTE_HANDLER){

									}
								}
							}else{
								__try{
									myCustomBreakPoint();
								}__except(EXCEPTION_EXECUTE_HANDLER){

								}
							}
							if(BreakPointBuffer[MaximumBreakPoints].BreakPointType != UE_SINGLESHOOT){
								DisableBPX((ULONG_PTR)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress);
								ResetBPX = true;
								ResetBPXSize = BreakPointBuffer[MaximumBreakPoints].BreakPointSize - 1;
								ResetBPXAddressTo = (ULONG_PTR)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress;
							}else{
								DeleteBPX((ULONG_PTR)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress);
								ResetBPX = false;
								ResetBPXSize = BreakPointBuffer[MaximumBreakPoints].BreakPointSize - 1;
								ResetBPXAddressTo = NULL;
							}
						}else{
							VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, BreakPointBuffer[MaximumBreakPoints].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
							DBGCode = DBG_CONTINUE;
						}
					}else{
						VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, BreakPointBuffer[MaximumBreakPoints].BreakPointSize, MemInfo.AllocationProtect, &OldProtect);
						DBGCode = DBG_EXCEPTION_NOT_HANDLED;
					}									
				}else{
					if(!FirstBPX){
						DBGCode = DBG_EXCEPTION_NOT_HANDLED;
					}else{
						DBGCode = DBG_CONTINUE;
						if(engineAttachedToProcess){
							myCustomBreakPoint = (fCustomBreakPoint)(engineAttachedProcessCallBack);
							__try{
								myCustomBreakPoint();
							}__except(EXCEPTION_EXECUTE_HANDLER){

							}
						}
						if(engineAutoHideFromDebugger){
							HideDebugger(dbgProcessInformation.hThread, dbgProcessInformation.hProcess, NULL);
						}
						if(DebugExeFileEntryPointCallBack != NULL){
							SetBPX((ULONG_PTR)DBGEntryPoint, UE_SINGLESHOOT, DebugExeFileEntryPointCallBack);						
						}
						if(engineTLSBreakOnCallBack){
							i = NULL;
							while(tlsCallBackList[i] != NULL){
								SetBPX((ULONG_PTR)tlsCallBackList[i], UE_SINGLESHOOT, (LPVOID)engineTLSBreakOnCallBackAddress);
								tlsCallBackList[i] = NULL;
								i++;
							}
							engineTLSBreakOnCallBackAddress = NULL;
							engineTLSBreakOnCallBack = false;
						}
						FirstBPX = false;
					}
				}
			}else if(DBGEvent.u.Exception.ExceptionRecord.ExceptionCode == STATUS_SINGLE_STEP){
				if(DBGCustomHandler->chSingleStep != NULL){
					myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chSingleStep);
					__try{
						myCustomHandler(&DBGEvent.u.Exception.ExceptionRecord);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						DBGCustomHandler->chSingleStep = NULL;
					}
				}
				if(ResetBPX == true || ResetHwBPX == true){
					DBGCode = DBG_CONTINUE;
					if(!ResetHwBPX){
						if(ResetBPXAddressTo + ResetBPXSize != (ULONG_PTR)DBGEvent.u.Exception.ExceptionRecord.ExceptionAddress){
							EnableBPX(ResetBPXAddressTo);
							ResetBPX = false;
							ResetBPXAddressTo = NULL;
							if(engineStepActive){
								if(engineStepCount == INFINITE){
									myCustomBreakPoint = (fCustomBreakPoint)(engineStepCallBack);
									__try{
										engineStepActive = false;
										engineStepCallBack = NULL;
										myCustomBreakPoint();
									}__except(EXCEPTION_EXECUTE_HANDLER){

									}
								}else{
									engineStepCount--;
									StepInto(engineStepCallBack);
								}
							}
						}else{
							hActiveThread = OpenThread(THREAD_GET_CONTEXT+THREAD_SET_CONTEXT+THREAD_QUERY_INFORMATION, false, DBGEvent.dwThreadId);
							myDBGContext.ContextFlags = CONTEXT_ALL | CONTEXT_DEBUG_REGISTERS;
							GetThreadContext(hActiveThread, &myDBGContext);
							if(!(myDBGContext.EFlags & 0x100)){
								myDBGContext.EFlags = myDBGContext.EFlags ^ 0x100;
							}
							SetThreadContext(hActiveThread, &myDBGContext);
							EngineCloseHandle(hActiveThread);							
						}
					}else{
						SetHardwareBreakPoint(DebugRegisterX.DrxBreakAddress, DebugRegisterXId, DebugRegisterX.DrxBreakPointType, DebugRegisterX.DrxBreakPointSize, (LPVOID)DebugRegisterX.DrxCallBack);
						ResetHwBPX = false;
					}
				}else{
					if(engineStepActive){
						DBGCode = DBG_CONTINUE;
						myCustomBreakPoint = (fCustomBreakPoint)(engineStepCallBack);
						__try{
							engineStepActive = false;
							engineStepCallBack = NULL;
							myCustomBreakPoint();
						}__except(EXCEPTION_EXECUTE_HANDLER){

						}
					}else{
						hActiveThread = OpenThread(THREAD_GET_CONTEXT+THREAD_SET_CONTEXT+THREAD_QUERY_INFORMATION, false, DBGEvent.dwThreadId);
						myDBGContext.ContextFlags = CONTEXT_ALL | CONTEXT_DEBUG_REGISTERS;
						GetThreadContext(hActiveThread, &myDBGContext);						
						if((ULONG_PTR)DBGEvent.u.Exception.ExceptionRecord.ExceptionAddress == myDBGContext.Dr0 || (myDBGContext.Dr6 & 0x1)){
							DBGCode = DBG_CONTINUE;
							if(DebugRegister0.DrxEnabled){
								if(!(myDBGContext.EFlags & 0x100)){
									myDBGContext.EFlags = myDBGContext.EFlags ^ 0x100;
								}
								SetThreadContext(hActiveThread, &myDBGContext);
								myCustomBreakPoint = (fCustomBreakPoint)(DebugRegister0.DrxCallBack);
								__try{
									myCustomBreakPoint();
								}__except(EXCEPTION_EXECUTE_HANDLER){

								}
								RtlZeroMemory(&DebugRegisterX, sizeof HARDWARE_DATA);
								RtlMoveMemory(&DebugRegisterX, &DebugRegister0, sizeof HARDWARE_DATA);
								DeleteHardwareBreakPoint(UE_DR0);
								DebugRegisterXId = UE_DR0;
								ResetHwBPX = true;
							}else{
								DeleteHardwareBreakPoint(UE_DR0);
							}
						}else if((ULONG_PTR)DBGEvent.u.Exception.ExceptionRecord.ExceptionAddress == myDBGContext.Dr1 || (myDBGContext.Dr6 & 0x2)){
							DBGCode = DBG_CONTINUE;
							if(DebugRegister1.DrxEnabled){
								if(!(myDBGContext.EFlags & 0x100)){
									myDBGContext.EFlags = myDBGContext.EFlags ^ 0x100;
								}
								SetThreadContext(hActiveThread, &myDBGContext);
								myCustomBreakPoint = (fCustomBreakPoint)(DebugRegister1.DrxCallBack);
								__try{
									myCustomBreakPoint();
								}__except(EXCEPTION_EXECUTE_HANDLER){

								}
								RtlZeroMemory(&DebugRegisterX, sizeof HARDWARE_DATA);
								RtlMoveMemory(&DebugRegisterX, &DebugRegister1, sizeof HARDWARE_DATA);
								DeleteHardwareBreakPoint(UE_DR1);
								DebugRegisterXId = UE_DR1;
								ResetHwBPX = true;
							}else{
								DeleteHardwareBreakPoint(UE_DR1);
							}
						}else if((ULONG_PTR)DBGEvent.u.Exception.ExceptionRecord.ExceptionAddress == myDBGContext.Dr2 || (myDBGContext.Dr6 & 0x4)){
							DBGCode = DBG_CONTINUE;
							if(DebugRegister2.DrxEnabled){
								if(!(myDBGContext.EFlags & 0x100)){
									myDBGContext.EFlags = myDBGContext.EFlags ^ 0x100;
								}
								SetThreadContext(hActiveThread, &myDBGContext);
								myCustomBreakPoint = (fCustomBreakPoint)(DebugRegister2.DrxCallBack);
								__try{
									myCustomBreakPoint();
								}__except(EXCEPTION_EXECUTE_HANDLER){

								}
								RtlZeroMemory(&DebugRegisterX, sizeof HARDWARE_DATA);
								RtlMoveMemory(&DebugRegisterX, &DebugRegister2, sizeof HARDWARE_DATA);
								DeleteHardwareBreakPoint(UE_DR2);
								DebugRegisterXId = UE_DR2;
								ResetHwBPX = true;
							}else{
								DeleteHardwareBreakPoint(UE_DR2);
							}
						}else if((ULONG_PTR)DBGEvent.u.Exception.ExceptionRecord.ExceptionAddress == myDBGContext.Dr3 || (myDBGContext.Dr6 & 0x8)){
							DBGCode = DBG_CONTINUE;
							if(DebugRegister3.DrxEnabled){
								if(!(myDBGContext.EFlags & 0x100)){
									myDBGContext.EFlags = myDBGContext.EFlags ^ 0x100;
								}
								SetThreadContext(hActiveThread, &myDBGContext);
								myCustomBreakPoint = (fCustomBreakPoint)(DebugRegister3.DrxCallBack);
								__try{
									myCustomBreakPoint();
								}__except(EXCEPTION_EXECUTE_HANDLER){

								}
								RtlZeroMemory(&DebugRegisterX, sizeof HARDWARE_DATA);
								RtlMoveMemory(&DebugRegisterX, &DebugRegister3, sizeof HARDWARE_DATA);
								DeleteHardwareBreakPoint(UE_DR3);
								DebugRegisterXId = UE_DR3;
								ResetHwBPX = true;
							}else{
								DeleteHardwareBreakPoint(UE_DR3);
							}
						}else{
							DBGCode = DBG_EXCEPTION_NOT_HANDLED;
						}
						EngineCloseHandle(hActiveThread);	
					}
				}
			}else if(DBGEvent.u.Exception.ExceptionRecord.ExceptionCode == STATUS_GUARD_PAGE_VIOLATION){
				if(DBGCustomHandler->chPageGuard != NULL){
					myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chPageGuard);
					__try{
						myCustomHandler(&DBGEvent.u.Exception.ExceptionRecord);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						DBGCustomHandler->chPageGuard = NULL;
					}
				}
				MemoryBpxFound = false;
				MaximumBreakPoints = 0;
				for(MaximumBreakPoints = 0; MaximumBreakPoints < BreakPointSetCount; MaximumBreakPoints++){
					if(BreakPointBuffer[MaximumBreakPoints].BreakPointAddress <= (ULONG_PTR)DBGEvent.u.Exception.ExceptionRecord.ExceptionInformation[1] && (ULONG_PTR)DBGEvent.u.Exception.ExceptionRecord.ExceptionInformation[1] < BreakPointBuffer[MaximumBreakPoints].BreakPointAddress +  BreakPointBuffer[MaximumBreakPoints].BreakPointSize){
						MemoryBpxFound = true;
						break;
					}
				}
				if(MaximumBreakPoints < MAXIMUM_BREAKPOINTS || MemoryBpxFound == true){
					if(BreakPointBuffer[MaximumBreakPoints].BreakPointActive == UE_BPXACTIVE && (BreakPointBuffer[MaximumBreakPoints].BreakPointType >= UE_MEMORY && BreakPointBuffer[MaximumBreakPoints].BreakPointType <= UE_MEMORY_WRITE)){
						DBGCode = DBG_CONTINUE;
						MemoryBpxCallBack = BreakPointBuffer[MaximumBreakPoints].ExecuteCallBack;
						if(BreakPointBuffer[MaximumBreakPoints].BreakPointType == UE_MEMORY){
							if(BreakPointBuffer[MaximumBreakPoints].MemoryBpxRestoreOnHit != 1){
								RemoveMemoryBPX(BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, BreakPointBuffer[MaximumBreakPoints].BreakPointSize);
							}else{
								VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
								OldProtect = MemInfo.AllocationProtect;
								NewProtect = OldProtect ^ PAGE_GUARD;
								VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, BreakPointBuffer[MaximumBreakPoints].BreakPointSize, NewProtect, &OldProtect);
							}
							myCustomBreakPoint = (fCustomBreakPoint)((LPVOID)MemoryBpxCallBack);
							__try{
								myCustomBreakPoint();
							}__except(EXCEPTION_EXECUTE_HANDLER){

							}
						}else if(BreakPointBuffer[MaximumBreakPoints].BreakPointType == UE_MEMORY_READ){
							if(BreakPointBuffer[MaximumBreakPoints].MemoryBpxRestoreOnHit != 1){
								RemoveMemoryBPX(BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, BreakPointBuffer[MaximumBreakPoints].BreakPointSize);
							}else{
								VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
								OldProtect = MemInfo.AllocationProtect;
								NewProtect = OldProtect ^ PAGE_GUARD;
								VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, BreakPointBuffer[MaximumBreakPoints].BreakPointSize, NewProtect, &OldProtect);
							}
							if(DBGEvent.u.Exception.ExceptionRecord.ExceptionInformation[0] == 0){
								myCustomBreakPoint = (fCustomBreakPoint)((LPVOID)MemoryBpxCallBack);
								__try{
									myCustomBreakPoint();
								}__except(EXCEPTION_EXECUTE_HANDLER){

								}
							}else{
								if(BreakPointBuffer[MaximumBreakPoints].BreakPointAddress >= (ULONG_PTR)DBGEvent.u.Exception.ExceptionRecord.ExceptionAddress && (ULONG_PTR)DBGEvent.u.Exception.ExceptionRecord.ExceptionAddress <= BreakPointBuffer[MaximumBreakPoints].BreakPointAddress + BreakPointBuffer[MaximumBreakPoints].BreakPointSize){
									VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
									OldProtect = MemInfo.AllocationProtect;
									NewProtect = OldProtect ^ PAGE_GUARD;
									VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, BreakPointBuffer[MaximumBreakPoints].BreakPointSize, NewProtect, &OldProtect);
								}
							}
						}else if(BreakPointBuffer[MaximumBreakPoints].BreakPointType == UE_MEMORY_WRITE){
							if(BreakPointBuffer[MaximumBreakPoints].MemoryBpxRestoreOnHit != 1){
								RemoveMemoryBPX(BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, BreakPointBuffer[MaximumBreakPoints].BreakPointSize);
							}else{
								VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
								OldProtect = MemInfo.AllocationProtect;
								NewProtect = OldProtect ^ PAGE_GUARD;
								VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, BreakPointBuffer[MaximumBreakPoints].BreakPointSize, NewProtect, &OldProtect);
							}
							if(DBGEvent.u.Exception.ExceptionRecord.ExceptionInformation[0] == 1){
								myCustomBreakPoint = (fCustomBreakPoint)((LPVOID)MemoryBpxCallBack);
								__try{
									myCustomBreakPoint();
								}__except(EXCEPTION_EXECUTE_HANDLER){

								}
							}else{
								if(BreakPointBuffer[MaximumBreakPoints].BreakPointAddress >= (ULONG_PTR)DBGEvent.u.Exception.ExceptionRecord.ExceptionAddress && (ULONG_PTR)DBGEvent.u.Exception.ExceptionRecord.ExceptionAddress <= BreakPointBuffer[MaximumBreakPoints].BreakPointAddress + BreakPointBuffer[MaximumBreakPoints].BreakPointSize){
									VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
									OldProtect = MemInfo.AllocationProtect;
									NewProtect = OldProtect ^ PAGE_GUARD;
									VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)BreakPointBuffer[MaximumBreakPoints].BreakPointAddress, BreakPointBuffer[MaximumBreakPoints].BreakPointSize, NewProtect, &OldProtect);
								}
							}
						}
					}else{
						DBGCode = DBG_EXCEPTION_NOT_HANDLED;
					}
				}else{
					DBGCode = DBG_EXCEPTION_NOT_HANDLED;
				}
			}else if(DBGEvent.u.Exception.ExceptionRecord.ExceptionCode == STATUS_ACCESS_VIOLATION){
				DBGCode = DBG_EXCEPTION_NOT_HANDLED;
				if(DBGCustomHandler->chAccessViolation != NULL){
					myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chAccessViolation);
					__try{
						myCustomHandler(&DBGEvent.u.Exception.ExceptionRecord);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						DBGCustomHandler->chAccessViolation = NULL;
					}
				}		
			}else if(DBGEvent.u.Exception.ExceptionRecord.ExceptionCode == STATUS_ILLEGAL_INSTRUCTION){
				DBGCode = DBG_EXCEPTION_NOT_HANDLED;
				if(DBGCustomHandler->chIllegalInstruction != NULL){
					myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chIllegalInstruction);
					__try{
						myCustomHandler(&DBGEvent.u.Exception.ExceptionRecord);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						DBGCustomHandler->chIllegalInstruction = NULL;
					}
				}
			}else if(DBGEvent.u.Exception.ExceptionRecord.ExceptionCode == STATUS_NONCONTINUABLE_EXCEPTION){
				DBGCode = DBG_EXCEPTION_NOT_HANDLED;
				if(DBGCustomHandler->chNonContinuableException != NULL){
					myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chNonContinuableException);
					__try{
						myCustomHandler(&DBGEvent.u.Exception.ExceptionRecord);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						DBGCustomHandler->chNonContinuableException = NULL;
					}
				}		
			}else if(DBGEvent.u.Exception.ExceptionRecord.ExceptionCode == STATUS_ARRAY_BOUNDS_EXCEEDED){
				DBGCode = DBG_EXCEPTION_NOT_HANDLED;
				if(DBGCustomHandler->chArrayBoundsException != NULL){
					myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chArrayBoundsException);
					__try{
						myCustomHandler(&DBGEvent.u.Exception.ExceptionRecord);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						DBGCustomHandler->chArrayBoundsException = NULL;
					}
				}		
			}else if(DBGEvent.u.Exception.ExceptionRecord.ExceptionCode == STATUS_FLOAT_DENORMAL_OPERAND){
				DBGCode = DBG_EXCEPTION_NOT_HANDLED;
				if(DBGCustomHandler->chFloatDenormalOperand != NULL){
					myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chFloatDenormalOperand);
					__try{
						myCustomHandler(&DBGEvent.u.Exception.ExceptionRecord);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						DBGCustomHandler->chFloatDenormalOperand = NULL;
					}
				}		
			}else if(DBGEvent.u.Exception.ExceptionRecord.ExceptionCode == STATUS_FLOAT_DIVIDE_BY_ZERO){
				DBGCode = DBG_EXCEPTION_NOT_HANDLED;
				if(DBGCustomHandler->chFloatDevideByZero != NULL){
					myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chFloatDevideByZero);
					__try{
						myCustomHandler(&DBGEvent.u.Exception.ExceptionRecord);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						DBGCustomHandler->chFloatDevideByZero = NULL;
					}
				}		
			}else if(DBGEvent.u.Exception.ExceptionRecord.ExceptionCode == STATUS_INTEGER_DIVIDE_BY_ZERO){
				DBGCode = DBG_EXCEPTION_NOT_HANDLED;
				if(DBGCustomHandler->chIntegerDevideByZero != NULL){
					myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chIntegerDevideByZero);
					__try{
						myCustomHandler(&DBGEvent.u.Exception.ExceptionRecord);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						DBGCustomHandler->chIntegerDevideByZero = NULL;
					}
				}		
			}else if(DBGEvent.u.Exception.ExceptionRecord.ExceptionCode == STATUS_INTEGER_OVERFLOW){
				DBGCode = DBG_EXCEPTION_NOT_HANDLED;
				if(DBGCustomHandler->chIntegerOverflow != NULL){
					myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chIntegerOverflow);
					__try{
						myCustomHandler(&DBGEvent.u.Exception.ExceptionRecord);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						DBGCustomHandler->chIntegerOverflow = NULL;
					}
				}		
			}else if(DBGEvent.u.Exception.ExceptionRecord.ExceptionCode == STATUS_PRIVILEGED_INSTRUCTION){
				DBGCode = DBG_EXCEPTION_NOT_HANDLED;
				if(DBGCustomHandler->chPrivilegedInstruction != NULL){
					myCustomHandler = (fCustomHandler)((LPVOID)DBGCustomHandler->chPrivilegedInstruction);
					__try{
						myCustomHandler(&DBGEvent.u.Exception.ExceptionRecord);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						DBGCustomHandler->chPrivilegedInstruction = NULL;
					}
				}		
			}
		}
		if(engineResumeProcessIfNoThreadIsActive){
			if(!ThreaderIsAnyThreadActive()){
				ThreaderResumeProcess();
			}
		}
		if(!ContinueDebugEvent(DBGEvent.dwProcessId, DBGEvent.dwThreadId, DBGCode)){
			break;
		}
	}
	if(!SecondChance){
		RtlMoveMemory(&TerminateDBGEvent, &DBGEvent, sizeof DEBUG_EVENT);
	}
	ForceClose();
}
__declspec(dllexport) void __stdcall SetDebugLoopTimeOut(DWORD TimeOut){

	if(TimeOut == NULL){
		TimeOut = INFINITE;
	}
	engineWaitForDebugEventTimeOut = TimeOut;
}
__declspec(dllexport) void __stdcall SetNextDbgContinueStatus(DWORD SetDbgCode){

	if(SetDbgCode != DBG_CONTINUE){
		DBGCode = DBG_EXCEPTION_NOT_HANDLED;
	}else{
		DBGCode = DBG_CONTINUE;
	}
}
__declspec(dllexport) void __stdcall DebugLoopEx(DWORD TimeOut){
	SetDebugLoopTimeOut(TimeOut);
	DebugLoop();
	SetDebugLoopTimeOut(INFINITE);
}
__declspec(dllexport) bool __stdcall AttachDebugger(DWORD ProcessId, bool KillOnExit, LPVOID DebugInfo, LPVOID CallBack){

	typedef void(__stdcall *fDebugSetProcessKillOnExit)(bool KillExitingDebugee);
	fDebugSetProcessKillOnExit myDebugSetProcessKillOnExit;
	LPVOID funcDebugSetProcessKillOnExit = NULL;

	if(ProcessId != NULL && dbgProcessInformation.hProcess == NULL){
		RtlZeroMemory(&BreakPointBuffer, sizeof BreakPointBuffer);
		if(DebugActiveProcess(ProcessId)){
			if(KillOnExit){
				funcDebugSetProcessKillOnExit = GetProcAddress(GetModuleHandleA("kernel32.dll"), "DebugSetProcessKillOnExit");
				if(funcDebugSetProcessKillOnExit != NULL){
					myDebugSetProcessKillOnExit = (fDebugSetProcessKillOnExit)(funcDebugSetProcessKillOnExit);
					myDebugSetProcessKillOnExit(KillOnExit);
				}
			}
			BreakPointSetCount = 0;
			engineDebuggingDLL = false;
			engineAttachedToProcess = true;
			engineAttachedProcessCallBack = (ULONG_PTR)CallBack;
			engineAttachedProcessDebugInfo = DebugInfo;
			dbgProcessInformation.dwProcessId = ProcessId;
			DebugLoop();
			engineAttachedToProcess = false;
			engineAttachedProcessCallBack = NULL;
			return(true);
		}
	}else{
		return(false);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall DetachDebugger(DWORD ProcessId){

	typedef bool(__stdcall *fDebugActiveProcessStop)(DWORD dwProcessId);
	fDebugActiveProcessStop myDebugActiveProcessStop;
	LPVOID funcDebugActiveProcessStop = NULL;
	bool FuncReturn = false;

	if(ProcessId != NULL){
		funcDebugActiveProcessStop = GetProcAddress(GetModuleHandleA("kernel32.dll"), "DebugActiveProcessStop");
		if(funcDebugActiveProcessStop != NULL){
			myDebugActiveProcessStop = (fDebugActiveProcessStop)(funcDebugActiveProcessStop);
			FuncReturn = myDebugActiveProcessStop(ProcessId);
			engineProcessIsNowDetached = true;
			Sleep(250);
		}
		engineAttachedToProcess = false;
		if(FuncReturn){
			return(true);
		}else{
			return(false);
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall DetachDebuggerEx(DWORD ProcessId){

	ThreaderPauseProcess();
	ContinueDebugEvent(DBGEvent.dwProcessId, DBGEvent.dwThreadId, DBG_CONTINUE);
	ThreaderResumeProcess();
	return(DetachDebugger(ProcessId));
}
__declspec(dllexport) void __stdcall AutoDebugEx(char* szFileName, bool ReserveModuleBase, char* szCommandLine, char* szCurrentFolder, DWORD TimeOut, LPVOID EntryCallBack){

	DWORD ThreadId;
	DWORD ExitCode = 0;
	HANDLE hSecondThread;
	bool FileIsDll = false;
#if !defined(_WIN64)
	PE32Struct PEStructure;
#else
	PE64Struct PEStructure;
#endif

	if(TimeOut == NULL){
		TimeOut = INFINITE;
	}

	if(szFileName != NULL){
		RtlZeroMemory(&engineExpertDebug, sizeof ExpertDebug);
		engineExpertDebug.ExpertModeActive = true;
		engineExpertDebug.szFileName = szFileName;
		engineExpertDebug.szCommandLine = szCommandLine;
		engineExpertDebug.szCurrentFolder = szCurrentFolder;
		engineExpertDebug.ReserveModuleBase = ReserveModuleBase;
		engineExpertDebug.EntryCallBack = EntryCallBack;
		GetPE32DataEx(szFileName, (LPVOID)&PEStructure);
		if(PEStructure.Characteristics & 0x2000){
			FileIsDll = true;
		}
		SetDebugLoopTimeOut(TimeOut);
		hSecondThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)DebugLoopInSecondThread, (LPVOID)FileIsDll, NULL, &ThreadId);
		WaitForSingleObject(hSecondThread, INFINITE);
		if(GetExitCodeThread(hSecondThread, &ExitCode)){
			if(ExitCode == -1){
				ForceClose();
			}
		}
		RtlZeroMemory(&engineExpertDebug, sizeof ExpertDebug);
		SetDebugLoopTimeOut(INFINITE);
	}
}
// Global.FindOEP.functions:
void __stdcall GenericOEPVirtualProtectHit(){

	PBreakPointDetail bpxList = (PBreakPointDetail)BreakPointBuffer;
	MEMORY_BASIC_INFORMATION MemInfo; 
	DWORD MaximumBreakPoints = 0;
	DWORD NewProtect = 0;
	DWORD OldProtect = 0;

	while(MaximumBreakPoints < MAXIMUM_BREAKPOINTS){
		bpxList = (PBreakPointDetail)((ULONG_PTR)bpxList + sizeof BreakPointDetail);
		if(bpxList->BreakPointType == UE_MEMORY && bpxList->BreakPointActive == UE_BPXACTIVE){
			VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)bpxList->BreakPointAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
			OldProtect = MemInfo.Protect;
			if(!(OldProtect & PAGE_GUARD)){
				NewProtect = OldProtect ^ PAGE_GUARD;
				VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxList->BreakPointAddress, bpxList->BreakPointSize, NewProtect, &OldProtect);
			}
		}
		MaximumBreakPoints++;
	}
}
void __stdcall GenericOEPTraceHit(){

	char* szInstructionType;
	typedef void(__stdcall *fEPCallBack)();
	fEPCallBack myEPCallBack = (fEPCallBack)glbEntryTracerData.EPCallBack;
	LPDEBUG_EVENT myDbgEvent = (LPDEBUG_EVENT)GetDebugData();

	glbEntryTracerData.MemoryAccessedFrom = (ULONG_PTR)GetContextData(UE_CIP);
	glbEntryTracerData.MemoryAccessed = myDbgEvent->u.Exception.ExceptionRecord.ExceptionInformation[1];
	glbEntryTracerData.AccessType = myDbgEvent->u.Exception.ExceptionRecord.ExceptionInformation[0];
	szInstructionType = (char*)DisassembleEx(dbgProcessInformation.hProcess, (void*)glbEntryTracerData.MemoryAccessedFrom, true);
	StepInto(&GenericOEPTraceHited);
}
void __stdcall GenericOEPTraceHited(){

	int i;
	void* lpHashBuffer;
	bool FakeEPDetected = false;
	ULONG_PTR NumberOfBytesRW;
	LPDEBUG_EVENT myDbgEvent = (LPDEBUG_EVENT)GetDebugData();
	typedef void(__stdcall *fEPCallBack)();
	fEPCallBack myEPCallBack = (fEPCallBack)glbEntryTracerData.EPCallBack;
	PMEMORY_COMPARE_HANDLER myCmpHandler;
	ULONG_PTR memBpxAddress;
	ULONG_PTR memBpxSize;
	DWORD originalHash;
	DWORD currentHash;
	
	if(myDbgEvent->u.Exception.ExceptionRecord.ExceptionCode == STATUS_SINGLE_STEP){
		if(glbEntryTracerData.MemoryAccessed >= glbEntryTracerData.LoadedImageBase && glbEntryTracerData.MemoryAccessed <= glbEntryTracerData.LoadedImageBase + glbEntryTracerData.SizeOfImage){
			for(i = 0; i < glbEntryTracerData.SectionNumber; i++){
				if(glbEntryTracerData.MemoryAccessed >= glbEntryTracerData.SectionData[i].SectionVirtualOffset + glbEntryTracerData.LoadedImageBase && glbEntryTracerData.MemoryAccessed < glbEntryTracerData.SectionData[i].SectionVirtualOffset + glbEntryTracerData.SectionData[i].SectionVirtualSize + glbEntryTracerData.LoadedImageBase){
					if(glbEntryTracerData.AccessType == 1){
						glbEntryTracerData.SectionData[i].AccessedAlready = true;
					}
					if(glbEntryTracerData.MemoryAccessedFrom >= glbEntryTracerData.SectionData[i].SectionVirtualOffset + glbEntryTracerData.LoadedImageBase && glbEntryTracerData.MemoryAccessedFrom <= glbEntryTracerData.SectionData[i].SectionVirtualOffset + glbEntryTracerData.SectionData[i].SectionVirtualSize + glbEntryTracerData.LoadedImageBase){
						if(i != glbEntryTracerData.OriginalEntryPointNum){
							glbEntryTracerData.SectionData[i].AccessedAlready = true;
						}
						lpHashBuffer = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
						memBpxAddress = (glbEntryTracerData.MemoryAccessed / 0x1000) * 0x1000;
						memBpxSize = glbEntryTracerData.SectionData[i].SectionVirtualOffset + glbEntryTracerData.SectionData[i].SectionVirtualSize + glbEntryTracerData.LoadedImageBase - memBpxAddress;
						if(memBpxSize > 0x1000){
							memBpxSize = 0x1000;
						}
						if(ReadProcessMemory(dbgProcessInformation.hProcess, (void*)(memBpxAddress), lpHashBuffer, memBpxSize, &NumberOfBytesRW)){
							currentHash = EngineHashMemory((char*)lpHashBuffer, (DWORD)memBpxSize, NULL);
							originalHash = EngineHashMemory((char*)((ULONG_PTR)glbEntryTracerData.SectionData[i].AllocatedSection + memBpxAddress - glbEntryTracerData.LoadedImageBase - glbEntryTracerData.SectionData[i].SectionVirtualOffset), (DWORD)memBpxSize, NULL);
							if(ReadProcessMemory(dbgProcessInformation.hProcess, (void*)(glbEntryTracerData.CurrentIntructionPointer), lpHashBuffer, MAXIMUM_INSTRUCTION_SIZE, &NumberOfBytesRW)){
								myCmpHandler = (PMEMORY_COMPARE_HANDLER)(lpHashBuffer);
								if(myCmpHandler->Array.bArrayEntry[0] == 0xC3){		// RET
									FakeEPDetected = true;
								}else if(myCmpHandler->Array.bArrayEntry[0] == 0x33 && myCmpHandler->Array.bArrayEntry[1] == 0xC0 && myCmpHandler->Array.bArrayEntry[2] == 0xC3){	// XOR EAX,EAX; RET
									FakeEPDetected = true;
								}
							}
							VirtualFree(lpHashBuffer, NULL, MEM_RELEASE);
							if(currentHash != originalHash && glbEntryTracerData.SectionData[i].AccessedAlready == true && i != glbEntryTracerData.OriginalEntryPointNum && FakeEPDetected == false){
								__try{
									if(glbEntryTracerData.EPCallBack != NULL){
										glbEntryTracerData.CurrentIntructionPointer = (ULONG_PTR)GetContextData(UE_CIP);
										SetContextData(UE_CIP, glbEntryTracerData.MemoryAccessedFrom);
										DeleteAPIBreakPoint("kernel32.dll", "VirtualProtect", UE_APIEND);
										RemoveAllBreakPoints(UE_OPTION_REMOVEALL);
										myEPCallBack();
										SetContextData(UE_CIP, glbEntryTracerData.CurrentIntructionPointer);
									}else{
										StopDebug();
									}
								}__except(EXCEPTION_EXECUTE_HANDLER){
									StopDebug();
								}
							}
						}
					}else{
						SetMemoryBPXEx((ULONG_PTR)(glbEntryTracerData.SectionData[i].SectionVirtualOffset + glbEntryTracerData.LoadedImageBase), glbEntryTracerData.SectionData[i].SectionVirtualSize, UE_MEMORY, false, &GenericOEPTraceHit);
					}
				}else{
					SetMemoryBPXEx((ULONG_PTR)(glbEntryTracerData.SectionData[i].SectionVirtualOffset + glbEntryTracerData.LoadedImageBase), glbEntryTracerData.SectionData[i].SectionVirtualSize, UE_MEMORY, false, &GenericOEPTraceHit);
				}
			}
		}
	}else{
		StopDebug();
	}
}
void __stdcall GenericOEPLibraryDetailsHit(){

	int i;
	bool memBreakPointSet = false;
	char szModuleName[2 * MAX_PATH] = {};
	#if !defined(_WIN64)
		int inReg = UE_EAX;
	#else
		int inReg = UE_RAX;
	#endif

	if(GetModuleBaseNameA(dbgProcessInformation.hProcess, (HMODULE)GetContextData(inReg), szModuleName, sizeof szModuleName) > NULL){
		if(lstrcmpiA(szModuleName, "kernel32.dll") != NULL){
			if(glbEntryTracerData.FileIsDLL){
				glbEntryTracerData.LoadedImageBase = (ULONG_PTR)GetDebuggedDLLBaseAddress();
			}else{
				glbEntryTracerData.LoadedImageBase = (ULONG_PTR)GetDebuggedFileBaseAddress();
			}
			for(i = 0; i < glbEntryTracerData.SectionNumber; i++){
				if(glbEntryTracerData.SectionData[i].SectionAttributes & IMAGE_SCN_MEM_EXECUTE || glbEntryTracerData.SectionData[i].SectionAttributes & IMAGE_SCN_CNT_CODE){
					SetMemoryBPXEx((ULONG_PTR)(glbEntryTracerData.SectionData[i].SectionVirtualOffset + glbEntryTracerData.LoadedImageBase), glbEntryTracerData.SectionData[i].SectionVirtualSize, UE_MEMORY, false, &GenericOEPTraceHit);
					memBreakPointSet = true;
				}
			}
			if(!memBreakPointSet){
				StopDebug();
			}else{
				DeleteAPIBreakPoint("kernel32.dll", "GetModuleHandleW", UE_APIEND);
				DeleteAPIBreakPoint("kernel32.dll", "LoadLibraryExW", UE_APIEND);
			}
		}
	}
}
void __stdcall GenericOEPTraceInit(){

	int i;
	void* lpHashBuffer;
	ULONG_PTR NumberOfBytesRW;
	typedef void(__stdcall *fInitCallBack)();
	fInitCallBack myInitCallBack = (fInitCallBack)glbEntryTracerData.InitCallBack;

	if(glbEntryTracerData.FileIsDLL){
		glbEntryTracerData.LoadedImageBase = (ULONG_PTR)GetDebuggedDLLBaseAddress();
	}else{
		glbEntryTracerData.LoadedImageBase = (ULONG_PTR)GetDebuggedFileBaseAddress();
	}
	for(i = 0; i < glbEntryTracerData.SectionNumber; i++){
		lpHashBuffer = VirtualAlloc(NULL, glbEntryTracerData.SectionData[i].SectionVirtualSize, MEM_COMMIT, PAGE_READWRITE);
		if(lpHashBuffer != NULL){
			if(ReadProcessMemory(dbgProcessInformation.hProcess, (void*)(glbEntryTracerData.SectionData[i].SectionVirtualOffset + glbEntryTracerData.LoadedImageBase), lpHashBuffer, glbEntryTracerData.SectionData[i].SectionVirtualSize, &NumberOfBytesRW)){
				glbEntryTracerData.SectionData[i].AllocatedSection = lpHashBuffer;
			}
		}
	}
	SetAPIBreakPoint("kernel32.dll", "VirtualProtect", UE_BREAKPOINT, UE_APIEND, &GenericOEPVirtualProtectHit);
	SetAPIBreakPoint("kernel32.dll", "GetModuleHandleW", UE_BREAKPOINT, UE_APIEND, &GenericOEPLibraryDetailsHit);
	SetAPIBreakPoint("kernel32.dll", "LoadLibraryExW", UE_BREAKPOINT, UE_APIEND, &GenericOEPLibraryDetailsHit);
	if(glbEntryTracerData.InitCallBack != NULL){
		__try{
			myInitCallBack();
		}__except(EXCEPTION_EXECUTE_HANDLER){
			StopDebug();
		}
	}
}
bool __stdcall GenericOEPFileInit(char* szFileName, LPVOID TraceInitCallBack, LPVOID CallBack){

	int i;
#if defined(_WIN64)
	PE64Struct PEStruct = {};
#else
	PE32Struct PEStruct = {};
#endif
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		if(GetPE32DataFromMappedFileEx(FileMapVA, &PEStruct)){
			RtlZeroMemory(&glbEntryTracerData, sizeof GenericOEPTracerData);
			glbEntryTracerData.OriginalImageBase = PEStruct.ImageBase;
			glbEntryTracerData.OriginalEntryPoint = PEStruct.OriginalEntryPoint;
			glbEntryTracerData.SizeOfImage = PEStruct.NtSizeOfImage;
			glbEntryTracerData.SectionNumber = PEStruct.SectionNumber;
			glbEntryTracerData.FileIsDLL = IsFileDLL(NULL, FileMapVA);
			glbEntryTracerData.OriginalEntryPointNum = GetPE32SectionNumberFromVA(FileMapVA, glbEntryTracerData.OriginalImageBase + glbEntryTracerData.OriginalEntryPoint);
			for(i = 0; i < glbEntryTracerData.SectionNumber; i++){
				glbEntryTracerData.SectionData[i].SectionVirtualOffset = (DWORD)GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONVIRTUALOFFSET);
				glbEntryTracerData.SectionData[i].SectionVirtualSize = (DWORD)GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONVIRTUALSIZE);
				if(glbEntryTracerData.SectionData[i].SectionVirtualSize % 0x1000 != 0){
					glbEntryTracerData.SectionData[i].SectionVirtualSize = ((glbEntryTracerData.SectionData[i].SectionVirtualSize / 0x1000) + 1) * 0x1000;
				}else{
					glbEntryTracerData.SectionData[i].SectionVirtualSize = (glbEntryTracerData.SectionData[i].SectionVirtualSize / 0x1000) * 0x1000;
				}
				glbEntryTracerData.SectionData[i].SectionAttributes = (DWORD)GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONFLAGS);
			}
			glbEntryTracerData.EPCallBack = CallBack;
			glbEntryTracerData.InitCallBack = TraceInitCallBack;
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			if(glbEntryTracerData.FileIsDLL){
				return(false);
			}else{
				return(true);
			}
		}else{
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
		}
	}
	return(false);
}
// UnpackEngine.FindOEP.functions:
__declspec(dllexport) void __stdcall FindOEPInit(){
	RemoveAllBreakPoints(UE_OPTION_REMOVEALL);
}
__declspec(dllexport) bool __stdcall FindOEPGenerically(char* szFileName, LPVOID TraceInitCallBack, LPVOID CallBack){

	int i;

	if(GenericOEPFileInit(szFileName, TraceInitCallBack, CallBack)){
		InitDebugEx(szFileName, NULL, NULL, &GenericOEPTraceInit);
		DebugLoop();
		for(i = 0; i < glbEntryTracerData.SectionNumber; i++){
			VirtualFree(glbEntryTracerData.SectionData[i].AllocatedSection, NULL, MEM_RELEASE);
		}
	}
	return(false);
}
// UnpackEngine.Importer.functions:
__declspec(dllexport) void __stdcall ImporterCleanup(){

	int i = 0;

	for(i = 0;i < 1000; i++){
		if(impDLLDataList[i][0] != NULL){
			VirtualFree((LPVOID)(impDLLDataList[i][0]), NULL, MEM_RELEASE);
			impDLLDataList[i][0] = 0;
			impDLLDataList[i][1] = 0;
		}
		if(impDLLStringList[i][0] != NULL){
			VirtualFree((LPVOID)(impDLLStringList[i][0]), NULL, MEM_RELEASE);
			impDLLStringList[i][0] = 0;
			impDLLStringList[i][1] = 0;
		}
		impOrdinalList[i][0] = 0;
		impOrdinalList[i][1] = 0;
	}
}
__declspec(dllexport) void __stdcall ImporterSetImageBase(ULONG_PTR ImageBase){
	impImageBase = ImageBase;
}
__declspec(dllexport) void __stdcall ImporterSetUnknownDelta(ULONG_PTR DeltaAddress){

	impDeltaStart = DeltaAddress;
	impDeltaCurrent = DeltaAddress;
}
__declspec(dllexport) long long __stdcall ImporterGetCurrentDelta(){
	return((ULONG_PTR)impDeltaCurrent);
}
__declspec(dllexport) void __stdcall ImporterInit(DWORD MemorySize, ULONG_PTR ImageBase){

	impImageBase = ImageBase;
	if(MemorySize != NULL){
		impAllocSize = MemorySize;
	}else{
		impAllocSize = 20 * 1024;
	}
	ImporterCleanup();
	impMoveIAT = false;
	impDLLNumber = -1;
	impDeltaStart = NULL;
	impDeltaCurrent = NULL;
}
__declspec(dllexport) void __stdcall ImporterAddNewDll(char* szDLLName, ULONG_PTR FirstThunk){

	int CopyDummy = 1;

	impDLLNumber++;
	impDLLDataList[impDLLNumber][0] = (ULONG_PTR)(VirtualAlloc(NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE));
	impDLLDataList[impDLLNumber][1] = impDLLDataList[impDLLNumber][0];
	impDLLStringList[impDLLNumber][0] = (ULONG_PTR)(VirtualAlloc(NULL, impAllocSize, MEM_COMMIT, PAGE_READWRITE));
	impDLLStringList[impDLLNumber][1] = impDLLStringList[impDLLNumber][0];
	RtlMoveMemory((LPVOID)(impDLLDataList[impDLLNumber][1]), &FirstThunk, sizeof ULONG_PTR);
	RtlMoveMemory((LPVOID)(impDLLDataList[impDLLNumber][1] + sizeof ULONG_PTR), &FirstThunk, sizeof ULONG_PTR);
	RtlMoveMemory((LPVOID)(impDLLDataList[impDLLNumber][1] + 2 * sizeof ULONG_PTR), &CopyDummy, 4);
#if !defined(_WIN64)
	impDLLDataList[impDLLNumber][1] = impDLLDataList[impDLLNumber][0] + 12;
#else
	impDLLDataList[impDLLNumber][1] = impDLLDataList[impDLLNumber][0] + 20;
#endif
	RtlMoveMemory((LPVOID)(impDLLStringList[impDLLNumber][1]), szDLLName, lstrlenA((LPCSTR)szDLLName));
	impDLLStringList[impDLLNumber][1] = impDLLStringList[impDLLNumber][1] + lstrlenA((LPCSTR)szDLLName) + 3;
	if(FirstThunk == NULL && impDeltaStart != NULL){
		impDeltaCurrent = impDeltaCurrent + sizeof ULONG_PTR;
	}
}
__declspec(dllexport) void __stdcall ImporterAddNewAPI(char* szAPIName, ULONG_PTR ThunkValue){

	int i = NULL;
	int CopyDummy = NULL;
	ULONG_PTR LastThunkValue = NULL;
	
	RtlMoveMemory(&LastThunkValue, (LPVOID)(impDLLDataList[impDLLNumber][0] + sizeof ULONG_PTR), sizeof ULONG_PTR);
	if(ThunkValue == NULL && impDeltaCurrent != NULL){
		ThunkValue = impDeltaCurrent;
		impDeltaCurrent = impDeltaCurrent + sizeof ULONG_PTR;
	}	
	if(LastThunkValue != NULL && LastThunkValue != ThunkValue){
		ImporterAddNewDll((char*)(LPVOID)impDLLStringList[impDLLNumber][0], ThunkValue);
	}else{
		if(LastThunkValue != NULL){
			LastThunkValue = LastThunkValue + sizeof ULONG_PTR;
		}else{
			RtlMoveMemory((LPVOID)(impDLLDataList[impDLLNumber][0]), &ThunkValue, sizeof ULONG_PTR);
			LastThunkValue = ThunkValue + sizeof ULONG_PTR;
		}
		RtlMoveMemory((LPVOID)(impDLLDataList[impDLLNumber][0] + sizeof ULONG_PTR), &LastThunkValue, sizeof ULONG_PTR);	
	}
	CopyDummy = (int)(impDLLStringList[impDLLNumber][1] - impDLLStringList[impDLLNumber][0]);
	RtlMoveMemory((LPVOID)(impDLLDataList[impDLLNumber][1]), &CopyDummy, 4);	
	impDLLDataList[impDLLNumber][1] = impDLLDataList[impDLLNumber][1] + 4;
	if((ULONG_PTR)szAPIName > 0x10000){
		RtlMoveMemory((LPVOID)(impDLLStringList[impDLLNumber][1] + 2), szAPIName, lstrlenA((LPCSTR)szAPIName));
		impDLLStringList[impDLLNumber][1] = impDLLStringList[impDLLNumber][1] + lstrlenA((LPCSTR)szAPIName) + 3;
	}else{
		for(i = 0; i < 1000; i++){
			if(impOrdinalList[i][0] == NULL){
				break;
			}
		}
		if(i < 1000){
			impOrdinalList[i][0] = ThunkValue;
			if(sizeof ULONG_PTR == 4){
				impOrdinalList[i][1] = (ULONG_PTR)szAPIName ^ IMAGE_ORDINAL_FLAG32;
			}else{
				impOrdinalList[i][1] = (ULONG_PTR)((ULONG_PTR)szAPIName ^ IMAGE_ORDINAL_FLAG64);
			}
		}
	}
	RtlMoveMemory(&CopyDummy, (LPVOID)(impDLLDataList[impDLLNumber][0] + 2 * sizeof ULONG_PTR), 4);
	CopyDummy++;
	RtlMoveMemory((LPVOID)(impDLLDataList[impDLLNumber][0] + 2 * sizeof ULONG_PTR), &CopyDummy, 4);
}
__declspec(dllexport) long __stdcall ImporterGetAddedDllCount(){
	return(impDLLNumber + 1);
}
__declspec(dllexport) long __stdcall ImporterGetAddedAPICount(){

	int i = 0;
	int CopyDummy = NULL;
	DWORD DLLNumber = NULL;
	long APINumber = NULL;

	DLLNumber = impDLLNumber + 1;
	while(DLLNumber > NULL){
		RtlMoveMemory(&CopyDummy, (LPVOID)(impDLLDataList[i][0] + 2 * sizeof ULONG_PTR), 4);
		APINumber = APINumber + CopyDummy - 1;
		DLLNumber--;
		i++;
	}
	return(APINumber);
}
__declspec(dllexport) void __stdcall ImporterMoveIAT(){
	impMoveIAT = true;
}
__declspec(dllexport) bool __stdcall ImporterExportIAT(ULONG_PTR StorePlace, ULONG_PTR FileMapVA){

	int i = 0;
	int j = 0;
	int x = 0;
	int NumberOfAPIs = NULL;
	DWORD DLLNumber = NULL;
	DWORD APINumber = NULL;
	PIMAGE_IMPORT_DESCRIPTOR StoreIID = (PIMAGE_IMPORT_DESCRIPTOR)StorePlace;
	ULONG_PTR StorePlaceVA = (ULONG_PTR)ConvertFileOffsetToVA(FileMapVA, StorePlace, true);
	ULONG_PTR OriginalStorePlaceRVA = (DWORD)(StorePlaceVA - impImageBase);
	ULONG_PTR StringStorePlaceFO = StorePlace + ((impDLLNumber + 2) * sizeof IMAGE_IMPORT_DESCRIPTOR);
	ULONG_PTR StringStorePlaceVA = StorePlaceVA + ((impDLLNumber + 2) * sizeof IMAGE_IMPORT_DESCRIPTOR);
	ULONG_PTR ThunkStorePlaceFO = NULL;
	ULONG_PTR ThunkReadValue = NULL;
	LPVOID ThunkReadPlace = NULL;
	ULONG_PTR FirstThunk = NULL;
	ULONG_PTR CurrentThunk = NULL;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	bool FileIs64 = false;
	bool OrdinalImport = false;

	if(ImporterGetAddedDllCount() > NULL){
		if(impMoveIAT){
			NumberOfAPIs = ImporterGetAddedAPICount() + ImporterGetAddedDllCount();
			StorePlaceVA = StorePlaceVA + (NumberOfAPIs * sizeof ULONG_PTR);
			StringStorePlaceFO = StringStorePlaceFO + (NumberOfAPIs * sizeof ULONG_PTR);
			StringStorePlaceVA = StringStorePlaceVA + (NumberOfAPIs * sizeof ULONG_PTR);
			OriginalStorePlaceRVA = OriginalStorePlaceRVA + (NumberOfAPIs * sizeof ULONG_PTR);
			StoreIID = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG_PTR)StorePlace + (NumberOfAPIs * sizeof ULONG_PTR));
		}

		__try{
			DLLNumber = impDLLNumber + 1;
			while(DLLNumber > NULL){
				RtlMoveMemory(&FirstThunk, (LPVOID)impDLLDataList[i][0], sizeof ULONG_PTR);
				StoreIID->FirstThunk = (DWORD)(FirstThunk - impImageBase);
				StoreIID->Name = (DWORD)(StringStorePlaceVA - impImageBase);
				RtlMoveMemory((LPVOID)StringStorePlaceFO, (LPVOID)impDLLStringList[i][0], (int)(impDLLStringList[i][1] - impDLLStringList[i][0]));
				StringStorePlaceFO = StringStorePlaceFO + (int)(impDLLStringList[i][1] - impDLLStringList[i][0]);			
		#if !defined(_WIN64)
				ThunkReadPlace = (LPVOID)(impDLLDataList[i][0] + 12);
		#else
				ThunkReadPlace = (LPVOID)(impDLLDataList[i][0] + 20);
		#endif
				ThunkStorePlaceFO = (ULONG_PTR)ConvertVAtoFileOffset(FileMapVA, FirstThunk, true);
				RtlMoveMemory(&APINumber, (LPVOID)(impDLLDataList[i][0] + 2 * sizeof ULONG_PTR), 4);
				CurrentThunk = FirstThunk;
				APINumber--;
				while(APINumber > NULL){
					OrdinalImport = false;
					for(j = 0; j < 1000; j++){
						if(impOrdinalList[j][0] == CurrentThunk){
							OrdinalImport = true;
							x = j;
							j = 1000;
						}else if(impOrdinalList[j][0] == NULL){
							j = 1000;
						}
					}
					if(!OrdinalImport){
						RtlMoveMemory(&ThunkReadValue, ThunkReadPlace, 4);
						ThunkReadValue = ThunkReadValue + StringStorePlaceVA - impImageBase;
						RtlMoveMemory((LPVOID)ThunkStorePlaceFO, &ThunkReadValue, sizeof ULONG_PTR);
						ThunkReadPlace = (LPVOID)((ULONG_PTR)ThunkReadPlace + 4);
						ThunkStorePlaceFO = ThunkStorePlaceFO + sizeof ULONG_PTR;
					}else{
						j = x;
						ThunkReadValue = impOrdinalList[j][1];
						RtlMoveMemory((LPVOID)ThunkStorePlaceFO, &ThunkReadValue, sizeof ULONG_PTR);
						ThunkReadPlace = (LPVOID)((ULONG_PTR)ThunkReadPlace + 4);
						ThunkStorePlaceFO = ThunkStorePlaceFO + sizeof ULONG_PTR;
					}
					CurrentThunk = CurrentThunk + sizeof ULONG_PTR;
					APINumber--;
				}
				ThunkReadValue = 0;
				RtlMoveMemory((LPVOID)ThunkStorePlaceFO, &ThunkReadValue, sizeof ULONG_PTR);

				StorePlaceVA = StorePlaceVA + (int)(impDLLStringList[i][1] - impDLLStringList[i][0]);
				StringStorePlaceVA = StringStorePlaceVA + (int)(impDLLStringList[i][1] - impDLLStringList[i][0]);
				StoreIID = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG_PTR)StoreIID + sizeof IMAGE_IMPORT_DESCRIPTOR);
				DLLNumber--;
				i++;
			}

			DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
			if(EngineValidateHeader(FileMapVA, NULL, NULL, DOSHeader, true)){
				PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
				PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
				if(PEHeader32->OptionalHeader.Magic == 0x10B){
					FileIs64 = false;
				}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
					FileIs64 = true;
				}
				if(!FileIs64){
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = (DWORD)OriginalStorePlaceRVA;
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = (DWORD)((impDLLNumber + 2) * sizeof IMAGE_IMPORT_DESCRIPTOR);
				}else{
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = (DWORD)OriginalStorePlaceRVA;
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = (DWORD)((impDLLNumber + 2) * sizeof IMAGE_IMPORT_DESCRIPTOR);
				}
			}
			ImporterCleanup();
			return(true);
		}__except(EXCEPTION_EXECUTE_HANDLER){
			ImporterCleanup();
			return(false);
		}
	}else{
		return(false);
	}
}
__declspec(dllexport) long __stdcall ImporterEstimatedSize(){

	int i = 0;
	DWORD DLLNumber = NULL;
	long EstimatedSize = 0x200;

	EstimatedSize = EstimatedSize + ((impDLLNumber + 2) * sizeof IMAGE_IMPORT_DESCRIPTOR);
	DLLNumber = impDLLNumber + 1;
	while(DLLNumber > NULL){
		EstimatedSize = EstimatedSize + (DWORD)(impDLLStringList[i][1] - impDLLStringList[i][0]);
		DLLNumber--;
		i++;
	}
	return(EstimatedSize);
}
__declspec(dllexport) bool __stdcall ImporterExportIATEx(char* szExportFileName, char* szSectionName){
	
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	DWORD NewSectionVO = NULL;
	DWORD NewSectionFO = NULL;
	bool ReturnValue = false;
	
	if(ImporterGetAddedDllCount() > NULL){
		NewSectionVO = AddNewSection(szExportFileName, szSectionName, ImporterEstimatedSize());
		if(MapFileEx(szExportFileName, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
			NewSectionFO = (DWORD)ConvertVAtoFileOffset(FileMapVA, NewSectionVO + impImageBase, true);
			ReturnValue = ImporterExportIAT(NewSectionFO, FileMapVA);
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			if(ReturnValue){
				return(true);
			}else{
				return(false);
			}
		}else{
			return(false);
		}
	}else{
		return(false);
	}
}
__declspec(dllexport) long long __stdcall ImporterFindAPIWriteLocation(char* szAPIName){

	int i = 0;
	DWORD DLLNumber = NULL;
	DWORD NumberOfAPIs = NULL;
	LPVOID NameReadPlace = NULL;
	ULONG_PTR CurrentAPILocation = NULL;
	DWORD APINameRelativeOffset = NULL;
	ULONG_PTR APIWriteLocation = NULL;

	if(ImporterGetAddedDllCount() > NULL){
		DLLNumber = impDLLNumber + 1;
		while(DLLNumber > NULL){
	#if !defined(_WIN64)
			NameReadPlace = (LPVOID)(impDLLDataList[i][0] + 12);
	#else
			NameReadPlace = (LPVOID)(impDLLDataList[i][0] + 20);
	#endif
			RtlMoveMemory(&CurrentAPILocation, (LPVOID)(impDLLDataList[i][0]), sizeof ULONG_PTR);
			RtlMoveMemory(&NumberOfAPIs, (LPVOID)(impDLLDataList[i][0] + 2 * sizeof ULONG_PTR), 4);
			while(NumberOfAPIs > NULL){
				RtlMoveMemory(&APINameRelativeOffset, NameReadPlace, 4);
				if(lstrcmpiA((LPCSTR)((ULONG_PTR)impDLLStringList[i][0] + APINameRelativeOffset + 2), (LPCSTR)szAPIName) == NULL){
					APIWriteLocation = CurrentAPILocation;
					break;
				}
				CurrentAPILocation = CurrentAPILocation + sizeof ULONG_PTR;
				NameReadPlace = (LPVOID)((ULONG_PTR)NameReadPlace + sizeof ULONG_PTR);
				NumberOfAPIs--;
			}
			DLLNumber--;
			i++;
		}
		return(APIWriteLocation);
	}
	return(NULL);
}
__declspec(dllexport) long long __stdcall ImporterFindAPIByWriteLocation(ULONG_PTR APIWriteLocation){

	int i = 0;
	DWORD DLLNumber = NULL;
	LPVOID NameReadPlace = NULL;
	ULONG_PTR MinAPILocation = NULL;
	ULONG_PTR MaxAPILocation = NULL;
	DWORD APINameRelativeOffset = NULL;
	ULONG_PTR APINameOffset = NULL;

	if(ImporterGetAddedDllCount() > NULL){
		DLLNumber = impDLLNumber + 1;
		while(DLLNumber > NULL){
	#if !defined(_WIN64)
			NameReadPlace = (LPVOID)(impDLLDataList[i][0] + 12);
	#else
			NameReadPlace = (LPVOID)(impDLLDataList[i][0] + 20);
	#endif
			RtlMoveMemory(&MinAPILocation, (LPVOID)(impDLLDataList[i][0]), sizeof ULONG_PTR);
			RtlMoveMemory(&MaxAPILocation, (LPVOID)(impDLLDataList[i][0] + sizeof ULONG_PTR), sizeof ULONG_PTR);
			if(MinAPILocation <= APIWriteLocation && APIWriteLocation <= MaxAPILocation){
				RtlMoveMemory(&APINameRelativeOffset, (LPVOID)((ULONG_PTR)NameReadPlace + (APIWriteLocation - MinAPILocation)), 4);			
				return((ULONG_PTR)(impDLLStringList[i][0] + APINameRelativeOffset + 2));
			}
			DLLNumber--;
			i++;
		}
	}
	return(NULL);
}
__declspec(dllexport) long long __stdcall ImporterFindDLLByWriteLocation(ULONG_PTR APIWriteLocation){

	int i = 0;
	DWORD DLLNumber = NULL;
	LPVOID NameReadPlace = NULL;
	ULONG_PTR MinAPILocation = NULL;
	ULONG_PTR MaxAPILocation = NULL;
	DWORD APINameRelativeOffset = NULL;
	ULONG_PTR APINameOffset = NULL;

	if(ImporterGetAddedDllCount() > NULL){
		DLLNumber = impDLLNumber + 1;
		while(DLLNumber > NULL){
	#if !defined(_WIN64)
			NameReadPlace = (LPVOID)(impDLLDataList[i][0] + 12);
	#else
			NameReadPlace = (LPVOID)(impDLLDataList[i][0] + 20);
	#endif
			RtlMoveMemory(&MinAPILocation, (LPVOID)(impDLLDataList[i][0]), sizeof ULONG_PTR);
			RtlMoveMemory(&MaxAPILocation, (LPVOID)(impDLLDataList[i][0] + sizeof ULONG_PTR), sizeof ULONG_PTR);
			if(MinAPILocation <= APIWriteLocation && APIWriteLocation <= MaxAPILocation){
				return((ULONG_PTR)(impDLLStringList[i][0]));
			}
			DLLNumber--;
			i++;
		}
	}
	return(NULL);
}
__declspec(dllexport) void* __stdcall ImporterGetAPIName(ULONG_PTR APIAddress){
	return((LPVOID)EngineGlobalAPIHandler(NULL, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_APINAME));
}
__declspec(dllexport) void* __stdcall ImporterGetAPINameEx(ULONG_PTR APIAddress, ULONG_PTR DLLBasesList){
	return((LPVOID)EngineGlobalAPIHandler(NULL, DLLBasesList, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_APINAME));
}
__declspec(dllexport) long long __stdcall ImporterGetRemoteAPIAddress(HANDLE hProcess, ULONG_PTR APIAddress){
	return((ULONG_PTR)EngineGlobalAPIHandler(hProcess, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_REALIGN_APIADDRESS));
}
__declspec(dllexport) long long __stdcall ImporterGetRemoteAPIAddressEx(char* szDLLName, char* szAPIName){

	int i = 0;
	int j = 0;
	PLIBRARY_ITEM_DATA hListLibraryPtr = NULL;
	ULONG_PTR APIFoundAddress = 0;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_EXPORT_DIRECTORY PEExports;
	PEXPORTED_DATA ExportedFunctions;
	PEXPORTED_DATA ExportedFunctionNames;
	PEXPORTED_DATA_WORD ExportedFunctionOrdinals;
	bool FileIs64 = false;

	hListLibraryPtr = (PLIBRARY_ITEM_DATA)hListLibrary;
	if(hListLibraryPtr != NULL){
		while(hListLibraryPtr->hFile != NULL){
			if(lstrcmpiA(hListLibraryPtr->szLibraryName, szDLLName) == NULL){
				__try{
					DOSHeader = (PIMAGE_DOS_HEADER)hListLibraryPtr->hFileMappingView;
					PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
					PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
					if(PEHeader32->OptionalHeader.Magic == 0x10B){
						FileIs64 = false;
					}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
						FileIs64 = true;
					}else{
						return(NULL);
					}
					if(!FileIs64){
						PEExports = (PIMAGE_EXPORT_DIRECTORY)((ULONG_PTR)ConvertVAtoFileOffsetEx((ULONG_PTR)hListLibraryPtr->hFileMappingView, GetFileSize(hListLibraryPtr->hFile, NULL), (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase, PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress, true, true));
						ExportedFunctions = (PEXPORTED_DATA)((ULONG_PTR)ConvertVAtoFileOffsetEx((ULONG_PTR)hListLibraryPtr->hFileMappingView, GetFileSize(hListLibraryPtr->hFile, NULL), (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase, PEExports->AddressOfFunctions, true, true));
						ExportedFunctionNames = (PEXPORTED_DATA)((ULONG_PTR)ConvertVAtoFileOffsetEx((ULONG_PTR)hListLibraryPtr->hFileMappingView, GetFileSize(hListLibraryPtr->hFile, NULL), (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase, PEExports->AddressOfNames, true, true));
						ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)((ULONG_PTR)ConvertVAtoFileOffsetEx((ULONG_PTR)hListLibraryPtr->hFileMappingView, GetFileSize(hListLibraryPtr->hFile, NULL), (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase, PEExports->AddressOfNameOrdinals, true, true));
					}else{
						PEExports = (PIMAGE_EXPORT_DIRECTORY)((ULONG_PTR)ConvertVAtoFileOffsetEx((ULONG_PTR)hListLibraryPtr->hFileMappingView, GetFileSize(hListLibraryPtr->hFile, NULL), (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress, true, true));
						ExportedFunctions = (PEXPORTED_DATA)((ULONG_PTR)ConvertVAtoFileOffsetEx((ULONG_PTR)hListLibraryPtr->hFileMappingView, GetFileSize(hListLibraryPtr->hFile, NULL), (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, PEExports->AddressOfFunctions, true, true));
						ExportedFunctionNames = (PEXPORTED_DATA)((ULONG_PTR)ConvertVAtoFileOffsetEx((ULONG_PTR)hListLibraryPtr->hFileMappingView, GetFileSize(hListLibraryPtr->hFile, NULL), (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, PEExports->AddressOfNames, true, true));
						ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)((ULONG_PTR)ConvertVAtoFileOffsetEx((ULONG_PTR)hListLibraryPtr->hFileMappingView, GetFileSize(hListLibraryPtr->hFile, NULL), (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, PEExports->AddressOfNameOrdinals, true, true));
					}
					for(j = 0; j <= (int)PEExports->NumberOfNames; j++){
						if(!FileIs64){
							if(lstrcmpiA((LPCSTR)szAPIName, (LPCSTR)((ULONG_PTR)ConvertVAtoFileOffsetEx((ULONG_PTR)hListLibraryPtr->hFileMappingView, GetFileSize(hListLibraryPtr->hFile, NULL), (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase, ExportedFunctionNames->ExportedItem, true, true))) == NULL){
								ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)((ULONG_PTR)ExportedFunctionOrdinals + j * 2);
								ExportedFunctions = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctions + (ExportedFunctionOrdinals->OrdinalNumber) * 4);
								APIFoundAddress = ExportedFunctions->ExportedItem + (ULONG_PTR)hListLibraryPtr->BaseOfDll;
								return((ULONG_PTR)APIFoundAddress);
							}
						}else{
							if(lstrcmpiA((LPCSTR)szAPIName, (LPCSTR)((ULONG_PTR)ConvertVAtoFileOffsetEx((ULONG_PTR)hListLibraryPtr->hFileMappingView, GetFileSize(hListLibraryPtr->hFile, NULL), (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, ExportedFunctionNames->ExportedItem, true, true))) == NULL){
								ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)((ULONG_PTR)ExportedFunctionOrdinals + j * 2);
								ExportedFunctions = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctions + (ExportedFunctionOrdinals->OrdinalNumber) * 4);
								APIFoundAddress = ExportedFunctions->ExportedItem + (ULONG_PTR)hListLibraryPtr->BaseOfDll;
								return((ULONG_PTR)APIFoundAddress);
							}
						}
						ExportedFunctionNames = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctionNames + 4);
					}
					return(NULL);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(NULL);
				}
			}
			hListLibraryPtr = (PLIBRARY_ITEM_DATA)((ULONG_PTR)hListLibraryPtr + sizeof LIBRARY_ITEM_DATA);
		}
	}
	return(NULL);
}
__declspec(dllexport) long long __stdcall ImporterGetLocalAPIAddress(HANDLE hProcess, ULONG_PTR APIAddress){
	return((ULONG_PTR)EngineGlobalAPIHandler(hProcess, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_REALIGN_LOCAL_APIADDRESS));
}
__declspec(dllexport) void* __stdcall ImporterGetDLLNameFromDebugee(HANDLE hProcess, ULONG_PTR APIAddress){
	return((LPVOID)EngineGlobalAPIHandler(hProcess, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_DLLNAME));
}
__declspec(dllexport) void* __stdcall ImporterGetAPINameFromDebugee(HANDLE hProcess, ULONG_PTR APIAddress){
	return((LPVOID)EngineGlobalAPIHandler(hProcess, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_APINAME));
}
__declspec(dllexport) long __stdcall ImporterGetDLLIndexEx(ULONG_PTR APIAddress, ULONG_PTR DLLBasesList){
	return((DWORD)EngineGlobalAPIHandler(NULL, DLLBasesList, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_DLLINDEX));
}
__declspec(dllexport) long __stdcall ImporterGetDLLIndex(HANDLE hProcess, ULONG_PTR APIAddress, ULONG_PTR DLLBasesList){
	return((DWORD)EngineGlobalAPIHandler(hProcess, DLLBasesList, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_DLLINDEX));
}
__declspec(dllexport) long long __stdcall ImporterGetRemoteDLLBase(HANDLE hProcess, HMODULE LocalModuleBase){
	return((ULONG_PTR)EngineGlobalAPIHandler(hProcess, NULL, (ULONG_PTR)LocalModuleBase, NULL, UE_OPTION_IMPORTER_RETURN_DLLBASE));
}
__declspec(dllexport) long long __stdcall ImporterGetRemoteDLLBaseEx(HANDLE hProcess, char* szModuleName){

	int i = 1;
	DWORD Dummy = NULL;
	ULONG_PTR EnumeratedModules[0x2000];
	char RemoteDLLName[MAX_PATH];

	if(EnumProcessModules(hProcess, (HMODULE*)EnumeratedModules, 0x2000, &Dummy)){
		RtlZeroMemory(&RemoteDLLName, MAX_PATH);
		while(EnumeratedModules[i] != NULL){
			if(GetModuleBaseNameA(hProcess, (HMODULE)EnumeratedModules[i], (LPSTR)RemoteDLLName, MAX_PATH) > NULL){
				if(lstrcmpiA((LPCSTR)RemoteDLLName, (LPCSTR)szModuleName)){
					return((ULONG_PTR)EnumeratedModules[i]);
				}
			}
			i++;
		}
	}
	return(NULL);
}
__declspec(dllexport) bool __stdcall ImporterRelocateWriteLocation(ULONG_PTR AddValue){

	unsigned int i;
	ULONG_PTR RealignData = NULL;

	if(impDLLNumber > NULL){
		for(i = 0; i < impDLLNumber; i++){
			RtlMoveMemory(&RealignData, (LPVOID)impDLLDataList[i][0], sizeof ULONG_PTR);
			RealignData = RealignData + AddValue;
			RtlMoveMemory((LPVOID)impDLLDataList[i][0], &RealignData, sizeof ULONG_PTR);
			RtlMoveMemory(&RealignData, (LPVOID)((ULONG_PTR)impDLLDataList[i][0] + sizeof ULONG_PTR), sizeof ULONG_PTR);
			RealignData = RealignData + AddValue;
			RtlMoveMemory((LPVOID)((ULONG_PTR)impDLLDataList[i][0] + sizeof ULONG_PTR), &RealignData, sizeof ULONG_PTR);
		}
		for(i = 0; i < 1000; i++){
			if(impOrdinalList[i][0] != NULL && impOrdinalList[i][1] != NULL){
				impOrdinalList[i][0] = impOrdinalList[i][0] + AddValue;
			}
		}
		return(true);
	}else{
		return(false);
	}
	return(false);
}

__declspec(dllexport) bool __stdcall ImporterIsForwardedAPI(HANDLE hProcess, ULONG_PTR APIAddress){
	if((ULONG_PTR)EngineGlobalAPIHandler(hProcess, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_FORWARDER_DLLINDEX) > NULL){
		return(true);
	}else{
		return(false);
	}
}
__declspec(dllexport) void* __stdcall ImporterGetForwardedAPIName(HANDLE hProcess, ULONG_PTR APIAddress){
	return((LPVOID)EngineGlobalAPIHandler(hProcess, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_FORWARDER_APINAME));
}
__declspec(dllexport) void* __stdcall ImporterGetForwardedDLLName(HANDLE hProcess, ULONG_PTR APIAddress){
	return((LPVOID)EngineGlobalAPIHandler(hProcess, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_FORWARDER_DLLNAME));
}
__declspec(dllexport) long __stdcall ImporterGetForwardedDLLIndex(HANDLE hProcess, ULONG_PTR APIAddress, ULONG_PTR DLLBasesList){
	return((DWORD)EngineGlobalAPIHandler(hProcess, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_FORWARDER_DLLINDEX));
}
__declspec(dllexport) long long __stdcall ImporterGetNearestAPIAddress(HANDLE hProcess, ULONG_PTR APIAddress){
	return((ULONG_PTR)EngineGlobalAPIHandler(hProcess, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_NEAREST_APIADDRESS));
}
__declspec(dllexport) void* __stdcall ImporterGetNearestAPIName(HANDLE hProcess, ULONG_PTR APIAddress){
	return((LPVOID)EngineGlobalAPIHandler(hProcess, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_RETURN_NEAREST_APINAME));
}
__declspec(dllexport) bool __stdcall ImporterCopyOriginalIAT(char* szOriginalFile, char* szDumpFile){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	HANDLE FileHandle1;
	DWORD FileSize1;
	HANDLE FileMap1;
	DWORD FileMapVA1;
	ULONG_PTR IATPointer;
	ULONG_PTR IATWritePointer;
	ULONG_PTR IATCopyStart;
	DWORD IATSection;
	DWORD IATCopySize;
	DWORD IATHeaderData;

	if(MapFileEx(szOriginalFile, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		if(MapFileEx(szDumpFile, UE_ACCESS_ALL, &FileHandle1, &FileSize1, &FileMap1, &FileMapVA1, NULL)){
			DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
			if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
				PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
				PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
				if(PEHeader32->OptionalHeader.Magic == 0x10B){
					FileIs64 = false;
				}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
					FileIs64 = true;
				}else{
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					UnMapFileEx(FileHandle1, FileSize1, FileMap1, FileMapVA1);
					return(false);
				}
				if(!FileIs64){
					IATPointer = (ULONG_PTR)(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase);
				}else{
					IATPointer = (ULONG_PTR)(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + PEHeader64->OptionalHeader.ImageBase);
				}
				IATSection = GetPE32SectionNumberFromVA(FileMapVA, IATPointer);
				IATPointer = (ULONG_PTR)ConvertVAtoFileOffset(FileMapVA, IATPointer, true);
				if((int)IATSection >= NULL){
					IATWritePointer = (ULONG_PTR)GetPE32DataFromMappedFile(FileMapVA1, IATSection, UE_SECTIONRAWOFFSET) + FileMapVA1;
					IATCopyStart = (ULONG_PTR)GetPE32DataFromMappedFile(FileMapVA, IATSection, UE_SECTIONRAWOFFSET) + FileMapVA;
					IATCopySize = (DWORD)GetPE32DataFromMappedFile(FileMapVA1, IATSection, UE_SECTIONRAWSIZE);
					__try{
						RtlMoveMemory((LPVOID)IATWritePointer, (LPVOID)IATCopyStart, IATCopySize);
						IATHeaderData = (DWORD)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_IMPORTTABLEADDRESS);
						SetPE32DataForMappedFile(FileMapVA1, NULL, UE_IMPORTTABLEADDRESS, (ULONG_PTR)IATHeaderData);
						IATHeaderData = (DWORD)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_IMPORTTABLESIZE);
						SetPE32DataForMappedFile(FileMapVA1, NULL, UE_IMPORTTABLESIZE, (ULONG_PTR)IATHeaderData);
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						UnMapFileEx(FileHandle1, FileSize1, FileMap1, FileMapVA1);
						return(true);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						UnMapFileEx(FileHandle1, FileSize1, FileMap1, FileMapVA1);
						return(false);
					}
				}
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				UnMapFileEx(FileHandle1, FileSize1, FileMap1, FileMapVA1);
				return(false);		
			}
		}
	}
	UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
	UnMapFileEx(FileHandle1, FileSize1, FileMap1, FileMapVA1);
	return(false);
}
__declspec(dllexport) bool __stdcall ImporterLoadImportTable(char* szFileName){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_IMPORT_DESCRIPTOR ImportIID;
	PIMAGE_THUNK_DATA32 ThunkData32;
	PIMAGE_THUNK_DATA64 ThunkData64;
	ULONG_PTR CurrentThunk;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(false);
			}
			if(!FileIs64){
				if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress != NULL){
					ImporterInit(MAX_IMPORT_ALLOC, (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase);
					ImportIID = (PIMAGE_IMPORT_DESCRIPTOR)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase), true);
					__try{
						while(ImportIID->FirstThunk != NULL){
							ImporterAddNewDll((char*)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)((ULONG_PTR)ImportIID->Name + PEHeader32->OptionalHeader.ImageBase), true), NULL);
							ThunkData32 = (PIMAGE_THUNK_DATA32)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)((ULONG_PTR)ImportIID->FirstThunk + PEHeader32->OptionalHeader.ImageBase), true);
							CurrentThunk = (ULONG_PTR)ImportIID->FirstThunk;
							while(ThunkData32->u1.AddressOfData != NULL){
								if(ThunkData32->u1.Ordinal & IMAGE_ORDINAL_FLAG32){
									ImporterAddNewAPI((char*)(ThunkData32->u1.Ordinal ^ IMAGE_ORDINAL_FLAG32), (ULONG_PTR)CurrentThunk + PEHeader32->OptionalHeader.ImageBase);
								}else{
									ImporterAddNewAPI((char*)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)((ULONG_PTR)ThunkData32->u1.AddressOfData + 2 + PEHeader32->OptionalHeader.ImageBase), true), (ULONG_PTR)CurrentThunk + PEHeader32->OptionalHeader.ImageBase);
								}
								CurrentThunk = CurrentThunk + 4;
								ThunkData32 = (PIMAGE_THUNK_DATA32)((ULONG_PTR)ThunkData32 + sizeof IMAGE_THUNK_DATA32);
							}
							ImportIID = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG_PTR)ImportIID + sizeof IMAGE_IMPORT_DESCRIPTOR);
						}
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(true);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						ImporterCleanup();
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(false);
					}
				}
			}else{
				if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress != NULL){
					ImporterInit(MAX_IMPORT_ALLOC, (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase);
					ImportIID = (PIMAGE_IMPORT_DESCRIPTOR)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + PEHeader64->OptionalHeader.ImageBase), true);
					__try{
						while(ImportIID->FirstThunk != NULL){
							ImporterAddNewDll((char*)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)((ULONG_PTR)ImportIID->Name + PEHeader64->OptionalHeader.ImageBase), true), NULL);
							ThunkData64 = (PIMAGE_THUNK_DATA64)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)((ULONG_PTR)ImportIID->FirstThunk + PEHeader64->OptionalHeader.ImageBase), true);
							CurrentThunk = (ULONG_PTR)ImportIID->FirstThunk;
							while(ThunkData64->u1.AddressOfData != NULL){
								if(ThunkData64->u1.Ordinal & IMAGE_ORDINAL_FLAG64){
									ImporterAddNewAPI((char*)(ThunkData64->u1.Ordinal ^ (ULONG_PTR)IMAGE_ORDINAL_FLAG64), (ULONG_PTR)CurrentThunk + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase);
								}else{
									ImporterAddNewAPI((char*)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)((ULONG_PTR)ThunkData64->u1.AddressOfData + 2 + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase), true), (ULONG_PTR)CurrentThunk + (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase);
								}
								CurrentThunk = CurrentThunk + 8;
								ThunkData64 = (PIMAGE_THUNK_DATA64)((ULONG_PTR)ThunkData64 + sizeof IMAGE_THUNK_DATA64);
							}
							ImportIID = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG_PTR)ImportIID + sizeof IMAGE_IMPORT_DESCRIPTOR);
						}
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(true);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						ImporterCleanup();
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(false);
					}
				}
			}
		}else{
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			return(false);		
		}
	}else{
		return(false);
	}
	UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
	return(false);
}
__declspec(dllexport) bool __stdcall ImporterMoveOriginalIAT(char* szOriginalFile, char* szDumpFile, char* szSectionName){
	
	if(ImporterLoadImportTable(szOriginalFile)){
		return(ImporterExportIATEx(szDumpFile, szSectionName));
	}
	return(false);
}
__declspec(dllexport) void __stdcall ImporterAutoSearchIAT(HANDLE hProcess, char* szFileName, ULONG_PTR ImageBase, ULONG_PTR SearchStart, DWORD SearchSize, LPVOID pIATStart, LPVOID pIATSize){
	
	int i = NULL;
	int j = NULL;
	int MaximumBlankSpaces;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	MEMORY_BASIC_INFORMATION MemInfo;
	PMEMORY_COMPARE_HANDLER cmpHandler;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	ULONG_PTR IATThunkSize;
	ULONG_PTR SearchStartFO;
	ULONG_PTR OriginalSearchStartFO;
	LPVOID SearchMemory = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	LPVOID memSearchMemory = SearchMemory;
	LPVOID CheckMemory = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	LPVOID memCheckMemory = CheckMemory;
	ULONG_PTR IATThunkPossiblePointer = NULL;
	DWORD IATThunkPossiblePointerFO = NULL;
	WORD IATCmpNearCall = 0x15FF;
	WORD IATCmpNearJump = 0x25FF;
	DWORD NtSizeOfImage = NULL;
	ULONG_PTR IATHigh = NULL;
	ULONG_PTR IATHighestFound = NULL;
	ULONG_PTR IATLow = NULL;
	ULONG_PTR IATLowestFound = NULL;
	ULONG_PTR SearchStartX64 = SearchStart;
	DWORD CurrentSearchSize;
	char* CompareBuffer[12];
	void* LastValidPtr;
	bool ValidPointer;

	RtlZeroMemory(&CompareBuffer, 10 * sizeof ULONG_PTR);
	if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				VirtualFree(SearchMemory, NULL, MEM_RELEASE);
				VirtualFree(CheckMemory, NULL, MEM_RELEASE);
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return;
			}
			if(!FileIs64){
				IATThunkSize = 4;
				NtSizeOfImage = PEHeader32->OptionalHeader.SizeOfImage;
			}else{
				IATThunkSize = 8;
				NtSizeOfImage = PEHeader64->OptionalHeader.SizeOfImage;
			}
			SearchStartFO = (ULONG_PTR)ConvertVAtoFileOffset(FileMapVA, SearchStart, true);
			OriginalSearchStartFO = SearchStartFO;
			if(SearchSize > 0x1000){
				CurrentSearchSize = 0x1000;
			}else{
				CurrentSearchSize = SearchSize;
			}
			while(SearchSize > NULL && EngineGrabDataFromMappedFile(FileHandle, FileMapVA, SearchStartFO, CurrentSearchSize, SearchMemory) == true){
				memSearchMemory = SearchMemory;
				i = CurrentSearchSize - 6;
				while(i > NULL){
					if(memcmp(memSearchMemory, &IATCmpNearCall, 2) == NULL || memcmp(memSearchMemory, &IATCmpNearJump, 2) == NULL){
						RtlMoveMemory(&IATThunkPossiblePointer, (LPVOID)((ULONG_PTR)memSearchMemory + 2), 4);
						if(!FileIs64){
							if(IATThunkPossiblePointer >= (DWORD)ImageBase && IATThunkPossiblePointer <= (DWORD)ImageBase + NtSizeOfImage){
								if(IATHigh == NULL && IATLow == NULL){
									IATThunkPossiblePointerFO = (DWORD)ConvertVAtoFileOffset(FileMapVA, IATThunkPossiblePointer, true);
									if(IATThunkPossiblePointerFO - FileMapVA > 0x1000 && FileSize - (IATThunkPossiblePointerFO - FileMapVA) > 0x1000){
										j = NULL;
										while(j == NULL){
											EngineGrabDataFromMappedFile(FileHandle, FileMapVA, IATThunkPossiblePointerFO, 0x1000, CheckMemory);
											memCheckMemory = CheckMemory;
											LastValidPtr = memCheckMemory;
											MaximumBlankSpaces = 4;
											ValidPointer = true;
											j = 0x1000 - 2 * 4;
											while(j > NULL && ValidPointer == true && MaximumBlankSpaces > NULL){
												if(memcmp(memCheckMemory, &CompareBuffer, sizeof ULONG_PTR) != NULL){
													cmpHandler = (PMEMORY_COMPARE_HANDLER)memCheckMemory;
													VirtualQueryEx(hProcess, (void*)cmpHandler->Array.dwArrayEntry[0], &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
													if(MemInfo.State != MEM_COMMIT){
														ValidPointer = false;
														memCheckMemory = LastValidPtr;
													}else{
														LastValidPtr = memCheckMemory;
														memCheckMemory = (LPVOID)((ULONG_PTR)memCheckMemory + 4);
														MaximumBlankSpaces = 4;
													}
												}else{
													memCheckMemory = (LPVOID)((ULONG_PTR)memCheckMemory + 4);
													MaximumBlankSpaces--;
												}
												j = j - 4;
											}
											IATThunkPossiblePointerFO = IATThunkPossiblePointerFO + 0x1000;
										}
										if(MaximumBlankSpaces == NULL){
											memCheckMemory = (LPVOID)((ULONG_PTR)memCheckMemory - (5 * sizeof ULONG_PTR));
										}
										IATHigh = (DWORD)ConvertFileOffsetToVA(FileMapVA, IATThunkPossiblePointerFO + (ULONG_PTR)memCheckMemory - (ULONG_PTR)CheckMemory - 0x1000, true);
										IATThunkPossiblePointerFO = (DWORD)ConvertVAtoFileOffset(FileMapVA, IATThunkPossiblePointer, true) - 0x1000;
										j = NULL;
										while(j == NULL){
											EngineGrabDataFromMappedFile(FileHandle, FileMapVA, IATThunkPossiblePointerFO, 0x1000, CheckMemory);
											memCheckMemory = (LPVOID)((ULONG_PTR)CheckMemory + 0x1000 - 8);
											LastValidPtr = (LPVOID)((ULONG_PTR)CheckMemory + 0x1000);
											MaximumBlankSpaces = 4;
											ValidPointer = true;
											j = 0x1000 - 2 * 4;
											while(j > NULL && ValidPointer == true && MaximumBlankSpaces > NULL){
												if(memcmp(memCheckMemory, &CompareBuffer, sizeof ULONG_PTR) != NULL){
													cmpHandler = (PMEMORY_COMPARE_HANDLER)memCheckMemory;
													VirtualQueryEx(hProcess, (void*)cmpHandler->Array.dwArrayEntry[0], &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
													if(MemInfo.State != MEM_COMMIT){
														ValidPointer = false;
														memCheckMemory = LastValidPtr;
													}else{
														LastValidPtr = memCheckMemory;
														memCheckMemory = (LPVOID)((ULONG_PTR)memCheckMemory - 4);
														MaximumBlankSpaces = 4;
													}
												}else{
													memCheckMemory = (LPVOID)((ULONG_PTR)memCheckMemory - 4);
													MaximumBlankSpaces--;
												}
												j = j - 4;
											}
											IATThunkPossiblePointerFO = IATThunkPossiblePointerFO - 0x1000;
										}
										if(MaximumBlankSpaces == NULL){
											memCheckMemory = (LPVOID)((ULONG_PTR)memCheckMemory + (5 * sizeof ULONG_PTR));
										}
										IATLow = (DWORD)ConvertFileOffsetToVA(FileMapVA, IATThunkPossiblePointerFO + (ULONG_PTR)memCheckMemory - (ULONG_PTR)CheckMemory + 0x1000, true);
										memSearchMemory = (LPVOID)((ULONG_PTR)memSearchMemory + 6);
										i = i - 6;
									}else{
										memSearchMemory = (LPVOID)((ULONG_PTR)memSearchMemory + 2);
										i = i - 2;
									}
								}else{
									if(IATHighestFound < IATThunkPossiblePointer || IATHighestFound == NULL){
										if(IATThunkPossiblePointer < IATHigh || IATHighestFound == NULL){
											IATHighestFound = IATThunkPossiblePointer;
										}
									}
									if(IATLowestFound > IATThunkPossiblePointer || IATLowestFound == NULL){
										if(IATThunkPossiblePointer > IATLow || IATLowestFound == NULL){
											IATLowestFound = IATThunkPossiblePointer;
										}
									}
								}
							}else{
								memSearchMemory = (LPVOID)((ULONG_PTR)memSearchMemory + 2);
								i = i - 2;
							}
						}else{
							SearchStartX64 = SearchStartFO - OriginalSearchStartFO + ((ULONG_PTR)memSearchMemory - (ULONG_PTR)SearchMemory) + SearchStart;
							IATThunkPossiblePointer = IATThunkPossiblePointer + SearchStartX64 + 6;
							if(IATThunkPossiblePointer >= ImageBase && IATThunkPossiblePointer <= ImageBase + NtSizeOfImage){
								if(IATHigh == NULL && IATLow == NULL){
									IATThunkPossiblePointerFO = (DWORD)ConvertVAtoFileOffset(FileMapVA, IATThunkPossiblePointer, true);
									if(IATThunkPossiblePointerFO - FileMapVA > 0x1000 && FileSize - (IATThunkPossiblePointerFO - FileMapVA) > 0x1000){
										j = NULL;
										while(j == NULL){
											EngineGrabDataFromMappedFile(FileHandle, FileMapVA, IATThunkPossiblePointerFO, 0x1000, CheckMemory);
											memCheckMemory = CheckMemory;
											LastValidPtr = memCheckMemory;
											MaximumBlankSpaces = 4;
											ValidPointer = true;
											j = 0x1000 - 2 * 8;
											while(j > NULL && ValidPointer == true && MaximumBlankSpaces > NULL){
												if(memcmp(memCheckMemory, &CompareBuffer, sizeof ULONG_PTR) != NULL){
													cmpHandler = (PMEMORY_COMPARE_HANDLER)memCheckMemory;
													VirtualQueryEx(hProcess, (void*)cmpHandler->Array.dwArrayEntry[0], &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
													if(MemInfo.State != MEM_COMMIT){
														ValidPointer = false;
														memCheckMemory = LastValidPtr;
													}else{
														LastValidPtr = memCheckMemory;
														memCheckMemory = (LPVOID)((ULONG_PTR)memCheckMemory + 8);
														MaximumBlankSpaces = 4;
													}
												}else{
													memCheckMemory = (LPVOID)((ULONG_PTR)memCheckMemory + 8);
													MaximumBlankSpaces--;
												}
												j = j - 8;
											}
											IATThunkPossiblePointerFO = IATThunkPossiblePointerFO + 0x1000;
										}
										if(MaximumBlankSpaces == NULL){
											memCheckMemory = (LPVOID)((ULONG_PTR)memCheckMemory - (5 * sizeof ULONG_PTR));
										}
										IATHigh = (DWORD)ConvertFileOffsetToVA(FileMapVA, IATThunkPossiblePointerFO + (ULONG_PTR)memCheckMemory - (ULONG_PTR)CheckMemory - 0x1000, true);
										IATThunkPossiblePointerFO = (DWORD)ConvertVAtoFileOffset(FileMapVA, IATThunkPossiblePointer, true) - 0x1000;
										j = NULL;
										while(j == NULL){
											EngineGrabDataFromMappedFile(FileHandle, FileMapVA, IATThunkPossiblePointerFO, 0x1000, CheckMemory);
											memCheckMemory = (LPVOID)((ULONG_PTR)CheckMemory + 0x1000 - 8);
											LastValidPtr = (LPVOID)((ULONG_PTR)CheckMemory + 0x1000);
											MaximumBlankSpaces = 4;
											ValidPointer = true;
											j = 0x1000 - 2 * 8;
											while(j > NULL && ValidPointer == true && MaximumBlankSpaces > NULL){
												if(memcmp(memCheckMemory, &CompareBuffer, sizeof ULONG_PTR) != NULL){
													cmpHandler = (PMEMORY_COMPARE_HANDLER)memCheckMemory;
													VirtualQueryEx(hProcess, (void*)cmpHandler->Array.dwArrayEntry[0], &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
													if(MemInfo.State != MEM_COMMIT){
														ValidPointer = false;
														memCheckMemory = LastValidPtr;
													}else{
														LastValidPtr = memCheckMemory;
														memCheckMemory = (LPVOID)((ULONG_PTR)memCheckMemory - 8);
														MaximumBlankSpaces = 4;
													}
												}else{
													memCheckMemory = (LPVOID)((ULONG_PTR)memCheckMemory - 8);
													MaximumBlankSpaces--;
												}
												j = j - 8;
											}
											IATThunkPossiblePointerFO = IATThunkPossiblePointerFO - 0x1000;
										}
										if(MaximumBlankSpaces == NULL){
											memCheckMemory = (LPVOID)((ULONG_PTR)memCheckMemory + (5 * sizeof ULONG_PTR));
										}
										IATLow = (DWORD)ConvertFileOffsetToVA(FileMapVA, IATThunkPossiblePointerFO + (ULONG_PTR)memCheckMemory - (ULONG_PTR)CheckMemory + 0x1000, true);
										memSearchMemory = (LPVOID)((ULONG_PTR)memSearchMemory + 6);
										i = i - 6;
									}else{
										memSearchMemory = (LPVOID)((ULONG_PTR)memSearchMemory + 2);
										i = i - 2;
									}
								}else{
									if(IATHighestFound < IATThunkPossiblePointer || IATHighestFound == NULL){
										if(IATThunkPossiblePointer < IATHigh || IATHighestFound == NULL){
											IATHighestFound = IATThunkPossiblePointer;
										}
									}
									if(IATLowestFound > IATThunkPossiblePointer || IATLowestFound == NULL){
										if(IATThunkPossiblePointer > IATLow || IATLowestFound == NULL){
											IATLowestFound = IATThunkPossiblePointer;
										}
									}
								}
							}else{
								memSearchMemory = (LPVOID)((ULONG_PTR)memSearchMemory + 2);
								i = i - 2;
							}
						}
					}
					memSearchMemory = (LPVOID)((ULONG_PTR)memSearchMemory + 1);
					i--;
				}
				if(SearchSize > 0x1000){
					SearchSize = SearchSize - 0x1000;
					CurrentSearchSize = 0x1000;
				}else{
					SearchSize = NULL;
					CurrentSearchSize = SearchSize;
				}
				SearchStartFO = SearchStartFO + CurrentSearchSize;
			}
			IATHigh = IATHigh - IATLow + sizeof ULONG_PTR;
			RtlMoveMemory(pIATStart, &IATLow, sizeof ULONG_PTR);
			RtlMoveMemory(pIATSize, &IATHigh, sizeof ULONG_PTR);
			VirtualFree(SearchMemory, NULL, MEM_RELEASE);
			VirtualFree(CheckMemory, NULL, MEM_RELEASE);
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			return;
		}else{
			VirtualFree(SearchMemory, NULL, MEM_RELEASE);
			VirtualFree(CheckMemory, NULL, MEM_RELEASE);
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			return;	
		}
	}
	VirtualFree(SearchMemory, NULL, MEM_RELEASE);
	VirtualFree(CheckMemory, NULL, MEM_RELEASE);
	return;
}
__declspec(dllexport) void __stdcall ImporterAutoSearchIATEx(HANDLE hProcess, ULONG_PTR ImageBase, ULONG_PTR SearchStart, DWORD SearchSize, LPVOID pIATStart, LPVOID pIATSize){

	char szTempName[MAX_PATH]; 
	char szTempFolder[MAX_PATH]; 

	RtlZeroMemory(&szTempName, MAX_PATH);
	RtlZeroMemory(&szTempFolder, MAX_PATH);
	if(GetTempPathA(MAX_PATH, szTempFolder) < MAX_PATH){
		if(GetTempFileNameA(szTempFolder, "DumpTemp", NULL, szTempName)){
			DumpProcess(hProcess, (LPVOID)ImageBase, szTempName, NULL);
			ImporterAutoSearchIAT(hProcess, szTempName, ImageBase, SearchStart, SearchSize, pIATStart, pIATSize);
			DeleteFileA(szTempName);
		}
	}
}
__declspec(dllexport) void __stdcall ImporterEnumAddedData(LPVOID EnumCallBack){

	int i = 0;
	DWORD DLLNumber = NULL;
	DWORD NumberOfAPIs = NULL;
	LPVOID NameReadPlace = NULL;
	ULONG_PTR CurrentAPILocation = NULL;
	DWORD APINameRelativeOffset = NULL;
	typedef void(__stdcall *fEnumCallBack)(LPVOID fImportDetail);
	fEnumCallBack myEnumCallBack = (fEnumCallBack)EnumCallBack;
	ImportEnumData myImportEnumData;

	if(EnumCallBack != NULL && ImporterGetAddedDllCount() > NULL){
		DLLNumber = impDLLNumber + 1;
		while(DLLNumber > NULL){
	#if !defined(_WIN64)
			NameReadPlace = (LPVOID)(impDLLDataList[i][0] + 12);
	#else
			NameReadPlace = (LPVOID)(impDLLDataList[i][0] + 20);
	#endif
			RtlMoveMemory(&CurrentAPILocation, (LPVOID)(impDLLDataList[i][0]), sizeof ULONG_PTR);
			RtlMoveMemory(&NumberOfAPIs, (LPVOID)(impDLLDataList[i][0] + 2 * sizeof ULONG_PTR), 4);
			RtlZeroMemory(&myImportEnumData, sizeof ImportEnumData);
			myImportEnumData.NumberOfImports = (int)(NumberOfAPIs - 1);
			myImportEnumData.BaseImportThunk = CurrentAPILocation;
			myImportEnumData.ImageBase = impImageBase;
			myImportEnumData.NewDll = true;
			while(NumberOfAPIs > 1){
				RtlMoveMemory(&APINameRelativeOffset, NameReadPlace, 4);
				myImportEnumData.ImportThunk = CurrentAPILocation;
				myImportEnumData.APIName = (char*)((ULONG_PTR)impDLLStringList[i][0] + APINameRelativeOffset + 2);
				myImportEnumData.DLLName = (char*)((ULONG_PTR)impDLLStringList[i][0]);
				__try{
					myEnumCallBack(&myImportEnumData);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					NumberOfAPIs = 2;
				}
				myImportEnumData.NewDll = false;
				CurrentAPILocation = CurrentAPILocation + sizeof ULONG_PTR;
				NameReadPlace = (LPVOID)((ULONG_PTR)NameReadPlace + sizeof ULONG_PTR);
				NumberOfAPIs--;
			}
			DLLNumber--;
			i++;
		}
	}
}
__declspec(dllexport) long __stdcall ImporterAutoFixIATEx(HANDLE hProcess, char* szDumpedFile, char* szSectionName, bool DumpRunningProcess, bool RealignFile, ULONG_PTR EntryPointAddress, ULONG_PTR ImageBase, ULONG_PTR SearchStart, DWORD SearchSize, DWORD SearchStep, bool TryAutoFix, bool FixEliminations, LPVOID UnknownPointerFixCallback){

	int i;
	int j;
	int delta;
	int currentSectionSize;
#if !defined(_WIN64)
	PE32Struct PEStructure;
#else
	PE64Struct PEStructure;
#endif
	typedef void*(__stdcall *fFixerCallback)(LPVOID fIATPointer);
	fFixerCallback myFixerCallback = (fFixerCallback)UnknownPointerFixCallback;
	MEMORY_BASIC_INFORMATION MemInfo;
	DWORD SectionFlags;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	LPVOID aSearchMemory;
	LPVOID cSearchMemory;
	LPVOID aEnumeratedModules;
	ULONG_PTR ueNumberOfBytesRead;
	DWORD dwPossibleIATPointer;
	ULONG_PTR qwPossibleIATPointer;
	ULONG_PTR PossibleIATPointer;
	ULONG_PTR TracedIATPointer;
	PMEMORY_COMPARE_HANDLER currentSearchPos;
	DWORD SetionVirtualOffset;
	ULONG_PTR TestReadData;
	DWORD LastDllId = 1024;
	bool FileIs64 = false;
	bool PossibleThunk = false;
	bool UpdateJump = false;
	DWORD CurrentDllId;
	char* szDLLName;
	char* szAPIName;
	DWORD TraceIndex;
	DWORD Dummy;

	if(hProcess == NULL){
		return(0x401);		// Error, process terminated
	}
	if(SearchStep == NULL){
		SearchStep++;
	}
	if(DumpRunningProcess){
		if(!DumpProcess(hProcess, (LPVOID)ImageBase, szDumpedFile, EntryPointAddress)){
			return(NULL);	// Critical error! *just to be safe, but it should never happen!
		}
	}

	aEnumeratedModules = VirtualAlloc(NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE);
	if(EnumProcessModules(hProcess, (HMODULE*)aEnumeratedModules, 0x2000, &Dummy)){
		aSearchMemory = VirtualAlloc(NULL, SearchSize, MEM_COMMIT, PAGE_READWRITE);
		cSearchMemory = aSearchMemory;
		__try{
			if(SearchStart == NULL || ReadProcessMemory(hProcess, (LPVOID)SearchStart, aSearchMemory, SearchSize, &ueNumberOfBytesRead)){
				ImporterInit(MAX_IMPORT_ALLOC, ImageBase);			
				if(SearchStart != NULL){
					SearchSize = SearchSize / SearchStep;
					while(SearchSize > NULL){
						RtlMoveMemory(&PossibleIATPointer, cSearchMemory, sizeof ULONG_PTR);
						if(ReadProcessMemory(hProcess, (LPVOID)PossibleIATPointer, &TestReadData, sizeof ULONG_PTR, &ueNumberOfBytesRead)){
							//CurrentDllId = ImporterGetDLLIndexEx(PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
							CurrentDllId = ImporterGetDLLIndex(hProcess, PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
							if(CurrentDllId == NULL && TryAutoFix == true){
								TraceIndex = TracerDetectRedirection(hProcess, PossibleIATPointer);
								if(TraceIndex > NULL){
									PossibleIATPointer = (ULONG_PTR)TracerFixKnownRedirection(hProcess, PossibleIATPointer, TraceIndex);
									if(PossibleIATPointer != NULL){
										//CurrentDllId = ImporterGetDLLIndexEx(PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
										CurrentDllId = ImporterGetDLLIndex(hProcess, PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
									}
								}else{
									TracedIATPointer = (ULONG_PTR)TracerLevel1(hProcess, PossibleIATPointer);
									if(TracedIATPointer > 0x1000){
										PossibleIATPointer = TracedIATPointer;
										//CurrentDllId = ImporterGetDLLIndexEx(PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
										CurrentDllId = ImporterGetDLLIndex(hProcess, PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
									}else{
										if(TracedIATPointer != NULL){
											TracedIATPointer = (ULONG_PTR)HashTracerLevel1(hProcess, PossibleIATPointer, (DWORD)TracedIATPointer);
											if(TracedIATPointer != NULL){
												PossibleIATPointer = TracedIATPointer;
												//CurrentDllId = ImporterGetDLLIndexEx(PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
												CurrentDllId = ImporterGetDLLIndex(hProcess, PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
											}
										}
									}
								}
							}
							if(CurrentDllId == NULL && UnknownPointerFixCallback != NULL){
								__try{
									PossibleIATPointer = (ULONG_PTR)myFixerCallback((LPVOID)PossibleIATPointer);
								}__except(EXCEPTION_EXECUTE_HANDLER){
									UnknownPointerFixCallback = NULL;
									PossibleIATPointer = NULL;
								}
								if(PossibleIATPointer != NULL){
									//CurrentDllId = ImporterGetDLLIndexEx(PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
									CurrentDllId = ImporterGetDLLIndex(hProcess, PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
								}
							}
							if(CurrentDllId != NULL){
								if(LastDllId != CurrentDllId){
									LastDllId = CurrentDllId;
									szDLLName = (char*)ImporterGetDLLNameFromDebugee(hProcess, PossibleIATPointer);
									szAPIName = (char*)ImporterGetAPINameFromDebugee(hProcess, PossibleIATPointer);
									if(szDLLName != NULL && szAPIName != NULL){
										ImporterAddNewDll(szDLLName, (ULONG_PTR)((ULONG_PTR)cSearchMemory - (ULONG_PTR)aSearchMemory + SearchStart));
										ImporterAddNewAPI(szAPIName, (ULONG_PTR)((ULONG_PTR)cSearchMemory - (ULONG_PTR)aSearchMemory + SearchStart));
									}
								}else{
									szAPIName = (char*)ImporterGetAPINameFromDebugee(hProcess, PossibleIATPointer);
									if(szAPIName != NULL){
										ImporterAddNewAPI(szAPIName, (ULONG_PTR)((ULONG_PTR)cSearchMemory - (ULONG_PTR)aSearchMemory + SearchStart));
									}							
								}
							}
						}
						cSearchMemory = (LPVOID)((ULONG_PTR)cSearchMemory + SearchStep);
						SearchSize = SearchSize - SearchStep;
					}
				}
				if(FixEliminations){
					LastDllId = 1024;
					if(ImporterGetAddedDllCount() == NULL){
						ImporterCleanup();
						ImporterInit(MAX_IMPORT_ALLOC, ImageBase);
					}
					if(GetPE32DataEx(szDumpedFile, &PEStructure)){
						if(MapFileEx(szDumpedFile, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
							ImporterMoveIAT();
							ImporterSetUnknownDelta((ULONG_PTR)EngineEstimateNewSectionRVA(FileMapVA));
							for(i = 0; i < PEStructure.SectionNumber; i++){
								if(GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONRAWSIZE) > 4){
									SectionFlags = (DWORD)GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONFLAGS);
									SetionVirtualOffset = (DWORD)GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONVIRTUALOFFSET);
									if(SectionFlags & IMAGE_SCN_MEM_EXECUTE || SectionFlags & IMAGE_SCN_CNT_CODE || SectionFlags & IMAGE_SCN_MEM_WRITE || SectionFlags & IMAGE_SCN_CNT_INITIALIZED_DATA){
										currentSearchPos = (PMEMORY_COMPARE_HANDLER)(FileMapVA + GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONRAWOFFSET));
										currentSectionSize = (int)GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONRAWSIZE) - 6;
										for(j = 0; j < currentSectionSize; j++){
											if(!FileIs64){
												// x86
												delta = 0;
												PossibleThunk = false;
												UpdateJump = false;
												if(currentSearchPos->Array.bArrayEntry[0] == 0xFF && (currentSearchPos->Array.bArrayEntry[1] == 0x15 || currentSearchPos->Array.bArrayEntry[1] == 0x25)){
													currentSearchPos = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)currentSearchPos + 2);
													PossibleIATPointer = currentSearchPos->Array.dwArrayEntry[0];
													currentSearchPos = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)currentSearchPos - 2);
													if(PossibleIATPointer > PEStructure.ImageBase && PossibleIATPointer < PEStructure.ImageBase + PEStructure.NtSizeOfImage){
														PossibleThunk = true;
														delta = 2;
													}else{
														VirtualQueryEx(hProcess, (LPVOID)PossibleIATPointer, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
														if(MemInfo.State == MEM_COMMIT && (MemInfo.Protect & PAGE_READWRITE || MemInfo.Protect & PAGE_EXECUTE_READWRITE || MemInfo.Protect & PAGE_EXECUTE)){
															PossibleIATPointer = currentSearchPos->Array.dwArrayEntry[0];
															PossibleThunk = true;
															delta = 2;
														}
													}
												}else if(currentSearchPos->Array.bArrayEntry[0] == 0xE8 || currentSearchPos->Array.bArrayEntry[0] == 0xE9){
													currentSearchPos = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)currentSearchPos + 1);
													PossibleIATPointer = currentSearchPos->Array.dwArrayEntry[0] + j + SetionVirtualOffset + ImageBase;
													currentSearchPos = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)currentSearchPos - 1);
													if(PossibleIATPointer > PEStructure.ImageBase && PossibleIATPointer < PEStructure.ImageBase + PEStructure.NtSizeOfImage){
														PossibleThunk = true;
														UpdateJump = true;
														delta = 1;
													}else{
														VirtualQueryEx(hProcess, (LPVOID)PossibleIATPointer, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
														if(MemInfo.State == MEM_COMMIT && (MemInfo.Protect & PAGE_READWRITE || MemInfo.Protect & PAGE_EXECUTE_READWRITE || MemInfo.Protect & PAGE_EXECUTE)){
															PossibleIATPointer = currentSearchPos->Array.dwArrayEntry[0];
															PossibleThunk = true;
															UpdateJump = true;
															delta = 1;
														}
													}
												}else if(currentSearchPos->Array.dwArrayEntry[0] > PEStructure.ImageBase && currentSearchPos->Array.dwArrayEntry[0] < PEStructure.ImageBase + PEStructure.NtSizeOfImage){
													PossibleIATPointer = currentSearchPos->Array.dwArrayEntry[0];
													PossibleThunk = true;
													delta = 0;
												}else{
													VirtualQueryEx(hProcess, (LPVOID)currentSearchPos->Array.dwArrayEntry[0], &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
													if(MemInfo.State == MEM_COMMIT && (MemInfo.Protect & PAGE_READWRITE || MemInfo.Protect & PAGE_EXECUTE_READWRITE || MemInfo.Protect & PAGE_EXECUTE)){
														PossibleIATPointer = currentSearchPos->Array.dwArrayEntry[0];
														PossibleThunk = true;
														delta = 0;
													}
												}
												if(PossibleThunk){
													if(ReadProcessMemory(hProcess, (LPVOID)PossibleIATPointer, &dwPossibleIATPointer, 4, &ueNumberOfBytesRead)){
														VirtualQueryEx(hProcess, (LPVOID)dwPossibleIATPointer, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
														if(MemInfo.State == MEM_COMMIT && (MemInfo.Protect >= PAGE_READONLY || MemInfo.Protect <= PAGE_EXECUTE_READWRITE)){
															PossibleIATPointer = (ULONG_PTR)dwPossibleIATPointer;
															//CurrentDllId = ImporterGetDLLIndexEx(PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
															CurrentDllId = ImporterGetDLLIndex(hProcess, PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
															if(CurrentDllId == NULL && TryAutoFix == true){
																TraceIndex = TracerDetectRedirection(hProcess, PossibleIATPointer);
																if(TraceIndex > NULL){
																	PossibleIATPointer = (ULONG_PTR)TracerFixKnownRedirection(hProcess, PossibleIATPointer, TraceIndex);
																	if(PossibleIATPointer != NULL){
																		//CurrentDllId = ImporterGetDLLIndexEx(PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																		CurrentDllId = ImporterGetDLLIndex(hProcess, PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																	}
																}else{
																	TracedIATPointer = (ULONG_PTR)TracerLevel1(hProcess, PossibleIATPointer);
																	if(TracedIATPointer > 0x1000){
																		PossibleIATPointer = TracedIATPointer;
																		//CurrentDllId = ImporterGetDLLIndexEx(PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																		CurrentDllId = ImporterGetDLLIndex(hProcess, PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																	}else{
																		if(TracedIATPointer != NULL){
																			TracedIATPointer = (ULONG_PTR)HashTracerLevel1(hProcess, PossibleIATPointer, (DWORD)TracedIATPointer);
																			if(TracedIATPointer != NULL){
																				PossibleIATPointer = TracedIATPointer;
																				//CurrentDllId = ImporterGetDLLIndexEx(PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																				CurrentDllId = ImporterGetDLLIndex(hProcess, PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																			}
																		}
																	}
																}
															}
															if(CurrentDllId == NULL && UnknownPointerFixCallback != NULL){
																__try{
																	PossibleIATPointer = (ULONG_PTR)myFixerCallback((LPVOID)PossibleIATPointer);
																}__except(EXCEPTION_EXECUTE_HANDLER){
																	UnknownPointerFixCallback = NULL;
																	PossibleIATPointer = NULL;
																}
																if(PossibleIATPointer != NULL){
																	//CurrentDllId = ImporterGetDLLIndexEx(PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																	CurrentDllId = ImporterGetDLLIndex(hProcess, PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																}
															}
															if(CurrentDllId != NULL){
																szDLLName = (char*)ImporterGetDLLNameFromDebugee(hProcess, PossibleIATPointer);
																szAPIName = (char*)ImporterGetAPINameFromDebugee(hProcess, PossibleIATPointer);
																if(szDLLName != NULL && szAPIName != NULL){
																	if(ImporterGetAddedDllCount() > NULL){
																		PossibleIATPointer = (ULONG_PTR)ImporterFindAPIWriteLocation(szAPIName);
																	}else{
																		PossibleIATPointer = NULL;
																	}
																	if(PossibleIATPointer != NULL){
																		dwPossibleIATPointer = (DWORD)(PossibleIATPointer);
																		if(!UpdateJump){
																			RtlMoveMemory(&currentSearchPos->Array.dwArrayEntry[0 + delta], &dwPossibleIATPointer, 4);
																		}else{
																			dwPossibleIATPointer = dwPossibleIATPointer - (j + SetionVirtualOffset) - (DWORD)ImageBase - 5;
																			RtlMoveMemory(&currentSearchPos->Array.dwArrayEntry[0 + delta], &dwPossibleIATPointer, 4);
																		}
																		currentSearchPos = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)currentSearchPos + delta + 4 - 1);
																	}else{
																		if(CurrentDllId != LastDllId){
																			LastDllId = CurrentDllId;
																			ImporterAddNewDll(szDLLName, NULL);
																			dwPossibleIATPointer = (DWORD)(ImporterGetCurrentDelta());
																			if(!UpdateJump){
																				RtlMoveMemory(&currentSearchPos->Array.dwArrayEntry[0 + delta], &dwPossibleIATPointer, 4);
																			}else{
																				dwPossibleIATPointer = dwPossibleIATPointer - (j + SetionVirtualOffset) - (DWORD)ImageBase - 5;
																				RtlMoveMemory(&currentSearchPos->Array.dwArrayEntry[0 + delta], &dwPossibleIATPointer, 4);
																			}
																			ImporterAddNewAPI(szAPIName, NULL);
																			currentSearchPos = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)currentSearchPos + delta + 4 - 1);
																		}else{
																			dwPossibleIATPointer = (DWORD)(ImporterGetCurrentDelta());
																			if(!UpdateJump){
																				RtlMoveMemory(&currentSearchPos->Array.dwArrayEntry[0 + delta], &dwPossibleIATPointer, 4);
																			}else{
																				dwPossibleIATPointer = dwPossibleIATPointer - (j + SetionVirtualOffset) - (DWORD)ImageBase - 5;
																				RtlMoveMemory(&currentSearchPos->Array.dwArrayEntry[0 + delta], &dwPossibleIATPointer, 4);
																			}
																			ImporterAddNewAPI(szAPIName, NULL);
																			currentSearchPos = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)currentSearchPos + delta + 4 - 1);
																		}
																	}
																}
															}
														}
													}
												}
											}else{
												// x64
												delta = 0;
												PossibleThunk = false;
												UpdateJump = false;
												if(currentSearchPos->Array.bArrayEntry[0] == 0xFF && (currentSearchPos->Array.bArrayEntry[1] == 0x15 || currentSearchPos->Array.bArrayEntry[1] == 0x25)){
													currentSearchPos = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)currentSearchPos + 2);
													PossibleIATPointer = currentSearchPos->Array.dwArrayEntry[0] + j + SetionVirtualOffset + ImageBase;
													currentSearchPos = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)currentSearchPos - 2);
													if(PossibleIATPointer > PEStructure.ImageBase && PossibleIATPointer < PEStructure.ImageBase + PEStructure.NtSizeOfImage){
														PossibleThunk = true;
														delta = 2;
													}else{
														VirtualQueryEx(hProcess, (LPVOID)PossibleIATPointer, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
														if(MemInfo.State == MEM_COMMIT && (MemInfo.Protect & PAGE_READWRITE || MemInfo.Protect & PAGE_EXECUTE_READWRITE || MemInfo.Protect & PAGE_EXECUTE)){
															PossibleIATPointer = currentSearchPos->Array.dwArrayEntry[0];
															PossibleThunk = true;
															delta = 2;
														}
													}
												}else if(currentSearchPos->Array.bArrayEntry[0] == 0xE8 || currentSearchPos->Array.bArrayEntry[0] == 0xE9){
													currentSearchPos = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)currentSearchPos + 1);
													PossibleIATPointer = currentSearchPos->Array.dwArrayEntry[0] + j + SetionVirtualOffset + ImageBase;
													currentSearchPos = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)currentSearchPos - 1);
													if(PossibleIATPointer > PEStructure.ImageBase && PossibleIATPointer < PEStructure.ImageBase + PEStructure.NtSizeOfImage){
														PossibleThunk = true;
														UpdateJump = true;
														delta = 1;
													}else{
														VirtualQueryEx(hProcess, (LPVOID)PossibleIATPointer, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
														if(MemInfo.State == MEM_COMMIT && (MemInfo.Protect & PAGE_READWRITE || MemInfo.Protect & PAGE_EXECUTE_READWRITE || MemInfo.Protect & PAGE_EXECUTE)){
															PossibleIATPointer = currentSearchPos->Array.dwArrayEntry[0];
															PossibleThunk = true;
															UpdateJump = true;
															delta = 1;
														}
													}
												}else if(currentSearchPos->Array.dwArrayEntry[0] + j + SetionVirtualOffset > PEStructure.ImageBase && currentSearchPos->Array.dwArrayEntry[0] + j + SetionVirtualOffset < PEStructure.ImageBase + PEStructure.NtSizeOfImage){
													PossibleIATPointer = currentSearchPos->Array.dwArrayEntry[0] + j + SetionVirtualOffset + ImageBase;
													PossibleThunk = true;
													delta = 0;
												}else{
													VirtualQueryEx(hProcess, (LPVOID)(currentSearchPos->Array.dwArrayEntry[0] + j + SetionVirtualOffset), &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
													if(MemInfo.State == MEM_COMMIT && (MemInfo.Protect & PAGE_READWRITE || MemInfo.Protect & PAGE_EXECUTE_READWRITE || MemInfo.Protect & PAGE_EXECUTE)){
														PossibleIATPointer = currentSearchPos->Array.dwArrayEntry[0] + j + SetionVirtualOffset + ImageBase;
														PossibleThunk = true;
														delta = 0;
													}
												}
												if(PossibleThunk){
													if(ReadProcessMemory(hProcess, (LPVOID)PossibleIATPointer, &qwPossibleIATPointer, 8, &ueNumberOfBytesRead)){
														VirtualQueryEx(hProcess, (LPVOID)qwPossibleIATPointer, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
														if(MemInfo.State == MEM_COMMIT && (MemInfo.Protect >= PAGE_READONLY || MemInfo.Protect <= PAGE_EXECUTE_READWRITE)){
															PossibleIATPointer = qwPossibleIATPointer;
															//CurrentDllId = ImporterGetDLLIndexEx(PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
															CurrentDllId = ImporterGetDLLIndex(hProcess, PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
															if(CurrentDllId == NULL && TryAutoFix == true){
																TraceIndex = TracerDetectRedirection(hProcess, PossibleIATPointer);
																if(TraceIndex > NULL){
																	PossibleIATPointer = (ULONG_PTR)TracerFixKnownRedirection(hProcess, PossibleIATPointer, TraceIndex);
																	if(PossibleIATPointer != NULL){
																		//CurrentDllId = ImporterGetDLLIndexEx(PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																		CurrentDllId = ImporterGetDLLIndex(hProcess, PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																	}
																}else{
																	TracedIATPointer = (ULONG_PTR)TracerLevel1(hProcess, PossibleIATPointer);
																	if(TracedIATPointer > 0x1000){
																		PossibleIATPointer = TracedIATPointer;
																		//CurrentDllId = ImporterGetDLLIndexEx(PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																		CurrentDllId = ImporterGetDLLIndex(hProcess, PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																	}else{
																		if(TracedIATPointer != NULL){
																			TracedIATPointer = (ULONG_PTR)HashTracerLevel1(hProcess, PossibleIATPointer, (DWORD)TracedIATPointer);
																			if(TracedIATPointer != NULL){
																				PossibleIATPointer = TracedIATPointer;
																				//CurrentDllId = ImporterGetDLLIndexEx(PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																				CurrentDllId = ImporterGetDLLIndex(hProcess, PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																			}
																		}
																	}
																}
															}
															if(CurrentDllId == NULL && UnknownPointerFixCallback != NULL){
																__try{
																	PossibleIATPointer = (ULONG_PTR)myFixerCallback((LPVOID)PossibleIATPointer);
																}__except(EXCEPTION_EXECUTE_HANDLER){
																	UnknownPointerFixCallback = NULL;
																	PossibleIATPointer = NULL;
																}
																if(PossibleIATPointer != NULL){
																	//CurrentDllId = ImporterGetDLLIndexEx(PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																	CurrentDllId = ImporterGetDLLIndex(hProcess, PossibleIATPointer, (ULONG_PTR)aEnumeratedModules);
																}
															}
															if(CurrentDllId != NULL){
																szDLLName = (char*)ImporterGetDLLNameFromDebugee(hProcess, PossibleIATPointer);
																szAPIName = (char*)ImporterGetAPINameFromDebugee(hProcess, PossibleIATPointer);
																if(szDLLName != NULL && szAPIName != NULL){
																	if(ImporterGetAddedDllCount() > NULL){
																		PossibleIATPointer = (ULONG_PTR)ImporterFindAPIWriteLocation(szAPIName);
																	}else{
																		PossibleIATPointer = NULL;
																	}
																	if(PossibleIATPointer != NULL){
																		if(!UpdateJump){
																			dwPossibleIATPointer = (DWORD)(PossibleIATPointer - j - SetionVirtualOffset - ImageBase);
																			RtlMoveMemory(&currentSearchPos->Array.dwArrayEntry[0 + delta], &dwPossibleIATPointer, 4);
																		}else{
																			dwPossibleIATPointer = (DWORD)(PossibleIATPointer);
																			dwPossibleIATPointer = (DWORD)(dwPossibleIATPointer - (j + SetionVirtualOffset) - (ULONG_PTR)ImageBase - 5);
																			RtlMoveMemory(&currentSearchPos->Array.dwArrayEntry[0 + delta], &dwPossibleIATPointer, 4);
																		}
																		currentSearchPos = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)currentSearchPos + delta + 4 - 1);
																	}else{
																		if(CurrentDllId != LastDllId){
																			LastDllId = CurrentDllId;
																			ImporterAddNewDll(szDLLName, NULL);
																			qwPossibleIATPointer = (ULONG_PTR)(ImporterGetCurrentDelta());
																			if(!UpdateJump){
																				qwPossibleIATPointer = (DWORD)(qwPossibleIATPointer - j - SetionVirtualOffset - ImageBase);
																				RtlMoveMemory(&currentSearchPos->Array.dwArrayEntry[0 + delta], &qwPossibleIATPointer, 4);
																			}else{
																				qwPossibleIATPointer = (DWORD)(qwPossibleIATPointer - j - SetionVirtualOffset - ImageBase - 5);
																				RtlMoveMemory(&currentSearchPos->Array.dwArrayEntry[0 + delta], &qwPossibleIATPointer, 4);
																			}
																			ImporterAddNewAPI(szAPIName, NULL);
																			currentSearchPos = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)currentSearchPos + delta + 4 - 1);
																		}else{
																			qwPossibleIATPointer = (ULONG_PTR)(ImporterGetCurrentDelta());
																			if(!UpdateJump){
																				qwPossibleIATPointer = (DWORD)(qwPossibleIATPointer - j - SetionVirtualOffset - ImageBase);
																				RtlMoveMemory(&currentSearchPos->Array.dwArrayEntry[0 + delta], &qwPossibleIATPointer, 4);
																			}else{
																				qwPossibleIATPointer = (DWORD)(qwPossibleIATPointer - j - SetionVirtualOffset - ImageBase - 5);
																				RtlMoveMemory(&currentSearchPos->Array.dwArrayEntry[0 + delta], &qwPossibleIATPointer, 4);
																			}
																			ImporterAddNewAPI(szAPIName, NULL);
																			currentSearchPos = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)currentSearchPos + delta + 4 - 1);
																		}
																	}
																}
															}
														}
													}
												}
											}
											currentSearchPos = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)currentSearchPos + 1);
										}
									}
								}
							}
 							UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						}else{
							return(0x405);	// Error, no API found!
						}
					}
				}
				VirtualFree(aEnumeratedModules, NULL, MEM_RELEASE);
				VirtualFree(aSearchMemory, NULL, MEM_RELEASE);
				if(ImporterGetAddedDllCount() > NULL && ImporterGetAddedAPICount() > NULL){
					if(!ImporterExportIATEx(szDumpedFile, szSectionName)){
						return(NULL); // Critical error! *just to be safe, but it should never happen!
					}
				}else{
					return(0x405);	// Error, no API found!
				}
				if(RealignFile){
					if(MapFileEx(szDumpedFile, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
						FileSize = RealignPE(FileMapVA, FileSize, NULL);
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					}else{
						return(0x406);	// Success, but realign failed!
					}
				}
				return(0x400);	// Success!
			}else{
				VirtualFree(aEnumeratedModules, NULL, MEM_RELEASE);
				VirtualFree(aSearchMemory, NULL, MEM_RELEASE);
				return(0x404);	// Error, memory could not be read!
			}
		}__except(EXCEPTION_EXECUTE_HANDLER){
			ImporterCleanup();
			VirtualFree(aEnumeratedModules, NULL, MEM_RELEASE);
			VirtualFree(aSearchMemory, NULL, MEM_RELEASE);
			return(NULL);	// Critical error! *just to be safe, but it should never happen!
		}
	}
	VirtualFree(aEnumeratedModules, NULL, MEM_RELEASE);
	return(NULL);	// Critical error! *just te bo safe, but it should never happen!
}
__declspec(dllexport) long __stdcall ImporterAutoFixIAT(HANDLE hProcess, char* szDumpedFile, ULONG_PTR ImageBase, ULONG_PTR SearchStart, DWORD SearchSize, DWORD SearchStep){
	return(ImporterAutoFixIATEx(hProcess, szDumpedFile, ".RL!TEv2", false, false, NULL, ImageBase, SearchStart, SearchSize, SearchStep, false, false, NULL));
}
// Global.Engine.Tracer.functions:
long long EngineGlobalTracerHandler1(HANDLE hProcess, ULONG_PTR AddressToTrace, bool HashInstructions, DWORD InputNumberOfInstructions){

	SIZE_T memSize = 0;
	int NumberOfInstructions = 0;
	int LengthOfValidInstruction = 0;
	int CurrentNumberOfInstructions = 0;
	MEMORY_BASIC_INFORMATION MemInfo;
	LPVOID TraceMemory, cTraceMemory;
	ULONG_PTR ueNumberOfBytesRead = NULL;
	DWORD LastPushValue = NULL;
	ULONG_PTR TraceStartAddress;
	ULONG_PTR TraceTestAddress;
	ULONG_PTR TraceTestReadAddress;
	DWORD CurrentInstructionSize;
	PMEMORY_CMP_HANDLER CompareMemory;
	PMEMORY_COMPARE_HANDLER longCompareMemory;
	DWORD InstructionHash = NULL;
	bool FoundValidAPI = false;
	bool SkipThisInstruction = false;
	bool LoopCondition = true;
	bool SkipHashing = false;
	BYTE EmptyCall[5] = {0xE8, 0x00, 0x00, 0x00, 0x00};

	if(VirtualQueryEx(hProcess, (LPVOID)AddressToTrace, &MemInfo, sizeof MEMORY_BASIC_INFORMATION) != NULL){
		if(MemInfo.RegionSize > NULL){
			memSize = MemInfo.RegionSize;
			if(memSize > 0x4000){
				memSize = 0x4000;
			}
			TraceMemory = VirtualAlloc(NULL, memSize, MEM_COMMIT, PAGE_READWRITE);
			cTraceMemory = TraceMemory;
			if(ReadProcessMemory(hProcess, (LPVOID)MemInfo.BaseAddress, TraceMemory, memSize, &ueNumberOfBytesRead)){
				TraceStartAddress = AddressToTrace - (ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)TraceMemory;
				if(HashInstructions){
					if(InputNumberOfInstructions > NULL){
						LoopCondition = true;
					}else{
						LoopCondition = false;
					}
				}else{
					if(CurrentNumberOfInstructions < 1000 && FoundValidAPI == false){
						LoopCondition = true;
					}else{
						LoopCondition = false;
					}
				}
				while(LoopCondition){
					SkipHashing = false;
					SkipThisInstruction = false;
					CompareMemory = (PMEMORY_CMP_HANDLER)TraceStartAddress;
					CurrentInstructionSize = StaticLengthDisassemble((LPVOID)TraceStartAddress);
					CurrentNumberOfInstructions++;
					/*
						Long JUMP (0xE9)
					*/
					if(HashInstructions == false && CompareMemory->DataByte[0] == 0xE9 && CurrentInstructionSize == 5){
						TraceTestAddress = (ULONG_PTR)GetJumpDestination(NULL, TraceStartAddress) - (ULONG_PTR)TraceMemory + (ULONG_PTR)MemInfo.BaseAddress;
						if(TraceTestAddress <= (ULONG_PTR)MemInfo.BaseAddress || TraceTestAddress >= (ULONG_PTR)MemInfo.BaseAddress + MemInfo.RegionSize){
							if(LengthOfValidInstruction == NULL){
								if(ImporterGetAPINameFromDebugee(hProcess, TraceTestAddress) != NULL){
									FoundValidAPI = true;
									break;
								}
							}
							if(ImporterGetAPINameFromDebugee(hProcess, TraceTestAddress) != NULL){
								FoundValidAPI = true;
								break;
							}else{
								if(ImporterGetAPINameFromDebugee(hProcess, TraceTestAddress - LengthOfValidInstruction) != NULL){
									FoundValidAPI = true;
									TraceTestAddress = TraceTestAddress - LengthOfValidInstruction;
									break;
								}
							}
						}
					/*
						Near JUMP (0xFF25)
					*/
					}else if(HashInstructions == false && CompareMemory->DataByte[0] == 0xFF && CompareMemory->DataByte[1] == 0x25 && CurrentInstructionSize == 6){
						TraceTestAddress = (ULONG_PTR)GetJumpDestination(NULL, TraceStartAddress);
						if(ReadProcessMemory(hProcess, (LPVOID)TraceTestAddress, &TraceTestAddress, 4, &ueNumberOfBytesRead)){
							if(TraceTestAddress <= (ULONG_PTR)MemInfo.BaseAddress || TraceTestAddress >= (ULONG_PTR)MemInfo.BaseAddress + MemInfo.RegionSize){
								if(LengthOfValidInstruction == NULL){
									if(ImporterGetAPINameFromDebugee(hProcess, TraceTestAddress) != NULL){
										FoundValidAPI = true;
										break;
									}
								}
								if(ImporterGetAPINameFromDebugee(hProcess, TraceTestAddress) != NULL){
									FoundValidAPI = true;
									break;
								}else{
									if(ImporterGetAPINameFromDebugee(hProcess, TraceTestAddress - LengthOfValidInstruction) != NULL){
										FoundValidAPI = true;
										TraceTestAddress = TraceTestAddress - LengthOfValidInstruction;
										break;
									}
								}
							}
						}
					/*
						PUSH then RET (0x68 ???????? 0xC3)
					*/
					}else if(HashInstructions == false && CompareMemory->DataByte[0] == 0x68 && CompareMemory->DataByte[5] == 0xC3 && CurrentInstructionSize == 5){
						longCompareMemory = (PMEMORY_COMPARE_HANDLER)((ULONG_PTR)CompareMemory + 1);
						TraceTestAddress = (DWORD)(longCompareMemory->Array.dwArrayEntry[0]);
						if(ReadProcessMemory(hProcess, (LPVOID)TraceTestAddress, &TraceTestReadAddress, 4, &ueNumberOfBytesRead)){
							if(TraceTestAddress <= (ULONG_PTR)MemInfo.BaseAddress || TraceTestAddress >= (ULONG_PTR)MemInfo.BaseAddress + MemInfo.RegionSize){
								if(LengthOfValidInstruction == NULL){
									if(ImporterGetAPINameFromDebugee(hProcess, TraceTestAddress) != NULL){
										FoundValidAPI = true;
										break;
									}
								}
								if(ImporterGetAPINameFromDebugee(hProcess, TraceTestAddress) != NULL){
									FoundValidAPI = true;
									break;
								}else{
									if(ImporterGetAPINameFromDebugee(hProcess, TraceTestAddress - LengthOfValidInstruction) != NULL){
										FoundValidAPI = true;
										TraceTestAddress = TraceTestAddress - LengthOfValidInstruction;
										break;
									}
								}
							}else{
								TraceStartAddress = TraceStartAddress - (ULONG_PTR)MemInfo.BaseAddress + (ULONG_PTR)TraceMemory;
							}
						}
					/*
						CALL (0xE8)
					*/
					}else if(HashInstructions == true && CompareMemory->DataByte[0] == 0xE8 && CurrentInstructionSize == 5){
						SkipHashing = true;
						InstructionHash = EngineHashMemory((char*)&EmptyCall, CurrentInstructionSize, InstructionHash);
					/*
						PUSH (0x68)
					*/
					}else if(CompareMemory->DataByte[0] == 0x68 && CurrentInstructionSize == 5){
						LastPushValue = (DWORD)(CompareMemory->DataByte[1] + CompareMemory->DataByte[2] * 0x1000 + CompareMemory->DataByte[3] * 0x100000 + CompareMemory->DataByte[4] * 0x10000000);
					/*
						ADD BYTE PTR[AL],AL (0x00, 0x00) -> End of page!
					*/
					}else if(CompareMemory->DataByte[0] == 0x00 && CurrentInstructionSize == 2){
						FoundValidAPI = false;
						break;
					/*
						RET (0xC3)
					*/
					}else if(CompareMemory->DataByte[0] == 0xC3 && CurrentInstructionSize == 1){
						NumberOfInstructions++;
						break;
					/*
						RET (0xC2)
					*/
					}else if(CompareMemory->DataByte[0] == 0xC2 && CurrentInstructionSize == 3){
						NumberOfInstructions++;
						break;
					/*
						Short JUMP (0xEB)
					*/
					}else if(CompareMemory->DataByte[0] == 0xEB && CurrentInstructionSize == 2){
						TraceStartAddress = TraceStartAddress + CompareMemory->DataByte[1];
						SkipThisInstruction = true;
					/*
						CLC (0xF8)
					*/
					}else if(CompareMemory->DataByte[0] == 0xF8 && CurrentInstructionSize == 1){
						SkipThisInstruction = true;
					/*
						STC (0xF9)
					*/
					}else if(CompareMemory->DataByte[0] == 0xF9 && CurrentInstructionSize == 1){
						SkipThisInstruction = true;
					/*
						NOP (0x90)
					*/
					}else if(CompareMemory->DataByte[0] == 0x90 && CurrentInstructionSize == 1){
						SkipThisInstruction = true;
					/*
						FNOP (0xD9 0xD0)
					*/
					}else if(CompareMemory->DataByte[0] == 0xD9 && CompareMemory->DataByte[1] == 0xD0 && CurrentInstructionSize == 2){
						SkipThisInstruction = true;
					/*
						Multiple MOV
					*/
					}else if(CompareMemory->DataByte[0] >= 0x8A && CompareMemory->DataByte[0] <= 0x8B){
						/*
							MOV EAX,EAX (0x8B 0xC8)
						*/
						if(CompareMemory->DataByte[0] == 0x8B && CompareMemory->DataByte[1] == 0xC8 && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV EBX,EBX (0x8B 0xC9)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8B && CompareMemory->DataByte[1] == 0xC9 && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV ECX,ECX (0x8B 0xDB)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8B && CompareMemory->DataByte[1] == 0xDB && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV (0x8B 0xED)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8B && CompareMemory->DataByte[1] == 0xED && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV (0x8B 0xF6)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8B && CompareMemory->DataByte[1] == 0xF6 && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV (0x8B 0xE4)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8B && CompareMemory->DataByte[1] == 0xE4 && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV EDX,EDX (0x8B 0xD2)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8B && CompareMemory->DataByte[1] == 0xD2 && CurrentNumberOfInstructions != 1 && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV EDI,EDI (0x8B 0xFF)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8B && CompareMemory->DataByte[1] == 0xFF && CurrentNumberOfInstructions != 1 && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV AL,AL (0x8A 0xC0)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8A && CompareMemory->DataByte[1] == 0xC0 && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV BL,BL (0x8A 0xDB)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8A && CompareMemory->DataByte[1] == 0xDB && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV CL,CL (0x8A 0xC9)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8A && CompareMemory->DataByte[1] == 0xC9 && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV (0x8A 0xD2)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8A && CompareMemory->DataByte[1] == 0xD2 && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV (0x8A 0xE4)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8A && CompareMemory->DataByte[1] == 0xE4 && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV (0x8A 0xED)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8A && CompareMemory->DataByte[1] == 0xED && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV (0x8A 0xFF)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8A && CompareMemory->DataByte[1] == 0xFF && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV (0x8A 0xF6)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8A && CompareMemory->DataByte[1] == 0xF6 && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV AX,AX (0x8B 0xC0)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8B && CompareMemory->DataByte[1] == 0xC0 && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV (0x8B 0xDB)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8B && CompareMemory->DataByte[1] == 0xDB && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV (0x8B 0xC9)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8B && CompareMemory->DataByte[1] == 0xC9 && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV (0x8B 0xF6)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8B && CompareMemory->DataByte[1] == 0xF6 && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						/*
							MOV (0x8B 0xED)
						*/
						}else if(CompareMemory->DataByte[0] == 0x8B && CompareMemory->DataByte[1] == 0xED && CurrentInstructionSize == 2){
							SkipThisInstruction = true;
						}	
					/*
						RDTSC (0x0F 0x31)
					*/
					}else if(CompareMemory->DataByte[0] == 0x0F && CompareMemory->DataByte[1] == 0x31 && CurrentInstructionSize == 2){
						SkipThisInstruction = true;
					/*
						CPUID (0x0F 0xA2)
					*/
					}else if(CompareMemory->DataByte[0] == 0x0F && CompareMemory->DataByte[1] == 0xA2 && CurrentInstructionSize == 2){
						SkipThisInstruction = true;
					/*
						XCHG EAX,EAX (0x87 0xC0)
					*/
					}else if(CompareMemory->DataByte[0] == 0x87 && CompareMemory->DataByte[1] == 0xC0 && CurrentInstructionSize == 2){
						SkipThisInstruction = true;
					/*
						SHL EAX,0 - SHL EDI,0 && SHR EAX,0 - SHR EDI,0
					*/
					}else if(CompareMemory->DataByte[0] == 0xC1 && CurrentInstructionSize == 3){
						if(CompareMemory->DataByte[1] >= 0xE0 && CompareMemory->DataByte[1] <= 0xEF && CompareMemory->DataByte[2] == 0x00){
							SkipThisInstruction = true;
						}
					/*
						ROR EAX,0 - ROR EDI,0 && ROL EAX,0 - ROL EDI,0
					*/
					}else if(CompareMemory->DataByte[0] == 0xC1 && CurrentInstructionSize == 3){
						if(CompareMemory->DataByte[1] >= 0xC0 && CompareMemory->DataByte[1] <= 0xCF && CompareMemory->DataByte[2] == 0x00){
							SkipThisInstruction = true;
						}
					/*
						LEA EAX,DWORD PTR[EAX] -> LEA EDI,DWORD PTR[EDI]
					*/
					}else if(CompareMemory->DataByte[0] == 0x8D && CurrentInstructionSize == 2){
						if(CompareMemory->DataByte[1] == 0x00 || CompareMemory->DataByte[1] == 0x09 || CompareMemory->DataByte[1] == 0x1B || CompareMemory->DataByte[1] == 0x12){
							SkipThisInstruction = true;
						}
						if(CompareMemory->DataByte[1] == 0x36 || CompareMemory->DataByte[1] == 0x3F){
							SkipThisInstruction = true;
						}
						if(CompareMemory->DataByte[1] == 0x6D && CompareMemory->DataByte[2] == 0x00){
							SkipThisInstruction = true;
						}
					}
					if(!SkipThisInstruction){
						if(HashInstructions == true && SkipHashing == false){
							InstructionHash = EngineHashMemory((char*)TraceStartAddress, CurrentInstructionSize, InstructionHash);
						}
						LengthOfValidInstruction = LengthOfValidInstruction + CurrentInstructionSize;
						NumberOfInstructions++;
					}
					if(HashInstructions){
						InputNumberOfInstructions--;
						if(InputNumberOfInstructions > NULL){
							LoopCondition = true;
						}else{
							LoopCondition = false;
						}
					}else{
						if(CurrentNumberOfInstructions < 1000 && FoundValidAPI == false){
							LoopCondition = true;
						}else{
							LoopCondition = false;
						}
					}
					TraceStartAddress = TraceStartAddress + CurrentInstructionSize;
				}
				VirtualFree(TraceMemory, NULL, MEM_RELEASE);
				if(!HashInstructions){
					if(FoundValidAPI == true){
						return((ULONG_PTR)TraceTestAddress);
					}else if(CurrentNumberOfInstructions < 1000){
						if(ImporterGetAPINameFromDebugee(hProcess, LastPushValue) != NULL){
							return((ULONG_PTR)LastPushValue);
						}else if(ImporterGetAPINameFromDebugee(hProcess, LastPushValue - LengthOfValidInstruction) != NULL){
							return((ULONG_PTR)(LastPushValue - LengthOfValidInstruction));
						}
						return((DWORD)NumberOfInstructions);
					}
				}else{
					return((DWORD)InstructionHash);
				}
			}else{
				VirtualFree(TraceMemory, NULL, MEM_RELEASE);
			}
		}
	}
	return(NULL);
}
// UnpackEngine.Tracer.functions:
__declspec(dllexport) void __stdcall TracerInit(){
	return;		// UE 1.5 compatibility mode
}
__declspec(dllexport) long long __stdcall TracerLevel1(HANDLE hProcess, ULONG_PTR AddressToTrace){
	return((ULONG_PTR)EngineGlobalTracerHandler1(hProcess, AddressToTrace, false, NULL));
}
__declspec(dllexport) long long __stdcall HashTracerLevel1(HANDLE hProcess, ULONG_PTR AddressToTrace, DWORD InputNumberOfInstructions){

	unsigned int i = 0;
	unsigned int j = 0;
	DWORD Dummy = NULL;
	MODULEINFO RemoteModuleInfo;
	ULONG_PTR EnumeratedModules[0x2000];
	ULONG_PTR LoadedModules[1000][4];
	char RemoteDLLName[MAX_PATH];
	HANDLE hLoadedModule = NULL;
	HANDLE ModuleHandle = NULL;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_EXPORT_DIRECTORY PEExports;
	PEXPORTED_DATA ExportedFunctions;
	ULONG_PTR APIFoundAddress = NULL;
	bool ValidateHeader = false;
	bool FileIs64 = false;
	bool FoundAPI = false;
	DWORD CompareHash = NULL;
	DWORD TestHash = NULL;

	if(InputNumberOfInstructions > NULL){
		CompareHash = (DWORD)EngineGlobalTracerHandler1(hProcess, AddressToTrace, true, InputNumberOfInstructions);
	}else{
		InputNumberOfInstructions = (DWORD)TracerLevel1(hProcess, AddressToTrace);
		if(InputNumberOfInstructions < 1000){
			CompareHash = (DWORD)EngineGlobalTracerHandler1(hProcess, AddressToTrace, true, InputNumberOfInstructions);
		}else{
			return(NULL);
		}
	}
	RtlZeroMemory(&EnumeratedModules, 0x2000 * sizeof ULONG_PTR);
	RtlZeroMemory(&LoadedModules, 1000 * 4 * sizeof ULONG_PTR);
	if(hProcess == NULL){
		if(dbgProcessInformation.hProcess == NULL){
			hProcess = GetCurrentProcess();
		}else{
			hProcess = dbgProcessInformation.hProcess;
		}
	}
	if(EnumProcessModules(hProcess, (HMODULE*)EnumeratedModules, 0x2000, &Dummy)){
		i++;
		while(FoundAPI == false && EnumeratedModules[i] != NULL){
			ValidateHeader = false;
			RtlZeroMemory(&RemoteDLLName, MAX_PATH);
			GetModuleFileNameExA(hProcess, (HMODULE)EnumeratedModules[i], (LPSTR)RemoteDLLName, MAX_PATH);
			if(GetModuleHandleA(RemoteDLLName) == NULL){
				RtlZeroMemory(&RemoteDLLName, MAX_PATH);
				GetModuleBaseNameA(hProcess, (HMODULE)EnumeratedModules[i], (LPSTR)RemoteDLLName, MAX_PATH);
				if(GetModuleHandleA(RemoteDLLName) == NULL){
					if(engineAlowModuleLoading){
						hLoadedModule = LoadLibraryA(RemoteDLLName);
						if(hLoadedModule != NULL){
							LoadedModules[i][0] = EnumeratedModules[i];
							LoadedModules[i][1] = (ULONG_PTR)hLoadedModule;
							LoadedModules[i][2] = 1;
						}
					}else{
						hLoadedModule = (HANDLE)EngineSimulateDllLoader(hProcess, RemoteDLLName);
						if(hLoadedModule != NULL){
							LoadedModules[i][0] = EnumeratedModules[i];
							LoadedModules[i][1] = (ULONG_PTR)hLoadedModule;
							LoadedModules[i][2] = 1;
							ValidateHeader = true;
						}
					}
				}else{
					LoadedModules[i][0] = EnumeratedModules[i];
					LoadedModules[i][1] = (ULONG_PTR)GetModuleHandleA(RemoteDLLName);
					LoadedModules[i][2] = 0;
				}
			}else{
				LoadedModules[i][0] = EnumeratedModules[i];
				LoadedModules[i][1] = (ULONG_PTR)GetModuleHandleA(RemoteDLLName);
				LoadedModules[i][2] = 0;
			}

			if(!FoundAPI){
				DOSHeader = (PIMAGE_DOS_HEADER)LoadedModules[i][1];
				RtlZeroMemory(&RemoteModuleInfo, sizeof MODULEINFO);
				GetModuleInformation(hProcess, (HMODULE)LoadedModules[i][1], &RemoteModuleInfo, sizeof MODULEINFO);
				if(ValidateHeader || EngineValidateHeader((ULONG_PTR)LoadedModules[i][1], hProcess, RemoteModuleInfo.lpBaseOfDll, DOSHeader, false)){
					PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
					PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
					if(PEHeader32->OptionalHeader.Magic == 0x10B){
						FileIs64 = false;
					}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
						FileIs64 = true;
					}else{
						return(NULL);
					}
					if(!FileIs64){
						PEExports = (PIMAGE_EXPORT_DIRECTORY)(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + LoadedModules[i][1]);
						ExportedFunctions = (PEXPORTED_DATA)(PEExports->AddressOfFunctions + LoadedModules[i][1]);
					}else{
						PEExports = (PIMAGE_EXPORT_DIRECTORY)(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + LoadedModules[i][1]);
						ExportedFunctions = (PEXPORTED_DATA)(PEExports->AddressOfFunctions + LoadedModules[i][1]);
					}
					for(j = 0; j < PEExports->NumberOfFunctions; j++){
						TestHash = (DWORD)EngineGlobalTracerHandler1(hProcess, (ULONG_PTR)(ExportedFunctions->ExportedItem + LoadedModules[i][1]), true, InputNumberOfInstructions);
						if(TestHash == CompareHash){
							APIFoundAddress = (ULONG_PTR)(ExportedFunctions->ExportedItem + LoadedModules[i][0]);
							FoundAPI = true;
						}
						ExportedFunctions = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctions + 4);
					}
				}
			}
			i++;
		}
		i = 1;
		while(EnumeratedModules[i] != NULL){
			if(engineAlowModuleLoading){
				if(LoadedModules[i][2] == 1){
					FreeLibrary((HMODULE)LoadedModules[i][1]);
				}
			}else{
				if(LoadedModules[i][2] == 1){
					VirtualFree((void*)LoadedModules[i][1], NULL, MEM_RELEASE);
				}
			}
			i++;
		}
	}
	return((ULONG_PTR)APIFoundAddress);
}
__declspec(dllexport) long __stdcall TracerDetectRedirection(HANDLE hProcess, ULONG_PTR AddressToTrace){

	int i,j;
	MEMORY_BASIC_INFORMATION MemInfo;
	DWORD KnownRedirectionIndex = NULL;
	ULONG_PTR ueNumberOfBytesRead = NULL;
	PMEMORY_CMP_HANDLER cMem;
	DWORD MemoryHash = NULL;
	DWORD MaximumReadSize;
	DWORD TestAddressX86;
	LPVOID TraceMemory;
	bool HashCheck = false;

	VirtualQueryEx(hProcess, (LPVOID)AddressToTrace, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
	if(MemInfo.RegionSize > NULL){
		MaximumReadSize = (DWORD)((ULONG_PTR)MemInfo.AllocationBase + MemInfo.RegionSize - AddressToTrace);
		if(MaximumReadSize > 0x1000){
			MaximumReadSize = 0x1000;
			HashCheck = true;
		}else if(MaximumReadSize > 256){
			HashCheck = true;
		}
		if(sizeof HANDLE == 4){
			TraceMemory = VirtualAlloc(NULL, MaximumReadSize, MEM_COMMIT, PAGE_READWRITE);
			if(ReadProcessMemory(hProcess, (LPVOID)AddressToTrace, TraceMemory, MaximumReadSize, &ueNumberOfBytesRead)){
				cMem = (PMEMORY_CMP_HANDLER)TraceMemory;
				if(cMem->DataByte[0] == 0xEB && cMem->DataByte[1] == 0x01 && ((cMem->DataByte[3] >= 0x50 && cMem->DataByte[3] <= 0x5F) || cMem->DataByte[3] == 0x6A || cMem->DataByte[3] == 0x68)){
					KnownRedirectionIndex = NULL;				// ; PeX 0.99 fail safe!
				}else if(cMem->DataByte[0] == 0x68 && cMem->DataByte[5] == 0x81 && cMem->DataByte[12] == 0xC3){
					KnownRedirectionIndex = 1;					//	; RLP 0.7.4 & CryptoPeProtector 0.9.x & ACProtect
																/*	;$ ==>    >  68 904B4013     PUSH 13404B90
																	;$+5      >  812C24 0A9E589B SUB DWORD PTR SS:[ESP],9B589E0A
																	;$+C      >  C3              RET
																	;$+D      >  68 E21554DF     PUSH DF5415E2
																	;$+12     >  813424 B6DCB2A8 XOR DWORD PTR SS:[ESP],A8B2DCB6
																	;$+19     >  C3              RET
																	;$+1A     >  68 34B2C6B1     PUSH B1C6B234
																	;$+1F     >  810424 4A2C21C6 ADD DWORD PTR SS:[ESP],C6212C4A
																	;$+26     >  C3              RET */
				}else if(cMem->DataByte[0] == 0xFF && cMem->DataByte[1] == 0x25){
					KnownRedirectionIndex = 2;					//	; tELock 0.80 - 0.85
																//	;$ ==>    >- FF25 48018E00   JMP NEAR DWORD PTR DS:[8E0148]
				}else if((cMem->DataByte[0] == 0xFF && cMem->DataByte[1] == 0x35) || (cMem->DataByte[1] == 0xFF && cMem->DataByte[2] == 0x35) && (cMem->DataByte[8] == 0xC3 || cMem->DataByte[9] == 0xC3)){
					KnownRedirectionIndex = 3;					//	; tELock 0.90 - 0.95
																/*	;$ ==>    >  FF35 AE018E00   PUSH DWORD PTR DS:[8E01AE]               ; kernel32.InitializeCriticalSection
																	;$+6      >  A8 C3           TEST AL,0C3
																	;$+8      >  C3              RET
																	;$+9      >  F9              STC
																	;$+A      >  FF35 B2018E00   PUSH DWORD PTR DS:[8E01B2]               ; kernel32.VirtualFree
																	;$+10     >  80FA C3         CMP DL,0C3
																	;$+13     >  C3              RET */
				}else if(cMem->DataByte[0] == 0xEB && cMem->DataByte[1] == 0x01 && cMem->DataByte[2] == 0xC9 && cMem->DataByte[3] == 0x60 && cMem->DataByte[4] == 0x0F && cMem->DataByte[5] == 0x31){
					KnownRedirectionIndex = 8;					//	; AlexProtector 1.x
																/*	;$ ==>    > /EB 01           JMP SHORT 008413F9
																	;$+2      > |C9              LEAVE
																	;$+3      > \60              PUSHAD
																	;$+4      >  0F31            RDTSC
																	;$+6      >  EB 01           JMP SHORT 008413FF
																	;$+8      >  C9              LEAVE
																	;$+9      >  8BD8            MOV EBX,EAX
																	;$+B      >  EB 01           JMP SHORT 00841404
																	;...
																	;$+33     >  68 E9B9D477     PUSH USER32.PostQuitMessage
																	;$+38     >  EB 01           JMP SHORT 00841431
																	;$+3A     >- E9 C3EB01E9     JMP E985FFF8 */
				}else if((cMem->DataByte[0] == 0x0B && cMem->DataByte[1] == 0xC5) || (cMem->DataByte[0] == 0x05 && cMem->DataByte[5] == 0xB8 && cMem->DataByte[10] == 0xEB && cMem->DataByte[11] == 0x02)){
					KnownRedirectionIndex = 5;					//	; tELock 0.99 - 1.0 Private!
																/*	;008E0122    05 F9DEBE71     ADD EAX,71BEDEF9
																	;008E0127    B8 28018E00     MOV EAX,8E0128
																	;008E012C    EB 02           JMP SHORT 008E0130
																	;008E012E    CD 20           INT 20
																	;008E0130    05 18000000     ADD EAX,18
																	;008E0135    8B00            MOV EAX,DWORD PTR DS:[EAX]
																	;008E0137    35 22018E00     XOR EAX,8E0122
																	;008E013C    90              NOP
																	;008E013D    90              NOP
																	;008E013E    50              PUSH EAX
																	;008E013F    C3              RET
																	;
																	;00850036    13C4            ADC EAX,ESP
																	;00850038    E8 0A000000     CALL 00850047
																	;0085003D    90              NOP
																	;0085003E    1BC2            SBB EAX,EDX
																	;00850040    E9 09000000     JMP 0085004E
																	;00850045    1BC3            SBB EAX,EBX
																	;00850047    83F8 74         CMP EAX,74
																	;0085004A    C3              RET
																	;0085004B    98              CWDE
																	;0085004C    33C7            XOR EAX,EDI
																	;0085004E    D6              SALC
																	;0085004F    B8 50008500     MOV EAX,850050
																	;00850054    EB 02           JMP SHORT 00850058
																	;00850056    CD 20           INT 20
																	;00850058    05 18000000     ADD EAX,18
																	;0085005D    8B00            MOV EAX,DWORD PTR DS:[EAX]
																	;0085005F    35 36008500     XOR EAX,850036
																	;00850064    90              NOP
																	;00850065    90              NOP
																	;00850066    50              PUSH EAX
																	;00850067    C3              RET */
				}else if((cMem->DataByte[0] == 0x13 && cMem->DataByte[1] == 0xC4 && cMem->DataByte[2] == 0xE8) || (cMem->DataByte[0] == 0x83 && cMem->DataByte[3] == 0xE8)){
					KnownRedirectionIndex = 5;					//	; tELock 0.99 - 1.0 Private!
				}else if((cMem->DataByte[0] == 0xB8 || cMem->DataByte[0] == 0x1D || cMem->DataByte[0] == 0x0D || cMem->DataByte[0] == 0x2D) && cMem->DataByte[5] == 0xB8 && cMem->DataByte[10] == 0xEB && cMem->DataByte[11] == 0x02){
					KnownRedirectionIndex = 5;					//	; tELock 0.99 - 1.0 Private!
																/*	;011F0000    B8 2107F205     MOV EAX,5F20721
																	;011F0005    B8 06008D00     MOV EAX,8D0006
																	;011F000A    EB 02           JMP SHORT 011F000E
																	;011F000C    CD 20           INT 20
																	;011F000E    05 18000000     ADD EAX,18
																	;011F0013    8B00            MOV EAX,DWORD PTR DS:[EAX]
																	;011F0015    35 00008D00     XOR EAX,8D0000
																	;011F001A    90              NOP
																	;011F001B    90              NOP
																	;011F001C    50              PUSH EAX
																	;011F001D    C3              RET
																	;
																	;01360000    1D A508F205     SBB EAX,5F208A5
																	;01360005    B8 28008D00     MOV EAX,8D0028
																	;0136000A    EB 02           JMP SHORT 0136000E
																	;0136000C    CD 20           INT 20
																	;0136000E    05 18000000     ADD EAX,18
																	;01360013    8B00            MOV EAX,DWORD PTR DS:[EAX]
																	;01360015    35 22008D00     XOR EAX,8D0022
																	;0136001A    90              NOP
																	;0136001B    90              NOP
																	;0136001C    50              PUSH EAX
																	;0136001D    C3              RET
																	;
																	;014B0000    0D F918F205     OR EAX,5F218F9
																	;014B0005    B8 4A008D00     MOV EAX,8D004A
																	;014B000A    EB 02           JMP SHORT 014B000E
																	;014B000C    CD 20           INT 20
																	;014B000E    05 18000000     ADD EAX,18
																	;014B0013    8B00            MOV EAX,DWORD PTR DS:[EAX]
																	;014B0015    35 44008D00     XOR EAX,8D0044
																	;014B001A    90              NOP
																	;014B001B    90              NOP
																	;014B001C    50              PUSH EAX
																	;014B001D    C3              RET
																	;
																	;01750000    2D 0B37F205     SUB EAX,5F2370B
																	;01750005    B8 8E008D00     MOV EAX,8D008E
																	;0175000A    EB 02           JMP SHORT 0175000E
																	;0175000C    CD 20           INT 20
																	;0175000E    05 18000000     ADD EAX,18
																	;01750013    8B00            MOV EAX,DWORD PTR DS:[EAX]
																	;01750015    35 88008D00     XOR EAX,8D0088
																	;0175001A    90              NOP
																	;0175001B    90              NOP
																	;0175001C    50              PUSH EAX
																	;0175001D    C3              RET
																	;
																	;019F0000    0BC4            OR EAX,ESP
																	;019F0002    F9              STC
																	;019F0003    E8 0B000000     CALL 019F0013
																	;019F0008    90              NOP
																	;019F0009    13C4            ADC EAX,ESP
																	;019F000B    E9 0A000000     JMP 019F001A
																	;019F0010    F9              STC
																	;019F0011    13C3            ADC EAX,EBX
																	;019F0013    98              CWDE
																	;019F0014    03C2            ADD EAX,EDX
																	;019F0016    C3              RET
																	;
																	;01B40000    48              DEC EAX
																	;01B40001    E8 0D000000     CALL 01B40013
																	;01B40006    03C5            ADD EAX,EBP
																	;01B40008    FC              CLD
																	;01B40009    E9 0A000000     JMP 01B40018
																	;01B4000E    35 D82FF205     XOR EAX,5F22FD8
																	;01B40013    C1C8 9A         ROR EAX,9A
																	;01B40016    C3              RET */
				}else if((cMem->DataByte[0] == 0x0B && cMem->DataByte[1] == 0xC4 && cMem->DataByte[2] == 0xF9 && cMem->DataByte[3] == 0xE8) || (cMem->DataByte[0] == 0x48 && cMem->DataByte[1] == 0xE8)){
					KnownRedirectionIndex = 5;					//	; tELock 0.99 - 1.0 Private!
				}else if((cMem->DataByte[0] == 0xB8 && cMem->DataByte[5] == 0xE8 && cMem->DataByte[10] == 0xF9 && cMem->DataByte[11] == 0xE9) && (cMem->DataByte[0] == 0xE8 && cMem->DataByte[1] == 0x0B && cMem->DataByte[10] == 0xE9 && cMem->DataByte[11] == 0x05 && cMem->DataByte[15] == 0x90 && cMem->DataByte[16] == 0xC3)){
					KnownRedirectionIndex = 5;					//	; tELock 0.99 - 1.0 Private!
																/*	;01C90000    B8 B853F205     MOV EAX,5F253B8
																	;01C90005    E8 07000000     CALL 01C90011
																	;01C9000A    F9              STC
																	;01C9000B    E9 07000000     JMP 01C90017
																	;01C90010    90              NOP
																	;01C90011    23C3            AND EAX,EBX
																	;01C90013    C3              RET
																	;
																	;00A40022    1BC2            SBB EAX,EDX
																	;00A40024    E8 08000000     CALL 00A40031
																	;00A40029    40              INC EAX
																	;00A4002A    E9 09000000     JMP 00A40038
																	;00A4002F    33C7            XOR EAX,EDI
																	;00A40031    C1E8 92         SHR EAX,92
																	;00A40034    C3              RET
																	;00A40035    83E0 25         AND EAX,25
																	;00A40038    25 E5AE65DD     AND EAX,DD65AEE5
																	;00A4003D    B8 3E00A400     MOV EAX,0A4003E
																	;00A40042    EB 02           JMP SHORT 00A40046
																	;00A40044    CD 20           INT 20
																	;00A40046    05 18000000     ADD EAX,18
																	;00A4004B    8B00            MOV EAX,DWORD PTR DS:[EAX]
																	;00A4004D    35 2200A400     XOR EAX,0A40022
																	;00A40052    90              NOP
																	;00A40053    90              NOP
																	;00A40054    50              PUSH EAX
																	;00A40055    C3              RET
																	;
																	;00A4005A    E8 0B000000     CALL 00A4006A
																	;00A4005F    15 06F265DD     ADC EAX,DD65F206
																	;00A40064    E9 05000000     JMP 00A4006E
																	;00A40069    90              NOP
																	;00A4006A    C3              RET
																	;00A4006B    1BC5            SBB EAX,EBP
																	;00A4006D    40              INC EAX
																	;00A4006E    1BC0            SBB EAX,EAX
																	;00A40070    F9              STC
																	;00A40071    B8 7200A400     MOV EAX,0A40072
																	;00A40076    EB 02           JMP SHORT 00A4007A
																	;00A40078    CD 20           INT 20
																	;00A4007A    05 18000000     ADD EAX,18
																	;00A4007F    8B00            MOV EAX,DWORD PTR DS:[EAX]
																	;00A40081    35 5A00A400     XOR EAX,0A4005A
																	;00A40086    90              NOP
																	;00A40087    90              NOP
																	;00A40088    50              PUSH EAX
																	;00A40089    C3              RET */
				}else if(cMem->DataByte[0] == 0x1B && cMem->DataByte[1] == 0xC2 && cMem->DataByte[2] == 0xE8 && cMem->DataByte[3] == 0x08 && cMem->DataByte[7] == 0x40 && cMem->DataByte[8] == 0xE9 && cMem->DataByte[9] == 0x09 && cMem->DataByte[10] == 0x00){
					KnownRedirectionIndex = 5;					//	; tELock 0.99 - 1.0 Private!
				}else if(cMem->DataByte[0] == 0x68 && cMem->DataByte[5] == 0xE9){
					RtlMoveMemory(&TestAddressX86, &cMem->DataByte[1], 4);
					if(TestAddressX86 > AddressToTrace){
						if(ImporterGetAPIName((ULONG_PTR)TestAddressX86) != NULL){
							KnownRedirectionIndex = 6;			//	; ReCrypt 0.74
																/*	;001739F1    68 E9D9D477     PUSH User32.EndDialog
																	;001739F6  ^ E9 FDFEFFFF     JMP 001738F8 */
						}
					}
				}else if((cMem->DataByte[0] == 0xE8 && cMem->DataByte[5] == 0x58 && cMem->DataByte[6] == 0xEB && cMem->DataByte[7] == 0x01) || (cMem->DataByte[0] == 0xC8 && cMem->DataByte[4] == 0xE8 && cMem->DataByte[9] == 0x5B)){
					KnownRedirectionIndex = 7;					//	; Orien 2.1x
																/* ;GetCommandLineA
																;$ ==>    >/$  E8 00000000     CALL crackme_.0040DF8F
																;$+5      >|$  58              POP EAX
																;$+6      >|.  EB 01           JMP SHORT crackme_.0040DF93
																;$+8      >|   B8              DB B8
																;$+9      >|>  85DB            TEST EBX,EBX
																;$+B      >|.  2D 8F1F0000     SUB EAX,1F8F
																;$+10     >|.  EB 01           JMP SHORT crackme_.0040DF9D
																;$+12     >|   A8              DB A8
																;$+13     >|>  8D80 F0550000   LEA EAX,DWORD PTR DS:[EAX+55F0]
																;$+19     >\.  C3              RET
																;GetCommandLineW
																;$ ==>    > .  E8 00000000     CALL crackme_.0040DFA9
																;$+5      >/$  58              POP EAX
																;$+6      >|.  EB 01           JMP SHORT crackme_.0040DFAD
																;$+8      >|   B8              DB B8
																;$+9      >|>  85DB            TEST EBX,EBX
																;$+B      >|.  2D A91F0000     SUB EAX,1FA9
																;$+10     >|.  EB 01           JMP SHORT crackme_.0040DFB7
																;$+12     >|   A8              DB A8
																;$+13     >|>  8D80 F4560000   LEA EAX,DWORD PTR DS:[EAX+56F4]
																;$+19     >\.  C3              RET
																;ExitProcess
																;$ ==>    > $  C8 000000       ENTER 0,0
																;$+4      > .  E8 00000000     CALL crackme_.0040DF2A
																;$+9      > $  5B              POP EBX
																;$+A      > .  EB 01           JMP SHORT crackme_.0040DF2E
																;$+C      >    B8              DB B8
																;$+D      > >  85DB            TEST EBX,EBX
																;$+F      > .  81EB 2A1F0000   SUB EBX,1F2A
																;$+15     > .  EB 01           JMP SHORT crackme_.0040DF39
																;$+17     >    A8              DB A8
																;$+18     > >  8D83 4D310000   LEA EAX,DWORD PTR DS:[EBX+314D]
																;$+1E     > .  8038 00         CMP BYTE PTR DS:[EAX],0
																;$+21     > .  74 29           JE SHORT crackme_.0040DF6D
																;$+23     > .  EB 01           JMP SHORT crackme_.0040DF47
																;$+25     >    A8              DB A8
																;$+26     > >  8D93 55380000   LEA EDX,DWORD PTR DS:[EBX+3855]
																;$+2C     > .  E8 01000000     CALL crackme_.0040DF53
																;$+31     >    E9              DB E9
																;$+32     > $  83EC FC         SUB ESP,-4
																;$+35     > .  6A 00           PUSH 0
																;$+37     > .  52              PUSH EDX
																;$+38     > .  50              PUSH EAX
																;$+39     > .  6A 00           PUSH 0
																;$+3B     > .  E8 05000000     CALL crackme_.0040DF66
																;$+40     > .  EB 0A           JMP SHORT crackme_.0040DF6D
																;$+42     >    88              DB 88
																;$+43     >    FC              DB FC
																;$+44     >    B6              DB B6
																;$+45     > $  FFA3 FF3A0000   JMP NEAR DWORD PTR DS:[EBX+3AFF]
																;$+4B     >    CD              DB CD
																;$+4C     > >  E8 01000000     CALL crackme_.0040DF73
																;$+51     >    E9              DB E9
																;$+52     > $  83EC FC         SUB ESP,-4
																;$+55     > .  FF75 08         PUSH DWORD PTR SS:[EBP+8]
																;$+58     > .  E8 05000000     CALL crackme_.0040DF83
																;$+5D     > .  EB 0A           JMP SHORT crackme_.0040DF8A
																;$+5F     >    88              DB 88
																;$+60     >    FC              DB FC
																;$+61     >    B6              DB B6
																;$+62     > $  FFA3 BF3A0000   JMP NEAR DWORD PTR DS:[EBX+3ABF] */
				}else if((cMem->DataByte[0] == 0xEB && cMem->DataByte[1] == 0x01 && cMem->DataByte[2] == 0x66 && cMem->DataByte[3] == 0x1B) || (cMem->DataByte[0] == 0xEB && cMem->DataByte[1] == 0x02 && cMem->DataByte[2] == 0xCD && cMem->DataByte[3] == 0x20) || (cMem->DataByte[0] == 0xEB && cMem->DataByte[1] == 0x01 && cMem->DataByte[2] == 0xB8 && cMem->DataByte[3] == 0xEB)){
					KnownRedirectionIndex = 4;					// ; tELock 0.96 - 0.98
																/* ;(BYTE PTR[ESI] == 0EBh && (BYTE PTR[ESI+3] == 0EBh || BYTE PTR[ESI+2] == 0EBh))
																;017B0000    0BE4            OR ESP,ESP
																;017B0002    75 01           JNZ SHORT 017B0005
																;
																;15940000    85E4            TEST ESP,ESP
																;15940002    79 03           JNS SHORT 15940007
																;
																;008E0359    B8 8DE44500     MOV EAX,45E48D
																;008E035E    90              NOP
																;008E035F    FF30            PUSH DWORD PTR DS:[EAX]
																;008E0361    C3              RET
																;
																;008F0033    B8 AF008F00     MOV EAX,8F00AF
																;008F0038    40              INC EAX
																;008F0039    FF30            PUSH DWORD PTR DS:[EAX]
																;008F003B    C3              RET
																;
																;008E02F7    B8 20078E00     MOV EAX,8E0720
																;008E02FC    FF20            JMP NEAR DWORD PTR DS:[EAX] */
				}else if((cMem->DataByte[0] == 0xEB && cMem->DataByte[1] == 0x03 && cMem->DataByte[2] == 0xFF && cMem->DataByte[3] == 0xEB) || (cMem->DataByte[0] == 0xEB && cMem->DataByte[1] == 0x01 && cMem->DataByte[2] == 0xB8 && cMem->DataByte[3] == 0x05) || (cMem->DataByte[0] == 0xEB && cMem->DataByte[1] == 0x02 && cMem->DataByte[2] == 0xFF && cMem->DataByte[3] == 0x20)){
					KnownRedirectionIndex = 4;					// ; tELock 0.96 - 0.98
				}else if((cMem->DataByte[0] == 0xF9 || cMem->DataByte[0] == 0xF8) || (cMem->DataByte[0] == 0x0B && cMem->DataByte[1] == 0xE4) || (cMem->DataByte[0] == 0x85 && cMem->DataByte[1] == 0xE4)){
					KnownRedirectionIndex = 4;					// ; tELock 0.96 - 0.98
				}else if(cMem->DataByte[0] == 0xEB && (cMem->DataByte[1] > NULL && cMem->DataByte[1] < 4)){
					i = 2;
					j = 30;
					while(j > NULL){
						if(cMem->DataByte[i] == 0xB8 && (cMem->DataByte[i+5] == 0x40 || cMem->DataByte[i+5] == 0x90) && cMem->DataByte[i+6] == 0xFF && cMem->DataByte[i+7] == 0x30 && cMem->DataByte[i+8] == 0xC3){
							KnownRedirectionIndex = 4;			// ; tELock 0.96 - 0.98		
							j = 1;
						}
						i++;
						j--;
					}
				}else if(HashCheck){
					if(cMem->DataByte[0] == 0x9C || cMem->DataByte[0] == 0xEB){
						MemoryHash = EngineHashMemory((char*)TraceMemory, 192, MemoryHash);
						if(MemoryHash == 0x5AF7E209 || MemoryHash == 0xEB480CAC || MemoryHash == 0x86218561 || MemoryHash == 0xCA9ABD85){
							KnownRedirectionIndex = 9;			// ; SVKP 1.x
						}else if(MemoryHash == 0xF1F84A98 || MemoryHash == 0x91823290 || MemoryHash == 0xBEE6BAA0 || MemoryHash == 0x79603232){
							KnownRedirectionIndex = 9;			// ; SVKP 1.x
						}
					}
				}
				VirtualFree(TraceMemory, NULL, MEM_RELEASE);
				return(KnownRedirectionIndex);
			}else{
				VirtualFree(TraceMemory, NULL, MEM_RELEASE);
			}
		}
	}
	return(NULL);
}
__declspec(dllexport) long long __stdcall TracerFixKnownRedirection(HANDLE hProcess, ULONG_PTR AddressToTrace, DWORD RedirectionId){

	int i = NULL;
	DWORD TestAddressX86;
	DWORD ReadAddressX86;
	DWORD MaximumReadSize;
	DWORD MemoryHash = NULL;
	PMEMORY_CMP_HANDLER cMem;
	MEMORY_BASIC_INFORMATION MemInfo;
	ULONG_PTR ueNumberOfBytesRead = NULL;
	LPVOID TracerReadMemory = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	cMem = (PMEMORY_CMP_HANDLER)TracerReadMemory;

	VirtualQueryEx(hProcess, (LPVOID)AddressToTrace, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
	if(MemInfo.RegionSize > NULL){
		MaximumReadSize = (DWORD)((ULONG_PTR)MemInfo.AllocationBase + MemInfo.RegionSize - AddressToTrace);
		if(MaximumReadSize > 0x1000){
			MaximumReadSize = 0x1000;
		}
	}
	if(RedirectionId == NULL){
		RedirectionId = (DWORD)TracerDetectRedirection(hProcess, AddressToTrace);
	}
	if(RedirectionId == 1){												//	TracerFix_ACProtect
		__try{
			if(ReadProcessMemory(hProcess, (LPVOID)AddressToTrace, TracerReadMemory, MaximumReadSize, &ueNumberOfBytesRead)){
				RtlMoveMemory(&TestAddressX86, &cMem->DataByte[1], 4);
				if(cMem->DataByte[5] == 0x81 && cMem->DataByte[6] == 0x2C){
					RtlMoveMemory(&ReadAddressX86, &cMem->DataByte[8], 4);
					TestAddressX86 = TestAddressX86 - ReadAddressX86;
				}else if(cMem->DataByte[5] == 0x81 && cMem->DataByte[6] == 0x34){
					RtlMoveMemory(&ReadAddressX86, &cMem->DataByte[8], 4);
					TestAddressX86 = TestAddressX86 ^ ReadAddressX86;
				}else if(cMem->DataByte[5] == 0x81 && cMem->DataByte[6] == 0x04){
					RtlMoveMemory(&ReadAddressX86, &cMem->DataByte[8], 4);
					TestAddressX86 = TestAddressX86 + ReadAddressX86;
				}
				VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
				return((DWORD)TestAddressX86);
			}
		}__except(EXCEPTION_EXECUTE_HANDLER){
			VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
			return(NULL);
		}
	}else if(RedirectionId == 2){										//	TracerFix_tELock_varA
		__try{
			if(ReadProcessMemory(hProcess, (LPVOID)AddressToTrace, TracerReadMemory, MaximumReadSize, &ueNumberOfBytesRead)){
				RtlMoveMemory(&TestAddressX86, &cMem->DataByte[2], 4);
				if(ReadProcessMemory(hProcess, (LPVOID)TestAddressX86, &TestAddressX86, 4, &ueNumberOfBytesRead)){
					VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
					return((DWORD)TestAddressX86);
				}
			}
		}__except(EXCEPTION_EXECUTE_HANDLER){
			VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
			return(NULL);
		}
	}else if(RedirectionId == 3){										//	TracerFix_tELock_varB
		__try{
			if(ReadProcessMemory(hProcess, (LPVOID)AddressToTrace, TracerReadMemory, MaximumReadSize, &ueNumberOfBytesRead)){
				if(cMem->DataByte[0] == 0xFF && cMem->DataByte[1] == 0x35){
					RtlMoveMemory(&TestAddressX86, &cMem->DataByte[2], 4);
				}else{
					RtlMoveMemory(&TestAddressX86, &cMem->DataByte[3], 4);
				}
				if(ReadProcessMemory(hProcess, (LPVOID)TestAddressX86, &TestAddressX86, 4, &ueNumberOfBytesRead)){
					VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
					return((DWORD)TestAddressX86);
				}
			}
		}__except(EXCEPTION_EXECUTE_HANDLER){
			VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
			return(NULL);
		}
	}else if(RedirectionId == 4){										//	TracerFix_tELock_varC
		__try{
			if(ReadProcessMemory(hProcess, (LPVOID)AddressToTrace, TracerReadMemory, MaximumReadSize, &ueNumberOfBytesRead)){
				i = 100;
				if(cMem->DataByte[0] == 0xEB && (cMem->DataByte[1] > 0 && cMem->DataByte[1] < 4)){
					cMem = (PMEMORY_CMP_HANDLER)((ULONG_PTR)cMem + cMem->DataByte[1] + 2);
				}
				while(i > NULL && (cMem->DataByte[0] != 0xFF && (cMem->DataByte[1] != 0x20 || cMem->DataByte[1] != 0x30))){
					cMem = (PMEMORY_CMP_HANDLER)((ULONG_PTR)cMem + 1);
					i--;
				}
				if(i != NULL && cMem->DataByte[0] == 0xFF && cMem->DataByte[1] == 0x20){
					if(cMem->DataByte[2] != 0x90){
						cMem = (PMEMORY_CMP_HANDLER)((ULONG_PTR)cMem + 1);
						while(i > NULL && (cMem->DataByte[0] != 0xFF && (cMem->DataByte[1] != 0x20 || cMem->DataByte[1] != 0x30))){
							cMem = (PMEMORY_CMP_HANDLER)((ULONG_PTR)cMem + 1);
							i--;
						}
					}
				}
				if(i != NULL && cMem->DataByte[0] == 0xFF && cMem->DataByte[1] == 0x30){
					cMem = (PMEMORY_CMP_HANDLER)((ULONG_PTR)cMem - 6);
					if(cMem->DataByte[0] == 0xB8){
						RtlMoveMemory(&TestAddressX86, &cMem->DataByte[1], 4);
						if(cMem->DataByte[5] == 0x40){
							TestAddressX86++;
						}
					}else{
						RtlMoveMemory(&TestAddressX86, &cMem->DataByte[2], 4);
					}
					if(ReadProcessMemory(hProcess, (LPVOID)TestAddressX86, &TestAddressX86, 4, &ueNumberOfBytesRead)){
						VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
						return((DWORD)TestAddressX86);
					}
				}else if(i != NULL && cMem->DataByte[0] == 0xFF && cMem->DataByte[1] == 0x20){
					cMem = (PMEMORY_CMP_HANDLER)((ULONG_PTR)cMem - 6);
					RtlMoveMemory(&TestAddressX86, &cMem->DataByte[2], 4);
					if(ReadProcessMemory(hProcess, (LPVOID)TestAddressX86, &TestAddressX86, 4, &ueNumberOfBytesRead)){
						VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
						return((DWORD)TestAddressX86);
					}
				}
			}
		}__except(EXCEPTION_EXECUTE_HANDLER){
			VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
			return(NULL);
		}
	}else if(RedirectionId == 5){										//	TracerFix_tELock_varD
		__try{
			if(ReadProcessMemory(hProcess, (LPVOID)AddressToTrace, TracerReadMemory, MaximumReadSize, &ueNumberOfBytesRead)){
				i = 100;
				while(i > NULL && (cMem->DataByte[0] != 0x50 || cMem->DataByte[1] != 0xC3)){
					cMem = (PMEMORY_CMP_HANDLER)((ULONG_PTR)cMem + 1);
					i--;
				}
				if(i != NULL && cMem->DataByte[0] == 0x50 && cMem->DataByte[1] == 0xC3){
					cMem = (PMEMORY_CMP_HANDLER)((ULONG_PTR)cMem - 0x16);
					RtlMoveMemory(&ReadAddressX86, &cMem->DataByte[0x10], 4);
					RtlMoveMemory(&TestAddressX86, &cMem->DataByte[0], 4);
					TestAddressX86 = TestAddressX86 + 0x18;
					if(ReadProcessMemory(hProcess, (LPVOID)TestAddressX86, &TestAddressX86, 4, &ueNumberOfBytesRead)){
						TestAddressX86 = TestAddressX86 ^ ReadAddressX86;
						VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
						return((DWORD)TestAddressX86);
					}
				}
			}
		}__except(EXCEPTION_EXECUTE_HANDLER){
			VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
			return(NULL);
		}
	}else if(RedirectionId == 6){										//	TracerFix_ReCrypt
		__try{
			if(ReadProcessMemory(hProcess, (LPVOID)AddressToTrace, TracerReadMemory, MaximumReadSize, &ueNumberOfBytesRead)){
				RtlMoveMemory(&TestAddressX86, &cMem->DataByte[1], 4);
				VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
				return((DWORD)TestAddressX86);
			}
		}__except(EXCEPTION_EXECUTE_HANDLER){
			VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
			return(NULL);
		}
	}else if(RedirectionId == 7){										//	TracerFix_Orien
		__try{
			if(ReadProcessMemory(hProcess, (LPVOID)AddressToTrace, TracerReadMemory, MaximumReadSize, &ueNumberOfBytesRead)){
				if(cMem->DataByte[0] == 0xE8){
					RtlMoveMemory(&ReadAddressX86, &cMem->DataByte[0x15], 4);
					if(ReadAddressX86 == 0x55F0){
						TestAddressX86 = (DWORD)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetCommandLineA"));
					}else{
						TestAddressX86 = (DWORD)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetCommandLineW"));
					}
					VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
					return((DWORD)TestAddressX86);
				}else if(cMem->DataByte[0] == 0xC8){
					TestAddressX86 = (DWORD)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "ExitProcess"));
					VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
					return((DWORD)TestAddressX86);
				}
			}
		}__except(EXCEPTION_EXECUTE_HANDLER){
			VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
			return(NULL);
		}
	}else if(RedirectionId == 8){										//	TracerFix_AlexProtector
		__try{
			if(ReadProcessMemory(hProcess, (LPVOID)AddressToTrace, TracerReadMemory, MaximumReadSize, &ueNumberOfBytesRead)){
				cMem = (PMEMORY_CMP_HANDLER)((ULONG_PTR)cMem + 0x34);
				RtlMoveMemory(&TestAddressX86, &cMem->DataByte[0], 4);
				VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
				return((DWORD)TestAddressX86);
			}
		}__except(EXCEPTION_EXECUTE_HANDLER){
			VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
			return(NULL);
		}
	}else if(RedirectionId == 9 && MaximumReadSize > 192){				//	TracerFix_SVKP
		__try{
			if(ReadProcessMemory(hProcess, (LPVOID)AddressToTrace, TracerReadMemory, MaximumReadSize, &ueNumberOfBytesRead)){
				if(cMem->DataByte[0] == 0x9C || cMem->DataByte[0] == 0xEB){
					MemoryHash = EngineHashMemory((char*)TracerReadMemory, 192, MemoryHash);
					if(MemoryHash == 0x5AF7E209){
						TestAddressX86 = (DWORD)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetCommandLineA"));
					}else if(MemoryHash == 0xEB480CAC){
						TestAddressX86 = (DWORD)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "ExitProcess"));
					}else if(MemoryHash == 0x86218561){
						TestAddressX86 = (DWORD)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetCurrentProcess"));
					}else if(MemoryHash == 0xCA9ABD85){
						TestAddressX86 = (DWORD)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetVersion"));
					}else if(MemoryHash == 0xF1F84A98){
						TestAddressX86 = (DWORD)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetVersionExA"));
					}else if(MemoryHash == 0x91823290){
						TestAddressX86 = (DWORD)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetModuleHandleA"));
					}else if(MemoryHash == 0xBEE6BAA0){
						TestAddressX86 = (DWORD)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("user32.dll"), "MessageBoxA"));
					}else if(MemoryHash == 0x79603232){
						TestAddressX86 = (DWORD)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetModuleHandleA"));
					}
					VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
					return((DWORD)TestAddressX86);
				}
			}
		}__except(EXCEPTION_EXECUTE_HANDLER){
			VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
			return(NULL);
		}
	}
	VirtualFree(TracerReadMemory, NULL, MEM_RELEASE);
	return(NULL);
}
// UnpackEngine.Exporter.functions:
__declspec(dllexport) void __stdcall ExporterCleanup(){

	int i = NULL;

	for(i = 0; i < 1000; i++){
		expExportAddress[i] = 0;
		expNamePointers[i] = 0;
		expNameHashes[i] = 0;
		expOrdinals[i] = 0;
	}
	RtlZeroMemory(&szExportFileName, 512);
	RtlZeroMemory(&expExportData, sizeof IMAGE_EXPORT_DIRECTORY);
	VirtualFree(expTableData, NULL, MEM_RELEASE);
	expExportNumber = NULL;
	expTableData = NULL;
	expImageBase = NULL;
}
__declspec(dllexport) void __stdcall ExporterSetImageBase(ULONG_PTR ImageBase){
	expImageBase = ImageBase;
}
__declspec(dllexport) void __stdcall ExporterInit(DWORD MemorySize, ULONG_PTR ImageBase, DWORD ExportOrdinalBase, char* szExportModuleName){
	
	if(expTableData != NULL){
		ExporterCleanup();
	}
	expExportData.Base = ExportOrdinalBase;
	expTableData = VirtualAlloc(NULL, MemorySize, MEM_COMMIT, PAGE_READWRITE);
	if(szExportModuleName != NULL){
		RtlMoveMemory(expTableData, szExportModuleName, lstrlenA(szExportModuleName));
		expTableDataCWP = (LPVOID)((ULONG_PTR)expTableData + lstrlenA(szExportModuleName) + 2);
		expNamePresent = true;
	}else{
		expTableDataCWP = expTableData;
		expNamePresent = false;
	}
	expImageBase = ImageBase;
}
__declspec(dllexport) bool __stdcall ExporterAddNewExport(char* szExportName, DWORD ExportRelativeAddress){

	unsigned int i;
	DWORD NameHash;

	if(expTableDataCWP != NULL && szExportName != NULL){
		NameHash = (DWORD)EngineHashString(szExportName);
		for(i = 0; i < expExportNumber; i++){
			if(expNameHashes[i] == NameHash){
				return(true);
			}
		}
		expExportAddress[expExportNumber] = ExportRelativeAddress;
		expNamePointers[expExportNumber] = (DWORD)expTableDataCWP;
		expNameHashes[expExportNumber] = (DWORD)EngineHashString(szExportName);
		expOrdinals[expExportNumber] = (WORD)(expExportNumber);
		RtlMoveMemory(expTableDataCWP, szExportName, lstrlenA(szExportName));
		expTableDataCWP = (LPVOID)((ULONG_PTR)expTableDataCWP + lstrlenA(szExportName) + 2);
		expExportNumber++;
		return(true);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall ExporterAddNewOrdinalExport(DWORD OrdinalNumber, DWORD ExportRelativeAddress){

	unsigned int i = NULL;
	char szExportFunctionName[512];

	RtlZeroMemory(&szExportFunctionName, 512);
	if(expTableDataCWP != NULL){
		if(expExportNumber == NULL){
			expExportData.Base = OrdinalNumber;
			wsprintfA(szExportFunctionName, "Func%d", expExportNumber + 1);
			return(ExporterAddNewExport(szExportFunctionName, ExportRelativeAddress));
		}else{
			if(OrdinalNumber == expExportData.Base + expExportNumber - 1){
				wsprintfA(szExportFunctionName, "Func%d", expExportNumber + 1);
				return(ExporterAddNewExport(szExportFunctionName, ExportRelativeAddress));
			}else if(OrdinalNumber > expExportData.Base + expExportNumber - 1){
				for(i = expExportData.Base + expExportNumber - 1; i <= OrdinalNumber; i++){
					RtlZeroMemory(&szExportFunctionName, 512);
					wsprintfA(szExportFunctionName, "Func%d", expExportNumber + 1);
					ExporterAddNewExport(szExportFunctionName, ExportRelativeAddress);
				}
				return(true);
			}else{
				return(true);
			}
		}
	}
	return(false);
}
__declspec(dllexport) long __stdcall ExporterGetAddedExportCount(){
	return(expExportNumber);
}
__declspec(dllexport) long __stdcall ExporterEstimatedSize(){

	DWORD EstimatedSize = NULL;

	EstimatedSize = (DWORD)((ULONG_PTR)expTableDataCWP - (ULONG_PTR)expTableData);
	EstimatedSize = EstimatedSize + (expExportNumber * 12) + sizeof IMAGE_EXPORT_DIRECTORY;
	return(EstimatedSize);
}
__declspec(dllexport) bool __stdcall ExporterBuildExportTable(ULONG_PTR StorePlace, ULONG_PTR FileMapVA){

	unsigned int i = NULL;
	unsigned int j = NULL;
	LPVOID expBuildExportDataOld;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	LPVOID expBuildExportData;
	LPVOID expBuildExportDataCWP;
	DWORD StorePlaceRVA = (DWORD)ConvertFileOffsetToVA(FileMapVA, StorePlace, false);
	DWORD TempDWORD;
	BOOL FileIs64;

	if(expTableDataCWP != NULL){
		expBuildExportData = VirtualAlloc(NULL, ExporterEstimatedSize(), MEM_COMMIT, PAGE_READWRITE);
		expBuildExportDataCWP = (LPVOID)((ULONG_PTR)expBuildExportData + sizeof IMAGE_EXPORT_DIRECTORY);
		
		expExportData.NumberOfNames = expExportNumber;
		expExportData.NumberOfFunctions = expExportNumber;
		for(i = 0; i < expExportNumber; i++){
			for(j = 0; j < expExportNumber; j++){
				if(lstrcmpiA((PCHAR)expNamePointers[i], (PCHAR)expNamePointers[j]) < NULL){
					TempDWORD = expNamePointers[j];
					expNamePointers[j] = expNamePointers[i];
					expNamePointers[i] = TempDWORD;
					TempDWORD = expExportAddress[j];
					expExportAddress[j] = expExportAddress[i];
					expExportAddress[i] = TempDWORD;
				}
			}
		}

		if(expNamePresent){
			expExportData.Name = StorePlaceRVA + (DWORD)((ULONG_PTR)expBuildExportDataCWP - (ULONG_PTR)expBuildExportData);
			RtlMoveMemory(expBuildExportDataCWP, (LPVOID)expTableData, lstrlenA((PCHAR)expTableData));
			expBuildExportDataCWP = (LPVOID)((ULONG_PTR)expBuildExportDataCWP + lstrlenA((PCHAR)expTableData) + 2);
		}
		for(i = 0; i < expExportNumber; i++){
			RtlMoveMemory(expBuildExportDataCWP, (LPVOID)expNamePointers[i], lstrlenA((PCHAR)expNamePointers[i]));
			expBuildExportDataOld = expBuildExportDataCWP;
			expBuildExportDataCWP = (LPVOID)((ULONG_PTR)expBuildExportDataCWP + lstrlenA((PCHAR)expNamePointers[i]) + 2);
			expNamePointers[i] = (DWORD)((ULONG_PTR)expBuildExportDataOld - (ULONG_PTR)expBuildExportData) + StorePlaceRVA;
		}
		expExportData.AddressOfFunctions = StorePlaceRVA + (DWORD)((ULONG_PTR)expBuildExportDataCWP - (ULONG_PTR)expBuildExportData);
		RtlMoveMemory(expBuildExportDataCWP, &expExportAddress, 4 * expExportNumber);
		expBuildExportDataCWP = (LPVOID)((ULONG_PTR)expBuildExportDataCWP + 4 * expExportNumber);
		expExportData.AddressOfNames = StorePlaceRVA + (DWORD)((ULONG_PTR)expBuildExportDataCWP - (ULONG_PTR)expBuildExportData);
		RtlMoveMemory(expBuildExportDataCWP, &expNamePointers, 4 * expExportNumber);
		expBuildExportDataCWP = (LPVOID)((ULONG_PTR)expBuildExportDataCWP + 4 * expExportNumber);
		expExportData.AddressOfNameOrdinals = StorePlaceRVA + (DWORD)((ULONG_PTR)expBuildExportDataCWP - (ULONG_PTR)expBuildExportData);
		RtlMoveMemory(expBuildExportDataCWP, &expOrdinals, 2 * expExportNumber);
		expBuildExportDataCWP = (LPVOID)((ULONG_PTR)expBuildExportDataCWP + 2 * expExportNumber);
		RtlMoveMemory(expBuildExportData, &expExportData, sizeof IMAGE_EXPORT_DIRECTORY);
		__try{
			RtlMoveMemory((LPVOID)StorePlace, expBuildExportData, (DWORD)((ULONG_PTR)expBuildExportDataCWP - (ULONG_PTR)expBuildExportData));
		}__except(EXCEPTION_EXECUTE_HANDLER){
			VirtualFree(expBuildExportData, NULL, MEM_RELEASE);
			ExporterCleanup();
			return(false);
		}

		if(FileMapVA != NULL){
			DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
			if(EngineValidateHeader(FileMapVA, NULL, NULL, DOSHeader, true)){
				PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
				PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
				if(PEHeader32->OptionalHeader.Magic == 0x10B){
					FileIs64 = false;
				}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
					FileIs64 = true;
				}
				if(!FileIs64){
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = (DWORD)StorePlaceRVA;
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = (DWORD)((ULONG_PTR)expBuildExportDataCWP - (ULONG_PTR)expBuildExportData);
				}else{
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = (DWORD)StorePlaceRVA;
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = (DWORD)((ULONG_PTR)expBuildExportDataCWP - (ULONG_PTR)expBuildExportData);
				}
			}
		}
		VirtualFree(expBuildExportData, NULL, MEM_RELEASE);
		ExporterCleanup();
		return(true);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall ExporterBuildExportTableEx(char* szExportFileName, char* szSectionName){

	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	DWORD NewSectionVO = NULL;
	DWORD NewSectionFO = NULL;
	bool ReturnValue = false;
	
	if(ExporterGetAddedExportCount() > NULL){
		NewSectionVO = AddNewSection(szExportFileName, szSectionName, ExporterEstimatedSize());
		if(MapFileEx(szExportFileName, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
			NewSectionFO = (DWORD)ConvertVAtoFileOffset(FileMapVA, NewSectionVO + (ULONG_PTR)GetPE32DataFromMappedFile(FileMapVA, NULL, UE_IMAGEBASE), true);
			ReturnValue = ExporterBuildExportTable(NewSectionFO, FileMapVA);
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			if(ReturnValue){
				return(true);
			}else{
				return(false);
			}
		}else{
			return(false);
		}
	}else{
		return(false);
	}

}
__declspec(dllexport) bool __stdcall ExporterLoadExportTable(char* szFileName){

	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int n = 0;
	unsigned int x = 0;
	bool ExportPresent = false;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_EXPORT_DIRECTORY PEExports;
	PEXPORTED_DATA ExportedFunctions;
	PEXPORTED_DATA ExportedFunctionNames;
	PEXPORTED_DATA_WORD ExportedFunctionOrdinals;
	char* ExportName = NULL;
	BOOL FileIs64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(false);
			}
			if(!FileIs64){
				if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress != NULL){
					PEExports = (PIMAGE_EXPORT_DIRECTORY)(ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + PEHeader32->OptionalHeader.ImageBase), true));
					ExportedFunctions = (PEXPORTED_DATA)(ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)(PEExports->AddressOfFunctions + PEHeader32->OptionalHeader.ImageBase), true));
					ExporterInit(50 * 1024, (ULONG_PTR)PEHeader32->OptionalHeader.ImageBase, PEExports->Base, NULL);
					ExportPresent = true;
				}
			}else{
				if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress != NULL){
					PEExports = (PIMAGE_EXPORT_DIRECTORY)(ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + PEHeader64->OptionalHeader.ImageBase), true));
					ExportedFunctions = (PEXPORTED_DATA)(ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)(PEExports->AddressOfFunctions + PEHeader64->OptionalHeader.ImageBase), true));
					ExporterInit(50 * 1024, (ULONG_PTR)PEHeader64->OptionalHeader.ImageBase, PEExports->Base, NULL);
					ExportPresent = true;
				}
			}
			if(ExportPresent){
				for(n = 0; n <= PEExports->NumberOfNames; n++){
					ExportPresent = false;
					x = n;
					if(!FileIs64){
						ExportedFunctionNames = (PEXPORTED_DATA)(ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)(PEExports->AddressOfNames + PEHeader32->OptionalHeader.ImageBase), true));
						ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)(ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)(PEExports->AddressOfNameOrdinals + PEHeader32->OptionalHeader.ImageBase), true));
					}else{
						ExportedFunctionNames = (PEXPORTED_DATA)(ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)(PEExports->AddressOfNames + PEHeader64->OptionalHeader.ImageBase), true));
						ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)(ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)(PEExports->AddressOfNameOrdinals + PEHeader64->OptionalHeader.ImageBase), true));
					}
					for(j = 0; j <= PEExports->NumberOfNames; j++){
						if(ExportedFunctionOrdinals->OrdinalNumber != x){
							ExportedFunctionOrdinals = (PEXPORTED_DATA_WORD)((ULONG_PTR)ExportedFunctionOrdinals + 2);
						}else{
							ExportPresent = true;
							break;
						}
					}
					if(ExportPresent){
						ExportedFunctionNames = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctionNames + j * 4);
						if(!FileIs64){
							ExportName = (char*)(ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)(ExportedFunctionNames->ExportedItem + PEHeader32->OptionalHeader.ImageBase), true));
						}else{
							ExportName = (char*)(ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)(ExportedFunctionNames->ExportedItem + PEHeader64->OptionalHeader.ImageBase), true));
						}
						ExporterAddNewExport(ExportName, ExportedFunctions->ExportedItem);
					}
					ExportedFunctions = (PEXPORTED_DATA)((ULONG_PTR)ExportedFunctions + 4);
				}
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(true);
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(false);
			}
		}else{
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			return(false);		
		}
	}else{
		return(false);
	}
	UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
	return(false);
}
// UnpackEngine.Librarian.functions:
__declspec(dllexport) bool __stdcall LibrarianSetBreakPoint(char* szLibraryName, DWORD bpxType, bool SingleShoot, LPVOID bpxCallBack){

	int i = MAX_LIBRARY_BPX;
	PLIBRARY_BREAK_DATA ptrLibrarianData = (PLIBRARY_BREAK_DATA)LibrarianData;

	if(szLibraryName != NULL && ptrLibrarianData != NULL){
		while(i > NULL && ptrLibrarianData->szLibraryName[0] != 0x00){
			ptrLibrarianData = (PLIBRARY_BREAK_DATA)((ULONG_PTR)ptrLibrarianData + sizeof LIBRARY_BREAK_DATA);
			i--;
		}
		lstrcpyA(&ptrLibrarianData->szLibraryName[0], szLibraryName);
		ptrLibrarianData->bpxCallBack = bpxCallBack;
		ptrLibrarianData->bpxSingleShoot = SingleShoot;
		ptrLibrarianData->bpxType = bpxType;
		return(true);
	}
	return(false);
}
__declspec(dllexport) bool __stdcall LibrarianRemoveBreakPoint(char* szLibraryName, DWORD bpxType){

	int i = MAX_LIBRARY_BPX;
	PLIBRARY_BREAK_DATA ptrLibrarianData = (PLIBRARY_BREAK_DATA)LibrarianData;

	if(szLibraryName != NULL && ptrLibrarianData != NULL){
		while(i > NULL){
			if(ptrLibrarianData->szLibraryName[0] != 0x00){
				if(lstrcmpiA(szLibraryName, ptrLibrarianData->szLibraryName) == NULL && (ptrLibrarianData->bpxType == bpxType || bpxType == UE_ON_LIB_ALL)){
					RtlZeroMemory(ptrLibrarianData, sizeof LIBRARY_BREAK_DATA);
				}
			}
			ptrLibrarianData = (PLIBRARY_BREAK_DATA)((ULONG_PTR)ptrLibrarianData + sizeof LIBRARY_BREAK_DATA);
			i--;
		}
		return(true);
	}
	return(false);
}
__declspec(dllexport) void* __stdcall LibrarianGetLibraryInfo(char* szLibraryName){

	PLIBRARY_ITEM_DATA hListLibraryPtr = NULL;

	if(hListLibrary != NULL){
		hListLibraryPtr = (PLIBRARY_ITEM_DATA)hListLibrary;
		while(hListLibraryPtr->hFile != NULL){
			if(hListLibraryPtr->hFile != (HANDLE)-1){
				if(lstrcmpiA(hListLibraryPtr->szLibraryName, szLibraryName) == NULL){
					return((void*)hListLibraryPtr);
				}
			}
			hListLibraryPtr = (PLIBRARY_ITEM_DATA)((ULONG_PTR)hListLibraryPtr + sizeof LIBRARY_ITEM_DATA);
		}
	}
	return(NULL);
}
__declspec(dllexport) void* __stdcall LibrarianGetLibraryInfoEx(void* BaseOfDll){

	PLIBRARY_ITEM_DATA hListLibraryPtr = NULL;

	if(hListLibrary != NULL){
		hListLibraryPtr = (PLIBRARY_ITEM_DATA)hListLibrary;
		while(hListLibraryPtr->hFile != NULL){
			if(hListLibraryPtr->hFile != (HANDLE)-1){
				if(hListLibraryPtr->BaseOfDll){
					return((void*)hListLibraryPtr);
				}
			}
			hListLibraryPtr = (PLIBRARY_ITEM_DATA)((ULONG_PTR)hListLibraryPtr + sizeof LIBRARY_ITEM_DATA);
		}
	}
	return(NULL);
}
__declspec(dllexport) void __stdcall LibrarianEnumLibraryInfo(void* EnumCallBack){

	PLIBRARY_ITEM_DATA hListLibraryPtr = NULL;
	typedef void(__stdcall *fEnumCallBack)(LPVOID fLibraryDetail);
	fEnumCallBack myEnumCallBack = (fEnumCallBack)EnumCallBack;

	if(hListLibrary != NULL){
		hListLibraryPtr = (PLIBRARY_ITEM_DATA)hListLibrary;
		while(EnumCallBack != NULL && hListLibraryPtr->hFile != NULL){
			if(hListLibraryPtr->hFile != (HANDLE)-1){
				__try{
					myEnumCallBack((void*)hListLibraryPtr);
				}__except(EXCEPTION_EXECUTE_HANDLER){
					EnumCallBack = NULL;
				}
			}
			hListLibraryPtr = (PLIBRARY_ITEM_DATA)((ULONG_PTR)hListLibraryPtr + sizeof LIBRARY_ITEM_DATA);
		}
	}
}
// UnpackEngine.Process.functions:
__declspec(dllexport) long __stdcall GetActiveProcessId(char* szImageName){

	int i;
	char* szTranslatedProcName;
	DWORD bProcessId[1024] = {};
	char szProcessPath[1024] = {};
	DWORD pProcessIdCount = NULL;
	HANDLE hProcess;

	if(EnumProcesses(bProcessId, sizeof bProcessId, &pProcessIdCount)){
		for(i = 0; i < (int)pProcessIdCount; i++){
			if(bProcessId[i] != NULL){
				hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, false, bProcessId[i]);
				if(hProcess != NULL){
					if(GetProcessImageFileNameA(hProcess, szProcessPath, 1024) > NULL){
						szTranslatedProcName = (char*)TranslateNativeName(szProcessPath);
						lstrcpyA(szProcessPath, szTranslatedProcName);
						VirtualFree((void*)szTranslatedProcName, NULL, MEM_RELEASE);
						EngineCloseHandle(hProcess);
						if(lstrcmpiA(szProcessPath, szImageName) == NULL){
							return(bProcessId[i]);
						}else if(lstrcmpiA(EngineExtractFileName(szProcessPath), szImageName) == NULL){
							return(bProcessId[i]);
						}
					}else{
						EngineCloseHandle(hProcess);
					}
				}
			}
		}
	}
	return(NULL);
}
__declspec(dllexport) void __stdcall EnumProcessesWithLibrary(char* szLibraryName, void* EnumFunction){

	int i;
	int j;
	typedef void(__stdcall *fEnumFunction)(DWORD ProcessId, HMODULE ModuleBaseAddress);
	fEnumFunction myEnumFunction = (fEnumFunction)EnumFunction;
	HMODULE EnumeratedModules[1024] = {};
	DWORD bProcessId[1024] = {};
	char szModuleName[1024] = {};
	DWORD pProcessIdCount = NULL;
	DWORD pModuleCount;
	HANDLE hProcess;

	if(EnumFunction != NULL){
		if(EnumProcesses(bProcessId, sizeof bProcessId, &pProcessIdCount)){
			for(i = 0; i < (int)pProcessIdCount; i++){
				if(bProcessId[i] != NULL){
					hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, false, bProcessId[i]);
					if(hProcess != NULL){
						RtlZeroMemory(&EnumeratedModules[0], sizeof EnumeratedModules);
						if(EnumProcessModules(hProcess, (HMODULE*)EnumeratedModules, sizeof EnumeratedModules, &pModuleCount)){
							for(j = 0; j < (int)pModuleCount; j++){
								if(EnumeratedModules[j] != NULL){
									if(GetModuleBaseNameA(hProcess, EnumeratedModules[j], szModuleName, 1024) > NULL){
										if(lstrcmpiA(szModuleName, szLibraryName) == NULL){
											__try{
												myEnumFunction(bProcessId[i], EnumeratedModules[j]);
											}__except(EXCEPTION_EXECUTE_HANDLER){
												EngineCloseHandle(hProcess);
												return;
											}
										}
									}
								}
							}
						}
						EngineCloseHandle(hProcess);
					}
				}
			}
		}
	}
}
// UnpackEngine.TLSFixer.functions:
__declspec(dllexport) bool __stdcall TLSBreakOnCallBack(LPVOID ArrayOfCallBacks, DWORD NumberOfCallBacks, LPVOID bpxCallBack){

	unsigned int i;
	LPVOID ReadArrayOfCallBacks = ArrayOfCallBacks;

	if(NumberOfCallBacks > NULL){
		for(i = 0; i < NumberOfCallBacks; i++){
			RtlMoveMemory(&tlsCallBackList[i], ReadArrayOfCallBacks, sizeof ULONG_PTR);
			ReadArrayOfCallBacks = (LPVOID)((ULONG_PTR)ReadArrayOfCallBacks + sizeof ULONG_PTR);
		}
		engineTLSBreakOnCallBackAddress = (ULONG_PTR)bpxCallBack;
		engineTLSBreakOnCallBack = true;
		return(true);
	}else{
		return(false);
	}
}
__declspec(dllexport) bool __stdcall TLSGrabCallBackData(char* szFileName, LPVOID ArrayOfCallBacks, LPDWORD NumberOfCallBacks){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	BOOL FileIs64;
	PIMAGE_TLS_DIRECTORY32 TLSDirectoryX86;	
	PIMAGE_TLS_DIRECTORY64 TLSDirectoryX64;
	ULONG_PTR TLSDirectoryAddress;
	ULONG_PTR TLSCallBackAddress;
	ULONG_PTR TLSCompareData = NULL;
	DWORD NumberOfTLSCallBacks = NULL;

	if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(false);
			}
			if(!FileIs64){
				if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress != NULL){
					TLSDirectoryAddress = (ULONG_PTR)((ULONG_PTR)PEHeader32->OptionalHeader.ImageBase + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
					TLSDirectoryX86 = (PIMAGE_TLS_DIRECTORY32)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)TLSDirectoryAddress, true);
					if(TLSDirectoryX86->AddressOfCallBacks != NULL){
						TLSCallBackAddress = (ULONG_PTR)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)TLSDirectoryX86->AddressOfCallBacks, true);
						while(memcmp((LPVOID)TLSCallBackAddress, &TLSCompareData, sizeof ULONG_PTR) != NULL){
							RtlMoveMemory(ArrayOfCallBacks, (LPVOID)TLSCallBackAddress, sizeof ULONG_PTR);
							ArrayOfCallBacks = (LPVOID)((ULONG_PTR)ArrayOfCallBacks + sizeof ULONG_PTR);
							TLSCallBackAddress = TLSCallBackAddress + sizeof ULONG_PTR;
							NumberOfTLSCallBacks++;
						}
						*NumberOfCallBacks = NumberOfTLSCallBacks;
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(true);
					}else{
						*NumberOfCallBacks = NULL;
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(false);
					}
				}else{
					*NumberOfCallBacks = NULL;
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);	
				}
			}else{
				if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress != NULL){
					TLSDirectoryAddress = (ULONG_PTR)((ULONG_PTR)PEHeader64->OptionalHeader.ImageBase + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
					TLSDirectoryX64 = (PIMAGE_TLS_DIRECTORY64)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)TLSDirectoryAddress, true);
					if(TLSDirectoryX64->AddressOfCallBacks != NULL){
						TLSCallBackAddress = (ULONG_PTR)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)TLSDirectoryX64->AddressOfCallBacks, true);
						while(memcmp((LPVOID)TLSCallBackAddress, &TLSCompareData, sizeof ULONG_PTR) != NULL){
							RtlMoveMemory(ArrayOfCallBacks, (LPVOID)TLSCallBackAddress, sizeof ULONG_PTR);
							ArrayOfCallBacks = (LPVOID)((ULONG_PTR)ArrayOfCallBacks + sizeof ULONG_PTR);
							TLSCallBackAddress = TLSCallBackAddress + sizeof ULONG_PTR;
							NumberOfTLSCallBacks++;
						}
						*NumberOfCallBacks = NumberOfTLSCallBacks;
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(true);
					}else{
						*NumberOfCallBacks = NULL;
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(false);
					}
				}else{
					*NumberOfCallBacks = NULL;
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);	
				}
			}
		}else{
			*NumberOfCallBacks = NULL;
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			return(false);		
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall TLSBreakOnCallBackEx(char* szFileName, LPVOID bpxCallBack){

	ULONG_PTR TlsArrayOfCallBacks[100];
	DWORD TlsNumberOfCallBacks;

	RtlZeroMemory(&TlsArrayOfCallBacks, 100 * sizeof ULONG_PTR);
	if(szFileName != NULL){
		if(TLSGrabCallBackData(szFileName, &TlsArrayOfCallBacks, &TlsNumberOfCallBacks)){
			TLSBreakOnCallBack(&TlsArrayOfCallBacks, TlsNumberOfCallBacks, bpxCallBack);
			return(true);
		}else{
			return(false);
		}
	}else{
		return(false);
	}
}
__declspec(dllexport) bool __stdcall TLSRemoveCallback(char* szFileName){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	BOOL FileIs64;
	PIMAGE_TLS_DIRECTORY32 TLSDirectoryX86;	
	PIMAGE_TLS_DIRECTORY64 TLSDirectoryX64;
	ULONG_PTR TLSDirectoryAddress;

	if(MapFileEx(szFileName, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(false);
			}
			if(!FileIs64){
				if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress != NULL){
					__try{
						TLSDirectoryAddress = (ULONG_PTR)((ULONG_PTR)PEHeader32->OptionalHeader.ImageBase + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
						TLSDirectoryX86 = (PIMAGE_TLS_DIRECTORY32)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)TLSDirectoryAddress, true);
						if(TLSDirectoryX86->AddressOfCallBacks != NULL){
							TLSDirectoryX86->AddressOfCallBacks = NULL;
							UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
							return(true);
						}else{
							UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
							return(false);
						}
					}__except(EXCEPTION_EXECUTE_HANDLER){
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(false);
					}
				}else{
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);	
				}
			}else{
				if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress != NULL){
					__try{
						TLSDirectoryAddress = (ULONG_PTR)((ULONG_PTR)PEHeader64->OptionalHeader.ImageBase + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
						TLSDirectoryX64 = (PIMAGE_TLS_DIRECTORY64)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)TLSDirectoryAddress, true);
						if(TLSDirectoryX64->AddressOfCallBacks != NULL){
							TLSDirectoryX64->AddressOfCallBacks = NULL;
							UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
							return(true);
						}else{
							UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
							return(false);
						}
					}__except(EXCEPTION_EXECUTE_HANDLER){
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(false);
					}
				}else{
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);	
				}
			}
		}else{
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			return(false);		
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall TLSRemoveTable(char* szFileName){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	BOOL FileIs64;
	PIMAGE_TLS_DIRECTORY32 TLSDirectoryX86;	
	PIMAGE_TLS_DIRECTORY64 TLSDirectoryX64;
	ULONG_PTR TLSDirectoryAddress;

	if(MapFileEx(szFileName, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(false);
			}
			if(!FileIs64){
				if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress != NULL){
					__try{
						TLSDirectoryAddress = (ULONG_PTR)((ULONG_PTR)PEHeader32->OptionalHeader.ImageBase + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
						TLSDirectoryX86 = (PIMAGE_TLS_DIRECTORY32)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)TLSDirectoryAddress, true);
						PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = NULL;
						PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = NULL;
						RtlZeroMemory(TLSDirectoryX86, sizeof IMAGE_TLS_DIRECTORY32);
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(true);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(false);
					}
				}else{
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);	
				}
			}else{
				if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress != NULL){
					__try{
						TLSDirectoryAddress = (ULONG_PTR)((ULONG_PTR)PEHeader64->OptionalHeader.ImageBase + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
						TLSDirectoryX64 = (PIMAGE_TLS_DIRECTORY64)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)TLSDirectoryAddress, true);
						PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = NULL;
						PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = NULL;
						RtlZeroMemory(TLSDirectoryX64, sizeof IMAGE_TLS_DIRECTORY64);
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(true);
					}__except(EXCEPTION_EXECUTE_HANDLER){
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(false);
					}
				}else{
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);	
				}
			}
		}else{
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			return(false);		
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall TLSBackupData(char* szFileName){

	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	BOOL FileIs64;
	PIMAGE_TLS_DIRECTORY32 TLSDirectoryX86;	
	PIMAGE_TLS_DIRECTORY64 TLSDirectoryX64;
	ULONG_PTR TLSDirectoryAddress;
	ULONG_PTR TLSCallBackAddress;
	ULONG_PTR TLSCompareData = NULL;
	DWORD NumberOfTLSCallBacks = NULL;
	LPVOID ArrayOfCallBacks = &engineBackupArrayOfCallBacks;
	LPDWORD NumberOfCallBacks = &engineBackupNumberOfCallBacks;

	engineBackupTLSAddress = NULL;
	RtlZeroMemory(engineBackupArrayOfCallBacks, 0x1000);
	RtlZeroMemory(&engineBackupTLSDataX86, sizeof IMAGE_TLS_DIRECTORY32);
	RtlZeroMemory(&engineBackupTLSDataX64, sizeof IMAGE_TLS_DIRECTORY64);
	if(MapFileEx(szFileName, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, FileHandle, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
				return(false);
			}
			if(!FileIs64){
				if(PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress != NULL){
					__try{
						engineBackupTLSx64 = false;
						engineBackupTLSAddress = PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
						TLSDirectoryAddress = (ULONG_PTR)((ULONG_PTR)PEHeader32->OptionalHeader.ImageBase + PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
						TLSDirectoryX86 = (PIMAGE_TLS_DIRECTORY32)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)TLSDirectoryAddress, true);
						RtlMoveMemory(&engineBackupTLSDataX86, (LPVOID)TLSDirectoryX86, sizeof IMAGE_TLS_DIRECTORY32);
						if(TLSDirectoryX86->AddressOfCallBacks != NULL){
							TLSCallBackAddress = (ULONG_PTR)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)TLSDirectoryX86->AddressOfCallBacks, true);
							while(memcmp((LPVOID)TLSCallBackAddress, &TLSCompareData, sizeof ULONG_PTR) != NULL){
								RtlMoveMemory(ArrayOfCallBacks, (LPVOID)TLSCallBackAddress, sizeof ULONG_PTR);
								ArrayOfCallBacks = (LPVOID)((ULONG_PTR)ArrayOfCallBacks + sizeof ULONG_PTR);
								TLSCallBackAddress = TLSCallBackAddress + sizeof ULONG_PTR;
								NumberOfTLSCallBacks++;
							}
							*NumberOfCallBacks = NumberOfTLSCallBacks;
							UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
							return(true);
						}else{
							*NumberOfCallBacks = NULL;
							UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
							return(false);
						}
					}__except(EXCEPTION_EXECUTE_HANDLER){
						*NumberOfCallBacks = NULL;
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(false);
					}
				}else{
					*NumberOfCallBacks = NULL;
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);	
				}
			}else{
				if(PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress != NULL){
					__try{
						engineBackupTLSx64 = true;
						engineBackupTLSAddress = PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
						TLSDirectoryAddress = (ULONG_PTR)((ULONG_PTR)PEHeader64->OptionalHeader.ImageBase + PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
						TLSDirectoryX64 = (PIMAGE_TLS_DIRECTORY64)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)TLSDirectoryAddress, true);
						RtlMoveMemory(&engineBackupTLSDataX64, (LPVOID)TLSDirectoryX64, sizeof IMAGE_TLS_DIRECTORY64);
						if(TLSDirectoryX64->AddressOfCallBacks != NULL){
							TLSCallBackAddress = (ULONG_PTR)ConvertVAtoFileOffset(FileMapVA, (ULONG_PTR)TLSDirectoryX64->AddressOfCallBacks, true);
							while(memcmp((LPVOID)TLSCallBackAddress, &TLSCompareData, sizeof ULONG_PTR) != NULL){
								RtlMoveMemory(ArrayOfCallBacks, (LPVOID)TLSCallBackAddress, sizeof ULONG_PTR);
								ArrayOfCallBacks = (LPVOID)((ULONG_PTR)ArrayOfCallBacks + sizeof ULONG_PTR);
								TLSCallBackAddress = TLSCallBackAddress + sizeof ULONG_PTR;
								NumberOfTLSCallBacks++;
							}
							*NumberOfCallBacks = NumberOfTLSCallBacks;
							UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
							return(true);
						}else{
							*NumberOfCallBacks = NULL;
							UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
							return(false);
						}
					}__except(EXCEPTION_EXECUTE_HANDLER){
						*NumberOfCallBacks = NULL;
						UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
						return(false);
					}
				}else{
					*NumberOfCallBacks = NULL;
					UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
					return(false);	
				}
			}
		}else{
			*NumberOfCallBacks = NULL;
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			return(false);		
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall TLSRestoreData(){

	ULONG_PTR ueNumberOfBytesRead = NULL;

	if(dbgProcessInformation.hProcess != NULL && engineBackupTLSAddress != NULL){
		if(engineBackupTLSx64){
			if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)(engineBackupTLSAddress + GetDebuggedFileBaseAddress()), &engineBackupTLSDataX64, sizeof IMAGE_TLS_DIRECTORY64, &ueNumberOfBytesRead)){
				if(engineBackupTLSDataX64.AddressOfCallBacks != NULL && engineBackupNumberOfCallBacks != NULL){
					if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)(engineBackupTLSDataX64.AddressOfCallBacks + GetDebuggedFileBaseAddress()), engineBackupArrayOfCallBacks, sizeof IMAGE_TLS_DIRECTORY64, &ueNumberOfBytesRead)){
						engineBackupTLSAddress = NULL;
						return(true);
					}
				}else{
					engineBackupTLSAddress = NULL;
					return(true);
				}
			}
		}else{
			if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)(engineBackupTLSAddress + GetDebuggedFileBaseAddress()), &engineBackupTLSDataX86, sizeof IMAGE_TLS_DIRECTORY32, &ueNumberOfBytesRead)){
				if(engineBackupTLSDataX86.AddressOfCallBacks != NULL && engineBackupNumberOfCallBacks != NULL){
					if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)(engineBackupTLSDataX86.AddressOfCallBacks + GetDebuggedFileBaseAddress()), engineBackupArrayOfCallBacks, sizeof IMAGE_TLS_DIRECTORY32, &ueNumberOfBytesRead)){
						engineBackupTLSAddress = NULL;
						return(true);
					}
				}else{
					engineBackupTLSAddress = NULL;
					return(true);
				}
			}
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall TLSBuildNewTable(ULONG_PTR FileMapVA, ULONG_PTR StorePlace, ULONG_PTR StorePlaceRVA, LPVOID ArrayOfCallBacks, DWORD NumberOfCallBacks){

	BOOL FileIs64;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_TLS_DIRECTORY32 TLSDirectoryX86;	
	PIMAGE_TLS_DIRECTORY64 TLSDirectoryX64;
	ULONG_PTR TLSWriteData = StorePlaceRVA;

	if(FileMapVA != NULL){
		DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
		if(EngineValidateHeader(FileMapVA, NULL, NULL, DOSHeader, true)){
			PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
			if(PEHeader32->OptionalHeader.Magic == 0x10B){
				FileIs64 = false;
			}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
				FileIs64 = true;
			}else{
				return(false);
			}
			if(!FileIs64){
				__try{
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = (DWORD)StorePlaceRVA;
					PEHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = sizeof IMAGE_TLS_DIRECTORY32;
					TLSDirectoryX86 = (PIMAGE_TLS_DIRECTORY32)StorePlace;
					TLSDirectoryX86->StartAddressOfRawData = (DWORD)TLSWriteData;
					TLSDirectoryX86->EndAddressOfRawData = (DWORD)TLSWriteData + 0x10;
					TLSDirectoryX86->AddressOfIndex = (DWORD)TLSWriteData + 0x14;
					TLSDirectoryX86->AddressOfCallBacks = (DWORD)TLSWriteData  + sizeof IMAGE_TLS_DIRECTORY32 + 8; 
					RtlMoveMemory((LPVOID)(StorePlace + sizeof IMAGE_TLS_DIRECTORY32 + 8), ArrayOfCallBacks, NumberOfCallBacks * 4);
					return(true);	
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(false);
				}
			}else{
				__try{
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = (DWORD)StorePlaceRVA;
					PEHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = sizeof IMAGE_TLS_DIRECTORY64;
					TLSDirectoryX64 = (PIMAGE_TLS_DIRECTORY64)StorePlace;
					TLSDirectoryX64->StartAddressOfRawData = TLSWriteData;
					TLSDirectoryX64->EndAddressOfRawData = TLSWriteData + 0x20;
					TLSDirectoryX64->AddressOfIndex = TLSWriteData + 0x28;
					TLSDirectoryX64->AddressOfCallBacks = TLSWriteData  + sizeof IMAGE_TLS_DIRECTORY64 + 12; 
					RtlMoveMemory((LPVOID)(StorePlace + sizeof IMAGE_TLS_DIRECTORY64 + 12), ArrayOfCallBacks, NumberOfCallBacks * 8);
					return(true);	
				}__except(EXCEPTION_EXECUTE_HANDLER){
					return(false);
				}
			}
		}else{
			return(false);		
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall TLSBuildNewTableEx(char* szFileName, char* szSectionName, LPVOID ArrayOfCallBacks, DWORD NumberOfCallBacks){

	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;
	DWORD NewSectionVO = NULL;
	DWORD NewSectionFO = NULL;
	bool ReturnValue = false;
	ULONG_PTR tlsImageBase;
	
	tlsImageBase = (ULONG_PTR)GetPE32Data(szFileName, NULL, UE_IMAGEBASE);
	NewSectionVO = AddNewSection(szFileName, szSectionName, ImporterEstimatedSize());
	if(MapFileEx(szExportFileName, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
		NewSectionFO = (DWORD)ConvertVAtoFileOffset(FileMapVA, NewSectionVO + tlsImageBase, true);
		ReturnValue = TLSBuildNewTable(FileMapVA, NewSectionFO, NewSectionVO, ArrayOfCallBacks, NumberOfCallBacks);
		UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
		if(ReturnValue){
			return(true);
		}else{
			return(false);
		}
	}else{
		return(false);
	}
}
// UnpackEngine.TranslateName.functions:
__declspec(dllexport) void* __stdcall TranslateNativeName(char* szNativeName){

	LPVOID TranslatedName = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	char szDeviceName[3] = "A:";
	char szDeviceCOMName[5] = "COM0";
	int CurrentDeviceLen;

	while(szDeviceName[0] <= 0x5A){
		RtlZeroMemory(TranslatedName, 0x1000);
		if(QueryDosDeviceA(szDeviceName, (LPSTR)TranslatedName, 0x1000) > NULL){
			CurrentDeviceLen = lstrlenA((LPSTR)TranslatedName);
			lstrcatA((LPSTR)TranslatedName, (LPCSTR)(szNativeName + CurrentDeviceLen));
			if(lstrcmpiA((LPCSTR)TranslatedName, szNativeName) == NULL){
				RtlZeroMemory(TranslatedName, 0x1000);
				lstrcatA((LPSTR)TranslatedName, szDeviceName);
				lstrcatA((LPSTR)TranslatedName, (LPCSTR)(szNativeName + CurrentDeviceLen));
				return(TranslatedName);
			}
		}
		szDeviceName[0]++;
	}
	while(szDeviceCOMName[3] <= 0x39){
		RtlZeroMemory(TranslatedName, 0x1000);
		if(QueryDosDeviceA(szDeviceCOMName, (LPSTR)TranslatedName, 0x1000) > NULL){
			CurrentDeviceLen = lstrlenA((LPSTR)TranslatedName);
			lstrcatA((LPSTR)TranslatedName, (LPCSTR)(szNativeName + CurrentDeviceLen));
			if(lstrcmpiA((LPCSTR)TranslatedName, szNativeName) == NULL){
				RtlZeroMemory(TranslatedName, 0x1000);
				lstrcatA((LPSTR)TranslatedName, szDeviceCOMName);
				lstrcatA((LPSTR)TranslatedName, (LPCSTR)(szNativeName + CurrentDeviceLen));
				return(TranslatedName);
			}
		}
		szDeviceCOMName[3]++;
	}
	VirtualFree(TranslatedName, NULL, MEM_RELEASE);
	return(NULL);
}
// UnpackEngine.Handler.functions:
__declspec(dllexport) long __stdcall HandlerGetActiveHandleCount(DWORD ProcessId){

	int HandleCount = NULL;
	LPVOID QuerySystemBuffer;
	ULONG QuerySystemBufferSize = 0x2000;
	ULONG RequiredSize = NULL;
	ULONG TotalHandleCount = NULL;
#if !defined(_WIN64)
	typedef NTSTATUS(__stdcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__stdcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#else
	typedef NTSTATUS(__fastcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__fastcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#endif
	LPVOID ZwQuerySystemInformation = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQuerySystemInformation");
	LPVOID ZwQueryObject = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQueryObject");
	fZwQuerySystemInformation cZwQuerySystemInformation = (fZwQuerySystemInformation)(ZwQuerySystemInformation);
	fZwQueryObject cZwQueryObject = (fZwQueryObject)(ZwQueryObject);
	PNTDLL_QUERY_HANDLE_INFO HandleInfo;

	if(ZwQuerySystemInformation != NULL && ZwQueryObject != NULL){
		QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		while(cZwQuerySystemInformation(NTDLL_SystemHandleInfo, QuerySystemBuffer, QuerySystemBufferSize, &RequiredSize) == (NTSTATUS)0xC0000004L){
			QuerySystemBufferSize = RequiredSize;
			VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
			QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		}
		RtlMoveMemory(&TotalHandleCount, QuerySystemBuffer, sizeof ULONG);
		QuerySystemBuffer = (LPVOID)((ULONG_PTR)QuerySystemBuffer + 4);
		HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)QuerySystemBuffer;
		while(TotalHandleCount > NULL){
			if(HandleInfo->ProcessId == ProcessId){
				HandleCount++;
			}
			HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)((ULONG_PTR)HandleInfo + sizeof NTDLL_QUERY_HANDLE_INFO);
			TotalHandleCount--;
		}
		VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
		return(HandleCount);
	}
	return(NULL);
}
__declspec(dllexport) bool __stdcall HandlerIsHandleOpen(DWORD ProcessId, HANDLE hHandle){

	bool HandleActive = false;
	LPVOID QuerySystemBuffer;
	ULONG QuerySystemBufferSize = 0x2000;
	ULONG RequiredSize = NULL;
	ULONG TotalHandleCount = NULL;
#if !defined(_WIN64)
	typedef NTSTATUS(__stdcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#else
	typedef NTSTATUS(__fastcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#endif
	LPVOID ZwQuerySystemInformation = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQuerySystemInformation");
	fZwQuerySystemInformation cZwQuerySystemInformation = (fZwQuerySystemInformation)(ZwQuerySystemInformation);
	PNTDLL_QUERY_HANDLE_INFO HandleInfo;

	if(ZwQuerySystemInformation != NULL){
		QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		while(cZwQuerySystemInformation(NTDLL_SystemHandleInfo, QuerySystemBuffer, QuerySystemBufferSize, &RequiredSize) == (NTSTATUS)0xC0000004L){
			QuerySystemBufferSize = RequiredSize;
			VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
			QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		}
		RtlMoveMemory(&TotalHandleCount, QuerySystemBuffer, sizeof ULONG);
		QuerySystemBuffer = (LPVOID)((ULONG_PTR)QuerySystemBuffer + 4);
		HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)QuerySystemBuffer;
		while(TotalHandleCount > NULL){
			if(HandleInfo->ProcessId == ProcessId && (HANDLE)HandleInfo->hHandle == hHandle){
				HandleActive = true;
				break;
			}
			HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)((ULONG_PTR)HandleInfo + sizeof NTDLL_QUERY_HANDLE_INFO);
			TotalHandleCount--;
		}
		VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
		if(HandleActive){
			return(true);
		}
	}
	return(false);
}
__declspec(dllexport) void* __stdcall HandlerGetHandleName(HANDLE hProcess, DWORD ProcessId, HANDLE hHandle, bool TranslateName){

	bool NameFound = false;
	HANDLE myHandle = NULL;
	LPVOID QuerySystemBuffer;
	ULONG QuerySystemBufferSize = 0x2000;
	ULONG RequiredSize = NULL;
	ULONG TotalHandleCount = NULL;
#if !defined(_WIN64)
	typedef NTSTATUS(__stdcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__stdcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#else
	typedef NTSTATUS(__fastcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__fastcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#endif
	LPVOID ZwQuerySystemInformation = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQuerySystemInformation");
	LPVOID ZwQueryObject = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQueryObject");
	fZwQuerySystemInformation cZwQuerySystemInformation = (fZwQuerySystemInformation)(ZwQuerySystemInformation);
	fZwQueryObject cZwQueryObject = (fZwQueryObject)(ZwQueryObject);
	PNTDLL_QUERY_HANDLE_INFO HandleInfo;
	PUBLIC_OBJECT_BASIC_INFORMATION ObjectBasicInfo;
	LPVOID ObjectNameInfo = VirtualAlloc(NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE);
	PPUBLIC_OBJECT_NAME_INFORMATION pObjectNameInfo = (PPUBLIC_OBJECT_NAME_INFORMATION)ObjectNameInfo;
	LPVOID HandleFullName = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	LPVOID tmpHandleFullName = NULL;

	if(ZwQuerySystemInformation != NULL && ZwQueryObject != NULL){
		QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		while(cZwQuerySystemInformation(NTDLL_SystemHandleInfo, QuerySystemBuffer, QuerySystemBufferSize, &RequiredSize) == (NTSTATUS)0xC0000004L){
			QuerySystemBufferSize = RequiredSize;
			VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
			QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		}
		RtlMoveMemory(&TotalHandleCount, QuerySystemBuffer, sizeof ULONG);
		QuerySystemBuffer = (LPVOID)((ULONG_PTR)QuerySystemBuffer + 4);
		HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)QuerySystemBuffer;
		while(TotalHandleCount > NULL){
			if(HandleInfo->ProcessId == ProcessId && (HANDLE)HandleInfo->hHandle == hHandle){
				if(!(HandleInfo->GrantedAccess & SYNCHRONIZE) || ((HandleInfo->GrantedAccess & SYNCHRONIZE) && ((WORD)HandleInfo->GrantedAccess != 0x19F && (WORD)HandleInfo->GrantedAccess != 0x89))){
					if(DuplicateHandle(hProcess, hHandle, GetCurrentProcess(), &myHandle, NULL, false, DUPLICATE_SAME_ACCESS)){
						RtlZeroMemory(&ObjectBasicInfo, sizeof PUBLIC_OBJECT_BASIC_INFORMATION);
						cZwQueryObject(myHandle, ObjectBasicInformation, &ObjectBasicInfo, sizeof PUBLIC_OBJECT_BASIC_INFORMATION, &RequiredSize);
						cZwQueryObject(myHandle, ObjectNameInformation, ObjectNameInfo, 8, &RequiredSize);
						cZwQueryObject(myHandle, ObjectNameInformation, ObjectNameInfo, RequiredSize, &RequiredSize);
						RtlZeroMemory(HandleFullName, 0x1000);
						if(pObjectNameInfo->Name.Length != NULL){
							WideCharToMultiByte(CP_ACP, NULL, (LPCWSTR)pObjectNameInfo->Name.Buffer, -1, (LPSTR)HandleFullName, 0x1000, NULL, NULL);
							NameFound = true;
							if(TranslateName){
								tmpHandleFullName = TranslateNativeName((char*)HandleFullName);
								if(tmpHandleFullName != NULL){
									VirtualFree(HandleFullName, NULL, MEM_RELEASE);
									HandleFullName = tmpHandleFullName;
								}
							}
						}
						EngineCloseHandle(myHandle);
						break;
					}
				}
			}
			HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)((ULONG_PTR)HandleInfo + sizeof NTDLL_QUERY_HANDLE_INFO);
			TotalHandleCount--;
		}
		VirtualFree(ObjectNameInfo, NULL, MEM_RELEASE);
		VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
		if(!NameFound){
			VirtualFree(HandleFullName, NULL, MEM_RELEASE);
			return(NULL);
		}else{
			return(HandleFullName);
		}
	}
	VirtualFree(HandleFullName, NULL, MEM_RELEASE);
	VirtualFree(ObjectNameInfo, NULL, MEM_RELEASE);
	return(NULL);
}
__declspec(dllexport) long __stdcall HandlerEnumerateOpenHandles(DWORD ProcessId, LPVOID HandleBuffer, DWORD MaxHandleCount){

	HANDLE myHandle = NULL;
	LPVOID QuerySystemBuffer;
	ULONG RequiredSize = NULL;
	ULONG TotalHandleCount = NULL;
	unsigned int HandleCount = NULL;
	ULONG QuerySystemBufferSize = 0x2000;
#if !defined(_WIN64)
	typedef NTSTATUS(__stdcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#else
	typedef NTSTATUS(__fastcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#endif
	LPVOID ZwQuerySystemInformation = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQuerySystemInformation");
	fZwQuerySystemInformation cZwQuerySystemInformation = (fZwQuerySystemInformation)(ZwQuerySystemInformation);
	PNTDLL_QUERY_HANDLE_INFO HandleInfo;

	if(ZwQuerySystemInformation != NULL){
		QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		while(cZwQuerySystemInformation(NTDLL_SystemHandleInfo, QuerySystemBuffer, QuerySystemBufferSize, &RequiredSize) == (NTSTATUS)0xC0000004L){
			QuerySystemBufferSize = RequiredSize;
			VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
			QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		}
		RtlMoveMemory(&TotalHandleCount, QuerySystemBuffer, sizeof ULONG);
		QuerySystemBuffer = (LPVOID)((ULONG_PTR)QuerySystemBuffer + 4);
		HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)QuerySystemBuffer;
		while(TotalHandleCount > NULL){
			if(HandleInfo->ProcessId == ProcessId && HandleCount < MaxHandleCount){
				myHandle = (HANDLE)HandleInfo->hHandle;
				RtlMoveMemory(HandleBuffer, &myHandle, sizeof HANDLE);
				HandleBuffer = (LPVOID)((ULONG_PTR)HandleBuffer + sizeof HANDLE);
				HandleCount++;
			}
			HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)((ULONG_PTR)HandleInfo + sizeof NTDLL_QUERY_HANDLE_INFO);
			TotalHandleCount--;
		}
		VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
		return(HandleCount);
	}
	return(NULL);
}
__declspec(dllexport) long long __stdcall HandlerGetHandleDetails(HANDLE hProcess, DWORD ProcessId, HANDLE hHandle, DWORD InformationReturn){

	ULONG_PTR ReturnData;
	HANDLE myHandle = NULL;
	LPVOID QuerySystemBuffer;
	ULONG QuerySystemBufferSize = 0x2000;
	ULONG RequiredSize = NULL;
	ULONG TotalHandleCount = NULL;
#if !defined(_WIN64)
	typedef NTSTATUS(__stdcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__stdcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#else
	typedef NTSTATUS(__fastcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__fastcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#endif
	LPVOID ZwQuerySystemInformation = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQuerySystemInformation");
	LPVOID ZwQueryObject = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQueryObject");
	fZwQuerySystemInformation cZwQuerySystemInformation = (fZwQuerySystemInformation)(ZwQuerySystemInformation);
	fZwQueryObject cZwQueryObject = (fZwQueryObject)(ZwQueryObject);
	PNTDLL_QUERY_HANDLE_INFO HandleInfo;
	PUBLIC_OBJECT_BASIC_INFORMATION ObjectBasicInfo;
	LPVOID HandleFullData = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	LPVOID HandleNameData = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	PPUBLIC_OBJECT_TYPE_INFORMATION pObjectTypeInfo = (PPUBLIC_OBJECT_TYPE_INFORMATION)HandleFullData;

	if(ZwQuerySystemInformation != NULL && ZwQueryObject != NULL){
		QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		while(cZwQuerySystemInformation(NTDLL_SystemHandleInfo, QuerySystemBuffer, QuerySystemBufferSize, &RequiredSize) == (NTSTATUS)0xC0000004L){
			QuerySystemBufferSize = RequiredSize;
			VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
			QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		}
		RtlMoveMemory(&TotalHandleCount, QuerySystemBuffer, sizeof ULONG);
		QuerySystemBuffer = (LPVOID)((ULONG_PTR)QuerySystemBuffer + 4);
		HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)QuerySystemBuffer;
		while(TotalHandleCount > NULL){
			if(HandleInfo->ProcessId == ProcessId && (HANDLE)HandleInfo->hHandle == hHandle){
				if(DuplicateHandle(hProcess, hHandle, GetCurrentProcess(), &myHandle, NULL, false, DUPLICATE_SAME_ACCESS)){
					RtlZeroMemory(&ObjectBasicInfo, sizeof PUBLIC_OBJECT_BASIC_INFORMATION);
					cZwQueryObject(myHandle, ObjectBasicInformation, &ObjectBasicInfo, sizeof PUBLIC_OBJECT_BASIC_INFORMATION, &RequiredSize);
					if(InformationReturn == UE_OPTION_HANDLER_RETURN_HANDLECOUNT){
						ReturnData = (ULONG_PTR)ObjectBasicInfo.HandleCount;
					}else if(InformationReturn == UE_OPTION_HANDLER_RETURN_ACCESS){
						ReturnData = (ULONG_PTR)HandleInfo->GrantedAccess;
					}else if(InformationReturn == UE_OPTION_HANDLER_RETURN_FLAGS){
						ReturnData = (ULONG_PTR)HandleInfo->Flags;
					}else if(InformationReturn == UE_OPTION_HANDLER_RETURN_TYPENAME){
						if(!(HandleInfo->GrantedAccess & SYNCHRONIZE) || ((HandleInfo->GrantedAccess & SYNCHRONIZE) && ((WORD)HandleInfo->GrantedAccess != 0x19F && (WORD)HandleInfo->GrantedAccess != 0x89))){
							RtlZeroMemory(HandleFullData, 0x1000);
							cZwQueryObject(myHandle, ObjectTypeInformation, HandleFullData, 8, &RequiredSize);
							cZwQueryObject(myHandle, ObjectTypeInformation, HandleFullData, RequiredSize, &RequiredSize);
							RtlZeroMemory(HandleNameData, 0x1000);
							if(pObjectTypeInfo->Name.Length != NULL){
								WideCharToMultiByte(CP_ACP, NULL, (LPCWSTR)pObjectTypeInfo->Name.Buffer, -1, (LPSTR)HandleNameData, 0x1000, NULL, NULL);
								ReturnData = (ULONG_PTR)HandleFullData;
							}
						}
					}
					EngineCloseHandle(myHandle);
					break;
				}
			}
			HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)((ULONG_PTR)HandleInfo + sizeof NTDLL_QUERY_HANDLE_INFO);
			TotalHandleCount--;
		}
		VirtualFree(HandleNameData, NULL, MEM_RELEASE);
		VirtualFree(HandleFullData, NULL, MEM_RELEASE);
		VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
		return(ReturnData);
	}
	VirtualFree(HandleNameData, NULL, MEM_RELEASE);
	VirtualFree(HandleFullData, NULL, MEM_RELEASE);
	return(NULL);
}
__declspec(dllexport) bool __stdcall HandlerCloseRemoteHandle(HANDLE hProcess, HANDLE hHandle){

	HANDLE myHandle;

	if(hProcess != NULL){
		DuplicateHandle(hProcess, hHandle, GetCurrentProcess(), &myHandle, NULL, false, DUPLICATE_CLOSE_SOURCE);
		EngineCloseHandle(myHandle);
	}
	return(false);
}
__declspec(dllexport) long __stdcall HandlerEnumerateLockHandles(char* szFileOrFolderName, bool NameIsFolder, bool NameIsTranslated, LPVOID HandleDataBuffer, DWORD MaxHandleCount){

	int FoundHandles = NULL;
	HANDLE hProcess = NULL;
	HANDLE myHandle = NULL;
	HANDLE CopyHandle = NULL;
	LPVOID QuerySystemBuffer;
	ULONG QuerySystemBufferSize = 0x2000;
	ULONG RequiredSize = NULL;
	ULONG TotalHandleCount = NULL;
	DWORD LastProcessId = NULL;
#if !defined(_WIN64)
	typedef NTSTATUS(__stdcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__stdcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#else
	typedef NTSTATUS(__fastcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__fastcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#endif
	LPVOID ZwQuerySystemInformation = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQuerySystemInformation");
	LPVOID ZwQueryObject = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQueryObject");
	fZwQuerySystemInformation cZwQuerySystemInformation = (fZwQuerySystemInformation)(ZwQuerySystemInformation);
	fZwQueryObject cZwQueryObject = (fZwQueryObject)(ZwQueryObject);
	PNTDLL_QUERY_HANDLE_INFO HandleInfo;
	PUBLIC_OBJECT_BASIC_INFORMATION ObjectBasicInfo;
	LPVOID ObjectNameInfo = VirtualAlloc(NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE);
	PPUBLIC_OBJECT_NAME_INFORMATION pObjectNameInfo = (PPUBLIC_OBJECT_NAME_INFORMATION)ObjectNameInfo;
	LPVOID HandleFullName = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	int LenFileOrFolderName = lstrlenA(szFileOrFolderName);
	LPVOID tmpHandleFullName = NULL;

	if(ZwQuerySystemInformation != NULL && ZwQueryObject != NULL){
		QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		while(cZwQuerySystemInformation(NTDLL_SystemHandleInfo, QuerySystemBuffer, QuerySystemBufferSize, &RequiredSize) == (NTSTATUS)0xC0000004L){
			QuerySystemBufferSize = RequiredSize;
			VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
			QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		}
		RtlMoveMemory(&TotalHandleCount, QuerySystemBuffer, sizeof ULONG);
		QuerySystemBuffer = (LPVOID)((ULONG_PTR)QuerySystemBuffer + 4);
		HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)QuerySystemBuffer;
		while(TotalHandleCount > NULL){
			if(LastProcessId != HandleInfo->ProcessId){
				if(hProcess != NULL){
					EngineCloseHandle(hProcess);
				}
				hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE, false, HandleInfo->ProcessId);
				LastProcessId = HandleInfo->ProcessId;
			}
			if(hProcess != NULL){
				if(!(HandleInfo->GrantedAccess & SYNCHRONIZE) || ((HandleInfo->GrantedAccess & SYNCHRONIZE) && ((WORD)HandleInfo->GrantedAccess != 0x19F && (WORD)HandleInfo->GrantedAccess != 0x89))){
					if(DuplicateHandle(hProcess, (HANDLE)HandleInfo->hHandle, GetCurrentProcess(), &myHandle, NULL, false, DUPLICATE_SAME_ACCESS)){
						RtlZeroMemory(&ObjectBasicInfo, sizeof PUBLIC_OBJECT_BASIC_INFORMATION);
						cZwQueryObject(myHandle, ObjectBasicInformation, &ObjectBasicInfo, sizeof PUBLIC_OBJECT_BASIC_INFORMATION, &RequiredSize);
						cZwQueryObject(myHandle, ObjectNameInformation, ObjectNameInfo, 8, &RequiredSize);
						cZwQueryObject(myHandle, ObjectNameInformation, ObjectNameInfo, RequiredSize, &RequiredSize);
						RtlZeroMemory(HandleFullName, 0x1000);
						if(pObjectNameInfo->Name.Length != NULL){
							WideCharToMultiByte(CP_ACP, NULL, (LPCWSTR)pObjectNameInfo->Name.Buffer, -1, (LPSTR)HandleFullName, 0x1000, NULL, NULL);
							if(NameIsTranslated){
								tmpHandleFullName = TranslateNativeName((char*)HandleFullName);
								if(tmpHandleFullName != NULL){
									VirtualFree(HandleFullName, NULL, MEM_RELEASE);
									HandleFullName = tmpHandleFullName;
								}
							}
							if(NameIsFolder){
								if(lstrlenA((LPCSTR)HandleFullName) > LenFileOrFolderName){
									RtlZeroMemory((LPVOID)((ULONG_PTR)HandleFullName + LenFileOrFolderName), 1);
								}
							}
							if(lstrcmpiA((LPCSTR)HandleFullName, szFileOrFolderName) == NULL && MaxHandleCount > NULL){
								RtlMoveMemory(HandleDataBuffer, &HandleInfo->ProcessId, sizeof ULONG);
								HandleDataBuffer = (LPVOID)((ULONG_PTR)HandleDataBuffer + sizeof ULONG);
								CopyHandle = (HANDLE)HandleInfo->hHandle;
								RtlMoveMemory(HandleDataBuffer, &CopyHandle, sizeof HANDLE);
								HandleDataBuffer = (LPVOID)((ULONG_PTR)HandleDataBuffer + sizeof HANDLE);
								FoundHandles++;
								MaxHandleCount--;
							}
						}
						EngineCloseHandle(myHandle);
					}
				}
			}
			HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)((ULONG_PTR)HandleInfo + sizeof NTDLL_QUERY_HANDLE_INFO);
			TotalHandleCount--;
		}
		VirtualFree(ObjectNameInfo, NULL, MEM_RELEASE);
		VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
		VirtualFree(HandleFullName, NULL, MEM_RELEASE);
		return(FoundHandles);
	}
	VirtualFree(ObjectNameInfo, NULL, MEM_RELEASE);
	VirtualFree(HandleFullName, NULL, MEM_RELEASE);
	return(NULL);
}
__declspec(dllexport) bool __stdcall HandlerCloseAllLockHandles(char* szFileOrFolderName, bool NameIsFolder, bool NameIsTranslated){

	bool AllHandled = true;
	HANDLE hProcess = NULL;
	HANDLE myHandle = NULL;
	HANDLE CopyHandle = NULL;
	LPVOID QuerySystemBuffer;
	ULONG QuerySystemBufferSize = 0x2000;
	ULONG RequiredSize = NULL;
	ULONG TotalHandleCount = NULL;
	DWORD LastProcessId = NULL;
#if !defined(_WIN64)
	typedef NTSTATUS(__stdcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__stdcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#else
	typedef NTSTATUS(__fastcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__fastcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#endif
	LPVOID ZwQuerySystemInformation = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQuerySystemInformation");
	LPVOID ZwQueryObject = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQueryObject");
	fZwQuerySystemInformation cZwQuerySystemInformation = (fZwQuerySystemInformation)(ZwQuerySystemInformation);
	fZwQueryObject cZwQueryObject = (fZwQueryObject)(ZwQueryObject);
	PNTDLL_QUERY_HANDLE_INFO HandleInfo;
	PUBLIC_OBJECT_BASIC_INFORMATION ObjectBasicInfo;
	LPVOID ObjectNameInfo = VirtualAlloc(NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE);
	PPUBLIC_OBJECT_NAME_INFORMATION pObjectNameInfo = (PPUBLIC_OBJECT_NAME_INFORMATION)ObjectNameInfo;
	LPVOID HandleFullName = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	int LenFileOrFolderName = lstrlenA(szFileOrFolderName);
	LPVOID tmpHandleFullName = NULL;

	if(ZwQuerySystemInformation != NULL && ZwQueryObject != NULL){
		QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		while(cZwQuerySystemInformation(NTDLL_SystemHandleInfo, QuerySystemBuffer, QuerySystemBufferSize, &RequiredSize) == (NTSTATUS)0xC0000004L){
			QuerySystemBufferSize = RequiredSize;
			VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
			QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		}
		RtlMoveMemory(&TotalHandleCount, QuerySystemBuffer, sizeof ULONG);
		QuerySystemBuffer = (LPVOID)((ULONG_PTR)QuerySystemBuffer + 4);
		HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)QuerySystemBuffer;
		while(TotalHandleCount > NULL){
			if(LastProcessId != HandleInfo->ProcessId){
				if(hProcess != NULL){
					EngineCloseHandle(hProcess);
				}
				hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE, false, HandleInfo->ProcessId);
				LastProcessId = HandleInfo->ProcessId;
			}
			if(hProcess != NULL){
				if(!(HandleInfo->GrantedAccess & SYNCHRONIZE) || ((HandleInfo->GrantedAccess & SYNCHRONIZE) && ((WORD)HandleInfo->GrantedAccess != 0x19F && (WORD)HandleInfo->GrantedAccess != 0x89))){
					if(DuplicateHandle(hProcess, (HANDLE)HandleInfo->hHandle, GetCurrentProcess(), &myHandle, NULL, false, DUPLICATE_SAME_ACCESS)){
						RtlZeroMemory(&ObjectBasicInfo, sizeof PUBLIC_OBJECT_BASIC_INFORMATION);
						cZwQueryObject(myHandle, ObjectBasicInformation, &ObjectBasicInfo, sizeof PUBLIC_OBJECT_BASIC_INFORMATION, &RequiredSize);
						cZwQueryObject(myHandle, ObjectNameInformation, ObjectNameInfo, 8, &RequiredSize);
						cZwQueryObject(myHandle, ObjectNameInformation, ObjectNameInfo, RequiredSize, &RequiredSize);
						RtlZeroMemory(HandleFullName, 0x1000);
						if(pObjectNameInfo->Name.Length != NULL){
							WideCharToMultiByte(CP_ACP, NULL, (LPCWSTR)pObjectNameInfo->Name.Buffer, -1, (LPSTR)HandleFullName, 0x1000, NULL, NULL);
							if(NameIsTranslated){
								tmpHandleFullName = TranslateNativeName((char*)HandleFullName);
								if(tmpHandleFullName != NULL){
									VirtualFree(HandleFullName, NULL, MEM_RELEASE);
									HandleFullName = tmpHandleFullName;
								}
							}
							if(NameIsFolder){
								if(lstrlenA((LPCSTR)HandleFullName) > LenFileOrFolderName){
									RtlZeroMemory((LPVOID)((ULONG_PTR)HandleFullName + LenFileOrFolderName), 1);
								}
							}
							if(lstrcmpiA((LPCSTR)HandleFullName, szFileOrFolderName) == NULL){
								if(!HandlerCloseRemoteHandle(hProcess, (HANDLE)HandleInfo->hHandle)){
									AllHandled = false;
								}
							}
						}
						EngineCloseHandle(myHandle);
					}
				}
			}
			HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)((ULONG_PTR)HandleInfo + sizeof NTDLL_QUERY_HANDLE_INFO);
			TotalHandleCount--;
		}
		VirtualFree(ObjectNameInfo, NULL, MEM_RELEASE);
		VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
		VirtualFree(HandleFullName, NULL, MEM_RELEASE);
		if(AllHandled){
			return(true);
		}else{
			return(false);
		}
	}
	VirtualFree(ObjectNameInfo, NULL, MEM_RELEASE);
	VirtualFree(HandleFullName, NULL, MEM_RELEASE);
	return(false);
}
__declspec(dllexport) bool __stdcall HandlerIsFileLocked(char* szFileOrFolderName, bool NameIsFolder, bool NameIsTranslated){

	HANDLE hProcess = NULL;
	HANDLE myHandle = NULL;
	HANDLE CopyHandle = NULL;
	LPVOID QuerySystemBuffer;
	ULONG QuerySystemBufferSize = 0x2000;
	ULONG RequiredSize = NULL;
	ULONG TotalHandleCount = NULL;
	DWORD LastProcessId = NULL;
#if !defined(_WIN64)
	typedef NTSTATUS(__stdcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__stdcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#else
	typedef NTSTATUS(__fastcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__fastcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#endif
	LPVOID ZwQuerySystemInformation = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQuerySystemInformation");
	LPVOID ZwQueryObject = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQueryObject");
	fZwQuerySystemInformation cZwQuerySystemInformation = (fZwQuerySystemInformation)(ZwQuerySystemInformation);
	fZwQueryObject cZwQueryObject = (fZwQueryObject)(ZwQueryObject);
	PNTDLL_QUERY_HANDLE_INFO HandleInfo;
	PUBLIC_OBJECT_BASIC_INFORMATION ObjectBasicInfo;
	LPVOID ObjectNameInfo = VirtualAlloc(NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE);
	PPUBLIC_OBJECT_NAME_INFORMATION pObjectNameInfo = (PPUBLIC_OBJECT_NAME_INFORMATION)ObjectNameInfo;
	LPVOID HandleFullName = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	int LenFileOrFolderName = lstrlenA(szFileOrFolderName);
	LPVOID tmpHandleFullName = NULL;

	if(ZwQuerySystemInformation != NULL && ZwQueryObject != NULL){
		QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		while(cZwQuerySystemInformation(NTDLL_SystemHandleInfo, QuerySystemBuffer, QuerySystemBufferSize, &RequiredSize) == (NTSTATUS)0xC0000004L){
			QuerySystemBufferSize = RequiredSize;
			VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
			QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		}
		RtlMoveMemory(&TotalHandleCount, QuerySystemBuffer, sizeof ULONG);
		QuerySystemBuffer = (LPVOID)((ULONG_PTR)QuerySystemBuffer + 4);
		HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)QuerySystemBuffer;
		while(TotalHandleCount > NULL){
			if(LastProcessId != HandleInfo->ProcessId){
				if(hProcess != NULL){
					EngineCloseHandle(hProcess);
				}
				hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE, false, HandleInfo->ProcessId);
				LastProcessId = HandleInfo->ProcessId;
			}
			if(hProcess != NULL){
				if(!(HandleInfo->GrantedAccess & SYNCHRONIZE) || ((HandleInfo->GrantedAccess & SYNCHRONIZE) && ((WORD)HandleInfo->GrantedAccess != 0x19F && (WORD)HandleInfo->GrantedAccess != 0x89))){
					if(DuplicateHandle(hProcess, (HANDLE)HandleInfo->hHandle, GetCurrentProcess(), &myHandle, NULL, false, DUPLICATE_SAME_ACCESS)){
						RtlZeroMemory(&ObjectBasicInfo, sizeof PUBLIC_OBJECT_BASIC_INFORMATION);
						cZwQueryObject(myHandle, ObjectBasicInformation, &ObjectBasicInfo, sizeof PUBLIC_OBJECT_BASIC_INFORMATION, &RequiredSize);
						cZwQueryObject(myHandle, ObjectNameInformation, ObjectNameInfo, 8, &RequiredSize);
						cZwQueryObject(myHandle, ObjectNameInformation, ObjectNameInfo, RequiredSize, &RequiredSize);
						RtlZeroMemory(HandleFullName, 0x1000);
						if(pObjectNameInfo->Name.Length != NULL){
							WideCharToMultiByte(CP_ACP, NULL, (LPCWSTR)pObjectNameInfo->Name.Buffer, -1, (LPSTR)HandleFullName, 0x1000, NULL, NULL);
							if(NameIsTranslated){
								tmpHandleFullName = TranslateNativeName((char*)HandleFullName);
								if(tmpHandleFullName != NULL){
									VirtualFree(HandleFullName, NULL, MEM_RELEASE);
									HandleFullName = tmpHandleFullName;
								}
							}
							if(NameIsFolder){
								if(lstrlenA((LPCSTR)HandleFullName) > LenFileOrFolderName){
									RtlZeroMemory((LPVOID)((ULONG_PTR)HandleFullName + LenFileOrFolderName), 1);
								}
							}
							if(lstrcmpiA((LPCSTR)HandleFullName, szFileOrFolderName) == NULL){
								VirtualFree(ObjectNameInfo, NULL, MEM_RELEASE);
								VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
								VirtualFree(HandleFullName, NULL, MEM_RELEASE);
								EngineCloseHandle(myHandle);
								return(true);
							}
						}
						EngineCloseHandle(myHandle);
					}
				}
			}
			HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)((ULONG_PTR)HandleInfo + sizeof NTDLL_QUERY_HANDLE_INFO);
			TotalHandleCount--;
		}
		VirtualFree(ObjectNameInfo, NULL, MEM_RELEASE);
		VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
		VirtualFree(HandleFullName, NULL, MEM_RELEASE);
		return(false);
	}
	VirtualFree(ObjectNameInfo, NULL, MEM_RELEASE);
	VirtualFree(HandleFullName, NULL, MEM_RELEASE);
	return(false);
}
// UnpackEngine.Handler[Mutex].functions:
__declspec(dllexport) long __stdcall HandlerEnumerateOpenMutexes(HANDLE hProcess, DWORD ProcessId, LPVOID HandleBuffer, DWORD MaxHandleCount){

	HANDLE myHandle = NULL;
	HANDLE copyHandle = NULL;
	LPVOID QuerySystemBuffer;
	ULONG RequiredSize = NULL;
	ULONG TotalHandleCount = NULL;
	unsigned int HandleCount = NULL;
	ULONG QuerySystemBufferSize = 0x2000;
#if !defined(_WIN64)
	typedef NTSTATUS(__stdcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__stdcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#else
	typedef NTSTATUS(__fastcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__fastcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#endif
	LPVOID ZwQuerySystemInformation = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQuerySystemInformation");
	LPVOID ZwQueryObject = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQueryObject");
	fZwQuerySystemInformation cZwQuerySystemInformation = (fZwQuerySystemInformation)(ZwQuerySystemInformation);
	fZwQueryObject cZwQueryObject = (fZwQueryObject)(ZwQueryObject);
	PNTDLL_QUERY_HANDLE_INFO HandleInfo;
	LPVOID HandleFullData = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	LPVOID HandleNameData = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	PPUBLIC_OBJECT_TYPE_INFORMATION pObjectTypeInfo = (PPUBLIC_OBJECT_TYPE_INFORMATION)HandleFullData;

	if(ZwQuerySystemInformation != NULL && ZwQueryObject != NULL){
		QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		while(cZwQuerySystemInformation(NTDLL_SystemHandleInfo, QuerySystemBuffer, QuerySystemBufferSize, &RequiredSize) == (NTSTATUS)0xC0000004L){
			QuerySystemBufferSize = RequiredSize;
			VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
			QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		}
		RtlMoveMemory(&TotalHandleCount, QuerySystemBuffer, sizeof ULONG);
		QuerySystemBuffer = (LPVOID)((ULONG_PTR)QuerySystemBuffer + 4);
		HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)QuerySystemBuffer;
		while(TotalHandleCount > NULL){
			if(HandleInfo->ProcessId == ProcessId && HandleCount < MaxHandleCount){
				if(!(HandleInfo->GrantedAccess & SYNCHRONIZE) || ((HandleInfo->GrantedAccess & SYNCHRONIZE) && ((WORD)HandleInfo->GrantedAccess != 0x19F && (WORD)HandleInfo->GrantedAccess != 0x89))){
					if(DuplicateHandle(hProcess, (HANDLE)HandleInfo->hHandle, GetCurrentProcess(), &myHandle, NULL, false, DUPLICATE_SAME_ACCESS)){
						RtlZeroMemory(HandleFullData, 0x1000);
						cZwQueryObject(myHandle, ObjectTypeInformation, HandleFullData, 8, &RequiredSize);
						cZwQueryObject(myHandle, ObjectTypeInformation, HandleFullData, RequiredSize, &RequiredSize);
						RtlZeroMemory(HandleNameData, 0x1000);
						if(pObjectTypeInfo->Name.Length != NULL){
							WideCharToMultiByte(CP_ACP, NULL, (LPCWSTR)pObjectTypeInfo->Name.Buffer, -1, (LPSTR)HandleNameData, 0x1000, NULL, NULL);
							if(lstrcmpiA((LPCSTR)HandleNameData, "Mutant") == NULL){
								copyHandle = (HANDLE)HandleInfo->hHandle;
								RtlMoveMemory(HandleBuffer, &copyHandle, sizeof HANDLE);
								HandleBuffer = (LPVOID)((ULONG_PTR)HandleBuffer + sizeof HANDLE);
								HandleCount++;
							}
						}
						EngineCloseHandle(myHandle);
					}
				}
			}
			HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)((ULONG_PTR)HandleInfo + sizeof NTDLL_QUERY_HANDLE_INFO);
			TotalHandleCount--;
		}
		VirtualFree(HandleFullData, NULL, MEM_RELEASE);
		VirtualFree(HandleNameData, NULL, MEM_RELEASE);
		VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
		return(HandleCount);
	}
	VirtualFree(HandleFullData, NULL, MEM_RELEASE);
	VirtualFree(HandleNameData, NULL, MEM_RELEASE);
	return(NULL);
}
__declspec(dllexport) long long __stdcall HandlerGetOpenMutexHandle(HANDLE hProcess, DWORD ProcessId, char* szMutexString){

	int i;
	HANDLE myHandle;
	LPVOID HandleBuffer = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	LPVOID cHandleBuffer = HandleBuffer;
	int OpenHandleCount = HandlerEnumerateOpenMutexes(hProcess, ProcessId, HandleBuffer, 0x1000 / sizeof HANDLE);
	char RealMutexName[512] = "\\BaseNamedObjects\\";
	char* HandleName;

	if(OpenHandleCount > NULL){
		lstrcatA(RealMutexName, szMutexString);
		for(i = 0; i < OpenHandleCount; i++){
			RtlMoveMemory(&myHandle, cHandleBuffer, sizeof HANDLE);
			HandleName = (char*)HandlerGetHandleName(hProcess, ProcessId, myHandle, true);
			if(HandleName != NULL){
				if(lstrcmpiA(HandleName, RealMutexName) == NULL){
					VirtualFree(HandleBuffer, NULL, MEM_RELEASE);
					return((ULONG_PTR)myHandle);
				}
			}
			cHandleBuffer = (LPVOID)((ULONG_PTR)cHandleBuffer + sizeof HANDLE);
		}
	}
	VirtualFree(HandleBuffer, NULL, MEM_RELEASE);
	return(NULL);
}
__declspec(dllexport) long __stdcall HandlerGetProcessIdWhichCreatedMutex(char* szMutexString){

	HANDLE hProcess = NULL;
	DWORD ReturnData = NULL;
	HANDLE myHandle = NULL;
	LPVOID QuerySystemBuffer;
	ULONG RequiredSize = NULL;
	DWORD LastProcessId = NULL;
	ULONG TotalHandleCount = NULL;
	ULONG QuerySystemBufferSize = 0x2000;
#if !defined(_WIN64)
	typedef NTSTATUS(__stdcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__stdcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#else
	typedef NTSTATUS(__fastcall *fZwQuerySystemInformation)(DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
	typedef NTSTATUS(__fastcall *fZwQueryObject)(HANDLE hObject, DWORD fInfoType, LPVOID fBuffer, ULONG fBufferSize, PULONG fRequiredSize);
#endif
	LPVOID ZwQuerySystemInformation = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQuerySystemInformation");
	LPVOID ZwQueryObject = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwQueryObject");
	fZwQuerySystemInformation cZwQuerySystemInformation = (fZwQuerySystemInformation)(ZwQuerySystemInformation);
	fZwQueryObject cZwQueryObject = (fZwQueryObject)(ZwQueryObject);
	PNTDLL_QUERY_HANDLE_INFO HandleInfo;
	LPVOID HandleFullData = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	LPVOID HandleNameData = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	PPUBLIC_OBJECT_TYPE_INFORMATION pObjectTypeInfo = (PPUBLIC_OBJECT_TYPE_INFORMATION)HandleFullData;
	LPVOID ObjectNameInfo = VirtualAlloc(NULL, 0x2000, MEM_COMMIT, PAGE_READWRITE);
	PPUBLIC_OBJECT_NAME_INFORMATION pObjectNameInfo = (PPUBLIC_OBJECT_NAME_INFORMATION)ObjectNameInfo;
	char RealMutexName[512] = "\\BaseNamedObjects\\";

	if(ZwQuerySystemInformation != NULL && ZwQueryObject != NULL){
		lstrcatA(RealMutexName, szMutexString);
		QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		while(cZwQuerySystemInformation(NTDLL_SystemHandleInfo, QuerySystemBuffer, QuerySystemBufferSize, &RequiredSize) == (NTSTATUS)0xC0000004L){
			QuerySystemBufferSize = RequiredSize;
			VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
			QuerySystemBuffer = VirtualAlloc(NULL, QuerySystemBufferSize, MEM_COMMIT, PAGE_READWRITE);
		}
		RtlMoveMemory(&TotalHandleCount, QuerySystemBuffer, sizeof ULONG);
		QuerySystemBuffer = (LPVOID)((ULONG_PTR)QuerySystemBuffer + 4);
		HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)QuerySystemBuffer;
		while(TotalHandleCount > NULL){
			if(LastProcessId != HandleInfo->ProcessId){
				if(hProcess != NULL){
					EngineCloseHandle(hProcess);
				}
				hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE, false, HandleInfo->ProcessId);
				LastProcessId = HandleInfo->ProcessId;
			}
			if(hProcess != NULL){
				if(!(HandleInfo->GrantedAccess & SYNCHRONIZE) || ((HandleInfo->GrantedAccess & SYNCHRONIZE) && ((WORD)HandleInfo->GrantedAccess != 0x19F && (WORD)HandleInfo->GrantedAccess != 0x89))){
					if(DuplicateHandle(hProcess, (HANDLE)HandleInfo->hHandle, GetCurrentProcess(), &myHandle, NULL, false, DUPLICATE_SAME_ACCESS)){
						RtlZeroMemory(HandleFullData, 0x1000);
						cZwQueryObject(myHandle, ObjectTypeInformation, HandleFullData, 8, &RequiredSize);
						cZwQueryObject(myHandle, ObjectTypeInformation, HandleFullData, RequiredSize, &RequiredSize);
						RtlZeroMemory(HandleNameData, 0x1000);
						if(pObjectTypeInfo->Name.Length != NULL){
							WideCharToMultiByte(CP_ACP, NULL, (LPCWSTR)pObjectTypeInfo->Name.Buffer, -1, (LPSTR)HandleNameData, 0x1000, NULL, NULL);
							if(lstrcmpiA((LPCSTR)HandleNameData, "Mutant") == NULL){
								cZwQueryObject(myHandle, ObjectNameInformation, ObjectNameInfo, 8, &RequiredSize);
								cZwQueryObject(myHandle, ObjectNameInformation, ObjectNameInfo, RequiredSize, &RequiredSize);
								RtlZeroMemory(HandleNameData, 0x1000);
								if(pObjectNameInfo->Name.Length != NULL){
									RtlZeroMemory(HandleNameData, 0x1000);
									WideCharToMultiByte(CP_ACP, NULL, (LPCWSTR)pObjectNameInfo->Name.Buffer, -1, (LPSTR)HandleNameData, 0x1000, NULL, NULL);
									if(lstrcmpiA((LPCSTR)HandleNameData, RealMutexName) == NULL){
										ReturnData = HandleInfo->ProcessId;
										break;
									}
								}
							}
						}
						EngineCloseHandle(myHandle);
					}
				}
			}
			HandleInfo = (PNTDLL_QUERY_HANDLE_INFO)((ULONG_PTR)HandleInfo + sizeof NTDLL_QUERY_HANDLE_INFO);
			TotalHandleCount--;
		}
		VirtualFree(HandleFullData, NULL, MEM_RELEASE);
		VirtualFree(HandleNameData, NULL, MEM_RELEASE);		
		VirtualFree(ObjectNameInfo, NULL, MEM_RELEASE);
		VirtualFree(QuerySystemBuffer, NULL, MEM_RELEASE);
		return(ReturnData);
	}
	VirtualFree(HandleFullData, NULL, MEM_RELEASE);
	VirtualFree(HandleNameData, NULL, MEM_RELEASE);
	VirtualFree(ObjectNameInfo, NULL, MEM_RELEASE);
	return(NULL);
}
// Global.Injector.functions: {DO NOT REORDER! USE ONLY IN RELEASE MODE!}
long __stdcall injectedImpRec(LPVOID Parameter){

	HANDLE hFile;
	HANDLE hFileMap;
	PInjectImpRecCodeData APIData = (PInjectImpRecCodeData)Parameter;
	LPVOID szFileName = (LPVOID)((ULONG_PTR)Parameter + sizeof InjectImpRecCodeData);
	typedef ULONG_PTR(__cdecl *fTrace)(DWORD hFileMap, DWORD dwSizeMap, DWORD dwTimeOut, DWORD dwToTrace, DWORD dwExactCall);
	typedef HANDLE(__stdcall *fCreateFileA)(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
	typedef HANDLE(__stdcall *fCreateFileMappingA)(HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName);
	typedef BOOL(__cdecl *fCloseHandle)(HANDLE hHandle);
	fTrace cTrace = (fTrace)(APIData->fTrace);
	fCreateFileA cCreateFileA = (fCreateFileA)(APIData->fCreateFileA);
	fCloseHandle cCloseHandle = (fCloseHandle)(APIData->fCloseHandle);
	fCreateFileMappingA cCreateFileMappingA = (fCreateFileMappingA)(APIData->fCreateFileMappingA);

	hFile = cCreateFileA((LPCSTR)szFileName, GENERIC_READ+GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE){
		hFileMap = cCreateFileMappingA(hFile, NULL, 4, NULL, 0x100, NULL);
		cTrace((DWORD)hFileMap, 0x100, -1, (DWORD)APIData->AddressToTrace, NULL);
		cCloseHandle(hFile);
		return(1);
	}else{
		return(0);
	}
}
long __stdcall injectedRemoteLoadLibrary(LPVOID Parameter){

	PInjectCodeData APIData = (PInjectCodeData)Parameter;
	Parameter = (LPVOID)((ULONG_PTR)Parameter + sizeof InjectCodeData);
#if !defined(_WIN64)
	typedef ULONG_PTR(__stdcall *fLoadLibraryA)(LPCSTR fLibraryName);
	typedef ULONG_PTR(__stdcall *fVirtualFree)(LPVOID fMemBase, SIZE_T fMemSize, DWORD fFreeType);
#else
	typedef ULONG_PTR(__fastcall *fLoadLibraryA)(LPCSTR fLibraryName);
	typedef ULONG_PTR(__fastcall *fVirtualFree)(LPVOID fMemBase, SIZE_T fMemSize, DWORD fFreeType);
#endif
	fLoadLibraryA cLoadLibraryA = (fLoadLibraryA)(APIData->fLoadLibrary);
	fVirtualFree cVirtualFree = (fVirtualFree)(APIData->fVirtualFree);
	long retValue = NULL;

	if(cLoadLibraryA((LPCSTR)Parameter) != NULL){
		retValue++;
	}
	cVirtualFree(Parameter, NULL, MEM_RELEASE);
	return(retValue);
}
long __stdcall injectedRemoteFreeLibrary(LPVOID Parameter){

	PInjectCodeData APIData = (PInjectCodeData)Parameter;
#if !defined(_WIN64)
	typedef ULONG_PTR(__stdcall *fFreeLibrary)(HMODULE fLibBase);
	typedef ULONG_PTR(__stdcall *fVirtualFree)(LPVOID fMemBase, SIZE_T fMemSize, DWORD fFreeType);
#else
	typedef ULONG_PTR(__fastcall *fFreeLibrary)(HMODULE fLibBase);
	typedef ULONG_PTR(__fastcall *fVirtualFree)(LPVOID fMemBase, SIZE_T fMemSize, DWORD fFreeType);
#endif
	fFreeLibrary cFreeLibrary = (fFreeLibrary)(APIData->fFreeLibrary);
	fVirtualFree cVirtualFree = (fVirtualFree)(APIData->fVirtualFree);
	long retValue = NULL;

	if(cFreeLibrary(APIData->fFreeLibraryHandle)){
		retValue++;
	}
	cVirtualFree(Parameter, NULL, MEM_RELEASE);
	return(retValue);
}
long __stdcall injectedRemoteFreeLibrarySimple(LPVOID Parameter){

	PInjectCodeData APIData = (PInjectCodeData)Parameter;
	LPVOID orgParameter = Parameter;
	Parameter = (LPVOID)((ULONG_PTR)Parameter + sizeof InjectCodeData);
#if !defined(_WIN64)
	typedef ULONG_PTR(__stdcall *fFreeLibrary)(HMODULE fLibBase);
	typedef HMODULE(__stdcall *fGetModuleHandleA)(LPCSTR fLibraryName);
	typedef ULONG_PTR(__stdcall *fVirtualFree)(LPVOID fMemBase, SIZE_T fMemSize, DWORD fFreeType);
#else
	typedef ULONG_PTR(__fastcall *fFreeLibrary)(HMODULE fLibBase);
	typedef HMODULE(__fastcall *fGetModuleHandleA)(LPCSTR fLibraryName);
	typedef ULONG_PTR(__fastcall *fVirtualFree)(LPVOID fMemBase, SIZE_T fMemSize, DWORD fFreeType);
#endif
	fGetModuleHandleA cGetModuleHandleA = (fGetModuleHandleA)(APIData->fGetModuleHandle);
	fFreeLibrary cFreeLibrary = (fFreeLibrary)(APIData->fFreeLibrary);
	fVirtualFree cVirtualFree = (fVirtualFree)(APIData->fVirtualFree);
	long retValue = NULL;
	HMODULE hModule;

	hModule = cGetModuleHandleA((LPCSTR)Parameter);
	if(hModule != NULL){
		if(cFreeLibrary(hModule)){
			retValue++;
		}
	}else{
		retValue++;
	}
	cVirtualFree(orgParameter, NULL, MEM_RELEASE);
	return(retValue);
}
long __stdcall injectedExitProcess(LPVOID Parameter){

	PInjectCodeData APIData = (PInjectCodeData)Parameter;
#if !defined(_WIN64)
	typedef ULONG_PTR(__stdcall *fExitProcess)(DWORD fExitCode);
#else
	typedef ULONG_PTR(__fastcall *fExitProcess)(DWORD fExitCode);
#endif
	fExitProcess cExitProcess = (fExitProcess)(APIData->fExitProcess);
	long retValue = NULL;

	cExitProcess(APIData->fExitProcessCode);
	return(NULL);
}
void __stdcall injectedTerminator(){

	int i;

	for(i = 0; i <= UE_MAX_RESERVED_MEMORY_LEFT; i++){
		if(engineReservedMemoryLeft[i] != NULL){
			VirtualFreeEx(engineReservedMemoryProcess, (LPVOID)engineReservedMemoryLeft[i], NULL, MEM_RELEASE);
			engineReservedMemoryLeft[i] = NULL;
		}
	}
}
// UnpackEngine.Injector.functions:
__declspec(dllexport) bool __stdcall RemoteLoadLibrary(HANDLE hProcess, char* szLibraryFile, bool WaitForThreadExit){

	int i;
	InjectCodeData APIData;
	LPVOID remStringData;
	LPVOID remCodeData;
	ULONG_PTR remInjectSize = (ULONG_PTR)((ULONG_PTR)&injectedRemoteFreeLibrary - (ULONG_PTR)&injectedRemoteLoadLibrary);
#if !defined(_WIN64)
	typedef NTSTATUS(__stdcall *fZwSetInformationThread)(HANDLE fThreadHandle, DWORD fThreadInfoClass, LPVOID fBuffer, ULONG fBufferSize);
#else
	typedef NTSTATUS(__fastcall *fZwSetInformationThread)(HANDLE fThreadHandle, DWORD fThreadInfoClass, LPVOID fBuffer, ULONG fBufferSize);
#endif
	LPVOID ZwSetInformationThread = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwSetInformationThread");
	fZwSetInformationThread cZwSetInformationThread = (fZwSetInformationThread)(ZwSetInformationThread);
	ULONG_PTR NumberOfBytesWritten;
	DWORD ThreadId;
	HANDLE hThread;
	DWORD ExitCode;

	if(hProcess != NULL){
		RtlZeroMemory(&APIData, sizeof InjectCodeData);
		APIData.fLoadLibrary = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"));
		APIData.fFreeLibrary = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "FreeLibrary"));
		APIData.fGetModuleHandle = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetModuleHandleA"));
		APIData.fGetProcAddress = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetProcAddress"));
		APIData.fVirtualFree = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "VirtualFree"));
		APIData.fExitProcess = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "ExitProcess"));
		remCodeData = VirtualAllocEx(hProcess, NULL, remInjectSize, MEM_COMMIT, PAGE_READWRITE);
		remStringData = VirtualAllocEx(hProcess, NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
		if(WriteProcessMemory(hProcess, (LPVOID)((ULONG_PTR)remStringData + sizeof InjectCodeData), (LPCVOID)szLibraryFile, lstrlenA(szLibraryFile), &NumberOfBytesWritten)){
			WriteProcessMemory(hProcess, remStringData, &APIData, sizeof InjectCodeData, &NumberOfBytesWritten);
			WriteProcessMemory(hProcess, remCodeData, (LPCVOID)&injectedRemoteLoadLibrary, remInjectSize, &NumberOfBytesWritten);
			if(WaitForThreadExit){
				hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)remCodeData, remStringData, CREATE_SUSPENDED, &ThreadId);
				if(ZwSetInformationThread != NULL){
					cZwSetInformationThread(hThread, 0x11, NULL, NULL);
				}
				ResumeThread(hThread);
				WaitForSingleObject(hThread, INFINITE);
				VirtualFreeEx(hProcess, remCodeData, NULL, MEM_RELEASE);
				VirtualFreeEx(hProcess, remStringData, NULL, MEM_RELEASE);
				if(GetExitCodeThread(hThread, &ExitCode)){
					if(ExitCode == NULL){
						return(false);
					}
				}
			}else{
				hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)remCodeData, remStringData, NULL, &ThreadId);
				for(i = 0; i < UE_MAX_RESERVED_MEMORY_LEFT; i++){
					if(engineReservedMemoryLeft[i] == NULL){
						break;
					}
				}
				engineReservedMemoryLeft[i] = (ULONG_PTR)remCodeData;
				engineReservedMemoryProcess = hProcess;
				ThreaderSetCallBackForNextExitThreadEvent((LPVOID)&injectedTerminator);
			}
			return(true);
		}else{
			VirtualFreeEx(hProcess, remCodeData, NULL, MEM_RELEASE);
			VirtualFreeEx(hProcess, remStringData, NULL, MEM_RELEASE);
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall RemoteFreeLibrary(HANDLE hProcess, HMODULE hModule, char* szLibraryFile, bool WaitForThreadExit){

	int i;
	InjectCodeData APIData;
	LPVOID remStringData;
	LPVOID remCodeData;
	ULONG_PTR remInjectSize1 = (ULONG_PTR)((ULONG_PTR)&injectedExitProcess - (ULONG_PTR)&injectedRemoteFreeLibrarySimple);
	ULONG_PTR remInjectSize2 = (ULONG_PTR)((ULONG_PTR)&injectedRemoteFreeLibrarySimple - (ULONG_PTR)&injectedRemoteFreeLibrary);
#if !defined(_WIN64)
	typedef NTSTATUS(__stdcall *fZwSetInformationThread)(HANDLE fThreadHandle, DWORD fThreadInfoClass, LPVOID fBuffer, ULONG fBufferSize);
#else
	typedef NTSTATUS(__fastcall *fZwSetInformationThread)(HANDLE fThreadHandle, DWORD fThreadInfoClass, LPVOID fBuffer, ULONG fBufferSize);
#endif
	LPVOID ZwSetInformationThread = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwSetInformationThread");
	fZwSetInformationThread cZwSetInformationThread = (fZwSetInformationThread)(ZwSetInformationThread);
	ULONG_PTR NumberOfBytesWritten;
	DWORD ThreadId;
	HANDLE hThread;
	DWORD ExitCode;

	if(hProcess != NULL){
		RtlZeroMemory(&APIData, sizeof InjectCodeData);
		APIData.fLoadLibrary = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"));
		APIData.fFreeLibrary = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "FreeLibrary"));
		APIData.fGetModuleHandle = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetModuleHandleA"));
		APIData.fGetProcAddress = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetProcAddress"));
		APIData.fVirtualFree = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "VirtualFree"));
		APIData.fExitProcess = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "ExitProcess"));
		APIData.fFreeLibraryHandle = hModule;
		remCodeData = VirtualAllocEx(hProcess, NULL, remInjectSize1, MEM_COMMIT, PAGE_READWRITE);
		if(hModule == NULL){
			remStringData = VirtualAllocEx(hProcess, NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
			if(WriteProcessMemory(hProcess, (LPVOID)((ULONG_PTR)remStringData + sizeof InjectCodeData), (LPCVOID)szLibraryFile, lstrlenA(szLibraryFile), &NumberOfBytesWritten)){
				WriteProcessMemory(hProcess, remStringData, &APIData, sizeof InjectCodeData, &NumberOfBytesWritten);
				WriteProcessMemory(hProcess, remCodeData, (LPCVOID)&injectedRemoteFreeLibrarySimple, remInjectSize1, &NumberOfBytesWritten);
				if(WaitForThreadExit){
					hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)remCodeData, remStringData, CREATE_SUSPENDED, &ThreadId);
					if(ZwSetInformationThread != NULL){
						cZwSetInformationThread(hThread, 0x11, NULL, NULL);
					}
					ResumeThread(hThread);
					WaitForSingleObject(hThread, INFINITE);
					VirtualFreeEx(hProcess, remCodeData, NULL, MEM_RELEASE);
					VirtualFreeEx(hProcess, remStringData, NULL, MEM_RELEASE);
					if(GetExitCodeThread(hThread, &ExitCode)){
						if(ExitCode == NULL){
							return(false);
						}
					}
				}else{
					hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)remCodeData, remStringData, NULL, &ThreadId);
					for(i = 0; i < UE_MAX_RESERVED_MEMORY_LEFT; i++){
						if(engineReservedMemoryLeft[i] == NULL){
							break;
						}
					}
					engineReservedMemoryLeft[i] = (ULONG_PTR)remCodeData;
					engineReservedMemoryProcess = hProcess;
					ThreaderSetCallBackForNextExitThreadEvent((LPVOID)&injectedTerminator);
				}
				return(true);
			}else{
				VirtualFreeEx(hProcess, remCodeData, NULL, MEM_RELEASE);
				VirtualFreeEx(hProcess, remStringData, NULL, MEM_RELEASE);
			}
		}else{
			remStringData = VirtualAllocEx(hProcess, NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
			if(WriteProcessMemory(hProcess, remStringData, &APIData, sizeof InjectCodeData, &NumberOfBytesWritten)){
				WriteProcessMemory(hProcess, remCodeData, (LPCVOID)&injectedRemoteFreeLibrary, remInjectSize2, &NumberOfBytesWritten);
				if(WaitForThreadExit){
					hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)remCodeData, remStringData, CREATE_SUSPENDED, &ThreadId);
					if(ZwSetInformationThread != NULL){
						cZwSetInformationThread(hThread, 0x11, NULL, NULL);
					}
					ResumeThread(hThread);
					WaitForSingleObject(hThread, INFINITE);
					VirtualFreeEx(hProcess, remCodeData, NULL, MEM_RELEASE);
					if(GetExitCodeThread(hThread, &ExitCode)){
						if(ExitCode == NULL){
							return(false);
						}
					}
				}else{
					hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)remCodeData, remStringData, NULL, &ThreadId);
					for(i = 0; i < UE_MAX_RESERVED_MEMORY_LEFT; i++){
						if(engineReservedMemoryLeft[i] == NULL){
							break;
						}
					}
					engineReservedMemoryLeft[i] = (ULONG_PTR)remCodeData;
					engineReservedMemoryProcess = hProcess;
					ThreaderSetCallBackForNextExitThreadEvent((LPVOID)&injectedTerminator);
				}
				return(true);
			}else{
				VirtualFreeEx(hProcess, remCodeData, NULL, MEM_RELEASE);
				VirtualFreeEx(hProcess, remStringData, NULL, MEM_RELEASE);
			}
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall RemoteExitProcess(HANDLE hProcess, DWORD ExitCode){

	InjectCodeData APIData;
	LPVOID remCodeData;
	LPVOID remStringData;
	ULONG_PTR remInjectSize = (ULONG_PTR)((ULONG_PTR)&injectedTerminator - (ULONG_PTR)&injectedExitProcess);
	ULONG_PTR NumberOfBytesWritten;
	DWORD ThreadId;
	HANDLE hThread;

	if(hProcess != NULL){
		RtlZeroMemory(&APIData, sizeof InjectCodeData);
		APIData.fLoadLibrary = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"));
		APIData.fFreeLibrary = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "FreeLibrary"));
		APIData.fGetModuleHandle = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetModuleHandleA"));
		APIData.fGetProcAddress = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetProcAddress"));
		APIData.fVirtualFree = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "VirtualFree"));
		APIData.fExitProcess = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "ExitProcess"));
		APIData.fExitProcessCode = ExitCode;
		remStringData = VirtualAllocEx(hProcess, NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
		remCodeData = VirtualAllocEx(hProcess, NULL, remInjectSize, MEM_COMMIT, PAGE_READWRITE);
		if(WriteProcessMemory(hProcess, remCodeData, (LPCVOID)&injectedExitProcess, remInjectSize, &NumberOfBytesWritten)){
			WriteProcessMemory(hProcess, remStringData, &APIData, sizeof InjectCodeData, &NumberOfBytesWritten);
			hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)remCodeData, remStringData, NULL, &ThreadId);
			VirtualFreeEx(hProcess, remCodeData, NULL, MEM_RELEASE);
			return(true);
		}else{
			VirtualFreeEx(hProcess, remCodeData, NULL, MEM_RELEASE);
			VirtualFreeEx(hProcess, remStringData, NULL, MEM_RELEASE);
		}
	}
	return(false);
}
// UnpackEngine.Tracer.functions:
__declspec(dllexport) long __stdcall TracerFixRedirectionViaImpRecPlugin(HANDLE hProcess, char* szPluginName, ULONG_PTR AddressToTrace){

	int szLenght = NULL;
	HMODULE hImpRecModule = NULL;
	ULONG_PTR fImpRecTrace = NULL;
	PMEMORY_CMP_HANDLER cmpModuleName;
	ULONG_PTR remInjectSize = (ULONG_PTR)((ULONG_PTR)&injectedRemoteLoadLibrary - (ULONG_PTR)&injectedImpRec);
#if !defined(_WIN64)
	typedef NTSTATUS(__stdcall *fZwSetInformationThread)(HANDLE fThreadHandle, DWORD fThreadInfoClass, LPVOID fBuffer, ULONG fBufferSize);
#else
	typedef NTSTATUS(__fastcall *fZwSetInformationThread)(HANDLE fThreadHandle, DWORD fThreadInfoClass, LPVOID fBuffer, ULONG fBufferSize);
#endif
	LPVOID ZwSetInformationThread = (LPVOID)GetProcAddress(GetModuleHandleA("ntdll.dll"),"ZwSetInformationThread");
	fZwSetInformationThread cZwSetInformationThread = (fZwSetInformationThread)(ZwSetInformationThread);
	LPVOID szModuleName = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	LPVOID szGarbageFile = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
	LPVOID cModuleName = szModuleName;
	ULONG_PTR NumberOfBytesWritten;
	InjectImpRecCodeData APIData;
	DWORD TracedAddress = NULL;
	DWORD TraceAddress = NULL;
	LPVOID remStringData;
	LPVOID remCodeData;
	DWORD ThreadId;
	HANDLE hThread;
	DWORD ExitCode;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	ULONG_PTR FileMapVA;

	if(GetModuleFileNameA(engineHandle, (LPCH)szModuleName, 0x1000) > NULL){
		cModuleName = (LPVOID)((ULONG_PTR)cModuleName + lstrlenA((LPCSTR)szModuleName));
		cmpModuleName = (PMEMORY_CMP_HANDLER)(cModuleName);
		while(cmpModuleName->DataByte[0] != 0x5C){
			cmpModuleName = (PMEMORY_CMP_HANDLER)((ULONG_PTR)cmpModuleName - 1);
		}
		cmpModuleName = (PMEMORY_CMP_HANDLER)((ULONG_PTR)cmpModuleName + 1);
		cmpModuleName->DataByte[0] = 0x00;
		lstrcpyA((LPSTR)szGarbageFile, (LPCSTR)szModuleName);
		lstrcatA((LPSTR)szGarbageFile, "garbage\\ImpRec.txt");
		lstrcatA((LPSTR)szModuleName, "imports\\ImpRec\\");
		lstrcatA((LPSTR)szModuleName, szPluginName);
		if(ReadProcessMemory(hProcess, (LPVOID)AddressToTrace, &TraceAddress, 4, &NumberOfBytesWritten)){
			if(RemoteLoadLibrary(hProcess, (char*)szModuleName, true)){
				hImpRecModule = LoadLibraryA((char*)szModuleName);
				if(hImpRecModule != NULL){
					fImpRecTrace = (ULONG_PTR)GetProcAddress(hImpRecModule, "Trace");
					if(fImpRecTrace != NULL){
						fImpRecTrace = fImpRecTrace - (ULONG_PTR)hImpRecModule;
						remCodeData = VirtualAllocEx(hProcess, NULL, remInjectSize, MEM_COMMIT, PAGE_READWRITE);
						remStringData = VirtualAllocEx(hProcess, NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
						RtlZeroMemory(&APIData, sizeof InjectImpRecCodeData);
						APIData.fTrace = fImpRecTrace + (ULONG_PTR)ImporterGetRemoteDLLBase(hProcess, hImpRecModule);
						APIData.AddressToTrace = (ULONG_PTR)TraceAddress;
						APIData.fCreateFileA = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateFileA"));
						APIData.fCreateFileMappingA = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateFileMappingA"));
						APIData.fCloseHandle = (ULONG_PTR)ImporterGetRemoteAPIAddress(hProcess, (ULONG_PTR)GetProcAddress(GetModuleHandleA("kernel32.dll"), "CloseHandle"));
						if(WriteProcessMemory(hProcess, remCodeData, (LPCVOID)&injectedImpRec, remInjectSize, &NumberOfBytesWritten)){
							WriteProcessMemory(hProcess, remStringData, &APIData, sizeof InjectImpRecCodeData, &NumberOfBytesWritten);
							WriteProcessMemory(hProcess, (LPVOID)((ULONG_PTR)remStringData + sizeof InjectImpRecCodeData), (LPCVOID)szGarbageFile, lstrlenA((LPSTR)szGarbageFile), &NumberOfBytesWritten);
							hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)remCodeData, remStringData, CREATE_SUSPENDED, &ThreadId);
							if(ZwSetInformationThread != NULL){
								cZwSetInformationThread(hThread, 0x11, NULL, NULL);
							}
							ResumeThread(hThread);
							WaitForSingleObject(hThread, INFINITE);
							if(GetExitCodeThread(hThread, &ExitCode)){
								if(ExitCode != NULL){
									if(MapFileEx((char*)szGarbageFile, UE_ACCESS_READ, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
										RtlMoveMemory(&TracedAddress, (LPVOID)FileMapVA, 4);
										UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
									}
									if(!DeleteFileA((LPCSTR)szGarbageFile)){
										HandlerCloseAllLockHandles((char*)szGarbageFile, false, true);
										DeleteFileA((LPCSTR)szGarbageFile);
									}
								}
							}
						}
						RemoteFreeLibrary(hProcess, NULL, (char*)szModuleName, true);
						VirtualFreeEx(hProcess, remCodeData, NULL, MEM_RELEASE);
						VirtualFreeEx(hProcess, remStringData, NULL, MEM_RELEASE);
					}else{
						RemoteFreeLibrary(hProcess, NULL, (char*)szModuleName, true);
					}
					FreeLibrary(hImpRecModule);
				}
			}
		}
	}
	VirtualFree(szModuleName, NULL, MEM_RELEASE);
	VirtualFree(szGarbageFile, NULL, MEM_RELEASE);
	return(TracedAddress);
}
// UnpackEngine.StaticUnpacker.functions:
__declspec(dllexport) bool __stdcall StaticFileLoad(char* szFileName, DWORD DesiredAccess, bool SimulateLoad, LPHANDLE FileHandle, LPDWORD LoadedSize, LPHANDLE FileMap, LPDWORD FileMapVA){

	if(!SimulateLoad){
		if(MapFileEx(szFileName, DesiredAccess, FileHandle, LoadedSize, FileMap, FileMapVA, NULL)){
			return(true);
		}
	}else{
		*FileMapVA = (DWORD)ResourcerLoadFileForResourceUse(szFileName);
		if(*FileMapVA != NULL){
			*LoadedSize = (DWORD)GetPE32DataFromMappedFile(*FileMapVA, NULL, UE_SIZEOFIMAGE);
			*FileHandle = NULL;
			*FileMap = NULL;
			return(true);
		}
	}
	return(false);
}
__declspec(dllexport) bool __stdcall StaticFileUnload(char* szFileName, bool CommitChanges, HANDLE FileHandle, DWORD LoadedSize, HANDLE FileMap, DWORD FileMapVA){

	DWORD PeHeaderSize;
	PIMAGE_DOS_HEADER DOSHeader;
	PIMAGE_NT_HEADERS32 PEHeader32;
	PIMAGE_NT_HEADERS64 PEHeader64;
	PIMAGE_SECTION_HEADER PESections;
	DWORD SectionNumber = 0;
	DWORD SectionRawOffset = 0;
	DWORD SectionRawSize = 0;
	BOOL FileIs64;
	HANDLE myFileHandle;
	DWORD myFileSize;
	HANDLE myFileMap;
	DWORD myFileMapVA;

	if(FileHandle != NULL && FileMap != NULL){
		UnMapFileEx(FileHandle, LoadedSize, FileMap, FileMapVA);
		return(true);
	}else{
		if(!CommitChanges){
			return(ResourcerFreeLoadedFile((LPVOID)FileMapVA));
		}else{
			if(MapFileEx(szFileName, UE_ACCESS_ALL, &myFileHandle, &myFileSize, &myFileMap, &myFileMapVA, NULL)){
				DOSHeader = (PIMAGE_DOS_HEADER)FileMapVA;
				if(DOSHeader->e_lfanew < 0x1000 - 108){
					PEHeader32 = (PIMAGE_NT_HEADERS32)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
					PEHeader64 = (PIMAGE_NT_HEADERS64)((ULONG_PTR)DOSHeader + DOSHeader->e_lfanew);
					if(PEHeader32->OptionalHeader.Magic == 0x10B){
						FileIs64 = false;
					}else if(PEHeader32->OptionalHeader.Magic == 0x20B){
						FileIs64 = true;
					}else{
						ResourcerFreeLoadedFile((LPVOID)FileMapVA);
						UnMapFileEx(myFileHandle, myFileSize, myFileMap, myFileMapVA);
						return(false);
					}
					if(!FileIs64){
						PeHeaderSize = PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_SECTION_HEADER) * PEHeader32->FileHeader.NumberOfSections;
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader32 + PEHeader32->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);
						SectionNumber = PEHeader32->FileHeader.NumberOfSections;
						RtlMoveMemory((LPVOID)myFileMapVA, (LPVOID)FileMapVA, PeHeaderSize);
						while(SectionNumber > 0){
							RtlMoveMemory((LPVOID)((ULONG_PTR)myFileMapVA + PESections->PointerToRawData), (LPVOID)(FileMapVA + PESections->VirtualAddress), PESections->SizeOfRawData);
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
							SectionNumber--;
						}
						ResourcerFreeLoadedFile((LPVOID)FileMapVA);
						UnMapFileEx(myFileHandle, myFileSize, myFileMap, myFileMapVA);
						return(true);
					}else{
						PeHeaderSize = PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_SECTION_HEADER) * PEHeader64->FileHeader.NumberOfSections;
						PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PEHeader64 + PEHeader64->FileHeader.SizeOfOptionalHeader + sizeof(IMAGE_FILE_HEADER) + 4);			
						SectionNumber = PEHeader64->FileHeader.NumberOfSections;
						RtlMoveMemory((LPVOID)myFileMapVA, (LPVOID)FileMapVA, PeHeaderSize);
						while(SectionNumber > 0){
							RtlMoveMemory((LPVOID)((ULONG_PTR)myFileMapVA + PESections->PointerToRawData), (LPVOID)(FileMapVA + PESections->VirtualAddress), PESections->SizeOfRawData);
							PESections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)PESections + IMAGE_SIZEOF_SECTION_HEADER);
							SectionNumber--;
						}
						ResourcerFreeLoadedFile((LPVOID)FileMapVA);
						UnMapFileEx(myFileHandle, myFileSize, myFileMap, myFileMapVA);
						return(true);
					}
				}else{
					ResourcerFreeLoadedFile((LPVOID)FileMapVA);
					UnMapFileEx(myFileHandle, myFileSize, myFileMap, myFileMapVA);
					return(false);
				}
			}
		}
	}
	return(false);
}
__declspec(dllexport) void __stdcall StaticMemoryDecrypt(LPVOID MemoryStart, DWORD MemorySize, DWORD DecryptionType, DWORD DecryptionKeySize, ULONG_PTR DecryptionKey){

	DWORD LoopCount = NULL;
	BYTE DataByte = NULL;
	WORD DataWord = NULL;
	DWORD DataDword = NULL;
	ULONG_PTR DataQword = NULL;

	if(MemoryStart != NULL && MemorySize > NULL){
		LoopCount = MemorySize / DecryptionKeySize;
		while(LoopCount > NULL){
			if(DecryptionType == UE_STATIC_DECRYPTOR_XOR){
				if(DecryptionKeySize == UE_STATIC_KEY_SIZE_1){
					RtlMoveMemory(&DataByte, MemoryStart, UE_STATIC_KEY_SIZE_1);
					DataByte = DataByte ^ (BYTE)DecryptionKey;
					RtlMoveMemory(MemoryStart, &DataByte, UE_STATIC_KEY_SIZE_1);
				}else if(DecryptionKeySize == UE_STATIC_KEY_SIZE_2){
					RtlMoveMemory(&DataWord, MemoryStart, UE_STATIC_KEY_SIZE_2);
					DataWord = DataWord ^ (WORD)DecryptionKey;
					RtlMoveMemory(MemoryStart, &DataWord, UE_STATIC_KEY_SIZE_2);
				}else if(DecryptionKeySize == UE_STATIC_KEY_SIZE_4){
					RtlMoveMemory(&DataDword, MemoryStart, UE_STATIC_KEY_SIZE_4);
					DataDword = DataDword ^ (DWORD)DecryptionKey;
					RtlMoveMemory(MemoryStart, &DataDword, UE_STATIC_KEY_SIZE_4);
				}else if(DecryptionKeySize == UE_STATIC_KEY_SIZE_8){
					RtlMoveMemory(&DataQword, MemoryStart, UE_STATIC_KEY_SIZE_8);
					DataQword = DataQword ^ (ULONG_PTR)DecryptionKey;
					RtlMoveMemory(MemoryStart, &DataQword, UE_STATIC_KEY_SIZE_8);
				}
			}else if(DecryptionType == UE_STATIC_DECRYPTOR_SUB){
				if(DecryptionKeySize == UE_STATIC_KEY_SIZE_1){
					RtlMoveMemory(&DataByte, MemoryStart, UE_STATIC_KEY_SIZE_1);
					DataByte = DataByte - (BYTE)DecryptionKey;
					RtlMoveMemory(MemoryStart, &DataByte, UE_STATIC_KEY_SIZE_1);
				}else if(DecryptionKeySize == UE_STATIC_KEY_SIZE_2){
					RtlMoveMemory(&DataWord, MemoryStart, UE_STATIC_KEY_SIZE_2);
					DataWord = DataWord - (WORD)DecryptionKey;
					RtlMoveMemory(MemoryStart, &DataWord, UE_STATIC_KEY_SIZE_2);
				}else if(DecryptionKeySize == UE_STATIC_KEY_SIZE_4){
					RtlMoveMemory(&DataDword, MemoryStart, UE_STATIC_KEY_SIZE_4);
					DataDword = DataDword - (DWORD)DecryptionKey;
					RtlMoveMemory(MemoryStart, &DataDword, UE_STATIC_KEY_SIZE_4);
				}else if(DecryptionKeySize == UE_STATIC_KEY_SIZE_8){
					RtlMoveMemory(&DataQword, MemoryStart, UE_STATIC_KEY_SIZE_8);
					DataQword = DataQword - (ULONG_PTR)DecryptionKey;
					RtlMoveMemory(MemoryStart, &DataQword, UE_STATIC_KEY_SIZE_8);
				}
			}else if(DecryptionType == UE_STATIC_DECRYPTOR_ADD){
				if(DecryptionKeySize == UE_STATIC_KEY_SIZE_1){
					RtlMoveMemory(&DataByte, MemoryStart, UE_STATIC_KEY_SIZE_1);
					DataByte = DataByte + (BYTE)DecryptionKey;
					RtlMoveMemory(MemoryStart, &DataByte, UE_STATIC_KEY_SIZE_1);
				}else if(DecryptionKeySize == UE_STATIC_KEY_SIZE_2){
					RtlMoveMemory(&DataWord, MemoryStart, UE_STATIC_KEY_SIZE_2);
					DataWord = DataWord + (WORD)DecryptionKey;
					RtlMoveMemory(MemoryStart, &DataWord, UE_STATIC_KEY_SIZE_2);
				}else if(DecryptionKeySize == UE_STATIC_KEY_SIZE_4){
					RtlMoveMemory(&DataDword, MemoryStart, UE_STATIC_KEY_SIZE_4);
					DataDword = DataDword + (DWORD)DecryptionKey;
					RtlMoveMemory(MemoryStart, &DataDword, UE_STATIC_KEY_SIZE_4);
				}else if(DecryptionKeySize == UE_STATIC_KEY_SIZE_8){
					RtlMoveMemory(&DataQword, MemoryStart, UE_STATIC_KEY_SIZE_8);
					DataQword = DataQword + (ULONG_PTR)DecryptionKey;
					RtlMoveMemory(MemoryStart, &DataQword, UE_STATIC_KEY_SIZE_8);
				}
			}
			MemoryStart = (LPVOID)((ULONG_PTR)MemoryStart + DecryptionKeySize);
			LoopCount--;
		}
	}
}
__declspec(dllexport) void __stdcall StaticMemoryDecryptEx(LPVOID MemoryStart, DWORD MemorySize, DWORD DecryptionKeySize, void* DecryptionCallBack){

	DWORD LoopCount = NULL;
	typedef bool(__stdcall *fStaticCallBack)(void* sMemoryStart, int sKeySize);
	fStaticCallBack myStaticCallBack = (fStaticCallBack)DecryptionCallBack;

	if(MemoryStart != NULL && MemorySize > NULL){
		LoopCount = MemorySize / DecryptionKeySize;
		while(LoopCount > NULL){
			__try{
				myStaticCallBack(MemoryStart, (int)DecryptionKeySize);
			}__except(EXCEPTION_EXECUTE_HANDLER){
				LoopCount = NULL;
			}
			MemoryStart = (LPVOID)((ULONG_PTR)MemoryStart + DecryptionKeySize);
			LoopCount--;
		}
	}
}
__declspec(dllexport) void __stdcall StaticSectionDecrypt(ULONG_PTR FileMapVA, DWORD SectionNumber, bool SimulateLoad, DWORD DecryptionType, DWORD DecryptionKeySize, ULONG_PTR DecryptionKey){

	if(!SimulateLoad){
		StaticMemoryDecrypt((LPVOID)((ULONG_PTR)GetPE32DataFromMappedFile(FileMapVA, SectionNumber, UE_SECTIONRAWOFFSET) + FileMapVA), (DWORD)GetPE32DataFromMappedFile(FileMapVA, SectionNumber, UE_SECTIONRAWSIZE), DecryptionType, DecryptionKeySize, DecryptionKey);
	}else{
		StaticMemoryDecrypt((LPVOID)((ULONG_PTR)GetPE32DataFromMappedFile(FileMapVA, SectionNumber, UE_SECTIONVIRTUALOFFSET) + FileMapVA), (DWORD)GetPE32DataFromMappedFile(FileMapVA, SectionNumber, UE_SECTIONRAWSIZE), DecryptionType, DecryptionKeySize, DecryptionKey);
	}
}
// UnpackEngine.Engine.functions:
__declspec(dllexport) void __stdcall SetEngineVariable(DWORD VariableId, bool VariableSet){

	if(VariableId == UE_ENGINE_ALOW_MODULE_LOADING){
		engineAlowModuleLoading = VariableSet;
	}else if(VariableId == UE_ENGINE_AUTOFIX_FORWARDERS){
		engineCheckForwarders = VariableSet;
	}else if(VariableId == UE_ENGINE_PASS_ALL_EXCEPTIONS){
		enginePassAllExceptions = VariableSet;
	}else if(VariableId == UE_ENGINE_NO_CONSOLE_WINDOW){
		engineRemoveConsoleForDebugee = VariableSet;
	}else if(VariableId == UE_ENGINE_BACKUP_FOR_CRITICAL_FUNCTIONS){
		engineBackupForCriticalFunctions = VariableSet;
	}
}
// Global.Garbage.functions:
bool CreateGarbageItem(void* outGargabeItem, int MaxGargabeStringSize){

	bool Created = false;
	char szGarbageItem[512];
	char szGargabeItemBuff[128];

	while(!Created){
		RtlZeroMemory(&szGarbageItem, 512);
		RtlZeroMemory(&szGargabeItemBuff, 128);
		srand((unsigned int)time(NULL));
		wsprintfA(szGargabeItemBuff, "Junk-%08x\\", (rand() % 128 + 1) * (rand() % 128 + 1) + (rand() % 1024 + 1));
		lstrcpyA(szGarbageItem, engineSzEngineGarbageFolder);
		lstrcatA(szGarbageItem, szGargabeItemBuff);
		if(CreateDirectoryA(szGarbageItem, NULL)){
			Created = true;
		}
	}
	if(lstrlenA(szGarbageItem) >= MaxGargabeStringSize){
		RtlMoveMemory(outGargabeItem, &szGarbageItem, MaxGargabeStringSize);
		return(false);
	}else{
		RtlMoveMemory(outGargabeItem, &szGarbageItem, lstrlenA(szGarbageItem));
		return(true);
	}
}
bool RemoveGarbageItem(char* szGarbageItem, bool RemoveFolder){

	char szFindSearchString[MAX_PATH];
	char szFoundFile[MAX_PATH];
	WIN32_FIND_DATAA FindData;
	bool QueryNextFile = true;
	HANDLE CurrentFile;

	if(szGarbageItem != NULL){
		lstrcpyA(szFindSearchString, szGarbageItem);
		if(szFindSearchString[0] != NULL){
			lstrcatA(szFindSearchString, "\\*.*");
			CurrentFile = FindFirstFileA(szFindSearchString, &FindData);
			while(QueryNextFile == true && CurrentFile != INVALID_HANDLE_VALUE){
				RtlZeroMemory(&szFoundFile, MAX_PATH);
				lstrcpyA(szFoundFile, szGarbageItem);
				lstrcatA(szFoundFile, FindData.cFileName);
				if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
					if(FindData.cFileName[0] != 0x2E){
						lstrcatA(szFoundFile, "\\");
						RemoveGarbageItem(szFoundFile, true);
					}
				}else{
					if(!DeleteFileA(szFoundFile)){
						if(HandlerCloseAllLockHandles(szFoundFile, false, true)){
							DeleteFileA(szFoundFile);
						}
					}
				}
				if(!FindNextFileA(CurrentFile, &FindData)){
					QueryNextFile = false;
				}
			}
			FindClose(CurrentFile);
			if(RemoveFolder){
				if(lstrlenA(engineSzEngineGarbageFolder) < lstrlenA(szGarbageItem)){
					if(!RemoveDirectoryA(szGarbageItem)){
						if(HandlerCloseAllLockHandles(szGarbageItem, true, true)){
							RemoveDirectoryA(szGarbageItem);
						}
					}
				}
			}
			return(true);
		}else{
			return(false);
		}
	}else{
		return(false);
	}
}
bool FillGarbageItem(char* szGarbageItem, char* szFileName, void* outGargabeItem, int MaxGargabeStringSize){

	char szCopyFileName[512];

	lstrcpyA(szCopyFileName, szGarbageItem);
	lstrcatA(szCopyFileName, EngineExtractFileName(szFileName));
	if(lstrlenA(szCopyFileName) >= MaxGargabeStringSize){
		RtlMoveMemory(outGargabeItem, &szCopyFileName, MaxGargabeStringSize);
		CopyFileA(szFileName, szCopyFileName, false);
	}else{
		RtlMoveMemory(outGargabeItem, &szCopyFileName, lstrlenA(szCopyFileName));
		CopyFileA(szFileName, szCopyFileName, false);
	}
	return(true);
}
void EmptyGarbage(){
	RemoveGarbageItem(engineSzEngineGarbageFolder, false);
}
// Global.Engine.Functions:
void EngineInit(){

	int i;

	RtlZeroMemory(&engineSzEngineFile, MAX_PATH);
	RtlZeroMemory(&engineSzEngineFolder, MAX_PATH);
	if(GetModuleFileNameA(engineHandle, engineSzEngineFile, MAX_PATH) > NULL){
		lstrcpyA(engineSzEngineFolder, engineSzEngineFile);
		i = lstrlenA(engineSzEngineFolder);
		while(i > NULL && engineSzEngineFolder[i] != 0x5C){
			engineSzEngineFolder[i] = 0x00;
			i--;
		}
		if(i > NULL){
			lstrcpyA(engineSzEngineGarbageFolder, engineSzEngineFolder);
			lstrcatA(engineSzEngineGarbageFolder, "garbage\\");
		}
	}
}
// Global.Engine.Entry:
bool APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved){

	int i;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		engineHandle = hModule;
		if(sizeof HANDLE != 4){
			engineCurrentPlatform = UE_PLATFORM_x64;
		}
		EngineInit();	
		EmptyGarbage();
		for(i = 0; i < UE_MAX_RESERVED_MEMORY_LEFT; i++){
			engineReservedMemoryLeft[i] = NULL;
		}

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}