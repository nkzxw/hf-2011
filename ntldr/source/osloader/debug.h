//********************************************************************
//	created:	11:8:2008   17:39
//	file:		debug.h
//	author:		tiamo
//	purpose:	debug
//********************************************************************

#pragma once

//
// single step exception handler
//
VOID BdTrap01();

//
// breakpoint exception handler
//
VOID BdTrap03();

//
// general protection exception handler
//
VOID BdTrap0d();

//
// page fault exception handler
//
VOID BdTrap0e();

//
// debug service interrupt handler
//
VOID BdTrap2d();

//
// dbg print
//
ULONG DbgPrint(__in PCCH Format,...);

//
// debug print
//
VOID DebugPrint(__in PSTRING Output,__in ULONG ComponentId,__in ULONG Level);

//
// load image symbols
//
VOID DbgLoadImageSymbols(__in PSTRING FileName,__in PVOID ImageBase,__in ULONG ProcessId);

//
// unload symbols
//
VOID DbgUnLoadImageSymbols(__in PSTRING FileName,__in PVOID ImageBase,__in ULONG ProcessId);

//
// breakpoint with status
//
VOID DbgBreakPointWithStatus(__in ULONG Status);

//
// breakpoint
//
VOID DbgBreakPoint();

//
// init debugger
//
VOID BdInitDebugger(__in PCHAR ImageName,__in ULONG BaseAddress,__in PCHAR LoadOptions);

//
// stop debugger
//
VOID BdStopDebugger();

//
// enabled
//
extern BOOLEAN BdDebuggerEnabled;