/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "precomp.h"

NTSTATUS FixupAti3duagBrokenFunctions(
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		IN PSECTION_OBJECT NewSectionObject,
		IN PVOID ImageBase,
		IN ULONG ViewSize);
NTSTATUS HandleMcAfeeAnnoyances(
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		IN PSECTION_OBJECT NewSectionObject,
		IN PVOID ImageBase,
		IN ULONG ViewSize);
NTSTATUS HandleNtdllPreprocessing(
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		IN PSECTION_OBJECT NewSectionObject,
		IN PVOID ImageBase,
		IN ULONG ViewSize);

struct
{
	PWSTR    FileName;
	NTSTATUS (*Function)(
			IN PPROCESS_OBJECT ProcessObject,
			IN PSECTION_OBJECT SectionObject,
			IN PSECTION_OBJECT NewSectionObject,
			IN PVOID ImageBase,
			IN ULONG ViewSize);
} CustomFileActions[] = 
{
	{ L"ATI3DUAG.DLL", FixupAti3duagBrokenFunctions },
#if !defined(DISABLE_MCAFEE_COMPAT_HACK)
	{ L"MCSHIELD.EXE", HandleMcAfeeAnnoyances       },
#endif

#if (!defined(DISABLE_NRE_RANDOMIZATION))        || \
	 (!defined(DISABLE_SEH_OVERWRITE_PREVENTION)) || \
    (!defined(DISABLE_NRER_RANDOMIZATION))
	{ L"NTDLL.DLL",    HandleNtdllPreprocessing     },
#endif
	{ L"NTDLL.DLL",    NrerSystemDllHandler         },
	{ NULL,            NULL                         },
};

//
// Global flags
//
BOOLEAN Ati3duagInUse = FALSE;

//
// Executes a custom action that may be associated with the section object that
// has been mapped.  This allows us to perform extended fixups
//
NTSTATUS ExecuteCustomSectionAction(
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		IN PSECTION_OBJECT NewSectionObject OPTIONAL,
		IN PVOID ImageBase,
		IN ULONG ViewSize)
{
	PFILE_OBJECT FileObject;
	NTSTATUS     Status = STATUS_SUCCESS;

	FileObject = GetSectionObjectControlAreaFilePointer(
			SectionObject);

	//
	// If the section has an associated file object
	//
	if ((FileObject) &&
	    (GetSectionObjectControlAreaFlags(
			SectionObject) & CONTROL_AREA_FLAG_IMAGE))
	{
		ULONG Index;

		//
		// Enumerate through all of the file-based section actions
		//
		for (Index = 0;
		     CustomFileActions[Index].FileName;
		     Index++)
		{
			//
			// Does the file name contain the one we are searching for?  If not,
			// skip it.
			//
			if (!RtleFindStringInUnicodeString(
					&FileObject->FileName,
					CustomFileActions[Index].FileName))
				continue;

			Status = CustomFileActions[Index].Function(
					ProcessObject,
					SectionObject,
					NewSectionObject,
					ImageBase,
					ViewSize);

			break;
		}
	}

	return Status;
}

//
// This routine searches for and fixes some of the broken functions in
// ATI3DUAG.DLL which assume memory will be mapped at a certain address in a
// user-mode process.  This is incredibly moronic on their part and can lead to
// blue screens if the assumption is not met.  I've only tested this code on one
// specific version of ATI3DUAG.DLL but would imagine this signature should be
// fairly common across other versions.  The signature matches the following
// assembly:
//
// .text:0002278E                 cmp     esi, eax
// .text:00022790                 jz      loc_2281C
// .text:00022796                 mov     edi, [ebp+arg_4]
// .text:00022799                 cmp     edi, eax
// .text:0002279B                 jbe     short loc_2281C
// .text:0002279D                 mov     [ebp+var_4], eax
// .text:000227A0                 mov     edx, 20000h
// .text:000227A5                 mov     [ebp+var_24], edx
// .text:000227A8                 movzx   ecx, word ptr ds:dword_20035+3
//
// By changing the 'cmp esi, eax' to two nops we are able to skip over the code
// that references the invalid pointer.  This does not seem to have any negative
// side effects that I have noticed thus far.  The jz succeeds because ZF is set
// by an 'xor eax, eax' a few instructions above the cmp.
//
// UPDATE:
//
// The above described solution seems to break Direct3D.  As such, I've switched
// to making it so memory allocations will not be randomized until 0x00020000
// has been allocated inside the process.  This really sucks, but I haven't been
// able to come up with a more reliable way around this problem.  This will
// *only* happen on boxes that load ATI3DUAG.DLL.  Anyone else have better
// ideas?
//
NTSTATUS FixupAti3duagBrokenFunctions(
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		IN PSECTION_OBJECT NewSectionObject,
		IN PVOID ImageBase,
		IN ULONG ViewSize)
{
	Ati3duagInUse = TRUE;

	return STATUS_SUCCESS;
}

BOOLEAN IsAtiHackRequired()
{
	return Ati3duagInUse;
}

#if !defined(DISABLE_MCAFEE_COMPAT_HACK)
//
// If MCSHIELD.EXE is being mapped into the supplied process context, flag this
// process as being forced to map images within the process' context.  This is
// to avoid nasty deadlock issues that occur when you attempt to randomize
// outside of MCSHIELD.EXE's process context from within a thread that is
// executing inside of MCSHIELD.EXE.
//
NTSTATUS HandleMcAfeeAnnoyances(
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		IN PSECTION_OBJECT NewSectionObject,
		IN PVOID ImageBase,
		IN ULONG ViewSize)
{
	PACCESS_TOKEN Token;

	//
	// Check to see if the process is running as SYSTEM.  We do this to mitigate
	// the threat of a non-TCB process tricking us into enabling in-process
	// memory mappings.
	//
	Token = PsReferencePrimaryToken(
			ProcessObject);

	if (Token)
	{
		BOOLEAN IsValid = FALSE;

		IsValid = SeTokenIsAdmin(
				Token);

		ObDereferenceObject(
				Token);

		if (!IsValid)
			return STATUS_UNSUCCESSFUL;
	}

	//
	// If the process is valid, enable in-process randomization.
	//
	SetProcessExecutionState(
			ProcessObject,
			PROCESS_EXECUTION_STATE_REQUIRES_IN_PROCESS_MAPPING,
			FALSE);

	return STATUS_SUCCESS;
}
#endif

//
// This routine is called every time NTDLL.DLL is mapped into the address space
// of a user-mode process.  If SEH overwrite prevention or random padding are
// enabled, the addresses of NtCreateThread, NtSetThreadContext, and potentially
// others are resolved and cached.  NtCreateThread is also subsequently hooked
// such that new thread creations can be intercepted.
//
// If NRER randomization is enabled or SEH overwrite protection is enabled, the
// NRER system DLL handler is called to initialize NRER in the calling process'
// context.
//
NTSTATUS HandleNtdllPreprocessing(
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		IN PSECTION_OBJECT NewSectionObject,
		IN PVOID ImageBase,
		IN ULONG ViewSize)
{
	PIMAGE_SET ImageSet = NULL;
	NTSTATUS   Status;
	PUCHAR     UserModeNtSetThreadContext = NULL;
	PUCHAR     UserModeNtCreateThread = NULL;

	//
	// Get the image set associated with this process so that we can cache the
	// location of NTDLL in all processes within this image set.
	//
	if ((NT_SUCCESS(GetImageSetForProcess(
			ProcessObject,
			&ImageSet))) &&
	    (ImageSet->NtdllImageBase == NULL))
	{
		ImageSet->NtdllImageBase = ImageBase;
		ImageSet->NtdllImageSize = ViewSize;
	}

	__try
	{
		do
		{
#if !defined(DISABLE_SEH_OVERWRITE_PREVENTION)

			//
			// If we haven't already hooked NtCreateThread...
			//
			if (!IsNtCreateThreadHooked())
			{
				//
				// Finally, resolve the address of NtCreateThread so that we can
				// install our hook.
				//
				if (!NT_SUCCESS(Status = LdrGetProcAddress(
						(ULONG_PTR)ImageBase,
						ViewSize,
						"NtCreateThread",
						(PVOID *)&UserModeNtCreateThread)))
				{
					DebugPrint(("HandleNtdllProcessing(): Failed to resolve NtCreateThread, %.8x.",
							Status));
					break;
				}

				//
				// Try to install the hook...
				//
				Status = InstallNtCreateThreadHook(
						ImageBase,
						ViewSize,
						UserModeNtCreateThread);

#if DBG
				if (NT_SUCCESS(Status))
				{
					DebugPrint(("HandleNtdllProcessing(): Successfully hooked NtCreateThread."));
				}
#endif
			}
#endif

#if !defined(DISABLE_SEH_OVERWRITE_PREVENTION) || !defined(DISABLE_NRER_RANDOMIZATION)
#endif

		} while (0);

	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		Status = GetExceptionCode();

		DebugPrint(("HandleNtdllPreprocessing(): Caught exception: %.8x.",
				Status));
	}

	return STATUS_SUCCESS;
}
