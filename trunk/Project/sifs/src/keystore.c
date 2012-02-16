#include "fileflt.h"

int
SifsGenerateKeyPacketSet(
	__inout PUCHAR DestBase,
	__in PCRYPT_CONTEXT CryptContext,
	__in PLONG Len,
	__in LONG Max
	)
{
	int rc = -1;

	*Len = 0;
	rc = 0;

	return rc;
}

int 
SifsParsePacketSet(
	__inout PCRYPT_CONTEXT CryptContext,
	__in PUCHAR Src
	)
{
	int rc = -1;

	rc = 0;

	return rc;
}