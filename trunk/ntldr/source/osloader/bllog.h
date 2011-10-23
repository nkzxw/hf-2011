//********************************************************************
//	created:	17:8:2008   21:53
//	file:		bllog.h
//	author:		tiamo
//	purpose:	log support
//********************************************************************

#pragma once

//
// log to display
//
#define LOG_DISPLAY										0x0001

//
// log to file
//
#define LOG_LOGFILE										0x0002

//
// log to debugger
//
#define LOG_DEBUGGER									0x0004

//
// wait for a key
//
#define LOG_WAIT										0x8000

//
// send to all devices
//
#define LOG_ALL											(LOG_DISPLAY | LOG_LOGFILE | LOG_DEBUGGER)

//
// and wait
//
#define LOG_ALL_W										(LOG_ALL | LOG_WAIT)

//
// initialize log
//
VOID BlLogInitialize(__in ULONG LogfileDeviceId);

//
// send a log string
//
VOID BlLogPrint(__in ULONG Targets,__in PCHAR Format,...);

//
// log wait
//
VOID BlLogWait();

//
// terminate
//
VOID BlLogTerminate();