//********************************************************************
//	created:	17:8:2008   21:47
//	file:		bllog.cpp
//	author:		tiamo
//	purpose:	log
//********************************************************************

#include "stdafx.h"

//
// log will send to
//
ULONG													BlLogActiveTargets = 0;

//
// log device id
//
ULONG													BlLogFileId;

//
// initialize log
//
VOID BlLogInitialize(__in ULONG LogfileDeviceId)
{
	BlLogActiveTargets									= 0;

	if(BlLoaderBlock->LoadOptions)
	{
		if(strstr(BlLoaderBlock->LoadOptions,"DBGDISPLAY"))
			BlLogActiveTargets							|= LOG_DISPLAY;

		if(strstr(BlLoaderBlock->LoadOptions,"DBGDEBUGGER"))
			BlLogActiveTargets							|= LOG_DEBUGGER;

		if(strstr(BlLoaderBlock->LoadOptions,"DBGLOG") && BlOpen(LogfileDeviceId,"\\LDRDBG.LOG",ArcSupersedeReadWrite,&BlLogFileId) == ESUCCESS)
			BlLogActiveTargets |= LOG_LOGFILE;
	}
}

//
// wait for key
//
VOID BlLogWaitForKeystroke()
{
	if(BlLogActiveTargets & LOG_DISPLAY)
	{
		while(1)
		{
			if(ArcGetReadStatus(0) == ESUCCESS)
			{
				UCHAR Key;
				ULONG Count;
				ArcRead(0,&Key,sizeof(Key),&Count);
				break;
			}
		}
	}
}

//
// log wait
//
VOID BlLogWait()
{
	if(BlLogActiveTargets & LOG_DEBUGGER)
		DbgBreakPoint();
	else if(BlLogActiveTargets & LOG_DISPLAY)
		BlLogWaitForKeystroke();
}

//
// terminate
//
VOID BlLogTerminate()
{
	BlLogPrint(BlLogActiveTargets,"BlLog terminating");

	if(BlLogActiveTargets & LOG_LOGFILE)
		BlClose(BlLogFileId);

	BlLogActiveTargets									= 0;
}

//
// send a log string
//
VOID BlLogPrint(__in ULONG Targets,__in PCHAR Format,...)
{
	BOOLEAN Wait										= Targets & LOG_WAIT ? TRUE : FALSE;
	Targets												= BlLogActiveTargets & Targets;
	if(!Targets)
		return;

	CHAR Buffer[80];
	va_list list;
	va_start(list,Format);

	ULONG Length										= _vsnprintf(Buffer,ARRAYSIZE(Buffer) - 1,Format,list);
	if(Length != -1)
		memset(Buffer + Length,' ',ARRAYSIZE(Buffer) - 2 - Length);

	Buffer[ARRAYSIZE(Buffer) - 2]						= '\r';
	Buffer[ARRAYSIZE(Buffer) - 1]						= '\n';
	va_end(list);

	if(Targets & LOG_LOGFILE)
		BlWrite(BlLogFileId,Buffer,sizeof(Buffer),&Length);

	if(Targets & LOG_DISPLAY)
		ArcWrite(BlConsoleOutDeviceId,Buffer,sizeof(Buffer),&Length);

	if(Targets & LOG_DEBUGGER)
		DbgPrint(Buffer);

	if(Wait)
		BlLogWait();
}
