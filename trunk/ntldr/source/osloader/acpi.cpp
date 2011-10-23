//********************************************************************
//	created:	11:8:2008   0:50
//	file:		acpi.cpp
//	author:		tiamo
//	purpose:	acpi table support
//********************************************************************

#include "stdafx.h"

//
// this is a legacy free bios
//
BOOLEAN BlLegacyFree									= FALSE;

//
// rsdp
//
PACPI_RSDP BlRsdp										= 0;

//
// rsdt
//
PACPI_RSDT BlRsdt										= 0;

//
// xsdt
//
PACPI_XSDT BlXsdt										= 0;

//
// find rsdp
//
VOID BlFindRsdp()
{
	for(ULONG i = 0; i < 2; i ++)
	{
		PULONGLONG Start								= 0;
		PULONGLONG End									= 0;
		PHYSICAL_ADDRESS MapStart;

		//
		// search Extended BIOS Data Area segment first
		//
		if(i == 0)
		{
			MapStart.QuadPart							= 0;
			PUSHORT FirstPage							= static_cast<PUSHORT>(MmMapIoSpace(MapStart,0x1000,MmNonCached));

			//
			// 0x40e is the Extended BIOS Data Area Segment
			//
			if(FirstPage[0x40e / sizeof(USHORT)])
			{
				//
				// get flat address by left-shifting 4 bit
				//
				MapStart.QuadPart						= FirstPage[0x40e / sizeof(USHORT)] << 4;
				Start									= static_cast<PULONGLONG>(MmMapIoSpace(MapStart,0x2000,MmNonCached));
				End										= Start + 0x400 / sizeof(ULONGLONG);
			}
		}
		else
		{
			MapStart.QuadPart							= 0xe0000;
			Start										= static_cast<PULONGLONG>(MmMapIoSpace(MapStart,0x1ffff,MmNonCached));
			End											= Start + 0x1ffff / sizeof(ULONGLONG);
		}

		//
		// search rsdp
		//
		for(; Start < End; Start ++)
		{
			if(*Start == ACPI_RSDP_SIGNATURE)
			{
				//
				// found it,checksum must be zero
				//
				UCHAR Checksum							= 0;
				for(ULONG i = 0; i < sizeof(ACPI_RSDP); i ++)
					Checksum							+= *Add2Ptr(Start,i,PUCHAR);

				//
				// valid rsdp found,save it
				//
				if(!Checksum)
				{
					BlRsdp								= Add2Ptr(Start,0,PACPI_RSDP);

					//
					// map header
					//
					MapStart.QuadPart					= BlRsdp->RsdtAddress;
					BlRsdt								= static_cast<PACPI_RSDT>(MmMapIoSpace(MapStart,sizeof(ACPI_DESCRIPTION_HEADER),MmNonCached));

					//
					// map the whole table
					//
					BlRsdt								= static_cast<PACPI_RSDT>(MmMapIoSpace(MapStart,BlRsdt->Header.Length,MmNonCached));

					return;
				}
			}
		}
	}

	BlRsdp												= 0;
	BlRsdt												= 0;
}

//
// get acpi table
//
PACPI_DESCRIPTION_HEADER BlFindACPITable(__in ULONG Signature,__in ULONG Length)
{
	//
	// get rsdt first
	//
	if(!BlRsdt)
		BlFindRsdp();

	//
	// bios did not support acpi,give up
	//
	if(!BlRsdt)
		return 0;

	if(Signature == ACPI_RSDT_SIGNATURE)
		return &BlRsdt->Header;

	if(Signature == ACPI_XSDT_SIGNATURE)
		return &BlXsdt->Header;

	if(Signature == ACPI_DSDT_SIGNATURE)
	{
		//
		// get fadt
		//
		PACPI_FADT Fadt									= CONTAINING_RECORD(BlFindACPITable(ACPI_FADT_SIGNATURE,sizeof(ACPI_FADT)),ACPI_FADT,Header);
		if(!Fadt)
			return 0;

		//
		// if bios gave us a 64bit dsdt,return it
		// otherwise return the 32bit one
		//
		PHYSICAL_ADDRESS MapAddress;
		if(BlXsdt)
			MapAddress.QuadPart							= Fadt->Dsdt64;
		else
			MapAddress.QuadPart							= Fadt->Dsdt;

		//
		// map it
		//
		return static_cast<PACPI_DESCRIPTION_HEADER>(MmMapIoSpace(MapAddress,Length,MmNonCached));
	}

	//
	// search in rsdt
	//
	ULONG Count											= BlXsdt ? BlXsdt->Header.Length : BlRsdt->Header.Length;
	if(Count < sizeof(ACPI_DESCRIPTION_HEADER))
		Count											= sizeof(ACPI_DESCRIPTION_HEADER);

	Count												= (Count - sizeof(ACPI_DESCRIPTION_HEADER)) / (BlXsdt ? sizeof(LONGLONG) : sizeof(ULONG));

	//
	// too big
	//
	if(Count > 0x100)
		return 0;

	for(ULONG i = 0; i < Count; i ++)
	{
		PHYSICAL_ADDRESS MapAddress;

		//
		// if bios gave us an extended system description table,use it
		// otherwise use rdst
		//
		if(BlXsdt)
			MapAddress.QuadPart							= BlXsdt->Tables[i];
		else
			MapAddress.QuadPart							= BlRsdt->Tables[i];

		//
		// map header
		//
		PACPI_DESCRIPTION_HEADER Header					= static_cast<PACPI_DESCRIPTION_HEADER>(MmMapIoSpace(MapAddress,sizeof(ACPI_DESCRIPTION_HEADER),MmNonCached));

		//
		// check signature
		//
		if(Header && Header->Signature == Signature)
		{
			//
			// the whole table fit in the same page?
			//
			if(PAGE_ALIGN(Add2Ptr(Header,Length,PVOID)) <= PAGE_ALIGN(Add2Ptr(Header,sizeof(ACPI_DESCRIPTION_HEADER),PVOID)))
				return Header;

			//
			// more pages are needed
			//
			return static_cast<PACPI_DESCRIPTION_HEADER>(MmMapIoSpace(MapAddress,Length,MmNonCached));
		}
	}

	return 0;
}

//
// detect legacy free bios
//
BOOLEAN BlDetectLegacyFreeBios()
{
	//
	// already detected?
	//
	if(BlLegacyFree)
		return TRUE;

	//
	// get fadt
	//
	PACPI_FADT Fadt										= CONTAINING_RECORD(BlFindACPITable(ACPI_FADT_SIGNATURE,sizeof(ACPI_FADT)),ACPI_FADT,Header);

	//
	// Fadt->BootArch & 2
	//	indicates that the motherboard contains support for a port 60 and 64 based keyboard controller, usually implemented as an 8042 or equivalent micro-controller.
	//
	if(!Fadt || Fadt->Header.Revision < 2 || Fadt->Header.Length <= FIELD_OFFSET(ACPI_FADT,ResetRegister) || (Fadt->BootArch & 2))
		return FALSE;

	BlLegacyFree										= TRUE;

	return TRUE;
}

//
// get redirection info
//
BOOLEAN BlRetrieveBIOSRedirectionInformation(__inout PLOADER_REDIRECTION_INFORMATION Info)
{
	//
	// find spcr table
	//
	PACPI_SPCR Spcr										= CONTAINING_RECORD(BlFindACPITable(ACPI_SPCR_SIGNATURE,sizeof(ACPI_SPCR)),ACPI_SPCR,Header);
	if(!Spcr)
		return FALSE;

	//
	// calc checksum
	//
	UCHAR CheckSum										= 0;
	for(ULONG i = 0; i < Spcr->Header.Length; i ++)
		CheckSum										+= *Add2Ptr(Spcr,i,PUCHAR);

	//
	// checksum should be zero
	//
	if(CheckSum)
		return FALSE;

	//
	// if the lower part is zero,it means spcr is disabled
	//
	if(!Spcr->BaseAddress.Address.LowPart)
		return FALSE;

	Info->Valid											= TRUE;
	Info->MemorySpace									= Spcr->BaseAddress.AddressSpaceID == 0;
	Info->PortNum										= 3;
	Info->PortAddress									= Spcr->BaseAddress.Address.LowPart;
	switch(Spcr->Baudrate)
	{
	case 7:
		Info->Baudrate									= 115200;
		break;

	case 6:
		Info->Baudrate									= 57600;
		break;

	case 4:
		Info->Baudrate									= 19200;
		break;

	default:
	case 3:
		Info->Baudrate									= 9600;
		break;
	}

	Info->StopBits										= Spcr->StopBits;
	Info->Parity										= Spcr->Parity;
	Info->TerminalType									= Spcr->TerminalType;

	if(Spcr->Header.Length >= sizeof(ACPI_SPCR))
	{
		Info->PciDeviceId								= Spcr->DeviceId;
		Info->PciVendorId								= Spcr->VendorId;
		Info->BusNumber									= Spcr->BusNumber;
		Info->DeviceNumber								= Spcr->DeviceNumber;
		Info->FunctionNumber							= Spcr->FunctionNumber;
		Info->Flags										= Spcr->Flags;
	}
	else
	{
		Info->PciDeviceId								= 0xffff;
		Info->PciVendorId								= 0xffff;
		Info->BusNumber									= 0;
		Info->DeviceNumber								= 0;
		Info->FunctionNumber							= 0;
		Info->Flags										= 0;
	}

	return TRUE;
}

//
// load guid
//
VOID BlLoadGUID(__out GUID* Guid)
{
	//
	// search UUID BIOS structure
	//
	PUCHAR Start										= reinterpret_cast<PUCHAR>(0xe0000);
	PUCHAR End											= reinterpret_cast<PUCHAR>(0x100000);

	for(;Start < End; Start ++)
	{
		//
		// check signature
		//
		if(Start[0] == '_' && Start[1] == 'U' && Start[2] == 'U' && Start[3] == 'I' && Start[4] == 'D')
		{
			//
			// read length (offset = 7)
			//
			USHORT Length								= Start[7] | (Start[8] << 8);

			//
			// calc checksum
			//
			UCHAR Checksum								= 0;
			for(USHORT i = 0; i < Length; i ++)
				Checksum								+= Start[7 + i];

			//
			// found
			//
			if(!Checksum)
			{
				RtlCopyMemory(Guid,Add2Ptr(Start,9,PVOID),sizeof(GUID));
				return;
			}
		}
	}

	RtlZeroMemory(Guid,sizeof(GUID));
}

//
// check SRAT acpi table
//
BOOLEAN Blx86NeedPaeForHotPlugMemory()
{
	PACPI_DESCRIPTION_HEADER Header						= BlFindACPITable(ACPI_SRAT_SIGNATURE,sizeof(ACPI_SRAT));
	if(!Header)
		return FALSE;

	Header												= BlFindACPITable(ACPI_SRAT_SIGNATURE,Header->Length);
	if(!Header)
		return FALSE;

	PACPI_MEMORY_AFFINITY Entry							= Add2Ptr(Header,sizeof(ACPI_SRAT),PACPI_MEMORY_AFFINITY);
	PACPI_MEMORY_AFFINITY EndOfTable					= Add2Ptr(Header,Header->Length,PACPI_MEMORY_AFFINITY);

	while(Entry < EndOfTable)
	{
		//
		// memory type == 1
		//
		if(Entry->Type == 1 && Entry->Enabled && Entry->HotPluggable && Entry->MemoryLength + Entry->BaseAddress > 1)
			return TRUE;

		Entry											= Add2Ptr(Entry,Entry->Length,PACPI_MEMORY_AFFINITY);
	}

	return FALSE;
}