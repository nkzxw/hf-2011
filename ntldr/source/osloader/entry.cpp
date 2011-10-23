//********************************************************************
//	created:	10:8:2008   17:34
//	file:		entry.cpp
//	author:		tiamo
//	purpose:	startup
//********************************************************************

#include "stdafx.h"

//
// nt build number
//
ULONG NtBuildNumber										= 0xc0000000 | BUILD_NUMBER;

//
// su proviced service table pointers
//
PEXTERNAL_SERVICES_TABLE								ExternalServicesTable;

//
// machine type
//
ULONG													MachineType;

//
// osloader start address
//
ULONG													OsLoaderStart;

//
// osloader end address
//
ULONG													OsLoaderEnd;

//
// osloader's resource directory
//
ULONG													BlpResourceDirectory;

//
// osloader's resource offset
//
ULONG													BlpResourceFileOffset;

//
// osloader's base address
//
ULONG													OsLoaderBase;

//
// osloader's export table
//
ULONG													OsLoaderExports;

//
// boot flags
//
ULONG													BootFlags;

//
// ntdetect start
//
ULONG													NtDetectStart;

//
// ntdetect end
//
ULONG													NtDetectEnd;

//
// ramdisk address
//
ULONG													SdiAddress;

//
// boot from net
//
BOOLEAN													BlBootingFromNet;

//
// boot from cd
//
BOOLEAN													ElToritoCDBoot;

//
// net boot path
//
CHAR													NetBootPath[0x100];

//
// temp buffer
//
CHAR													MyBuffer[544];

//
// force boot last known good config
//
BOOLEAN													ForceLastKnownGood;

//
// net boot ini path
//
CHAR													NetBootIniPath[260];

//
// net boot ini content
//
CHAR													NetBootIniContents[1024];

//
// kernel boot device
//
CHAR													KernelBootDevice[96];

//
// boot partition name
//
CHAR													BootPartitionName[80];

//
// os load file name
//
CHAR													OsLoadFilename[200];

//
// loader file name
//
CHAR													OsLoaderFilename[200];

//
// system partition
//
CHAR													SystemPartition[200];

//
// os load partition
//
CHAR													OsLoadPartition[200];

//
// load options
//
CHAR													OsLoadOptions[200];

//
// input console device name
//
CHAR													ConsoleInputName[64];

//
// output console device name
//
CHAR													ConsoleOutputName[64];

//
// x86 system partition
//
CHAR													X86SystemPartition[200];

//
// command line from su module
//
CHAR													BlSuCmdLine[0x101];

//
// loads and runs NTDETECT.COM to populate the ARC configuration tree.
//
BOOLEAN BlDetectHardware(__in ULONG DriveId,__in PCHAR LoadOptions)
{
	ARC_STATUS Status									= ESUCCESS;
	PUCHAR DetectionBuffer								= reinterpret_cast<PUCHAR>(0x10000);
	ULONG DetectFileId									= 0;
	BOOLEAN Ret											= FALSE;
	CHAR Buffer[0x200];

	__try
	{
		//
		// su module has already load it into memory
		//
		if(NtDetectStart)
			try_leave(RtlCopyMemory(DetectionBuffer,reinterpret_cast<PVOID>(NtDetectStart),NtDetectEnd - NtDetectStart));

		//
		// load the file
		//
		PCHAR NtDetectPatch								= "\\ntdetect.com";
		if(ElToritoCDBoot)
		{
			//
			// booting from cd
			// we assume ntdetect.com is in the i386 directory
			//
			NtDetectPatch								= "\\i386\\ntdetect.com";
		}
		else if(BlBootingFromNet)
		{
			//
			// booting from net
			//
			strcpy(Buffer,NetBootPath);
			strcat(Buffer,"ntdetect.com");
			NtDetectPatch								= Buffer;
		}

		//
		// open it
		//
		Status											= BlOpen(DriveId,NtDetectPatch,ArcOpenReadOnly,&DetectFileId);
		if(Status != ESUCCESS)
			try_leave(BlPrint("Error opening NTDETECT.COM, status = %x\r\n",Status));

		//
		// get file size
		//
		FILE_INFORMATION FileInformation;
		Status											= BlGetFileInformation(DetectFileId,&FileInformation);
		if(Status != ESUCCESS)
			try_leave(BlPrint("Error getting NTDETECT.COM file information, status = %x\r\n",Status));

		//
		// check zero-length file
		//
		ULONG FileSize									= FileInformation.EndingAddress.LowPart;
		if(!FileSize)
			try_leave(BlPrint("Error: size of NTDETECT.COM is zero.\r\n");Status = ENOMEM);

		//
		// seek to beginning
		//
		LARGE_INTEGER Offset;
		Offset.QuadPart									= 0;
		Status											= BlSeek(DetectFileId,&Offset,SeekAbsolute);
		if(Status != ESUCCESS)
			try_leave(BlPrint("Error seeking to start of NTDETECT.COM file, status = %x\r\n",Status));

		//
		// read it
		//
		ULONG Read;
		Status											= BlRead(DetectFileId,DetectionBuffer,FileSize,&Read);
		if(Status != ESUCCESS)
			try_leave(BlPrint("Error reading from NTDETECT.COM\r\n, status = %x,read = %lx\r\n",Status,Read));
	}
	__finally
	{
		//
		// close ntdetect.com
		//
		if(DetectFileId)
			BlClose(DetectFileId);

		if(Status == ESUCCESS)
		{
			//
			// we need to pass NTDETECT pointers < 1Mb
			// try to use local storage off the stack first (which is always < 1Mb).
			// if the options is too big,we steal some space from heap to pass it
			//
			PCHAR Options								= 0;
			ULONG Heap									= /*TEMPORARY_HEAP_START*/ 0x50 * PAGE_SIZE;
			ULONG HeapSize								= /*(OSLOADER_STACK_START - TEMPORARY_HEAP_START)*/ 0x10 * PAGE_SIZE;

			if(LoadOptions)
			{
				ULONG Length							= strlen(LoadOptions);

				//
				// we can use our local buffer,if the whole options can fit into it
				//
				if(Length + sizeof(" NOLEGACY") < ARRAYSIZE(Buffer))
				{
					Options								= Buffer;
				}
				else
				{
					//
					// otherwise we use some heap space
					//
					HeapSize							-= Length + sizeof(" NOLEGACY");
					Options								= reinterpret_cast<PCHAR>(Heap);
					Heap								+= sizeof(" NOLEGACY");
				}

				//
				// copy it
				//
				strcpy(Options,LoadOptions);
			}

			//
			// check legacy free bios
			//
			if(BlDetectLegacyFreeBios())
			{
				PCHAR Append							= Buffer;

				if(!Options)
					Options								= Buffer;
				else
					Append								+= strlen(Options);

				RtlCopyMemory(Options," NOLEGACY",sizeof(" NOLEGACY"));
			}

			//
			// transfer control to ntdetect.com
			//
			PVOID TempFwTree							= 0;
			ULONG Dummy									= 0;
			ULONG OptionsLength							= Options ? strlen(Options) : 0;
			ExternalServicesTable->DetectHardware(Heap,HeapSize,&TempFwTree,&Dummy,Options,OptionsLength);

			FwConfigurationTree							= static_cast<PCONFIGURATION_COMPONENT_DATA>(TempFwTree);

			extern ARC_STATUS BlpMarkExtendedVideoRegionOffLimits();
			Ret											= BlpMarkExtendedVideoRegionOffLimits() == ESUCCESS;
		}
	}

	return Ret;
}

//
// startup
//
VOID BlStartup(__in PCHAR PartitionName)
{
	__try
	{
	#if _RAMDISK_SUPPORT_
		if(!strncmp(PartitionName,"ramdisk(0)",10))
			RamdiskInitialize(0,TRUE);
	#endif

		//
		// open the boot partition so we can load boot drivers off it.
		//
		ULONG DriveId									= 0;
		ARC_STATUS Status								= ArcOpen(PartitionName,ArcOpenReadOnly,&DriveId);;

		if(Status != ESUCCESS)
			try_leave(BlPrint("Couldn't open drive %s\r\n",PartitionName);BlPrint(BlFindMessage(BL_DRIVE_ERROR),PartitionName));

		//
		// initialize dbcs font and display support.
		//
		TextGrInitialize(DriveId,0);

		//
		// initialize stdio
		//
		PCHAR Argv[10];
		strcpy(ConsoleInputName,"consolein=multi(0)key(0)keyboard(0)");
		strcpy(ConsoleOutputName,"consoleout=multi(0)video(0)monitor(0)");
		Argv[0]											= ConsoleInputName;
		Argv[1]											= ConsoleOutputName;
		BlInitStdio(2,Argv);

	#if _HIBERNATE_SUPPORT_
		//
		// retore from hiberfil.sys
		//
		PCHAR LinkedRestorePath							= 0;
		BlHiberRestore(DriveId,&LinkedRestorePath);
	#endif

		//
		// close it
		//
		ArcCacheClose(DriveId);

		//
		// reopen it with read/write access
		//
		Status												= ArcOpen(PartitionName,ArcOpenReadWrite,&DriveId);
		if(Status != ESUCCESS)
			try_leave(BlPrint("Couldn't open drive %s\r\n",PartitionName);BlPrint(BlFindMessage(BL_DRIVE_ERROR),PartitionName));

	#if (_HIBERNATE_SUPPORT_ && _SCSI_SUPPORT_)
		//
		// we are restoring from a scsi device
		//
		if(LinkedRestorePath && (!strnicmp(LinkedRestorePath,"scsi(",5) || !strnicmp(LinkedRestorePath,"signature(",10)))
		{
			//
			// fast detect hardware,and open the restore device
			//
			ULONG RestoreDriveId;
			if(BlDetectHardware(DriveId,"/fastdetect") && AEInitializeIo(DriveId) == ESUCCESS && ArcOpen(LinkedRestorePath,ArcOpenReadWrite,&RestoreDriveId) == ESUCCESS)
			{
				//
				// restore it
				//
				BlHiberRestore(RestoreDriveId,0);
				ArcCacheClose(RestoreDriveId);
			}
		}
	#endif

		BOOLEAN UseTimeOut								= TRUE;
		BOOLEAN AlreadyInitialized						= FALSE;
		while(1)
		{
			//
			// open boot.ini and read it in
			//
			PCHAR BootIniPath							= "\\boot.ini";
			PCHAR IniFileContents						= 0;
			if(BlBootingFromNet)
			{
				if(NetBootIniContents[0])
					IniFileContents						= NetBootIniContents;

				if(NetBootIniPath[0])
				{
					BootIniPath							= NetBootIniPath;
				}
				else
				{
					strcpy(MyBuffer,NetBootPath);
					strcat(MyBuffer,"boot.ini");
					BootIniPath							= MyBuffer;
				}
			}

			ULONG BootIniFileId							= 0;
			ULONG Count									= 0;
			if(!IniFileContents)
			{
				//
				// open boot.ini
				//
				Status									= BlOpen(DriveId,BootIniPath,ArcOpenReadOnly,&BootIniFileId);
				RtlZeroMemory(MyBuffer,sizeof(MyBuffer));

				//
				// opened?
				//
				if(Status == ESUCCESS)
				{
					//
					// get total length
					//
					ULONG TotalLength					= 0;

					do
					{
						Status							= BlRead(BootIniFileId,MyBuffer,0x200,&Count);
						if(Status != ESUCCESS)
							try_leave(BlClose(BootIniFileId);BlPrint(BlFindMessage(BL_READ_ERROR),Status);MyBuffer[0] = 0);

						TotalLength						+= Count;
					}while(Count);

					//
					// too big,allocate a buffer
					//
					PCHAR Buffer						= MyBuffer;
					if(TotalLength >= 0x200)
						Buffer							= static_cast<PCHAR>(FwAllocateHeap(TotalLength));

					if(!Buffer)
						try_leave(BlClose(BootIniFileId);Status = ENOMEM;BlPrint(BlFindMessage(BL_READ_ERROR),ENOMEM);MyBuffer[0] = 0);

					//
					// seek to beginning
					//
					LARGE_INTEGER Offset;
					Offset.QuadPart						= 0;
					Status								= BlSeek(BootIniFileId,&Offset,SeekAbsolute);
					if(Status != ESUCCESS)
						try_leave(BlClose(BootIniFileId);BlPrint(BlFindMessage(BL_READ_ERROR),Status);MyBuffer[0] = 0);

					//
					// read it
					//
					BlRead(BootIniFileId,Buffer,TotalLength,&Count);

					//
					// what is this?
					//
					Offset.QuadPart						= 0;
					Status								= BlSeek(BootIniFileId,&Offset,SeekAbsolute);
					if(Status != ESUCCESS)
						try_leave(BlClose(BootIniFileId);BlPrint(BlFindMessage(BL_READ_ERROR),Status);MyBuffer[0] = 0);

					//
					// search for ctrl+z(0x1a)
					//
					Buffer[Count]						= 0;
					for(ULONG i = 0; Buffer[i]; i ++)
					{
						if(Buffer[i] == 0x1a)
						{
							Buffer[i]					= 0;
							break;
						}
					}

					IniFileContents						= Buffer;

					BlClose(BootIniFileId);
				}
			}

			//
			// stop floppy
			//
			MdShutoffFloppy();

			//
			// clear screen
			//
			ArcWrite(BlConsoleOutDeviceId,"\x1B[2J",4,&Count);

			//
			// if we are booting from net,we must have a valid boot.ini
			//
			if(BlBootingFromNet && !IniFileContents)
				try_leave(BlClose(BootIniFileId);BlPrint(BlFindMessage(BL_READ_ERROR),Status);MyBuffer[0] = 0);

			//
			// show the select kernel dialog
			//
			PCHAR LoadOptions							= 0;
			PCHAR BootDeviceName						= BlSelectKernel(DriveId,IniFileContents,&LoadOptions,UseTimeOut);
			if(BootDeviceName)
			{
				//
				// already detect hardware?
				//
				if(!AlreadyInitialized)
				{
					//
					// detect hardware
					//
					if(!BlDetectHardware(DriveId,LoadOptions))
						try_leave(BlPrint(BlFindMessage(BL_NTDETECT_FAILURE)));

					//
					// clear screen
					//
					ArcWrite(BlConsoleOutDeviceId,"\x1B[2J",4,&Count);

				#if (_HIBERNATE_SUPPORT_ && _SCSI_SUPPORT_)
					//
					// load scsi driver
					//
					if(!strnicmp(LinkedRestorePath,"scsi(",5) || !strnicmp(LinkedRestorePath,"signature(",10))
						AEInitializeIo(DriveId);
				#endif

					//
					// close boot partition
					//
					ArcCacheClose(DriveId);

					//
					// from now on,no one can change firmware's permanent heap,because we also use it to save config tree
					//
					FwDescriptorsValid					= FALSE;
				}
				else
				{
					//
					// clear screen
					//
					ArcWrite(BlConsoleOutDeviceId,"\x1B[2J",4,&Count);
				}

				//
				// already load scsi driver
				//
				AlreadyInitialized						= TRUE;

				//
				// for the subseqents reboot,the menu will always be there
				//
				UseTimeOut								= FALSE;

				//
				// set boot device
				//
				for(Count = 0; *BootDeviceName != '\\'; Count ++)
					KernelBootDevice[Count]				= *BootDeviceName ++;

				//
				// terminate it with NULL
				//
				KernelBootDevice[Count]					= 0;

				//
				// get advanced boot option
				//
				if(BlGetAdvancedBootOption() == 0xffffffff)
				{
					//
					// check last boot status
					//
					ULONG BootStatusFileId;
					ULONG LastStatus					= 1;
					if(BlLockBootStatusData(0,KernelBootDevice,BootDeviceName,&BootStatusFileId) == ESUCCESS)
					{
						ULONG DefaultAdvBootIndex		= BlGetLastBootStatus(BootStatusFileId,&LastStatus);

						//
						// last boot failed,disable verifier
						//
						extern BOOLEAN BlDisableVerifier;
						if(DefaultAdvBootIndex != 0xffffffff && (LastStatus == 2 || LastStatus == 3))
							BlDisableVerifier			= TRUE;

						if(LastStatus == 2)
						{
							if(DefaultAdvBootIndex != 0xffffffff)
							{
								UCHAR Timeout			= 0;
								if(BlGetSetBootStatusData(BootStatusFileId,TRUE,3,&Timeout,sizeof(Timeout),0) != ESUCCESS)
									Timeout				= 0x1e;

								//
								// do advanced boot
								//
								DefaultAdvBootIndex		= BlDoAdvancedBoot(BL_ADVANCED_BOOT_START_FAILED,0xffffffff,1,Timeout);
							}

							BlAutoAdvancedBoot(&LoadOptions,1,DefaultAdvBootIndex);
						}

						//
						// unlock boot status data
						//
						BlUnlockBootStatusData(BootStatusFileId);
					}
				}

				//
				// setup osloader's argv
				//
				strcpy(OsLoaderFilename,"osloader=");
				strcat(OsLoaderFilename,BootDeviceName);
				strcat(OsLoaderFilename,"\\System32\\NTLDR");
				strcpy(SystemPartition,"systempartition=");
				strcat(SystemPartition,KernelBootDevice);
				strcpy(OsLoadPartition,"osloadpartition=");
				strcat(OsLoadPartition,KernelBootDevice);
				strcpy(OsLoadFilename,"osloadfilename=");
				strcat(OsLoadFilename,BootDeviceName);
				strcpy(OsLoadOptions,"osloadoptions=");
				strcpy(ConsoleInputName,"consolein=multi(0)key(0)keyboard(0)");
				strcpy(ConsoleOutputName,"consoleout=multi(0)video(0)monitor(0)");
				strcpy(X86SystemPartition,"x86systempartition=");
				strcat(X86SystemPartition,PartitionName);

				if(LoadOptions)
					strcat(OsLoadOptions,LoadOptions);

				Argv[0]									= "load";
				Argv[1]									= OsLoaderFilename;
				Argv[2]									= SystemPartition;
				Argv[3]									= OsLoadFilename;
				Argv[4]									= OsLoadPartition;
				Argv[5]									= OsLoadOptions;
				Argv[6]									= ConsoleInputName;
				Argv[7]									= ConsoleOutputName;
				Argv[8]									= X86SystemPartition;

				//
				// call osloader
				//
				Status									= BlOsLoader(9,Argv,0);
			}
			else
			{
				BlPrint(BlFindMessage(BL_INVALID_BOOT_INI2));
			}

			ForceLastKnownGood							= FALSE;
			if(Status != ESUCCESS)
				try_leave(NOTHING);

			//
			// reopen boot partiton
			//
			Status										= ArcOpen(BootPartitionName,ArcOpenReadOnly,&DriveId);
			if(Status != ESUCCESS)
				try_leave(BlPrint("Couldn't open drive %s\r\n",BootPartitionName);BlPrint(BlFindMessage(BL_DRIVE_ERROR),BootPartitionName));
		}
	}
	__finally
	{

	}
}

//
// determine the ARC name for the partition NTLDR was started from
//
VOID BlGetActivePartition(__out PCHAR BootPartitionName,__in ULONG BootDrive,__in ULONG Partition)
{
	//
	// if caller gave us a partition number,try to open it
	// use it,if it can be opened,otherwise searh for the partition which is loaded into 0x7c00
	// the method we use is to open each partition on the drive and read in its boot sector
	// then we compare that to the boot sector that we used to boot from (at physical address 0x7c00)
	// if they are the same, we've found it,if we run out of partitions, just try partition 1
	//
	ULONG CurrentPartition								= Partition ? Partition : 1;

	while(TRUE)
	{
		CHAR NameBuffer[80];
		sprintf(NameBuffer,"multi(0)disk(0)rdisk(%u)partition(%u)",BootDrive & 0x7f,CurrentPartition);

		ULONG FileId;
		ARC_STATUS Status								= ArcOpen(NameBuffer,ArcOpenReadOnly,&FileId);
		if(Status != ESUCCESS)
		{
			if(CurrentPartition == Partition)
			{
				//
				// input partition is not valid,try to find one
				//
				CurrentPartition						= 1;
				Partition								= 0;

				continue;
			}

			//
			// we've run out of partitions, return the default.
			//
			CurrentPartition							= 1;
			break;
		}

		//
		// input partition is valid,use it
		//
		if(Partition == CurrentPartition)
			break;

		//
		// read in the first 512 bytes
		//
		UCHAR SectorBuffer[512];
		ULONG Count;
		Status											= ArcRead(FileId,SectorBuffer,512,&Count);
		ArcCacheClose(FileId);

		if(Status == ESUCCESS)
		{
			//
			// only need to compare the first 36 bytes
			//    Jump instr. == 3 bytes
			//    Oem field   == 8 bytes
			//    BPB         == 25 bytes
			//
			if(!memcmp(SectorBuffer,Add2Ptr(0x7c00,0,PVOID),36))
				break;
		}

		CurrentPartition								+= 1;
	}

	sprintf(BootPartitionName, "multi(0)disk(0)rdisk(%u)partition(%u)",BootDrive & 0x7f,CurrentPartition);
}

//
// check ElToritoCD
//
BOOLEAN BlIsElToritoCDBoot(__in ULONG DriveNum)
{
	// Note, even though args are short, they are pushed on the stack with 32bit alignment
	// so the effect on the stack seen by the 16bit real mode code is the same as if we were pushing longs here.
	//
	if(DriveNum <= 0x81)
		return FALSE;

	//
	// SU_GET_ELTORITO_STATUS is 0 if we are in emulation mode
	//
	return !ExternalServicesTable->GetElToritoStatus(FwDiskCache,DriveNum);
}

//
// global initialization
//
VOID DoGlobalInitialization(__in PBOOT_CONTEXT BootContextRecord)
{
	//
	// save boot context record
	//
	ExternalServicesTable								= BootContextRecord->ExternalServicesTable;
	MachineType											= BootContextRecord->MachineType;
	BlpResourceDirectory								= BootContextRecord->ResourceDirectory;
	BlpResourceFileOffset								= BootContextRecord->ResourceOffset;
	OsLoaderBase										= BootContextRecord->OsLoaderBase;
	OsLoaderExports										= BootContextRecord->OsLoaderExports;
	NtDetectEnd											= BootContextRecord->NtDetectEnd;
	NtDetectStart										= BootContextRecord->NtDetectStart;

	//
	// if we are booting from ramdisk save ramdisk address
	//
	if(BootContextRecord->FSContextPointer->BootDrive == 0x41)
		SdiAddress										= BootContextRecord->SdiAddress;

	//
	// initialize memory subsytem
	//
	ARC_STATUS Status									= InitializeMemorySubsystem(BootContextRecord);
	if(Status != ESUCCESS)
	{
		BlPrint("InitializeMemory failed %lx\r\n",Status);
		while(1);
	}

	//
	// turn the cursor off - move cursor to (127,0)
	//
	ExternalServicesTable->HardwareCursor(0,127);

	InitializeMemoryDescriptors();
}

//
// parse command line
//
VOID ParseCommandLine(__in PCHAR CommandLine,__in PBOOT_CONTEXT BootRecord)
{
	//
	// parse root device
	//
	PCHAR RootDevice									= strstr(CommandLine,"ROOT=/DEV/");
	if(RootDevice)
	{
		//
		// skip ROOT=/DEV/,and check HDx or SDx
		//
		RootDevice										+= 10;
		if(!strncmp(RootDevice,"HD",2) || !strncmp(RootDevice,"SD",2))
		{
			//
			// skip HD,SD
			//
			RootDevice									+= 2;

			//
			// get disk
			//
			if(*RootDevice < 'A' || *RootDevice > 'Z')
				return;

			ULONG Disk									= *RootDevice - 'A';

			//
			// get partition
			//
			RootDevice									+= 1;
			if(*RootDevice < '1' || *RootDevice > '9')
				return;

			ULONG Partition								= atoi(RootDevice);

			//
			// partition should not be zero
			//
			if(!Partition || Partition > 0xff)
				return;

			//
			// set boot record
			//
			BootRecord->FSContextPointer->BootDrive		= 0x80 | Disk;
			BootRecord->FSContextPointer->Partition		= static_cast<UCHAR>(Partition);
		}
		else if(!strncmp(RootDevice,"NET",2))
		{
			BootRecord->FSContextPointer->BootDrive		= 0x40;
			BootRecord->FSContextPointer->Partition		= 0;
		}
		else if(!strncmp(RootDevice,"RAM",3))
		{
			BootRecord->FSContextPointer->BootDrive		= 0x41;
			BootRecord->FSContextPointer->Partition		= 0;
		}
		else if(!strncmp(RootDevice,"FD",2))
		{
			RootDevice									+= 2;
			ULONG Floppy								= *RootDevice - '0';
			if(Floppy == 0 || Floppy == 1)
				BootRecord->FSContextPointer->BootDrive	= Floppy;

			BootRecord->FSContextPointer->Partition		= 0;
		}
	}

	//
	// force int13 extension supported
	//
	extern BOOLEAN ForceInt13Ext;
	if(BootRecord->FSContextPointer->BootDrive >= 0x80)
		ForceInt13Ext									= strstr(BlSuCmdLine,"FORCELBA") == 0 ? FALSE : TRUE;

	//
	// parse redirect config
	//
	PCHAR PortString									= strstr(CommandLine,"REDIRECTPORT");
	if(PortString)
	{
		//
		// skip REDIRECTPORT=
		//
		PortString										+= 13;

		//
		// check debug port type
		//
		if(!strncmp(PortString,"COM",3))
		{
			//
			// get port number
			//
			LoaderRedirectionInfo.PortNum				= atol(PortString + 3);

			//
			// get baudrate
			//
			PCHAR BaudrateString						= strstr(CommandLine,"REDIRECTBAUDRATE");
			if(BaudrateString)
				LoaderRedirectionInfo.Baudrate			= atol(BaudrateString + 17);
		}
	}

	//
	// auto reboot flags
	//
	if(strstr(CommandLine,"AUTOREBOOT"))
		BootRecord->BootFlags							|= 1;
}

//
// entry pointer,su module will call it with page disabled,pe enabled,interrupt off
//
VOID NtProcessStartup(__in PBOOT_CONTEXT BootContextRecord)
{
	//
	// save boot context record and initialize memory system
	//
	DoGlobalInitialization(BootContextRecord);

	//
	// parse command line
	//
	ParseCommandLine(BlSuCmdLine,BootContextRecord);

	//
	// setup global firmware vectors
	//
	BlFillInSystemParameters(BootContextRecord);

	//
	// save boot flags
	//
	BootFlags											= BootContextRecord->BootFlags;

	//
	// get boot partition
	//
	if(BootContextRecord->FSContextPointer->BootDrive == 0x00 || BootContextRecord->FSContextPointer->BootDrive == 0x01)
	{
		//
		// booting from A:,B:
		//
		sprintf(BootPartitionName,"multi(0)disk(0)fdisk(%u)",BootContextRecord->FSContextPointer->BootDrive);

		//
		// to get around an apparent bug on the BIOS of some MCA machines (specifically the NCR 386sx/MC20 w/ BIOS version 1.04.00 (3421), Phoenix BIOS 1.02.07),
		// whereby the first int13 to floppy results in a garbage buffer, reset drive 0 here.
		//
		ExternalServicesTable->DiskIOSystem(0,0,0,0,0,0,0);
	}
	else if(BootContextRecord->FSContextPointer->BootDrive == 0x40)
	{
		//
		// booting from net
		//
		strcpy(BootPartitionName,"net(0)");

		BlBootingFromNet								= TRUE;
	}
	else if(BootContextRecord->FSContextPointer->BootDrive == 0x41)
	{
		//
		// booting from ramdisk
		//
		strcpy(BootPartitionName,"ramdisk(0)");
	}
	else if(BlIsElToritoCDBoot(BootContextRecord->FSContextPointer->BootDrive))
	{
		//
		// booting from cd
		//
		sprintf(BootPartitionName,"multi(0)disk(0)cdrom(%u)",BootContextRecord->FSContextPointer->BootDrive);

		ElToritoCDBoot									= TRUE;
	}
	else
	{
		//
		// booting from hdd
		//
		BlGetActivePartition(BootPartitionName,BootContextRecord->FSContextPointer->BootDrive,BootContextRecord->FSContextPointer->Partition);

		//
		// update boot partition's signature
		//
		CHAR BootDiskName[80]							= {0};
		ULONG PartitionId								= 0;
		sprintf(BootDiskName,"multi(0)disk(0)rdisk(%u)partition(0)",BootContextRecord->FSContextPointer->BootDrive & 0x7f);
		if(ArcOpen(BootDiskName,ArcOpenReadWrite,&PartitionId) == ESUCCESS)
		{
			LARGE_INTEGER Offset;
			Offset.QuadPart								= 0;
			if(ArcSeek(PartitionId,&Offset,SeekAbsolute) == ESUCCESS)
			{
				UCHAR Sector[0x200];
				ULONG Count;
				if(ArcRead(PartitionId,Sector,sizeof(Sector),&Count) == ESUCCESS)
				{
					PULONG Signature					= static_cast<PULONG>(static_cast<PVOID>(Sector + 0x1b8));
					if(*Signature == 0)
					{
						*Signature						= (ArcGetRelativeTime() << 0x10) + ArcGetRelativeTime();

						if(ArcSeek(PartitionId,&Offset,SeekAbsolute) == ESUCCESS)
						{
							if(ArcWrite(PartitionId,Sector,sizeof(Sector),&Count) != ESUCCESS)
								BlPrint("Falied to write the new signature on the boot partition.\r\n");
						}
						else
						{
							BlPrint("Failed second ArcSeek on the boot partition to check for a signature.\r\n");
						}
					}
				}
				else
				{
					BlPrint("Failed to ArcRead the boot partition to check for a signature.\r\n");
				}
			}
			else
			{
				BlPrint("Failed to ArcSeek the boot partition to check for a signature.\r\n");
			}

			ArcCacheClose(PartitionId);
		}
		else
		{
			BlPrint("Couldn\'t Open the boot partition to check for a signature.\r\n");
		}
	}

	//
	// allocate pcr page
	//
	PcrBasePage											= reinterpret_cast<ULONG>(FwAllocateHeapPermanent(2));
	if(PcrBasePage)
		PcrBasePage										>>= PAGE_SHIFT;

	//
	// allocate tss page
	//
	TssBasePage											= reinterpret_cast<ULONG>(FwAllocateHeapPermanent(3));
	if(TssBasePage)
		TssBasePage										>>= PAGE_SHIFT;

	//
	// initialize memory descriptor list, the OS loader heap, and the OS loader parameter block.
	//
	BlMemoryInitialize();

	//
	// initialze stall count
	//
	AEInitializeStall();

	//
	// initialize serial console
	//
	BlInitializeHeadlessPort();

	//
	// initialize io system
	//
	BlIoInitialize();

	//
	// transfer control to startup
	//
	BlStartup(BootPartitionName);

	//
	// BlStartup failed
	// bit0 = autoreboot
	//
	if(BootFlags & 1)
	{
		BlPrint("\r\nRebooting in 5 seconds...\r\n");

		ULONG Start										= ArcGetRelativeTime();
		while(ArcGetRelativeTime() - Start < 5);

		ArcRestart();
	}

	while(1)
	{
		while(1)
		{
			//
			// display mini-sac
			//
			if(BlTerminalHandleLoaderFailure())
				break;
		}

		ArcRestart();
	}
}