//********************************************************************
//	created:	19:8:2008   20:57
//	file:		netboot.cpp
//	author:		tiamo
//	purpose:	net boot
//********************************************************************

#include "stdafx.h"

//
// net debug
//
ULONG													NetDebugFlag = 3;

//
// already initialized
//
BOOLEAN													NetBootInitialized;

//
// cached file data
//
PVOID													CachedFileData;

//
// cached file path
//
PCHAR													CachedFilePath;

//
// device table
//
BL_DEVICE_ENTRY_TABLE									NetDeviceEntryTable;

//
// boot fs info
//
BOOTFS_INFO												NetBootFsInfo = {L"net"};

//
// old open routine
//
PARC_OPEN_ROUTINE										NetRealArcOpenRoutine;

//
// old close routine
//
PARC_CLOSE_ROUTINE										NetRealArcCloseRoutine;

//
// close
//
ARC_STATUS NetClose(__in ULONG FileId)
{
	if(NetDebugFlag & 0x40)
		DbgPrint("NetClose FileId = %d\r\n",FileId);

	PBL_FILE_TABLE FileEntry							= BlFileTable + FileId;
	if(NetDebugFlag & 8)
		DbgPrint("NetClose: id %d, freeing memory at 0x%08x, %d bytes\n",FileId,FileEntry->u.NetFileContext.FileBuffer,FileEntry->u.NetFileContext.FileSize);

	//
	// free memory allocation descriptor
	//
	BlFreeDescriptor(reinterpret_cast<ULONG>(FileEntry->u.NetFileContext.FileBuffer) >> PAGE_SHIFT);

	//
	// invalid cache
	//
	if(FileEntry->u.NetFileContext.FileBuffer == CachedFileData)
	{
		CachedFileData									= 0;
		CachedFilePath									= 0;
	}

	//
	// free file table entry
	//
	FileEntry->Flags.Open								= FALSE;

	return EROFS;
}

//
// mount
//
ARC_STATUS NetMount(__in PCHAR MountPath,__in MOUNT_OPERATION Operation)
{
	if(NetDebugFlag & 0x40)
		DbgPrint("NetMount");

	return EROFS;
}

//
// open
//
ARC_STATUS NetOpen(__in PCHAR OpenPath,__in OPEN_MODE OpenMode,__out PULONG FileId)
{
#if _NET_SUPPORT_
	#error unimplemented
#endif

	return EINVAL;
}

//
// read
//
ARC_STATUS NetRead(__in ULONG FileId,__out PVOID Buffer,__in ULONG Length,__out PULONG Count)
{
	PBL_FILE_TABLE FileEntry							= BlFileTable + FileId;
	ULONG CopyLength									= Length;
	if(CopyLength + FileEntry->Position.LowPart > FileEntry->u.NetFileContext.FileSize)
		CopyLength										= FileEntry->u.NetFileContext.FileSize - FileEntry->Position.LowPart;

	PVOID SrcBuffer										= Add2Ptr(FileEntry->u.NetFileContext.FileBuffer,FileEntry->Position.LowPart,PVOID);
	RtlCopyMemory(Buffer,SrcBuffer,CopyLength);
	*Count												= CopyLength;
	FileEntry->Position.QuadPart						+= CopyLength;

	return ESUCCESS;
}

//
// read status
//
ARC_STATUS NetGetReadStatus(__in ULONG FileId)
{
	if(NetDebugFlag & 0x40)
		DbgPrint("NetGetReadStatus");

	return ESUCCESS;
}

//
// seek
//
ARC_STATUS NetSeek(__in ULONG FileId,__in PLARGE_INTEGER Offset,__in SEEK_MODE SeekMode)
{
	PBL_FILE_TABLE FileEntry							= BlFileTable + FileId;
	PNET_FILE_CONTEXT Context							= &FileEntry->u.NetFileContext;
	LONGLONG NewPos										= 0;

	if(SeekMode == SeekAbsolute)
		NewPos											= Offset->QuadPart;
	else if(SeekMode == SeekRelative)
		NewPos											= Offset->QuadPart + FileEntry->Position.QuadPart;
	else
		return EROFS;

	if(NetDebugFlag & 8)
		DbgPrint("NetSeek: id %d, mode %d, offset %x, new pos %x\n",FileId,SeekMode,Offset->LowPart,static_cast<ULONG>(NewPos));

	if(NewPos > Context->FileSize)
		return EROFS;

	FileEntry->Position.QuadPart						= NewPos;
	return ESUCCESS;
}

//
// write
//
ARC_STATUS NetWrite(__in ULONG FileId,__in PVOID Buffer,__in ULONG Length,__out PULONG Count)
{
	if(NetDebugFlag & 0x40)
		DbgPrint("NetWrite");

	return EROFS;
}

//
// get file info
//
ARC_STATUS NetGetFileInformation(__in ULONG FileId,__out PFILE_INFORMATION FileInformation)
{
	PBL_FILE_TABLE FileEntry							= BlFileTable + FileId;
	PNET_FILE_CONTEXT Context							= &FileEntry->u.NetFileContext;

	FileInformation->CurrentPosition.QuadPart			= FileEntry->Position.QuadPart;
	FileInformation->EndingAddress.QuadPart				= Context->FileSize;

	if(NetDebugFlag & 8)
		DbgPrint("NetGetFileInformation returning size %x, position %x\n",Context->FileSize,FileEntry->Position.LowPart);

	return ESUCCESS;
}

//
// set file info
//
ARC_STATUS NetSetFileInformation(__in ULONG FileId,__in ULONG AttributeFlags,__in ULONG AttributeMask)
{
	if(NetDebugFlag & 0x40)
		DbgPrint("NetSetFileInformation");

	return EROFS;
}

//
// rename
//
ARC_STATUS NetRename(__in ULONG FileId,__in PCHAR NewName)
{
	if(NetDebugFlag & 0x40)
		DbgPrint("NetRename");

	return EROFS;
}

//
// get dir entry
//
ARC_STATUS NetGetDirectoryEntry(__in ULONG FileId,__out PDIRECTORY_ENTRY Buffer,__in ULONG Length,__out PULONG Count)
{
	if(NetDebugFlag & 0x40)
		DbgPrint("NetGetDirectoryEntry");

	return EROFS;
}

#if _NET_SUPPORT_
//
// hooked close
//
ARC_STATUS NetArcClose(__in ULONG FileId)
{
	if(NetDebugFlag & 0x40)
		DbgPrint("NetArcClose");

	if(FileId != 'dten')
		return NetRealArcCloseRoutine(FileId);

	return ESUCCESS;
}

//
// hooked open
//
ARC_STATUS NetArcOpen(__in PCHAR OpenPath,__in OPEN_MODE OpenMode,__out PULONG FileId)
{
	if(NetDebugFlag & 0x40)
		DbgPrint("NetArcOpen");

	if(_strnicmp(OpenPath,"net(",4))
		return NetRealArcOpenRoutine(OpenPath,OpenMode,FileId);

	*FileId												= 'dten';
	return ESUCCESS;
}
#endif

//
// initialize net boot
//
ARC_STATUS NetInitialize()
{
	if(NetDebugFlag & 0x40)
		DbgPrint("NetInitialize\n");

	//
	// already initialized
	//
	if(NetBootInitialized)
		return ESUCCESS;

	//
	// setup device table
	//
	NetDeviceEntryTable.BootFsInfo						= &NetBootFsInfo;
	NetDeviceEntryTable.Close							= &NetClose;
	NetDeviceEntryTable.GetDirectoryEntry				= &NetGetDirectoryEntry;
	NetDeviceEntryTable.GetFileInformation				= &NetGetFileInformation;
	NetDeviceEntryTable.GetReadStatus					= &NetGetReadStatus;
	NetDeviceEntryTable.Mount							= &NetMount;
	NetDeviceEntryTable.Open							= &NetOpen;
	NetDeviceEntryTable.Read							= &NetRead;
	NetDeviceEntryTable.Rename							= &NetRename;
	NetDeviceEntryTable.Seek							= &NetSeek;
	NetDeviceEntryTable.SetFileInformation				= &NetSetFileInformation;
	NetDeviceEntryTable.Write							= &NetWrite;

	//
	// not booting from net
	//
	if(!BlBootingFromNet)
		return ESUCCESS;

#if _NET_SUPPORT_
	//
	// initialized
	//
	NetBootInitialized									= TRUE;

	if(NetDebugFlag & 4)
		DbgPrint("NetInitialize: booting from net\n");

	//
	// hook open and close routine
	//
	NetRealArcOpenRoutine								= reinterpret_cast<PARC_OPEN_ROUTINE>(SYSTEM_BLOCK->FirmwareVector[OpenRoutine]);
	SYSTEM_BLOCK->FirmwareVector[OpenRoutine]			= reinterpret_cast<PVOID>(&NetArcOpen);

	NetRealArcCloseRoutine								= reinterpret_cast<PARC_CLOSE_ROUTINE>(SYSTEM_BLOCK->FirmwareVector[CloseRoutine]);
	SYSTEM_BLOCK->FirmwareVector[CloseRoutine]			= reinterpret_cast<PVOID>(&NetArcClose);

	return GetParametersFromRom();
#else
	return ESUCCESS;
#endif
}