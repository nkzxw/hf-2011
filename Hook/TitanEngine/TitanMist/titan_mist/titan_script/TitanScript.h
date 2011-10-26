#ifndef TITANSCRIPT_H
#define TITANSCRIPT_H

#if _MSC_VER > 1000
	//#pragma once
#endif

#include <windows.h>

enum eLogType {TS_LOG_NORMAL, TS_LOG_ERROR, TS_LOG_COMMAND, TS_LOG_DEBUG};
typedef void(__stdcall *fLogCallback)(const char* szString, eLogType Type);

typedef bool (__stdcall *tScripterLoadFileA)(const char*);
typedef bool (__stdcall *tScripterLoadFileW)(const wchar_t*);
typedef bool (__stdcall *tScripterLoadBuffer)(const char*);
typedef bool (__stdcall *tScripterResume)();
typedef bool (__stdcall *tScripterPause)();
typedef bool (__stdcall *tScripterAutoDebugA)(const char*);
typedef bool (__stdcall *tScripterAutoDebugW)(const wchar_t*);
typedef void (__stdcall *tScripterSetLogCallback)(fLogCallback Callback);

// use like this: tScripterResume foo = GetTSFunctionPointer(Resume);
#define GetTSFunctionPointer(x) ((tScripter ## x)GetProcAddress(GetModuleHandleA("TitanScript"), "Scripter" #x))

#endif /*TITANSCRIPT_H*/
