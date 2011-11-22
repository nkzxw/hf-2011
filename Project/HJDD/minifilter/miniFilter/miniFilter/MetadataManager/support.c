/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    operations.c

Abstract:

    This is the support routines module of the kernel mode filter driver implementing
    filter metadata management.


Environment:

    Kernel mode


--*/



#include "pch.h"

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FmmAllocateUnicodeString)
#pragma alloc_text(PAGE, FmmFreeUnicodeString)
#pragma alloc_text(PAGE, FmmTargetIsVolumeOpen)
#endif

//
//  Support Routines
//

NTSTATUS
FmmAllocateUnicodeString (
    __inout PUNICODE_STRING String
    )
/*++

Routine Description:

    This routine allocates a unicode string

Arguments:

    String - supplies the size of the string to be allocated in the MaximumLength field
             return the unicode string

Return Value:

    STATUS_SUCCESS                  - success
    STATUS_INSUFFICIENT_RESOURCES   - failure

--*/
{
    PAGED_CODE();

    String->Buffer = ExAllocatePoolWithTag( PagedPool,
                                            String->MaximumLength,
                                            FMM_STRING_TAG );

    if (String->Buffer == NULL) {

        DebugTrace( DEBUG_TRACE_ERROR,
                    ("[Fmm]: Failed to allocate unicode string of size 0x%x\n",
                    String->MaximumLength) );

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    String->Length = 0;

    return STATUS_SUCCESS;
}

VOID
FmmFreeUnicodeString (
    __inout PUNICODE_STRING String
    )
/*++

Routine Description:

    This routine frees a unicode string

Arguments:

    String - supplies the string to be freed

Return Value:

    None

--*/
{
    PAGED_CODE();

    ExFreePoolWithTag( String->Buffer,
                       FMM_STRING_TAG );

    String->Length = String->MaximumLength = 0;
    String->Buffer = NULL;
}


BOOLEAN
FmmTargetIsVolumeOpen (
    __in PFLT_CALLBACK_DATA Cbd
    )
/*++

Routine Description:

    This routine returns if the target object in this callback datastructure
    is a volume.  If the file object is NULL (which can happen when draining)
    then assume this is NOT a volume file object

Arguments:

    Cbd                   - Supplies a pointer to the callbackData which
                            declares the requested operation.

Return Value:

    TRUE  - target is a volume
    FALSE - target is not a volume

--*/
{
    PAGED_CODE();

    if ((Cbd->Iopb->TargetFileObject != NULL) &&
        FlagOn( Cbd->Iopb->TargetFileObject->Flags, FO_VOLUME_OPEN )) {

         return TRUE;
    } else {

        return FALSE;
    }
}

