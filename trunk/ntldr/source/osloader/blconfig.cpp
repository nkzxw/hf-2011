//********************************************************************
//	created:	14:8:2008   11:14
//	file:		blconfig.cpp
//	author:		tiamo
//	purpose:	config
//********************************************************************

#include "stdafx.h"

//
// name table
//
PCHAR MnemonicTable[] =
{
	"arc",
	"cpu",
	"fpu",
	"pic",
	"pdc",
	"sic",
	"sdc",
	"sc",
	"eisa",
	"tc",
	"scsi",
	"dti",
	"multi",
	"disk",
	"tape",
	"cdrom",
	"worm",
	"serial",
	"net",
	"video",
	"par",
	"point",
	"key",
	"audio",
	"other",
	"rdisk",
	"fdisk",
	"tape",
	"modem",
	"monitor",
	"print",
	"pointer",
	"keyboard",
	"term",
	"other",
};

//
// adapter types
//
PCHAR													BlAdapterTypes[]	= {"eisa","scsi","multi","net","ramdisk",0};

//
// controller types
//
PCHAR													BlControllerTypes[] = {"disk","cdrom",0};

//
// peripheral types
//
PCHAR													BlPeripheralTypes[] = {"rdisk","fdisk","cdrom",0};

//
// get path mnemonic key
//
BOOLEAN BlGetPathMnemonicKey(__in PCHAR OpenPath,__in PCHAR Mnemonic,__out PULONG Key)
{
	CHAR String[16];
	ULONG i;
	//
	// construct a string of the form ")mnemonic("
	//
	String[0]											= ')';
	for(i = 1; *Mnemonic; i ++)
		String[i]										= *Mnemonic ++;

	String[i ++]										= '(';
	String[i]											= 0;

	PCHAR Tmp											= strstr(OpenPath,&String[1]);
	if(!Tmp)
		return TRUE;

	if(Tmp != OpenPath)
	{
		Tmp												= strstr(OpenPath,String);
		if(!Tmp)
			return TRUE;
	}
	else
	{
		i												-= 1;
	}

	//
	// skip the mnemonic and convert the value in between parentheses to integer
	//
	CHAR Digits[4];
	Tmp													+= i;
	for(i = 0; i < 3; i ++)
	{
		if(*Tmp == ')')
		{
			Digits[i]									= 0;
			break;
		}

		Digits[i]										= *Tmp++;
	}

	Digits[i]											= 0;
	*Key												= static_cast<ULONG>(atol(Digits));

	return FALSE;
}

//
// initialize config
//
ARC_STATUS BlConfigurationInitialize(__in PCONFIGURATION_COMPONENT Parent,__in PCONFIGURATION_COMPONENT_DATA ParentEntry)
{
	//
	// traverse the child configuration tree and allocate, initialize, and construct the corresponding NT configuration tree.
	//
	PCONFIGURATION_COMPONENT Child = ArcGetChild(Parent);
	while(Child)
	{
		//
		// allocate an entry of the appropriate size to hold the child configuration information.
		//
		ULONG Length									= sizeof(CONFIGURATION_COMPONENT_DATA) + Child->IdentifierLength + Child->ConfigurationDataLength;
		PCONFIGURATION_COMPONENT_DATA ChildEntry		= static_cast<PCONFIGURATION_COMPONENT_DATA>(BlAllocateHeap(Length));
		if(!ChildEntry)
			return ENOMEM;

		//
		// initialize the tree pointers and copy the component data.
		//
		if(!ParentEntry)
			BlLoaderBlock->ConfigurationRoot			= ChildEntry;
		else
			ParentEntry->Child							= ChildEntry;

		ChildEntry->Parent								= ParentEntry;
		ChildEntry->Sibling								= 0;
		ChildEntry->Child								= 0;

		RtlMoveMemory(&ChildEntry->ComponentEntry,Child,sizeof(CONFIGURATION_COMPONENT));

		PVOID ConfigurationData							= Add2Ptr(ChildEntry,sizeof(CONFIGURATION_COMPONENT_DATA),PVOID);

		//
		// if configuration data is specified, then copy the configuration data.
		//
		if(Child->ConfigurationDataLength)
		{
			ChildEntry->ConfigurationData				= ConfigurationData;
			ARC_STATUS Status							= ArcGetConfigurationData(ConfigurationData,Child);

			if(Status != ESUCCESS)
				return Status;

			ConfigurationData							= Add2Ptr(ConfigurationData,Child->ConfigurationDataLength,PVOID);
		}
		else
		{
			ChildEntry->ConfigurationData				= 0;
		}

		//
		// if identifier data is specified, then copy the identifier data.
		//
		if(Child->IdentifierLength)
		{
			ChildEntry->ComponentEntry.Identifier		= static_cast<PCHAR>(ConfigurationData);
			RtlMoveMemory(ConfigurationData,Child->Identifier,Child->IdentifierLength);
		}
		else
		{
			ChildEntry->ComponentEntry.Identifier		= 0;
		}

		//
		// traverse the sibling configuration tree and allocate, initialize, and construct the corresponding NT configuration tree.
		//
		PCONFIGURATION_COMPONENT_DATA PreviousSibling	= ChildEntry;
		PCONFIGURATION_COMPONENT Sibling				= ArcGetPeer(Child);
		while(Sibling)
		{
			//
			// allocate an entry of the appropriate size to hold the sibling configuration information.
			//
			ULONG Length								= sizeof(CONFIGURATION_COMPONENT_DATA) + Sibling->IdentifierLength + Sibling->ConfigurationDataLength;
			PCONFIGURATION_COMPONENT_DATA SiblingEntry	= static_cast<PCONFIGURATION_COMPONENT_DATA>(BlAllocateHeap(Length));
			if(!SiblingEntry)
				return ENOMEM;

			//
			// initialize the tree pointers and copy the component data.
			//
			SiblingEntry->Parent						= ParentEntry;
			SiblingEntry->Sibling						= 0;
			ChildEntry->Child							= 0;
			RtlMoveMemory(&SiblingEntry->ComponentEntry,Sibling,sizeof(CONFIGURATION_COMPONENT));

			ConfigurationData							= Add2Ptr(SiblingEntry,sizeof(CONFIGURATION_COMPONENT_DATA),PVOID);

			//
			// if configuration data is specified, then copy the configuration data.
			//
			if(Sibling->ConfigurationDataLength)
			{
				SiblingEntry->ConfigurationData			= ConfigurationData;
				ARC_STATUS Status						= ArcGetConfigurationData(ConfigurationData,Sibling);
				if(Status != ESUCCESS)
					return Status;

				ConfigurationData						= Add2Ptr(ConfigurationData,Sibling->ConfigurationDataLength,PVOID);
			}
			else
			{
				SiblingEntry->ConfigurationData			= 0;
			}

			//
			// if identifier data is specified, then copy the identifier data.
			//
			if(Sibling->IdentifierLength)
			{
				SiblingEntry->ComponentEntry.Identifier = static_cast<PCHAR>(ConfigurationData);
				RtlMoveMemory(ConfigurationData,Sibling->Identifier,Sibling->IdentifierLength);
			}
			else
			{
				SiblingEntry->ComponentEntry.Identifier = 0;
			}

			//
			// if the sibling has a child, then generate the tree for the child.
			//
			if(ArcGetChild(Sibling))
			{
				ARC_STATUS Status						= BlConfigurationInitialize(Sibling,SiblingEntry);
				if(Status != ESUCCESS)
					return Status;
			}

			//
			// set new sibling pointers and get the next sibling tree entry.
			//
			PreviousSibling->Sibling					= SiblingEntry;
			PreviousSibling								= SiblingEntry;
			Sibling										= ArcGetPeer(Sibling);
		}

		//
		// set new parent pointers and get the next child tree entry.
		//
		Parent											= Child;
		ParentEntry										= ChildEntry;
		Child											= ArcGetChild(Child);
	}

	return ESUCCESS;
}

//
// get next entry
//
PCONFIGURATION_COMPONENT_DATA KeFindConfigurationNextEntry(__in PCONFIGURATION_COMPONENT_DATA Child,__in CONFIGURATION_CLASS Class,
														   __in CONFIGURATION_TYPE Type,__in_opt PULONG Key,__in PCONFIGURATION_COMPONENT_DATA *Resume)
{
	//
	// initialize the match key and mask based on whether the optional key value is specified.
	//
	ULONG MatchMask										= 0;
	ULONG MatchKey										= 0;
	if(Key)
	{
		MatchMask										= 0xffffffff;
		MatchKey										= *Key;

	}

	//
	// search specified configuration tree for an entry that matches the the specified class, type, and key.
	//
	while(Child)
	{
		if(*Resume)
		{
			//
			// if resume location found, clear resume location and continue search with next entry
			//
			if(Child == *Resume)
				*Resume									= 0;
		}
		else
		{
			//
			// if the class, type, and key match, then return a pointer to the child entry.
			//
			if(Child->ComponentEntry.Class == Class && Child->ComponentEntry.Type == Type && (Child->ComponentEntry.Key & MatchMask) == MatchKey)
				return Child;
		}

		//
		// if the child has a sibling list, then search the sibling list for an entry that matches the specified class, type, and key.
		//
		PCONFIGURATION_COMPONENT_DATA Sibling			= Child->Sibling;
		while(Sibling)
		{
			if(*Resume)
			{
				//
				// if resume location found, clear resume location and continue search with next entry
				//
				if(Sibling == *Resume)
					*Resume								= 0;
			}
			else
			{
				//
				// if the class, type, and key match, then return a pointer to the child entry.
				//
				if(Sibling->ComponentEntry.Class == Class && Sibling->ComponentEntry.Type == Type && (Sibling->ComponentEntry.Key & MatchMask) == MatchKey)
					return Sibling;
			}

			//
			// if the sibling has a child tree, then search the child tree for an entry that matches the specified class, type, and key.
			//
			if(Sibling->Child)
			{
				PCONFIGURATION_COMPONENT_DATA Entry		= KeFindConfigurationNextEntry(Sibling->Child,Class,Type,Key,Resume);
				if(Entry)
					return Entry;
			}

			Sibling										= Sibling->Sibling;
		}

		Child											= Child->Child;
	}

	return 0;
}

//
// find config entry
//
PCONFIGURATION_COMPONENT_DATA KeFindConfigurationEntry(__in PCONFIGURATION_COMPONENT_DATA Child,__in CONFIGURATION_CLASS Class,
													   __in CONFIGURATION_TYPE Type,__in_opt PULONG Key)
{
	PCONFIGURATION_COMPONENT_DATA Resume				= 0;
	return KeFindConfigurationNextEntry (Child,Class,Type,Key,&Resume);
}

//
// builds an ARC pathname
//
VOID BlGetPathnameFromComponent(__in PCONFIGURATION_COMPONENT_DATA Component,__out PCHAR ArcName)
{
	if(Component->Parent)
	{
		//
		// ask parent first
		//
		BlGetPathnameFromComponent(Component->Parent,ArcName);

		//
		// append our segment to the arcname
		//
		sprintf(ArcName + strlen(ArcName),"%s(%d)",MnemonicTable[Component->ComponentEntry.Type],Component->ComponentEntry.Key);
	}
	else
	{
		//
		// we are the parent, initialize the string and return
		//
		ArcName[0]										= '\0';
	}
}

//
// depth-first search of the firmware configuration tree
//
BOOLEAN BlSearchConfigTree(__in PCONFIGURATION_COMPONENT_DATA Node,__in CONFIGURATION_CLASS Class,__in CONFIGURATION_TYPE Type,__in ULONG Key,__in PNODE_CALLBACK Callback)
{
	do
	{
		PCONFIGURATION_COMPONENT_DATA Child				= Node->Child;
		if(Child)
		{
			if(!BlSearchConfigTree(Child,Class,Type,Key,Callback))
				return FALSE;
		}

		if( (Class == 0xffffffff	|| Node->ComponentEntry.Class == Class) &&
			(Type == 0xffffffff		|| Node->ComponentEntry.Type == Type) &&
			(Key == 0xffffffff		|| Node->ComponentEntry.Key == Key))
		{
			if(!Callback(Node))
				return FALSE;
		}

		Node											= Node->Sibling;
	}while(Node);

	return TRUE;
}

//
// get next token
//
PCHAR BlGetNextToken (__in PCHAR TokenString,__out PCHAR OutputToken,__out PULONG UnitNumber)
{
	//
	// if there are more characters in the token string, then parse the next token.otherwise, return a value of NULL.
	//
	if(*TokenString == 0)
		return 0;

	while(*TokenString != 0 && *TokenString != '(')
		*OutputToken ++									= *TokenString ++;

	*OutputToken										= 0;

	//
	// if a unit number is specified, then convert it to binary.otherwise, default the unit number to zero.
	//
	*UnitNumber											= 0;
	if(*TokenString == '(')
	{
		TokenString										+= 1;
		while(*TokenString != '\0' && *TokenString != ')')
			*UnitNumber									= *UnitNumber * 10 + *TokenString ++ - '0';

		if(*TokenString == ')')
			TokenString									+= 1;
	}

	return TokenString;
}

//
// match token
//
ULONG BlMatchToken(__in PCHAR TokenValue,__in PCHAR TokenArray[])
{
	//
	// scan the match array until either a match is found or all of the match strings have been scanned.
	//
	ULONG Index = 0;
	while(TokenArray[Index])
	{
		PCHAR MatchString								= TokenArray[Index];
		PCHAR TokenString								= TokenValue;

		while(*MatchString && *TokenString)
		{
			if(toupper(*MatchString) != toupper(*TokenString))
				break;

			MatchString									+= 1;
			TokenString									+= 1;
		}

		if(*MatchString == 0 && *TokenString == 0)
			break;

		Index											+= 1;
	}

	return Index;
}

//
// generates an NT device name prefix and a canonical ARC device name from an ARC device name.
//
ARC_STATUS BlGenerateDeviceNames(__in PCHAR ArcDeviceName,__out PCHAR ArcCanonicalName,__out_opt PCHAR NtDevicePrefix)
{
	CHAR AdapterName[32];
	ULONG AdapterNumber;
	CHAR ControllerName[32];
	ULONG ControllerNumber;
	CHAR PartitionName[32];
	ULONG PartitionNumber;
	CHAR PeripheralName[32];
	ULONG PeripheralNumber;
	CHAR TokenValue[32];

	//
	// get the adapter and make sure it is valid.
	//
	ArcDeviceName										= BlGetNextToken(ArcDeviceName,AdapterName,&AdapterNumber);
	if(!ArcDeviceName)
		return EINVAL;

	ULONG MatchIndex									= BlMatchToken(AdapterName,BlAdapterTypes);
	if(MatchIndex == ARRAYSIZE(BlAdapterTypes) - 1)
		return EINVAL;

	CHAR AdapterPath[64];
	sprintf(AdapterPath,"%s(%d)",AdapterName,AdapterNumber);

	//
	// special for net and ramdisk
	//
	if(MatchIndex == 3 || MatchIndex == 4)
	{
		strcpy(ArcCanonicalName,AdapterPath);

		if(NtDevicePrefix)
			*NtDevicePrefix								= 0;

		return ESUCCESS;
	}

	//
	// the next token is either another adapter or a controller.ARC names can have multiple adapters.(e.g. "multi(0)scsi(0)disk(0)...")
	// iterate until we find a token that is not an adapter.
	//
	while(MatchIndex < 4)
	{
		ArcDeviceName									= BlGetNextToken(ArcDeviceName,ControllerName,&ControllerNumber);
		if(!ArcDeviceName)
			return EINVAL;

		MatchIndex										= BlMatchToken(ControllerName,BlAdapterTypes);
		if(MatchIndex == ARRAYSIZE(BlAdapterTypes) - 1)
		{
			//
			// if it is not an adapter, we must have reached the last adapter in the name.  Fall through to the controller logic.
			//
			break;
		}
		else
		{
			//
			// we have found another adapter, add it to the canonical adapter path
			//
			sprintf(AdapterPath + strlen(AdapterPath),"%s(%d)",ControllerName,ControllerNumber);
		}

	}

	MatchIndex											= BlMatchToken(ControllerName,BlControllerTypes);
	switch(MatchIndex)
	{
		//
		// cdrom controller.get the peripheral name and make sure it is valid.
		//
	case 1:
		ArcDeviceName									= BlGetNextToken(ArcDeviceName,PeripheralName,&PeripheralNumber);
		if(!ArcDeviceName)
			return EINVAL;

		if(_stricmp(PeripheralName,"fdisk"))
			return EINVAL;

		ArcDeviceName									= BlGetNextToken(ArcDeviceName,TokenValue,&MatchIndex);
		if(ArcDeviceName)
			return EINVAL;

		sprintf(ArcCanonicalName,"%s%s(%d)%s(%d)",AdapterPath,ControllerName,ControllerNumber,PeripheralName,PeripheralNumber);

		if(NtDevicePrefix)
			strcpy(NtDevicePrefix, "\\Device\\CDRom");
		break;

		//
		// disk controller.
		//
	case 0:
		ArcDeviceName									= BlGetNextToken(ArcDeviceName,PeripheralName,&PeripheralNumber);
		if(!ArcDeviceName)
			return EINVAL;

		MatchIndex										= BlMatchToken(PeripheralName,BlPeripheralTypes);
		switch (MatchIndex)
		{
			//
			// Rigid Disk.
			//
		case 0:
			ArcDeviceName								= BlGetNextToken(ArcDeviceName,PartitionName,&PartitionNumber);
			if(!ArcDeviceName)
			{
				strcpy(PartitionName, "partition");
				PartitionNumber							= 1;
			}
			else
			{
				if(_stricmp(PartitionName, "partition"))
					return EINVAL;

				ArcDeviceName							= BlGetNextToken(ArcDeviceName,TokenValue,&MatchIndex);
				if(ArcDeviceName)
					return EINVAL;
			}

			sprintf(ArcCanonicalName,"%s%s(%d)%s(%d)%s(%d)",AdapterPath,ControllerName,ControllerNumber,PeripheralName,PeripheralNumber,PartitionName,PartitionNumber);

			if(NtDevicePrefix)
				strcpy(NtDevicePrefix, "\\Device\\Harddisk");

			break;

			//
			// floppy disk.
			//
		case 1:
			ArcDeviceName								= BlGetNextToken(ArcDeviceName,TokenValue,&MatchIndex);
			if(ArcDeviceName)
				return EINVAL;

			sprintf(ArcCanonicalName,"%s%s(%d)%s(%d)",AdapterPath,ControllerName,ControllerNumber,PeripheralName,PeripheralNumber);

			if(NtDevicePrefix)
				strcpy(NtDevicePrefix, "\\Device\\Floppy");

			break;

			//
			// El Torito cd-rom.
			//
		case 2:
			ArcDeviceName								= BlGetNextToken(ArcDeviceName,TokenValue,&MatchIndex);
			if(ArcDeviceName)
				return EINVAL;

			sprintf(ArcCanonicalName,"%s%s(%d)%s(%d)",AdapterPath,ControllerName,ControllerNumber,PeripheralName,PeripheralNumber);

			if(NtDevicePrefix)
				strcpy(NtDevicePrefix, "\\Device\\CDRom");

			break;

			//
			// invalid peripheral.
			//
		default:
			return EINVAL;
		}

		break;

		//
		// invalid controller.
		//
	default:
		return EINVAL;
	}

	return ESUCCESS;
}
