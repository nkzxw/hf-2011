//********************************************************************
//	created:	21:8:2008   3:55
//	file:		pcibus.cpp
//	author:		tiamo
//	purpose:	pci bus
//********************************************************************

#include "stdafx.h"

UCHAR													PCIDeref[4][4] = {{0,1,2,2},{1,1,1,1},{2,1,2,2},{1,1,1,1}};

//
// sync type1
//
VOID HalpPCISynchronizeType1(__in PBUSHANDLER BusHandler,__in PCI_SLOT_NUMBER Slot,__in PUCHAR Irql,__in PPCI_TYPE1_CFG_BITS PciCfg1)
{
	//
	// initialize PciCfg1
	//
	PciCfg1->u.AsULONG									= 0;
	PciCfg1->u.bits.BusNumber							= BusHandler->BusNumber;
	PciCfg1->u.bits.DeviceNumber						= Slot.u.bits.DeviceNumber;
	PciCfg1->u.bits.FunctionNumber						= Slot.u.bits.FunctionNumber;
	PciCfg1->u.bits.Enable								= TRUE;
}

//
// release type1
//
VOID HalpPCIReleaseSynchronzationType1(__in PBUSHANDLER BusHandler,__in UCHAR Irql)
{
	//
	// disable PCI configuration space
	//
	PCI_TYPE1_CFG_BITS  PciCfg1;
	PciCfg1.u.AsULONG									= 0;
	PPCIPBUSDATA BusData								= static_cast<PPCIPBUSDATA>(BusHandler->BusData);

	WRITE_PORT_ULONG(BusData->Config.Type1.Address,PciCfg1.u.AsULONG);
}

//
// read byte type1
//
ULONG HalpPCIReadUcharType1(__in PPCIPBUSDATA BusData,__in PPCI_TYPE1_CFG_BITS PciCfg1,__in PVOID Buffer,__in ULONG Offset)
{
	ULONG i												= Offset % sizeof(ULONG);
	PciCfg1->u.bits.RegisterNumber						= Offset / sizeof(ULONG);

	WRITE_PORT_ULONG(BusData->Config.Type1.Address,PciCfg1->u.AsULONG);

	*static_cast<PUCHAR>(Buffer)						= READ_PORT_UCHAR(Add2Ptr(BusData->Config.Type1.Data,i,PUCHAR));

	return sizeof(UCHAR);
}

//
// read USHORT type1
//
ULONG HalpPCIReadUshortType1(__in PPCIPBUSDATA BusData,__in PPCI_TYPE1_CFG_BITS PciCfg1,__in PVOID Buffer,__in ULONG Offset)
{
	ULONG i												= Offset % sizeof(ULONG);
	PciCfg1->u.bits.RegisterNumber						= Offset / sizeof(ULONG);

	WRITE_PORT_ULONG(BusData->Config.Type1.Address,PciCfg1->u.AsULONG);

	*static_cast<PUSHORT>(Buffer)						= READ_PORT_USHORT(Add2Ptr(BusData->Config.Type1.Data,i,PUSHORT));

	return sizeof(USHORT);
}

//
// read ULONG type1
//
ULONG HalpPCIReadUlongType1(__in PPCIPBUSDATA BusData,__in PPCI_TYPE1_CFG_BITS PciCfg1,__in PVOID Buffer,__in ULONG Offset)
{
	PciCfg1->u.bits.RegisterNumber						= Offset / sizeof(ULONG);

	WRITE_PORT_ULONG(BusData->Config.Type1.Address, PciCfg1->u.AsULONG);

	*static_cast<PULONG>(Buffer)						= READ_PORT_ULONG(reinterpret_cast<PULONG>(BusData->Config.Type1.Data));

	return sizeof(ULONG);
}

//
// write BYTE type1
//
ULONG HalpPCIWriteUcharType1(__in PPCIPBUSDATA BusData,__in PPCI_TYPE1_CFG_BITS PciCfg1,__in PVOID Buffer,__in ULONG Offset)
{
	ULONG i												= Offset % sizeof(ULONG);
	PciCfg1->u.bits.RegisterNumber						= Offset / sizeof(ULONG);

	WRITE_PORT_ULONG(BusData->Config.Type1.Address,PciCfg1->u.AsULONG);

	WRITE_PORT_UCHAR(Add2Ptr(BusData->Config.Type1.Data,i,PUCHAR),*static_cast<PUCHAR>(Buffer));

	return sizeof(UCHAR);
}

//
// write USHORT type1
//
ULONG HalpPCIWriteUshortType1(__in PPCIPBUSDATA BusData,__in PPCI_TYPE1_CFG_BITS PciCfg1,__in PVOID Buffer,__in ULONG Offset)
{
	ULONG i												= Offset % sizeof(ULONG);
	PciCfg1->u.bits.RegisterNumber						= Offset / sizeof(ULONG);

	WRITE_PORT_ULONG(BusData->Config.Type1.Address,PciCfg1->u.AsULONG);

	WRITE_PORT_USHORT(Add2Ptr(BusData->Config.Type1.Data,i,PUSHORT),*static_cast<PUSHORT>(Buffer));

	return sizeof(UCHAR);
}

//
// write ULONG type1
//
ULONG HalpPCIWriteUlongType1(__in PPCIPBUSDATA BusData,__in PPCI_TYPE1_CFG_BITS PciCfg1,__in PVOID Buffer,__in ULONG Offset)
{
	PciCfg1->u.bits.RegisterNumber						= Offset / sizeof(ULONG);

	WRITE_PORT_ULONG(BusData->Config.Type1.Address,PciCfg1->u.AsULONG);
	WRITE_PORT_ULONG(reinterpret_cast<PULONG>(BusData->Config.Type1.Data), *static_cast<PULONG>(Buffer));

	return sizeof(ULONG);
}

//
// sync type2
//
VOID HalpPCISynchronizeType2(__in PBUSHANDLER BusHandler,__in PCI_SLOT_NUMBER Slot,__in PUCHAR Irql,__in PPCI_TYPE2_ADDRESS_BITS PciCfg2Addr)
{
	PPCIPBUSDATA BusData								= static_cast<PPCIPBUSDATA>(BusHandler->BusData);

	//
	// initialize Cfg2Addr
	//
	PCI_TYPE2_CSE_BITS PciCfg2Cse;
	PciCfg2Addr->u.AsUSHORT								= 0;
	PciCfg2Addr->u.bits.Agent							= static_cast<USHORT>(Slot.u.bits.DeviceNumber);
	PciCfg2Addr->u.bits.AddressBase						= static_cast<USHORT>(BusData->Config.Type2.Base);

	PciCfg2Cse.u.AsUCHAR								= 0;
	PciCfg2Cse.u.bits.Enable							= TRUE;
	PciCfg2Cse.u.bits.FunctionNumber					= static_cast<UCHAR>(Slot.u.bits.FunctionNumber);
	PciCfg2Cse.u.bits.Key								= 0xff;

	//
	// select bus & enable type 2 configuration space
	//
	WRITE_PORT_UCHAR(BusData->Config.Type2.Forward,static_cast<UCHAR>(BusHandler->BusNumber));
	WRITE_PORT_UCHAR(BusData->Config.Type2.CSE,PciCfg2Cse.u.AsUCHAR);
}

//
// release type2
//
VOID HalpPCIReleaseSynchronzationType2(__in PBUSHANDLER BusHandler,__in UCHAR Irql)
{
	PPCIPBUSDATA BusData								= static_cast<PPCIPBUSDATA>(BusHandler->BusData);

	//
	// disable PCI configuration space
	//
	WRITE_PORT_UCHAR(BusData->Config.Type2.CSE,0);
}

//
// read BYTE type2
//
ULONG HalpPCIReadUcharType2(__in PPCIPBUSDATA BusData,__in PPCI_TYPE2_ADDRESS_BITS PciCfg2Addr,__in PVOID Buffer,__in ULONG Offset)
{
	PciCfg2Addr->u.bits.RegisterNumber					= static_cast<USHORT>(Offset);

	*static_cast<PUCHAR>(Buffer)						= READ_PORT_UCHAR(reinterpret_cast<PUCHAR>(PciCfg2Addr->u.AsUSHORT));

	return sizeof(UCHAR);
}

//
// read USHORT type2
//
ULONG HalpPCIReadUshortType2(__in PPCIPBUSDATA BusData,__in PPCI_TYPE2_ADDRESS_BITS PciCfg2Addr,__in PVOID Buffer,__in ULONG Offset)
{
	PciCfg2Addr->u.bits.RegisterNumber					= static_cast<USHORT>(Offset);

	*static_cast<PUSHORT>(Buffer)						= READ_PORT_USHORT(reinterpret_cast<PUSHORT>(PciCfg2Addr->u.AsUSHORT));

	return sizeof(USHORT);
}

//
// read ULONG type2
//
ULONG HalpPCIReadUlongType2(__in PPCIPBUSDATA BusData,__in PPCI_TYPE2_ADDRESS_BITS PciCfg2Addr,__in PVOID Buffer,__in ULONG Offset)
{
	PciCfg2Addr->u.bits.RegisterNumber					= static_cast<USHORT>(Offset);

	*static_cast<PULONG>(Buffer)						= READ_PORT_ULONG(reinterpret_cast<PULONG>(PciCfg2Addr->u.AsUSHORT));

	return sizeof(ULONG);
}

//
// write BYTE type2
//
ULONG HalpPCIWriteUcharType2(__in PPCIPBUSDATA BusData,__in PPCI_TYPE2_ADDRESS_BITS PciCfg2Addr,__in PVOID Buffer,__in ULONG Offset)
{
	PciCfg2Addr->u.bits.RegisterNumber					= static_cast<USHORT>(Offset);

	WRITE_PORT_UCHAR(reinterpret_cast<PUCHAR>(PciCfg2Addr->u.AsUSHORT),*static_cast<PUCHAR>(Buffer));

	return sizeof(UCHAR);
}

//
// write USHORT type2
//
ULONG HalpPCIWriteUshortType2(__in PPCIPBUSDATA BusData,__in PPCI_TYPE2_ADDRESS_BITS PciCfg2Addr,__in PVOID Buffer,__in ULONG Offset)
{
	PciCfg2Addr->u.bits.RegisterNumber					= static_cast<USHORT>(Offset);

	WRITE_PORT_USHORT(reinterpret_cast<PUSHORT>(PciCfg2Addr->u.AsUSHORT),*static_cast<PUSHORT>(Buffer));

	return sizeof(USHORT);
}

//
// write ULONG type2
//
ULONG HalpPCIWriteUlongType2(__in PPCIPBUSDATA BusData,__in PPCI_TYPE2_ADDRESS_BITS PciCfg2Addr,__in PVOID Buffer,__in ULONG Offset)
{
	PciCfg2Addr->u.bits.RegisterNumber					= static_cast<USHORT>(Offset);

	WRITE_PORT_ULONG(reinterpret_cast<PULONG>(PciCfg2Addr->u.AsUSHORT),*static_cast<PULONG>(Buffer));

	return sizeof(ULONG);
}

//
// type1 config handler
//
CONFIG_HANDLER											PCIConfigHandlers =
{
	reinterpret_cast<PPCI_CONFIG_HANDLER_SYNC>(&HalpPCISynchronizeType1),
	reinterpret_cast<PPCI_CONFIG_HANDLER_RELEASE>(&HalpPCIReleaseSynchronzationType1),
	reinterpret_cast<PPCI_CONFIG_HANDLER_IO>(&HalpPCIReadUlongType1),
	reinterpret_cast<PPCI_CONFIG_HANDLER_IO>(&HalpPCIReadUcharType1),
	reinterpret_cast<PPCI_CONFIG_HANDLER_IO>(&HalpPCIReadUshortType1),
	reinterpret_cast<PPCI_CONFIG_HANDLER_IO>(&HalpPCIWriteUlongType1),
	reinterpret_cast<PPCI_CONFIG_HANDLER_IO>(&HalpPCIWriteUcharType1),
	reinterpret_cast<PPCI_CONFIG_HANDLER_IO>(&HalpPCIWriteUshortType1),
};

//
// type2 config handler
//
CONFIG_HANDLER											PCIConfigHandlersType2 =
{
	reinterpret_cast<PPCI_CONFIG_HANDLER_SYNC>(&HalpPCISynchronizeType2),
	reinterpret_cast<PPCI_CONFIG_HANDLER_RELEASE>(&HalpPCIReleaseSynchronzationType2),
	reinterpret_cast<PPCI_CONFIG_HANDLER_IO>(&HalpPCIReadUlongType2),
	reinterpret_cast<PPCI_CONFIG_HANDLER_IO>(&HalpPCIReadUcharType2),
	reinterpret_cast<PPCI_CONFIG_HANDLER_IO>(&HalpPCIReadUshortType2),
	reinterpret_cast<PPCI_CONFIG_HANDLER_IO>(&HalpPCIWriteUlongType2),
	reinterpret_cast<PPCI_CONFIG_HANDLER_IO>(&HalpPCIWriteUcharType2),
	reinterpret_cast<PPCI_CONFIG_HANDLER_IO>(&HalpPCIWriteUshortType2),
};

//
// bus handler
//
BUSHANDLER												PCIBusHandler;

//
// max devices per bus
//
ULONG													PCIMaxDevice = PCI_MAX_DEVICES;

//
// initialize pci bus
//
VOID HalpInitializePciBus()
{
	PBUSHANDLER Bus										= &PCIBusHandler;
	PCIBusHandler.BusData								= &PCIBusHandler.theBusData;

	//
	// not found
	//
	PPCI_REGISTRY_INFO PCIRegInfo						= 0;

	//
	// search from beginning
	//
	PCONFIGURATION_COMPONENT_DATA ConfigData			= 0;

	do
	{
		//
		// search next adapter
		//
		ConfigData										= KeFindConfigurationNextEntry(FwConfigurationTree,AdapterClass,MultiFunctionAdapter,0,&ConfigData);

		//
		// PCI info not found
		//
		if(!ConfigData)
			return;

		//
		// not pci
		//
		if(!ConfigData->ComponentEntry.Identifier || _stricmp(ConfigData->ComponentEntry.Identifier,"PCI"))
			continue;

		//
		// search reg info
		//
		PCIRegInfo										= 0;
		PCM_PARTIAL_RESOURCE_LIST List					= static_cast<PCM_PARTIAL_RESOURCE_LIST>(ConfigData->ConfigurationData);
		PCM_PARTIAL_RESOURCE_DESCRIPTOR CmResDesc		= List->PartialDescriptors;
		for(ULONG i = 0; i < List->Count; i ++)
		{
			if(CmResDesc->Type == CmResourceTypeDeviceSpecific)
			{
				PCIRegInfo								= Add2Ptr(CmResDesc,sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR),PPCI_REGISTRY_INFO);
				break;
			}

			CmResDesc									+= 1;
		}
	}while(!PCIRegInfo);

	//
	// PCIRegInfo describes the system's PCI support as indicated by the BIOS.
	//
	ULONG HwType										= PCIRegInfo->HardwareMechanism & 0xf;

	switch(HwType)
	{
		//
		// this is the default case
		//
	case 1:
		PCIMaxDevice									= PCI_MAX_DEVICES;
		break;

		//
		// type2 does not work MP, nor does the default type2 support more the 0xf device slots
		//
	case 2:
		RtlMoveMemory(&PCIConfigHandlers,&PCIConfigHandlersType2,sizeof(PCIConfigHandlersType2));
		PCIMaxDevice									= 0x10;
		break;

		//
		// unsupport type
		//
	default:
		PCIRegInfo->NoBuses								= 0;
	}

	//
	// set bus count
	//
	PCIBusHandler.NoBuses								= PCIRegInfo->NoBuses;

	//
	// setup port value
	//
	if(PCIRegInfo->NoBuses)
	{
		PPCIPBUSDATA BusData							= static_cast<PPCIPBUSDATA>(Bus->BusData);
		switch(HwType)
		{
		case 1:
			BusData->Config.Type1.Address				= reinterpret_cast<PULONG>(0xcf8);
			BusData->Config.Type1.Data					= 0xcfc;
			break;

		case 2:
			BusData->Config.Type2.CSE					= reinterpret_cast<PUCHAR>(0xcf8);
			BusData->Config.Type2.Forward				= reinterpret_cast<PUCHAR>(0xcfa);
			BusData->Config.Type2.Base					= 0x0c;
			break;
		}
	}
}

//
// get bus handler
//
PBUSHANDLER HalpGetPciBusHandler(__in ULONG BusNumber)
{
	if(!PCIBusHandler.BusData)
		HalpInitializePciBus();

	if(BusNumber >= PCIBusHandler.NoBuses)
		return 0;

	PCIBusHandler.BusNumber								= BusNumber;

	return &PCIBusHandler;
}

//
// read/write pci config space
//
VOID HalpPCIConfig(__in PBUSHANDLER BusHandler,__in PCI_SLOT_NUMBER Slot,__in PUCHAR Buffer,__in ULONG Offset,__in ULONG Length,__in PPCI_CONFIG_HANDLER_IO* ConfigIO)
{
	PPCIPBUSDATA BusData								= static_cast<PPCIPBUSDATA>(BusHandler->BusData);
	UCHAR State[20];
	UCHAR OldIrql;
	PCIConfigHandlers.Synchronize(BusHandler,Slot,&OldIrql,State);

	while(Length)
	{
		ULONG i											= PCIDeref[Offset % sizeof(ULONG)][Length % sizeof(ULONG)];
		i												= ConfigIO[i](BusData, State, Buffer, Offset);

		Offset											+= i;
		Buffer											+= i;
		Length											-= i;
	}

	PCIConfigHandlers.ReleaseSynchronzation(BusHandler,OldIrql);
}

//
// validate slot number
//
BOOLEAN HalpValidPCISlot(__in PBUSHANDLER BusHandler,__in PCI_SLOT_NUMBER Slot);

//
// read pci config
//
VOID HalpReadPCIConfig(__in PBUSHANDLER BusHandler,__in PCI_SLOT_NUMBER Slot,__in PVOID Buffer,__in ULONG Offset,__in ULONG Length)
{
	//
	// validate params
	//
	if(!HalpValidPCISlot(BusHandler,Slot))
	{
		//
		// invalid slotid return no data
		//
		RtlFillMemory(Buffer,Length,0xff);

		return;
	}

	HalpPCIConfig(BusHandler,Slot,static_cast<PUCHAR>(Buffer),Offset,Length,PCIConfigHandlers.ConfigRead);
}

//
// write pci config
//
VOID HalpWritePCIConfig(__in PBUSHANDLER BusHandler,__in PCI_SLOT_NUMBER Slot,__in PVOID Buffer,__in ULONG Offset,__in ULONG Length)
{
	//
	// invalid slotid do nothing
	//
	if(!HalpValidPCISlot (BusHandler, Slot))
		return;

	HalpPCIConfig(BusHandler,Slot,static_cast<PUCHAR>(Buffer),Offset,Length,PCIConfigHandlers.ConfigWrite);
}

//
// validate slot number
//
BOOLEAN HalpValidPCISlot(__in PBUSHANDLER BusHandler,__in PCI_SLOT_NUMBER Slot)
{
	//
	// reserved bits should be zero
	//
	if(Slot.u.bits.Reserved != 0)
		return FALSE;

	//
	// device number should less than devices per bus
	//
	if(Slot.u.bits.DeviceNumber >= PCIMaxDevice)
		return FALSE;

	//
	// zero function number is always ok
	//
	if(Slot.u.bits.FunctionNumber == 0)
		return TRUE;

	//
	// non zero function numbers are only supported if the device has the PCI_MULTIFUNCTION bit set in it's header
	//
	ULONG i												= Slot.u.bits.DeviceNumber;

	//
	// read (dev,0) to determine if the PCI supports multifunction devices
	//
	PCI_SLOT_NUMBER Slot2								= Slot;
	Slot2.u.bits.FunctionNumber							= 0;
	UCHAR HeaderType									= 0;
	HalpReadPCIConfig(BusHandler,Slot2,&HeaderType,FIELD_OFFSET(PCI_COMMON_CONFIG,HeaderType),sizeof(UCHAR));

	//
	// does not exist or not a multifunction device
	//
	if(HeaderType == 0xff || !(HeaderType & PCI_MULTIFUNCTION))
		return FALSE;

	return TRUE;
}

//
// get pci bus data
//
ULONG HalpGetPCIData(__in ULONG BusNumber,__in PCI_SLOT_NUMBER Slot,__in PVOID Buffer,__in ULONG Offset,__in ULONG Length)
{
	//
	// get bus handler
	//
	PBUSHANDLER BusHandler								= HalpGetPciBusHandler(BusNumber);
	if(!BusHandler)
		return 0;

	//
	// if length is bigger than common config,truncate it
	//
	if (Length > sizeof(PCI_COMMON_CONFIG))
		Length											= sizeof(PCI_COMMON_CONFIG);

	ULONG Len											= 0;
	UCHAR LocalBuffer[PCI_COMMON_HDR_LENGTH];
	PPCI_COMMON_CONFIG PciData							= reinterpret_cast<PPCI_COMMON_CONFIG>(LocalBuffer);

	if(Offset >= PCI_COMMON_HDR_LENGTH)
	{
		//
		// the user did not request any data from the common header.
		// verify the PCI device exists, then continue in the device specific area.
		//
		HalpReadPCIConfig(BusHandler,Slot,PciData,0,sizeof(ULONG));

		if(PciData->VendorID == PCI_INVALID_VENDORID || PciData->VendorID == 0)
			return 0;
	}
	else
	{
		//
		// caller requested at least some data within the common header.
		// read the whole header, effect the fields we need to and then copy the user's requested bytes from the header
		//
		Len												= PCI_COMMON_HDR_LENGTH;
		HalpReadPCIConfig(BusHandler,Slot,PciData,0,Len);

		//
		// if the device does not exist or this is a bridge,return invalid id
		//
		if(PciData->VendorID == PCI_INVALID_VENDORID || PciData->VendorID == 0 || PCI_CONFIGURATION_TYPE(PciData))
		{
			PciData->VendorID							= PCI_INVALID_VENDORID;

			//
			// only return invalid id
			//
			Len											= 2;
		}

		//
		// has this PCI device been configured?
		// noop for osloader
		//
		//BusData											= static_cast<PPCIPBUSDATA>(BusHandler->BusData);
		//HalpPCIPin2Line(BusHandler,RootHandler,Slot,PciData);

		//
		// copy whatever data overlaps into the callers buffer
		//
		if(Len < Offset)
			return 0;

		Len												-= Offset;
		if(Len > Length)
			Len											= Length;

		RtlMoveMemory(Buffer,LocalBuffer + Offset,Len);

		Offset											+= Len;
		Buffer											= Add2Ptr(Buffer,Len,PVOID);
		Length											-= Len;
	}

	//
	// read device specific area
	//
	if(Length && Offset >= PCI_COMMON_HDR_LENGTH)
	{
		//
		// the remaining Buffer comes from the Device Specific
		// area - put on the kitten gloves and read from it.
		//
		// specific read/writes to the PCI device specific area are guarenteed:
		//
		//	not to read/write any byte outside the area specified by the caller.
		//	(this may cause WORD or BYTE references to the area in order to read the non-dword aligned ends of the request)
		//
		//	to use a WORD access if the requested length is exactly a WORD long & WORD aligned.
		//
		//  to use a BYTE access if the requested length is exactly a BYTE long.
		//
		HalpReadPCIConfig(BusHandler,Slot,Buffer,Offset,Length);

		Len												+= Length;
	}

	return Len;
}

//
// set pci data
//
ULONG HalpSetPCIData(__in ULONG BusNumber,__in PCI_SLOT_NUMBER Slot,__in PVOID Buffer,__in ULONG Offset,__in ULONG Length)
{
	//
	// get bus handler
	//
	PBUSHANDLER BusHandler								= HalpGetPciBusHandler(BusNumber);
	if(!BusHandler)
		return 0;

	//
	// if length is bigger than common config,truncate it
	//
	if (Length > sizeof(PCI_COMMON_CONFIG))
		Length											= sizeof(PCI_COMMON_CONFIG);

	UCHAR LocalBuffer1[PCI_COMMON_HDR_LENGTH];
	UCHAR LocalBuffer2[PCI_COMMON_HDR_LENGTH];
	ULONG Len											= 0;
	PPCI_COMMON_CONFIG PciData							= reinterpret_cast<PPCI_COMMON_CONFIG>(LocalBuffer1);
	PPCI_COMMON_CONFIG PciData2							= reinterpret_cast<PPCI_COMMON_CONFIG>(LocalBuffer2);

	if(Offset >= PCI_COMMON_HDR_LENGTH)
	{
		//
		// the user did not request any data from the common header.
		// verify the PCI device exists, then continue in the device specific area.
		//
		HalpReadPCIConfig(BusHandler,Slot,PciData,0,sizeof(ULONG));

		if (PciData->VendorID == PCI_INVALID_VENDORID || PciData->VendorID == 0)
			return 0;
	}
	else
	{
		//
		// caller requested to set at least some data within the common header.
		//
		Len												= PCI_COMMON_HDR_LENGTH;
		HalpReadPCIConfig(BusHandler,Slot,PciData,0,Len);

		//
		// if the device does not exist or this is a bridge,nothing to write
		//
		if(PciData->VendorID == PCI_INVALID_VENDORID || PciData->VendorID == 0 || PCI_CONFIGURATION_TYPE(PciData))
			return 0;

		//
		// copy COMMON_HDR values to buffer2, then overlay callers changes.
		//
		RtlMoveMemory(LocalBuffer2,LocalBuffer1,Len);

		Len												-= Offset;
		if(Len > Length)
			Len											= Length;

		RtlMoveMemory(LocalBuffer2 + Offset,Buffer,Len);

		//
		//
		// set this device as configured
		//
		// PPCIPBUSDATA BusData							= static_cast<PPCIPBUSDATA>(BusHandler->BusData);
		//
		// in case interrupt line or pin was editted
		// noop for osloader
		//
		//HalpPCILine2Pin(BusHandler,RootHandler,Slot,PciData2,PciData);

		//
		// set new PCI configuration
		//
		HalpWritePCIConfig(BusHandler,Slot,LocalBuffer2 + Offset,Offset,Len);

		Offset											+= Len;
		Buffer											= Add2Ptr(Buffer,Len,PVOID);
		Length											-= Len;
	}

	//
	// write device specific area
	//
	if(Length && Offset >= PCI_COMMON_HDR_LENGTH)
	{
		HalpWritePCIConfig(BusHandler,Slot,Buffer,Offset,Length);

		Len												+= Length;
	}

	return Len;
}