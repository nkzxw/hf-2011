/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Precomp.h"

HMODULE LanguageLibrary = NULL;

//
// Load the supplied language resource DLL.  Right now we just assume that
// there will only be one language library regardless of locale supplied.
//
DWORD Locale::LoadLanguageLibrary(
		IN LPCSTR Locale)
{
	TCHAR LocaleLibrary[512];

	ZeroMemory(
			LocaleLibrary,
			sizeof(LocaleLibrary));

	Config::GetLocale(
			LocaleLibrary,
			sizeof(LocaleLibrary) / sizeof(TCHAR));

	LanguageLibrary = LoadLibrary(
			LocaleLibrary);

	return (LanguageLibrary)
		? ERROR_SUCCESS
		: GetLastError();
}

//
// Load a string out of the library and store it in the output buffer
//
DWORD Locale::LoadString(
		IN INT Identifier, 
		OUT LPTSTR OutString,
		IN UINT OutStringSize)
{
	return (::LoadString(
				LanguageLibrary,
				Identifier,
				OutString,
				OutStringSize) != 0)
		? ERROR_SUCCESS
		: GetLastError();
}

//
// Load a string and return a dynamically allocated context to the caller.  
// This context should be deallocated with FreeString.
//
LPTSTR Locale::LoadString(
		IN INT Identifier)
{
	UINT StringSize = 4096;
	LPTSTR String = (LPTSTR)malloc(sizeof(TCHAR) * StringSize);

	if (LoadString(
			Identifier,
			String,
			StringSize) != ERROR_SUCCESS)
	{
		free(String);

		String = NULL;
	}
	
	return String;
}

//
// Load a string with a default string to use if it does not exist in the
// resource DLL.
//
// This string should be deallocated with FreeString.
//
LPTSTR Locale::LoadStringDefault(
		IN INT Identifier,
		IN LPCTSTR Default)
{
	LPTSTR String = LoadString(
			Identifier);

	if ((!String) ||
	    (!String[0]))
	{
		if (String)
			Locale::FreeString(
					String);

		String = _strdup(Default);
	}

	return String;
}

//
// Free's a string that was previously allocated by the second LoadString
// implementation.
//
VOID Locale::FreeString(
		IN LPTSTR String)
{
	if (String)
		free(String);
}
