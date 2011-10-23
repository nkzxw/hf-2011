//********************************************************************
//	created:	18:8:2008   2:43
//	file:		checkpae.cpp
//	author:		tiamo
//	purpose:	check pae supported
//********************************************************************

#include "stdafx.h"

//
// cpu vendor
//
enum
{
	//
	// unknonw
	//
	CpuVendorUnknow,

	//
	// intel
	//
	CpuVendorIntel,

	//
	// amd
	//
	CpuVendorAMD,

	//
	// cyrix
	//
	CpuVendorCyrixInstead,

	//
	// genuninetm x86
	//
	CpuVendorGenuineTmx86,

	//
	// centaur hauls
	//
	CpuVendorCentaurHauls,
};

//
// is cpuid present
//
BOOLEAN __declspec(naked) BlIsCpuidPresent()
{
	__asm
	{
		pushfd
		pop     ecx
		xor     ecx, 200000h
		push    ecx
		popfd
		pushfd
		pop     eax
		xor     eax, ecx
		shr     eax, 15h
		and     eax, 1
		xor     eax, 1
		retn
	}
}

//
// cpuid
//
VOID __declspec(naked) CPUID(__in ULONG Index,__out PVOID EaxRegister,__out PVOID EbxRegister,__out PVOID EcxRegister,__out PVOID EdxRegister)
{
	__asm
	{
		push    ebx
		push    esi
		mov     eax, [esp+0ch]
		cpuid
		mov     esi, [esp+10h]
		mov     [esi], eax
		mov     esi, [esp+14h]
		mov     [esi], ebx
		mov     esi, [esp+18h]
		mov     [esi], ecx
		mov     esi, [esp+1ch]
		mov     [esi], edx
		pop     esi
		pop     ebx
		retn    14h
	}
}

//
// check cpu support pae
//
BOOLEAN __declspec(naked) BlpPaeSupported()
{
	__asm
	{
		mov     ecx, 200000h
		pushfd
		pop     eax
		xor     ecx, eax
		push    ecx
		popfd
		pushfd
		pop     ecx
		xor     eax, ecx
		jz      return_false

		push    ebx
		mov     eax, 1
		cpuid
		pop     ebx
		sub     eax, eax
		test    dl, 40h
		jz      return_false

		inc     eax

return_false:
		retn
	}
}

//
// get cpu vendor
//
ULONG BlGetCpuVendor()
{
	//
	// cpu does not support cpuid
	//
	if(!BlIsCpuidPresent())
		return CpuVendorUnknow;

	//
	// read vendor string
	//
	CHAR Buffer[20];
	CPUID(0,Buffer + 0x10,Buffer + 0x00,Buffer + 0x08,Buffer + 0x04);
	Buffer[0x0c]										= 0;

	if(!strcmp(Buffer,"GenuineIntel"))
		return CpuVendorIntel;

	if(!strcmp(Buffer,"AuthenticAMD"))
		return CpuVendorAMD;

	if(!strcmp(Buffer,"CyrixInstead"))
		return CpuVendorCyrixInstead;

	if(!strcmp(Buffer,"GenuineTmx86"))
		return CpuVendorGenuineTmx86;

	if(!strcmp(Buffer,"CentaurHauls"))
		return CpuVendorCentaurHauls;

	return CpuVendorUnknow;
}

//
// check cpu support Nx
//
BOOLEAN BlDoesCpuSupportNx()
{
	//
	// get cpu vendor
	//
	ULONG CpuVendor										= BlGetCpuVendor();
	if(CpuVendor == CpuVendorUnknow)
		return FALSE;

	ULONG volatile Check								= 0;
	ULONG Dummy											= 0;

	if(CpuVendor == CpuVendorAMD)
	{
		CPUID(1,(PVOID)&Check,&Dummy,&Dummy,&Dummy);

		if((Check & 0xf00) < 0x500)
			return FALSE;
	}

	//
	// check max cpuid supported
	//
	CPUID(0x80000000,(PVOID)(&Check),&Dummy,&Dummy,&Dummy);

	if((Check & 0xffffff00) != 0x80000000)
		return FALSE;

	//
	// does not supported
	//
	if(Check < 0x80000001)
		return FALSE;

	CPUID(0x80000001,&Dummy,&Dummy,&Dummy,(PVOID)(&Check));

	return Check & 0x100000 ? TRUE : FALSE;
}

//
// filter some bug chipset
//
BOOLEAN BlpChipsetPaeSupported()
{
	ULONG BadChipsets[]	=
	{
		0,0x00,0x1a208086,
		0,0x00,0x1a218086,
		0,0x00,0x1a228086,
		0,0x1e,0x24188086,
		0,0x1e,0x24288086,
	};

	for(ULONG i = 0; i < ARRAYSIZE(BadChipsets) / 3; i ++)
	{
		ULONG VendorId									= PCI_INVALID_VENDORID;
		if(HalGetBusData(PCIConfiguration,BadChipsets[i * 3],BadChipsets[i * 3 + 1],&VendorId,sizeof(VendorId)) == sizeof(ULONG) && VendorId == BadChipsets[i * 3 + 2])
			return FALSE;
	}

	return TRUE;
}

//
// get image image properites
//
ARC_STATUS Blx86GetImageProperties(__in ULONG DeviceId,__in PCHAR FileName,__out PBOOLEAN LargeAddressAware,__out PBOOLEAN SupportHotplugMemory,__out PBOOLEAN SupportDep)
{
	//
	// open the file
	//
	ULONG FileId;
	ARC_STATUS Status									= BlOpen(DeviceId,FileName,ArcOpenReadOnly,&FileId);
	if(Status != ESUCCESS)
		return Status;

	//
	// read it
	//
	CHAR LocalBuffer[SECTOR_SIZE * 3];
	PVOID Buffer										= reinterpret_cast<PVOID>(ROUND_TO_SECTORS(LocalBuffer));
	ULONG Count											= 0;
	Status												= BlRead(FileId,Buffer,SECTOR_SIZE * 2,&Count);
	BlClose(FileId);

	//
	// read failed
	//
	if(Status != ESUCCESS || Count != SECTOR_SIZE * 2)
		return Status == ESUCCESS ? EBADF : Status;

	//
	// get nt image headers and check it
	//
	PIMAGE_NT_HEADERS NtHeaders							= RtlImageNtHeader(Buffer);
	if(!NtHeaders)
		return EBADF;

	//
	// large address aware is a characteristic
	//
	*LargeAddressAware									= NtHeaders->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE ? TRUE : FALSE;

	//
	// hotplug is supported for nt 5.1(windows xp)
	//
	USHORT Major										= NtHeaders->OptionalHeader.MajorOperatingSystemVersion;
	USHORT Minor										= NtHeaders->OptionalHeader.MinorOperatingSystemVersion;
	*SupportHotplugMemory								= Major > MAJOR_VERSION_NT5 || (Major == MAJOR_VERSION_NT5 && Minor >= MINOR_VERSION_XP);
	*SupportDep											= FALSE;

	if(Major > MAJOR_VERSION_NT5 || (Major == MAJOR_VERSION_NT5 && Minor > MINOR_VERSION_2003))
	{
		//
		// dep is always supported by vista and server 2008
		//
		*SupportDep										= TRUE;
	}
	else if(Major == MAJOR_VERSION_NT5 && (Minor == MINOR_VERSION_XP || Minor == MINOR_VERSION_2003))
	{
		//
		// dep is supported by xp(5.1) with sp2 and 2003(5.2) with sp1
		// read the version resource data and check it
		//
		if(BlOpen(DeviceId,FileName,ArcOpenReadOnly,&FileId) == ESUCCESS)
		{
			//
			// get file size
			//
			FILE_INFORMATION FileInfo;
			if(BlGetFileInformation(FileId,&FileInfo) == ESUCCESS)
			{
				//
				// allocate memory
				//
				ULONG BasePage							= 0;
				if(BlAllocateAlignedDescriptor(LoaderFirmwareTemporary,0,BYTES_TO_PAGES(FileInfo.EndingAddress.LowPart),1,&BasePage) == ESUCCESS)
				{
					//
					// read it
					//
					Buffer								= MAKE_KERNEL_ADDRESS_PAGE(BasePage);
					if(BlRead(FileId,Buffer,FileInfo.EndingAddress.LowPart,&Count) == ESUCCESS)
					{
						//
						// get resource version info
						//
						ULONGLONG Version				= 0;
						if(BlGetResourceVersionInfo(Buffer,&Version) == ESUCCESS)
						{
							//
							// 5.1.2600.2016 for xp
							// 5.2.3790.1127 for 2003 sp1
							//
							if(Minor == MINOR_VERSION_XP && Version >= WINDOWS_XP_WITH_DEP_FULL_VERSION)
								*SupportDep				= TRUE;
							else if(Minor == MINOR_VERSION_2003 && Version >= WINDOWS_2003_SP1_FULL_VERSION)
								*SupportDep				= TRUE;
						}
					}

					BlFreeDescriptor(BasePage);
				}
			}

			BlClose(FileId);
		}
	}

	return Status;
}

//
// check this kernel can work with the hal
//
BOOLEAN Blx86IsKernelCompatible(__in ULONG DeviceId,__in PCHAR KernelPath,__in BOOLEAN HalSupportPae,__inout PBOOLEAN EnablePae)
{
	BOOLEAN LargeAddressAware							= FALSE;
	BOOLEAN SupportHotplugMemory						= FALSE;
	BOOLEAN SupportDep									= FALSE;
	if(Blx86GetImageProperties(DeviceId,KernelPath,&LargeAddressAware,&SupportHotplugMemory,&SupportDep) != ESUCCESS)
		return FALSE;

	if(!LargeAddressAware)
		*EnablePae										= FALSE;
	else if(!HalSupportPae)
		return FALSE;
	else
		*EnablePae										= TRUE;

	return TRUE;
}

//
// check we can use a PAE version kenrel
//
ARC_STATUS Blx86CheckForPaeKernel(__in BOOLEAN PaeOn,__in BOOLEAN PaeOff,__in BOOLEAN DepOn,__in BOOLEAN DepOff,__in_opt PCHAR SpecifiedKernel,__in PCHAR HalPath,
								  __in ULONG LoadPartitionId,__in ULONG SystemPartitionId,__out PULONG MaxPageFrameNumber,__out PBOOLEAN UsePae,__out PCHAR KernelPath)
{
	ULONG KernelAppendPosition							= strlen(KernelPath);
	*MaxPageFrameNumber									= 0;
	*UsePae												= 0;

	//
	// get the max page frame number
	//
	PLIST_ENTRY NextEntry								= BlLoaderBlock->MemoryDescriptorListHead.Flink;

	while(NextEntry != &BlLoaderBlock->MemoryDescriptorListHead)
	{
		PMEMORY_ALLOCATION_DESCRIPTOR Desc				= CONTAINING_RECORD(NextEntry,MEMORY_ALLOCATION_DESCRIPTOR,ListEntry);
		ULONG EndPage									= Desc->BasePage + Desc->PageCount - 1;
		if(EndPage > *MaxPageFrameNumber)
			*MaxPageFrameNumber							= EndPage;

		NextEntry										= NextEntry->Flink;
	}

	//
	// get those...
	//
	BOOLEAN CpuSupportNx								= BlDoesCpuSupportNx();
	BOOLEAN CpuSupportPae								= BlpPaeSupported();
	BOOLEAN ChipsetSupportPae							= BlpChipsetPaeSupported();
	BOOLEAN HardwareSupportPae							= CpuSupportPae && ChipsetSupportPae;
	BOOLEAN EnablePae									= PaeOn;

	//
	// check image properties
	//
	BOOLEAN HalSupportHotplugMemory						= FALSE;
	BOOLEAN HalLargeAddressAwared						= FALSE;
	BOOLEAN HalSupportDep								= FALSE;
	if(Blx86GetImageProperties(SystemPartitionId,HalPath,&HalLargeAddressAwared,&HalSupportHotplugMemory,&HalSupportDep) != ESUCCESS)
		return EBADF;

	//
	// acpi tells us to handle hotplug memory
	//
	if(HalSupportHotplugMemory && Blx86NeedPaeForHotPlugMemory())
		EnablePae										= TRUE;

	//
	// hal can't support large address
	//
	if(!HalLargeAddressAwared)
		HardwareSupportPae								= FALSE;

	//
	// hardware does not support it,or we are asking to do not use pae
	//
	if(!HardwareSupportPae || PaeOff)
		EnablePae										= FALSE;

	//
	// dep option is only valid if both hardware and software support it
	//
	if(HardwareSupportPae && CpuSupportNx && HalSupportDep && !DepOff)
		EnablePae										= TRUE;

	//
	// now check kernel,first try to use caller supplied kernel file
	//
	if(SpecifiedKernel)
	{
		strcpy(KernelPath + KernelAppendPosition,SpecifiedKernel);
		if(Blx86IsKernelCompatible(LoadPartitionId,KernelPath,HardwareSupportPae,&EnablePae))
		{
			*UsePae										= EnablePae;
			return ESUCCESS;
		}
	}

	//
	// check the default kernels
	//
	PCHAR NormalKernel									= "ntoskrnl.exe";
	PCHAR PaeKernel										= "ntkrnlpa.exe";
	PCHAR CheckKernelFile								= EnablePae ? PaeKernel : NormalKernel;

	//
	// first check the desired
	//
	strcpy(KernelPath + KernelAppendPosition,CheckKernelFile);
	if(Blx86IsKernelCompatible(LoadPartitionId,KernelPath,HardwareSupportPae,&EnablePae))
	{
		*UsePae											= EnablePae;
		return ESUCCESS;
	}

	//
	// finally check the no pae version
	//
	if(CheckKernelFile == NormalKernel)
		return EINVAL;

	strcpy(KernelPath + KernelAppendPosition,NormalKernel);
	if(!Blx86IsKernelCompatible(LoadPartitionId,KernelPath,HardwareSupportPae,&EnablePae))
		return EINVAL;

	*UsePae												= EnablePae;

	return ESUCCESS;
}
