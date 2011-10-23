//********************************************************************
//	created:	17:9:2008   23:44
//	file: 		ntfsboot.cpp
//	author:		tiamo
//	purpose:	ntfs boot
//********************************************************************

#include "stdafx.h"

#if _NTFS_SUPPORT_

//
// file record size
//
#define MAXIMUM_FILE_RECORD_SIZE						4096

//
// index allocation size
//
#define MAXIMUM_INDEX_ALLOCATION_SIZE					4096

//
// compression unit size
//
#define MAXIMUM_COMPRESSION_UNIT_SIZE					65536

#define $UNUSED											0x00
#define $STANDARD_INFORMATION							0x10
#define $ATTRIBUTE_LIST									0x20
#define $FILE_NAME										0x30
#define $OBJECT_ID										0x40
#define $SECURITY_DESCRIPTOR							0x50
#define $VOLUME_NAME									0x60
#define $VOLUME_INFORMATION								0x70
#define $DATA											0x80
#define $INDEX_ROOT										0x90
#define $INDEX_ALLOCATION								0xa0
#define $BITMAP											0xb0
#define $SYMBOLIC_LINK									0xc0
#define $EA_INFORMATION									0xd0
#define $EA												0xe0
#define $FIRST_USER_DEFINED_ATTRIBUTE					0x100
#define $END											0xFFFFFFFF

//
// read only file
//
#define FAT_DIRENT_ATTR_READ_ONLY						0x01

//
// hidden file
//
#define FAT_DIRENT_ATTR_HIDDEN							0x02

//
// system file
//
#define FAT_DIRENT_ATTR_SYSTEM							0x04

//
// volume id
//
#define FAT_DIRENT_ATTR_VOLUME_ID						0x08

//
// archive
//
#define FAT_DIRENT_ATTR_ARCHIVE							0x20

//
// device
//
#define FAT_DIRENT_ATTR_DEVICE							0x40

//
// directory
//
#define DUP_FILE_NAME_INDEX_PRESENT						0x10000000

//
// record inuse
//
#define FILE_RECORD_SEGMENT_IN_USE						0x0001

//
// file name is indexed
//
#define FILE_FILE_NAME_INDEX_PRESENT					0x0002

//
// resident form
//
#define RESIDENT_FORM									0x00

//
// non-resident form
//
#define NONRESIDENT_FORM								0x01

//
// compression mask
//
#define ATTRIBUTE_FLAG_COMPRESSION_MASK					0xff

//
// none compression
//
#define COMPRESSION_FORMAT_NONE							0x0000

//
// default compression
//
#define COMPRESSION_FORMAT_DEFAULT						0x0001

//
// lzx compression
//
#define COMPRESSION_FORMAT_LZNT1						0x0002

//
// sparse
//
#define ATTRIBUTE_FLAG_SPARSE							0x8000

//
// this attribute is indexed.
//
#define RESIDENT_FORM_INDEXED							0x01

//
// intermediate index node
//
#define INDEX_NODE										0x01

//
// this entry is currently in the intermediate node form, i.e., it has a Vcn at the end.
//
#define INDEX_ENTRY_NODE								0x0001

//
// this entry is the special END record for the Index or Index Allocation buffer.
//
#define INDEX_ENTRY_END									0x0002

//
// upcase
//
#define NtfsUpCase(C)									((C) >= 'a' && (C) <= 'z' ? ((C) - 'a' + 'A') : (C))

//
// get first attribute
//
#define NtfsFirstAttribute(FRS)							Add2Ptr((FRS),(FRS)->FirstAttributeOffset,PATTRIBUTE_RECORD_HEADER)

//
// get next record
//
#define NtfsGetNextRecord(STRUCT)						Add2Ptr(STRUCT,(STRUCT)->RecordLength,PATTRIBUTE_RECORD_HEADER)

//
// get value
//
#define NtfsGetValue(ATTRIBUTE)							Add2Ptr((ATTRIBUTE),(ATTRIBUTE)->Form.Resident.ValueOffset,PVOID)

//
// read attribute
//
#define NtfsReadAttribute(A,B,C,D,E)					((B)->IsAttributeResident ? NtfsReadResidentAttribute(A,B,C,D,E) : NtfsReadNonresidentAttribute(A,B,C,D,E))

//
// write attribute
//
#define NtfsWriteAttribute(A,B,C,D,E)					((B)->IsAttributeResident ? EROFS : NtfsWriteNonresidentAttribute(A,B,C,D,E))

//
// pin record
//
#define NtfsPinFileRecord(I)							{NtfsFileRecordBufferPinned[I] += 1;}

//
// unpin record
//
#define NtfsUnpinFileRecord(I)							{NtfsFileRecordBufferPinned[I] -= 1;}

//
// attribute type code
//
typedef ULONG											ATTRIBUTE_TYPE_CODE;

//
// collation rule
//
typedef ULONG											COLLATION_RULE;

//
// lsn
//
typedef LARGE_INTEGER									LSN,*PLSN;

//
// lcn
//
typedef LONGLONG										LCN,*PLCN;

//
// vcn
//
typedef LONGLONG										VCN,*PVCN;

//
// lbo
//
typedef LONGLONG										LBO,*PLBO;


//
// vbo
//
typedef LONGLONG										VBO,*PVBO;

//
// update sequence number
//
typedef USHORT											UPDATE_SEQUENCE_NUMBER,*PUPDATE_SEQUENCE_NUMBER;

//
// update seq array
//
typedef UPDATE_SEQUENCE_NUMBER							UPDATE_SEQUENCE_ARRAY[1];

//
// ptr
//
typedef UPDATE_SEQUENCE_ARRAY							*PUPDATE_SEQUENCE_ARRAY;

#include <pshpack4.h>

//
// bios parameter block
//
typedef struct BIOS_PARAMETER_BLOCK
{
	//
	// bytes per sector
	//
	USHORT												BytesPerSector;

	//
	// sector per cluster
	//
	UCHAR												SectorsPerCluster;

	//
	// reserved sectors
	//
	USHORT												ReservedSectors;

	//
	// fat count
	//
	UCHAR												Fats;

	//
	// root entries
	//
	USHORT												RootEntries;

	//
	// sectors
	//
	USHORT												Sectors;

	//
	// media
	//
	UCHAR												Media;

	//
	// sectors per fat
	//
	USHORT												SectorsPerFat;

	//
	// sectors per track
	//
	USHORT												SectorsPerTrack;

	//
	// heads
	//
	USHORT												Heads;

	//
	// hidden sectors
	//
	ULONG												HiddenSectors;

	//
	// large sectors
	//
	ULONG												LargeSectors;

}BIOS_PARAMETER_BLOCK,*PBIOS_PARAMETER_BLOCK;

//
// packed bios parameter block,sizeof = 0x019
//
typedef struct _PACKED_BIOS_PARAMETER_BLOCK
{
	//
	// bytes per sector,offset = 0x000
	//
	UCHAR												BytesPerSector[2];

	//
	// sectors per cluster,offset = 0x002
	//
	UCHAR												SectorsPerCluster[1];

	//
	// reserved sectors,offset = 0x003 (zero)
	//
	UCHAR												ReservedSectors[2];

	//
	// fat count,offset = 0x005 (zero)
	//
	UCHAR												Fats[1];

	//
	// root entries,offset = 0x006 (zero)
	//
	UCHAR												RootEntries[2];

	//
	// sectors,offset = 0x008 (zero)
	//
	UCHAR												Sectors[2];

	//
	// media,offset = 0x00a
	//
	UCHAR												Media[1];

	//
	// sectors per fat,offset = 0x00B (zero)
	//
	UCHAR												SectorsPerFat[2];

	//
	// sectors per track,offset = 0x00D
	//
	UCHAR												SectorsPerTrack[2];

	//
	// heads,offset = 0x00F
	//
	UCHAR												Heads[2];

	//
	// hidden sectors,offset = 0x011 (zero)
	//
	UCHAR												HiddenSectors[4];

	//
	// large sectors,offset = 0x015 (zero)
	//
	UCHAR												LargeSectors[4];

}PACKED_BIOS_PARAMETER_BLOCK,*PPACKED_BIOS_PARAMETER_BLOCK;

//
// sector
//
typedef struct _PACKED_BOOT_SECTOR
{
	//
	// jmp code,offset = 0x000
	//
	UCHAR												Jump[3];

	//
	// oem id,offset = 0x003
	//
	UCHAR												Oem[8];

	//
	// bios parameter block,offset = 0x00B
	//
	PACKED_BIOS_PARAMETER_BLOCK							PackedBpb;

	//
	// offset = 0x024
	//
	UCHAR												Unused[4];

	//
	// sectors,offset = 0x028
	//
	LONGLONG											NumberSectors;

	//
	// mft lcn,offset = 0x030
	//
	LCN													MftStartLcn;

	//
	// mft2 lcn,offset = 0x038
	//
	LCN													Mft2StartLcn;

	//
	// clusters per file record segment,offset = 0x040
	//
	CHAR												ClustersPerFileRecordSegment;

	//
	// padding
	//
	UCHAR												Reserved0[3];

	//
	// clusters per index allocation buffer,offset = 0x44
	//
	CHAR												DefaultClustersPerIndexAllocationBuffer;

	//
	// padding
	//
	UCHAR												Reserved1[3];

	//
	// serial,offset = 0x048
	//
	LONGLONG											SerialNumber;

	//
	// checksum,offset = 0x050
	//
	ULONG												Checksum;

	//
	// boot code,offset = 0x054
	//
	UCHAR												BootStrap[0x200 - 0x054];

}PACKED_BOOT_SECTOR,*PPACKED_BOOT_SECTOR;

//
// the MFT Segment Reference is an address in the MFT tagged with a circularly reused sequence number set at the time that the MFT Segment Reference was valid.
//
typedef struct _MFT_SEGMENT_REFERENCE
{
	//
	// low part of first a 48 bit segment number,offset = 0x000
	//
	ULONG												SegmentNumberLowPart;

	//
	// high part,offset = 0x004
	//
	USHORT												SegmentNumberHighPart;

	//
	// a 16 bit nonzero sequence number,offset = 0x006
	//
	USHORT												SequenceNumber;

}MFT_SEGMENT_REFERENCE,*PMFT_SEGMENT_REFERENCE,FILE_REFERENCE,*PFILE_REFERENCE;

//
// Multi-Sector Header
//
typedef struct _MULTI_SECTOR_HEADER
{
	//
	// space for a four-character signature,offset = 0
	//
	UCHAR												Signature[4];

	//
	// offset to Update Sequence Array, from start of structure,offset = 4
	//
	USHORT												UpdateSequenceArrayOffset;

	//
	// size of Update Sequence Array,offset = 8
	//
	USHORT												UpdateSequenceArraySize;

}MULTI_SECTOR_HEADER,*PMULTI_SECTOR_HEADER;

//
// File Record Segment
//
typedef struct _FILE_RECORD_SEGMENT_HEADER
{
	//
	// Multi-Sector Header,offset = 0x000
	//
	MULTI_SECTOR_HEADER									MultiSectorHeader;

	//
	// Log File Sequence Number of last logged update to this File Record Segment,offset = 0x008
	//
	LSN													Lsn;

	//
	// Sequence Number,offset = 0x010
	//
	USHORT												SequenceNumber;

	//
	// this is the count of the number of references which exist for this segment, from an INDEX_xxx attribute,offset = 0x012
	//
	USHORT												ReferenceCount;

	//
	// offset to the first Attribute record in bytes,offset = 0x014
	//
	USHORT												FirstAttributeOffset;

	//
	// FILE_xxx flags,offset = 0x016
	//
	USHORT												Flags;

	//
	// first free byte available for attribute storage, from start of this header.aligned to a quad-word boundary,offset = x0018
	//
	ULONG												FirstFreeByte;

	//
	// total bytes available in this file record segment, from the start of this header,offset = 0x01c
	//
	ULONG												BytesAvailable;

	//
	// this is a File Reference to the Base file record segment for this file,offset = 0x020
	//
	FILE_REFERENCE										BaseFileRecordSegment;

	//
	// this is the attribute instance number to be used when creating an attribute,offset = 0x028
	//
	USHORT												NextAttributeInstance;

	//
	// Update Sequence Array to protect multi-sector transfers of the File Record Segment,offset = 0x02a
	//
	UPDATE_SEQUENCE_ARRAY								UpdateArrayForCreateOnly;

}FILE_RECORD_SEGMENT_HEADER,*PFILE_RECORD_SEGMENT_HEADER;

//
// Attribute Record.
//
typedef struct _ATTRIBUTE_RECORD_HEADER
{
	//
	// Attribute Type Code,offset = 0x000
	//
	ATTRIBUTE_TYPE_CODE									TypeCode;

	//
	// Length of this Attribute Record in bytes,offset = 0x004
	//
	ULONG												RecordLength;

	//
	// Attribute Form Code,offset = 0x008
	//
	UCHAR												FormCode;

	//
	// Length of the optional attribute name in characters, or 0 if there is none,offset = 0x009
	//
	UCHAR												NameLength;

	//
	// Offset to the attribute name from start of attribute record, in bytes,offset = 0x00a
	//
	USHORT												NameOffset;

	//
	// ATTRIBUTE_xxx flags,offset = 0x00c
	//
	USHORT												Flags;

	//
	// the file-record-unique attribute instance number for this attribute,offset = 0x00e
	//
	USHORT												Instance;

	//
	// the following union handles the cases distinguished by the Form Code.
	//
	union
	{
		//
		// Resident Form,Attribute resides in file record segment.
		//
		struct
		{
			//
			// Length of attribute value in bytes,offset = 0x010
			//
			ULONG										ValueLength;

			//
			// Offset to value from start of attribute record, in bytes,offset = 0x014
			//
			USHORT										ValueOffset;

			//
			// RESIDENT_FORM_xxx Flags,offset = 0x016
			//
			UCHAR										ResidentFlags;

			//
			// Reserved,offset = 0x017
			//
			UCHAR										Reserved;

		}Resident;

		//
		// Nonresident Form,Attribute resides in separate stream.
		//
		struct
		{
			//
			// Lowest VCN covered by this attribute record,offset = 0x010
			//
			VCN											LowestVcn;

			//
			// Highest VCN covered by this attribute record,offset = 0x018
			//
			VCN											HighestVcn;

			//
			// Offset to the Mapping Pairs Array  (defined below), in bytes, from the start of the attribute record,offset = 0x020
			//
			USHORT										MappingPairsOffset;

			//
			// Unit of Compression size for this stream, expressed as a log of the cluster size,offset = 0x022
			//
			UCHAR										CompressionUnit;

			//
			// Reserved to get to quad word boundary,offset = 0x023
			//
			UCHAR										Reserved[5];

			//
			// Allocated Length of the file in bytes,offset = 0x028
			//
			LONGLONG									AllocatedLength;

			//
			// File Size in bytes,offset = 0x030
			//
			LONGLONG									FileSize;

			//
			// Valid Data Length (highest initialized byte + 1),offset = 0x038
			//
			LONGLONG									ValidDataLength;

			//
			// Totally allocated,offset = 0x040
			//
			LONGLONG									TotalAllocated;

			//
			// Mapping Pairs Array, starting at the offset stored above.
			//
		}Nonresident;
	}Form;
}ATTRIBUTE_RECORD_HEADER,*PATTRIBUTE_RECORD_HEADER;

//
// attribute list entry
//
typedef struct _ATTRIBUTE_LIST_ENTRY
{

	//
	// Attribute Type Code, the first key on which this list is ordered,offset = 0x000
	//
	ATTRIBUTE_TYPE_CODE									AttributeTypeCode;

	//
	// size of this record in bytes, including the optional name appended to this structure,offset = 0x004
	//
	USHORT												RecordLength;

	//
	// Length of attribute name,offset = 0x006
	//
	UCHAR												AttributeNameLength;

	//
	// name offset,offset = 0x007
	//
	UCHAR												AttributeNameOffset;

	//
	// Lowest Vcn for this attribute,offset = 0x008
	//
	VCN													LowestVcn;

	//
	// Reference to the MFT segment in which the attribute resides,offset = 0x010
	//
	MFT_SEGMENT_REFERENCE								SegmentReference;

	//
	// the file-record-unique attribute instance number for this attribute,offset = 0x018
	//
	USHORT												Instance;

	//
	// when creating an attribute list entry, start the name here,(When reading one, use the AttributeNameOffset field.),offset = 0x01a
	//
	WCHAR												AttributeName[1];

}ATTRIBUTE_LIST_ENTRY,*PATTRIBUTE_LIST_ENTRY;

//
// Index Header for Index Root and Index Allocation Buffers,sizeof = 0x010
//
typedef struct _INDEX_HEADER
{
	//
	// offset from the start of this structure to the first Index Entry,offset = 0x000
	//
	ULONG												FirstIndexEntry;

	//
	// offset from the start of the first index entry to the first (quad-word aligned) free byte,offset = 0x004
	//
	ULONG												FirstFreeByte;

	//
	// total number of bytes available, from the start of the first index entry,offset = 0x008
	//
	ULONG												BytesAvailable;

	//
	// INDEX_xxx flags,offset = 0x00c
	//
	UCHAR												Flags;

	//
	// reserved to round up to quad word boundary,offset = 0x00D
	//
	UCHAR												Reserved[3];

}INDEX_HEADER,*PINDEX_HEADER;

//
// Index Root attribute,sizeof = 0x020
//
typedef struct _INDEX_ROOT
{
	//
	// Attribute Type Code of the attribute being indexed,offset = 0x000
	//
	ATTRIBUTE_TYPE_CODE									IndexedAttributeType;

	//
	// collation rule for this index,offset = 0x004
	//
	COLLATION_RULE										CollationRule;

	//
	// size of Index Allocation Buffer in bytes,offset = 0x008
	//
	ULONG												BytesPerIndexBuffer;

	//
	// size of Index Allocation Buffers in units of blocks,offset = 0x00c
	//
	UCHAR												BlocksPerIndexBuffer;

	//
	// reserved to round to quad word boundary,offset = 0x00D
	//
	UCHAR												Reserved[3];

	//
	// Index Header to describe the Index Entries which follow,offset = 0x010
	//
	INDEX_HEADER										IndexHeader;

}INDEX_ROOT,*PINDEX_ROOT;

//
// Index Allocation record
//
typedef struct _INDEX_ALLOCATION_BUFFER
{
	//
	// Multi-Sector Header,offset = 0x000
	//
	MULTI_SECTOR_HEADER									MultiSectorHeader;

	//
	//  Log File Sequence Number,offset = 0x008
	//
	LSN													Lsn;

	//
	// we store the index block of this Index Allocation buffer for convenience and possible consistency checking,offset = 0x010
	//
	VCN													ThisBlock;

	//
	// Index Header to describe the Index Entries which follow,offset = 0x018
	//
	INDEX_HEADER										IndexHeader;

	//
	// Update Sequence Array to protect multi-sector transfers of the Index Allocation Buffer,offset = 0x028
	//
	UPDATE_SEQUENCE_ARRAY								UpdateSequenceArray;

}INDEX_ALLOCATION_BUFFER,*PINDEX_ALLOCATION_BUFFER;

//
// Index Entry,sizeof = 0x010
//
typedef struct _INDEX_ENTRY
{
	//
	// reference to file containing the attribute with this attribute value,offset = 0x000
	//
	FILE_REFERENCE										FileReference;

	//
	// length of this index entry, in bytes,offset = 0x008
	//
	USHORT												Length;

	//
	// length of attribute value, in bytes,offset = 0x00a
	//
	USHORT												AttributeLength;

	//
	// INDEX_ENTRY_xxx Flags,offset = 0x00c
	//
	USHORT												Flags;

	//
	// reserved to round to quad-word boundary,offset = 0x00e
	//
	USHORT												Reserved;

	//
	// if this Index Entry is an intermediate node in the tree, as determined by the INDEX_xxx flags,
	// then a VCN  is stored at the end of this entry at Length - sizeof(VCN).
	//
}INDEX_ENTRY,*PINDEX_ENTRY;

//
// Duplicated Information,sizeof = 0x038
//
typedef struct _DUPLICATED_INFORMATION
{
	//
	// file creation time,offset = 0x000
	//
	LONGLONG											CreationTime;

	//
	// last time the DATA attribute was modified,offset = 0x008
	//
	LONGLONG											LastModificationTime;

	//
	// last time any attribute was modified,offset = 0x010
	//
	LONGLONG											LastChangeTime;

	//
	// last time the file was accessed,offset = 0x018
	//
	LONGLONG											LastAccessTime;

	//
	// allocated Length of the file in bytes,offset = 0x020
	//
	LONGLONG											AllocatedLength;

	//
	// file Size in bytes,offset = 0x028
	//
	LONGLONG											FileSize;

	//
	// file attributes,offset = 0x030
	//
	ULONG												FileAttributes;

	//
	// the size of buffer needed to pack these Ea's,offset = 0x034
	//
	USHORT												PackedEaSize;

	//
	// reserved for quad word alignment,offset = 0x036
	//
	USHORT												Reserved;

}DUPLICATED_INFORMATION,*PDUPLICATED_INFORMATION;

//
// File Name in an Index Entry
//
typedef struct _FILE_NAME
{
	//
	// this is a File Reference to the directory file which indexes to this name,offset = 0x000
	//
	FILE_REFERENCE										ParentDirectory;

	//
	// information for faster directory operations,offset = 0x008
	//
	DUPLICATED_INFORMATION								Info;

	//
	// length of the name to follow, in (Unicode) characters,offset = 0x040
	//
	UCHAR												FileNameLength;

	//
	// FILE_NAME_xxx flags,offset = 0x041
	//
	UCHAR												Flags;

	//
	// first character of Unicode File Name,offset = 0x042
	//
	WCHAR												FileName[1];

}FILE_NAME,*PFILE_NAME;

//
// standard Information Attribute,sizeof = 0x048
//
typedef struct _STANDARD_INFORMATION
{
	//
	// creation time,offset = 0x000
	//
	LONGLONG											CreationTime;

	//
	// last time the DATA attribute was modified,offset = 0x008
	//
	LONGLONG											LastModificationTime;

	//
	// last time any attribute was modified,offset = 0x010
	//
	LONGLONG											LastChangeTime;

	//
	// last time the file was accessed,offset = 0x018
	//
	LONGLONG											LastAccessTime;

	//
	// file attributes,offset = 0x020
	//
	ULONG												FileAttributes;

	//
	// maximum file versions allowed for this file,offset = 0x024
	//
	ULONG												MaximumVersions;

	//
	// version number for this file,offset = 0x028
	//
	ULONG												VersionNumber;

	//
	// class Id from the bidirectional Class Id index,offset = 0x02c
	//
	ULONG												ClassId;

	//
	// Id for file owner, from bidir security index,offset = 0x030
	//
	ULONG												OwnerId;

	//
	// security id for the file - translates via bidir index to granted access Acl,offset = 0x034
	//
	ULONG												SecurityId;

	//
	// current amount of quota that has been charged for all the streams of this file,offset = 0x038
	// changed in same transaction with the quota file itself
	//
	ULONGLONG											QuotaCharged;

	//
	// update sequence number for this file,offset = 0x040
	//
	ULONGLONG											Usn;

}STANDARD_INFORMATION,*PSTANDARD_INFORMATION;

#include <poppack.h>

//
// link cache
//
typedef struct _NTFS_LINK_CACHE_ITEM
{
	//
	// device id
	//
	ULONG												DeviceId;

	//
	// parent file record
	//
	LONGLONG											ParentFileRecord;

	//
	// name length
	//
	ULONG												LinkNameLength;

	//
	// name buffer
	//
	CHAR												LinkName[36];

	//
	// file record
	//
	LONGLONG											FileRecord;
}NTFS_LINK_CACHE_ITEM,*PNTFS_LINK_CACHE_ITEM;

//
// link cache
//
NTFS_LINK_CACHE_ITEM									NtfsLinkCache[8];

//
// link cache count
//
ULONG													NtfsLinkCacheCount;

//
// file record buffer
//
PFILE_RECORD_SEGMENT_HEADER								NtfsFileRecordBuffer[64];

//
// pinned count
//
USHORT													NtfsFileRecordBufferPinned[64];

//
// vbo
//
VBO														NtfsFileRecordBufferVbo[64];

//
// compressed file record
//
LONGLONG												NtfsCompressedFileRecord;

//
// offset in file record
//
USHORT													NtfsCompressedOffset;

//
// vbo
//
ULONG													NtfsCompressedVbo;

//
// index allocation buffer
//
PINDEX_ALLOCATION_BUFFER								NtfsIndexAllocationBuffer;

//
// compressed buffer
//
PUCHAR													NtfsCompressedBuffer;

//
// uncompressed buffer
//
PUCHAR													NtfsUncompressedBuffer;

//
// file record buffer
//
UCHAR													NtfsBuffer0[MAXIMUM_FILE_RECORD_SIZE + 256];

//
// file record buffer
//
UCHAR													NtfsBuffer1[MAXIMUM_FILE_RECORD_SIZE + 256];

//
// index allocation buffer
//
UCHAR													NtfsBuffer2[MAXIMUM_INDEX_ALLOCATION_SIZE + 256];

//
// compressed buffer
//
UCHAR													NtfsBuffer3[MAXIMUM_COMPRESSION_UNIT_SIZE + 256];

//
// uncompressed buffer
//
UCHAR													NtfsBuffer4[MAXIMUM_COMPRESSION_UNIT_SIZE + 256];

//
// last cached mcb index
//
ULONG													LastMcb;

//
// forward declare
//
ARC_STATUS NtfsLoadMcb(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__in PNTFS_ATTRIBUTE_CONTEXT AttributeContext,__in VBO Vbo,__inout PNTFS_MCB Mcb);

//
// compare name
//
LONG NtfsCompareAnsiUniNames(__in STRING AnsiFileName,__in UNICODE_STRING UnicodeFileName)
{
	ULONG Length										= AnsiFileName.Length;
	if(Length > UnicodeFileName.Length)
		Length											= UnicodeFileName.Length / sizeof(WCHAR);

	for(ULONG i = 0; i < Length; i ++)
	{
		WCHAR Ansi										= NtfsUpCase(AnsiFileName.Buffer[i]);
		WCHAR Unicode									= NtfsUpCase(UnicodeFileName.Buffer[i]);

		LONG Diff										= static_cast<LONG>(Ansi) - static_cast<LONG>(Unicode);
		if(Diff)
			return Diff;
	}

	return static_cast<LONG>(AnsiFileName.Length) - static_cast<LONG>(UnicodeFileName.Length / sizeof(WCHAR));
}

//
// unpack bios
//
VOID NtfsUnPackBios(__out PBIOS_PARAMETER_BLOCK Bios,__in PPACKED_BIOS_PARAMETER_BLOCK Pbios)
{
	RtlCopyMemory(&Bios->BytesPerSector,&Pbios->BytesPerSector,sizeof(UCHAR) * 2);
	RtlCopyMemory(&Bios->SectorsPerCluster,&Pbios->SectorsPerCluster,sizeof(UCHAR));
	RtlCopyMemory(&Bios->ReservedSectors,&Pbios->ReservedSectors,sizeof(UCHAR) * 2);
	RtlCopyMemory(&Bios->Fats,&Pbios->Fats,sizeof(UCHAR));
	RtlCopyMemory(&Bios->RootEntries,&Pbios->RootEntries,sizeof(UCHAR) * 2);
	RtlCopyMemory(&Bios->Sectors,&Pbios->Sectors,sizeof(UCHAR) * 2);
	RtlCopyMemory(&Bios->Media,&Pbios->Media,sizeof(UCHAR));
	RtlCopyMemory(&Bios->SectorsPerFat,&Pbios->SectorsPerFat,sizeof(UCHAR) * 2);
	RtlCopyMemory(&Bios->SectorsPerTrack,&Pbios->SectorsPerTrack,sizeof(UCHAR) * 2);
	RtlCopyMemory(&Bios->Heads,&Pbios->Heads,sizeof(UCHAR) * 2);
	RtlCopyMemory(&Bios->HiddenSectors,&Pbios->HiddenSectors,sizeof(UCHAR) * 4);
	RtlCopyMemory(&Bios->LargeSectors,&Pbios->LargeSectors,sizeof(UCHAR) * 4);
}

//
// initialize attribute context
//
VOID FORCEINLINE NtfsInitializeAttributeContext(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__in PVOID FileRecordBuffer,__in PATTRIBUTE_RECORD_HEADER AttributeHeader,
												__in LONGLONG FileRecord,__out PNTFS_ATTRIBUTE_CONTEXT AttributeContext)
{
	AttributeContext->TypeCode							= AttributeHeader->TypeCode;
	AttributeContext->FileRecord						= FileRecord;
	AttributeContext->FileRecordOffset					= static_cast<USHORT>(reinterpret_cast<PUCHAR>(AttributeHeader) - static_cast<PUCHAR>(FileRecordBuffer));
	AttributeContext->IsAttributeResident				= AttributeHeader->FormCode == RESIDENT_FORM;

	if(AttributeContext->IsAttributeResident)
		AttributeContext->DataSize						= AttributeHeader->Form.Resident.ValueLength;
	else
		AttributeContext->DataSize						= AttributeHeader->Form.Nonresident.FileSize;

	AttributeContext->CompressionFormat					= COMPRESSION_FORMAT_NONE;

	if(AttributeHeader->Flags & ATTRIBUTE_FLAG_COMPRESSION_MASK)
	{
		AttributeContext->CompressionFormat				= COMPRESSION_FORMAT_LZNT1;
		AttributeContext->CompressionUnit				= StructureContext->BytesPerCluster;
		for(ULONG i = 0; i < AttributeHeader->Form.Nonresident.CompressionUnit; i ++)
			AttributeContext->CompressionUnit			*= 2;
	}
}

//
// file reference to large integer
//
VOID FORCEINLINE NtfsFileReferenceToLargeInteger(__in PFILE_REFERENCE FileReference,__out PLONGLONG LargeInteger)
{
	PFILE_REFERENCE Temp								= reinterpret_cast<PFILE_REFERENCE>(LargeInteger);
	*Temp												= *FileReference;
	Temp->SequenceNumber								= 0;
}

//
// get first part name
//
VOID NtfsFirstComponent(__inout PSTRING String,__out PSTRING FirstComponent)
{
	//
	// copy over the string variable into the first component variable
	//
	*FirstComponent										= *String;

	//
	// if the first character in the name is a backslash then simply skip over the backslash.
	//
	if(FirstComponent->Buffer[0] == '\\')
	{
		FirstComponent->Buffer							+= 1;
		FirstComponent->Length							-= sizeof(CHAR);
	}

	//
	// search the name for a backslash
	//
	ULONG Index											= 0;
	for(; Index < FirstComponent->Length; Index ++)
	{
		if(FirstComponent->Buffer[Index] == '\\')
			break;
	}

	//
	// at this point Index denotes a backslash or is equal to the length of the string.
	// update string to be the remaining part.
	// decrement the length of the first component by the approprate amount
	//
	String->Buffer										= &FirstComponent->Buffer[Index];
	String->Length										= static_cast<USHORT>(FirstComponent->Length - Index);
	FirstComponent->Length								= static_cast<USHORT>(Index);
}

//
// read disk
//
ARC_STATUS NtfsReadDisk(__in ULONG DeviceId,__in LBO Lbo,__in ULONG ByteCount,__out PVOID Buffer,__in BOOLEAN SaveToCache)
{
	//
	// zero-length
	//
	if(!ByteCount)
		return ESUCCESS;

	LARGE_INTEGER Offset;
	Offset.QuadPart										= Lbo;
	ULONG ReadCount										= 0;
	ARC_STATUS Status									= BlDiskCacheRead(DeviceId,&Offset,Buffer,ByteCount,&ReadCount,SaveToCache);
	if(Status != ESUCCESS)
		return Status;

	return ReadCount == ByteCount ? ESUCCESS : EIO;
}

//
// write disk
//
ARC_STATUS NtfsWriteDisk(__in ULONG DeviceId,__in LBO Lbo,__in ULONG ByteCount,__out PVOID Buffer)
{
	//
	// zero-length
	//
	if(!ByteCount)
		return ESUCCESS;

	LARGE_INTEGER Offset;
	Offset.QuadPart										= Lbo;
	ULONG WritenCount									= 0;
	ARC_STATUS Status									= BlDiskCacheWrite(DeviceId,&Offset,Buffer,ByteCount,&WritenCount);
	if(Status != ESUCCESS)
		return Status;

	return WritenCount == ByteCount ? ESUCCESS : EIO;
}

//
// decode usa
//
ARC_STATUS NtfsDecodeUsa(__in PVOID UsaBuffer,__in ULONG Length)
{
	PMULTI_SECTOR_HEADER MultiSectorHeader				= static_cast<PMULTI_SECTOR_HEADER>(UsaBuffer);
	PUSHORT UsaOffset									= Add2Ptr(UsaBuffer,MultiSectorHeader->UpdateSequenceArrayOffset,PUSHORT);
	ULONG UsaSize										= MultiSectorHeader->UpdateSequenceArraySize;

	//
	// for every entry in the usa
	// we need to compute the address of the protected ushort and then check that the protected ushort is equal to the current sequence number (the number at UsaOffset[0])
	// and then replace the protected ushort number with the saved ushort in the usa.
	//
	for(ULONG i = 1; i < UsaSize; i ++)
	{
		PUSHORT ProtectedUshort							= Add2Ptr(UsaBuffer,512 * i - sizeof(USHORT),PUSHORT);
		if(*ProtectedUshort != UsaOffset[0])
			return EBADF;

		*ProtectedUshort								= UsaOffset[i];
	}

	return ESUCCESS;
}

//
// decode vbo,lbo map
//
ARC_STATUS NtfsDecodeRetrievalInformation(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__inout PNTFS_MCB Mcb,__in VBO Vbo,__in PATTRIBUTE_RECORD_HEADER AttributeHeader)
{
	//
	// setup the next vbo and current lbo and ch for the following loop that decodes the retrieval information
	//
	ULONG BytesPerCluster								= StructureContext->BytesPerCluster;
	VBO NextVbo											= AttributeHeader->Form.Nonresident.LowestVcn * BytesPerCluster;
	LBO CurrentLbo										= 0;
	PCHAR ch											= Add2Ptr(AttributeHeader,AttributeHeader->Form.Nonresident.MappingPairsOffset,PCHAR);
	Mcb->InUse											= 0;

	//
	// loop to process mapping pairs
	//
	while(ch[0] & 0xff)
	{
		//
		// set current Vbo from initial value or last pass through loop
		//
		VBO CurrentVbo									= NextVbo;

		//
		// extract the counts from the two nibbles of this byte
		//
		ULONG VboBytes									= *ch & 0x0f;
		ULONG LboBytes									= *ch++ >> 4;

		//
		// extract the Vbo change and update next vbo
		//
		LONGLONG Change									= 0;

		if((ch[VboBytes - 1] & 0x80) == 0x80)
			return EINVAL;

		RtlMoveMemory(&Change,ch,VboBytes);

		ch												+= VboBytes;
		NextVbo											+= Change * BytesPerCluster;

		//
		// if we have reached the maximum for this mcb then it is time to return and not decipher any more retrieval information
		//
		if(Mcb->InUse >= MAXIMUM_NUMBER_OF_MCB_ENTRIES - 1)
			break;

		//
		// check if there is an lbo change.
		// if there isn't then we only need to update the vbo, because this is sparse/compressed file.
		//
		if(LboBytes)
		{
			//
			// extract the Lbo change and update current lbo
			//
			Change										= 0;

			if((ch[LboBytes - 1] & 0x80) == 0x80)
				Change									-= 1;

			RtlMoveMemory(&Change,ch,LboBytes);

			ch											+= LboBytes;
			CurrentLbo									+= Change * BytesPerCluster;
		}

		//
		// check if the Next Vbo is greater than the Vbo we after
		//
		if(NextVbo >= Vbo)
		{
			//
			// load this entry into the mcb and advance our in use counter
			//
			Mcb->Vbo[Mcb->InUse]						= CurrentVbo;
			Mcb->Lbo[Mcb->InUse]						= (LboBytes != 0 ? CurrentLbo : 0);
			Mcb->Vbo[++ Mcb->InUse]						= NextVbo;
		}
	}

	return ESUCCESS;
}

//
// check file name is cached
//
ARC_STATUS NtfsIsNameCached(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__in STRING FileName,__inout PLONGLONG FileRecord,
							__out PBOOLEAN FoundFileName,__out PBOOLEAN IsDirectory)
{
	*FoundFileName										= FALSE;

	for(ULONG i = 0; i < ARRAYSIZE(NtfsLinkCache); i ++)
	{
		//
		// check device id
		//
		if(NtfsLinkCache[i].DeviceId != StructureContext->DeviceId)
			continue;

		//
		// check parent file record
		//
		if(NtfsLinkCache[i].ParentFileRecord != *FileRecord)
			continue;

		//
		// check file name length
		//
		if(NtfsLinkCache[i].LinkNameLength != FileName.Length)
			continue;

		//
		// check file name
		//
		if(_strnicmp(FileName.Buffer,NtfsLinkCache->LinkName,FileName.Length))
			continue;

		//
		// found it
		//
		*FoundFileName									= TRUE;
		*FileRecord										= NtfsLinkCache[i].FileRecord;

		//
		// only dir is cached
		//
		*IsDirectory									= TRUE;

		break;
	}

	return *FoundFileName;
}

//
// add name to cache
//
VOID NtfsAddNameToCache(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__in STRING FileName,__in LONGLONG ParentFileRecord,__in LONGLONG FileRecord)
{
	//
	// buffer full
	//
	if(NtfsLinkCacheCount >= ARRAYSIZE(NtfsLinkCache))
		return;

	//
	// search an empty slot
	//
	ULONG i;
	for(i = 0; i < ARRAYSIZE(NtfsLinkCache); i ++)
	{
		if(NtfsLinkCache[i].DeviceId == 0xffffffff)
			break;
	}

	//
	// unable to find an empty slot
	//
	if(i >= ARRAYSIZE(NtfsLinkCache))
		return;

	//
	// setup it
	//
	NtfsLinkCache[i].DeviceId							= StructureContext->DeviceId;
	NtfsLinkCache[i].FileRecord							= FileRecord;
	NtfsLinkCache[i].ParentFileRecord					= ParentFileRecord;
	NtfsLinkCache[i].LinkNameLength						= FileName.Length;

	for(ULONG j = 0; j < FileName.Length; j ++)
		NtfsLinkCache[i].LinkName[j]					= NtfsUpCase(FileName.Buffer[j]);

	NtfsLinkCacheCount									+= 1;
}

//
// map vbo to lbo
//
ARC_STATUS NtfsVboToLbo(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__in PNTFS_ATTRIBUTE_CONTEXT AttributeContext,__in VBO Vbo,__out PLBO Lbo,__out PULONG ByteCount)
{
	ULONG i;

	//
	// check if we are doing the mft or some other attribute
	//
	PNTFS_MCB Mcb										= 0;
	if(AttributeContext == &StructureContext->MftAttributeContext)
	{
		//
		// for the mft we start with the base mcb but if the vbo is not in the mcb then we immediately switch over to the cached mcb
		//
		Mcb												= &StructureContext->MftBaseMcb;

		if(Vbo < Mcb->Vbo[0] || Vbo >= Mcb->Vbo[Mcb->InUse])
			Mcb											= 0;
	}

	//
	// if the Mcb is still null then we are to use the cached mcb, first find if one of the cached ones contains the range we're after
	//
	if(!Mcb)
	{
		for(ULONG i = 0; i < ARRAYSIZE(StructureContext->CachedMcb); i ++)
		{
			//
			// check if we have a hit, on the same attribute and range
			//
			Mcb											= &StructureContext->CachedMcb[i];

			if( AttributeContext->FileRecord == StructureContext->CachedMcbFileRecord[i] &&
				AttributeContext->FileRecordOffset == StructureContext->CachedMcbFileRecordOffset[i] &&
				Mcb->Vbo[0] <= Vbo && Vbo < Mcb->Vbo[Mcb->InUse])
			{
				break;
			}

			Mcb											= 0;
		}

		//
		// if we didn't get a hit then we need to load a new mcb we'll alternate through our two cached mcbs
		//
		if(!Mcb)
		{
			ULONG Index											= LastMcb % ARRAYSIZE(StructureContext->CachedMcb);
			Mcb													= &StructureContext->CachedMcb[Index];
			StructureContext->CachedMcbFileRecord[Index]		= AttributeContext->FileRecord;
			StructureContext->CachedMcbFileRecordOffset[Index]	= AttributeContext->FileRecordOffset;

			LastMcb												+= 1;

			//
			// load it
			//
			ARC_STATUS Status									= NtfsLoadMcb(StructureContext,AttributeContext,Vbo,Mcb);
			if(Status != ESUCCESS)
				return Status;
		}
	}

	//
	// at this point the mcb contains the vbo asked for,search for the vbo.
	//
	for(ULONG i = 0; i < Mcb->InUse; i ++)
	{
		//
		// we found our slot if the vbo we're after is less than the next mcb's vbo
		//
		if(Vbo < Mcb->Vbo[i + 1])
		{
			//
			// compute the corresponding lbo which is the stored lbo plus the difference between the stored vbo and the vbo we're looking up.
			// also compute the byte count which is the difference between the current vbo we're looking up and the vbo for the next run
			//
			if(Mcb->Lbo[i])
				*Lbo									= Vbo - Mcb->Vbo[i] + Mcb->Lbo[i];
			else
				*Lbo									= 0;

			*ByteCount									= static_cast<ULONG>(Mcb->Vbo[i + 1] - Vbo);

			return ESUCCESS;
		}
	}

	//
	// unable to find the vbo
	//
	return EINVAL;
}

//
// read non-resident attribute
//
ARC_STATUS NtfsReadNonresidentAttribute(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__in PNTFS_ATTRIBUTE_CONTEXT AttributeContext,
										__in VBO Vbo,__in ULONG Length,__in PVOID Buffer)
{
	//
	// cache mft and index data
	//
	ULONG TypeCode										= AttributeContext->TypeCode;
	BOOLEAN CacheRead									= FALSE;
	if(AttributeContext == &StructureContext->MftAttributeContext || TypeCode == $BITMAP || TypeCode == $INDEX_ALLOCATION || TypeCode == $INDEX_ROOT)
		CacheRead										= TRUE;

	//
	// compressed data
	//
	if(AttributeContext->CompressionFormat != COMPRESSION_FORMAT_NONE)
	{
		//
		// while there is still some more to copy into the caller's buffer, we will load the cached compressed buffers and then copy out the data
		//
		while(Length > 0)
		{
			//
			// load up the cached compressed buffers with the the proper data.
			// first check if the buffer is already (i.e., the file record and offset match and the vbo we're after is within the buffers range)
			//
			if( NtfsCompressedFileRecord != AttributeContext->FileRecord		||
				NtfsCompressedOffset != AttributeContext->FileRecordOffset		||
				Vbo < NtfsCompressedVbo											||
				Vbo >= NtfsCompressedVbo + AttributeContext->CompressionUnit)
			{
				//
				// load up the cached identification information
				//
				NtfsCompressedFileRecord				= AttributeContext->FileRecord;
				NtfsCompressedOffset					= AttributeContext->FileRecordOffset;
				NtfsCompressedVbo						= static_cast<ULONG>(Vbo) & ~(AttributeContext->CompressionUnit - 1);

				//
				// load up the compressed buffer with data.
				// we keep on loading until we're done loading or the Lbo we get back is zero.
				//
				ULONG i									= 0;
				ULONG ByteCount							= 0;
				for(; i < AttributeContext->CompressionUnit; i += ByteCount)
				{
					LBO Lbo								= 0;
					ARC_STATUS Status					= NtfsVboToLbo(StructureContext,AttributeContext,NtfsCompressedVbo + i,&Lbo,&ByteCount);
					if(Status != ESUCCESS)
						return Status;

					if(!Lbo)
						break;

					//
					// trim the byte count down to a compression unit and we'll catch the excess the next time through the loop
					//
					if(i + ByteCount > AttributeContext->CompressionUnit)
						ByteCount						= AttributeContext->CompressionUnit - i;

					Status								= NtfsReadDisk(StructureContext->DeviceId,Lbo,ByteCount,NtfsCompressedBuffer + i,CacheRead);
					if(Status != ESUCCESS)
						return Status;
				}

				//
				// if the index for the preceding loop is zero
				// then we know that there isn't any data on disk for the compression unit and in-fact the compression unit is all zeros
				//
				if(i == 0)
				{
					RtlZeroMemory(NtfsUncompressedBuffer,AttributeContext->CompressionUnit);
				}
				else if(i >= AttributeContext->CompressionUnit)
				{
					//
					// otherwise the unit we just read in cannot be compressed because it completely fills up the compression unit
					//
					RtlMoveMemory(NtfsUncompressedBuffer,NtfsCompressedBuffer,AttributeContext->CompressionUnit);
				}
				else
				{
					//
					// if the index for the preceding loop is less than the compression unit size
					// then we know that the data we read in is less than the compression unit and we hit a zero lbo.
					// so the unit must be compressed.
					//
					NTSTATUS Status						= RtlDecompressBuffer(AttributeContext->CompressionFormat,NtfsUncompressedBuffer,
																			  AttributeContext->CompressionUnit,NtfsCompressedBuffer,i,&ByteCount);
					if(!NT_SUCCESS(Status))
						return EINVAL;

					//
					// check if the decompressed buffer doesn't fill up the compression unit and if so then zero out the remainder of the uncompressed buffer
					//
					if(ByteCount < AttributeContext->CompressionUnit)
						RtlZeroMemory(NtfsUncompressedBuffer + ByteCount,AttributeContext->CompressionUnit - ByteCount);
				}
			}

			//
			// copy off the data from the compressed buffer to the user buffer and continue the loop until the length is zero.
			// the amount of data we need to copy is the smaller of the length the user wants back
			// or the number of bytes left in the uncompressed buffer from the requested vbo to the end of the buffer.
			//
			ULONG ByteCount								= NtfsCompressedVbo + AttributeContext->CompressionUnit - static_cast<ULONG>(Vbo);
			if(ByteCount > Length)
				ByteCount								= Length;

			//
			// copy to user buffer
			//
			RtlMoveMemory(Buffer,NtfsUncompressedBuffer + static_cast<ULONG>(Vbo) - NtfsCompressedVbo,ByteCount);

			//
			// update the length to be what the user now needs read in, also update the Vbo and Buffer to be the next locations to be read in.
			//
			Length										-= ByteCount;
			Vbo											+= ByteCount;
			Buffer										= Add2Ptr(Buffer,ByteCount,PVOID);
		}

		return ESUCCESS;
	}

	//
	// read in runs of data until the byte count goes to zero
	//
	while(Length > 0)
	{
		//
		// lookup the corresponding Lbo and run length for the current position
		//
		LBO Lbo											= -1;
		ULONG CurrentRunByteCount						= 0;
		ARC_STATUS Status								= NtfsVboToLbo(StructureContext,AttributeContext,Vbo,&Lbo,&CurrentRunByteCount);
		if(Status != ESUCCESS)
			return Status;

		//
		// while there are bytes to be read in from the current run length and we haven't exhausted the request we loop reading in bytes.
		//
		while(Length > 0 && CurrentRunByteCount > 0)
		{
			//
			// compute the size of the next physical read
			//
			ULONG SingleReadSize						= Length < 32 * 1024 ? Length : 32 * 1024;
			if(SingleReadSize > CurrentRunByteCount)
				SingleReadSize							= CurrentRunByteCount;

			//
			// don't read beyond the data size
			//
			if(Vbo + SingleReadSize > AttributeContext->DataSize)
			{
				//
				// if the readjusted read length is now zero then we're done
				//
				if(AttributeContext->DataSize - Vbo <= 0)
					return ESUCCESS;

				//
				// by also setting length we'll make sure that this is our last read
				//
				SingleReadSize							= static_cast<ULONG>(AttributeContext->DataSize - Vbo);
				Length									= static_cast<ULONG>(SingleReadSize);
			}

			//
			// read
			//
			Status										= NtfsReadDisk(StructureContext->DeviceId,Lbo,SingleReadSize,Buffer,CacheRead);
			if(Status != ESUCCESS)
				return Status;

			//
			// update the remaining length, current run byte count, and new lbo offset
			//
			Length										-= SingleReadSize;
			CurrentRunByteCount							-= SingleReadSize;
			Lbo											+= SingleReadSize;
			Vbo											+= SingleReadSize;

			//
			// update the buffer to point to the next byte location
			//
			Buffer										= Add2Ptr(Buffer,SingleReadSize,PVOID);
		}
	}

	return ESUCCESS;
}

//
// read and decode file record
//
ARC_STATUS NtfsReadAndDecodeFileRecord(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__in LONGLONG FileRecord,__out PULONG Index)
{
	//
	// for each buffer that is not null check if we have a hit on the file record and if so then increment the pin count and return that index
	//
	for(*Index = 0; *Index < ARRAYSIZE(NtfsFileRecordBuffer) && NtfsFileRecordBuffer[*Index]; *Index += 1)
	{
		if(NtfsFileRecordBufferVbo[*Index] == FileRecord)
		{
			NtfsPinFileRecord(*Index);
			return ESUCCESS;
		}
	}

	//
	// check for the first unpinned buffer and make sure we haven't exhausted the array
	//
	for(*Index = 0; *Index < ARRAYSIZE(NtfsFileRecordBufferPinned) && NtfsFileRecordBufferPinned[*Index]; *Index += 1);

	if(*Index == ARRAYSIZE(NtfsFileRecordBufferPinned))
		return E2BIG;

	//
	// we have an unpinned buffer that we want to use, check if we need to allocate a buffer to actually hold the data
	//
	if(!NtfsFileRecordBuffer[*Index])
		NtfsFileRecordBuffer[*Index]					= static_cast<PFILE_RECORD_SEGMENT_HEADER>(BlAllocateHeapAligned(MAXIMUM_FILE_RECORD_SIZE));

	//
	// pin the buffer and then read in the data
	//
	NtfsPinFileRecord(*Index);

	//
	// read mft
	//
	ARC_STATUS Status									= NtfsReadNonresidentAttribute(StructureContext,&StructureContext->MftAttributeContext,
																					   FileRecord * StructureContext->BytesPerFileRecord,
																					   StructureContext->BytesPerFileRecord,NtfsFileRecordBuffer[*Index]);
	if(Status != ESUCCESS)
		return Status;

	//
	// decode the usa
	//
	Status												= NtfsDecodeUsa(NtfsFileRecordBuffer[*Index],StructureContext->BytesPerFileRecord);
	if(Status != ESUCCESS)
		return Status;

	//
	// and set the file record so that we know where it came from
	//
	NtfsFileRecordBufferVbo[*Index]						= FileRecord;

	return ESUCCESS;
}

//
// read resident attribute
//
ARC_STATUS NtfsReadResidentAttribute(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__in PNTFS_ATTRIBUTE_CONTEXT AttributeContext,
									 __in VBO Vbo,__in ULONG Length,__in PVOID Buffer)
{
	//
	// read in the file record containing the resident attribute
	//
	ULONG BufferIndex									= 0;
	ARC_STATUS Status									= NtfsReadAndDecodeFileRecord(StructureContext,AttributeContext->FileRecord,&BufferIndex);
	if(Status != ESUCCESS)
		return Status;

	//
	// get a pointer to the attribute header
	//
	PATTRIBUTE_RECORD_HEADER AttributeHeader			= Add2Ptr(NtfsFileRecordBuffer[BufferIndex],AttributeContext->FileRecordOffset,PATTRIBUTE_RECORD_HEADER);

	//
	// copy the amount of data the user asked for starting with the proper offset
	//
	RtlMoveMemory(Buffer,Add2Ptr(NtfsGetValue(AttributeHeader),static_cast<ULONG>(Vbo),PVOID),Length);

	NtfsUnpinFileRecord(BufferIndex);

	return ESUCCESS;
}

//
// load mcb
//
ARC_STATUS NtfsLoadMcb(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__in PNTFS_ATTRIBUTE_CONTEXT AttributeContext,__in VBO Vbo,__inout PNTFS_MCB Mcb)
{
	ULONG BytesPerCluster								= StructureContext->BytesPerCluster;

	//
	// setup a pointer to the cached mcb, indicate the attribute context that is will now own the cached mcb, and zero out the mcb
	//
	Mcb->InUse											= 0;

	//
	// read in the file record that contains the non-resident attribute
	//
	ULONG BufferIndex;
	ARC_STATUS Status									= NtfsReadAndDecodeFileRecord(StructureContext,AttributeContext->FileRecord,&BufferIndex);
	if(Status != ESUCCESS)
		return Status;

	//
	// get a pointer to the attribute header
	//
	PATTRIBUTE_RECORD_HEADER AttributeHeader			= Add2Ptr(NtfsFileRecordBuffer[BufferIndex],AttributeContext->FileRecordOffset,PATTRIBUTE_RECORD_HEADER);

	//
	// compute the lowest and highest vbo that is described by this attribute header
	//
	VBO LowestVbo										= AttributeHeader->Form.Nonresident.LowestVcn * BytesPerCluster;
	VBO HighestVbo										= AttributeHeader->Form.Nonresident.HighestVcn * BytesPerCluster;

	//
	// check if the vbo we are after is within the range of this attribute header
	// and if so then decode the retrieval information and return to our caller
	//
	if(LowestVbo <= Vbo && Vbo <= HighestVbo)
	{
		Status											= NtfsDecodeRetrievalInformation(StructureContext,Mcb,Vbo,AttributeHeader);
		if(Status == ESUCCESS)
			NtfsUnpinFileRecord(BufferIndex);

		return Status;
	}

	//
	// at this point the attribute header does not contain the range we need
	// so read in the base file record and we'll search the attribute list for a attribute header that we need.
	//
	LONGLONG FileRecord;
	NtfsFileReferenceToLargeInteger(&NtfsFileRecordBuffer[BufferIndex]->BaseFileRecordSegment,&FileRecord);

	NtfsUnpinFileRecord(BufferIndex);

	Status												= NtfsReadAndDecodeFileRecord(StructureContext,FileRecord,&BufferIndex);
	if(Status != ESUCCESS)
		return Status;

	//
	// search for the attribute list attribute
	//
	PNTFS_ATTRIBUTE_CONTEXT AttributeList				= 0;
	NTFS_ATTRIBUTE_CONTEXT AttributeContext1;
	for(AttributeHeader = NtfsFirstAttribute(NtfsFileRecordBuffer[BufferIndex]); AttributeHeader->TypeCode != $END; AttributeHeader = NtfsGetNextRecord(AttributeHeader))
	{
		//
		// check if this is the attribute list attribute and if so then setup a local attribute context
		//
		if(AttributeHeader->TypeCode == $ATTRIBUTE_LIST)
		{
			AttributeList								= &AttributeContext1;
			NtfsInitializeAttributeContext(StructureContext,NtfsFileRecordBuffer[BufferIndex],AttributeHeader,FileRecord,AttributeList);
		}
	}

	//
	// we have better located an attribute list otherwise we're in trouble
	//
	if(!AttributeList)
	{
		NtfsUnpinFileRecord(BufferIndex);
		return EINVAL;
	}

	//
	// we've located the attribute list we need to continue our search.
	// so what this outer loop does is search down the attribute list looking for a match.
	//
	ATTRIBUTE_LIST_ENTRY AttributeListEntry;
	ATTRIBUTE_TYPE_CODE TypeCode						= AttributeContext->TypeCode;
	ULONG SavedBufferIndex								= BufferIndex;
	LONGLONG Previousli									= 0;
	LONGLONG li											= 0;
	NtfsPinFileRecord(BufferIndex);
	for(; li < AttributeList->DataSize; li = li + AttributeListEntry.RecordLength)
	{
		//
		// read in the attribute list entry.
		// we don't need to read in the name, just the first part of the list entry.
		//
		Status											= NtfsReadAttribute(StructureContext,AttributeList,li,sizeof(ATTRIBUTE_LIST_ENTRY),&AttributeListEntry);
		if(Status != ESUCCESS)
			return Status;

		//
		// check if the attribute matches, and either it is not $data or if it is $data then it is unnamed
		//
		if(AttributeListEntry.AttributeTypeCode == TypeCode && (TypeCode != $DATA || (TypeCode == $DATA && !AttributeListEntry.AttributeNameLength)))
		{
			//
			// if the lowest vcn is is greater than the vbo we've after then we are done and can use previous li otherwise set previous li accordingly.
			//
			if(Vbo < AttributeListEntry.LowestVcn * BytesPerCluster)
				break;

			Previousli									= li;
		}
	}

	//
	// we should have found the offset for the attribute list entry so read it in and verify that it is correct
	//
	Status												= NtfsReadAttribute(StructureContext,AttributeList,Previousli,sizeof(ATTRIBUTE_LIST_ENTRY),&AttributeListEntry);

	if(AttributeListEntry.AttributeTypeCode == TypeCode && (TypeCode != $DATA || (TypeCode == $DATA && !AttributeListEntry.AttributeNameLength)))
	{
		//
		// we found a match so now compute the file record containing this attribute and read in the file record
		//
		NtfsFileReferenceToLargeInteger(&AttributeListEntry.SegmentReference,&FileRecord);

		NtfsUnpinFileRecord(BufferIndex);

		Status											= NtfsReadAndDecodeFileRecord(StructureContext,FileRecord,&BufferIndex);
		if(Status != ESUCCESS)
			return Status;

		//
		// search down the file record for our matching attribute, and it better be there otherwise the attribute list is wrong.
		//
		for(AttributeHeader = NtfsFirstAttribute(NtfsFileRecordBuffer[BufferIndex]);AttributeHeader->TypeCode != $END;AttributeHeader = NtfsGetNextRecord(AttributeHeader))
		{
			//
			// as a quick check make sure that this attribute is non resident
			//
			if(AttributeHeader->FormCode == NONRESIDENT_FORM)
			{
				//
				// compute the range of this attribute header
				//
				LowestVbo								= AttributeHeader->Form.Nonresident.LowestVcn * BytesPerCluster;
				HighestVbo								= AttributeHeader->Form.Nonresident.HighestVcn * BytesPerCluster;

				//
				// we have located the attribute in question if the type code match,
				// it is within the proper range, and if it is either not the data attribute
				// or if it is the data attribute then it is also unnamed
				//
				if(AttributeHeader->TypeCode == TypeCode && LowestVbo <= Vbo && Vbo <= HighestVbo && (TypeCode != $DATA||(TypeCode == $DATA&&!AttributeHeader->NameLength)))
				{
					//
					// we've located the attribute so now it is time to decode the retrieval information and return to our caller
					//
					Status								= NtfsDecodeRetrievalInformation(StructureContext,Mcb,Vbo,AttributeHeader);
					if(Status != ESUCCESS)
						return Status;

					NtfsUnpinFileRecord(BufferIndex);
					NtfsUnpinFileRecord(SavedBufferIndex);

					return ESUCCESS;
				}
			}
		}
	}

	NtfsUnpinFileRecord(BufferIndex);
	NtfsUnpinFileRecord(SavedBufferIndex);

	return EINVAL;
}

//
// write non-resident attribute
//
ARC_STATUS NtfsWriteNonresidentAttribute(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__in PNTFS_ATTRIBUTE_CONTEXT AttributeContext,
										 __in VBO Vbo,__in ULONG Length,__in PVOID Buffer)
{
	//
	// only support uncompressed attribute
	//
	if(AttributeContext->CompressionFormat != COMPRESSION_FORMAT_NONE)
		return EROFS;

	//
	// write out runs of data until the byte count goes to zero
	//
	while(Length > 0)
	{
		//
		// lookup the corresponding Lbo and run length for the current position
		//
		LBO Lbo											= -1;
		ULONG CurrentRunByteCount						= 0;
		ARC_STATUS Status								= NtfsVboToLbo(StructureContext,AttributeContext,Vbo,&Lbo,&CurrentRunByteCount);
		if(Status != ESUCCESS)
			return Status;

		//
		// while there are bytes to be write out from the current run length and we haven't exhausted the request we loop writing out bytes.
		//
		while(Length > 0 && CurrentRunByteCount > 0)
		{
			//
			// compute the size of the next physical write
			//
			ULONG SingleWriteSize						= Length < 32 * 1024 ? Length : 32 * 1024;
			if(SingleWriteSize > CurrentRunByteCount)
				SingleWriteSize							= CurrentRunByteCount;

			//
			// don't write beyond the data size
			//
			if(Vbo + SingleWriteSize > AttributeContext->DataSize)
			{
				//
				// if the readjusted read length is now zero then we're done
				//
				if(AttributeContext->DataSize - Vbo <= 0)
					return ESUCCESS;

				//
				// by also setting length we'll make sure that this is our last write
				//
				SingleWriteSize							= static_cast<ULONG>(AttributeContext->DataSize - Vbo);
				Length									= static_cast<ULONG>(SingleWriteSize);
			}

			//
			// write
			//
			Status										= NtfsWriteDisk(StructureContext->DeviceId,Lbo,SingleWriteSize,Buffer);
			if(Status != ESUCCESS)
				return Status;

			//
			// update the remaining length, current run byte count, and new lbo offset
			//
			Length										-= SingleWriteSize;
			CurrentRunByteCount							-= SingleWriteSize;
			Lbo											+= SingleWriteSize;
			Vbo											+= SingleWriteSize;

			//
			// update the buffer to point to the next byte location
			//
			Buffer										= Add2Ptr(Buffer,SingleWriteSize,PVOID);
		}
	}

	return ESUCCESS;
}

//
// lookup attribute
//
ARC_STATUS NtfsLookupAttribute(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__in LONGLONG FileRecord,__in ATTRIBUTE_TYPE_CODE TypeCode,
							   __out PBOOLEAN FoundAttribute,__out PNTFS_ATTRIBUTE_CONTEXT AttributeContext)
{
	//
	// read in the file record and if necessary move ourselves up to the base file record
	//
	*FoundAttribute										= FALSE;
	ULONG BufferIndex									= 0;
	ARC_STATUS Status									= NtfsReadAndDecodeFileRecord(StructureContext,FileRecord,&BufferIndex);
	if(Status != ESUCCESS)
		return Status;

	if( NtfsFileRecordBuffer[BufferIndex]->BaseFileRecordSegment.SegmentNumberHighPart ||
		NtfsFileRecordBuffer[BufferIndex]->BaseFileRecordSegment.SegmentNumberLowPart ||
		NtfsFileRecordBuffer[BufferIndex]->BaseFileRecordSegment.SequenceNumber)
	{
		//
		// this isn't the base file record so now extract the base file record number and read it in
		//
		NtfsFileReferenceToLargeInteger(&NtfsFileRecordBuffer[BufferIndex]->BaseFileRecordSegment,&FileRecord);

		NtfsUnpinFileRecord(BufferIndex);

		Status											= NtfsReadAndDecodeFileRecord(StructureContext,FileRecord,&BufferIndex);
		if(Status != ESUCCESS)
			return Status;
	}

	//
	// we have read in the base file record so search for the target attribute type code and also remember if we find the attribute list attribute
	//
	NTFS_ATTRIBUTE_CONTEXT AttributeContext1;
	PNTFS_ATTRIBUTE_CONTEXT AttributeList				= 0;
	PATTRIBUTE_RECORD_HEADER AttributeHeader			= NtfsFirstAttribute(NtfsFileRecordBuffer[BufferIndex]);
	for(; AttributeHeader->TypeCode != $END; AttributeHeader = NtfsGetNextRecord(AttributeHeader))
	{
		//
		// we have located the attribute in question if the type code match and if it is either not the data attribute or if it is the data attribute then it is also unnamed
		//
		if(AttributeHeader->TypeCode == TypeCode && (TypeCode != $DATA || (TypeCode == $DATA && !AttributeHeader->NameLength)))
		{
			//
			// we have found the attribute and setup the output attribute context and then return to our caller
			//
			*FoundAttribute								= TRUE;

			NtfsInitializeAttributeContext(StructureContext,NtfsFileRecordBuffer[BufferIndex],AttributeHeader,FileRecord,AttributeContext);

			NtfsUnpinFileRecord(BufferIndex);

			return ESUCCESS;
		}

		//
		// check if this is the attribute list attribute
		// and if so then setup a local attribute context to use just in case we don't find the attribute we're after in the base file record
		//
		if(AttributeHeader->TypeCode == $ATTRIBUTE_LIST)
		{
			AttributeList								= &AttributeContext1;
			NtfsInitializeAttributeContext(StructureContext,NtfsFileRecordBuffer[BufferIndex],AttributeHeader,FileRecord,AttributeList);
		}
	}

	//
	// if we reach this point then the attribute has not been found in the base file record so check if we have located an attribute list.
	// if not then the search has not been successful
	//
	if(!AttributeList)
	{
		NtfsUnpinFileRecord(BufferIndex);
		return ESUCCESS;
	}

	//
	// we've located the attribute list we need to continue our search.
	// so what this outer loop does is search down the attribute list looking for a match.
	//
	ATTRIBUTE_LIST_ENTRY AttributeListEntry;
	LONGLONG li											= 0;
	for(; li < AttributeList->DataSize; li += AttributeListEntry.RecordLength)
	{
		//
		// read in the attribute list entry.  We don't need to read in the name, just the first part of the list entry.
		//
		Status											= NtfsReadAttribute(StructureContext,AttributeList,li,sizeof(ATTRIBUTE_LIST_ENTRY),&AttributeListEntry);
		if(Status != ESUCCESS)
			return Status;

		//
		// check if the attribute matches, and it is the first of multiple segments, and either it is not $data or if it is $data then it is unnamed
		//
		if( AttributeListEntry.AttributeTypeCode == TypeCode && !AttributeListEntry.LowestVcn &&
			(TypeCode != $DATA || (TypeCode == $DATA && !AttributeListEntry.AttributeNameLength)))
		{
			//
			// we found a match so now compute the file record containing the attribute we're after and read in the file record
			//
			NtfsFileReferenceToLargeInteger(&AttributeListEntry.SegmentReference,&FileRecord);

			NtfsUnpinFileRecord(BufferIndex);

			Status										= NtfsReadAndDecodeFileRecord(StructureContext,FileRecord,&BufferIndex);
			if(Status != ESUCCESS)
				return Status;

			//
			// now search down the file record for our matching attribute, and it better be there otherwise the attribute list is wrong.
			//
			AttributeHeader								= NtfsFirstAttribute(NtfsFileRecordBuffer[BufferIndex]);
			for(; AttributeHeader->TypeCode != $END; AttributeHeader = NtfsGetNextRecord(AttributeHeader))
			{
				if(AttributeHeader->TypeCode == TypeCode && (TypeCode != $DATA || (TypeCode == $DATA && !AttributeHeader->NameLength)))
				{
					//
					// we have found the attribute and setup the output attribute context and return to our caller
					//
					*FoundAttribute						= TRUE;

					NtfsInitializeAttributeContext(StructureContext,NtfsFileRecordBuffer[BufferIndex],AttributeHeader,FileRecord,AttributeContext);

					NtfsUnpinFileRecord(BufferIndex);

					return ESUCCESS;
				}
			}

			NtfsUnpinFileRecord(BufferIndex);

			return EBADF;
		}
	}

	NtfsUnpinFileRecord(BufferIndex);

	return ESUCCESS;
}

//
// inexact sorted scan diretory
//
ARC_STATUS NtfsInexactSortedDirectoryScan(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__in STRING FileName,__inout PLONGLONG FileRecord,
										  __out PBOOLEAN FoundFileName,__out PBOOLEAN IsDirectory)
{
	//
	// get index_root
	//
	NTFS_ATTRIBUTE_CONTEXT IndexRootContext;
	PNTFS_ATTRIBUTE_CONTEXT IndexRoot					= &IndexRootContext;
	ARC_STATUS Status									= NtfsLookupAttribute(StructureContext,*FileRecord,$INDEX_ROOT,FoundFileName,&IndexRootContext);
	if(Status != ESUCCESS)
		return Status;

	if(*FoundFileName == FALSE)
		return EBADF;

	//
	// get index_allocation
	//
	NTFS_ATTRIBUTE_CONTEXT IndexAllocationContextBuffer;
	Status												= NtfsLookupAttribute(StructureContext,*FileRecord,$INDEX_ALLOCATION,FoundFileName,&IndexAllocationContextBuffer);
	if(Status != ESUCCESS)
		return Status;

	PNTFS_ATTRIBUTE_CONTEXT IndexAllocation				= *FoundFileName ? &IndexAllocationContextBuffer : 0;

	//
	// get bitmap
	//
	NTFS_ATTRIBUTE_CONTEXT BitmapContextBuffer;
	Status												= NtfsLookupAttribute(StructureContext,*FileRecord,$BITMAP,FoundFileName,&BitmapContextBuffer);
	if(Status != ESUCCESS)
		return Status;

	PNTFS_ATTRIBUTE_CONTEXT Bitmap						= *FoundFileName ? &BitmapContextBuffer : 0;

	//
	// read file record
	//
	ULONG BufferIndex;
	Status												= NtfsReadAndDecodeFileRecord(StructureContext,IndexRootContext.FileRecord,&BufferIndex);
	if(Status != ESUCCESS)
		return Status;

	PATTRIBUTE_RECORD_HEADER IndexAttributeHeader		= Add2Ptr(NtfsFileRecordBuffer[BufferIndex],IndexRoot->FileRecordOffset,PATTRIBUTE_RECORD_HEADER);
	PINDEX_ROOT IndexRootValue							= static_cast<PINDEX_ROOT>(NtfsGetValue(IndexAttributeHeader));
	PINDEX_HEADER IndexHeader							= &IndexRootValue->IndexHeader;
	ULONG BytesPerIndexBuffer							= IndexRootValue->BytesPerIndexBuffer;
	*FoundFileName										= FALSE;

	while(1)
	{
		//
		// search the current index buffer (from index header looking for a match)
		//
		PINDEX_ENTRY IndexEntry							= Add2Ptr(IndexHeader,IndexHeader->FirstIndexEntry,PINDEX_ENTRY);
		for(; !(IndexEntry->Flags & INDEX_ENTRY_END); IndexEntry = Add2Ptr(IndexEntry,IndexEntry->Length,PINDEX_ENTRY))
		{
			//
			// get the FileName for this index entry
			//
			PFILE_NAME FileNameEntry					= Add2Ptr(IndexEntry,sizeof(INDEX_ENTRY),PFILE_NAME);

			UNICODE_STRING UnicodeFileName;
			UnicodeFileName.Length						= FileNameEntry->FileNameLength * 2;
			UnicodeFileName.Buffer						= FileNameEntry->FileName;

			//
			// check if this the name we're after if it is then say we found it and setup the output variables
			//
			LONG Result									= NtfsCompareAnsiUniNames(FileName,UnicodeFileName);
			if(!Result)
			{
				*FoundFileName							= TRUE;
				NtfsFileReferenceToLargeInteger(&IndexEntry->FileReference,FileRecord);

				*IsDirectory							= FileNameEntry->Info.FileAttributes & DUP_FILE_NAME_INDEX_PRESENT ? TRUE : FALSE;

				NtfsUnpinFileRecord(BufferIndex);

				return ESUCCESS;
			}

			//
			// search child tree
			//
			if(Result < 0)
				break;
		}

		//
		// is not a node
		//
		if(!(IndexEntry->Flags & INDEX_ENTRY_NODE))
			break;

		//
		// no index allocation buffer
		//
		if(!IndexAllocation || !Bitmap)
			break;

		//
		// get vbo
		//
		VCN IndexBlock									= *Add2Ptr(IndexEntry,IndexEntry->Length - sizeof(VCN),PVCN);
		VBO Vbo											= IndexBlock * StructureContext->BytesPerCluster;
		if(Vbo >= IndexAllocation->DataSize)
			break;

		//
		// read index allocation
		//
		Status											= NtfsReadAttribute(StructureContext,IndexAllocation,Vbo,BytesPerIndexBuffer,NtfsIndexAllocationBuffer);
		if(Status != ESUCCESS)
			return Status;

		//
		// decode usa
		//
		Status											= NtfsDecodeUsa(NtfsIndexAllocationBuffer,BytesPerIndexBuffer);
		if(Status != ESUCCESS)
			return Status;

		IndexHeader										= &NtfsIndexAllocationBuffer->IndexHeader;
	}

	NtfsUnpinFileRecord(BufferIndex);

	return ESUCCESS;
}

//
// is record allocated
//
ARC_STATUS NtfsIsRecordAllocated(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__in PNTFS_ATTRIBUTE_CONTEXT AllocationBitmap,__in ULONG BitOffset,__out PBOOLEAN IsAllocated)
{
	//
	// setup index
	//
	ULONG ByteIndex										= BitOffset / 8;
	ULONG BitIndex										= BitOffset % 8;
	UCHAR LocalByte										= 0;

	//
	// read in a single byte containing the bit we need to check
	//
	ARC_STATUS Status									= NtfsReadAttribute(StructureContext,AllocationBitmap,ByteIndex,sizeof(LocalByte),&LocalByte);
	if(Status != ESUCCESS)
		return Status;

	//
	// shift over the local byte so that the bit we want is in the low order bit and then mask it out to see if the bit is set
	//
	*IsAllocated										= (LocalByte >> BitIndex) & 0x01 ? TRUE : FALSE;

	return ESUCCESS;
}

//
// linear scan diretory
//
ARC_STATUS NtfsLinearDirectoryScan(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__in STRING FileName,__inout PLONGLONG FileRecord,
								   __out PBOOLEAN FoundFileName,__out PBOOLEAN IsDirectory)
{
	//
	// get index_root
	//
	NTFS_ATTRIBUTE_CONTEXT IndexRootContext;
	PNTFS_ATTRIBUTE_CONTEXT IndexRoot					= &IndexRootContext;
	ARC_STATUS Status									= NtfsLookupAttribute(StructureContext,*FileRecord,$INDEX_ROOT,FoundFileName,&IndexRootContext);
	if(Status != ESUCCESS)
		return Status;

	if(*FoundFileName == FALSE)
		return EBADF;

	//
	// get index_allocation
	//
	NTFS_ATTRIBUTE_CONTEXT IndexAllocationContextBuffer;
	Status												= NtfsLookupAttribute(StructureContext,*FileRecord,$INDEX_ALLOCATION,FoundFileName,&IndexAllocationContextBuffer);
	if(Status != ESUCCESS)
		return Status;

	PNTFS_ATTRIBUTE_CONTEXT IndexAllocation				= *FoundFileName ? &IndexAllocationContextBuffer : 0;

	//
	// get bitmap
	//
	NTFS_ATTRIBUTE_CONTEXT BitmapContextBuffer;
	Status												= NtfsLookupAttribute(StructureContext,*FileRecord,$BITMAP,FoundFileName,&BitmapContextBuffer);
	if(Status != ESUCCESS)
		return Status;

	PNTFS_ATTRIBUTE_CONTEXT Bitmap						= *FoundFileName ? &BitmapContextBuffer : 0;

	//
	// read file record
	//
	ULONG BufferIndex;
	Status												= NtfsReadAndDecodeFileRecord(StructureContext,IndexRootContext.FileRecord,&BufferIndex);
	if(Status != ESUCCESS)
		return Status;

	PATTRIBUTE_RECORD_HEADER IndexAttributeHeader		= Add2Ptr(NtfsFileRecordBuffer[BufferIndex],IndexRoot->FileRecordOffset,PATTRIBUTE_RECORD_HEADER);
	PINDEX_ROOT IndexRootValue							= static_cast<PINDEX_ROOT>(NtfsGetValue(IndexAttributeHeader));
	PINDEX_HEADER IndexHeader							= &IndexRootValue->IndexHeader;
	ULONG BytesPerIndexBuffer							= IndexRootValue->BytesPerIndexBuffer;
	*FoundFileName										= FALSE;
	ULONG NextIndexBuffer								= 0;

	while(1)
	{
		//
		// search the current index buffer (from index header looking for a match)
		//
		PINDEX_ENTRY IndexEntry							= Add2Ptr(IndexHeader,IndexHeader->FirstIndexEntry,PINDEX_ENTRY);
		for(; !(IndexEntry->Flags & INDEX_ENTRY_END); IndexEntry = Add2Ptr(IndexEntry,IndexEntry->Length,PINDEX_ENTRY))
		{
			//
			// get the FileName for this index entry
			//
			PFILE_NAME FileNameEntry					= Add2Ptr(IndexEntry,sizeof(INDEX_ENTRY),PFILE_NAME);

			UNICODE_STRING UnicodeFileName;
			UnicodeFileName.Length						= FileNameEntry->FileNameLength * 2;
			UnicodeFileName.Buffer						= FileNameEntry->FileName;

			//
			// check if this the name we're after if it is then say we found it and setup the output variables
			//
			LONG Result									= NtfsCompareAnsiUniNames(FileName,UnicodeFileName);
			if(!Result)
			{
				*FoundFileName							= TRUE;
				NtfsFileReferenceToLargeInteger(&IndexEntry->FileReference,FileRecord);

				*IsDirectory							= FileNameEntry->Info.FileAttributes & DUP_FILE_NAME_INDEX_PRESENT ? TRUE : FALSE;

				NtfsUnpinFileRecord(BufferIndex);

				return ESUCCESS;
			}
		}

		//
		// no index allocation buffer
		//
		if(!IndexAllocation || !Bitmap)
			break;

		//
		// read valid index buffer
		//
		BOOLEAN IsAllocated								= FALSE;
		VBO Vbo											= 0;
		while(!IsAllocated)
		{
			//
			// compute the starting vbo of the next index buffer and check if it is still within the data size.
			//
			Vbo											= BytesPerIndexBuffer * NextIndexBuffer;

			if(Vbo >= IndexAllocation->DataSize)
				break;

			//
			// check if the index buffer is in use
			//
			Status										= NtfsIsRecordAllocated(StructureContext,Bitmap,NextIndexBuffer,&IsAllocated);
			if(Status != ESUCCESS)
				return Status;

			NextIndexBuffer								+= 1;
		}

		if(Vbo >= IndexAllocation->DataSize)
			break;

		//
		// read index allocation
		//
		Status											= NtfsReadAttribute(StructureContext,IndexAllocation,Vbo,BytesPerIndexBuffer,NtfsIndexAllocationBuffer);
		if(Status != ESUCCESS)
			return Status;

		//
		// decode usa
		//
		Status											= NtfsDecodeUsa(NtfsIndexAllocationBuffer,BytesPerIndexBuffer);
		if(Status != ESUCCESS)
			return Status;

		IndexHeader										= &NtfsIndexAllocationBuffer->IndexHeader;
	}

	NtfsUnpinFileRecord(BufferIndex);

	return ESUCCESS;
}

//
// search file
//
ARC_STATUS NtfsSearchForFileName(__in PNTFS_STRUCTURE_CONTEXT StructureContext,__in STRING FileName,__inout PLONGLONG FileRecord,
								 __out PBOOLEAN FoundFileName,__out PBOOLEAN IsDirectory)
{
	//
	// check name cache first
	//
	if(NtfsIsNameCached(StructureContext,FileName,FileRecord,FoundFileName,IsDirectory))
		return ESUCCESS;

	//
	// parent file record
	//
	LONGLONG ParentFileRecord							= *FileRecord;

	//
	// b-tree search
	//
	ARC_STATUS Status									= NtfsInexactSortedDirectoryScan(StructureContext,FileName,FileRecord,FoundFileName,IsDirectory);
	if(Status != ESUCCESS)
		return Status;

	//
	// file not found with b-tree search,then try linear search,but why did b-tree search fail?
	//
	if(*FoundFileName == FALSE)
	{
		//
		// linear search
		//
		Status											= NtfsLinearDirectoryScan(StructureContext,FileName,FileRecord,FoundFileName,IsDirectory);
		if(Status != ESUCCESS)
			return Status;
	}

	//
	// add to cache if this is a directory
	//
	if(*IsDirectory && *FoundFileName)
		NtfsAddNameToCache(StructureContext,FileName,ParentFileRecord,*FileRecord);

	return ESUCCESS;
}

//
// invalidate cache entries
//
VOID NtfsInvalidateCacheEntries(__in ULONG DeviceId)
{
	ULONG Count											= 0;
	for(ULONG i = 0; i < ARRAYSIZE(NtfsLinkCache); i ++)
	{
		if(NtfsLinkCache[i].DeviceId != DeviceId)
			continue;

		NtfsLinkCache[i].DeviceId						= 0xffffffff;
		Count											+= 1;
	}

	if(NtfsLinkCacheCount < Count)
		NtfsLinkCacheCount								= 0;
	else
		NtfsLinkCacheCount								-= Count;
}

//
// initialize
//
ARC_STATUS NtfsInitialize()
{
	//
	// initialize link cache
	//
	for(ULONG i = 0; i < ARRAYSIZE(NtfsLinkCache); i ++)
		NtfsLinkCache[i].DeviceId						= 0xffffffff;

	//
	// setup buffers
	//
	RtlZeroMemory(NtfsFileRecordBuffer,sizeof(NtfsFileRecordBuffer));
	NtfsFileRecordBuffer[0]								= static_cast<PFILE_RECORD_SEGMENT_HEADER>(ALIGN_BUFFER(NtfsBuffer0));
	NtfsFileRecordBuffer[1]								= static_cast<PFILE_RECORD_SEGMENT_HEADER>(ALIGN_BUFFER(NtfsBuffer1));
	NtfsIndexAllocationBuffer							= static_cast<PINDEX_ALLOCATION_BUFFER>(ALIGN_BUFFER(NtfsBuffer2));
	NtfsCompressedBuffer								= static_cast<PUCHAR>(ALIGN_BUFFER(NtfsBuffer3));
	NtfsUncompressedBuffer								= static_cast<PUCHAR>(ALIGN_BUFFER(NtfsBuffer4));

	//
	// register device close notification
	//
	return ArcRegisterForDeviceClose(&NtfsInvalidateCacheEntries);
}

//
// check ntfs structure
//
PBL_DEVICE_ENTRY_TABLE IsNtfsFileStructure(__in ULONG DeviceId,__out PVOID OpaqueStructureContext)
{
	PNTFS_STRUCTURE_CONTEXT StructureContext			= static_cast<PNTFS_STRUCTURE_CONTEXT>(OpaqueStructureContext);

	//
	// clear the file system context block for the specified channel and initialize the global buffer pointers that we use for buffering I/O
	//
	RtlZeroMemory(StructureContext,sizeof(NTFS_STRUCTURE_CONTEXT));

	//
	// zero out the pinned buffer array because we start with nothing pinned
	// also negate the vbo array to not let us get spooked with stale data
	//
	RtlZeroMemory(NtfsFileRecordBufferPinned,sizeof(NtfsFileRecordBufferPinned));

	for(ULONG i = 0; i < ARRAYSIZE(NtfsFileRecordBufferVbo); i ++)
		NtfsFileRecordBufferVbo[i]						= -1;

	NtfsCompressedFileRecord							= 0;
	NtfsCompressedOffset								= 0;
	NtfsCompressedVbo									= 0;

	//
	// set up a local pointer that we will use to read in the boot sector and check for an Ntfs partition
	// temporarily use the global file record buffer
	//
	PPACKED_BOOT_SECTOR BootSector						= reinterpret_cast<PPACKED_BOOT_SECTOR>(NtfsFileRecordBuffer[0]);

	//
	// read in the boot sector and return null if we can't do the read
	//
	if(NtfsReadDisk(DeviceId,0,sizeof(PACKED_BOOT_SECTOR),BootSector,TRUE) != ESUCCESS)
		return 0;

	//
	// unpack the Bios parameter block
	//
	BIOS_PARAMETER_BLOCK Bpb;
	NtfsUnPackBios(&Bpb,&BootSector->PackedBpb);

	//
	// check if it is NTFS, by first checking the signature, then must be zero fields, then the media type, and then sanity check the non zero fields.
	//
	if (RtlCompareMemory(BootSector->Oem, "NTFS    ", 8) != 8)
		return 0;

	if(Bpb.ReservedSectors || Bpb.Fats || Bpb.RootEntries || Bpb.Sectors || Bpb.SectorsPerFat || Bpb.LargeSectors)
		return 0;

	if(Bpb.Media != 0xf0 && Bpb.Media != 0xf8 && Bpb.Media != 0xf9 && Bpb.Media != 0xfc && Bpb.Media != 0xfd && Bpb.Media != 0xfe && Bpb.Media != 0xff)
		return 0;

	if(Bpb.BytesPerSector !=  128 && Bpb.BytesPerSector !=  256 && Bpb.BytesPerSector !=  512 && Bpb.BytesPerSector != 1024 && Bpb.BytesPerSector != 2048)
		return 0;

	if( Bpb.SectorsPerCluster !=  1 && Bpb.SectorsPerCluster !=  2 && Bpb.SectorsPerCluster !=  4 && Bpb.SectorsPerCluster !=  8 &&
		Bpb.SectorsPerCluster != 16 && Bpb.SectorsPerCluster != 32 && Bpb.SectorsPerCluster != 64 && Bpb.SectorsPerCluster != 128)
	{
		return 0;
	}

	if( !BootSector->NumberSectors || !BootSector->MftStartLcn || !BootSector->Mft2StartLcn ||
		!BootSector->ClustersPerFileRecordSegment || !BootSector->DefaultClustersPerIndexAllocationBuffer)
	{
		return 0;
	}

	if(BootSector->ClustersPerFileRecordSegment < 0 && (BootSector->ClustersPerFileRecordSegment > -9 || BootSector->ClustersPerFileRecordSegment < -31))
		return 0;

	//
	// so far the boot sector has checked out to be an NTFS partition so now compute some of the volume constants.
	//
	StructureContext->DeviceId							= DeviceId;
	StructureContext->BytesPerCluster					= Bpb.SectorsPerCluster * Bpb.BytesPerSector;
	ULONG ClusterSize									= StructureContext->BytesPerCluster;

	//
	// if the number of clusters per file record is less than zero then the file record size computed by using the negative of this number as a shift value.
	//
	if(BootSector->ClustersPerFileRecordSegment > 0)
		StructureContext->BytesPerFileRecord			= BootSector->ClustersPerFileRecordSegment * ClusterSize;
	else
		StructureContext->BytesPerFileRecord			= 1 << (-1 * BootSector->ClustersPerFileRecordSegment);

	ULONG FileRecordSize								= StructureContext->BytesPerFileRecord;

	//
	// read in the base file record for the mft
	//
	if(NtfsReadDisk(DeviceId,BootSector->MftStartLcn * ClusterSize,FileRecordSize,NtfsFileRecordBuffer[0],TRUE) != ESUCCESS)
		return 0;

	//
	// decode Usa for the file record
	//
	if(NtfsDecodeUsa(NtfsFileRecordBuffer[0],FileRecordSize) != ESUCCESS)
		return 0;

	//
	//  Make sure the file record is in use
	//
	if(!(NtfsFileRecordBuffer[0]->Flags & FILE_RECORD_SEGMENT_IN_USE))
		return 0;

	//
	// search for the unnamed $data attribute header
	//
	PATTRIBUTE_RECORD_HEADER AttributeHeader			= NtfsFirstAttribute(NtfsFileRecordBuffer[0]);
	for(; AttributeHeader->TypeCode != $DATA || AttributeHeader->NameLength; AttributeHeader = NtfsGetNextRecord(AttributeHeader))
	{
		if(AttributeHeader->TypeCode == $END)
			return 0;
	}

	//
	// make sure the $data attribute for the mft is non resident
	//
	if(AttributeHeader->FormCode != NONRESIDENT_FORM)
		return 0;

	//
	// set the mft structure context up for later use
	//
	NtfsInitializeAttributeContext(StructureContext,NtfsFileRecordBuffer[0],AttributeHeader,0,&StructureContext->MftAttributeContext);

	//
	// decipher the part of the Mcb that is stored in the file record
	//
	if(NtfsDecodeRetrievalInformation(StructureContext,&StructureContext->MftBaseMcb,0,AttributeHeader) != ESUCCESS)
		return 0;

	//
	// return device entry table
	//
	extern BL_DEVICE_ENTRY_TABLE NtfsDeviceEntryTable;
	return &NtfsDeviceEntryTable;
}

//
// close
//
ARC_STATUS NtfsClose(__in ULONG FileId)
{
	//
	// set as file not opened
	//
	BlFileTable[FileId].Flags.Open						= FALSE;

	return ESUCCESS;
}

//
// open
//
ARC_STATUS NtfsOpen(__in PCHAR OpenPath,__in OPEN_MODE OpenMode,__out PULONG FileId)
{
	PBL_FILE_TABLE FileTableEntry						= &BlFileTable[*FileId];
	PNTFS_STRUCTURE_CONTEXT StructureContext			= static_cast<PNTFS_STRUCTURE_CONTEXT>(FileTableEntry->StructureContext);
	PNTFS_FILE_CONTEXT FileContext						= &FileTableEntry->u.NtfsFileContext;

	//
	// zero out the file context and position information in the file table entry
	//
	FileTableEntry->Position.QuadPart					= 0;

	RtlZeroMemory(FileContext,sizeof(NTFS_FILE_CONTEXT));

	//
	// construct a file name descriptor from the input file name
	//
	STRING PathName;
	RtlInitAnsiString(&PathName,OpenPath);

	//
	// open the root directory as our starting point,the root directory file reference number is 5.
	//
	LONGLONG FileRecord									= 5;
	BOOLEAN IsDirectory									= TRUE;

	//
	// while the path name has some characters left in it and current attribute context is a directory we will continue our search
	//
	while(PathName.Length > 0 && IsDirectory)
	{
		//
		// extract the first component and search the directory for a match, but first copy the first part to the file name buffer in the file table entry
		//
		if(PathName.Buffer[0] == '\\')
		{
			PathName.Buffer								+= 1;
			PathName.Length								-= sizeof(CHAR);
		}

		UCHAR i											= 0;
		for(; i < PathName.Length && PathName.Buffer[i] != '\\'; i ++)
			FileTableEntry->FileName[i]					= PathName.Buffer[i];

		FileTableEntry->FileNameLength					= i;

		STRING Name;
		NtfsFirstComponent(&PathName,&Name);

		//
		// search for the name in the current directory
		//
		BOOLEAN Found									= FALSE;
		ARC_STATUS Status								= NtfsSearchForFileName(StructureContext,Name,&FileRecord,&Found,&IsDirectory);
		if(Status != ESUCCESS)
			return Status;

		//
		// if we didn't find it then we should get out right now
		//
		if(!Found)
			return ENOENT;
	}

	//
	// at this point we have exhausted our pathname or we did not get a directory
	// check if we didn't get a directory and we still have a name to crack
	//
	if(PathName.Length > 0)
		return ENOTDIR;

	//
	// FileRecord is the one we wanted to open.
	// check the various open modes against what we have located
	//
	if(IsDirectory)
	{
		switch(OpenMode)
		{
		case ArcOpenDirectory:
			{
				//
				// to open the directory we will lookup the index root as our file context and then increment the appropriate counters.
				//
				BOOLEAN Found							= FALSE;
				ARC_STATUS Status						= NtfsLookupAttribute(StructureContext,FileRecord,$INDEX_ROOT,&Found,FileContext);
				if(Status != ESUCCESS)
					return Status;

				if(!Found)
					return EBADF;

				FileTableEntry->Flags.Open				= TRUE;
				FileTableEntry->Flags.Read				= TRUE;

				return ESUCCESS;
			}
			break;

		case ArcCreateDirectory:
			return EROFS;
			break;

		default:
			return EISDIR;
			break;
		}
	}

	//
	// open as file
	//
	switch(OpenMode)
	{
	case ArcOpenReadWrite:
		//
		// only allow opening hiber file and boot state as read/write
		//
		if(!strstr(OpenPath,"\\hiberfil.sys") && !strstr(OpenPath,"\\bootstat.dat"))
			return EROFS;
		//
		// fall-through
		//

	case ArcOpenReadOnly:
		{
			//
			// to open the file we will lookup the $data as our file context and then increment the appropriate counters.
			//
			BOOLEAN Found								= FALSE;
			ARC_STATUS Status							= NtfsLookupAttribute(StructureContext,FileRecord,$DATA,&Found,FileContext);
			if(Status != ESUCCESS)
				return Status;

			if(!Found)
				return EBADF;

			FileTableEntry->Flags.Open					= TRUE;
			FileTableEntry->Flags.Read					= TRUE;

			if(OpenMode == ArcOpenReadWrite)
				FileTableEntry->Flags.Write				= TRUE;

			return ESUCCESS;
		}
		break;

	case ArcOpenDirectory:
		return ENOTDIR;
		break;

	default:
		return EROFS;
		break;
	}

	return EROFS;
}

//
// read
//
ARC_STATUS NtfsRead(__in ULONG FileId,__out PVOID Buffer,__in ULONG Length,__out PULONG Count)
{
	PBL_FILE_TABLE FileTableEntry						= &BlFileTable[FileId];
	PNTFS_STRUCTURE_CONTEXT StructureContext			= static_cast<PNTFS_STRUCTURE_CONTEXT>(FileTableEntry->StructureContext);
	PNTFS_FILE_CONTEXT FileContext						= &FileTableEntry->u.NtfsFileContext;

	//
	// compute the amount left in the file and then from that we compute the amount for the transfer
	//
	LONGLONG AmountLeft									= FileContext->DataSize - FileTableEntry->Position.QuadPart;

	if(Length <= AmountLeft)
		*Count											= Length;
	else
		*Count											= static_cast<ULONG>(AmountLeft);

	//
	// read attribute
	//
	ARC_STATUS Status									= NtfsReadAttribute(StructureContext,FileContext,FileTableEntry->Position.QuadPart,*Count,Buffer);
	if(Status != ESUCCESS)
		return Status;

	//
	// update the current position
	//
	FileTableEntry->Position.QuadPart					= FileTableEntry->Position.QuadPart + *Count;

	return ESUCCESS;
}

//
// seek
//
ARC_STATUS NtfsSeek(__in ULONG FileId,__in PLARGE_INTEGER Offset,__in SEEK_MODE SeekMode)
{
	PBL_FILE_TABLE FileTableEntry						= &BlFileTable[FileId];
	LONGLONG NewPosition;

	//
	// compute the new position
	//
	if(SeekMode == SeekAbsolute)
		NewPosition										= Offset->QuadPart;
	else
		NewPosition										= FileTableEntry->Position.QuadPart + Offset->QuadPart;

	//
	// if the new position is greater than the file size then return an error
	//
	if(NewPosition > FileTableEntry->u.NtfsFileContext.DataSize)
		return EINVAL;

	//
	// otherwise set the new position and return to our caller
	//
	FileTableEntry->Position.QuadPart					= NewPosition;

	return ESUCCESS;
}

//
// write
//
ARC_STATUS NtfsWrite(__in ULONG FileId,__in PVOID Buffer,__in ULONG Length,__out PULONG Count)
{
	PBL_FILE_TABLE FileTableEntry						= &BlFileTable[FileId];
	PNTFS_STRUCTURE_CONTEXT StructureContext			= static_cast<PNTFS_STRUCTURE_CONTEXT>(FileTableEntry->StructureContext);
	PNTFS_FILE_CONTEXT FileContext						= &FileTableEntry->u.NtfsFileContext;

	//
	// compute the amount left in the file and then from that we compute the amount for the transfer
	//
	LONGLONG AmountLeft									= FileContext->DataSize - FileTableEntry->Position.QuadPart;

	if(Length <= AmountLeft)
		*Count											= Length;
	else
		*Count											= static_cast<ULONG>(AmountLeft);

	//
	// write attribute
	//
	ARC_STATUS Status									= NtfsWriteAttribute(StructureContext,FileContext,FileTableEntry->Position.QuadPart,*Count,Buffer);
	if(Status != ESUCCESS)
		return Status;

	//
	// update the current position
	//
	FileTableEntry->Position.QuadPart					= FileTableEntry->Position.QuadPart + *Count;

	return ESUCCESS;
}

//
// get file info
//
ARC_STATUS NtfsGetFileInformation(__in ULONG FileId,__out PFILE_INFORMATION FileInformation)
{
	PBL_FILE_TABLE FileTableEntry						= &BlFileTable[FileId];
	PNTFS_STRUCTURE_CONTEXT StructureContext			= static_cast<PNTFS_STRUCTURE_CONTEXT>(FileTableEntry->StructureContext);
	PNTFS_FILE_CONTEXT FileContext						= &FileTableEntry->u.NtfsFileContext;

	//
	// zero out the output buffer and fill in its non-zero values
	//
	RtlZeroMemory(FileInformation,sizeof(FILE_INFORMATION));
	FileInformation->EndingAddress.QuadPart				= FileContext->DataSize;
	FileInformation->CurrentPosition					= FileTableEntry->Position;

	//
	// locate and read in the standard information for the file.
	//
	NTFS_ATTRIBUTE_CONTEXT AttributeContext;
	BOOLEAN Found										= FALSE;
	ARC_STATUS Status									= NtfsLookupAttribute(StructureContext,FileContext->FileRecord,$STANDARD_INFORMATION,&Found,&AttributeContext);
	if(ESUCCESS != Status)
		return Status;

	if(!Found)
		return EBADF;

	STANDARD_INFORMATION StandardInformation;
	Status												= NtfsReadAttribute(StructureContext,&AttributeContext,0,sizeof(STANDARD_INFORMATION),&StandardInformation);
	if(ESUCCESS != Status)
		return Status;

	//
	// now check for set bits in the standard information structure and set the appropriate bits in the output buffer
	//
	if(StandardInformation.FileAttributes & FAT_DIRENT_ATTR_READ_ONLY)
		FileInformation->Attributes						|= ArcReadOnlyFile;

	if(StandardInformation.FileAttributes & FAT_DIRENT_ATTR_HIDDEN)
		FileInformation->Attributes						|= ArcHiddenFile;

	if(StandardInformation.FileAttributes & FAT_DIRENT_ATTR_SYSTEM)
		FileInformation->Attributes						|= ArcSystemFile;

	if(StandardInformation.FileAttributes & FAT_DIRENT_ATTR_ARCHIVE)
		FileInformation->Attributes						|= ArcArchiveFile;

	if(StandardInformation.FileAttributes & DUP_FILE_NAME_INDEX_PRESENT)
		FileInformation->Attributes						|= ArcDirectoryFile;

	//
	// get the file name from the file table entry
	//
	FileInformation->FileNameLength						= FileTableEntry->FileNameLength;
	for(ULONG i = 0; i < FileTableEntry->FileNameLength; i ++)
		FileInformation->FileName[i]					= FileTableEntry->FileName[i];

	return ESUCCESS;
}

//
// set file info
//
ARC_STATUS NtfsSetFileInformation(__in ULONG FileId,__in ULONG AttributeFlags,__in ULONG AttributeMask)
{
	return EROFS;
}

//
// boot info
//
BOOTFS_INFO												NtfsBootFsInfo = {L"ntfs"};

//
// device entry table
//
BL_DEVICE_ENTRY_TABLE NtfsDeviceEntryTable =
{
	&NtfsClose,
	0,
	&NtfsOpen,
	&NtfsRead,
	0,
	&NtfsSeek,
	&NtfsWrite,
	&NtfsGetFileInformation,
	&NtfsSetFileInformation,
	0,
	0,
	&NtfsBootFsInfo,
};

#endif