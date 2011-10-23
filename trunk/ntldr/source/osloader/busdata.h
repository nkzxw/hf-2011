//********************************************************************
//	created:	21:8:2008   20:34
//	file:		busdata.h
//	author:		tiamo
//	purpose:	get set bus data
//********************************************************************

#pragma once

//
// get bus data by offset
//
ULONG HalGetBusDataByOffset(__in BUS_DATA_TYPE BusDataType,__in ULONG BusNumber,__in ULONG Slot,__in PVOID Buffer,__in ULONG Offset,__in ULONG Length);

//
// get bus data
//
ULONG HalGetBusData(__in BUS_DATA_TYPE BusDataType,__in ULONG BusNumber,__in ULONG SlotNumber,__in PVOID Buffer,__in ULONG Length);

//
// set bus data by offset
//
ULONG HalSetBusDataByOffset(__in BUS_DATA_TYPE BusDataType,__in ULONG BusNumber,__in ULONG Slot,__in PVOID Buffer,__in ULONG Offset,__in ULONG Length);

//
// set bus data
//
ULONG HalSetBusData(__in BUS_DATA_TYPE BusDataType,__in ULONG BusNumber,__in ULONG SlotNumber,__in PVOID Buffer,__in ULONG Length);

//
// translate bus address
//
BOOLEAN HalTranslateBusAddress(__in INTERFACE_TYPE InterfaceType,__in ULONG BusNumber,__in PHYSICAL_ADDRESS BusAddress,
							   __inout PULONG AddressSpace,__out PPHYSICAL_ADDRESS TranslatedAddress);