#include "fileflt.h"

ULONG
SifsValidateFileSize(
	__in PSTREAM_CONTEXT StreamContext
	)
{
	ULONG rc = 0;

	if(StreamContext->CryptedFile == TRUE) {
		
		rc = StreamContext->FileSize.QuadPart - StreamContext->CryptContext.MetadataSize;
	}else{

		rc = StreamContext->FileSize.QuadPart;
	}

	return rc;
}

FLT_PREOP_CALLBACK_STATUS
SifsPreCreate(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext,
    __in PVOLUME_CONTEXT VolumeContext
    )
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SifsPreCleanup(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SifsPreClose(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SifsPreRead(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SifsPreWrite(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SifsPreQueryInformation (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SifsPreSetInformation(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SifsPreNetworkQueryOpen(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

	return retValue;
}

FLT_PREOP_CALLBACK_STATUS
SifsPreDirCtrlBuffers(
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
	FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;

	return retValue;
}

