//********************************************************************
//	created:	13:8:2008   4:41
//	file:		ntfsboot.h
//	author:		tiamo
//	purpose:	ntfsboot
//********************************************************************

#pragma once

//
// mcb entries
//
#define MAXIMUM_NUMBER_OF_MCB_ENTRIES					16

//
// collection of vbo - lbo pairs,size = 108
//
typedef struct _NTFS_MCB
{
	//
	// the number of entries in use by the mcb,the last InUse entry Lbo's value is ignored, because it is only used to give the length of the previous run.
	//
	ULONG												InUse;

	//
	// vbo,offset = 8
	//
	LONGLONG											Vbo[MAXIMUM_NUMBER_OF_MCB_ENTRIES];

	//
	// lbo,offset = 88
	//
	LONGLONG											Lbo[MAXIMUM_NUMBER_OF_MCB_ENTRIES];
}NTFS_MCB,*PNTFS_MCB;

//
// ntfs file context structure and the attribute context structure.size = 0x28
//
typedef struct _NTFS_FILE_CONTEXT
{
	//
	//  attribute type,offset = 0
	//
	ULONG												TypeCode;

	//
	// attribute's data size,offset = 8
	//
	LONGLONG											DataSize;

	//
	// file record,offset = 10
	//
	LONGLONG											FileRecord;

	//
	// and offset in the file record,offset = 18
	//
	USHORT												FileRecordOffset;

	//
	// resident attribute or not,offset = 1a
	//
	BOOLEAN												IsAttributeResident;

	//
	// if the attribute is compressed then it contains the value to pass to the decompression engine.offset = 1c
	//
	USHORT												CompressionFormat;

	//
	// the number of bytes in each unit of compression.offset = 20
	//
	ULONG												CompressionUnit;
}NTFS_FILE_CONTEXT,NTFS_ATTRIBUTE_CONTEXT,*PNTFS_FILE_CONTEXT,*PNTFS_ATTRIBUTE_CONTEXT;

//
// ntfs volume structure context,size = 1260
//
typedef struct _NTFS_STRUCTURE_CONTEXT
{
	//
	// device id,offset = 0
	//
	ULONG												DeviceId;

	//
	// byte per cluster,offset = 4
	//
	ULONG												BytesPerCluster;

	//
	// bytes per file record,offset = 8
	//
	ULONG												BytesPerFileRecord;

	//
	// describe the $DATA stream for the the MFT,offset = 10
	//
	NTFS_ATTRIBUTE_CONTEXT								MftAttributeContext;

	//
	// holds the base of the mft,offset = 38
	//
	NTFS_MCB											MftBaseMcb;

	//
	// cached mcb file record,offset = 140
	//
	LONGLONG											CachedMcbFileRecord[16];

	//
	// cached offset,offset = 1c0
	//
	USHORT												CachedMcbFileRecordOffset[16];

	//
	// cached mcb,offset = 1e0
	//
	NTFS_MCB											CachedMcb[16];
}NTFS_STRUCTURE_CONTEXT,*PNTFS_STRUCTURE_CONTEXT;

//
// intitialize ntfs file system
//
ARC_STATUS NtfsInitialize();

//
// check ntfs file system
//
struct _BL_DEVICE_ENTRY_TABLE* IsNtfsFileStructure(__in ULONG DeviceId,__out PVOID OpaqueStructureContext);