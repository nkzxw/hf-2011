/*
 * WehnTrust
 *
 * Copyright (c) 2005, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNSERV_SERVLOG_H
#define _WEHNTRUST_WEHNSERV_SERVLOG_H

#include "LogSink.h"

//
// Logs various information to various places.
//
class ServLog
{
	public:
		ServLog();
		~ServLog();

		////
		//
		// Static entry points for logging in a generic fashion.
		//
		////
		
		//
		// Returns the singleton instance of the log class.
		//
		static ServLog *GetInstance();

		//
		// Refreshes the log sinks that should be active based on configuration
		// found in the registry.
		//
		static VOID Refresh();

		//
		// Logs information about an exploitation attempt.
		//
		static VOID LogExploitationInformation(
				IN ULONG ProcessId,
				IN LPCWSTR ProcessFileName,
				IN LPCWSTR ProcessFilePath,
				IN PEXPLOIT_INFORMATION ExploitInformation);

	protected:

		//
		// Flushes all allocated log sinks.
		//
		VOID FlushLogSinks();

		////
		//
		// Attributes
		//
		////
		
		//
		// The list of active log sinks.
		//
		std::list<LogSink *> SinkList;
		//
		// The bit vector of currently enabled sink types.
		//
		LOG_SINK_TYPE        ActiveSinkTypes;
};

#endif
