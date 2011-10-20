#ifndef _WEHNTRUST_WEHNSERV_LOGSINK_H
#define _WEHNTRUST_WEHNSERV_LOGSINK_H

//
// This class acts as an abstract interface to one of the various log sink
// implementations, such as the system event log, a file, SNMP, or any other
// arbitrary medium.
//
class LogSink
{
	public:
		////
		//
		// Factory methods
		//
		////

		//
		// Creates a log sink instance.
		//
		static LogSink *create(
				IN LOG_SINK_TYPE Type);

		//
		// Destroys a log sink instance.
		//
		static VOID destroy(
				IN LogSink *Sink);

		////
		//
		// Abstract sink interface
		//
		////

		//
		// Returns the type of the derived implementation.
		//
		virtual LOG_SINK_TYPE GetType() = 0;

		//
		// Logs information about an exploitation attempt.
		//
		virtual VOID LogExploitInformation(
				IN ULONG ProcessId,
				IN LPCWSTR ProcessFileName,
				IN LPCWSTR ProcessFilePath,
				IN PEXPLOIT_INFORMATION ExploitInformation) = 0;

		//
		// Refreshes the sink type, if supported.
		//
		virtual VOID Refresh();
	protected:
};

////
//
// Include the various sink implementations.
//
////

#include "EventLog.h"

#endif
