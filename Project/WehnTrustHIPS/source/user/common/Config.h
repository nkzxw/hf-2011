/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNTRUST_UTIL_CONFIG_H
#define _WEHNTRUST_WEHNTRUST_UTIL_CONFIG_H

#define EXEMPTION_KEY_NAME_SIZE ((SHA1_HASH_SIZE * 2) + 1)

//
// Log sink types.
//
#define LOG_SINK_TYPE_EVENTLOG (1 << 0)
#define LOG_SINK_TYPE_FILE     (1 << 1)
#define LOG_SINK_TYPE_SNMP     (1 << 2)

typedef ULONG LOG_SINK_TYPE, *PLOG_SINK_TYPE;

//
// This class provides an interface to the various configuration subsystems that
// can be used to drive WehnTrust.
//
class Config
{
	public:
		static BOOLEAN SetSystemTrayEnabled(
				IN BOOLEAN Enabled);
		static BOOLEAN IsSystemTrayEnabled();

		static VOID SetLocale(
				IN LPCTSTR Locale);
		static VOID GetLocale(
				OUT LPTSTR Locale,
				IN DWORD LocaleSize);

		//
		// Exemption management
		//
		static HKEY OpenExemptionScopeKey(
				IN EXEMPTION_SCOPE Scope);
		static VOID CloseExemptionScopeKey(
				IN HKEY ScopeKey);

		static DWORD EnumerateExemptionScopeKey(
				IN HKEY ScopeKey,
				IN EXEMPTION_TYPE Type,
				IN LPDWORD Index,
				OUT LPTSTR FilePath,
				OUT LPDWORD FilePathLength,
				OUT LPDWORD Flags);
		static DWORD GetExemptionInfo(
				IN EXEMPTION_SCOPE Scope,
				IN EXEMPTION_TYPE Type,
				IN LPTSTR FilePath,
				OUT LPDWORD Flags);

		static VOID GetExemptionKeyNameFromNtPath(
				IN LPTSTR FilePath,
				OUT TCHAR KeyName[EXEMPTION_KEY_NAME_SIZE]);

		//
		// Event notification interface
		//
		static DWORD EnableLogSinks(
				IN LOG_SINK_TYPE Types,
				IN BOOLEAN Disable = FALSE);
		static DWORD DisableLogSinks(
				IN LOG_SINK_TYPE Types);
		static LOG_SINK_TYPE GetActiveLogSinks();	

		//
		// Exploits prevented counter
		//
		static VOID IncrementExploitsPrevented();
		static ULONG GetExploitsPreventedCount();
	protected:
		static HKEY OpenConfigKey();
		static VOID CloseConfigKey(
				IN HKEY ConfigKey);
		
		static HKEY OpenSysConfigKey(
				IN ULONG DesiredAccess = KEY_READ);
		static VOID CloseSysConfigKey(
				IN HKEY ConfigKey);
};

#endif
