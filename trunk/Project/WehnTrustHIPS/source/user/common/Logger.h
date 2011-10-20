/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_NATIVE_LOGGER_H
#define _WEHNTRUST_NATIVE_LOGGER_H

#define LOG_SEV_INFO  (1 << 0)
#define LOG_SEV_WARN  (1 << 1)
#define LOG_SEV_ERROR (1 << 2)
#define LOG_SEV_DEBUG (1 << 3)

//
// Wrapper for logging messages to a log file
//
class Logger
{
	public:
		Logger();
		~Logger();

		static Logger *GetInstance();

		static VOID Log(
				IN ULONG Sev,
				IN LPCTSTR Format,
				...);
	protected:
		ULONG Enabled;
		FILE  *LogFile;
};

#endif
