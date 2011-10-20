/*
 * WenTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Precomp.h"

Kernel32::Kernel32()
: Image(TEXT("KERNEL32.DLL"))
{
}

Kernel32::~Kernel32()
{
}

//
// The KERNEL32 synchronize routine is responsible for building out the address
// table for the following symbols:
//
// 2000/XP/2003:
//
//   - BaseThreadStartThunk
//   - BaseProcessStartThunk
//
// Neither of these symbols are exported and as such interesting means must be
// used to locate their addresses.
//
DWORD Kernel32::Synchronize()
{
	DWORD Result;

	do
	{
		//
		// Open the registry key and load the image into memory
		//
		if (((Result = Open()) != ERROR_SUCCESS) ||
		    ((Result = LoadLibrary()) != ERROR_SUCCESS))
			break;

		//
		// Flush the existing address table
		//
		FlushAddressTable();

		//
		// Try to locate 'BaseThreadStartThunk'
		//
		if ((Result = ResolveBaseThreadStartThunk()) != ERROR_SUCCESS)
			break;

		//
		// Try to locate 'BaseProcessStartThunk'
		//
		if ((Result = ResolveBaseProcessStartThunk()) != ERROR_SUCCESS)
			break;

		//
		// Set the flags for kernel32 to indicate that jump tables have to be
		// built relative to every other image set as processes that create
		// threads or start process will need to make sure that jumps have been
		// built at the proper addresses in the target process in order for things
		// to not explode.
		//
		if ((Result = SetFlags(
				IMAGE_FLAG_SET_RELATIVE_JUMP_TABLE)) != ERROR_SUCCESS)
			break;

	} while (0);

	//
	// If the extended synchronization was successful, synchronize the rest of
	// the image's information
	//
	return (Result == ERROR_SUCCESS)
		? Image::Synchronize()
		: Result;
}

////
// 
// Protected Methods
//
////

//
// Resolve the virtual address of the 'BaseThreadStartThunk' symbol by using
// CreateThread with a suspended thread and inspecting EIP to get the address of
// the thread entry point (which is BaseThreadStartThunk).
//
DWORD Kernel32::ResolveBaseThreadStartThunk()
{
	CONTEXT ThreadContext;
	HANDLE  ThreadHandle = NULL;
	DWORD   Result;

	do
	{
		//
		// Create a suspended thread
		//
		if (!(ThreadHandle = CreateRemoteThread(
				(HANDLE)-1,
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)DummyThreadFunction,
				NULL,
				CREATE_SUSPENDED,
				NULL)))
		{
			Result = GetLastError();
			break;
		}

		//
		// Get the suspended thread's context
		//
		ThreadContext.ContextFlags = CONTEXT_CONTROL;

		if (!GetThreadContext(
				ThreadHandle,
				&ThreadContext))
		{
			Result = GetLastError();
			break;
		}

		//
		// Make sure the address is valid
		//
		if (!ThreadContext.Eip)
		{
			Result = ERROR_INVALID_FUNCTION;
			break;
		}

		Result = AddAddressTableEntry(
				(PVOID)ThreadContext.Eip,
				TEXT("BaseThreadStartThunk"));

	} while (0);

	//
	// If the thread handle is valid, kill it.
	//
	if (ThreadHandle)
		TerminateThread(
				ThreadHandle,
				0);

	return Result;
}

//
// Resolve the virtual address of BaseProcessStartThunk in much the same fashion as
// BaseThreadStartThunk except instead of creating a thread in the context of this
// process, we must instead start another process in the suspended state and
// inspect EIP of the thread that is created.  EIP should point to
// BaseProcessStartThunk.
//
DWORD Kernel32::ResolveBaseProcessStartThunk()
{
	PROCESS_INFORMATION ProcessInformation;
	STARTUPINFO         StartupInformation;
	CONTEXT             ThreadContext;
	DWORD               Result;

	ZeroMemory(
			&StartupInformation,
			sizeof(StartupInformation));

	//
	// Initialize the startup information to hide the process' window
	//
	StartupInformation.cb          = sizeof(StartupInformation);
	StartupInformation.dwFlags     = STARTF_USESHOWWINDOW |
	                                 STARTF_USESTDHANDLES;
	StartupInformation.wShowWindow = SW_HIDE;
	StartupInformation.hStdInput   = NULL;
	StartupInformation.hStdOutput  = NULL;
	StartupInformation.hStdError   = NULL;

	do
	{
		//
		// Create a dummy cmd.exe instance
		//
		if (!CreateProcess(
				NULL,
				TEXT("cmd.exe"),
				NULL,
				NULL,
				FALSE,
				CREATE_SUSPENDED,
				NULL,
				NULL,
				&StartupInformation,
				&ProcessInformation))
		{
			Result = GetLastError();
			break;
		}

		//
		// Get the thread context that was created inside the other process
		//
		ThreadContext.ContextFlags = CONTEXT_CONTROL;

		if (!GetThreadContext(
				ProcessInformation.hThread,
				&ThreadContext))
		{
			Result = GetLastError();
			break;
		}

		//
		// Make sure the EIP is valid
		//
		if (!ThreadContext.Eip)
		{
			Result = ERROR_INVALID_FUNCTION;
			break;
		}
	
		//
		// Create the address table entry for the symbol
		//
		Result = AddAddressTableEntry(
				(PVOID)ThreadContext.Eip,
				TEXT("BaseProcessStartThunk"));

	} while (0);

	//
	// If the process handle is valid, kill it.
	//
	if (ProcessInformation.hProcess)
		TerminateProcess(
				ProcessInformation.hProcess,
				0);

	return ERROR_SUCCESS;
}

VOID Kernel32::DummyThreadFunction()
{
}
