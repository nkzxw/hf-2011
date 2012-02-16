#include "fileflt.h"

static VOID
SifsSetDefaultSizes(
	__inout PCRYPT_CONTEXT CryptContext
	)
{
	CryptContext->ExternSize = SIFS_DEFAULT_EXTERN_SIZE;

	if(PAGE_CACHE_SIZE <= SIFS_MINIMUM_HEADER_EXTENT_SIZE) {
		
		CryptContext->MetadataSize = SIFS_MINIMUM_HEADER_EXTENT_SIZE;
	}else{

		CryptContext->MetadataSize = PAGE_CACHE_SIZE;
	}
}

static VOID
SifsSetDefaultHeaderData(
	__inout PCRYPT_CONTEXT CryptContext)
{
	CryptContext->MetadataSize= SIFS_MINIMUM_HEADER_EXTENT_SIZE;
}

VOID
SifsFileSizeInit(
	__in PUCHAR PageVirt, 
	__out PCRYPT_CONTEXT CryptContext
	)
{
	u64 file_size;

	file_size = get_unaligned_be64(PageVirt);
	CryptContext->ValidDataSize = file_size;
	CryptContext->Flags |= SIFS_I_SIZE_INITIALIZED;
}


static int 
SifsParseHeaderMetadata(
	__in PCRYPT_CONTEXT CryptContext,
	__in PUCHAR PageVirt, 
	__out PLONG BytesRead,
	__in LONG ValidateHeaderSize)
{
	int rc = 0;
	u32 header_extent_size = 0;
	u16 num_header_extents_at_front = 0;

	header_extent_size = get_unaligned_be32(PageVirt);
	PageVirt += sizeof(int);
	num_header_extents_at_front = get_unaligned_be16(PageVirt);
	CryptContext->MetadataSize = (((LONG)num_header_extents_at_front
				     * (LONG)header_extent_size));
	(*BytesRead) = (sizeof(int) + sizeof(short int));
	if ((ValidateHeaderSize == SIFS_VALIDATE_HEADER_SIZE)
	    && (CryptContext->MetadataSize
		< SIFS_MINIMUM_HEADER_EXTENT_SIZE)) {
		rc = -1;
	}
	return rc;
}

static int 
SifsValidateMarker(
	__in PUCHAR Data
	)
{
	int rc = -1;
	
	u32 m_1, m_2;

	m_1 = get_unaligned_be32(Data);
	m_2 = get_unaligned_be32(Data + 4);
	if ((m_1 ^ MAGIC_SIFS_MARKER) == m_2)
		rc = 0;
	
	return rc;
}

struct sifs_flag_map_elem {
	u32 file_flag;
	u32 local_flag;
};

/* Add support for additional flags by adding elements here. */
static struct sifs_flag_map_elem sifs_flag_map[] = {
	{0x00000001, SIFS_ENABLE_HMAC},
	{0x00000002, SIFS_ENCRYPTED},
	{0x00000004, SIFS_METADATA_IN_XATTR},
	{0x00000008, SIFS_ENCRYPT_FILENAMES}
};

static int 
SifsProcessFlags(
	__inout PCRYPT_CONTEXT CryptContext,
	__in PUCHAR PageVirt, 
	__out PLONG BytesRead
	)
{
	int rc = 0;
	int i = 0;
	u32 flags = 0;

	flags = get_unaligned_be32(PageVirt);
	for (i = 0; i < ((sizeof(sifs_flag_map)
			  / sizeof(struct sifs_flag_map_elem))); i++)
		if (flags & sifs_flag_map[i].file_flag) {
			CryptContext->Flags |= sifs_flag_map[i].local_flag;
		} else
			CryptContext->Flags &= ~(sifs_flag_map[i].local_flag);
	/* Version is in top 8 bits of the 32-bit flag vector */
	CryptContext->FileVersion = ((flags >> 24) & 0xFF);
	(*BytesRead) = 4;
	return rc;
}

static VOID
SifsWriteMarker(
	__inout PUCHAR PageVirt,
	__out   PULONG  WriteSize
	)
{
	u32 m_1, m_2;

	FsGetRandBytes(&m_1, (MAGIC_SIFS_MARKER_SIZE_BYTES / 2));
	m_2 = (m_1 ^ MAGIC_SIFS_MARKER);
	put_unaligned_be32(m_1, PageVirt);
	PageVirt += (MAGIC_SIFS_MARKER_SIZE_BYTES / 2);
	put_unaligned_be32(m_2, PageVirt);
	(*WriteSize) = MAGIC_SIFS_MARKER_SIZE_BYTES;
}

static VOID 
SifsWriteCryptStatFlags(
	__inout PUCHAR PageVirt,
	__in PCRYPT_CONTEXT CryptContext,
	__out   PLONG Written
	)
{
	u32 flags = 0;
	int i = 0;

	for (i = 0; i < ((sizeof(sifs_flag_map)
			  / sizeof(struct sifs_flag_map_elem))); i++)
		if (CryptContext->Flags & sifs_flag_map[i].local_flag)
			flags |= sifs_flag_map[i].file_flag;
	/* Version is in top 8 bits of the 32-bit flag vector */
	flags |= ((((u8)CryptContext->FileVersion) << 24) & 0xFF000000);
	put_unaligned_be32(flags, PageVirt);
	(*Written) = 4;
}

VOID
SifsWriteHeaderMetadata(
	__inout PUCHAR Virt,
	__in PCRYPT_CONTEXT CryptContext,
	__out PLONG Written
	)
{
	u32 header_extent_size = 0;
	u16 num_header_extents_at_front = 0;

	header_extent_size = (u32)CryptContext->ExternSize;
	num_header_extents_at_front =
		(u16)(CryptContext->MetadataSize / CryptContext->ExternSize);
	put_unaligned_be32(header_extent_size, Virt);
	Virt += 4;
	put_unaligned_be16(num_header_extents_at_front, Virt);
	(*Written) = 6;
}

int
SifsWriteHeadersVirt(
	__inout PUCHAR PageVirt,
	__in LONG Max,
	__out PLONG Size,
	__in PCRYPT_CONTEXT CryptContext
	)
{
	int rc = -1;

	LONG written = 0;
	LONG offset = 0;

	offset = SIFS_FILE_SIZE_BYTES;
	SifsWriteMarker((PageVirt + offset), &written);

#if 0
	offset += written;
	SifsWriteCryptStatFlags((PageVirt + offset), CryptContext,
					&written);
	offset += written;
	SifsWriteHeaderMetadata((PageVirt + offset), CryptContext,
				       &written);
	offset += written;
	rc = SifsGenerateKeyPacketSet((PageVirt + offset), CryptContext,
					      &written,
					      Max - offset);
#endif
	if (Size) {
		offset += written;
		*Size = offset;
	}

	rc = 0;

	return rc;
}

int
SifsReadHeadersVirt(
	__in PUCHAR PageVirt,
	__inout PCRYPT_CONTEXT CryptContext,
	__in  LONG ValidateHeaderSize
	)
{
	int rc = -1;

	int offset = 0;
	int bytes_read = 0;

	SifsSetDefaultSizes(CryptContext);

	offset = SIFS_FILE_SIZE_BYTES;
	rc = SifsValidateMarker(PageVirt + offset);
	if (rc)
		goto SifsReadHeadersVirtCleanup;
#if 0
	if (!(CryptContext->Flags & SIFS_I_SIZE_INITIALIZED))
		SifsFileSizeInit(PageVirt, CryptContext);
	offset += MAGIC_SIFS_MARKER_SIZE_BYTES;
	rc = SifsProcessFlags(CryptContext, (PageVirt + offset),
				    &bytes_read);
	if (rc) {
		goto SifsReadHeadersVirtCleanup;
	}
	if (CryptContext->FileVersion > SIFS_SUPPORTED_FILE_VERSION) {
		
		rc = -1;
		goto SifsReadHeadersVirtCleanup;
	}
	offset += bytes_read;
	if (CryptContext->FileVersion >= 1) {
		rc = SifsParseHeaderMetadata(CryptContext, (PageVirt + offset),
					   &bytes_read, ValidateHeaderSize);
		
		offset += bytes_read;
	} else
		SifsSetDefaultHeaderData(CryptContext);
	rc = SifsParsePacketSet(CryptContext, (PageVirt + offset));

#endif
SifsReadHeadersVirtCleanup:
	
	return rc;
}

int
SifsQuickCheckValidate_i(
	__in PUCHAR Buffer
	)
{
	int rc = -1;
	int offset = 0;

	offset = SIFS_FILE_SIZE_BYTES;
	rc = SifsValidateMarker(Buffer + offset);

	return rc;
}

VOID
SifsInitializeCryptContext(
	__inout PCRYPT_CONTEXT CryptContext
	)
{
	SifsSetDefaultSizes(CryptContext);

	RtlCopyMemory(CryptContext->Key, "0123456", 7);
}
