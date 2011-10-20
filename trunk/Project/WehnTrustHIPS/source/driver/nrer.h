#ifndef _WEHNTRUST_DRIVER_NRER_H
#define _WEHNTRUST_DRIVER_NRER_H

//
// This structure contains the dispatch table for routines that need to be
// called or referenced by the kernel within the NRER user-mode DLL.
//
typedef struct _NRER_DISPATCH_TABLE
{
	PVOID             NrerImageBase;
	ULONG             NrerImageSize;
	KAPC_USER_ROUTINE CreateSehValidationFrame;
	KAPC_USER_ROUTINE NreInitialize;
} NRER_DISPATCH_TABLE, *PNRER_DISPATCH_TABLE;

NTSTATUS InitializeNreSubsystem();

NTSTATUS NrerSystemDllHandler(
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		IN PSECTION_OBJECT NewSectionObject,
		IN PVOID ImageBase,
		IN ULONG ViewSize);

NTSTATUS CreateNrerImageSetMapping(
		IN PPROCESS_OBJECT ProcessObject,
		IN PIMAGE_SET ImageSet OPTIONAL,
		OUT PIMAGE_SET_MAPPING *NrerMapping OPTIONAL,
		OUT PNRER_DISPATCH_TABLE NrerDispatchTable OPTIONAL);

//
// SEH overwrite protection exports
//
NTSTATUS CreateSehValidationFrameForThread(
		IN HANDLE ProcessHandle,
		IN HANDLE ThreadHandle);

#endif
