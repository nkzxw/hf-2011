// ntos.cpp: implementation of the ntos class.
//
//////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


#include "commhdr.h"


// SINGLE_LIST_ENTRY KiProcessInSwapListHead;
// SINGLE_LIST_ENTRY KiProcessOutSwapListHead;
// const UNICODE_STRING PsNtDllPathName;
// PVOID PsSystemDllBase;

// PKTHREAD KiSwappingThread;
// KEVENT KiSwapEvent;
// LUID    SeDebugPrivilege;
// PLIST_ENTRY PsLoadedModuleList; 
// POBJECT_TYPE HxDbgkDebugObjectType = NULL;
// PUCHAR KeI386XMMIPresent;
// 
//这些要找外部符号

PVOID KeUserExceptionDispatcher=NULL;
PUCHAR KeI386XMMIPresent;
BOOLEAN PsImageNotifyEnabled;
PULONG KeFeatureBits;
const UNICODE_STRING PsNtDllPathName;
PVOID PsSystemDllBase;
extern "C"
VOID
KiDispatchException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN FirstChance
    )

/*++

Routine Description:

    This function is called to dispatch an exception to the proper mode and
    to cause the exception dispatcher to be called. If the previous mode is
    kernel, then the exception dispatcher is called directly to process the
    exception. Otherwise the exception record, exception frame, and trap
    frame contents are copied to the user mode stack. The contents of the
    exception frame and trap are then modified such that when control is
    returned, execution will commense in user mode in a routine which will
    call the exception dispatcher.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame. For NT386,
        this should be NULL.

    TrapFrame - Supplies a pointer to a trap frame.

    PreviousMode - Supplies the previous processor mode.

    FirstChance - Supplies a boolean value that specifies whether this is
        the first (TRUE) or second (FALSE) chance for the exception.

Return Value:

    None.

--*/

{
    CONTEXT ContextFrame;
    EXCEPTION_RECORD ExceptionRecord1, ExceptionRecord2;
    LONG Length;
    ULONG UserStack1;
    ULONG UserStack2;
    //
    // Move machine state from trap and exception frames to a context frame,
    // and increment the number of exceptions dispatched.
    //
// 
// 	if (1)
// 	{
// 			_asm
// 			{
// // 				push FirstChance
// // 				push PreviousMode
// // 				push TrapFrame
// // 				push	ExceptionFrame
// // 				push ExceptionRecord
// 				mov esp,ebp	//恢复原来的ESP
// 				add esp,4
// 				push    390h
// 				mov eax,0x804fd5c7
// 				JMP	eax
// 
// 			}
// 			
// 	}
    KeGetCurrentPrcb()->KeExceptionDispatchCount += 1;
    ContextFrame.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;

    if ((PreviousMode == UserMode) || KdDebuggerEnabled) {
        //
        // For usermode exceptions always try to dispatch the floating
        // point state.  This allows exception handlers & debuggers to
        // examine/edit the npx context if required.  Plus it allows
        // exception handlers to use fp instructions without destroying
        // the npx state at the time of the exception.
        //
        // Note: If there's no 80387, ContextTo/FromKFrames will use the
        // emulator's current state.  If the emulator can not give the
        // current state, then the context_floating_point bit will be
        // turned off by ContextFromKFrames.
        //

        ContextFrame.ContextFlags |= CONTEXT_FLOATING_POINT;
        if (KeI386XMMIPresent) {
            ContextFrame.ContextFlags |= CONTEXT_EXTENDED_REGISTERS;
        }
    }

    KeContextFromKframes(TrapFrame, ExceptionFrame, &ContextFrame);

    //
    // if it is BREAK_POINT exception, we subtract 1 from EIP and report
    // the updated EIP to user.  This is because Cruiser requires EIP
    // points to the int 3 instruction (not the instruction following int 3).
    // In this case, BreakPoint exception is fatal. Otherwise we will step
    // on the int 3 over and over again, if user does not handle it
    //
    // if the BREAK_POINT occured in V86 mode, the debugger running in the
    // VDM will expect CS:EIP to point after the exception (the way the
    // processor left it.  this is also true for protected mode dos
    // app debuggers.  We will need a way to detect this.
    //
    //

    switch (ExceptionRecord->ExceptionCode) {
        case STATUS_BREAKPOINT:
            ContextFrame.Eip--;
            break;

        case KI_EXCEPTION_ACCESS_VIOLATION:
            ExceptionRecord->ExceptionCode = STATUS_ACCESS_VIOLATION;
            if (PreviousMode == UserMode) {
                if (KiCheckForAtlThunk(ExceptionRecord,&ContextFrame) != FALSE) {
                    goto Handled1;
                }

                if ((SharedUserData->ProcessorFeatures[PF_NX_ENABLED] == TRUE) &&
                    (ExceptionRecord->ExceptionInformation [0] ==8/* EXCEPTION_EXECUTE_FAULT*/)) {
					//全局变量
                    ULONG	tmp	=	*KeFeatureBits;
                    if (((tmp & KF_GLOBAL_32BIT_EXECUTE) != 0) ||
                        (PsGetCurrentProcess()->Pcb.Flags.ExecuteEnable != 0) ||
                        (((tmp & KF_GLOBAL_32BIT_NOEXECUTE) == 0) &&
                         (PsGetCurrentProcess()->Pcb.Flags.ExecuteDisable == 0))) {
                        ExceptionRecord->ExceptionInformation [0] = 0;
                    }
                }
            }
            break;
    }

    //
    // Select the method of handling the exception based on the previous mode.
    //

    ASSERT ((
             !((PreviousMode == KernelMode) &&
             (ContextFrame.EFlags & EFLAGS_V86_MASK))
           ));

    if (PreviousMode == KernelMode) {

        //
        // Previous mode was kernel.
        //
        // If the kernel debugger is active, then give the kernel debugger the
        // first chance to handle the exception. If the kernel debugger handles
        // the exception, then continue execution. Else attempt to dispatch the
        // exception to a frame based handler. If a frame based handler handles
        // the exception, then continue execution.
        //
        // If a frame based handler does not handle the exception,
        // give the kernel debugger a second chance, if it's present.
        //
        // If the exception is still unhandled, call KeBugCheck().
        //

        if (FirstChance == TRUE) {
		
            if ((KiDebugRoutine != NULL) &&
               (((KiDebugRoutine) (TrapFrame,
                                   ExceptionFrame,
                                   ExceptionRecord,
                                   &ContextFrame,
                                   PreviousMode,
                                   FALSE)) != FALSE)) {

                goto Handled1;
            }

            // Kernel debugger didn't handle exception.

            if (RtlDispatchException(ExceptionRecord, &ContextFrame) == TRUE) {
                goto Handled1;
            }
        }

        //
        // This is the second chance to handle the exception.
        //

        if ((KiDebugRoutine != NULL) &&
            (((KiDebugRoutine) (TrapFrame,
                                ExceptionFrame,
                                ExceptionRecord,
                                &ContextFrame,
                                PreviousMode,
                                TRUE)) != FALSE)) {

            goto Handled1;
        }

        KeBugCheckEx(
            KERNEL_MODE_EXCEPTION_NOT_HANDLED,
            ExceptionRecord->ExceptionCode,
            (ULONG)ExceptionRecord->ExceptionAddress,
            (ULONG)TrapFrame,
            0);

    } else {

        //
        // Previous mode was user.
        //
        // If this is the first chance and the current process has a debugger
        // port, then send a message to the debugger port and wait for a reply.
        // If the debugger handles the exception, then continue execution. Else
        // transfer the exception information to the user stack, transition to
        // user mode, and attempt to dispatch the exception to a frame based
        // handler. If a frame based handler handles the exception, then continue
        // execution with the continue system service. Else execute the
        // NtRaiseException system service with FirstChance == FALSE, which
        // will call this routine a second time to process the exception.
        //
        // If this is the second chance and the current process has a debugger
        // port, then send a message to the debugger port and wait for a reply.
        // If the debugger handles the exception, then continue execution. Else
        // if the current process has a subsystem port, then send a message to
        // the subsystem port and wait for a reply. If the subsystem handles the
        // exception, then continue execution. Else terminate the process.
        //


        if (FirstChance == TRUE) {

            //
            // This is the first chance to handle the exception.
            //

            if ((KiDebugRoutine != NULL)  &&
                ((PsGetCurrentProcess()->DebugPort == NULL &&
                  /*!KdIgnoreUmExceptions*/1) ||
                 (KdIsThisAKdTrap(ExceptionRecord, &ContextFrame, UserMode)))) {
                //
                // Now dispatch the fault to the kernel debugger.
                //

                if ((((KiDebugRoutine) (TrapFrame,
                                        ExceptionFrame,
                                        ExceptionRecord,
                                        &ContextFrame,
                                        PreviousMode,
                                        FALSE)) != FALSE)) {

                    goto Handled1;
                }
            }

            if (DbgkForwardException(ExceptionRecord, TRUE, FALSE)) {
                goto Handled2;
            }

            //
            // Transfer exception information to the user stack, transition
            // to user mode, and attempt to dispatch the exception to a frame
            // based handler.

            ExceptionRecord1.ExceptionCode = 0; // satisfy no_opt compilation

        repeat:
            try {

                //
                // If the SS segment is not 32 bit flat, there is no point
                // to dispatch exception to frame based exception handler.
                //

                if (TrapFrame->HardwareSegSs != (KGDT_R3_DATA | RPL_MASK) ||
                    TrapFrame->EFlags & EFLAGS_V86_MASK ) {
                    ExceptionRecord2.ExceptionCode = STATUS_ACCESS_VIOLATION;
                    ExceptionRecord2.ExceptionFlags = 0;
                    ExceptionRecord2.NumberParameters = 0;
                    ExRaiseException(&ExceptionRecord2);
                }

                //
                // Compute length of context record and new aligned user stack
                // pointer.
                //

                UserStack1 = (ContextFrame.Esp & ~CONTEXT_ROUND) - CONTEXT_ALIGNED_SIZE;

                //
                // Probe user stack area for writability and then transfer the
                // context record to the user stack.
                //

                ProbeForWrite((PCHAR)UserStack1, CONTEXT_ALIGNED_SIZE, CONTEXT_ALIGN);
                RtlCopyMemory((PULONG)UserStack1, &ContextFrame, sizeof(CONTEXT));

                //
                // Compute length of exception record and new aligned stack
                // address.
                //

                Length = (sizeof(EXCEPTION_RECORD) - (EXCEPTION_MAXIMUM_PARAMETERS -
                         ExceptionRecord->NumberParameters) * sizeof(ULONG) +3) &
                         (~3);
                UserStack2 = UserStack1 - Length;

                //
                // Probe user stack area for writeability and then transfer the
                // context record to the user stack area.
                // N.B. The probing length is Length+8 because there are two
                //      arguments need to be pushed to user stack later.
                //

                ProbeForWrite((PCHAR)(UserStack2 - 8), Length + 8, sizeof(ULONG));
                RtlCopyMemory((PULONG)UserStack2, ExceptionRecord, Length);

                //
                // Push address of exception record, context record to the
                // user stack.  They are the two parameters required by
                // _KiUserExceptionDispatch.
                //

                *(PULONG)(UserStack2 - sizeof(ULONG)) = UserStack1;
                *(PULONG)(UserStack2 - 2*sizeof(ULONG)) = UserStack2;

                //
                // Set new stack pointer to the trap frame.
                //

                KiSegSsToTrapFrame(TrapFrame, KGDT_R3_DATA);
                KiEspToTrapFrame(TrapFrame, (UserStack2 - sizeof(ULONG)*2));

                //
                // Force correct R3 selectors into TrapFrame.
                //

                TrapFrame->SegCs = SANITIZE_SEG(KGDT_R3_CODE, PreviousMode);
                TrapFrame->SegDs = SANITIZE_SEG(KGDT_R3_DATA, PreviousMode);
                TrapFrame->SegEs = SANITIZE_SEG(KGDT_R3_DATA, PreviousMode);
                TrapFrame->SegFs = SANITIZE_SEG(KGDT_R3_TEB, PreviousMode);
                TrapFrame->SegGs = 0;

                //
                // Set the address of the exception routine that will call the
                // exception dispatcher and then return to the trap handler.
                // The trap handler will restore the exception and trap frame
                // context and continue execution in the routine that will
                // call the exception dispatcher.
                //

                TrapFrame->Eip = (ULONG)KeUserExceptionDispatcher;
                return;

            } except (KiCopyInformation(&ExceptionRecord1,
                        (GetExceptionInformation())->ExceptionRecord)) {

                //
                // If the exception is a stack overflow, then attempt
                // to raise the stack overflow exception. Otherwise,
                // the user's stack is not accessible, or is misaligned,
                // and second chance processing is performed.
                //

                if (ExceptionRecord1.ExceptionCode == STATUS_STACK_OVERFLOW) {
                    ExceptionRecord1.ExceptionAddress = ExceptionRecord->ExceptionAddress;
                    RtlCopyMemory((PVOID)ExceptionRecord,
                                  &ExceptionRecord1, sizeof(EXCEPTION_RECORD));
                    goto repeat;
                }
            }
        }

        //
        // This is the second chance to handle the exception.
        //

        if (DbgkForwardException(ExceptionRecord, TRUE, TRUE)) {
            goto Handled2;
        } else if (DbgkForwardException(ExceptionRecord, FALSE, TRUE)) {
            goto Handled2;
        } else {
            ZwTerminateProcess(NtCurrentProcess(), ExceptionRecord->ExceptionCode);
            KeBugCheckEx(
                KERNEL_MODE_EXCEPTION_NOT_HANDLED,
                ExceptionRecord->ExceptionCode,
                (ULONG)ExceptionRecord->ExceptionAddress,
                (ULONG)TrapFrame,
                0);
        }
    }

    //
    // Move machine state from context frame to trap and exception frames and
    // then return to continue execution with the restored state.
    //

Handled1:

    KeContextToKframes(TrapFrame, ExceptionFrame, &ContextFrame,
                       ContextFrame.ContextFlags, PreviousMode);

    //
    // Exception was handled by the debugger or the associated subsystem
    // and state was modified, if necessary, using the get state and set
    // state capabilities. Therefore the context frame does not need to
    // be transferred to the trap and exception frames.
    //

Handled2:
    return;
}
////////////////////////////////////////////////////////////////////////////////
VOID
ProbeForWriteSmallStructure (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG Alignment
    )

/*++

Routine Description:

    Probes a structure for write access whose size is known at compile time.

Arguments:

    Address - Supples a pointer to the structure.

    Size - Supplies the size of the structure.

    Alignment - Supplies the alignment of structure.

Return Value:

    None

--*/

{

    ASSERT((Alignment == 1) || (Alignment == 2) ||
           (Alignment == 4) || (Alignment == 8) ||
           (Alignment == 16));

    //
    // If the size of the structure is > 4k then call the standard routine.
    // wow64 uses a page size of 4k even on ia64.
    //

    if ((Size == 0) || (Size >= 0x1000)) {

        ASSERT(0);

        ProbeForWrite(Address, Size, Alignment);

    } else {
        if (((ULONG_PTR)(Address) & (Alignment - 1)) != 0) {
            ExRaiseDatatypeMisalignment();
        }

#if defined(_AMD64_)

        if ((ULONG_PTR)(Address) >= (ULONG_PTR)MM_USER_PROBE_ADDRESS) {
             Address = (UCHAR * const)MM_USER_PROBE_ADDRESS;
        }
    
        ((volatile UCHAR *)(Address))[0] = ((volatile UCHAR *)(Address))[0];
        ((volatile UCHAR *)(Address))[Size - 1] = ((volatile UCHAR *)(Address))[Size - 1];

#else

        if ((ULONG_PTR)(Address) >= (ULONG_PTR)MM_USER_PROBE_ADDRESS) {
             *((volatile UCHAR * const)MM_USER_PROBE_ADDRESS) = 0;
        }
    
        *(volatile UCHAR *)(Address) = *(volatile UCHAR *)(Address);
        if (Size > Alignment) {
            ((volatile UCHAR *)(Address))[(Size - 1) & ~(SIZE_T)(Alignment - 1)] =
                ((volatile UCHAR *)(Address))[(Size - 1) & ~(SIZE_T)(Alignment - 1)];
        }

#endif

    }
}

