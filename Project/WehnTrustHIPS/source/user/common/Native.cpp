#include "Common.h"
#include "psapi.h"
#undef GetProcessImageFileName

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0
#endif

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (Status == 0)
#endif

typedef enum _PROCESSINFOCLASS
{
	ProcessBasicInformation = 0,
	ProcessImageFileName    = 27
} PROCESSINFOCLASS, *PPROCESSINFOCLASS;

typedef struct _PROCESS_BASIC_INFORMATION
{
	ULONG  ExitStatus;
	PVOID  PebBaseAddress;
	ULONG  AffinityMask;
	ULONG  BasePriority;
	HANDLE UniqueProcessId;
	HANDLE InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef ULONG (NTAPI *NtQueryInformationProcessProc)(
		IN HANDLE ProcessHandle,
		IN PROCESSINFOCLASS InformationClass,
		OUT PVOID ProcessInformation,
		IN ULONG ProcessInformationLength,
		OUT PULONG ReturnLength OPTIONAL);
////
//
// Global native API dependencies.
//
////

NtQueryInformationProcessProc NtQueryInformationProcess = NULL;

//
// This function doesn't use entirely native API routines.  Make sure you pay
// attention when using it.
//
ULONG Native::GetProcessImageFileName(
		IN ULONG ProcessId,
		OUT PWCHAR ProcessFileName,
		IN ULONG ProcessFileNameLength,
		OUT PWCHAR *ProcessFilePath)
{
	PUNICODE_STRING NameUnicodeString;
	HANDLE          ProcessHandle = NULL;
	USHORT          BufferLength = 0;
	PWCHAR          Buffer = NULL;
	WCHAR           TempBuffer[1024];
	ULONG           Status;

	//
	// Try to get a handle to the process instance.
	//
	if (!(ProcessHandle = OpenProcess(
			PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
			FALSE,
			ProcessId)))
		return GetLastError();

	//
	// Resolve NtQueryInformationProcess if it hasn't been resolved yet.
	//
	if (!NtQueryInformationProcess)
		NtQueryInformationProcess = (NtQueryInformationProcessProc)GetProcAddress(
				GetModuleHandle(
					TEXT("NTDLL")),
				"NtQueryInformationProcess");

	NameUnicodeString = (PUNICODE_STRING)TempBuffer;
		
	//
	// Extract the process image file name.  If using the native API fails (as it
	// will < XP, use EnumProcessModules / GetModuleFileName.
	//
	if ((Status = NtQueryInformationProcess(
			ProcessHandle,
			ProcessImageFileName,
			NameUnicodeString,
			sizeof(TempBuffer),
			NULL)) != STATUS_SUCCESS)
	{
		HMODULE ProcessModule = NULL;
		DWORD   Needed = 0;
		DWORD   Bytes = 0;

		if ((EnumProcessModules(
				ProcessHandle,
				&ProcessModule,
				1,
				&Needed)) &&
		    ((Bytes = GetModuleFileNameExW(
				ProcessHandle,
				ProcessModule,
				TempBuffer,
				sizeof(TempBuffer) / sizeof(WCHAR))) > 0))
		{
			BufferLength = (USHORT)(Bytes * 2);
			Buffer       = (PWCHAR)TempBuffer;
			Status       = STATUS_SUCCESS; 
		}
	}
	else
	{
		BufferLength = NameUnicodeString->Length;
		Buffer       = NameUnicodeString->Buffer;
	}

	// 
	// If we succeed in getting the 
	//
	if ((NT_SUCCESS(Status)) &&
	    (Buffer))
	{
		PWCHAR Current;
		ULONG  Chars = BufferLength / sizeof(WCHAR);
		ULONG  Offset = Chars - 1;
		ULONG  Length = 0;

		//
		// Scan the path to find the executable name.
		//
		for (Current = Buffer + Offset;
			  Offset > 0;
			  Current--, Offset--, Length += sizeof(WCHAR))
		{
			if (*Current == L'\\')
			{
				Current++;
				break;
			}
		}

		//
		// Copy just the executable name, if possible.
		//
		memcpy(
				ProcessFileName,
				Current,
				Length > ProcessFileNameLength 
					? ProcessFileNameLength
					: Length);

		//
		// If requested, duplicate the image file path.
		//
		if (ProcessFilePath)
		{
			*ProcessFilePath = (PWCHAR)LocalAlloc(
					LMEM_ZEROINIT | LMEM_FIXED,
					BufferLength + sizeof(WCHAR));

			memcpy(
					*ProcessFilePath,
					Buffer,
					BufferLength);
		}
	}

	CloseHandle(
			ProcessHandle);

	return Status;
}
