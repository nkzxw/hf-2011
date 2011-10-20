#include "WehnServ.h"

EventLogSinky::EventLogSinky()
: EventLogSource(NULL)
{
	if (!(EventLogSource = RegisterEventSourceW(
			NULL,
			L"baserand")))
	{
		Log(LOG_SEV_ERROR, TEXT("Failed to acquire event log source, %lu."),
				GetLastError());
	}
}

EventLogSinky::~EventLogSinky()
{
	if (EventLogSource)
		DeregisterEventSource(
				EventLogSource);
}

VOID EventLogSinky::LogExploitInformation(
		IN ULONG ProcessId,
		IN LPCWSTR ProcessFileName,
		IN LPCWSTR ProcessFilePath,
		IN PEXPLOIT_INFORMATION ExploitInformation)
{
	LPCWSTR ErrorStrings[2];
	WCHAR   ErrorString[256];
	DWORD   EventId = 0;
	WORD    NumberOfStrings = 1;
	WORD    LogType = EVENTLOG_ERROR_TYPE;

	_snwprintf_s(
			ErrorString,
			sizeof(ErrorString) / sizeof(WCHAR),
			(sizeof(ErrorString) / sizeof(WCHAR)) - 1,
			L"\n\n"
			L"General Information\n\n"
			L"  Process: %lu (%s)\n",
			ProcessId,
			ProcessFileName);

	//
	// Generate the appropriate error message for this exploitation type.
	//
	switch (ExploitInformation->Type)
	{
		//
		// SEH Overwrite
		//
		case SehOverwrite:
			{
				WCHAR InfoString[128];

				//
				// Our event identifier is related to SEH overwrites.
				//
				EventId = MSG_EXP_SEH_OVERWRITE;

				_snwprintf_s(
						InfoString,
						sizeof(InfoString) / sizeof(WCHAR),
						(sizeof(InfoString) / sizeof(WCHAR)) - 1,
						L"\n"
						L"SEH Overwrite Information\n\n"
						L"  Frame handler: 0x%p\n"
						L"  Frame next: 0x%p\n"
						L"  Short jump detected: %s\n",
						ExploitInformation->Seh.InvalidFrameHandler,
						ExploitInformation->Seh.InvalidFrameNextPointer,
						ExploitInformation->Seh.NextContainsShortJumpInstruction
							? L"Yes"
							: L"No");

				//
				// We use an extra string for the SEH overwrite message.
				//
				InfoString[sizeof(InfoString) / sizeof(WCHAR) - sizeof(WCHAR)] = 0;

				ErrorStrings[NumberOfStrings++] = InfoString;
			}
			break;
		//
		// Stack overflow
		//
		case StackOverflow:
			{
				WCHAR InfoString[128];

				//
				// Our event identifier is related to SEH overwrites.
				//
				EventId = MSG_EXP_STACK_OVERFLOW;

				_snwprintf_s(
						InfoString,
						sizeof(InfoString) / sizeof(WCHAR),
						(sizeof(InfoString) / sizeof(WCHAR)) - 1,
						L"\n"
						L"Stack Overflow Information\n\n"
						L"  Fault address: 0x%p\n"
						L"  Frame pointer: 0x%p\n",
						ExploitInformation->Stack.FaultAddress,
						ExploitInformation->Stack.InvalidFramePointer);

				InfoString[sizeof(InfoString) / sizeof(WCHAR) - sizeof(WCHAR)] = 0;

				ErrorStrings[NumberOfStrings++] = InfoString;
			}
			break;
		//
		// Format string
		//
		case FormatStringBug:
			{
				WCHAR InfoString[512];

				//
				// Our event identifier is related to format string bugs.
				//
				EventId = MSG_EXP_FORMAT_STRING;

				_snwprintf_s(
						InfoString,
						sizeof(InfoString) / sizeof(WCHAR),
						(sizeof(InfoString) / sizeof(WCHAR)) - 1,
						L"\n"
						L"Format String Information\n\n"
						L"  Format specifiers: %lu\n"
						L"  Actual arguments: %lu\n"
						L"  Return address: 0x%p\n"
						L"  Format string: %S%s\n",
						ExploitInformation->FormatString.NumberOfFormatSpecifiers,
						ExploitInformation->FormatString.NumberOfArguments,
						ExploitInformation->FormatString.ReturnAddress,
						ExploitInformation->FormatString.UnicodeFormatString
							? "" : ExploitInformation->FormatString.String.a,
						ExploitInformation->FormatString.UnicodeFormatString
							? ExploitInformation->FormatString.String.w : L"");

				InfoString[sizeof(InfoString) / sizeof(WCHAR) - sizeof(WCHAR)] = 0;

				ErrorStrings[NumberOfStrings++] = InfoString;
			}
			break;
		//
		// Unsupported exploit type?
		//
		default:
			return;
	}

	//
	// Null terminate the error string.
	//
	ErrorString[sizeof(ErrorString) / sizeof(WCHAR) - 1] = 0;

	//
	// Log the event
	//
	ErrorStrings[0] = ErrorString;

	if (!ReportEventW(
			EventLogSource,
			LogType,
			CAT_EXPLOITATION,
			EventId,
			NULL,
			NumberOfStrings,
			sizeof(EXPLOIT_INFORMATION),
			ErrorStrings,
			ExploitInformation))
	{
		Log(LOG_SEV_ERROR, TEXT("ReportEventW failed, %lu."),
				GetLastError());
	}
}
