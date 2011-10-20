/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_NATIVE_DRIVERCLIENT_H
#define _WEHNTRUST_NATIVE_DRIVERCLIENT_H

//
// This class is responsible for communicating with the device driver
//
class DriverClient
{
	public:
		static HANDLE Open();
		static VOID Close(
				IN HANDLE Driver);

		static DWORD Start(
				IN HANDLE Driver,
				IN ULONG Subsystems);
		static DWORD Stop(
				IN HANDLE Driver,
				IN ULONG Subsystems);

		static DWORD GetStatistics(
				IN HANDLE Driver,
				OUT PWEHNTRUST_STATISTICS Statistics);

		static DWORD AddExemption(
				IN HANDLE Driver,
				IN EXEMPTION_SCOPE Scope,
				IN EXEMPTION_TYPE Type,
				IN ULONG Flags,
				IN LPCTSTR ExemptionPath);
		static DWORD RemoveExemption(
				IN HANDLE Driver,
				IN EXEMPTION_SCOPE Scope,
				IN EXEMPTION_TYPE Type,
				IN ULONG Flags,
				IN LPCTSTR ExemptionPath,
				IN BOOLEAN IsNtPath = TRUE);
		static DWORD FlushExemptions(
				IN HANDLE Driver,
				IN EXEMPTION_SCOPE Scope,
				IN EXEMPTION_TYPE Type);
		
		static BOOLEAN GetNtPath(
				IN LPCTSTR FilePath,
				OUT PUNICODE_STRING NtFilePath,
				IN BOOLEAN IsNtPath = FALSE);
		static VOID FreeNtPath(
				IN PUNICODE_STRING NtFilePath);
	protected:
		static PWEHNTRUST_EXEMPTION_INFO InitializeExemptionInfo(
				IN EXEMPTION_SCOPE Scope,
				IN EXEMPTION_TYPE Type,
				IN ULONG Flags,
				IN LPCTSTR ExemptionPath,
				IN BOOLEAN ExemptionPathIsNt,
				OUT LPDWORD ExemptionInfoSize);
		static VOID FreeExemptionInfo(
				IN PWEHNTRUST_EXEMPTION_INFO ExemptionInfo);
};

#endif
