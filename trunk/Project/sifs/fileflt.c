/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    SwapBuffers.c

Abstract:

    This is a sample filter which demonstrates proper access of data buffer
    and a general guideline of how to swap buffers.
    For now it only swaps buffers for:

    IRP_MJ_READ
    IRP_MJ_WRITE
    IRP_MJ_DIRECTORY_CONTROL

    By default this filter attaches to all volumes it is notified about.  It
    does support having multiple instances on a given volume.

Environment:

    Kernel mode

--*/

#include "fileflt.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

FILEFLT_CONTEXT g_FileFltContext;

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, InstanceSetup)
#pragma alloc_text(PAGE, CleanupContext)
#pragma alloc_text(PAGE, InstanceQueryTeardown)
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, ReadDriverParameters)
#pragma alloc_text(PAGE, FilterUnload)
#pragma alloc_text(PAGE, SwapPreCleanup)
#pragma alloc_text(PAGE, SwapPreClose)
#pragma alloc_text(PAGE, SwapPreRead)
#pragma alloc_text(PAGE, SwapPreWrite)
#pragma alloc_text(PAGE, SwapPreQueryInformation)
#pragma alloc_text(PAGE, SwapPreSetInformation)
#pragma alloc_text(PAGE, SwapPreNetworkQueryOpen)
#pragma alloc_text(PAGE, SwapPreDirCtrlBuffers)
#endif

//
//  Operation we currently care about.
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
	{ IRP_MJ_CREATE,
	  0,//FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
	  SwapPreCreate,
	  SwapPostCreate},
		
	{ IRP_MJ_CLEANUP,
	  0,
	  SwapPreCleanup,
	  NULL },

	{ IRP_MJ_CLOSE,
	  0,
	  SwapPreClose,
	  NULL },
			
	{ IRP_MJ_READ,
	  0,
	  SwapPreRead,
	  SwapPostRead },

	{ IRP_MJ_WRITE,
	  0,
	  SwapPreWrite,
	  SwapPostWrite},

	{ IRP_MJ_QUERY_INFORMATION,
	  0,
	  SwapPreQueryInformation,
	  SwapPostQueryInformation},
	
	{ IRP_MJ_SET_INFORMATION,
	  0,
	  SwapPreSetInformation,
	  SwapPostSetInformation },

#if 0
	{ IRP_MJ_NETWORK_QUERY_OPEN,
	  0,
	  SwapPreNetworkQueryOpen,
	  SwapPostNetworkQueryOpen},
#endif
	{ IRP_MJ_DIRECTORY_CONTROL,
         0,
         SwapPreDirCtrlBuffers,
         SwapPostDirCtrlBuffers },

	{ IRP_MJ_OPERATION_END }
};

//
//  Context definitions we currently care about.  Note that the system will
//  create a lookAside list for the volume context because an explicit size
//  of the context is specified.
//

CONST FLT_CONTEXT_REGISTRATION ContextNotifications[] = {

     { FLT_VOLUME_CONTEXT,
       0,
       CleanupContext,
       sizeof(VOLUME_CONTEXT),
       VOLUME_CONTEXT_TAG },
       
     { FLT_STREAM_CONTEXT,
       0,
       CleanupContext,
       sizeof(STREAM_CONTEXT),
       STREAM_CONTEXT_TAG },

     { FLT_STREAMHANDLE_CONTEXT,
       0,
       CleanupContext,
       sizeof(STREAMHANDLE_CONTEXT),
       STREAMHANDLE_CONTEXT_TAG },

     { FLT_CONTEXT_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    ContextNotifications,               //  Context
    Callbacks,                          //  Operation callbacks

    FilterUnload,                       //  MiniFilterUnload

    InstanceSetup,                      //  InstanceSetup
    InstanceQueryTeardown,              //  InstanceQueryTeardown
    NULL,                               //  InstanceTeardownStart
    NULL,                               //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//                      Routines
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


NTSTATUS
InstanceSetup (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume.

    By default we want to attach to all volumes.  This routine will try and
    get a "DOS" name for the given volume.  If it can't, it will try and
    get the "NT" name for the volume (which is what happens on network
    volumes).  If a name is retrieved a volume context will be created with
    that name.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
    PVOLUME_CONTEXT ctx = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    
    PAGED_CODE();

    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    try {

        //
        //  Allocate a volume context structure.
        //

        status = FltAllocateContext( FltObjects->Filter,
                                     FLT_VOLUME_CONTEXT,
                                     VOLUME_CONTEXT_SIZE,
                                     NonPagedPool,
                                     &ctx );

        if (!NT_SUCCESS(status)) {

            //
            //  We could not allocate a context, quit now
            //

            leave;
        }

	RtlZeroMemory(ctx,  VOLUME_CONTEXT_SIZE);

	status = FltInstanceSetup(FltObjects, Flags, VolumeDeviceType, VolumeFilesystemType, ctx);
	
       if(!NT_SUCCESS(status)) {		
		
		leave;
      	}
	
        //
        //  Set the context
        //

        status = FltSetVolumeContext( FltObjects->Volume,
                                      FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                      ctx,
                                      NULL );       

        //
        //  It is OK for the context to already be defined.
        //

        if (status == STATUS_FLT_CONTEXT_ALREADY_DEFINED) {

            status = STATUS_SUCCESS;
        }

    } finally {

        //
        //  Always release the context.  If the set failed, it will free the
        //  context.  If not, it will remove the reference added by the set.
        //  Note that the name buffer in the ctx will get freed by the context
        //  cleanup routine.
        //

        if (ctx) {

            FltReleaseContext( ctx );
        }
    }

    return status;
}


VOID
CleanupContext(
    __in PFLT_CONTEXT Context,
    __in FLT_CONTEXT_TYPE ContextType
    )
/*++

Routine Description:

    The given context is being freed.
    Free the allocated name buffer if there one.

Arguments:

    Context - The context being freed

    ContextType - The type of context this is

Return Value:

    None

--*/
{	
	PAGED_CODE();

	FltCleanupContext(Context, ContextType);
	
}


NTSTATUS
InstanceQueryTeardown (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach.  We always return it is OK to
    detach.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Always succeed.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    return STATUS_SUCCESS;
}


/*************************************************************************
    Initialization and unload routines.
*************************************************************************/



NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine.  This registers with FltMgr and
    initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Status of the operation

--*/
{
    NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;

    if(module_init() == -1) {

		goto SwapDriverEntryExit;
    }
		
    RtlZeroMemory( &g_FileFltContext, sizeof( g_FileFltContext) );

    g_FileFltContext.Status |= (FILEFLT_SYSTEM_INSTALL | FILEFLT_WORK_RUNNING);


    //
    //  Get debug trace flags
    //

    ReadDriverParameters( RegistryPath );

  
    //
    //  Register with FltMgr
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &g_FileFltContext.FileFltHandle );

    if (! NT_SUCCESS( status )) {

        goto SwapDriverEntryExit;
    }

    //
    //  Start filtering i/o
    //

    status = FltStartFiltering( g_FileFltContext.FileFltHandle );

    if (! NT_SUCCESS( status )) {

        FltUnregisterFilter( g_FileFltContext.FileFltHandle );
        goto SwapDriverEntryExit;
    }

SwapDriverEntryExit:

    if(!NT_SUCCESS(status)) {

	module_exit();
    }
		
    return status;
}


NTSTATUS
FilterUnload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    Called when this mini-filter is about to be unloaded.  We unregister
    from the FltMgr and then return it is OK to unload

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns the final status of this operation.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER( Flags );

    //
    //  Unregister from FLT mgr
    //

    FltUnregisterFilter( g_FileFltContext.FileFltHandle );

    //
    //  Delete lookaside list
    //

    module_exit();

    return STATUS_SUCCESS;
}


/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
SwapPreCreate(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine demonstrates how to swap buffers for the CREATE operation.

    Note that it handles all errors by simply not doing the buffer swap.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Receives the context that will be passed to the
        post-operation callback.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - we want a postOpeation callback
    FLT_PREOP_SUCCESS_NO_CALLBACK - we don't want a postOperation callback

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PVOLUME_CONTEXT volCtx = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();
	
    try {	
	
	// only return FLT_PREOP_SUCCESS_NO_CALLBACK or FLT_PREOP_COMPLETE
	
    	retValue = SifsPreCreate(Data, FltObjects, CompletionContext, volCtx);
    
	if(retValue == FLT_PREOP_SUCCESS_NO_CALLBACK) {

            status = FltGetVolumeContext( FltObjects->Filter,
                                      FltObjects->Volume,
                                      &volCtx );

            if (!NT_SUCCESS(status)) {
                
                leave;
            }


	     retValue = FltPreCreate(Data, FltObjects, CompletionContext, volCtx);
	}
	
    }finally{

	     if (volCtx != NULL) {

                FltReleaseContext( volCtx );
            }			
    }
	
    return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
SwapPostCreate(
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
	FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;

	retValue = FltPostCreate(Cbd, FltObjects, CbdContext, Flags);

	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SwapPreCleanup(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine demonstrates how to swap buffers for the CREATE operation.

    Note that it handles all errors by simply not doing the buffer swap.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Receives the context that will be passed to the
        post-operation callback.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - we want a postOpeation callback
    FLT_PREOP_SUCCESS_NO_CALLBACK - we don't want a postOperation callback

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

    PAGED_CODE();

    // only return FLT_PREOP_SUCCESS_NO_CALLBACK or FLT_PREOP_COMPLETE
    
    retValue = SifsPreCleanup(Data, FltObjects, CompletionContext);
    
    if(retValue == FLT_PREOP_SUCCESS_NO_CALLBACK){
		
    	retValue = FltPreCleanup(Data, FltObjects, CompletionContext);
    }
    
    return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SwapPreClose(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine demonstrates how to swap buffers for the CREATE operation.

    Note that it handles all errors by simply not doing the buffer swap.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Receives the context that will be passed to the
        post-operation callback.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - we want a postOpeation callback
    FLT_PREOP_SUCCESS_NO_CALLBACK - we don't want a postOperation callback

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

    PAGED_CODE();

    // only return FLT_PREOP_SUCCESS_NO_CALLBACK or FLT_PREOP_COMPLETE
    
    retValue = SifsPreClose(Data, FltObjects, CompletionContext);
    
    if(retValue == FLT_PREOP_SUCCESS_NO_CALLBACK){
		
    	retValue = FltPreClose(Data, FltObjects, CompletionContext);
    }
    
    return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SwapPreRead(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine demonstrates how to swap buffers for the READ operation.

    Note that it handles all errors by simply not doing the buffer swap.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Receives the context that will be passed to the
        post-operation callback.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - we want a postOpeation callback
    FLT_PREOP_SUCCESS_NO_CALLBACK - we don't want a postOperation callback

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    ULONG readLen = iopb->Parameters.Read.Length;
    PVOLUME_CONTEXT volumeContext = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    // only return FLT_PREOP_SUCCESS_NO_CALLBACK or FLT_PREOP_COMPLETE
    
    retValue = SifsPreRead(Data, FltObjects, CompletionContext);
    
    if(retValue == FLT_PREOP_SUCCESS_NO_CALLBACK) {


		if(readLen == 0) {

			goto SwapPreReadCleanup;
		}
        
		status = FltGetVolumeContext( FltObjects->Filter,
                                      FltObjects->Volume,
                                      &volumeContext );

             if (!NT_SUCCESS(status)) {

                LOG_PRINT( LOGFL_ERRORS,
                           ("FileFlt!SwapPreRead:          Error getting volume context, status=%x\n",
                            status) );

                 goto SwapPreReadCleanup;
             }

    		retValue = FltPreRead(Data, FltObjects, CompletionContext, volumeContext);
    }

SwapPreReadCleanup:

    if(volumeContext != NULL ){

		FltReleaseContext(volumeContext);
    }
    	
    return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
SwapPostRead(
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;

    retValue = FltPostRead(Cbd, FltObjects, CbdContext, Flags);
    		
    return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SwapPreWrite(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine demonstrates how to swap buffers for the WRITE operation.

    Note that it handles all errors by simply not doing the buffer swap.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Receives the context that will be passed to the
        post-operation callback.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - we want a postOpeation callback
    FLT_PREOP_SUCCESS_NO_CALLBACK - we don't want a postOperation callback
    FLT_PREOP_COMPLETE -
--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    ULONG writeLen = iopb->Parameters.Write.Length;
    PVOLUME_CONTEXT volumeContext = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    // only return FLT_PREOP_SUCCESS_NO_CALLBACK or FLT_PREOP_COMPLETE
    
    retValue = SifsPreWrite(Data, FltObjects, CompletionContext);
    
    if(retValue == FLT_PREOP_SUCCESS_NO_CALLBACK) {

         if (writeLen == 0) {

            goto SwapPreWriteCleanup;
         }

         status = FltGetVolumeContext( FltObjects->Filter,
                                      FltObjects->Volume,
                                      &volumeContext );

         if (!NT_SUCCESS(status)) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("FileFlt!SwapPreWrite:          Error getting volume context, status=%x\n",
                        status) );

             goto SwapPreWriteCleanup;
         }
             
    	  retValue = FltPreWrite(Data, FltObjects, CompletionContext, volumeContext);
    }

 SwapPreWriteCleanup:

     if(volumeContext != NULL ){

		FltReleaseContext(volumeContext);
     }
    
    return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
SwapPostWrite(
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;

    retValue = FltPostWrite(Cbd, FltObjects, CbdContext, Flags);
    		
    return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SwapPreQueryInformation (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
	FLT_PREOP_CALLBACK_STATUS 	retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

	PAGED_CODE();

	// only return FLT_PREOP_SUCCESS_NO_CALLBACK or FLT_PREOP_COMPLETE
    
	retValue = SifsPreQueryInformation(Data, FltObjects, CompletionContext);

	if(retValue == FLT_PREOP_SUCCESS_NO_CALLBACK) {
		
		retValue = FltPreQueryInformation(Data, FltObjects, CompletionContext);
	}

	return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
SwapPostQueryInformation (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
	FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;

	retValue = FltPostQueryInformation(Data, FltObjects, CompletionContext, Flags);
    		
	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SwapPreSetInformation(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine demonstrates how to swap buffers for the CREATE operation.

    Note that it handles all errors by simply not doing the buffer swap.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Receives the context that will be passed to the
        post-operation callback.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - we want a postOpeation callback
    FLT_PREOP_SUCCESS_NO_CALLBACK - we don't want a postOperation callback

--*/
{
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

    PAGED_CODE();

   // only return FLT_PREOP_SUCCESS_NO_CALLBACK or FLT_PREOP_COMPLETE
   
    retValue = SifsPreSetInformation(Data, FltObjects, CompletionContext);
    
    if(retValue == FLT_PREOP_SUCCESS_NO_CALLBACK){
    			
    		retValue = FltPreSetInformation(Data, FltObjects, CompletionContext);
    }
    
    return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
SwapPostSetInformation (
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;

    retValue = FltPostSetInformation(Cbd, FltObjects, CbdContext, Flags);
    
    return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SwapPreNetworkQueryOpen(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine demonstrates how to swap buffers for the CREATE operation.

    Note that it handles all errors by simply not doing the buffer swap.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Receives the context that will be passed to the
        post-operation callback.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - we want a postOpeation callback
    FLT_PREOP_SUCCESS_NO_CALLBACK - we don't want a postOperation callback

--*/
{
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

    PAGED_CODE();

   // only return FLT_PREOP_SUCCESS_NO_CALLBACK or FLT_PREOP_COMPLETE
   
    retValue = SifsPreNetworkQueryOpen(Data, FltObjects, CompletionContext);
    
    if(retValue == FLT_PREOP_SUCCESS_NO_CALLBACK){
    			
    		retValue = FltPreNetworkQueryOpen(Data, FltObjects, CompletionContext);
    }
    
    return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
SwapPostNetworkQueryOpen (
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;

    retValue = FltPostNetworkQueryOpen(Cbd, FltObjects, CbdContext, Flags);
    
    return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SwapPreDirCtrlBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine demonstrates how to swap buffers for the Directory Control
    operations.  The reason this routine is here is because directory change
    notifications are long lived and this allows you to see how FltMgr
    handles long lived IRP operations that have swapped buffers when the
    mini-filter is unloaded.  It does this by canceling the IRP.

    Note that it handles all errors by simply not doing the
    buffer swap.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Receives the context that will be passed to the
        post-operation callback.

Return Value:

    FLT_PREOP_SUCCESS_WITH_CALLBACK - we want a postOpeation callback
    FLT_PREOP_SUCCESS_NO_CALLBACK - we don't want a postOperation callback

--*/
{    
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PVOLUME_CONTEXT volumeContext = NULL;

    PAGED_CODE();

   // only return FLT_PREOP_SUCCESS_NO_CALLBACK or FLT_PREOP_COMPLETE
   
    retValue = SifsPreDirCtrlBuffers(Data, FltObjects, CompletionContext);

    if(retValue == FLT_PREOP_SUCCESS_NO_CALLBACK){

        NTSTATUS status = STATUS_SUCCESS;
		
        PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;	 
		
        //
        //  If they are trying to get ZERO bytes, then don't do anything and
        //  we don't need a post-operation callback.
        //

        if (iopb->Parameters.DirectoryControl.QueryDirectory.Length == 0) {

            goto SwapPreDirCtrlBuffersCleanup;
        }

	 if(iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY) {

	     goto SwapPreDirCtrlBuffersCleanup;
	 }

	 if(Data->RequestorMode == KernelMode) {

		goto SwapPreDirCtrlBuffersCleanup;
	 }

	 //
        //  Get our volume context.  If we can't get it, just return.
        //

        status = FltGetVolumeContext( FltObjects->Filter,
                                      FltObjects->Volume,
                                      &volumeContext );

        if (!NT_SUCCESS(status)) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("FileFlt!SwapPreDirCtrlBuffers:          Error getting volume context, status=%x\n",
                        status) );

             goto SwapPreDirCtrlBuffersCleanup;
        }


    	 retValue = FltPreDirCtrlBuffers(Data, FltObjects, CompletionContext, volumeContext);
    }

SwapPreDirCtrlBuffersCleanup:

    if(volumeContext != NULL) {

	FltReleaseContext(volumeContext);
    }
	
    return retValue;
 }


FLT_POSTOP_CALLBACK_STATUS
SwapPostDirCtrlBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine does the post Directory Control buffer swap handling.

Arguments:

    This routine does postRead buffer swap handling
    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The completion context set in the pre-operation routine.

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING
    FLT_POSTOP_MORE_PROCESSING_REQUIRED

--*/
{
    FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;

    retValue = FltPostDirCtrlBuffers(Data, FltObjects, CompletionContext, Flags);
    
    return retValue;
}

static VOID
SwapGetRegisterStringValue(
	  __in  PUCHAR  Source,
	  __in  USHORT SrcLen,
	  __out PWCHAR Dest,
	  __in  USHORT DestLen
	  )
{
	UNICODE_STRING source;
	UNICODE_STRING dest;

	UNREFERENCED_PARAMETER( SrcLen );

	RtlInitUnicodeString(&source,((PCWSTR)(((PKEY_VALUE_PARTIAL_INFORMATION) Source)->Data)));
	
	RtlInitUnicodeString(&dest, Dest);
	dest.MaximumLength = DestLen;

	RtlCopyUnicodeString(&dest, &source);
}

void
ReadDriverParameters (
    __in PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine tries to read the driver-specific parameters from
    the registry.  These values will be found in the registry location
    indicated by the RegistryPath passed in.

Arguments:

    RegistryPath - the path key passed to the driver during driver entry.

Return Value:

    None.

--*/
{
    OBJECT_ATTRIBUTES attributes;
    HANDLE driverRegKey;
    NTSTATUS status;
    ULONG resultLength;
    UNICODE_STRING valueName;
    UCHAR buffer[sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + DEVICE_NAME_MAX_LENGTH];

    //
    //  If this value is not zero then somebody has already explicitly set it
    //  so don't override those settings.
    //


    //
    //  Open the desired registry key
    //

    InitializeObjectAttributes( &attributes,
                                RegistryPath,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    status = ZwOpenKey( &driverRegKey,
                        KEY_READ,
                        &attributes );

    if (!NT_SUCCESS( status )) {

        return;
    }

    //
    // Read the given value from the registry.
    //

    if( 0 == g_FileFltContext.LoggingFlags) {
	
        RtlInitUnicodeString( &valueName, L"DebugFlags" );
        if( FsReadDriverParameters(driverRegKey, &valueName, buffer, sizeof(buffer)) == TRUE){

	     g_FileFltContext.LoggingFlags = *((PULONG) &(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data));
        }
    }

    RtlInitUnicodeString(&valueName, L"Start");
    if( FsReadDriverParameters(driverRegKey, &valueName, buffer, sizeof(buffer)) == TRUE){
		
       ULONG start = *((PULONG) &(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data));
	   
	g_FileFltContext.Status |= FILEFLT_DRIVER_STATUS_VALUE(start);
    }
	
    //
    //  Close the registry entry
    //

    ZwClose(driverRegKey);
}

