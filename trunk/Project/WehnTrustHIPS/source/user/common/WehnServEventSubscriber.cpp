#include "Common.h"

WehnServEventSubscriber::WehnServEventSubscriber()
: EventMonitorThread(NULL),
  Terminate(FALSE)
{
}

WehnServEventSubscriber::~WehnServEventSubscriber()
{
	DeregisterEventSubscriber();
}

BOOLEAN WehnServEventSubscriber::RegisterEventSubscriber()
{
	BOOLEAN Result = FALSE;
	ULONG   ThreadId;

	Terminate = FALSE;

	do
	{
		//
		// Start the monitor thread.
		//
		if (!(EventMonitorThread = CreateThread(
				NULL,
				NULL,
				(LPTHREAD_START_ROUTINE)MonitorEventsSt,
				this,
				NULL,
				&ThreadId)))
			break;

		//
		// We're running...
		//
		Result = TRUE;

	} while (0);

	return Result;
}

VOID WehnServEventSubscriber::DeregisterEventSubscriber()
{
	Terminate = TRUE;

	//
	// Wait for the thread to terminate.
	//
	if (EventMonitorThread)
	{
		if (WaitForSingleObjectEx(
				EventMonitorThread,
				500,
				FALSE) != WAIT_OBJECT_0)
			TerminateThread(
					EventMonitorThread,
					0);

		CloseHandle(
				EventMonitorThread);

		EventMonitorThread = NULL;
	}
}

VOID WehnServEventSubscriber::OnExploitationEvent(
		IN ULONG ProcessId,
		IN PEXPLOIT_INFORMATION ExploitInformation)
{
}

////
//
// Protected Methods
//
////

ULONG WehnServEventSubscriber::MonitorEventsSt(
		IN WehnServEventSubscriber *Subscriber)
{
	return Subscriber->MonitorEvents();
}

ULONG WehnServEventSubscriber::MonitorEvents()
{
	WEHNSERV_RESPONSE Response;
	WEHNSERV_REQUEST  Request;
	ULONG             BytesRead;

	//
	// Post our request to retrieve an event.
	//
	Request.Type            = WehnServWaitForEvent;
	Request.SourceProcessId = GetCurrentProcessId();

	while (1)
	{
		if (!CallNamedPipe(
				WEHNSERV_PIPE_NAME_ANSI,
				&Request,
				sizeof(WEHNSERV_REQUEST),
				&Response,
				sizeof(WEHNSERV_RESPONSE),
				&BytesRead,
				NMPWAIT_WAIT_FOREVER))
		{
			if (GetLastError() == ERROR_PIPE_BUSY)
			{
				WaitNamedPipe(
						WEHNSERV_PIPE_NAME_ANSI,
						20000);
			}
			else
			{
				SleepEx(
						30000,
						TRUE);
			}

			continue;
		}

		//
		// If terminate is set, then we shall break out of our loop and return.
		//
		if (Terminate)
			break;

		//
		// Dispatch the event.
		//
		switch (Response.Event.Type)
		{
			case WehnServNotifyExploitationEvent:
				OnExploitationEvent(
						Response.Event.ProcessId,
						&Response.Event.ExploitInformation);
				break;
			default:
				break;
		}
	}

	return 0;
}
