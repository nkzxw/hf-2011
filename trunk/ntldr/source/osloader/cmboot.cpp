//********************************************************************
//	created:	24:8:2008   20:33
//	file:		cmboot.cpp
//	author:		tiamo
//	purpose:	registry support
//********************************************************************

#include "stdafx.h"

//
// boot hive
//
CMHIVE													BootHive;

//
// find nls data
//
BOOLEAN CmpFindNLSData(__in PHHIVE Hive,__in HCELL_INDEX ControlSetKeyCell,__out PUNICODE_STRING AnsiCodePage,
					   __out PUNICODE_STRING OemCodePage,__out PUNICODE_STRING LanguageTable,__out PUNICODE_STRING OemHalFont)
{
	//
	// find control key
	//
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name,L"Control");
	HCELL_INDEX ControlKeyCell							= CmpFindSubKeyByName(Hive,HvGetCell(Hive,ControlSetKeyCell),&Name);
	if(ControlKeyCell == HCELL_NIL)
		return FALSE;

	//
	// find nls key
	//
	RtlInitUnicodeString(&Name,L"NLS");
	HCELL_INDEX NlsKeyCell								= CmpFindSubKeyByName(Hive,HvGetCell(Hive,ControlKeyCell),&Name);
	if(NlsKeyCell == HCELL_NIL)
		return FALSE;

	//
	// find code page key
	//
	RtlInitUnicodeString(&Name,L"CodePage");
	HCELL_INDEX CodePageKeyCell							= CmpFindSubKeyByName(Hive,HvGetCell(Hive,NlsKeyCell),&Name);
	if(CodePageKeyCell == HCELL_NIL)
		return FALSE;

	//
	// find ACP value
	//
	RtlInitUnicodeString(&Name,L"ACP");
	HCELL_INDEX AcpValueCell							= CmpFindValueByName(Hive,HvGetCell(Hive,CodePageKeyCell),&Name);
	if(AcpValueCell == HCELL_NIL)
		return FALSE;

	//
	// build acp file name
	//
	PCM_KEY_VALUE Value									= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,AcpValueCell));
	if(!Value || Value->Type != REG_SZ)
		return FALSE;

	ULONG ValueSize										= 0;
	Name.Buffer											= static_cast<PWCHAR>(CmpValueToData(Hive,Value,&ValueSize));
	Name.MaximumLength									= static_cast<USHORT>(ValueSize);
	for(Name.Length = 0; Name.Length < Name.MaximumLength && Name.Buffer[Name.Length / sizeof(WCHAR)]; Name.Length += sizeof(WCHAR));

	//
	// find acp file name
	//
	HCELL_INDEX AcpFileNameValueCell					= CmpFindValueByName(Hive,HvGetCell(Hive,CodePageKeyCell),&Name);
	if(AcpFileNameValueCell == HCELL_NIL)
		return FALSE;

	//
	// read acp file name
	//
	Value												= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,AcpFileNameValueCell));
	if(!Value || Value->Type != REG_SZ)
		return FALSE;

	AnsiCodePage->Buffer								= static_cast<PWCHAR>(CmpValueToData(Hive,Value,&ValueSize));
	AnsiCodePage->Length								= static_cast<USHORT>(ValueSize);
	AnsiCodePage->MaximumLength							= static_cast<USHORT>(ValueSize);

	//
	// find oemcp value
	//
	RtlInitUnicodeString(&Name,L"OEMCP");
	HCELL_INDEX OemcpValueCell							= CmpFindValueByName(Hive,HvGetCell(Hive,CodePageKeyCell),&Name);
	if(OemcpValueCell == HCELL_NIL)
		return FALSE;

	//
	// build file name
	//
	Value												= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,OemcpValueCell));
	if(!Value || Value->Type != REG_SZ)
		return FALSE;

	Name.Buffer											= static_cast<PWCHAR>(CmpValueToData(Hive,Value,&ValueSize));
	Name.MaximumLength									= static_cast<USHORT>(ValueSize);
	for(Name.Length = 0; Name.Length < Name.MaximumLength && Name.Buffer[Name.Length / sizeof(WCHAR)]; Name.Length += sizeof(WCHAR));

	//
	// find oem file name
	//
	HCELL_INDEX OemcpFileNameValueCell					= CmpFindValueByName(Hive,HvGetCell(Hive,CodePageKeyCell),&Name);
	if(OemcpFileNameValueCell == HCELL_NIL)
		return FALSE;

	//
	// read oemcp file name
	//
	Value												= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,OemcpFileNameValueCell));
	if(!Value || Value->Type != REG_SZ)
		return FALSE;

	OemCodePage->Buffer									= static_cast<PWCHAR>(CmpValueToData(Hive,Value,&ValueSize));
	OemCodePage->Length									= static_cast<USHORT>(ValueSize);
	OemCodePage->MaximumLength							= static_cast<USHORT>(ValueSize);

	//
	// find language key
	//
	RtlInitUnicodeString(&Name,L"Language");
	HCELL_INDEX LanguageKeyCell							= CmpFindSubKeyByName(Hive,HvGetCell(Hive,NlsKeyCell),&Name);
	if(LanguageKeyCell == HCELL_NIL)
		return FALSE;

	//
	// find default value
	//
	RtlInitUnicodeString(&Name,L"Default");
	HCELL_INDEX DefaultValueCell						= CmpFindValueByName(Hive,HvGetCell(Hive,LanguageKeyCell),&Name);
	if(DefaultValueCell == HCELL_NIL)
		return FALSE;

	//
	// build file name
	//
	Value												= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,DefaultValueCell));
	if(!Value || Value->Type != REG_SZ)
		return FALSE;

	Name.Buffer											= static_cast<PWCHAR>(CmpValueToData(Hive,Value,&ValueSize));
	Name.MaximumLength									= static_cast<USHORT>(ValueSize);
	for(Name.Length = 0; Name.Length < Name.MaximumLength && Name.Buffer[Name.Length / sizeof(WCHAR)]; Name.Length += sizeof(WCHAR));

	//
	// find default file name
	//
	HCELL_INDEX DefaultFileNameValueCell				= CmpFindValueByName(Hive,HvGetCell(Hive,LanguageKeyCell),&Name);
	if(DefaultFileNameValueCell == HCELL_NIL)
		return FALSE;

	//
	// read language table name
	//
	Value												= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,DefaultFileNameValueCell));
	if(!Value || Value->Type != REG_SZ)
		return FALSE;

	LanguageTable->Buffer								= static_cast<PWCHAR>(CmpValueToData(Hive,Value,&ValueSize));
	LanguageTable->Length								= static_cast<USHORT>(ValueSize);
	LanguageTable->MaximumLength						= static_cast<USHORT>(ValueSize);

	//
	// find oemhal value
	//
	OemHalFont->Buffer									= 0;
	OemHalFont->Length									= 0;
	OemHalFont->MaximumLength							= 0;
	RtlInitUnicodeString(&Name,L"OEMHAL");
	HCELL_INDEX OemHalValueCell							= CmpFindValueByName(Hive,HvGetCell(Hive,CodePageKeyCell),&Name);
	if(OemHalValueCell != HCELL_NIL)
	{
		Value											= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,OemHalValueCell));
		if(Value && Value->Type == REG_SZ)
		{
			OemHalFont->Buffer							= static_cast<PWCHAR>(CmpValueToData(Hive,Value,&ValueSize));
			OemHalFont->Length							= static_cast<USHORT>(ValueSize);
			OemHalFont->MaximumLength					= static_cast<USHORT>(ValueSize);
		}
	}

	return TRUE;
}

//
// check load type
//
BOOLEAN CmpIsLoadType(__in PHHIVE Hive,__in HCELL_INDEX DriverKeyCell,__in SERVICE_LOAD_TYPE LoadType)
{
	//
	// read start value
	//
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name,L"Start");
	HCELL_INDEX StartValueCell							= CmpFindValueByName(Hive,HvGetCell(Hive,DriverKeyCell),&Name);
	if(StartValueCell == HCELL_NIL)
		return FALSE;

	PCM_KEY_VALUE Value									= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,StartValueCell));
	if(!Value)
		return FALSE;

	PULONG Data											= static_cast<PULONG>(CmpValueToData(Hive,Value,0));
	if(!Data || *Data != LoadType)
		return FALSE;

	return TRUE;
}

//
// find tag index
//
ULONG CmpFindTagIndex(__in PHHIVE Hive,__in HCELL_INDEX TagValueCell,__in HCELL_INDEX GroupOrderListKeyCell,__in PUNICODE_STRING Group)
{
	PCM_KEY_VALUE Tag									= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,TagValueCell));
	if(!Tag)
		return 0xfffffffe;

	PULONG TagData										= static_cast<PULONG>(CmpValueToData(Hive,Tag,0));
	if(!TagData)
		return 0xfffffffe;

	HCELL_INDEX OrderValueCell							= CmpFindValueByName(Hive,HvGetCell(Hive,GroupOrderListKeyCell),Group);
	if(OrderValueCell == HCELL_NIL)
		return 0xfffffffe;

	PCM_KEY_VALUE Order									= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,OrderValueCell));
	if(!Order)
		return 0xfffffffe;

	PULONG OrderData									= static_cast<PULONG>(CmpValueToData(Hive,Order,0));
	if(!OrderData)
		return 0xfffffffe;

	for(ULONG i = 1; i < OrderData[0]; i ++)
	{
		if(OrderData[i] == *TagData)
			return i;
	}

	return 0xfffffffe;
}

//
// add driver to list
//
BOOLEAN CmpAddDriverToList(__in PHHIVE Hive,__in HCELL_INDEX DriverKeyCell,__in HCELL_INDEX GroupOrderListKeyCell,__in PUNICODE_STRING BasePath,__in PLIST_ENTRY DriverListHead)
{
	//
	// get driver
	//
	PCM_KEY_NODE Driver									= static_cast<PCM_KEY_NODE>(HvGetCell(Hive,DriverKeyCell));
	if(!Driver)
		return TRUE;

	//
	// allocate driver node
	//
	PBOOT_DRIVER_NODE Node								= static_cast<PBOOT_DRIVER_NODE>(Hive->Allocate(sizeof(BOOT_DRIVER_NODE),FALSE,' 1MC'));
	if(!Node)
		return FALSE;

	RtlZeroMemory(Node,sizeof(BOOT_DRIVER_NODE));
	ULONG Length										= 0;

	//
	// copy name
	//
	if(Driver->Flags & KEY_COMP_NAME)
	{
		Length											= CmpCompressedNameSize(Driver->Name,Driver->NameLength);
		Node->Name.Buffer								= static_cast<PWCHAR>(Hive->Allocate(Length,FALSE,' 2MC'));
		if(!Node->Name.Buffer)
			return FALSE;

		Node->Name.Length								= static_cast<USHORT>(Length);
		CmpCopyCompressedName(Node->Name.Buffer,Length,Driver->Name,Driver->NameLength);
	}
	else
	{
		Node->Name.Buffer								= static_cast<PWCHAR>(Hive->Allocate(Driver->NameLength,FALSE,' 2MC'));
		if(!Node->Name.Buffer)
			return FALSE;

		Node->Name.Length								= Driver->NameLength;
		RtlCopyMemory(Node->Name.Buffer,Driver->Name,Driver->NameLength);
	}

	Node->Name.MaximumLength							= Node->Name.Length;

	//
	// find image path value
	//
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name,L"ImagePath");
	HCELL_INDEX ImagePathValueCell						= CmpFindValueByName(Hive,HvGetCell(Hive,DriverKeyCell),&Name);
	if(ImagePathValueCell == HCELL_NIL)
	{
		//
		// image path not found,make a default file path
		//
		Length											= sizeof(L"System32\\Drivers\\") + Node->Name.Length + sizeof(L".sys");
		Node->ListEntry.FilePath.Buffer					= static_cast<PWCHAR>(Hive->Allocate(Length,FALSE,' 3MC'));
		if(!Node->ListEntry.FilePath.Buffer)
			return FALSE;

		Node->ListEntry.FilePath.Length					= 0;
		Node->ListEntry.FilePath.MaximumLength			= static_cast<USHORT>(Length);

		if(!NT_SUCCESS(RtlAppendUnicodeToString(&Node->ListEntry.FilePath,L"System32\\Drivers\\")))
			return FALSE;

		if(!NT_SUCCESS(RtlAppendUnicodeStringToString(&Node->ListEntry.FilePath,&Node->Name)))
			return FALSE;

		if(!NT_SUCCESS(RtlAppendUnicodeToString(&Node->ListEntry.FilePath,L".sys")))
			return FALSE;
	}
	else
	{
		//
		// copy image path as file path
		//
		PCM_KEY_VALUE Value								= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,ImagePathValueCell));
		PWCHAR ImagePath								= static_cast<PWCHAR>(CmpValueToData(Hive,Value,&Length));
		Node->ListEntry.FilePath.Buffer					= static_cast<PWCHAR>(Hive->Allocate(Length,FALSE,' 3MC'));
		if(!Node->ListEntry.FilePath.Buffer || !ImagePath)
			return FALSE;

		RtlCopyMemory(Node->ListEntry.FilePath.Buffer,ImagePath,Length);
		Node->ListEntry.FilePath.Length					= static_cast<USHORT>(Length);
		Node->ListEntry.FilePath.MaximumLength			= static_cast<USHORT>(Length);
	}

	//
	// set reg path
	//
	Length												= BasePath->Length + Node->Name.Length;
	Node->ListEntry.RegistryPath.Buffer					= static_cast<PWCHAR>(Hive->Allocate(Length,FALSE,' 4MC'));
	if(!Node->ListEntry.RegistryPath.Buffer)
		return FALSE;

	Node->ListEntry.RegistryPath.Length					= 0;
	Node->ListEntry.RegistryPath.MaximumLength			= static_cast<USHORT>(Length);
	RtlAppendUnicodeStringToString(&Node->ListEntry.RegistryPath,BasePath);
	RtlAppendUnicodeStringToString(&Node->ListEntry.RegistryPath,&Node->Name);

	//
	// insert into list
	//
	InsertHeadList(DriverListHead,&Node->ListEntry.Link);

	//
	// find ErrorControl value
	//
	RtlInitUnicodeString(&Name,L"ErrorControl");
	HCELL_INDEX ErrorControlValueCell					= CmpFindValueByName(Hive,HvGetCell(Hive,DriverKeyCell),&Name);
	Node->ErrorControl									= SERVICE_ERROR_NORMAL;
	if(ErrorControlValueCell != HCELL_NIL)
	{
		PCM_KEY_VALUE Value								= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,ErrorControlValueCell));
		if(Value)
			Node->ErrorControl							= *static_cast<PULONG>(CmpValueToData(Hive,Value,&Length));
	}

	//
	// find Group value
	//
	RtlInitUnicodeString(&Name,L"Group");
	HCELL_INDEX GroupValueCell							= CmpFindValueByName(Hive,HvGetCell(Hive,DriverKeyCell),&Name);
	if(GroupValueCell != HCELL_NIL)
	{
		PCM_KEY_VALUE Value								= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,GroupValueCell));
		if(Value)
		{
			PWCHAR Group								= static_cast<PWCHAR>(CmpValueToData(Hive,Value,&Length));
			if(Group && Length)
			{
				Node->Group.Buffer						= static_cast<PWCHAR>(Hive->Allocate(Length,FALSE,' 5MC'));
				if(Node->Group.Buffer)
				{
					RtlCopyMemory(Node->Group.Buffer,Group,Length);
					Node->Group.Length					= static_cast<USHORT>(Length - sizeof(WCHAR));
					Node->Group.MaximumLength			= Node->Group.Length;
				}
			}
		}
	}

	//
	// find tag value
	//
	RtlInitUnicodeString(&Name,L"Tag");
	HCELL_INDEX TagValueCell							= CmpFindValueByName(Hive,HvGetCell(Hive,DriverKeyCell),&Name);
	Node->Tag											= 0xffffffff;

	//
	// search group list for tag index
	//
	if(TagValueCell != HCELL_NIL)
		Node->Tag										= CmpFindTagIndex(Hive,TagValueCell,GroupOrderListKeyCell,&Node->Group);

	return TRUE;
}

//
// find boot drivers
//
BOOLEAN CmpFindDrivers(__in PHHIVE Hive,__in HCELL_INDEX ControlSetKeyCell,__in SERVICE_LOAD_TYPE LoadType,__in_opt PWSTR BootFileSystem,__in PLIST_ENTRY DriverListHead)
{
	//
	// find services key under current_control_set
	//
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name,L"Services");
	HCELL_INDEX ServicesKeyCell							= CmpFindSubKeyByName(Hive,HvGetCell(Hive,ControlSetKeyCell),&Name);
	if(ServicesKeyCell == HCELL_NIL)
		return FALSE;

	//
	// find control key under current_control_set
	//
	RtlInitUnicodeString(&Name,L"Control");
	HCELL_INDEX ControlKeyCell							= CmpFindSubKeyByName(Hive,HvGetCell(Hive,ControlSetKeyCell),&Name);
	if(ControlSetKeyCell == HCELL_NIL)
		return FALSE;

	//
	// find group order list under current_control_set\control
	//
	RtlInitUnicodeString(&Name,L"GroupOrderList");
	HCELL_INDEX GroupOrderListKeyCell					= CmpFindSubKeyByName(Hive,HvGetCell(Hive,ControlKeyCell),&Name);
	if(GroupOrderListKeyCell == HCELL_NIL)
		return FALSE;

	//
	// set base path
	//
	WCHAR BasePathBuffer[128];
	UNICODE_STRING BasePath;
	BasePath.Buffer										= BasePathBuffer;
	BasePath.Length										= 0;
	BasePath.MaximumLength								= sizeof(BasePathBuffer);
	RtlAppendUnicodeToString(&BasePath,L"\\Registry\\Machine\\System\\");
	RtlAppendUnicodeToString(&BasePath,L"CurrentControlSet\\Services\\");

	for(ULONG i = 0; TRUE; i ++)
	{
		//
		// for each key in services
		//
		HCELL_INDEX DriverKeyCell						= CmpFindSubKeyByNumber(Hive,HvGetCell(Hive,ServicesKeyCell),i);
		if(DriverKeyCell == HCELL_NIL)
			break;

		//
		// add this driver to list if it should be loaded by osloader
		//
		if(CmpIsLoadType(Hive,DriverKeyCell,LoadType))
			CmpAddDriverToList(Hive,DriverKeyCell,GroupOrderListKeyCell,&BasePath,DriverListHead);
	}

	//
	// add boot file system driver
	//
	if(BootFileSystem)
	{
		//
		// find boot file system key
		//
		RtlInitUnicodeString(&Name,BootFileSystem);
		HCELL_INDEX BootFileSystemKeyCell				= CmpFindSubKeyByName(Hive,HvGetCell(Hive,ServicesKeyCell),&Name);
		if(BootFileSystemKeyCell != HCELL_NIL)
		{
			if(CmpAddDriverToList(Hive,BootFileSystemKeyCell,GroupOrderListKeyCell,&BasePath,DriverListHead))
			{
				//
				// mark the Boot Filesystem critical
				//
				PBOOT_DRIVER_NODE Node					= CONTAINING_RECORD(DriverListHead->Flink,BOOT_DRIVER_NODE,ListEntry.Link);
				Node->ErrorControl						= SERVICE_ERROR_CRITICAL;
			}
		}
	}

	return TRUE;
}

//
// sort driver list
//
BOOLEAN CmpDoSort(__in PLIST_ENTRY DriverListHead,__in PUNICODE_STRING OrderList)
{
	PWCHAR Current										= Add2Ptr(OrderList->Buffer,OrderList->Length,PWCHAR);
	while(Current > OrderList->Buffer)
	{
		PWCHAR End										= Current;

		do
		{
			if(*Current == 0)
				End										= Current;

			Current										-= 1;
		}while(Current[-1] && Current != OrderList->Buffer);

		//
		// current now points to the beginning of the NULL-terminated string.
		// end now points to the end of the string
		//
		UNICODE_STRING CurrentGroup;
		CurrentGroup.Length								= static_cast<USHORT>(End - Current) * sizeof(WCHAR);
		CurrentGroup.MaximumLength						= CurrentGroup.Length;
		CurrentGroup.Buffer								= Current;
		PLIST_ENTRY Next								= DriverListHead->Flink;
		while(Next != DriverListHead)
		{
			PBOOT_DRIVER_NODE CurrentNode				= CONTAINING_RECORD(Next,BOOT_DRIVER_NODE,ListEntry.Link);
			Next										= CurrentNode->ListEntry.Link.Flink;
			if(CurrentNode->Group.Buffer)
			{
				if(RtlEqualUnicodeString(&CurrentGroup,&CurrentNode->Group,TRUE))
				{
					RemoveEntryList(&CurrentNode->ListEntry.Link);
					InsertHeadList(DriverListHead,&CurrentNode->ListEntry.Link);
				}
			}
		}

		Current											-= 1;
	}

	return TRUE;
}

//
// sort driver list
//
BOOLEAN CmpSortDriverList(__in PHHIVE Hive,__in HCELL_INDEX ControlSetKeyCell,__in PLIST_ENTRY DriverListHead)
{
	//
	// find control key
	//
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name,L"Control");
	HCELL_INDEX ControlKeyCell							= CmpFindSubKeyByName(Hive,HvGetCell(Hive,ControlSetKeyCell),&Name);
	if(ControlSetKeyCell == HCELL_NIL)
		return FALSE;

	//
	// find ServiceGroupOrder key
	//
	RtlInitUnicodeString(&Name,L"ServiceGroupOrder");
	HCELL_INDEX ServiceGroupOrderKeyCell				= CmpFindSubKeyByName(Hive,HvGetCell(Hive,ControlKeyCell),&Name);
	if(ServiceGroupOrderKeyCell == HCELL_NIL)
		return FALSE;

	//
	// find list value
	//
	RtlInitUnicodeString(&Name,L"List");
	HCELL_INDEX ListValueCell							= CmpFindValueByName(Hive,HvGetCell(Hive,ServiceGroupOrderKeyCell),&Name);
	if(ListValueCell == HCELL_NIL)
		return FALSE;

	//
	// get list
	//
	PCM_KEY_VALUE List									= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,ListValueCell));
	if(!List || List->Type != REG_MULTI_SZ)
		return FALSE;

	//
	// build list buffer
	//
	UNICODE_STRING ListStrings;
	ULONG Length;
	ListStrings.Buffer									= static_cast<PWCHAR>(CmpValueToData(Hive,List,&Length));
	ListStrings.Length									= static_cast<USHORT>(Length - sizeof(WCHAR));
	ListStrings.MaximumLength							= ListStrings.Length;

	//
	// sort it according to order list
	//
	return CmpDoSort(DriverListHead,&ListStrings);
}

//
// reorders the nodes in a driver group based on their tag values.
//
BOOLEAN CmpOrderGroup(__in PBOOT_DRIVER_NODE GroupStart,__in PBOOT_DRIVER_NODE GroupEnd)
{
	if(GroupStart == GroupEnd)
		return TRUE;

	PBOOT_DRIVER_NODE Current							= GroupStart;

	do
	{
		//
		// if the driver before the current one has a lower tag,then we do not need to move it.
		// if not, then remove the driver from the list and scan backwards until we find a driver with a tag that is <= the current tag,
		// or we reach the beginning of the list.
		//
		PBOOT_DRIVER_NODE Previous						= Current;
		PLIST_ENTRY ListEntry							= Current->ListEntry.Link.Flink;
		Current											= CONTAINING_RECORD(ListEntry,BOOT_DRIVER_NODE,ListEntry.Link);

		if(Previous->Tag > Current->Tag)
		{
			//
			// remove the Current driver from the list, and search backwards until we find a tag that is <= the current driver's tag.
			// reinsert the current driver there.
			//
			if(Current == GroupEnd)
			{
				ListEntry								= Current->ListEntry.Link.Blink;
				GroupEnd								= CONTAINING_RECORD(ListEntry,BOOT_DRIVER_NODE,ListEntry.Link);
			}

			RemoveEntryList(&Current->ListEntry.Link);

			while(Previous->Tag > Current->Tag && Previous != GroupStart)
			{
				ListEntry								= Previous->ListEntry.Link.Blink;
				Previous								= CONTAINING_RECORD(ListEntry,BOOT_DRIVER_NODE,ListEntry.Link);
			}

			InsertTailList(&Previous->ListEntry.Link,&Current->ListEntry.Link);

			if(Previous == GroupStart)
				GroupStart								= Current;
		}
	}while(Current != GroupEnd);

	return TRUE;
}

//
// orders driver nodes in a group based on their dependencies on one another
//
BOOLEAN CmpResolveDriverDependencies(__in PLIST_ENTRY DriverListHead)
{
	PLIST_ENTRY CurrentEntry							= DriverListHead->Flink;

	while(CurrentEntry != DriverListHead)
	{
		//
		// the list is already ordered by groups.
		// find the first and last entry in each group, and order each of these sub-lists based on their dependencies.
		//
		PBOOT_DRIVER_NODE GroupStart					= CONTAINING_RECORD(CurrentEntry,BOOT_DRIVER_NODE,ListEntry.Link);
		PBOOT_DRIVER_NODE GroupEnd						= GroupStart;
		do
		{
			GroupEnd									= CONTAINING_RECORD(CurrentEntry,BOOT_DRIVER_NODE,ListEntry.Link);
			CurrentEntry								= CurrentEntry->Flink;
			PBOOT_DRIVER_NODE CurrentNode				= CONTAINING_RECORD(CurrentEntry,BOOT_DRIVER_NODE,ListEntry.Link);

			if(CurrentEntry == DriverListHead)
				break;

			if(!RtlEqualUnicodeString(&GroupStart->Group,&CurrentNode->Group,TRUE))
				break;

		}while(CurrentEntry != DriverListHead);

		//
		// GroupStart now points to the first driver node in the group,
		// GroupEnd points to the last driver node in the group.
		//
		CmpOrderGroup(GroupStart,GroupEnd);

	}

	return TRUE;
}

//
// read bios date from registry
//
BOOLEAN CmpGetBiosDateFromRegistry(__in PHHIVE Hive,__in HCELL_INDEX ControlSetKeyCell,__out PUNICODE_STRING BiosDate)
{
	//
	// get control key
	//
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name,L"Control");
	HCELL_INDEX ControlKeyCell							= CmpFindSubKeyByName(Hive,HvGetCell(Hive,ControlSetKeyCell),&Name);
	if(ControlKeyCell == HCELL_NIL)
		return FALSE;

	//
	// get bios info key
	//
	RtlInitUnicodeString(&Name,L"BiosInfo");
	HCELL_INDEX BiosInfoKeyCell							= CmpFindSubKeyByName(Hive,HvGetCell(Hive,ControlKeyCell),&Name);
	if(BiosInfoKeyCell == HCELL_NIL)
		return FALSE;

	//
	// get system bios date
	//
	RtlInitUnicodeString(&Name,L"SystemBiosDate");
	HCELL_INDEX SystemBiosDateValueCell					= CmpFindValueByName(Hive,HvGetCell(Hive,BiosInfoKeyCell),&Name);
	if(SystemBiosDateValueCell == HCELL_NIL)
		return FALSE;

	//
	// build date
	//
	PCM_KEY_VALUE Value									= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,SystemBiosDateValueCell));
	if(!Value || Value->Type != REG_SZ)
		return FALSE;

	ULONG ValueSize										= 0;
	BiosDate->Buffer									= static_cast<PWCHAR>(CmpValueToData(Hive,Value,&ValueSize));
	BiosDate->MaximumLength								= static_cast<USHORT>(ValueSize);
	for(BiosDate->Length = 0; BiosDate->Length < BiosDate->MaximumLength && BiosDate->Buffer[BiosDate->Length / sizeof(WCHAR)]; BiosDate->Length += sizeof(WCHAR));

	return TRUE;
}

//
// get bios info inf file name
//
BOOLEAN CmpGetBiosinfoFileNameFromRegistry(__in PHHIVE Hive,__in HCELL_INDEX ControlSetKeyCell,__out PUNICODE_STRING BiosInfoInf)
{
	//
	// get control key
	//
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name,L"Control");
	HCELL_INDEX ControlKeyCell							= CmpFindSubKeyByName(Hive,HvGetCell(Hive,ControlSetKeyCell),&Name);
	if(ControlKeyCell == HCELL_NIL)
		return FALSE;

	//
	// get bios info key
	//
	RtlInitUnicodeString(&Name,L"BiosInfo");
	HCELL_INDEX BiosInfoKeyCell							= CmpFindSubKeyByName(Hive,HvGetCell(Hive,ControlKeyCell),&Name);
	if(BiosInfoKeyCell == HCELL_NIL)
		return FALSE;

	//
	// get inf name
	//
	RtlInitUnicodeString(&Name,L"InfName");
	HCELL_INDEX InfNameValueCell						= CmpFindValueByName(Hive,HvGetCell(Hive,BiosInfoKeyCell),&Name);
	if(InfNameValueCell == HCELL_NIL)
		return FALSE;

	//
	// build file name
	//
	PCM_KEY_VALUE Value									= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,InfNameValueCell));
	if(!Value || Value->Type != REG_SZ)
		return FALSE;

	ULONG ValueSize										= 0;
	BiosInfoInf->Buffer									= static_cast<PWCHAR>(CmpValueToData(Hive,Value,&ValueSize));
	BiosInfoInf->MaximumLength							= static_cast<USHORT>(ValueSize);
	for(BiosInfoInf->Length = 0; BiosInfoInf->Length < BiosInfoInf->MaximumLength && BiosInfoInf->Buffer[BiosInfoInf->Length / sizeof(WCHAR)]; )
		BiosInfoInf->Length								+= sizeof(WCHAR);

	return TRUE;
}

//
// validate select
//
BOOLEAN CmpValidateSelect(__in PHHIVE Hive,__in HCELL_INDEX RootCell)
{
	//
	// find select key
	//
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name,L"Select");
	HCELL_INDEX SelectKeyCell							= CmpFindSubKeyByName(Hive,HvGetCell(Hive,RootCell),&Name);
	if(SelectKeyCell == HCELL_NIL)
		return FALSE;

	//
	// check current value
	//
	RtlInitUnicodeString(&Name,L"Current");
	HCELL_INDEX CurrentValueCell						= CmpFindValueByName(Hive,HvGetCell(Hive,SelectKeyCell),&Name);
	if(CurrentValueCell == HCELL_NIL)
		return FALSE;

	//
	// check default value
	//
	RtlInitUnicodeString(&Name,L"Default");
	HCELL_INDEX DefaultValueCell						= CmpFindValueByName(Hive,HvGetCell(Hive,SelectKeyCell),&Name);
	if(DefaultValueCell == HCELL_NIL)
		return FALSE;

	//
	// check failed value
	//
	RtlInitUnicodeString(&Name,L"Failed");
	HCELL_INDEX FailedValueCell							= CmpFindValueByName(Hive,HvGetCell(Hive,SelectKeyCell),&Name);
	if(FailedValueCell == HCELL_NIL)
		return FALSE;

	//
	// check last known good value
	//
	RtlInitUnicodeString(&Name,L"LastKnownGood");
	HCELL_INDEX LastKnownGoodValueCell					= CmpFindValueByName(Hive,HvGetCell(Hive,SelectKeyCell),&Name);
	if(LastKnownGoodValueCell == HCELL_NIL)
		return FALSE;

	return TRUE;
}

//
// find control set
//
HCELL_INDEX CmpFindControlSet(__in PHHIVE Hive,__in HCELL_INDEX RootCell,__in PUNICODE_STRING SelectName,__out PBOOLEAN AutoSelect)
{
	//
	// find select key
	//
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name,L"Select");
	HCELL_INDEX SelectKeyCell							= CmpFindSubKeyByName(Hive,HvGetCell(Hive,RootCell),&Name);
	if(SelectKeyCell == HCELL_NIL)
		return HCELL_NIL;

	//
	// get autoselect value
	//
	RtlInitUnicodeString(&Name,L"AutoSelect");
	HCELL_INDEX AutoSelectValueCell						= CmpFindValueByName(Hive,HvGetCell(Hive,SelectKeyCell),&Name);
	if(AutoSelectValueCell == HCELL_NIL)
	{
		*AutoSelect										= TRUE;
	}
	else
	{
		PCM_KEY_VALUE Value								= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,AutoSelectValueCell));
		if(!Value)
			return HCELL_NIL;

		PUCHAR Data										= static_cast<PUCHAR>(CmpValueToData(Hive,Value,0));
		if(!Data)
			return HCELL_NIL;

		*AutoSelect										= *Data;
	}

	//
	// get selected key
	//
	HCELL_INDEX SelectedValueCell						= CmpFindValueByName(Hive,HvGetCell(Hive,SelectKeyCell),SelectName);
	if(SelectedValueCell == HCELL_NIL)
		return HCELL_NIL;

	//
	// get value
	//
	PCM_KEY_VALUE Value									= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,SelectedValueCell));
	if(!Value || Value->Type != REG_DWORD)
		return HCELL_NIL;

	PULONG Data											= static_cast<PULONG>(CmpValueToData(Hive,Value,0));
	if(!Data)
		return HCELL_NIL;

	//
	// build key name
	//
	CHAR SubKeyName[128];
	ANSI_STRING AnsiString;
	WCHAR UnicodeBuffer[128];
	sprintf(SubKeyName,"ControlSet%03d",*Data);

	ULONG Selected										= *Data;
	AnsiString.Buffer									= SubKeyName;
	AnsiString.Length									= strlen(SubKeyName);
	AnsiString.MaximumLength							= AnsiString.Length;
	Name.Buffer											= UnicodeBuffer;
	Name.MaximumLength									= sizeof(UnicodeBuffer);
	if(!NT_SUCCESS(RtlAnsiStringToUnicodeString(&Name,&AnsiString,FALSE)))
		return HCELL_NIL;

	//
	// get controlset key
	//
	HCELL_INDEX ControlSetKeyCell						= CmpFindSubKeyByName(Hive,HvGetCell(Hive,RootCell),&Name);
	if(ControlSetKeyCell == HCELL_NIL)
		return HCELL_NIL;

	//
	// get current value
	//
	RtlInitUnicodeString(&Name,L"Current");
	HCELL_INDEX CurrentValueCell						= CmpFindValueByName(Hive,HvGetCell(Hive,SelectKeyCell),&Name);

	//
	// set current value
	//
	if(CurrentValueCell != HCELL_NIL)
	{
		PCM_KEY_VALUE Value								= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,CurrentValueCell));
		if(!Value)
			return HCELL_NIL;

		if(Value->Type == REG_DWORD)
		{
			PULONG Data									= static_cast<PULONG>(CmpValueToData(Hive,Value,0));
			if(!Data)
				return HCELL_NIL;

			*Data										= Selected;
		}
	}

	return ControlSetKeyCell;
}

//
// find profile option
//
HCELL_INDEX CmpFindProfileOption(__in PHHIVE Hive,__in HCELL_INDEX ControlSetKeyCell,__out_opt PCM_HARDWARE_PROFILE_LIST* ProfileList,
								 __out_opt PCM_HARDWARE_DOCK_INFO_LIST* DockInfoList,__out_opt PULONG ProfileTimeout)
{
	//
	// find control node
	//
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name,L"Control");
	HCELL_INDEX ControlKeyCell							= CmpFindSubKeyByName(Hive,HvGetCell(Hive,ControlSetKeyCell),&Name);
	if(ControlKeyCell == HCELL_NIL)
		return HCELL_NIL;

	//
	// get control key node
	//
	PCM_KEY_NODE ControlKeyNode							= static_cast<PCM_KEY_NODE>(HvGetCell(Hive,ControlKeyCell));
	if(!ControlKeyNode)
		return HCELL_NIL;

	//
	// find IDConfigDB node
	//
	RtlInitUnicodeString(&Name,L"IDConfigDB");
	HCELL_INDEX IDConfigDBKeyCell						= CmpFindSubKeyByName(Hive,ControlKeyNode,&Name);
	if(IDConfigDBKeyCell == HCELL_NIL)
		return HCELL_NIL;

	PCM_KEY_NODE IDConfigDBKeyNode						= static_cast<PCM_KEY_NODE>(HvGetCell(Hive,IDConfigDBKeyCell));
	if(!IDConfigDBKeyNode)
		return HCELL_NIL;

	//
	// find UserWaitInterval value
	//
	if(ProfileTimeout)
	{
		RtlInitUnicodeString(&Name,L"UserWaitInterval");
		*ProfileTimeout									= 0;
		HCELL_INDEX TimeoutValueCell					= CmpFindValueByName(Hive,IDConfigDBKeyNode,&Name);
		if(TimeoutValueCell != HCELL_NIL)
		{
			PCM_KEY_VALUE TimeoutValue					= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,TimeoutValueCell));
			if(!TimeoutValue)
				return HCELL_NIL;

			if(TimeoutValue->Type == REG_DWORD)
			{
				PULONG Value							= static_cast<PULONG>(CmpValueToData(Hive,TimeoutValue,0));
				if(!Value)
					return HCELL_NIL;

				*ProfileTimeout							= *Value;
			}
		}
	}

	if(ProfileList)
	{
		PCM_HARDWARE_PROFILE_LIST List					= *ProfileList;

		//
		// enumerate the keys under IDConfigDB\Hardware Profiles and build the list of available hardware profiles.
		// the list is built sorted by PreferenceOrder
		//
		RtlInitUnicodeString(&Name,L"Hardware Profiles");
		HCELL_INDEX ProfilesKeyCell						= CmpFindSubKeyByName(Hive,IDConfigDBKeyNode,&Name);
		ULONG ProfileCount								= 0;
		if(ProfilesKeyCell == HCELL_NIL)
		{
			if(List)
				List->CurrentProfileCount				= 0;
		}
		else
		{
			PCM_KEY_NODE ProfilesKeyNode				= static_cast<PCM_KEY_NODE>(HvGetCell(Hive,ProfilesKeyCell));
			if(!ProfilesKeyNode)
				return HCELL_NIL;

			//
			// profile count is sub key count
			//
			ProfileCount								= ProfilesKeyNode->SubKeyCounts[0];

			//
			// allocate a larger List
			//
			if(!List || List->MaxProfileCount < ProfileCount)
			{
				ULONG Length							= sizeof(CM_HARDWARE_PROFILE_LIST) + (ProfileCount - 1) * sizeof(CM_HARDWARE_PROFILE);
				List									= static_cast<PCM_HARDWARE_PROFILE_LIST>(Hive->Allocate(Length,FALSE,' 5MC'));
				if(!List)
					return HCELL_NIL;

				List->MaxProfileCount					= ProfileCount;
			}

			List->CurrentProfileCount					= 0;

			//
			// enumerate the keys and fill in the profile list.
			//
			for(ULONG i = 0; i < ProfileCount; i ++)
			{
				CM_HARDWARE_PROFILE TempProfile;
				HCELL_INDEX KeyCell						= CmpFindSubKeyByNumber(Hive,ProfilesKeyNode,i);
				if(KeyCell == HCELL_NIL)
				{
					List->CurrentProfileCount			= i;
					break;
				}

				PCM_KEY_NODE KeyNode					= static_cast<PCM_KEY_NODE>(HvGetCell(Hive,KeyCell));
				if(!KeyNode)
					return HCELL_NIL;

				UNICODE_STRING KeyName;
				WCHAR NameBuffer[20];
				if(KeyNode->Flags & KEY_COMP_NAME)
				{
					KeyName.Buffer						= NameBuffer;
					KeyName.Length						= static_cast<USHORT>(CmpCompressedNameSize(KeyNode->Name,KeyNode->NameLength));
					KeyName.MaximumLength				= sizeof(NameBuffer);

					if(KeyName.MaximumLength < KeyName.Length)
						KeyName.Length					= KeyName.MaximumLength;

					CmpCopyCompressedName(KeyName.Buffer,KeyName.Length,KeyNode->Name,KeyNode->NameLength);
				}
				else
				{
					KeyName.MaximumLength				= KeyNode->NameLength;
					KeyName.Length						= KeyName.MaximumLength;
					KeyName.Buffer						= KeyNode->Name;
				}

				//
				// fill in the temporary profile structure with this profile's data.
				//
				RtlUnicodeStringToInteger(&KeyName,0,&TempProfile.Id);
				RtlInitUnicodeString(&Name,L"PreferenceOrder");
				HCELL_INDEX ValueCell					= CmpFindValueByName(Hive,KeyNode,&Name);
				if(ValueCell == HCELL_NIL)
				{
					TempProfile.PreferenceOrder			= -1;
				}
				else
				{
					PCM_KEY_VALUE ValueNode				= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,ValueCell));
					if(!ValueNode)
						return HCELL_NIL;

					PULONG Data							= static_cast<PULONG>(CmpValueToData(Hive,ValueNode,0));
					if(!Data)
						return HCELL_NIL;

					TempProfile.PreferenceOrder			= *Data;
				}

				//
				// get friendly name
				//
				RtlInitUnicodeString(&Name,L"FriendlyName");
				ValueCell								= CmpFindValueByName(Hive,KeyNode,&Name);
				if(ValueCell == HCELL_NIL)
				{
					TempProfile.FriendlyName			= L"-------";
					TempProfile.NameLength				= wcslen(TempProfile.FriendlyName) * sizeof(WCHAR);
				}
				else
				{
					PCM_KEY_VALUE ValueNode				= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,ValueCell));
					if(!ValueNode)
						return HCELL_NIL;

					ULONG Size;
					PWSTR Data							= static_cast<PWSTR>(CmpValueToData(Hive,ValueNode,&Size));
					if(!Data)
						return HCELL_NIL;

					TempProfile.FriendlyName			= Data;
					TempProfile.NameLength				= Size - sizeof(WCHAR);
				}

				//
				// read Aliasable
				//
				RtlInitUnicodeString(&Name,L"Aliasable");
				ValueCell								= CmpFindValueByName(Hive,KeyNode,&Name);
				if(ValueCell == HCELL_NIL)
				{
					TempProfile.DockState				= 1;
				}
				else
				{
					PCM_KEY_VALUE ValueNode				= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,ValueCell));
					if(!ValueNode)
						return HCELL_NIL;

					PULONG Data							= static_cast<PULONG>(CmpValueToData(Hive,ValueNode,0));
					if(!Data)
						return HCELL_NIL;

					TempProfile.DockState				= *Data ? 1 : 0;
				}

				//
				// read Pristine
				//
				RtlInitUnicodeString(&Name,L"Pristine");
				ValueCell								= CmpFindValueByName(Hive,KeyNode,&Name);
				if(ValueCell != HCELL_NIL)
				{
					PCM_KEY_VALUE ValueNode				= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,ValueCell));
					if(!ValueNode)
						return HCELL_NIL;

					PULONG Data							= static_cast<PULONG>(CmpValueToData(Hive,ValueNode,0));
					if(!Data)
						return HCELL_NIL;

					if(*Data)
						TempProfile.DockState			= 4;
				}

				if(!TempProfile.Id)
				{
					TempProfile.DockState				= 4;
					TempProfile.PreferenceOrder			= -1;
				}

				//
				// insert this new profile into the appropriate spot in the profile array.
				// entries are sorted by preference order.
				//
				ULONG j									= 0;
				for(; j < List->CurrentProfileCount; j ++)
				{
					if(List->Profile[j].PreferenceOrder >= TempProfile.PreferenceOrder)
					{
						//
						// insert at position j.
						//
						RtlMoveMemory(&List->Profile[j + 1],&List->Profile[j],sizeof(CM_HARDWARE_PROFILE) * (List->MaxProfileCount - j - 1));
						break;
					}
				}

				List->Profile[j]						= TempProfile;
				List->CurrentProfileCount				+= 1;
			}
		}

		*ProfileList									= List;
	}

	if(DockInfoList)
	{
		PCM_HARDWARE_DOCK_INFO_LIST List				= *DockInfoList;

		//
		// enumerate the keys under IDConfigDB\Alias and build the list of available alias.
		//
		RtlInitUnicodeString(&Name,L"Alias");
		HCELL_INDEX AliasKeyCell						= CmpFindSubKeyByName(Hive,IDConfigDBKeyNode,&Name);
		ULONG AliasCount								= 0;
		if(AliasKeyCell == HCELL_NIL)
		{
			if(List)
				List->CurrentCount						= 0;
		}
		else
		{
			PCM_KEY_NODE AliasKeyNode					= static_cast<PCM_KEY_NODE>(HvGetCell(Hive,AliasKeyCell));
			if(!AliasKeyNode)
				return HCELL_NIL;

			//
			// profile count is sub key count
			//
			AliasCount									= AliasKeyNode->SubKeyCounts[0];

			//
			// allocate a larger List
			//
			if(!List || List->MaxCount < AliasCount)
			{
				//
				// BUGBUGBUG ?
				//
				ULONG Length							= sizeof(CM_HARDWARE_PROFILE_LIST) + (AliasCount - 1) * sizeof(CM_HARDWARE_PROFILE);
				List									= static_cast<PCM_HARDWARE_DOCK_INFO_LIST>(Hive->Allocate(Length,FALSE,' 6MC'));
				if(!List)
					return HCELL_NIL;

				List->MaxCount							= AliasCount;
			}

			List->CurrentCount							= 0;

			//
			// enumerate the keys and fill in the list.
			//
			for(ULONG i = 0; i < AliasCount; i ++)
			{
				PCM_HARDWARE_DOCK_INFO CurDockInfo		= List->DockInfo + i;
				HCELL_INDEX KeyCell						= CmpFindSubKeyByNumber(Hive,AliasKeyNode,i);
				if(KeyCell == HCELL_NIL)
				{
					List->CurrentCount					= i;
					break;
				}

				PCM_KEY_NODE KeyNode					= static_cast<PCM_KEY_NODE>(HvGetCell(Hive,KeyCell));
				if(!KeyNode)
					return HCELL_NIL;

				//
				// read profile number
				//
				RtlInitUnicodeString(&Name,L"ProfileNumber");
				HCELL_INDEX ValueCell					= CmpFindValueByName(Hive,KeyNode,&Name);
				if(ValueCell == HCELL_NIL)
				{
					CurDockInfo->ProfileNumber			= 0;
				}
				else
				{
					PCM_KEY_VALUE ValueNode				= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,ValueCell));
					if(!ValueNode)
						return HCELL_NIL;

					PULONG Data							= static_cast<PULONG>(CmpValueToData(Hive,ValueNode,0));
					if(!Data)
						return HCELL_NIL;

					CurDockInfo->ProfileNumber			= *Data;
				}

				//
				// DockState
				//
				RtlInitUnicodeString(&Name,L"DockState");
				ValueCell								= CmpFindValueByName(Hive,KeyNode,&Name);
				if(ValueCell == HCELL_NIL)
				{
					CurDockInfo->DockState				= 0;
				}
				else
				{
					PCM_KEY_VALUE ValueNode				= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,ValueCell));
					if(!ValueNode)
						return HCELL_NIL;

					PULONG Data							= static_cast<PULONG>(CmpValueToData(Hive,ValueNode,0));
					if(!Data)
						return HCELL_NIL;

					CurDockInfo->DockState				= *Data;
				}

				//
				// read DockID
				//
				RtlInitUnicodeString(&Name,L"DockID");
				ValueCell								= CmpFindValueByName(Hive,KeyNode,&Name);
				if(ValueCell == HCELL_NIL)
				{
					CurDockInfo->DockID					= 0;
				}
				else
				{
					PCM_KEY_VALUE ValueNode				= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,ValueCell));
					if(!ValueNode)
						return HCELL_NIL;

					PULONG Data							= static_cast<PULONG>(CmpValueToData(Hive,ValueNode,0));
					if(!Data)
						return HCELL_NIL;

					CurDockInfo->DockID					= *Data;
				}

				//
				// read SerialNumber
				//
				RtlInitUnicodeString(&Name,L"SerialNumber");
				ValueCell								= CmpFindValueByName(Hive,KeyNode,&Name);
				if(ValueCell == HCELL_NIL)
				{
					CurDockInfo->SerialNumber			= 0;
				}
				else
				{
					PCM_KEY_VALUE ValueNode				= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,ValueCell));
					if(!ValueNode)
						return HCELL_NIL;

					PULONG Data							= static_cast<PULONG>(CmpValueToData(Hive,ValueNode,0));
					if(!Data)
						return HCELL_NIL;

					CurDockInfo->SerialNumber			= *Data;
				}

				List->CurrentCount						+= 1;
			}
		}

		*DockInfoList									= List;
	}

	return IDConfigDBKeyCell;
}

//
// filter docking state
//
VOID BlDockInfoFilterDockingState(__inout PCM_HARDWARE_PROFILE_LIST ProfileList,__in PCM_HARDWARE_DOCK_INFO_LIST DockInfoList,
								  __in USHORT Capabilities,__in ULONG DockID,__in ULONG SerialNumber)
{
	BOOLEAN Found										= FALSE;

	if(DockInfoList)
	{
		for(ULONG i = 0; i < DockInfoList->CurrentCount; i ++)
		{
			PCM_HARDWARE_DOCK_INFO DockInfo				= DockInfoList->DockInfo + i;

			if(((DockInfo->DockState & 3) == Capabilities || !(DockInfo->DockState & 3)) && DockInfo->DockID == DockID && DockInfo->SerialNumber == SerialNumber)
			{
				for(ULONG j = 0; j < ProfileList->CurrentProfileCount; j ++)
				{
					PCM_HARDWARE_PROFILE Profile		= ProfileList->Profile + j;
					if(Profile->Id == DockInfo->ProfileNumber)
					{
						Found							= TRUE;
						Profile->DockState				= 2;
					}
				}
			}
		}
	}

	ULONG Count = 0;
	PCM_HARDWARE_PROFILE WritePosition					= ProfileList->Profile;
	PCM_HARDWARE_PROFILE CurrentPosition				= WritePosition;
	PCM_HARDWARE_PROFILE NextPosition					= CurrentPosition + 1;

	while(Count < ProfileList->CurrentProfileCount)
	{
		ULONG CurrentState								= CurrentPosition->DockState;

		if((!(CurrentState & 4) || Found || !DockInfoList) && !(CurrentState & 3))
		{
			ULONG CopyLength							= sizeof(CM_HARDWARE_PROFILE) * (ProfileList->CurrentProfileCount - Count - 1);

			if(CopyLength)
				RtlMoveMemory(WritePosition,NextPosition,CopyLength);

			ProfileList->CurrentProfileCount			-= 1;
			continue;
		}

		Count											+= 1;
		NextPosition									+= 1;
		CurrentPosition									+= 1;
		WritePosition									+= 1;
	}
}

//
// filter profile
//
VOID BlDockInfoFilterProfileList(__inout PCM_HARDWARE_PROFILE_LIST ProfileList,__in PCM_HARDWARE_DOCK_INFO_LIST DockInfoList)
{
	if(!ProfileList)
		return;

	if(ProfileList->CurrentProfileCount == 1 && (ProfileList->Profile[0].DockState & 4))
		return;

	PPROFILE_PARAMETER_BLOCK ProfileBlock				= &BlLoaderBlock->Extension->Profile;
	BlDockInfoFilterDockingState(ProfileList,DockInfoList,ProfileBlock->Capabilities,ProfileBlock->DockID,ProfileBlock->SerialNumber);
}

//
// set current profile
//
VOID CmpSetCurrentProfile(__in PHHIVE Hive,__in HCELL_INDEX ControlSetKeyCell,__in PCM_HARDWARE_PROFILE Profile)
{
	//
	// get profile control key
	//
	HCELL_INDEX ProfileControlKeyCell					= CmpFindProfileOption(Hive,ControlSetKeyCell,0,0,0);
	if(ProfileControlKeyCell == HCELL_NIL)
		return;

	//
	// get current config value
	//
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name,L"CurrentConfig");
	HCELL_INDEX CurrentConfigValueCell					= CmpFindValueByName(Hive,HvGetCell(Hive,ProfileControlKeyCell),&Name);
	if(CurrentConfigValueCell == HCELL_NIL)
		return;

	//
	// set value
	//
	PCM_KEY_VALUE Value									= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,CurrentConfigValueCell));
	if(!Value || Value->Type != REG_DWORD)
		return;

	PULONG Data											= static_cast<PULONG>(CmpValueToData(Hive,Value,0));
	if(Data)
		*Data											= Profile->Id;
}

//
// update profile option
//
VOID BlUpdateProfileOption(__in PCM_HARDWARE_PROFILE_LIST ProfileList,__in HCELL_INDEX ControlSetKeyCell,__in ULONG Index)
{
	if(!ProfileList || Index >= ProfileList->CurrentProfileCount)
		return;

	PPROFILE_PARAMETER_BLOCK ProfileBlock				= &BlLoaderBlock->Extension->Profile;
	if(ProfileBlock->Status == HW_PROFILE_STATUS_SUCCESS)
	{
		ULONG Temp										= ProfileList->Profile[Index].DockState;

		if(Temp & 4)
			ProfileBlock->DockingState					= HW_PROFILE_DOCKSTATE_UNKNOWN;
		else if(Temp & 2)
			ProfileBlock->DockingState					= HW_PROFILE_DOCKSTATE_DOCKED;
		else if(Temp & 1)
			ProfileBlock->DockingState					= HW_PROFILE_DOCKSTATE_UNDOCKED;
	}

	//
	// update hive
	//
	CmpSetCurrentProfile(&BootHive.Hive,ControlSetKeyCell,ProfileList->Profile + Index);
}
//
// determine control set
//
HCELL_INDEX BlpDetermineControlSet(__inout PBOOLEAN UseLastKnowGood)
{
	UNICODE_STRING DefaultString;
	UNICODE_STRING LastKnownGoodString;
	RtlInitUnicodeString(&DefaultString,L"Default");
	RtlInitUnicodeString(&LastKnownGoodString,L"LastKnownGood");

	extern BOOLEAN ForceLastKnownGood;
	BOOLEAN UseLastKnownGoodKey							= ForceLastKnownGood ? TRUE : *UseLastKnowGood;

	//
	// validate
	//
	if(!CmpValidateSelect(&BootHive.Hive,BootHive.Hive.BaseBlock->RootCell))
		return HCELL_NIL;

	while(1)
	{
		//
		// find control set
		//
		PUNICODE_STRING KeyName							= UseLastKnownGoodKey ? &LastKnownGoodString : &DefaultString;
		BOOLEAN AutoSelect								= FALSE;
		HCELL_INDEX ControlSetKeyCell					= CmpFindControlSet(&BootHive.Hive,BootHive.Hive.BaseBlock->RootCell,KeyName,&AutoSelect);
		if(ControlSetKeyCell == HCELL_NIL)
			return HCELL_NIL;

		//
		// find profile option
		//
		PCM_HARDWARE_PROFILE_LIST ProfileList			= 0;
		PCM_HARDWARE_DOCK_INFO_LIST DockInfoList		= 0;
		ULONG ProfileTimeout							= 0;
		HCELL_INDEX ProfileControlKeyCell				= CmpFindProfileOption(&BootHive.Hive,ControlSetKeyCell,&ProfileList,&DockInfoList,&ProfileTimeout);

		//
		// get docking data
		//
		PCONFIGURATION_COMPONENT_DATA DockingData		= KeFindConfigurationEntry(BlLoaderBlock->ConfigurationRoot,PeripheralClass,DockingInformation,0);
		PPROFILE_PARAMETER_BLOCK ProfileBlock			= &BlLoaderBlock->Extension->Profile;
		RtlZeroMemory(ProfileBlock,sizeof(PROFILE_PARAMETER_BLOCK));

		if(!DockingData)
		{
			ProfileBlock->Status						= HW_PROFILE_STATUS_SUCCESS;
			ProfileBlock->DockingState					= HW_PROFILE_DOCKSTATE_UNKNOWN;
		}
		else if(DockingData->ComponentEntry.ConfigurationDataLength < 0x0c)
		{
			ProfileBlock->Status						= HW_PROFILE_STATUS_SUCCESS;
			ProfileBlock->DockingState					= HW_PROFILE_DOCKSTATE_UNSUPPORTED;
		}
		else
		{
			ProfileBlock->Status						= HW_PROFILE_STATUS_SUCCESS;

			USHORT State								= 0;
			RtlCopyMemory(&State,Add2Ptr(DockingData->ConfigurationData,0x18 + 0x0a,PUSHORT),sizeof(USHORT));

			switch(State)
			{
			case 0:
				ProfileBlock->DockingState				= HW_PROFILE_DOCKSTATE_DOCKED;
				break;

			case 0x87:
				ProfileBlock->DockingState				= HW_PROFILE_DOCKSTATE_UNDOCKED;
				break;

			case 0x89:
				ProfileBlock->DockingState				= HW_PROFILE_DOCKSTATE_UNKNOWN;
				break;

			default:
				ProfileBlock->Status					= HW_PROFILE_STATUS_FAILURE;

			case 0x82:
			case 0xffff:
				ProfileBlock->DockingState				= HW_PROFILE_DOCKSTATE_UNSUPPORTED;
				break;
			}

			RtlCopyMemory(&ProfileBlock->DockID,Add2Ptr(DockingData->ConfigurationData,0x18 + 0x00,PVOID),sizeof(ULONG));
			RtlCopyMemory(&ProfileBlock->SerialNumber,Add2Ptr(DockingData->ConfigurationData,0x18 + 0x04,PVOID),sizeof(ULONG));
			RtlCopyMemory(&ProfileBlock->Capabilities,Add2Ptr(DockingData->ConfigurationData,0x18 + 0x08,PVOID),sizeof(USHORT));
		}

		//
		// filter profile list
		//
		if(ProfileList && ProfileList->CurrentProfileCount)
			BlDockInfoFilterProfileList(ProfileList,DockInfoList);

		//
		// check to see whether the config menu should be displayed.
		// Display the menu if:
		//  - user has pressed a key OR
		//  - we are booting from LKG and AutoSelect is FALSE. OR
		//  - ProfileTimeout != 0
		//
		BOOLEAN KeyPressed								= BlEndConfigPrompt();
		if(!KeyPressed && !UseLastKnownGoodKey && ForceLastKnownGood)
		{
			UseLastKnownGoodKey							= TRUE;
			continue;
		}

		if(KeyPressed || ForceLastKnownGood || (UseLastKnownGoodKey && !AutoSelect) || (ProfileTimeout && ProfileList && ProfileList->CurrentProfileCount > 1))
		{
			//
			// display config menu
			//
			extern BOOLEAN BlRebootSystem;
			BlRebootSystem								= !BlConfigMenuPrompt(ProfileTimeout,&UseLastKnownGoodKey,&ControlSetKeyCell,&ProfileList,&DockInfoList);

			//
			// clear screen
			//
			BlClearScreen();
		}
		else
		{
			//
			// the system is configured to boot the default profile directly.
			// since the returned profile list is sorted by priority, the first entry in the list is our default.
			//
			if(ProfileControlKeyCell != HCELL_NIL)
				BlUpdateProfileOption(ProfileList,ControlSetKeyCell,0);
		}

		*UseLastKnowGood								= UseLastKnownGoodKey;

		return ControlSetKeyCell;
	}

	return HCELL_NIL;
}

//
// switch control set
//
VOID BlpSwitchControlSet(__out PCM_HARDWARE_PROFILE_LIST* ProfileList,__out PCM_HARDWARE_DOCK_INFO_LIST* DockInfoList,
						 __in BOOLEAN UseLastKnownGood,__out PHCELL_INDEX ControlSetKeyCell)
{
	UNICODE_STRING ControlName;
	RtlInitUnicodeString(&ControlName,UseLastKnownGood ? L"LastKnownGood" : L"Default");

	BOOLEAN AutoSelect									= FALSE;
	HCELL_INDEX	NewControlSet							= CmpFindControlSet(&BootHive.Hive,BootHive.Hive.BaseBlock->RootCell,&ControlName,&AutoSelect);
	if(NewControlSet == HCELL_NIL)
		return;

	CmpFindProfileOption(&BootHive.Hive,NewControlSet,ProfileList,DockInfoList,0);

	*ControlSetKeyCell									= NewControlSet;
}

//
// disable auto reboot after crash dump
//
VOID BlCheckAndDisableAutoRebootCrashRecover(__in PHCELL_INDEX ControlSetKeyCell)
{
	extern BOOLEAN BlDisableCrashAutoReboot;
	if(!BlDisableCrashAutoReboot)
		return;

	//
	// find control key
	//
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name,L"Control");
	HCELL_INDEX ControlKeyCell							= CmpFindSubKeyByName(&BootHive.Hive,HvGetCell(&BootHive.Hive,*ControlSetKeyCell),&Name);
	if(ControlKeyCell == HCELL_NIL)
		return;

	//
	// find crash control
	//
	RtlInitUnicodeString(&Name,L"CrashControl");
	HCELL_INDEX CrashControlKeyCell						= CmpFindSubKeyByName(&BootHive.Hive,HvGetCell(&BootHive.Hive,ControlKeyCell),&Name);
	if(CrashControlKeyCell == HCELL_NIL)
		return;

	//
	// find autoboot value
	//
	RtlInitUnicodeString(&Name,L"AutoReboot");
	HCELL_INDEX AutoRebootValueCell						= CmpFindValueByName(&BootHive.Hive,HvGetCell(&BootHive.Hive,CrashControlKeyCell),&Name);
	if(AutoRebootValueCell == HCELL_NIL)
		return;

	PCM_KEY_VALUE Value									= static_cast<PCM_KEY_VALUE>(HvGetCell(&BootHive.Hive,AutoRebootValueCell));
	if(!Value || Value->Type != REG_DWORD)
		return;

	PULONG Data											= static_cast<PULONG>(CmpValueToData(&BootHive.Hive,Value,0));
	if(Data)
		*Data											= FALSE;
}

//
// disable verifier check
//
VOID BlCheckAndDisableVerifierOnCurrentBoot(__in PHCELL_INDEX ControlSetKeyCell)
{
	extern BOOLEAN BlDisableVerifier;
	if(!BlDisableVerifier)
		return;

	//
	// find control key
	//
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name,L"Control");
	HCELL_INDEX ControlKeyCell							= CmpFindSubKeyByName(&BootHive.Hive,HvGetCell(&BootHive.Hive,*ControlSetKeyCell),&Name);
	if(ControlKeyCell == HCELL_NIL)
		return;

	//
	// open session manager key
	//
	RtlInitUnicodeString(&Name,L"Session Manager");
	HCELL_INDEX SessionMgrKeyCell						= CmpFindSubKeyByName(&BootHive.Hive,HvGetCell(&BootHive.Hive,ControlKeyCell),&Name);
	if(SessionMgrKeyCell == HCELL_NIL)
		return;

	//
	// open memory manager key
	//
	RtlInitUnicodeString(&Name,L"Memory Management");
	HCELL_INDEX MemoryMgrKeyCell						= CmpFindSubKeyByName(&BootHive.Hive,HvGetCell(&BootHive.Hive,SessionMgrKeyCell),&Name);
	if(MemoryMgrKeyCell == HCELL_NIL)
		return;

	//
	// open verify mode value
	//
	RtlInitUnicodeString(&Name,L"VerifyMode");
	HCELL_INDEX VerifyModeValueCell						= CmpFindValueByName(&BootHive.Hive,HvGetCell(&BootHive.Hive,MemoryMgrKeyCell),&Name);
	if(VerifyModeValueCell == HCELL_NIL)
		return;

	//
	// check and set verify mode
	//
	PCM_KEY_VALUE Value									= static_cast<PCM_KEY_VALUE>(HvGetCell(&BootHive.Hive,VerifyModeValueCell));
	if(!Value || Value->Type != REG_DWORD)
		return;

	PULONG Data											= static_cast<PULONG>(CmpValueToData(&BootHive.Hive,Value,0));
	if(Data && *Data == 1)
		*Data											= 0;
}

//
// scan registry
//
PCHAR BlScanRegistry(__in PWCHAR BootFileSystem,__inout PBOOLEAN UseLastKnownGood,__in PLIST_ENTRY BootDriverListHead,__out PUNICODE_STRING AnsiCodePage,
					 __out PUNICODE_STRING OemCodePage,__out PUNICODE_STRING LanguageTable,__out PUNICODE_STRING OemHalFont,__out PUNICODE_STRING InfFile,
					 __out struct _SETUP_LOADER_BLOCK* SetupLoaderBlock,__out PBOOLEAN LoadSacDriver)
{
	//
	// find out which control set we should use
	//
	HCELL_INDEX ControlSetKeyCell						= BlpDetermineControlSet(UseLastKnownGood);
	if(ControlSetKeyCell == HCELL_NIL)
		return "CmpFindControlSet";

	//
	// find nls data
	//
	if(!CmpFindNLSData(&BootHive.Hive,ControlSetKeyCell,AnsiCodePage,OemCodePage,LanguageTable,OemHalFont))
		return "CmpFindNLSData";

	//
	// find boot drivers
	//
	InitializeListHead(BootDriverListHead);
	if(!CmpFindDrivers(&BootHive.Hive,ControlSetKeyCell,BootLoad,BootFileSystem,BootDriverListHead))
		return "CmpFindDrivers";

	//
	// sort drivers list by order and tag
	//
	if(!CmpSortDriverList(&BootHive.Hive,ControlSetKeyCell,BootDriverListHead))
		return "Missing or invalid Control\\ServiceGroupOrder\\List registry value";

	//
	// resolve dependencies
	//
	if(!CmpResolveDriverDependencies(BootDriverListHead))
		return "CmpResolveDriverDependencies";

	//
	// only load sac on a server family
	//
	if(LoadSacDriver)
	{
		*LoadSacDriver									= FALSE;

		//
		// find control key
		//
		UNICODE_STRING Name;
		RtlInitUnicodeString(&Name,L"Control");
		HCELL_INDEX ControlKeyCell						= CmpFindSubKeyByName(&BootHive.Hive,HvGetCell(&BootHive.Hive,ControlSetKeyCell),&Name);
		if(ControlKeyCell == HCELL_NIL)
			return "Missing Control key";

		//
		// find product options key
		//
		RtlInitUnicodeString(&Name,L"ProductOptions");
		HCELL_INDEX ProductOptionsKeyCell				= CmpFindSubKeyByName(&BootHive.Hive,HvGetCell(&BootHive.Hive,ControlKeyCell),&Name);
		if(ProductOptionsKeyCell == HCELL_NIL)
			return "Missing ProductOptions key";

		//
		// read product type
		//
		RtlInitUnicodeString(&Name,L"ProductType");
		HCELL_INDEX ProductTypeValueCell				= CmpFindValueByName(&BootHive.Hive,HvGetCell(&BootHive.Hive,ProductOptionsKeyCell),&Name);
		if(ProductTypeValueCell == HCELL_NIL)
			return "Missing ProductType value";

		//
		// get value
		//
		PCM_KEY_VALUE Value								= static_cast<PCM_KEY_VALUE>(HvGetCell(&BootHive.Hive,ProductTypeValueCell));
		if(!Value || Value->Type != REG_SZ)
			return "Bad ProductType value";

		//
		// get data
		//
		PWCHAR Data										= static_cast<PWCHAR>(CmpValueToData(&BootHive.Hive,Value,0));
		if(Data)
			*LoadSacDriver								= _wcsicmp(Data,L"WinNT") != 0;
	}

	//
	// check bios date
	//
	BOOLEAN ReadBiosInf									= TRUE;
	UNICODE_STRING BiosDateStringFromRegistry;
	if(CmpGetBiosDateFromRegistry(&BootHive.Hive,ControlSetKeyCell,&BiosDateStringFromRegistry))
	{
		//
		// read [ffff5,ffffcd)
		//
		CHAR AnsiBiosDateStringFromMemoryBuffer[9];
		RtlCopyMemory(&AnsiBiosDateStringFromMemoryBuffer,reinterpret_cast<PVOID>(0xffff5),8);
		AnsiBiosDateStringFromMemoryBuffer[8]			= 0;

		//
		// build an ansi string
		//
		ANSI_STRING AnsiBiosDateStringFromMemory;
		RtlInitAnsiString(&AnsiBiosDateStringFromMemory,AnsiBiosDateStringFromMemoryBuffer);

		//
		// convert to unicode string
		//
		UNICODE_STRING BiosDateStringFromMemory;
		WCHAR BiosDateStringFromMemoryBuffer[10];
		BiosDateStringFromMemory.Buffer					= BiosDateStringFromMemoryBuffer;
		BiosDateStringFromMemory.MaximumLength			= sizeof(BiosDateStringFromMemoryBuffer);
		RtlAnsiStringToUnicodeString(&BiosDateStringFromMemory,&AnsiBiosDateStringFromMemory,FALSE);

		//
		// if the value we found in memory is the same to the value we recorded in registry,skip read biosinfo
		//
		if(!RtlCompareUnicodeString(&BiosDateStringFromRegistry,&BiosDateStringFromMemory,TRUE))
			ReadBiosInf									= FALSE;
	}

	InfFile->Length										= 0;

	if(ReadBiosInf)
		CmpGetBiosinfoFileNameFromRegistry(&BootHive.Hive,ControlSetKeyCell,InfFile);

	//
	// should we disable auto reboot after crash dump?
	//
	BlCheckAndDisableAutoRebootCrashRecover(&ControlSetKeyCell);

	//
	// should we disable verifier
	//
	BlCheckAndDisableVerifierOnCurrentBoot(&ControlSetKeyCell);

	return 0;
}

//
// check restart setup
//
BOOLEAN BlpCheckRestartSetup()
{
	//
	// find the Setup key
	//
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name,L"Setup");
	HCELL_INDEX SetuptKeyCell							= CmpFindSubKeyByName(&BootHive.Hive,HvGetCell(&BootHive.Hive,BootHive.Hive.BaseBlock->RootCell),&Name);
	if(SetuptKeyCell == HCELL_NIL)
		return FALSE;

	//
	// find restartSetup value in Setup key
	//
	RtlInitUnicodeString(&Name,L"RestartSetup");
	HCELL_INDEX RestartSetupValueCell					= CmpFindValueByName(&BootHive.Hive,HvGetCell(&BootHive.Hive,SetuptKeyCell),&Name);
	if(RestartSetupValueCell == HCELL_NIL)
		return FALSE;

	//
	// Validate value and check.
	//
	PCM_KEY_VALUE Value									= static_cast<PCM_KEY_VALUE>(HvGetCell(&BootHive.Hive,RestartSetupValueCell));
	if(!Value || Value->Type != REG_DWORD)
		return FALSE;

	ULONG Length										= 0;
	PULONG Data											= static_cast<PULONG>(CmpValueToData(&BootHive.Hive,Value,&Length));
	if(Length != sizeof(ULONG) || !Data)
		return FALSE;

	return *Data != 0;
}

//
// allocate routine
//
PVOID BlpHiveAllocate(__in ULONG Size,__in BOOLEAN UsedForIo,__in ULONG PoolTag)
{
	return BlAllocateHeap(Size);
}

//
// initializes the hive data
//
BOOLEAN BlInitializeHive(__in PVOID HiveImage,__in PCMHIVE Hive,__in BOOLEAN IsAlternate)
{
	//
	// initialize it
	//
	NTSTATUS Status										= HvInitializeHive(&Hive->Hive,HINIT_MEMORY_INPLACE,FALSE,IsAlternate ? HFILE_TYPE_ALTERNATE : HFILE_TYPE_PRIMARY,
																		   HiveImage,reinterpret_cast<PALLOCATE_ROUTINE>(&BlpHiveAllocate),0,0,0,0,0,1,0);
	if(!NT_SUCCESS(Status))
		return FALSE;

	//
	// check it
	//	0x10000	= check hive
	//	0x00004	= force volatiles sub key to be empty
	//
	if(CmCheckRegistry(Hive,0x10004))
		return FALSE;

	return TRUE;
}

//
// load and init system hive
//
ARC_STATUS BlLoadAndInitSystemHive(__in ULONG DeviceId,__in PCHAR DeviceName,__in PCHAR DirectoryPath,__in PCHAR HiveName,
								   __in BOOLEAN IsAlternate,__out PBOOLEAN Recovered,__out PBOOLEAN RestartSetup)
{
	*Recovered											= FALSE;
	*RestartSetup										= FALSE;

	BlClearToEndOfLine();

	//
	// load file
	//
	ARC_STATUS Status									= BlLoadSystemHive(DeviceId,DeviceName,DirectoryPath,HiveName);
	if(Status != ESUCCESS)
		return Status;

	//
	// try to initialize it
	//
	if(!BlInitializeHive(BlLoaderBlock->RegistryBase,&BootHive,IsAlternate))
	{
		//
		// already use alternate hive
		//
		if(IsAlternate)
			return EINVAL;

		//
		// load system.log
		//
		PVOID SystemLog									= 0;
		Status											= BlLoadSystemHiveLog(DeviceId,DeviceName,DirectoryPath,"system.log",&SystemLog);
		if(Status != ESUCCESS)
			return Status;

		//
		// try to recover from system.log
		//
		if(BlRecoverHive(BlLoaderBlock->RegistryBase,SystemLog))
		{
			//
			// recover from alternate
			//
			*Recovered									= TRUE;

			//
			// free system.log and initialize system hive again
			//
			BlFreeDescriptor(GET_PHYSICAL_PAGE(SystemLog));

			if(!BlInitializeHive(BlLoaderBlock->RegistryBase,&BootHive,FALSE))
				return EINVAL;

			BootHive.Hive.BaseBlock->RecoveredFromLog	= TRUE;
		}
		else
		{
			//
			// recover failed,free memory add return error status
			//
			BlFreeDescriptor(GET_PHYSICAL_PAGE(SystemLog));
			return EINVAL;
		}
	}
	else
	{
		BootHive.Hive.BaseBlock->RecoveredFromLog		= FALSE;
	}

	//
	// check restart setup
	//
	*RestartSetup										= BlpCheckRestartSetup();

	return ESUCCESS;
}