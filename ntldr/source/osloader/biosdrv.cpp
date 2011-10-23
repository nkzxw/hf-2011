//********************************************************************
//	created:	13:8:2008   8:20
//	file:		biosdrv.cpp
//	author:		tiamo
//	purpose:	bios device
//********************************************************************

#include "stdafx.h"

//
// disk cache size
//
#define FW_DISK_CACHE_SIZE								(0x48 << SECTOR_SHIFT)

//
// key buffer
//
UCHAR													KeyBuffer[16];

//
// start
//
ULONG													KeyBufferStart;

//
// end
//
ULONG													KeyBufferEnd;

//
// control seq
//
BOOLEAN													ControlSequence;

//
// font selection
//
BOOLEAN													FontSelection;

//
// escape sequence
//
BOOLEAN													EscapeSequence;

//
// parameters
//
ULONG													Parameter[11];

//
// parameters count
//
ULONG													PCount;

//
// translate char
//
UCHAR													TranslateColor[] = {0,4,2,6,1,5,3,7};

//
// high light
//
BOOLEAN													HighIntensity;

//
// blink
//
BOOLEAN													Blink;

//
// force int13 ext
//
BOOLEAN													ForceInt13Ext;

//
// partition routine
//
extern BL_DEVICE_ENTRY_TABLE							BiosPartitionEntryTable;

//
// disk routine
//
extern BL_DEVICE_ENTRY_TABLE							BiosDiskEntryTable;

//
// cdrom routine
//
extern BL_DEVICE_ENTRY_TABLE							BiosEDDSEntryTable;

//
// last sector cache
//
struct _LAST_SECTOR_CACHE
{
	//
	// buffer allocated
	//
	BOOLEAN												BufferAllocated;

	//
	// valid
	//
	BOOLEAN												Valid;

	//
	// disk id
	//
	ULONG												DiskId;

	//
	// LBA
	//
	ULONGLONG											CachedLBA;

	//
	// buffer
	//
	PVOID												Buffer;
}FwLastSectorCache;

//
// open bios console
//
ARC_STATUS BiosConsoleOpen(__in PCHAR OpenPath,__in OPEN_MODE OpenMode,__out PULONG FileId)
{
	if(!_stricmp(OpenPath,"multi(0)key(0)keyboard(0)"))
	{
		//
		// open the keyboard for input
		//
		if(OpenMode != ArcOpenReadOnly)
			return EACCES;

		*FileId											= 0;

		return ESUCCESS;
	}

	if(!_stricmp(OpenPath,"multi(0)video(0)monitor(0)"))
	{
		//
		// open the display for output
		//
		if(OpenMode != ArcOpenWriteOnly)
			return EACCES;

		*FileId											= 1;

		return ESUCCESS;
	}

	return ENOENT;
}

//
// fill keyboard buffer
//
VOID BiosConsoleFillBuffer(__in ULONG Key)
{
	switch(Key)
	{
		//
		// ESC
		//
	case ESC_KEY:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

	case TAB_KEY:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= '\t';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

		//
		// 0x3b00 - F1
		//
	case F1_KEY:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'O';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'P';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

		//
		// 0x3c00 - F2
		//
	case F2_KEY:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'O';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'Q';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

		//
		// 0x3d00 - F3
		//
	case F3_KEY:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'O';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'R';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

		//
		// 0x3f00 - F5
		//
	case F5_KEY:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'O';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'T';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

		//
		// 0x4000 - F6
		//
	case F6_KEY:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'O';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'U';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

	case F7_KEY:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'O';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'V';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

	case F8_KEY:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'O';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'W';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

	case F10_KEY:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'O';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'Y';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

		//
		// 0x4700 - HOME
		//
	case HOME_KEY:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'H';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

		//
		// 0x4800 - UP
		//
	case UP_ARROW:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'A';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

	case LEFT_ARROW:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'D';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

	case RIGHT_ARROW:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'C';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

		//
		// 0x4f00 - END
		//
	case END_KEY:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'K';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

		//
		// 0x5000 - DOWN
		//
	case DOWN_ARROW:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'B';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

	case INSERT_KEY:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= '@';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

	case DEL_KEY:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'P';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

	case 0xd900:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'O';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'A';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

	case 0xda00:
		KeyBuffer[KeyBufferEnd]							= ASCI_CSI_IN;
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'O';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		KeyBuffer[KeyBufferEnd]							= 'B';
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;

	default:
		KeyBuffer[KeyBufferEnd]							= static_cast<UCHAR>(Key & 0xff);
		KeyBufferEnd									= (KeyBufferEnd + 1) % ARRAYSIZE(KeyBuffer);
		break;
	}
}

//
// get key
//
ULONG BlGetKey()
{
	if(ArcGetReadStatus(BlConsoleInDeviceId) != ESUCCESS)
		return 0;

	UCHAR Input;
	ULONG Count;
	ArcRead(BlConsoleInDeviceId,&Input,sizeof(Input),&Count);
	if(Input != ASCI_CSI_IN)
	{
		if(Input == ASCI_BS)
			return BACKSPACE_KEY;

		if(Input == ASCI_HT)
			return TAB_KEY;

		return Input;
	}

	if(ArcGetReadStatus(BlConsoleInDeviceId) != ESUCCESS)
		return ESC_KEY;

	ArcRead(BlConsoleInDeviceId,&Input,sizeof(Input),&Count);

	switch(Input)
	{
	case '\t':
		return TAB_KEY;
		break;

	case '@':
		return INSERT_KEY;
		break;

	case 'A':
		return UP_ARROW;
		break;

	case 'B':
		return DOWN_ARROW;
		break;

	case 'C':
		return RIGHT_ARROW;
		break;

	case 'D':
		return LEFT_ARROW;
		break;

	case 'H':
		return HOME_KEY;
		break;

	case 'K':
		return END_KEY;
		break;

	case 'P':
		return DEL_KEY;
		break;

	case 'O':
		ArcRead(BlConsoleInDeviceId,&Input,sizeof(Input),&Count);
		switch(Input)
		{
		case 'P':
			return F1_KEY;
			break;

		case 'Q':
			return F2_KEY;
			break;

		case 'R':
		case 'w':
			return F3_KEY;
			break;

		case 'S':
		case 'x':
			return F4_KEY;
			break;

		case 'T':
		case 't':
			return F5_KEY;
			break;

		case 'U':
		case 'u':
			return F6_KEY;
			break;

		case 'V':
		case 'q':
			return F7_KEY;
			break;

		case 'W':
		case 'r':
			return F8_KEY;
			break;

		case 'X':
		case 'p':
			return F9_KEY;
			break;

		case 'Y':
		case 'M':
			return F10_KEY;
			break;

		case 'Z':
		case 'A':
			return F11_KEY;
			break;

		case '[':
		case 'B':
			return F12_KEY;
			break;
		}
		break;
	}

	return 0;
}

//
// read console status
//
ARC_STATUS BiosConsoleReadStatus(__in ULONG FileId)
{
	//
	// something in the key buffer
	//
	if(KeyBufferEnd != KeyBufferStart)
		return ESUCCESS;

	//
	// check key
	//
	ULONG Key											= ExternalServicesTable->GetKey();
	if(!Key)
		return EAGAIN;

	//
	// got a key,stick it back into our buffer
	//
	BiosConsoleFillBuffer(Key);

	return ESUCCESS;
}

//
// read console
//
ARC_STATUS BiosConsoleRead(__in ULONG FileId,__out PUCHAR Buffer,__in ULONG Length,__out PULONG Count)
{
	//
	// poll the debugger,actually,this should be put in a timer interrupt handler,but boot loader is running with interrupt-off
	// so put here
	//
	BdPollConnection();

	*Count												= 0;

	while(*Count < Length)
	{
		//
		// if buffer is empty
		//
		if(KeyBufferEnd == KeyBufferStart)
		{
			ULONG Key									= 0;
			do
			{
				//
				// poll the keyboard until input is available
				//
				Key										= ExternalServicesTable->GetKey();
			}while(!Key);

			BiosConsoleFillBuffer(Key);
		}

		Buffer[*Count]									= KeyBuffer[KeyBufferStart];
		KeyBufferStart									= (KeyBufferStart + 1) % ARRAYSIZE(KeyBuffer);

		*Count											+= 1;
	}

	return ESUCCESS;
}

//
// write console
//
ARC_STATUS BiosConsoleWrite(__in ULONG FileId,__out PUCHAR Buffer,__in ULONG Length,__out PULONG Count)
{
	//
	// process each character in turn.
	//
	ARC_STATUS Status									= ESUCCESS;
	PUCHAR String										= Buffer;

	for(*Count = 0; *Count < Length; (*Count) ++, String ++)
	{
		//
		// if we're in the middle of a control sequence, continue scanning, otherwise process character.
		//
		if(ControlSequence)
		{
			//
			// if the character is a digit, update parameter value.
			//
			if(*String >= '0' && *String <= '9')
			{
				Parameter[PCount]						= Parameter[PCount] * 10 + *String - '0';
				continue;
			}

			//
			// if we are in the middle of a font selection sequence, this character must be a 'D', otherwise reset control sequence.
			//
			if(FontSelection)
			{
				ControlSequence							= FALSE;
				FontSelection							= FALSE;
				continue;
			}

			switch(*String)
			{
				//
				// if a semicolon, move to the next parameter.
				//
			case ';':
				PCount									+= 1;
				if (PCount > ARRAYSIZE(Parameter) - 1)
					PCount = ARRAYSIZE(Parameter) - 1;

				Parameter[PCount]						= 0;
				break;

				//
				// if a 'J', erase part or all of the screen.
				//
			case 'J':
				switch(Parameter[0])
				{
				case 0:
					//
					// erase to end of the screen
					//
					TextClearToEndOfDisplay();
					break;

				case 1:
					//
					// erase from the beginning of the screen
					//
					break;

				default:
					//
					// erase entire screen
					//
					TextClearDisplay();
					break;
				}

				ControlSequence						= FALSE;
				break;

				//
				// if a 'K', erase part or all of the line.
				//
			case 'K':
				switch(Parameter[0])
				{
					//
					// erase to end of the line.
					//
				case 0:
					TextClearToEndOfLine();
					break;

					//
					// erase from the beginning of the line.
					//
				case 1:
					TextClearFromStartOfLine();
					break;

					//
					// erase entire line.
					//
				default:
					TextClearFromStartOfLine();
					TextClearToEndOfLine();
					break;
				}

				ControlSequence							= FALSE;
				break;

				//
				// if a 'H', move cursor to position.
				//
			case 'H':
				TextSetCursorPosition(Parameter[1] - 1, Parameter[0] - 1);
				ControlSequence							= FALSE;
				break;

				//
				// if a ' ', could be a FNT selection command.
				//
			case ' ':
				FontSelection							= TRUE;
				break;

			case 'm':
				HighIntensity							= FALSE;
				Blink									= FALSE;

				//
				// select action based on each parameter.
				//
				for(ULONG Index = 0 ; Index <= PCount ; Index ++)
				{
					switch(Parameter[Index])
					{
						//
						// attributes off.
						//
					case 0:
						TextSetCurrentAttribute(7);
						HighIntensity					= FALSE;
						Blink							= FALSE;
						break;

						//
						// high intensity.
						//
					case 1:
						TextSetCurrentAttribute(0xf);
						HighIntensity					= TRUE;
						break;

						//
						// underscored.
						//
					case 4:
						TextSetCurrentAttribute(0x87);
						Blink							= TRUE;
						break;

						//
						// reverse Video.
						//
					case 7:
						TextSetCurrentAttribute(0x70);
						HighIntensity					= FALSE;
						Blink							= FALSE;
						break;

						//
						// font selection, not implemented yet.
						//
					case 10:
					case 11:
					case 12:
					case 13:
					case 14:
					case 15:
					case 16:
					case 17:
					case 18:
					case 19:
						break;

						//
						// foreground Color
						//
					case 30:
					case 31:
					case 32:
					case 33:
					case 34:
					case 35:
					case 36:
					case 37:
						{
							UCHAR a						= (TextGetCurrentAttribute() & 0x70) | TranslateColor[Parameter[Index] - 30];

							if(HighIntensity)
								a						|= 0x08;

							if(Blink)
								a						|= 0x80;

							TextSetCurrentAttribute(a);
						}
						break;

						//
						// background Color
						//
					case 40:
					case 41:
					case 42:
					case 43:
					case 44:
					case 45:
					case 46:
					case 47:
						TextSetCurrentAttribute((TextGetCurrentAttribute() & 0x8f) | (TranslateColor[Parameter[Index] - 40] << 4));
						break;

					default:
						break;
					}
				}

			default:
				ControlSequence = FALSE;
				break;
			}
		}
		else
		{
			//
			// if escape sequence, check for control sequence, otherwise process single character.
			//
			if(EscapeSequence)
			{
				//
				// check for '[', means control sequence, any other following character is ignored.
				//
				if(*String == '[')
				{
					ControlSequence						= TRUE;

					//
					// initialize first parameter.
					//
					PCount								= 0;
					Parameter[0]						= 0;
				}

				EscapeSequence							= FALSE;
			}
			else
			{
				//
				// this is not a control or escape sequence, process single character.
				//
				switch(*String)
				{
					//
					// check for escape sequence.
					//
				case ASCI_ESC:
					EscapeSequence						= TRUE;
					break;

				default:
					{
						//
						// each pass through the loop increments String by 1.if we output a dbcs char we need to increment by one more.
						//
						PUCHAR p						= static_cast<PUCHAR>(static_cast<PVOID>(TextCharOut(static_cast<PCHAR>(static_cast<PVOID>(String)))));
						(*Count)						+= (p - String) - 1;
						String							+= (p - String) - 1;
					}
					break;
				}

			}
		}
	}
	return Status;
}

//
// read/write disk using LBA
//
ARC_STATUS XferExtendedPhysicalDiskSectors(__in ULONG DriveId,__in ULONGLONG LBA,__in ULONG Count,__in PVOID Buffer,__in BOOLEAN WriteDisk)
{
	//
	// buffer must below 1MB for real mode access
	//
	if(Add2Ptr(Buffer,Count >> SECTOR_SHIFT,ULONG) >= 1 * 1024 * 1024)
		return EFAULT;

	//
	// zero length
	//
	if(!Count)
		return ESUCCESS;

	//
	// retry 3 times
	//
	//	int 13 - ibm/ms int 13 extensions - installation check  ah = 41h
	//	int 13 - ibm/ms int 13 extensions - extended read       ah = 42h
	//	int 13 - ibm/ms int 13 extensions - extended write      ah = 43h
	//	int 13 - disk                     - reset disk system   ah = 00h
	//
	if(!ExternalServicesTable->ExtendedDiskIOSystem(DriveId,LBA,Count,Buffer,WriteDisk ? 0x43 : 0x42))
		return ESUCCESS;

	if(!ExternalServicesTable->ExtendedDiskIOSystem(DriveId,LBA,Count,Buffer,WriteDisk ? 0x43 : 0x42))
		return ESUCCESS;

	return ExternalServicesTable->ExtendedDiskIOSystem(DriveId,LBA,Count,Buffer,WriteDisk ? 0x43 : 0x42);
}

//
// read/write disk
//
ARC_STATUS XferPhysicalDiskSectors(__in ULONG DriveId,__in ULONGLONG LBA,__in ULONG TransferSectors,__in PVOID Buffer,__in ULONG SectorsPerTrack,
								   __in ULONG Heads,__in ULONG Cylinders,__in BOOLEAN TryExtendedMethod,__in BOOLEAN WriteDisk)
{
	//
	// figure out CHS values to address the given sector
	//	lba = c*num_heads*sectors_per_track + h*sectors_per_track + s - 1
	//
	ULONGLONG SectorsPerCylinder						= static_cast<ULONGLONG>(Heads) * SectorsPerTrack;
	ULONG Cylinder										= static_cast<ULONG>(LBA / SectorsPerCylinder);
	ULONGLONG r											= LBA % SectorsPerCylinder;
	ULONG Head											= static_cast<ULONG>(r / SectorsPerTrack);
	ULONG Sector										= static_cast<ULONG>(r % SectorsPerTrack) + 1;

	//
	// use int13
	//
	if(Cylinder < Cylinders && Cylinder < 1024)
	{
		if(!ExternalServicesTable->DiskIOSystem(WriteDisk ? 3 : 2,DriveId,Head,Cylinder,Sector,TransferSectors,Buffer))
			return ESUCCESS;
	}

	if(TryExtendedMethod)
		return XferExtendedPhysicalDiskSectors(DriveId,LBA,TransferSectors,Buffer,WriteDisk);

	if(Cylinder >= Cylinders || Cylinder >= 1024)
		return E2BIG;

	ULONG RetryCount									= DriveId < 0x80 ? 3 : 2;
	ARC_STATUS Status;
	for(ULONG i = 0; i < RetryCount; i ++)
	{
		Status											= ExternalServicesTable->DiskIOSystem(WriteDisk ? 3 : 2,DriveId,Head,Cylinder,Sector,TransferSectors,Buffer);
		if(Status == ESUCCESS)
			return ESUCCESS;

		ExternalServicesTable->DiskIOSystem(DriveId < 0x80 ? 0 : 13,DriveId,0,0,0,0,0);
	}

	return Status;
}

//
// open bios disk
//	drive id:
// 		0 - Floppy 0
//		1 - Floppy 1
//		0x80 - Hard Drive 0
//		0x81 - Hard Drive 1
//		0x82 - Hard Drive 2
//		.....
//		high bit set and ID > 0x81 means the device is expected to be a CD-ROM drive.
//
ARC_STATUS BiosDiskOpen(__in ULONG DriveId,__in OPEN_MODE OpenMode,__out PULONG FileId)
{
	BOOLEAN IsCd										= DriveId > 0x80000081;
	DriveId												&= 0x7fffffff;
	PVOID Buffer										= FwDiskCache;
	ARC_STATUS Return									= ESUCCESS;
	USHORT NumberHeads									= 0;
	USHORT NumberCylinders								= 0;
	USHORT NumberDrives									= 0;
	USHORT NumberSectors								= 0;
	BOOLEAN Int13Ext									= FALSE;

	//
	// if we are opening Floppy 0 or Floppy 1, we want to read the BPB off the disk so we can deal with all the odd disk formats.
	// if we are opening a hard drive, we can just call the BIOS to find out its characteristics
	//
	if(DriveId < 0x80)
	{
		//
		// read the boot sector off the floppy and extract the cylinder,sector,and head information.
		//
		Return											= XferPhysicalDiskSectors(DriveId,0,1,Buffer,1,1,1,FALSE,FALSE);
		if(Return != ESUCCESS)
			return EIO;

		NumberHeads										= *Add2Ptr(Buffer,0x1a,PUSHORT);
		NumberSectors									= *Add2Ptr(Buffer,0x18,PUSHORT);
		NumberCylinders									= *Add2Ptr(Buffer,0x13,PUSHORT) / (NumberHeads * NumberSectors);
	}
	else if(IsCd)
	{
		//
		// this is an El Torito drive,just use bogus values since CHS values are meaningless for no-emulation El Torito boot
		//
		NumberCylinders									= 1;
		NumberHeads										= 1;
		NumberSectors									= 1;
	}
	else
	{
		ULONG Retries									= 0;
		UCHAR NumberDrives								= 0;
		do
		{
			//
			// bios get disk parameters
			//
			ULONG Result								= 0;
			NTSTATUS Return								= ExternalServicesTable->DiskIOSystem(0x08,DriveId,0,0,0,0,0);
			//
			// At this point, AH should hold our return code, and ECX should look
			// like this:
			//    bits 31..22  - Maximum cylinder
			//    bits 21..16  - Maximum sector
			//    bits 15..8   - Maximum head
			//    bits 7..0    - Number of drives
			//
			_asm
			{
				mov Result, ecx
			}

			if(Return)
				return EIO;

			//
			// unpack the information from ecx
			//
			NumberHeads									= (static_cast<UCHAR>(Result >> 8) & 0xff) + 1;
			NumberSectors								= static_cast<UCHAR>(Result >> 16) & 0x3f;
			NumberCylinders								= static_cast<UCHAR>(((Result >> 24) + ((Result >> 14) & 0x300)) + 1);
			NumberDrives								= static_cast<UCHAR>(Result & 0xff);
			Retries										+= 1;

		}while((!NumberHeads || !NumberSectors || !NumberCylinders) && Retries < 5);

		//
		// the requested DriveId does not exist
		//
		if((DriveId & 0x7f) >= NumberDrives || Retries == 5)
			return EIO;

		RtlZeroMemory(Buffer,SECTOR_SIZE);

		Int13Ext										= ForceInt13Ext || ExternalServicesTable->CheckInt13ExtensionSupported(DriveId,Buffer);
	}

	//
	// find an available FileId descriptor to open the device with
	//
	*FileId												= 2;
	while(BlFileTable[*FileId].Flags.Open)
	{
		*FileId											+= 1;
		if(*FileId == ARRAYSIZE(BlFileTable))
			return ENOENT;
	}

	//
	// we found an entry we can use, so mark it as open.
	//
	BlFileTable[*FileId].Flags.Open						= TRUE;

	//
	// we're using the Phoenix Enhanced Disk Drive Spec for cdrom
	//
	if(IsCd)
		BlFileTable[*FileId].DeviceEntryTable			= &BiosEDDSEntryTable;
	else
		BlFileTable[*FileId].DeviceEntryTable			= &BiosDiskEntryTable;

	BlFileTable[*FileId].u.DriveContext.IsCdRom			= IsCd;
	BlFileTable[*FileId].u.DriveContext.DriveId			= static_cast<UCHAR>(DriveId);
	BlFileTable[*FileId].u.DriveContext.Cylinders		= NumberCylinders;
	BlFileTable[*FileId].u.DriveContext.Heads			= NumberHeads;
	BlFileTable[*FileId].u.DriveContext.Sectors			= NumberSectors;
	BlFileTable[*FileId].u.DriveContext.Int13Ext		= Int13Ext;

	return ESUCCESS;
}

//
// close disk
//
ARC_STATUS BiosDiskClose(__in ULONG FileId)
{
	if(!BlFileTable[FileId].Flags.Open)
		BlPrint("ERROR - Unopened fileid %lx closed\r\n",FileId);

	BlFileTable[FileId].Flags.Open						= FALSE;

	if(FwLastSectorCache.DiskId	== FileId)
		FwLastSectorCache.Valid							= FALSE;

	return ESUCCESS;
}

//
// write partial sector
//
ARC_STATUS BiospWritePartialSector(__in ULONG DriveId,__in ULONGLONG LBA,__in PVOID Buffer,__in BOOLEAN IsHead,__in ULONG Bytes,
								   __in ULONG SectorsPerTrack,__in ULONG Heads,__in ULONG Cylinders,__in BOOLEAN TryExMethod)
{
	//
	// read sector into the write buffer
	//
	ARC_STATUS Status									= XferPhysicalDiskSectors(DriveId,LBA,1,FwDiskCache,SectorsPerTrack,Heads,Cylinders,TryExMethod,FALSE);
	if(Status != ESUCCESS)
		return(Status);

	//
	// xfer the appropriate bytes from the user buffer to the write buffer
	//
	RtlMoveMemory(IsHead ? Add2Ptr(FwDiskCache,Bytes,PVOID) : FwDiskCache,Buffer,IsHead ? SECTOR_SIZE - Bytes : Bytes);

	//
	// write the sector out
	//
	Status												= XferPhysicalDiskSectors(DriveId,LBA,1,FwDiskCache,SectorsPerTrack,Heads,Cylinders,TryExMethod,TRUE);
	return Status;
}

//
// write disk
//
ARC_STATUS BiosDiskWrite(IN ULONG FileId, OUT PVOID Buffer, IN ULONG Length, OUT PULONG Count)
{
	ULONGLONG HeadLBA									= BlFileTable[FileId].Position.QuadPart >> SECTOR_SHIFT;
	ULONGLONG HeadOffset								= BlFileTable[FileId].Position.QuadPart % SECTOR_SIZE;
	ULONGLONG TailLBA									= (BlFileTable[FileId].Position.QuadPart + Length) >> SECTOR_SHIFT;
	ULONGLONG TailByteCount								= (BlFileTable[FileId].Position.QuadPart + Length) % SECTOR_SIZE;
	ULONGLONG CurrentLBA								= HeadLBA;
	ULONG SectorsPerTrack								= BlFileTable[FileId].u.DriveContext.Sectors;
	ULONG Heads											= BlFileTable[FileId].u.DriveContext.Heads;
	ULONG Cylinders										= BlFileTable[FileId].u.DriveContext.Cylinders;
	ULONGLONG SectorsPerCylinder						= static_cast<ULONGLONG>(SectorsPerTrack) * Heads;
	BOOLEAN UseExt										= BlFileTable[FileId].u.DriveContext.Int13Ext;
	ULONG DriveId										= BlFileTable[FileId].u.DriveContext.DriveId;
	ULONG LengthLeftToTransfer							= Length;
	PVOID UserBuffer									= Buffer;
	ARC_STATUS Status									= ESUCCESS;

	//
	// check last sector cache
	//
	if(FwLastSectorCache.BufferAllocated && FwLastSectorCache.Valid && FwLastSectorCache.CachedLBA >= HeadLBA && FwLastSectorCache.CachedLBA <= TailLBA)
		FwLastSectorCache.Valid							= FALSE;

	//
	// special case of transfer occuring entirely within one sector
	//
	if(HeadOffset && TailByteCount && HeadLBA == TailLBA)
	{
		Status											= XferPhysicalDiskSectors(DriveId,HeadLBA,1,FwDiskCache,SectorsPerTrack,Heads,Cylinders,UseExt,TRUE);
		if(Status != ESUCCESS)
			return Status;

		RtlMoveMemory(Add2Ptr(FwDiskCache,HeadOffset,PVOID),Buffer,Length);

		Status											= XferPhysicalDiskSectors(DriveId,HeadLBA,1,FwDiskCache,SectorsPerTrack,Heads,Cylinders,UseExt,TRUE);
		if(Status != ESUCCESS)
			return Status;

		*Count											= Length;
		BlFileTable[FileId].Position.QuadPart			+= Length;

		return ESUCCESS;
	}

	if(HeadOffset)
	{
		Status											= BiospWritePartialSector(DriveId,HeadLBA,Buffer,TRUE,static_cast<ULONG>(HeadOffset),
																				  SectorsPerTrack,Heads,Cylinders,UseExt);

		if(Status != ESUCCESS)
			return Status;

		LengthLeftToTransfer							-= SECTOR_SIZE - static_cast<ULONG>(HeadOffset);
		UserBuffer										= Add2Ptr(UserBuffer,SECTOR_SIZE - static_cast<ULONG>(HeadOffset),PVOID);
		CurrentLBA										+= 1;
	}

	if(TailByteCount)
	{
		Status											= BiospWritePartialSector(DriveId,TailLBA,Add2Ptr(Buffer,Length - static_cast<ULONG>(TailByteCount),PVOID),FALSE,
																				  static_cast<ULONG>(TailByteCount),SectorsPerTrack,Heads,Cylinders,UseExt);

		if(Status != ESUCCESS)
			return Status;

		LengthLeftToTransfer							-= static_cast<ULONG>(TailByteCount);
	}

	//
	// the following calculation is not inside the transfer loop because it is unlikely that a caller's buffer will *cross* the 1 meg line due to the PC memory map.
	//
	BOOLEAN	Under1MegLine								= Add2Ptr(UserBuffer,LengthLeftToTransfer,ULONG) <= 1 * 1024 * 1024;

	//
	// now handle the middle part.this is some number of whole sectors.
	//
	while(LengthLeftToTransfer)
	{
		//
		// get CHS address of current sector
		//
		ULONG Cylinder									= static_cast<ULONG>(CurrentLBA / SectorsPerCylinder);
		ULONG r											= static_cast<ULONG>(CurrentLBA % SectorsPerCylinder);
		ULONG Head										= r / SectorsPerTrack;
		ULONG Sector									= (r % SectorsPerTrack) + 1;
		PVOID TransferBuffer							= 0;

		//
		// the number of sectors to transfer is the minimum of:
		// - the number of sectors left in the current track
		// - LengthLeftToTransfer / SECTOR_SIZE
		//
		ULONG SectorsToTransfer							= min(SectorsPerTrack - Sector + 1,LengthLeftToTransfer >> SECTOR_SHIFT);

		//
		// if the caller's buffer is under the 1 meg line, we can transfer the data directly from the caller's buffer.
		// otherwise we'll copy the user's buffer to our cache buffer and transfer from there.
		// In the latter case we can only transfer in chunks of 0x48 because that's the size of the cache buffer.
		//
		// also make sure the transfer won't cross a 64k boundary.
		//
		if(Under1MegLine)
		{
			//
			// check if the transfer would cross a 64k boundary.if so, use the cache buffer.  Otherwise use the user's buffer.
			//
			if((Add2Ptr(UserBuffer,0,ULONG) & 0xffff0000) != (Add2Ptr(UserBuffer,(SectorsToTransfer << SECTOR_SHIFT) - 1,ULONG) & 0xffff0000))
			{
				TransferBuffer							= FwDiskCache;
				SectorsToTransfer						= min(SectorsToTransfer,FW_DISK_CACHE_SIZE >> SECTOR_SHIFT);
			}
			else
			{
				TransferBuffer							= UserBuffer;
			}
		}
		else
		{
			TransferBuffer								= FwDiskCache;
			SectorsToTransfer							= min(SectorsToTransfer,FW_DISK_CACHE_SIZE >> SECTOR_SHIFT);
		}

		if(TransferBuffer == FwDiskCache)
			RtlMoveMemory(FwDiskCache,UserBuffer,SectorsToTransfer << SECTOR_SHIFT);

		Status											= XferPhysicalDiskSectors(DriveId,CurrentLBA,SectorsToTransfer,TransferBuffer,
																				  SectorsPerTrack,Heads,Cylinders,UseExt,TRUE);

		if(Status != ESUCCESS)
			break;

		CurrentLBA										+= SectorsToTransfer;
		LengthLeftToTransfer							-= SectorsToTransfer << SECTOR_SHIFT;
		UserBuffer										= Add2Ptr(UserBuffer,SectorsToTransfer << SECTOR_SHIFT,PVOID);
	}

	*Count												= Length - LengthLeftToTransfer;
	BlFileTable[FileId].Position.QuadPart				+= *Count;

	return Status;
}

//
// helper
//
ARC_STATUS pBiosDiskReadWorker(__in ULONG FileId,__out PVOID Buffer,__in ULONG Length,__out PULONG Count,__in ULONG BytesPerSector,__in BOOLEAN IsCdrom)
{
	*Count												= 0;
	if(!Length)
		return ESUCCESS;

	//
	// allocate a last-sector cache
	//
	if(!FwLastSectorCache.BufferAllocated)
	{
		FwLastSectorCache.Buffer						= FwAllocatePool(0x1000);
		if(FwLastSectorCache.Buffer)
			FwLastSectorCache.BufferAllocated			= TRUE;
	}

	ULONGLONG HeadLBA									= BlFileTable[FileId].Position.QuadPart / BytesPerSector;
	ULONGLONG HeadOffset								= BlFileTable[FileId].Position.QuadPart % BytesPerSector;
	ULONGLONG TailLBA									= (BlFileTable[FileId].Position.QuadPart + Length - 1) / BytesPerSector;
	ULONGLONG TailByteCount								= (BlFileTable[FileId].Position.QuadPart + Length - 1) % BytesPerSector + 1;
	ULONG SectorsPerTrack								= BlFileTable[FileId].u.DriveContext.Sectors;
	ULONG Heads											= BlFileTable[FileId].u.DriveContext.Heads;
	ULONG Cylinders										= BlFileTable[FileId].u.DriveContext.Cylinders;
	ULONGLONG SectorsPerCylinder						= static_cast<ULONGLONG>(SectorsPerTrack) * Heads;
	BOOLEAN UseExt										= BlFileTable[FileId].u.DriveContext.Int13Ext;
	ULONG DriveId										= BlFileTable[FileId].u.DriveContext.DriveId;
	ARC_STATUS Status									= ESUCCESS;
	ULONGLONG CurrentLBA								= HeadLBA;
	ULONG LeftBytesLength								= Length;
	ULONG TransferLengthThisRun							= 0;
	ULONGLONG LastSectorLBA								= 0;
	PVOID LastSectorBuffer								= 0;
	PVOID CurrentBuffer									= Buffer;

	//
	// clear KSEG0_BASE to get physical address,if it is below 1MB,we can use the physical address directly
	//
	if(((reinterpret_cast<ULONG>(Buffer) + Length) & ~KSEG0_BASE) < 1 * 1024 * 1024)
		CurrentBuffer									= GET_PHYSICAL_ADDRESS(Buffer);

	while(LeftBytesLength)
	{
		//
		// check we got a cache hit
		//
		if(FwLastSectorCache.Valid && FwLastSectorCache.DiskId == FileId && FwLastSectorCache.CachedLBA == CurrentLBA)
		{
			//
			// we just cached only one sector
			//
			TransferLengthThisRun						= BytesPerSector;

			PVOID CacheBuffer							= FwLastSectorCache.Buffer;
			if(CurrentLBA == HeadLBA)
			{
				//
				// this is the first sector,skip HeadOffset
				//
				CacheBuffer								= Add2Ptr(CacheBuffer,static_cast<ULONG>(HeadOffset),PVOID);
				TransferLengthThisRun					-= static_cast<ULONG>(HeadOffset);
			}

			if(CurrentLBA == TailLBA)
			{
				//
				// also skip tail length
				//
				TransferLengthThisRun					-= (BytesPerSector - static_cast<ULONG>(TailByteCount));
			}

			//
			// copy from cache buffer
			//
			RtlCopyMemory(CurrentBuffer,CacheBuffer,TransferLengthThisRun);

			//
			// goto the next LBA
			//
			CurrentLBA									+= 1;
			*Count										+= TransferLengthThisRun;
			CurrentBuffer								= Add2Ptr(CurrentBuffer,TransferLengthThisRun,PVOID);
			LeftBytesLength								-= TransferLengthThisRun;

			continue;
		}

		//
		// otherwise we need to read it from disk
		//
		ULONG TransferSectorCount						= min(FW_DISK_CACHE_SIZE / BytesPerSector,static_cast<ULONG>(TailLBA - CurrentLBA + 1));

		//
		// add make sure it doesn't cross a track bound
		//
		if(!IsCdrom)
		{
			ULONG LeftSectorsInThisTrack				= static_cast<ULONG>(SectorsPerTrack - CurrentLBA % SectorsPerTrack);
			TransferSectorCount							= min(TransferSectorCount,LeftSectorsInThisTrack);
		}

		//
		// and check we can use user's buffer if
		//	user's buffer is below 1MB
		//	and it does not cross a 64KB bound
		//	and this is not the last sector
		//
		PVOID TransferBuffer							= FwDiskCache;
		TransferLengthThisRun							= TransferSectorCount * BytesPerSector;
		PVOID EndOfTransferBuffer						= Add2Ptr(CurrentBuffer,TransferLengthThisRun,PVOID);
		if( reinterpret_cast<ULONG>(EndOfTransferBuffer) < 1 * 1024 * 1024 &&
			(reinterpret_cast<ULONG>(CurrentBuffer) & 0xffff0000) == (reinterpret_cast<ULONG>(EndOfTransferBuffer) & 0xffff0000) &&
			LeftBytesLength >= TransferLengthThisRun)
		{
			TransferBuffer								= CurrentBuffer;
		}

		//
		// if this is cdrom,we can use int13 extension
		//
		if(IsCdrom)
		{
			Status										= XferExtendedPhysicalDiskSectors(DriveId,CurrentLBA,TransferSectorCount,TransferBuffer,FALSE);
		}
		else
		{
			Status										= XferPhysicalDiskSectors(DriveId,CurrentLBA,TransferSectorCount,TransferBuffer,SectorsPerTrack,
																				  Heads,Cylinders,UseExt,FALSE);
		}

		if(Status != ESUCCESS)
			break;

		//
		// setup last sector info
		//
		LastSectorLBA									= CurrentLBA + TransferSectorCount - 1;
		LastSectorBuffer								= Add2Ptr(TransferBuffer,TransferLengthThisRun - BytesPerSector,PVOID);

		//
		// if we are using cache buffer,we should copy it to user's buffer
		//
		if(TransferBuffer != CurrentBuffer)
		{
			//
			// check head and tail LBA
			//
			if(CurrentLBA == HeadLBA)
			{
				TransferBuffer							= Add2Ptr(TransferBuffer,static_cast<ULONG>(HeadOffset),PVOID);
				TransferLengthThisRun					-= static_cast<ULONG>(HeadOffset);
			}

			if(LastSectorLBA == TailLBA)
				TransferLengthThisRun					-= (BytesPerSector - static_cast<ULONG>(TailByteCount));

			RtlCopyMemory(CurrentBuffer,TransferBuffer,TransferLengthThisRun);
		}

		CurrentLBA										+= TransferSectorCount;
		CurrentBuffer									= Add2Ptr(CurrentBuffer,TransferLengthThisRun,PVOID);
		LeftBytesLength									-= TransferLengthThisRun;
		*Count											+= TransferLengthThisRun;
	}

	//
	// setup last-sector cache
	//
	if(LastSectorBuffer && FwLastSectorCache.BufferAllocated && BytesPerSector <= 0x1000)
	{
		FwLastSectorCache.DiskId						= FileId;
		FwLastSectorCache.Valid							= TRUE;
		FwLastSectorCache.CachedLBA						= LastSectorLBA;
		RtlCopyMemory(FwLastSectorCache.Buffer,LastSectorBuffer,BytesPerSector);
	}

	BlFileTable[FileId].Position.QuadPart				+= *Count;
	return Status;
}

//
// read disk
//
ARC_STATUS BiosDiskRead(__in ULONG FileId,__out PVOID Buffer,__in ULONG Length,__out PULONG Count)
{
	return pBiosDiskReadWorker(FileId,Buffer,Length,Count,SECTOR_SIZE,FALSE);
}

//
// read cdrom
//
ARC_STATUS BiosElToritoDiskRead(__in ULONG FileId,__out PVOID Buffer,__in ULONG Length,__out PULONG Count)
{
	return pBiosDiskReadWorker(FileId,Buffer,Length,Count,2048,TRUE);
}

//
// get disk info
//
ARC_STATUS BiosDiskGetFileInfo(__in ULONG FileId,__out PFILE_INFORMATION Finfo)
{
	if(!Finfo)
		return EINVAL;

	PDRIVE_CONTEXT Context								= &BlFileTable[FileId].u.DriveContext;
	ULONGLONG TotalSize									= static_cast<ULONGLONG>(Context->Cylinders) * Context->Heads * Context->Sectors;
	if(!TotalSize)
		return EINVAL;

	RtlZeroMemory(Finfo,sizeof(FILE_INFORMATION));

	Finfo->StartingAddress.QuadPart						= 0;
	Finfo->EndingAddress.QuadPart						= TotalSize * (Context->IsCdRom ? 2048 : SECTOR_SIZE);
	Finfo->CurrentPosition.QuadPart						= BlFileTable[FileId].Position.QuadPart;
	Finfo->Type											= TotalSize > 3 * 1024 * 1024 ? DiskPeripheral : FloppyDiskPeripheral;

	return ESUCCESS;
}

//
// close partition
//
ARC_STATUS BiosPartitionClose(__in ULONG FileId)
{
	if(!BlFileTable[FileId].Flags.Open)
		BlPrint("ERROR - Unopened fileid %lx closed\r\n",FileId);

	BlFileTable[FileId].Flags.Open						= FALSE;

	return BiosDiskClose(BlFileTable[FileId].u.PartitionContext.DiskId);
}

//
// open partition
//
ARC_STATUS BiosPartitionOpen(__in PCHAR OpenPath,__in OPEN_MODE OpenMode,__out PULONG FileId)
{
	//
	// BIOS devices are always "multi(0)" (except for EISA flakiness where we treat "eisa(0)..." like "multi(0)..." in floppy cases).
	//
	ULONG Key											= 0;
	BOOLEAN IsEisa										= FALSE;
	if(FwGetPathMnemonicKey(OpenPath,"multi",&Key))
	{
		if(FwGetPathMnemonicKey(OpenPath,"eisa", &Key))
			return EBADF;
		else
			IsEisa = TRUE;
	}

	if(Key)
		return EBADF;

	//
	// if we're opening a floppy drive, there are no partitions so we can just return the physical device.
	//
	if(!_stricmp(OpenPath,"multi(0)disk(0)fdisk(0)partition(0)") || !_stricmp(OpenPath,"eisa(0)disk(0)fdisk(0)partition(0)"))
		return BiosDiskOpen(0,ArcOpenReadOnly,FileId);

	if(!_stricmp(OpenPath,"multi(0)disk(0)fdisk(1)partition(0)") || !_stricmp(OpenPath,"eisa(0)disk(0)fdisk(1)partition(0)"))
		return BiosDiskOpen(1,ArcOpenReadOnly,FileId);

	if(!_stricmp(OpenPath,"multi(0)disk(0)fdisk(0)") || !_stricmp(OpenPath,"eisa(0)disk(0)fdisk(0)"))
		return BiosDiskOpen(0,ArcOpenReadOnly,FileId);

	if(!_stricmp(OpenPath,"multi(0)disk(0)fdisk(1)") || !_stricmp(OpenPath,"eisa(0)disk(0)fdisk(1)"))
		return BiosDiskOpen(1,ArcOpenReadOnly,FileId);

	//
	// we can't handle eisa(0) cases for hard disks.
	//
	if(IsEisa)
		return EBADF;

	//
	// we can only deal with disk controller 0
	//
	ULONG Controller									= 0;
	if(FwGetPathMnemonicKey(OpenPath,"disk",&Controller))
		return EBADF;

	if(Controller)
		return EBADF;

	if(!FwGetPathMnemonicKey(OpenPath,"cdrom",&Key))
	{
		//
		// now we have a cd-rom disk number, so we open that for raw access.
		// use a special bit to indicate CD-ROM, because otherwise the BiosDiskOpen routine thinks a third or greater disk is a CD-ROM.
		//
		return BiosDiskOpen(Key | 0x80000000,ArcOpenReadOnly,FileId);
	}

	if(FwGetPathMnemonicKey(OpenPath,"rdisk",&Key))
		return EBADF;

	//
	// now we have a disk number, so we open that for raw access.we need to add 0x80 to translate it to a BIOS number.
	//
	ULONG DiskFileId									= 0;
	ARC_STATUS Status									= BiosDiskOpen(0x80 + Key,ArcOpenReadOnly,&DiskFileId);
	if(Status != ESUCCESS)
		return Status;

	//
	// find the partition number to open
	//
	if(FwGetPathMnemonicKey(OpenPath,"partition",&Key))
	{
		BiosPartitionClose(DiskFileId);
		return EBADF;
	}

	//
	// if the partition number was 0, then we are opening the device for raw access, so we are already done.
	//
	if(!Key)
	{
		*FileId											= DiskFileId;
		return ESUCCESS;
	}

	//
	// before we open the partition, we need to find an available file descriptor.
	//
	*FileId												= 2;
	while(BlFileTable[*FileId].Flags.Open)
	{
		*FileId											+= 1;
		if (*FileId == ARRAYSIZE(BlFileTable))
			return ENOENT;
	}

	//
	// we found an entry we can use, so mark it as open.
	//
	BlFileTable[*FileId].Flags.Open						= 1;
	BlFileTable[*FileId].DeviceEntryTable				= &BiosPartitionEntryTable;

	//
	// convert to zero-based partition number
	//
	UCHAR PartitionNumber								= static_cast<UCHAR>(Key - 1);
	Status												= HardDiskPartitionOpen(*FileId,DiskFileId,PartitionNumber);
	if(Status == ESUCCESS)
		return ESUCCESS;

#if _EFI_SUPPORT_
	return BlOpenGPTDiskPartition(*FileId,DiskFileId,PartitionNumber);
#else
	return Status;
#endif
}

//
// read partition
//
ARC_STATUS BiosPartitionRead(__in ULONG FileId,__out PVOID Buffer,__in ULONG Length,__out PULONG Count)
{
	LARGE_INTEGER PhysicalOffset;
	PhysicalOffset.QuadPart								= BlFileTable[FileId].Position.QuadPart + (BlFileTable[FileId].u.PartitionContext.StartingSector << SECTOR_SHIFT);
	ULONG DiskId										= BlFileTable[FileId].u.PartitionContext.DiskId;
	ARC_STATUS Status									= BlFileTable[DiskId].DeviceEntryTable->Seek(DiskId,&PhysicalOffset,SeekAbsolute);
	if(Status != ESUCCESS)
		return Status;

	Status												= BlFileTable[DiskId].DeviceEntryTable->Read(DiskId,Buffer,Length,Count);
	BlFileTable[FileId].Position.QuadPart				+= *Count;

	return Status;
}

//
// seek bios partition
//
ARC_STATUS BiosPartitionSeek(__in ULONG FileId,__in PLARGE_INTEGER Offset,__in SEEK_MODE SeekMode)
{
	switch(SeekMode)
	{
	case SeekAbsolute:
		BlFileTable[FileId].Position					= *Offset;
		break;

	case SeekRelative:
		BlFileTable[FileId].Position.QuadPart			+= Offset->QuadPart;
		break;

	default:
		BlPrint("SeekMode %lx not supported\r\n",SeekMode);
		return EACCES;
		break;
	}

	return ESUCCESS;
}

//
// write partition
//
ARC_STATUS BiosPartitionWrite(__in ULONG FileId,__out PVOID Buffer,__in ULONG Length,__out PULONG Count)
{
	LARGE_INTEGER PhysicalOffset;
	PhysicalOffset.QuadPart								= BlFileTable[FileId].Position.QuadPart + (BlFileTable[FileId].u.PartitionContext.StartingSector << SECTOR_SHIFT);
	ULONG DiskId										= BlFileTable[FileId].u.PartitionContext.DiskId;
	ARC_STATUS Status									= BlFileTable[DiskId].DeviceEntryTable->Seek(DiskId,&PhysicalOffset,SeekAbsolute);
	if(Status != ESUCCESS)
		return Status;

	Status												= BlFileTable[DiskId].DeviceEntryTable->Write(DiskId,Buffer,Length,Count);
	if(Status == ESUCCESS)
		BlFileTable[FileId].Position.QuadPart			+= *Count;

	return(Status);
}

//
// get partition info
//
ARC_STATUS BiosPartitionGetFileInfo(__in ULONG FileId,__out PFILE_INFORMATION Finfo)
{
	RtlZeroMemory(Finfo,sizeof(FILE_INFORMATION));

	PPARTITION_CONTEXT Context							= &BlFileTable[FileId].u.PartitionContext;

	Finfo->StartingAddress.QuadPart						= Context->StartingSector;
	Finfo->StartingAddress.QuadPart						= Finfo->StartingAddress.QuadPart << Context->SectorShift;
	Finfo->EndingAddress.QuadPart						= Finfo->StartingAddress.QuadPart + Context->PartitionLength.QuadPart;
	Finfo->Type											= DiskPeripheral;

	return ESUCCESS;
}


BL_DEVICE_ENTRY_TABLE BiosPartitionEntryTable =
{
	reinterpret_cast<PARC_CLOSE_ROUTINE>(&BiosPartitionClose),
	reinterpret_cast<PARC_MOUNT_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PARC_OPEN_ROUTINE>(&BiosPartitionOpen),
	reinterpret_cast<PARC_READ_ROUTINE>(&BiosPartitionRead),
	reinterpret_cast<PARC_READ_STATUS_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PARC_SEEK_ROUTINE>(&BiosPartitionSeek),
	reinterpret_cast<PARC_WRITE_ROUTINE>(&BiosPartitionWrite),
	reinterpret_cast<PARC_GET_FILE_INFO_ROUTINE>(&BiosPartitionGetFileInfo),
	reinterpret_cast<PARC_SET_FILE_INFO_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PRENAME_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PARC_GET_DIRECTORY_ENTRY_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PBOOTFS_INFO>(&BlArcNotYetImplemented),
};

BL_DEVICE_ENTRY_TABLE BiosDiskEntryTable =
{
	reinterpret_cast<PARC_CLOSE_ROUTINE>(&BiosDiskClose),
	reinterpret_cast<PARC_MOUNT_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PARC_OPEN_ROUTINE>(&BiosDiskOpen),
	reinterpret_cast<PARC_READ_ROUTINE>(&BiosDiskRead),
	reinterpret_cast<PARC_READ_STATUS_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PARC_SEEK_ROUTINE>(&BiosPartitionSeek),
	reinterpret_cast<PARC_WRITE_ROUTINE>(&BiosDiskWrite),
	reinterpret_cast<PARC_GET_FILE_INFO_ROUTINE>(&BiosDiskGetFileInfo),
	reinterpret_cast<PARC_SET_FILE_INFO_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PRENAME_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PARC_GET_DIRECTORY_ENTRY_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PBOOTFS_INFO>(&BlArcNotYetImplemented),
};

BL_DEVICE_ENTRY_TABLE BiosEDDSEntryTable =
{
	reinterpret_cast<PARC_CLOSE_ROUTINE>(&BiosDiskClose),
	reinterpret_cast<PARC_MOUNT_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PARC_OPEN_ROUTINE>(&BiosDiskOpen),
	reinterpret_cast<PARC_READ_ROUTINE>(&BiosElToritoDiskRead),
	reinterpret_cast<PARC_READ_STATUS_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PARC_SEEK_ROUTINE>(&BiosPartitionSeek),
	reinterpret_cast<PARC_WRITE_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PARC_GET_FILE_INFO_ROUTINE>(&BiosDiskGetFileInfo),
	reinterpret_cast<PARC_SET_FILE_INFO_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PRENAME_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PARC_GET_DIRECTORY_ENTRY_ROUTINE>(&BlArcNotYetImplemented),
	reinterpret_cast<PBOOTFS_INFO>(&BlArcNotYetImplemented),
};