#include "TE_Interface.h"

#include "SDK.hpp"

using namespace TE;

bool TSErrorExit = false;
fLogCallback LogCallback = NULL;

char TargetPath[MAX_PATH];
char OutputPath[MAX_PATH];
char TargetDirectory[MAX_PATH];

DWORD TE_GetProcessId()
{
	return Debugger::GetProcessInformation()->dwProcessId;
}

HANDLE TE_GetProcessHandle()
{
	return Debugger::GetProcessInformation()->hProcess;
}

DWORD TE_GetMainThreadId()
{
	return Debugger::GetProcessInformation()->dwThreadId;
}

HANDLE TE_GetMainThreadHandle()
{
	return Debugger::GetProcessInformation()->hThread;
}

DWORD TE_GetCurrentThreadId()
{
	return Debugger::GetDebugData()->dwThreadId;
}

HANDLE TE_GetCurrentThreadHandle()
{
	return Threader::GetThreadInfo(NULL, TE_GetCurrentThreadId())->hThread;
}

const char* TE_GetTargetPath()
{
	return TargetPath;
}

const char* TE_GetOutputPath()
{
	return OutputPath;
}

const char* TE_GetTargetDirectory()
{
	return TargetDirectory;
}

bool TE_ReadMemory(ULONG_PTR Address, size_t Size, void* Buffer)
{
std::vector<MEMORY_BASIC_INFORMATION> MemBlocks;
HANDLE hProcess;
size_t i = 0;
SIZE_T Read;
bool Success = true;

	if(!TE_GetMemoryInfo(Address, Size, MemBlocks))
		return false;

	hProcess = TE_GetProcessHandle();

	while(i < MemBlocks.size())
	{
		if(!(MemBlocks[i].Protect & (PAGE_NOACCESS | PAGE_EXECUTE)))
		{
			MemBlocks.erase(MemBlocks.begin() + i);
		}
		else if(!VirtualProtectEx(hProcess, MemBlocks[i].BaseAddress, MemBlocks[i].RegionSize, PAGE_READONLY, &MemBlocks[i].Protect))
		{
			Success = false;
			break;
		}
		else i++;
	}

	Success = Success && ReadProcessMemory(hProcess, (void*)Address, Buffer, Size, &Read) && (Read == Size);

	while(i > 0)
	{
		i--;
		VirtualProtectEx(hProcess, MemBlocks[i].BaseAddress, MemBlocks[i].RegionSize, MemBlocks[i].Protect, &MemBlocks[i].Protect);
	}

	return Success;
}

bool TE_WriteMemory(ULONG_PTR Address, size_t Size, const void* Buffer)
{
std::vector<MEMORY_BASIC_INFORMATION> MemBlocks;
HANDLE hProcess;
size_t i = 0;
SIZE_T Written;
bool Success = true;

	if(!TE_GetMemoryInfo(Address, Size, MemBlocks))
		return false;

	hProcess = TE_GetProcessHandle();

	while(i < MemBlocks.size())
	{
		if(MemBlocks[i].Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))
		{
			MemBlocks.erase(MemBlocks.begin() + i);
		}
		else if(!VirtualProtectEx(hProcess, MemBlocks[i].BaseAddress, MemBlocks[i].RegionSize, PAGE_READWRITE, &MemBlocks[i].Protect))
		{
			Success = false;
			break;
		}
		else i++;
	}

	Success = Success && WriteProcessMemory(hProcess, (void*)Address, Buffer, Size, &Written) && (Written == Size);

	while(i > 0)
	{
		i--;
		VirtualProtectEx(hProcess, MemBlocks[i].BaseAddress, MemBlocks[i].RegionSize, MemBlocks[i].Protect, &MemBlocks[i].Protect);
	}

	return Success;
}

bool TE_GetMemoryInfo(ULONG_PTR Address, MEMORY_BASIC_INFORMATION* MemInfo)
{
	return (0 != VirtualQueryEx(TE_GetProcessHandle(), (void*)Address, MemInfo, sizeof(*MemInfo)));
}

bool TE_GetMemoryInfo(ULONG_PTR Address, size_t Size, vector<MEMORY_BASIC_INFORMATION>& MemInfo)
{
HANDLE hProcess;
MEMORY_BASIC_INFORMATION TmpInfo;
ULONG_PTR CurAddr;

	hProcess = TE_GetProcessHandle();

	CurAddr = Address;
	while(CurAddr < (Address+Size))
	{
		if(!TE_GetMemoryInfo(CurAddr, &TmpInfo))
		{
			MemInfo.clear();
			return false;
		}
		MemInfo.push_back(TmpInfo);
		CurAddr = (ULONG_PTR)TmpInfo.BaseAddress + TmpInfo.RegionSize;
	}
	return true;
}

ULONG_PTR TE_AllocMemory(size_t Size)
{
	return (ULONG_PTR)VirtualAllocEx(TE_GetProcessHandle(), NULL, Size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

bool TE_FreeMemory(ULONG_PTR Address)
{
MEMORY_BASIC_INFORMATION MemInfo;

	if(TE_GetMemoryInfo(Address, &MemInfo))
	{
		return (TRUE == VirtualFreeEx(TE_GetProcessHandle(), MemInfo.AllocationBase, 0, MEM_RELEASE));
	}
	return false;
}

bool TE_FreeMemory(ULONG_PTR Address, size_t Size)
{
	return (TRUE == VirtualFreeEx(TE_GetProcessHandle(), (void*)Address, Size, MEM_DECOMMIT));
}

bool TE_GetModules(vector<MODULEENTRY32>& Modules)
{
bool Success = false;

	HANDLE ModuleShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, TE_GetProcessId());
	if(ModuleShot != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 ModInfo;
		ModInfo.dwSize = sizeof(MODULEENTRY32);

		if(Module32First(ModuleShot, &ModInfo))
		{
			do
			{
				Modules.push_back(ModInfo);
			}
			while(Module32Next(ModuleShot, &ModInfo));
			Success = true;
		}
		CloseHandle(ModuleShot);
	}
	return Success;
}

void TE_SetLogCallback(fLogCallback Callback)
{
	LogCallback = Callback;
}

void TE_Log(const char* String, eLogType Type)
{
	if(LogCallback)
	{
		LogCallback(String, Type);
	}
}
