/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_COMMON_WEHNTRUSTCOMMON_H
#define _WEHNTRUST_COMMON_WEHNTRUSTCOMMON_H

#ifdef USER_MODE

typedef struct _ANSI_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PCHAR  Buffer;
} ANSI_STRING, *PANSI_STRING;

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWCHAR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

#endif

////
//
// Constants & Enumerations
//
////

//
// The version of the common library.  This is used by structures for
// versioning.  It should be incremented once per major structure revision.
//
#define WEHNTRUST_VERSION                   0x0001

//
// This flag indicates that the specified image cannot be unloaded from an image
// set after it has been mapped in.  This is used specifically for USER32.DLL
// because the PFN array that is supplied to the kernel for that session must
// not change, and if we were to allow user32 to be unloaded then we would lose
// the previous address at which the PFN array was mapped and thus lead to
// chaos.
//
#define IMAGE_FLAG_CANNOT_UNLOAD           (1 << 0)

//
// This flag tells the driver that it should build the jump tables relative to
// every other image set than the one currently being used rather than building
// the jump table relative to the original base address of the image supplied in
// the registry.  The offsets will still be calculated from the address table in
// the registry, but the jump table will be built relative to every other image
// set.  This is used for kernel32 due to the fact that a process using another
// image set could CreateRemoteThread on another one and thus supply a
// _BaseProcessStartThunk or _BaseThreadStartThunk that is relative to its image
// set and not the new image set.
//
#define IMAGE_FLAG_SET_RELATIVE_JUMP_TABLE (1 << 5)

//
// These bitvector values are used to indicate a given randomization subsystem.
// Their context is variably defined.
//
#define RANDOMIZATION_SUBSYSTEM_DLL         (1 << 0)
#define RANDOMIZATION_SUBSYSTEM_ALLOCATIONS (1 << 1)
#define RANDOMIZATION_SUBSYSTEM_NRE         (1 << 2)
#define RANDOMIZATION_SUBSYSTEM_ALL         0xffffffff

//
// Checksumming algorithm types 
//
typedef enum _IMAGE_CHECKSUM_TYPE
{	
	IMAGE_CHECKSUM_TYPE_NONE  = 0,
	IMAGE_CHECKSUM_TYPE_ROR16 = 1, // Unused
	IMAGE_CHECKSUM_TYPE_SHA1  = 2,
} IMAGE_CHECKSUM_TYPE, *PIMAGE_CHECKSUM_TYPE;

////
//
// Data structures
//
////

#define InitializeStructure(Object, Size) \
	((PWEHNTRUST_STATISTICS)(Object))->Info = ((Size & 0xffff) | (WEHNTRUST_VERSION << 16))
#define GetStructureSize(Object) \
	(((PWEHNTRUST_STATISTICS)(Object))->Info & 0xffff)
#define GetStructureVersion(Object) \
	((((PWEHNTRUST_STATISTICS)(Object))->Info >> 16) & 0xffff)
#define InlineInitializeStructure(Size) \
	((Size & 0xffff) | (WEHNTRUST_VERSION << 16))

//
// This structure contains information about the state of the randomization
// driver, such as the number of currently randomized images, whether or not
// it's currently started or stopped, and so on.
//
typedef struct _WEHNTRUST_STATISTICS_V1
{
	//
	// Structure meta information
	//
	ULONG Info;

	//
	// A bitvector of enabled states for the randomization subsystems defined in
	// RANDOMIZATION_SUBSYSTEM_XXX
	//
	ULONG Enabled;
	//
	// The number of image sets that currently exist in the driver
	//
	ULONG NumberOfImageSets;
	//
	// The total number of DLLs that are currently randomized across all image
	// sets.
	//
	ULONG NumberOfRandomizedDlls;
	//
	// The number of randomization attempts that have been exempted for one
	// reason or another.
	//
	ULONG NumberOfRandomizationsExempted;
	//
	// The number of randomized allocations that have occurred.  This number does
	// not include library memory range allocations.
	//
	ULONG NumberOfRandomizedAllocations;
} WEHNTRUST_STATISTICS_V1, *PWEHNTRUST_STATISTICS_V1;

////
//
// Exemptions
//
////

//
// The type and scope of exemptions that can exist
//
typedef ULONG EXEMPTION_TYPE, *PEXEMPTION_TYPE;
typedef ULONG EXEMPTION_SCOPE, *PEXEMPTION_SCOPE;

#define UnknownExemption     0
#define ApplicationExemption (1 << 0)
#define ImageFileExemption   (1 << 1)
#define DirectoryExemption   (1 << 2)
#define AnyExemption         0xffffffff

#define UnknownScope         0
#define GlobalScope          (1 << 0)
#define UserScope            (1 << 1)
#define AnyScope             0xffffffff

//
// The types of things that can be exempted
//
#define EXEMPT_IMAGE_FILES        (1 << 0)
#define EXEMPT_MEMORY_ALLOCATIONS (1 << 1)
#define EXEMPT_NRER               (1 << 2)
#define EXEMPT_NRE_SEH            (1 << 10)
#define EXEMPT_NRE_STACK          (1 << 11)
#define EXEMPT_NRE_FORMAT         (1 << 12)
#define EXEMPT_ALL                0xffffffff

//
// Licensing constants
//
#define LICENSE_END_USER_MAX_SIZE    128
#define LICENSE_CORPORATION_MAX_SIZE 128

//
// This structure contains information about an exemption
//
typedef struct _WEHNTRUST_EXEMPTION_INFO_V1
{
	//
	// The size information for this structure such that versioning can be
	// supported
	//
	ULONG Info;

	//
	// The type of exemption
	//
	EXEMPTION_TYPE Type;

	//
	// The scope of the exemption
	//
	EXEMPTION_SCOPE Scope;

	//
	// The flags of things that should be exempted
	//
	ULONG Flags;

	//
	// Reserved for future growth
	//
	ULONG Reserved[4];

	//
	// The size and path of the thing that is to be exempted
	//
	USHORT ExemptionPathSize;
	WCHAR ExemptionPath[1];

} WEHNTRUST_EXEMPTION_INFO_V1, *PWEHNTRUST_EXEMPTION_INFO_V1;

#if WEHNTRUST_VERSION == 0x0001
#define WEHNTRUST_STATISTICS  WEHNTRUST_STATISTICS_V1
#define PWEHNTRUST_STATISTICS PWEHNTRUST_STATISTICS_V1
#define WEHNTRUST_EXEMPTION_INFO  WEHNTRUST_EXEMPTION_INFO_V1
#define PWEHNTRUST_EXEMPTION_INFO PWEHNTRUST_EXEMPTION_INFO_V1
#else
#error "Unsupported WehnTrust version number."
#endif

////
//
// NRER (Non-relocatable Executable Randomization) support
//
////

#ifndef NTAPI
#define NTAPI _stdcall
#endif

//
// This structure is exported by NRER.DLL and is populated by the driver when it
// is initially mapped into the context of a process.  The symbols are cached
// internally by the driver the first time NTDLL is mapped so that they do not
// have to be continually resolved.
//
typedef struct _NRER_NT_DISPATCH_TABLE
{
	// 
	// The versioning and size information
	//
	ULONG Info;

	//
	// The base address of NTDLL
	//
	PVOID NtdllImageBase;

	//
	// The dispatch table itself
	//
	ULONG (NTAPI *LdrGetProcedureAddress)(
			IN PVOID ModuleHandle,
			IN PANSI_STRING FunctionName OPTIONAL,
			IN USHORT Ordinal OPTIONAL,
			OUT PVOID *FunctionAddress);
	ULONG (NTAPI *LdrLoadDll)(
			IN PWCHAR PathToFile OPTIONAL,
			IN ULONG Flags OPTIONAL,
			IN PUNICODE_STRING ModuleFileName,
			OUT PHANDLE ModuleHandle);
	ULONG (NTAPI *NtProtectVirtualMemory)(
			IN HANDLE ProcessHandle,
			IN OUT PVOID *BaseAddress,
			IN OUT PULONG ProtectSize,
			IN ULONG NewProtect,
			OUT PULONG OldProtect);

	//
	// Routines that have to be hooked by NRER.dll
	//
	ULONG (NTAPI *LdrInitializeThunk)(
			IN PVOID Unused1,
			IN PVOID Unused2,
			IN PVOID Unused3);
	ULONG (NTAPI *KiUserExceptionDispatcher)(
			IN PEXCEPTION_RECORD ExceptionRecord,
			IN PCONTEXT Context);
} NRER_NT_DISPATCH_TABLE, *PNRER_NT_DISPATCH_TABLE;

//
// The following flags are used to tell the NRER user-mode DLL what features it
// should enforce when it's initialized and loaded.
//

//
// Enforce SEH overwrite protection
//
#define NRE_FLAG_ENFORCE_SEH   0x00000001
//
// Enforce stack overflow protection
//
#define NRE_FLAG_ENFORCE_STACK 0x00000002
//
// Enforce format string protections
//
#define NRE_FLAG_ENFORCE_FMT   0x00000004
//
// Flag to disable all protections
//
#define NRE_FLAG_DISABLE_ALL   0xf0000000

#define NRE_FLAG_ENFORCE_ALL   \
	NRE_FLAG_ENFORCE_SEH   |    \
	NRE_FLAG_ENFORCE_STACK |    \
	NRE_FLAG_ENFORCE_FMT

////
//
// User<->Kernel communication
//
////

#define FILE_DEVICE_WEHNTRUST 0x00009293 // ((R + A) | (N + D << 8))

//
// These two control codes are used to start and stop randomization.  This does
// not imply that currently randomized images will be unrandomized but instead
// stops or starts future images from being randomized.  These two control codes
// require administrative access.  Both of these control codes requires a ULONG
// argument that specifies a bitvector of the subsystems that are to be
// started or stopped (RANDOMIZATION_SUBSYSTEM_XXX)
//
#define IOCTL_WEHNTRUST_START            (ULONG) CTL_CODE(FILE_DEVICE_WEHNTRUST, 0x00, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WEHNTRUST_STOP             (ULONG) CTL_CODE(FILE_DEVICE_WEHNTRUST, 0x01, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// This control code is used to obtain statistics from the driver which is
// passed back in a WEHNTRUST_STATISTICS structure.  This control code can be
// accessed by a non-privileged user.
//
#define IOCTL_WEHNTRUST_GET_STATISTICS   (ULONG) CTL_CODE(FILE_DEVICE_WEHNTRUST, 0x10, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// These control codes are used to add and remove exemptions from the driver.
//
// The add, remove, and flush control codes take a WEHNTRUST_EXEMPTION_INFO structure.
//
// The flush control code only pays attention to the type and scope.
//
#define IOCTL_WEHNTRUST_ADD_EXEMPTION    (ULONG) CTL_CODE(FILE_DEVICE_WEHNTRUST, 0x20, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WEHNTRUST_REMOVE_EXEMPTION (ULONG) CTL_CODE(FILE_DEVICE_WEHNTRUST, 0x21, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WEHNTRUST_FLUSH_EXEMPTIONS (ULONG) CTL_CODE(FILE_DEVICE_WEHNTRUST, 0x22, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif
