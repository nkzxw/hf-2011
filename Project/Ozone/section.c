/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		section.c
 *
 * Abstract:
 *
 *		This module defines various routines used for hooking section objects related routines.
 *		Section objects are objects that can be mapped into the virtual address space of a process.
 *		The Win32 API refers to section objects as file-mapping objects.
 *
 *		Hooked routines protect "\Device\PhysicalMemory" device from being accessed.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 29-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */


#include <NTDDK.h>
#include "section.h"
#include "hookproc.h"
#include "pathproc.h"
#include "process.h"
#include "accessmask.h"
#include "procname.h"
#include "learn.h"
#include "log.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitSectionHooks)
#endif


fpZwCreateSection		OriginalNtCreateSection = NULL;
fpZwOpenSection			OriginalNtOpenSection = NULL;
fpZwMapViewOfSection	OriginalNtMapViewOfSection = NULL;


//XXX make sure people cannot create symlinks to physicalmemory or we at least resolve all of them!
// http://www.blackhat.com/presentations/bh-usa-03/bh-us-03-rutkowski/bh-us-03-rutkowski-r2.pdf


/*
 * HookedNtCreateSection()
 *
 * Description:
 *		This function mediates the NtCreateSection() system service and checks the
 *		provided section name against the global and current process security policies.
 *
 *		NOTE: ZwCreateSection creates a section object. [NAR]
 *
 * Parameters:
 *		Those of NtCreateSection().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtCreateSection().
 */

NTSTATUS
NTAPI
HookedNtCreateSection
(
	OUT PHANDLE SectionHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PLARGE_INTEGER SectionSize OPTIONAL,
	IN ULONG Protect,
	IN ULONG Attributes,
	IN HANDLE FileHandle
)
{
	PCHAR	FunctionName = "HookedNtCreateSection";


	HOOK_ROUTINE_START(SECTION);


	ASSERT(OriginalNtCreateSection);

	rc = OriginalNtCreateSection(SectionHandle, DesiredAccess, ObjectAttributes, SectionSize,
								 Protect, Attributes, FileHandle);


//	HOOK_ROUTINE_FINISH(SECTION);
	if (LearningMode == TRUE)
	{
		if (GetPathFromOA(ObjectAttributes, SECTIONNAME, MAX_PATH, DO_NOT_RESOLVE_LINKS))
		{
			/*
			 * Special Case.
			 * \KnownDlls\* requests are processed as DLL rules.
			 *
			 * In addition, they are processed even if NtCreateSection() failed because not
			 * all the existing DLLs are "known".
			 */

			if (_strnicmp(SECTIONNAME, "\\KnownDlls\\", 11) == 0)
			{
				AddRule(RULE_DLL, SECTIONNAME, Get_SECTION_OperationType(DesiredAccess));
			}
			else if (NT_SUCCESS(rc))
			{
				AddRule(RULE_SECTION, SECTIONNAME, Get_SECTION_OperationType(DesiredAccess));
			}
		}
	}

	HOOK_ROUTINE_EXIT(rc);
}



/*
 * HookedNtOpenSection()
 *
 * Description:
 *		This function mediates the NtOpenSection() system service and checks the
 *		provided section name against the global and current process security policies.
 *
 *		NOTE: ZwOpenSection opens a section object. [NAR]
 *
 * Parameters:
 *		Those of NtOpenSection().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtOpenSection().
 */

NTSTATUS
NTAPI
HookedNtOpenSection
(
    OUT PHANDLE  SectionHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes
)
{
	PCHAR	FunctionName = "HookedNtOpenSection";


	HOOK_ROUTINE_START(SECTION);


	ASSERT(OriginalNtOpenSection);

	rc = OriginalNtOpenSection(SectionHandle, DesiredAccess, ObjectAttributes);


//	HOOK_ROUTINE_FINISH(SECTION);
	if (LearningMode == TRUE)
	{
		if (GetPathFromOA(ObjectAttributes, SECTIONNAME, MAX_PATH, DO_NOT_RESOLVE_LINKS))
		{
			/*
			 * Special Case.
			 * \KnownDlls\* requests are processed as DLL rules.
			 *
			 * In addition, they are processed even if NtOpenSection() failed because not
			 * all the existing DLLs are "known".
			 */

			if (_strnicmp(SECTIONNAME, "\\KnownDlls\\", 11) == 0)
			{
				AddRule(RULE_DLL, SECTIONNAME, Get_SECTION_OperationType(DesiredAccess));
			}
			else if (NT_SUCCESS(rc))
			{
				AddRule(RULE_SECTION, SECTIONNAME, Get_SECTION_OperationType(DesiredAccess));
			}
		}
	}

	HOOK_ROUTINE_EXIT(rc);
}



/*
 * HookedNtMapViewOfSection()
 *
 * Description:
 *		This function mediates the NtMapViewOfSection() system service and checks the
 *		provided section name against the global and current process security policies.
 *
 *		NOTE: ZwMapViewOfSection maps a view of a section to a range of virtual addresses. [NAR]
 *
 * Parameters:
 *		Those of NtMapViewOfSection().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtMapViewOfSection().
 */

NTSTATUS
NTAPI
HookedNtMapViewOfSection
(
	IN HANDLE SectionHandle,
	IN HANDLE ProcessHandle,
	IN OUT PVOID *BaseAddress,
	IN ULONG ZeroBits,
	IN ULONG CommitSize,
	IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
	IN OUT PULONG ViewSize,
	IN SECTION_INHERIT InheritDisposition,
	IN ULONG AllocationType,
	IN ULONG Protect
)
{
	CHAR	section[512];


	HOOK_ROUTINE_ENTER();

//	LOG(LOG_SS_SECTION, LOG_PRIORITY_DEBUG, ("%d HookedNtMapViewOfSection: %x %x %x %x\n", (ULONG) PsGetCurrentProcessId(), SectionHandle, ProcessHandle, BaseAddress, CommitSize));
/*
	if (GetPathFromOA(ObjectAttributes, section, RESOLVE_LINKS))
	{
		LOG(LOG_SS_SECTION, LOG_PRIORITY_DEBUG, ("HookedNtMapViewOfSection: %s\n", section));
//		if (PolicyCheck(&gSecPolicy, key, GetRegistryOperationType(DesiredAccess)) == STATUS_ACCESS_DENIED)

//			HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}
*/

	ASSERT(OriginalNtMapViewOfSection);

	rc =  OriginalNtMapViewOfSection(SectionHandle, ProcessHandle, BaseAddress, ZeroBits, CommitSize,
									SectionOffset, ViewSize, InheritDisposition, AllocationType, Protect);

	HOOK_ROUTINE_EXIT(rc);
}



/*
 * InitSectionHooks()
 *
 * Description:
 *		Initializes all the mediated section object operation pointers. The "OriginalFunction" pointers
 *		are initialized by InstallSyscallsHooks() that must be called prior to this function.
 *
 *		NOTE: Called once during driver initialization (DriverEntry()).
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

BOOLEAN
InitSectionHooks()
{
	if ( (OriginalNtCreateSection = (fpZwCreateSection) ZwCalls[ZW_CREATE_SECTION_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_SECTION, LOG_PRIORITY_DEBUG, ("InitSectionHooks: OriginalNtCreateSection is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtOpenSection = (fpZwOpenSection) ZwCalls[ZW_OPEN_SECTION_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_SECTION, LOG_PRIORITY_DEBUG, ("InitSectionHooks: OriginalNtOpenSection is NULL\n"));
		return FALSE;
	}
/*
	if ((OriginalNtMapViewOfSection = (fpZwMapViewOfSection) ZwCalls[ZW_MAPVIEW_SECTION_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_SECTION, LOG_PRIORITY_DEBUG, ("InitSectionHooks: OriginalNtMapViewOfSection is NULL\n"));
		return FALSE;
	}
*/
	return TRUE;
}
