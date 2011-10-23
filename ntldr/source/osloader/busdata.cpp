//********************************************************************
//	created:	21:8:2008   19:47
//	file:		busdata.cpp
//	author:		tiamo
//	purpose:	get set bus data
//********************************************************************

#include "stdafx.h"

//
// get cmos data
//
ULONG HalpGetCmosData(__in ULONG SourceLocation,__in ULONG SourceAddress,__in PVOID Buffer,__in ULONG Length)
{
	PUCHAR ReturnBuffer									= static_cast<PUCHAR>(Buffer);
	ULONG ReadCount										= 0;
	if(SourceLocation == 0)
	{
		//
		// cmos data
		//
		while(Length)
		{
			if(ReadCount > 0xff)
				break;

			//
			// set address
			//
			WRITE_REGISTER_UCHAR(reinterpret_cast<PUCHAR>(0x70),static_cast<UCHAR>(ReadCount));

			//
			// read in data
			//
			ReturnBuffer[ReadCount]						= READ_PORT_UCHAR(reinterpret_cast<PUCHAR>(0x71));

			Length										-= 1;
			ReadCount									+= 1;
		}
	}
	else if(SourceLocation == 1)
	{
		//
		// ecmos data
		//
		while(Length)
		{
			if(ReadCount > 0xffff)
				break;

			//
			// set address
			//
			WRITE_REGISTER_UCHAR(reinterpret_cast<PUCHAR>(0x74),static_cast<UCHAR>(ReadCount & 0xff));
			WRITE_REGISTER_UCHAR(reinterpret_cast<PUCHAR>(0x75),static_cast<UCHAR>(ReadCount >> 8));

			//
			// read in data
			//
			ReturnBuffer[ReadCount]							= READ_PORT_UCHAR(reinterpret_cast<PUCHAR>(0x76));

			Length											-= 1;
			ReadCount										+= 1;
		}
	}

	return ReadCount;
}

//
// get eisa bus data
//
ULONG HalpGetEisaData(__in ULONG BusNumber,__in ULONG SlotNumber,__in PVOID Buffer,__in ULONG Offset,__in ULONG Length)
{
	if(MachineType != 1/*MACHINE_TYPE_EISA*/)
		return 0;

	//
	// search eisa adapter
	//
	PCONFIGURATION_COMPONENT_DATA ConfigData			= KeFindConfigurationEntry(FwConfigurationTree,AdapterClass,EisaAdapter,0);
	if(!ConfigData)
	{
		DbgPrint("HalGetBusData: KeFindConfigurationEntry failed\n");
		return 0;
	}

	//
	// get resource dscriptor
	//
	PCM_PARTIAL_RESOURCE_LIST CmResList					= static_cast<PCM_PARTIAL_RESOURCE_LIST>(ConfigData->ConfigurationData);
	PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResource		= CmResList->PartialDescriptors;
	ULONG PartialCount									= CmResList->Count;
	ULONG DataLength = 0;

	PCM_EISA_SLOT_INFORMATION SlotInformation			= 0;
	ULONG SlotDataSize									= 0;
	BOOLEAN Found										= FALSE;
	ULONG i												= 0;
	for(i = 0; i < PartialCount; i++)
	{
		//
		// for each partial Resource
		//
		switch(PartialResource->Type)
		{
			//
			// we dont care about these.
			//
		case CmResourceTypeNull:
		case CmResourceTypePort:
		case CmResourceTypeInterrupt:
		case CmResourceTypeMemory:
		case CmResourceTypeDma:
			PartialResource								+= 1;
			break;

		default:
			DbgPrint("Bad Data in registry!\n");
			return(0);
			break;

			//
			// found it
			//
		case CmResourceTypeDeviceSpecific:
			{
				ULONG TotalDataSize						= PartialResource->u.DeviceSpecificData.DataSize;

				SlotInformation							= Add2Ptr(PartialResource,sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR),PCM_EISA_SLOT_INFORMATION);

				while(TotalDataSize > 0)
				{
					if(SlotInformation->ReturnCode == EISA_EMPTY_SLOT)
						SlotDataSize					= sizeof(CM_EISA_SLOT_INFORMATION);
					else
						SlotDataSize					= sizeof(CM_EISA_SLOT_INFORMATION) + SlotInformation->NumberFunctions * sizeof(CM_EISA_FUNCTION_INFORMATION);

					if(SlotDataSize > TotalDataSize)
					{
						//
						// Something is wrong again
						//
						DbgPrint("HalGetBusData: SlotDataSize > TotalDataSize\n");
						return 0;
					}

					if(SlotNumber != 0)
					{
						SlotNumber						-= 1;

						SlotInformation					= Add2Ptr(SlotInformation,SlotDataSize,PCM_EISA_SLOT_INFORMATION);

						TotalDataSize					-= SlotDataSize;

						continue;
					}

					//
					// this is our slot
					//
					Found								= TRUE;
					break;
				}

				//
				// end loop
				//
				i											= PartialCount;
			}
			break;
		}
	}

	if(Found)
	{
		//
		// HACK,if the length is zero then the buffer points to a PVOID where the pointer to the data should be stored.
		// this is done in the loader because we quickly run out of heap scaning all of the EISA configuration data.
		//
		if(Length == 0)
		{
			//
			// Return the pointer to the mini-port driver.
			//
			*static_cast<PVOID*>(Buffer)				= SlotInformation;
			return SlotDataSize;
		}

		i												= Length + Offset;
		if(i > SlotDataSize)
			i											= SlotDataSize;

		ULONG DataLength								= i - Offset;
		RtlMoveMemory(Buffer,Add2Ptr(SlotInformation,Offset,PVOID),DataLength);

		return DataLength;
	}

	return 0;
}

//
// get bus data by offset
//
ULONG HalGetBusDataByOffset(__in BUS_DATA_TYPE BusDataType,__in ULONG BusNumber,__in ULONG Slot,__in PVOID Buffer,__in ULONG Offset,__in ULONG Length)
{
	switch(BusDataType)
	{
	case Cmos:
		if(Offset != 0)
			return 0;

		return HalpGetCmosData(BusNumber,Slot,Buffer,Length);
		break;

	case EisaConfiguration:
		return HalpGetEisaData(BusNumber,Slot,Buffer,Offset,Length);
		break;

	case PCIConfiguration:
		PCI_SLOT_NUMBER PciSlot;
		PciSlot.u.AsULONG								= Slot;
		return HalpGetPCIData(BusNumber,PciSlot,Buffer,Offset,Length);
		break;
	}

	return 0;
}

//
// get bus data
//
ULONG HalGetBusData(__in BUS_DATA_TYPE BusDataType,__in ULONG BusNumber,__in ULONG SlotNumber,__in PVOID Buffer,__in ULONG Length)
{
	return HalGetBusDataByOffset(BusDataType,BusNumber,SlotNumber,Buffer,0,Length);
}

//
// set bus data by offset
//
ULONG HalSetBusDataByOffset(__in BUS_DATA_TYPE BusDataType,__in ULONG BusNumber,__in ULONG Slot,__in PVOID Buffer,__in ULONG Offset,__in ULONG Length)
{
	if(BusDataType == PCIConfiguration)
	{
		PCI_SLOT_NUMBER PciSlot;
		PciSlot.u.AsULONG								= Slot;
		return HalpSetPCIData(BusNumber,PciSlot,Buffer,Offset,Length);
	}

	return 0;
}

//
// set bus data
//
ULONG HalSetBusData(__in BUS_DATA_TYPE BusDataType,__in ULONG BusNumber,__in ULONG SlotNumber,__in PVOID Buffer,__in ULONG Length)
{
	return HalSetBusDataByOffset(BusDataType,BusNumber,SlotNumber,Buffer,0,Length);
}

//
// translate bus address
//
BOOLEAN HalTranslateBusAddress(__in INTERFACE_TYPE InterfaceType,__in ULONG BusNumber,__in PHYSICAL_ADDRESS BusAddress,
							   __inout PULONG AddressSpace,__out PPHYSICAL_ADDRESS TranslatedAddress)
{
	if(BusAddress.HighPart)
		return FALSE;

	TranslatedAddress->QuadPart							= BusAddress.QuadPart;

	return TRUE;
}
