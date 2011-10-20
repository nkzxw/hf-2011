/*++

	This is the part of NGdbg kernel debugger
	
	ngdbg.cpp

	This file contains DriverEntry and DriverUnload routines.
	Also in contais routines BootStartUp and BootCleanUp used
	 when debugger is set to be started while Windows is booting up.
	
--*/

#include <ntifs.h>
#include "winnt.h"
#include "win32k.h"
#include "splice.h"
#include "dbgeng.h"
#include "winddi.h"
#include "win32k.h"
#include <stdarg.h>
#include "symbols.h"
#include "gui.h"
#include "mouse.h"

_SURFOBJ *pPrimarySurf;
PVOID pDrvCopyBits;
KEVENT SynchEvent;


PVOID *GetMem()
{
	return ((PVOID*)&((IMAGE_DOS_HEADER*)W32BaseAddress)->e_res2);
}

PSHARED_DISP_DATA GetSharedData()
{
	PSHARED_DISP_DATA* pData = (PSHARED_DISP_DATA*)GetMem();

	if (!*pData)
	{
		KdPrint(("Shared data not allocated, creating\n"));

		*pData = (PSHARED_DISP_DATA) ExAllocatePool (NonPagedPool, sizeof(SHARED_DISP_DATA));

		if (!*pData)
		{
			KdPrint (("ExAllocatePool failed\n"));
			return NULL;
		}

		memset (*pData, 0, sizeof(SHARED_DISP_DATA));

		(*pData)->Signature = SHARED_SIGNATURE;
	}

	return *pData;
}


VOID _cdecl EngPrint (char *fmt, ...)
{
	va_list va;
	va_start (va, fmt);

	EngDebugPrint ("", fmt, va);
}


VOID
Worker(
	);

PVOID 
IoHookInterrupt (
	ULONG Vector, 
	PVOID NewRoutine
	);

UCHAR SplicingBuffer[50];
UCHAR BackupBuffer[5];
ULONG BackupWritten;

BOOLEAN BootDebuggingInitiated = FALSE;

extern PEPROCESS CsrProcess;
extern "C"
{
	extern POBJECT_TYPE *PsProcessType;
	extern POBJECT_TYPE *PsThreadType;
}



BOOLEAN
NewDrvCopyBits(
   OUT _SURFOBJ *psoDst,
   IN _SURFOBJ *psoSrc,
   IN VOID *pco,
   IN VOID *pxlo,
   IN VOID *prclDst,
   IN VOID *pptlSrc
   );

VOID 
REINITIALIZE_ADAPTER( 
	PVOID 
	);

NTSTATUS
KbdWinQueryLeds(
	);

VOID
Cleanup(
	);

VOID ResetTrampoline();
VOID CreateTrampoline();

KDPC HotkeyResetStateDpc;
VOID
  HotkeyResetStateDeferredRoutine(
    IN struct _KDPC  *Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    );



//
// Driver unload routine
//
void DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	W32PrepareCall ();

#if KBD_HOOK_ISR
	IoHookInterrupt (OldKbd, OldISR);
#else
	ResetTrampoline();
	//I8042HookKeyboard  ((PI8042_KEYBOARD_ISR) NULL);
#endif
	Cleanup();
	W32ReleaseCall ();

	//
	// Perform clean-up operations on multiprocessor system
	//

	DbgHalCleanupMP ();

	DbgCleanup();

	KdPrint(("[~] DriverUnload()\n"));
}


//
// new DrvCopyBits splice hook
//

BOOLEAN
NewDrvCopyBits(
   OUT _SURFOBJ *psoDst,
   IN _SURFOBJ *psoSrc,
   IN VOID *pco,
   IN VOID *pxlo,
   IN VOID *prclDst,
   IN VOID *pptlSrc
   )
{
	KdPrint(("NewDrvCopyBits (pdoDst=%X)\n", psoDst));

	if (pPrimarySurf == NULL &&
		psoDst->sizlBitmap.LowPart >= 640 &&
		psoDst->sizlBitmap.HighPart >= 480)
	{
		KdPrint(("Got primary surface %X\n", psoDst));
		pPrimarySurf = psoDst;
		KeSetEvent (&SynchEvent, 0, 0);
	}

	return ((BOOLEAN (*)(SURFOBJ*,SURFOBJ*,VOID*,VOID*,VOID*,VOID*))&SplicingBuffer) (psoDst, psoSrc, pco, pxlo, prclDst, pptlSrc);
}

VOID MmAllowPageFaultsAtRaisedIrql ();
VOID MmForbidPageFaultsAtRaisedIrql ();

ULONG MajorVersion;
ULONG MinorVersion;

//
// Driver entry point
//
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING )
{
	DriverObject->DriverUnload = DriverUnload;
	KdPrint(("[~] DriverEntry()\n"));

	PsGetVersion (&MajorVersion, &MinorVersion, 0, 0);

	if (MajorVersion >= 6)
	{
		KdPrint(("Windows Vista and later are not supported yet\n"));
		return STATUS_NOT_SUPPORTED;
	}
	if (MajorVersion < 5 || MinorVersion == 0)
	{
		KdPrint(("Windows NT and 2000 are not supported\n"));
		return STATUS_NOT_SUPPORTED;
	}

	ASSERT (MajorVersion == 5);
	ASSERT (MinorVersion >= 1 && MinorVersion <= 2);

	if (MinorVersion == 1)
	{
		KdPrint(("Running on Windows XP\n"));
	}
	else
	{
		KdPrint(("Running on Windows 2003 Server\n"));
	}

	if (KeNumberProcessors > 1)
	{
		KdPrint(("Loading on multiprocessor system (NumberProcessors %d)\n", KeNumberProcessors));
	}
	else
	{
		KdPrint(("Loading on uniprocessor system\n"));
	}

	KdPrint (("First hello from nt\n"));

	if(!NT_SUCCESS(W32FindAndSwapIAT ()))
	{
		KdPrint(("could not swap import\n"));
		return STATUS_INVALID_FILE_FOR_SECTION;
	}

	// import something from W32k
	EngPrint ("Second hello from win32k\n");

	HANDLE hCsrProcess;
	NTSTATUS Status;

	Status = ObOpenObjectByPointer (
		CsrProcess,
		OBJ_KERNEL_HANDLE,
		NULL, 
		PROCESS_ALL_ACCESS,
		*PsProcessType, 
		KernelMode, 
		&hCsrProcess
		); 

	if (!NT_SUCCESS(Status))
	{
		KdPrint(("ObOpenObjectByPointer failed with status %X\n", Status));
		W32ReleaseCall();
		return Status;
	}

	KdPrint(("csr opened, handle %X\n", hCsrProcess));

	//
	// EngLoadImage uses KeAttachProcess/KeDetachProcess to attach to csrss process
	// KeDetachProcess detaches to thread's original process, but our thread's
	// original process is System! (because we are running in the context of system
	// worker thread that loads a driver). 
	//    (
	//    |  fucken windows programmers could not call KeStackAttachProcess 
	//    |   instead of KeAttachProcess :(
	//    )
	// So we have to run our function in the context of csrss.exe
	//

	HANDLE ThreadHandle;
	CLIENT_ID ClientId;
	OBJECT_ATTRIBUTES Oa;
	InitializeObjectAttributes (&Oa, NULL, OBJ_KERNEL_HANDLE, 0, 0);

	Status = PsCreateSystemThread (
		&ThreadHandle,
		THREAD_ALL_ACCESS,
		&Oa,
		hCsrProcess,
		&ClientId,
		REINITIALIZE_ADAPTER,
		NULL
		);

	if (!NT_SUCCESS(Status))
	{
		KdPrint(("PsCreateSystemThread failed with status %X\n", Status));
		ZwClose (hCsrProcess);
		W32ReleaseCall();
		return Status;
	}

	KdPrint(("thread created, handle %X\n", ThreadHandle));

	PETHREAD Thread;

	Status = ObReferenceObjectByHandle(
		ThreadHandle,
		THREAD_ALL_ACCESS,
		*PsThreadType,
		KernelMode,
		(PVOID*) &Thread,
		NULL
		);

	if (!NT_SUCCESS(Status))
	{
		KdPrint(("ObReferenceObjectByHandle failed with status %X\n", Status));
		// cannot unload because thread is running
		KeBugCheck (0);
	}

	KdPrint(("thread referenced to %X\n", Thread));

	KeWaitForSingleObject (Thread, Executive, KernelMode, FALSE, NULL);

	KdPrint(("Thread terminated\n"));

	ZwClose (hCsrProcess);
	ObDereferenceObject (Thread);
	ZwClose (ThreadHandle);

	KdPrint(("success\n", hCsrProcess));

	if (!pDrvCopyBits)
	{
		KdPrint(("Could not find DrvCopyBits\n"));
		W32ReleaseCall();
		return STATUS_UNSUCCESSFUL;
	}

	//
	// Query keyboard LEDs
	//

	if(!NT_SUCCESS(KbdWinQueryLeds()))
	{
		W32ReleaseCall();
		return STATUS_UNSUCCESSFUL;
	}

	PSHARED_DISP_DATA disp = GetSharedData();
	if (!disp)
	{
		EngPrint ("ngvid: could not get shared data\n");
		W32ReleaseCall();
		return STATUS_UNSUCCESSFUL;
	}
	if (disp->Signature != SHARED_SIGNATURE)
	{
		EngPrint ("ngvid: Damaged shared block %X signature %X should be %X\n",
			disp, disp->Signature, SHARED_SIGNATURE);
		//__asm int 3

		W32ReleaseCall();
		return STATUS_UNSUCCESSFUL;
	}

	KdPrint (("Got shared %X Sign %X Surf %X\n", disp, disp->Signature, disp->pPrimarySurf));

#if 0
	//
	// Temporarily hook DrvCopyBits
	//

	pDrvCopyBits = disp->pDrvCopyBits;

#endif

	if (!disp->pPrimarySurf)
	{
		KdPrint(("DrvCopyBits %X\n", pDrvCopyBits));

		KeInitializeEvent (&SynchEvent, SynchronizationEvent, FALSE);

		if (SpliceFunctionStart (pDrvCopyBits, NewDrvCopyBits, SplicingBuffer, sizeof(SplicingBuffer), BackupBuffer, &BackupWritten, FALSE))
		{
			KdPrint(("SpliceFunctionStart FAILED!!!\n"));
			W32ReleaseCall();
			return STATUS_UNSUCCESSFUL;
		}

		KdPrint(("Now you have to move mouse pointer across the display ...\n"));

		KeWaitForSingleObject (&SynchEvent, Executive, KernelMode, FALSE, NULL);

		UnspliceFunctionStart (pDrvCopyBits, BackupBuffer, FALSE);

		KdPrint(("Wait succeeded, so got primary surf %X\n", pPrimarySurf));
		disp->pPrimarySurf = pPrimarySurf;
	}
	else
	{
		KdPrint(("Already have primary surface\n"));
		pPrimarySurf = disp->pPrimarySurf;
	}

	// Hook kbd & mouse

#if KBD_HOOK_ISR
	OldKbd = GetIOAPICIntVector (1);
	*(PVOID*)&OldISR = IoHookInterrupt ( (UCHAR)OldKbd, InterruptService);
#else
	CreateTrampoline();
	//I8042HookKeyboard  ((PI8042_KEYBOARD_ISR) IsrHookRoutine);
#endif

	MouseInitialize (StateChangeCallbackRoutine);

	KdPrint(("Keyboard & mouse hooked\n"));

	// Initialize reset DPC
	KeInitializeDpc (&HotkeyResetStateDpc, HotkeyResetStateDeferredRoutine, NULL);

	//
	// Perform multiprocessor initialization
	//

	DbgHalInitializeMP ();

	///

	Worker();

	///

	W32ReleaseCall();

	DbgInitialize ();

	KdPrint(("[+] Driver initialization successful\n"));
	return STATUS_SUCCESS;
}

VOID
BootGuiInitialize(
	);

extern "C"
BOOLEAN
FindBaseAndSize(
	IN PVOID SomePtr,
	OUT PVOID *BaseAddress OPTIONAL, 
	OUT ULONG *ImageSize OPTIONAL
	);

PVOID GetKernelAddress (PWSTR);

extern "C" extern PVOID pNtBase;
VOID I8042HookKeyboardIsr(VOID (NTAPI *)(UCHAR));
VOID ProcessScanCode(UCHAR);

NTSTATUS
BootStartUp(
	)

/*++

Routine Description

	This routine is called during Windows boot up from ngboot.sys
	This routine performs initialization like DriverEntry, but 
	 without driver-specific initialization (like modifying DriverObject, etc)
	 becase ngbood.sys simply imports from ngdbg.sys and Windows does not
	 create driver object for driver being imported from another drivers.
	 Also Windows Kernel does not call entry point of driver being imported.
	So ngboot.sys have to manually call BootStartUp and BootStartUp should
	 perform initialization.

Arguments

	None

Return Value

	NTSTATUS of initialization

Environment

	System startup

--*/

{
	NTSTATUS Status;

	//
	// Indicate that debugging is initiated at boot time.
	//

	BootDebuggingInitiated = TRUE;


	//
	// At system startup we cannot use several features:
	//
	// 1) i8042prt routines to hook keyboard (i8042prt.sys possibly is not loaded yet).
	//    So we have to hook keyboard IRQ manually.
	//
	// 2) win32k services to output to screen. Win32 subsystem may be not loaded yet.
	//    So we have to output to screen using bootvid.dll
	//

	//
	// Initialize boot gui
	//

	BootGuiInitialize ();

	//
	// Perform multiprocessor initialization
	//

	DbgHalInitializeMP ();

	//
	// Load symbols (usually Worker() loads symbols, which is not called at boot time)
	//

	if(!FindBaseAndSize (GetKernelAddress(L"DbgPrint"), &pNtBase, NULL))
	{
		KdPrint(("Could not get nt base\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// Initialize symbol engine and load symbols
	SymInitialize ( FALSE );	// don't use EngMapFile

	Status = SymLoadSymbolFile (L"nt", pNtBase);
	KdPrint(("nt symbols loaded with status %X\n", Status));

	Status = SymLoadSymbolFile (L"hal.dll", NULL);
	KdPrint(("hal.dll symbols loaded with status %X\n", Status));


	//
	// Set keyboard hook to 8042.sys instead of i8042prt.sys system driver
	//

	I8042HookKeyboardIsr(&ProcessScanCode);

	//
	// Initialize mouse hook
	//

	MouseInitialize (StateChangeCallbackRoutine);

	//
	// Initialize debug engine
	//

	DbgInitialize ();


	//
	// Write some chars
	//

	for (ULONG i=0; i<90; i++)
	{
		GuiPrintf ("Line %d\n", i);
	}

	return STATUS_SUCCESS;
}


VOID
BootCleanUp(
	)
{
	KdPrint (( __FUNCTION__ " : Not implemented\n"));

	DbgHalCleanupMP ();

	ASSERT (FALSE);
}

