//********************************************************************
//	created:	20:8:2008   3:00
//	file:		blload.cpp
//	author:		tiamo
//	purpose:	load registry,device driver,nls data,hal font
//********************************************************************

#include "stdafx.h"

//
// disable driver verifier
//
BOOLEAN													BlDisableVerifier;

//
// disable crash auto reboot
//
BOOLEAN													BlDisableCrashAutoReboot;

//
// used by load oem hal font
//
#define FONT_DIRECTORY									0x8007

//
// font resource
//
#define FONT_RESOURCE									0x8008

//
// resource type info
//
typedef struct _RESOURCE_TYPE_INFORMATION
{
	//
	// id
	//
	USHORT												Ident;

	//
	// number
	//
	USHORT												Number;

	//
	// proc
	//
	LONG												Proc;
}RESOURCE_TYPE_INFORMATION,*PRESOURCE_TYPE_INFORMATION;

//
// resource name info
//
typedef struct _RESOURCE_NAME_INFORMATION
{
	//
	// offset
	//
	USHORT												Offset;

	//
	// length
	//
	USHORT												Length;

	//
	// flags
	//
	USHORT												Flags;

	//
	// ident
	//
	USHORT												Ident;

	//
	// handle
	//
	USHORT												Handle;

	//
	// usage
	//
	USHORT												Usage;
}RESOURCE_NAME_INFORMATION,*PRESOURCE_NAME_INFORMATION;

//
// load normal file
//
ARC_STATUS BlLoadFileImage(__in ULONG DeviceId,__in PCHAR DeviceName,__in PCHAR DirectoryPath,__in PUNICODE_STRING FileName,
						   __in TYPE_OF_MEMORY MemoryType,__out PVOID* FileBuffer,__out PULONG FileSize,__out_opt PCHAR BadFile)
{
	*FileSize											= 0;
	*FileBuffer											= 0;

	//
	// build full file path
	//
	CHAR LocalFilePath[0x100];
	sprintf(LocalFilePath,"%s%wZ",DirectoryPath,FileName);

	//
	// output loading message
	//
	BlOutputLoadMessage(DeviceName,LocalFilePath,0);

	//
	// open the file
	//
	ULONG FileId										= 0;
	ARC_STATUS Status									= BlOpen(DeviceId,LocalFilePath,ArcOpenReadOnly,&FileId);
	if(Status == ESUCCESS)
	{
		//
		// update boot status
		//
		BlUpdateBootStatus();

		//
		// get file size
		//
		FILE_INFORMATION FileInfo;
		Status											= BlGetFileInformation(FileId,&FileInfo);
		if(Status == ESUCCESS)
		{
			//
			// allocate memory
			//
			*FileSize									= FileInfo.EndingAddress.LowPart;
			ULONG BasePage								= 0;
			Status										= BlAllocateAlignedDescriptor(MemoryType,0,BYTES_TO_PAGES(*FileSize),1,&BasePage);
			if(Status == ESUCCESS)
			{
				//
				// read file
				//
				*FileBuffer								= MAKE_KERNEL_ADDRESS_PAGE(BasePage);
				Status									= BlRead(FileId,*FileBuffer,*FileSize,FileSize);
				if(Status != ESUCCESS)
				{
					*FileSize							= 0;
					*FileBuffer							= 0;

					BlFreeDescriptor(BasePage);
				}
			}
		}

		BlClose(FileId);
	}

	if(Status != ESUCCESS && BadFile)
		strcpy(BadFile,LocalFilePath);

	return Status;
}

//
// load system.log
//
ARC_STATUS BlLoadSystemHiveLog(__in ULONG DeviceId,__in PCHAR DeviceName,__in PCHAR DirectoryPath,__in PCHAR FileName,__out PVOID* FileBuffer)
{
	//
	// build full file path
	//
	CHAR LocalFilePath[0x100];
	strcpy(LocalFilePath,DirectoryPath);
	strcat(LocalFilePath,FileName);

	//
	// output loading message
	//
	BlOutputLoadMessage(DeviceName,LocalFilePath,0);

	//
	// open the file
	//
	ULONG FileId										= 0;
	ARC_STATUS Status									= BlOpen(DeviceId,LocalFilePath,ArcOpenReadOnly,&FileId);
	if(Status != ESUCCESS)
		return Status;

	//
	// update boot status
	//
	BlUpdateBootStatus();

	//
	// get file size
	//
	FILE_INFORMATION FileInfo;
	Status												= BlGetFileInformation(FileId,&FileInfo);
	if(Status == ESUCCESS)
	{
		if(!FileInfo.EndingAddress.LowPart)
		{
			Status										= EINVAL;
		}
		else
		{
			//
			// allocate memory
			//
			ULONG FileSize								= FileInfo.EndingAddress.LowPart;
			ULONG BasePage								= 0;
			Status										= BlAllocateAlignedDescriptor(LoaderRegistryData,0,BYTES_TO_PAGES(FileSize),1,&BasePage);
			if(Status == ESUCCESS)
			{
				//
				// read file
				//
				*FileBuffer								= MAKE_KERNEL_ADDRESS_PAGE(BasePage);
				Status									= BlRead(FileId,*FileBuffer,FileSize,&FileSize);
				if(Status != ESUCCESS)
				{
					*FileBuffer							= 0;
					BlFreeDescriptor(BasePage);
				}
			}
		}
	}

	BlClose(FileId);

	return Status;
}

//
// load system hive
//
ARC_STATUS BlLoadSystemHive(__in ULONG DeviceId,__in PCHAR DeviceName,__in PCHAR DirectoryPath,__in PCHAR FileName)
{
	//
	// build full file path
	//
	CHAR LocalFilePath[0x100];
	strcpy(LocalFilePath,DirectoryPath);
	strcat(LocalFilePath,FileName);

	//
	// output loading message
	//
	BlOutputLoadMessage(DeviceName,LocalFilePath,0);

	//
	// open the file
	//
	ULONG FileId										= 0;
	ARC_STATUS Status									= BlOpen(DeviceId,LocalFilePath,ArcOpenReadOnly,&FileId);
	if(Status != ESUCCESS)
		return Status;

	//
	// update boot status
	//
	BlUpdateBootStatus();

	//
	// get file size
	//
	FILE_INFORMATION FileInfo;
	Status												= BlGetFileInformation(FileId,&FileInfo);
	if(Status == ESUCCESS)
	{
		if(!FileInfo.EndingAddress.LowPart)
		{
			Status										= EINVAL;
		}
		else
		{
			//
			// allocate memory
			//
			ULONG FileSize								= FileInfo.EndingAddress.LowPart;
			ULONG BasePage								= 0;
			Status										= BlAllocateAlignedDescriptor(LoaderRegistryData,0,BYTES_TO_PAGES(FileSize),1,&BasePage);
			if(Status == ESUCCESS)
			{
				//
				// read file
				//
				BlLoaderBlock->RegistryBase				= Add2Ptr(MAKE_KERNEL_ADDRESS_PAGE(BasePage),BlVirtualBias,PVOID);
				BlLoaderBlock->RegistryLength			= FileSize;
				Status									= BlRead(FileId,MAKE_KERNEL_ADDRESS_PAGE(BasePage),FileSize,&FileSize);
			}
		}
	}

	BlClose(FileId);

	return Status;
}

//
// load nls data
//
ARC_STATUS BlLoadNLSData(__in ULONG DeviceId,__in PCHAR DeviceName,__in PCHAR DirectoryPath,__in PUNICODE_STRING AnsiCodepage,
						 __in PUNICODE_STRING OemCodepage,__in PUNICODE_STRING LanguageTable,__out PCHAR BadFileName)
{
	//
	// under the japanese version of windows,ANSI code page and OEM codepage is same.in this case, we share the same data to save and memory.
	//
	BOOLEAN OemIsSameAsAnsi								= FALSE;
	if(AnsiCodepage->Length == OemCodepage->Length && !_wcsnicmp(AnsiCodepage->Buffer,OemCodepage->Buffer,AnsiCodepage->Length))
		OemIsSameAsAnsi									= TRUE;

	CHAR LocalFileName[132];
	ARC_STATUS Status									= ESUCCESS;
	ULONG AnsiFileId									= 0;
	ULONG OemFileId										= 0;
	ULONG LanguageFileId								= 0;
	ULONG ActualBase									= 0;

	__try
	{
		//
		// make ansic code page file name
		//
		sprintf(LocalFileName,"%s%wZ",DirectoryPath,AnsiCodepage);

		//
		// Output load message
		//
		BlOutputLoadMessage(DeviceName, LocalFileName,0);

		//
		// open the ansi data file
		//
		Status											= BlOpen(DeviceId,LocalFileName,ArcOpenReadOnly,&AnsiFileId);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// update boot status
		//
		BlUpdateBootStatus();

		//
		// get file size
		//
		FILE_INFORMATION FileInformation;
		Status											= BlGetFileInformation(AnsiFileId,&FileInformation);

		//
		// close it
		//
		BlClose(AnsiFileId);
		AnsiFileId										= 0;

		if(Status != ESUCCESS)
			try_leave(NOTHING);

		ULONG AnsiFileSize								= FileInformation.EndingAddress.LowPart;
		ULONG OemFileSize								= 0;
		if(!OemIsSameAsAnsi)
		{
			//
			// make oem file name
			//
			sprintf(LocalFileName,"%s%wZ",DirectoryPath,OemCodepage);

			//
			// output load message
			//
			BlOutputLoadMessage(DeviceName,LocalFileName,0);

			//
			// open the oem data file
			//
			Status										= BlOpen(DeviceId,LocalFileName,ArcOpenReadOnly,&OemFileId);
			if(Status != ESUCCESS)
				try_leave(NOTHING);

			//
			// update boot status
			//
			BlUpdateBootStatus();

			//
			// get file size
			//
			Status										= BlGetFileInformation(OemFileId, &FileInformation);

			//
			// close it
			//
			BlClose(OemFileId);
			OemFileId									= 0;

			if(Status != ESUCCESS)
				try_leave(NOTHING);

			OemFileSize									= FileInformation.EndingAddress.LowPart;
		}

		//
		// make language table file name
		//
		sprintf(LocalFileName,"%s%wZ",DirectoryPath,LanguageTable);

		//
		// output load message
		//
		BlOutputLoadMessage(DeviceName,LocalFileName,0);

		//
		// open the language codepage file
		//
		Status											= BlOpen(DeviceId,LocalFileName,ArcOpenReadOnly,&LanguageFileId);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// update boot status
		//
		BlUpdateBootStatus();

		//
		// get file size
		//
		Status											= BlGetFileInformation(LanguageFileId,&FileInformation);

		//
		// close it
		//
		BlClose(LanguageFileId);
		LanguageFileId									= 0;

		if(Status != ESUCCESS)
			try_leave(NOTHING);

		ULONG LanguageFileSize							= FileInformation.EndingAddress.LowPart;

		//
		// Calculate the total size of the descriptor needed.
		// we want each data file to start on a page boundary, so round up each size to page granularity.
		//
		ULONG TotalSize									=  ROUND_TO_PAGES(AnsiFileSize);
		TotalSize										+= ROUND_TO_PAGES(OemFileSize);
		TotalSize										+= ROUND_TO_PAGES(LanguageFileSize);
		Status											= BlAllocateAlignedDescriptor(LoaderNlsData,0,TotalSize >> PAGE_SHIFT,1,&ActualBase);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		PVOID LocalPointer								= MAKE_KERNEL_ADDRESS_PAGE(ActualBase);
		PVOID AnsiCodePageData							= LocalPointer;
		PVOID OemCodePageData							= Add2Ptr(LocalPointer,ROUND_TO_PAGES(AnsiFileSize),PVOID);
		PVOID UnicodeCaseTableData						= Add2Ptr(OemCodePageData,ROUND_TO_PAGES(OemFileSize),PVOID);

		//
		// open ansi data file
		//
		sprintf(LocalFileName,"%s%wZ",DirectoryPath,AnsiCodepage);
		Status											= BlOpen(DeviceId,LocalFileName,ArcOpenReadOnly,&AnsiFileId);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// seek to begin
		//
		LARGE_INTEGER SeekValue;
		SeekValue.QuadPart = 0;
		Status											= BlSeek(AnsiFileId,&SeekValue,SeekAbsolute);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// read
		//
		ULONG Count										= 0;
		Status											= BlRead(AnsiFileId,AnsiCodePageData,AnsiFileSize,&Count);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// close it
		//
		BlClose(AnsiFileId);
		AnsiFileId										= 0;

		if(!OemIsSameAsAnsi)
		{
			//
			// open oem data file
			//
			sprintf(LocalFileName,"%s%wZ",DirectoryPath,OemCodepage);
			Status										= BlOpen(DeviceId,LocalFileName,ArcOpenReadOnly,&OemFileId);
			if(Status != ESUCCESS)
				try_leave(NOTHING);

			//
			// seek to begin
			//
			SeekValue.QuadPart							= 0;
			Status										= BlSeek(OemFileId,&SeekValue,SeekAbsolute);
			if(Status != ESUCCESS)
				try_leave(NOTHING);

			//
			// read
			//
			Status										= BlRead(OemFileId,OemCodePageData,OemFileSize,&Count);
			if(Status != ESUCCESS)
				try_leave(NOTHING);

			//
			// close it
			//
			BlClose(OemFileId);
			OemFileId									= 0;
		}

		//
		// open language file
		//
		sprintf(LocalFileName,"%s%wZ",DirectoryPath,LanguageTable);
		Status											= BlOpen(DeviceId,LocalFileName,ArcOpenReadOnly,&LanguageFileId);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// seek to beginning
		//
		SeekValue.QuadPart = 0;
		Status											= BlSeek(LanguageFileId,&SeekValue,SeekAbsolute);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// read it
		//
		Status											= BlRead(LanguageFileId,UnicodeCaseTableData,LanguageFileSize,&Count);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// close it
		//
		BlClose(LanguageFileId);
		LanguageFileId									= 0;

		//
		// setup load block
		//
		BlLoaderBlock->NlsData->AnsiCodePageData		= Add2Ptr(AnsiCodePageData,BlVirtualBias,PVOID);
		BlLoaderBlock->NlsData->OemCodePageData			= OemIsSameAsAnsi ? BlLoaderBlock->NlsData->AnsiCodePageData : Add2Ptr(OemCodePageData,BlVirtualBias,PVOID);
		BlLoaderBlock->NlsData->UnicodeCaseTableData	= Add2Ptr(UnicodeCaseTableData,BlVirtualBias,PVOID);
	}
	__finally
	{
		if(Status != ESUCCESS)
		{
			if(ActualBase)
				BlFreeDescriptor(ActualBase);

			strcpy(BadFileName,LocalFileName);
		}

		if(LanguageFileId)
			BlClose(LanguageFileId);

		if(OemFileId)
			BlClose(OemFileId);

		if(AnsiFileId)
			BlClose(AnsiFileId);
	}

	return Status;
}

//
// load font used by HalDisplayString
//
ARC_STATUS BlLoadOemHalFont(__in ULONG DeviceId,__in PCHAR DeviceName,__in PCHAR DirectoryPath,__in PUNICODE_STRING OemHalFont,__out PCHAR BadFileName)
{
	BlLoaderBlock->OemFontFile							= 0;
	ULONG FileId										= 0;
	ARC_STATUS Status									= ESUCCESS;

	CHAR LocalFileName[132];
	sprintf(LocalFileName,"%s%wZ",DirectoryPath,OemHalFont);

	__try
	{
		//
		// output load message
		//
		BlOutputLoadMessage(DeviceName,LocalFileName,0);

		//
		// open the oem font file.
		//
		Status											= BlOpen(DeviceId,LocalFileName,ArcOpenReadOnly,&FileId);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// update boot status
		//
		BlUpdateBootStatus();

		//
		// get the size of the font file
		//
		FILE_INFORMATION FileInformation;
		Status											= BlGetFileInformation(FileId,&FileInformation);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// allocate file buffer
		//
		ULONG FileSize									= FileInformation.EndingAddress.LowPart;
		PVOID FileBuffer								= BlAllocateHeap(FileSize + BlDcacheFillSize - 1);
		if(!FileBuffer)
			try_leave(Status = ENOMEM);

		//
		// round the file buffer address up to a cache line boundary and read the file into memory.
		//
		FileBuffer										= ALIGN_BUFFER(FileBuffer);
		ULONG Count										= 0;
		Status											= BlRead(FileId,FileBuffer,FileSize,&Count);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// check if the file has a DOS header or a font file header.
		// if the file has a font file header, then it is a .fnt file.
		// otherwise, it must be checked for an OS/2 executable with a font resource.
		//
		PIMAGE_DOS_HEADER DosHeader						= static_cast<PIMAGE_DOS_HEADER>(FileBuffer);
		if(DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		{
			//
			// check if the file has a font file header.
			//
			POEM_FONT_FILE_HEADER FontHeader			= static_cast<POEM_FONT_FILE_HEADER>(FileBuffer);
			if(FontHeader->Version != OEM_FONT_VERSION || FontHeader->Type != OEM_FONT_TYPE || FontHeader->Italic != OEM_FONT_ITALIC)
				try_leave(Status = EBADF);

			if(FontHeader->Underline != OEM_FONT_UNDERLINE || FontHeader->StrikeOut != OEM_FONT_STRIKEOUT || FontHeader->CharacterSet != OEM_FONT_CHARACTER_SET)
				try_leave(Status = EBADF);

			if(FontHeader->Family != OEM_FONT_FAMILY ||FontHeader->PixelWidth > 32)
				try_leave(Status = EBADF);

			BlLoaderBlock->OemFontFile					= FileBuffer;
			try_leave(Status = ESUCCESS);
		}

		//
		// check if the file has an os/2 header.
		//
		if(FileSize < sizeof(IMAGE_DOS_HEADER) || FileSize < static_cast<ULONG>(DosHeader->e_lfanew))
			try_leave(Status = EBADF);

		PIMAGE_OS2_HEADER Os2Header						= Add2Ptr(DosHeader,DosHeader->e_lfanew,PIMAGE_OS2_HEADER);
		if(Os2Header->ne_magic != IMAGE_OS2_SIGNATURE)
			try_leave(Status = EBADF);

		//
		// check if the resource table exists.
		//
		if(Os2Header->ne_restab - Os2Header->ne_rsrctab == 0)
			try_leave(Status = EBADF);

		//
		// compute address of resource table and search the table for a font resource.
		//
		PRESOURCE_TYPE_INFORMATION TableAddress			= Add2Ptr(Os2Header,Os2Header->ne_rsrctab,PRESOURCE_TYPE_INFORMATION);
		PRESOURCE_TYPE_INFORMATION TableEnd				= Add2Ptr(Os2Header,Os2Header->ne_restab,PRESOURCE_TYPE_INFORMATION);
		SHORT ScaleFactor								= *reinterpret_cast<SHORT UNALIGNED *>(TableAddress);
		TableAddress									= Add2Ptr(TableAddress,sizeof(SHORT),PRESOURCE_TYPE_INFORMATION);

		while(TableAddress < TableEnd && TableAddress->Ident != 0 && TableAddress->Ident != FONT_RESOURCE)
			TableAddress								= Add2Ptr(TableAddress + 1,TableAddress->Number * sizeof(RESOURCE_NAME_INFORMATION),PRESOURCE_TYPE_INFORMATION);

		if(TableAddress >= TableEnd || TableAddress->Ident != FONT_RESOURCE)
			try_leave(Status = EBADF);

		//
		// compute address of resource name information and check if the resource is within the file.
		//
		PRESOURCE_NAME_INFORMATION TableName			= static_cast<PRESOURCE_NAME_INFORMATION>(static_cast<PVOID>(TableAddress + 1));
		if(FileSize < (TableName->Offset << ScaleFactor) + sizeof(OEM_FONT_FILE_HEADER))
			try_leave(Status = EBADF);

		//
		// compute the address of the font file header and check if the header contains correct information.
		//
		POEM_FONT_FILE_HEADER FontHeader				= Add2Ptr(FileBuffer,TableName->Offset << ScaleFactor,POEM_FONT_FILE_HEADER);
		if(FontHeader->Version != OEM_FONT_VERSION || FontHeader->Type != OEM_FONT_TYPE || FontHeader->Italic != OEM_FONT_ITALIC)
			try_leave(Status = EBADF);

		if(FontHeader->Underline != OEM_FONT_UNDERLINE || FontHeader->StrikeOut != OEM_FONT_STRIKEOUT || FontHeader->CharacterSet != OEM_FONT_CHARACTER_SET)
			try_leave(Status = EBADF);

		if(FontHeader->Family != OEM_FONT_FAMILY ||FontHeader->PixelWidth > 32)
			try_leave(Status = EBADF);

		BlLoaderBlock->OemFontFile						= FontHeader;
		Status											= ESUCCESS;
	}
	__finally
	{
		if(FileId)
			BlClose(FileId);

		if(Status != ESUCCESS)
			strcpy(BadFileName,LocalFileName);
	}

	return Status;
}

//
// load and scan system hive
//
ARC_STATUS BlLoadAndScanSystemHive(__in ULONG DeviceId,__in PCHAR DeviceName,__in PCHAR DirectoryPath,__in PWCHAR BootFileSystem,
								   __inout PBOOLEAN UseLastKnownGood,__out PBOOLEAN LoadSacDriver,__out PCHAR BadFile)
{
	ARC_STATUS Status									= ESUCCESS;

	__try
	{
		//
		// build local path
		//
		CHAR LocalPath[0x100];
		strcpy(LocalPath,DirectoryPath);
		strcat(LocalPath,"\\system32\\config\\");

		//
		// try to load system hive
		//
		BOOLEAN IsAlternate								= FALSE;
		BOOLEAN RestartSetup							= FALSE;
		Status											= BlLoadAndInitSystemHive(DeviceId,DeviceName,LocalPath,"system",FALSE,&IsAlternate,&RestartSetup);

		//
		// load system failed
		//
		if(Status != ESUCCESS)
		{
			//
			// try to load system.alt
			//
			if(!RestartSetup)
				Status									= BlLoadAndInitSystemHive(DeviceId,DeviceName,LocalPath,"system.alt",TRUE,&IsAlternate,&RestartSetup);

			//
			// so bad,can not load system hive,return
			//
			if(Status != ESUCCESS)
				try_leave(strcpy(BadFile,DirectoryPath);strcat(BadFile,"\\SYSTEM32\\CONFIG\\SYSTEM"));
		}

		//
		// if we are using alternate hive,try to change to system.sav
		//
		if(IsAlternate)
			Status										= BlLoadAndInitSystemHive(DeviceId,DeviceName,LocalPath,"system.sav",TRUE,&IsAlternate,&RestartSetup);

		//
		// load system.sav failed
		//
		if(Status != ESUCCESS)
			try_leave(strcpy(BadFile,DirectoryPath);strcat(BadFile,"\\SYSTEM32\\CONFIG\\SYSTEM"));

		//
		// scan system hive
		//
		UNICODE_STRING AnsiCodePage						= {0,0,0};
		UNICODE_STRING OemCodePage						= {0,0,0};
		UNICODE_STRING LanguageTable					= {0,0,0};
		UNICODE_STRING OemFont							= {0,0,0};
		UNICODE_STRING InfFile							= {0,0,0};
		PCHAR Temp										= BlScanRegistry(BootFileSystem,UseLastKnownGood,&BlLoaderBlock->BootDriverListHead,&AnsiCodePage,&OemCodePage,
																		 &LanguageTable,&OemFont,&InfFile,BlLoaderBlock->SetupLoaderBlock,LoadSacDriver);
		//
		// scan registry failed
		//
		if(Temp)
			try_leave(strcpy(BadFile,LocalPath);strcat(BadFile,"SYSTEM");Status = EBADF);

		//
		// load nls data
		//
		strcpy(LocalPath,DirectoryPath);
		strcat(LocalPath,"\\system32\\");
		Status											= BlLoadNLSData(DeviceId,DeviceName,LocalPath,&AnsiCodePage,&OemCodePage,&LanguageTable,BadFile);
		if(Status != ESUCCESS)
			try_leave(NOTHING);

		//
		// load font
		//
		if(OemFont.Buffer)
		{
			//
			// on newer systems fonts are in the FONTS directory.
			//
			strcpy(LocalPath,DirectoryPath);
			strcat(LocalPath,"\\FONTS\\");
			Status										= BlLoadOemHalFont(DeviceId,DeviceName,LocalPath,&OemFont,BadFile);
			if(Status != ESUCCESS)
			{
				//
				// on older systems fonts are in the SYSTEM directory.
				//
				strcpy(LocalPath,DirectoryPath);
				strcat(LocalPath,"\\SYSTEM\\");
				Status									= BlLoadOemHalFont(DeviceId,DeviceName,LocalPath,&OemFont,BadFile);
			}
		}

		//
		// setup loader extension
		//
		if(!BlLoaderBlock->Extension || BlLoaderBlock->Extension->Size < sizeof(LOADER_PARAMETER_EXTENSION))
			try_leave(NOTHING);

		//
		// only for windows xp or later
		//
		PLOADER_PARAMETER_EXTENSION Extension			= BlLoaderBlock->Extension;
		if(Extension->MajorVersion < MAJOR_VERSION_NT5 || (Extension->MajorVersion == MAJOR_VERSION_NT5 && Extension->MinorVersion == MINOR_VERSION_2000))
			try_leave(NOTHING);

		//
		// load inf file
		//
		if(InfFile.Length)
		{
			strcpy(LocalPath,DirectoryPath);
			strcat(LocalPath,"\\inf\\");
			Status										= BlLoadFileImage(DeviceId,DeviceName,LocalPath,&InfFile,LoaderRegistryData,
																		  &Extension->InfFileImage,&Extension->InfFileSize,BadFile);
			if(Status != ESUCCESS)
				try_leave(NOTHING);
		}

		//
		// load drvmain.sdb,this may fail
		//
		UNICODE_STRING DrvMainSdbFile;
		RtlInitUnicodeString(&DrvMainSdbFile,L"drvmain.sdb");
		strcpy(LocalPath,DirectoryPath);
		strcat(LocalPath,"\\AppPatch\\");
		BlLoadFileImage(DeviceId,DeviceName,LocalPath,&DrvMainSdbFile,LoaderRegistryData,&Extension->DrvMainSdbFileImage,&Extension->DrvMainSdbFileSize,0);

		//
		// load acpitabl.dat
		//
		UNICODE_STRING AcpiTableDatFile;
		RtlInitUnicodeString(&AcpiTableDatFile,L"acpitabl.dat");
		strcpy(LocalPath,DirectoryPath);
		strcat(LocalPath,"\\system32\\");
		BlLoadFileImage(DeviceId,DeviceName,LocalPath,&AcpiTableDatFile,LoaderRegistryData,&Extension->AcpiTableDatFileImage,&Extension->AcpiTableDatFileSize,0);
	}
	__finally
	{
		//
		// set last known good flags
		//
		if(Status != ESUCCESS)
			*UseLastKnownGood								= FALSE;
	}

	return Status;
}

//
// load device driver
//
ARC_STATUS BlLoadDeviceDriver(__in PLDR_SCAN_IMPORT_SERACH_PATH_SET SearchPathSet,__in PCHAR FileName,__in ULONG Arg8,__in ULONG Flags,__out PLDR_DATA_TABLE_ENTRY* LdrEntry)
{
	CHAR LocalDllName[0x200];
	strcpy(LocalDllName,FileName);

	//
	// already loaded
	//
	if(BlCheckForLoadedDll(LocalDllName,LdrEntry))
		return ESUCCESS;

	//
	// for each search path,try to load the image file
	//
	ARC_STATUS Status									= EINVAL;
	for(ULONG i = 0; i < SearchPathSet->SearchPathCount; i ++)
	{
		CHAR FullPath[0x200];
		strcpy(FullPath,SearchPathSet->SearchPath[i].Path);
		strcat(FullPath,SearchPathSet->PrefixPath);
		strcat(FullPath,FileName);

		PVOID Base										= 0;
		Status											= BlLoadImageEx(SearchPathSet->SearchPath[i].DeviceId,LoaderBootDriver,FullPath,IMAGE_FILE_MACHINE_I386,0,0,&Base);
		if(Status != ESUCCESS)
			continue;

		//
		// output load message,and update boot status
		//
		BlOutputLoadMessage(SearchPathSet->SearchPath[i].DevicePath,FullPath,Arg8);
		BlUpdateBootStatus();

		//
		// allocate loader data entry
		//
		Status											= BlAllocateDataTableEntry(LocalDllName,FullPath,Base,LdrEntry);
		if(Status != ESUCCESS)
			return Status;

		//
		// setup flags
		//
		(*LdrEntry)->Flags								|= (Flags | LDRP_DRIVER_DEPENDENT_DLL);

		//
		// scan import table
		//
		Status											= BlScanImportDescriptorTable(SearchPathSet,*LdrEntry,LoaderHalCode);
		if(Status != ESUCCESS)
			RemoveEntryList(&(*LdrEntry)->InLoadOrderLinks);
		else
			(*LdrEntry)->Flags							&= ~LDRP_DRIVER_DEPENDENT_DLL;

		return Status;
	}

	return Status;
}

//
// load boot device driver
//
ARC_STATUS BlLoadBootDrivers(__in PLDR_SCAN_IMPORT_SERACH_PATH_SET LoadDevicePath,__in PLIST_ENTRY ListHead,__out PCHAR BadFileName)
{
	//
	// search path set
	//
	UCHAR SearchPathSetBuffer[sizeof(LDR_SCAN_IMPORT_SERACH_PATH_SET) + sizeof(LoadDevicePath->SearchPath[0]) * 2];
	PLDR_SCAN_IMPORT_SERACH_PATH_SET SearchPathSet		= reinterpret_cast<PLDR_SCAN_IMPORT_SERACH_PATH_SET>(SearchPathSetBuffer);
	ARC_STATUS Status									= ESUCCESS;

	//
	// for each item in the list,load its image file
	//
	for(PLIST_ENTRY NextEntry = ListHead->Flink; NextEntry != ListHead; NextEntry = NextEntry->Flink)
	{
		//
		// get driver node
		//
		PBOOT_DRIVER_NODE DriverNode					= CONTAINING_RECORD(NextEntry,BOOT_DRIVER_NODE,ListEntry.Link);

		CHAR DevicePath[0x80];
		CHAR FilePath[0x80];
		BOOLEAN AbsolutePath							= FALSE;
		ULONG DeviceId									= 0;

		//
		// file path start with a \\ ?
		//
		if(DriverNode->ListEntry.FilePath.Buffer[0] == L'\\')
		{
			if(!wcsncmp(DriverNode->ListEntry.FilePath.Buffer,L"\\SystemRoot\\",sizeof(L"\\SystemRoot\\") / sizeof(WCHAR) - 1))
			{
				//
				// the file path start with \\SystemRoot\\,it isn't an absolute path
				//
				UNICODE_STRING Temp;
				Temp.Length								= static_cast<USHORT>(DriverNode->ListEntry.FilePath.Length - sizeof(L"\\SystemRoot\\") + sizeof(WCHAR));
				Temp.Buffer								= Add2Ptr(DriverNode->ListEntry.FilePath.Buffer,sizeof(L"\\SystemRoot\\") - sizeof(WCHAR),PWCHAR);
				Temp.MaximumLength						= Temp.Length;

				_snprintf(FilePath,ARRAYSIZE(FilePath),"%wZ",&Temp);
			}
			else
			{
				//
				// this is an absolute path
				//
				AbsolutePath							= TRUE;

				//
				// get device path and open it
				// skip the first \\
				//
				UNICODE_STRING Temp;
				Temp.Buffer								= DriverNode->ListEntry.FilePath.Buffer + 1;
				Temp.Length								= 0;
				Temp.MaximumLength						= DriverNode->ListEntry.FilePath.Length - sizeof(WCHAR);

				//
				// find the next \\
				//
				for(ULONG i = 0; Temp.Buffer[i] != L'\\' && i < Temp.MaximumLength / sizeof(WCHAR); i ++)
					Temp.Length							+= sizeof(WCHAR);

				Temp.MaximumLength						= Temp.Length;

				//
				// now,temp is the arc device name
				//
				_snprintf(DevicePath,ARRAYSIZE(DevicePath),"%wZ",&Temp);

				//
				// open the device
				//
				Status									= ArcOpen(DevicePath,ArcOpenReadOnly,&DeviceId);

				//
				// left part is file path
				//
				Temp.Buffer								= Add2Ptr(Temp.Buffer,Temp.Length + sizeof(WCHAR),PWCHAR);
				Temp.Length								= DriverNode->ListEntry.FilePath.Length - sizeof(WCHAR) - Temp.Length - sizeof(WCHAR);
				Temp.MaximumLength						= Temp.Length;

				_snprintf(FilePath,ARRAYSIZE(FilePath),"%wZ",&Temp);
			}
		}
		else
		{
			//
			// this is a relative path,convert it to ansi encoding
			//
			_snprintf(FilePath,ARRAYSIZE(FilePath),"%wZ",&DriverNode->ListEntry.FilePath);
		}

		//
		// terminate file path with NULL
		//
		FilePath[ARRAYSIZE(FilePath) - 1]				= 0;

		//
		// split file path into directory and file name
		//
		CHAR FileName[64];
		PCHAR LastSep									= strrchr(FilePath,'\\');
		if(!LastSep)
		{
			//
			// file path is an empty string
			//
			if(!FilePath[0])
				continue;

			//
			// the file is in the root directory
			//
			strncpy(FileName,FilePath,ARRAYSIZE(FileName));

			//
			// make sure file name is terminated by NULL
			//
			FileName[ARRAYSIZE(FileName) - 1]			= 0;

			//
			// directory is empty
			//
			FilePath[0]									= 0;
		}
		else
		{
			//
			// copy file name
			//
			strncpy(FileName,LastSep + 1,ARRAYSIZE(FileName));

			//
			// make sure file name is terminated by NULL
			//
			FileName[ARRAYSIZE(FileName) - 1]			= 0;

			//
			// terminate directory
			//
			*LastSep									= 0;
		}

		//
		// append a sep to the FilePath
		//
		if(FilePath[0])
			strncat(FilePath,"\\",ARRAYSIZE(FilePath) - strlen(FilePath));

		//
		// make sure it is terminated by NULL
		//
		FilePath[ARRAYSIZE(FilePath) - 1]				= 0;

		//
		// if the driver image path is an absolute path,the search path just have only one entry specified by the path
		// otherwise,use the caller's search path
		//
		if(AbsolutePath)
		{
			SearchPathSet->SearchPathCount				= 1;
			SearchPathSet->SystemRootPath				= 0;
			SearchPathSet->SearchPath[0].DeviceId		= DeviceId;
			SearchPathSet->SearchPath[0].Path			= "\\";
			SearchPathSet->SearchPath[0].DevicePath		= DevicePath;

			strncpy(SearchPathSet->PrefixPath,FilePath,ARRAYSIZE(SearchPathSet->PrefixPath));
		}
		else
		{
			//
			// copy search path
			//
			ULONG CopyLength							= FIELD_OFFSET(LDR_SCAN_IMPORT_SERACH_PATH_SET,SearchPath);
			CopyLength									+= sizeof(LoadDevicePath->SearchPath[0]) * LoadDevicePath->SearchPathCount;
			RtlCopyMemory(SearchPathSet,LoadDevicePath,CopyLength);

			//
			// append file path
			//
			strncat(SearchPathSet->PrefixPath,FilePath,ARRAYSIZE(SearchPathSet->PrefixPath) - strlen(SearchPathSet->PrefixPath));
		}

		//
		// terminate prefix path
		//
		ULONG const temp								= ARRAYSIZE(SearchPathSet->PrefixPath) - 1;
		SearchPathSet->PrefixPath[temp]					= 0;

		//
		// load it
		//
		if(Status == ESUCCESS)
			Status										= BlLoadDeviceDriver(SearchPathSet,FileName,FALSE,LDRP_ENTRY_PROCESSED,&DriverNode->ListEntry.LdrEntry);

		//
		// close arc device
		// windows sp2's osloader.exe did not close the device handle
		//
		if(AbsolutePath && DeviceId)
			ArcCacheClose(DeviceId);

		//
		// continue to process the next one
		//
		if(Status == ESUCCESS)
			continue;

		//
		// load current driver failed,remove it from the list
		//
		RemoveEntryList(&DriverNode->ListEntry.Link);

		//
		// if the error control is SERVICE_ERROR_CRITICAL,break the loop and set current entry as bad file
		//
		if(DriverNode->ErrorControl != SERVICE_ERROR_CRITICAL)
			continue;

		if(strlen(FileName) + strlen(FilePath) < 0x7f)
		{
			strcpy(BadFileName,FilePath);
			strcat(BadFileName,FileName);
		}

		break;
	}

	return Status;
}

//
// add boot driver
//
ARC_STATUS BlAddToBootDriverList(__in PLIST_ENTRY ListHead,__in PWCHAR FileName,__in PWCHAR RegistryName,__in PWCHAR Group,
								 __in ULONG Tag,__in ULONG ErrorControl,__in BOOLEAN InsertHeadOrTail)
{
	//
	// allocate a boot driver node
	//
	PBOOT_DRIVER_NODE BootDriverNode					= static_cast<PBOOT_DRIVER_NODE>(BlAllocateHeap(sizeof(BOOT_DRIVER_NODE)));
	if(!BootDriverNode)
		return ENOMEM;

	//
	// allocate file name buffer
	//
	ULONG Length										= wcslen(FileName) * sizeof(WCHAR) + sizeof(L"System32\\Drivers\\");
	BootDriverNode->ListEntry.FilePath.Buffer			= static_cast<PWCHAR>(BlAllocateHeap(Length));
	if(!BootDriverNode->ListEntry.FilePath.Buffer)
		return ENOMEM;

	//
	// set file path
	//
	BootDriverNode->ListEntry.FilePath.MaximumLength	= static_cast<USHORT>(Length);
	BootDriverNode->ListEntry.FilePath.Length			= 0;
	RtlAppendUnicodeToString(&BootDriverNode->ListEntry.FilePath,L"System32\\Drivers\\");
	RtlAppendUnicodeToString(&BootDriverNode->ListEntry.FilePath,FileName);

	//
	// allocate registry buffer
	//
	Length												= wcslen(RegistryName) * sizeof(WCHAR) + sizeof(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
	BootDriverNode->ListEntry.RegistryPath.Buffer		= static_cast<PWCHAR>(BlAllocateHeap(Length));
	if(!BootDriverNode->ListEntry.RegistryPath.Buffer)
		return ENOMEM;

	//
	// set registry path
	//
	BootDriverNode->ListEntry.RegistryPath.MaximumLength= static_cast<USHORT>(Length);
	BootDriverNode->ListEntry.RegistryPath.Length		= 0;
	RtlAppendUnicodeToString(&BootDriverNode->ListEntry.RegistryPath,L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
	RtlAppendUnicodeToString(&BootDriverNode->ListEntry.RegistryPath,RegistryName);

	//
	// allocate group buffer
	//
	Length												= wcslen(Group) * sizeof(WCHAR) + sizeof(WCHAR);
	BootDriverNode->Group.Buffer						= static_cast<PWCHAR>(BlAllocateHeap(Length));
	if(!BootDriverNode->Group.Buffer)
		return ENOMEM;

	//
	// set group
	//
	BootDriverNode->Group.MaximumLength					= static_cast<USHORT>(Length);
	BootDriverNode->Group.Length						= 0;
	RtlAppendUnicodeToString(&BootDriverNode->Group,Group);

	//
	// allocate name buffer
	//
	Length												= wcslen(RegistryName) * sizeof(WCHAR) + sizeof(WCHAR);
	BootDriverNode->Name.Buffer							= static_cast<PWCHAR>(BlAllocateHeap(Length));
	if(!BootDriverNode->Name.Buffer)
		return ENOMEM;

	//
	// set name
	//
	BootDriverNode->Name.MaximumLength					= static_cast<USHORT>(Length);
	BootDriverNode->Name.Length							= 0;
	RtlAppendUnicodeToString(&BootDriverNode->Name,RegistryName);

	//
	// set tag and error control
	//
	BootDriverNode->Tag									= Tag;
	BootDriverNode->ErrorControl						= ErrorControl;

	//
	// link to list head
	//
	if(InsertHeadOrTail)
		InsertHeadList(ListHead,&BootDriverNode->ListEntry.Link);
	else
		InsertTailList(ListHead,&BootDriverNode->ListEntry.Link);

	return ESUCCESS;
}