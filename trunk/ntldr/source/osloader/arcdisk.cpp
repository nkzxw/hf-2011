//********************************************************************
//	created:	17:8:2008   19:24
//	file:		arcdisk.cpp
//	author:		tiamo
//	purpose:	arc disk
//********************************************************************

#include "stdafx.h"

typedef BOOLEAN (*PGPT_DISK_READ_CALLBACK)(__in ULONGLONG LBA,__in ULONG Count,__in PULONG DiskId,__in PVOID Buffer);

//
// cached disk info
//
ARC_DISK_INFORMATION									AEArcDiskInformation;

//
// cached disk info is initialized
//
BOOLEAN													AEArcDiskInformationInitialized;

#if _EFI_SUPPORT_
//
// temp buffer used by gpt disk routines
//
UCHAR													GptScratchBuffer2[1024];

//
// temp buffer
//
UCHAR													GptScratchBuffer[0x4400];

//
// partition buffer
//
UCHAR													EfiPartitionBuffer[0x4000];
#endif

//
// setup int13 extension info
//
VOID BlpSetArcDiskInt13ExtensionInfo()
{
	CHAR DiskName[100];
	PLIST_ENTRY NextEntry								= BlLoaderBlock->ArcDiskInformation->DiskSignatures.Flink;
	while(NextEntry != &BlLoaderBlock->ArcDiskInformation->DiskSignatures)
	{
		PARC_DISK_SIGNATURE Disk						= CONTAINING_RECORD(NextEntry,ARC_DISK_SIGNATURE,ListEntry);

		//
		// open partition 0.etc the whole disk
		//
		strcpy(DiskName,Disk->ArcName);
		strcat(DiskName,"partition(0)");

		ULONG DiskId;
		if(ArcOpen(DiskName,ArcOpenReadOnly,&DiskId) == ESUCCESS)
		{
			//
			// HACK,copy from drive context
			//
			Disk->SupportInt13Extension					= BlFileTable[DiskId].u.DriveContext.Int13Ext;

			ArcCacheClose(DiskId);
		}
		else
		{
			Disk->SupportInt13Extension					= FALSE;
		}


		NextEntry										= NextEntry->Flink;
	}
}

//
// find disk signature
//
BOOLEAN BlFindDiskSignature(__in PCHAR DiskName,__inout PARC_DISK_SIGNATURE Signature)
{
	if(!AEArcDiskInformationInitialized)
		return FALSE;

	PLIST_ENTRY NextEntry								= AEArcDiskInformation.DiskSignatures.Flink;

	//
	// NTDETECT creates an "eisa(0)..." arcname for detected BIOS disks on an EISA machine.
	// change this to "multi(0)..." in order to be consistent with the rest of the system (particularly the arcname in boot.ini)
	//
	CHAR FixedName[32]									= "multi(xxx)disk(xxx)rdisk(xxx)";
	PCHAR FixedDiskName									= FixedName;
	if(!strncmp(DiskName,"eisa",4))
		strcpy(FixedName + 5,DiskName + 4);
	else
		FixedDiskName									= DiskName;

	while(NextEntry != &AEArcDiskInformation.DiskSignatures)
	{
		PARC_DISK_SIGNATURE NextDisk					= CONTAINING_RECORD(NextEntry,ARC_DISK_SIGNATURE,ListEntry);

		//
		// compare those name
		//
		if(!strcmp(DiskName,NextDisk->ArcName))
		{
			//
			// found one,copy signature from it
			//
			Signature->Signature						= NextDisk->Signature;

			//
			// also name,checksum and partition table info
			//
			strcpy(Signature->ArcName,NextDisk->ArcName);
			Signature->ValidPartitionTable				= NextDisk->ValidPartitionTable;
			Signature->CheckSum							= NextDisk->CheckSum;

			return TRUE;
		}

		NextEntry										= NextEntry->Flink;
	}

	DbgPrint("BlFindDiskSignature found no match for %s\n",DiskName);

	return FALSE;
}

#if _EFI_SUPPORT_
//
// read gpt disk callback
//
BOOLEAN ArcDiskGPTDiskReadCallback(__in ULONGLONG LBA,__in ULONG Count,__in PULONG DiskId,__in PVOID Buffer)
{
	//
	// convert LBA to byte offset
	//
	LARGE_INTEGER LBALarge;
	LBALarge.QuadPart									= LBA << SECTOR_SHIFT;

	//
	// seek to it
	//
	if(ArcSeek(*DiskId,&LBALarge,SeekAbsolute) != ESUCCESS)
		return FALSE;

	//
	// read it
	//
	ULONG ReadCount;
	return ArcRead(*DiskId,Buffer,Count,&ReadCount) == ESUCCESS && ReadCount ==  Count;
}

//
// check valid efi partition
//
BOOLEAN BlIsValidGUIDPartitionTableHelper(__in PVOID Buffer,__in ULONGLONG LBA,__in PULONG DiskId,__in PGPT_DISK_READ_CALLBACK Callback)
{
	//
	// check signature
	//
	if(strncmp(static_cast<PCHAR>(Buffer),"EFI PART",8))
		return FALSE;

	//
	// compute checksum
	//	offset 0x10 = checksum
	//	offset 0x0c	= header size
	//
	ULONG SavedChecksum									= *Add2Ptr(Buffer,0x10,PULONG);
	*Add2Ptr(Buffer,0x10,PULONG)						= 0;
	ULONG CheckSum										= RtlComputeCrc32(0,Buffer,*Add2Ptr(Buffer,0x0c,PULONG));
	*Add2Ptr(Buffer,0x10,PULONG)						= SavedChecksum;
	if(CheckSum != SavedChecksum)
		return FALSE;

	//
	// check MyLBA
	//
	if(*Add2Ptr(Buffer,0x18,PULONGLONG) != LBA)
		return FALSE;

	ULONGLONG PartitionEntryLBA							= *Add2Ptr(Buffer,0x48,PULONGLONG);
	ULONG PartitionSize									= (*Add2Ptr(Buffer,0x54,PULONG)) * (*Add2Ptr(Buffer,0x50,PLONG));
	CheckSum											= 0;
	while(PartitionSize)
	{
		ULONG ReadSize									= PartitionSize > 0x4000 ? 0x4000 : PartitionSize;
		if(!Callback(PartitionEntryLBA,ReadSize,DiskId,GptScratchBuffer))
			return FALSE;

		CheckSum										= RtlComputeCrc32(CheckSum,GptScratchBuffer,ReadSize);

		PartitionSize									-= ReadSize;
		PartitionEntryLBA								+= (ReadSize >> SECTOR_SHIFT);
	}

	//
	// check PartitionEntryArrayCRC32
	//
	return CheckSum == *Add2Ptr(Buffer,0x58,PULONG);
}

//
// check valid efi partition
//
BOOLEAN BlIsValidGUIDPartitionTable(__in PVOID Buffer,__in ULONGLONG LBA,__in PULONG DiskId,__in PGPT_DISK_READ_CALLBACK Callback)
{
	//
	// valid this table first
	//
	if(!BlIsValidGUIDPartitionTableHelper(Buffer,LBA,DiskId,Callback))
		return FALSE;

	//
	// this is the backup table
	//
	if(LBA != 1)
		return TRUE;

	//
	// read the backup table
	//	offset 0x20	= AlternateLBA
	//	offset 0x0c	= HeaderSize
	//	bad...define as a structure?
	//
	if(!Callback(*Add2Ptr(Buffer,0x20,PULONGLONG),*Add2Ptr(Buffer,0x0c,PULONG),DiskId,GptScratchBuffer2))
		return FALSE;

	//
	// check it
	//
	return BlIsValidGUIDPartitionTableHelper(Buffer,*Add2Ptr(Buffer,0x20,PULONGLONG),DiskId,Callback);
}
#endif

//
// get disk signature
//
BOOLEAN BlGetDiskSignature(__in PCHAR DiskName,__in BOOLEAN IsCdRom,__inout PARC_DISK_SIGNATURE Signature)
{
	CHAR LocalSectorBuffer[2048 + 256];
	CHAR Partition[100];

	//
	// convert eisa to multi
	//
	if(!_strnicmp(DiskName,"eisa",4))
	{
		strcpy(Signature->ArcName,"multi");
		strcpy(Partition,"multi");
		strcat(Signature->ArcName,DiskName + 4);
		strcat(Partition,DiskName + 4);
	}
	else
	{
		strcpy(Signature->ArcName,DiskName);
		strcpy(Partition,DiskName);
	}

	//
	// we need to read partition to get disk signature
	//
	strcat(Partition,"partition(0)");

	//
	// setup sector size according to device type
	//
	ULONG SectorSize									= IsCdRom ? 2048 : SECTOR_SIZE;

	//
	// open partition 0
	//
	ULONG DiskId;
	ARC_STATUS Status									= ArcOpen(Partition,ArcOpenReadOnly,&DiskId);
	if(Status != ESUCCESS)
		return FALSE;

	//
	// seek to first sector
	//
	LARGE_INTEGER SeekOffset;
	SeekOffset.QuadPart									= 0;
	Status												= ArcSeek(DiskId,&SeekOffset,SeekAbsolute);
	if(Status == ESUCCESS)
	{
		//
		// read it
		//
		PVOID SectorBuffer								= ALIGN_BUFFER(LocalSectorBuffer);
		ULONG Count										= 0;
		Status											= ArcRead(DiskId,SectorBuffer,SectorSize,&Count);
		if(Status == ESUCCESS)
		{
			//
			// check the last word,if it is 0xaa55,the is a MBR with valid partition table
			//
			Signature->ValidPartitionTable				= *Add2Ptr(SectorBuffer,0x1fe,PUSHORT) == 0xaa55;

			//
			// disk signature is just before partiton table
			//
			Signature->Signature						= *Add2Ptr(SectorBuffer,0x1b8,PULONG);

			//
			// calc checksum
			//
			Signature->CheckSum							= 0;
			for(ULONG i = 0; i < SectorSize; i += sizeof(ULONG))
				Signature->CheckSum						+= *Add2Ptr(SectorBuffer,i,PULONG);

			Signature->CheckSum							= ~Signature->CheckSum + 1;

			//
			// does it has an EFI partition
			//
			Signature->IsGptDisk						= FALSE;

		#if _EFI_SUPPORT_
			if(!IsCdRom)
			{
				//
				// efi partition table header is in the second sector
				//
				SeekOffset.QuadPart						= SectorSize;
				if(ArcSeek(DiskId,&SeekOffset,SeekAbsolute) == ESUCCESS)
				{
					//
					// read the second buffer
					//
					if(ArcRead(DiskId,SectorBuffer,SectorSize,&Count) == ESUCCESS)
					{
						//
						// check is valid
						//
						if(BlIsValidGUIDPartitionTable(SectorBuffer,1,&DiskId,&ArcDiskGPTDiskReadCallback))
						{
							//
							// copy the guid,and set it as an EFI disk
							//
							Signature->IsGptDisk		= TRUE;
							RtlCopyMemory(&Signature->EfiDiskGuid,Add2Ptr(SectorBuffer,0x38,PVOID),sizeof(GUID));
						}
					}
				}
			}
		#endif
		}
	}

	ArcCacheClose(DiskId);

	return Status == ESUCCESS;
}

//
// read disk's signature
//
BOOLEAN BlReadSignature(__in PCHAR DiskName,__in BOOLEAN IsCdRom)
{
	PARC_DISK_SIGNATURE	Signature						= static_cast<PARC_DISK_SIGNATURE>(BlAllocateHeap(sizeof(ARC_DISK_SIGNATURE)));
	if(!Signature)
		return FALSE;

	//
	// additional 1 bytes is used when we need to convert 'esia' to 'multi'
	//
	Signature->ArcName									= static_cast<PCHAR>(BlAllocateHeap(strlen(DiskName) + 2));
	if(!Signature->ArcName)
		return FALSE;

	//
	// first try BlFindDiskSignature,if it failed,then try BlGetDiskSignature
	// if both are failed,we ignore this disk,but let the enumeration go on
	//
	if(!BlFindDiskSignature(DiskName,Signature) && !BlGetDiskSignature(DiskName,IsCdRom,Signature))
		return TRUE;

	InsertHeadList(&BlLoaderBlock->ArcDiskInformation->DiskSignatures,&Signature->ListEntry);

	return TRUE;
}

//
// enumerate callback
//
BOOLEAN BlpEnumerateDisks(__in PCONFIGURATION_COMPONENT_DATA ConfigData)
{
	CHAR DiskName[100];

	BlGetPathnameFromComponent(ConfigData,DiskName);

	return BlReadSignature(DiskName,FALSE);
}

//
// enumerate all disks and collect those info
//
ARC_STATUS BlGetArcDiskInformation(__in BOOLEAN SetupInt13ExtensionInfo)
{
	//
	// allocate info buffer
	//
	PARC_DISK_INFORMATION DiskInfo						= static_cast<PARC_DISK_INFORMATION>(BlAllocateHeap(sizeof(ARC_DISK_INFORMATION)));
	if(!DiskInfo)
		return ENOMEM;

	//
	// initialize header
	//
	InitializeListHead(&DiskInfo->DiskSignatures);

	//
	// save in loader block
	//
	BlLoaderBlock->ArcDiskInformation					= DiskInfo;

	//
	// traverse config tree to collect disk info
	//
	BlSearchConfigTree(BlLoaderBlock->ConfigurationRoot,PeripheralClass,DiskPeripheral,0xffffffff,&BlpEnumerateDisks);

	if(SetupInt13ExtensionInfo)
		BlpSetArcDiskInt13ExtensionInfo();

	return ESUCCESS;
}

//
// open MBR partition and update file table info
//
ARC_STATUS HardDiskPartitionOpen(__in ULONG FileId,__in ULONG DiskId,__in UCHAR PartitionNumber)
{
	//
	// change to a 1-based partition number
	//
	PartitionNumber										+= 1;
	ULONG PartitionOffset								= 0;
	ULONG PartitionCount								= 0;
	ULONG VolumeOffset									= 0;
	BOOLEAN PrimaryPartitionTable						= TRUE;
	BlFileTable[FileId].u.PartitionContext.DiskId		= static_cast<UCHAR>(DiskId);
	BlFileTable[FileId].Position.QuadPart				= 0;
	PPARTITION_CONTEXT Context							= &BlFileTable[FileId].u.PartitionContext;
	ARC_STATUS Status									= ESUCCESS;

	do
	{
		LARGE_INTEGER SeekPosition;
		SeekPosition.QuadPart							= PartitionOffset << SECTOR_SHIFT;
		Status											= BlFileTable[DiskId].DeviceEntryTable->Seek(DiskId,&SeekPosition,SeekAbsolute);
		if(Status != ESUCCESS)
			return Status;

		ULONG Count										= 0;
		USHORT DataBuffer[SECTOR_SIZE / sizeof(USHORT)];
		Status											= BlFileTable[DiskId].DeviceEntryTable->Read(DiskId,DataBuffer,SECTOR_SIZE,&Count);

		if(Status == ESUCCESS)
		{
			//
			// if sector zero is not a master boot record, then return failure status
			//
			if(DataBuffer[SECTOR_SIZE / sizeof(USHORT) - 1] != 0xaa55)
			{
				BlPrint("Boot record signature %x not found (%x found)\r\n",0xaa55,DataBuffer[SECTOR_SIZE / sizeof(USHORT) - 1]);
				Status									= EIO;
				break;
			}

			//
			// read the partition information until the four entries are checked or until we found the requested one.
			//
			for(ULONG i = 0; i < 4; i ++)
			{
				//
				// count first the partitions in the MBR.the units inside the extended partition are counted later.
				//	0xee,EFI
				//	0x05,ext
				//	0x0f,xint13
				//
				UCHAR Type								= *Add2Ptr(DataBuffer,i * 0x10 + 0x1be,PUCHAR);
				if(Type && Type != 0xee && Type != 0x05 && Type != 0x0f)
					PartitionCount						+= 1;

				//
				// check if the requested partition has already been found,set the partition info in the file table and return.
				//
				if(PartitionCount == PartitionNumber)
				{
					ULONG StartingSector				= 0;
					RtlCopyMemory(&StartingSector,Add2Ptr(DataBuffer,i * 0x10 + 0x1be + 0x08,PVOID),sizeof(ULONG));

					ULONG PartitionLength				= 0;
					RtlCopyMemory(&PartitionLength,Add2Ptr(DataBuffer,i * 0x10 + 0x1be + 0x0c,PVOID),sizeof(ULONG));

					Context->PartitionLength.QuadPart	= PartitionLength << SECTOR_SHIFT;
					Context->StartingSector				= PartitionOffset + StartingSector;

					return ESUCCESS;
				}
			}

			//
			//  if requested partition was not yet found,Look for an extended partition.
			//
			PartitionOffset								= 0;
			for(ULONG i = 0; i < 4; i ++)
			{
				UCHAR Type								= *Add2Ptr(DataBuffer,i * 0x10 + 0x1be,PUCHAR);
				if(Type == 0x05 || Type == 0x0f)
				{
					ULONG StartingSector				= 0;
					RtlCopyMemory(&StartingSector,Add2Ptr(DataBuffer,i * 0x10 + 0x1be + 0x08,PVOID),sizeof(ULONG));

					PartitionOffset						= VolumeOffset + StartingSector;
					if(PrimaryPartitionTable)
						VolumeOffset					= StartingSector;

					break;
				}
			}
		}

		PrimaryPartitionTable							= FALSE;

	}while(PartitionOffset);

	return EBADF;
}

#if _EFI_SUPPORT_

//
// read gpt disk callback
//
BOOLEAN BlDiskOpenGPTDiskReadCallback(__in ULONGLONG LBA,__in ULONG Count,__in PULONG DiskId,__in PVOID Buffer)
{
	LARGE_INTEGER SeekPosition;
	SeekPosition.QuadPart								= static_cast<LONGLONG>(LBA << SECTOR_SHIFT);
	if(BlFileTable[*DiskId].DeviceEntryTable->Seek(*DiskId,&SeekPosition,SeekAbsolute) != ESUCCESS)
		return FALSE;

	ULONG ReadCount										= 0;
	if(BlFileTable[*DiskId].DeviceEntryTable->Read(*DiskId,Buffer,Count,&ReadCount) != ESUCCESS || Count != ReadCount)
		return FALSE;

	return TRUE;
}

//
// locate partition entry
//
PVOID BlLocateGPTPartition(__in ULONG PartitionNumber,__in ULONG SearchCount,__inout_opt PULONG StartPosition)
{
	//
	// change to 1 based
	//
	PartitionNumber										+= 1;
	GUID ZeroGuid;
	RtlZeroMemory(&ZeroGuid,sizeof(GUID));

	ULONG ValidPartitionsCount							= StartPosition ? *StartPosition : 0;
	PVOID CurrentEntry									= 0;

	for(ULONG CurrentIndex = 0; CurrentIndex < SearchCount; CurrentIndex ++)
	{
		if(ValidPartitionsCount >= PartitionNumber)
			return CurrentEntry;

		//
		// partition entry is always 128 bytes length
		//
		CurrentEntry									= Add2Ptr(EfiPartitionBuffer,CurrentIndex << 7,PVOID);

		//
		// PartitionTypeGUID and UniquePartitionGUID should not be zero
		//
		GUID ZeroGuid;
		RtlZeroMemory(&ZeroGuid,sizeof(GUID));

		if(RtlCompareMemory(&ZeroGuid,CurrentEntry,sizeof(GUID)) == sizeof(GUID))
			continue;

		if(RtlCompareMemory(&ZeroGuid,Add2Ptr(CurrentEntry,sizeof(GUID),PVOID),sizeof(GUID)) == sizeof(GUID))
			continue;

		//
		// start LBA and end LBA should not be zero
		//
		if(*Add2Ptr(CurrentEntry,0x20,PULONGLONG) == 0 || *Add2Ptr(CurrentEntry,0x28,PULONGLONG) == 0)
			continue;

		ValidPartitionsCount							+= 1;
		if(StartPosition)
			*StartPosition								+= 1;

		if(ValidPartitionsCount == PartitionNumber)
			return CurrentEntry;
	}

	return 0;
}

//
// open EFI partition and update file table info
//
ARC_STATUS BlOpenGPTDiskPartition(__in ULONG FileId,__in ULONG DiskId,__in UCHAR PartitionNumber)
{
	if(PartitionNumber >= 0x80)
		return EINVAL;

	//
	// seek to the second sector
	//
	LARGE_INTEGER SeekPosition;
	SeekPosition.QuadPart								= SECTOR_SIZE;
	ARC_STATUS Status									= BlFileTable[DiskId].DeviceEntryTable->Seek(DiskId,&SeekPosition,SeekAbsolute);
	if(Status != ESUCCESS)
		return Status;

	//
	// read it
	//
	ULONG Count											= 0;
	CHAR LocalBuffer[SECTOR_SIZE];
	Status												= BlFileTable[DiskId].DeviceEntryTable->Read(DiskId,&LocalBuffer,SECTOR_SIZE,&Count);
	if(Status != ESUCCESS)
		return Status;

	if(Count != SECTOR_SIZE)
		return EIO;

	//
	// check this partition table
	//
	if(!BlIsValidGUIDPartitionTable(LocalBuffer,1,&DiskId,&BlDiskOpenGPTDiskReadCallback))
		return EBADF;

	//
	// seek to partition entry
	//
	ULONGLONG PartitionEntryLBA							= *Add2Ptr(LocalBuffer,0x48,PULONGLONG);
	SeekPosition.QuadPart								= static_cast<LONGLONG>(PartitionEntryLBA << SECTOR_SHIFT);
	Status												= BlFileTable[DiskId].DeviceEntryTable->Seek(DiskId,&SeekPosition,SeekAbsolute);
	if(Status != ESUCCESS)
		return Status;

	//
	// read partition table
	//
	RtlZeroMemory(EfiPartitionBuffer,sizeof(EfiPartitionBuffer));
	Status												= BlFileTable[DiskId].DeviceEntryTable->Read(DiskId,EfiPartitionBuffer,sizeof(EfiPartitionBuffer),&Count);
	if(Status != ESUCCESS)
		return Status;

	if(Count != sizeof(EfiPartitionBuffer))
		return EIO;

	//
	// get partition table by partition number
	//
	PVOID PartitionTable								= BlLocateGPTPartition(PartitionNumber,0x80,0);
	if(!PartitionTable)
		return EBADF;

	//
	// PartitionTypeGUID and UniquePartitionGUID should not be zero
	//
	GUID ZeroGuid;
	RtlZeroMemory(&ZeroGuid,sizeof(GUID));

	if(RtlCompareMemory(&ZeroGuid,PartitionTable,sizeof(GUID)) == sizeof(GUID))
		return EBADF;

	if(RtlCompareMemory(&ZeroGuid,Add2Ptr(PartitionTable,sizeof(GUID),PVOID),sizeof(GUID)) == sizeof(GUID))
		return EBADF;

	//
	// start LBA and end LBA should not be zero
	//
	if(*Add2Ptr(PartitionTable,0x20,PULONGLONG) == 0 || *Add2Ptr(PartitionTable,0x28,PULONGLONG) == 0)
		return EBADF;

	//
	// this is a valid partition,set file table
	//
	PPARTITION_CONTEXT Context							= &BlFileTable[FileId].u.PartitionContext;
	Context->DiskId										= static_cast<UCHAR>(DiskId);
	Context->EndingSector								= *Add2Ptr(PartitionTable,0x28,PULONG);
	Context->StartingSector								= *Add2Ptr(PartitionTable,0x28,PULONG);
	Context->PartitionLength.QuadPart					= static_cast<LONGLONG>(Context->EndingSector - Context->StartingSector) << SECTOR_SHIFT;
	BlFileTable[FileId].Position.QuadPart				= 0;

	return ESUCCESS;
}
#endif