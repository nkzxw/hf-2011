#include "fileflt.h"

PVOID
SifsAllocateCryptContext(
	VOID
	)
{
	PCRYPT_CONTEXT cryptContext = NULL;

	cryptContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(*cryptContext), CRYPT_CONTEXT_TAG);

	return cryptContext;
}

VOID
SifsFreeCryptContext(
	__in PVOID CryptContext
	)
{
	ExFreePoolWithTag(CryptContext, CRYPT_CONTEXT_TAG);
}

VOID
SifsInitializeCryptContext(
	__inout PCRYPT_CONTEXT CryptContext
	)
{
	CryptContext->ExternSize = SIFS_DEFAULT_EXTERN_SIZE;
	CryptContext->MetadataSize = SIFS_MINIMUM_HEADER_EXTENT_SIZE;

	RtlCopyMemory(CryptContext->Key, "0123456", 7);
}
