#include "fileflt.h"
#include <ntifs.h>
#include <ntdddisk.h>

NTSTATUS
FltInstanceSetup(
   __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType,
    __inout PVOLUME_CONTEXT VolumeContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG retLen = 0;
    PUNICODE_STRING workingName = NULL;
    USHORT size = 0;
    UCHAR volPropBuffer[sizeof(FLT_VOLUME_PROPERTIES)+512];
    PFLT_VOLUME_PROPERTIES volProp = (PFLT_VOLUME_PROPERTIES)volPropBuffer;
    PDEVICE_OBJECT volumeDevice = NULL;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    try {
        
        //
        //  Always get the volume properties, so I can get a sector size
        //

        status = FltGetVolumeProperties( FltObjects->Volume,
                                         volProp,
                                         sizeof(volPropBuffer),
                                         &retLen );

        if (!NT_SUCCESS(status)) {

            leave;
        }

        //
        //  Save the sector size in the context for later use.  Note that
        //  we will pick a minimum sector size if a sector size is not
        //  specified.
        //

        ASSERT((volProp->SectorSize == 0) || (volProp->SectorSize >= MIN_SECTOR_SIZE));

        VolumeContext->SectorSize = max(volProp->SectorSize,MIN_SECTOR_SIZE);
        
	 VolumeContext->FileSystemType = VolumeFilesystemType;
	 VolumeContext->DeviceType = VolumeDeviceType;
	 ExInitializeNPagedLookasideList( &VolumeContext->Pre2PostContextList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(PRE_2_POST_CONTEXT),
                                     PRE_2_POST_TAG,
                                     0 );

	 RtlInitEmptyUnicodeString(&(VolumeContext->VolumeName), VolumeContext->VolumeNameBuffer, sizeof(VolumeContext->VolumeNameBuffer));
	 
	 if(volProp->RealDeviceName.Length > 0){
	 	
	 	workingName = &volProp->RealDeviceName;
	 }else{

	 	workingName = &volProp->FileSystemDeviceName;
	 }

	 RtlCopyUnicodeString(&(VolumeContext->VolumeName), workingName);
	 
        //
        //  Log debug info
        //

     LOG_PRINT( LOGFL_VOLCTX,
                   ("FileFlt!InstanceSetup:                  Real SectSize=0x%04x, Used SectSize=0x%04x, Name=\"%wZ\", DeviceType = 0x%x, FileSystemType = 0x%x\n",
                    volProp->SectorSize,
                    VolumeContext->SectorSize,
                    &(VolumeContext->VolumeName),
                    VolumeContext->DeviceType,
                    VolumeContext->FileSystemType)
                    );
        

    } finally {

        //
        //  Remove the reference added to the device object by
        //  FltGetDiskDeviceObject.
        //

        if (volumeDevice) {

            ObDereferenceObject( volumeDevice );
        }
    }

    return status;

}

VOID
FltCleanupContext(
    __in PFLT_CONTEXT Context,
    __in FLT_CONTEXT_TYPE ContextType
    )
{
	PVOLUME_CONTEXT volumeContext = NULL;
	PSTREAM_CONTEXT streamContext = NULL;
	PSTREAMHANDLE_CONTEXT streamHandleContext = NULL;

	PAGED_CODE();

	UNREFERENCED_PARAMETER( ContextType );
	
	switch(ContextType){
		
	case FLT_VOLUME_CONTEXT:

		volumeContext = Context;
		
		ExDeleteNPagedLookasideList( &volumeContext->Pre2PostContextList );
		
		break;
	case FLT_STREAM_CONTEXT:
		
		streamContext = Context;

		if (streamContext->Resource != NULL) {

            		ExDeleteResourceLite( streamContext->Resource );
            		FsFreeResource( streamContext->Resource);
        	}
		if(streamContext->Lower.FileObject != NULL) {

			FsCloseFile(streamContext->Lower.FileHandle, streamContext->Lower.FileObject);
		}
		if(streamContext->Lower.Metadata != NULL) {

			ExFreePoolWithTag(streamContext->Lower.Metadata, SIFS_METADATA_TAG);
		}
		if(streamContext->NameInfo != NULL) {

			FltReleaseFileNameInformation( streamContext->NameInfo  );
		}
		
		break;
	case FLT_STREAMHANDLE_CONTEXT:

		streamHandleContext =  Context;

		 //
	        //  Delete the resource and memory the memory allocated for the resource
	        //

	        if (streamHandleContext->Resource != NULL) {

	            ExDeleteResourceLite( streamHandleContext->Resource );
	            FsFreeResource( streamHandleContext->Resource );
	        }
			
		break;
	}   
}

static FLT_TASK_STATE
FltGetTaskStateInPreCreate(
	__inout PFLT_CALLBACK_DATA Data
	)
{
	FLT_TASK_STATE 		taskState = FLT_TASK_STATE_UNKNOWN;

	TaskGetState(PsGetCurrentProcessId(), &taskState);

	return taskState;
}

static int
FltCheckPreCreatePassthru_1(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects
    )
{
	int rc = 0;

	if(FltObjects->FileObject->FileName.Length == 0) {
		
		goto RuleCheckPreCreatePassthru_1Cleanup;
	}

	if (FlagOn( Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE )) {
		
		goto RuleCheckPreCreatePassthru_1Cleanup;
	}
	
	if(FlagOn( Data->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY)){
		
		goto RuleCheckPreCreatePassthru_1Cleanup;
	}
	
	if (FlagOn( Data->Iopb->TargetFileObject->Flags, FO_VOLUME_OPEN )) { 
	
		goto RuleCheckPreCreatePassthru_1Cleanup;
	}   

	rc = -1;
	
RuleCheckPreCreatePassthru_1Cleanup:
	

	return rc;
}

static int
FltCheckPreCreatePassthru_2(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOLUME_CONTEXT VolumeContext,
    __in PFLT_FILE_NAME_INFORMATION NameInfo
    )
{
	int rc = 0;
	UNICODE_STRING pattern;

	RtlInitUnicodeString(&pattern, L"txt");

	  // 测试条件: 仅对\\Device\\HarddiskVolume3 加密 LanmanRedirector
       if((NameInfo->Volume.Length == 0)
		|| ((RtlCompareMemory(NameInfo->Volume.Buffer, L"\\Device\\HarddiskVolume3", 46) != 46)
		&& (RtlCompareMemory(NameInfo->Volume.Buffer, L"\\Device\\LanmanRedirector", 48) != 48))){

		goto FltCheckPreCreatePassthru_2_Cleanup;

	}
	   
	if((NameInfo->Extension.Length == 6)
		&& (RtlCompareUnicodeString (&NameInfo->Extension,  &pattern, TRUE) == 0)) {
		
		rc = -1;
	}

FltCheckPreCreatePassthru_2_Cleanup:
	
	return rc;
}

FLT_PREOP_CALLBACK_STATUS
FltPreCreate(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext,
    __in PVOLUME_CONTEXT VolumeContext
    )
{
	FLT_PREOP_CALLBACK_STATUS 	retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
	FLT_TASK_STATE 				taskState = FLT_TASK_STATE_UNKNOWN;
	PFLT_FILE_NAME_INFORMATION 	nameInfo = NULL;
	NTSTATUS 					status = STATUS_SUCCESS;	
	BOOLEAN 					directory = FALSE;
	BOOLEAN 					fileExist = FALSE;	
	BOOLEAN						cryptedFile = FALSE;
	BOOLEAN						sifsMetadataExist = FALSE;
	BOOLEAN						isEmptyFile = FALSE;
	PPRE_2_POST_CONTEXT 		p2pCtx = NULL;
	FLT_IO_RULE					ioRule = FLT_IORULE_RDWT;
	CRYPT_CONTEXT				cryptContext;
       ULONG                                       desiredAccess = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
       ULONG                                        createDisposition = (Data->Iopb->Parameters.Create.Options >> 24) & 0x000000ff;
       ULONG                                        createOptions = Data->Iopb->Parameters.Create.Options & 0x00ffffff;
       ULONG                                        shareAccess = Data->Iopb->Parameters.Create.ShareAccess;
       ULONG                                        fileAttribute = Data->Iopb->Parameters.Create.FileAttributes;

	if(FileFltStatusValidate) {

		goto FltPreCreateCleanup;
		
	}

	if(FltCheckPreCreatePassthru_1(Data, FltObjects) == 0) {
		
		goto FltPreCreateCleanup;
	}
	
	status = FltGetFileNameInformation( Data,
	                                        FLT_FILE_NAME_NORMALIZED |
	                                        FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
	                                        &nameInfo );
       if (!NT_SUCCESS( status )) {

            goto FltPreCreateCleanup;
       }

       status = FltParseFileNameInformation( nameInfo );
	  
       if (!NT_SUCCESS( status )) {
        
	 	goto FltPreCreateCleanup;
       }

	if((BooleanFlagOn( Data->Iopb->Parameters.Create.Options, FILE_DIRECTORY_FILE))
		|| ((FsCheckFileExistAndDirectoryByFileName(Data, FltObjects, nameInfo, &fileExist, &directory) == 0)
			&& (directory == TRUE))) {
		
		goto FltPreCreateCleanup;
		
	}else{

		if(FltCheckPreCreatePassthru_2(Data, FltObjects, VolumeContext, nameInfo) == 0) {
			
			goto FltPreCreateCleanup;
		}
	}	

	taskState = FltGetTaskStateInPreCreate(Data);	
	
	SifsInitializeCryptContext(&cryptContext);

	if((createDisposition == FILE_CREATE)
		|| (createDisposition == FILE_SUPERSEDE)
		|| (createDisposition == FILE_OVERWRITE)
		|| (createDisposition == FILE_OVERWRITE_IF)) { //FILE_OPEN_IF

		
		fileExist = FALSE;		
	}
    
       if(taskState == FLT_TASK_STATE_EXPLORE_HOOK) {

              if(fileExist == TRUE) {

                if(SifsQuickCheckValidate(Data->Iopb->TargetInstance, &nameInfo->Name
					, &cryptContext, &isEmptyFile, VolumeContext->SectorSize) == 0){

                    ioRule = FLT_IORULE_RDWT;
                    sifsMetadataExist = TRUE;
                    cryptedFile = TRUE;                    
                }else{

			if(isEmptyFile == FALSE){
				
                    		goto FltPreCreateCleanup;
			}
                }
              }else {

                    goto FltPreCreateCleanup;
              }
	}else  if(taskState != FLT_TASK_STATE_TRUST_HOOK) {
		// no crypted file
		if(fileExist == TRUE) {

			if(SifsQuickCheckValidate(Data->Iopb->TargetInstance, &nameInfo->Name
					, &cryptContext, &isEmptyFile, VolumeContext->SectorSize) == 0) {

				Data->IoStatus.Status = STATUS_ACCESS_DENIED;
				Data->IoStatus.Information = 0;		
					
				retValue = FLT_PREOP_COMPLETE;
						
				LOG_PRINT( LOGFL_ERRORS,
		                   	("FileFlt!FltPreCreate:    Pid = %d,  fileName = \"%wZ\" Failed to open crypted file\n",
		                    		PsGetCurrentProcessId(), &(nameInfo->Name)) );				
			}
		}
        
              goto FltPreCreateCleanup;
              
        }else {
        
		if(fileExist == TRUE) {

			if((SifsQuickCheckValidate(Data->Iopb->TargetInstance, &nameInfo->Name
					, &cryptContext, &isEmptyFile, VolumeContext->SectorSize) == -1)
				&& (isEmptyFile == FALSE)){

				ioRule = FLT_IORULE_ONLYRD;
			}else{

				if(isEmptyFile == FALSE) {

					sifsMetadataExist = TRUE;
				}
				
				cryptedFile = TRUE;
			}
		}else{

			cryptedFile = TRUE;
		}		
	}

       if((cryptedFile == TRUE) 
            && (sifsMetadataExist == FALSE)){

           if(desiredAccess & (FILE_WRITE_DATA|  FILE_APPEND_DATA)) {

               if(SifsWriteSifsMetadata(Data->Iopb->TargetInstance
                            , desiredAccess, createDisposition, createOptions, shareAccess, fileAttribute
                            , nameInfo, &cryptContext) == -1) {

                    LOG_PRINT( LOGFL_ERRORS,
                       	("FileFlt!FltPreCreate:    Pid = %d,  FileName = \"%wZ\" Failed to write sifs metadata.\n",
                        		PsGetCurrentProcessId(), &nameInfo->Name) );
                    
                    Data->IoStatus.Status = STATUS_ACCESS_DENIED;
		      Data->IoStatus.Information = 0;		
			
		      retValue = FLT_PREOP_COMPLETE;
                    goto FltPreCreateCleanup;
                    
        	    }	else{
                 
                   Data->Iopb->Parameters.Create.Options &= 0x00ffffff;
    	            Data->Iopb->Parameters.Create.Options |= (FILE_OPEN << 24);
    	            FltSetCallbackDataDirty(Data);
        	  }
            }else{

                goto FltPreCreateCleanup;
            }
       }
	
	p2pCtx = ExAllocateFromNPagedLookasideList( &VolumeContext->Pre2PostContextList );

	if(p2pCtx == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                   	("FileFlt!FltPreCreate:    Pid = %d,  FileName = \"%wZ\" Failed to allocate pre2Post context structure\n",
                    		PsGetCurrentProcessId(), &nameInfo->Name) );
            
            Data->IoStatus.Status = STATUS_ACCESS_DENIED;
            Data->IoStatus.Information = 0;		
            	
            retValue = FLT_PREOP_COMPLETE;	
		
            goto FltPreCreateCleanup;
	}
	
	FltReferenceContext(VolumeContext);
	p2pCtx->VolCtx = VolumeContext;
	p2pCtx->TaskState = taskState;
	p2pCtx->IoRule = ioRule;
	p2pCtx->CryptedFile = cryptedFile;
	p2pCtx->NameInfo = nameInfo;
	
	nameInfo = NULL;	

	*CompletionContext = p2pCtx;
	
	retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	
FltPreCreateCleanup:

	if(nameInfo != NULL){

		FltReleaseFileNameInformation( nameInfo );

	}
	
	return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
FltPostCreate(
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
	FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
	PPRE_2_POST_CONTEXT 		p2pCtx = CbdContext;
	PVOLUME_CONTEXT 			volumeContext = p2pCtx->VolCtx;
       PFLT_FILE_NAME_INFORMATION 	nameInfo = p2pCtx->NameInfo;
	PSTREAM_CONTEXT 			streamContext = NULL;
	PSTREAMHANDLE_CONTEXT 		streamHandleContext = NULL;	
	NTSTATUS 					status = STATUS_SUCCESS;
	BOOLEAN                                    streamContextCreated = FALSE, streamHandleContextReplaced = FALSE;
	ULONG                                       desiredAccess = Cbd->Iopb->Parameters.Create.SecurityContext->DesiredAccess ;
	FILE_STANDARD_INFORMATION   fileStandardInformation ;
	
	if(!NT_SUCCESS(Cbd->IoStatus.Status)) {

		goto FltPostCreateCleanup;
	}      

	status = FltQueryInformationFile(Cbd->Iopb->TargetInstance,
									 Cbd->Iopb->TargetFileObject,
									 &fileStandardInformation,
									 sizeof(fileStandardInformation),
									 FileStandardInformation,
									 NULL
									 ) ;

	if(!NT_SUCCESS(status)) {

               LOG_PRINT( LOGFL_ERRORS,
                   	("FileFlt!FltPostCreate:    Pid = %d,  FileName = \"%wZ\" Failed to get standard information.\n",
                    		PsGetCurrentProcessId(), &p2pCtx->NameInfo->Name) );
               
		goto FltPostCreateCleanup;
	}
	
	if(fileStandardInformation.Directory == TRUE) {

		goto FltPostCreateCleanup;
	}


	status = CtxFindOrCreateStreamContext(Cbd, 
                                          TRUE,
                                          &streamContext,
                                          &streamContextCreated);

	if (!NT_SUCCESS( status )) {

		goto FltPostCreateCleanup;
	}

	//
	//  Acquire write acccess to the context
	//

       FsAcquireResourceExclusive(streamContext->Resource);

	SifsInitializeCryptContext(&(streamContext->CryptContext));

	if((p2pCtx->CryptedFile == TRUE)
		&& ((streamContextCreated == TRUE)
			|| (streamContext->CryptedFile == FALSE))) {

		if(SifsReadSifsMetadata(Cbd->Iopb->TargetInstance, Cbd->Iopb->TargetFileObject, streamContext) == -1){

			p2pCtx->CryptedFile = FALSE;
		}			
	}    	

	//
    	//  Update the file name in the context
    	//
    	
    	CtxUpdateAttributeInStreamContext(streamContext, volumeContext);

    	streamContext->CryptedFile = p2pCtx->CryptedFile;
    	streamContext->FileSize = fileStandardInformation.EndOfFile;

	if(streamContext->NameInfo == NULL) {

		streamContext->NameInfo = nameInfo;

		nameInfo = NULL;
	}	
			
    	//
    	//  Relinquish write acccess to the context
    	//

       FsReleaseResource(streamContext->Resource);	

	LOG_PRINT(LOGFL_CREATE,  ("FileFlt!FltPostCreate:    Pid = %d, Name(%d)=\"%wZ\", TaskState = 0x%x, IoRule = %d, CryptedFile = %d, FileSize = %d\n"
		,  PsGetCurrentProcessId(), p2pCtx->NameInfo->Name.Length, &(p2pCtx->NameInfo->Name), p2pCtx->TaskState, p2pCtx->IoRule, p2pCtx->CryptedFile, streamContext->FileSize.QuadPart));
	
FltPostCreateCleanup:

	if(nameInfo != NULL) {

		FltReleaseFileNameInformation( nameInfo  );
	}
	
	if (streamContext != NULL) {

        	FltReleaseContext( streamContext );            
       }

	if (streamHandleContext != NULL) {

        	FltReleaseContext( streamHandleContext );            
    	}	
	
	ExFreeToNPagedLookasideList(&volumeContext->Pre2PostContextList,
                                         p2pCtx );

       if(volumeContext != NULL){

		FltReleaseContext( volumeContext );
	}
	
	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
FltPreCleanup(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext,
    __in PVOLUME_CONTEXT VolumeContext
    )
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;	
	PSTREAM_CONTEXT streamContext = NULL;
	BOOLEAN streamContextCreated = FALSE;
	NTSTATUS status = STATUS_SUCCESS;

	__try{

		status = CtxFindOrCreateStreamContext(Data, 
	                                              FALSE,
	                                              &streamContext,
	                                              &streamContextCreated);

	        if (!NT_SUCCESS( status )) {

	        	__leave;
	        }

	        FsAcquireResourceShared(streamContext->Resource);

	        if(streamContext->CryptedFile == FALSE) {

	            FsReleaseResource(streamContext->Resource);
	            
	            __leave;
	        }	        		 

		 FsReleaseResource(streamContext->Resource);
		 
	}__finally{

		if(streamContext != NULL ){

			FltReleaseContext(streamContext);
		}
	}

	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
FltPreClose(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext,
    __in PVOLUME_CONTEXT VolumeContext
    )
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

	PSTREAM_CONTEXT streamContext = NULL;
	BOOLEAN streamContextCreated = FALSE;
	NTSTATUS status = STATUS_SUCCESS;

	__try{

		status = CtxFindOrCreateStreamContext(Data, 
	                                              FALSE,
	                                              &streamContext,
	                                              &streamContextCreated);

	        if (!NT_SUCCESS( status )) {

	        	__leave;
	        }

	        FsAcquireResourceShared(streamContext->Resource);

	        if(streamContext->CryptedFile == FALSE) {

	            FsReleaseResource(streamContext->Resource);
	            
	            __leave;
	        }	        		 

		 FsReleaseResource(streamContext->Resource);

#if 0
		 if(SifsWriteFileSize(Data->Iopb->TargetInstance, &(streamContext->NameInfo->Name)
			,streamContext->Lower.Metadata, streamContext->CryptContext.MetadataSize, streamContext->FileSize.QuadPart) == -1) {

			LOG_PRINT(LOGFL_ERRORS,  ("FileFlt!FltPreClose:    Pid = %d, FileSize = %lld failed to write head.\n"
				,  PsGetCurrentProcessId(),  streamContext->FileSize.QuadPart));
		 }
#endif
		 
	}__finally{

		if(streamContext != NULL ){

			FltReleaseContext(streamContext);
		}
	}

	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
FltPreRead(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext,
    __in PVOLUME_CONTEXT VolumeContext
    )
{
    PFLT_IO_PARAMETER_BLOCK       iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS   retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PSTREAM_CONTEXT 			    streamContext = NULL;
    NTSTATUS 					    status = STATUS_SUCCESS;
    BOOLEAN  					    streamContextCreated = FALSE;
    FLT_TASK_STATE 			    taskState = FLT_TASK_STATE_UNKNOWN;
    FLT_IO_RULE				    ioRule = FLT_IORULE_RDWT;
    ULONG 					    readLen = iopb->Parameters.Read.Length;
    PVOID                                       newBuf = NULL;
    PMDL                                         newMdl = NULL;
    PPRE_2_POST_CONTEXT             p2pCtx = NULL;
    ULONG					    noCache = Data->Iopb->IrpFlags & (IRP_NOCACHE | IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO);
    

    __try{        

        status = CtxFindOrCreateStreamContext(Data, 
                                              FALSE,
                                              &streamContext,
                                              &streamContextCreated);

        if (!NT_SUCCESS( status )) {

        	__leave;
        }

        FsAcquireResourceShared(streamContext->Resource);

        if(streamContext->CryptedFile == FALSE) {

            FsReleaseResource(streamContext->Resource);
            
            __leave;
        }

        FsReleaseResource(streamContext->Resource);
	 
	if(!noCache) {
		
		if((iopb->Parameters.Read.ByteOffset.LowPart == FILE_USE_FILE_POINTER_POSITION)
			&& (iopb->Parameters.Read.ByteOffset.HighPart == -1)){

			FsAcquireResourceShared(streamContext->Resource);

			if(streamContext->FileSize.QuadPart <= Data->Iopb->TargetFileObject->CurrentByteOffset.QuadPart) {

				FsReleaseResource(streamContext->Resource);

				Data->IoStatus.Status = STATUS_END_OF_FILE ;
				Data->IoStatus.Information = 0;		

				retValue = FLT_PREOP_COMPLETE;

				__leave;
			}				

			FsReleaseResource(streamContext->Resource);

		}else{

			FsAcquireResourceShared(streamContext->Resource);

			if(iopb->Parameters.Read.ByteOffset.QuadPart >= SifsValidateFileSize(streamContext)) {

				FsReleaseResource(streamContext->Resource);

				Data->IoStatus.Status = STATUS_END_OF_FILE ;
				Data->IoStatus.Information = 0;		

				retValue = FLT_PREOP_COMPLETE;

				__leave;
			}

			FsReleaseResource(streamContext->Resource);
		}         

	
	}else{

	        //
	        //  If this is a non-cached I/O we need to round the length up to the
	        //  sector size for this device.  We must do this because the file
	        //  systems do this and we need to make sure our buffer is as big
	        //  as they are expecting.
	        //

	        if (FlagOn(IRP_NOCACHE,iopb->IrpFlags)) {

	            readLen = (ULONG)ROUND_TO_SIZE(readLen,VolumeContext->SectorSize);
	        }

	        //
	        //  Allocate nonPaged memory for the buffer we are swapping to.
	        //  If we fail to get the memory, just don't swap buffers on this
	        //  operation.
	        //

	        newBuf = ExAllocatePoolWithTag( NonPagedPool,
	                                        readLen,
	                                        BUFFER_SWAP_TAG );

	        if (newBuf == NULL) {

	            LOG_PRINT( LOGFL_ERRORS,
	                       ("FileFlt!FltPreRead:    Pid = %d, %wZ Failed to allocate %d bytes of memory\n",
	                       PsGetCurrentProcessId(),
	                        &VolumeContext->VolumeName,
	                        readLen) );

	            __leave;
	        }

	        //
	        //  We only need to build a MDL for IRP operations.  We don't need to
	        //  do this for a FASTIO operation since the FASTIO interface has no
	        //  parameter for passing the MDL to the file system.
	        //

	        if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_IRP_OPERATION)) {

	            //
	            //  Allocate a MDL for the new allocated memory.  If we fail
	            //  the MDL allocation then we won't swap buffer for this operation
	            //

	            newMdl = IoAllocateMdl( newBuf,
	                                    readLen,
	                                    FALSE,
	                                    FALSE,
	                                    NULL );

	            if (newMdl == NULL) {

	                LOG_PRINT( LOGFL_ERRORS,
	                           ("FileFlt!FltPreRead:    Pid = %d %wZ Failed to allocate MDL\n",
	                           PsGetCurrentProcessId(),
	                            &VolumeContext->VolumeName) );

	                __leave;
	            }

	            //
	            //  setup the MDL for the non-paged pool we just allocated
	            //

	            MmBuildMdlForNonPagedPool( newMdl );
	        }

		if((iopb->Parameters.Read.ByteOffset.LowPart == FILE_USE_FILE_POINTER_POSITION)
	        	&& (iopb->Parameters.Read.ByteOffset.HighPart == -1)){

	            iopb->Parameters.Read.ByteOffset = Data->Iopb->TargetFileObject->CurrentByteOffset;

		     if(Data->Iopb->TargetFileObject->CurrentByteOffset.QuadPart == 0) {

			  iopb->Parameters.Read.ByteOffset.QuadPart += streamContext->CryptContext.MetadataSize;
		     }

	        }else{

	            FsAcquireResourceShared(streamContext->Resource);
	            
	            iopb->Parameters.Read.ByteOffset.QuadPart += streamContext->CryptContext.MetadataSize;

	            FsReleaseResource(streamContext->Resource);
	        }

		 //
	        //  Update the buffer pointers and MDL address, mark we have changed
	        //  something.
	        //

	        iopb->Parameters.Read.ReadBuffer = newBuf;
	        iopb->Parameters.Read.MdlAddress = newMdl;


	        FltSetCallbackDataDirty( Data );

	 }

        //
        //  We are ready to swap buffers, get a pre2Post context structure.
        //  We need it to pass the volume context and the allocate memory
        //  buffer to the post operation callback.
        //

        p2pCtx = ExAllocateFromNPagedLookasideList( &VolumeContext->Pre2PostContextList );

        if (p2pCtx == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("FileFlt!FltPreRead:     Pid = %d %wZ Failed to allocate pre2Post context structure\n",
                       PsGetCurrentProcessId(),
                        &VolumeContext->VolumeName) );

            __leave;
        }

        
	 
        //
        //  Log that we are swapping
        //

        LOG_PRINT( LOGFL_READ,
                   ("FileFlt!FltPreRead:    Pid = %d %wZ newB=%p newMdl=%p oldB=%p oldMdl=%p len=%d byteOffset=%d(%d) noCache = 0x%x\n",
                   PsGetCurrentProcessId(),
                    &VolumeContext->VolumeName,
                    newBuf,
                    newMdl,
                    iopb->Parameters.Read.ReadBuffer,
                    iopb->Parameters.Read.MdlAddress,
                    readLen, 
                    iopb->Parameters.Read.Length,
                    iopb->Parameters.Read.ByteOffset.QuadPart,
                    noCache) ); 
        
        //
        //  Pass state to our post-operation callback.
        //

        FltReferenceContext(VolumeContext);
        FltReferenceContext(streamContext);

        p2pCtx->SwappedBuffer = newBuf;
        p2pCtx->VolCtx = VolumeContext;
        p2pCtx->StreamContext = streamContext;

        *CompletionContext = p2pCtx;

        //
        //  Return we want a post-operation callback
        //

        retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
        
    }__finally{

	if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK) {

            if (newBuf != NULL) {

                ExFreePool( newBuf );
            }

            if (newMdl != NULL) {

                IoFreeMdl( newMdl );
            }
        }
    
        if(streamContext != NULL) {

        	FltReleaseContext(streamContext);
        }
    }

    return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
FltPostRead(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    PVOID origBuf = NULL;
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    BOOLEAN cleanupAllocatedBuffer = TRUE;
    ULONG   noCache = Data->Iopb->IrpFlags & (IRP_NOCACHE | IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO);

    //
    //  This system won't draining an operation with swapped buffers, verify
    //  the draining flag is not set.
    //

    ASSERT(!FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING));

    __try {

        //
        //  If the operation failed or the count is zero, there is no data to
        //  copy so just return now.
        //

        if (!NT_SUCCESS(Data->IoStatus.Status) ||
            (Data->IoStatus.Information == 0)) {

            LOG_PRINT( LOGFL_READ,
                       ("FileFlt!FltPostRead:    Pid = %d %wZ newB=%p No data read, status=%x, info=%x\n",
                       PsGetCurrentProcessId(),
                        &p2pCtx->VolCtx->VolumeName,
                        p2pCtx->SwappedBuffer,
                        Data->IoStatus.Status,
                        Data->IoStatus.Information) );

            __leave;
        }

	if(!noCache) {

		if(Data->IoStatus.Information > 0) {

			if((Data->Iopb->Parameters.Read.ByteOffset.LowPart == FILE_USE_FILE_POINTER_POSITION)
	        	    && (Data->Iopb->Parameters.Read.ByteOffset.HighPart == -1)) {

	                if(p2pCtx->StreamContext->FileSize.QuadPart < Data->Iopb->TargetFileObject->CurrentByteOffset.QuadPart){

	                    Data->IoStatus.Information = p2pCtx->StreamContext->FileSize.QuadPart 
							- (Data->Iopb->TargetFileObject->CurrentByteOffset.QuadPart - Data->IoStatus.Information);
	                }
	                
	            }else{

	                if((Data->Iopb->Parameters.Read.ByteOffset.QuadPart  + Data->IoStatus.Information) > SifsValidateFileSize(p2pCtx->StreamContext)) {

	                    Data->IoStatus.Information = SifsValidateFileSize(p2pCtx->StreamContext) - Data->Iopb->Parameters.Read.ByteOffset.QuadPart;
	                }	                
	            }
	     	}
		
		__leave;
	}
        //
        //  We need to copy the read data back into the users buffer.  Note
        //  that the parameters passed in are for the users original buffers
        //  not our swapped buffers.
        //

        if (iopb->Parameters.Read.MdlAddress != NULL) {

            //
            //  There is a MDL defined for the original buffer, get a
            //  system address for it so we can copy the data back to it.
            //  We must do this because we don't know what thread context
            //  we are in.
            //

            origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.Read.MdlAddress,
                                                    NormalPagePriority );

            if (origBuf == NULL) {

                LOG_PRINT( LOGFL_ERRORS,
                           ("FileFlt!FltPostRead:    Pid = %d %wZ Failed to get system address for MDL: %p\n",
                           PsGetCurrentProcessId(),
                            &p2pCtx->VolCtx->VolumeName,
                            iopb->Parameters.Read.MdlAddress) );

                //
                //  If we failed to get a SYSTEM address, mark that the read
                //  failed and return.
                //

                Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                Data->IoStatus.Information = 0;
                __leave;
            }

        } else if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) ||
                   FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_FAST_IO_OPERATION)) {

            //
            //  If this is a system buffer, just use the given address because
            //      it is valid in all thread contexts.
            //  If this is a FASTIO operation, we can just use the
            //      buffer (inside a try/except) since we know we are in
            //      the correct thread context (you can't pend FASTIO's).
            //

            origBuf = iopb->Parameters.Read.ReadBuffer;

        } else {

            //
            //  They don't have a MDL and this is not a system buffer
            //  or a fastio so this is probably some arbitrary user
            //  buffer.  We can not do the processing at DPC level so
            //  try and get to a safe IRQL so we can do the processing.
            //

            if (FltDoCompletionProcessingWhenSafe( Data,
                                                   FltObjects,
                                                   CompletionContext,
                                                   Flags,
                                                   FltPostReadWhenSafe,
                                                   &retValue )) {

                //
                //  This operation has been moved to a safe IRQL, the called
                //  routine will do (or has done) the freeing so don't do it
                //  in our routine.
                //

                cleanupAllocatedBuffer = FALSE;

            } else {

                //
                //  We are in a state where we can not get to a safe IRQL and
                //  we do not have a MDL.  There is nothing we can do to safely
                //  copy the data back to the users buffer, fail the operation
                //  and return.  This shouldn't ever happen because in those
                //  situations where it is not safe to post, we should have
                //  a MDL.
                //

                LOG_PRINT( LOGFL_ERRORS,
                           ("FileFlt!FltPostRead:    Pid = %d %wZ Unable to post to a safe IRQL\n",
                           PsGetCurrentProcessId(),
                            &p2pCtx->VolCtx->VolumeName) );

                Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
                Data->IoStatus.Information = 0;
            }

            __leave;
        }

        //
        //  We either have a system buffer or this is a fastio operation
        //  so we are in the proper context.  Copy the data handling an
        //  exception.
        //

        try {
                        
            RtlCopyMemory( origBuf,
                           p2pCtx->SwappedBuffer,
                           Data->IoStatus.Information );

	    LOG_PRINT( LOGFL_READ,
               ("FileFlt!FltPostRead:    Pid = %d %wZ newB=%p status=0x%x info=%d byteOffset=%d(%d) Freeing\n",
               PsGetCurrentProcessId(),
                &p2pCtx->VolCtx->VolumeName,
                p2pCtx->SwappedBuffer,
                Data->IoStatus.Status,
                Data->IoStatus.Information,
                Data->Iopb->Parameters.Read.ByteOffset.QuadPart,
                Data->Iopb->TargetFileObject->CurrentByteOffset.QuadPart) ); 

        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            //  The copy failed, return an error, failing the operation.
            //

            Data->IoStatus.Status = GetExceptionCode();
            Data->IoStatus.Information = 0;

            LOG_PRINT( LOGFL_ERRORS,
                       ("FileFlt!FltPostRead:    Pid = %d %wZ Invalid user buffer, oldB=%p, status=%x\n",
                       PsGetCurrentProcessId(),
                        &p2pCtx->VolCtx->VolumeName,
                        origBuf,
                        Data->IoStatus.Status) );
        }

    }__finally {

        //
        //  If we are supposed to, cleanup the allocated memory and release
        //  the volume context.  The freeing of the MDL (if there is one) is
        //  handled by FltMgr.
        //

        if (cleanupAllocatedBuffer) {

            PVOLUME_CONTEXT volumeContext = p2pCtx->VolCtx;

	     if(p2pCtx->SwappedBuffer != NULL) {
		 	
            	ExFreePool( p2pCtx->SwappedBuffer ); 
	     }
            FltReleaseContext(p2pCtx->StreamContext);

            ExFreeToNPagedLookasideList( &volumeContext->Pre2PostContextList,
                                         p2pCtx );

            FltReleaseContext( volumeContext );
        }
    }

    return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
FltPostReadWhenSafe (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    We had an arbitrary users buffer without a MDL so we needed to get
    to a safe IRQL so we could lock it and then copy the data.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Contains state from our PreOperation callback

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING - This is always returned.

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    PVOID origBuf = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    PVOLUME_CONTEXT volumeContext = p2pCtx->VolCtx;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    ASSERT(Data->IoStatus.Information != 0);

    //
    //  This is some sort of user buffer without a MDL, lock the user buffer
    //  so we can access it.  This will create a MDL for it.
    //

    status = FltLockUserBuffer( Data );

    if (!NT_SUCCESS(status)) {

        LOG_PRINT( LOGFL_ERRORS,
                   ("FileFlt!FltPostReadWhenSafe:    Pid = %d %wZ Could not lock user buffer, oldB=%p, status=%x\n",
                   PsGetCurrentProcessId(),
                    &p2pCtx->VolCtx->VolumeName,
                    iopb->Parameters.Read.ReadBuffer,
                    status) );

        //
        //  If we can't lock the buffer, fail the operation
        //

        Data->IoStatus.Status = status;
        Data->IoStatus.Information = 0;

    } else {

        //
        //  Get a system address for this buffer.
        //

        origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.Read.MdlAddress,
                                                NormalPagePriority );

        if (origBuf == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("FileFlt!FltPostReadWhenSafe:    Pid = %d %wZ Failed to get system address for MDL: %p\n",
                       PsGetCurrentProcessId(),
                        &p2pCtx->VolCtx->VolumeName,
                        iopb->Parameters.Read.MdlAddress) );

            //
            //  If we couldn't get a SYSTEM buffer address, fail the operation
            //

            Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Data->IoStatus.Information = 0;

        } else {

             
             
            //
            //  Copy the data back to the original buffer.  Note that we
            //  don't need a try/except because we will always have a system
            //  buffer address.
            //

            RtlCopyMemory( origBuf,
                           p2pCtx->SwappedBuffer,
                           Data->IoStatus.Information );

            FsAcquireResourceShared( p2pCtx->StreamContext->Resource );
            
            if(Data->IoStatus.Information > 0) {

			if((Data->Iopb->Parameters.Read.ByteOffset.LowPart == FILE_USE_FILE_POINTER_POSITION)
	        	    && (Data->Iopb->Parameters.Read.ByteOffset.HighPart == -1)) {

	                if(p2pCtx->StreamContext->FileSize.QuadPart < Data->Iopb->TargetFileObject->CurrentByteOffset.QuadPart){

	                    Data->IoStatus.Information = p2pCtx->StreamContext->FileSize.QuadPart 
							- (Data->Iopb->TargetFileObject->CurrentByteOffset.QuadPart - Data->IoStatus.Information);
	                }
	                
	            }else{

	                if((Data->Iopb->Parameters.Read.ByteOffset.QuadPart  + Data->IoStatus.Information) > SifsValidateFileSize(p2pCtx->StreamContext)) {

	                    Data->IoStatus.Information = SifsValidateFileSize(p2pCtx->StreamContext) - Data->Iopb->Parameters.Read.ByteOffset.QuadPart;
	                }	                
	            }
		 	
	     }

            FsReleaseResource(p2pCtx->StreamContext->Resource);

        }
    }

    //
    //  Free allocated memory and release the volume context
    //

    LOG_PRINT( LOGFL_READ,
               ("FileFlt!FltPostReadWhenSafe:    Pid = %d %wZ newB=%p status=0x%x info=%d byteOffset=%d(%d) Freeing\n",
               PsGetCurrentProcessId(),
                &p2pCtx->VolCtx->VolumeName,
                p2pCtx->SwappedBuffer,
                Data->IoStatus.Status,
                Data->IoStatus.Information,
                Data->Iopb->Parameters.Read.ByteOffset.QuadPart,
                Data->Iopb->TargetFileObject->CurrentByteOffset.QuadPart) );

    ExFreePool( p2pCtx->SwappedBuffer );
    FltReleaseContext(p2pCtx->StreamContext);

    ExFreeToNPagedLookasideList( &volumeContext->Pre2PostContextList,
                                 p2pCtx );

    FltReleaseContext( volumeContext );

    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
FltPreWrite(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext,
    __in PVOLUME_CONTEXT VolumeContext
    )
{
    PFLT_IO_PARAMETER_BLOCK       iopb                              = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS   retValue 				= FLT_PREOP_SUCCESS_NO_CALLBACK;
    PSTREAM_CONTEXT 			    streamContext 		= NULL;
    NTSTATUS 					    status 				= STATUS_SUCCESS;
    BOOLEAN  					    streamContextCreated 	= FALSE;
    FLT_TASK_STATE 			    taskState 				= FLT_TASK_STATE_UNKNOWN;	
    FLT_IO_RULE				    ioRule 				= FLT_IORULE_RDWT;
    PVOID                                       newBuf                           = NULL;
    PMDL                                         newMdl                            = NULL;
    PPRE_2_POST_CONTEXT             p2pCtx                            = NULL;
    PVOID                                       origBuf                            = NULL;
    ULONG                                       writeLen                         = iopb->Parameters.Write.Length;

    __try{           
	     		 
            status = CtxFindOrCreateStreamContext(Data, 
                                              FALSE,
                                              &streamContext,
                                              &streamContextCreated);

            if (!NT_SUCCESS( status )) {

                __leave;
            }

            FsAcquireResourceShared(streamContext->Resource);

            if(FALSE == streamContext->CryptedFile){

                FsReleaseResource(streamContext->Resource);
                
                __leave;
            }

            FsReleaseResource(streamContext->Resource);

	     if(!(Data->Iopb->IrpFlags & (IRP_NOCACHE | IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO))) {

			retValue = FLT_PREOP_SYNCHRONIZE;	
			
	     }else {

	            //
	            //  If this is a non-cached I/O we need to round the length up to the
	            //  sector size for this device.  We must do this because the file
	            //  systems do this and we need to make sure our buffer is as big
	            //  as they are expecting.
	            //

	            if (FlagOn(IRP_NOCACHE,iopb->IrpFlags)) {

	                writeLen = (ULONG)ROUND_TO_SIZE(writeLen,VolumeContext->SectorSize);
	            }

	            //
	            //  Allocate nonPaged memory for the buffer we are swapping to.
	            //  If we fail to get the memory, just don't swap buffers on this
	            //  operation.
	            //

	            newBuf = ExAllocatePoolWithTag( NonPagedPool,
	                                            writeLen,
	                                            BUFFER_SWAP_TAG );

	            if (newBuf == NULL) {

	                LOG_PRINT( LOGFL_ERRORS,
	                           ("FileFlt!FltPreWrite:    Pid = %d %wZ Failed to allocate %d bytes of memory.\n",
	                           PsGetCurrentProcessId(),
	                            &VolumeContext->VolumeName,
	                            writeLen) );

	                __leave;
	            }

	            //
	            //  We only need to build a MDL for IRP operations.  We don't need to
	            //  do this for a FASTIO operation because it is a waste of time since
	            //  the FASTIO interface has no parameter for passing the MDL to the
	            //  file system.
	            //

	            if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_IRP_OPERATION)) {

	                //
	                //  Allocate a MDL for the new allocated memory.  If we fail
	                //  the MDL allocation then we won't swap buffer for this operation
	                //

	                newMdl = IoAllocateMdl( newBuf,
	                                        writeLen,
	                                        FALSE,
	                                        FALSE,
	                                        NULL );

	                if (newMdl == NULL) {

	                    LOG_PRINT( LOGFL_ERRORS,
	                               ("FileFlt!FltPreWrite:    Pid = %d %wZ Failed to allocate MDL.\n",
	                                PsGetCurrentProcessId(),
	                                &VolumeContext->VolumeName) );

	                    __leave;
	                }

	                //
	                //  setup the MDL for the non-paged pool we just allocated
	                //

	                MmBuildMdlForNonPagedPool( newMdl );
	            }

	            //
	            //  If the users original buffer had a MDL, get a system address.
	            //

	            if (iopb->Parameters.Write.MdlAddress != NULL) {

	                origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.Write.MdlAddress,
	                                                        NormalPagePriority );

	                if (origBuf == NULL) {

	                    LOG_PRINT( LOGFL_ERRORS,
	                               ("FileFlt!FltPreWrite:    Pid = %d %wZ Failed to get system address for MDL: %p\n",
	                               PsGetCurrentProcessId(),
	                                &VolumeContext->VolumeName,
	                                iopb->Parameters.Write.MdlAddress) );

	                    //
	                    //  If we could not get a system address for the users buffer,
	                    //  then we are going to fail this operation.
	                    //

	                    Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
	                    Data->IoStatus.Information = 0;
	                    retValue = FLT_PREOP_COMPLETE;
	                    __leave;
	                }

	            } else {

	                //
	                //  There was no MDL defined, use the given buffer address.
	                //

	                origBuf = iopb->Parameters.Write.WriteBuffer;
	            }

	            //
	            //  Copy the memory, we must do this inside the try/except because we
	            //  may be using a users buffer address
	            //

	            try {  

			  // 处理数据
			  
	                RtlCopyMemory( newBuf,
	                               origBuf,
	                               writeLen );


	            } except (EXCEPTION_EXECUTE_HANDLER) {

	                //
	                //  The copy failed, return an error, failing the operation.
	                //

	                Data->IoStatus.Status = GetExceptionCode();
	                Data->IoStatus.Information = 0;
	                retValue = FLT_PREOP_COMPLETE;

	                LOG_PRINT( LOGFL_ERRORS,
	                           ("FileFlt!FltPreWrite:    Pid = %d %wZ Invalid user buffer, oldB=%p, status=%x\n",
	                           PsGetCurrentProcessId(),
	                            &VolumeContext->VolumeName,
	                            origBuf,
	                            Data->IoStatus.Status) );

	                __leave;
	            }

		    FsAcquireResourceShared(streamContext->Resource);
		 
	            if((Data->Iopb->Parameters.Write.ByteOffset.LowPart ==  FILE_USE_FILE_POINTER_POSITION)
	            	&&  (Data->Iopb->Parameters.Write.ByteOffset.HighPart == -1)){
			  
			  if(streamContext->FileSize.QuadPart > Data->Iopb->TargetFileObject->CurrentByteOffset.QuadPart) {

				Data->Iopb->Parameters.Write.ByteOffset  = Data->Iopb->TargetFileObject->CurrentByteOffset;   
				
				if(Data->Iopb->TargetFileObject->CurrentByteOffset.QuadPart == 0) {
					
	                		Data->Iopb->Parameters.Write.ByteOffset.QuadPart += streamContext->CryptContext.MetadataSize;
				}
			  }else{

			  	Data->Iopb->Parameters.Write.ByteOffset = streamContext->FileSize;
			  }
	            }
	            else if ((Data->Iopb->Parameters.Write.ByteOffset.LowPart == FILE_WRITE_TO_END_OF_FILE)
	            	&& (Data->Iopb->Parameters.Write.ByteOffset.HighPart == -1 )) {

	                 Data->Iopb->Parameters.Write.ByteOffset = streamContext->FileSize; 

	            }else {

	                 Data->Iopb->Parameters.Write.ByteOffset.QuadPart += streamContext->CryptContext.MetadataSize; 
	            }

		     FsReleaseResource(streamContext->Resource);		     

	     }

            //
            //  We are ready to swap buffers, get a pre2Post context structure.
            //  We need it to pass the volume context and the allocate memory
            //  buffer to the post operation callback.
            //

            p2pCtx = ExAllocateFromNPagedLookasideList( &VolumeContext->Pre2PostContextList );

            if (p2pCtx == NULL) {

                LOG_PRINT( LOGFL_ERRORS,
                           ("FileFlt!FltPreWrite:    Pid = %d %wZ Failed to allocate pre2Post context structure\n",
                           PsGetCurrentProcessId(),
                            &VolumeContext->VolumeName) );

                __leave;
            }   	     
            
	     if((Data->Iopb->IrpFlags & (IRP_NOCACHE | IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO))) {

		     //
	            //  Set new buffers
	            //    
            
	            iopb->Parameters.Write.WriteBuffer = newBuf;
	            iopb->Parameters.Write.MdlAddress = newMdl;
	            FltSetCallbackDataDirty( Data );

		     //
	            //  Return we want a post-operation callback
	            //

	            retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	     }

	     LOG_PRINT( LOGFL_WRITE,
                       ("FileFlt!FltPreWrite:    Pid = %d %wZ newB=%p newMdl=%p oldB=%p oldMdl=%p len=%d(%d) byteOffset = %lld\n",
                       PsGetCurrentProcessId(),
                        &VolumeContext->VolumeName,
                        newBuf,
                        newMdl,
                        iopb->Parameters.Write.WriteBuffer,
                        iopb->Parameters.Write.MdlAddress,
                        writeLen,
                        iopb->Parameters.Write.Length, 
                        Data->Iopb->Parameters.Write.ByteOffset.QuadPart) );
		 
            //
            //  Pass state to our post-operation callback.
            //

            FltReferenceContext(VolumeContext);
            FltReferenceContext(streamContext);
        
            p2pCtx->SwappedBuffer = newBuf;
            p2pCtx->VolCtx = VolumeContext;
            p2pCtx->StreamContext = streamContext;

            *CompletionContext = p2pCtx;           
            
    }__finally{

            if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK) {

                if (newBuf != NULL) {

                    ExFreePool( newBuf );
                }

                if (newMdl != NULL) {

                    IoFreeMdl( newMdl );
                }
            }
            
            if(streamContext != NULL) {

                FltReleaseContext(streamContext);
            }
    }

    return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
FltPostWrite(
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{

    FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
    PPRE_2_POST_CONTEXT p2pCtx = CbdContext;
    PVOLUME_CONTEXT volumeContext = p2pCtx->VolCtx;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
	
    if(!(Cbd->Iopb->IrpFlags & (IRP_NOCACHE | IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO))) {

		NTSTATUS status = STATUS_SUCCESS;
		FILE_END_OF_FILE_INFORMATION fileEndOfFileInformation;
		LONGLONG fileSize = 0;

	__try{		
		
		if (!NT_SUCCESS(Cbd->IoStatus.Status) ||
	            (Cbd->IoStatus.Information == 0)) {

	            __leave;
	        }

		FsAcquireResourceShared(p2pCtx->StreamContext->Resource);

		if((Cbd->Iopb->Parameters.Write.ByteOffset.LowPart ==  FILE_USE_FILE_POINTER_POSITION)
            		&&  (Cbd->Iopb->Parameters.Write.ByteOffset.HighPart == -1)){

			if(Cbd->Iopb->TargetFileObject->CurrentByteOffset.QuadPart > p2pCtx->StreamContext->FileSize.QuadPart) {
				
				fileSize = Cbd->Iopb->TargetFileObject->CurrentByteOffset.QuadPart;
			}else{

				fileSize = p2pCtx->StreamContext->FileSize.QuadPart;
			}
		}else if ((Cbd->Iopb->Parameters.Write.ByteOffset.LowPart == FILE_WRITE_TO_END_OF_FILE)
            	 	&& (Cbd->Iopb->Parameters.Write.ByteOffset.HighPart == -1 )) {

			fileSize = p2pCtx->StreamContext->FileSize.QuadPart + Cbd->IoStatus.Information;
		
		}else{

			fileSize = Cbd->Iopb->Parameters.Write.ByteOffset.QuadPart + Cbd->IoStatus.Information + p2pCtx->StreamContext->CryptContext.MetadataSize;
		}

		if(fileSize > p2pCtx->StreamContext->FileSize.QuadPart) {
			
			fileEndOfFileInformation.EndOfFile.QuadPart = fileSize;

			FsReleaseResource(p2pCtx->StreamContext->Resource);
				
			status = FsSetInformationFile(Cbd->Iopb->TargetInstance,
								Cbd->Iopb->TargetFileObject,
								&fileEndOfFileInformation,
								sizeof(fileEndOfFileInformation),
								FileEndOfFileInformation);

			if(NT_SUCCESS(status)) {

				FsAcquireResourceExclusive(p2pCtx->StreamContext->Resource);

				p2pCtx->StreamContext->FileSize.QuadPart += Cbd->IoStatus.Information;

				FsReleaseResource(p2pCtx->StreamContext->Resource);
			}
		}else{

			FsReleaseResource(p2pCtx->StreamContext->Resource);
		}

#if 0
		if(SifsWriteFileSize(Cbd->Iopb->TargetInstance, Cbd->Iopb->TargetFileObject
			,p2pCtx->StreamContext->Lower.Metadata, p2pCtx->StreamContext->CryptContext.MetadataSize, fileSize) == -1) {

			LOG_PRINT(LOGFL_WRITE,  ("FileFlt!FltPostWrite:    Pid = %d, FileSize = %lld\n"
				,  PsGetCurrentProcessId(),  fileSize));
		 }
#endif

	}__finally{

		
	}			
    }else{
	    
	    //
	    //  Free allocate POOL and volume context
	    //

	    ExFreePool( p2pCtx->SwappedBuffer );	    
    }

     LOG_PRINT( LOGFL_WRITE,
	               ("FileFlt!FltPostWrite:    Pid = %d %wZ newB=%p status = 0x%x info=%d byteOffset = %lld fileSize = %lld Freeing\n",
	               PsGetCurrentProcessId(),
	                &p2pCtx->VolCtx->VolumeName,
	                p2pCtx->SwappedBuffer,
	                Cbd->IoStatus.Status,
	                Cbd->IoStatus.Information,
	                Cbd->Iopb->Parameters.Write.ByteOffset.QuadPart,
	                p2pCtx->StreamContext->FileSize.QuadPart) );
	 
    FltReleaseContext( p2pCtx->StreamContext );

    ExFreeToNPagedLookasideList( &volumeContext->Pre2PostContextList,
                                 p2pCtx );
    FltReleaseContext( volumeContext );

    return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
FltPostWriteWhenSafe (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    We had an arbitrary users buffer without a MDL so we needed to get
    to a safe IRQL so we could lock it and then copy the data.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - Contains state from our PreOperation callback

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING - This is always returned.

--*/
{
    FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    PVOLUME_CONTEXT volumeContext = p2pCtx->VolCtx;    

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    LOG_PRINT( LOGFL_WRITE,
               ("FileFlt!FltPostWriteWhenSafe:    Pid = %d %wZ newB=%p status = 0x%x info=%d byteOffset = %lld fileSize = %lld Freeing\n",
               PsGetCurrentProcessId(),
                &p2pCtx->VolCtx->VolumeName,
                p2pCtx->SwappedBuffer,
                Data->IoStatus.Status,
                Data->IoStatus.Information,
                Data->Iopb->Parameters.Write.ByteOffset.QuadPart,
                p2pCtx->StreamContext->FileSize.QuadPart) );

    ExFreePool( p2pCtx->SwappedBuffer );
    FltReleaseContext(p2pCtx->StreamContext);

    ExFreeToNPagedLookasideList( &volumeContext->Pre2PostContextList,
                                 p2pCtx );

    FltReleaseContext( volumeContext );

    return retValue;
}

FLT_PREOP_CALLBACK_STATUS
FltPreQueryInformation (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
	FLT_PREOP_CALLBACK_STATUS 	retValue = FLT_PREOP_SYNCHRONIZE;
	NTSTATUS status = STATUS_SUCCESS;
	PSTREAM_CONTEXT streamContext = NULL;
	BOOLEAN streamContextCreated = FALSE;
	FILE_INFORMATION_CLASS fileInformationClass = Data->Iopb->Parameters.QueryFileInformation.FileInformationClass ;	
	
	__try{

              if((fileInformationClass == FileAllInformation) || 
			(fileInformationClass == FileAllocationInformation) ||
			(fileInformationClass == FileEndOfFileInformation) ||
			(fileInformationClass == FileStandardInformation) ||
			(fileInformationClass == FilePositionInformation) ||
			(fileInformationClass == FileValidDataLengthInformation) ||
			(fileInformationClass == FileNetworkOpenInformation)){
			
			if(FLT_IS_FASTIO_OPERATION(Data)) {

				retValue = FLT_PREOP_DISALLOW_FASTIO;

				__leave;
			}
			
		}else{

			retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

                     __leave;
		}

		status = CtxFindOrCreateStreamContext(Data, FALSE, &streamContext, &streamContextCreated) ;
		
		if (!NT_SUCCESS(status)){
			
			retValue = FLT_PREOP_SUCCESS_NO_CALLBACK ;
			__leave ;
		}

		FsAcquireResourceShared(streamContext->Resource);
		
		if (streamContext->CryptedFile == FALSE)	{

			FsReleaseResource(streamContext->Resource);
			
			retValue = FLT_PREOP_SUCCESS_NO_CALLBACK ;
			__leave ;
		}	

		FsReleaseResource(streamContext->Resource);
		
	}__finally{

		if(streamContext != NULL) {

			FltReleaseContext(streamContext);
		}
	}
	
	return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
FltPostQueryInformation (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
	FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
	NTSTATUS status = STATUS_SUCCESS;
	PSTREAM_CONTEXT streamContext = NULL;
	BOOLEAN streamContextCreated = FALSE;
	FILE_INFORMATION_CLASS fileInformationClass = Data->Iopb->Parameters.QueryFileInformation.FileInformationClass ;
	PVOID fileInformationBuffer = Data->Iopb->Parameters.QueryFileInformation.InfoBuffer ;
	ULONG fileInformationBufferLength = Data->IoStatus.Information ;
	ULONG removeLen = 0;


	__try{

		status = CtxFindOrCreateStreamContext(Data, FALSE, &streamContext, &streamContextCreated) ;
		
		if (!NT_SUCCESS(status)){

			__leave;
		}

              FsAcquireResourceShared(streamContext->Resource);

		removeLen = streamContext->CryptContext.MetadataSize /* + padding len */;
              
		switch (fileInformationClass){
			
		case FileAllInformation:	
		{			
			PFILE_ALL_INFORMATION fileAllInformation = (PFILE_ALL_INFORMATION)fileInformationBuffer ;

			if (fileInformationBufferLength >= (sizeof(FILE_BASIC_INFORMATION) + sizeof(FILE_STANDARD_INFORMATION))) {

				if(fileAllInformation->StandardInformation.AllocationSize.QuadPart >= streamContext->CryptContext.MetadataSize) {
					
					fileAllInformation->StandardInformation.AllocationSize.QuadPart -= removeLen;
				}
				if(fileAllInformation->StandardInformation.EndOfFile.QuadPart >= streamContext->CryptContext.MetadataSize){

					fileAllInformation->StandardInformation.EndOfFile.QuadPart -= removeLen;
				}

				if(fileInformationBufferLength >= (sizeof(FILE_BASIC_INFORMATION) + 
										sizeof(FILE_STANDARD_INFORMATION) +
										sizeof(FILE_INTERNAL_INFORMATION) +
										sizeof(FILE_EA_INFORMATION) +
										sizeof(FILE_ACCESS_INFORMATION) +
										sizeof(FILE_POSITION_INFORMATION))) {

					if(fileAllInformation->PositionInformation.CurrentByteOffset.QuadPart >= streamContext->CryptContext.MetadataSize) {

						fileAllInformation->PositionInformation.CurrentByteOffset.QuadPart -= removeLen;
					}
				}
			}			
		}
			break;
		case FileAllocationInformation:
		{
			PFILE_ALLOCATION_INFORMATION fileAllocationInformation = (PFILE_ALLOCATION_INFORMATION)fileInformationBuffer;

			if(fileAllocationInformation->AllocationSize.QuadPart >= streamContext->CryptContext.MetadataSize) {
				
				fileAllocationInformation->AllocationSize.QuadPart -= removeLen;
			}
		}
			break;
		case FileValidDataLengthInformation:
		{
			PFILE_VALID_DATA_LENGTH_INFORMATION fileValidDataLengthInformation = (PFILE_VALID_DATA_LENGTH_INFORMATION)fileInformationBuffer ;

			if(fileValidDataLengthInformation->ValidDataLength.QuadPart >= streamContext->CryptContext.MetadataSize ){

				fileValidDataLengthInformation->ValidDataLength.QuadPart -= removeLen;
			}
		}
			break;
		case FileStandardInformation:
		{
			PFILE_STANDARD_INFORMATION fileStandardInformation = (PFILE_STANDARD_INFORMATION)fileInformationBuffer ;
			
			if(fileStandardInformation->AllocationSize.QuadPart >= streamContext->CryptContext.MetadataSize) {

				fileStandardInformation->AllocationSize.QuadPart -= removeLen;
			}
			if(fileStandardInformation->EndOfFile.QuadPart >= streamContext->CryptContext.MetadataSize) {

				fileStandardInformation->EndOfFile.QuadPart -= removeLen;
			}			
		}
			break;
		case FileEndOfFileInformation:
		{
			PFILE_END_OF_FILE_INFORMATION fileEndOfFileInformation = (PFILE_END_OF_FILE_INFORMATION)fileInformationBuffer ;

			if(fileEndOfFileInformation->EndOfFile.QuadPart >= streamContext->CryptContext.MetadataSize) {
				
				fileEndOfFileInformation->EndOfFile.QuadPart -= removeLen;
			}
		}
			break;
		case FilePositionInformation:
		{
			PFILE_POSITION_INFORMATION filePositionInformation = (PFILE_POSITION_INFORMATION)fileInformationBuffer ;

			if(filePositionInformation->CurrentByteOffset.QuadPart >= streamContext->CryptContext.MetadataSize) {
				
				filePositionInformation->CurrentByteOffset.QuadPart -= removeLen;
			}			
		}
			break;
		case FileNetworkOpenInformation:
		{
			PFILE_NETWORK_OPEN_INFORMATION fileNetworkOpenInformation = (PFILE_NETWORK_OPEN_INFORMATION) fileInformationBuffer;

			if(fileNetworkOpenInformation->AllocationSize.QuadPart >= streamContext->CryptContext.MetadataSize){

				fileNetworkOpenInformation->AllocationSize.QuadPart -= removeLen;
			}

			if(fileNetworkOpenInformation->EndOfFile.QuadPart >= streamContext->CryptContext.MetadataSize){

				fileNetworkOpenInformation->EndOfFile.QuadPart -= removeLen;
			}
		}
			break;
		default:
			break;
		}		

	     FltSetCallbackDataDirty(Data);
		 
            FsReleaseResource(streamContext->Resource);
	
	}__finally{

		if(streamContext != NULL) {

			FltReleaseContext(streamContext);
		}
	}	

FltPostQueryInformationCleanup:
	
	return retValue;
}


FLT_PREOP_CALLBACK_STATUS
FltPreSetInformation(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{	
	FLT_PREOP_CALLBACK_STATUS 	retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
	PSTREAM_CONTEXT 			streamContext = NULL;
	BOOLEAN  					streamContextCreated = FALSE;
	FILE_INFORMATION_CLASS 		fileInformationClass = Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
	PVOID 						fileInformationBuffer = Data->Iopb->Parameters.SetFileInformation.InfoBuffer ;
	NTSTATUS 					status = STATUS_SUCCESS;

	__try{

	     if(FltGetTaskStateInPreCreate(Data)  == FLT_TASK_STATE_SYSTEM) {

			__leave;
	     }
		 
            if ((fileInformationClass == FileAllInformation) || 
			(fileInformationClass == FileAllocationInformation) ||
			(fileInformationClass == FileEndOfFileInformation) ||
			(fileInformationClass == FileStandardInformation) ||
			(fileInformationClass == FilePositionInformation) ||
			(fileInformationClass == FileValidDataLengthInformation)){
						
		}
		else	{
			
			__leave ;
		}	     
			 
             status = CtxFindOrCreateStreamContext(Data, FALSE, &streamContext, &streamContextCreated) ;
		
		if (!NT_SUCCESS(status)) {
			
			__leave ;
		}		

              FsAcquireResourceExclusive(streamContext->Resource);

              if(streamContext->CryptedFile == FALSE) {

                    FsReleaseResource(streamContext->Resource);
					
			__leave ;
		}
		
		switch(fileInformationClass)
		{
		case FileAllInformation:
			{
				PFILE_ALL_INFORMATION fileAllInformation = (PFILE_ALL_INFORMATION)fileInformationBuffer ;
                            
				fileAllInformation->PositionInformation.CurrentByteOffset.QuadPart += streamContext->CryptContext.MetadataSize;
				fileAllInformation->StandardInformation.EndOfFile.QuadPart += streamContext->CryptContext.MetadataSize;

                            streamContext->FileSize = fileAllInformation->StandardInformation.EndOfFile;
			}
			break;
		case FileAllocationInformation:
			{
				PFILE_ALLOCATION_INFORMATION fileAllocationInformation = (PFILE_ALLOCATION_INFORMATION)fileInformationBuffer ;	

				fileAllocationInformation->AllocationSize.QuadPart += streamContext->CryptContext.MetadataSize;  
			} 
			break;
		case FileEndOfFileInformation:
			{
				PFILE_END_OF_FILE_INFORMATION fileEndOfFileInformation = (PFILE_END_OF_FILE_INFORMATION)fileInformationBuffer ;

				DbgPrint("PreSetInforamtion: pid = %d, %lld, %lld\n", PsGetCurrentProcessId(), streamContext->FileSize.QuadPart, fileEndOfFileInformation->EndOfFile.QuadPart);
				
				fileEndOfFileInformation->EndOfFile.QuadPart += streamContext->CryptContext.MetadataSize;
                            
                            streamContext->FileSize = fileEndOfFileInformation->EndOfFile;

				DbgPrint("PreSetInforamtion(1): pid = %d, %lld, %lld\n", PsGetCurrentProcessId(), streamContext->FileSize.QuadPart, fileEndOfFileInformation->EndOfFile.QuadPart);
			}
			break;
		case FileStandardInformation:
			{
				PFILE_STANDARD_INFORMATION fileStandardInforamtion = (PFILE_STANDARD_INFORMATION)fileInformationBuffer ;

				fileStandardInforamtion->EndOfFile.QuadPart += streamContext->CryptContext.MetadataSize;

                            streamContext->FileSize = fileStandardInforamtion->EndOfFile;
			}
			break;
		case FilePositionInformation:
			{
				PFILE_POSITION_INFORMATION filePositionInformation = (PFILE_POSITION_INFORMATION)fileInformationBuffer ;
                            
				filePositionInformation->CurrentByteOffset.QuadPart += streamContext->CryptContext.MetadataSize;
			}
			break;
		case FileValidDataLengthInformation:
			{
				PFILE_VALID_DATA_LENGTH_INFORMATION fileValidDataLengthInformation = (PFILE_VALID_DATA_LENGTH_INFORMATION)fileInformationBuffer ;

				fileValidDataLengthInformation->ValidDataLength.QuadPart += streamContext->CryptContext.MetadataSize;				
			}
			break;
		default:
			break;
		};

              FltSetCallbackDataDirty(Data);

              FsReleaseResource(streamContext->Resource);
	
	}__finally{

		if(streamContext != NULL) {

			FltReleaseContext(streamContext);
		}
	}
	
FltPreSetInformationCleanup:

	return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
FltPostSetInformation (
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
	FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
    		
	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
FltPreNetworkQueryOpen(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext,
    __in PVOLUME_CONTEXT VolumeContext
    )
{
	FLT_PREOP_CALLBACK_STATUS 	retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

#if 0
	PFLT_FILE_NAME_INFORMATION 	nameInfo = NULL;
	NTSTATUS status = STATUS_SUCCESS;
	CRYPT_CONTEXT cryptContext ;
	BOOLEAN isEmptyFile = FALSE;

	status = FltGetFileNameInformation( Data,
	                                        FLT_FILE_NAME_NORMALIZED |
	                                        FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
	                                        &nameInfo );
	
       if (!NT_SUCCESS( status )) {

            goto FltPreNetworkQueryOpenCleanup;
       }

       status = FltParseFileNameInformation( nameInfo );
	  
       if (!NT_SUCCESS( status )) {
        
	 	goto FltPreNetworkQueryOpenCleanup;
       }

	SifsInitializeCryptContext(&cryptContext);
	
	if(SifsQuickCheckValidate(Data->Iopb->TargetInstance, &nameInfo->Name, &cryptContext, &isEmptyFile, VolumeContext->SectorSize) == 0){

		retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}
	
FltPreNetworkQueryOpenCleanup:
	
	if(retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK) {
		
		if(FLT_IS_FASTIO_OPERATION(Data)) {

			retValue = FLT_PREOP_DISALLOW_FASTIO;
		}
	}
#endif

	if(FLT_IS_FASTIO_OPERATION(Data)) {

		retValue = FLT_PREOP_DISALLOW_FASTIO;
	}
	
	return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
FltPostNetworkQueryOpen (    
    __inout PFLT_CALLBACK_DATA Cbd,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __inout_opt PVOID CbdContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
	FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
	PFILE_NETWORK_OPEN_INFORMATION fileNetworkOpenInformation = NULL;
	CRYPT_CONTEXT cryptContext ;

	__try{

		if(!NT_SUCCESS(Cbd->IoStatus.Status)) {

			__leave;
		}

		SifsInitializeCryptContext(&cryptContext);
		
		fileNetworkOpenInformation = Cbd->Iopb->Parameters.NetworkQueryOpen.NetworkInformation;

		if(fileNetworkOpenInformation != NULL) {

			BOOLEAN modified = FALSE;

			if(fileNetworkOpenInformation->EndOfFile.QuadPart >= cryptContext.MetadataSize){

				fileNetworkOpenInformation->EndOfFile.QuadPart -= cryptContext.MetadataSize;

				modified = TRUE;
			}

			if(fileNetworkOpenInformation->AllocationSize.QuadPart >= cryptContext.MetadataSize){

				fileNetworkOpenInformation->AllocationSize.QuadPart -= cryptContext.MetadataSize;

				modified = TRUE;
			}

			if(modified == TRUE) {

				FltSetCallbackDataDirty(Cbd);
			}			
		}

	}__finally{

	}	
	
	return retValue;
}

#define 	FILE_NAME_MAX_LENGTH 128

int
FltCheckValidateSifs(
       __in PFLT_INSTANCE    Instance,
	__in PWCHAR PathName,
	__in ULONG	PathNameLength,
	__in PWCHAR FileName,
	__in ULONG  FileNameLength,
	__inout PUCHAR PageVirt,
	__in    LONG PageVirtLen
	)
{
	int rc = -1;

	if((FileNameLength * sizeof(WCHAR)) <= FILE_NAME_MAX_LENGTH){

		UNICODE_STRING fileName;
		NTSTATUS 		status = STATUS_SUCCESS;
		HANDLE			fileHandle = NULL;
		PFILE_OBJECT		fileObject = NULL;
		
		RtlCopyMemory(PathName + PathNameLength, FileName, FileNameLength * sizeof(WCHAR));		
		PathName[PathNameLength + FileNameLength] = L'\0';

		RtlInitUnicodeString(&fileName, PathName);

		status = FsOpenFile(Instance, &fileName, &fileHandle, &fileObject, NULL);

		if(NT_SUCCESS(status)) {

			if(FsCheckFileIsDirectoryByObject(Instance, fileObject) == FALSE) {
				
				rc = SifsQuickCheckValidateSifs(Instance, fileObject, PageVirt, PageVirtLen);
			}

			FsCloseFile(fileHandle, fileObject);
		}		

		LOG_PRINT(LOGFL_DIRCTRL, ("FileFlt!FltCheckFileIsSifs:    Pid = %d, FileName = %wZ, rc = %d\n", PsGetCurrentProcessId(), &fileName, rc));
	}


	return rc;
}

VOID
FltCheckDirectoryValue(
	__inout PFLT_CALLBACK_DATA Data,
    	__in PCFLT_RELATED_OBJECTS FltObjects,
    	__inout PPRE_2_POST_CONTEXT P2pCtx
    	)
{
	FILE_INFORMATION_CLASS  fileInformationClass = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;
	PWCHAR pathName = NULL;
	ULONG pathNameLength = 0;
	PUCHAR sifsHeadMetadata = NULL;
	LONG   sifsHeadMetadataLen = 0;

	__try {

		if((fileInformationClass == FileBothDirectoryInformation)
			|| (fileInformationClass == FileDirectoryInformation)
			|| (fileInformationClass == FileFullDirectoryInformation)
			|| (fileInformationClass == FileIdBothDirectoryInformation)
			|| (fileInformationClass == FileIdFullDirectoryInformation)){

			pathNameLength = P2pCtx->NameInfo->Name.Length / sizeof(WCHAR);

			pathName = ExAllocatePoolWithTag(NonPagedPool, P2pCtx->NameInfo->Name.Length + FILE_NAME_MAX_LENGTH + 2 + 2, NAME_TAG);

			if(pathName == NULL){

				__leave;
			}		

			sifsHeadMetadataLen = ROUND_TO_SIZE(SIFS_CHECK_FILE_VALID_MINIMUN_SIZE, P2pCtx->VolCtx->SectorSize);
			sifsHeadMetadata = ExAllocatePoolWithTag(NonPagedPool, sifsHeadMetadataLen, SIFS_METADATA_TAG);

			if(sifsHeadMetadata == NULL) {

				__leave;
			}

			RtlCopyMemory(pathName, P2pCtx->NameInfo->Name.Buffer, P2pCtx->NameInfo->Name.Length);
			pathName[pathNameLength] = L'\0';

			if(pathName[pathNameLength - 1] != L'\\'){

				pathName[pathNameLength] = L'\\';
				pathName[pathNameLength + 1] = L'\0';

				pathNameLength++;
			}

			
		}

		switch(fileInformationClass){

		case FileBothDirectoryInformation:
			{
				PFILE_BOTH_DIR_INFORMATION fileBothDirInformation = (PFILE_BOTH_DIR_INFORMATION)P2pCtx->SwappedBuffer; 

				do{

					if(FltCheckValidateSifs(Data->Iopb->TargetInstance, pathName, pathNameLength,fileBothDirInformation->FileName
						, fileBothDirInformation->FileNameLength /sizeof(WCHAR), sifsHeadMetadata, sifsHeadMetadataLen) == 0) {

						if(fileBothDirInformation->EndOfFile.QuadPart >= SIFS_MINIMUM_HEADER_EXTENT_SIZE) {

							fileBothDirInformation->EndOfFile.QuadPart -= SIFS_MINIMUM_HEADER_EXTENT_SIZE;
						}

						if(fileBothDirInformation->AllocationSize.QuadPart >= SIFS_MINIMUM_HEADER_EXTENT_SIZE) {

							fileBothDirInformation->AllocationSize.QuadPart -= SIFS_MINIMUM_HEADER_EXTENT_SIZE;
						}
					}
					
					if (!fileBothDirInformation->NextEntryOffset) {
						
                                		break;
					}
					
                            	fileBothDirInformation = (PFILE_BOTH_DIR_INFORMATION)((PCHAR)fileBothDirInformation + fileBothDirInformation->NextEntryOffset);
					
				}while(1);	
				
			}
			break;
		case FileDirectoryInformation:
			{
				PFILE_DIRECTORY_INFORMATION fileDirectoryInformation = (PFILE_DIRECTORY_INFORMATION)P2pCtx->SwappedBuffer; 

				do{

					if(FltCheckValidateSifs(Data->Iopb->TargetInstance, pathName, pathNameLength, fileDirectoryInformation->FileName
						, fileDirectoryInformation->FileNameLength /sizeof(WCHAR), sifsHeadMetadata, sifsHeadMetadataLen) == 0) {

						if(fileDirectoryInformation->EndOfFile.QuadPart >= SIFS_MINIMUM_HEADER_EXTENT_SIZE) {

							fileDirectoryInformation->EndOfFile.QuadPart -= SIFS_MINIMUM_HEADER_EXTENT_SIZE;
						}

						if(fileDirectoryInformation->AllocationSize.QuadPart >= SIFS_MINIMUM_HEADER_EXTENT_SIZE) {

							fileDirectoryInformation->AllocationSize.QuadPart -= SIFS_MINIMUM_HEADER_EXTENT_SIZE;
						}
					}
					
					if (!fileDirectoryInformation->NextEntryOffset) {
						
                                		break;
					}
					
                            	fileDirectoryInformation = (PFILE_DIRECTORY_INFORMATION)((PCHAR)fileDirectoryInformation + fileDirectoryInformation->NextEntryOffset);
					
				}while(1);				
			}
			break;
		case FileFullDirectoryInformation:
			{
				PFILE_FULL_DIR_INFORMATION fileFullDirInformation = (PFILE_FULL_DIR_INFORMATION)P2pCtx->SwappedBuffer; 

				do{

					if(FltCheckValidateSifs(Data->Iopb->TargetInstance, pathName, pathNameLength, fileFullDirInformation->FileName
						, fileFullDirInformation->FileNameLength /sizeof(WCHAR), sifsHeadMetadata, sifsHeadMetadataLen) == 0) {

						if(fileFullDirInformation->EndOfFile.QuadPart >= SIFS_MINIMUM_HEADER_EXTENT_SIZE) {

							fileFullDirInformation->EndOfFile.QuadPart -= SIFS_MINIMUM_HEADER_EXTENT_SIZE;
						}

						if(fileFullDirInformation->AllocationSize.QuadPart >= SIFS_MINIMUM_HEADER_EXTENT_SIZE) {

							fileFullDirInformation->AllocationSize.QuadPart -= SIFS_MINIMUM_HEADER_EXTENT_SIZE;
						}
					}
					
					if (!fileFullDirInformation->NextEntryOffset) {
						
                                		break;
					}
					
                            	fileFullDirInformation = (PFILE_FULL_DIR_INFORMATION)((PCHAR)fileFullDirInformation + fileFullDirInformation->NextEntryOffset);
					
				}while(1);				
			}
			break;
		case FileIdBothDirectoryInformation:
			{
				PFILE_ID_BOTH_DIR_INFORMATION fileIdBothDirInformation = (PFILE_ID_BOTH_DIR_INFORMATION)P2pCtx->SwappedBuffer; 

				do{

					if(FltCheckValidateSifs(Data->Iopb->TargetInstance, pathName, pathNameLength, fileIdBothDirInformation->FileName
						, fileIdBothDirInformation->FileNameLength /sizeof(WCHAR), sifsHeadMetadata, sifsHeadMetadataLen) == 0) {

						if(fileIdBothDirInformation->EndOfFile.QuadPart >= SIFS_MINIMUM_HEADER_EXTENT_SIZE) {

							fileIdBothDirInformation->EndOfFile.QuadPart -= SIFS_MINIMUM_HEADER_EXTENT_SIZE;
						}

						if(fileIdBothDirInformation->AllocationSize.QuadPart >= SIFS_MINIMUM_HEADER_EXTENT_SIZE) {

							fileIdBothDirInformation->AllocationSize.QuadPart -= SIFS_MINIMUM_HEADER_EXTENT_SIZE;
						}
					}
					
					if (!fileIdBothDirInformation->NextEntryOffset) {
						
                                		break;
					}
					
                            	fileIdBothDirInformation = (PFILE_ID_BOTH_DIR_INFORMATION)((PCHAR)fileIdBothDirInformation + fileIdBothDirInformation->NextEntryOffset);
					
				}while(1);				
			}
			break;
		case FileIdFullDirectoryInformation:
			{
				PFILE_ID_FULL_DIR_INFORMATION  fileIdFullDirInformation = (PFILE_ID_FULL_DIR_INFORMATION )P2pCtx->SwappedBuffer; 

				do{

					if(FltCheckValidateSifs(Data->Iopb->TargetInstance, pathName, pathNameLength, fileIdFullDirInformation->FileName
						, fileIdFullDirInformation->FileNameLength /sizeof(WCHAR), sifsHeadMetadata, sifsHeadMetadataLen) == 0) {

						if(fileIdFullDirInformation->EndOfFile.QuadPart >= SIFS_MINIMUM_HEADER_EXTENT_SIZE) {

							fileIdFullDirInformation->EndOfFile.QuadPart -= SIFS_MINIMUM_HEADER_EXTENT_SIZE;
						}

						if(fileIdFullDirInformation->AllocationSize.QuadPart >= SIFS_MINIMUM_HEADER_EXTENT_SIZE) {

							fileIdFullDirInformation->AllocationSize.QuadPart -= SIFS_MINIMUM_HEADER_EXTENT_SIZE;
						}
					}
					
					if (!fileIdFullDirInformation->NextEntryOffset) {
						
                                		break;
					}
					
                            	fileIdFullDirInformation = (PFILE_ID_FULL_DIR_INFORMATION )((PCHAR)fileIdFullDirInformation + fileIdFullDirInformation->NextEntryOffset);
					
				}while(1);				
			}
			break;
			
		}
	}__finally{

		if(pathName != NULL){

			ExFreePoolWithTag(pathName, NAME_TAG);
		}

		if(sifsHeadMetadata != NULL) {

			ExFreePoolWithTag(sifsHeadMetadata, SIFS_METADATA_TAG);
		}
	}
}
    	
FLT_PREOP_CALLBACK_STATUS
FltPreDirCtrlBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext,
    __in PVOLUME_CONTEXT VolumeContext
    )
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    PVOID newBuf = NULL;
    PMDL newMdl = NULL;
    PPRE_2_POST_CONTEXT p2pCtx = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    PFLT_FILE_NAME_INFORMATION 	nameInfo = NULL;
    FILE_INFORMATION_CLASS  fileInformationClass = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;

    try {        

	if((VolumeContext->FileSystemType == FLT_FSTYPE_LANMAN)
		&& (FltGetTaskStateInPreCreate(Data) != FLT_TASK_STATE_EXPLORE_HOOK)){

		__leave;
	}
	
	if(!((fileInformationClass == FileBothDirectoryInformation)
			|| (fileInformationClass == FileDirectoryInformation)
			|| (fileInformationClass == FileFullDirectoryInformation)
			|| (fileInformationClass == FileIdBothDirectoryInformation)
			|| (fileInformationClass == FileIdFullDirectoryInformation))){

		__leave;
	}
	
	 status = FltGetFileNameInformation( Data,
	                                        FLT_FILE_NAME_NORMALIZED |
	                                        FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
	                                        &nameInfo );

	 if(!NT_SUCCESS(status)) {

		LOG_PRINT( LOGFL_ERRORS,
                       ("FileFlt!FltPreDirCtrlBuffers:          %wZ Failed to get directory name.\n",
                        &VolumeContext->VolumeName));

            	leave;
	 }

        //
        //  Allocate nonPaged memory for the buffer we are swapping to.
        //  If we fail to get the memory, just don't swap buffers on this
        //  operation.
        //

        newBuf = ExAllocatePoolWithTag( NonPagedPool,
                                        iopb->Parameters.DirectoryControl.QueryDirectory.Length,
                                        BUFFER_SWAP_TAG );

        if (newBuf == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("FileFlt!FltPreDirCtrlBuffers:          %wZ Failed to allocate %d bytes of memory.\n",
                        &VolumeContext->VolumeName,
                        iopb->Parameters.DirectoryControl.QueryDirectory.Length) );

            leave;
        }

        //
        //  We need to build a MDL because Directory Control Operations are always IRP operations.  
        //


        //
        //  Allocate a MDL for the new allocated memory.  If we fail
        //  the MDL allocation then we won't swap buffer for this operation
        //

        newMdl = IoAllocateMdl( newBuf,
                                iopb->Parameters.DirectoryControl.QueryDirectory.Length,
                                FALSE,
                                FALSE,
                                NULL );

        if (newMdl == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("FileFlt!FltPreDirCtrlBuffers:          %wZ Failed to allocate MDL.\n",
                        &VolumeContext->VolumeName) );

           leave;
        }

        //
        //  setup the MDL for the non-paged pool we just allocated
        //

        MmBuildMdlForNonPagedPool( newMdl );

        //
        //  We are ready to swap buffers, get a pre2Post context structure.
        //  We need it to pass the volume context and the allocate memory
        //  buffer to the post operation callback.
        //

        p2pCtx = ExAllocateFromNPagedLookasideList( &VolumeContext->Pre2PostContextList );

        if (p2pCtx == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("FileFlt!FltPreDirCtrlBuffers:          %wZ Failed to allocate pre2Post context structure\n",
                        &VolumeContext->VolumeName) );

            leave;
        }

        //
        //  Log that we are swapping
        //

        LOG_PRINT( LOGFL_DIRCTRL,
                   ("FileFlt!FltPreDirCtrlBuffers:          %wZ newB=%p newMdl=%p oldB=%p oldMdl=%p len=%d\n",
                    &VolumeContext->VolumeName,
                    newBuf,
                    newMdl,
                    iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer,
                    iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
                    iopb->Parameters.DirectoryControl.QueryDirectory.Length) );

        //
        //  Update the buffer pointers and MDL address
        //

        iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer = newBuf;
        iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress = newMdl;
        FltSetCallbackDataDirty( Data );

        //
        //  Pass state to our post-operation callback.
        //

        FltReferenceContext(VolumeContext);
		
        p2pCtx->SwappedBuffer = newBuf;
        p2pCtx->VolCtx = VolumeContext;
	 p2pCtx->NameInfo = nameInfo;

        *CompletionContext = p2pCtx;

	newBuf = NULL;
	newMdl = NULL;
	nameInfo = NULL;

        //
        //  Return we want a post-operation callback
        //

        //retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
        retValue = FLT_PREOP_SYNCHRONIZE;

    } finally {

        //
        //  If we don't want a post-operation callback, then cleanup state.
        //

            if (newBuf != NULL) {

                ExFreePool( newBuf );
            }

            if (newMdl != NULL) {

                IoFreeMdl( newMdl );
            } 

	     if(nameInfo != NULL) {

		 FltReleaseFileNameInformation(nameInfo);
	     }
    }

    return retValue;

}

FLT_POSTOP_CALLBACK_STATUS
FltPostDirCtrlBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    PVOID origBuf = NULL;
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    BOOLEAN cleanupAllocatedBuffer = TRUE;


    //
    //  This system won't draining an operation with swapped buffers, verify
    //  the draining flag is not set.
    //

    ASSERT(!FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING));
    
    try {

        //
        //  If the operation failed or the count is zero, there is no data to
        //  copy so just return now.
        //

        if (!NT_SUCCESS(Data->IoStatus.Status) ||
            (Data->IoStatus.Information == 0)) {

            LOG_PRINT( LOGFL_DIRCTRL,
                       ("FileFlt!FltPostDirCtrlBuffers:         %wZ newB=%p No data read, status=%x, info=%x\n",
                        &p2pCtx->VolCtx->VolumeName,
                        p2pCtx->SwappedBuffer,
                        Data->IoStatus.Status,
                        Data->IoStatus.Information) );

            leave;
        }

        //
        //  We need to copy the read data back into the users buffer.  Note
        //  that the parameters passed in are for the users original buffers
        //  not our swapped buffers
        //

        if (iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress != NULL) {

            //
            //  There is a MDL defined for the original buffer, get a
            //  system address for it so we can copy the data back to it.
            //  We must do this because we don't know what thread context
            //  we are in.
            //

            origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
                                                    NormalPagePriority );

            if (origBuf == NULL) {

                LOG_PRINT( LOGFL_ERRORS,
                           ("FileFlt!FltPostDirCtrlBuffers:         %wZ Failed to get system address for MDL: %p\n",
                            &p2pCtx->VolCtx->VolumeName,
                            iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress) );

                //
                //  If we failed to get a SYSTEM address, mark that the
                //  operation failed and return.
                //

                Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                Data->IoStatus.Information = 0;
                leave;
            }

        } else if (FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) ||
                   FlagOn(Data->Flags,FLTFL_CALLBACK_DATA_FAST_IO_OPERATION)) {

            //
            //  If this is a system buffer, just use the given address because
            //      it is valid in all thread contexts.
            //  If this is a FASTIO operation, we can just use the
            //      buffer (inside a try/except) since we know we are in
            //      the correct thread context.
            //

            origBuf = iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;

        } else {

            //
            //  They don't have a MDL and this is not a system buffer
            //  or a fastio so this is probably some arbitrary user
            //  buffer.  We can not do the processing at DPC level so
            //  try and get to a safe IRQL so we can do the processing.
            //

            if (FltDoCompletionProcessingWhenSafe( Data,
                                                   FltObjects,
                                                   CompletionContext,
                                                   Flags,
                                                   FltPostDirCtrlBuffersWhenSafe,
                                                   &retValue )) {

                //
                //  This operation has been moved to a safe IRQL, the called
                //  routine will do (or has done) the freeing so don't do it
                //  in our routine.
                //

                cleanupAllocatedBuffer = FALSE;

            } else {

                //
                //  We are in a state where we can not get to a safe IRQL and
                //  we do not have a MDL.  There is nothing we can do to safely
                //  copy the data back to the users buffer, fail the operation
                //  and return.  This shouldn't ever happen because in those
                //  situations where it is not safe to post, we should have
                //  a MDL.
                //

                LOG_PRINT( LOGFL_ERRORS,
                           ("FileFlt!FltPostDirCtrlBuffers:         %wZ Unable to post to a safe IRQL\n",
                            &p2pCtx->VolCtx->VolumeName) );

                Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
                Data->IoStatus.Information = 0;
            }

            leave;
        }

        //
        //  We either have a system buffer or this is a fastio operation
        //  so we are in the proper context.  Copy the data handling an
        //  exception.
        //
        //  NOTE:  Due to a bug in FASTFAT where it is returning the wrong
        //         length in the information field (it is sort) we are always
        //         going to copy the original buffer length.
        //

        try {     

	     FltCheckDirectoryValue(Data, FltObjects, p2pCtx);
		 
            RtlCopyMemory( origBuf,
                           p2pCtx->SwappedBuffer,
                           /*Data->IoStatus.Information*/
                           iopb->Parameters.DirectoryControl.QueryDirectory.Length );	     

        } except (EXCEPTION_EXECUTE_HANDLER) {

            Data->IoStatus.Status = GetExceptionCode();
            Data->IoStatus.Information = 0;

            LOG_PRINT( LOGFL_ERRORS,
                       ("FileFlt!FltPostDirCtrlBuffers:         %wZ Invalid user buffer, oldB=%p, status=%x, info=%x\n",
                        &p2pCtx->VolCtx->VolumeName,
                        origBuf,
                        Data->IoStatus.Status,
                        Data->IoStatus.Information) );
        }

    } finally {

        //
        //  If we are supposed to, cleanup the allocate memory and release
        //  the volume context.  The freeing of the MDL (if there is one) is
        //  handled by FltMgr.
        //

        if (cleanupAllocatedBuffer) {

	     PVOLUME_CONTEXT volumeContext  = p2pCtx->VolCtx;
		 
            LOG_PRINT( LOGFL_DIRCTRL,
                       ("FileFlt!FltPostDirCtrlBuffers:         %wZ newB=%p info=%d Freeing\n",
                        &p2pCtx->VolCtx->VolumeName,
                        p2pCtx->SwappedBuffer,
                        Data->IoStatus.Information) );

	     if(p2pCtx->NameInfo != NULL) {

			FltReleaseFileNameInformation(p2pCtx->NameInfo);
	     }

            ExFreePool( p2pCtx->SwappedBuffer );

            ExFreeToNPagedLookasideList( &volumeContext->Pre2PostContextList,
                                         p2pCtx );
			
	     FltReleaseContext( volumeContext );
        }
    }

    return retValue;
}

FLT_POSTOP_CALLBACK_STATUS
FltPostDirCtrlBuffersWhenSafe (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    )
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
    PPRE_2_POST_CONTEXT p2pCtx = CompletionContext;
    PVOID origBuf = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    PVOLUME_CONTEXT volumeContext = NULL;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    ASSERT(Data->IoStatus.Information != 0);

    //
    //  This is some sort of user buffer without a MDL, lock the
    //  user buffer so we can access it
    //

    status = FltLockUserBuffer( Data );

    if (!NT_SUCCESS(status)) {

        LOG_PRINT( LOGFL_ERRORS,
                   ("FileFlt!FltPostDirCtrlBuffersWhenSafe: %wZ Could not lock user buffer, oldB=%p, status=%x\n",
                    &p2pCtx->VolCtx->VolumeName,
                    iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer,
                    status) );

        //
        //  If we can't lock the buffer, fail the operation
        //

        Data->IoStatus.Status = status;
        Data->IoStatus.Information = 0;

    } else {

        //
        //  Get a system address for this buffer.
        //

        origBuf = MmGetSystemAddressForMdlSafe( iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
                                                NormalPagePriority );

        if (origBuf == NULL) {

            LOG_PRINT( LOGFL_ERRORS,
                       ("FileFlt!FltPostDirCtrlBuffersWhenSafe: %wZ Failed to get System address for MDL: %p\n",
                        &p2pCtx->VolCtx->VolumeName,
                        iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress) );

            //
            //  If we couldn't get a SYSTEM buffer address, fail the operation
            //

            Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Data->IoStatus.Information = 0;

        } else {

	     FltCheckDirectoryValue(Data, FltObjects, p2pCtx);
		 
            //
            //  Copy the data back to the original buffer
            //
            //  NOTE:  Due to a bug in FASTFAT where it is returning the wrong
            //         length in the information field (it is short) we are
            //         always going to copy the original buffer length.
            //

            RtlCopyMemory( origBuf,
                           p2pCtx->SwappedBuffer,
                           /*Data->IoStatus.Information*/
                           iopb->Parameters.DirectoryControl.QueryDirectory.Length );
        }
    }

    //
    //  Free the memory we allocated and return
    //

    LOG_PRINT( LOGFL_DIRCTRL,
               ("FileFlt!FltPostDirCtrlBuffersWhenSafe: %wZ newB=%p info=%d Freeing\n",
                &p2pCtx->VolCtx->VolumeName,
                p2pCtx->SwappedBuffer,
                Data->IoStatus.Information) );

    volumeContext = p2pCtx->VolCtx;

    if(p2pCtx->NameInfo != NULL) {

		FltReleaseFileNameInformation(p2pCtx->NameInfo);
     }
	
    ExFreePool( p2pCtx->SwappedBuffer );
    ExFreeToNPagedLookasideList( &volumeContext->Pre2PostContextList,
                                 p2pCtx );

    FltReleaseContext( volumeContext );

    return FLT_POSTOP_FINISHED_PROCESSING;
}

