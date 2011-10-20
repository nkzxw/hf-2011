/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_DRIVER_IMAGES_H
#define _WEHNTRUST_DRIVER_IMAGES_H

/*
 * This structure wrappers the checksum that is used to determine whether or not
 * an image has changed with what was originally used to calculate the address
 * and jump tables
 */
typedef struct _IMAGE_CHECKSUM
{
	IMAGE_CHECKSUM_TYPE Type;

	union
	{
		ULONG            Ror16;
		UCHAR            Sha1[SHA1_HASH_SIZE];
	} Expected;

	union
	{
		ULONG            Ror16;
		UCHAR            Sha1[SHA1_HASH_SIZE];
	} Actual;
} IMAGE_CHECKSUM, *PIMAGE_CHECKSUM;

/*
 * This structure holds the virtual address of a symbol that requires a jump
 * table entry.
 */
typedef struct _IMAGE_ADDRESS
{
	ULONG               Address;
	ULONG               RegionBase;
	ULONG               RegionSize;
	PUCHAR              Prepend;
	ULONG               PrependSize;
} IMAGE_ADDRESS, *PIMAGE_ADDRESS;

/*
 * This structure holds zero or more IMAGE_ADDRESS's that are used when building
 * a jump table.
 */
typedef struct _IMAGE_ADDRESS_TABLE
{
	PIMAGE_ADDRESS      Table;
	ULONG               TableSize;
} IMAGE_ADDRESS_TABLE, *PIMAGE_ADDRESS_TABLE;

/*
 * The image structure is a logical wrapper around an image that has a
 * predefined set of rules and requirements for building the jump table and
 * validating its checksum.  This information is gathered from the registry at
 * boot time.
 */
typedef struct _IMAGE
{
	LIST_ENTRY          Entry;
	ULONG               References;
	IMAGE_ADDRESS_TABLE AddressTable;
	UNICODE_STRING      Name;
	UNICODE_STRING      Path;
	IMAGE_CHECKSUM      Checksum;
	ULONG               OriginalBaseAddress;
	ULONG               Flags;
} IMAGE, *PIMAGE;

PIMAGE CreateImage(
		IN PUNICODE_STRING ImageName);

VOID InsertImageEntry(
		IN PIMAGE Image);
PIMAGE FindImageEntryByPath(
		IN PUNICODE_STRING Path);
VOID RemoveImageEntry(
		IN PIMAGE Image);

VOID LockImages();
VOID UnlockImages();

PIMAGE ReferenceImage(
		IN PIMAGE Image);
BOOLEAN DereferenceImage(
		IN PIMAGE Image);

NTSTATUS InitializeImageTable();
NTSTATUS RefreshImageTable();
VOID FlushImageTable();

NTSTATUS ValidateImageChecksum(
		IN PIMAGE Image);

NTSTATUS BuildJumpTableForImage(
		IN PIMAGE Image,
		IN PPROCESS_OBJECT ProcessObject OPTIONAL,
		IN PVOID NewBase);
NTSTATUS BuildJumpTableSingleMapping(
		IN PPROCESS_OBJECT ProcessObject OPTIONAL,
		IN PVOID OldAddress,
		IN PVOID NewAddress);

#endif
