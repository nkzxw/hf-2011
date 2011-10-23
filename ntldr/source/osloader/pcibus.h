//********************************************************************
//	created:	21:8:2008   4:19
//	file:		pcibus.h
//	author:		tiamo
//	purpose:	pci bus
//********************************************************************

#pragma once

//
// registry info,in the firmware config tree
//
typedef struct _PCI_REGISTRY_INFO
{
	//
	// major revision
	//
	UCHAR												MajorRevision;

	//
	// minor revision
	//
	UCHAR												MinorRevision;

	//
	// number of buses
	//
	UCHAR												NoBuses;

	//
	// config type
	//
	UCHAR												HardwareMechanism;
}PCI_REGISTRY_INFO,*PPCI_REGISTRY_INFO;

//
// type1 config bits
//
typedef struct _PCI_TYPE1_CFG_BITS
{
	union
	{
		struct
		{
			//
			// reserved
			//
			ULONG										Reserved1		: 2;

			//
			// register
			//
			ULONG										RegisterNumber	: 6;

			//
			// function
			//
			ULONG										FunctionNumber	: 3;

			//
			// device
			//
			ULONG										DeviceNumber	: 5;

			//
			// bus
			//
			ULONG										BusNumber		: 8;

			//
			// reserved
			//
			ULONG										Reserved2		: 7;

			//
			// enable
			//
			ULONG										Enable			: 1;
		}bits;

		//
		// as long
		//
		ULONG											AsULONG;
	}u;
}PCI_TYPE1_CFG_BITS,*PPCI_TYPE1_CFG_BITS;

//
// cse bits
//
typedef struct _PCI_TYPE2_CSE_BITS
{
	union
	{
		struct
		{
			//
			// enable
			//
			UCHAR										Enable			: 1;

			//
			// function
			//
			UCHAR										FunctionNumber	: 3;

			//
			// key
			//
			UCHAR										Key				: 4;
		}bits;

		//
		// as BYTE
		//
		UCHAR											AsUCHAR;
	}u;
}PCI_TYPE2_CSE_BITS,*PPCI_TYPE2_CSE_BITS;

//
// type2 address
//
typedef struct _PCI_TYPE2_ADDRESS_BITS
{
	union
	{
		struct
		{
			//
			// register
			//
			USHORT										RegisterNumber	: 8;

			//
			// agent
			//
			USHORT										Agent			: 4;

			//
			// address base
			//
			USHORT										AddressBase		: 4;
		}bits;

		//
		// ushort
		//
		USHORT											AsUSHORT;
	}u;
}PCI_TYPE2_ADDRESS_BITS,*PPCI_TYPE2_ADDRESS_BITS;

//
// pci bus private data
//
typedef struct tagPCIPBUSDATA
{
	union
	{
		struct
		{
			//
			// type1 address cf8
			//
			PULONG										Address;

			//
			// type1 data cfc
			//
			ULONG										Data;
		}Type1;

		struct
		{
			//
			// cse
			//
			PUCHAR										CSE;

			//
			// forward
			//
			PUCHAR										Forward;

			//
			// base
			//
			ULONG										Base;
		}Type2;
	}Config;
}PCIPBUSDATA,*PPCIPBUSDATA;

//
// bus handler
//
typedef struct
{
	//
	// number of buses
	//
	ULONG												NoBuses;

	//
	// bus number
	//
	ULONG												BusNumber;

	//
	// bus data
	//
	PVOID												BusData;

	//
	// used by ntldr internal
	//
	PCIPBUSDATA											theBusData;
}BUSHANDLER,*PBUSHANDLER;

//
// config io
//
typedef ULONG (*PPCI_CONFIG_HANDLER_IO)(__in PPCIPBUSDATA BusData,__in PVOID State,__in PVOID Buffer,__in ULONG Offset);

//
// start config
//
typedef VOID (*PPCI_CONFIG_HANDLER_SYNC)(__in PBUSHANDLER BusHandler,__in PCI_SLOT_NUMBER Slot,__in PUCHAR Irql,__in PVOID State);

//
// end config
//
typedef VOID (*PPCI_CONFIG_HANDLER_RELEASE)(__in PBUSHANDLER BusHandler,__in UCHAR Irql);

//
// config handler
//
typedef struct
{
	//
	// start
	//
	PPCI_CONFIG_HANDLER_SYNC							Synchronize;

	//
	// end
	//
	PPCI_CONFIG_HANDLER_RELEASE							ReleaseSynchronzation;

	//
	// read BYTE,USHORT,ULONG
	//
	PPCI_CONFIG_HANDLER_IO								ConfigRead[3];

	//
	// write BYTE,USHORT,ULONG
	//
	PPCI_CONFIG_HANDLER_IO								ConfigWrite[3];
}CONFIG_HANDLER,*PCONFIG_HANDLER;

//
// get pci bus data
//
ULONG HalpGetPCIData(__in ULONG BusNumber,__in PCI_SLOT_NUMBER Slot,__in PVOID Buffer,__in ULONG Offset,__in ULONG Length);

//
// set pci data
//
ULONG HalpSetPCIData(__in ULONG BusNumber,__in PCI_SLOT_NUMBER Slot,__in PVOID Buffer,__in ULONG Offset,__in ULONG Length);