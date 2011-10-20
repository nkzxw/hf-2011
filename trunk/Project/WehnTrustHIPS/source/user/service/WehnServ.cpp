/*
 * WehnTrust
 *
 * Copyright (c) 2005, Wehnus.
 */
#include "WehnServ.h"

#define FSCTL_PIPE_QUERY_CLIENT_PROCESS CTL_CODE(FILE_DEVICE_NAMED_PIPE, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _IO_STATUS_BLOCK 
{
	union 
	{
		ULONG Status;
		PVOID Pointer;
	};

	ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

#if defined(_WIN64)
typedef struct _IO_STATUS_BLOCK32 
{
	ULONG Status;
	ULONG Information;
} IO_STATUS_BLOCK32, *PIO_STATUS_BLOCK32;
#endif

//
// Control structure for FSCTL_PIPE_SET_CLIENT_PROCESS and FSCTL_PIPE_QUERY_CLIENT_PROCESS
//

typedef struct _FILE_PIPE_CLIENT_PROCESS_BUFFER 
{
#if !defined(BUILD_WOW6432)
	PVOID ClientSession;
	PVOID ClientProcess;
#else
	ULONGLONG ClientSession;
	ULONGLONG ClientProcess;
#endif
} FILE_PIPE_CLIENT_PROCESS_BUFFER, *PFILE_PIPE_CLIENT_PROCESS_BUFFER;

typedef ULONG (_stdcall *NtFsControlFileProc)(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL,
	IN PVOID ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG FsControlCode,
	IN PVOID InputBuffer OPTIONAL,
	IN ULONG InputBufferLength,
	OUT PVOID OutputBuffer OPTIONAL,
	IN ULONG OutputBufferLength);

static NtFsControlFileProc NtFsControlFile = NULL;

////
//
// Entry point
//
////

static WehnServ *Ginst = NULL;

INT main(
		IN UINT Ac,
		IN LPCTSTR Av)
{
	SERVICE_TABLE_ENTRY DispatchTable[] = 
	{
		{
			WEHNSERV_SVC_NAME,
			(LPSERVICE_MAIN_FUNCTION)WehnServ::ServiceEntrySt
		},
		{
			NULL,
			NULL
		}
	};

	//
	// Allocate storage for the WehnServ class instance.
	//
	if (!(Ginst = new WehnServ))
		return ERROR_NOT_ENOUGH_MEMORY;

	//
	// Start the service dispatcher
	//
	if (!StartServiceCtrlDispatcher(
			DispatchTable))
		return GetLastError();

	return 0;
}

////
//
// Class implementation
//
////

WehnServ::WehnServ()
: Checkpoint(0),
  StatusHandle(NULL),
  StopEvent(NULL)
{
	SECURITY_DESCRIPTOR Sd;
	SECURITY_ATTRIBUTES Sa;

	ZeroMemory(
			&Sd,
			sizeof(Sd));
	ZeroMemory(
			&Sa,
			sizeof(Sa));

	InitializeSecurityDescriptor(
			&Sd,
			SECURITY_DESCRIPTOR_REVISION);

	SetSecurityDescriptorDacl(
			&Sd,
			TRUE,
			NULL,
			FALSE);

	Sa.nLength              = sizeof(Sa);
	Sa.lpSecurityDescriptor = &Sd;
	Sa.bInheritHandle       = FALSE;

	//
	// Create the event we'll use for notifying when the SCM wants us to stop.
	//
	StopEvent = CreateEvent(
			NULL,
			TRUE,
			FALSE,
			NULL);

	//
	// Create the event we'll expose to user-mode that allows configuration to be
	// refreshed.
	//
	RefreshEvent = CreateEvent(
			&Sa,
			TRUE,
			FALSE,
			TEXT("Global\\WehnServRefresh"));

	//
	// Set the context on the ev manager.
	//
	EvManager.SetWehnServ(
			this);
}

WehnServ::~WehnServ()
{
	if (StopEvent)
		CloseHandle(
				StopEvent);
	if (RefreshEvent)
		CloseHandle(
				RefreshEvent);
}

VOID WINAPI WehnServ::ServiceEntrySt(
		IN UINT Ac, 
		IN LPCTSTR Av)
{
	Ginst->Initialize();
	Ginst->Run();
}

VOID WINAPI WehnServ::ServiceCtrlSt(
		IN ULONG Code)
{
	//
	// If we're stopping, set the notification event.
	//
	if (Code == SERVICE_CONTROL_STOP)
		SetEvent(
				Ginst->StopEvent);

	Ginst->ReportStatusToScm(
			Code);
}

VOID WehnServ::Initialize()
{
	//
	// Register the service status control handler.
	//
	if (!(StatusHandle = RegisterServiceCtrlHandler(
			WEHNSERV_SVC_NAME,
			ServiceCtrlSt)))
		return;

	//
	// Initialize the current status structure.
	//
	ServiceStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwServiceSpecificExitCode = 0;

	//
	// Update the SCM on where we stand at this point.
	//
	ReportStatusToScm(
			SERVICE_START_PENDING);
	
}

VOID WehnServ::Run()
{
	SECURITY_DESCRIPTOR Sd;
	SECURITY_ATTRIBUTES Sa;
	HANDLE              Events[2 + PIPE_INSTANCES];
	ULONG               Index;

	ReportStatusToScm(
			SERVICE_START_PENDING);

	//
	// Zero out the events array.
	//
	ZeroMemory(
			Events,
			sizeof(Events));

	do
	{
		//
		// Prepare our events, one for termination monitoring and the other for
		// named pipe connection monitoring.
		//
		Events[0] = StopEvent;
		Events[1] = RefreshEvent;

		//
		// Initialize the security descriptor that is needed to create the named
		// pipe.
		//
		ZeroMemory(
				&Sd,
				sizeof(Sd));
		ZeroMemory(
				&Sa,
				sizeof(Sa));

		if (!InitializeSecurityDescriptor(
				&Sd,
				SECURITY_DESCRIPTOR_REVISION))
			break;

		if (!SetSecurityDescriptorDacl(
				&Sd,
				TRUE,
				NULL,
				FALSE))
			break;

		Sa.nLength              = sizeof(Sa);
		Sa.lpSecurityDescriptor = &Sd;
		Sa.bInheritHandle       = TRUE;

		//
		// Initialize all of the pipe client instances.
		//
		for (Index = 0;
		     Index < PIPE_INSTANCES;
		     Index++)
		{
			//
			// Prepare our named pipe.
			//
			if (!(Pipes[Index].Pipe = CreateNamedPipeW(
					WEHNSERV_PIPE_NAME,
					FILE_FLAG_OVERLAPPED | PIPE_ACCESS_DUPLEX,
					PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
					PIPE_UNLIMITED_INSTANCES,
					sizeof(WEHNSERV_RESPONSE),
					sizeof(WEHNSERV_REQUEST),
					1000,
					&Sa)))
				break;

			//
			// Create the event that we'll use for overlapped I/O.
			//
			if (!(Pipes[Index].Ov.hEvent = CreateEvent(
					NULL,
					TRUE,
					FALSE,
					NULL)))
				break;

			//
			// Add the event to the array of events we'll wait on.
			//
			Events[2 + Index] = Pipes[Index].Ov.hEvent;

			//
			// Start connecting to the named pipe.
			//
			ReconnectPipeClient(
					&Pipes[Index]);
		}

		ReportStatusToScm(
				SERVICE_START_PENDING);

		//
		// Report that we are now running.
		//
		ReportStatusToScm(
				SERVICE_RUNNING);

		//
		// Keep reading in named pipe notifications until we need to stop...
		//
		while (1)
		{
			DWORD Object;

			//
			// Now we wait on the stop event and the overlapped event.
			//
			if ((Object = WaitForMultipleObjects(
					2 + PIPE_INSTANCES,
					Events,
					FALSE,
					INFINITE)) >= WAIT_OBJECT_0)
			{
				DWORD WaitIndex = Object - WAIT_OBJECT_0;

				//
				// If the terminate event was set, break out.
				//
				if (WaitIndex == 0)
					break;
				//
				// Otherwise, if the refresh event was set, refresh the ServLog
				// configuration.
				//
				else if (WaitIndex == 1)
				{
					ResetEvent(
							Events[1]);

					ServLog::Refresh();

					continue;
				}
				//
				// Otherwise, it's probably one of the pipe clients.
				//
				else if (WaitIndex < 2 + PIPE_INSTANCES)
				{
					ResetEvent(
							Events[WaitIndex]);

					ProcessPipeClient(
							&Pipes[WaitIndex - 2]);
				}
			}
		}

	} while (0);

	//
	// Flush all open pipe clients.
	//
	FlushPipeClients();

	ReportStatusToScm(
			SERVICE_STOPPED);
}

ScoreKeeper &WehnServ::GetScoreKeeper()
{
	return Keeper;
}

VOID WehnServ::ProcessPipeClient(
		IN PPIPE_CLIENT Client)
{
	BOOLEAN Success;
	BOOLEAN HasResponse = FALSE;
	ULONG   Bytes;

	//
	// If this client had pending I/O, see what's up.
	//
	if (Client->Pending)
	{
		Success = GetOverlappedResult(
				Client->Pipe,
				&Client->Ov,
				&Bytes,
				FALSE);

		switch (Client->State)
		{
			//
			// The client was in the connecting state.
			//
			case PipeClientConnecting:
				if (!Success)
				{
					ReconnectPipeClient(
							Client,
							TRUE);
					return;
				}

				Client->State = PipeClientReading;
				break;
			//
			// The client was in the reading state.
			//
			case PipeClientReading:
				if ((!Success) ||
				    (Bytes == 0))
				{
					ReconnectPipeClient(
							Client,
							TRUE);
					return;
				}

				Client->State = PipeClientWriting;
				break;
			//
			// The client was in the writing state.
			//
			case PipeClientWriting:
				if ((!Success) ||
				    (Bytes == 0))
				{
					ReconnectPipeClient(
							Client,
							TRUE);
					return;
				}
				break;
			default:
				break;
		}
	}

	//
	// Now take action depending on what state the client is in.
	//
	switch (Client->State)
	{
		case PipeClientReading:
			Success = ReadFile(
					Client->Pipe,
					&Client->Request,
					sizeof(Client->Request),
					&Bytes,
					&Client->Ov);

			//
			// If the read completed and bytes is non-zero, then we've read the
			// message.
			//
			if ((Success) &&
			    (Bytes != 0))
			{
				Client->Pending = FALSE;
				Client->State   = PipeClientWriting;
				return;
			}
			else if ((!Success) &&
			         (GetLastError() == ERROR_IO_PENDING))
			{
				Client->Pending = TRUE;
				return;
			}

			ReconnectPipeClient(
					Client,
					TRUE);
			break;
		case PipeClientWriting:
			{
				ULONG ProcessId = GetPipeClientProcessId(
						Client);

				//
				// Process the complete request.
				//
				HasResponse = DispatchPacket(
						Client,
						Client->Request.SourceProcessId,
						&Client->Request,
						&Client->Response);
			}

			//
			// If we have an immediate response, send it off.
			//
			if (HasResponse)
				SendResponseToPipeClient(
						Client);
			break;
		default:
			break;
	}
}

VOID WehnServ::SendResponseToPipeClient(
		IN PPIPE_CLIENT Client,
		IN PWEHNSERV_RESPONSE Response OPTIONAL)
{
	BOOLEAN Success;
	ULONG   Bytes;

	if (!Response)
		Response = &Client->Response;

	//
	// Write the response.
	//
	Success = WriteFile(
			Client->Pipe,
			Response,
			sizeof(WEHNSERV_RESPONSE),
			&Bytes,
			&Client->Ov);

	if ((Success) &&
		 (Bytes == sizeof(WEHNSERV_RESPONSE)))
	{
		Client->Pending = FALSE;
		Client->State   = PipeClientReading;
		return;
	}
	else if ((!Success) &&
				(GetLastError() == ERROR_IO_PENDING))
	{
		Client->Pending = TRUE;
		return;
	}

	ReconnectPipeClient(
			Client,
			TRUE);
}

VOID WehnServ::ReconnectPipeClient(
		IN PPIPE_CLIENT Client,
		IN BOOLEAN Disconnect)
{
	//
	// If we should disconnect first, do that now.
	//
	if (Disconnect)
	{
		if (Client->MonitoringEvents)
			EvManager.DeregisterEventClient(
					Client);

		if (!DisconnectNamedPipe(
				Client->Pipe))
			return;
	}

	//
	// Try to establish the initial connection in an overlapped fashion.
	//
	ConnectNamedPipe(
			Client->Pipe,
			&Client->Ov);

	switch (GetLastError())
	{
		//
		// Continue on in the connecting state
		//
		//
		case ERROR_IO_PENDING:
			Client->State = PipeClientConnecting;
			Client->Pending = TRUE;
			break;
		// 
		// The client is already connected, transition to the reading state.
		//
		case ERROR_PIPE_CONNECTED:
			Client->State = PipeClientReading;
			SetEvent(
					Client->Ov.hEvent);
			break;
		default:
			break;
	}
}

BOOLEAN WehnServ::DispatchPacket(
		IN PPIPE_CLIENT Client,
		IN ULONG ProcessId,
		IN PWEHNSERV_REQUEST Request,
		OUT PWEHNSERV_RESPONSE Response)
{
	//
	// Populate the response to a default here.
	//
	Response->ExploitInformation.ExitMethod = UseExitThread;

	switch (Request->Type)
	{
		//
		// Dispatch and exploit information event.
		//
		case WehnServExploitInformation:
			{
				PWCHAR ProcessFilePath = NULL;
				WCHAR  ProcessFileName[32] = { 0 };
				ULONG  StateId;

				//
				// Get information about the client process.
				//
				Native::GetProcessImageFileName(
						ProcessId,
						ProcessFileName,
						sizeof(ProcessFileName) - sizeof(WCHAR),
						&ProcessFilePath);

				//
				// Log the exploitation attempt.
				//
				ServLog::LogExploitationInformation(
						ProcessId,
						ProcessFileName,
						ProcessFilePath ? ProcessFilePath : L"",
						&Request->ExploitInformation);

				//
				// Determine the ExitMethod that should be used.
				//
				if (GetScoreKeeper().GetApplicationStateId(
						ProcessFilePath,
						&StateId))
				{
					GetScoreKeeper().IncrementExploitAttempts(
							StateId);

					GetScoreKeeper().GetApplicationStateExitMethod(
							StateId,
							&Response->ExploitInformation.ExitMethod);
				}

				EvManager.NotifyExploitationEvent(
						ProcessId,
						&Request->ExploitInformation);

				//
				// Free the file path if we have one.
				//
				if (ProcessFilePath)
					LocalFree(
							ProcessFilePath);
			}
			return TRUE;

		case WehnServWaitForEvent:
			EvManager.RegisterEventClient(
					Client);

			return FALSE;
		default:
			break;
	}

	return FALSE;
}

VOID WehnServ::ReportStatusToScm(
		IN ULONG CurrentState,
		IN ULONG ErrorCode,
		IN ULONG WaitHint)
{
	//
	// Select our accept codes depending on whether or not the state is start
	// pending.
	//
	if (CurrentState == SERVICE_START_PENDING)
		ServiceStatus.dwControlsAccepted = 0;
	else
		ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

	//
	// Populate the rest of our status structure.
	//
	ServiceStatus.dwCurrentState  = CurrentState;
	ServiceStatus.dwWin32ExitCode = ErrorCode;
	ServiceStatus.dwWaitHint      = WaitHint;

	//
	// Increment our check point marker if necessary.
	//
	if ((CurrentState == SERVICE_RUNNING) ||
	    (CurrentState == SERVICE_STOPPED))
		ServiceStatus.dwCheckPoint = 0;
	else
		ServiceStatus.dwCheckPoint = Checkpoint++;

	//
	// Update our service state.
	//
	SetServiceStatus(
			StatusHandle,
			&ServiceStatus);
}

ULONG WehnServ::GetPipeClientProcessId(
		IN PPIPE_CLIENT Client)
{
	FILE_PIPE_CLIENT_PROCESS_BUFFER PipeInfo;
	IO_STATUS_BLOCK                 IoStatus;
	ULONG                           Status;

	//
	// Resolve the address of NtFsControlFile
	//
	if (!NtFsControlFile)
	{
		if (!(NtFsControlFile = (NtFsControlFileProc)GetProcAddress(
				GetModuleHandle("NTDLL"),
				"NtFsControlFile")))
			return (ULONG)-1;
	}

	//
	// Query the client's process information.
	//
	if ((Status = NtFsControlFile(
			Client->Pipe,
			0,
			0,
			0,
			&IoStatus,
			FSCTL_PIPE_QUERY_CLIENT_PROCESS,
			0,
			0,
			&PipeInfo,
			sizeof(PipeInfo))) != 0)
		return (ULONG)-1;

	return (ULONG)(PipeInfo.ClientProcess);
}

VOID WehnServ::FlushPipeClients()
{
	ULONG Index;

	for (Index = 0;
	     Index < PIPE_INSTANCES;
	     Index++)
	{
		if (Pipes[Index].Pipe)
			CloseHandle(
					Pipes[Index].Pipe);

		if (Pipes[Index].Ov.hEvent)
			CloseHandle(
					Pipes[Index].Ov.hEvent);

		Pipes[Index].Pipe      = NULL;
		Pipes[Index].Ov.hEvent = NULL;
	}
}
