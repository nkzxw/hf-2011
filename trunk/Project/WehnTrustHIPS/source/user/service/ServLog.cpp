/*
 * WehnTrust
 *
 * Copyright (c) 2005, Wehnus.
 */
#include "WehnServ.h"

static ServLog *LogInstance = NULL;

//
// An array of log sink types used for making code smaller.
//
static LOG_SINK_TYPE LogSinkTypes[] =
{
	LOG_SINK_TYPE_EVENTLOG,
	LOG_SINK_TYPE_FILE,
	LOG_SINK_TYPE_SNMP
};

ServLog::ServLog()
: ActiveSinkTypes(0)
{
	LogInstance = this;

	Refresh();
}

ServLog::~ServLog()
{
	FlushLogSinks();

	LogInstance = NULL;
}

ServLog *ServLog::GetInstance()
{
	if (!LogInstance)
	{
		try
		{
			new ServLog;
		} catch (...)
		{
		}
	}

	return LogInstance;
}

VOID ServLog::Refresh()
{
	LOG_SINK_TYPE OldActiveLogSinks = 0;
	LOG_SINK_TYPE NewActiveLogSinks = Config::GetActiveLogSinks();
	ULONG         Index;
	ServLog       *Inst = ServLog::GetInstance();

	//
	// Save off the original active log sinks
	//
	OldActiveLogSinks = Inst->ActiveSinkTypes;

	//
	// Walk each individual type, checking to see whether or not it's been
	// enabled.
	//
	for (Index = 0;
	     Index < (sizeof(LogSinkTypes) / sizeof(LOG_SINK_TYPE));
	     Index++)
	{
		// 
		// If this log sink type is enabled and was not previously enabled, we
		// instantiate it and add it to the list of active sinks.
		//
		if ((NewActiveLogSinks & LogSinkTypes[Index]) &&
		    (!(OldActiveLogSinks & LogSinkTypes[Index])))
		{
			LogSink *Sink = LogSink::create(
					LogSinkTypes[Index]);

			Log(LOG_SEV_INFO, TEXT("Enabling log sink type %lu."),
					LogSinkTypes[Index]);

			//
			// Add it to the list.
			//
			if (Sink)
				Inst->SinkList.push_back(
						Sink);
		}
		//
		// Otherwise, if the original log sink had this sink type enabled, we may
		// either need to remove it from the list of active sinks (if it's no
		// longer in the new set), or we may need to refresh its settings.
		//
		else if (OldActiveLogSinks & LogSinkTypes[Index])
		{
			std::list<LogSink *>::iterator It;

			for (It = Inst->SinkList.begin();
			     It != Inst->SinkList.end();
			     It++)
			{
				//
				// Did we find the type we're looking for?  If not, continue.
				//
				if ((*It)->GetType() != LogSinkTypes[Index])
					continue;

				LogSink *Sink = (*It);

				//
				// If this log sink type is no longer in the active set, then we
				// must remove it from the list of active log sinks.
				//
				if (!(NewActiveLogSinks & LogSinkTypes[Index]))
				{
					Inst->SinkList.erase(It);

					Log(LOG_SEV_INFO, TEXT("Disabling log sink type %lu."),
							LogSinkTypes[Index]);

					LogSink::destroy(
							Sink);

					break;
				}
				//
				// Otherwise, if it's been enabled and is still enabled, refresh it to
				// make sure everything is still good to go.
				//
				else
				{
					Log(LOG_SEV_INFO, TEXT("Refreshing log sink type %lu."),
							LogSinkTypes[Index]);

					Sink->Refresh();
				}
			}
		}
	}

	//
	// Update the active log sinks bitvector.
	//
	Inst->ActiveSinkTypes = NewActiveLogSinks;
}

VOID ServLog::LogExploitationInformation(
		IN ULONG ProcessId,
		IN LPCWSTR ProcessFileName,
		IN LPCWSTR ProcessFilePath,
		IN PEXPLOIT_INFORMATION ExploitInformation)
{
	std::list<LogSink *>::const_iterator It;
	ServLog                              *Inst;

	//
	// Increment the total number of detected exploitation attempts
	//
	Config::IncrementExploitsPrevented();

	//
	// Create an instance of the logger.
	//
	if (!(Inst = ServLog::GetInstance()))
	{
		Log(LOG_SEV_ERROR, TEXT("Failed to create a ServLog instance."));
	}

	//
	// Walk the list of registered log sinks calling the LogExploitInformation on
	// each.
	//
	for (It = Inst->SinkList.begin();
	     It != Inst->SinkList.end();
	     It++)
	{
		(*It)->LogExploitInformation(
				ProcessId,
				ProcessFileName,
				ProcessFilePath,
				ExploitInformation);
	}
}

////
//
// Protected Methods
//
////

VOID ServLog::FlushLogSinks()
{
	std::list<LogSink *>::iterator It;

	for (It = SinkList.begin();
	     It != SinkList.end();
	     It = SinkList.begin())
	{
		LogSink *Sink = (*It);

		SinkList.erase(It);

		LogSink::destroy(
				Sink);
	}
}
