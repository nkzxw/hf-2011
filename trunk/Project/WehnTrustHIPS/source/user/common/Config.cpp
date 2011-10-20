/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Common.h"

//
// Sets whether or not the system tray is enabled
//
BOOLEAN Config::SetSystemTrayEnabled(
		IN BOOLEAN Enabled)
{
	DWORD Value = !Enabled;
	HKEY  ConfigKey = NULL;

	if ((ConfigKey = OpenConfigKey()))
	{
		RegSetValueEx(
				ConfigKey,
				TEXT("SystemTrayDisabled"),
				0,
				REG_DWORD,
				(LPBYTE)&Value,
				sizeof(Value));
	}

	if (ConfigKey)
		CloseConfigKey(
				ConfigKey);

	return TRUE;
}

//
// Check to see if the system tray icon should be disabled
//
BOOLEAN Config::IsSystemTrayEnabled()
{
	BOOLEAN Enabled = TRUE;
	DWORD   Value = 0;
	DWORD   ValueSize = sizeof(Value);
	HKEY    ConfigKey = NULL;

	do
	{
		//
		// No config key? break out
		//
		if (!(ConfigKey = OpenConfigKey()))
			break;

		RegQueryValueEx(
				ConfigKey,
				TEXT("SystemTrayDisabled"),
				0,
				NULL,
				(LPBYTE)&Value,
				&ValueSize);

		if (Value)
			Enabled = FALSE;

	} while (0);

	if (ConfigKey)
		CloseConfigKey(
				ConfigKey);

	return Enabled;
}

//
// Sets the locale that should be used.
//
VOID Config::SetLocale(
		IN LPCTSTR Locale)
{
	HKEY Key = OpenConfigKey();

	RegSetValueEx(
			Key,
			TEXT("Locale"),
			0,
			REG_SZ,
			(LPBYTE)Locale,
			lstrlen(Locale) + sizeof(TCHAR));

	if (Key)
		CloseConfigKey(
				Key);
}

//
// Gets the active locale.  If one isn't defined, en_US is returned.
//
VOID Config::GetLocale(
		OUT LPTSTR Locale,
		IN DWORD LocaleSize)
{
	HKEY Key = OpenConfigKey();

	if (RegQueryValueEx(
			Key,
			TEXT("Locale"),
			0,
			NULL,
			(LPBYTE)Locale,
			&LocaleSize) != ERROR_SUCCESS)
	{
		lstrcpyn(
				Locale,
				TEXT("en_US"),
				LocaleSize);
	}

	if (Key)
		CloseConfigKey(
				Key);
}

//
// Opens an exemptions key for the purpose of enumeration
//
HKEY Config::OpenExemptionScopeKey(
		IN EXEMPTION_SCOPE Scope)
{
	HKEY ScopeKey = NULL;

	switch (Scope)
	{
		case GlobalScope:
			RegOpenKeyEx(
					WEHNTRUST_ROOT_KEY,
					WEHNTRUST_BASE_KEY_ROOT TEXT("\\Exemptions\\Global"),
					0,
					KEY_READ,
					&ScopeKey);
			break;
		default:
			break;
	}

	return ScopeKey;
}

//
// Closes a previously opened exemption key
//
VOID Config::CloseExemptionScopeKey(
		IN HKEY ScopeKey)
{
	if (ScopeKey)
		RegCloseKey(
				ScopeKey);
}

//
// Enumerates an exempted file at the given index for a given exemption key
//
DWORD Config::EnumerateExemptionScopeKey(
		IN HKEY ScopeKey,
		IN EXEMPTION_TYPE Type,
		IN LPDWORD Index,
		OUT LPTSTR FilePath,
		OUT LPDWORD FilePathLength,
		OUT LPDWORD Flags)
{
	TCHAR TempKeyName[256];
	DWORD TempKeyNameSize = sizeof(TempKeyName);
	DWORD Result;

	while ((Result = RegEnumKey(
			ScopeKey,
			*Index,
			TempKeyName,
			TempKeyNameSize)) == ERROR_SUCCESS)
	{
		DWORD FlagsSize = sizeof(Flags);
		DWORD ExemptionTypeSize = sizeof(DWORD);
		DWORD ExemptionType = 0;
		HKEY  ExemptionKey;
	
		if ((Result = RegOpenKeyEx(
				ScopeKey,
				TempKeyName,
				0,
				KEY_READ,
				&ExemptionKey)) != ERROR_SUCCESS)
			break;

		RegQueryValueEx(
				ExemptionKey,
				TEXT("Type"),
				NULL,
				NULL,
				(LPBYTE)&ExemptionType,
				&ExemptionTypeSize);

		//
		// If this is the type we're searching for...
		//
		if (!(ExemptionType & Type))
		{
			RegCloseKey(
					ExemptionKey);

			(*Index)++;

			continue;
		}

		RegQueryValueEx(
				ExemptionKey,
				TEXT("Path"),
				NULL,
				NULL,
				(LPBYTE)FilePath,
				FilePathLength);

		if (Flags)
			RegQueryValueEx(
					ExemptionKey,
					TEXT("Flags"),
					NULL,
					NULL,
					(LPBYTE)Flags,
					&FlagsSize);

		RegCloseKey(
				ExemptionKey);

		break;
	}

	return Result;
}

//
// Gets information about an existing exemption
//
DWORD Config::GetExemptionInfo(
		IN EXEMPTION_SCOPE Scope,
		IN EXEMPTION_TYPE Type,
		IN LPTSTR NtFilePath,
		OUT LPDWORD Flags)
{
	TCHAR KeyName[EXEMPTION_KEY_NAME_SIZE];
	DWORD Result = ERROR_SUCCESS;
	DWORD FlagsSize = sizeof(DWORD);
	HKEY  ExemptionKey = NULL;
	HKEY  ScopeKey = NULL;

	*Flags = 0;

	do
	{
		//
		// Open the scope key
		// 
		if (!(ScopeKey = OpenExemptionScopeKey(
				Scope)))
		{
			Result = GetLastError();
			break;
		}

		GetExemptionKeyNameFromNtPath(
				NtFilePath,
				KeyName);
	
		//
		// No key name?  No cookie.
		//
		if (!KeyName[0])
		{
			Result = GetLastError();
			break;
		}

		//
		// Open the exemption key
		//
		if ((Result = RegOpenKeyEx(
				ScopeKey,
				KeyName,
				0,
				KEY_READ,
				&ExemptionKey)) != ERROR_SUCCESS)
			break;

		//
		// Query the flags on the exemption
		//
		RegQueryValueEx(
				ExemptionKey,
				TEXT("Flags"),
				0,
				NULL,
				(LPBYTE)Flags,
				&FlagsSize);

	} while (0);

	//
	// Cleanup
	//
	if (ExemptionKey)
		RegCloseKey(
				ExemptionKey);
	if (ScopeKey)
		CloseExemptionScopeKey(
				ScopeKey);

	return Result;
}

//
// Gets the hash key name from the NT file path
//
VOID Config::GetExemptionKeyNameFromNtPath(
		IN LPTSTR FilePath,
		OUT TCHAR KeyName[EXEMPTION_KEY_NAME_SIZE])
{
	UNICODE_STRING NtFilePath = { 0 };
	SHA1_CTX       ShaContext;
	UCHAR          TempDigest[SHA1_HASH_SIZE];
	ULONG          Index;

	//
	// Zero the name out
	//
	ZeroMemory(
			KeyName,
			EXEMPTION_KEY_NAME_SIZE);

	do
	{ 
		//
		// Get the UNICODE_STRING version of the NT path
		//
		if (!DriverClient::GetNtPath(
				FilePath,
				&NtFilePath,
				TRUE))
			break;

		//
		// Since GetNtPath returns with a NULL terminator, truncate it
		//
		Index = (NtFilePath.Length / sizeof(WCHAR)) - 1;

		while ((Index) && 
		       (!NtFilePath.Buffer[Index--]))
			NtFilePath.Length -= sizeof(WCHAR);

		//
		// Calculate the hash of the path name
		//
		SHA1_Init(
				&ShaContext);
		SHA1_Update(
				&ShaContext,
				(LPBYTE)NtFilePath.Buffer,
				NtFilePath.Length);
		SHA1_Final(
				&ShaContext,
				TempDigest);

		//
		// Write off the hash 
		//
		_stprintf_s(
				KeyName,
				EXEMPTION_KEY_NAME_SIZE,
				"%.8x%.8x%.8x%.8x%.8x",
				ShaContext.A,
				ShaContext.B,
				ShaContext.C,
				ShaContext.D,
				ShaContext.E);

	} while (0);

	//
	// Call free directory as we know it came from the heap
	//
	if (NtFilePath.Buffer)
		free(
				NtFilePath.Buffer);
}

//
// Enables one or more log sink types.
//
DWORD Config::EnableLogSinks(
		IN LOG_SINK_TYPE Types,
		IN BOOLEAN Disable)
{
	DWORD Result;
	HKEY  SysKey = OpenSysConfigKey(
			KEY_READ | KEY_WRITE);

	if (SysKey)
	{
		LOG_SINK_TYPE ActiveTypes = 0;
		ULONG ActiveTypesSize = sizeof(ActiveTypes);

		//
		// Try to query the currently set value of the active sinks
		//
		if ((Result = RegQueryValueEx(
				SysKey,
				TEXT("LogSinks"),
				0,
				NULL,
				(LPBYTE)&ActiveTypes,
				&ActiveTypesSize)) != NO_ERROR)
			ActiveTypes = 0;

		// 
		// If we're disabling, remove the types supplied.  Otherwise, add them in.
		//
		if (Disable)
			ActiveTypes &= ~Types;
		else
			ActiveTypes |= Types;

		//
		// Update the value in the registry.
		//
		Result = RegSetValueEx(
				SysKey,
				TEXT("LogSinks"),
				0,
				NULL,
				(LPBYTE)&Types,
				sizeof(Types));

		CloseSysConfigKey(
				SysKey);
	}
	else
		Result = GetLastError();

	return Result;
}

//
// Disables one or more log sink types.
//
DWORD Config::DisableLogSinks(
		IN LOG_SINK_TYPE Types)
{
	return EnableLogSinks(
			Types,
			TRUE);
}

//
// Returns the active log sinks
//
LOG_SINK_TYPE Config::GetActiveLogSinks()
{
	LOG_SINK_TYPE ActiveTypes = 0;
	HKEY          SysKey = OpenSysConfigKey();

	if (SysKey)
	{
		ULONG ActiveTypesSize = sizeof(ActiveTypes);

		RegQueryValueEx(
				SysKey,
				TEXT("LogSinks"),
				0,
				NULL,
				(LPBYTE)&ActiveTypes,
				&ActiveTypesSize);

		CloseSysConfigKey(
				SysKey);
	}

	return ActiveTypes;
}

VOID Config::IncrementExploitsPrevented()
{
	HKEY SysKey = OpenSysConfigKey(
			KEY_WRITE);

	if (SysKey)
	{
		ULONG Current = GetExploitsPreventedCount() + 1;

		RegSetValueEx(
				SysKey,
				TEXT("ExploitsPrevented"),
				0,
				REG_DWORD,
				(LPBYTE)&Current,
				sizeof(Current));

		RegCloseKey(
				SysKey);
	}
}

ULONG Config::GetExploitsPreventedCount()
{
	ULONG Prevented = 0;
	ULONG PreventedSize = sizeof(Prevented);
	HKEY  SysKey = OpenSysConfigKey();

	if (SysKey)
	{
		RegQueryValueEx(
				SysKey,
				TEXT("ExploitsPrevented"),
				0,
				NULL,
				(LPBYTE)&Prevented,
				&PreventedSize);

		RegCloseKey(
				SysKey);
	}

	return Prevented;
}

////
//
// Protected Methods
//
////

//
// Opens the configuration key for this user
//
HKEY Config::OpenConfigKey()
{
	DWORD Result;
	HKEY  Key = NULL;
	
	if ((Result = RegCreateKeyEx(
			WEHNTRUST_USER_ROOT_KEY,
			WEHNTRUST_USER_BASE_KEY,
			0,
			NULL,
			0,
			KEY_READ | KEY_WRITE,
			NULL,
			&Key,
			NULL)) != NO_ERROR)
		SetLastError(
				Result);

	return Key;
}

//
// Closes a previously opened config key
//
VOID Config::CloseConfigKey(
		IN HKEY ConfigKey)
{
	RegCloseKey(
			ConfigKey);
}

//
// Opens the system configuration key
//
HKEY Config::OpenSysConfigKey(
		IN ULONG DesiredAccess)
{
	DWORD Result;
	HKEY  Key = NULL;
	
	if ((Result = RegCreateKeyEx(
			WEHNTRUST_SYSCONF_ROOT_KEY,
			WEHNTRUST_SYSCONF_BASE_KEY,
			0,
			NULL,
			0,
			DesiredAccess,
			NULL,
			&Key,
			NULL)) != NO_ERROR)
		SetLastError(
				Result);

	return Key;
}

//
// Closes a previously opened system config key
//
VOID Config::CloseSysConfigKey(
		IN HKEY SysConfigKey)
{
	RegCloseKey(
			SysConfigKey);
}
