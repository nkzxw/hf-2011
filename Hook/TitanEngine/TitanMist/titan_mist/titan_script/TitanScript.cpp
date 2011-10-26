#include "TitanScript.h"

#include <windows.h>
#include <psapi.h>
#include "OllyLang.h"
#include "TE_Interface.h"
#include "HelperFunctions.h"
#include "SDK.hpp"
#include "globals.h"

#include "Debug.h"

using namespace TE;

extern "C" __declspec(dllexport) void __stdcall TitanDebuggingCallBack(DEBUG_EVENT* debugEvent, int CallReason);

/*
Design:

ScripterResume MUST be called from within the debug loop
 - BP callback
 - or via plugin interface:
   + Call to ScripterAutoDebug which loads exe and calls DebugLoop and calls ScripterResume on EP
it will immediately return, this is needed for returning to the debug loop
and executing until a breakpoint/exception occurs:

/ + DebugLoop()
^   + OnBP/OnException callback
|     + OllyLang::Step()
^	  [do commands until return to loop is required (RUN, STI, etc.)]
|     -
^   -
\ -

When done, call FinishedCallback
(if script loaded inside debug loop and not via ScripterExecuteScript)
or return
*/

void __stdcall AutoDebugEntry();
bool FileNameFromHandle(HANDLE hFile, char Name[MAX_PATH]);

// OllyLang object
OllyLang& ollylang = OllyLang::Instance();

BOOL APIENTRY DllMain(HINSTANCE hi, DWORD reason, LPVOID)
{
    switch(reason)
	{
		case DLL_PROCESS_ATTACH:
			hinst = hi;
			DisableThreadLibraryCalls(hi);
			break;
    }
    return TRUE;
}

// TitanEngine plugin callbacks

extern "C" __declspec(dllexport) void __stdcall TitanDebuggingCallBack(DEBUG_EVENT* debugEvent, int CallReason)
{
static ULONG_PTR OldIP;

	switch(CallReason)
	{
	case UE_PLUGIN_CALL_REASON_PREDEBUG:
		//ollylang.Reset();
		ollylang.debuggee_running = false;
		OldIP = 0;
		break;
	case UE_PLUGIN_CALL_REASON_POSTDEBUG:
		break;
	case UE_PLUGIN_CALL_REASON_EXCEPTION:
		switch(debugEvent->dwDebugEventCode)
		{
		case CREATE_PROCESS_DEBUG_EVENT:
			if(FileNameFromHandle(debugEvent->u.CreateProcessInfo.hFile, TargetPath))
			{
				strcpy(TargetDirectory, folderfrompath(TargetPath).c_str());
				ollylang.InitGlobalVariables();
			}
			break;
		case EXCEPTION_DEBUG_EVENT:
			if(ollylang.script_running)
			{
				ULONG_PTR NewIP = Debugger::GetContextData(UE_CIP);
				if(debugEvent->u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
					NewIP--;

				DBG_LOG("Exception debug event @ " + rul2hexstr(NewIP));

				if(NewIP != OldIP)
					ollylang.debuggee_running = false;

				if(!debugEvent->u.Exception.dwFirstChance)
					ollylang.OnException();

				OldIP = NewIP;
			}
			break;
		}
	}
}

extern "C" __declspec(dllexport) bool __stdcall TitanRegisterPlugin(char* szPluginName, DWORD* titanPluginMajorVersion, DWORD* titanPluginMinorVersion)
{
const DWORD PLUGIN_MAJOR_VERSION = 1;
const DWORD PLUGIN_MINOR_VERSION = 0;

	if(titanPluginMajorVersion && titanPluginMinorVersion)
	{
		*titanPluginMajorVersion = PLUGIN_MAJOR_VERSION;
		*titanPluginMinorVersion = PLUGIN_MINOR_VERSION;
		strcpy(szPluginName, "TitanScript");
		return true;
	}
	return false;
}

extern "C" __declspec(dllexport) void __stdcall TitanResetPlugin()
{
	//ollylang.Reset();
	//ollylang.script.clear();
}

extern "C" __declspec(dllexport) void __stdcall TitanReleasePlugin()
{

}

// Plugin APIs

extern "C" __declspec(dllexport) bool __stdcall ScripterLoadFileW(const wchar_t* szFileName)
{
	ollylang.Reset();
	return ollylang.script.load_file(szFileName);
}

extern "C" __declspec(dllexport) bool __stdcall ScripterLoadFileA(const char* szFileName)
{
	return ScripterLoadFileW(ascii2unicode(szFileName).c_str());
}

extern "C" __declspec(dllexport) bool __stdcall ScripterLoadBuffer(const char* szScript)
{
	ollylang.Reset();
	return ollylang.script.load_buff(szScript, strlen(szScript));
}

extern "C" __declspec(dllexport) bool __stdcall ScripterResume()
{
	return ollylang.Run();
}

extern "C" __declspec(dllexport) bool __stdcall ScripterPause()
{
	return ollylang.Pause();
}

extern "C" __declspec(dllexport) bool __stdcall ScripterAutoDebugW(const wchar_t* szDebuggee)
{
	if(ollylang.script.isloaded() && Debugger::InitDebugEx(szDebuggee, NULL, NULL, &AutoDebugEntry))
	{
		Debugger::DebugLoop();
		return true;
	}
	return false;
}

extern "C" __declspec(dllexport) bool __stdcall ScripterAutoDebugA(const char* szDebuggee)
{
	return ScripterAutoDebugW(ascii2unicode(szDebuggee).c_str());
}

extern "C" __declspec(dllexport) bool __stdcall ScripterExecuteWithTitanMistW(const wchar_t* szInputFile, const wchar_t* szOutputFile)
{
	if(ollylang.script.isloaded())
	{
		lstrcpyA(OutputPath, unicode2ascii(szOutputFile).c_str());
		if(Debugger::InitDebugEx(szInputFile, NULL, NULL, &AutoDebugEntry)){
			Debugger::DebugLoop();
			if(!TSErrorExit){
				return true;
			}
		}
	}
	return false;
}

extern "C" __declspec(dllexport) bool __stdcall ScripterExecuteWithTitanMistA(const char* szInputFile, const char* szOutputFile)
{
	return ScripterExecuteWithTitanMistW(ascii2unicode(szInputFile).c_str(), ascii2unicode(szOutputFile).c_str());
}

extern "C" __declspec(dllexport) void __stdcall ScripterSetLogCallback(fLogCallback Callback)
{
	TE_SetLogCallback(Callback);
}

// Help functions

void __stdcall AutoDebugEntry()
{
	ScripterResume();
}

bool FileNameFromHandle(HANDLE hFile, char Name[MAX_PATH])
{
bool Success = false;

	if(hFile != INVALID_HANDLE_VALUE)
	{
		HANDLE hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, NULL, GetFileSize(hFile, NULL), NULL);
		if(hFileMapping)
		{
			void* FileMappingView = MapViewOfFile(hFileMapping, FILE_MAP_READ, NULL, NULL, NULL);
			if(FileMappingView)
			{
				if(GetMappedFileName(GetCurrentProcess(), FileMappingView, Name, MAX_PATH))
				{
					Success = true;
					strcpy(Name, Translate::NativeName(Name));
				}
				UnmapViewOfFile(FileMappingView);
			}
			CloseHandle(hFileMapping);
		}
	}
	return Success;
}
