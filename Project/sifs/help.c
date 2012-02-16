#pragma warning(disable:4995)

#include "fileflt.h"
#include <ntifs.h>
#include <ntdddisk.h>
#include <ntstrsafe.h>

#ifdef ALLOC_PRAGMA
#endif

//----------------------------------------------------------------------------------------
//字符串相关

PCHAR 
FsLowerString( 
	__inout PCHAR Source, 
	__in ULONG Length
	)
{
 	ULONG   i = 0;
    
	for( ; i < Length; i++ ) {
		
		if( (Source[i] >= 'A' ) && (Source[i] <= 'Z' ) ){
			
			Source[i] = Source[i] + ('a' - 'A');
		}
	}

	return Source;
} 


int
FsCompareStringWithPatten(
	  __in PUNICODE_STRING FileName,
	  __in WCHAR *StringList[],
	  __in ULONG Count
	  )
{
	int ret = -1;
	ULONG  i = 0;

	for(; i < Count; i++) {
		
		if(NULL != FsWcsstrExtern(FileName->Buffer, FileName->Length / sizeof(WCHAR), FileName->MaximumLength / sizeof(WCHAR), StringList[i])){

			ret = 0;
			break;
		}
	}

	return ret;
}

int
FsReverseCompareString(
	  __in PUNICODE_STRING FileName,
	  __in WCHAR *StringList[],
	  __in ULONG Count
	  )
{
	int ret = -1;
	ULONG  i = 0;
	
	for(; i < Count; i++) {

		ULONG pattern_len = wcslen(StringList[i]);
		ULONG buffer_len = (FileName->Length / sizeof(WCHAR));

		if((buffer_len >= pattern_len)
				&& (_wcsnicmp(StringList[i], FileName->Buffer + (buffer_len - pattern_len), pattern_len) == 0)){

				ret = 0;
				break;
			}
	}

	return ret;
}

NTSTATUS
FsAllocateUnicodeString (
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
    String->Buffer = ExAllocatePoolWithTag( PagedPool,
                                            String->MaximumLength,
                                            NAME_TAG );

    if (String->Buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    String->Length = 0;

    return STATUS_SUCCESS;
}

VOID
FsFreeUnicodeString (
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
                       NAME_TAG );

    String->Length = String->MaximumLength = 0;
    String->Buffer = NULL;
}

static PWCHAR
FsWcsstrExtern_i(
    __in PWCHAR Source,
	__in LONG	SourceLength,
	__in PWCHAR Search,
	__in LONG   SearchLength
	)
{
	PWCHAR rc = NULL;

	LONG i = 0;
	LONG searchLength = SourceLength - SearchLength + 1;
	PWCHAR currentSource = Source;

	for( ; i < searchLength; i++){

		if(_wcsnicmp(currentSource, Search, SearchLength) == 0){

			rc = currentSource;
			break;
		}

		currentSource++;
	}

	return rc;
}

PWCHAR
FsWcsstrExtern(
    __in PWCHAR Source,
	__in LONG	SourceLength,
	__in LONG   SourceMaxLength,
	__in PWCHAR StrSearch
	)
{
	PWCHAR rc = NULL;
	
	if((SourceMaxLength > SourceLength) && (Source[SourceLength] == L'\0')){

		rc = wcsstr(Source, StrSearch);

	}else{

		LONG strSearchLength = wcslen(StrSearch);

		if(SourceLength < strSearchLength) {

			goto out;
		}

		rc = FsWcsstrExtern_i(Source, SourceLength, StrSearch, strSearchLength);
	}

out:
	return rc;
}

//-------------------------------------------------------------------------------------
//系统相关

VOID
FsGetCurrentVersion (
    )
/*++

Routine Description:

    This routine reads the current OS version using the correct routine based
    on what routine is available.

Arguments:

    None.
    
Return Value:

    None.

--*/
{
    if (NULL != g_FileFltContext.DynamicFunctions.GetVersion) {

        RTL_OSVERSIONINFOW versionInfo;
        NTSTATUS status;

        //
        //  VERSION NOTE: RtlGetVersion does a bit more than we need, but
        //  we are using it if it is available to show how to use it.  It
        //  is available on Windows XP and later.  RtlGetVersion and
        //  RtlVerifyVersionInfo (both documented in the IFS Kit docs) allow
        //  you to make correct choices when you need to change logic based
        //  on the current OS executing your code.
        //

        versionInfo.dwOSVersionInfoSize = sizeof( RTL_OSVERSIONINFOW );

        status = (g_FileFltContext.DynamicFunctions.GetVersion)( &versionInfo );

        ASSERT( NT_SUCCESS( status ) );

        g_FileFltContext.OsMajorVersion = versionInfo.dwMajorVersion;
        g_FileFltContext.OsMinorVersion = versionInfo.dwMinorVersion;
        
    } else {

        PsGetVersion( &g_FileFltContext.OsMajorVersion,
                      &g_FileFltContext.OsMinorVersion,
                      NULL,
                      NULL );
    }
}

BOOLEAN
FsReadDriverParameters(
	 __in HANDLE 				DriverRegKey,
	 __in PUNICODE_STRING 	ValueName,
	 __out UCHAR  			*Buffer,
	 __in  ULONG  			BufferLength
	 )
{
	BOOLEAN ret = FALSE;

	ULONG resultLength = 0;
	NTSTATUS status;   

	status = ZwQueryValueKey( DriverRegKey,
                                  ValueName,
                                  KeyValuePartialInformation,
                                  Buffer,
                                  BufferLength,
                                  &resultLength );

    if (NT_SUCCESS( status )) {

		ret = TRUE;
    } 

	return ret;
}


VOID
FsLoadDynamicFunctions (
    )
/*++

Routine Description:

    This routine tries to load the function pointers for the routines that
    are not supported on all versions of the OS.  These function pointers are
    then stored in the global structure SpyDynamicFunctions.

    This support allows for one driver to be built that will run on all 
    versions of the OS Windows 2000 and greater.  Note that on Windows 2000, 
    the functionality may be limited.
    
Arguments:

    None.
    
Return Value:

    None.

--*/
{
    UNICODE_STRING functionName;

    RtlZeroMemory( &g_FileFltContext.DynamicFunctions, sizeof( g_FileFltContext. DynamicFunctions) );

    //
    //  For each routine that we would want to use, lookup its address in the
    //  kernel or hal.  If it is not present, that field in our global
    //  SpyDynamicFunctions structure will be set to NULL.
    //    

    RtlInitUnicodeString( &functionName, L"RtlGetVersion" );
    g_FileFltContext.DynamicFunctions.GetVersion = MmGetSystemRoutineAddress( &functionName );

    RtlInitUnicodeString(&functionName, L"ZwQueryInformationProcess");
    g_FileFltContext.DynamicFunctions.QueryInformationProcess = MmGetSystemRoutineAddress( &functionName );
    
}

NTSTATUS
FsIsShadowCopyVolume (
    __in PDEVICE_OBJECT StorageStackDeviceObject,
    __out PBOOLEAN IsShadowCopy
    )
/*++

Routine Description:

    This routine will determine if the given volume is for a ShadowCopy volume
    or some other type of volume.

    VERSION NOTE:

    ShadowCopy volumes were introduced in Windows XP, therefore, if this
    driver is running on W2K, we know that this is not a shadow copy volume.

    Also note that in Windows XP, we need to test to see if the driver name
    of this device object is \Driver\VolSnap in addition to seeing if this
    device is read-only.  For Windows Server 2003, we can infer that
    this is a ShadowCopy by looking for a DeviceType == FILE_DEVICE_VIRTUAL_DISK
    and read-only volume.
    
Arguments:

    StorageStackDeviceObject - pointer to the disk device object
    IsShadowCopy - returns TRUE if this is a shadow copy, FALSE otherwise
        
Return Value:

    The status of the operation.  If this operation fails IsShadowCopy is
    always set to FALSE.

--*/
{

    PAGED_CODE();

    //
    //  Default to NOT a shadow copy volume
    //

    *IsShadowCopy = FALSE;

    if (IS_WINDOWS2000()) {

        UNREFERENCED_PARAMETER( StorageStackDeviceObject );
        return STATUS_SUCCESS;

    }

    if (IS_WINDOWSXP()) {

        UNICODE_STRING volSnapDriverName;
        WCHAR buffer[DEVICE_NAME_MAX_LENGTH];
        PUNICODE_STRING storageDriverName;
        ULONG returnedLength;
        NTSTATUS status;

        //
        //  In Windows XP, all ShadowCopy devices were of type FILE_DISK_DEVICE.
        //  If this does not have a device type of FILE_DISK_DEVICE, then
        //  it is not a ShadowCopy volume.  Return now.
        //

        if (FILE_DEVICE_DISK != StorageStackDeviceObject->DeviceType) {

            return STATUS_SUCCESS;
        }

        //
        //  Unfortunately, looking for the FILE_DEVICE_DISK isn't enough.  We
        //  need to find out if the name of this driver is \Driver\VolSnap as
        //  well.
        //

        storageDriverName = (PUNICODE_STRING) buffer;
        RtlInitEmptyUnicodeString( storageDriverName, 
                                   Add2Ptr( storageDriverName, sizeof( UNICODE_STRING ) ),
                                   sizeof( buffer ) - sizeof( UNICODE_STRING ) );

        status = ObQueryNameString( StorageStackDeviceObject,
                                    (POBJECT_NAME_INFORMATION)storageDriverName,
                                    storageDriverName->MaximumLength,
                                    &returnedLength );

        if (!NT_SUCCESS( status )) {

            return status;
        }

        RtlInitUnicodeString( &volSnapDriverName, L"\\Driver\\VolSnap" );

        if (RtlEqualUnicodeString( storageDriverName, &volSnapDriverName, TRUE )) {

            //
            // This is a ShadowCopy volume, so set our return parameter to true.
            //

            *IsShadowCopy = TRUE;

        } else {

            //
            //  This is not a ShadowCopy volume, but IsShadowCopy is already 
            //  set to FALSE.  Fall through to return to the caller.
            //

            NOTHING;
        }

        return STATUS_SUCCESS;
        
    } else {

        PIRP irp;
        KEVENT event;
        IO_STATUS_BLOCK iosb;
        NTSTATUS status;

        //
        //  For Windows Server 2003 and later, it is sufficient to test for a
        //  device type fo FILE_DEVICE_VIRTUAL_DISK and that the device
        //  is read-only to identify a ShadowCopy.
        //

        //
        //  If this does not have a device type of FILE_DEVICE_VIRTUAL_DISK, then
        //  it is not a ShadowCopy volume.  Return now.
        //

        if (FILE_DEVICE_VIRTUAL_DISK != StorageStackDeviceObject->DeviceType) {

            return STATUS_SUCCESS;
        }

        //
        //  It has the correct device type, see if it is marked as read only.
        //
        //  NOTE:  You need to be careful which device types you do this operation
        //         on.  It is accurate for this type but for other device
        //         types it may return misleading information.  For example the
        //         current microsoft cdrom driver always returns CD media as
        //         readonly, even if the media may be writable.  On other types
        //         this state may change.
        //

        KeInitializeEvent( &event, NotificationEvent, FALSE );

        irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_IS_WRITABLE,
                                             StorageStackDeviceObject,
                                             NULL,
                                             0,
                                             NULL,
                                             0,
                                             FALSE,
                                             &event,
                                             &iosb );

        //
        //  If we could not allocate an IRP, return an error
        //

        if (irp == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Call the storage stack and see if this is readonly
        //

        status = IoCallDriver( StorageStackDeviceObject, irp );

        if (status == STATUS_PENDING) {

            (VOID)KeWaitForSingleObject( &event,
                                         Executive,
                                         KernelMode,
                                         FALSE,
                                         NULL );

            status = iosb.Status;
        }

        //
        //  If the media is write protected then this is a shadow copy volume
        //

        if (STATUS_MEDIA_WRITE_PROTECTED == status) {

            *IsShadowCopy = TRUE;
            status = STATUS_SUCCESS;
        }

        //
        //  Return the status of the IOCTL.  IsShadowCopy is already set to FALSE
        //  which is what we want if STATUS_SUCCESS was returned or if an error
        //  was returned.
        //

        return status;
    }
}

NTSTATUS 
FsGetStorageDeviceBusType(
	IN PDEVICE_OBJECT StorageStackDeviceObject, 
	ULONG* puType
	)
/*++

Routine Description:

    This routine is the routine for the general purpose file
    system driver.  It simply passes requests onto the next driver in the
    stack, which is presumably a disk file system.

Arguments:

    StorageStackDeviceObject - Pointer to the device object for this driver.

    puType - Pointer to the type of lower media.

Return Value:

    The function value is the status of the operation.

Note:

    A note to file system filter implementers:  
        This routine actually "passes" through the request by taking this
        driver out of the IRP stack.  If the driver would like to pass the
        I/O request through, but then also see the result, then rather than
        taking itself out of the loop it could keep itself in by copying the
        caller's parameters to the next stack location and then set its own
        completion routine.  

        Hence, instead of calling:
    
            IoSkipCurrentIrpStackLocation( Irp );

        You could instead call:

            IoCopyCurrentIrpStackLocationToNext( Irp );
            IoSetCompletionRoutine( Irp, NULL, NULL, FALSE, FALSE, FALSE );


        This example actually NULLs out the caller's I/O completion routine, but
        this driver could set its own completion routine so that it would be
        notified when the request was completed (see SfCreate for an example of
        this).

--*/
{

	PIRP newIrp = NULL;

	PSTORAGE_DEVICE_DESCRIPTOR pDescriptor = NULL;
	PSTORAGE_PROPERTY_QUERY 	pQuery = NULL;
	PSTORAGE_PROPERTY_QUERY 	pBuffer = NULL;

	KEVENT 						waitEvent;	

	NTSTATUS 					status = STATUS_SUCCESS;
	IO_STATUS_BLOCK 			ioStatus;

	*puType = BusTypeUnknown;

	if(KeGetCurrentIrql() > APC_LEVEL)
		goto out;
	
	pBuffer = (PSTORAGE_PROPERTY_QUERY)ExAllocatePoolWithTag(NonPagedPool, sizeof(STORAGE_DEVICE_DESCRIPTOR) * 4, FLT_MEMORY_TAG);
	if(pBuffer == NULL){
		
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto out;
	}
	
	pQuery  = (PSTORAGE_PROPERTY_QUERY)ExAllocatePoolWithTag(NonPagedPool, sizeof(STORAGE_DEVICE_DESCRIPTOR), FLT_MEMORY_TAG);
	if(pQuery == NULL) {
		
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto out;
	}

	/* first set the query properties */
	pQuery->PropertyId = StorageDeviceProperty;
	pQuery->QueryType = PropertyStandardQuery;

	/* initialize the waitable event */
	KeInitializeEvent(&waitEvent, NotificationEvent, FALSE);

	newIrp = IoBuildDeviceIoControlRequest(
			IOCTL_STORAGE_QUERY_PROPERTY,
			StorageStackDeviceObject, 
			(PVOID)pQuery,
			sizeof(STORAGE_DEVICE_DESCRIPTOR), 
			(PVOID)pBuffer,
			sizeof(STORAGE_DEVICE_DESCRIPTOR) * 4, 
			FALSE,
			&waitEvent, 
			&ioStatus);

	if (NULL == newIrp) {
		
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto out;
	}

	/* send this irp to the storage device */
	status = IoCallDriver(StorageStackDeviceObject, newIrp);
	if (status == STATUS_PENDING) 	{
		
		status = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
		status = ioStatus.Status;
	}
	
	if (!NT_SUCCESS(status))
		goto out;
 
	pDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR)pBuffer;
	*puType = pDescriptor->BusType;

out:
	
	if(pBuffer) {
		
		ExFreePoolWithTag(pBuffer, FLT_MEMORY_TAG);
	}
	
	if(pQuery){
		
		ExFreePoolWithTag(pQuery, FLT_MEMORY_TAG);
	}

	return status;
}

BOOLEAN
FsIsNetworkAccess(
    __in PFLT_CALLBACK_DATA Data
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN pToken = NULL;
    PTOKEN_SOURCE pTokenSrc = NULL ;
    PSECURITY_SUBJECT_CONTEXT pSecSubCtx = NULL;
    BOOLEAN ret = FALSE;
	
    pSecSubCtx = &(Data->Iopb->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);

    __try  { 

	    if((pSecSubCtx->ClientToken != NULL) 
			|| (pSecSubCtx->PrimaryToken != NULL)
			|| (KeGetCurrentIrql() == PASSIVE_LEVEL)) {

			
	        pToken = SeQuerySubjectContextToken(pSecSubCtx);

		 if(pToken == NULL) {

			__leave;
		 }
			
	    } else{

		__leave;
	    }
	
	    status = SeQueryInformationToken(pToken, TokenSource, &pTokenSrc);	

           if (NT_SUCCESS(status)) 	{
		   	
			pTokenSrc->SourceName[TOKEN_SOURCE_LENGTH - 1] = 0x00;

			if (_stricmp(pTokenSrc->SourceName,"NtLmSsp") == 0 )	{ 
				
				ret = TRUE;
			}
          }	
    } __finally  {

		if(pTokenSrc) {
			
			ExFreePool(pTokenSrc);
		}
    }

    return ret;
} 

#define FLT_DELAY_EXECUTE_MICROSECOND_INTERVAL (-10) //micro second

VOID
FsKernelSleep(
	__in ULONG MicroSecond
	)
{
	LARGE_INTEGER asceInterval;

	if(APC_LEVEL >= KeGetCurrentIrql()) {
		
		asceInterval.QuadPart = FLT_DELAY_EXECUTE_MICROSECOND_INTERVAL;
		asceInterval.QuadPart *= MicroSecond;
		
		KeDelayExecutionThread(KernelMode, 0, &asceInterval);
	}
}
#undef FLT_DELAY_EXECUTE_MICROSECOND_INTERVAL

//----------------------------------------------------------------------------------------
//文件相关

NTSTATUS
FsCreateFile(
    __in PFLT_INSTANCE        Instance, 
    __in ULONG                     DesiredAccess,
    __in ULONG                     CreateDisposition,
    __in ULONG                     CreateOptions,
    __in ULONG                     ShareAccess,
    __in ULONG                     FileAttribute,
    __in PUNICODE_STRING 	FileName,
    __out HANDLE *			FileHandle,
    __out PFILE_OBJECT *	FileObject
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES objAttributes; 
    IO_STATUS_BLOCK ioStatusBlock;     

    InitializeObjectAttributes (&objAttributes,  
	                        FileName,  
	                        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,  
	                        NULL,  
	                        NULL); 
	
    status = FltCreateFile (g_FileFltContext.FileFltHandle,  
	                Instance,  
	                FileHandle,  
	                DesiredAccess,     
	                &objAttributes, 
	                &ioStatusBlock, 
	                (PLARGE_INTEGER) NULL, 
	                FileAttribute, 
	                ShareAccess, 
	                CreateDisposition, 
	                CreateOptions, 
	                NULL, 
	                0, 
	                IO_IGNORE_SHARE_ACCESS_CHECK); 

     if (NT_SUCCESS( status )) {

	  status = ObReferenceObjectByHandle(
	           *FileHandle,                 //Handle
	           0,                      //DesiredAccess
	           NULL,                   //ObjectType
	           KernelMode,             //AccessMode
	           FileObject,      //Object
	           NULL);                  //HandleInformation

	  if(!NT_SUCCESS( status )) {

              FltClose(*FileHandle); 
		*FileHandle = NULL;		
	  }
    }

     return status;
}

NTSTATUS
FsOpenFile(
       __in PFLT_INSTANCE                   Instance,  	
	__in PUNICODE_STRING 		FileName,
	__out HANDLE *				FileHandle,
	__out PFILE_OBJECT *			FileObject,
	__out PULONG					FileStatus
	)
{
    NTSTATUS status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES objAttributes; 
    IO_STATUS_BLOCK ioStatusBlock; 

    InitializeObjectAttributes (&objAttributes,  
	                        FileName,  
	                        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,  
	                        NULL,  
	                        NULL); 
	
    status = FltCreateFile (g_FileFltContext.FileFltHandle,  
	                Instance,  
	                FileHandle,  
	                GENERIC_READ,     
	                &objAttributes, 
	                &ioStatusBlock, 
	                (PLARGE_INTEGER) NULL, 
	                FILE_ATTRIBUTE_NORMAL, 
	                FILE_SHARE_READ, 
	                FILE_OPEN, 
	                0, 
	                NULL, 
	                0, 
	                IO_IGNORE_SHARE_ACCESS_CHECK); 

    if(FileStatus != NULL) {
		
    	*FileStatus = ioStatusBlock.Information;
    }
	
    if (NT_SUCCESS( status )) {

	  status = ObReferenceObjectByHandle(
	           *FileHandle,                 //Handle
	           0,                      //DesiredAccess
	           NULL,                   //ObjectType
	           KernelMode,             //AccessMode
	           FileObject,      //Object
	           NULL);                  //HandleInformation

	  if(!NT_SUCCESS( status )) {

              FltClose(*FileHandle); 
		*FileHandle = NULL;		
	  }
    }
	        

    return status;
}


VOID
FsCloseFile(
	__in HANDLE 			FileHandle,
	__in PFILE_OBJECT 	FileObject
	)
{
	if(FileObject)
		ObDereferenceObject (FileObject);
	if(FileHandle)
		FltClose(FileHandle); 
}

int
FsDeleteFile(
        __in PFLT_INSTANCE    Instance,
        __in PFILE_OBJECT       FileObject
        )
{
    int rc = -1;
    
    NTSTATUS status = STATUS_SUCCESS;
    FILE_DISPOSITION_INFORMATION fileDispositionInformation;

    fileDispositionInformation.DeleteFile = TRUE;
    
    status = FltSetInformationFile(Instance,
							FileObject,
							&fileDispositionInformation,
							sizeof(fileDispositionInformation),
							FileDispositionInformation
							) ;

    if(NT_SUCCESS(status)) {

        rc = 0;
    }

    return rc;
}

BOOLEAN
FsCheckFileIsDirectoryByObject(
    __in PFLT_INSTANCE    Instance,
    __in PFILE_OBJECT  	   FileObject
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN rc = FALSE;

	status = FltIsDirectory(FileObject, Instance, &rc);

	return rc ;
}

int
FsCheckFileExistAndDirectoryByFileName(
       __inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in PFLT_FILE_NAME_INFORMATION NameInfo,
	__out BOOLEAN *Exist,
	__out BOOLEAN *Directory
	)
{
	int rc = -1;
	
	HANDLE fileHandle = NULL;
	PFILE_OBJECT fileObject = NULL;
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING fileName = { 0, 0, NULL };
	ULONG fileStatus = 0;

	*Exist = FALSE;
	*Directory = FALSE;
	
	if(NameInfo->Name.Buffer[ (NameInfo->Name.Length  / sizeof(WCHAR)) - 1 ] == L'\\') {

		*Directory = TRUE;
		rc = 0;
		goto FsCheckFileExistAndDirectoryByFileNameCleanup;
		
	}else if(FltObjects->FileObject->FileName.Buffer[(FltObjects->FileObject->FileName.Length / sizeof(WCHAR)) - 1] == L'\\') {

		*Directory = TRUE;
		rc = 0;
		goto FsCheckFileExistAndDirectoryByFileNameCleanup;
	}
	
	if(FsGetFileNameWithoutStreamName(NameInfo, &fileName) == -1) {

		*Directory = TRUE;
		rc = 0;
		goto FsCheckFileExistAndDirectoryByFileNameCleanup;
	}
	
	status = FsOpenFile(Data->Iopb->TargetInstance, &fileName, &fileHandle, &fileObject, &fileStatus);

	if(status != STATUS_OBJECT_NAME_NOT_FOUND) {

		*Exist = TRUE;
	}/*else if(fileStatus == FILE_DOES_NOT_EXIST){
	
		*Exist = TRUE;
	}*/


	if(NT_SUCCESS(status)) {	
		
		if(FsCheckFileIsDirectoryByObject(Data->Iopb->TargetInstance, fileObject) == TRUE) {

			*Directory = TRUE;
		}

		FsCloseFile(fileHandle, fileObject);		
		
		rc = 0;
	}

FsCheckFileExistAndDirectoryByFileNameCleanup:


	if(fileName.Buffer != NULL ) {
		
		FsFreeUnicodeString(&fileName);
	}

	return rc;
}


//-----------------------------------------------------------------------------------------
//名字相关

ULONG 
FsGetTaskNameOffset(
	VOID
	)
{
    PEPROCESS proc = PsGetCurrentProcess();
    ULONG i = 0;

    for( ; i < (3 * PAGE_SIZE) ; i++ ) {
		
        if( strncmp( "System", (((PCHAR)proc) + i), 6) == 0) {
			
            return i;
        }
    }

    return 0;
}

NTSTATUS
FsGetObjectName(
    __in 		PVOID Object,
    __inout 	PUNICODE_STRING Name
    )
{
	NTSTATUS rc ;
	CHAR rawNameInfo[512] = { 0 }; 
    	POBJECT_NAME_INFORMATION nameInfo = (POBJECT_NAME_INFORMATION)rawNameInfo;
    	ULONG retLength = 0;

	rc = ObQueryNameString( Object, nameInfo, sizeof(rawNameInfo), &retLength);

	Name->Length = 0;
	if (NT_SUCCESS( rc )) {

	    RtlCopyUnicodeString( Name, &(nameInfo->Name) );
		
	}else if(rc == STATUS_INFO_LENGTH_MISMATCH){

		CHAR *rawNameInfoD = ExAllocatePoolWithTag(NonPagedPool, sizeof(OBJECT_NAME_INFORMATION) + retLength, NAME_TAG);

		if(rawNameInfoD){

			nameInfo = (POBJECT_NAME_INFORMATION)rawNameInfoD;

			rc = ObQueryNameString( Object, nameInfo, sizeof(rawNameInfo), &retLength);

			Name->Length = 0;
			if (NT_SUCCESS( rc )) {

			    RtlCopyUnicodeString( Name, &(nameInfo->Name) );
			}

			ExFreePoolWithTag(rawNameInfoD, NAME_TAG);
		}
	}

	return rc;
}

static NTSTATUS
FsQuerySymbolicLink(
    __in  PUNICODE_STRING SymbolicLinkName,
    __out PUNICODE_STRING LinkTarget
    )
{
	NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;

	HANDLE Handle = NULL;
	OBJECT_ATTRIBUTES ObjAttribute;
	InitializeObjectAttributes(&ObjAttribute, SymbolicLinkName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, 0);

	status = ZwOpenSymbolicLinkObject(&Handle, GENERIC_READ, &ObjAttribute);

	if (NT_SUCCESS(status)){
		
		ULONG ret_length = 0;

		LinkTarget->MaximumLength = 0;
		LinkTarget->Length = 0;
		LinkTarget->Buffer = NULL;

		status = ZwQuerySymbolicLinkObject(Handle, LinkTarget, &ret_length);

		if((!NT_SUCCESS(status)) 
			&& (status == STATUS_BUFFER_TOO_SMALL)){
			
			LinkTarget->MaximumLength = (USHORT)(ret_length + sizeof(WCHAR));
			LinkTarget->Length = 0;
			LinkTarget->Buffer = ExAllocatePoolWithTag(NonPagedPool, LinkTarget->MaximumLength, IMAGE_NAME_TAG);

			status = STATUS_INSUFFICIENT_RESOURCES;

			if (LinkTarget->Buffer){
				
				status = ZwQuerySymbolicLinkObject(Handle, LinkTarget, &ret_length);

				if (!NT_SUCCESS(status)){
					
					ExFreePool(LinkTarget->Buffer);
				}	

				LinkTarget->Buffer[(ret_length / sizeof(WCHAR)) - 1] = L'\0';
			}		
		}else{
		
			status = STATUS_INSUFFICIENT_RESOURCES;
		}		

		ZwClose(Handle);
	}

	return status;
}

NTSTATUS
SfGetDeviceDosName(
    __in WCHAR *VolumeDeviceName,
    __out WCHAR *DosLetter,
    __in  ULONG *Offset
    )
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	WCHAR dosName[8] = { 0 };
    	UNICODE_STRING dosNameU;    
    	WCHAR index = L'C';    

	dosName[0] = L'\\';
	dosName[1] = L'?';
	dosName[2] = L'?';
	dosName[3] = L'\\';
	dosName[4] = L'C';
	dosName[5] = L':';
	dosName[6] = L'\0';

    	RtlInitUnicodeString(&dosNameU, dosName);

    	for (; index <= L'Z'; index++)  {
		
		UNICODE_STRING linkTarget;

		RtlZeroMemory(&linkTarget, sizeof(linkTarget));
		
        	dosNameU.Buffer[4] = index;

        	status = FsQuerySymbolicLink(&dosNameU, &linkTarget);
			
        	if (!NT_SUCCESS(status)){
				
            		continue;
		}

		if(_wcsnicmp(linkTarget.Buffer, VolumeDeviceName, (linkTarget.Length / sizeof(WCHAR))) == 0) {
			
			*Offset = linkTarget.Length / sizeof(WCHAR);
			*DosLetter = index;

            		ExFreePool(linkTarget.Buffer);

            		break;
        	}

        	ExFreePool(linkTarget.Buffer);
    	}

	status = STATUS_UNSUCCESSFUL;

    	if (index <= L'Z')  {
			
        	status = STATUS_SUCCESS;
    	}

    	return status;
}


NTSTATUS     
FsGetProcessImageName(    
    __in  HANDLE   ProcessId,    
    __out PUNICODE_STRING *ProcessImageName    
    )    
{    
    NTSTATUS 		status = STATUS_INSUFFICIENT_RESOURCES;    
    HANDLE 		procHandle = NULL;    
    PEPROCESS 	eProcess = NULL;    
    ULONG		retLength = 0;    
    ULONG 		bufferLength = 0;    
    PVOID 		buffer = NULL;    
       

    if(g_FileFltContext.DynamicFunctions.QueryInformationProcess == NULL) {

        goto FsGetProcessImageNameCleanup;
    }
	
    status = PsLookupProcessByProcessId(ProcessId, &eProcess);  
	
    if (!NT_SUCCESS(status)){
		
        goto FsGetProcessImageNameCleanup;
    }
        
    status = ObOpenObjectByPointer(eProcess,          // Object    
                                   OBJ_KERNEL_HANDLE,  // HandleAttributes    
                                   NULL,               // PassedAccessState OPTIONAL    
                                   GENERIC_READ,       // DesiredAccess    
                                   /**PsThreadType*/NULL,     // ObjectType    
                                   KernelMode,         // AccessMode    
                                   &procHandle);  
    if (!NT_SUCCESS(status)){
		
        goto FsGetProcessImageNameCleanup;
    }
        
        
    //    
    // Step one - get the size we need    
    //    
    status = g_FileFltContext.DynamicFunctions.QueryInformationProcess( procHandle,    
                                        ProcessImageFileName,    
                                        NULL, // buffer    
                                        0, // buffer size    
                                        &retLength);    
        
   
    if (STATUS_INFO_LENGTH_MISMATCH != status) {
		
	goto FsGetProcessImageNameCleanup;
    }    
   
    //    
    // Is the passed-in buffer going to be big enough for us?     
    // This function returns a single contguous buffer model...    
    //    
    bufferLength = retLength - sizeof(UNICODE_STRING);   

    if ((FILE_PATH_MAX_LENGTH * sizeof(WCHAR)) < bufferLength) 	{

	status = STATUS_BUFFER_OVERFLOW;
	
	goto FsGetProcessImageNameCleanup;
           
    }
   
    //    
    // If we get here, the buffer IS going to be big enough for us, so    
    // let's allocate some storage.    
    //    
    buffer = ExAllocatePoolWithTag(PagedPool, retLength, IMAGE_NAME_TAG);    
   
    if (NULL == buffer) { 

	status = STATUS_INSUFFICIENT_RESOURCES;
	
	goto FsGetProcessImageNameCleanup;
    }    
   
    //    
    // Now lets go get the data    
    //    
    status = g_FileFltContext.DynamicFunctions.QueryInformationProcess( procHandle,    
                                        ProcessImageFileName,    
                                        buffer,    
                                        retLength,    
                                        &retLength);    
   
    if (NT_SUCCESS(status)) 	{   	
		
        *ProcessImageName = ((PUNICODE_STRING) buffer);    

	 buffer = NULL;
    }    

FsGetProcessImageNameCleanup:


    //    
    // free our buffer    
    //
    if(buffer != NULL ) {
		
   	 ExFreePoolWithTag(buffer, IMAGE_NAME_TAG);  
    }
	
    if(procHandle != NULL) {

	ZwClose(procHandle);

    }
	
    if(eProcess != NULL) {

	ObDereferenceObject(eProcess);
    }
	 
    //    
    // And tell the caller what happened.    
    //       
    return status;           
} 

int
FsGetFileNameWithoutStreamName(
	__in PFLT_FILE_NAME_INFORMATION NameInfo,
	__inout PUNICODE_STRING FileName
	)
{
	int rc = -1;
	NTSTATUS status = STATUS_SUCCESS;
	
	FileName->MaximumLength = NameInfo->Name.Length + sizeof(WCHAR);
	
    	status = FsAllocateUnicodeString(FileName);
		
	if(!NT_SUCCESS( status )){

	 	goto FsGetFileNameWithoutStreamNameCleanup;
	}

	RtlCopyUnicodeString(FileName, &NameInfo->Name);
	if(NameInfo->Stream.Length > 0) {
		
		FileName->Length -= NameInfo->Stream.Length;
		FileName->Buffer[FileName->Length / sizeof(WCHAR)] = L'\0';	
	}

	rc = 0;

FsGetFileNameWithoutStreamNameCleanup:
	
	return rc;

}

//-------------------------------------------------------------------------------------
//thread
NTSTATUS 
FsStartThreadInProcess (
	__in PKSTART_ROUTINE threadProc, 
	__in PVOID threadArg, 
	__out PKTHREAD *kThread, 
	__in PEPROCESS process
	)
{
	NTSTATUS status;
	HANDLE threadHandle;
	HANDLE processHandle = NULL;
	OBJECT_ATTRIBUTES threadObjAttributes;

	if (process)
	{
		status = ObOpenObjectByPointer (process, OBJ_KERNEL_HANDLE, NULL, 0, NULL, KernelMode, &processHandle);
		if (!NT_SUCCESS (status))
			return status;
	}

	InitializeObjectAttributes (&threadObjAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	
	status = PsCreateSystemThread (&threadHandle, THREAD_ALL_ACCESS, &threadObjAttributes, processHandle, NULL, threadProc, threadArg);
	if (!NT_SUCCESS (status))
		return status;

	status = ObReferenceObjectByHandle (threadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID *) kThread, NULL);
	if (!NT_SUCCESS (status))
	{
		ZwClose (threadHandle);
		*kThread = NULL;
		return status;
	}

	if (processHandle)
		ZwClose (processHandle);

	ZwClose (threadHandle);
	return STATUS_SUCCESS;
}

NTSTATUS 
FsStartThread (
	__in PKSTART_ROUTINE threadProc, 
	__in PVOID threadArg, 
	__out PKTHREAD *kThread
	)
{
	return FsStartThreadInProcess (threadProc, threadArg, kThread, NULL);
}

VOID FsStopThread (
	__in PKTHREAD kThread
	)
{
	KeWaitForSingleObject (kThread, Executive, KernelMode, FALSE, NULL);
	ObDereferenceObject (kThread);
}

int
FsCheckCryptFile(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects
	)
{
	int rc = -1;
	PFLT_FILE_NAME_INFORMATION 	nameInfo = NULL;
	NTSTATUS 					status = STATUS_SUCCESS;	

	status = FltGetFileNameInformation( Data,
	                                        FLT_FILE_NAME_NORMALIZED |
	                                        FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
	                                        &nameInfo );
    
       if (!NT_SUCCESS( status )) {

		if(Data->Iopb->TargetFileObject == NULL)
			DbgPrint("FsCheckCrptFile(0-1): error ---------------------------\n");
		else if(Data->Iopb->TargetFileObject->FileName.Length == 0)
			DbgPrint("FsCheckCrptFile(0-2):error --------0x%p\n", Data->Iopb->TargetFileObject);
		else 
			DbgPrint("FsCheckCrptFile(0-3):error : --0x%p, %wZ\n", Data->Iopb->TargetFileObject, &Data->Iopb->TargetFileObject->FileName);

		if(FltObjects->FileObject == NULL)
			DbgPrint("FsCheckCrptFile(0-4):error ---------------------------\n");
		else if(FltObjects->FileObject->FileName.Length == 0)
			DbgPrint("FsCheckCrptFile(0-5):error ---------0x%p\n", FltObjects->FileObject );
		else 
			DbgPrint("FsCheckCrptFile(0-6):error --0x%p, %wZ\n", FltObjects->FileObject , &FltObjects->FileObject->FileName);

		if(FltObjects->FileObject && FltObjects->FileObject->FileName.Length == 12)
			rc = 0;

              DbgPrint("FsCheckCryptFile: error ---rc = %d,  status = 0x%x\n", rc , status);
              
		goto FsCheckCryptFileCleanup;
       }

	if((nameInfo->Name.Length < 46)
              ||(RtlCompareMemory(nameInfo->Name.Buffer, L"\\Device\\HarddiskVolume3", 46) == 46)){

              rc = 0;

              DbgPrint("FsCheckCryptFile: %wZ, rc = %d\n", &(nameInfo->Name), rc);
	}           

FsCheckCryptFileCleanup:

	if(nameInfo != NULL){

		FltReleaseFileNameInformation( nameInfo );

	}
	
	return rc;
}