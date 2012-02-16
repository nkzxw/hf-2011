#ifndef __FILEFLT_SIFS_H__
#define __FILEFLT_SIFS_H__


//--------------------------------------------------------------
//sifs

FLT_PREOP_CALLBACK_STATUS
SifsPreCreate(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext,
    __in PVOLUME_CONTEXT VolumeContext
    );

FLT_PREOP_CALLBACK_STATUS
SifsPreCleanup(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
SifsPreClose(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
SifsPreRead(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
SifsPreWrite(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
SifsPreQueryInformation (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
SifsPreSetInformation(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
SifsPreNetworkQueryOpen(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
SifsPreDirCtrlBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

ULONG
SifsValidateFileSize(
	__in PSTREAM_CONTEXT StreamContext
	);


//--------------------------------------------------------------
//crypto

VOID
SifsInitializeCryptContext(
	__inout PCRYPT_CONTEXT CryptContext
	);

int
SifsWriteHeadersVirt(
	__inout PUCHAR PageVirt,
	__in LONG Max,
	__out PLONG Size,
	__in PCRYPT_CONTEXT CryptContext
	);

int
SifsReadHeadersVirt(
	__in PUCHAR PageVirt,
	__inout PCRYPT_CONTEXT CryptContext,
	__in  LONG ValidateHeaderSize
	);

int
SifsQuickCheckValidate_i(
	__in PUCHAR Buffer
	);
//--------------------------------------------------------------
//read_write


int
SifsWriteSifsMetadata(
       __inout PFLT_INSTANCE Instance,
       __in ULONG                     DesiredAccess,
        __in ULONG                     CreateDisposition,
        __in ULONG                     CreateOptions,
        __in ULONG                     ShareAccess,
        __in ULONG                     FileAttribute,
	__in PFLT_FILE_NAME_INFORMATION NameInfo,
	__inout PCRYPT_CONTEXT CryptContext
	);

int
SifsReadSifsMetadata(
       __in PFLT_INSTANCE Instance,
	__in PFILE_OBJECT FileObject,	
	__inout PSTREAM_CONTEXT StreamContext
	);

int
SifsQuickCheckValidate(
       __in PFLT_INSTANCE Instance,
	__in PUNICODE_STRING FileName,
	__inout PCRYPT_CONTEXT CryptContext,
	__inout PBOOLEAN IsEmptyFile,
	__in LONG Aligned
	);

int
SifsQuickCheckValidateSifs(
	__in PFLT_INSTANCE Instance,
	__in PFILE_OBJECT FileObject,
	__out PUCHAR  PageVirt,
	__in LONG PageVirtLen
	);

int
SifsWriteFileSize(
	__in PFLT_INSTANCE Instance,
	__in PUNICODE_STRING FileName,
	__inout PUCHAR Metadata,
	__in LONG MetadataLen,
	__in LONGLONG  FileSize
	);

//-----------------------------------------------------------------------
//keystore
int
SifsGenerateKeyPacketSet(
	__inout PUCHAR DestBase,
	__in PCRYPT_CONTEXT CryptContext,
	__in PLONG Len,
	__in LONG Max
	);

int 
SifsParsePacketSet(
	__inout PCRYPT_CONTEXT CryptContext,
	__in PUCHAR Src
	);

#endif /* __FILEFLT_SIFS_H__ */
