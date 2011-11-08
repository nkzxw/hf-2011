//======================================================================
// 
// Reglib.c
//
// Copyright (C) 1996-2002 Mark Russinovich and Bryce Cogswell
//
// Implements secret formula so that we work on Whistler.
//
//======================================================================
#include "ntddk.h"
#include "..\Exe\ioctlcmd.h"
#include "..\sys\reglib.h"

//
// Signature so that we can recognize drivers based on Regmon
//
CHAR Signature[] = "slanretnisys";

//
// Definition for system call service table
//
typedef struct _SRVTABLE {
	PVOID           *ServicePointers;
#if defined(_M_IA64)
	ULONGLONG       Count;
	ULONG           Limit;
    ULONG           Unused;
#else
	ULONG           Count;
	ULONG           Limit;
#endif
	PVOID		    *NumArguments;
} SRVTABLE, *PSRVTABLE;

//
// Pointer to the image of the system service table
//
extern PSRVTABLE KeServiceDescriptorTable;

//
// Mdl that describes the system service table
//
PMDL                    KeServiceTableMdl = NULL;

//----------------------------------------------------------------------
//
// RegmonUnmapMem
//
// Unmaps previously mapped memory.
//
//----------------------------------------------------------------------
VOID
RegmonUnmapMem(
    PVOID Pointer,
    PMDL Mdl
    )
{
    MmUnmapLockedPages( Pointer, Mdl );
    ExFreePool( Mdl );
}

//----------------------------------------------------------------------
//
// RegmonMapMem
//
// Double maps memory for writing.
//
//----------------------------------------------------------------------
PVOID 
RegmonMapMem( 
    PVOID Pointer, 
    ULONG Length, 
    PMDL *MapMdl 
    )
{
    *MapMdl = MmCreateMdl( NULL, Pointer, Length );
    if( !(*MapMdl)) {

        return NULL;
    }

    MmBuildMdlForNonPagedPool( *MapMdl );
    (*MapMdl)->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;
#if defined(_M_IA64)
    return MmMapLockedPagesSpecifyCache( *MapMdl, KernelMode, 
                                         MmCached, NULL, FALSE, NormalPagePriority );
#else
    return MmMapLockedPages( *MapMdl, KernelMode );
#endif
}

//----------------------------------------------------------------------
//
// RegmonUnmapServiceTable
//
//----------------------------------------------------------------------
VOID
RegmonUnmapServiceTable( 
    PVOID KeServiceTablePointers
    )
{
    if( KeServiceTableMdl ) {

        MmUnmapLockedPages( KeServiceTablePointers, KeServiceTableMdl );
        ExFreePool( KeServiceTableMdl );
    }
}


//----------------------------------------------------------------------
//
// RegmonMapServiceTable
//
// If we are running on Whistler then we have
// to double map the system service table to get around the 
// fact that the system service table is write-protected on systems
// with > 128MB memory. Since there's no harm in always double mapping,
// we always do it, regardless of whether or not we are on Whistler.
//
//----------------------------------------------------------------------
PVOID *
RegmonMapServiceTable(
    SERVICE_HOOK_DESCRIPTOR **HookDescriptors
    )
{
    //
    // Allocate an array to store original function addresses in. This
    // makes us play well with other hookers.
    //
    *HookDescriptors = (SERVICE_HOOK_DESCRIPTOR *) ExAllocatePool( NonPagedPool,
                          KeServiceDescriptorTable->Limit * sizeof(SERVICE_HOOK_DESCRIPTOR));
    if( !*HookDescriptors ) {

        return NULL;
    }
    memset( *HookDescriptors, 0, 
            KeServiceDescriptorTable->Limit * sizeof(SERVICE_HOOK_DESCRIPTOR));

    //
    // Build an MDL that describes the system service table function 
    // pointers array.
    //
    KdPrint(("Reglib: KeServiceDescriptorTable: %I64x Pointers: %I64x Limit: %d\n",
              KeServiceDescriptorTable, KeServiceDescriptorTable->ServicePointers,
              KeServiceDescriptorTable->Limit ));
    KeServiceTableMdl = MmCreateMdl( NULL, KeServiceDescriptorTable->ServicePointers, 
                                     KeServiceDescriptorTable->Limit * sizeof(PVOID));
    if( !KeServiceTableMdl ) {

        return NULL;
    }

    //
    // Fill in the physical pages and then double-map the description. Note
    // that MmMapLockedPages is obsolete as of Win2K and has been replaced
    // with MmMapLockedPagesSpecifyCache. However, we use the same driver
    // on all NT platforms, so we use it anyway.
    //
    MmBuildMdlForNonPagedPool( KeServiceTableMdl );
    KeServiceTableMdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;
#if defined(_M_IA64)
    return MmMapLockedPagesSpecifyCache( KeServiceTableMdl, KernelMode, 
                                         MmCached, NULL, FALSE, NormalPagePriority );
#else
    return MmMapLockedPages( KeServiceTableMdl, KernelMode );
#endif
}
