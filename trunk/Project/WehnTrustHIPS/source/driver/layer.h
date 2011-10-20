/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_DRIVER_LAYER_H
#define _WEHNTRUST_DRIVER_LAYER_H

typedef struct _CALL_LAYER
{
	ULONG_PTR FunctionToHookAddress;
	ULONG_PTR RealFunctionPostPreamble;
	ULONG_PTR HookFunctionAddress;
	ULONG_PTR Target;
	PVOID     RealFunction;
	UCHAR     SavePreamble[6];
	PVOID     LockedAddress;
	PMDL      Mdl;
} CALL_LAYER, *PCALL_LAYER;

NTSTATUS InstallCallLayer(
		IN ULONG_PTR FunctionToHookAddress, 
		IN ULONG_PTR HookFunctionAddress, 
		OUT PCALL_LAYER *layer);
NTSTATUS UninstallCallLayer(
		IN PCALL_LAYER Layer);

NTSTATUS GetSystemCallRoutine(
		IN PVOID SystemCallToHook OPTIONAL,
		IN ULONG SystemCallIndex OPTIONAL,
		OUT PULONG_PTR SystemCallAddress);


#endif
