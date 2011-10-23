//********************************************************************
//	created:	10:8:2008   22:47
//	file:		arcemul.cpp
//	author:		tiamo
//	purpose:	enum layer
//********************************************************************

#include "stdafx.h"

//
// used only by scsi port
//
VOID													(*AEDriverUnloadRoutine)(__in PVOID DriverObject);

//
// firmware vectors
//
extern PVOID											GlobalFirmwareVectors[MaximumRoutine];

//
// system block
//
SYSTEM_PARAMETER_BLOCK									GlobalSystemBlock = {0,sizeof(SYSTEM_PARAMETER_BLOCK),0,0,0,0,0,0,MaximumRoutine,GlobalFirmwareVectors,0,0};

//
// stall count
//
ULONG													FwStallCounter;

//
// config tree
//
PCONFIGURATION_COMPONENT_DATA							FwConfigurationTree;

//
// serial console port buffer start pos
//
UCHAR													PortBufferStart;

//
// end pos
//
UCHAR													PortBufferEnd;

//
// buffer
//
UCHAR													PortBuffer[0x0a];

//
// bios disabled
//
BOOLEAN													AEBiosDisabled;

//
// local buffer used by AERead
//
PVOID													AEReadLocalBuffer = 0;

//
// stub
//
ARC_STATUS BlArcNotYetImplemented(__in ULONG FileId)
{
	BlPrint("ERROR - Unimplemented Firmware Vector called (FID %lx)\r\n",FileId);

	return EINVAL;
}

//
// get relative time
//
ULONG AEGetRelativeTime()
{
	return ExternalServicesTable->GetCounter() * 10 / 182;
}

//
// close
//
ARC_STATUS AEClose(__in ULONG FileId)
{
	return BlFileTable[FileId].DeviceEntryTable->Close(FileId);
}

//
// open
//
ARC_STATUS AEOpen(__in PCHAR OpenPath,__in OPEN_MODE OpenMode,__out PULONG FileId)
{
#if _RAMDISK_SUPPORT_
	if(RamdiskOpen(OpenPath,OpenMode,FileId) == ESUCCESS)
		return ESUCCESS;
#endif

	if(BiosConsoleOpen(OpenPath,OpenMode,FileId) == ESUCCESS)
		return ESUCCESS;

	if(!AEBiosDisabled && BiosPartitionOpen(OpenPath,OpenMode,FileId) == ESUCCESS)
		return ESUCCESS;

	ARC_STATUS Status									= ENOENT;

#if _SCSI_SUPPORT_
	//
	// it's not the console or a BIOS partition, so let's try the SCSI driver.
	//

	//
	// find a free FileId,the first and second one are reserved for console in/out
	//
	*FileId												= 2;
	while(BlFileTable[*FileId].Flags.Open == TRUE)
	{
		*FileId											+= 1;

		if(*FileId == ARRAYSIZE(BlFileTable))
			return ENOENT;
	}

	CHAR Buffer[128];
	strcpy(Buffer,OpenPath);

	Status												= ScsiDiskOpen(Buffer,OpenMode,FileId);
	if(Status == ESUCCESS)
	{
		//
		// SCSI successfully opened it.for now, we stick the appropriate SCSI DeviceEntryTable into the BlFileTable.  This is temporary.
		//
		extern BL_DEVICE_ENTRY_TABLE ScsiDiskEntryTable;
		BlFileTable[*FileId].Flags.Open					= TRUE;
		BlFileTable[*FileId].DeviceEntryTable			= &ScsiDiskEntryTable;
	}
#endif

	return Status;
}

//
// get memory descriptor
//
PMEMORY_DESCRIPTOR AEGetMemoryDescriptor(__in_opt PMEMORY_DESCRIPTOR MemoryDescriptor)
{
	extern MEMORY_DESCRIPTOR MDArray[];
	extern ULONG NumberDescriptors;

	PMEMORY_DESCRIPTOR Return							= 0;
	if(!MemoryDescriptor)
		return MDArray;

	if(static_cast<ULONG>(MemoryDescriptor - MDArray) >= NumberDescriptors - 1)
		return 0;

	return MemoryDescriptor + 1;
}

//
// seek
//
ARC_STATUS AESeek(__in ULONG FileId,__in PLARGE_INTEGER Offset,__in SEEK_MODE SeekMode)
{
	return BlFileTable[FileId].DeviceEntryTable->Seek(FileId,Offset,SeekMode);
}

//
// read
//
ARC_STATUS AERead(__in ULONG FileId,__out PVOID Buffer,__in ULONG Length,__out PULONG Count)
{
	*Count												= 0;

	//
	// special case for console input
	//
	if(!FileId)
	{
		while(1)
		{
			//
			// serial console is enabled
			//
			if(BlTerminalConnected)
			{
				//
				// copy from port buffer first
				//
				for(*Count = 0; *Count < Length; *Count ++)
				{
					PUCHAR ByteBuffer					= Add2Ptr(Buffer,*Count,PUCHAR);
					if(PortBufferStart != PortBufferEnd)
					{
						*ByteBuffer						= PortBuffer[PortBufferStart];
						PortBufferStart					= (PortBufferStart + 1) % ARRAYSIZE(PortBuffer);
					}
					else
					{
						//
						// poll and read a byte
						//
						if(BlPortPollByte(BlTerminalDeviceId,ByteBuffer) != CP_GET_SUCCESS)
							break;

						//
						// esc seq
						//
						if(*ByteBuffer == ASCI_ESC)
						{
							*ByteBuffer					= ASCI_CSI_IN;

							//
							// wait the next byte
							//
							ULONG Time1					= AEGetRelativeTime();

							while(BlPortPollOnly(BlTerminalDeviceId) != CP_GET_SUCCESS)
							{
								//
								// time out
								//
								if(AEGetRelativeTime() > Time1)
								{
									*Count				+= 1;
									return ESUCCESS;
								}
							}

							//
							// read it
							//
							UCHAR Input;
							if(BlPortPollByte(BlTerminalDeviceId,&Input) != CP_GET_SUCCESS)
							{
								*Count					+= 1;
								return ESUCCESS;
							}

							//
							// check seq
							//
							switch(Input)
							{
							case '[':
								{
									Time1					= AEGetRelativeTime();

									while(BlPortPollOnly(BlTerminalDeviceId) != CP_GET_SUCCESS)
									{
										//
										// time out
										//
										if(AEGetRelativeTime() > Time1)
											break;
									}

									//
									// read it
									//
									UCHAR Input;
									if(BlPortPollByte(BlTerminalDeviceId,&Input) != CP_GET_SUCCESS)
									{
										PortBuffer[PortBufferEnd]	= '[';
										PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
									}
									else
									{
										if(Input >= 'A' && Input <= 'D')
										{
											//
											// left,right,up,down arrow
											//
											PortBuffer[PortBufferEnd]	= Input;
											PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
										}
										else
										{
											PortBuffer[PortBufferEnd]	= '[';
											PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);

											PortBuffer[PortBufferEnd]	= Input;
											PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
										}
									}
								}
								break;

								//
								// HOME
								//
							case 'H':
							case 'h':
								PortBuffer[PortBufferEnd]	= 'H';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// END
								//
							case 'K':
							case 'k':
								PortBuffer[PortBufferEnd]	= 'K';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// F1
								//
							case '1':
								PortBuffer[PortBufferEnd]	= 'O';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);

								PortBuffer[PortBufferEnd]	= 'P';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// F2
								//
							case '2':
								PortBuffer[PortBufferEnd]	= 'O';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);

								PortBuffer[PortBufferEnd]	= 'Q';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// F3
								//
							case '3':
								PortBuffer[PortBufferEnd]	= 'O';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);

								PortBuffer[PortBufferEnd]	= 'R';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// F4
								//
							case '4':
								PortBuffer[PortBufferEnd]	= 'O';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);

								PortBuffer[PortBufferEnd]	= 'S';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// F5
								//
							case '5':
								PortBuffer[PortBufferEnd]	= 'O';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);

								PortBuffer[PortBufferEnd]	= 'T';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// F6
								//
							case '6':
								PortBuffer[PortBufferEnd]	= 'O';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);

								PortBuffer[PortBufferEnd]	= 'U';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// F7
								//
							case '7':
								PortBuffer[PortBufferEnd]	= 'O';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);

								PortBuffer[PortBufferEnd]	= 'V';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// F8
								//
							case '8':
								PortBuffer[PortBufferEnd]	= 'O';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);

								PortBuffer[PortBufferEnd]	= 'W';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// F9
								//
							case '9':
								PortBuffer[PortBufferEnd]	= 'O';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);

								PortBuffer[PortBufferEnd]	= 'X';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// F10
								//
							case '0':
								PortBuffer[PortBufferEnd]	= 'O';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);

								PortBuffer[PortBufferEnd]	= 'Y';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// F11
								//
							case '!':
								PortBuffer[PortBufferEnd]	= 'O';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);

								PortBuffer[PortBufferEnd]	= 'Z';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// F12
								//
							case '@':
								PortBuffer[PortBufferEnd]	= 'O';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);

								PortBuffer[PortBufferEnd]	= '[';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// tab
								//
							case '\t':
								PortBuffer[PortBufferEnd]	= '\t';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// INSERT
								//
							case '+':
								PortBuffer[PortBufferEnd]	= '@';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

								//
								// delete
								//
							case '-':
								PortBuffer[PortBufferEnd]	= 'P';
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;

							default:
								PortBuffer[PortBufferEnd]	= Input;
								PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
								break;
							}
						}

						//
						// DEL
						//
						if(*ByteBuffer == 0x7f)
						{
							*ByteBuffer					= ASCI_CSI_IN;
							PortBuffer[PortBufferEnd]	= 'P';
							PortBufferEnd				= (PortBufferEnd + 1) % ARRAYSIZE(PortBuffer);
						}
					}
				}

				//
				// got some data from serial console,return it,othewise try bios console
				//
				if(*Count)
					return ESUCCESS;
			}

			if(BiosConsoleReadStatus(0) != ESUCCESS)
				continue;

			return BiosConsoleRead(FileId,static_cast<PUCHAR>(Buffer),Length,Count);
		}
	}

	if(!AEReadLocalBuffer)
		AEReadLocalBuffer								= reinterpret_cast<PVOID>((reinterpret_cast<ULONG>(FwAllocatePool(0x20000)) + 0xffff) & 0xffff0000);

	PVOID CurrentBuffer									= Buffer;
	PVOID EndBuffer										= Add2Ptr(Buffer,Length - 1, PVOID);

	while(Length)
	{
		PVOID TransferBuffer;
		ULONG TransferLength;
		BOOLEAN UseLocalBuffer							= FALSE;
		if((reinterpret_cast<ULONG>(CurrentBuffer) & 0xffff0000) == (reinterpret_cast<ULONG>(EndBuffer) & 0xffff0000))
		{
			TransferLength								= Length;
			TransferBuffer								= CurrentBuffer;
		}
		else
		{
			if(AEReadLocalBuffer)
			{
				UseLocalBuffer							= TRUE;
				TransferBuffer							= AEReadLocalBuffer;
				if(Length < 0x10000)
					TransferLength						= Length;
				else
					TransferLength						= 0xffff;
			}
			else
			{
				TransferBuffer							= reinterpret_cast<PVOID>(reinterpret_cast<ULONG>(CurrentBuffer) & 0xffff0000);
				TransferLength							= 0x10000 - reinterpret_cast<ULONG>(TransferBuffer);
			}
		}

		ULONG ReadCount;
		ARC_STATUS Status								= BlFileTable[FileId].DeviceEntryTable->Read(FileId,TransferBuffer,TransferLength,&ReadCount);
		if(UseLocalBuffer)
			RtlCopyMemory(CurrentBuffer,AEReadLocalBuffer,ReadCount);

		CurrentBuffer									= Add2Ptr(CurrentBuffer,ReadCount,PVOID);
		Length											-= ReadCount;
		*Count											+= ReadCount;

		if(Status != ESUCCESS)
			return Status;
	}

	return ESUCCESS;
}

//
// read status
//
ARC_STATUS AEReadStatus(__in ULONG FileId)
{
	//
	// special case for console input
	//
	if(FileId)
		return BlArcNotYetImplemented(FileId);

	//
	// check serial console first
	//
	if(BlTerminalConnected)
	{
		//
		// something in the buffer
		//
		if(PortBufferStart != PortBufferEnd)
			return ESUCCESS;

		//
		// poll port
		//
		if(BlPortPollOnly(BlTerminalDeviceId) == CP_GET_SUCCESS)
			return ESUCCESS;
	}

	return BiosConsoleReadStatus(0);
}

//
// write
//
ARC_STATUS AEWrite(__in ULONG FileId,__out PVOID Buffer,__in ULONG Length,__out PULONG Count)
{
	//
	// special case for console output
	//
	if(FileId == 1)
	{
		if(BlTerminalConnected)
		{
			PCHAR FixedBuffer							= 0;
			ULONG FixedLength							= 0;

			if(!strncmp(static_cast<PCHAR>(Buffer),"\x1B[2J",4))
			{
				FixedBuffer								= "\x1B[H\x1B[J";
				FixedLength								= 6;
			}
			else if(!strncmp(static_cast<PCHAR>(Buffer),"\x1B[0J",4))
			{
				FixedBuffer								= "\x1B[J";
				FixedLength								= 3;
			}
			else if(!strncmp(static_cast<PCHAR>(Buffer),"\x1B[0K",4))
			{
				FixedBuffer								= "\x1B[K";
				FixedLength								= 3;
			}
			else if(!strncmp(static_cast<PCHAR>(Buffer),"\x1B[0m",4))
			{
				FixedBuffer								= "\x1B[m";
				FixedLength								= 3;
			}
			else
			{
				FixedBuffer								= static_cast<PCHAR>(Buffer);
				FixedLength								= Length;
			}

			for(ULONG i = 0; i < FixedLength; i ++,FixedBuffer ++)
			{
				UCHAR UTF8Buffer[3];

				//
				// dbcs language
				//
				if(DbcsLangId)
				{
					//
					// lead byte
					//
					if(GrIsDBCSLeadByte(*Add2Ptr(FixedBuffer,0,PUCHAR)))
					{
						//
						// 2 bytes
						//
						GetDBCSUtf8Translation(static_cast<PUCHAR>(static_cast<PVOID>(FixedBuffer)),UTF8Buffer);
						FixedBuffer						+= 1;
						i								+= 1;
					}
					else
					{
						GetSBCSUtf8Translation(static_cast<PUCHAR>(static_cast<PVOID>(FixedBuffer)),UTF8Buffer);
					}
				}
				else
				{
					//
					// fix some 'table graph'
					//
					UCHAR Byte							= *Add2Ptr(FixedBuffer,0,PUCHAR);
					switch(Byte)
					{
					case 169:
					case 170:
					case 183:
					case 184:
					case 187:
					case 188:
					case 196:
					case 197:
					case 213:
					case 214:
						Byte							= '+';
						break;

					case 176:
					case 179:
					case 182:
						Byte							= '|';
						break;

					case 177:
					case 216:
					case 217:
					case 218:
					case 219:
						Byte							= '%';
						break;

					case 178:
					case 215:
						Byte							= '#';
						break;

					case 192:
						Byte							= '-';
						break;

					case 201:
						Byte							= '=';
						break;
					}

					if(Byte < 0x80)
					{
						UTF8Buffer[0]					= Byte;
						UTF8Buffer[1]					= 0;
						UTF8Buffer[2]					= 0;
					}
					else
					{
						UTF8Encode(PcAnsiToUnicode[Byte & 0x7f],UTF8Buffer);
					}
				}

				//
				// output utf8 buffer
				//
				for(ULONG j = 0; j < 3; j ++)
				{
					if(!UTF8Buffer[j])
						continue;

					BlPortPutByte(BlTerminalDeviceId,UTF8Buffer[j]);
					FwStallExecution(BlTerminalDelay);
				}
			}
		}

		//
		// also send to bios console
		//
		return BiosConsoleWrite(FileId,static_cast<PUCHAR>(Buffer),Length,Count);
	}

	*Count = 0;

	do
	{
		ULONG Limit;
		ULONG PartCount;
		if((reinterpret_cast<ULONG>(Buffer) & 0xffff0000) != ((reinterpret_cast<ULONG>(Buffer) + Length) & 0xffff0000))
			Limit										= 0x10000 - (reinterpret_cast<ULONG>(Buffer) & 0x0000ffff);
		else
			Limit										= Length;

		ARC_STATUS Status								= BlFileTable[FileId].DeviceEntryTable->Write(FileId,Buffer,Limit,&PartCount);

		*Count											+= PartCount;
		Length											-= Limit;
		Buffer											= Add2Ptr(Buffer,Limit,PVOID);

		if(Status != ESUCCESS)
		{
			BlPrint("AERead: Status = %lx\r\n",Status);
			return Status;
		}
	}while(Length > 0);

	return ESUCCESS;
}

//
// get file info
//
ARC_STATUS AEGetFileInformation(__in ULONG FileId,__out PFILE_INFORMATION FileInformation)
{
	return BlFileTable[FileId].DeviceEntryTable->GetFileInformation(FileId,FileInformation);
}

//
// get time
//
PTIME_FIELDS AEGetTime()
{
	static TIME_FIELDS AETime;
	ULONG Date											= 0;
	ULONG Time											= 0;
	ExternalServicesTable->GetDateTime(&Date,&Time);

	//
	// date and time are filled as as follows:
	//
	// date:
	//
	//    bits 0  - 4  : day
	//    bits 5  - 8  : month
	//    bits 9  - 31 : year
	//
	// time:
	//
	//    bits 0  - 5  : second
	//    bits 6  - 11 : minute
	//    bits 12 - 16 : hour
	//

	AETime.Second										= static_cast<CSHORT>((Time & 0x0000003f) >> 0);
	AETime.Minute										= static_cast<CSHORT>((Time & 0x00000fc0) >> 6);
	AETime.Hour											= static_cast<CSHORT>((Time & 0x0001f000) >> 12);
	AETime.Day											= static_cast<CSHORT>((Date & 0x0000001f) >> 0);
	AETime.Month										= static_cast<CSHORT>((Date & 0x000001e0) >> 5);
	AETime.Year											= static_cast<CSHORT>((Date & 0xfffffe00) >> 9);

	//
	// invalid
	//
	AETime.Milliseconds									= 0;
	AETime.Weekday										= 7;

	return &AETime;
}

//
// get peer
//
PCONFIGURATION_COMPONENT FwGetPeer(__in PCONFIGURATION_COMPONENT Current)
{
	if(!Current)
		return 0;

	PCONFIGURATION_COMPONENT_DATA CurrentEntry			= CONTAINING_RECORD(Current,CONFIGURATION_COMPONENT_DATA,ComponentEntry);

	return CurrentEntry->Sibling ? &(CurrentEntry->Sibling->ComponentEntry) : 0;
}

//
// get child
//
PCONFIGURATION_COMPONENT FwGetChild(__in PCONFIGURATION_COMPONENT Current)
{
	//
	// if current component is NULL, return a pointer to first system component; otherwise return current component's child component.
	//
	if(Current)
	{
		PCONFIGURATION_COMPONENT_DATA CurrentEntry		= CONTAINING_RECORD(Current,CONFIGURATION_COMPONENT_DATA,ComponentEntry);

		return CurrentEntry->Child ? &(CurrentEntry->Child->ComponentEntry) : 0;
	}

	return FwConfigurationTree ? &(FwConfigurationTree->ComponentEntry) : 0;
}

//
// get parent
//
PCONFIGURATION_COMPONENT AEGetParent(__in PCONFIGURATION_COMPONENT Current)
{
	if(!Current)
		return 0;

	PCONFIGURATION_COMPONENT_DATA CurrentEntry			= CONTAINING_RECORD(Current,CONFIGURATION_COMPONENT_DATA,ComponentEntry);

	return CurrentEntry->Parent ? &(CurrentEntry->Parent->ComponentEntry) : 0;
}

//
// get component
//
PCONFIGURATION_COMPONENT FwGetComponent(__in PCHAR Pathname)
{
	PCONFIGURATION_COMPONENT Component;

	//
	// get the the root component.
	//
	PCONFIGURATION_COMPONENT MatchComponent				= FwConfigurationTree ? &FwConfigurationTree->ComponentEntry : 0;
	PCHAR PathString									= Pathname;

	//
	// repeat search for each new match component.
	//
	do
	{
		//
		// get the first child of the current match component.
		//
		Component										= FwGetChild(MatchComponent);

		//
		// search each child of the current match component for the next match.
		//
		while(Component)
		{
			//
			// reset Token to be the current position on the pathname.
			//
			PCHAR Token									= PathString;
			extern PCHAR MnemonicTable[];
			PCHAR MatchString							= MnemonicTable[Component->Type];

			//
			// compare strings.
			//
			while(*MatchString == tolower(*Token))
			{
				MatchString								+= 1;
				Token									+= 1;
			}

			//
			// strings compare if the first mismatch is the terminator for each.
			//
			if(*MatchString == 0 && *Token == '(')
			{
				//
				// form key.
				//
				ULONG Key								= 0;
				Token									+= 1;
				while(*Token != ')' && *Token != 0)
					Key									= Key * 10 + *Token ++ - '0';

				//
				// if the key matches the component matches, so update pointers and break.
				//
				if(Component->Key == Key)
				{
					PathString							= Token + 1;
					MatchComponent						= Component;
					break;
				}
			}

			Component									= FwGetPeer(Component);
		}
	}while(Component && *PathString);

	return MatchComponent;
}

//
// get config data
//
ARC_STATUS AEGetConfigurationData(__in PVOID ConfigurationData,__in PCONFIGURATION_COMPONENT Current)
{
	if(!Current)
		return EINVAL;

	PCONFIGURATION_COMPONENT_DATA CurrentEntry			= CONTAINING_RECORD(Current,CONFIGURATION_COMPONENT_DATA,ComponentEntry);
	RtlMoveMemory(ConfigurationData,CurrentEntry->ConfigurationData,Current->ConfigurationDataLength);

	return ESUCCESS;
}

//
// get env
//
PCHAR AEGetEnvironment(__in PCHAR Variable)
{
	//
	// the only one supported
	//
	if(_stricmp(Variable, "LastKnownGood") != 0)
		return 0;

	//
	// read the Daylight Savings Bit(bit0) out of the RTC to determine whether the LastKnownGood environment variable is TRUE or FALSE.
	// control port = 0x70,data port = 0x71
	//
	WRITE_PORT_UCHAR(reinterpret_cast<PUCHAR>(0x70),0x0b);

	return READ_PORT_UCHAR(reinterpret_cast<PUCHAR>(0x71)) & 1 ? "TRUE" : 0;
}

//
// reboot
//
VOID AEReboot()
{
	TextGrTerminate();

	//
	// HACKHACK John Vert (jvert)
	//	some SCSI drives get really confused and return zeroes when you use the BIOS to query their size after the AHA driver has initialized.
	//	this can completely tube OS/2 or DOS.
	//	so here we try and open both BIOS-accessible hard drives.
	//	our open code is smart enough to retry if it gets back zeros, so hopefully this will give the SCSI drives a chance to get their act together.
	//
	ULONG DriveId;
	ARC_STATUS Status									= ArcOpen("multi(0)disk(0)rdisk(0)partition(0)",ArcOpenReadOnly,&DriveId);
	if(Status == ESUCCESS)
		ArcCacheClose(DriveId);

	Status												= ArcOpen("multi(0)disk(0)rdisk(1)partition(0)",ArcOpenReadOnly,&DriveId);
	if(Status == ESUCCESS)
		ArcCacheClose(DriveId);

	BdStopDebugger();

	ExternalServicesTable->RebootProcessor();
}

//
// fills in all the fields in the Global System Parameter Block that it can
//
VOID BlFillInSystemParameters(__in PBOOT_CONTEXT BootContextRecord)
{
	for(ULONG i = 0; i < MaximumRoutine; i ++)
		GlobalFirmwareVectors[i]						= static_cast<PVOID>(&BlArcNotYetImplemented);

	GlobalFirmwareVectors[CloseRoutine]					= static_cast<PVOID>(&AEClose);
	GlobalFirmwareVectors[OpenRoutine]					= static_cast<PVOID>(&AEOpen);
	GlobalFirmwareVectors[MemoryRoutine]				= static_cast<PVOID>(&AEGetMemoryDescriptor);
	GlobalFirmwareVectors[SeekRoutine]					= static_cast<PVOID>(&AESeek);
	GlobalFirmwareVectors[ReadRoutine]					= static_cast<PVOID>(&AERead);
	GlobalFirmwareVectors[ReadStatusRoutine]			= static_cast<PVOID>(&AEReadStatus);
	GlobalFirmwareVectors[WriteRoutine]					= static_cast<PVOID>(&AEWrite);
	GlobalFirmwareVectors[GetFileInformationRoutine]	= static_cast<PVOID>(&AEGetFileInformation);
	GlobalFirmwareVectors[GetTimeRoutine]				= static_cast<PVOID>(&AEGetTime);
	GlobalFirmwareVectors[GetRelativeTimeRoutine]		= static_cast<PVOID>(&AEGetRelativeTime);
	GlobalFirmwareVectors[GetPeerRoutine]				= static_cast<PVOID>(&FwGetPeer);
	GlobalFirmwareVectors[GetChildRoutine]				= static_cast<PVOID>(&FwGetChild);
	GlobalFirmwareVectors[GetParentRoutine]				= static_cast<PVOID>(&AEGetParent);
	GlobalFirmwareVectors[GetComponentRoutine]			= static_cast<PVOID>(&FwGetComponent);
	GlobalFirmwareVectors[GetDataRoutine]				= static_cast<PVOID>(&AEGetConfigurationData);
	GlobalFirmwareVectors[GetEnvironmentRoutine]		= static_cast<PVOID>(&AEGetEnvironment);
	GlobalFirmwareVectors[RestartRoutine]				= static_cast<PVOID>(&AEReboot);
	GlobalFirmwareVectors[RebootRoutine]				= static_cast<PVOID>(&AEReboot);
}

//
// initialize stall
//
VOID AEInitializeStall()
{
	FwStallCounter										= ExternalServicesTable->GetStallCount();
}

//
// sleep
//
VOID FwStallExecution(__in ULONG Microseconds)
{
	ULONG i												= Microseconds * FwStallCounter;

	__asm
	{
		mov		eax,i
	looptop:
		sub		eax,1
		jnz		looptop
	}
}

//
// if Mnemonic is a component of the path, then it converts the key value to an integer wich is returned in Key.
//
BOOLEAN FwGetPathMnemonicKey(__in PCHAR OpenPath,__in PCHAR Mnemonic,__in PULONG Key)
{
	return BlGetPathMnemonicKey(OpenPath,Mnemonic,Key);
}

//
// terminate io
//
VOID AETerminateIo()
{
	if(AEDriverUnloadRoutine)
		AEDriverUnloadRoutine(0);
}

//
// initialize scsi driver
//
ARC_STATUS AEInitializeIo(__in ULONG DriveId)
{
	//
	// intialize stall count
	//
	FwStallCounter										= ExternalServicesTable->GetStallCount();

#if _SCSI_SUPPORT_
	//
	// load ntbootdd.sys
	//
	PVOID ImageBase										= 0;
	ARC_STATUS Status									= BlLoadImageEx(DriveId,MemoryFirmwarePermanent,"\\NTBOOTDD.SYS",IMAGE_FILE_MACHINE_I386,0,0,&ImageBase);
	if(Status != ESUCCESS)
		return Status;

	//
	// search the loaded image's memory allocation descriptor and update firmware's memory descriptor
	//
	PLIST_ENTRY NextEntry								= BlLoaderBlock->MemoryDescriptorListHead.Flink;
	ULONG BasePage										= GET_PHYSICAL_PAGE(ImageBase);
	while(NextEntry != &BlLoaderBlock->MemoryDescriptorListHead)
	{
		PMEMORY_ALLOCATION_DESCRIPTOR AllocationDesc	= CONTAINING_RECORD(NextEntry,MEMORY_ALLOCATION_DESCRIPTOR,ListEntry);

		if(AllocationDesc->BasePage	== BasePage)
		{
			ULONG EndPage								= AllocationDesc->BasePage + AllocationDesc->PageCount;
			if(EndPage == 0)
				return EINVAL;

			ARC_STATUS MempAllocDescriptor(__in ULONG StartPage,__in ULONG EndPage,__in TYPE_OF_MEMORY MemoryType);
			if(MempAllocDescriptor(BasePage,EndPage,MemoryFirmwareTemporary) != ESUCCESS)
				return EINVAL;

			break;
		}
	}

	//
	// allocate a data table entry
	//
	PLDR_DATA_TABLE_ENTRY DriverDataTableEntry			= 0;
	Status												= BlAllocateDataTableEntry("NTBOOTDD.SYS","\\NTBOOTDD.SYS",ImageBase,&DriverDataTableEntry);
	if(Status != ESUCCESS)
		return Status;

	//
	// initialize list entry
	//
	InitializeListHead(&DriverDataTableEntry->InLoadOrderLinks);

	//
	// Scan the import table and bind to osloader
	//
	Status = BlScanOsloaderBoundImportTable(DriverDataTableEntry);
	if(Status != ESUCCESS)
		return Status;

	//
	// enum all disks
	//
	AEGetArcDiskInformation();

	//
	// call driver entry
	//
	typedef NTSTATUS (*PDRIVER_ENTRY)(__in PVOID DriverObject,__in PVOID Parameter2);
	PDRIVER_ENTRY Entry									= static_cast<PDRIVER_ENTRY>(DriverDataTableEntry->EntryPoint);
	Status												= (*Entry)(0,0);
	if(Status != STATUS_SUCCESS)
		return Status;

	//
	// allocate disk entry array
	//
	PVOID Buffer										= FwAllocateHeap(32 * sizeof(DRIVER_LOOKUP_ENTRY));
	if(!Buffer)
		return ENOMEM;

	//
	// initizlize harddisk
	//
	HardDiskInitialize(Buffer,32,0);

	//
	// disable bios disk
	//
	AEBiosDisabled										= TRUE;
#endif
	return ESUCCESS;
}

//
// translate dos drive name to arc name
//
VOID BlpTranslateDosToArc(__in PCHAR DosName,__out PCHAR ArcName)
{
	ULONG DriveId										= 0;
	__try
	{
		//
		// eliminate the easy ones first.
		//    A: is always "multi(0)disk(0)fdisk(0)partition(0)"
		//    B: is always "multi(0)disk(0)fdisk(1)partition(0)"
		//    C: is always "multi(0)disk(0)rdisk(0)partition(1)"
		//
		if(!_stricmp(DosName,"A:"))
			try_leave(strcpy(ArcName,"multi(0)disk(0)fdisk(0)partition(0)"));

		if(!_stricmp(DosName,"B:"))
			try_leave(strcpy(ArcName,"multi(0)disk(0)fdisk(1)partition(0)"));

		if(!_stricmp(DosName,"C:"))
			try_leave(strcpy(ArcName,"multi(0)disk(0)rdisk(0)partition(1)"));

		//
		// if there are two drives, then D: is the primary partition on the second drive.s
		// uccessive letters are the secondary partitions on the first drive,
		// then back to the second drive when that runs out.
		//
		// the exception to this is when there is no primary partition on the second drive.
		// then, we letter the partitions on the first driver consecutively,
		// and when those partitions run out,we letter the partitions on the second drive.
		//

		//
		// try to open the second drive.if this doesn't work, we only have one drive and life is easy.
		//
		ARC_STATUS Status								= ArcOpen("multi(0)disk(0)rdisk(1)partition(0)",ArcOpenReadOnly,&DriveId);

		//
		// we only have one drive, so whatever drive letter he's requesting has got to be on it.
		//
		if(Status != ESUCCESS)
			try_leave(sprintf(ArcName,"multi(0)disk(0)rdisk(0)partition(%d)", toupper(DosName[0]) - 'C' + 1));

		//
		// seek to mbr
		//
		LARGE_INTEGER SeekPosition;
		SeekPosition.QuadPart							= 0;
		Status											= ArcSeek(DriveId,&SeekPosition,SeekAbsolute);
		if(Status != ESUCCESS)
			try_leave(ArcName[0] = 0);

		//
		// read the partition table off the second drive,so we can tell if there is a primary partition or not.
		//
		UCHAR DataBuffer[SECTOR_SIZE];
		ULONG Count										= 0;
		Status											= ArcRead(DriveId,DataBuffer,SECTOR_SIZE,&Count);
		ArcCacheClose(DriveId);
		DriveId											= 0;

		if(Status != ESUCCESS)
			try_leave(ArcName[0] = 0);

		//
		// check the second driver to see if there is a primary partition
		//
		BOOLEAN HasPrimary								= FALSE;
		for(ULONG i = 0; i < 4; i ++)
		{
			UCHAR PartitionType							= *Add2Ptr(DataBuffer,0x1be + i * 0x10,PUCHAR);

		}

		//
		// go through and count the partitions on the first drive.
		//
		ULONG PartitionCount							= 0;

		while(1)
		{
			PartitionCount								+= 1;
			sprintf(ArcName,"multi(0)disk(0)rdisk(0)partition(%d)",PartitionCount + 1);

			//
			// we do this by just constructing ARC names for each successive partition until one BiosPartitionOpen call fails.
			//
			ULONG PartitionId							= 0;
			Status										= BiosPartitionOpen(ArcName,ArcOpenReadOnly,&PartitionId);
			if(Status == ESUCCESS)
				BiosPartitionClose(PartitionId);
			else
				break;
		}

		ULONG PartitionNumber							= toupper(DosName[0]) - 'C' + 1;

		if(HasPrimary)
		{
			//
			// there is Windows NT primary partition on the second drive.
			//
			// if the DosName is "D:" then we know this is the first partition on the second drive.
			//
			if(!_stricmp(DosName,"D:"))
				try_leave(strcpy(ArcName,"multi(0)disk(0)rdisk(1)partition(1)"));


			if(PartitionNumber - 1 > PartitionCount)
			{
				PartitionNumber							-= PartitionCount;
				sprintf(ArcName,"multi(0)disk(0)rdisk(1)partition(%d)",PartitionNumber);
			}
			else
			{
				sprintf(ArcName,"multi(0)disk(0)rdisk(0)partition(%d)",PartitionNumber - 1);
			}
		}
		else
		{
			//
			// There is no primary partition on the second drive, so we
			// consecutively letter the partitions on the first drive,
			// then the second drive.
			//
			if(PartitionNumber > PartitionCount)
			{
				PartitionNumber							-= PartitionCount;
				sprintf(ArcName,"multi(0)disk(0)rdisk(1)partition(%d)",PartitionNumber);
			}
			else
			{
				sprintf(ArcName,"multi(0)disk(0)rdisk(0)partition(%d)",PartitionNumber);
			}
		}
	}
	__finally
	{
		if(DriveId)
			ArcCacheClose(DriveId);
	}
}

//
// vectors
//
PVOID GlobalFirmwareVectors[MaximumRoutine] =
{
	&BlArcNotYetImplemented,				// LoadRoutine
	&BlArcNotYetImplemented,				// InvokeRoutine
	&BlArcNotYetImplemented,				// ExecuteRoutine
	&BlArcNotYetImplemented,				// HaltRoutine
	&BlArcNotYetImplemented,				// PowerDownRoutine
	&AEReboot,								// RestartRoutine
	&AEReboot,								// RebootRoutine
	&BlArcNotYetImplemented,				// InteractiveModeRoutine
	&BlArcNotYetImplemented,				// Reserved1
	&FwGetPeer,								// GetPeerRoutine
	&FwGetChild,							// GetChildRoutine
	&AEGetParent,							// GetParentRoutine
	&AEGetConfigurationData,				// GetDataRoutine
	&BlArcNotYetImplemented,				// AddChildRoutine
	&BlArcNotYetImplemented,				// DeleteComponentRoutine
	&FwGetComponent,						// GetComponentRoutine
	&BlArcNotYetImplemented,				// SaveConfigurationRoutine
	&BlArcNotYetImplemented,				// GetSystemIdRoutine
	&AEGetMemoryDescriptor,					// MemoryRoutine
	&BlArcNotYetImplemented,				// Reserved2
	&AEGetTime,								// GetTimeRoutine
	&AEGetRelativeTime,						// GetRelativeTimeRoutine
	&BlArcNotYetImplemented,				// GetDirectoryEntryRoutine
	&AEOpen,								// OpenRoutine
	&AEClose,								// CloseRoutine
	&AERead,								// ReadRoutine
	&AEReadStatus,							// ReadStatusRoutine
	&AEWrite,								// WriteRoutine
	&AESeek,								// SeekRoutine
	&BlArcNotYetImplemented,				// MountRoutine
	&AEGetEnvironment,						// GetEnvironmentRoutine
	&BlArcNotYetImplemented,				// SetEnvironmentRoutine
	&AEGetFileInformation,					// GetFileInformationRoutine
	&BlArcNotYetImplemented,				// SetFileInformationRoutine
	&BlArcNotYetImplemented,				// FlushAllCachesRoutine
	&BlArcNotYetImplemented,				// TestUnicodeCharacterRoutine
	&BlArcNotYetImplemented,				// GetDisplayStatusRoutine
};