/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNTRUST_UI_LOCALE_H
#define _WEHNTRUST_WEHNTRUST_UI_LOCALE_H

//
// This class wrappers the obtaining of locale strings
//
class Locale
{
	public:
		static DWORD LoadLanguageLibrary(
				IN LPCSTR Locale = "en_US");

		static DWORD LoadString(
				IN INT Identifier, 
				OUT LPTSTR OutString,
				IN UINT OutStringSize);
		static LPTSTR LoadString(
				IN INT Identifier);
		static LPTSTR LoadStringDefault(
				IN INT Identifier,
				IN LPCTSTR DefaultString);
		static VOID FreeString(
				IN LPTSTR String);
	protected:
};

#endif
