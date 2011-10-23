//********************************************************************
//	created:	15:8:2008   21:30
//	file:		dbg1394.cpp
//	author:		tiamo
//	purpose:	debug over 1394
//********************************************************************

#include "stdafx.h"

//
// how does it work?
//	1.ohci controller provides us a function to read write peer's physical memory directly.
//	2.debuggee(us) setups the ohci controller to map his physical memory,and let anyone to be able to read it.
//	3.debuggee also setups a config rom to let the debugger to find detail info used to communicate with it.
//	4.debuggee uses a well-known (bus,node) number.
//	5.debugger(peer, running windbg) uses the well-known (bus,node) number as peer's address and read its config rom.
//	6.debugger decodes the config rom,and check whether it contains a debug info or not.
//	7.if the debugger found a debug info in the config rom,then it assumes that the peer can be debugged
//	8.debuggee uses two memory buffers to communicate with debugger,one for send,the other for receive
//	9.for each memory buffer there is an indicator used to present current buffer state,pending or not
//	10.debuggee also provides those two memory buffers' physical address in the config rom,then the debugger can read/write those buffers too.
//	11.if debuggee wants to send some data to debugger
//		a.the debuggee copies data to the send buffer,and set the send buffer's state to PENDING,means there is data to be read by debugger
//		b.debuggee loops and checks the send buffer's state is PENDING or not,if the state remains PENDING,the debugee will contine looping
//		c.debugger reads debuggee's send buffer's state,if the state is NOT_PENDING,which means there is no data pending,debugger will
//			wait a small time,then reads the state again,if the state is PENDING,it will set the state to NOT_PENDING,then return,otherwise continue waiting
//		d.debuggee will see the state becomes to NOT_PENDING,means that the debugger has already picked the data,send complete.
//	12.if the debugger wants to send some data to debugger
//		a.debugger first writes those data to debuggee's physical memory
//		b.debugger writes the debugee's receive buffer's state to PENDING
//		c.debugger reads the debuggee's receive buffer's state back,if it remains PENDING,it will continue reading the state and wait it to become NOT_PENDING
//		d.debuggee checks his receive buffer's state,if it is PENDING,which means that there is some data sent from the debugger,
//			it will read it,and set state to NOT_PENDING,otherwise return a timeout to his caller,
//			NOTE the debuggee will NOT loop here,it is the caller's responsibility to do the loop check work.
//
// you can see
//	debuggee simply exports his physical memory to ohci controller,and let the others to be able to read it.
//	debuggee will NOT send a 1394 request packet to ohci controller,it just read and write some physical memory directly.
//	after it sends the physical memory mapping info to the ohci controller,debuggee will NOT touch the hardware anymore.(but it will make sure the hardware is enbled)
//	debuggee and debuger use STATE to communicate to each other
//	there is NO such method to be used by data sender to notify the receiver "hey guy,there is something,read it!",it just assumes those data should be read in the later
//	but it can not be told when it will be read,so sender must do a loop to check whether the data has been read or not
//	yes,it is POLL mode,not INTERRUPT mode
//

#include <PshPack1.h>

//
// send buffer
//
typedef union _DBG1394_SEND_BUFFER
{
	struct
	{
		//
		// state
		//
		NTSTATUS											Status;

		//
		// packet header
		//
		KD_PACKET											PacketHeader;

		//
		// data length
		//
		ULONG												DataLength;

		//
		// data buffer
		//
		UCHAR												DataBuffer[PACKET_MAX_SIZE];
	};

	struct
	{
		//
		// padding to page size
		//
		UCHAR												Padding[PAGE_SIZE];
	};
}DBG1394_SEND_BUFFER,*PDBG1394_SEND_BUFFER;

//
// receive buffer
//
typedef union _DBG1394_RECEIVE_BUFFER
{
	struct
	{
		//
		// state
		//
		NTSTATUS											Status;

		//
		// length
		//
		ULONG												TotalLength;

		//
		// packet header
		//
		KD_PACKET											PacketHeader;

		//
		// buffer
		//
		UCHAR												DataBuffer[PACKET_MAX_SIZE - sizeof(KD_PACKET)];
	};

	struct
	{
		//
		// padding to page size
		//
		UCHAR												Padding[PAGE_SIZE];
	};
}DBG1394_RECEIVE_BUFFER,*PDBG1394_RECEIVE_BUFFER;

//
// config info,shared between debuggee and debugger
//
typedef struct _DBG1394_DEBUG_CONFIG_INFO
{
	//
	// signature
	//
	ULONG												Signature;

	//
	// major version
	//
	USHORT												MajorVersion;

	//
	// minor version
	//
	USHORT												MinorVersion;

	//
	// channel
	//
	ULONG												Channel;

	//
	// bus present
	//
	ULONG												BusPresent;

	//
	// send buffer physical address
	//
	ULONG64												SendBuffer;

	//
	// receive buffer physical address
	//
	ULONG64												ReceiveBuffer;
}DBG1394_DEBUG_CONFIG_INFO,*PDBG1394_DEBUG_CONFIG_INFO;

//
// config rom header
//
typedef union _IEEE1394_CONFIG_ROM_INFO
{
	struct
	{
		union
		{
			//
			// cri crc value
			//
			USHORT										CriCrcValue;

			struct
			{
				//
				// saved info length
				//
				UCHAR									CriSavedInfoLength;

				//
				// saved crc length
				//
				UCHAR									CriSavedCrcLength;
			};
		};

		//
		// crc length
		//
		UCHAR											CriCrcLength;

		//
		// info length
		//
		UCHAR											CriInfoLength;
	};

	//
	// whole ULONG
	//
	ULONG   AsULong;
}IEEE1394_CONFIG_ROM_INFO,*PIEEE1394_CONFIG_ROM_INFO;

//
// config rom
//
typedef struct _DBG1394_DEBUG_CONFIG_ROM
{
	//
	// rom header
	//
	IEEE1394_CONFIG_ROM_INFO							Header;

	//
	// signature,'1394'
	//
	ULONG												Singnature;

	//
	// bus option
	//
	ULONG												BusOptions;

	//
	// guid
	//
	ULONG												GlobalUniqueId[2];

	//
	// directory info head
	//
	IEEE1394_CONFIG_ROM_INFO							DirectoryInfoHead;

	//
	// node capabilities
	//
	ULONG												NodeCapabilities;

	//
	// module vendor id
	//
	ULONG												ModuleVendorId;

	//
	// extended key
	//
	ULONG												ExtendedKey;

	//
	// debug key
	//
	ULONG												DebugKey;

	//
	// debug value,debug config info's physical address
	//
	ULONG												DebugConfigInfo;

	//
	// padding
	//
	UCHAR												Padding[212];
}DBG1394_DEBUG_CONFIG_ROM,*PDBG1394_DEBUG_CONFIG_ROM;

//
// debugger data,must be page algined
//
typedef struct _DBG1394_GLOBAL_DATA
{
	//
	// send buffer
	//
	DBG1394_SEND_BUFFER									SendBuffer;

	//
	// receive buffer
	//
	DBG1394_RECEIVE_BUFFER								ReceiveBuffer;

	//
	// config rom
	//
	DBG1394_DEBUG_CONFIG_ROM							DebugConfigRom;

	//
	// debug info
	//
	DBG1394_DEBUG_CONFIG_INFO							DebugConfigInfo;
}DBG1394_GLOBAL_DATA,*PDBG1394_GLOBAL_DATA;

#include <PopPack.h>

//
// ohci controller base address
//
PUCHAR													OhciControllerRegisterBase;

//
// global data
//
PDBG1394_GLOBAL_DATA									Dbg1394GlobalData;

//
// debugger present
//
extern BOOLEAN											BdDebuggerNotPresent;

//
// ctrl+c pending
//
extern BOOLEAN											BdControlCPending;

//
// retries
//
extern ULONG											BdNumberRetries;

//
// default retries count
//
extern ULONG											BdRetryCount;

//
// next send id
//
extern ULONG											BdNextPacketIdToSend;

//
// send control packet
//
extern VOID												(*BdSendControlPacket)(__in ULONG PacketType,__in_opt ULONG PacketId);

//
// receive a packet
//
extern ULONG											(*BdReceivePacket)(__in ULONG PacketType,__out PSTRING Header,__out PSTRING Data,__out PULONG DataLength);

//
// send control packet
//
extern VOID												(*BdSendPacket)(__in ULONG PacketType,__in PSTRING MessageHeader,__in PSTRING MessageData);

//
// compute checksum
//
ULONG BdComputeChecksum(__in PVOID Data,__in ULONG Length);

//
// stall
//
VOID Bd1394StallExecution(__in ULONG Microseconds)
{
	for(ULONG k = 0,b = 1; k < Microseconds; k ++)
	{
		for(ULONG i = 1; i < 100000; i++)
		{
			__asm pause;
			b											= b * (i >> k);
		}
	}
}

//
// crc16
//
ULONG Bd1394Crc16(__in ULONG Data,__in ULONG Check)
{
	ULONG Next											= Check;
	for(LONG Shift = 28; Shift >= 0; Shift -= 4)
	{
		ULONG Sum										= ((Next >> 12) ^ (Data >> Shift)) & 0xf;
		Next											= (Next << 4) ^ (Sum << 12) ^ (Sum << 5) ^ (Sum);
	}

	return Next & 0xFFFF;
}

//
// cacl crc
//
USHORT Bd1394CalculateCrc(__in PVOID Buffer,__in ULONG Count)
{
	ULONG Temp											= 0;
	PULONG Quadlet										= static_cast<PULONG>(Buffer);

	for(ULONG Index = 0; Index < Count; Index++)
		Temp = Bd1394Crc16(Quadlet[Index],Temp);

	return static_cast<USHORT>(Temp);
}

//
// swap ulong
//
ULONG Bd1394SwapULong(__in ULONG Value)
{
	return ((Value & 0xff000000) >> 24) | ((Value & 0x00ff0000) >> 8) | ((Value & 0x0000ff00) << 8) | ((Value & 0x000000ff) << 24);
}

//
// read phy register
//
BOOLEAN Bd1394ReadPhyRegister(__in PUCHAR BaseRegister,__in UCHAR Index,__out PUCHAR Value)
{
	//
	// write phy register index to phy control
	//
	ULONG PhyControl									= ((Index & 0xf) | 0x80) << 8;
	WRITE_REGISTER_ULONG(Add2Ptr(BaseRegister,0xec,PULONG),PhyControl);

	for(ULONG i = 0; i < 400000; i ++)
	{
		//
		// check read done
		//
		ULONG Temp										= READ_REGISTER_ULONG(Add2Ptr(BaseRegister,0xec,PULONG));
		if(Temp & 0x80000000)
		{
			*Value										= static_cast<UCHAR>((Temp >> 0x10) & 0xff);
			return TRUE;
		}
	}

	return FALSE;
}

//
// write phy register
//
BOOLEAN Bd1394WritePhyRegister(__in PUCHAR BaseRegister,__in UCHAR Index,__in UCHAR Value)
{
	//
	// write phy register index to phy control
	//
	ULONG PhyControl									= (((Index & 0xf) | 0x40) << 8) | Value;
	WRITE_REGISTER_ULONG(Add2Ptr(BaseRegister,0xec,PULONG),PhyControl);

	for(ULONG i = 0; i < 400000; i ++)
	{
		//
		// check write done
		//
		if(!(READ_REGISTER_ULONG(Add2Ptr(BaseRegister,0xec,PULONG)) & 0x00004000))
			return TRUE;
	}

	return FALSE;
}

//
// enable physical access
//
VOID Bd1394EnablePhysicalAccess(__in PDBG1394_GLOBAL_DATA Data)
{
	ULONG Temp											= READ_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x50,PULONG));

	//
	// link enabled
	//
	if(!(Temp & 0x20000))
		return;

	//
	// link <-> PHY communication permitted
	//
	if(!(Temp & 0x80000))
		return;

	//
	// soft resetting
	//
	if(Temp & 0x10000)
		return;

	//
	// clear bus reset interrupt
	//
	if(READ_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x80,PULONG)) & 0x20000)
		WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x84,PULONG),0x20000);

	//
	// allow asyn request from any node
	//
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x100,PULONG),0xffffffff);
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x108,PULONG),0xffffffff);

	//
	// allow asyn phy request from any node
	//
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x110,PULONG),0xffffffff);
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x118,PULONG),0xffffffff);
}

//
// read packet
//
ULONG Bd1394ReadPacket(__in PDBG1394_GLOBAL_DATA Data,__out PKD_PACKET PacketHeader,__out_opt PSTRING MessageHeader,__out_opt PSTRING MessageData,__in BOOLEAN Wait)
{
	for(ULONG i = 0; i < 512000; i ++)
	{
		//
		// enable physical access
		//
		Bd1394EnablePhysicalAccess(Data);

		//
		// check receive buffer state
		//
		if(Data->ReceiveBuffer.Status == STATUS_PENDING)
		{
			//
			// some data pending,debugger present
			//
			BdDebuggerNotPresent						= FALSE;

			//
			// read packet header
			//
			RtlCopyMemory(PacketHeader,&Data->ReceiveBuffer.PacketHeader,sizeof(KD_PACKET));

			//
			// packet header is invalid
			//
			if(Data->ReceiveBuffer.TotalLength < sizeof(KD_PACKET))
			{
				//
				// finish
				//
				Data->ReceiveBuffer.Status				= STATUS_SUCCESS;

				return KDP_PACKET_RESEND;
			}

			if(MessageHeader)
			{
				//
				// copy message header
				//
				RtlCopyMemory(MessageHeader->Buffer,Data->ReceiveBuffer.DataBuffer,MessageHeader->MaximumLength);

				//
				// check data buffer
				//
				ULONG CopyLength						= Data->ReceiveBuffer.TotalLength - sizeof(KD_PACKET) - MessageHeader->MaximumLength;
				if(Data->ReceiveBuffer.TotalLength > sizeof(KD_PACKET) + MessageHeader->MaximumLength && MessageData)
					RtlCopyMemory(MessageData->Buffer,Data->ReceiveBuffer.DataBuffer + MessageHeader->MaximumLength,CopyLength);
			}

			//
			// finish
			//
			Data->ReceiveBuffer.Status					= STATUS_SUCCESS;

			return KDP_PACKET_RECEIVED;
		}

		//
		// caller did not want to wait
		//
		if(!Wait)
			return KDP_PACKET_RESEND;
	}

	//
	// timeout
	//
	return KDP_PACKET_TIMEOUT;
}

//
// send control packet
//
VOID Bd1394SendControlPacket(__in ULONG PacketType,__in ULONG PacketId)
{
	//
	// zero send buffer out
	//
	RtlZeroMemory(&Dbg1394GlobalData->SendBuffer,sizeof(Dbg1394GlobalData->SendBuffer));

	//
	// control packet does not have a data buffer
	//
	Dbg1394GlobalData->SendBuffer.DataLength			= 0;

	//
	// setup packet header,the other fields are already zeroed out by RtlZeroMemory,and PacketId is always set to zero in ms's original KD1394.dll
	//
	PKD_PACKET PacketHeader								= &Dbg1394GlobalData->SendBuffer.PacketHeader;
	PacketHeader->PacketLeader							= CONTROL_PACKET_LEADER;
	PacketHeader->PacketType							= static_cast<USHORT>(PacketType);

	//
	// 'send' it without wait
	//
	Dbg1394GlobalData->SendBuffer.Status				= STATUS_PENDING;
}

//
// receive a packet
//
ULONG Bd1394ReceivePacket(__in ULONG PacketType,__out PSTRING MessageHeader,__out PSTRING MessageData,__out PULONG DataLength)
{
	//
	// enable physical access
	//
	Bd1394EnablePhysicalAccess(Dbg1394GlobalData);

	//
	// special case for poll breakin,well,boot loader will never use this,but ....
	//
	if(PacketType == PACKET_TYPE_KD_POLL_BREAKIN)
	{
		//
		// have a pending packet?
		//
		if(Dbg1394GlobalData->ReceiveBuffer.Status != STATUS_PENDING)
			return KDP_PACKET_TIMEOUT;

		//
		// breakin?
		//
		if(static_cast<UCHAR>(Dbg1394GlobalData->ReceiveBuffer.PacketHeader.PacketLeader) != BREAKIN_PACKET_BYTE)
			return KDP_PACKET_TIMEOUT;

		//
		// yes,got it
		//
		BdDebuggerNotPresent							= FALSE;
		Dbg1394GlobalData->ReceiveBuffer.Status			= STATUS_SUCCESS;
		return KDP_PACKET_RECEIVED;
	}

	//
	// read the packet
	//
	KD_PACKET PacketHeader;
	for( ; TRUE ; Bd1394SendControlPacket(PACKET_TYPE_KD_RESEND,0))
	{
		ULONG Ret										= Bd1394ReadPacket(Dbg1394GlobalData,&PacketHeader,MessageHeader,MessageData,TRUE);

		//
		// received something,it has high possibility that kernel debugger is alive,so reset counter
		//
		if(Ret != KDP_PACKET_TIMEOUT)
		{
			BdNumberRetries								= BdRetryCount;
			BdDebuggerNotPresent						= FALSE;
		}

		//
		// failed to read
		//
		if(Ret != KDP_PACKET_RECEIVED)
		{
			//
			// check breakin
			//
			if(static_cast<UCHAR>(PacketHeader.PacketLeader) == BREAKIN_PACKET_BYTE)
			{
				BdControlCPending						= TRUE;
				Ret										= KDP_PACKET_RESEND;
			}

			return Ret;
		}

		//
		// if the packet we received is a resend request, we return true and let caller resend the packet.
		//
		if(PacketHeader.PacketLeader == CONTROL_PACKET_LEADER && PacketHeader.PacketType == PACKET_TYPE_KD_RESEND)
			return KDP_PACKET_RESEND;

		//
		// check length,if length is invalid,send a resend control packet and loop again
		//
		if(PacketHeader.ByteCount > PACKET_MAX_SIZE || PacketHeader.ByteCount < MessageHeader->MaximumLength)
			continue;

		//
		// setup length
		//
		*DataLength										= PacketHeader.ByteCount - MessageHeader->MaximumLength;
		MessageHeader->Length							= MessageHeader->MaximumLength;
		MessageData->Length								= static_cast<USHORT>(*DataLength);

		//
		// not the one we are waiting for
		//
		if(PacketHeader.PacketType != PacketType)
			continue;

		//
		// calc checksum,and compare it
		//
		ULONG Checksum									= BdComputeChecksum(MessageHeader->Buffer,MessageHeader->Length);
		Checksum										+= BdComputeChecksum(MessageData->Buffer,MessageData->Length);

		if(Checksum == PacketHeader.Checksum)
			return KDP_PACKET_RECEIVED;
	}

	return KDP_PACKET_TIMEOUT;
}

//
// send packet
//
VOID Bd1394SendPacket(__in ULONG PacketType,__in PSTRING MessageHeader,__in PSTRING MessageData)
{
	//
	// calc checksum
	//
	KD_PACKET PacketHeader;
	if(MessageData)
	{
		PacketHeader.Checksum							= BdComputeChecksum(MessageData->Buffer,MessageData->Length);
		PacketHeader.ByteCount							= MessageData->Length;
	}
	else
	{
		PacketHeader.Checksum							= 0;
		PacketHeader.ByteCount							= 0;
	}

	//
	// setup packet header
	//
	PacketHeader.Checksum								+= BdComputeChecksum(MessageHeader->Buffer,MessageHeader->Length);
	PacketHeader.PacketType								= static_cast<USHORT>(PacketType);
	PacketHeader.ByteCount								+= MessageHeader->Length;
	PacketHeader.PacketId								= BdNextPacketIdToSend;
	PacketHeader.PacketLeader							= PACKET_LEADER;
	BdNextPacketIdToSend								+= 1;

	//
	// set retry count
	//
	BdNumberRetries										= BdRetryCount;

	//
	// setup send buffer
	//
	PDBG1394_SEND_BUFFER SendBuffer						= &Dbg1394GlobalData->SendBuffer;
	RtlCopyMemory(&SendBuffer->PacketHeader,&PacketHeader,sizeof(KD_PACKET));

	RtlCopyMemory(SendBuffer->DataBuffer,MessageHeader->Buffer,MessageHeader->Length);
	SendBuffer->DataLength								= MessageHeader->Length;

	if(MessageData)
	{
		RtlCopyMemory(SendBuffer->DataBuffer + MessageHeader->Length,MessageData->Buffer,MessageData->Length);
		SendBuffer->DataLength							+= MessageData->Length;
	}

	//
	// and 'send' it
	//
	SendBuffer->Status									= STATUS_PENDING;

	ULONG ReturnCode									= KDP_PACKET_TIMEOUT;
	BOOLEAN ExceptionPacketTimeouted					= FALSE;
	PDBG1394_RECEIVE_BUFFER ReceiveBuffer				= &Dbg1394GlobalData->ReceiveBuffer;

	while(1)
	{
		if(BdNumberRetries == 0)
		{
			PDBGKD_WAIT_STATE_CHANGE64 StateChange		= Add2Ptr(MessageHeader->Buffer,0,PDBGKD_WAIT_STATE_CHANGE64);
			PDBGKD_DEBUG_IO DebugIo						= Add2Ptr(MessageHeader->Buffer,0,PDBGKD_DEBUG_IO);
			PDBGKD_FILE_IO FileIo						= Add2Ptr(MessageHeader->Buffer,0,PDBGKD_FILE_IO);

			//
			// if the packet is not for reporting exception, we give up and declare debugger not present.
			//
			if( (PacketType == PACKET_TYPE_KD_DEBUG_IO && DebugIo->ApiNumber == DbgKdPrintStringApi) ||
				(PacketType == PACKET_TYPE_KD_STATE_CHANGE64 && StateChange->NewState == DbgKdLoadSymbolsStateChange) ||
				(PacketType == PACKET_TYPE_KD_FILE_IO && FileIo->ApiNumber == DbgKdCreateFileApi))
			{
				BdDebuggerNotPresent					= TRUE;
				SendBuffer->Status						= STATUS_SUCCESS;
				return;
			}

			if(PacketType != PACKET_TYPE_KD_DEBUG_IO && PacketType != PACKET_TYPE_KD_STATE_CHANGE64 && PacketType != PACKET_TYPE_KD_FILE_IO)
				ExceptionPacketTimeouted				= TRUE;
		}

		BOOLEAN Done									= FALSE;

		for(ULONG i = 0; i < 512000; i ++)
		{
			//
			// enable physical access
			//
			Bd1394EnablePhysicalAccess(Dbg1394GlobalData);

			//
			// debugger read this packet? or debugger gave us a new command?
			//
			if(SendBuffer->Status != STATUS_PENDING || (ReceiveBuffer->Status == STATUS_PENDING && !ExceptionPacketTimeouted))
			{
				Done									= TRUE;
				break;
			}
		}

		//
		// finished
		//
		if(Done)
			break;

		//
		// retry
		//
		BdNumberRetries									-= 1;
	}

	//
	// since we are able to talk to debugger, the retrycount is set to maximum value.
	//
	BdRetryCount										= 0x14;
	BdDebuggerNotPresent								= FALSE;
}

//
// find pci device
//
BOOLEAN BdFindOhciController(__out PPHYSICAL_ADDRESS BaseAddress,__in_opt UCHAR HardwareType,__in_opt UCHAR NoBuses)
{
	//
	// HACK HACK HACK
	// before we make HalGetBusData,the pci bus must be initialized,
	// we must know the hardware config mechanism,and setup ports
	// those info is token from firmware config tree,which is built by ntdetect.com
	// if we are doing earlier initialize before we run ntdetect.com,the firmware config tree is empty
	// therefor HalGetBusData will fail,so we setup a dummy firmware config tree to make pci happy
	//
	UCHAR HackDataBuffer[sizeof(CONFIGURATION_COMPONENT_DATA) + sizeof(CM_PARTIAL_RESOURCE_LIST) + sizeof(PCI_REGISTRY_INFO)];
	BOOLEAN EarlierInitialize							= FwConfigurationTree == 0 ? TRUE : FALSE;
	if(EarlierInitialize)
	{
		PCONFIGURATION_COMPONENT_DATA HackData			= reinterpret_cast<PCONFIGURATION_COMPONENT_DATA>(HackDataBuffer);
		HackData->Child									= 0;
		HackData->Parent								= 0;
		HackData->Sibling								= 0;
		HackData->ComponentEntry.Class					= AdapterClass;
		HackData->ComponentEntry.Type					= MultiFunctionAdapter;
		HackData->ComponentEntry.Key					= 0;
		HackData->ComponentEntry.Identifier				= "PCI";
		HackData->ComponentEntry.IdentifierLength		= 4;

		PCM_PARTIAL_RESOURCE_LIST CmResList				= Add2Ptr(HackData,sizeof(CONFIGURATION_COMPONENT_DATA),PCM_PARTIAL_RESOURCE_LIST);
		CmResList->Count								= 1;
		CmResList->Revision								= 1;
		CmResList->Version								= 1;
		CmResList->PartialDescriptors->Type				= CmResourceTypeDeviceSpecific;

		PPCI_REGISTRY_INFO RegInfo						= Add2Ptr(CmResList,sizeof(CM_PARTIAL_RESOURCE_LIST),PPCI_REGISTRY_INFO);
		RegInfo->HardwareMechanism						= HardwareType;
		RegInfo->NoBuses								= NoBuses;

		HackData->ConfigurationData						= CmResList;

		FwConfigurationTree								= HackData;
	}

	//
	// BUG BUG BUG
	// HalGetBusData did not return bridges to us,so we need to go though the whole 256 bridges
	//
	PCI_COMMON_HEADER Config;
	BOOLEAN Ret											= FALSE;
	for(ULONG i = 0; i < PCI_MAX_BRIDGE_NUMBER; i ++)
	{
		for(UCHAR j = 0; j < PCI_MAX_DEVICES; j ++)
		{
			for(UCHAR k = 0; k < PCI_MAX_FUNCTION; k ++)
			{
				PCI_SLOT_NUMBER Slot;
				Slot.u.bits.DeviceNumber				= j;
				Slot.u.bits.FunctionNumber				= k;
				Slot.u.bits.Reserved					= 0;

				//
				// read bus data
				//
				Config.VendorID							= PCI_INVALID_VENDORID;
				if(HalGetBusData(PCIConfiguration,i,Slot.u.AsULONG,&Config,sizeof(Config)) == sizeof(Config))
				{
					//
					// check base/sub/progif
					//	BaseClass	= 0x0c  = serial bus
					//	SubClass	= 0x00	= IEEE 1394
					//	ProgIf		= 0x10	= OpenHCI Host Controller
					//
					if(Config.BaseClass == PCI_CLASS_SERIAL_BUS_CTLR && Config.SubClass == PCI_SUBCLASS_SB_IEEE1394 && Config.ProgIf == 0x10)
					{
						//
						// found it
						//
						BaseAddress->LowPart			= Config.u.type0.BaseAddresses[0];
						ULONG AddressSpace				= BaseAddress->LowPart & PCI_ADDRESS_IO_SPACE ? 1 : 0;
						if(AddressSpace == 0 && (BaseAddress->LowPart & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT)
							BaseAddress->HighPart		= Config.u.type0.BaseAddresses[1];
						else
							BaseAddress->HighPart		= 0;

						Ret								= HalTranslateBusAddress(PCIBus,i,*BaseAddress,&AddressSpace,BaseAddress);

						i								= PCI_MAX_BRIDGE_NUMBER;
						j								= PCI_MAX_DEVICES;
						k								= PCI_MAX_FUNCTION;
						break;
					}
				}

				//
				// check multifunction device
				//
				if(k == 0 && (Config.VendorID == PCI_INVALID_VENDORID || Config.VendorID == 0 || !PCI_MULTIFUNCTION_DEVICE(&Config)))
					break;
			}
		}
	}

	//
	// restore pci bus info
	//
	if(EarlierInitialize)
	{
		//
		// reset those fields to zero,then pci will reinitialize it if you make a pci read/write config call
		//
		extern BUSHANDLER PCIBusHandler;
		PCIBusHandler.BusData							= 0;
		PCIBusHandler.NoBuses							= 0;
		PCIBusHandler.BusNumber							= 0;

		FwConfigurationTree								= 0;
	}

	return Ret;
}

//
// initialize debugger
//
BOOLEAN Bd1394Initialize(__in ULONG Channel,__in_opt UCHAR HardwareType,__in_opt UCHAR NoBuses)
{
	//
	// find ohci controller with
	//
	PHYSICAL_ADDRESS BaseAddress;
	if(!BdFindOhciController(&BaseAddress,HardwareType,NoBuses))
		return FALSE;

	//
	// must be mapped as 32bit memory
	//
	if((BaseAddress.LowPart & PCI_ADDRESS_IO_SPACE) || BaseAddress.HighPart)
		return FALSE;

	//
	// map it
	//
	OhciControllerRegisterBase							= static_cast<PUCHAR>(MmMapIoSpace(BaseAddress,2048,MmNonCached));
	if(!OhciControllerRegisterBase)
		return FALSE;

	Dbg1394GlobalData									= static_cast<PDBG1394_GLOBAL_DATA>(FwAllocateHeapPermanent(3));
	RtlZeroMemory(Dbg1394GlobalData,sizeof(DBG1394_GLOBAL_DATA));

	//
	// setup debug info
	//
	Dbg1394GlobalData->DebugConfigInfo.Signature		= 0xbabababa;
	Dbg1394GlobalData->DebugConfigInfo.BusPresent		= FALSE;
	Dbg1394GlobalData->DebugConfigInfo.Channel			= (Channel < 0x100) ? Channel : 0;
	Dbg1394GlobalData->DebugConfigInfo.MajorVersion		= 1;
	Dbg1394GlobalData->DebugConfigInfo.MinorVersion		= 0;
	Dbg1394GlobalData->DebugConfigInfo.ReceiveBuffer	= reinterpret_cast<ULONG>(&Dbg1394GlobalData->ReceiveBuffer);
	Dbg1394GlobalData->DebugConfigInfo.SendBuffer		= reinterpret_cast<ULONG>(&Dbg1394GlobalData->SendBuffer);

	//
	// read version,must be 1
	//
	ULONG Temp											= READ_REGISTER_ULONG(reinterpret_cast<PULONG>(OhciControllerRegisterBase));
	if((Temp & 0x00ff0000) != 0x00010000)
		return FALSE;

	//
	// do a soft reset
	//
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x50,PULONG),0x10000);

	//
	// wait soft reset complete
	//
	for(ULONG i = 0; i < 1000; i ++)
	{
		Temp											= READ_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x50,PULONG));
		Bd1394StallExecution(1);

		if(Temp & 0x10000)
			continue;

		break;
	}

	//
	// soft reset timeout
	//
	if(Temp & 0x10000)
		return FALSE;

	//
	// enable link <-> phy communication
	//
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x50,PULONG),0x80000);
	Bd1394StallExecution(20);

	//
	// end / recv data in little-endian
	//
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x54,PULONG),0x40000000);

	//
	// enable post-write
	//
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x50,PULONG),0x40000);

	//
	// cycle Master | cycle Enable
	//
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0xe4,PULONG),0x300600);
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0xe0,PULONG),0x300000);

	//
	// busNumber 0x3ff,NodeNumber 0
	//
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0xe8,PULONG),0xffc0);

	//
	// set singnatuer
	//
	Dbg1394GlobalData->DebugConfigRom.Singnature		= '1394';

	//
	// set bus options
	//
	Temp												= READ_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x20,PULONG));
	Temp												= Bd1394SwapULong(Temp);
	Temp												&= 0x7ffff3f;
	Temp												|= 0x40;
	Dbg1394GlobalData->DebugConfigRom.BusOptions		= Bd1394SwapULong(Temp);

	//
	// read global unique id
	//
	Dbg1394GlobalData->DebugConfigRom.GlobalUniqueId[0]	= READ_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x24,PULONG));
	Dbg1394GlobalData->DebugConfigRom.GlobalUniqueId[1]	= READ_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x28,PULONG));

	//
	// setup config rom
	//
	IEEE1394_CONFIG_ROM_INFO Info;
	Info.AsULong										= 0;
	Info.CriCrcLength									= 4;
	Info.CriInfoLength									= 4;
	Info.CriCrcValue									= Bd1394CalculateCrc(&Dbg1394GlobalData->DebugConfigRom.Singnature,4);
	Dbg1394GlobalData->DebugConfigRom.Header			= Info;
	Dbg1394GlobalData->DebugConfigRom.NodeCapabilities	= 0xc083000c;
	Dbg1394GlobalData->DebugConfigRom.ModuleVendorId	= 0xf2500003;
	Dbg1394GlobalData->DebugConfigRom.ExtendedKey		= 0xf250001c;
	Dbg1394GlobalData->DebugConfigRom.DebugKey			= 0x0200001d;
	Dbg1394GlobalData->DebugConfigRom.DebugConfigInfo	= Bd1394SwapULong((0xffffff & reinterpret_cast<ULONG>(&Dbg1394GlobalData->DebugConfigInfo)) | 0x1e000000);
	Info.AsULong										= 0;
	Info.CriCrcLength									= 5;
	Info.CriCrcValue									= Bd1394CalculateCrc(&Dbg1394GlobalData->DebugConfigRom.NodeCapabilities,Info.CriCrcLength);
	Info.AsULong										= Bd1394SwapULong(Info.AsULong);
	Dbg1394GlobalData->DebugConfigRom.DirectoryInfoHead	= Info;

	//
	// write the new config rom head
	//
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x18,PULONG),Dbg1394GlobalData->DebugConfigRom.Header.AsULong);
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x1c,PULONG),Dbg1394GlobalData->DebugConfigRom.Singnature);
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x20,PULONG),Dbg1394GlobalData->DebugConfigRom.BusOptions);
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x24,PULONG),Dbg1394GlobalData->DebugConfigRom.GlobalUniqueId[0]);
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x28,PULONG),Dbg1394GlobalData->DebugConfigRom.GlobalUniqueId[1]);

	//
	// write config rom address
	//
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x34,PULONG),reinterpret_cast<ULONG>(&Dbg1394GlobalData->DebugConfigRom));

	//
	// mask all interrupt
	//
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x8c,PULONG),0xffffffff);

	//
	// enable link
	//
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x50,PULONG),0x20000);
	Bd1394StallExecution(1000);

	//
	// accept asyn request from all the nodes
	//
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x108,PULONG),0xffffffff);
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x100,PULONG),0xffffffff);

	//
	// accept asyn physical request from all the nodes
	//
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x110,PULONG),0xffffffff);
	WRITE_REGISTER_ULONG(Add2Ptr(OhciControllerRegisterBase,0x118,PULONG),0xffffffff);

	//
	// hard reset on the bus
	//
	UCHAR PhyTemp;
	if(!Bd1394ReadPhyRegister(OhciControllerRegisterBase,1,&PhyTemp))
		return FALSE;

	PhyTemp												|= 0x40;
	Bd1394WritePhyRegister(OhciControllerRegisterBase,1,PhyTemp);
	Bd1394StallExecution(1000);

	//
	// register with kd package
	//
	BdSendPacket										= &Bd1394SendPacket;
	BdSendControlPacket									= &Bd1394SendControlPacket;
	BdReceivePacket										= &Bd1394ReceivePacket;
	BdNextPacketIdToSend								= 0;

	return TRUE;
}