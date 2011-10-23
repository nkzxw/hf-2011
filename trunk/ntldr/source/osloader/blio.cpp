//********************************************************************
//	created:	10:8:2008   23:17
//	file:		blio.cpp
//	author:		tiamo
//	purpose:	io support
//********************************************************************

#include "stdafx.h"

//
// close notify
//
PARC_DEVICE_CLOSE_NOTIFICATION_ROUTINE					DeviceCloseNotify[20];

//
// fs cache
//
BL_FILE_SYSTEM_CACHE									DeviceFSCache[48];

//
// opend file count
//
ULONG													BlFilesOpened;

//
// file table
//
BL_FILE_TABLE											BlFileTable[48];

//
// initializes the file table used by the OS loader and initializes the boot loader filesystems.
//
ARC_STATUS BlIoInitialize()
{
	//
	// initialize close notification table
	//
	RtlZeroMemory(DeviceCloseNotify,sizeof(DeviceCloseNotify));

	//
	// initialize the file table and fs cache table
	//
	for(ULONG i = 0; i < ARRAYSIZE(BlFileTable); i ++)
	{
		BlFileTable[i].Flags.Open						= FALSE;
		BlFileTable[i].StructureContext					= 0;
		DeviceFSCache[i].DeviceId						= 0xffffffff;
		DeviceFSCache[i].StructureContext				= 0;
		DeviceFSCache[i].DeviceEntryTable				= 0;
	}

	ARC_STATUS Status									= ESUCCESS;

#if _NET_SUPPORT_
	if((Status = NetInitialize()) != ESUCCESS)
		return Status;
#endif

#if _FAT_SUPPORT_
	if((Status = FatInitialize()) != ESUCCESS)
		return Status;
#endif

#if _UDFS_SUPPORT_
	if((Status = UdfsInitialize()) != ESUCCESS)
		return Status;
#endif

#if _CDFS_SUPPORT_
	if((Status = CdfsInitialize()) != ESUCCESS)
		return Status;
#endif

#if _NTFS_SUPPORT_
	if((Status = NtfsInitialize()) != ESUCCESS)
		return Status;
#endif

	return Status;
}

//
// cache close
//
VOID ArcCacheClose(__in ULONG DeviceId)
{
	//
	// send notification
	//
	for(ULONG i = 0; i < ARRAYSIZE(DeviceCloseNotify); i ++)
	{
		if(DeviceCloseNotify[i])
			DeviceCloseNotify[i](DeviceId);
	}

	//
	// close fs cache
	//
	for(ULONG i = 0; i < ARRAYSIZE(DeviceFSCache); i ++)
	{
		if(DeviceFSCache[i].DeviceId == DeviceId)
			DeviceFSCache[i].DeviceId					= 0xffffffff;
	}

	//
	// close it
	//
	ArcClose(DeviceId);
}

//
// register for device close
//
ARC_STATUS ArcRegisterForDeviceClose(__in PARC_DEVICE_CLOSE_NOTIFICATION_ROUTINE Routine)
{
	if(!Routine)
		return EINVAL;

	for(ULONG i = 0; i < ARRAYSIZE(DeviceCloseNotify); i ++)
	{
		if(!DeviceCloseNotify[i])
		{
			DeviceCloseNotify[i]						= Routine;
			return ESUCCESS;
		}
	}

	return ENOENT;
}

//
// get fs info
//
PBOOTFS_INFO BlGetFsInfo(IN ULONG DeviceId)
{
	PBL_DEVICE_ENTRY_TABLE Table						= 0;
	FS_STRUCTURE_CONTEXT FsStructure;

#if _NTFS_SUPPORT_
	if(Table = IsNtfsFileStructure(DeviceId,&FsStructure))
		return Table->BootFsInfo;
#endif

#if _NET_SUPPORT_
	if(Table = IsNetFileStructure(DeviceId, &FsStructure))
		return Table->BootFsInfo;
#endif

#if _FAT_SUPPORT_
	if(Table = IsFatFileStructure(DeviceId, &FsStructure))
		return Table->BootFsInfo;
#endif

#if _CDFS_SUPPORT_
	if(Table = IsCdfsFileStructure(DeviceId, &FsStructure))
		return Table->BootFsInfo;
#endif

	return 0;
}

//
// close
//
ARC_STATUS BlClose(__in ULONG FileId)
{
	if(BlFileTable[FileId].Flags.Open)
		return BlFileTable[FileId].DeviceEntryTable->Close(FileId);

	return EACCES;
}

//
// open helper
//
ARC_STATUS _BlOpen(__in ULONG DeviceId,__in PCHAR OpenPath,__in OPEN_MODE OpenMode,__out PULONG FileId)
{
	//
	// check we have already cached this device
	//
	ULONG DeviceFSCacheIndex							= 0xffffffff;
	for(ULONG i = 0; i < ARRAYSIZE(DeviceFSCache); i ++)
	{
		if(DeviceFSCache[i].DeviceId == DeviceId)
		{
			DeviceFSCacheIndex							= i;
			break;
		}
	}

	//
	// find an empty slot
	//
	ULONG FileTableSlot									= 0xffffffff;
	for(ULONG i = 2; i < ARRAYSIZE(BlFileTable); i ++)
	{
		if(!BlFileTable[i].Flags.Open)
		{
			FileTableSlot								= i;
			break;
		}
	}

	//
	// unable to find an unused slot
	//
	if(FileTableSlot >= ARRAYSIZE(BlFileTable))
		return EACCES;

	//
	// if we have cached this device,copy context and device entry from cache,otherwise query each file system to retrieve those info
	//
	if(DeviceFSCacheIndex >= ARRAYSIZE(DeviceFSCache))
	{
		ULONG ContextSize								= 0;
		FS_STRUCTURE_CONTEXT LocalStructure;

		do
		{
		#if _NET_SUPPORT_
			ContextSize									= sizeof(NET_STRUCTURE_CONTEXT);	/* 0x04 */
			BlFileTable[FileTableSlot].DeviceEntryTable	= IsNetFileStructure(DeviceId,&LocalStructure);
			if(BlFileTable[FileTableSlot].DeviceEntryTable)
				break;
		#endif

		#if _NTFS_SUPPORT_
			ContextSize									= sizeof(NTFS_STRUCTURE_CONTEXT);	/* 0x1260 */
			BlFileTable[FileTableSlot].DeviceEntryTable	= IsNtfsFileStructure(DeviceId,&LocalStructure);
			if(BlFileTable[FileTableSlot].DeviceEntryTable)
				break;
		#endif

		#if _FAT_SUPPORT_
			ContextSize									= sizeof(FAT_STRUCTURE_CONTEXT);	/* 0x938 */
			BlFileTable[FileTableSlot].DeviceEntryTable	= IsNetFileStructure(DeviceId,&LocalStructure);
			if(BlFileTable[FileTableSlot].DeviceEntryTable)
				break;
		#endif

		#if _UDFS_SUPPORT_
			ContextSize									= sizeof(UDFS_STRUCTURE_CONTEXT);	/* 0x04 */
			BlFileTable[FileTableSlot].DeviceEntryTable	= IsUDFSFileStructure(DeviceId,&LocalStructure);
			if(BlFileTable[FileTableSlot].DeviceEntryTable)
				break;
		#endif

		#if _ETFS_SUPPORT_
			ContextSize									= sizeof(ETFS_STRUCTURE_CONTEXT);	/* 0x24 */
			BlFileTable[FileTableSlot].DeviceEntryTable	= IsEtfsFileStructure(DeviceId,&LocalStructure);
			if(BlFileTable[FileTableSlot].DeviceEntryTable)
				break;
		#endif

		#if _CDFS_SUPPORT_
			ContextSize									= sizeof(CDFS_STRUCTURE_CONTEXT);	/* 0x24 */
			BlFileTable[FileTableSlot].DeviceEntryTable	= IsCdfsFileStructure(DeviceId,&LocalStructure);
			if(BlFileTable[FileTableSlot].DeviceEntryTable)
				break;
		#endif

			//
			// not supported file system
			//
			return EACCES;
		}while(0);

		//
		// try to find an empty slot in fs cache table
		//
		for(ULONG i = 0; i < ARRAYSIZE(DeviceFSCache); i ++)
		{
			if(DeviceFSCache[i].DeviceId == 0xffffffff)
			{
				DeviceFSCacheIndex						= i;
				break;
			}
		}

		//
		// unable to find
		//
		if(DeviceFSCacheIndex >= ARRAYSIZE(DeviceFSCache))
			return ENOSPC;

		//
		// save device id
		//
		DeviceFSCache[DeviceFSCacheIndex].DeviceId		= DeviceId;

		//
		// setup context and device entry in cache
		//
		PBL_FILE_SYSTEM_CACHE Cache						= DeviceFSCache + DeviceFSCacheIndex;
		if(!Cache->StructureContext)
		{
			Cache->StructureContext						= BlAllocateHeap(sizeof(FS_STRUCTURE_CONTEXT));
			if(!Cache->StructureContext)
				return ENOMEM;

			RtlZeroMemory(Cache->StructureContext,sizeof(FS_STRUCTURE_CONTEXT));
		}

		RtlCopyMemory(Cache->StructureContext,&LocalStructure,ContextSize);

		//
		// setup context file table
		//
		BlFileTable[FileTableSlot].StructureContext		= Cache->StructureContext;

		//
		// setup device entry in cache
		//
		Cache->DeviceEntryTable							= BlFileTable[FileTableSlot].DeviceEntryTable;
	}
	else
	{
		//
		// copy from context and device entry
		//
		BlFileTable[FileTableSlot].StructureContext		= DeviceFSCache[DeviceFSCacheIndex].StructureContext;
		BlFileTable[FileTableSlot].DeviceEntryTable		= DeviceFSCache[DeviceFSCacheIndex].DeviceEntryTable;
	}

	//
	// save device id in file table,and set output file id
	//
	BlFileTable[FileTableSlot].DeviceId					= DeviceId;
	*FileId												= FileTableSlot;

	//
	// give debugger a chance to map this file
	//
	ARC_STATUS Status									= EINVAL;
	if(BdDebuggerEnabled && BdPullRemoteFile(OpenPath,0x80,5,0x20,FileTableSlot) == STATUS_SUCCESS)
	{
		DbgPrint("BlLoadImageEx: Pulled %s from Kernel Debugger\n",OpenPath);
		return ESUCCESS;
	}

	//
	// otherwise try the normal open
	//
	return BlFileTable[FileTableSlot].DeviceEntryTable->Open(OpenPath,OpenMode,FileId);
}

//
// check compressed file
//
BOOLEAN BlCheckCompressedFile(__in PCHAR OpenPath,__out PCHAR ChangedOpenPath)
{
	return FALSE;
}

//
// prepare to read compressed file
//
ARC_STATUS DecompPrepareToReadCompressedFile(__in PCHAR FilePath,__in ULONG FileId)
{
	return 0xffffffff;
}

//
// open
//
ARC_STATUS BlOpen(__in ULONG DeviceId,__in PCHAR OpenPath,__in OPEN_MODE OpenMode,__out PULONG FileId)
{
	CHAR TempPath[0x100];
	if(OpenMode == ArcOpenReadOnly && BlCheckCompressedFile(OpenPath,TempPath) && _BlOpen(DeviceId,TempPath,ArcOpenReadOnly,FileId) == ESUCCESS)
	{
		ARC_STATUS Status								= DecompPrepareToReadCompressedFile(TempPath,*FileId);
		if(Status != 0xffffffff)
			return Status;

		BlFilesOpened									+= 1;
		return ESUCCESS;
	}

	ARC_STATUS Status									= _BlOpen(DeviceId,OpenPath,OpenMode,FileId);
	if(Status == ESUCCESS)
		BlFilesOpened									+= 1;

	return Status;
}

//
// read
//
ARC_STATUS BlRead(__in ULONG FileId,__out PVOID Buffer,__in ULONG Length,__out PULONG Count)
{
	//
	// if the file is open for read, then attempt to read from it.
	// otherwise return an access error.
	//
	if(BlFileTable[FileId].Flags.Open && BlFileTable[FileId].Flags.Read)
		return BlFileTable[FileId].DeviceEntryTable->Read(FileId,Buffer,Length,Count);

	return EACCES;
}

//
// read status
//
ARC_STATUS BlGetReadStatus(__in ULONG FileId)
{
	return ESUCCESS;
}

//
// seek
//
ARC_STATUS BlSeek(__in ULONG FileId,__in PLARGE_INTEGER Offset,__in SEEK_MODE SeekMode)
{
	//
	// if the file is open, then attempt to seek on it.otherwise return an access error.
	//
	if(BlFileTable[FileId].Flags.Open)
		return BlFileTable[FileId].DeviceEntryTable->Seek(FileId,Offset,SeekMode);

	return EACCES;
}

//
// write
//
ARC_STATUS BlWrite(__in ULONG FileId,__in PVOID Buffer,__in ULONG Length,__out PULONG Count)
{
	//
	// if the file is open for write, then attempt to write to it.otherwise return an access error.
	//
	if(BlFileTable[FileId].Flags.Open && BlFileTable[FileId].Flags.Write)
		return BlFileTable[FileId].DeviceEntryTable->Write(FileId,Buffer,Length,Count);

	return EACCES;
}

//
// get file info
//
ARC_STATUS BlGetFileInformation(__in ULONG FileId,__in PFILE_INFORMATION FileInformation)
{
	//
	// if the file is open,then attempt to get file information. Otherwise return an access error.
	//
	if(BlFileTable[FileId].Flags.Open)
		return BlFileTable[FileId].DeviceEntryTable->GetFileInformation(FileId,FileInformation);

	return EACCES;
}

//
// set file info
//
ARC_STATUS BlSetFileInformation(__in ULONG FileId,__in ULONG AttributeFlags,__in ULONG AttributeMask)
{
	//
	// if the file is open, then attempt to Set file information.otherwise return an access error.
	//
	if(BlFileTable[FileId].Flags.Open)
		return BlFileTable[FileId].DeviceEntryTable->SetFileInformation(FileId,AttributeFlags,AttributeMask);

	return EACCES;
}

//
// rename
//
ARC_STATUS BlRename(__in ULONG FileId,__in PCHAR NewName)
{
	if(BlFileTable[FileId].Flags.Open)
		return BlFileTable[FileId].DeviceEntryTable->Rename(FileId,NewName);

	return EACCES;
}

//
// read at offset
//
ARC_STATUS BlReadAtOffset(__in ULONG FileId,__in ULONG Offset,__in ULONG Length,__in PVOID Buffer)
{
	LARGE_INTEGER LargeOffset;
	LargeOffset.QuadPart								= Offset;
	ARC_STATUS Status									= BlSeek(FileId,&LargeOffset,SeekAbsolute);
	if(Status != ESUCCESS)
		return Status;

	Status												= BlRead(FileId,Buffer,Length,&Offset);
	if(Status != ESUCCESS)
		return Status;

	return Offset == Length ? ESUCCESS : EINVAL;
}