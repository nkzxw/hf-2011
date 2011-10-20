/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_DRIVER_EXECUTIVE_H
#define _WEHNTRUST_DRIVER_EXECUTIVE_H

typedef enum _COUNTER_TYPE
{
	RandomizedDllCounter,
	ImageSetCounter,
	RandomizationsExemptedCounter,
	RandomizedAllocationsCounter
} COUNTER_TYPE, *PCOUNTER_TYPE;

BOOLEAN IsFreeVersion();

VOID InitializeExecutive();

BOOLEAN AddThreadExemption(
		IN PVOID Thread);
BOOLEAN IsThreadExempted(
		IN PVOID Thread);
VOID RemoveThreadExemption(
		IN PVOID Thread);

BOOLEAN AddRegionExemption(
		IN PVOID RegionBase,
		IN ULONG RegionSize);
BOOLEAN IsRegionExempted(
		IN PVOID RegionBase,
		IN ULONG RegionSize,
		OUT PULONG EndDisplacement);
VOID RemoveRegionExemption(
		IN PVOID RegionBase,
		IN ULONG RegionSize);

VOID EnableRandomizationSubsystems(
		IN ULONG RandomizationSubsystems,
		IN BOOLEAN Save);
BOOLEAN IsRandomizationSubsystemEnabled(
		IN ULONG RandomizationSubsystems);
VOID DisableRandomizationSubsystems(
		IN ULONG RandomizationSubsystems);

VOID IncrementExecutiveCounter(
		IN COUNTER_TYPE Type,
		IN ULONG Increment OPTIONAL);
VOID DecrementExecutiveCounter(
		IN COUNTER_TYPE Type,
		IN ULONG Decrement OPTIONAL);
VOID ResetExecutiveCounter(
		IN COUNTER_TYPE Type);

NTSTATUS GetExecutiveStatistics(
		OUT PWEHNTRUST_STATISTICS Statistics);

#endif
