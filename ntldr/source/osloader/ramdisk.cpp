//********************************************************************
//	created:	26:9:2008   12:17
//	file:		ramdisk.cpp
//	author:		tiamo
//	purpose:	ramdisk
//********************************************************************

#include "stdafx.h"

#if _RAMDISK_SUPPORT_

//
// trace info
//
#define RAMDISK_TRACE_INFO(x)							{if(RamdiskDebug && RamdiskDebugLevel >= 2) DbgPrint x;}

//
// debug print
//
#define RAMDISK_DEBUG_PRINT(x)							{if(RamdiskDebug) DbgPrint x;}

//
// TOC record
//
typedef struct _SDI_TOC_RECORD
{
	//
	// blob type
	//	BOOT (startrom.*, you could use server' hdlscom?.* for debug)
	//	LOAD (osloader.exe, bootmgr.exe for PE 2.0)
	//	PART (Winimage .ima - just a filesystem, as old Linux initrd)
	//	DISK (image with a MBR, you could import flat-VHD or *-flat.vmdk)
	//	WIM (a new squashfs-style Windows Vista filesystem)
	//
	ULONG												BLOBtype[2];

	//
	// attribute
	//
	LARGE_INTEGER										Attribute;

	//
	// offset
	//
	LARGE_INTEGER										Offset;

	//
	// size
	//
	LARGE_INTEGER										Size;

	//
	// base address
	//
	LARGE_INTEGER										BaseAddress;
	//
	// 0 for non-filesystem BLOBs
	// filesystem code for PART (as in MBR) 7 for NTFS, 6 for BIGFAT, etc.
	//
	LARGE_INTEGER										Reserved[3];
}SDI_TOC_RECORD,*PSDI_TOC_RECORD;

//
// sdi file header
//
typedef struct _SDI_HEADER
{
	//
	// signature,"$SDI0001"
	//
	CHAR												Signature[8];

	//
	// ram or rom
	//
	LARGE_INTEGER										MDBtype;

	//
	// boot code offset
	//
	LARGE_INTEGER										BootCodeOffset;

	//
	// boot code size
	//
	LARGE_INTEGER										BootCodeSize;

	//
	// vendor id
	//
	LARGE_INTEGER										VendorID;

	//
	// device id
	//
	LARGE_INTEGER										DeviceID;

	//
	// device model
	//
	GUID												DeviceModel;

	//
	// device role
	//
	LARGE_INTEGER										DeviceRole;

	//
	// reserved
	//
	LARGE_INTEGER										Reserved1;

	//
	// runtime guid
	//
	GUID												RuntimeGUID;

	//
	// runtime oem revision
	//
	LARGE_INTEGER										RuntimeOEMrev;

	//
	// reserved
	//
	LARGE_INTEGER										Reserved2;

	//
	// page alignment
	//
	LARGE_INTEGER										PageAlignment;

	//
	// reserved
	//
	LARGE_INTEGER										Reserved3[48];

	//
	// checkusm
	//
	LARGE_INTEGER										Checksum;

	//
	// reserved
	//
	UCHAR												Reserved[512];

	//
	// toc
	//
	SDI_TOC_RECORD										TOCRecord[48];
}SDI_HEADER,*PSDI_HEADER;

//
// debug flags
//
BOOLEAN													RamdiskDebug = TRUE;

//
// debug level
//
UCHAR													RamdiskDebugLevel = 1;

//
// break flags
//
BOOLEAN													RamdiskBreak;

//
// build
//
BOOLEAN													RamdiskBuild;

//
// active
//
BOOLEAN													RamdiskActive;

//
// ramdisk path
//
PCHAR													RamdiskPath;

//
// disk image length
//
LONGLONG												RamdiskImageLength;

//
// disk image offset
//
ULONG													RamdiskImageOffset;

//
// mapped base page
//
ULONG													RamdiskBasePage;

//
// file size
//
LONGLONG												RamdiskFileSize;

//
// file size in page
//
ULONG													RamdiskFileSizeInPages;

#if _NET_SUPPORT_
//
// TFTP server address
//
ULONG													RamdiskTFTPAddr;

//
// MTFTP server address
//
ULONG													RamdiskMTFTPAddr;

//
// MTFTP C port
//
USHORT													RamdiskMTFTPCPort;

//
// MTFTP S port
//
USHORT													RamdiskMTFTPSPort;

//
// MTFTP file size
//
LONGLONG												RamdiskMTFTPFileSize;

//
// MTFTP chunk size
//
LONGLONG												RamdiskMTFTPChunkSize;

//
// delay
//
USHORT													RamdiskMTFTPDelay = 5;

//
// timeout
//
USHORT													RamdiskMTFTPTimeout = 5;
#endif

//
// device entry
//
extern BL_DEVICE_ENTRY_TABLE							RamdiskEntryTable;

//
// map disk
//
PVOID MapRamdisk(__in LONGLONG Offset,__out PLONGLONG AvailableLength)
{
	*AvailableLength									= RamdiskFileSize - Offset;
	return reinterpret_cast<PVOID>((RamdiskBasePage << PAGE_SHIFT) + Offset);
}

//
// fatal error
//
VOID RamdiskFatalError(__in ULONG MessageId1,__in ULONG MessageId2)
{
	BlClearScreen();

	CHAR Buffer[40]										= {0};
	ULONG Count											= 0;
	PCHAR Message										= BlFindMessage(MessageId1);
	if(!Message)
	{
		sprintf(Buffer,"%08lx\r\n",MessageId1);
		Message											= Buffer;
	}

	ArcWrite(BlConsoleOutDeviceId,Message,strlen(Message),&Count);

	Message										= BlFindMessage(MessageId2);
	if(!Message)
	{
		sprintf(Buffer,"%08lx\r\n",MessageId1);
		Message											= Buffer;
	}

	ArcWrite(BlConsoleOutDeviceId,Message,strlen(Message),&Count);

	if(BdDebuggerEnabled)
		DbgBreakPoint();
}

//
// close
//
ARC_STATUS RamdiskClose(__in ULONG FileId)
{
	PBL_FILE_TABLE FileEntry							= BlFileTable + FileId;
	if(!FileEntry->Flags.Open)
		DbgPrint("ERROR - Unopened fileid %lx closed.\n",FileId);

	FileEntry->Flags.Open								= FALSE;
	return ESUCCESS;
}

//
// mount
//
ARC_STATUS RamdiskMount(__in PCHAR MountPath,__in MOUNT_OPERATION Operation)
{
	if(RamdiskDebug && RamdiskDebugLevel >= 2)
		DbgPrint("RamdiskMount called.\n");

	return EINVAL;
}

//
// ramdisk open
//
ARC_STATUS RamdiskOpen(__in PCHAR OpenPath,__in OPEN_MODE OpenMode,__out PULONG FileId)
{
	//
	// there is no ramdisk mapped
	//
	if(!RamdiskActive)
		return EBADF;

	//
	// check ramdisk path
	//
	ULONG Key											= 0;
	if(FwGetPathMnemonicKey(OpenPath,"ramdisk",&Key))
	{
		RAMDISK_TRACE_INFO(("RamdiskOpen: not a ramdisk path.\n"));
		return EBADF;
	}

	if(Key)
	{
		RAMDISK_DEBUG_PRINT(("RamdiskOpen: not ramdisk 0.\n"));
		return EBADF;
	}

	//
	// search an unused slot
	//
	for(*FileId = 2; *FileId < ARRAYSIZE(BlFileTable); *FileId += 1)
	{
		if(!BlFileTable[*FileId].Flags.Open)
			break;
	}

	//
	// no free slot
	//
	if(*FileId == ARRAYSIZE(BlFileTable))
	{
		RAMDISK_DEBUG_PRINT(("RamdiskOpen: no file table entry available.\n"));

		return ENOENT;
	}

	//
	// setup file entry
	//
	BlFileTable[*FileId].Flags.Open						= TRUE;
	BlFileTable[*FileId].DeviceEntryTable				= &RamdiskEntryTable;

	//
	// setup file context
	//

	RAMDISK_TRACE_INFO(("RamdiskOpen: exit success.\n"));

	return ESUCCESS;
}

//
// read
//
ARC_STATUS RamdiskRead(__in ULONG FileId,__out PVOID Buffer,__in ULONG Length,__out PULONG Count)
{
	PBL_FILE_TABLE FileEntry							= BlFileTable + FileId;
	LONGLONG Offset										= FileEntry->Position.QuadPart;
	RAMDISK_TRACE_INFO(("RamdiskRead: offset %x, length %x, buffer %p.\n",static_cast<ULONG>(Offset),Length,Buffer));

	//
	// check EOF
	//
	if(Offset >= RamdiskImageLength)
	{
		RAMDISK_DEBUG_PRINT(("RamdiskRead: read beyond EOF.\n"));
		return EINVAL;
	}

	LONGLONG LeftSize									= RamdiskImageLength - Offset;
	ULONG MaxReadLength									= Length;
	ARC_STATUS Status									= ESUCCESS;
	*Count												= 0;
	if(Length > LeftSize)
		MaxReadLength									= static_cast<ULONG>(LeftSize);

	//
	// read it
	//
	while(MaxReadLength)
	{
		//
		// map to memory
		//
		LONGLONG AvailableLength						= 0;
		PVOID MappedAddress								= MapRamdisk(RamdiskImageOffset + Offset,&AvailableLength);
		RAMDISK_TRACE_INFO(("Mapped offset %x, va %p, availableLength %x.\n",static_cast<ULONG>(Offset),MappedAddress,AvailableLength));

		ULONG CopyLength								= MaxReadLength;
		if(CopyLength > AvailableLength)
			CopyLength									= static_cast<ULONG>(AvailableLength);

		//
		// copy to user buffer
		//
		RtlCopyMemory(Buffer,MappedAddress,CopyLength);

		//
		// update those
		//
		MaxReadLength									-= CopyLength;
		Buffer											= Add2Ptr(Buffer,CopyLength,PVOID);
		*Count											+= CopyLength;
		Offset											+= CopyLength;
	}

	//
	// update position
	//
	FileEntry->Position.QuadPart						+= *Count;
	return Status;
}

//
// read status
//
ARC_STATUS RamdiskReadStatus(__in ULONG FileId)
{
	RAMDISK_TRACE_INFO(("RamdiskReadStatus called.\n"));
	return EINVAL;
}

//
// seek
//
ARC_STATUS RamdiskSeek(__in ULONG FileId,__in PLARGE_INTEGER Offset,__in SEEK_MODE SeekMode)
{
	PBL_FILE_TABLE FileEntry							= BlFileTable + FileId;

	if(SeekMode == SeekAbsolute)
	{
		FileEntry->Position.QuadPart					= Offset->QuadPart;
	}
	else if(SeekMode == SeekRelative)
	{
		FileEntry->Position.QuadPart					= Offset->QuadPart + FileEntry->Position.QuadPart;
	}
	else
	{
		DbgPrint("SeekMode %lx not supported.\n");
		return EACCES;
	}

	return ESUCCESS;
}

//
// write
//
ARC_STATUS RamdiskWrite(__in ULONG FileId,__in PVOID Buffer,__in ULONG Length,__out PULONG Count)
{
	PBL_FILE_TABLE FileEntry							= BlFileTable + FileId;
	LONGLONG Offset										= FileEntry->Position.QuadPart;
	RAMDISK_DEBUG_PRINT(("RamdiskWrite entered.\n"));

	//
	// check EOF
	//
	if(Offset >= RamdiskImageLength)
		return EINVAL;

	LONGLONG LeftSize									= RamdiskImageLength - Offset;
	ULONG MaxWriteLength								= Length;
	ARC_STATUS Status									= ESUCCESS;
	*Count												= 0;
	if(Length > LeftSize)
		MaxWriteLength									= static_cast<ULONG>(LeftSize);

	//
	// write it
	//
	while(MaxWriteLength)
	{
		//
		// map to memory
		//
		LONGLONG AvailableLength						= 0;
		PVOID MappedAddress								= MapRamdisk(RamdiskImageOffset + Offset,&AvailableLength);
		ULONG CopyLength								= MaxWriteLength;
		if(CopyLength > AvailableLength)
			CopyLength									= static_cast<ULONG>(AvailableLength);

		//
		// copy to image buffer
		//
		RtlCopyMemory(MappedAddress,Buffer,CopyLength);

		//
		// update those
		//
		MaxWriteLength									-= CopyLength;
		Buffer											= Add2Ptr(Buffer,CopyLength,PVOID);
		*Count											+= CopyLength;
		Offset											+= CopyLength;
	}

	//
	// update position
	//
	FileEntry->Position.QuadPart						+= *Count;
	return Status;
}

//
// get file info
//
ARC_STATUS RamdiskGetFileInfo(__in ULONG FileId,__out PFILE_INFORMATION FileInformation)
{
	RtlZeroMemory(FileInformation,sizeof(FILE_INFORMATION));
	PBL_FILE_TABLE FileEntry							= BlFileTable + FileId;
	FileInformation->CurrentPosition.QuadPart			= FileEntry->Position.QuadPart;
	FileInformation->EndingAddress.QuadPart				= RamdiskImageLength;
	FileInformation->Type								= DiskPeripheral;
	return ESUCCESS;
}

//
// set file info
//
ARC_STATUS RamdiskSetFileInfo(__in ULONG FileId,__in ULONG AttributeFlags,__in ULONG AttributeMask)
{
	RAMDISK_TRACE_INFO(("RamdiskSetFileInfo called.\n"));
	return EINVAL;
}

//
// rename
//
ARC_STATUS RamdiskRename(__in ULONG FileId,__in PCHAR NewName)
{
	RAMDISK_TRACE_INFO(("RamdiskRename called.\n"));
	return EINVAL;
}

//
// get dir entry
//
ARC_STATUS RamdiskGetDirectoryEntry(__in ULONG FileId,__out PDIRECTORY_ENTRY Buffer,__in ULONG Length,__out PULONG Count)
{
	RAMDISK_TRACE_INFO(("RamdiskGetDirectoryEntry called.\n"));
	return EINVAL;
}

//
// read image from normal file
//
ARC_STATUS RamdiskReadImageFromNormalFile(__in ULONG DeviceId,__in PCHAR FilePath)
{
	ULONG FileId										= 0;
	BOOLEAN UpdateProgressBar							= TRUE;
	ULONG LastPercent									= 0;
	ARC_STATUS Status									= ESUCCESS;
	__try
	{
		//
		// open file
		//
		Status											= BlOpen(DeviceId,FilePath,ArcOpenReadOnly,&FileId);
		if(Status != ESUCCESS)
			try_leave(FileId = 0;RAMDISK_DEBUG_PRINT(("BlOpen(%s) failed: %d.\n",FilePath,Status)));

		//
		// get file size
		//
		FILE_INFORMATION FileInfo;
		Status											= BlGetFileInformation(FileId,&FileInfo);
		if(Status != ESUCCESS)
			try_leave(RAMDISK_DEBUG_PRINT(("BlGetFileInformation(%s) failed: %d.\n",FilePath,Status)));

		//
		// compute .....
		//
		RamdiskFileSize									= FileInfo.EndingAddress.QuadPart;
		RamdiskFileSizeInPages							= static_cast<ULONG>((RamdiskFileSize + PAGE_SIZE - 1) >> PAGE_SHIFT);
		if(!RamdiskImageLength || RamdiskImageLength > RamdiskFileSize - RamdiskImageOffset)
			RamdiskImageLength							= RamdiskFileSize - RamdiskImageOffset;

		//
		// allocate memory
		//
		Status											= BlAllocateAlignedDescriptor(LoaderXIPMemory,0,RamdiskFileSizeInPages,0,&RamdiskBasePage);
		if(Status != ESUCCESS)
			try_leave(RAMDISK_DEBUG_PRINT(("BlAllocateAlignedDescriptor(%d pages) failed %d.\n",RamdiskFileSizeInPages,Status)));

		RAMDISK_TRACE_INFO(("Allocated %d pages at page %x for RAM disk.\n",RamdiskFileSizeInPages,RamdiskBasePage));

		//
		// read to memory
		//
		LONGLONG LeftLength								= RamdiskFileSize;
		LARGE_INTEGER Offset							= {0};
		while(LeftLength)
		{
			//
			// get mapped address
			//
			LONGLONG AvailableLength					= 0;
			PVOID MappedAddress							= MapRamdisk(Offset.QuadPart,&AvailableLength);

			//
			// compute read length
			//
			ULONG ReadLength							= static_cast<ULONG>(min(LeftLength,AvailableLength));
			if(ReadLength > 1 * 1024 * 1024)
				ReadLength								= 1 * 1024 * 1024;

			//
			// seek
			//
			Status										= BlSeek(FileId,&Offset,SeekAbsolute);
			if(Status != ESUCCESS)
				try_leave(RAMDISK_DEBUG_PRINT(("BlSeek(%s) failed %d.\n",FilePath,Status)));

			//
			// read
			//
			ULONG ActualLength							= 0;
			Status										= BlRead(FileId,MappedAddress,ReadLength,&ActualLength);
			if(Status != ESUCCESS || ReadLength != ActualLength)
				try_leave(RAMDISK_DEBUG_PRINT(("BlRead(%s) failed %d.\n",FilePath,Status)));

			//
			// update
			//
			Offset.QuadPart								+= ReadLength;
			LeftLength									-= ReadLength;

			//
			// update progress bar
			//
			ULONG NewPercent							= static_cast<ULONG>(Offset.QuadPart * 100 / RamdiskFileSize);
			if(UpdateProgressBar || LastPercent != NewPercent)
				BlUpdateProgressBar(NewPercent);


			LastPercent									= NewPercent;
			UpdateProgressBar							= FALSE;
		}

		RAMDISK_TRACE_INFO(("Done reading ramdisk.\n"));
	}
	__finally
	{
		if(FileId)
			BlClose(FileId);
	}

	return Status;
}

//
// read image file
//
ARC_STATUS RamdiskReadImage(__in PCHAR FilePath)
{
	BlOutputStartupMsg(BL_RAMDISK_LOADING);
	BlUpdateProgressBar(0);
	RAMDISK_TRACE_INFO(("RamdiskReadImage(%s).\n",FilePath));

	PCHAR Temp											= strchr(FilePath,'\\');
	if(!Temp)
	{
		RAMDISK_DEBUG_PRINT(("no \\ found in path.\n"));
		return EINVAL;
	}

	*Temp												= 0;

	//
	// file path now should be an ARC device path
	//
	ULONG DeviceId										= 0;
	BOOLEAN LowcasedNameTried							= FALSE;
	while(1)
	{
		ARC_STATUS Status								= ArcOpen(FilePath,ArcOpenReadWrite,&DeviceId);
		if(Status == ESUCCESS)
			break;

		RAMDISK_DEBUG_PRINT(("ArcOpen(%s) failed: %d.\n",FilePath));
		if(LowcasedNameTried)
			return Status;

		_strlwr(FilePath);
		LowcasedNameTried								= TRUE;
	}

	//
	// restore \
	//
	*Temp												= '\\';

	//
	// and point temp to file path
	//
	Temp												+= 1;

	//
	// backup
	//
	ULONG SavedBase										= BlUsableBase;
	ULONG SavedLimit									= BlUsableLimit;
	BlUsableLimit										= 0x20000;
	ARC_STATUS Status									= EINVAL;

	if(DeviceId == 'dten')
	{
	#if _NET_SUPPORT_
		//
		// 'net' device
		//
		if(RamdiskMTFTPAddr)
			Status										= RamdiskReadImageByMTFTP(DeviceId,Temp);
		else
			Status										= RamdiskReadImageByTFTP(DeviceId,Temp);
	#endif
	}
	else
	{
		//
		// 'normal' device
		//
		Status											= RamdiskReadImageFromNormalFile(DeviceId,Temp);
	}

	BlUsableBase										= SavedBase;
	BlUsableLimit										= SavedLimit;
	ArcCacheClose(DeviceId);

	return Status;
}

//
// initialize from path
//
ARC_STATUS RamdiskInitializeFromPath()
{
	ASSERT(RamdiskPath);
	RAMDISK_TRACE_INFO(("RamdiskInitializeFromPath(%s).\n",RamdiskPath));

	ARC_STATUS Status									= RamdiskReadImage(RamdiskPath);
	if(Status != ESUCCESS)
		return Status;

	RAMDISK_TRACE_INFO(("Ramdisk is active.\n"));
	RamdiskActive										= TRUE;
	return Status;
}

//
// get option value
//
PCHAR RamdiskGetOptionValue(__in PCHAR Options,__in PCHAR Name)
{
	ASSERT(Options);
	ASSERT(Name);

	PCHAR Temp											= strstr(Options,Name);
	if(!Temp)
		return 0;

	Temp												= strchr(Temp,'=');
	if(!Temp)
		return 0;

	Temp												+= 1;
	PCHAR End											= Temp;
	while(*End && *End != ' ' && *End != '/' && *End != '\t' && *End != '\r' && *End != '\n')
		End												+= 1;

	ULONG Length										= End - Temp;
	PCHAR Value											= static_cast<PCHAR>(BlAllocateHeap(Length + 1));
	if(Value)
		strncpy(Value,Temp,Length);

	return Value;
}

#if _NET_SUPPORT_
//
// parse ip address
//
ULONG RamdiskParseIPAddr(__in PCHAR Value)
{
	if(!Value)
		return 0;

	ULONG DotCount										= 0;
	ULONG Current										= 0;
	ULONG IpAddress										= 0;
	while(*Value)
	{
		CHAR ch											= *Value;
		if(ch >= '0' && ch <= '9')
		{
			Current										= Current * 10 + ch - '0';
			if(Current > 255)
				return 0;
		}
		else if(ch == '.')
		{
			IpAddress									<<= 8;
			IpAddress									+= Current;
			Current										= 0;
			DotCount									+= 1;
		}
		else
		{
			break;
		}

		Value											+= 1;
	}

	if(DotCount != 3)
		return 0;

	return ((IpAddress & 0xff) << 24) | ((IpAddress & 0xff00) << 8) | ((IpAddress & 0xff0000) >> 8) | ((IpAddress & 0xff000000) >> 24);
}
#endif

//
// parse options
//
ARC_STATUS RamdiskParseOptions(__in PCHAR Options)
{
	if(!Options)
		return ESUCCESS;

	PCHAR Value											= RamdiskGetOptionValue(Options,"RDPATH");
	RamdiskPath											= Value;
	if(!Value)
		return ESUCCESS;

	Value												= RamdiskGetOptionValue(Options,"RDIMAGEOFFSET");
	if(Value)
		RamdiskImageOffset								= atoi(Value);

	Value												= RamdiskGetOptionValue(Options,"RDIMAGELENGTH");
	if(Value)
		RamdiskImageLength								= _atoi64(Value);

#if _NET_SUPPORT_
	RamdiskTFTPAddr										= NetServerIpAddress;
	Value												= RamdiskGetOptionValue(Options,"RDMTFTPADDR");
	if(Value)
	{
		RamdiskMTFTPAddr								= RamdiskParseIPAddr(Value);

		Value											= RamdiskGetOptionValue(Options,"RDMTFTPCPORT");
		if(Value)
			RamdiskMTFTPCPort							= static_cast<USHORT>(((atoi(Value) & 0xff) << 8) | ((atoi(Value) & 0xff00) >> 8));

		Value											= RamdiskGetOptionValue(Options,"RDMTFTPSPORT");
		if(Value)
			RamdiskMTFTPSPort							= static_cast<USHORT>(((atoi(Value) & 0xff) << 8) | ((atoi(Value) & 0xff00) >> 8));

		Value											= RamdiskGetOptionValue(Options,"RDMTFTPDELAY");
		if(Value)
			RamdiskMTFTPDelay							= static_cast<USHORT>(atoi(Value));

		Value											= RamdiskGetOptionValue(Options,"RDMTFTPTIMEOUT");
		if(Value)
			RamdiskMTFTPTimeout							= static_cast<USHORT>(atoi(Value));

		Value											= RamdiskGetOptionValue(Options,"RDFILESIZE");
		if(Value)
			RamdiskMTFTPFileSize						= _atoi64(Value);

		Value											= RamdiskGetOptionValue(Options,"RDCHUNKSIZE");
		if(Value)
			RamdiskMTFTPChunkSize						= _atoi64(Value);

		//
		// check network options
		//
		if(!RamdiskMTFTPAddr || !RamdiskMTFTPCPort || !RamdiskMTFTPSPort || !RamdiskMTFTPDelay || !RamdiskMTFTPTimeout || !RamdiskMTFTPFileSize)
			return EINVAL;

		//
		// check chunk size
		//
		if(RamdiskMTFTPChunkSize > RamdiskMTFTPFileSize)
			return EINVAL;
	}
#endif

	return ESUCCESS;
}

//
// intialize ramdisk
//
ARC_STATUS RamdiskInitialize(__in PCHAR Options,__in BOOLEAN FirstTime)
{
	//
	// debug break
	//
	if(RamdiskBreak)
		DbgBreakPoint();

	ARC_STATUS Status									= ESUCCESS;
	BOOLEAN SaveShowProgressBar							= BlShowProgressBar;
	BOOLEAN SavedBlOutputDots							= BlOutputDots;
	ULONG ErrorMessage									= 0x3a99;
	__try
	{
		if(FirstTime)
		{
			//
			// sdi address must be valid
			//
			if(!SdiAddress)
				try_leave(Status = EINVAL);
		}
		else
		{
			//
			// free 0x10000
			//
			if(RamdiskBasePage)
				BlFreeDescriptor(0x10);

			//
			// parse options
			//
			Status										= RamdiskParseOptions(Options);
			if(Status != ESUCCESS)
				try_leave(Status);
		}

		//
		// show progress bar
		//
		if(RamdiskBuild || RamdiskPath)
		{
			DisplayLogoOnBoot							= FALSE;
			BlShowProgressBar							= TRUE;
			BlOutputDots								= TRUE;
			GraphicsMode								= FALSE;
		}

		//
		// initialize from file
		//
		if(RamdiskPath)
			try_leave(Status = RamdiskInitializeFromPath());

		//
		// is not the first time? otherwise try to load from SdiAddress
		//
		if(!FirstTime)
			try_leave(NOTHING);

		//
		// allocate 0x10000 - 0x20000
		//
		ULONG BasePage									= 0x10;
		BlAllocateAlignedDescriptor(LoaderFirmwareTemporary,0x10,0x10,0,&BasePage);

		//
		// allocate sdi pages
		//
		ULONG SavedBase									= BlUsableBase;
		ULONG SavedLimit								= BlUsableLimit;
		BlUsableLimit									= 0x20000;
		BasePage										= SdiAddress >> PAGE_SHIFT;
		ULONG Count										= ADDRESS_AND_SIZE_TO_SPAN_PAGES(SdiAddress,PAGE_SIZE) >> PAGE_SHIFT;
		BlAllocateAlignedDescriptor(LoaderFirmwareTemporary,BasePage,Count,0,&BasePage);
		BlUsableLimit									= SavedLimit;
		BlUsableBase									= SavedBase;

		//
		// search PART record
		//
		PSDI_HEADER SdiHeader							= reinterpret_cast<PSDI_HEADER>(SdiAddress);
		ULONG PartRecord								= 0;
		for(; PartRecord < 8; PartRecord ++)
		{
			if(SdiHeader->TOCRecord[PartRecord].BLOBtype[0] == 'TRAP')
				break;
		}

		//
		// PART record not found?
		//
		if(PartRecord == 8)
			try_leave(Status = ENOENT;ErrorMessage = BL_RAMDISK_OPEN_FAILED);

		//
		// setup those......
		//
		ULONG PartAddress								= SdiHeader->TOCRecord[PartRecord].Offset.LowPart + SdiAddress;
		RamdiskBasePage									= PartAddress >> PAGE_SHIFT;
		RamdiskImageOffset								= PartAddress - (RamdiskBasePage << PAGE_SHIFT);
		RamdiskImageLength								= SdiHeader->TOCRecord[PartRecord].Size.QuadPart;
		RamdiskFileSizeInPages							= static_cast<ULONG>(((PartAddress & (PAGE_SIZE - 1)) + RamdiskImageLength + PAGE_SIZE - 1) >> PAGE_SHIFT);
		RamdiskFileSize									= RamdiskFileSizeInPages << PAGE_SHIFT;

		//
		// free sdi address
		//
		BlFreeDescriptor(BasePage);

		//
		// reallocate new pages
		//
		SavedBase										= BlUsableBase;
		SavedLimit										= BlUsableLimit;
		BlUsableLimit									= 0x20000;
		BasePage										= RamdiskBasePage;
		Status											= BlAllocateAlignedDescriptor(LoaderXIPMemory,RamdiskBasePage,RamdiskFileSizeInPages,0,&BasePage);
		BlUsableLimit									= SavedLimit;
		BlUsableBase									= SavedBase;
		ASSERT(Status == ESUCCESS);
		ASSERT(BasePage == RamdiskBasePage);

		RAMDISK_TRACE_INFO(("Ramdisk is active.\n"));
		RamdiskActive									= TRUE;
	}
	__finally
	{
		if(Status != ESUCCESS)
			RamdiskFatalError(BL_RAMDISK_BOOT_FAILED,ErrorMessage);
		else if(RamdiskBuild || RamdiskPath)
			BlClearScreen();

		BlShowProgressBar								= SaveShowProgressBar;
		BlOutputDots									= SavedBlOutputDots;
	}

	return Status;
}

//
// sdi boot
//
VOID RamdiskSdiBoot(__in PCHAR SdiFilePath)
{
	//
	// always show progress bar
	//
	BOOLEAN SavedShowProgressBar						= BlShowProgressBar;
	BlShowProgressBar									= TRUE;

	//
	// setup ......
	//
	RamdiskImageOffset									= 0;
	RamdiskImageLength									= 0;

#if _NET_SUPPORT_
	//
	// copy net server ip address
	//
	extern ULONG NetServerIpAddress;
	RamdiskTFTPAddr										= NetServerIpAddress;
#endif

	//
	// read the file
	//
	ARC_STATUS Status									= RamdiskReadImage(SdiFilePath);
	if(Status != ESUCCESS)
		return RamdiskFatalError(BL_RAMDISK_BOOT_FAILED,BL_RAMDISK_OPEN_FAILED);

	//
	// restore progress bar
	//
	BlShowProgressBar									= SavedShowProgressBar;

	//
	// map file header
	//
	LONGLONG AvailableLength							= 0;
	PSDI_HEADER	SdiHeader								= static_cast<PSDI_HEADER>(MapRamdisk(0,&AvailableLength));
	ASSERT(AvailableLength >= sizeof(SDI_HEADER));
	ASSERT(AvailableLength >= (SdiHeader->BootCodeOffset.QuadPart + SdiHeader->BootCodeSize.QuadPart));
	ASSERT(SdiHeader->BootCodeOffset.HighPart == 0);
	ASSERT(SdiHeader->BootCodeSize.HighPart == 0);

	//
	// copy to 7c00:0000
	//
	RtlCopyMemory(reinterpret_cast<PVOID>(0x7c00),Add2Ptr(SdiHeader,SdiHeader->BootCodeOffset.LowPart,PVOID),SdiHeader->BootCodeSize.LowPart);

	//
	// shutdown net
	//
#if _NET_SUPPORT_
	if(BlBootingFromNet)
		NetTerminate();
#endif

	//
	// stop debugger
	//
	BdStopDebugger();

	//
	// reboot
	//
	ExternalServicesTable->Reboot(reinterpret_cast<ULONG>(SdiHeader) & 3);
}

//
// device entry
//	why bootfs info is zero?
//
BL_DEVICE_ENTRY_TABLE									RamdiskEntryTable =
{
	&RamdiskClose,
	&RamdiskMount,
	&RamdiskOpen,
	&RamdiskRead,
	&RamdiskReadStatus,
	&RamdiskSeek,
	&RamdiskWrite,
	&RamdiskGetFileInfo,
	&RamdiskSetFileInfo,
	&RamdiskRename,
	&RamdiskGetDirectoryEntry,
	0,
};
#endif