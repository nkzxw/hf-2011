/*++

	This is the part of NGdbg kernel debugger

	mmsup.cpp

	Memory-Manager support routines for the debugger

--*/


#include <ntifs.h>

typedef struct _MMPTE_SOFTWARE {
    ULONG Valid : 1;
    ULONG PageFileLow : 4;
    ULONG Protection : 5;
    ULONG Prototype : 1;
    ULONG Transition : 1;
    ULONG PageFileHigh : 20;
} MMPTE_SOFTWARE;

typedef struct _MMPTE_TRANSITION {
    ULONG Valid : 1;
    ULONG Write : 1;
    ULONG Owner : 1;
    ULONG WriteThrough : 1;
    ULONG CacheDisable : 1;
    ULONG Protection : 5;
    ULONG Prototype : 1;
    ULONG Transition : 1;
    ULONG PageFrameNumber : 20;
} MMPTE_TRANSITION;

typedef struct _MMPTE_PROTOTYPE {
    ULONG Valid : 1;
    ULONG ProtoAddressLow : 7;
    ULONG ReadOnly : 1;  // if set allow read only access.
    ULONG WhichPool : 1;
    ULONG Prototype : 1;
    ULONG ProtoAddressHigh : 21;
} MMPTE_PROTOTYPE;

typedef struct _MMPTE_HARDWARE {
    ULONG Valid : 1;
    ULONG Write : 1;       // UP version
    ULONG Owner : 1;
    ULONG WriteThrough : 1;
    ULONG CacheDisable : 1;
    ULONG Accessed : 1;
    ULONG Dirty : 1;
    ULONG LargePage : 1;
    ULONG Global : 1;
    ULONG CopyOnWrite : 1; // software field
    ULONG Prototype : 1;   // software field
    ULONG reserved : 1;    // software field
    ULONG PageFrameNumber : 20;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

typedef struct _MMPTE {
    union  {
        ULONG Long;
        MMPTE_HARDWARE Hard;
        MMPTE_PROTOTYPE Proto;
        MMPTE_SOFTWARE Soft;
        MMPTE_TRANSITION Trans;
        } u;
} MMPTE, *PMMPTE;

typedef struct _MMPTE_SOFTWARE_PAE {
    ULONGLONG Valid : 1;
    ULONGLONG PageFileLow : 4;
    ULONGLONG Protection : 5;
    ULONGLONG Prototype : 1;
    ULONGLONG Transition : 1;
    ULONGLONG Unused : 20;
    ULONGLONG PageFileHigh : 32;
} MMPTE_SOFTWARE_PAE;

typedef struct _MMPTE_TRANSITION_PAE {
    ULONGLONG Valid : 1;
    ULONGLONG Write : 1;
    ULONGLONG Owner : 1;
    ULONGLONG WriteThrough : 1;
    ULONGLONG CacheDisable : 1;
    ULONGLONG Protection : 5;
    ULONGLONG Prototype : 1;
    ULONGLONG Transition : 1;
    ULONGLONG PageFrameNumber : 24;
    ULONGLONG Unused : 28;
} MMPTE_TRANSITION_PAE;

typedef struct _MMPTE_PROTOTYPE_PAE {
    ULONGLONG Valid : 1;
    ULONGLONG Unused0: 7;
    ULONGLONG ReadOnly : 1;  // if set allow read only access.  LWFIX: remove
    ULONGLONG Unused1: 1;
    ULONGLONG Prototype : 1;
    ULONGLONG Protection : 5;
    ULONGLONG Unused: 16;
    ULONGLONG ProtoAddress: 32;
} MMPTE_PROTOTYPE_PAE;

typedef struct _MMPTE_HARDWARE_PAE {
    ULONGLONG Valid : 1;
    ULONGLONG Write : 1;        // UP version
    ULONGLONG Owner : 1;
    ULONGLONG WriteThrough : 1;
    ULONGLONG CacheDisable : 1;
    ULONGLONG Accessed : 1;
    ULONGLONG Dirty : 1;
    ULONGLONG LargePage : 1;
    ULONGLONG Global : 1;
    ULONGLONG CopyOnWrite : 1; // software field
    ULONGLONG Prototype : 1;   // software field
    ULONGLONG reserved0 : 1;  // software field
    ULONGLONG PageFrameNumber : 24;
    ULONGLONG reserved1 : 28;  // software field
} MMPTE_HARDWARE_PAE, *PMMPTE_HARDWARE_PAE;

typedef struct _MMPTE_PAE {
    union  {
        LARGE_INTEGER Long;
        MMPTE_HARDWARE_PAE Hard;
        MMPTE_PROTOTYPE_PAE Proto;
        MMPTE_SOFTWARE_PAE Soft;
        MMPTE_TRANSITION_PAE Trans;
        } u;
} MMPTE_PAE;

typedef MMPTE_PAE *PMMPTE_PAE;

#define PTE_BASE    0xC0000000
#define PDE_BASE    0xC0300000
#define PDE_BASE_PAE 0xc0600000

#define MiGetPdeAddress(va)  ((MMPTE*)(((((ULONG)(va)) >> 22) << 2) + PDE_BASE))
#define MiGetPteAddress(va) ((MMPTE*)(((((ULONG)(va)) >> 12) << 2) + PTE_BASE))

#define MiGetPdeAddressPae(va)   ((PMMPTE_PAE)(PDE_BASE_PAE + ((((ULONG)(va)) >> 21) << 3)))
#define MiGetPteAddressPae(va)   ((PMMPTE_PAE)(PTE_BASE + ((((ULONG)(va)) >> 12) << 3)))

#define MM_ZERO_PTE 0
#define MM_ZERO_KERNEL_PTE 0


#define MM_ZERO_ACCESS         0  // this value is not used.
#define MM_READONLY            1
#define MM_EXECUTE             2
#define MM_EXECUTE_READ        3
#define MM_READWRITE           4  // bit 2 is set if this is writable.
#define MM_WRITECOPY           5
#define MM_EXECUTE_READWRITE   6
#define MM_EXECUTE_WRITECOPY   7
#define MM_NOCACHE             8




#define PAE_ON (1<<5)

__inline
ULONG
 CR4(
	)
{
	// mov eax, cr4
	__asm _emit 0x0F __asm _emit 0x20 __asm _emit 0xE0
}


BOOLEAN
MmIsSystemAddressAccessable(
	PVOID VirtualAddress
	)

/*++

Routine Description

	For a given virtual address this function returns TRUE if the address
	is accessable without an access violation (it may incur a page fault).

Arguments

	VirtualAddress
	
		Address to be checked

Return value

	TRUE if address may be safely accessed, FALSE otherwise

--*/

{
	if (CR4() & PAE_ON)
	{
		PMMPTE_PAE PointerPte;

		PointerPte = MiGetPdeAddressPae (VirtualAddress);
		if ((PointerPte->u.Long.QuadPart == MM_ZERO_PTE) ||
			(PointerPte->u.Long.QuadPart == MM_ZERO_KERNEL_PTE) ||
			(PointerPte->u.Soft.Protection == MM_ZERO_ACCESS)) 
		{
			return FALSE;
		}
		
		PointerPte = MiGetPteAddressPae (VirtualAddress);
		if ((PointerPte->u.Long.QuadPart == MM_ZERO_PTE) ||
			(PointerPte->u.Long.QuadPart == MM_ZERO_KERNEL_PTE) ||
			(PointerPte->u.Soft.Protection == MM_ZERO_ACCESS)) 
		{
			return FALSE;
		}
	}
	else
	{
		PMMPTE PointerPte;

		PointerPte = MiGetPdeAddress (VirtualAddress);
		if ((PointerPte->u.Long == MM_ZERO_PTE) ||
			(PointerPte->u.Long == MM_ZERO_KERNEL_PTE) ||
			(PointerPte->u.Soft.Protection == MM_ZERO_ACCESS)) 
		{
			return FALSE;
		}
		
		PointerPte = MiGetPteAddress (VirtualAddress);
		if ((PointerPte->u.Long == MM_ZERO_PTE) ||
			(PointerPte->u.Long == MM_ZERO_KERNEL_PTE) ||
			(PointerPte->u.Soft.Protection == MM_ZERO_ACCESS)) 
		{
			return FALSE;
		}
	}

	return TRUE;
}

#if 0
extern "C" extern ULONG KiBugCheckData;

VOID MmAllowPageFaultsAtRaisedIrql ()
/**
	Allow page faults at raised IRQL
*/
{
	//
	// KiTrap0E performs check if KiBugCheckData == 0.
	// If so, KiTrap0E calls KeBugCheckEx(IRQL_NOT_LESS_OR_EQUAL,...)
	//  if not, it calls MmAccessFault and resolves fault.
	//

	KiBugCheckData = 0xFFFFFFFF;
}

VOID MmForbidPageFaultsAtRaisedIrql ()
{
	KiBugCheckData = 0;
}
#endif

// Irql < DISPATCH_LEVEL
VOID MmMakeAddressWritable (PVOID VirtualAddress, BOOLEAN Write)
{
	ASSERT (MmIsSystemAddressAccessable(VirtualAddress));

	// load page
	ULONG t = *(ULONG*)VirtualAddress;

	// make it writable
	if (CR4() & PAE_ON)
	{
		PMMPTE_PAE PointerPte;

		PointerPte = MiGetPteAddressPae (VirtualAddress);
		PointerPte->u.Hard.Write = Write;
	}
	else
	{
		PMMPTE PointerPte;

		PointerPte = MiGetPteAddress (VirtualAddress);
		PointerPte->u.Hard.Valid = Write;
	}
}

PMDL LockMem (PVOID Buffer, ULONG Size)
{
	PMDL Mdl = IoAllocateMdl (Buffer, Size, FALSE, FALSE, NULL);
	if (Mdl)
	{
		KdPrint(("LOCKMEM: Mdl allocated at %X\n", Mdl));
		
		__try
		{
			for (ULONG i=0; i<Size; i++)
			{
				((UCHAR*)Buffer)[i] = ((UCHAR*)Buffer)[i];
			}
			
			KdPrint (("LOCKMEM: Loaded pages\n"));

			MmProbeAndLockPages (Mdl, KernelMode, IoWriteAccess);

			KdPrint (("LOCKMEM: Mdl probed and locked\n"));
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			IoFreeMdl (Mdl);
			KdPrint(("LockMem: MmProbeAndLock pages failed with exception code %X\n", GetExceptionCode()));
			return NULL;
		}

		return Mdl;
	}

	KdPrint(("LockMem: IoAllocateMdl failed\n"));
	return NULL;
}


VOID UnlockMem (PMDL Mdl)
{
	MmUnlockPages (Mdl);
	IoFreeMdl (Mdl);
}
