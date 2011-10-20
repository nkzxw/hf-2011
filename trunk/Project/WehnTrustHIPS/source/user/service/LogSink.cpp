#include "WehnServ.h"
#include "EventLog.h"

LogSink *LogSink::create(
		IN LOG_SINK_TYPE Type)
{
	LogSink *Sink = NULL;

	try
	{
		switch (Type)
		{
			case LOG_SINK_TYPE_EVENTLOG:
				Sink = new EventLogSinky;
				break;
			case LOG_SINK_TYPE_FILE:
				break;
			case LOG_SINK_TYPE_SNMP:
				break;
			default:
				break;
		}
	} catch (...)
	{
	}

	return Sink;
}

VOID LogSink::destroy(
		IN LogSink *Sink)
{
	if (Sink)
		delete Sink;
}

VOID LogSink::Refresh()
{
}
