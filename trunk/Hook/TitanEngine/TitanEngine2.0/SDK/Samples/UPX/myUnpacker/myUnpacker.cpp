// myUnpacker.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "myUnpacker.h"
#include "sdk\SDK.h"

#define UE_ACCESS_READ 0
#define UE_ACCESS_WRITE 1
#define UE_ACCESS_ALL 2
#define ID_TIMEREVENT 101
// Global Variables:
HWND BoxHandle;
HWND WindowHandle;
bool fdFileIsDll = false;
long fdImageBase = NULL;
long fdLoadedBase = NULL;
long fdEntryPoint = NULL;
long fdSizeOfImage = NULL;
long SnapshootMemorySize = NULL;
long SnapshootMemoryStartRVA = NULL;
bool UnpackerRunning = false;
char SnapShoot1[1024] = {};
char SnapShoot2[1024] = {};
char GlobalBuffer[1024] = {};
char szReadStringData[256] = {};
char GlobalBackBuffer[1024] = {};
char GlobalTempBuffer[1024] = {};
char UnpackFileNameBuffer[1024] = {};
char GlobalUnpackerFolderBuffer[1024] = {};
HMODULE hInstance = GetModuleHandleA(NULL);
LPPROCESS_INFORMATION fdProcessInfo = NULL;
bool dtSecondSnapShootOnEP = false;
void* cbInitCallBack = NULL;

// Global Functions:
bool MapFileEx(char*, DWORD, LPHANDLE, LPDWORD, LPHANDLE, LPDWORD, DWORD);
void UnMapFileEx(HANDLE, DWORD, HANDLE, DWORD);
void cbCreateProcess(LPCREATE_PROCESS_DEBUG_INFO);
void InitializeUnpacker(char* szFileName, void* CallBack);
bool ExtractResource(char*, char*);
void AddLogMessage(char*);
void GetFileDialog();
void cbLoadLibrary();
void cbGetProcAddress();
void cbEntryPoint();
void cbMakeSnapShoot1();
void cbFindPatterns();

// Unpacker Data:
BYTE glWildCard = 0x00;
int dtPattern1Size = 5;
BYTE dtPattern1[5] = {0x50,0x83,0xC7,0x08,0xFF};
void* dtPattern1CallBack = &cbLoadLibrary;
void* dtPattern1BPXAddress = NULL;

int dtPattern2Size = 7;
BYTE dtPattern2[7] = {0x50,0x47,0x00,0x57,0x48,0xF2,0xAE};
void* dtPattern2CallBack = &cbGetProcAddress;
void* dtPattern2BPXAddress = NULL;

int dtPattern3Size = 6;
BYTE dtPattern3[6] = {0x57,0x48,0xF2,0xAE,0x00,0xFF};
void* dtPattern3CallBack = &cbGetProcAddress;
void* dtPattern3BPXAddress = NULL;

int dtPattern4Size = 8;
BYTE dtPattern4[8] = {0x89,0xF9,0x57,0x48,0xF2,0xAE,0x52,0xFF};
void* dtPattern4CallBack = &cbGetProcAddress;
void* dtPattern4BPXAddress = NULL;

int dtPattern5Size = 2;
BYTE dtPattern5[2] = {0x61,0xE9};
void* dtPattern5CallBack = &cbEntryPoint;
void* dtPattern5BPXAddress = NULL;

int dtPattern51Size = 4;
BYTE dtPattern51[4] = {0x83,0xEC,0x00,0xE9};
void* dtPattern51CallBack = &cbEntryPoint;
void* dtPattern51BPXAddress = NULL;

int dtPattern6Size = 43;
BYTE dtPattern6[43] = {0x31,0xC0,0x8A,0x07,0x47,0x09,0xC0,0x74,0x22,0x3C,0xEF,0x77,0x11,0x01,\
					   0xC3,0x8B,0x03,0x86,0xC4,0xC1,0xC0,0x10,0x86,0xC4,0x01,0xF0,0x89,0x03,\
					   0xEB,0xE2,0x24,0x0F,0xC1,0xE0,0x10,0x66,0x8B,0x07,0x83,0xC7,0x02,0xEB,0xE2};
void* dtPattern6CallBack = &cbMakeSnapShoot1;
void* dtPattern6BPXAddress = NULL;

// Forward declarations of functions included in this code module:
INT_PTR CALLBACK WndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){

	char szDlgTitle[] = "[UPX 1.x - 3.x Unpacker]";
	char szAboutText[] = "RL!deUPX 1.x - 3.x unpacker \r\n\r\n Visit Reversing Labs at http://www.reversinglabs.com \r\n\r\n  Minimum engine version needed:\r\n- TitanEngine 2.0 by RevLabs\r\n\r\nUnpacker coded by Reversing Labs";
	char szAboutTitle[] = "[ About ]";

	if(uMsg == WM_INITDIALOG){
		SendMessageA(hwndDlg, WM_SETTEXT, NULL, (LPARAM)&szDlgTitle);
		SendMessageA(hwndDlg, WM_SETICON, NULL, (LPARAM)LoadIconA((HINSTANCE)hInstance, MAKEINTRESOURCEA(IDI_ICON1)));
		SetDlgItemTextA(hwndDlg, IDC_FILENAME, "filename.exe");
		CheckDlgButton(hwndDlg, IDC_REALING, 1);
		WindowHandle = hwndDlg;
	}else if(uMsg == WM_DROPFILES){
		DragQueryFileA((HDROP)wParam, NULL, GlobalBuffer, 1024);
		SetDlgItemTextA(hwndDlg, IDC_FILENAME, GlobalBuffer);
	}else if(uMsg == WM_CLOSE){
		EndDialog(hwndDlg, NULL);
	}else if(uMsg == WM_COMMAND){
		if(wParam == IDC_UNPACK){
			if(!UnpackerRunning){
				UnpackerRunning = true;
				BoxHandle = GetDlgItem(hwndDlg, IDC_LISTBOX);
				SendMessageA(BoxHandle, LB_RESETCONTENT, NULL, NULL);
				InitializeUnpacker(GlobalBuffer, &cbFindPatterns);
				UnpackerRunning = false;
			}
		}else if(wParam == IDC_BROWSE){
			GetFileDialog();
			if(GlobalBuffer[0] != 0x00){
				SetDlgItemTextA(hwndDlg, IDC_FILENAME, GlobalBuffer);
			}
		}else if(wParam == IDC_ABOUT){
			MessageBoxA(hwndDlg, szAboutText, szAboutTitle, MB_ICONASTERISK);
		}else if(wParam == IDC_EXIT){
			EndDialog(hwndDlg, NULL);
		}
	}
	return(NULL);
}
void GetUnpackerFolder(){

	int i;

	if(GetModuleFileNameA((HMODULE)hInstance, GlobalUnpackerFolderBuffer, 1024) > NULL){
		i = lstrlenA(GlobalUnpackerFolderBuffer);
		while(GlobalUnpackerFolderBuffer[i] != 0x5C){
			GlobalUnpackerFolderBuffer[i] = 0x00;
			i--;
		}
	}
}

void GetFileDialog(){

	OPENFILENAMEA sOpenFileName;
	char szFilterString[] = "All Files \0*.*\0\0";
	char szDialogTitle[] = "RL!deUPX 1.x - 3.x from Reversing Labs";

	RtlZeroMemory(&sOpenFileName, sizeof(OPENFILENAMEA)); 
	sOpenFileName.lStructSize = sizeof(OPENFILENAMEA);
	sOpenFileName.lpstrFilter = &szFilterString[0];
	sOpenFileName.lpstrFile = &GlobalBuffer[0];
	sOpenFileName.nMaxFile = 1024;
	sOpenFileName.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER | OFN_HIDEREADONLY;
	sOpenFileName.lpstrTitle = &szDialogTitle[0];
	if(!GetOpenFileNameA(&sOpenFileName)){
		RtlZeroMemory(&GlobalBuffer[0], 1024);
	}
}
void AddLogMessage(char* szLogMessage){

	int cSelect;

	SendMessageA(BoxHandle, LB_ADDSTRING, NULL, (LPARAM)szLogMessage);
	cSelect = (int)SendMessageA(BoxHandle, LB_GETCOUNT, NULL, NULL);
	cSelect--;
	SendMessageA(BoxHandle, LB_SETCURSEL, (WPARAM)cSelect, NULL);
}
void InitializeUnpacker(char* szFileName, void* CallBack){

	int i,j;
	FILE_STATUS_INFO inFileStatus = {};
	char szTempFolder[1024] = {};

	if(szFileName != NULL){
		AddLogMessage("-> Unpack started...");
		if(IsPE32FileValidEx(szFileName, UE_DEPTH_DEEP, &inFileStatus)){
			cbInitCallBack = CallBack;
			fdImageBase = (long)GetPE32Data(szFileName, NULL, UE_IMAGEBASE);
			fdEntryPoint = (long)GetPE32Data(szFileName, NULL, UE_OEP);
			fdSizeOfImage = (long)GetPE32Data(szFileName, NULL, UE_SIZEOFIMAGE);

			SnapshootMemoryStartRVA = (long)GetPE32Data(szFileName, NULL, UE_SECTIONVIRTUALOFFSET);
			SnapshootMemorySize = fdEntryPoint - SnapshootMemoryStartRVA;
			dtSecondSnapShootOnEP = true;

			lstrcpyA(GlobalBackBuffer, GlobalBuffer);
			i = lstrlenA(GlobalBackBuffer);
			while(GlobalBackBuffer[i] != 0x2E){
				i--;
			}
			GlobalBackBuffer[i] = 0x00;
			j = i + 1;
			while(GlobalBackBuffer[i] != 0x5C){
				i--;
			}
			i++;
			wsprintfA(GlobalTempBuffer, "%s.unpacked.%s", &GlobalBackBuffer[i], &GlobalBackBuffer[j]);
			GlobalBackBuffer[i] = 0x00;
			lstrcpyA(UnpackFileNameBuffer, GlobalBackBuffer);
			lstrcatA(UnpackFileNameBuffer, GlobalTempBuffer);

			fdFileIsDll = inFileStatus.FileIsDLL;
			if(!fdFileIsDll){
				fdProcessInfo = (LPPROCESS_INFORMATION)InitDebug(szFileName, NULL, NULL);
			}else{
				GetTempPathA(1024, szTempFolder);
				lstrcpyA(SnapShoot1, szTempFolder);
				lstrcatA(SnapShoot1, "snapshoot.1");
				lstrcpyA(SnapShoot2, szTempFolder);
				lstrcatA(SnapShoot2, "snapshoot.2");
				fdProcessInfo = (LPPROCESS_INFORMATION)InitDLLDebug(szFileName, true, NULL, NULL, CallBack);
			}
			if(fdProcessInfo != NULL){
				if(!fdFileIsDll){
					SetCustomHandler(UE_CH_CREATEPROCESS, &cbCreateProcess);
				}
				DebugLoop();
			}else{
				AddLogMessage("[Error]");
				AddLogMessage("-> Unpack ended...");
			}
		}else{
			AddLogMessage("[Error] Selected file is not a valid PE32 file!");
			AddLogMessage("-> Unpack ended...");
		}
	}
}
void cbCreateProcess(LPCREATE_PROCESS_DEBUG_INFO lpCreateProcInfo){

	fdLoadedBase = (long)lpCreateProcInfo->lpBaseOfImage;
	SetCustomHandler(UE_CH_CREATEPROCESS, NULL);
	SetBPX(fdLoadedBase + fdEntryPoint, UE_BREAKPOINT, cbInitCallBack);
	ImporterInit(50 * 1024, fdLoadedBase);
}
void cbLoadLibrary(){

	long cAddress;
	long cReadSize;
	long cTargetAddress;
	SIZE_T NumberOfBytes;
	MEMORY_BASIC_INFORMATION MemInfo;

	cAddress = (long)GetContextData(UE_EIP);
	if((long)dtPattern1BPXAddress == cAddress){
		cTargetAddress = (long)GetContextData(UE_EAX);
	}
	if(cTargetAddress > fdLoadedBase){
		VirtualQueryEx(fdProcessInfo->hProcess, (void*)cTargetAddress, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION));
		cReadSize = (long)MemInfo.BaseAddress + MemInfo.RegionSize;
		VirtualQueryEx(fdProcessInfo->hProcess, (void*)((long)MemInfo.BaseAddress + MemInfo.RegionSize), &MemInfo, sizeof(MEMORY_BASIC_INFORMATION));
		cReadSize = cReadSize + MemInfo.RegionSize;
		cReadSize = cReadSize - cAddress;
		if(cReadSize > 256){
			cReadSize = 256;
		}
		if(ReadProcessMemory(fdProcessInfo->hProcess, (void*)cTargetAddress, &szReadStringData, cReadSize, &NumberOfBytes)){
			ImporterAddNewDll(szReadStringData, NULL);
			RtlZeroMemory(&GlobalTempBuffer, 1024);
			wsprintfA(GlobalTempBuffer,"[x] LoadLibrary BPX -> %s",szReadStringData);
			AddLogMessage(GlobalTempBuffer);
		}
	}
}
void cbGetProcAddress(){

	long cThunk;
	long cAddress;
	long cReadSize;
	long cTargetAddress;
	SIZE_T NumberOfBytes;
	MEMORY_BASIC_INFORMATION MemInfo;

	cAddress = (long)GetContextData(UE_EIP);
	if((long)dtPattern2BPXAddress == cAddress){
		cTargetAddress = (long)GetContextData(UE_EAX);
		cThunk = (long)GetContextData(UE_EBX);
	}else if((long)dtPattern3BPXAddress == cAddress){
		cTargetAddress = (long)GetContextData(UE_EDI);
		cThunk = (long)GetContextData(UE_EBX);
	}else if((long)dtPattern4BPXAddress == cAddress){
		cTargetAddress = (long)GetContextData(UE_EDI);
		cThunk = (long)GetContextData(UE_EBX);
	}
	if(cTargetAddress > fdLoadedBase){
		VirtualQueryEx(fdProcessInfo->hProcess, (void*)cTargetAddress, &MemInfo, sizeof(MEMORY_BASIC_INFORMATION));
		cReadSize = (long)MemInfo.BaseAddress + MemInfo.RegionSize;
		VirtualQueryEx(fdProcessInfo->hProcess, (void*)((long)MemInfo.BaseAddress + MemInfo.RegionSize), &MemInfo, sizeof(MEMORY_BASIC_INFORMATION));
		cReadSize = cReadSize + MemInfo.RegionSize;
		cReadSize = cReadSize - cAddress;
		if(cReadSize > 256){
			cReadSize = 256;
		}
		if(ReadProcessMemory(fdProcessInfo->hProcess, (void*)cTargetAddress, &szReadStringData, cReadSize, &NumberOfBytes)){
			ImporterAddNewAPI(szReadStringData, cThunk);
			RtlZeroMemory(&GlobalTempBuffer, 1024);
			wsprintfA(GlobalTempBuffer,"[x] GetProcAddress BPX -> %s",szReadStringData);
			AddLogMessage(GlobalTempBuffer);
		}
	}else{
		ImporterAddNewAPI((char*)cTargetAddress, cThunk);
		RtlZeroMemory(&GlobalTempBuffer, 1024);
		wsprintfA(GlobalTempBuffer,"[x] GetProcAddress BPX -> %08X",cTargetAddress);
		AddLogMessage(GlobalTempBuffer);
	}
}
void cbEntryPoint(){

	int i;
	long cAddress;	
	int UnpackedOEP;
	HANDLE FileHandle;
	DWORD FileSize;
	HANDLE FileMap;
	DWORD FileMapVA;
	SIZE_T NumberOfBytes;
	long mImportTableOffset;
	long mRelocTableOffset;
	DWORD pOverlayStart;
	DWORD pOverlaySize;

	cAddress = (long)GetContextData(UE_EIP);
	__try{
		if(ReadProcessMemory(fdProcessInfo->hProcess, (void*)(cAddress + 1), &UnpackedOEP, 4, &NumberOfBytes)){
			UnpackedOEP = UnpackedOEP + cAddress + 5;
		}
		RtlZeroMemory(&GlobalTempBuffer, 1024);
		wsprintfA(GlobalTempBuffer,"[x] Entry Point at: %08X",UnpackedOEP);
		AddLogMessage(GlobalTempBuffer);

		if(!fdFileIsDll){
			PastePEHeader(fdProcessInfo->hProcess, (void*)fdImageBase, GlobalBuffer);
			AddLogMessage("[x] Paste PE32 header!");
		}else{
			if(dtSecondSnapShootOnEP){
				RelocaterMakeSnapshot(fdProcessInfo->hProcess, SnapShoot2, (void*)(SnapshootMemoryStartRVA + fdLoadedBase), SnapshootMemorySize);
			}
			RelocaterCompareTwoSnapshots(fdProcessInfo->hProcess, fdLoadedBase, fdSizeOfImage, SnapShoot1, SnapShoot2, SnapshootMemoryStartRVA + fdLoadedBase);
			DeleteFileA(SnapShoot1);
			DeleteFileA(SnapShoot2);
		}
		DumpProcess(fdProcessInfo->hProcess, (void*)fdLoadedBase, UnpackFileNameBuffer, UnpackedOEP);
		AddLogMessage("[x] Process dumped!");
		StopDebug();
		mImportTableOffset = AddNewSection(UnpackFileNameBuffer, "TEv20", ImporterEstimatedSize() + 200) + fdLoadedBase;
		if(fdFileIsDll){
			mRelocTableOffset = AddNewSection(UnpackFileNameBuffer, "TEv20", RelocaterEstimatedSize() + 200);
		}
		if(MapFileEx(UnpackFileNameBuffer, UE_ACCESS_ALL, &FileHandle, &FileSize, &FileMap, &FileMapVA, NULL)){
			ImporterExportIAT((ULONG_PTR)ConvertVAtoFileOffset(FileMapVA, mImportTableOffset, true), FileMapVA);
			AddLogMessage("[x] IAT has been fixed!");
			if(fdFileIsDll){
				RelocaterExportRelocation((ULONG_PTR)ConvertVAtoFileOffset(FileMapVA, mRelocTableOffset + fdLoadedBase, true), mRelocTableOffset, FileMapVA);
				AddLogMessage("[x] Exporting relocations!");
			}
			if(IsDlgButtonChecked(WindowHandle, IDC_REALING)){
				FileSize = RealignPE(FileMapVA, FileSize, 2);
				AddLogMessage("[x] Realigning file!");
			}
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
			MakeAllSectionsRWE(UnpackFileNameBuffer);
			if(fdFileIsDll){
				RelocaterChangeFileBase(UnpackFileNameBuffer, fdImageBase);
				AddLogMessage("[x] Rebase file image!");
			}
			if(FindOverlay(GlobalBuffer, &pOverlayStart, &pOverlaySize)){
				AddLogMessage("[x] Moving overlay to unpacked file!");
				CopyOverlay(GlobalBuffer, UnpackFileNameBuffer);
			}
			AddLogMessage("[Success] File has been unpacked!");

			i = lstrlenA(UnpackFileNameBuffer);
			while(UnpackFileNameBuffer[i] != 0x5C){
				i--;
			}
			i++;
			RtlZeroMemory(&GlobalTempBuffer, 1024);
			wsprintfA(GlobalTempBuffer,"[x] File has been unpacked to: %s",&UnpackFileNameBuffer[i]);
			AddLogMessage(GlobalTempBuffer);
			AddLogMessage("-> Unpack ended...");
		}else{
			AddLogMessage("[Fatal Unpacking Error] Please mail file you tried to unpack to Reversing Labs!");
			AddLogMessage("-> Unpack ended...");
		}
	}__except(EXCEPTION_EXECUTE_HANDLER){
		ForceClose();
		ImporterCleanup();
		if(FileMapVA > NULL){
			UnMapFileEx(FileHandle, FileSize, FileMap, FileMapVA);
		}
		DeleteFileA(UnpackFileNameBuffer);
		AddLogMessage("[Fatal Unpacking Error] Please mail file you tried to unpack to Reversing Labs!");
		AddLogMessage("-> Unpack ended...");
	}
}
void cbMakeSnapShoot1(){

	RelocaterMakeSnapshot(fdProcessInfo->hProcess, SnapShoot1, (void*)(SnapshootMemoryStartRVA + fdLoadedBase), SnapshootMemorySize);
}
void cbFindPatterns(){

	bool DontLog = false;
	long cReadSize = NULL;
	MEMORY_BASIC_INFORMATION MemInfo;

	if(fdFileIsDll){
		fdLoadedBase = (long)GetDebuggedDLLBaseAddress();
		ImporterInit(50 * 1024, fdLoadedBase);
		RelocaterInit(100 * 1024, fdImageBase, fdLoadedBase);
	}
	VirtualQueryEx(fdProcessInfo->hProcess, (void*)(fdLoadedBase + fdEntryPoint), &MemInfo, sizeof(MEMORY_BASIC_INFORMATION));
	cReadSize = (long)MemInfo.BaseAddress + MemInfo.RegionSize - (fdLoadedBase + fdEntryPoint);

	dtPattern1BPXAddress = (void*)Find((void*)(fdLoadedBase + fdEntryPoint), cReadSize, &dtPattern1[0], dtPattern1Size, &glWildCard);
	if(dtPattern1BPXAddress != NULL){
		SetBPX((long)dtPattern1BPXAddress, UE_BREAKPOINT, dtPattern1CallBack);
	}else{
		if(!DontLog){
			AddLogMessage("[Error] File is not packed with UPX 1.x - 3.x");
			AddLogMessage("-> Unpack ended...");
			StopDebug();
			DontLog = true;
		}
	}

	dtPattern2BPXAddress = (void*)Find((void*)(fdLoadedBase + fdEntryPoint), cReadSize, &dtPattern2[0], dtPattern2Size, &glWildCard);
	if(dtPattern2BPXAddress != NULL){
		SetBPX((long)dtPattern2BPXAddress, UE_BREAKPOINT, dtPattern2CallBack);
	}

	dtPattern3BPXAddress = (void*)Find((void*)(fdLoadedBase + fdEntryPoint), cReadSize, &dtPattern3[0], dtPattern3Size, &glWildCard);
	if(dtPattern3BPXAddress != NULL){
		SetBPX((long)dtPattern3BPXAddress, UE_BREAKPOINT, dtPattern3CallBack);
	}else{
		if(!DontLog){
			AddLogMessage("[Error] File is not packed with UPX 1.x - 3.x");
			AddLogMessage("-> Unpack ended...");
			StopDebug();
			DontLog = true;
		}
	}

	dtPattern4BPXAddress = (void*)Find((void*)(fdLoadedBase + fdEntryPoint), cReadSize, &dtPattern4[0], dtPattern4Size, &glWildCard);
	if(dtPattern4BPXAddress != NULL){
		dtPattern4BPXAddress = (void*)((long)dtPattern4BPXAddress + 2);
		SetBPX((long)dtPattern4BPXAddress, UE_BREAKPOINT, dtPattern4CallBack);
	}

	dtPattern5BPXAddress = (void*)Find((void*)(fdLoadedBase + fdEntryPoint), cReadSize, &dtPattern5[0], dtPattern5Size, &glWildCard);
	if(dtPattern5BPXAddress != NULL){
		dtPattern5BPXAddress = (void*)((long)dtPattern5BPXAddress + 1);
		SetBPX((long)dtPattern5BPXAddress, UE_BREAKPOINT, dtPattern5CallBack);
	}else{
		dtPattern51BPXAddress = (void*)Find((void*)(fdLoadedBase + fdEntryPoint), cReadSize, &dtPattern51[0], dtPattern51Size, &glWildCard);
		if(dtPattern51BPXAddress != NULL){
			dtPattern51BPXAddress = (void*)((long)dtPattern51BPXAddress + 3);
			SetBPX((long)dtPattern51BPXAddress, UE_BREAKPOINT, dtPattern51CallBack);
		}else{
			if(!DontLog){
				AddLogMessage("[Error] File is not packed with UPX 1.x - 3.x");
				AddLogMessage("-> Unpack ended...");
				StopDebug();
				DontLog = true;
			}
		}
	}

	if(fdFileIsDll){
		dtPattern6BPXAddress = (void*)Find((void*)(fdLoadedBase + fdEntryPoint), cReadSize, &dtPattern6[0], dtPattern6Size, &glWildCard);
		if(dtPattern6BPXAddress != NULL){
			dtPattern6BPXAddress = (void*)((long)dtPattern6BPXAddress - 3);
			SetBPX((long)dtPattern6BPXAddress, UE_BREAKPOINT, dtPattern6CallBack);
		}else{
			if(!DontLog){
				AddLogMessage("[Error] File is not packed with UPX 1.x - 3.x");
				AddLogMessage("-> Unpack ended...");
				StopDebug();
				DontLog = true;
			}
		}
	}
}
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
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow){

	GetUnpackerFolder();
	DialogBoxParamA(hInstance, MAKEINTRESOURCEA(IDD_MAINWINDOW), NULL, (DLGPROC)WndProc, NULL);
	ExitProcess(NULL);
}