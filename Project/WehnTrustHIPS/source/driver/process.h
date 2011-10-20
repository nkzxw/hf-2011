/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_DRIVER_PROCESS_H
#define _WEHNTRUST_DRIVER_PROCESS_H

//
// This flag indicates that NRER executable mirroring is required.  This flag is
// only used if DISABLE_NRE_RANDOMIZATION is not defined.
//
#define PROCESS_EXECUTION_STATE_NRER_MIRRORING_NEEDED       (1 << 0)

//
// This flag indicates that a process requires images to be mapped in-process
// rather than from the context of the system process.
//
#define PROCESS_EXECUTION_STATE_REQUIRES_IN_PROCESS_MAPPING (1 << 1)

//
// This flag indicates that the NRER.dll user-mode DLL has been loaded into a
// process' address space.
//
#define PROCESS_EXECUTION_STATE_NRER_DLL_LOADED             (1 << 2)

//
// This flag indicates that the NRER.dll user-mode DLL has been initialized in
// that the NreInitialize entry point has been called.
//
#define PROCESS_EXECUTION_STATE_NRER_DLL_INITIALIZED        (1 << 3)

NTSTATUS InitializeProcess();
NTSTATUS DeinitializeProcess();

VOID FlagProcessAsExempted(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG ExemptionFlags);
BOOLEAN IsProcessExempted(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG ExemptionFlag);

VOID SetProcessExecutionState(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG ExecutionState,
		IN BOOLEAN Unset);
BOOLEAN IsProcessExecutionState(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG ExecutionState);

VOID SetProcessExecutableMapping(
		IN PPROCESS_OBJECT ProcessObject,
		IN PIMAGE_SET_MAPPING Mapping);
PIMAGE_SET_MAPPING GetProcessExecutableMapping(
		IN PPROCESS_OBJECT ProcessObject);

VOID SetProcessNrerDispatchTable(
		IN PPROCESS_OBJECT ProcessObject,
		IN PNRER_DISPATCH_TABLE NrerDispatchTable);
BOOLEAN GetProcessNrerDispatchTable(
		IN PPROCESS_OBJECT ProcessObject,
		OUT PNRER_DISPATCH_TABLE NrerDispatchTable);

VOID SetProcessNreExecutionFlag(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG ExecutionFlag,
		IN BOOLEAN Unset);
ULONG GetProcessNreExecutionFlags(
		IN PPROCESS_OBJECT ProcessObject);

ULONG_PTR GetNextRandomizedBaseForProcess(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG RegionSize,
		IN ULONG RegionIncrement);

#endif
