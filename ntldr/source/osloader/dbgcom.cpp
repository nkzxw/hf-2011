//********************************************************************
//	created:	16:8:2008   2:15
//	file:		dbgcom.cpp
//	author:		tiamo
//	purpose:	debug over serial
//********************************************************************

#include "stdafx.h"

//
// com port file id
//
ULONG													BdFileId;

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
// next expected id
//
extern ULONG											BdPacketIdExpected;

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
// send byte
//
VOID BdComPutByte(__in UCHAR Value)
{
	BlPortPutByte(BdFileId,Value);
}

//
// receive byte
//
ULONG BdComGetByte(__out PUCHAR Value)
{
	return BlPortGetByte(BdFileId,Value);
}

//
// poll byte
//
ULONG BdComPortPollByte(__out PUCHAR Value)
{
	return BlPortPollByte(BdFileId,Value);
}

//
// send string
//
VOID BdComSendString(__in PVOID Buffer,__in ULONG Length)
{
	//
	// write bytes to the kernel debugger port.
	//
	PUCHAR Src											= static_cast<PUCHAR>(Buffer);
	while(Length > 0)
	{
		UCHAR Output									= *Src;
		BdComPutByte(Output);
		Length											-= 1;
		Src												+= 1;
	}

	return;
}

//
// receive string
//
ULONG BdComReceiveString(__out PVOID Destination,__in ULONG Length)
{
	//
	// read bytes until either a error is encountered or the entire string has been read.
	//
	PUCHAR Buffer										= static_cast<PUCHAR>(Destination);
	while(Length > 0)
	{
		UCHAR Input;
		ULONG ReturnCode								= BdComGetByte(&Input);

		if(ReturnCode != CP_GET_SUCCESS)
			return ReturnCode;

		*Buffer											= Input;
		Length											-= 1;
		Buffer											+= 1;
	}

	return CP_GET_SUCCESS;
}

//
// send control packet
//
VOID BdComSendControlPacket(__in ULONG PacketType,__in_opt ULONG PacketId)
{
	KD_PACKET PacketHeader;

	PacketHeader.PacketLeader							= CONTROL_PACKET_LEADER;
	if(PacketId)
		PacketHeader.PacketId							= PacketId;

	PacketHeader.ByteCount								= 0;
	PacketHeader.Checksum								= 0;
	PacketHeader.PacketType								= static_cast<USHORT>(PacketType);

	BdComSendString(&PacketHeader,sizeof(KD_PACKET));
}

//
// receive packet leader
//
USHORT BdComReceivePacketLeader(__in ULONG PacketType,__out PULONG PacketLeader)
{
	//
	// with all the interrupts being off, it is very hard to implement the actual timeout code.(Maybe, by reading the CMOS.)
	// here we use a loop count to wait about 3 seconds.
	// the CpGetByte will return with error code = CP_GET_NODATA if it cannot find data byte within 1 second.
	// kernel debugger's timeout period is 5 seconds.
	//
	ULONG Index											= 0;
	UCHAR Input											= 0;
	UCHAR PreviousByte									= 0;
	BOOLEAN BreakinDetected								= FALSE;
	ULONG PacketId										= 0;
	do
	{
		ULONG ReturnCode								= BdComGetByte(&Input);
		if(ReturnCode == CP_GET_NODATA)
		{
			if(BreakinDetected)
			{
				BdControlCPending						= TRUE;
				return KDP_PACKET_RESEND;
			}

			return KDP_PACKET_TIMEOUT;
		}

		if(ReturnCode == CP_GET_ERROR)
		{
			Index										= 0;
			continue;
		}

		if(Input == PACKET_LEADER_BYTE || Input == CONTROL_PACKET_LEADER_BYTE)
		{
			if(Index == 0)
			{
				PreviousByte							= Input;
				Index									+= 1;
			}
			else if(Input == PreviousByte)
			{
				Index									+= 1;
			}
			else
			{
				PreviousByte							= Input;
				Index									= 1;
			}
		}
		else
		{
			//
			// if we detect breakin character, we need to verify it validity.
			// (it is possible that we missed a packet leader and the breakin character is simply a data byte in the packet.)
			// since kernel debugger send out breakin character ONLY when it is waiting for State Change packet.
			// the breakin character should not be followed by any other character except packet leader byte.
			//
			if(Input == BREAKIN_PACKET_BYTE )
			{
				BreakinDetected							= TRUE;
			}
			else
			{
				//
				// The following statement is ABSOLUTELY necessary.
				//
				BreakinDetected							= FALSE;
			}

			Index = 0;
		}
	}while(Index < 4);

	if(BreakinDetected)
		BdControlCPending								= TRUE;

	//
	// return the packet leader and FALSE to indicate no resend is needed.
	//
	if(Input == PACKET_LEADER_BYTE)
		*PacketLeader = PACKET_LEADER;
	else
		*PacketLeader = CONTROL_PACKET_LEADER;

	BdDebuggerNotPresent								= FALSE;

	return KDP_PACKET_RECEIVED;
}

//
// receive a packet
//
ULONG BdComReceivePacket(__in ULONG PacketType,__out PSTRING MessageHeader,__out PSTRING MessageData,__out PULONG DataLength)
{
	KD_PACKET PacketHeader;

	//
	// read Packet Leader
	//
	while(1)
	{
		while(1)
		{
			ULONG ReturnCode							= BdComReceivePacketLeader(PacketType,&PacketHeader.PacketLeader);

			//
			// if we can successfully read packet leader,it has high possibility that kernel debugger is alive,so reset count.
			//
			if(ReturnCode != KDP_PACKET_TIMEOUT)
				BdNumberRetries							= BdRetryCount;

			if(ReturnCode != KDP_PACKET_RECEIVED)
				return ReturnCode;

			//
			// read packet type.
			//
			ReturnCode									= BdComReceiveString(&PacketHeader.PacketType,sizeof(PacketHeader.PacketType));
			if(ReturnCode == CP_GET_NODATA)
				return KDP_PACKET_TIMEOUT;

			if(ReturnCode == CP_GET_ERROR)
			{
				if(PacketHeader.PacketLeader == CONTROL_PACKET_LEADER)
				{
					//
					// if read error and it is for a control packet, simply preptend that we have not seen this packet.
					// hopefully we will receive the packet we desire which automatically acks the packet we just sent.
					//
					continue;
				}
				else
				{
					//
					// if read error while reading data packet, we have to ask kernel debugger to resend us the packet.
					//
					break;
				}
			}

			//
			// if the packet we received is a resend request, we return true and let caller resend the packet.
			//
			if(PacketHeader.PacketLeader == CONTROL_PACKET_LEADER && PacketHeader.PacketType == PACKET_TYPE_KD_RESEND)
				return KDP_PACKET_RESEND;

			//
			// read data length.
			//
			ReturnCode									= BdComReceiveString(&PacketHeader.ByteCount,sizeof(PacketHeader.ByteCount));
			if(ReturnCode == CP_GET_NODATA)
				return KDP_PACKET_TIMEOUT;

			if(ReturnCode == CP_GET_ERROR)
			{
				if(PacketHeader.PacketLeader == CONTROL_PACKET_LEADER)
					continue;
				else
					break;
			}

			//
			// read Packet Id.
			//
			ReturnCode									= BdComReceiveString(&PacketHeader.PacketId,sizeof(PacketHeader.PacketId));

			if(ReturnCode == CP_GET_NODATA)
				return KDP_PACKET_TIMEOUT;

			if(ReturnCode == CP_GET_ERROR)
			{
				if(PacketHeader.PacketLeader == CONTROL_PACKET_LEADER)
					continue;
				else
					break;
			}

			//
			// read packet checksum.
			//
			ReturnCode									= BdComReceiveString(&PacketHeader.Checksum,sizeof(PacketHeader.Checksum));
			if(ReturnCode == CP_GET_NODATA)
				return KDP_PACKET_TIMEOUT;

			if(ReturnCode == CP_GET_ERROR)
			{
				if(PacketHeader.PacketLeader == CONTROL_PACKET_LEADER)
					continue;
				else
					break;
			}

			//
			// a complete packet header is received.check its validity and perform appropriate action depending on packet type.
			//
			if(PacketHeader.PacketLeader == CONTROL_PACKET_LEADER)
			{
				if(PacketHeader.PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
				{
					//
					// if we received an expected ACK packet and we are not waiting for any new packet, update outgoing packet id and return.
					// if we are NOT waiting for ACK packet we will keep on waiting.
					// if the ACK packet is not for the packet we send, ignore it and keep on waiting.
					//
					if(PacketHeader.PacketId != (BdNextPacketIdToSend & ~SYNC_PACKET_ID))
						continue;
					else if(PacketType != PACKET_TYPE_KD_ACKNOWLEDGE)
						break;

					BdNextPacketIdToSend				^= 1;
					return KDP_PACKET_RECEIVED;
				}

				 if(PacketHeader.PacketType == PACKET_TYPE_KD_RESET)
				{
					//
					// if we received Reset packet, reset the packet control variables and resend earlier packet.
					//
					BdNextPacketIdToSend						= INITIAL_PACKET_ID;
					BdPacketIdExpected							= INITIAL_PACKET_ID;
					BdComSendControlPacket(PACKET_TYPE_KD_RESET,0L);
					return KDP_PACKET_RESEND;
				}

				if(PacketHeader.PacketType == PACKET_TYPE_KD_RESEND)
					return KDP_PACKET_RESEND;

				//
				// invalid packet header, ignore it.
				//
				continue;
			}

			if(PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
			{
				//
				// the packet header is for data packet (not control packet).
				//

				//
				// if we are waiting for ACK packet ONLY and we receive a data packet header,
				// check if the packet id is what we expected.
				// if yes, assume the acknowledge is lost (but sent), ask sender to resend and return with PACKET_RECEIVED.
				//

				if(PacketHeader.PacketId == BdPacketIdExpected)
				{
					BdComSendControlPacket(PACKET_TYPE_KD_RESEND,0L);
					BdNextPacketIdToSend				^= 1;
					return KDP_PACKET_RECEIVED;
				}

				BdComSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE,PacketHeader.PacketId);
				continue;
			}

			//
			// we are waiting for data packet and we received the packet header for data packet.
			// perform the following checkings to make sure it is the packet we are waiting for.
			//

			//
			// check ByteCount received is valid
			//
			ULONG MessageLength							= MessageHeader->MaximumLength;
			if(PacketHeader.ByteCount > PACKET_MAX_SIZE || PacketHeader.ByteCount < MessageLength)
				break;

			*DataLength									= PacketHeader.ByteCount - MessageLength;

			//
			// read the message header.
			//
			ReturnCode									= BdComReceiveString(MessageHeader->Buffer,MessageLength);
			if(ReturnCode != CP_GET_SUCCESS)
				break;

			MessageHeader->Length						= static_cast<USHORT>(MessageLength);

			//
			// read the message data.
			//
			ReturnCode									= BdComReceiveString(MessageData->Buffer,*DataLength);
			if(ReturnCode != CP_GET_SUCCESS)
				break;

			MessageData->Length							= static_cast<USHORT>(*DataLength);

			//
			// read packet trailing byte
			//
			UCHAR Input;
			ReturnCode									= BdComGetByte(&Input);

			if(ReturnCode != CP_GET_SUCCESS || Input != PACKET_TRAILING_BYTE)
				break;

			//
			// check PacketType is what we are waiting for.
			//
			if(PacketType != PacketHeader.PacketType)
			{
				BdComSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE,PacketHeader.PacketId);
				continue;
			}

			//
			// check PacketId is valid.
			//
			if(PacketHeader.PacketId == INITIAL_PACKET_ID || PacketHeader.PacketId == (INITIAL_PACKET_ID ^ 1))
			{
				if(PacketHeader.PacketId != BdPacketIdExpected)
				{
					BdComSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE,PacketHeader.PacketId);
					continue;
				}
			}
			else
			{
				break;
			}

			//
			// check checksum is valid.
			//
			ULONG Checksum								= BdComputeChecksum(MessageHeader->Buffer,MessageHeader->Length);
			Checksum									+= BdComputeChecksum(MessageData->Buffer,MessageData->Length);
			if(Checksum != PacketHeader.Checksum)
				break;

			//
			// send Acknowledge byte and the Id of the packet received.then, update the ExpectId for next incoming packet.
			//
			BdComSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE,PacketHeader.PacketId);

			//
			// we have successfully received the packet so update the packet control variables and return sucess.
			//
			BdPacketIdExpected							^= 1;

			return KDP_PACKET_RECEIVED;
		}

		//
		// send resend packet
		//
		BdComSendControlPacket(PACKET_TYPE_KD_RESEND,0L);
	}
}

//
// send packet
//
VOID BdComSendPacket(__in ULONG PacketType,__in PSTRING MessageHeader,__in PSTRING MessageData)
{
	KD_PACKET PacketHeader;
	ULONG MessageDataLength;

	//
	// if there is a data buffer,calc checksum
	//
	if(MessageData)
	{
		MessageDataLength								= MessageData->Length;
		PacketHeader.Checksum							= BdComputeChecksum(MessageData->Buffer,MessageData->Length);
	}
	else
	{
		MessageDataLength								= 0;
		PacketHeader.Checksum							= 0;
	}

	//
	// and compute header's checksum
	//
	PacketHeader.Checksum								+= BdComputeChecksum(MessageHeader->Buffer,MessageHeader->Length);

	//
	// initialize and send the packet header.
	//
	PacketHeader.PacketLeader							= PACKET_LEADER;
	PacketHeader.ByteCount								= static_cast<USHORT>(MessageHeader->Length + MessageDataLength);
	PacketHeader.PacketType								= static_cast<USHORT>(PacketType);
	BdNumberRetries										= BdRetryCount;
	ULONG ReturnCode									= KDP_PACKET_TIMEOUT;
	do
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
				BdDebuggerNotPresent				= TRUE;
				BdNextPacketIdToSend				= INITIAL_PACKET_ID | SYNC_PACKET_ID;
				BdPacketIdExpected					= INITIAL_PACKET_ID;
				return;
			}
		}

		//
		// setting PacketId has to be in the do loop in case Packet Id was reset.
		//
		PacketHeader.PacketId							= BdNextPacketIdToSend;
		BdComSendString(&PacketHeader,sizeof(KD_PACKET));

		//
		// output message header.
		//
		BdComSendString(MessageHeader->Buffer,MessageHeader->Length);

		//
		// output message data.
		//
		if(MessageDataLength)
			BdComSendString(MessageData->Buffer,MessageData->Length);

		//
		// output a packet trailing byte
		//
		BdComPutByte(PACKET_TRAILING_BYTE);

		//
		// wait for the Ack Packet
		//
		ReturnCode = BdComReceivePacket(PACKET_TYPE_KD_ACKNOWLEDGE,0,0,0);
		if(ReturnCode == KDP_PACKET_TIMEOUT)
			BdNumberRetries								-= 1;

	}while(ReturnCode != KDP_PACKET_RECEIVED);

	//
	// reset Sync bit in packet id.  The packet we sent may have Sync bit set
	//
	BdNextPacketIdToSend								&= ~SYNC_PACKET_ID;

	//
	// since we are able to talk to debugger, the retrycount is set to maximum value.
	//
	BdRetryCount										= 0x14;
}

//
// open com port
//
BOOLEAN BdPortInitialize(__in ULONG Baudrate,__in ULONG PortNum)
{
	if(!BlPortInitialize(Baudrate,PortNum,0,FALSE,&BdFileId))
		return FALSE;

	extern CHAR	DebugMessage[];
	sprintf(DebugMessage,"\nBoot Debugger Using: COM%d (Baud Rate %d)\n",PortNum,Baudrate);

	TextStringOut(DebugMessage);

	//
	// register with kd package
	//
	BdSendPacket										= &BdComSendPacket;
	BdSendControlPacket									= &BdComSendControlPacket;
	BdReceivePacket										= &BdComReceivePacket;
	BdNextPacketIdToSend								= SYNC_PACKET_ID | INITIAL_PACKET_ID;
	BdPacketIdExpected									= INITIAL_PACKET_ID;

	return TRUE;
}
