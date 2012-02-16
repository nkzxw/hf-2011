#include "fileflt.h"

int
SifsWriteFileSize(
	__in PFLT_INSTANCE Instance,
	__in PUNICODE_STRING FileName,
	__inout PUCHAR Metadata,
	__in  LONG MetadataLen,
	__in LONGLONG  FileSize
	)
{
	int rc = -1;
	NTSTATUS status = STATUS_SUCCESS;
	LARGE_INTEGER byteOffset;
	ULONG writedLen = 0;
	HANDLE fileHandle = NULL;
	PFILE_OBJECT fileObject = NULL;

	__try{

		status = FsOpenFile(Instance, FileName, &fileHandle, &fileObject, NULL);

		if(!NT_SUCCESS(status)) {

			__leave;
		}
		
		byteOffset.QuadPart = 0;

		put_unaligned_be64(FileSize, Metadata);
		
		status = FltWriteFile(Instance, fileObject, &byteOffset, MetadataLen, Metadata
				, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET | FLTFL_IO_OPERATION_NON_CACHED, &writedLen, NULL, NULL);

		/*status = FsWriteFile(Instance, FileObject , 0, MetadataLen, Metadata
				, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET | FLTFL_IO_OPERATION_NON_CACHED, &writedLen, NULL, NULL);*/

		if(NT_SUCCESS(status)) {

			rc = 0;
		}
		
	}__finally{

		if(fileObject != NULL) {

			FsCloseFile(fileHandle, fileObject);
		}		

		DbgPrint("SifsWriteFileSize: 0x%x\n", status);
	}


	return rc;
}

int
SifsWriteSifsMetadata(
        __in PFLT_INSTANCE        Instance,
        __in ULONG                     DesiredAccess,
        __in ULONG                     CreateDisposition,
        __in ULONG                     CreateOptions,
        __in ULONG                     ShareAccess,
        __in ULONG                     FileAttribute,
	__in PFLT_FILE_NAME_INFORMATION NameInfo,
	__inout PCRYPT_CONTEXT CryptContext
	)
{
	int rc = -1;

	NTSTATUS status = STATUS_SUCCESS;
	HANDLE fileHandle = NULL;
	PFILE_OBJECT fileObject = NULL;
            
	status = FsCreateFile(Instance
                        , DesiredAccess, CreateDisposition, CreateOptions, ShareAccess, FileAttribute
                        , &NameInfo->Name, &fileHandle, &fileObject, NULL);

	if(NT_SUCCESS(status)) {

              ULONG writeLen = 0;
		ULONG writedLen = 0;
              LARGE_INTEGER byteOffset;
		PCHAR buffer = ExAllocatePoolWithTag(NonPagedPool, CryptContext->MetadataSize, SIFS_METADATA_TAG);

              if(buffer == NULL) {
                
                    goto SifsWriteSifsMetadataCloseFile;
              }

              RtlZeroMemory(buffer, CryptContext->MetadataSize);

		if(SifsWriteHeadersVirt(buffer, CryptContext->MetadataSize, &writeLen, CryptContext) == -1){

			goto SifsWriteSifsMetadataCloseFile;
		}

              byteOffset.QuadPart = 0;
                
        	status = FltWriteFile(Instance, fileObject, &byteOffset, CryptContext->MetadataSize, buffer
        				, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET | FLTFL_IO_OPERATION_NON_CACHED, &writedLen, NULL, NULL );
              
		if(NT_SUCCESS(status)) {

			rc = 0;
		}
        

 SifsWriteSifsMetadataCloseFile:

		if(buffer != NULL) {

			ExFreePoolWithTag(buffer, SIFS_METADATA_TAG);
		}
              if(rc == -1) {

                    FsDeleteFile(Instance, fileObject);
              }
              
		FsCloseFile(fileHandle, fileObject);              
	}
	

SifsWriteSifsMetadataCleanup:

       DbgPrint("SifsWriteSifsMetadata: %wZ(%d), rc = %d, status = 0x%x, createDisposition = %d, createOptions = 0x%x\n"
            , &(NameInfo->Name), NameInfo->Name.Length, rc, status, CreateDisposition, CreateOptions);
       
	return rc;
}

static int
SifsCheckValidateSifs(
	__in PFLT_INSTANCE Instance,
	__in PFILE_OBJECT FileObject,
	__out PUCHAR  PageVirt,
	__in LONG PageVirtLen,
	__inout PCRYPT_CONTEXT CryptContext
	)
{
	int rc = -1;

       NTSTATUS status = STATUS_SUCCESS;
       LARGE_INTEGER byteOffset;
       ULONG readLen = 0;

	RtlZeroMemory(PageVirt, PageVirtLen);
       byteOffset.QuadPart = 0;

       status = FltReadFile(Instance, FileObject, &byteOffset, PageVirtLen, PageVirt
				, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET | FLTFL_IO_OPERATION_NON_CACHED, &readLen, NULL, NULL );

	  if(NT_SUCCESS(status)) {

		if(SifsReadHeadersVirt(PageVirt, CryptContext, SIFS_VALIDATE_HEADER_SIZE) == 0) {

			rc = 0;
		}
	  }  

	return rc;
}

int
SifsQuickCheckValidateSifs(
	__in PFLT_INSTANCE Instance,
	__in PFILE_OBJECT FileObject,
	__out PUCHAR  PageVirt,
	__in LONG PageVirtLen
	)
{
	int rc = -1;

       NTSTATUS status = STATUS_SUCCESS;
       LARGE_INTEGER byteOffset;
       ULONG readLen = 0;

	RtlZeroMemory(PageVirt, PageVirtLen);
       byteOffset.QuadPart = 0;

       status = FltReadFile(Instance, FileObject, &byteOffset, PageVirtLen, PageVirt
				, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET | FLTFL_IO_OPERATION_NON_CACHED, &readLen, NULL, NULL );

	  if(NT_SUCCESS(status)) {

		if(SifsQuickCheckValidate_i(PageVirt) == 0) {

			rc = 0;
		}
	  }  

	return rc;
}


int
SifsQuickCheckValidate(
       __in PFLT_INSTANCE Instance,
	__in PUNICODE_STRING FileName,
	__inout PCRYPT_CONTEXT CryptContext,
	__inout PBOOLEAN IsEmptyFile,
	__in LONG Aligned
	)
{
	int rc = -1;

	NTSTATUS status = STATUS_SUCCESS;
	HANDLE fileHandle = NULL;
	PFILE_OBJECT fileObject = NULL;
	FILE_STANDARD_INFORMATION fileStandardInformation ;
	LONG  bufferLen = ROUND_TO_SIZE(SIFS_CHECK_FILE_VALID_MINIMUN_SIZE, Aligned);
	PCHAR buffer = NULL; 
       
	status = FsOpenFile(Instance, FileName, &fileHandle, &fileObject, NULL);

	if(!NT_SUCCESS(status)) {

		goto SifsQuickCheckValidateCleanup;
	}

	status = FltQueryInformationFile(Instance,
								 fileObject,
								 &fileStandardInformation,
								 sizeof(fileStandardInformation),
								 FileStandardInformation,
								 NULL
								 ) ;

	if(NT_SUCCESS(status)) {

		if(fileStandardInformation.Directory == TRUE) {

			goto SifsQuickCheckValidateCleanup;
		}

		if(fileStandardInformation.EndOfFile.QuadPart  == 0) {

			*IsEmptyFile = TRUE;
			goto SifsQuickCheckValidateCleanup;
		}
	}

	buffer = ExAllocatePoolWithTag(NonPagedPool, bufferLen, SIFS_METADATA_TAG);

	if(buffer == NULL) {

		goto SifsQuickCheckValidateCleanup;
	}

	if(SifsQuickCheckValidateSifs(Instance, fileObject, buffer, bufferLen) == 0) {

		rc = 0;
	}
	
SifsQuickCheckValidateCleanup:

	if(buffer != NULL) {

		 ExFreePoolWithTag(buffer, SIFS_METADATA_TAG);
	}
	
	if(fileObject != NULL) {

		FsCloseFile(fileHandle, fileObject);
	}
	
	return rc;
}

int
SifsReadSifsMetadata(
       __in PFLT_INSTANCE Instance,
	__in PFILE_OBJECT FileObject,
	__inout PSTREAM_CONTEXT StreamContext
	)
{
	int rc = -1;
	
	NTSTATUS status = STATUS_SUCCESS;
	PCHAR buffer = ExAllocatePoolWithTag(NonPagedPool, StreamContext->CryptContext.MetadataSize, SIFS_METADATA_TAG);

	if(buffer == NULL) {

		goto SifsReadSifsMetadataCleanup;
	}

	if(SifsCheckValidateSifs(Instance, FileObject, buffer, StreamContext->CryptContext.MetadataSize, &(StreamContext->CryptContext)) == 0) {

		StreamContext->Lower.Metadata = buffer;
		rc = 0;
	}
	
SifsReadSifsMetadataCleanup:

	if(rc == -1) {
		
		if(buffer != NULL) {

			ExFreePoolWithTag(buffer, SIFS_METADATA_TAG);
		}
	}
	
	return rc;
}

