
#ifndef __lib__
#define __lib__

#include <windows.h>
#include <commdlg.h>

// functions
LPVOID MapFileR(char * targetfile);   // maps a file into memory with read access
LPVOID MapFileRW(char * targetfile);  // maps a file into memory with read and write access
BOOL IsPE (LPVOID MapAddress);        // checks whether a file is a valid PE file
DWORD GetFsize(PSTR szTargetFile);    // returns the filesize
VOID ShowLastError();                 // does the GetLastError,... shit
VOID MakeOfn(OPENFILENAME &TMPofn);   // initializes a OPENFILENAME struct
int mb(char* Title,char* Text,int Style); // easier MessageBox
BOOL Hexstr2Int(CHAR* szHexStr,DWORD &dwHexNum); // returns the DWORD for a hexnumber - string
BOOL Str2Int(CHAR* szStr,DWORD &dwNum);  // returns the DWORD for a string
BOOL IsRoundedTo(DWORD dwTarNum,DWORD dwRoundNum);
BOOL IsNT();

#endif