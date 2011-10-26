#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include "TitanScript.h"

using std::vector;

extern bool TSErrorExit;
extern char TargetPath[MAX_PATH];
extern char OutputPath[MAX_PATH];
extern char TargetDirectory[MAX_PATH];
const DWORD PAGE_SIZE = 0x1000; // same on x86 and x64

DWORD TE_GetProcessId();
HANDLE TE_GetProcessHandle();
DWORD TE_GetMainThreadId();
HANDLE TE_GetMainThreadHandle();
DWORD TE_GetCurrentThreadId();
HANDLE TE_GetCurrentThreadHandle();
const char* TE_GetTargetPath(); // full path
const char* TE_GetOutputPath(); // full path
const char* TE_GetTargetDirectory(); // dir with trailing backslash

bool TE_ReadMemory(ULONG_PTR Address, size_t Size, void* Buffer);
bool TE_WriteMemory(ULONG_PTR Address, size_t Size, const void* Buffer);
bool TE_GetMemoryInfo(ULONG_PTR Address, MEMORY_BASIC_INFORMATION* MemInfo);
bool TE_GetMemoryInfo(ULONG_PTR Address, size_t Size, vector<MEMORY_BASIC_INFORMATION>& MemInfo);
ULONG_PTR TE_AllocMemory(size_t Size);
bool TE_FreeMemory(ULONG_PTR Address);
bool TE_FreeMemory(ULONG_PTR Address, size_t Size);

bool TE_GetModules(vector<MODULEENTRY32>& Modules);

void TE_SetLogCallback(fLogCallback Callback);
void TE_Log(const char* String, eLogType = TS_LOG_NORMAL);
