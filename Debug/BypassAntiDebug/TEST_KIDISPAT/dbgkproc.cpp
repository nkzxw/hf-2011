/*++
    This module implements process control primitives for the
    Dbg component of NT
--*/

#include "commhdr.h"

NTSTATUS  (WINAPI *PsResumeThread)(IN PETHREAD Thread,OUT PULONG PreviousSuspendCount OPTIONAL);
NTSTATUS  (WINAPI *PsTerminateProcess)(IN PEPROCESS Process,IN NTSTATUS Status);
PEPROCESS (WINAPI *PsGetNextProcess) (IN PEPROCESS Process);
PETHREAD  (WINAPI *PsGetNextProcessThread) (IN PEPROCESS Process,IN PETHREAD Thread);
NTSTATUS  (WINAPI *PsSuspendThread) (IN PETHREAD Thread,OUT PULONG PreviousSuspendCount );

NTSTATUS  (WINAPI *MmGetFileNameForAddress) (IN PVOID ProcessVa,OUT PUNICODE_STRING FileName);
NTSTATUS  (WINAPI *ObDuplicateObject) (IN PEPROCESS SourceProcess,IN HANDLE SourceHandle,IN PEPROCESS TargetProcess OPTIONAL,OUT PHANDLE TargetHandle OPTIONAL,IN ACCESS_MASK DesiredAccess,IN ULONG HandleAttributes,IN ULONG Options,IN KPROCESSOR_MODE PreviousMode );

NTSTATUS (WINAPI *LpcRequestWaitReplyPortEx) (IN PVOID PortAddress,IN PPORT_MESSAGE RequestMessage,OUT PPORT_MESSAGE ReplyMessage);
VOID      (WINAPI *PsQuitNextProcessThread) (IN PETHREAD Thread);
VOID      (WINAPI *PsCallImageNotifyRoutines)(IN PUNICODE_STRING FullImageName,IN HANDLE ProcessId, IN PIMAGE_INFO ImageInfo );


NTSTATUS  (WINAPI * MmCopyVirtualMemory)(IN PEPROCESS FromProcess,IN CONST VOID *FromAddress,IN PEPROCESS ToProcess,OUT PVOID ToAddress,IN SIZE_T BufferSize,IN KPROCESSOR_MODE PreviousMode,OUT PSIZE_T NumberOfBytesCopied);

FAST_MUTEX *DbgkpProcessDebugPortMutex;

VOID      (WINAPI *KeFreezeAllThreads) (VOID);
VOID      (WINAPI *KeThawAllThreads) (VOID);
VOID      (FASTCALL  *KiReadyThread)(IN PKTHREAD Thread);
VOID      (WINAPI * KiSetSwapEvent)();
BOOLEAN   (WINAPI *KiSwapProcess) (IN PKPROCESS NewProcess,IN PKPROCESS OldProcess);
int       (FASTCALL *KiSwapThread) ();
NTSTATUS  (FASTCALL *MiMakeProtectionMask)(unsigned int a1);
NTSTATUS  (WINAPI*MiProtectVirtualMemory) (IN PEPROCESS Process,IN PVOID *BaseAddress,IN PULONG RegionSize,IN ULONG NewProtectWin32,IN PULONG LastProtect);
NTSTATUS  (WINAPI *PspCreateThread)(OUT PHANDLE ThreadHandle,IN ACCESS_MASK DesiredAccess,IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,IN HANDLE ProcessHandle,IN PEPROCESS ProcessPointer,OUT PCLIENT_ID ClientId OPTIONAL,IN PCONTEXT ThreadContext OPTIONAL,IN PINITIAL_TEB InitialTeb OPTIONAL,IN BOOLEAN CreateSuspended,IN PKSTART_ROUTINE StartRoutine OPTIONAL,IN PVOID StartContext);
void      (WINAPI *IopDeallocateApc)(PVOID P, int a2, int a3, int a4, int a5);

VOID (FASTCALL *HalRequestSoftwareInterrupt) (KIRQL RequestIrql);
VOID (FASTCALL *KiUnlockDispatcherDatabase)(KIRQL irql);

VOID  (WINAPI *KeContextFromKframes)(IN PKTRAP_FRAME TrapFrame,PKEXCEPTION_FRAME ExceptionFrame,PCONTEXT ContextFrame);
VOID (WINAPI *KeContextToKframes) ( PKTRAP_FRAME TrapFrame,PKEXCEPTION_FRAME ExceptionFrame,IN PCONTEXT ContextFrame,IN ULONG ContextFlags,IN KPROCESSOR_MODE PreviousMode);
BOOLEAN (WINAPI *KiCheckForAtlThunk) (IN PEXCEPTION_RECORD ExceptionRecord,IN PCONTEXT Context);
BOOLEAN (WINAPI *RtlDispatchException) (IN PEXCEPTION_RECORD ExceptionRecord,IN PCONTEXT ContextRecord);


BOOLEAN (WINAPI *KdIsThisAKdTrap) (IN PEXCEPTION_RECORD ExTrapFrameceptionRecord,IN PCONTEXT ContextRecord,IN KPROCESSOR_MODE PreviousMode);
VOID (WINAPI *KiSegSsToTrapFrame ) (IN PKTRAP_FRAME ,IN ULONG SegSs);
VOID (WINAPI *KiEspToTrapFrame)(IN PKTRAP_FRAME TrapFrame,IN ULONG Esp);
ULONG (WINAPI *KiCopyInformation) (IN OUT PEXCEPTION_RECORD ExceptionRecord1,IN PEXCEPTION_RECORD ExceptionRecord2);
BOOLEAN
(WINAPI *KiDebugRoutine) (
						  IN PKTRAP_FRAME TrapFrame,
						  IN PKEXCEPTION_FRAME ExceptionFrame,
						  IN PEXCEPTION_RECORD ExceptionRecord,
						  IN PCONTEXT ContextRecord,
						  IN KPROCESSOR_MODE PreviousMode,
						  IN BOOLEAN SecondChance
						  );

VOID (WINAPI *ObpDeleteObjectType )(IN  PVOID   Object);
                 
NTSTATUS  (WINAPI *MmGetFileNameForSection) (IN PVOID SectionObject,OUT POBJECT_NAME_INFORMATION *FileNameInfo);

BOOLEAN
DbgkpSuspendProcess (
    VOID
    )

/*++

Routine Description:

    This function causes all threads in the calling process except for
    the calling thread to suspend.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    // Freeze the execution of all threads in the current process, but the
    // calling thread. If we are in the process of being deleted don't do this.
    //
    if ((PsGetCurrentProcess()->u1.Flags&PS_PROCESS_FLAGS_PROCESS_DELETE) == 0) {
        KeFreezeAllThreads();
        return TRUE;
    }

    return FALSE;
}

VOID
DbgkpResumeProcess (
    VOID
    )

/*++

Routine Description:

    This function causes all threads in the calling process except for
    the calling thread to resume.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PAGED_CODE();

    //
    // Thaw the execution of all threads in the current process, but
    // the calling thread.
    //

    KeThawAllThreads();

    return;
}

HANDLE
DbgkpSectionToFileHandle(
    IN PVOID SectionObject
    )

/*++

Routine Description:

    This function opens a handle to the file associated with the processes
    section. The file is opened such that it can be duplicated all the way to
    the UI where the UI can either map the file or read the file to get
    the debug info.

Arguments:

    SectionObject - Supplies the section whose associated file is to be opened.

Return Value:

    NULL - The file could not be opened.

    NON-NULL - Returns a handle to the file associated with the specified
        section.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    POBJECT_NAME_INFORMATION FileNameInfo;

    PAGED_CODE();

    Status = MmGetFileNameForSection(SectionObject, &FileNameInfo);
    if ( !NT_SUCCESS(Status) ) {
        return NULL;
    }

    InitializeObjectAttributes(
        &Obja,
        &FileNameInfo->Name,
        OBJ_CASE_INSENSITIVE | OBJ_FORCE_ACCESS_CHECK | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    Status = ZwOpenFile(
                &Handle,
                (ACCESS_MASK)(GENERIC_READ | SYNCHRONIZE),
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT
                );
    ExFreePool(FileNameInfo);
    if ( !NT_SUCCESS(Status) ) {
        return NULL;
    } else {
        return Handle;
    }
}

//////////////////////////////////////////////////////////////////////////

VOID WINAPI
DbgkCreateThread(
    PVOID StartAddress
    )

/*++

Routine Description:

    This function is called when a new thread begins to execute. If the
    thread has an associated DebugPort, then a message is sent thru the
    port.

    If this thread is the first thread in the process, then this event
    is translated into a CreateProcessInfo message.

    If a message is sent, then while the thread is awaiting a reply,
    all other threads in the process are suspended.

Arguments:

    Thread - New thread just being started

    StartAddress - Supplies the start address for the thread that is
        starting.

Return Value:

    None.

--*/

{
    PVOID Port;
    DBGKM_APIMSG m;
    PDBGKM_CREATE_THREAD CreateThreadArgs;
    PDBGKM_CREATE_PROCESS CreateProcessArgs;
    PEPROCESS Process;
    PDBGKM_LOAD_DLL LoadDllArgs;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
	PIMAGE_NT_HEADERS NtHeaders;
	PTEB Teb;
	ULONG OldFlags;
	PETHREAD Thread;

    PAGED_CODE();
	Thread	=	PsGetCurrentThread();
    Process = PsGetCurrentProcessByThread (Thread);


    OldFlags = PS_TEST_SET_BITS (&Process->u1.Flags, PS_PROCESS_FLAGS_CREATE_REPORTED|PS_PROCESS_FLAGS_IMAGE_NOTIFY_DONE);

    if ((OldFlags&PS_PROCESS_FLAGS_IMAGE_NOTIFY_DONE) == 0 && PsImageNotifyEnabled) {
        IMAGE_INFO ImageInfo;
        UNICODE_STRING UnicodeFileName;
        POBJECT_NAME_INFORMATION FileNameInfo;

        //
        // notification of main .exe
        //
        ImageInfo.Properties = 0;
        ImageInfo.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
        ImageInfo.ImageBase = Process->SectionBaseAddress;
        ImageInfo.ImageSize = 0;

        try {
            NtHeaders = RtlImageNtHeader (Process->SectionBaseAddress);
    
            if (NtHeaders) {

                    ImageInfo.ImageSize = DBGKP_FIELD_FROM_IMAGE_OPTIONAL_HEADER (NtHeaders, SizeOfImage);

            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            ImageInfo.ImageSize = 0;
        }
        ImageInfo.ImageSelector = 0;
        ImageInfo.ImageSectionNumber = 0;

        Status = MmGetFileNameForSection (Process->SectionObject, &FileNameInfo);
        if (FileNameInfo != NULL) {
            PsCallImageNotifyRoutines (&FileNameInfo->Name,
                                       Process->UniqueProcessId,
                                       &ImageInfo);
            ExFreePool (FileNameInfo);
        } else {
            PsCallImageNotifyRoutines (NULL,
                                       Process->UniqueProcessId,
                                       &ImageInfo);
        }

        //
        // and of ntdll.dll
        //
        ImageInfo.Properties = 0;
        ImageInfo.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
        ImageInfo.ImageBase = PsSystemDllBase;
        ImageInfo.ImageSize = 0;

        try {
            NtHeaders = RtlImageNtHeader (PsSystemDllBase);
            if ( NtHeaders ) {

                    ImageInfo.ImageSize = DBGKP_FIELD_FROM_IMAGE_OPTIONAL_HEADER (NtHeaders, SizeOfImage);

            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            ImageInfo.ImageSize = 0;
        }

        ImageInfo.ImageSelector = 0;
        ImageInfo.ImageSectionNumber = 0;

        RtlInitUnicodeString (&UnicodeFileName,
                              L"\\SystemRoot\\System32\\ntdll.dll");
        PsCallImageNotifyRoutines (&UnicodeFileName,
                                   Process->UniqueProcessId,
                                   &ImageInfo);
    }

		Port	=	myReturnDebugPort(Process);
 //   Port = Process->DebugPort;

    if (Port == NULL) {
        return;
    }

    //
    // Make sure we only get one create process message
    //

	if ((OldFlags&PS_PROCESS_FLAGS_CREATE_REPORTED) == 0) {

        //
        // This is a create process
        //

        CreateThreadArgs = &m.u.CreateProcessInfo.InitialThread;
        CreateThreadArgs->SubSystemKey = 0;

        CreateProcessArgs = &m.u.CreateProcessInfo;
        CreateProcessArgs->SubSystemKey = 0;
        CreateProcessArgs->FileHandle = DbgkpSectionToFileHandle(
                                            Process->SectionObject
                                            );
        CreateProcessArgs->BaseOfImage = Process->SectionBaseAddress;
        CreateThreadArgs->StartAddress = NULL;
        CreateProcessArgs->DebugInfoFileOffset = 0;
        CreateProcessArgs->DebugInfoSize = 0;

        try {
                        
            NtHeaders = RtlImageNtHeader(Process->SectionBaseAddress);

            if ( NtHeaders ) {


                    CreateThreadArgs->StartAddress = (PVOID) (DBGKP_FIELD_FROM_IMAGE_OPTIONAL_HEADER (NtHeaders, ImageBase) +
                        DBGKP_FIELD_FROM_IMAGE_OPTIONAL_HEADER (NtHeaders, AddressOfEntryPoint));


                //
                // The following fields are safe for Wow64 as the offsets
                // are the same for a PE32+ as a PE32 header.
                //
                
                CreateProcessArgs->DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
                CreateProcessArgs->DebugInfoSize = NtHeaders->FileHeader.NumberOfSymbols;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            CreateThreadArgs->StartAddress = NULL;
            CreateProcessArgs->DebugInfoFileOffset = 0;
            CreateProcessArgs->DebugInfoSize = 0;
        }

        DBGKM_FORMAT_API_MSG(m,DbgKmCreateProcessApi,sizeof(*CreateProcessArgs));

        DbgkpSendApiMessage(&m,FALSE);

        if (CreateProcessArgs->FileHandle != NULL) {
            ObCloseHandle(CreateProcessArgs->FileHandle, KernelMode);
        }

        LoadDllArgs = &m.u.LoadDll;
        LoadDllArgs->BaseOfDll = PsSystemDllBase;
        LoadDllArgs->DebugInfoFileOffset = 0;
        LoadDllArgs->DebugInfoSize = 0;
        LoadDllArgs->NamePointer = NULL;

        Teb = NULL;
        try {
            NtHeaders = RtlImageNtHeader(PsSystemDllBase);
            if ( NtHeaders ) {
                LoadDllArgs->DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
                LoadDllArgs->DebugInfoSize = NtHeaders->FileHeader.NumberOfSymbols;
            }

            //
            // Normaly the ntdll loaded fills in this pointer for the debug API's. We fake it here
            // as ntdll isn't loaded yet and it can't load itself.
            //

            Teb = (PTEB)Thread->Tcb.Teb;
            if (Teb != NULL) {
                wcsncpy (Teb->StaticUnicodeBuffer,
                         L"ntdll.dll",
                         sizeof (Teb->StaticUnicodeBuffer) / sizeof (Teb->StaticUnicodeBuffer[0]));
                Teb->NtTib.ArbitraryUserPointer = Teb->StaticUnicodeBuffer;
                LoadDllArgs->NamePointer = &Teb->NtTib.ArbitraryUserPointer;
            }
            
        } except (EXCEPTION_EXECUTE_HANDLER) {
            LoadDllArgs->DebugInfoFileOffset = 0;
            LoadDllArgs->DebugInfoSize = 0;
            LoadDllArgs->NamePointer = NULL;
        }

        //
        // Send load dll section for NT dll !
        //
		UNICODE_STRING	myPsNtDllPathName;
		RtlInitUnicodeString(&myPsNtDllPathName,  L"\\SystemRoot\\System32\\ntdll.dll");
        InitializeObjectAttributes(
            &Obja,
			(PUNICODE_STRING)&myPsNtDllPathName,
        //    (PUNICODE_STRING)&PsNtDllPathName,
            OBJ_CASE_INSENSITIVE | OBJ_FORCE_ACCESS_CHECK | OBJ_KERNEL_HANDLE,
            NULL,
            NULL
            );

        Status = ZwOpenFile(
                    &LoadDllArgs->FileHandle,
                    (ACCESS_MASK)(GENERIC_READ | SYNCHRONIZE),
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_SYNCHRONOUS_IO_NONALERT
                    );

        if (!NT_SUCCESS (Status)) {
            LoadDllArgs->FileHandle = NULL;
        }

        DBGKM_FORMAT_API_MSG(m,DbgKmLoadDllApi,sizeof(*LoadDllArgs));
        DbgkpSendApiMessage(&m,TRUE);

        if (LoadDllArgs->FileHandle != NULL) {
            ObCloseHandle(LoadDllArgs->FileHandle, KernelMode);
        }

        try {
            if (Teb != NULL) {
                Teb->NtTib.ArbitraryUserPointer = NULL;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
        }

    } else {

        CreateThreadArgs = &m.u.CreateThread;
        CreateThreadArgs->SubSystemKey = 0;
        CreateThreadArgs->StartAddress = StartAddress;

        DBGKM_FORMAT_API_MSG (m,DbgKmCreateThreadApi,sizeof(*CreateThreadArgs));

        DbgkpSendApiMessage (&m,TRUE);
    }
}

VOID
DbgkExitThread(
    NTSTATUS ExitStatus
    )

/*++

Routine Description:

    This function is called when a new thread terminates. At this
    point, the thread will no longer execute in user-mode. No other
    exit processing has occured.

    If a message is sent, then while the thread is awaiting a reply,
    all other threads in the process are suspended.

Arguments:

    ExitStatus - Supplies the ExitStatus of the exiting thread.

Return Value:

    None.

--*/

{
    PVOID Port;
    DBGKM_APIMSG m;
    PDBGKM_EXIT_THREAD args;
    PEPROCESS Process;
    BOOLEAN Frozen;

    PAGED_CODE();

    Process = PsGetCurrentProcess();
//     if (PsGetCurrentThread()->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_HIDEFROMDBG) {
//         Port = NULL;
//     } else {
// 	  
//         Port = Process->DebugPort;
//     }
 Port = myReturnDebugPort(Process);
    if ( !Port ) {
        return;
    }

    if (PsGetCurrentThread()->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_DEADTHREAD) {
        return;
    }

    args = &m.u.ExitThread;
    args->ExitStatus = ExitStatus;

    DBGKM_FORMAT_API_MSG(m,DbgKmExitThreadApi,sizeof(*args));

    Frozen = DbgkpSuspendProcess();

    DbgkpSendApiMessage(&m,FALSE);

    if (Frozen) {
        DbgkpResumeProcess();
    }
}

VOID
DbgkExitProcess(
    NTSTATUS ExitStatus
    )

/*++

Routine Description:

    This function is called when a process terminates. The address
    space of the process is still intact, but no threads exist in
    the process.

Arguments:

    ExitStatus - Supplies the ExitStatus of the exiting process.

Return Value:

    None.

--*/

{
    PVOID Port;
    DBGKM_APIMSG m;
    PDBGKM_EXIT_PROCESS args;
    PEPROCESS Process;

    PAGED_CODE();

    Process = PsGetCurrentProcess();

//     if (PsGetCurrentThread()->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_HIDEFROMDBG) {
//         Port = NULL;
//     } else {
//         Port = myReturnDebugPort(Process);
//     }
	Port = myReturnDebugPort(Process);
    if ( !Port ) {
        return;
    }

    if (PsGetCurrentThread()->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_DEADTHREAD) {
        return;
    }

    //
    // this ensures that other timed lockers of the process will bail
    // since this call is done while holding the process lock, and lock duration
    // is controlled by debugger
    //
    KeQuerySystemTime(&Process->ExitTime);

    args = &m.u.ExitProcess;
    args->ExitStatus = ExitStatus;

    DBGKM_FORMAT_API_MSG(m,DbgKmExitProcessApi,sizeof(*args));

    DbgkpSendApiMessage(&m,FALSE);

}

VOID
DbgkMapViewOfSection(
    IN PVOID SectionObject,
    IN PVOID BaseAddress,
    IN ULONG SectionOffset,
    IN ULONG_PTR ViewSize
    )

/*++

Routine Description:

    This function is called when the current process successfully
    maps a view of an image section. If the process has an associated
    debug port, then a load dll message is sent.

Arguments:

    SectionObject - Supplies a pointer to the section mapped by the
        process.

    BaseAddress - Supplies the base address of where the section is
        mapped in the current process address space.

    SectionOffset - Supplies the offset in the section where the
        process' mapped view begins.

    ViewSize - Supplies the size of the mapped view.

Return Value:

    None.

--*/

{

    PVOID Port;
    DBGKM_APIMSG m;
    PDBGKM_LOAD_DLL LoadDllArgs;
    PEPROCESS Process;
    PIMAGE_NT_HEADERS NtHeaders;
	PTEB p_Teb;
	PETHREAD Thread;

    PAGED_CODE();

    UNREFERENCED_PARAMETER (SectionOffset);
    UNREFERENCED_PARAMETER (ViewSize);

    if ( KeGetPreviousMode() == KernelMode ) {
        return;
    }

    Process = PsGetCurrentProcess();
	Thread=PsGetCurrentThread();


//     if (PsGetCurrentThread()->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_HIDEFROMDBG) {
//         Port = NULL;
//     } else {
	       Port = myReturnDebugPort(Process);
//     }

    if ( !Port ) {
        return;
    }

    LoadDllArgs = &m.u.LoadDll;
    LoadDllArgs->FileHandle = DbgkpSectionToFileHandle(SectionObject);
    LoadDllArgs->BaseOfDll = BaseAddress;
    LoadDllArgs->DebugInfoFileOffset = 0;
    LoadDllArgs->DebugInfoSize = 0;

    //
    // The loader fills in the module name in this pointer before mapping
    // the section.  It's a very poor linkage.
    //
    p_Teb=(PTEB)Thread->Tcb.Teb;
    LoadDllArgs->NamePointer = p_Teb->NtTib.ArbitraryUserPointer;

    try {

        NtHeaders = RtlImageNtHeader (BaseAddress);

        if (NtHeaders != NULL) {
            LoadDllArgs->DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
            LoadDllArgs->DebugInfoSize = NtHeaders->FileHeader.NumberOfSymbols;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {

        LoadDllArgs->DebugInfoFileOffset = 0;
        LoadDllArgs->DebugInfoSize = 0;
        LoadDllArgs->NamePointer = NULL;

    }

    DBGKM_FORMAT_API_MSG(m,DbgKmLoadDllApi,sizeof(*LoadDllArgs));

    DbgkpSendApiMessage(&m,TRUE);
    if (LoadDllArgs->FileHandle != NULL) {
        ObCloseHandle(LoadDllArgs->FileHandle, KernelMode);
    }
}

VOID
DbgkUnMapViewOfSection(
    IN PVOID BaseAddress
    )

/*++

Routine Description:

    This function is called when the current process successfully
    un maps a view of an image section. If the process has an associated
    debug port, then an "unmap view of section" message is sent.

Arguments:

    BaseAddress - Supplies the base address of the section being unmapped.

Return Value:

    None.

--*/

{

    PVOID Port;
    DBGKM_APIMSG m;
    PDBGKM_UNLOAD_DLL UnloadDllArgs;

    PAGED_CODE();

    if ( KeGetPreviousMode() == KernelMode ) {
        return;
    }

//     if (PsGetCurrentThread()->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_HIDEFROMDBG) {
//         Port = NULL;
//     } else {
// 		Port = myReturnDebugPort(PsGetCurrentProcess());
//         Port = PsGetCurrentProcess()->DebugPort;
//     }
	Port = myReturnDebugPort(PsGetCurrentProcess());
    if ( !Port ) {
        return;
    }

    UnloadDllArgs = &m.u.UnloadDll;
    UnloadDllArgs->BaseAddress = BaseAddress;

    DBGKM_FORMAT_API_MSG(m,DbgKmUnloadDllApi,sizeof(*UnloadDllArgs));

    DbgkpSendApiMessage(&m,TRUE);
}

