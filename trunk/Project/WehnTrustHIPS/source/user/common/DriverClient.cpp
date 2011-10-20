/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Common.h"

DWORD (__stdcall *RtlFreeUnicodeString)(
		IN PUNICODE_STRING UnicodeString) = NULL;
DWORD (__stdcall *RtlDosPathNameToNtPathName_U)(
		IN PWSTR DosPathName,
		OUT PUNICODE_STRING NtPathName,
		OUT PWSTR *FilePartInNtPathName,
		OUT PVOID NotUsed) = NULL;

//
// Opens a handle to the device driver
//
HANDLE DriverClient::Open()
{
	HANDLE Handle = CreateFile(
			TEXT("\\\\.\\wehntrust"),
			MAXIMUM_ALLOWED,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

	if (Handle == INVALID_HANDLE_VALUE)
		Handle = NULL;

	return Handle;
}

//
// Close a previously opened driver handle
//
VOID DriverClient::Close(
		IN HANDLE Driver)
{
	if (Driver)
		CloseHandle(Driver);
}

//
// Instruct the driver to start one or more selected subsystems
//
DWORD DriverClient::Start(
		IN HANDLE Driver,
		IN ULONG Subsystems)
{
	DWORD Returned;
	DWORD Result = ERROR_SUCCESS;

	if (!DeviceIoControl(
			Driver,
			IOCTL_WEHNTRUST_START,
			&Subsystems,
			sizeof(ULONG),
			NULL,
			0,
			&Returned,
			NULL))
		Result = GetLastError();

	return Result;
}

//
// Instruct the driver to stop one or more selected subsystems
//
DWORD DriverClient::Stop(
		IN HANDLE Driver,
		IN ULONG Subsystems)
{
	DWORD Returned;
	DWORD Result = ERROR_SUCCESS;

	if (!DeviceIoControl(
			Driver,
			IOCTL_WEHNTRUST_STOP,
			&Subsystems,
			sizeof(ULONG),
			NULL,
			0,
			&Returned,
			NULL))
		Result = GetLastError();

	return Result;
}

//
// Gets statistics from the driver such as number of randomized DLLs, the
// current subsystem status, and other such things.
//
DWORD DriverClient::GetStatistics(
		IN HANDLE Driver,
		OUT PWEHNTRUST_STATISTICS Statistics)
{
	DWORD Returned;
	DWORD Result = ERROR_SUCCESS;

	if (!DeviceIoControl(
			Driver,
			IOCTL_WEHNTRUST_GET_STATISTICS,
			NULL,
			0,
			Statistics,
			GetStructureSize(Statistics),
			&Returned,
			NULL))
		Result = GetLastError();

	return Result;
}

//
// Adds an exemption 
//
DWORD DriverClient::AddExemption(
		IN HANDLE Driver,
		IN EXEMPTION_SCOPE Scope,
		IN EXEMPTION_TYPE Type,
		IN ULONG Flags,
		IN LPCTSTR ExemptionPath)
{
	PWEHNTRUST_EXEMPTION_INFO ExemptionInfo = NULL;
	DWORD                     ExemptionInfoSize = 0;
	DWORD                     Returned;
	DWORD                     Result = ERROR_SUCCESS;

	do
	{
		//
		// Initialize the exemption information
		//
		if (!(ExemptionInfo = InitializeExemptionInfo(
				Scope,
				Type,
				Flags,
				ExemptionPath,
				FALSE,
				&ExemptionInfoSize)))
		{
			Result = GetLastError();
			break;
		}

		//
		// Pass it up to the driver
		//
		if (!DeviceIoControl(
				Driver,
				IOCTL_WEHNTRUST_ADD_EXEMPTION,
				ExemptionInfo,
				ExemptionInfoSize,
				NULL,
				0,
				&Returned,
				NULL))
			Result = GetLastError();

	} while (0);

	if (ExemptionInfo)
		FreeExemptionInfo(
				ExemptionInfo);

	return Result;
}

//
// Removes a global exemption
//
DWORD DriverClient::RemoveExemption(
		IN HANDLE Driver,
		IN EXEMPTION_SCOPE Scope,
		IN EXEMPTION_TYPE Type,
		IN ULONG Flags,
		IN LPCTSTR ExemptionPath,
		IN BOOLEAN IsNtPath)
{
	PWEHNTRUST_EXEMPTION_INFO ExemptionInfo = NULL;
	DWORD                     ExemptionInfoSize = 0;
	DWORD                     Returned;
	DWORD                     Result = ERROR_SUCCESS;

	do
	{
		//
		// Initialize the exemption information
		//
		if (!(ExemptionInfo = InitializeExemptionInfo(
				Scope,
				Type,
				Flags,
				ExemptionPath,
				IsNtPath,
				&ExemptionInfoSize)))
		{
			Result = GetLastError();
			break;
		}

		//
		// Pass it up to the driver
		//
		if (!DeviceIoControl(
				Driver,
				IOCTL_WEHNTRUST_REMOVE_EXEMPTION,
				ExemptionInfo,
				ExemptionInfoSize,
				NULL,
				0,
				&Returned,
				NULL))
			Result = GetLastError();

	} while (0);

	if (ExemptionInfo)
		FreeExemptionInfo(
				ExemptionInfo);

	return Result;
}

//
// Flushes all established global exemptions
//
DWORD DriverClient::FlushExemptions(
		IN HANDLE Driver,
		IN EXEMPTION_SCOPE Scope,
		IN EXEMPTION_TYPE Type)
{
	WEHNTRUST_EXEMPTION_INFO ExemptionInfo;
	DWORD                    Returned;
	DWORD                    Result = ERROR_SUCCESS;

	ZeroMemory(
			&ExemptionInfo,
			sizeof(ExemptionInfo));

	InitializeStructure(
			&ExemptionInfo,
			sizeof(ExemptionInfo));

	ExemptionInfo.Scope = Scope;
	ExemptionInfo.Type  = Type;

	if (!DeviceIoControl(
			Driver,
			IOCTL_WEHNTRUST_FLUSH_EXEMPTIONS,
			&ExemptionInfo,
			sizeof(ExemptionInfo),
			NULL,
			0,
			&Returned,
			NULL))
		Result = GetLastError();

	return Result;
}

////
//
// Protected Methods
//
////

//
// Gets the NT path associated with the file path passed in
//
BOOLEAN DriverClient::GetNtPath(
		IN LPCTSTR FilePath,
		OUT PUNICODE_STRING NtFilePath,
		IN BOOLEAN IsNtPath)
{
	BOOLEAN Allocated = FALSE;
	BOOLEAN Result = FALSE;
	DWORD   WideFilePathLength;
	PWSTR   WideFilePath;

	ZeroMemory(
			NtFilePath,
			sizeof(UNICODE_STRING));

	//
	// If we haven't resolved the RtlDosPath.. symbol, resolve it now.
	//
	if (!RtlDosPathNameToNtPathName_U)
		RtlDosPathNameToNtPathName_U = (DWORD (__stdcall *)(
				PWSTR, PUNICODE_STRING, PWSTR *, PVOID))GetProcAddress(
					GetModuleHandle(
						TEXT("NTDLL.DLL")),
					TEXT("RtlDosPathNameToNtPathName_U"));
	if (!RtlFreeUnicodeString)
		RtlFreeUnicodeString = (DWORD (__stdcall *)(
				PUNICODE_STRING))GetProcAddress(
					GetModuleHandle(
						TEXT("NTDLL.DLL")),
					TEXT("RtlFreeUnicodeString"));

	do
	{
		//
		// If we couldn't resolve the symbol...
		//
		if ((!RtlDosPathNameToNtPathName_U) ||
		    (!RtlFreeUnicodeString))
			break;

		//
		// First, convert the file path, if necessary, to a unicode string
		//
#ifdef UNICODE
		if (IsNtPath)
		{
			WideFilePathLength = (lstrlen(WideFilePath) + 1) * sizeof(TCHAR);

			if (!(WideFilePath = (PWSTR)malloc(
					WideFilePathLength)))
				break;

			ZeroMemory(
					WideFilePath,
					WideFilePathLength);

			memcpy(
					WideFilePath,
					FilePath,
					lstrlen(WideFilePath));
		}
		else
		{
			WideFilePath       = (PWSTR)FilePath;
			WideFilePathLength = lstrlen(WideFilePath);
		}
#else
		WideFilePathLength = (lstrlen(FilePath) + 1) * sizeof(WCHAR);

		if (!(WideFilePath = (PWSTR)malloc(
				WideFilePathLength)))
			break;

		Allocated = TRUE;

		ZeroMemory(
				WideFilePath,
				WideFilePathLength);

		//
		// Convert the ANSI path to the wide path
		//
		if (MultiByteToWideChar(
				CP_ACP,
				0,
				FilePath,
				-1,
				WideFilePath,
				WideFilePathLength / sizeof(WCHAR)) == 0)
			break;
#endif

		//
		// If the supplied path is already an NT path, do not try to get the nt
		// path from the dos path
		//
		if (IsNtPath)
		{
			NtFilePath->Buffer        = WideFilePath;
			NtFilePath->Length        = (USHORT)WideFilePathLength;
			NtFilePath->MaximumLength = (USHORT)WideFilePathLength;

			Result = TRUE;
			break;
		}

		//
		// Get the NT path name from the DOS path name
		//
		if (!RtlDosPathNameToNtPathName_U(
				WideFilePath,
				NtFilePath,
				NULL,
				NULL))
			break;

		Result = TRUE;

	} while (0);

	//
	// If we allocated the temporary string, deallocate it
	//
	if ((!IsNtPath) &&
	    (Allocated))
		free(
				WideFilePath);

	return Result;
}

//
// Frees the memory associated with the supplied unicode string
//
VOID DriverClient::FreeNtPath(
		IN PUNICODE_STRING NtFilePath)
{
	if ((RtlFreeUnicodeString) &&
	    (NtFilePath->Buffer))
		RtlFreeUnicodeString(
				NtFilePath);
}

//
// Initialize an exemption information context that will be passed up to the
// driver
//
PWEHNTRUST_EXEMPTION_INFO DriverClient::InitializeExemptionInfo(
		IN EXEMPTION_SCOPE Scope,
		IN EXEMPTION_TYPE Type,
		IN ULONG Flags,
		IN LPCTSTR ExemptionPath,
		IN BOOLEAN ExemptionPathIsNt,
		OUT LPDWORD ExemptionInfoSize)
{
	PWEHNTRUST_EXEMPTION_INFO ExemptionInfo = NULL;
	UNICODE_STRING            ExemptionNtPath = { 0 };
	DWORD                     Size = 0;

	//
	// Double check to see if the supplied path is an NT path
	//
	if (!StrCmpN(
			ExemptionPath,
			TEXT("\\??\\"),
			sizeof(TCHAR) * 4))
		ExemptionPathIsNt = TRUE;

	do
	{
		//
		// If we fail to acquire the NT path, break out
		//
		if (!GetNtPath(
				ExemptionPath,
				&ExemptionNtPath,
				ExemptionPathIsNt))
			break;

		Size = ExemptionNtPath.Length + sizeof(WEHNTRUST_EXEMPTION_INFO);

		//
		// If we fail to acquire storage for the exemption...
		//
		if (!(ExemptionInfo = (PWEHNTRUST_EXEMPTION_INFO)malloc(Size)))
			break;

		InitializeStructure(
				ExemptionInfo,
				sizeof(WEHNTRUST_EXEMPTION_INFO));

		ExemptionInfo->Type              = Type;
		ExemptionInfo->Scope             = Scope;
		ExemptionInfo->Flags             = Flags;
		ExemptionInfo->ExemptionPathSize = ExemptionNtPath.Length;

		memcpy(
				ExemptionInfo->ExemptionPath,
				ExemptionNtPath.Buffer,
				ExemptionNtPath.Length);

	} while (0);

	//
	// Free the NT path storage
	//
	if (ExemptionPathIsNt)
		free(
				ExemptionNtPath.Buffer);
	else
		FreeNtPath(
				&ExemptionNtPath);

	//
	// Set the out size
	//
	if (ExemptionInfoSize)
		*ExemptionInfoSize = Size;

	return ExemptionInfo;
}

//
// Frees an exemption info context
//
VOID DriverClient::FreeExemptionInfo(
		IN PWEHNTRUST_EXEMPTION_INFO ExemptionInfo)
{
	free(
			ExemptionInfo);
}
