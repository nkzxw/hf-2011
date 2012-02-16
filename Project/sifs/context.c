#include "fileflt.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CtxFindOrCreateStreamContext)
#pragma alloc_text(PAGE, CtxCreateStreamContext)
#pragma alloc_text(PAGE, CtxFindOrCreateStreamHandleContext)
#pragma alloc_test(PAGE, CtxCreateOrReplaceStreamHandleContext)
#pragma alloc_text(PAGE, CtxCreateStreamHandleContext)
#endif

NTSTATUS
CtxFindOrCreateStreamContext (
    __in PFLT_CALLBACK_DATA Cbd,
    __in BOOLEAN CreateIfNotFound,
    __deref_out PSTREAM_CONTEXT *StreamContext,
    __out_opt PBOOLEAN ContextCreated
    )
/*++

Routine Description:

    This routine finds the stream context for the target stream.
    Optionally, if the context does not exist this routing creates
    a new one and attaches the context to the stream.

Arguments:

    Cbd                   - Supplies a pointer to the callbackData which
                            declares the requested operation.
    CreateIfNotFound      - Supplies if the stream must be created if missing
    StreamContext         - Returns the stream context
    ContextCreated        - Returns if a new context was created

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PSTREAM_CONTEXT streamContext;
    PSTREAM_CONTEXT oldStreamContext;

    PAGED_CODE();

    *StreamContext = NULL;
    if (ContextCreated != NULL) *ContextCreated = FALSE;

    //
    //  First try to get the stream context.
    //

    status = FltGetStreamContext( Cbd->Iopb->TargetInstance,
                                  Cbd->Iopb->TargetFileObject,
                                  &streamContext );

    //
    //  If the call failed because the context does not exist
    //  and the user wants to creat a new one, the create a
    //  new context
    //

    if (!NT_SUCCESS( status ) &&
        (status == STATUS_NOT_FOUND) &&
        CreateIfNotFound) {


        //
        //  Create a stream context
        //

        status = CtxCreateStreamContext( &streamContext );

        if (!NT_SUCCESS( status )) {

            return status;
        }


        //
        //  Set the new context we just allocated on the file object
        //

        status = FltSetStreamContext( Cbd->Iopb->TargetInstance,
                                      Cbd->Iopb->TargetFileObject,
                                      FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                      streamContext,
                                      &oldStreamContext );

        if (!NT_SUCCESS( status )) {

            //
            //  We release the context here because FltSetStreamContext failed
            //
            //  If FltSetStreamContext succeeded then the context will be returned
            //  to the caller. The caller will use the context and then release it
            //  when he is done with the context.
            //

            FltReleaseContext( streamContext );

            if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED) {

                //
                //  FltSetStreamContext failed for a reason other than the context already
                //  existing on the stream. So the object now does not have any context set
                //  on it. So we return failure to the caller.
                //

                return status;
            }

            //
            //  Race condition. Someone has set a context after we queried it.
            //  Use the already set context instead
            //

            //
            //  Return the existing context. Note that the new context that we allocated has already been
            //  realeased above.
            //

            streamContext = oldStreamContext;
            status = STATUS_SUCCESS;

        } else {

            if (ContextCreated != NULL) *ContextCreated = TRUE;
        }
    }

    *StreamContext = streamContext;

    return status;
}




NTSTATUS
CtxCreateStreamContext (
    __deref_out PSTREAM_CONTEXT *StreamContext
    )
/*++

Routine Description:

    This routine creates a new stream context

Arguments:

    StreamContext         - Returns the stream context

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PSTREAM_CONTEXT streamContext;

    PAGED_CODE();

    //
    //  Allocate a stream context
    //

    status = FltAllocateContext( g_FileFltContext.FileFltHandle,
                                 FLT_STREAM_CONTEXT,
                                 STREAM_CONTEXT_SIZE,
                                 NonPagedPool,
                                 &streamContext );

    if (!NT_SUCCESS( status )) {

        return status;
    }

    //
    //  Initialize the newly created context
    //

    RtlZeroMemory( streamContext,  STREAM_CONTEXT_SIZE );

    streamContext->Resource = FsAllocateResource();
    if(streamContext->Resource == NULL) {

        FltReleaseContext( streamContext );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    ExInitializeResourceLite( streamContext->Resource );

    *StreamContext = streamContext;

    return STATUS_SUCCESS;
}

VOID
CtxUpdateAttributeInStreamContext(
	__inout PSTREAM_CONTEXT StreamContext, 
	__in PVOLUME_CONTEXT VolumeContext
	)
{
	StreamContext->VolumeDeviceType = VolumeContext->DeviceType;
	StreamContext->FileSystemType = VolumeContext->FileSystemType;
}

NTSTATUS
CtxFindOrCreateStreamHandleContext (
    __in PFLT_CALLBACK_DATA Cbd,
    __in BOOLEAN CreateIfNotFound,
    __deref_out PSTREAMHANDLE_CONTEXT *StreamHandleContext,
    __out_opt PBOOLEAN ContextCreated
    )
/*++

Routine Description:

    This routine finds the stream context for the target stream.
    Optionally, if the context does not exist this routing creates
    a new one and attaches the context to the stream.

Arguments:

    Cbd                   - Supplies a pointer to the callbackData which
                            declares the requested operation.
    CreateIfNotFound      - Supplies if the stream must be created if missing
    StreamContext         - Returns the stream context
    ContextCreated        - Returns if a new context was created

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PSTREAMHANDLE_CONTEXT streamHandleContext;
    PSTREAMHANDLE_CONTEXT oldStreamHandleContext;

    PAGED_CODE();

    *StreamHandleContext = NULL;
    if (ContextCreated != NULL) *ContextCreated = FALSE;

    //
    //  First try to get the stream context.
    //

    status = FltGetStreamHandleContext( Cbd->Iopb->TargetInstance,
                                  Cbd->Iopb->TargetFileObject,
                                  &streamHandleContext );

    //
    //  If the call failed because the context does not exist
    //  and the user wants to creat a new one, the create a
    //  new context
    //

    if (!NT_SUCCESS( status ) &&
        (status == STATUS_NOT_FOUND) &&
        CreateIfNotFound) {


        //
        //  Create a stream context
        //

        status = CtxCreateStreamHandleContext( &streamHandleContext );

        if (!NT_SUCCESS( status )) {

            return status;
        }


        //
        //  Set the new context we just allocated on the file object
        //

        status = FltSetStreamContext( Cbd->Iopb->TargetInstance,
                                      Cbd->Iopb->TargetFileObject,
                                      FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                      streamHandleContext,
                                      &oldStreamHandleContext );

        if (!NT_SUCCESS( status )) {

            //
            //  We release the context here because FltSetStreamContext failed
            //
            //  If FltSetStreamContext succeeded then the context will be returned
            //  to the caller. The caller will use the context and then release it
            //  when he is done with the context.
            //

            FltReleaseContext( streamHandleContext );

            if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED) {

                //
                //  FltSetStreamContext failed for a reason other than the context already
                //  existing on the stream. So the object now does not have any context set
                //  on it. So we return failure to the caller.
                //

                return status;
            }

            //
            //  Race condition. Someone has set a context after we queried it.
            //  Use the already set context instead
            //

            //
            //  Return the existing context. Note that the new context that we allocated has already been
            //  realeased above.
            //

            streamHandleContext = oldStreamHandleContext;
            status = STATUS_SUCCESS;

        } else {

            if (ContextCreated != NULL) *ContextCreated = TRUE;
        }
    }

    *StreamHandleContext = streamHandleContext;

    return status;
}

NTSTATUS
CtxCreateOrReplaceStreamHandleContext (
    __in PFLT_CALLBACK_DATA Cbd,
    __in BOOLEAN ReplaceIfExists,
    __deref_out PSTREAMHANDLE_CONTEXT *StreamHandleContext,
    __out_opt PBOOLEAN ContextReplaced
    )
/*++

Routine Description:

    This routine creates a stream handle context for the target stream
    handle. Optionally, if the context already exists, this routine
    replaces it with the new context and releases the old context

Arguments:

    Cbd                   - Supplies a pointer to the callbackData which
                            declares the requested operation.
    ReplaceIfExists       - Supplies if the stream handle context must be
                            replaced if already present
    StreamContext         - Returns the stream context
    ContextReplaced       - Returns if an existing context was replaced

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PSTREAMHANDLE_CONTEXT streamHandleContext;
    PSTREAMHANDLE_CONTEXT oldStreamHandleContext;

    PAGED_CODE();

    *StreamHandleContext = NULL;
    if (ContextReplaced != NULL) *ContextReplaced = FALSE;

    //
    //  Create a stream context
    //

    status = CtxCreateStreamHandleContext( &streamHandleContext );

    if (!NT_SUCCESS( status )) {

        return status;
    }

    //
    //  Set the new context we just allocated on the file object
    //

    status = FltSetStreamHandleContext( Cbd->Iopb->TargetInstance,
                                        Cbd->Iopb->TargetFileObject,
                                        ReplaceIfExists ? FLT_SET_CONTEXT_REPLACE_IF_EXISTS : FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                        streamHandleContext,
                                        &oldStreamHandleContext );

    if (!NT_SUCCESS( status )) {

        //
        //  We release the context here because FltSetStreamContext failed
        //
        //  If FltSetStreamContext succeeded then the context will be returned
        //  to the caller. The caller will use the context and then release it
        //  when he is done with the context.
        //


        FltReleaseContext( streamHandleContext );

        if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED) {

            //
            //  FltSetStreamContext failed for a reason other than the context already
            //  existing on the stream. So the object now does not have any context set
            //  on it. So we return failure to the caller.
            //


            return status;
        }

        //
        //  We will reach here only if we have failed with STATUS_FLT_CONTEXT_ALREADY_DEFINED
        //  and we can fail with that code only if the context already exists and we have used
        //  the FLT_SET_CONTEXT_KEEP_IF_EXISTS flag

        ASSERT( ReplaceIfExists  == FALSE );

        //
        //  Race condition. Someone has set a context after we queried it.
        //  Use the already set context instead
        //


        //
        //  Return the existing context. Note that the new context that we allocated has already been
        //  realeased above.
        //

        streamHandleContext = oldStreamHandleContext;
        status = STATUS_SUCCESS;

    } else {

        //
        //  FltSetStreamContext has suceeded. The new context will be returned
        //  to the caller. The caller will use the context and then release it
        //  when he is done with the context.
        //
        //  However, if we have replaced an existing context then we need to
        //  release the old context so as to decrement the ref count on it.
        //
        //  Note that the memory allocated to the objects within the context
        //  will be freed in the context cleanup and must not be done here.
        //

        if ( ReplaceIfExists &&
             oldStreamHandleContext != NULL) {

	      streamHandleContext->TaskState = oldStreamHandleContext->TaskState;

            FltReleaseContext( oldStreamHandleContext );
            if (ContextReplaced != NULL) *ContextReplaced = TRUE;
        }
    }

    *StreamHandleContext = streamHandleContext;

    return status;
}





NTSTATUS
CtxCreateStreamHandleContext (
    __deref_out PSTREAMHANDLE_CONTEXT *StreamHandleContext
    )
/*++

Routine Description:

    This routine creates a new stream context

Arguments:

    StreamContext         - Returns the stream context

Return Value:

    Status

--*/
{
    NTSTATUS status;
    PSTREAMHANDLE_CONTEXT streamHandleContext;

    PAGED_CODE();

    //
    //  Allocate a stream context
    //

    status = FltAllocateContext( g_FileFltContext.FileFltHandle,
                                 FLT_STREAMHANDLE_CONTEXT,
                                 STREAMHANDLE_CONTEXT_SIZE,
                                 NonPagedPool,
                                 &streamHandleContext );

    if (!NT_SUCCESS( status )) {


        return status;
    }

    //
    //  Initialize the newly created context
    //

    RtlZeroMemory( streamHandleContext, STREAMHANDLE_CONTEXT_SIZE );

    streamHandleContext->Resource = FsAllocateResource();
    if(streamHandleContext->Resource == NULL) {

        FltReleaseContext( streamHandleContext );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    ExInitializeResourceLite( streamHandleContext->Resource );

    *StreamHandleContext = streamHandleContext;

    return STATUS_SUCCESS;
}
