/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Common.h"
#undef Log

#ifndef APP_LOG_FILE
#define APP_LOG_FILE TEXT("WehnTrust.log")
#endif

static Logger *Instance = NULL;

Logger::Logger()
: Enabled(0),
  LogFile(NULL)
{
	ULONG EnabledSize = sizeof(Enabled);
	HKEY  Key = NULL;

	//
	// Check to see if logging is enabled
	//
	if (RegOpenKeyEx(
				WEHNTRUST_USER_ROOT_KEY,
				WEHNTRUST_USER_BASE_KEY,
				0,
				KEY_READ,
				&Key) == ERROR_SUCCESS)
	{
		RegQueryValueEx(
				Key,
				TEXT("LoggingEnabled"),
				0,
				NULL,
				(LPBYTE)&Enabled,
				&EnabledSize);

		RegCloseKey(
				Key);
	}

	//
	// If logging is enabled, open the log file
	//
	if (Enabled)
	{
		TCHAR LogFilePath[1024];

		LogFilePath[sizeof(LogFilePath) - sizeof(TCHAR)] = 0;

		ExpandEnvironmentStrings(
				TEXT("%TEMP%\\") APP_LOG_FILE,
				LogFilePath,
				sizeof(LogFilePath) - sizeof(TCHAR));

		fopen_s(
				&LogFile,
				LogFilePath,
				"a");

		if (!LogFile)
			Enabled = 0;
	}
}

Logger::~Logger()
{
	if (LogFile)
		fclose(LogFile);
}

//
// Get or allocate a pointer to the global instance
//
Logger *Logger::GetInstance()
{
	if (!Instance)
		Instance = new Logger;

	return Instance;
}

//
// Logs a specific message
//
VOID Logger::Log(
		IN ULONG Sev,
		IN LPCTSTR Format,
		...)
{
	va_list Ap;
	Logger  *Instance = Logger::GetInstance();
	TCHAR   Buffer[4096];

	if ((!Instance) ||
	    (!Instance->Enabled))
		return;

	va_start(
			Ap,
			Format);

	_vsntprintf_s(
			Buffer,
			sizeof(Buffer) / sizeof(TCHAR) - sizeof(TCHAR),
			Format,
			Ap);

	va_end(
			Ap);

	//
	// Log the entry
	//
	fprintf(
			Instance->LogFile,
			TEXT("%s\n"),
			Buffer);
}
