//********************************************************************
//	created:	16:8:2008   14:00
//	file:		blres.h
//	author:		tiamo
//	purpose:	pe resource support
//********************************************************************

#pragma once

//
// message resource block
//
typedef struct _MESSAGE_RESOURCE_BLOCK
{
	//
	// lowest message id in thes block
	//
	ULONG												LowId;

	//
	// highest message id
	//
	ULONG												HighId;

	//
	// offset to entries
	//
	ULONG												OffsetToEntries;
}MESSAGE_RESOURCE_BLOCK,*PMESSAGE_RESOURCE_BLOCK;

//
// message resource data
//
typedef struct _MESSAGE_RESOURCE_DATA
{
	//
	// blocks
	//
	ULONG												NumberOfBlocks;

	//
	// block array
	//
	MESSAGE_RESOURCE_BLOCK								Blocks[1];
}MESSAGE_RESOURCE_DATA,*PMESSAGE_RESOURCE_DATA;

//
// resource entry
//
typedef struct _MESSAGE_RESOURCE_ENTRY
{
	//
	// total length
	//
	USHORT												Length;

	//
	// flags
	//
	USHORT												Flags;

	//
	// text buffer
	//
	CHAR												Text[1];
}MESSAGE_RESOURCE_ENTRY,*PMESSAGE_RESOURCE_ENTRY;

//
// find message by id
//
PCHAR BlFindMessage(__in ULONG Id);

//
// init resource
//
ARC_STATUS BlInitResources(__in PCHAR StartCommand);

//
// read resource version
//
ARC_STATUS BlGetResourceVersionInfo(__in PVOID ImageBase,__out PULONGLONG Version);