//********************************************************************
//	created:	16:8:2008   13:49
//	file:		blres.cpp
//	author:		tiamo
//	purpose:	pe resource support
//********************************************************************

#include "stdafx.h"

//
// search resource directory by id
//
PVOID BlpFindDirectoryEntry(__in PIMAGE_RESOURCE_DIRECTORY Directory,__in ULONG Id,__in PVOID SectionStart)
{
	PIMAGE_RESOURCE_DIRECTORY_ENTRY FoundDirectory		= Add2Ptr(Directory,sizeof(IMAGE_RESOURCE_DIRECTORY),PIMAGE_RESOURCE_DIRECTORY_ENTRY);

	//
	// skip entries with names.
	//
	for(ULONG i = 0; i < Directory->NumberOfNamedEntries; i ++)
		FoundDirectory									+= 1;

	//
	// Search for matching ID.
	//
	for(ULONG i = 0; i < Directory->NumberOfIdEntries; i ++)
	{
		//
		// found a match.
		//
		if(FoundDirectory->Name == Id || Id == 0xffffffff)
			return Add2Ptr(SectionStart,FoundDirectory->OffsetToData & ~IMAGE_RESOURCE_DATA_IS_DIRECTORY,PVOID);

		FoundDirectory									+= 1;
	}

	return 0;
}

//
// find message by id
//
PCHAR BlFindMessage(__in ULONG Id)
{
	//
	// osloader has no resource directory
	//
	if(!BlpResourceDirectory)
		return 0;

	PIMAGE_RESOURCE_DIRECTORY ResourceDirectory			= reinterpret_cast<PIMAGE_RESOURCE_DIRECTORY>(BlpResourceDirectory);

	//
	// search the directory.  We are looking for the type RT_MESSAGETABLE (11)
	//
	PIMAGE_RESOURCE_DIRECTORY NextDirectory				= static_cast<PIMAGE_RESOURCE_DIRECTORY>(BlpFindDirectoryEntry(ResourceDirectory,11,ResourceDirectory));
	if(!NextDirectory)
		return 0;

	//
	// find the next directory.should only be one entry here (nameid == 1)
	//
	NextDirectory										= static_cast<PIMAGE_RESOURCE_DIRECTORY>(BlpFindDirectoryEntry(NextDirectory,1,ResourceDirectory));
	if(!NextDirectory)
		return 0;

	// find the message table.
	// if a dbcs locale is active, then we look for the appropriate message table first,otherwise we just look for the first message table.
	//
	PIMAGE_RESOURCE_DATA_ENTRY DataEntry				= 0;
	if(DbcsLangId)
		DataEntry										= static_cast<PIMAGE_RESOURCE_DATA_ENTRY>(BlpFindDirectoryEntry(NextDirectory,DbcsLangId,ResourceDirectory));

	//
	// try the first table
	//
	if(!DataEntry)
		DataEntry										= static_cast<PIMAGE_RESOURCE_DATA_ENTRY>(BlpFindDirectoryEntry(NextDirectory,0xffffffff,ResourceDirectory));

	if(!DataEntry)
		return 0;

	PMESSAGE_RESOURCE_DATA MessageData					= Add2Ptr(BlpResourceDirectory,DataEntry->OffsetToData - BlpResourceFileOffset,PMESSAGE_RESOURCE_DATA);
	ULONG NumberOfBlocks								= MessageData->NumberOfBlocks;
	PMESSAGE_RESOURCE_BLOCK MessageBlock				= MessageData->Blocks;

	//
	// search the message id
	//
	while(NumberOfBlocks --)
	{
		if(Id >= MessageBlock->LowId && Id <= MessageBlock->HighId)
		{
			//
			// the requested ID is within this block, scan forward until we find it.
			//
			PMESSAGE_RESOURCE_ENTRY MessageEntry	= Add2Ptr(MessageData,MessageBlock->OffsetToEntries,PMESSAGE_RESOURCE_ENTRY);
			ULONG Index								= Id - MessageBlock->LowId;

			while(Index --)
				MessageEntry						= Add2Ptr(MessageEntry,MessageEntry->Length,PMESSAGE_RESOURCE_ENTRY);

			return MessageEntry->Text;
		}

		//
		// check the next block for this ID.
		//
		MessageBlock									+= 1;
	}

	return 0;
}

//
// init resource
//
ARC_STATUS BlInitResources(__in PCHAR StartCommand)
{
	//
	// already initialized
	//
	if(BlpResourceDirectory)
		return ESUCCESS;

	//
	// extract device name from the startup path
	//
	PCHAR p												= strrchr(StartCommand,')');
	if(!p)
		return ENODEV;

	//
	// setup device name and file name
	//
	CHAR DeviceName[80];
	strncpy(DeviceName,StartCommand,p - StartCommand + 1);
	DeviceName[p - StartCommand + 1]					= 0;
	PCHAR FileName										= p + 1;

	//
	// open the device.
	//
	ULONG DeviceId										= 0;
	ARC_STATUS Status									= ArcOpen(DeviceName, ArcOpenReadOnly, &DeviceId);
	if(Status != ESUCCESS)
		return Status;

	//
	// open the file.
	//
	ULONG FileId										= 0;
	Status												= BlOpen(DeviceId,FileName,ArcOpenReadOnly,&FileId);
	if(Status != ESUCCESS)
	{
		ArcCacheClose(DeviceId);
		return Status;
	}

	//
	// read the first two sectors of the image header from the file.
	//
	UCHAR LocalBuffer[(SECTOR_SIZE * 2) + 256];
	PVOID LocalPointer									= ALIGN_BUFFER(LocalBuffer);
	ULONG Count											= 0;
	Status												= BlRead(FileId, LocalPointer,SECTOR_SIZE * 2,&Count);
	BlClose(FileId);
	ArcCacheClose(DeviceId);

	if(Status != ESUCCESS)
		return Status;

	PIMAGE_FILE_HEADER FileHeader						= static_cast<PIMAGE_FILE_HEADER>(LocalPointer);
	PIMAGE_OPTIONAL_HEADER OptionalHeader				= Add2Ptr(LocalPointer,sizeof(IMAGE_FILE_HEADER),PIMAGE_OPTIONAL_HEADER);
	ULONG NumberOfSections								= FileHeader->NumberOfSections;
	PIMAGE_SECTION_HEADER SectionHeader					= Add2Ptr(OptionalHeader,FileHeader->SizeOfOptionalHeader,PIMAGE_SECTION_HEADER);

	//
	// find .rsrc section
	//
	while(NumberOfSections)
	{
		if(!_stricmp(reinterpret_cast<PCHAR>(SectionHeader->Name),".rsrc"))
		{
			BlpResourceDirectory						= SectionHeader->VirtualAddress;
			BlpResourceFileOffset						= SectionHeader->PointerToRawData;

			if(FileHeader->Machine == IMAGE_FILE_MACHINE_POWERPC)
				BlpResourceDirectory					+= OptionalHeader->ImageBase;

			return ESUCCESS;
		}

		SectionHeader									+= 1;
		NumberOfSections								-= 1;
	}

	return EBADF;
}

//
// read resource version
//
ARC_STATUS BlGetResourceVersionInfo(__in PVOID ImageBase,__out PULONGLONG Version)
{
	PIMAGE_NT_HEADERS NtHeaders							= RtlImageNtHeader(ImageBase);
	if(!NtHeaders)
		return EBADF;

	ULONG NumberOfSections								= NtHeaders->FileHeader.NumberOfSections;
	PIMAGE_SECTION_HEADER SectionHeader					= IMAGE_FIRST_SECTION(NtHeaders);

	//
	// find .rsrc section
	//
	while(NumberOfSections)
	{
		if(!_stricmp(reinterpret_cast<PCHAR>(SectionHeader->Name),".rsrc"))
		{
			PVOID SectionStart							= Add2Ptr(ImageBase,SectionHeader->PointerToRawData,PVOID);
			PIMAGE_RESOURCE_DIRECTORY Directory			= static_cast<PIMAGE_RESOURCE_DIRECTORY>(SectionStart);

			//
			// 0x10 means version block
			//
			Directory									= static_cast<PIMAGE_RESOURCE_DIRECTORY>(BlpFindDirectoryEntry(Directory,0x10,SectionStart));
			if(!Directory)
				return EBADF;

			//
			// there should be only one entry
			//
			Directory									= static_cast<PIMAGE_RESOURCE_DIRECTORY>(BlpFindDirectoryEntry(Directory,1,SectionStart));
			if(!Directory)
				return EBADF;

			//
			// any laungage is ok
			//
			Directory									= static_cast<PIMAGE_RESOURCE_DIRECTORY>(BlpFindDirectoryEntry(Directory,0xffffffff,SectionStart));
			if(!Directory)
				return EBADF;

			//
			// got it,check start string
			// BUGBUGBUG,define a structure
			//
			PVOID VersionInfo							= Add2Ptr(SectionStart,*reinterpret_cast<PULONG>(Directory) - SectionHeader->VirtualAddress,PVOID);
			PWCHAR VersionString						= Add2Ptr(VersionInfo,6,PWCHAR);
			PWCHAR CheckString							= L"VS_VERSION_INFO";
			if(_wcsicmp(VersionString,L"VS_VERSION_INFO"))
				return EBADF;

			ULARGE_INTEGER VersionValue;
			VersionValue.HighPart						= *Add2Ptr(VersionInfo,0x30,PULONG);
			VersionValue.LowPart						= *Add2Ptr(VersionInfo,0x34,PULONG);

			*Version									= VersionValue.QuadPart;

			return ESUCCESS;
		}

		SectionHeader									+= 1;
		NumberOfSections								-= 1;
	}

	return EBADF;
}