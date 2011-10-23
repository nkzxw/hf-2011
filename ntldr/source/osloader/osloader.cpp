//********************************************************************
//	created:	16:8:2008   14:23
//	file:		osloader.h
//	author:		tiamo
//	purpose:	load kernel,hal,device driver and system registry
//********************************************************************

#include "stdafx.h"

//
// dcache fill size
//
ULONG													BlDcacheFillSize = 32;

//
// loader block
//
PLOADER_PARAMETER_BLOCK									BlLoaderBlock;

//
// kernel file name
//
CHAR													KernelFileName[8 + 1 + 3 + 1]	= "ntoskrnl.exe";

//
// hal file name
//
CHAR													HalFileName[8 + 1 + 3 + 1]		= "hal.dll";

//
// kd file name
//
CHAR													KdFileName[8 + 1 + 3 + 1]		= "kdcom.dll";

//
// use alternate kd dll
//
BOOLEAN													UseAlternateKdDll;

//
// use PAE
//
BOOLEAN													BlUsePae;

//
// reboot system
//
BOOLEAN													BlRebootSystem;

//
// kernel has been checked
//
BOOLEAN													BlKernelChecked;

//
// check we can use a PAE version kenrel
//
ARC_STATUS Blx86CheckForPaeKernel(__in BOOLEAN PaeOn,__in BOOLEAN PaeOff,__in BOOLEAN DepOn,__in BOOLEAN DepOff,__in_opt PCHAR SpecifiedKernel,__in PCHAR HalPath,
								  __in ULONG LoadPartitionId,__in ULONG SystemPartitionId,__out PULONG MaxPageFrameNumber,__out PBOOLEAN UsePae,__out PCHAR KernelPath);

//
// load triage dump
//
ARC_STATUS BlLoadTriageDump(__in ULONG DeviceId,__out PVOID* DumpBlock);

//
// initialize kernel's data structures
//
ARC_STATUS BlSetupForNt(__in PLOADER_PARAMETER_BLOCK BlLoaderBlock);

//
// check kernel version,adjust VirtualBias as needed
//
ARC_STATUS BlpCheckVersion(__in ULONG DeviceId,__in PCHAR KernelFileName)
{
	ULONG FileId;
	ARC_STATUS Status									= BlOpen(DeviceId,KernelFileName,ArcOpenReadOnly,&FileId);
	if(Status != ESUCCESS)
		return Status;

	CHAR LocalBuffer[SECTOR_SIZE * 3];
	PVOID Buffer										= reinterpret_cast<PVOID>(reinterpret_cast<ULONG>(LocalBuffer + SECTOR_SIZE - 1) & ~(SECTOR_SIZE - 1));
	ULONG Count											= 0;
	Status												= BlRead(FileId,Buffer,SECTOR_SIZE * 2,&Count);
	BlClose(FileId);
	if(Status != ESUCCESS)
		return Status;

	PIMAGE_NT_HEADERS NtHeaders							= RtlImageNtHeader(Buffer);
	if(!NtHeaders)
		return EBADF;

	USHORT Major										= NtHeaders->OptionalHeader.MajorOperatingSystemVersion;
	USHORT Minor										= NtHeaders->OptionalHeader.MinorOperatingSystemVersion;
	if(Major > MAJOR_VERSION_NT5 || (Major == MAJOR_VERSION_NT5 && Minor >= MINOR_VERSION_XP))
		return ESUCCESS;

	BlOldKernel											= TRUE;
	BlKernelChecked										= TRUE;
	BlHighestPage										= 0xfd8;
	if(!BlVirtualBias)
		return ESUCCESS;

	BlVirtualBias										= 0x5d000000;
	for(ULONG i = 0; i < 4; i ++)
		PDE[i + 0x374]									= PDE[i + 0x374 + 0xc];

	for(ULONG i = 0; i < 12; i ++)
		PDE[i + 0x378]									= ZeroPte;

	return ESUCCESS;
}

//
// load system files into memory and transfer control to kernel
//
ARC_STATUS BlOsLoader(__in ULONG Argc,__in PCHAR Argv[],__in PCHAR Envp[])
{
	BlShowProgressBar									= FALSE;
	BlStartTime											= ArcGetRelativeTime();

	//
	// initialzie stdio
	//
	ARC_STATUS Status									= BlInitStdio(Argc,Argv);
	if(Status != ESUCCESS)
		return Status;

	//
	// find cache info,we need it to setup BlDcacheFillSize
	//
	PCONFIGURATION_COMPONENT_DATA DataCache				= KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,CacheClass,SecondaryCache,0);
	if(!DataCache)
		DataCache										= KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,CacheClass,SecondaryDcache,0);
	if(!DataCache)
		DataCache										= KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,CacheClass,PrimaryDcache,0);

	//
	// found,update DcacheFillSize
	//
	if(DataCache)
		BlDcacheFillSize								= (DataCache->ComponentEntry.Key >> 24) * (1 << ((DataCache->ComponentEntry.Key >> 16) & 0xff));

	BOOLEAN UseDiskCache								= FALSE;

	__try
	{
		//
		// initialize file io
		//
		Status											= BlIoInitialize();
		if(Status != ESUCCESS)
			try_leave(BlFatalError(LOAD_HW_DISK_CLASS,DIAG_BL_IO_INIT,LOAD_HW_DISK_ACT));

		//
		// initialize resources
		//
		Status											= BlInitResources(Argv[0]);
		if(Status != ESUCCESS)
			try_leave(BlFatalError(LOAD_HW_DISK_CLASS,DIAG_BL_IO_INIT,LOAD_HW_DISK_ACT));

		//
		// setup progressbar message
		//
		BlSetProgBarCharacteristics(BL_PROGRESS_BAR_FRONT_CHAR,BL_PROGRESS_BAR_BACK_CHAR);

		//
		// initialize kernel config tree
		//
		BlLoaderBlock->ConfigurationRoot				= 0;
		Status											= BlConfigurationInitialize(0,0);
		if(Status != ESUCCESS)
			try_leave(BlFatalError(LOAD_HW_FW_CFG_CLASS,DIAG_BL_CONFIG_INIT,LOAD_HW_FW_CFG_ACT));

		//
		// get loader options
		//
		PCHAR LoaderOptions								= BlGetArgumentValue(Argc,Argv,"osloadoptions");
		BOOLEAN SafeBoot								= FALSE;
		BOOLEAN PaeInOptions							= FALSE;
		BOOLEAN NoPaeInOptions							= FALSE;
		BOOLEAN DepAwaysOff								= FALSE;
		BOOLEAN DepAwaysOn								= FALSE;
		PCHAR SpecifiedKernelFile						= 0;
		if(LoaderOptions)
		{
			//
			// allocate a buffer from pool and copy the loader options
			//
			ULONG Length								= strlen(LoaderOptions) + 1;
			BlLoaderBlock->LoadOptions					= static_cast<PCHAR>(BlAllocateHeap(Length));
			strcpy(BlLoaderBlock->LoadOptions,LoaderOptions);

			//
			// safeboot
			//
			if(strstr(LoaderOptions,"SAFEBOOT") || strstr(LoaderOptions,"safeboot"))
				SafeBoot								= TRUE;

			//
			// check for magic switch that says we should output the filenames of the files instead of just dots.
			//
			if(strstr(LoaderOptions,"SOS") || strstr(LoaderOptions,"sos"))
				BlOutputDots							= FALSE;

			//
			// shoow windows logo
			//
			extern BOOLEAN GraphicsMode;
			GraphicsMode								= strstr(LoaderOptions,"BOOTLOGO") != 0;

			//
			// 3gb switch
			//
			if(strstr(LoaderOptions,"3GB") || strstr(LoaderOptions,"3gb"))
				BlVirtualBias							= 0x60000000;

			//
			// enable pae
			//
			if(strstr(LoaderOptions,"PAE") || strstr(LoaderOptions,"pae"))
				PaeInOptions							= TRUE;

			//
			// disable pae
			//
			if(strstr(LoaderOptions,"NOPAE") || strstr(LoaderOptions,"nopae"))
				NoPaeInOptions							= TRUE;

			//
			// data execute protect
			//
			if(strstr(LoaderOptions,"NOEXECUTE=ALWAYSOFF") || strstr(LoaderOptions,"noexecute=alwaysoff"))
				DepAwaysOff								= TRUE;

			if(strstr(LoaderOptions,"NOEXECUTE") || strstr(LoaderOptions,"noexecute"))
				DepAwaysOn								= TRUE;

			if((strstr(LoaderOptions,"EXECUTE") || strstr(LoaderOptions,"execute")) && !DepAwaysOn)
				DepAwaysOff								= TRUE;

			if(SafeBoot)
			{
				NoPaeInOptions							= TRUE;
				DepAwaysOff								= TRUE;
			}

			//
			// hal.dll name
			//
			PCHAR FileName								= strstr(LoaderOptions,"HAL=");
			if(FileName)
			{
				for(ULONG i = 0; i < sizeof(HalFileName); i ++)
				{
					if(FileName[4 + i] == ' ')
					{
						HalFileName[i]					= 0;
						break;
					}

					HalFileName[i]						= FileName[4 + i];
				}
			}

			//
			// ntoskrn.exe name
			//
			FileName									= strstr(LoaderOptions,"KERNEL=");
			if(FileName)
			{
				for(ULONG i = 0; i < sizeof(KernelFileName); i ++)
				{
					if(FileName[7 + i] == ' ')
					{
						KernelFileName[i]				= 0;
						break;
					}

					KernelFileName[i]					= FileName[7 + i];
				}

				SpecifiedKernelFile						= KernelFileName;
			}

			//
			// debug port
			//
			PCHAR DebugPortName							= strstr(BlLoaderBlock->LoadOptions,"debugport=");
			if(!DebugPortName)
				DebugPortName							= strstr(BlLoaderBlock->LoadOptions,"DEBUGPORT=");

			//
			// found a debug port string
			//
			if(DebugPortName)
			{
				//
				// upper case
				//
				_strupr(DebugPortName);

				//
				// use alternate kd dll(1394,usb)
				//
				if(!strstr(DebugPortName,"COM"))
				{
					UseAlternateKdDll					= TRUE;

					//
					// 7 = 'KD' + '.dll' + NULL
					//
					ULONG i;
					for(i = 0; i < sizeof(KdFileName) - 7; i ++)
					{
						if(DebugPortName[10 + i] == ' ')
						{
							KdFileName[i + 2]			= 0;
							break;
						}

						KdFileName[i + 2]				= DebugPortName[10 + i];
					}

					KdFileName[i + 2]					= 0;
					strcat(KdFileName,".DLL");
				}
			}
		}
		else
		{
			BlLoaderBlock->LoadOptions						= 0;
		}

	#if _XIP_SUPPORT_
		if(LoaderOptions)
		{
			PCHAR XIPBoot								= strstr(LoaderOptions,"XIPBOOT");
			if(!XIPBoot)
				XIPBoot									= strstr(LoaderOptions,"xipboot");

			PCHAR XIPROM								= strstr(LoaderOptions,"XIPROM=");
			if(!XIPROM)
				XIPROM									= strstr(LoaderOptions,"xiprom=");

			PCHAR XIPRAM								= strstr(LoaderOptions,"XIPRAM=");
			if(!XIPRAM)
				XIPRAM									= strstr(LoaderOptions,"xipram=");

			PCHAR XIPMEGS								= strstr(LoaderOptions,"XIPMEGS=");
			if(!XIPMEGS)
				XIPMEGS									= strstr(LoaderOptions,"xipmegs=");

			XIPEnabled									= FALSE;
			//
			// rom and ram must not be both specified
			//
			if((XIPRAM && !XIPROM) || (XIPROM && !XIPRAM) && XIPMEGS)
			{
				ULONG XipMegs							= atoi(strchr(XIPMEGS,'=') + 1);
				PCHAR Path								= strchr(XIPROM ? XIPROM : XIPRAM,'=') + 1;

				if(XipMegs && Path)
				{
					//
					// (<<20) = 1 * 1024 * 1024 = 1MB
					//
					XIPPageCount						= (XipMegs << 20) >> PAGE_SHIFT;
					XIPBootFlag							= XIPBoot ? TRUE : FALSE;

					PCHAR End							= Path;
					while(*End && *End != ' ' && *End != '/' && *End != '\t' && *End != '\r' && *End != '\n')
						End								+= 1;

					ULONG Length						= End - Path;
					if(Length > 1)
					{
						XIPLoadPath						= static_cast<PCHAR>(BlAllocateHeap(Length + 1));
						if(XIPLoadPath)
						{
							strncpy(XIPLoadPath,Path,Length);
							XIPLoadPath[Length]			= 0;
							XIPEnabled					= TRUE;
						}
					}
				}
			}
		}

		if(XIPEnabled)
		{
			ULONG SavedLimit							= BlUsableLimit;
			ULONG SavedBase								= BlUsableBase;
			BlUsableLimit								= 0x20000;

			if(BlAllocateAlignedDescriptor(LoaderXIPMemory,0,XIPPageCount,0x400,&XIPBasePage) != ESUCCESS)
			{
				XIPEnabled								= FALSE;
			}
			else
			{
				BlUsableLimit							= SavedLimit;
				BlUsableBase							= SavedBase;
			}
		}
	#endif

		//
		// "\\WINDOWS\\system32\\NTLDR",not used
		//
		PCHAR OsLoader									= BlGetArgumentValue(Argc,Argv,"osloader");

		//
		// "\\WINDOWS"
		//
		PCHAR OsLoadFileName							= BlGetArgumentValue(Argc,Argv,"osloadfilename");
		if(!OsLoadFileName)
			try_leave(BlFatalError(LOAD_HW_FW_CFG_CLASS,DIAG_BL_FW_GET_BOOT_DEVICE,LOAD_HW_FW_CFG_ACT);Status = ENOENT);

		if(!OsLoader)
			try_leave(BlFatalError(LOAD_HW_FW_CFG_CLASS,0x233a,LOAD_HW_FW_CFG_ACT);Status = ENOENT)

		//
		// initialize disk cache
		//
		if(BlDiskCacheInitialize() == ESUCCESS)
			UseDiskCache								= TRUE;

		//
		// get arc disk info
		//
		BlGetArcDiskInformation(0);

		//
		// start config prompt
		//
		BlStartConfigPrompt();

		//
		// use last known good system registry
		//
		BOOLEAN UseLastKnownGood						= LoaderOptions && strstr(LoaderOptions,"LASTKNOWNGOOD");

		//
		// load device,"multi(0)disk(0)rdisk(0)partition(1)"
		//
		PCHAR OsLoadPartition							= BlGetArgumentValue(Argc,Argv,"osloadpartition");
		if(!OsLoadPartition)
			try_leave(BlFatalError(LOAD_HW_FW_CFG_CLASS,DIAG_BL_FW_GET_BOOT_DEVICE,LOAD_HW_FW_CFG_ACT);Status = ENODEV);

	#if _RAMDISK_SUPPORT_
		Status											= RamdiskInitialize(LoaderOptions,FALSE);
		if(Status != ESUCCESS)
			try_leave(NOTHING);
	#endif

		//
		// translate the device name
		//
	#if _SCSI_SUPPORT_
		PCHAR TranslatedName							= BlTranslateSignatureArcName(OsLoadPartition);
		if(TranslatedName)
			OsLoadPartition								= TranslatedName;
	#endif

		//
		// open it
		//
		ULONG LoadPartitionDeviceId						= 0;
		Status											= ArcOpen(OsLoadPartition,ArcOpenReadWrite,&LoadPartitionDeviceId);
		if(Status != ESUCCESS)
			try_leave(BlFatalError(LOAD_HW_DISK_CLASS,DIAG_BL_OPEN_BOOT_DEVICE,LOAD_HW_DISK_ACT));

	#if _RAMDISK_SUPPORT_
		PCHAR SdiBoot									= 0;
		if(BlLoaderBlock->LoadOptions && (SdiBoot = strstr(BlLoaderBlock->LoadOptions,"SDIBOOT=")))
			RamdiskSdiBoot(strchr(SdiBoot,'=') + 1);
	#endif

	#if _GRAPHICS_MODE_SUPPORT_
		if(GraphicsMode)
		{
			//
			// enable graphics mode
			//
			ExternalServicesTable->HardwareCursor(0x80000000,0x12);
			VgaEnableVideo();

			//
			// load boot log
			//
			LoadBootLogoBitmap(LoadPartitionDeviceId,OsLoadFileName);

			if(DisplayLogoOnBoot)
			{
				PrepareGfxProgressBar();
				BlUpdateBootStatus();
			}
		}
	#endif

		//
		// start disk caching
		//
		if(UseDiskCache)
			BlDiskCacheStartCachingOnDevice(LoadPartitionDeviceId);

		//
		// "\\WINDOWS\\"
		//
		CHAR OsLoadDirectory[256];
		strcpy(OsLoadDirectory,OsLoadFileName);
		strcat(OsLoadDirectory,"\\");

		//
		// "\\WINDOWS\\LastGood.Tmp\\"
		//
		CHAR LastGoodDotTmpDir[256];
		strcpy(LastGoodDotTmpDir,OsLoadDirectory);
		strcat(LastGoodDotTmpDir,"LastGood.Tmp\\");

		//
		// "\\WINDOWS\\LastGood\\"
		//
		CHAR LastGoodDir[256];
		strcpy(LastGoodDir,OsLoadDirectory);
		strcat(LastGoodDir,"LastGood\\");

	#if _XIP_SUPPORT_
		if(XIPEnabled)
		{
			ULONG XipFileId;
			XIPEnabled									= FALSE;
			if(BlOpen(LoadPartitionDeviceId,XIPLoadPath,ArcOpenReadOnly,&XipFileId) == ESUCCESS)
			{
				if(XipLargeRead(XipFileId,XIPBasePage,XIPPageCount) == ESUCCESS)
					XIPEnabled							= TRUE;

				BlClose(XipFileId);
			}
		}
	#endif

		LDR_SCAN_IMPORT_SERACH_PATH_SET SearchPathSet;
		SearchPathSet.SearchPathCount					= 1;
		SearchPathSet.SystemRootPath					= "\\SystemRoot";
		SearchPathSet.SearchPath[0].DeviceId			= LoadPartitionDeviceId;
		SearchPathSet.SearchPath[0].DevicePath			= OsLoadPartition;
		SearchPathSet.SearchPath[0].Path				= OsLoadDirectory;
		strcpy(SearchPathSet.PrefixPath,"system32\\");

		UCHAR LoadDriverSearchPathSet[sizeof(LDR_SCAN_IMPORT_SERACH_PATH_SET) + sizeof(SearchPathSet.SearchPath) * 2];
		PLDR_SCAN_IMPORT_SERACH_PATH_SET LoadDevicePath	= reinterpret_cast<PLDR_SCAN_IMPORT_SERACH_PATH_SET>(LoadDriverSearchPathSet);
		LoadDevicePath->SearchPathCount					= 0;

		if(UseLastKnownGood)
		{
			LoadDevicePath->SearchPathCount				= 2;
			LoadDevicePath->SearchPath[0].Path			= LastGoodDotTmpDir;
			LoadDevicePath->SearchPath[0].DevicePath	= OsLoadPartition;
			LoadDevicePath->SearchPath[0].DeviceId		= LoadPartitionDeviceId;

			LoadDevicePath->SearchPath[1].Path			= LastGoodDir;
			LoadDevicePath->SearchPath[1].DevicePath	= OsLoadPartition;
			LoadDevicePath->SearchPath[1].DeviceId		= LoadPartitionDeviceId;
		}

		ULONG Count										= LoadDevicePath->SearchPathCount;
		LoadDevicePath->SearchPathCount					+= 1;
		LoadDevicePath->SearchPath[Count].DeviceId		= LoadPartitionDeviceId;
		LoadDevicePath->SearchPath[Count].DevicePath	= OsLoadPartition;
		LoadDevicePath->SearchPath[Count].Path			= OsLoadDirectory;
		LoadDevicePath->SystemRootPath					= "\\SystemRoot";
		LoadDevicePath->PrefixPath[0]					= 0;

		//
		// "\\WINDOWS\\system32\\"
		//
		CHAR OsLoadSystem32Dir[256];
		strcpy(OsLoadSystem32Dir,OsLoadFileName);
		strcat(OsLoadSystem32Dir,"\\system32\\");

		//
		// "multi(0)disk(0)rdisk(0)partition(1)"
		//
		PCHAR OsSystemPartition							= BlGetArgumentValue(Argc,Argv,"systempartition");
		if(!OsSystemPartition)
			try_leave(BlFatalError(LOAD_HW_FW_CFG_CLASS,DIAG_BL_FW_GET_SYSTEM_DEVICE,LOAD_HW_FW_CFG_ACT);Status = ENODEV);

	#if _SCSI_SUPPORT_
		TranslatedName									= BlTranslateSignatureArcName(OsSystemPartition);
		if(TranslatedName)
			OsSystemPartition							= TranslatedName;
	#endif

		ULONG SystemPartitionDeviceId					= LoadPartitionDeviceId;
		if(_stricmp(OsSystemPartition,OsLoadPartition))
		{
			Status										= ArcOpen(OsSystemPartition,ArcOpenReadWrite,&SystemPartitionDeviceId);
			if(Status != ESUCCESS)
				try_leave(BlFatalError(LOAD_HW_FW_CFG_CLASS,DIAG_BL_FW_OPEN_SYSTEM_DEVICE,LOAD_HW_FW_CFG_ACT));

			if(UseDiskCache)
				BlDiskCacheStartCachingOnDevice(SystemPartitionDeviceId);
		}

		PCHAR LastSep									= strrchr(OsLoader,'\\');
		PCHAR FirstSep									= strchr(OsLoader,'\\');
		ULONG Length									= LastSep - FirstSep + 1;
		BOOLEAN OsLoaderInRootDir;
		if(LastSep != FirstSep && LastSep)
			OsLoaderInRootDir							= FALSE;

		if(BlLoadTriageDump(LoadPartitionDeviceId,&BlLoaderBlock->Extension->TriageDumpBlock) != ESUCCESS)
			BlLoaderBlock->Extension->TriageDumpBlock	= 0;

	#if _HIBERNATE_SUPPORT_
		if(BlHiberRestore(LoadPartitionDeviceId,0) != ESUCCESS)
			try_leave(NOTHING);
	#endif

		BlLogInitialize(SystemPartitionDeviceId);

		BlLoaderBlock->u.I386.CommonDataArea			= 0;

		extern BOOLEAN RedirectEnabled;
		if(RedirectEnabled && LoaderRedirectionInfo.PortAddress)
		{
			BlLoaderBlock->Extension->RedirectionInfo	= static_cast<PLOADER_REDIRECTION_INFORMATION>(BlAllocateHeap(sizeof(LOADER_REDIRECTION_INFORMATION)));
			RtlCopyMemory(BlLoaderBlock->Extension->RedirectionInfo,&LoaderRedirectionInfo,sizeof(LOADER_REDIRECTION_INFORMATION));
		}
		else
		{
			BlLoaderBlock->Extension->RedirectionInfo	= 0;
		}

		//
		// "\\WINDOWS\\system32\\hal.dll"
		//
		CHAR HalFullPath[256];
		strcpy(HalFullPath,OsLoadSystem32Dir);
		strcat(HalFullPath,HalFileName);

		CHAR KernelFullPath[256];
		strcpy(KernelFullPath,OsLoadSystem32Dir);

		ULONG MaxPhysicalPage							= 0;
		Status											= Blx86CheckForPaeKernel(PaeInOptions,NoPaeInOptions,DepAwaysOn,DepAwaysOff,SpecifiedKernelFile,HalFullPath,
																				 LoadPartitionDeviceId,SystemPartitionDeviceId,&MaxPhysicalPage,&BlUsePae,KernelFullPath);

		if(Status != ESUCCESS)
			try_leave(BlFatalError(LOAD_SW_MIS_FILE_CLASS,Status == EBADF ? DIAG_BL_LOAD_HAL_IMAGE_X86 : DIAG_BL_LOAD_SYSTEM_IMAGE,LOAD_SW_FILE_REINST_ACT));

		BlUsableBase									= BlVirtualBias ? 0xc00  : 0x400;
		BlUsableLimit									= BlVirtualBias ? 0x1000 : 0x800;

		if(BlTerminalConnected)
		{
			BlOutputStartupMsg(BL_STARTING_WINDOWS);
			BlOutputTrailerMsg(BL_GOTO_ADVANCED_BOOT_F8);
		}

		BlpCheckVersion(LoadPartitionDeviceId,KernelFullPath);
		BlOutputLoadMessage(OsLoadPartition,KernelFullPath,0);

		PVOID KernelBase								= 0;
		while(1)
		{
			Status										= BlLoadImageEx(LoadPartitionDeviceId,LoaderSystemCode,KernelFullPath,IMAGE_FILE_MACHINE_I386,0,0,&KernelBase);
			if(Status != ENOMEM)
				break;

			if(BlUsableBase || BlUsableLimit != 0x20000)
			{
				BlUsableBase							= 0;
				BlUsableLimit							= 0x20000;
			}
			else
			{
				break;
			}
		}

		if(Status != ESUCCESS)
			try_leave(BlFatalError(LOAD_SW_MIS_FILE_CLASS,DIAG_BL_LOAD_SYSTEM_IMAGE,LOAD_SW_FILE_REINST_ACT));

		BlUpdateBootStatus();

		PBOOTFS_INFO BootFsInfo							= BlGetFsInfo(LoadPartitionDeviceId);
		if(!BootFsInfo)
			try_leave(BlFatalError(LOAD_SW_MIS_FILE_CLASS,DIAG_BL_LOAD_SYSTEM_IMAGE,LOAD_SW_FILE_REINST_ACT));

		typedef VOID (*PTRANSFER_ROUTINE)(__in PLOADER_PARAMETER_BLOCK LoaderBlock);;
		PTRANSFER_ROUTINE KernelEntry					= Add2Ptr(KernelBase,RtlImageNtHeader(KernelBase)->OptionalHeader.AddressOfEntryPoint,PTRANSFER_ROUTINE);

		BlOutputLoadMessage(OsLoadPartition,HalFullPath,0);

		PVOID HalBase									= 0;
		while(1)
		{
			Status										= BlLoadImageEx(LoadPartitionDeviceId,LoaderHalCode,HalFullPath,IMAGE_FILE_MACHINE_I386,0,0,&HalBase);
			if(Status != ENOMEM)
				break;

			if(BlUsableBase || BlUsableLimit != 0x20000)
			{
				BlUsableBase							= 0;
				BlUsableLimit							= 0x20000;
			}
			else
			{
				break;
			}
		}

		if(Status != ESUCCESS)
			try_leave(BlFatalError(LOAD_SW_MIS_FILE_CLASS,DIAG_BL_LOAD_HAL_IMAGE_X86,LOAD_SW_FILE_REINST_ACT));

		BlUpdateBootStatus();

		CHAR KdFileFullPath[256];
		strcpy(KdFileFullPath,OsLoadSystem32Dir);
		strcat(KdFileFullPath,KdFileName);

		BlOutputLoadMessage(OsLoadPartition,KdFileFullPath,0);

		PVOID KdBase									= 0;
		Status											= BlLoadImageEx(LoadPartitionDeviceId,LoaderSystemCode,KdFileFullPath,IMAGE_FILE_MACHINE_I386,0,0,&KdBase);

		if(Status != ESUCCESS && UseAlternateKdDll)
		{
			strcpy(KdFileFullPath,OsLoadSystem32Dir);
			strcat(KdFileFullPath,"kdcom.dll");

			BlOutputLoadMessage(OsLoadPartition,KdFileFullPath,0);

			Status										= BlLoadImageEx(LoadPartitionDeviceId,LoaderSystemCode,KdFileFullPath,IMAGE_FILE_MACHINE_I386,0,0,&KdBase);
		}

		BOOLEAN KdFailedToLoad							= Status != ESUCCESS;

		BlUpdateBootStatus();

		BlUsableBase									= 0;
		BlUsableLimit									= 0x20000;
		PLDR_DATA_TABLE_ENTRY KernelLdrEntry			= 0;
		Status											= BlAllocateDataTableEntry("ntoskrnl.exe",KernelFullPath,KernelBase,&KernelLdrEntry);
		if(Status != ESUCCESS)
			try_leave(BlFatalError(LOAD_SW_INT_ERR_CLASS,DIAG_BL_LOAD_SYSTEM_IMAGE,LOAD_SW_INT_ERR_ACT));

		PLDR_DATA_TABLE_ENTRY HalLdrEntry				= 0;
		Status											= BlAllocateDataTableEntry("hal.dll",HalFullPath,HalBase,&HalLdrEntry);
		if(Status != ESUCCESS)
			try_leave(BlFatalError(LOAD_SW_INT_ERR_CLASS,DIAG_BL_LOAD_HAL_IMAGE_X86,LOAD_SW_INT_ERR_ACT));

		PLDR_DATA_TABLE_ENTRY KdLdrEntry				= 0;
		if(!KdFailedToLoad)
		{
			Status										= BlAllocateDataTableEntry("kdcom.dll",KdFileFullPath,KdBase,&KdLdrEntry);
			if(Status != ESUCCESS)
				try_leave(BlFatalError(LOAD_SW_INT_ERR_CLASS,DIAG_BL_LOAD_SYSTEM_DLLS,LOAD_SW_INT_ERR_ACT));
		}

		Status											= BlScanImportDescriptorTable(&SearchPathSet,KernelLdrEntry,LoaderSystemCode);
		if(Status != ESUCCESS)
			try_leave(BlFatalError(LOAD_SW_INT_ERR_CLASS,DIAG_BL_LOAD_SYSTEM_IMAGE,LOAD_SW_INT_ERR_ACT));

		Status											= BlScanImportDescriptorTable(&SearchPathSet,HalLdrEntry,LoaderHalCode);
		if(Status != ESUCCESS)
			try_leave(BlFatalError(LOAD_SW_INT_ERR_CLASS,DIAG_BL_LOAD_HAL_IMAGE_X86,LOAD_SW_INT_ERR_ACT));

		if(!KdFailedToLoad)
		{
			Status										= BlScanImportDescriptorTable(&SearchPathSet,KdLdrEntry,LoaderSystemCode);
			if(Status != ESUCCESS)
				try_leave(BlFatalError(LOAD_SW_INT_ERR_CLASS,DIAG_BL_LOAD_SYSTEM_DLLS,LOAD_SW_INT_ERR_ACT));
		}

		BlLoaderBlock->NlsData							= static_cast<PNLS_DATA_BLOCK>(BlAllocateHeap(sizeof(NLS_DATA_BLOCK)));
		if(!BlLoaderBlock->NlsData)
			try_leave(BlFatalError(LOAD_HW_MEM_CLASS,DIAG_BL_LOAD_SYSTEM_HIVE,LOAD_HW_MEM_ACT);Status = ENOMEM);

		CHAR BadFileName[128];
		BOOLEAN LoadDeviceWithLastKnownGood				= UseLastKnownGood;
		BOOLEAN LoadSacDriver							= TRUE;
		Status											= BlLoadAndScanSystemHive(LoadPartitionDeviceId,OsLoadPartition,OsLoadFileName,BootFsInfo->DriverName,
																				  &LoadDeviceWithLastKnownGood,&LoadSacDriver,BadFileName);

		if(Status == ESUCCESS)
		{
			if(!LoadDeviceWithLastKnownGood)
			{
				if(UseLastKnownGood)
				{
					LoadDevicePath->SearchPathCount		= 1;
					RtlCopyMemory(LoadDevicePath->SearchPath,LoadDevicePath->SearchPath + 2,sizeof(LoadDevicePath->SearchPath[0]));
				}
			}
			else if(!UseLastKnownGood)
			{
				RtlCopyMemory(LoadDevicePath->SearchPath + 1,LoadDevicePath->SearchPath,sizeof(LoadDevicePath->SearchPath[0]));
				RtlCopyMemory(LoadDevicePath->SearchPath + 2,LoadDevicePath->SearchPath,sizeof(LoadDevicePath->SearchPath[0]));

				LoadDevicePath->SearchPathCount			= 3;
				LoadDevicePath->SearchPath[0].Path		= LastGoodDotTmpDir;
				LoadDevicePath->SearchPath[1].Path		= LastGoodDir;
			}

			BlMaxFilesToLoad							= BlNumFilesLoaded;
			for(PLIST_ENTRY NextEntry = BlLoaderBlock->BootDriverListHead.Flink;NextEntry != &BlLoaderBlock->BootDriverListHead; NextEntry = NextEntry->Flink)
				BlMaxFilesToLoad						+= 1;

			BlRedrawProgressBar();

			if(BlLoaderBlock->Extension->RedirectionInfo && LoadSacDriver)
				BlAddToBootDriverList(&BlLoaderBlock->BootDriverListHead,L"sacdrv.sys",L"sacdrv",L"SAC",1,1,TRUE);

			Status										= BlLoadBootDrivers(LoadDevicePath,&BlLoaderBlock->BootDriverListHead,BadFileName);
		}

		if(Status != ESUCCESS)
		{
			if(BlRebootSystem)
				try_leave(Status = ESUCCESS);

			try_leave(BlBadFileMessage(BadFileName));
		}

		CHAR DevicePrefix[256];
		CHAR DeviceName[256];
		Status											= BlGenerateDeviceNames(OsLoadPartition,DeviceName,DevicePrefix);
		if(Status != ESUCCESS)
			try_leave(BlFatalError(LOAD_HW_FW_CFG_CLASS,DIAG_BL_ARC_BOOT_DEV_NAME,LOAD_HW_FW_CFG_ACT));

		Length											= strlen(DeviceName) + 1;
		BlLoaderBlock->ArcBootDeviceName				= static_cast<PCHAR>(BlAllocateHeap(Length));
		strcpy(BlLoaderBlock->ArcBootDeviceName,DeviceName);

		Length											= strlen(OsLoadFileName) + 2;
		BlLoaderBlock->NtBootPathName					= static_cast<PCHAR>(BlAllocateHeap(Length));
		strcpy(BlLoaderBlock->NtBootPathName,OsLoadFileName);
		strcat(BlLoaderBlock->NtBootPathName,"\\");

		strcpy(DeviceName,BlGetArgumentValue(Argc,Argv,"x86systempartition"));
		Length											= strlen(DeviceName) + 1;
		BlLoaderBlock->ArcHalDeviceName					= static_cast<PCHAR>(BlAllocateHeap(Length));
		strcpy(BlLoaderBlock->ArcHalDeviceName,DeviceName);

		BlLoaderBlock->NtHalPathName					= static_cast<PCHAR>(BlAllocateHeap(2));
		BlLoaderBlock->NtHalPathName[0]					= '\\';
		BlLoaderBlock->NtHalPathName[1]					= 0;

		ArcCacheClose(LoadPartitionDeviceId);
		if(UseDiskCache)
			BlDiskCacheStopCachingOnDevice(LoadPartitionDeviceId);

		if(SystemPartitionDeviceId != LoadPartitionDeviceId)
		{
			ArcCacheClose(SystemPartitionDeviceId);
			if(UseDiskCache)
				BlDiskCacheStopCachingOnDevice(SystemPartitionDeviceId);
		}

		BlUpdateProgressBar(100);

	#if _NET_SUPPORT_
		if(BlBootingFromNet)
		{
			BlLoaderBlock->Extension->NetworkBootInfo	= BlAllocateHeap(0x10);
			RtlZeroMemory(BlLoaderBlock->Extension->NetworkBootInfo,0x10);
			Status										= NetFillNetworkLoaderBlock(BlLoaderBlock->Extension->NetworkBootInfo);
			if(Status != ESUCCESS)
				try_leave(BlFatalError(LOAD_HW_MEM_CLASS,DIAG_BL_MEMORY_INIT,LOAD_HW_MEM_ACT));

			NetTerminate();
		}
	#endif

		BlWriteBootStatusFlags(LoadPartitionDeviceId,OsLoadFileName,0,0);

		AETerminateIo();

		Status											= BlSetupForNt(BlLoaderBlock);
		if(Status != ESUCCESS)
			try_leave(BlFatalError(LOAD_SW_INT_ERR_CLASS,DIAG_BL_SETUP_FOR_NT,LOAD_SW_INT_ERR_ACT));

		BlLogTerminate();

		KernelEntry(BlLoaderBlock);

		Status											= EBADF;
		BlFatalError(LOAD_SW_BAD_FILE_CLASS,DIAG_BL_KERNEL_INIT_XFER,LOAD_SW_FILE_REINST_ACT);
	}
	__finally
	{
		if(UseDiskCache)
			BlDiskCacheUninitialize();
	}

	return Status;
}
