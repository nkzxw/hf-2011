#ifndef _WEHNTRUST_WEHNSERV_LOGSINKS_EVENTLOG
#define _WEHNTRUST_WEHNSERV_LOGSINKS_EVENTLOG

//
// This class provides a log sink interface through the system event log.
//
class EventLogSinky :
	public LogSink
{
	public:
		EventLogSinky();
		~EventLogSinky();

		LOG_SINK_TYPE GetType() { return LOG_SINK_TYPE_EVENTLOG; }

		//
		// Logs information about any number of supported exploitation attempts to
		// the event log.
		//
		VOID LogExploitInformation(
				IN ULONG ProcessId,
				IN LPCWSTR ProcessFileName,
				IN LPCWSTR ProcessFilePath,
				IN PEXPLOIT_INFORMATION ExploitInformation);
	protected:

		////
		//
		// Attributes
		//
		////

		//
		// Registered event log source.
		//
		HANDLE EventLogSource;
};

#endif
