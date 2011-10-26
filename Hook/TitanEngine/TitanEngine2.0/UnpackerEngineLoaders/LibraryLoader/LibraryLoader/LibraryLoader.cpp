// LibraryLoader.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "LibraryLoader.h"

char szLibraryName[512];
char szFileName[512];

void ReserveImageBase(){

	HANDLE hFile;

	lstrcpyA((LPSTR)szFileName, (LPCSTR)szLibraryName);
	lstrcatA((LPSTR)szFileName, ".module");
	hFile = CreateFileA((LPCSTR)szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE){
		CloseHandle(hFile);
		LoadLibraryA((LPCSTR)szFileName);
	}
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow){

	ReserveImageBase();
	if(LoadLibraryA((LPCSTR)szLibraryName) == NULL){
		//DeleteFileA((LPCSTR)szFileName);
		ExitProcess(0x61703078);
	}else{
		//DeleteFileA((LPCSTR)szFileName);
		ExitProcess(NULL);
	}
}