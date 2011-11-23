#ifndef __COMMHDR__
#define __COMMHDR__


#include <Winsvc.h>
#include "loadDriver.h"
#include "ntdll.h"
#include <dbghelp.h>
#include <stdio.h>
#include <windows.h>
#include <winioctl.h>
#include <shlobj.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "symMgr.h"
#include "GalaxyApMGR.h"
#include "..\TEST_KIDISPAT\TEST_KIDISPAT.h"
#include "..\HookPortBypass\HookPortBypass.h"

#pragma comment( lib, "dbghelp.lib" )

#pragma comment(lib,"ntdll")


void	setlog(char *plog);
VOID inline myprint(char *fmt,...)
{
	va_list argptr; 
	int cnt; 
	va_start(argptr, fmt);
	char buffer[2048]={0};
	cnt = _vsnprintf(buffer,2047 ,fmt, argptr);
	va_end(argptr);
// 	OutputDebugString("[GalaxyAp]: ");
// 	OutputDebugString(buffer);
	setlog(buffer);
}
#endif