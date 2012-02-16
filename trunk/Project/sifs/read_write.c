#include "fileflt.h"


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

	UNICODE_STRING fileName = { 0, 0, NULL };
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE fileHandle = NULL;
	PFILE_OBJECT fileObject = NULL;

       if(FsGetFileNameWithoutStreamName(NameInfo, &fileName) == -1) {

		goto SifsWriteSifsMetadataCleanup;
	}
            
	status = FsCreateFile(Instance
                        , DesiredAccess, CreateDisposition, CreateOptions, ShareAccess, FileAttribute
                        , &fileName, &fileHandle, &fileObject, NULL);

	if(NT_SUCCESS(status)) {

              ULONG writeLen = 0;
              LARGE_INTEGER byteOffset;
		PCHAR buffer = ExAllocatePoolWithTag(NonPagedPool, CryptContext->MetadataSize, SIFS_METADATA_TAG);

              if(buffer == NULL) {
                
                    goto SifsWriteSifsMetadataCloseFile;
              }

              RtlZeroMemory(buffer, CryptContext->MetadataSize);
		buffer[0] = '1';

              byteOffset.QuadPart = 0;

                
        	status = FltWriteFile(Instance, fileObject, &byteOffset, CryptContext->MetadataSize, buffer
        				, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET | FLTFL_IO_OPERATION_NON_CACHED, &writeLen, NULL, NULL );
              
		if(NT_SUCCESS(status)
                    && (writeLen == CryptContext->MetadataSize)) {

			rc = 0;
		}
        
              ExFreePoolWithTag(buffer, SIFS_METADATA_TAG);

 SifsWriteSifsMetadataCloseFile:

              if(rc == -1) {

                    FsDeleteFile(Instance, fileObject);
              }
              
		FsCloseFile(fileHandle, fileObject);              
	}
	

SifsWriteSifsMetadataCleanup:

       DbgPrint("SifsWriteSifsMetadata: %wZ, %wZ(%d), rc = %d, status = 0x%x, createDisposition = %d, createOptions = 0x%x\n"
            , &fileName, &(NameInfo->Name), NameInfo->Name.Length, rc, status, CreateDisposition, CreateOptions);
       
	if(fileName.Buffer != NULL ) {
		
		FsFreeUnicodeString(&fileName);
	}
	
	return rc;
}

int
SifsCheckFileValid(
	__in PFLT_INSTANCE Instance,
	__in PFILE_OBJECT FileObject
	)
{
	int rc = -1;

       NTSTATUS status = STATUS_SUCCESS;
       LARGE_INTEGER byteOffset;
	PCHAR buffer = ExAllocatePoolWithTag(NonPagedPool, SIFS_MINIMUM_HEADER_EXTENT_SIZE, SIFS_METADATA_TAG);

      if(buffer != NULL) {

         ULONG readLen = 0;
         
         RtlZeroMemory(buffer, SIFS_MINIMUM_HEADER_EXTENT_SIZE);

         byteOffset.QuadPart = 0;

         status = FltReadFile(Instance, FileObject, &byteOffset, SIFS_MINIMUM_HEADER_EXTENT_SIZE, buffer
				, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET | FLTFL_IO_OPERATION_NON_CACHED, &readLen, NULL, NULL );

	  if(NT_SUCCESS(status)
            && (readLen == SIFS_MINIMUM_HEADER_EXTENT_SIZE)) {

		if(buffer[0] == '1') {

			rc = 0;
		}
	  }  

          ExFreePoolWithTag(buffer, SIFS_METADATA_TAG);
      }

	return rc;
}
	
int
SifsReadSifsMetadata(
       __in PFLT_INSTANCE Instance,
	__in PFLT_FILE_NAME_INFORMATION NameInfo,
	__inout PCRYPT_CONTEXT CryptContext,
	__inout PBOOLEAN IsEmptyFile
	)
{
	int rc = -1;
	
	UNICODE_STRING fileName = { 0, 0, NULL };
	NTSTATUS status = STATUS_SUCCESS;
	HANDLE fileHandle = NULL;
	PFILE_OBJECT fileObject = NULL;
       int count = 0;
       

	if(FsGetFileNameWithoutStreamName(NameInfo, &fileName) == -1) {

            count = 1;
		goto SifsReadSifsMetadataCleanup;
	}

	status = FsOpenFile(Instance, &fileName, &fileHandle, &fileObject, NULL);

	if(NT_SUCCESS(status)) {

		FILE_STANDARD_INFORMATION fileStandardInformation ;

               count = 2;
		rc = SifsCheckFileValid(Instance, fileObject);

		if( rc == -1) {

            count = 3;
			status = FltQueryInformationFile(Instance,
										 fileObject,
										 &fileStandardInformation,
										 sizeof(fileStandardInformation),
										 FileStandardInformation,
										 NULL
										 ) ;

			if(NT_SUCCESS(status)) {

count = 4;
				if( fileStandardInformation.EndOfFile.QuadPart  == 0) {

					*IsEmptyFile = TRUE;
				}
			}
		}

		FsCloseFile(fileHandle, fileObject);
	}

SifsReadSifsMetadataCleanup:

	if(fileName.Buffer != NULL ) {
		
		FsFreeUnicodeString(&fileName);
	}

       DbgPrint("SifsReadSifsMetadata: pid = %d %wZ, rc = %d, empty = %d, count = %d\n", PsGetCurrentProcessId(), &NameInfo->Name, rc, *IsEmptyFile, count);
	
	return rc;
}

