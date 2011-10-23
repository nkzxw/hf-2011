//********************************************************************
//	created:	25:8:2008   1:06
//	file:		registry.cpp
//	author:		tiamo
//	purpose:	cm hive support
//********************************************************************

#include "stdafx.h"

//
// try to fix error
//
#define CM_TRY_TO_FIX_ERROR								(CmpSelfHeal || (CmpBootType & 6))

//
// self heal
//
BOOLEAN													CmpSelfHeal = TRUE;

//
// boot type
//
UCHAR													CmpBootType;

//
// check hive debug
//
CHECK_HIVE_DEBUG										HvCheckHiveDebug;

//
// check registry debug
//
CHECK_REGISTRY_DEBUG									CmCheckRegistryDebug;

//
// check registry2 debug
//
CHECK_REGISTRY_DEBUG									CmpCheckRegistry2Debug;

//
// check bin debug
//
CHECK_BIN_DEBUG											HvCheckBinDebug;

//
// check key debug
//
CHECK_KEY_DEBUG											 CmpCheckKeyDebug;

//
// check value list debug
//
CHECK_VALUE_LIST_DEBUG									CmpCheckValueListDebug;

//
// debug checking cell
//
HCELL_INDEX												CmpKeyCellDebug = HCELL_NIL;

//
// master hive
//
PCMHIVE													CmpMasterHive;

//
// find first set left
//
CCHAR													KiFindFirstSetLeft[256] =
{
	0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

//
// value to data
//
BOOLEAN CmpGetValueData(__in PHHIVE Hive,__in PCM_KEY_VALUE Value,__out_opt PULONG DataLength,__out PVOID* Buffer,__out PBOOLEAN BufferAllocated,__out PHCELL_INDEX DateCell)
{
	if(Value->Signature != CM_KEY_VALUE_SIGNATURE)
		return FALSE;

	*BufferAllocated									= FALSE;
	BOOLEAN SmallData									= Value->DataLength >= CM_KEY_VALUE_SPECIAL_SIZE ? TRUE : FALSE;
	ULONG Length										= SmallData ? Value->DataLength - CM_KEY_VALUE_SPECIAL_SIZE : Value->DataLength;
	if(SmallData)
	{
		*Buffer											= &Value->Data;
		*DateCell										= HCELL_NIL;
	}
	else
	{
		*DateCell										= Value->Data;
		*Buffer											= HvGetCell(Hive,Value->Data);
	}

	if(DataLength)
		*DataLength										= Length;

	return *Buffer == 0 ? FALSE : TRUE;
}

//
// value to data
//
PVOID CmpValueToData(__in PHHIVE Hive,__in PCM_KEY_VALUE Value,__out_opt PULONG DataLength)
{
	PVOID Data											= 0;
	BOOLEAN BufferAllocated								= FALSE;
	HCELL_INDEX DataCell								= HCELL_NIL;
	if(CmpGetValueData(Hive,Value,DataLength,&Data,&BufferAllocated,&DataCell))
		return Data;

	return 0;
}

//
// compare compressed name
//
LONG CmpCompareCompressedName(__in PCUNICODE_STRING SearchName,__in PWCHAR CompressedName,__in ULONG NameLength,__in ULONG UpcaseFlags)
{
	PWCHAR s1											= SearchName->Buffer;
	PUCHAR s2											= reinterpret_cast<PUCHAR>(CompressedName);
	USHORT n1											= static_cast<USHORT>(SearchName->Length / sizeof(WCHAR));
	USHORT n2											= static_cast<USHORT>(NameLength);
	while(n1 && n2)
	{
		WCHAR c1										= *s1++;
		WCHAR c2										= *s2++;

		if(!(UpcaseFlags & 1))
			c1											= RtlUpcaseUnicodeChar(c1);

		if(!(UpcaseFlags & 2))
			c2											= RtlUpcaseUnicodeChar(c2);

		LONG cDiff										= static_cast<LONG>(c1) - static_cast<LONG>(c2);
		if(cDiff)
			return cDiff;

		n1												-= 1;
		n2												-= 1;
	}

	return n1 - n2;
}

//
// find name in list
//
BOOLEAN CmpFindNameInList(__in PHHIVE Hive,__in PCHILD_LIST ChildList,__in PCUNICODE_STRING Name,__out_opt PULONG ChildIndex,__out PHCELL_INDEX CellIndex)
{
	PCELL_DATA List										= 0;
	HCELL_INDEX LastCellIndex							= HCELL_NIL;
	*CellIndex											= HCELL_NIL;
	BOOLEAN Ret											= TRUE;

	__try
	{
		//
		// child list is empty
		//
		if(!ChildList->Count)
			try_leave(ChildIndex ? *ChildIndex = 0 : FALSE);

		//
		// get list
		//
		List											= static_cast<PCELL_DATA>(HvGetCell(Hive,ChildList->List));
		if(!List)
			try_leave(Ret = FALSE);

		//
		// search children
		//
		for(ULONG i = 0; i < ChildList->Count; i ++)
		{
			//
			// release last cell first
			//
			if(LastCellIndex)
			{
				HvReleaseCell(Hive,LastCellIndex);
				LastCellIndex							= HCELL_NIL;
			}

			//
			// get current cell
			//
			PCM_KEY_VALUE CurrentValue					= static_cast<PCM_KEY_VALUE>(HvGetCell(Hive,List->u.KeyList[i]));
			if(!CurrentValue)
				try_leave(Ret = FALSE);

			//
			// save it
			//
			LastCellIndex								= List->u.KeyList[i];

			//
			// compare those name
			//
			LONG Result									= 0;
			if(CurrentValue->Flags & VALUE_COMP_NAME)
			{
				Result									= CmpCompareCompressedName(Name,CurrentValue->Name,CurrentValue->NameLength,0);
			}
			else
			{
				UNICODE_STRING String;
				String.Buffer							= CurrentValue->Name;
				String.Length							= CurrentValue->NameLength;
				String.MaximumLength					= String.Length;
				Result									= RtlCompareUnicodeString(Name,&String,TRUE);
			}

			//
			// the name is the same,found it
			//
			if(Result == 0)
				try_leave(*CellIndex = LastCellIndex; ChildIndex ? *ChildIndex = i : FALSE);
		}

		if(ChildIndex)
			*ChildIndex									= ChildList->Count;
	}
	__finally
	{
		if(LastCellIndex != HCELL_NIL)
			HvReleaseCell(Hive,LastCellIndex);

		if(List)
			HvReleaseCell(Hive,ChildList->List);
	}

	return Ret;
}

//
// find value by name
//
HCELL_INDEX CmpFindValueByName(__in PHHIVE Hive,__in PVOID Parent,__in PUNICODE_STRING SearchName)
{
	PCM_KEY_NODE KeyNode								= static_cast<PCM_KEY_NODE>(Parent);
	HCELL_INDEX CellIndex								= HCELL_NIL;
	if(CmpFindNameInList(Hive,&KeyNode->ValueList,SearchName,0,&CellIndex))
		return CellIndex;

	return HCELL_NIL;
}

//
// compare key name
//
LONG CmpDoCompareKeyName(__in PHHIVE Hive,__in PCUNICODE_STRING SearchName,__in HCELL_INDEX CellIndex)
{
	PCM_KEY_NODE KeyNode								= static_cast<PCM_KEY_NODE>(HvGetCell(Hive,CellIndex));
	if(!KeyNode)
		return 2;

	LONG Result											= 0;
	if(KeyNode->Flags & KEY_COMP_NAME)
	{
		Result											= CmpCompareCompressedName(SearchName,KeyNode->Name,KeyNode->NameLength,0);
	}
	else
	{
		UNICODE_STRING KeyName;
		KeyName.Buffer									= KeyNode->Name;
		KeyName.Length									= KeyNode->NameLength;
		KeyName.MaximumLength							= KeyName.Length;
		Result											= RtlCompareUnicodeString(SearchName,&KeyName,TRUE);
	}

	HvReleaseCell(Hive,CellIndex);

	if(!Result)
		return 0;

	if(Result > 0)
		return 1;

	return -1;
}

//
// compute hash key
//
ULONG CmpComputeHashKey(__in PCUNICODE_STRING Name)
{
	ULONG Hash											= 0;
	for(USHORT i = 0; i < Name->Length; i += sizeof(WCHAR))
		Hash											= (Hash * 0x25) + RtlUpcaseUnicodeChar(Name->Buffer[i / sizeof(WCHAR)]);

	return Hash;
}

//
// find sub key by hash
//
HCELL_INDEX CmpFindSubKeyByHash(__in PHHIVE Hive,__in PCM_KEY_FAST_INDEX KeyIndex,__in PCUNICODE_STRING SearchName)
{
	if(KeyIndex->Signature != CM_KEY_INDEX_HASH)
		return HCELL_NIL;

	ULONG HashValue										= CmpComputeHashKey(SearchName);
	for(ULONG i = 0; i < KeyIndex->Count; i ++)
	{
		if(KeyIndex->List[i].NameHash == HashValue && CmpDoCompareKeyName(Hive,SearchName,KeyIndex->List[i].Cell) == 0)
			return KeyIndex->List[i].Cell;
	}

	return HCELL_NIL;
}

//
// compare name in index
//
LONG CmpCompareInIndex(__in PHHIVE Hive,__in PCUNICODE_STRING SearchName,__in ULONG Count,__in PCM_KEY_INDEX Index,__in PHCELL_INDEX Child)
{
	LONG Result											= 0;
	*Child												= HCELL_NIL;

	if(Index->Signature == CM_KEY_FAST_LEAF)
	{
		PCM_KEY_FAST_INDEX FastIndex					= reinterpret_cast<PCM_KEY_FAST_INDEX>(Index);
		PCM_INDEX Hint									= FastIndex->List + Count;

		//
		// compute the number of valid characters in the hint to compare.
		//
		ULONG HintLength								= 4;
		for(ULONG i = 0; i < 4; i ++)
		{
			if(!Hint->NameHint[i])
			{
				HintLength								= i;
				break;
			}
		}

		ULONG NameLength								= SearchName->Length / sizeof(WCHAR);
		ULONG ValidChars								= NameLength < HintLength ? NameLength : HintLength;

		for(ULONG i = 0; i < ValidChars; i ++)
		{
			WCHAR c1									= SearchName->Buffer[i];
			WCHAR c2									= FastIndex->List[Count].NameHint[i];
			Result										= static_cast<LONG>(RtlUpcaseUnicodeChar(c1)) - static_cast<LONG>(RtlUpcaseUnicodeChar(c2));
			if(Result)
				return Result;
		}

		//
		// we have compared all the available characters without a discrepancy.
		// go ahead and do the actual comparison now.
		//
		Result											= CmpDoCompareKeyName(Hive,SearchName,FastIndex->List[Count].Cell);
		if(Result == 0)
			*Child										= Hint->Cell;
	}
	else if(Index->Signature == CM_KEY_INDEX_HASH)
	{
		PCM_KEY_FAST_INDEX FastIndex					= reinterpret_cast<PCM_KEY_FAST_INDEX>(Index);
		Result											= CmpDoCompareKeyName(Hive,SearchName,FastIndex->List[Count].Cell);
		if(Result == 0)
			*Child										= FastIndex->List[Count].Cell;
	}
	else
	{
		//
		// this is just a normal old slow index.
		//
		Result											= CmpDoCompareKeyName(Hive,SearchName,Index->List[Count]);
		if(Result == 0)
			*Child										= Index->List[Count];
	}

	return Result;
}

//
// find key in leaf
//
ULONG CmpFindSubKeyInLeaf(__in PHHIVE Hive,__in PCM_KEY_INDEX Index,__in PCUNICODE_STRING SearchName,__out PHCELL_INDEX Child)
{
	ULONG High											= Index->Count - 1;
	ULONG Low											= 0;
	ULONG CanCount										= (Index->Count - 1) / 2;
	LONG Result											= 0;
	*Child												= HCELL_NIL;

	if(!Index->Count)
		return 0;

	while(TRUE)
	{
		//
		// compute where to look next, get correct pointer, do compare
		//
		Result											= CmpCompareInIndex(Hive,SearchName,CanCount,Index,Child);

		//
		// SearchName == KeyName
		//
		if(Result == 0)
			return CanCount;

		//
		// error
		//
		if(Result == 2)
			return 0x80000000;

		if(Result < 0)
			High										= CanCount;
		else
			Low											= CanCount;

		if(High - Low <= 1)
			break;

		CanCount										= (High-Low) / 2 + Low;
	}

	//
	// if we get here, High - Low = 1 or High == Low,simply look first at Low, then at High
	//
	Result												= CmpCompareInIndex(Hive,SearchName,Low,Index,Child);

	//
	// found it
	//
	if(Result == 0)
		return Low;

	//
	// error
	//
	if(Result == 2)
		return 0x80000000;

	//
	// does not exist, under
	//
	if(Result < 0)
		return Low;

	//
	// see if High matches, we will return High as the closest key regardless.
	//
	Result												= CmpCompareInIndex(Hive,SearchName,High,Index,Child);

	return High;
}

//
// find the leaf index that would contain a key
//
ULONG CmpFindSubKeyInRoot(__in PHHIVE Hive,__in PCM_KEY_INDEX Index,__in PCUNICODE_STRING SearchName,__in PHCELL_INDEX Child)
{
	HCELL_INDEX LeafCell								= HCELL_NIL;
	PCM_KEY_INDEX Leaf									= 0;
	LONG Result											= 0;
	ULONG High											= Index->Count - 1;
	ULONG Low											= 0;
	*Child												= HCELL_NIL;
	ULONG Ret											= 0;

	__try
	{
		while(TRUE)
		{
			//
			// compute where to look next, get correct pointer, do compare
			//
			ULONG CanCount								= (High - Low) / 2 + Low;
			LeafCell									= Index->List[CanCount];
			Leaf										= static_cast<PCM_KEY_INDEX>(HvGetCell(Hive,LeafCell));
			if(!Leaf)
				try_leave(LeafCell = HCELL_NIL;Ret = 0x80000000);

			Result										= CmpCompareInIndex(Hive,SearchName,Leaf->Count - 1,Leaf,Child);

			//
			// searchName == KeyName of last key in leaf, so this is our leaf
			//
			if(Result == 0)
				try_leave(*Child = LeafCell;Ret = CanCount);

			//
			// error
			//
			if(Result == 2)
				try_leave(Ret = 0x80000000);

			if(Result < 0)
			{
				//
				// SearchName < KeyName, so this may still be our leaf
				//
				Result									= CmpCompareInIndex(Hive,SearchName,0,Leaf,Child);

				//
				// error
				//
				if(Result == 2)
					try_leave(Ret = 0x80000000);

				//
				// we know from above that SearchName is less than last key in leaf.
				// since it is also >= first key in leaf, it must reside in leaf somewhere, and we are done
				//
				if(Result >= 0)
					try_leave(*Child = LeafCell;Ret = CanCount);

				High									= CanCount;
			}
			else
			{
				//
				// SearchName > KeyName
				//
				Low										= CanCount;
			}

			HvReleaseCell(Hive,LeafCell);
			LeafCell									= HCELL_NIL;

			if(High - Low <= 1)
				break;
		}

		//
		// if we get here, High - Low = 1 or High == Low
		//
		LeafCell										= Index->List[Low];
		Leaf											= static_cast<PCM_KEY_INDEX>(HvGetCell(Hive,LeafCell));
		if(!Leaf)
			try_leave(LeafCell = HCELL_NIL;Ret = 0x80000000);

		Result											= CmpCompareInIndex(Hive,SearchName,Leaf->Count - 1,Leaf,Child);

		//
		// found it
		//
		if(Result == 0)
			try_leave(*Child = LeafCell;Ret = Low);

		//
		// error
		//
		if(Result == 2)
			try_leave(Ret = 0x80000000);

		if(Result < 0)
		{
			//
			// SearchName < KeyName, so this may still be our leaf
			//
			Result										= CmpCompareInIndex(Hive,SearchName,0,Leaf,Child);

			//
			// error
			//
			if(Result == 2)
				try_leave(Ret = 0x80000000);

			//
			// we know from above that SearchName is less than last key in leaf.
			// since it is also >= first key in leaf, it must reside in leaf somewhere, and we are done
			//
			if(Result >= 0)
				try_leave(*Child = LeafCell;Ret = Low);

			//
			// does not exist, but belongs in Low or Leaf below low
			//
			try_leave(Ret = Low);
		}

		//
		// see if High matches
		//
		HvReleaseCell(Hive,LeafCell);
		LeafCell										= Index->List[High];
		Leaf											= static_cast<PCM_KEY_INDEX>(HvGetCell(Hive,LeafCell));
		if(!Leaf)
			try_leave(LeafCell = HCELL_NIL;Ret = 0x80000000);

		Result											= CmpCompareInIndex(Hive,SearchName,Leaf->Count - 1,Leaf,Child);

		//
		// found it
		//
		if(Result == 0)
			try_leave(*Child = LeafCell;Ret = High);

		//
		// error
		//
		if(Result == 2)
			try_leave(Ret = 0x80000000);

		//
		// clearly greater than low, or we wouldn't be here.
		// so regardless of whether it's below the start of this leaf, it would be in this leaf if it were where,
		// so report this leaf.
		//
		if(Result < 0)
			try_leave(*Child = LeafCell;Ret = High);

		//
		// off the high end
		//
		Ret												= High;
	}
	__finally
	{
		if(LeafCell != HCELL_NIL)
			HvReleaseCell(Hive,LeafCell);
	}

	return Ret;
}

//
// find key by name
//
HCELL_INDEX CmpFindSubKeyByName(__in PHHIVE Hive,__in PVOID Parent,__in PUNICODE_STRING SearchName)
{
	PCM_KEY_NODE ParentKeyNode							= static_cast<PCM_KEY_NODE>(Parent);
	HCELL_INDEX ChildCell								= HCELL_NIL;
	HCELL_INDEX CellToRelease							= HCELL_NIL;
	for(ULONG i = 0; i < Hive->StorageTypeCount; i ++)
	{
		if(ParentKeyNode->SubKeyCounts[i])
		{
			PCM_KEY_INDEX KeyIndex						= static_cast<PCM_KEY_INDEX>(HvGetCell(Hive,ParentKeyNode->SubKeyLists[i]));
			if(!KeyIndex)
				break;

			CellToRelease								= ParentKeyNode->SubKeyLists[i];
			if(KeyIndex->Signature == CM_KEY_INDEX_ROOT)
			{
				LONG RetValue							= CmpFindSubKeyInRoot(Hive,KeyIndex,SearchName,&ChildCell);
				HvReleaseCell(Hive,CellToRelease);

				if(RetValue < 0)
					break;

				if(ChildCell != HCELL_NIL)
				{
					KeyIndex							= static_cast<PCM_KEY_INDEX>(HvGetCell(Hive,ChildCell));
					if(!KeyIndex)
					{
						ChildCell						= HCELL_NIL;
						break;
					}

					CellToRelease						= ChildCell;
				}
				else
				{
					continue;
				}
			}

			LONG RetValue								= 0;
			if(KeyIndex->Signature == CM_KEY_INDEX_HASH)
			{
				ChildCell								= CmpFindSubKeyByHash(Hive,reinterpret_cast<PCM_KEY_FAST_INDEX>(KeyIndex),SearchName);
			}
			else if(KeyIndex->Signature == CM_KEY_INDEX_LEAF || KeyIndex->Signature == CM_KEY_FAST_LEAF)
			{
				RetValue								= CmpFindSubKeyInLeaf(Hive,KeyIndex,SearchName,&ChildCell);
			}

			HvReleaseCell(Hive,CellToRelease);

			if(RetValue < 0)
				break;

			if(ChildCell != HCELL_NIL)
				break;
		}
	}

	return ChildCell;
}

//
// find key by number
//
HCELL_INDEX CmpDoFindSubKeyByNumber(__in PHHIVE Hive,__in PCM_KEY_INDEX KeyIndex,__in ULONG SearchIndex)
{
	PCM_KEY_INDEX LeafIndex								= KeyIndex;
	HCELL_INDEX LastCellIndex							= HCELL_NIL;
	if(KeyIndex->Signature == CM_KEY_INDEX_ROOT)
	{
		//
		// find leaf first
		//
		ULONG i;
		for(i = 0; i < KeyIndex->Count; i ++)
		{
			if(i)
				HvReleaseCell(Hive,LastCellIndex);

			LastCellIndex								= HCELL_NIL;
			LeafIndex									= static_cast<PCM_KEY_INDEX>(HvGetCell(Hive,KeyIndex->List[i]));
			if(!LeafIndex)
				return HCELL_NIL;

			LastCellIndex								= KeyIndex->List[i];
			if(SearchIndex < LeafIndex->Count)
				break;

			SearchIndex									-= KeyIndex->Count;
		}

		if(i == KeyIndex->Count)
			LeafIndex									= 0;
	}

	if(LastCellIndex != HCELL_NIL)
		HvReleaseCell(Hive,LastCellIndex);

	if(!LeafIndex)
		return HCELL_NIL;

	if(LeafIndex->Signature == CM_KEY_FAST_LEAF || LeafIndex->Signature == CM_KEY_INDEX_HASH)
		return reinterpret_cast<PCM_KEY_FAST_INDEX>(LeafIndex)->List[SearchIndex].Cell;

	return LeafIndex->List[SearchIndex];
}

//
// find key by number
//
HCELL_INDEX CmpFindSubKeyByNumber(__in PHHIVE Hive,__in PVOID Parent,__in ULONG SearchIndex)
{
	PCM_KEY_NODE ParentKeyNode							= static_cast<PCM_KEY_NODE>(Parent);
	if(SearchIndex < ParentKeyNode->SubKeyCounts[0])
	{
		//
		// in the stable list
		//
		PCM_KEY_INDEX KeyIndex							= static_cast<PCM_KEY_INDEX>(HvGetCell(Hive,ParentKeyNode->SubKeyLists[0]));
		if(!KeyIndex)
			return HCELL_NIL;

		HCELL_INDEX Found								= CmpDoFindSubKeyByNumber(Hive,KeyIndex,SearchIndex);
		HvReleaseCell(Hive,ParentKeyNode->SubKeyLists[SearchIndex]);
		return Found;
	}

	if(Hive->StorageTypeCount > 1)
	{
		//
		// in the volatile list
		//
		SearchIndex										-= ParentKeyNode->SubKeyCounts[0];
		if(SearchIndex < ParentKeyNode->SubKeyCounts[1])
		{
			PCM_KEY_INDEX KeyIndex						= static_cast<PCM_KEY_INDEX>(HvGetCell(Hive,ParentKeyNode->SubKeyLists[1]));
			if(!KeyIndex)
				return HCELL_NIL;

			HCELL_INDEX Found								= CmpDoFindSubKeyByNumber(Hive,KeyIndex,SearchIndex);
			HvReleaseCell(Hive,ParentKeyNode->SubKeyLists[SearchIndex]);
			return Found;
		}
	}

	return HCELL_NIL;
}

//
// get compressed name size
//
ULONG CmpCompressedNameSize(__in PWCHAR Name,__in ULONG Length)
{
	return Length * sizeof(WCHAR);
}

//
// copy compressed name
//
VOID CmpCopyCompressedName(__in PWCHAR Destination,__in ULONG DestinationLength,__in PWCHAR Source,__in ULONG SourceLength)
{
	ULONG Chars											= DestinationLength / sizeof(WCHAR) < SourceLength ? DestinationLength / sizeof(WCHAR) : SourceLength;

	for(ULONG i = 0; i < Chars; i ++)
		Destination[i]									= *Add2Ptr(Source,i,PUCHAR);
}

//
// header checksum
//
ULONG HvpHeaderCheckSum(__in PHBASE_BLOCK BaseBlock)
{
	ULONG sum											= 0;
	for(ULONG i = 0; i < 127; i ++)
		sum												^= *Add2Ptr(BaseBlock,i * sizeof(ULONG),PULONG);

	if(sum == 0xffffffff)
		sum												= 0xfffffffe;

	if(sum == 0)
		sum												= 1;

	return sum;
}

//
// recover
//
BOOLEAN BlRecoverHive(__in PVOID DstBaseBlock,__in PVOID SrcBaseBlock)
{
	PHBASE_BLOCK LogBaseBlock							= static_cast<PHBASE_BLOCK>(SrcBaseBlock);
	PHBASE_BLOCK PrimaryBaseBlock						= static_cast<PHBASE_BLOCK>(DstBaseBlock);

	//
	// compute log offset
	//
	ULONG ClusterSize									= LogBaseBlock->Cluster * HSECTOR_SIZE;
	ULONG SectorRoundToCluster							= (HSECTOR_SIZE + ClusterSize - 1) & ~(ClusterSize - 1);
	ULONG FileOffset									= (ClusterSize + SectorRoundToCluster) & ~(SectorRoundToCluster - 1);

	//
	// check file header and recover it if checksum is not the same
	//
	if(HvpHeaderCheckSum(PrimaryBaseBlock) != PrimaryBaseBlock->CheckSum)
	{
		RtlCopyMemory(PrimaryBaseBlock,LogBaseBlock,ClusterSize);
		PrimaryBaseBlock->Type							= HFILE_TYPE_PRIMARY;
	}

	//
	// check signature
	//
	if(*Add2Ptr(SrcBaseBlock,FileOffset,PULONG) != HLOG_DV_SIGNATURE)
		return FALSE;

	//
	// skip signature
	//
	FileOffset											+= sizeof(ULONG);

	//
	// setup dirty vector
	//
	RTL_BITMAP DirtyVector;
	ULONG DirtyVectorSize								= LogBaseBlock->Length / HSECTOR_SIZE;
	RtlInitializeBitMap(&DirtyVector,Add2Ptr(SrcBaseBlock,FileOffset,PULONG),DirtyVectorSize);

	//
	// skip vector data
	//
	FileOffset											= FileOffset + DirtyVectorSize / 8;

	//
	// round up it
	//
	FileOffset											= (FileOffset + ClusterSize - 1) & ~(ClusterSize - 1);

	//
	// recover from dirty data
	//
	for(ULONG Current = 0; Current < DirtyVectorSize; )
	{
		//
		// find next contiguous block of entries to read in
		//
		ULONG i;
		for(i = Current; i < DirtyVectorSize; i ++)
		{
			if(RtlCheckBit(&DirtyVector,i) == 1)
				break;
		}
		ULONG Start										= i;

		for( ; i < DirtyVectorSize; i ++)
		{
			if(RtlCheckBit(&DirtyVector, i) == 0)
				break;
		}

		ULONG End										= i;
		Current											= End;
		ULONG Length									= (End - Start) * HSECTOR_SIZE;

		//
		// nothing to recover
		//
		if(!Length)
			break;

		//
		// recorve this block
		//
		PVOID DestStart									= Add2Ptr(DstBaseBlock,(Start + 8) * HSECTOR_SIZE,PVOID);
		PVOID SrcStart									= Add2Ptr(SrcBaseBlock,FileOffset,PVOID);
		FileOffset										+= Length;

		RtlCopyMemory(DestStart,SrcStart,Length);
	}

	//
	// reset seq and checksum
	//
	PrimaryBaseBlock->Sequence2							= PrimaryBaseBlock->Sequence1;
	PrimaryBaseBlock->CheckSum							= HvpHeaderCheckSum(PrimaryBaseBlock);

	return TRUE;
}

//
// release global quota
//
VOID CmpReleaseGlobalQuota(__in ULONG Size)
{
}

//
// adjust free display
//
NTSTATUS HvpAdjustHiveFreeDisplay(__in PHHIVE Hive,__in ULONG Length,__in ULONG)
{
	return STATUS_SUCCESS;
}

//
// write hive
//
BOOLEAN HvpDoWriteHive(__in PHHIVE Hive,__in ULONG Type)
{
	return TRUE;
}

//
// get mapped cell
//
PVOID HvpGetCellMapped(__in PHHIVE Hive,__in HCELL_INDEX Index)
{
	return 0;
}

//
// release mapped cell
//
VOID HvpReleaseCellMapped(__in PHHIVE Hive,__in HCELL_INDEX Index)
{
}

//
// load hive
//
NTSTATUS HvLoadHive(__in PHHIVE Hive)
{
	return STATUS_SUCCESS;
}

//
// copy last 64 bytes name to base block
//
VOID HvpFillFileName(__in PHBASE_BLOCK BaseBlock,__in PUNICODE_STRING FileName)
{
	RtlZeroMemory(BaseBlock->FileName,HBASE_NAME_ALLOC);

	if(!FileName)
		return;

	ULONG Offset										= FileName->Length - HBASE_NAME_ALLOC;
	ULONG Length										= FileName->Length;
	if(FileName->Length <= HBASE_NAME_ALLOC)
		Offset											= 0;
	else
		Length											= HBASE_NAME_ALLOC;

	RtlMoveMemory(BaseBlock->FileName,Add2Ptr(FileName->Buffer,Offset,PVOID),Length);
}

//
// get flat cell
//
PVOID HvpGetCellFlat(__in PHHIVE Hive,__in HCELL_INDEX CellIndex)
{
	//
	// address is base of Hive image + Cell
	//
	PHCELL Cell											= Add2Ptr(Hive->BaseBlock,HBLOCK_SIZE + CellIndex,PHCELL);
	if(USE_OLD_CELL(Hive))
		return &(Cell->u.OldCell.u.UserData);

	return &(Cell->u.NewCell.u.UserData);
}

//
// get paged cell
//
PVOID HvpGetCellPaged(__in PHHIVE Hive,__in HCELL_INDEX CellIndex)
{
	ULONG Type											= HvGetCellType(CellIndex);
	ULONG Table											= (CellIndex & HCELL_TABLE_MASK) >> HCELL_TABLE_SHIFT;
	ULONG Block											= (CellIndex & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;
	ULONG Offset										= (CellIndex & HCELL_OFFSET_MASK);
	PHMAP_ENTRY Map										= Hive->Storage[Type].Map->Directory[Table]->Table + Block;
	PHCELL Cell											= Add2Ptr(Map->BlockAddress,Offset,PHCELL);

	if(USE_OLD_CELL(Hive))
		return &(Cell->u.OldCell.u.UserData);

	return &(Cell->u.NewCell.u.UserData);
}

//
// allocate
//
BOOLEAN HvpAllocateMap(__in PHHIVE Hive,__in PHMAP_DIRECTORY Dir,__in ULONG Start,__in ULONG End)
{
	for(ULONG i = Start; i <= End; i++)
	{
		PHMAP_TABLE t								= static_cast<PHMAP_TABLE>(Hive->Allocate(sizeof(HMAP_TABLE),FALSE,'92MC'));
		if(!t)
			return FALSE;

		RtlZeroMemory(t,sizeof(HMAP_TABLE));
		Dir->Directory[i]							= t;
	}

	return TRUE;
}

//
// free map
//
VOID HvpFreeMap(__in PHHIVE Hive,__in PHMAP_DIRECTORY Dir,__in ULONG Start,__in ULONG End)
{
	if(End >= HDIRECTORY_SLOTS)
		End												= HDIRECTORY_SLOTS - 1;

	for(ULONG i = Start; i <= End; i++)
	{
		if(Dir->Directory[i])
		{
			Hive->Free(Dir->Directory[i],sizeof(HMAP_TABLE));
			Dir->Directory[i]							= 0;
		}
	}
}

//
// get cell map
//
PHMAP_ENTRY HvpGetCellMap(__in PHHIVE Hive,__in HCELL_INDEX Cell)
{
	ULONG Type											= HvGetCellType(Cell);
	ULONG Table											= (Cell & HCELL_TABLE_MASK) >> HCELL_TABLE_SHIFT;
	ULONG Block											= (Cell & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;

	if(Cell - Type * HCELL_TYPE_MASK >= Hive->Storage[Type].Length)
		return 0;

	return Hive->Storage[Type].Map->Directory[Table]->Table + Block;
}

//
// get bin men alloc size
//
ULONG HvpGetBinMemAlloc(__in PHHIVE Hive,__in PHBIN Bin,__in ULONG Type)
{
	HCELL_INDEX Cell									= Type * HCELL_TYPE_MASK + Bin->FileOffset;
	PHMAP_ENTRY Me										= HvpGetCellMap(Hive,Cell);
	return Me->MemAlloc;
}

//
// mark dirty
//
BOOLEAN HvMarkDirty(__in PHHIVE Hive,__in HCELL_INDEX Start,__in ULONG Length,__in ULONG)
{
	return TRUE;
}

//
// mark cell dirty
//
BOOLEAN HvMarkCellDirty(__in PHHIVE Hive,__in HCELL_INDEX Cell)
{
	return TRUE;
}

//
// get hcell
//
PHCELL HvpGetHCell(__in PHHIVE Hive,__in HCELL_INDEX CellIndex)
{
	PVOID Cell											= HvGetCell(Hive,CellIndex);
	if(!Cell)
		return 0;

	if(USE_OLD_CELL(Hive))
		return CONTAINING_RECORD(Cell,HCELL,u.OldCell.u.UserData);

	return CONTAINING_RECORD(Cell,HCELL,u.NewCell.u.UserData);
}

//
// free cell
//
VOID HvpDelistFreeCell(__in PHHIVE Hive,__in PHCELL Pcell,__in ULONG Type)
{
}

//
// puts the newly freed cell on the appropriate list.
//
VOID HvpEnlistFreeCell(__in PHHIVE Hive,__in HCELL_INDEX Cell,__in ULONG Size,__in ULONG Type,__in BOOLEAN CoalesceForward)
{
	ULONG Index											= (Size >> 3) - 1;

	if(Index >= 16)
	{
		if(Index > 255)
			Index										= HHIVE_FREE_DISPLAY_SIZE - 1;
		else
			Index										= KiFindFirstSetLeft[Index] + 7;
	}

	PHCELL pcell										= HvpGetHCell(Hive,Cell);
	HvReleaseCell(Hive,Cell);

	//
	// check to see if this is the first cell in the bin and if the entire bin consists just of this cell.
	//
	PHMAP_ENTRY Map										= HvpGetCellMap(Hive,Cell);
	PHBIN Bin											= reinterpret_cast<PHBIN>(Map->BinAddress & 0xfffffff0);
	PHCELL FirstCell									= 0;

	if(pcell == reinterpret_cast<PHCELL>(Bin + 1) && Size == Bin->Size - sizeof(HBIN))
	{
		//
		// we have a bin that is entirely free.  But we cannot do anything with it unless the memalloc that contains the bin is entirely free.
		// walk the bins backwards until we find the first one in the alloc, then walk forwards until we find the last one.
		// if any of the other bins in the memalloc are not free, bail out.
		//
		PHBIN FirstBin									= Bin;
		while(HvpGetBinMemAlloc(Hive,FirstBin,Type))
		{
			Map											= HvpGetCellMap(Hive,FirstBin->FileOffset - HBLOCK_SIZE + Type * HCELL_TYPE_MASK);
			FirstBin									= reinterpret_cast<PHBIN>(Map->BinAddress & 0xfffffff0);
			FirstCell									= reinterpret_cast<PHCELL>(FirstBin + 1);

			if(FirstCell->Size != FirstBin->Size - sizeof(HBIN))
			{
				//
				// the first cell in the bin is either allocated, or not the only cell in the HBIN.we cannot free any HBINs.
				//
				return;
			}
		}

		//
		// we can never discard the first bin of a hive as that always gets marked dirty and written out.
		//
		if(FirstBin->FileOffset == 0)
			return;

		PHBIN LastBin									= Bin;

		while(LastBin->FileOffset+LastBin->Size < FirstBin->FileOffset + HvpGetBinMemAlloc(Hive,FirstBin,Type))
		{
			if(!CoalesceForward)
			{
				//
				// we are at the end of what's been built up. just return and this will get freed up when the next HBIN is added.
				//
				return;
			}

			Map											= HvpGetCellMap(Hive,LastBin->FileOffset + LastBin->Size + Type * HCELL_TYPE_MASK);
			LastBin										= reinterpret_cast<PHBIN>(Map->BinAddress & 0xfffffff0);
			FirstCell									= reinterpret_cast<PHCELL>(LastBin + 1);

			if(FirstCell->Size != LastBin->Size - sizeof(HBIN))
			{
				//
				// the first cell in the bin is either allocated, or not the only cell in the HBIN.we cannot free any HBINs.
				//
				return;
			}
		}

		//
		// all the bins in this alloc are freed.
		// coalesce all the bins into one alloc-sized bin, then either discard the bin or mark it as discardable.
		//
		if(FirstBin->Size != HvpGetBinMemAlloc(Hive,FirstBin,Type))
		{
			//
			// mark the first HBLOCK of the first HBIN dirty, since we will need to update the on disk field for the bin size
			//
			if(!HvMarkDirty(Hive,FirstBin->FileOffset + Type * HCELL_TYPE_MASK,sizeof(HBIN) + sizeof(HCELL),0))
				return;
		}

		PFREE_HBIN FreeBin								= static_cast<PFREE_HBIN>(Hive->Allocate(sizeof(FREE_HBIN),FALSE,' 7MC'));
		if(!FreeBin)
			return;

		//
		// walk through the bins and delist each free cell
		//
		Bin												= FirstBin;
		while(1)
		{
			FirstCell									= reinterpret_cast<PHCELL>(Bin + 1);
			HvpDelistFreeCell(Hive,FirstCell,Type);

			if(Bin == LastBin)
				break;

			Map											= HvpGetCellMap(Hive,Bin->FileOffset + Bin->Size + Type * HCELL_TYPE_MASK);
			Bin											= reinterpret_cast<PHBIN>(Map->BinAddress & 0xfffffff0);
		}

		//
		// coalesce them all into one bin.
		//
		FirstBin->Size									= FirstBin->MemAlloc;
		FreeBin->Size									= FirstBin->Size;
		FreeBin->FileOffset								= FirstBin->FileOffset;
		FirstCell										= reinterpret_cast<PHCELL>(FirstBin + 1);
		FirstCell->Size									= FirstBin->Size - sizeof(HBIN);
		if(USE_OLD_CELL(Hive))
			FirstCell->u.OldCell.Last					= 0xffffffff;

		InsertHeadList(&Hive->Storage[Type].FreeBins,&FreeBin->ListEntry);

		HCELL_INDEX FreeCell							= FirstBin->FileOffset + Type * HCELL_TYPE_MASK;
		FreeBin->Flags									= 1;

		while(FreeCell - FirstBin->FileOffset < FirstBin->Size)
		{
			Map											= HvpGetCellMap(Hive,FreeCell);

			if(Map->BinAddress & 1)
				Map->BinAddress							= reinterpret_cast<ULONG>(FirstBin) | 3;
			else
				Map->BinAddress							= reinterpret_cast<ULONG>(FirstBin) | 2;

			Map->BlockAddress							= reinterpret_cast<ULONG>(FreeBin);

			FreeCell									+= HBLOCK_SIZE;
		}
	}
}

//
// scan through the cells in the bin, locating the free ones. enlist them in the hive's free list set.
//
BOOLEAN HvpEnlistFreeCells(__in PHHIVE Hive,__in PHBIN Bin,__in ULONG BinOffset)
{
	//
	// Scan all the cells in the bin, total free and allocated, check
	// for impossible pointers.
	//
	ULONG celloffset									= sizeof(HBIN);
	PHCELL p											= Add2Ptr(Bin,sizeof(HBIN),PHCELL);
	ULONG size											= 0;
	BOOLEAN Ret											= TRUE;
	while(p < Add2Ptr(Bin,Bin->Size,PHCELL))
	{
		//
		// if free cell, check it out, add it to free list for hive
		//
		if(p->Size >= 0)
		{
			size										= static_cast<ULONG>(p->Size);
			if(size > Bin->Size || Add2Ptr(p,size,PHCELL) > Add2Ptr(Bin,Bin->Size,PHCELL) || size % HCELL_PAD(Hive) || !size)
			{
				Ret										= FALSE;

				if(CM_TRY_TO_FIX_ERROR)
				{
					p->Size								= static_cast<LONG>(Add2Ptr(Bin,Bin->Size,PUCHAR) - reinterpret_cast<PUCHAR>(p));
					RtlZeroMemory(&p->u,p->Size - sizeof(p->Size));
					size								= p->Size;
					Hive->BaseBlock->OffsetFF8			|= 4;
				}
				else
				{
					break;
				}
			}

			//
			// cell is free, and is not obviously corrupt, add to free list
			//
			celloffset									= reinterpret_cast<PUCHAR>(p) - reinterpret_cast<PUCHAR>(Bin);
			HCELL_INDEX cellindex						= BinOffset + celloffset;

			//
			// enlist this free cell, but do not coalesce with the next free cell as we haven't gotten that far yet.
			//
			HvpEnlistFreeCell(Hive,cellindex,size,0,FALSE);
		}
		else
		{
			size										= static_cast<ULONG>(p->Size * -1);
			if(size > Bin->Size || Add2Ptr(p,size,PHCELL) > Add2Ptr(Bin,Bin->Size,PHCELL) || size % HCELL_PAD(Hive) || !size)
			{
				Ret										= FALSE;

				if(CM_TRY_TO_FIX_ERROR)
				{
					p->Size								= static_cast<LONG>(Add2Ptr(Bin,Bin->Size,PUCHAR) - reinterpret_cast<PUCHAR>(p));
					RtlZeroMemory(&p->u,p->Size - sizeof(p->Size));
					size								= p->Size;
					Hive->BaseBlock->OffsetFF8			|= 4;
				}
				else
				{
					break;
				}

				celloffset								= reinterpret_cast<PUCHAR>(p) - reinterpret_cast<PUCHAR>(Bin);
				HCELL_INDEX cellindex					= BinOffset + celloffset;

				//
				// enlist this free cell, but do not coalesce with the next free cell as we haven't gotten that far yet.
				//
				HvpEnlistFreeCell(Hive,cellindex,size,0,FALSE);
			}
		}

		p												= Add2Ptr(p,size,PHCELL);
	}

	return Ret;
}

//
// creates map entries and enlist free cells for the specified bin
//
NTSTATUS HvpEnlistBinInMap(__in PHHIVE Hive,__in ULONG Length,__in PHBIN Bin,__in ULONG Offset,__in_opt PHCELL_INDEX TailDisplay)
{
	NTSTATUS Status										= STATUS_SUCCESS;

	//
	// memory was allocated for this bin
	//
	Bin->MemAlloc										= Bin->Size;

	//
	// create map entries for each block/page in bin
	//
	ULONG BinOffset										= Offset;
	for(ULONG Address = reinterpret_cast<ULONG>(Bin); Address < reinterpret_cast<ULONG>(Bin) + Bin->Size; Address += HBLOCK_SIZE)
	{
		PHMAP_ENTRY Me									= HvpGetCellMap(Hive,Offset);
		Me->BlockAddress								= Address;
		Me->BinAddress									= reinterpret_cast<ULONG>(Bin);
		if(Offset == BinOffset)
		{
			Me->BinAddress								|= 1;
			Me->MemAlloc								= Bin->Size;
		}
		else
		{
			Me->MemAlloc								= 0;
		}

		if(TailDisplay)
		{
			Me->BinAddress								|= 4;
		}
		else
		{
			Me->BinAddress								|= 8;
			Me->Offset8									= 0;
		}

		Offset											+= HBLOCK_SIZE;
	}

	if(!Hive->ReadOnly)
	{
		//
		// add free cells in the bin to the appropriate free lists
		//
		if(!HvpEnlistFreeCells(Hive,Bin,BinOffset))
		{
			HvCheckHiveDebug.Hive						= Hive;
			HvCheckHiveDebug.Status						= 0xA002;
			HvCheckHiveDebug.Space						= Length;
			HvCheckHiveDebug.MapPoint					= BinOffset;
			HvCheckHiveDebug.BinPoint					= Bin;

			if(CM_TRY_TO_FIX_ERROR)
				Status									= STATUS_REGISTRY_RECOVERED;
			else
				Status									= STATUS_REGISTRY_CORRUPT;
		}
	}

	return Status;
}

//
// init hive map
//
NTSTATUS HvpInitMap(__in PHHIVE  Hive)
{
	//
	// compute size of data region to be mapped
	//
	PHBASE_BLOCK BaseBlock								= Hive->BaseBlock;
	ULONG Length										= BaseBlock->Length;
	if(Length % HBLOCK_SIZE)
		return STATUS_REGISTRY_CORRUPT;

	ULONG MapSlots										= Length / HBLOCK_SIZE;
	ULONG Tables										= MapSlots > 0 ? (MapSlots - 1) / HTABLE_SLOTS : 0;
	Hive->Storage[0].Length								= Length;

	//
	// allocate dirty vector if one is not already present (from HvpRecoverData)
	//
	if(!Hive->DirtyVector.Buffer)
	{
		ULONG Size										= Length / HSECTOR_SIZE / 8;
		PULONG Vector									= static_cast<PULONG>(Hive->Allocate((Size + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1),TRUE,'72MC'));
		if(!Vector)
			return STATUS_NO_MEMORY;

		RtlZeroMemory(Vector,Size);
		RtlInitializeBitMap(&Hive->DirtyVector,Vector,Size * 8);
		Hive->DirtyAlloc								= Size;
	}

	//
	// allocate and build structure for map
	//
	if(Tables == 0)
	{
		//
		// just 1 table, no need for directory
		//
		PHMAP_TABLE t									= static_cast<PHMAP_TABLE>(Hive->Allocate(sizeof(HMAP_TABLE),FALSE,'62MC'));
		if(!t)
			return STATUS_INSUFFICIENT_RESOURCES;

		RtlZeroMemory(t,sizeof(HMAP_TABLE));
		Hive->Storage[0].Map							= reinterpret_cast<PHMAP_DIRECTORY>(&Hive->Storage[0].SmallDir);
		Hive->Storage[0].SmallDir						= t;

		return STATUS_SUCCESS;
	}

	//
	// need directory and multiple tables
	//
	PHMAP_DIRECTORY d									= static_cast<PHMAP_DIRECTORY>(Hive->Allocate(sizeof(HMAP_DIRECTORY),FALSE,'82MC'));
	if(!d)
		return STATUS_INSUFFICIENT_RESOURCES;

	RtlZeroMemory(d,sizeof(HMAP_DIRECTORY));

	//
	// allocate tables and fill in dir
	//
	if(HvpAllocateMap(Hive,d,0,Tables))
	{
		Hive->Storage[0].Map							= d;
		Hive->Storage[0].SmallDir						= 0;

		return STATUS_SUCCESS;
	}

	//
	// directory was built and allocated, so clean it up
	//
	HvpFreeMap(Hive,d,0,Tables);
	Hive->Free(d,sizeof(HMAP_DIRECTORY));

	return STATUS_INSUFFICIENT_RESOURCES;
}

//
// build map
//
NTSTATUS HvpBuildMap(__in PHHIVE Hive,__in PVOID Image)
{
	//
	// init the map
	//
	NTSTATUS Status										= HvpInitMap(Hive);

	//
	// just return failure; HvpInitMap took care of cleanup
	//
	if(!NT_SUCCESS(Status))
		return Status;

	//
	// Fill in the map
	//
	ULONG Offset										= 0;
	PHBIN Bin											= static_cast<PHBIN>(Image);
	ULONG Length										= Hive->Storage[0].Length;

	while(Bin < Add2Ptr(Image,Length,PHBIN))
	{
		//
		// check the validity of the bin header
		//
		if(Bin->Size > Length || Bin->Size < HBLOCK_SIZE || Bin->Signature != HBIN_SIGNATURE || Bin->FileOffset != Offset)
		{
			//
			// bin is bogus
			//
			HvCheckHiveDebug.Hive						= Hive;
			HvCheckHiveDebug.Status						= 0xA001;
			HvCheckHiveDebug.Space						= Length;
			HvCheckHiveDebug.MapPoint					= Offset;
			HvCheckHiveDebug.BinPoint					= Bin;
			if(!CM_TRY_TO_FIX_ERROR)
			{
				Status									= STATUS_REGISTRY_CORRUPT;
				break;
			}

			Bin->Signature								= HBIN_SIGNATURE;
			Bin->FileOffset								= Offset;
			if(Bin->Size + Offset > Length || Bin->Size < HBLOCK_SIZE || (Bin->Size & (HBLOCK_SIZE - 1)))
				Bin->Size								= HBLOCK_SIZE;

			Hive->BaseBlock->OffsetFF8					|= 4;
		}

		//
		// enlist this bin
		//
		Status = HvpEnlistBinInMap(Hive,Length,Bin,Offset,0);
		if(CM_TRY_TO_FIX_ERROR && Status == STATUS_REGISTRY_RECOVERED)
		{
			Hive->BaseBlock->OffsetFF8					|= 4;
			Status										= STATUS_SUCCESS;
		}

		if(!NT_SUCCESS(Status))
			break;

		//
		// the next bin
		//
		Offset											+= Bin->Size;
		Bin												= Add2Ptr(Bin,Bin->Size,PHBIN);
	}

	return Status;
}

//
// build map
//
NTSTATUS HvpBuildMapAndCopy(__in PHHIVE Hive,__in PVOID Image)
{
	//
	// compute size of data region to be mapped
	//
	PHMAP_DIRECTORY d									= 0;
	PHBASE_BLOCK BaseBlock								= Hive->BaseBlock;
	ULONG Length										= BaseBlock->Length;
	if(Length % HBLOCK_SIZE)
		return STATUS_REGISTRY_CORRUPT;

	ULONG MapSlots										= Length / HBLOCK_SIZE;
	ULONG Tables										= MapSlots > 0 ? (MapSlots - 1) / HTABLE_SLOTS : 0;
	Hive->Storage[0].Length								= Length;

	//
	// allocate dirty vector if one is not already present (from HvpRecoverData)
	//
	if(!Hive->DirtyVector.Buffer)
	{
		ULONG Size										= Length / HSECTOR_SIZE / 8;
		PULONG Vector									= static_cast<PULONG>(Hive->Allocate((Size + sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1),TRUE,'22MC'));
		if(!Vector)
			return STATUS_NO_MEMORY;

		RtlZeroMemory(Vector,Size);
		RtlInitializeBitMap(&Hive->DirtyVector,Vector,Size * 8);

		Hive->DirtyAlloc								= Size;
	}

	//
	// allocate and build structure for map
	//
	if(!Tables)
	{
		//
		// just 1 table, no need for directory
		//
		PHMAP_TABLE t									= static_cast<PHMAP_TABLE>(Hive->Allocate(sizeof(HMAP_TABLE),FALSE,'32MC'));
		if(!t)
			return STATUS_INSUFFICIENT_RESOURCES;

		RtlZeroMemory(t,sizeof(HMAP_TABLE));
		Hive->Storage[0].Map							= reinterpret_cast<PHMAP_DIRECTORY>(&Hive->Storage[0].SmallDir);
		Hive->Storage[0].SmallDir						= t;
	}
	else
	{
		//
		// need directory and multiple tables
		//
		d												= static_cast<PHMAP_DIRECTORY>(Hive->Allocate(sizeof(HMAP_DIRECTORY),FALSE,'42MC'));
		if(!d)
			return STATUS_INSUFFICIENT_RESOURCES;

		RtlZeroMemory(d,sizeof(HMAP_DIRECTORY));

		//
		// allocate tables and fill in dir
		//
		if(HvpAllocateMap(Hive,d,0,Tables) == FALSE)
		{
			HvpFreeMap(Hive,d,0,Tables);
			Hive->Free(d,sizeof(HMAP_DIRECTORY));

			return STATUS_INSUFFICIENT_RESOURCES;
		}

		Hive->Storage[0].Map							= d;
		Hive->Storage[0].SmallDir						= 0;
	}

	//
	// now we have to allocate the memory for the HBINs and fill in the map appropriately.
	// we are careful never to allocate less than a page to avoid fragmenting pool.
	// as long as the page size is a multiple of HBLOCK_SIZE (a fairly good assumption as long as HBLOCK_SIZE is 4k)
	// this strategy will prevent pool fragmentation.
	//
	// if we come across an HBIN that is entirely composed of a freed HCELL,
	// then we do not allocate memory, but mark its HBLOCKs in the map as not present.
	// HvAllocateCell will allocate memory for the bin when it is needed.
	//
	ULONG Offset										= 0;
	PHBIN Bin											= static_cast<PHBIN>(Image);

	while(Bin < Add2Ptr(Image,Length,PHBIN))
	{
		if(Bin->Size > Length - Offset || Bin->Signature != HBIN_SIGNATURE || Bin->FileOffset != Offset)
		{
			if(d)
			{
				HvpFreeMap(Hive,d,0,Tables);
				Hive->Free(d,sizeof(HMAP_DIRECTORY));
			}

			return STATUS_REGISTRY_CORRUPT;
		}

		ULONG Size										= Bin->Size;

		//
		// we now have a series of HBINs to group together in one chunk of memory.
		//
		PHBIN NewBins									= static_cast<PHBIN>(Hive->Allocate(Size,FALSE,'52MC'));
		if(!NewBins)
		{
			if(d)
			{
				HvpFreeMap(Hive,d,0,Tables);
				Hive->Free(d,sizeof(HMAP_DIRECTORY));
			}

			return STATUS_INSUFFICIENT_RESOURCES;
		}

		RtlCopyMemory(NewBins,Add2Ptr(Image,Offset,PUCHAR),Size);
		NewBins->MemAlloc								= Size;

		//
		// create map entries for each block/page in bin
		//
		ULONG Address									= reinterpret_cast<ULONG>(NewBins);

		PHBIN CurrentBin								= reinterpret_cast<PHBIN>(Address);
		do
		{
			PHMAP_ENTRY Me								= HvpGetCellMap(Hive, Offset);
			Me->BlockAddress							= Address;
			Me->BinAddress								= reinterpret_cast<ULONG>(CurrentBin);

			if(CurrentBin == NewBins)
			{
				Me->BinAddress							|= 1;
				Me->MemAlloc							= CurrentBin->Size;
			}
			else
			{
				Me->MemAlloc							= 0;
			}

			Me->Offset8									= 0;
			Me->BinAddress								|= 8;

			Address										+= HBLOCK_SIZE;
			Offset										+= HBLOCK_SIZE;
		}while(Address < reinterpret_cast<ULONG>(CurrentBin) + CurrentBin->Size);

		if(Hive->ReadOnly == FALSE)
		{
			//
			// add free cells in the bin to the appropriate free lists
			//
			if(!HvpEnlistFreeCells(Hive,CurrentBin,CurrentBin->FileOffset))
			{
				if(d)
				{
					HvpFreeMap(Hive,d,0,Tables);
					Hive->Free(d,sizeof(HMAP_DIRECTORY));
				}

				return STATUS_REGISTRY_CORRUPT;
			}
		}

		Bin												= Add2Ptr(Bin,Bin->Size,PHBIN);
	}

	return STATUS_SUCCESS;
}

//
// free all bins
//
VOID HvpFreeAllocatedBins(__in PHHIVE Hive)
{
	//
	// calculate the number of tables in the map
	//
	ULONG Length										= Hive->Storage[0].Length;
	ULONG MapSlots										= Length / HBLOCK_SIZE;
	ULONG Tables										= MapSlots > 0 ? (MapSlots - 1) / HTABLE_SLOTS : 0;

	if(Hive->Storage[0].Map)
	{
		//
		// iterate through the directory
		//
		for(ULONG i = 0; i <= Tables; i ++)
		{
			PHMAP_TABLE Tab								= Hive->Storage[0].Map->Directory[i];

			//
			// iterate through the slots in the directory
			//
			for(ULONG j = 0; j < HTABLE_SLOTS; j ++)
			{
				PHMAP_ENTRY Me							= Tab->Table + j;

				//
				// BinAddress non-zero means allocated bin
				//
				if(Me->BinAddress)
				{
					if((Me->BinAddress & 1) && (Me->BinAddress & 8))
					{
						PHBIN Bin						= reinterpret_cast<PHBIN>(Me->BinAddress & 0xfffffff0);
						Hive->Free(Bin,HvpGetBinMemAlloc(Hive,Bin,0));
					}

					Me->BinAddress						= 0;
				}
			}
		}
	}
}

//
// initialize a hive
//
NTSTATUS HvInitializeHive(__in PHHIVE Hive,__in ULONG OperationType,__in ULONG HiveFlags,__in ULONG FileType,__in_opt PVOID HiveData,
						  __in PALLOCATE_ROUTINE AllocateRoutine,__in PFREE_ROUTINE FreeRoutine,__in PFILE_SET_SIZE_ROUTINE FileSetSizeRoutine,
						  __in PFILE_WRITE_ROUTINE FileWriteRoutine,__in PFILE_READ_ROUTINE FileReadRoutine,__in PFILE_FLUSH_ROUTINE FileFlushRoutine,
						  __in ULONG Cluster,__in PUNICODE_STRING FileName)
{
	NTSTATUS Status										= STATUS_SUCCESS;

	//
	// reject invalid parameter combinations
	//
	if(!HiveData && (OperationType == HINIT_MEMORY || OperationType == HINIT_FLAT || OperationType == HINIT_MEMORY_INPLACE))
		return STATUS_INVALID_PARAMETER;

	if(	OperationType != HINIT_CREATE &&
		OperationType != HINIT_MEMORY &&
		OperationType != HINIT_MEMORY_INPLACE &&
		OperationType != HINIT_FLAT &&
		OperationType != HINIT_FILE)
	{
		return STATUS_INVALID_PARAMETER;
	}

	//
	// setup common field
	//
	Hive->Signature										= HHIVE_SIGNATURE;
	Hive->Allocate										= AllocateRoutine;
	Hive->Free											= FreeRoutine;
	Hive->FileSetSize									= FileSetSizeRoutine;
	Hive->FileWrite										= FileWriteRoutine;
	Hive->FileRead										= FileReadRoutine;
	Hive->FileFlush										= FileFlushRoutine;
	Hive->Log											= FileType == HFILE_TYPE_LOG ? TRUE : FALSE;
	Hive->Alternate										= FileType == HFILE_TYPE_ALTERNATE ? TRUE : FALSE;
	Hive->HiveFlags										= HiveFlags;

	if((Hive->Log || Hive->Alternate)  && (HiveFlags & HIVE_VOLATILE))
		return STATUS_INVALID_PARAMETER;

	if(Cluster == 0 || Cluster > HSECTOR_COUNT)
		return STATUS_INVALID_PARAMETER;

	Hive->Cluster										= Cluster;
	Hive->RefreshCount									= 0;
	Hive->StorageTypeCount								= HTYPE_COUNT;
	Hive->Storage[1].Length								= 0;
	Hive->Storage[1].Map								= 0;
	Hive->Storage[1].SmallDir							= 0;
	Hive->Storage[1].Guard								= -1;
	Hive->Storage[1].FreeSummary						= 0;
	InitializeListHead(&Hive->Storage[1].FreeBins);
	for(ULONG i = 0; i < ARRAYSIZE(Hive->Storage[1].FreeDisplay); i ++)
		RtlInitializeBitMap(&Hive->Storage[1].FreeDisplay[i],0,0);

	Hive->Storage[0].Length								= 0;
	Hive->Storage[0].Map								= 0;
	Hive->Storage[0].SmallDir							= 0;
	Hive->Storage[0].Guard								= -1;
	Hive->Storage[0].FreeSummary						= 0;
	InitializeListHead(&Hive->Storage[0].FreeBins);
	for(ULONG i = 0; i < ARRAYSIZE(Hive->Storage[0].FreeDisplay); i ++)
		RtlInitializeBitMap(&Hive->Storage[0].FreeDisplay[i],0,0);

	RtlInitializeBitMap(&Hive->DirtyVector,0,0);

	Hive->DirtyCount									= 0;
	Hive->DirtyAlloc									= 0;
	Hive->LogSize										= 0;
	Hive->Offset38										= 0;

	Hive->GetCellRoutine								= &HvpGetCellPaged;
	Hive->ReleaseCellRoutine							= 0;
	Hive->Flat											= FALSE;
	Hive->ReadOnly										= FALSE;

	BOOLEAN UseForIo									= Hive->HiveFlags & HIVE_VOLATILE ? FALSE : TRUE;

	//
	// new create case
	//
	if(OperationType == HINIT_CREATE)
	{
		PHBASE_BLOCK BaseBlock							= static_cast<PHBASE_BLOCK>(Hive->Allocate(sizeof(HBASE_BLOCK),UseForIo,'11MC'));
		if(!BaseBlock)
			return STATUS_INSUFFICIENT_RESOURCES;

		//
		// make sure the buffer we got back is cluster-aligned.if not, try harder to get an aligned buffer.
		//
		ULONG Alignment									= Cluster * HSECTOR_SIZE - 1;
		if(reinterpret_cast<ULONG_PTR>(BaseBlock) & Alignment)
		{
			Hive->Free(BaseBlock,sizeof(HBASE_BLOCK));

			BaseBlock									= static_cast<PHBASE_BLOCK>(Hive->Allocate(PAGE_SIZE,TRUE,'21MC'));
			if(!BaseBlock)
				return STATUS_INSUFFICIENT_RESOURCES;

			//
			// return the quota for the extra allocation, as we are not really using it and it will not be accounted for later when we free it.
			//
			CmpReleaseGlobalQuota(PAGE_SIZE - sizeof(HBASE_BLOCK));
		}

		BaseBlock->Signature							= HBASE_BLOCK_SIGNATURE;
		BaseBlock->Sequence1							= 1;
		BaseBlock->Sequence2							= 1;
		BaseBlock->TimeStamp.QuadPart					= 0;
		BaseBlock->Major								= 1;
		BaseBlock->Minor								= 3;
		BaseBlock->Type									= HFILE_TYPE_PRIMARY;
		BaseBlock->Format								= 1;
		BaseBlock->RootCell								= HCELL_NIL;
		BaseBlock->Length								= 0;
		BaseBlock->Cluster								= Cluster;
		BaseBlock->CheckSum								= 0;
		BaseBlock->OffsetFF8							= 0;
		Hive->BaseBlock									= BaseBlock;
		Hive->Version									= 3;

		HvpFillFileName(BaseBlock,FileName);

		return STATUS_SUCCESS;
	}

	//
	// flat image case
	//
	if(OperationType == HINIT_FLAT)
	{
		Hive->BaseBlock									= static_cast<PHBASE_BLOCK>(HiveData);
		Hive->Version									= Hive->BaseBlock->Minor;
		Hive->Flat										= TRUE;
		Hive->ReadOnly									= TRUE;
		Hive->GetCellRoutine							= &HvpGetCellFlat;
		Hive->Storage[0].Length							= Hive->BaseBlock->Length;
		Hive->StorageTypeCount							= 1;
		Hive->BaseBlock->OffsetFF8						= 0;

		return STATUS_SUCCESS;
	}

	//
	// readonly image case
	//
	if(OperationType == HINIT_MEMORY_INPLACE)
	{
		PHBASE_BLOCK BaseBlock							= static_cast<PHBASE_BLOCK>(HiveData);

		if( (BaseBlock->Signature != HBASE_BLOCK_SIGNATURE)		||
			(BaseBlock->Type != HFILE_TYPE_PRIMARY)				||
			(BaseBlock->Major != 1)								||
			(BaseBlock->Minor > 5)								||
			(BaseBlock->Format != 1)							||
			(BaseBlock->Sequence1 != BaseBlock->Sequence2)		||
			(HvpHeaderCheckSum(BaseBlock) != BaseBlock->CheckSum))
		{
			return STATUS_REGISTRY_CORRUPT;
		}

		Hive->BaseBlock									= BaseBlock;
		Hive->Version									= BaseBlock->Minor;
		Hive->ReadOnly									= TRUE;
		Hive->StorageTypeCount							= 1;
		BaseBlock->OffsetFF8							= 0;

		Status											= HvpAdjustHiveFreeDisplay(Hive,BaseBlock->Length,0);
		if(!NT_SUCCESS(Status))
			return Status;

		if(!NT_SUCCESS(HvpBuildMap(Hive,Add2Ptr(HiveData,HBLOCK_SIZE,PVOID))))
			return STATUS_REGISTRY_CORRUPT;

		return STATUS_SUCCESS;
	}

	//
	// memory copy case
	//
	if(OperationType == HINIT_MEMORY)
	{
		PHBASE_BLOCK BaseBlock							= static_cast<PHBASE_BLOCK>(HiveData);

		if( (BaseBlock->Signature != HBASE_BLOCK_SIGNATURE)										||
			(BaseBlock->Type != HFILE_TYPE_PRIMARY && BaseBlock->Type != HFILE_TYPE_ALTERNATE)	||
			(BaseBlock->Format != 1)															||
			(BaseBlock->Major != 1)																||
			(BaseBlock->Minor > 5)																||
			(HvpHeaderCheckSum(BaseBlock) != BaseBlock->CheckSum))
		{
			return STATUS_REGISTRY_CORRUPT;
		}

		Hive->BaseBlock									= static_cast<PHBASE_BLOCK>(Hive->Allocate(sizeof(HBASE_BLOCK),UseForIo,'31MC'));
		if(!Hive->BaseBlock)
			return STATUS_INSUFFICIENT_RESOURCES;

		//
		// make sure the buffer we got back is cluster-aligned.if not, try harder to get an aligned buffer.
		//
		ULONG Alignment									= Cluster * HSECTOR_SIZE - 1;

		if(reinterpret_cast<ULONG>(Hive->BaseBlock) & Alignment)
		{
			Hive->Free(Hive->BaseBlock,sizeof(HBASE_BLOCK));
			Hive->BaseBlock								= static_cast<PHBASE_BLOCK>(Hive->Allocate(PAGE_SIZE,TRUE,'41MC'));
			if(!Hive->BaseBlock)
				return STATUS_INSUFFICIENT_RESOURCES;
		}

		RtlCopyMemory(Hive->BaseBlock,BaseBlock,HSECTOR_SIZE);

		Hive->Version									= Hive->BaseBlock->Minor;
		Hive->BaseBlock->OffsetFF8						= BaseBlock->OffsetFF8;
		Hive->BaseBlock->RecoveredFromLog				= BaseBlock->RecoveredFromLog;

		Status											= HvpAdjustHiveFreeDisplay(Hive,BaseBlock->Length,0);
		if(!NT_SUCCESS(Status))
			return Status;

		if(!NT_SUCCESS(HvpBuildMapAndCopy(Hive,Add2Ptr(HiveData,HBLOCK_SIZE,PVOID))))
		{
			Hive->Free(Hive->BaseBlock,sizeof(HBASE_BLOCK));
			Hive->BaseBlock								= 0;
			return STATUS_REGISTRY_CORRUPT;
		}

		HvpFillFileName(Hive->BaseBlock, FileName);

		return STATUS_SUCCESS;
	}

	//
	// file read case
	//
	if(OperationType == HINIT_FILE || OperationType == HINIT_MAPPED)
	{
		if(OperationType == HINIT_MAPPED)
		{
			Hive->GetCellRoutine						= &HvpGetCellMapped;
			Hive->ReleaseCellRoutine					= &HvpReleaseCellMapped;
		}

		//
		// get the file image (possible recovered via log) into memory
		//
		Status											= HvLoadHive(Hive);
		if(Status != STATUS_SUCCESS && Status != STATUS_REGISTRY_RECOVERED)
			return Status;

		if(Status == STATUS_REGISTRY_RECOVERED)
		{
			//
			// we have a good hive, with a log, and a dirty map, all set up.
			// only problem is that we need to flush the file so the log can be cleared and new writes posted against the hive.
			// since we know we have a good log in hand, we just write the hive image.
			//
			if(!HvpDoWriteHive(Hive,HFILE_TYPE_PRIMARY))
			{
				//
				// Clean up the bins already allocated
				//
				HvpFreeAllocatedBins(Hive);

				return STATUS_REGISTRY_IO_FAILED;
			}

			//
			// if we get here, we have recovered the hive, and written it out to disk correctly.so we clear the log here.
			//
			RtlClearAllBits(&(Hive->DirtyVector));
			Hive->DirtyCount							= 0;
			Hive->FileSetSize(Hive,HFILE_TYPE_LOG,0);
			Hive->LogSize								= 0;
		}

		//
		// slam debug name data into base block
		//
		HvpFillFileName(Hive->BaseBlock, FileName);

		return STATUS_SUCCESS;
	}

	return STATUS_INVALID_PARAMETER;
}

//
// check security descriptors
//
BOOLEAN CmpValidateHiveSecurityDescriptors(__in PHHIVE Hive,__out PBOOLEAN SecurityStatus)
{
	return TRUE;
}

//
// check bin
//
ULONG HvCheckBin(__in PHHIVE Hive,__in PHBIN Bin,__out_opt PULONG Storage)
{
	ULONG freespace										= 0;
	ULONG allocated										= 0;
	ULONG userallocated									= 0;
	HvCheckBinDebug.Bin									= Bin;
	HvCheckBinDebug.Status								= 0;
	HvCheckBinDebug.CellPoint							= 0;
	PHCELL p											= Add2Ptr(Bin,sizeof(HBIN),PHCELL);
	PHCELL lp											= p;
	PHCELL np											= 0;

	//
	// scan all the cells in the bin, total free and allocated, check for impossible pointers.
	//
	while(p < Add2Ptr(Bin,Bin->Size,PHCELL))
	{
		//
		// check last pointer
		//
		if(USE_OLD_CELL(Hive))
		{
			if(lp == p)
			{
				if(p->u.OldCell.Last != 0xffffffff)
				{
					HvCheckBinDebug.Status				= 20;
					HvCheckBinDebug.CellPoint			= p;
					return 20;
				}
			}
			else
			{
				if(Add2Ptr(Bin,p->u.OldCell.Last,PHCELL) != lp)
				{
					HvCheckBinDebug.Status				= 30;
					HvCheckBinDebug.CellPoint			= p;
					return 30;
				}
			}
		}

		//
		// check size
		//
		if(p->Size < 0)
		{
			//
			// allocated cell
			//
			if(static_cast<ULONG>(p->Size * -1) > Bin->Size || Add2Ptr(p,p->Size * -1,PHCELL) > Add2Ptr(Bin,Bin->Size,PHCELL))
			{
				HvCheckBinDebug.Status					= 40;
				HvCheckBinDebug.CellPoint				= p;
				return 40;
			}

			allocated									+= (p->Size * -1);

			if(USE_OLD_CELL(Hive))
				userallocated							+= (p->Size * -1) - FIELD_OFFSET(HCELL, u.OldCell.u.UserData);
			else
				userallocated							+= (p->Size * -1) - FIELD_OFFSET(HCELL, u.NewCell.u.UserData);

			if(allocated > Bin->Size)
			{
				HvCheckBinDebug.Status					= 50;
				HvCheckBinDebug.CellPoint				= p;
				return 50;
			}

			np											= Add2Ptr(p,p->Size * -1,PHCELL);
		}
		else
		{
			//
			// free cell
			//
			if(static_cast<ULONG>(p->Size) > Bin->Size || Add2Ptr(p,p->Size,PHCELL) > Add2Ptr(Bin,Bin->Size,PHCELL) || !p->Size)
			{
				HvCheckBinDebug.Status					= 60;
				HvCheckBinDebug.CellPoint				= p;
				return 60;
			}

			freespace									= freespace + p->Size;

			if(freespace > Bin->Size)
			{
				HvCheckBinDebug.Status					= 70;
				HvCheckBinDebug.CellPoint				= p;
				return 70;
			}

			np											= Add2Ptr(p,p->Size,PHCELL);
		}

		lp												= p;
		p												= np;
	}

	if(freespace + allocated + sizeof(HBIN) != Bin->Size)
	{
		HvCheckBinDebug.Status							= 995;
		return 995;
	}

	if(p != Add2Ptr(Bin,Bin->Size,PHCELL))
	{
		HvCheckBinDebug.Status							= 1000;
		return 1000;
	}

	if(Storage)
		*Storage										+= userallocated;

	return 0;
}

//
// check hive
//
ULONG HvCheckHive(__in PHHIVE Hive,__out_opt PULONG Storage)
{
	ULONG localstorage									= 0;
	HCELL_INDEX p										= 0;
	HvCheckHiveDebug.Hive								= Hive;
	HvCheckHiveDebug.Status								= 0;
	HvCheckHiveDebug.Space								= 0xffffffff;
	HvCheckHiveDebug.MapPoint							= HCELL_NIL;
	HvCheckHiveDebug.BinPoint							= 0;

	//
	// one pass for Stable space, one pass for Volatile
	//
	for(ULONG i = 0; i <= 1; i++)
	{
		ULONG Length									= Hive->Storage[i].Length;

		//
		// for each bin in the space
		//
		while(p < Length)
		{
			PHMAP_ENTRY t								= HvpGetCellMap(Hive,p);
			if(!t)
			{
				HvCheckHiveDebug.Status					= 2005;
				HvCheckHiveDebug.Space					= i;
				HvCheckHiveDebug.MapPoint				= p;
				return HvCheckHiveDebug.Status;
			}

			if((t->BinAddress & 2) == 0)
			{
				PHBIN Bin								= reinterpret_cast<PHBIN>(t->BinAddress & 0xfffffff0);

				//
				// bin header valid?
				//
				if(Bin->Size > Length || Bin->Signature != HBIN_SIGNATURE || Bin->FileOffset != p)
				{
					HvCheckHiveDebug.Status				= 2010;
					HvCheckHiveDebug.Space				= i;
					HvCheckHiveDebug.MapPoint			= p;
					HvCheckHiveDebug.BinPoint			= Bin;
					return HvCheckHiveDebug.Status;
				}

				//
				// structure inside the bin valid?
				//
				ULONG rc								= HvCheckBin(Hive,Bin,&localstorage);
				if(rc)
				{
					HvCheckHiveDebug.Status				= rc;
					HvCheckHiveDebug.Space				= i;
					HvCheckHiveDebug.MapPoint			= p;
					HvCheckHiveDebug.BinPoint			= Bin;

					return rc;
				}

				p										+= Bin->Size;
			}
			else
			{
				//
				// bin is not present, skip it and advance to the next one.
				//
				PFREE_HBIN FreeBin						= reinterpret_cast<PFREE_HBIN>(t->BlockAddress);
				p										+= FreeBin->Size;
			}
		}

		//
		// beginning of volatile space
		//
		p												= 0x80000000;
	}

	if(Storage)
		*Storage = localstorage;

	return 0;
}

//
// get cell size
//
LONG HvGetCellSize(__in PHHIVE Hive,__in PVOID Address)
{
	LONG Size											= 0;

	if(USE_OLD_CELL(Hive))
	{
		Size											= ((CONTAINING_RECORD(Address,HCELL,u.OldCell.u.UserData))->Size) * -1;
		Size											-= FIELD_OFFSET(HCELL,u.OldCell.u.UserData);
	}
	else
	{
		Size											= ((CONTAINING_RECORD(Address,HCELL,u.NewCell.u.UserData))->Size) * -1;
		Size											-= FIELD_OFFSET(HCELL,u.NewCell.u.UserData);
	}

	return Size;
}

//
// is cell allocated
//
BOOLEAN HvIsCellAllocated(__in PHHIVE Hive,__in HCELL_INDEX Cell)
{
	if(Hive->Flat)
		return TRUE;

	ULONG Type											= HvGetCellType(Cell);

	if((Cell & ~HCELL_TYPE_MASK) > Hive->Storage[Type].Length || Cell % HCELL_PAD(Hive))
		return FALSE;

	PHCELL Address										= HvpGetHCell(Hive,Cell);
	PHMAP_ENTRY Me										= HvpGetCellMap(Hive,Cell);
	if(!Me || (Me->BinAddress & 2))
		return FALSE;

	PHBIN Bin											= reinterpret_cast<PHBIN>(static_cast<ULONG_PTR>(Me->BinAddress) & 0xfffffff0);
	ULONG Offset										= static_cast<ULONG>(reinterpret_cast<ULONG_PTR>(Address) - reinterpret_cast<ULONG_PTR>(Bin));
	LONG Size											= Address->Size * -1;

	if(Address->Size > 0 || Offset + static_cast<ULONG>(Size) > Bin->Size || Offset < sizeof(HBIN))
		return FALSE;

	if(USE_OLD_CELL(Hive))
	{
		if(Address->u.OldCell.Last != 0xffffffff)
		{
			if(Address->u.OldCell.Last > Bin->Size)
				return FALSE;

			PHCELL Below								= Add2Ptr(Bin,Address->u.OldCell.Last,PHCELL);
			Size										= (Below->Size < 0) ? Below->Size * -1 : Below->Size;

			if(reinterpret_cast<ULONG_PTR>(Below) + Size != reinterpret_cast<ULONG_PTR>(Address))
				return FALSE;
		}
	}

	return TRUE;
}

//
// check value list
//
ULONG CmpCheckValueList(__in PHHIVE Hive,__in PCELL_DATA List,__in ULONG Count,__in HCELL_INDEX KeyCell)
{
	CmpCheckValueListDebug.Hive							= Hive;
	CmpCheckValueListDebug.Status						= 0;
	CmpCheckValueListDebug.List							= List;
	CmpCheckValueListDebug.Index						= -1;
	CmpCheckValueListDebug.Cell							= 0;
	CmpCheckValueListDebug.CellPoint					= 0;

	for(ULONG i = 0; i < Count; i++)
	{
		HCELL_INDEX Cell								= List->u.KeyList[i];
		ULONG Status									= 0;

		do
		{
			if(Cell == HCELL_NIL)
			{
				Status									= 5010;
				CmpCheckValueListDebug.Index			= i;
				CmpCheckValueListDebug.Cell				= Cell;
				break;
			}

			if(!HvIsCellAllocated(Hive,Cell))
			{
				Status									= 5020;
				CmpCheckValueListDebug.Index			= i;
				CmpCheckValueListDebug.Cell				= Cell;
				break;
			}

			PCELL_DATA ValueCell						= static_cast<PCELL_DATA>(HvGetCell(Hive,Cell));
			if(!ValueCell)
			{
				CmpCheckValueListDebug.Status			= 5099;
				CmpCheckValueListDebug.Index			= i;
				CmpCheckValueListDebug.Cell				= Cell;
				return CmpCheckValueListDebug.Status;
			}

			ULONG Size									= HvGetCellSize(Hive,ValueCell);
			if(ValueCell->u.KeyValue.Signature != CM_KEY_VALUE_SIGNATURE)
			{
				Status									= 5030;
				CmpCheckValueListDebug.Index			= i;
				CmpCheckValueListDebug.Cell				= Cell;
				CmpCheckValueListDebug.CellPoint		= ValueCell;
				break;
			}

			ULONG UsedLength							= FIELD_OFFSET(CM_KEY_VALUE,Name) + ValueCell->u.KeyValue.NameLength;
			if(UsedLength > Size)
			{
				Status									= 5040;
				CmpCheckValueListDebug.Index			= i;
				CmpCheckValueListDebug.Cell				= Cell;
				CmpCheckValueListDebug.CellPoint		= ValueCell;
				break;
			}

			ULONG DataLength							= ValueCell->u.KeyValue.DataLength;
			if(DataLength >= CM_KEY_VALUE_SPECIAL_SIZE)
				break;

			HCELL_INDEX Data							= ValueCell->u.KeyValue.Data;
			if(!DataLength && Data != HCELL_NIL)
			{
				Status									= 5050;
				CmpCheckValueListDebug.Index			= i;
				CmpCheckValueListDebug.Cell				= Cell;
				CmpCheckValueListDebug.CellPoint		= ValueCell;
				break;
			}

			if(DataLength > 0 && !HvIsCellAllocated(Hive,Data))
			{
				Status									= 5060;
				CmpCheckValueListDebug.Index			= i;
				CmpCheckValueListDebug.Cell				= Cell;
				CmpCheckValueListDebug.CellPoint		= ValueCell;
				break;
			}

			if(Hive->Version < 4 || DataLength < 0x3FD8)
				break;

			PCELL_DATA ValueData						= static_cast<PCELL_DATA>(HvGetCell(Hive,Data));
			if(!ValueData)
			{
				CmpCheckValueListDebug.Status			= 5060;
				CmpCheckValueListDebug.Index			= i;
				CmpCheckValueListDebug.Cell				= Data;
				return CmpCheckValueListDebug.Status;
			}

			if(ValueData->u.KeyBigData.Signature != 'bd' || !ValueData->u.KeyBigData.Count || ValueData->u.KeyBigData.DataList == HCELL_NIL)
			{
				Status									= 5097;
				CmpCheckValueListDebug.Index			= i;
				CmpCheckValueListDebug.Cell				= Data;
				break;
			}

			if(!HvIsCellAllocated(Hive,ValueData->u.KeyBigData.DataList))
			{
				Status									= 5096;
				CmpCheckValueListDebug.Index			= i;
				CmpCheckValueListDebug.Cell				= ValueData->u.KeyBigData.DataList;
				CmpCheckValueListDebug.CellPoint		= ValueData;
			}

			PHCELL_INDEX DataArray						= static_cast<PHCELL_INDEX>(HvGetCell(Hive,ValueData->u.KeyBigData.DataList));
			if(!DataArray)
			{
				CmpCheckValueListDebug.Status			= 5098;
				CmpCheckValueListDebug.Index			= i;
				CmpCheckValueListDebug.Cell				= ValueData->u.KeyBigData.DataList;
				return 5095;
			}

			for(USHORT j = 0; j < ValueData->u.KeyBigData.Count; j ++)
			{
				if(HvIsCellAllocated(Hive,DataArray[i]))
					continue;

				Status									= 5094;
				CmpCheckValueListDebug.Index			= j;
				CmpCheckValueListDebug.Cell				= DataArray[i];
				CmpCheckValueListDebug.CellPoint		= ValueData;
				break;
			}
		}while(0);

		if(!Status)
			continue;

		PCELL_DATA KeyData								= static_cast<PCELL_DATA>(HvGetCell(Hive,KeyCell));
		if(CM_TRY_TO_FIX_ERROR && KeyData && HvMarkCellDirty(Hive,KeyCell) && HvMarkCellDirty(Hive,KeyData->u.KeyNode.ValueList.List))
		{
			KeyData->u.KeyNode.ValueList.Count			-= 1;
			Count										-= 1;
			RtlMoveMemory(List->u.KeyList + i,List->u.KeyList + i + 1,(Count - i) * sizeof(HCELL_INDEX));
			i											-= 1;
			Hive->BaseBlock->OffsetFF8					|= 4;
		}
		else
		{
			return Status;
		}
	}

	return 0;
}

//
// check key
//
ULONG CmpCheckKey(__in PHHIVE Hive,__in ULONG Flags,__in HCELL_INDEX Cell,__in HCELL_INDEX ParentCell,__in BOOLEAN SecurityStatus)
{
	CmpCheckKeyDebug.Hive								= Hive;
	CmpCheckKeyDebug.Status								= 0;
	CmpCheckKeyDebug.Cell								= Cell;
	CmpCheckKeyDebug.CellPoint							= 0;
	CmpCheckKeyDebug.RootPoint							= 0;
	CmpCheckKeyDebug.Index								= -1;

	if(CmpKeyCellDebug == Cell)
		DbgBreakPoint();

	//
	// Check key itself
	//
	if(!HvIsCellAllocated(Hive,Cell))
		return CmpCheckKeyDebug.Status = 4010;

	PCELL_DATA KeyCellData								= static_cast<PCELL_DATA>(HvGetCell(Hive,Cell));
	if(!KeyCellData)
		return CmpCheckKeyDebug.Status = 4095;

	CmpCheckKeyDebug.CellPoint							= KeyCellData;

	ULONG KeyCellSize									= HvGetCellSize(Hive,KeyCellData);
	if(KeyCellSize > 0x45c/*REG_MAX_PLAUSIBLE_KEY_SIZE*/)
		return CmpCheckKeyDebug.Status = 4020;

	ULONG UsedLength									= FIELD_OFFSET(CM_KEY_NODE,Name) + KeyCellData->u.KeyNode.NameLength;
	if(UsedLength > KeyCellSize || !KeyCellData->u.KeyNode.NameLength)
		return CmpCheckKeyDebug.Status = 4030;

	if(KeyCellData->u.KeyNode.Signature != CM_KEY_NODE_SIGNATURE)
	{
		CmpCheckKeyDebug.Status							= 4040;
		if(CM_TRY_TO_FIX_ERROR && HvMarkCellDirty(Hive,Cell))
		{
			KeyCellData->u.KeyNode.Signature			= CM_KEY_NODE_SIGNATURE;
			Hive->BaseBlock->OffsetFF8					|= 4;
		}
		else
		{
			return CmpCheckKeyDebug.Status;
		}
	}
	if(ParentCell != HCELL_NIL && KeyCellData->u.KeyNode.Parent != ParentCell)
	{
		CmpCheckKeyDebug.Status							= 4045;
		if(CM_TRY_TO_FIX_ERROR && HvMarkCellDirty(Hive,Cell))
		{
			KeyCellData->u.KeyNode.Parent				= ParentCell;
			Hive->BaseBlock->OffsetFF8					|= 4;
		}
		else
		{
			return CmpCheckKeyDebug.Status;
		}
	}

	USHORT ClassLength									= KeyCellData->u.KeyNode.ClassLength;
	HCELL_INDEX Class									= KeyCellData->u.KeyNode.u1.s1.Class;
	ULONG ValueCount									= KeyCellData->u.KeyNode.ValueList.Count;
	HCELL_INDEX ValueList								= KeyCellData->u.KeyNode.ValueList.List;
	HCELL_INDEX Security								= KeyCellData->u.KeyNode.u1.s1.Security;

	//
	// check simple non-empty cases
	//
	if(ClassLength > 0)
	{
		if(Class == HCELL_NIL)
		{
			KeyCellData->u.KeyNode.ClassLength			= 0;
			HvMarkCellDirty(Hive,Cell);
		}
		else if(!HvIsCellAllocated(Hive,Class))
		{
			CmpCheckKeyDebug.Status						= 4080;

			if(CM_TRY_TO_FIX_ERROR && HvMarkCellDirty(Hive,Cell))
			{
				KeyCellData->u.KeyNode.ClassLength		= 0;
				KeyCellData->u.KeyNode.u1.s1.Class		= HCELL_NIL;
				Hive->BaseBlock->OffsetFF8				|= 4;
			}
			else
			{
				return CmpCheckKeyDebug.Status;
			}
		}
	}

	ULONG RetValue										= 0;
	if(Security != HCELL_NIL)
	{
		if(!HvIsCellAllocated(Hive,Security) || (ParentCell != HCELL_NIL && CM_TRY_TO_FIX_ERROR && SecurityStatus))
		{
			RetValue									= 4090;
		}
	}
	else
	{
		RetValue										= 4130;
	}

	if(RetValue)
	{
		if(!CM_TRY_TO_FIX_ERROR || ParentCell == HCELL_NIL)
			return RetValue;

		PCELL_DATA ParentCellData						= static_cast<PCELL_DATA>(HvGetCell(Hive,ParentCell));
		if(!ParentCellData || ParentCellData->u.KeyNode.u1.s1.Security == HCELL_NIL)
			return RetValue;

		PCELL_DATA ParentSecurity						= static_cast<PCELL_DATA>(HvGetCell(Hive,ParentCellData->u.KeyNode.u1.s1.Security));
		if(!ParentSecurity)
			return RetValue;

		if(!HvMarkCellDirty(Hive,Cell) || !HvMarkCellDirty(Hive,ParentCellData->u.KeyNode.u1.s1.Security))
			return RetValue;

		KeyCellData->u.KeyNode.u1.s1.Security			= ParentCellData->u.KeyNode.u1.s1.Security;
		ParentSecurity->u.KeySecurity.ReferenceCount	+= 1;
		Hive->BaseBlock->OffsetFF8						|= 4;
	}

	//
	// check value list case
	//
	RetValue											= 0;
	if(ValueCount > 0)
	{
		if(!HvIsCellAllocated(Hive,ValueList))
		{
			CmpCheckKeyDebug.Status						= 4100;

			if(CM_TRY_TO_FIX_ERROR && HvMarkCellDirty(Hive,Cell) && (KeyCellData = static_cast<PCELL_DATA>(HvGetCell(Hive,Cell))))
			{
				KeyCellData->u.KeyNode.ValueList.Count	= 0;
				KeyCellData->u.KeyNode.ValueList.List	= HCELL_NIL;
				Hive->BaseBlock->OffsetFF8				|= 4;
			}
			else
			{
				return CmpCheckKeyDebug.Status;
			}
		}
		else
		{
			KeyCellData									= static_cast<PCELL_DATA>(HvGetCell(Hive,ValueList));
			if(!KeyCellData)
				return CmpCheckKeyDebug.Status = 4094;

			ULONG ValueSize								= HvGetCellSize(Hive,KeyCellData);
			RetValue									= ValueCount * 4 <= ValueSize ? CmpCheckValueList(Hive,KeyCellData,ValueCount,Cell) : 4095;
		}
	}
	else if(Hive->Version >= 5 && ValueList != HCELL_NIL)
	{
		RetValue										= 4106;
	}

	if(RetValue)
	{
		CmpCheckKeyDebug.CellPoint						= KeyCellData;
		CmpCheckKeyDebug.Status							= RetValue;

		if(CM_TRY_TO_FIX_ERROR && HvMarkCellDirty(Hive,Cell) && (KeyCellData = static_cast<PCELL_DATA>(HvGetCell(Hive,Cell))))
		{
			KeyCellData->u.KeyNode.ValueList.Count		= 0;
			KeyCellData->u.KeyNode.ValueList.List		= HCELL_NIL;
			Hive->BaseBlock->OffsetFF8					|= 4;
		}
		else
		{
			return RetValue;
		}
	}

	//
	// check subkey list case
	//
	KeyCellData											= static_cast<PCELL_DATA>(HvGetCell(Hive,Cell));
	CmpCheckKeyDebug.CellPoint							= KeyCellData;
	if(HvGetCellType(Cell) == 1 && KeyCellData->u.KeyNode.SubKeyCounts[0])
		return CmpCheckKeyDebug.Status = 4108;

	if(KeyCellData->u.KeyNode.SubKeyCounts[0] > 0)
	{
		if(!HvIsCellAllocated(Hive,KeyCellData->u.KeyNode.SubKeyLists[0]))
		{
			CmpCheckKeyDebug.Status						= 4110;
			if(CM_TRY_TO_FIX_ERROR && HvMarkCellDirty(Hive,Cell))
			{
				KeyCellData->u.KeyNode.SubKeyCounts[0]	= 0;
				KeyCellData->u.KeyNode.SubKeyLists[0]	= HCELL_NIL;
				Hive->BaseBlock->OffsetFF8				|= 4;
			}
			else
			{
				return CmpCheckKeyDebug.Status;
			}
		}
		else
		{
			PCM_KEY_INDEX Root							= static_cast<PCM_KEY_INDEX>(HvGetCell(Hive,KeyCellData->u.KeyNode.SubKeyLists[0]));
			if(!Root)
				return CmpCheckKeyDebug.Status = 4093;

			CmpCheckKeyDebug.RootPoint					= Root;

			if(Root->Signature == CM_KEY_INDEX_LEAF || Root->Signature == CM_KEY_FAST_LEAF || Root->Signature == CM_KEY_INDEX_HASH)
			{
				if(Root->Count != KeyCellData->u.KeyNode.SubKeyCounts[0])
				{
					CmpCheckKeyDebug.Status				= 4120;
					if(CM_TRY_TO_FIX_ERROR && HvMarkCellDirty(Hive,Cell) && (KeyCellData = static_cast<PCELL_DATA>(HvGetCell(Hive,Cell))))
					{
						KeyCellData->u.KeyNode.SubKeyCounts[0] = Root->Count;
						Hive->BaseBlock->OffsetFF8		|= 4;
					}
					else
					{
						return CmpCheckKeyDebug.Status;
					}
				}
			}
			else if(Root->Signature == CM_KEY_INDEX_ROOT)
			{
				ULONG SubCount							= 0;
				RetValue								= 0;
				for(ULONG i = 0; i < Root->Count; i++)
				{
					CmpCheckKeyDebug.Index				= i;
					if(!HvIsCellAllocated(Hive,Root->List[i]))
					{
						RetValue						= 4130;
						break;
					}

					PCM_KEY_INDEX Leaf					= static_cast<PCM_KEY_INDEX>(HvGetCell(Hive,Root->List[i]));

					if(Leaf->Signature != CM_KEY_INDEX_LEAF && Leaf->Signature != CM_KEY_FAST_LEAF && Leaf->Signature != CM_KEY_INDEX_HASH)
					{
						RetValue						= 4140;
						break;
					}

					SubCount							+= Leaf->Count;
				}

				if(RetValue)
				{
					CmpCheckKeyDebug.Status				= RetValue;
					if(CM_TRY_TO_FIX_ERROR && HvMarkCellDirty(Hive,Cell))
					{
						KeyCellData->u.KeyNode.SubKeyCounts[0]	= 0;
						KeyCellData->u.KeyNode.SubKeyLists[0]	= HCELL_NIL;
						Hive->BaseBlock->OffsetFF8				|= 4;
					}

					return RetValue;
				}

				if(KeyCellData->u.KeyNode.SubKeyCounts[0] != SubCount)
				{
					CmpCheckKeyDebug.Status						= 4150;
					if(CM_TRY_TO_FIX_ERROR && HvMarkCellDirty(Hive,Cell))
					{
						KeyCellData->u.KeyNode.SubKeyCounts[0]	= SubCount;
						Hive->BaseBlock->OffsetFF8				|= 4;
					}
					else
					{
						return CmpCheckKeyDebug.Status;
					}
				}
			}
			else
			{
				CmpCheckKeyDebug.Status						= 4120;
				if(CM_TRY_TO_FIX_ERROR && HvMarkCellDirty(Hive,Cell))
				{
					KeyCellData->u.KeyNode.SubKeyCounts[0]	= 0;
					KeyCellData->u.KeyNode.SubKeyLists[0]	= HCELL_NIL;
					Hive->BaseBlock->OffsetFF8				|= 4;
				}
				else
				{
					return CmpCheckKeyDebug.Status;
				}
			}
		}
	}

	//
	// force volatiles to be empty, if this is a load operation
	//
	if((Flags & 2) || ((Flags & 5) && KeyCellData->u.KeyNode.SubKeyCounts[1]) || ((Flags & 8) && (KeyCellData->u.KeyNode.SubKeyLists[1] != HCELL_NIL || Hive->Version < 4)))
	{
		HvMarkCellDirty(Hive,Cell);
		KeyCellData->u.KeyNode.SubKeyCounts[1]			= 0;

		if((Flags & 4) && Hive->Version >= 4)
			KeyCellData->u.KeyNode.SubKeyLists[1]		= 0xbaadf00d;
		else
			KeyCellData->u.KeyNode.SubKeyLists[1]		= HCELL_NIL;
	}

	return 0;
}

//
// remove unreferenced sub key
//
BOOLEAN CmpRemoveSubKeyCellNoCellRef(__in PHHIVE Hive,__in HCELL_INDEX ParentCell,__in HCELL_INDEX SubKeyCell)
{
	PCM_KEY_NODE ParentKeyNode							= static_cast<PCM_KEY_NODE>(HvGetCell(Hive,ParentCell));
	if(!ParentKeyNode)
		return FALSE;

	PCM_KEY_INDEX IndexRoot								= static_cast<PCM_KEY_INDEX>(HvGetCell(Hive,ParentKeyNode->SubKeyLists[0]));
	if(!IndexRoot)
		return FALSE;

	if(IndexRoot->Signature == CM_KEY_INDEX_ROOT)
	{
		for(USHORT i = 0; i < IndexRoot->Count; i ++)
		{
			PCM_KEY_INDEX IndexLeaf						= static_cast<PCM_KEY_INDEX>(HvGetCell(Hive,IndexRoot->List[i]));
			if(!IndexLeaf)
				return FALSE;

			PCM_KEY_FAST_INDEX FastIndex				= reinterpret_cast<PCM_KEY_FAST_INDEX>(IndexLeaf);
			for(USHORT j = 0; j < IndexLeaf->Count; j ++)
			{
				HCELL_INDEX Current						= IndexLeaf->Signature == CM_KEY_INDEX_LEAF ? IndexLeaf->List[j] : FastIndex->List[j].Cell;
				if(Current == SubKeyCell)
				{
					HvMarkCellDirty(Hive,IndexRoot->List[i]);
					IndexLeaf->Count					-= 1;
					ULONG ItemLength					= IndexLeaf->Signature == CM_KEY_INDEX_LEAF ? sizeof(HCELL_INDEX) : sizeof(CM_INDEX);
					RtlMoveMemory(Add2Ptr(IndexLeaf->List,j * ItemLength,PVOID),Add2Ptr(IndexLeaf->List,(j + 1) * ItemLength,PVOID),(IndexLeaf->Count - j) * ItemLength);

					HvMarkCellDirty(Hive,ParentCell);
					ParentKeyNode->SubKeyCounts[0]			-= 1;
					return TRUE;
				}
			}
		}
	}
	else
	{
		PCM_KEY_FAST_INDEX FastIndex					= reinterpret_cast<PCM_KEY_FAST_INDEX>(IndexRoot);
		for(USHORT i = 0; i < IndexRoot->Count; i ++)
		{
			HCELL_INDEX Current							= IndexRoot->Signature == CM_KEY_INDEX_LEAF ? IndexRoot->List[i] : FastIndex->List[i].Cell;
			if(Current == SubKeyCell)
			{
				HvMarkCellDirty(Hive,ParentKeyNode->SubKeyLists[0]);
				IndexRoot->Count						-= 1;
				ULONG ItemLength						= (IndexRoot->Signature == CM_KEY_INDEX_LEAF ? sizeof(HCELL_INDEX) : sizeof(CM_INDEX));
				RtlMoveMemory(Add2Ptr(IndexRoot->List,i * ItemLength,PVOID),Add2Ptr(IndexRoot->List,(i + 1) * ItemLength,PVOID),(IndexRoot->Count - i) * ItemLength);


				HvMarkCellDirty(Hive,ParentCell);
				ParentKeyNode->SubKeyCounts[0]			-= 1;
				return TRUE;
			}
		}
	}

	return FALSE;
}

//
// check registry2
//
ULONG CmpCheckRegistry2(__in PHHIVE Hive,__in ULONG Flags,__in HCELL_INDEX Cell,__in HCELL_INDEX ParentCell,__in BOOLEAN SecurityStatus)
{
	ULONG Index											= 0;
	HCELL_INDEX StartCell								= Cell;
	HCELL_INDEX RootCell								= ParentCell;
	HCELL_INDEX SubKey									= HCELL_NIL;
	ULONG rc											= 0;
	PCELL_DATA pcell									= 0;
	CmpCheckRegistry2Debug.Hive							= Hive;
	CmpCheckRegistry2Debug.Status						= 0;

NewKey:
	rc													= CmpCheckKey(Hive,Flags,Cell,ParentCell,SecurityStatus);
	if(rc != 0)
	{
		CmpCheckRegistry2Debug.Status					= rc;

		if(!CM_TRY_TO_FIX_ERROR || Cell == StartCell || !CmpRemoveSubKeyCellNoCellRef(Hive,ParentCell,Cell))
			return rc;

		Hive->BaseBlock->OffsetFF8						|= 4;
		ParentCell										= RootCell;
		Cell											= StartCell;
		Index											= 0;
		goto NewKey;
	}

	//
	// save Index and check out children
	//
	pcell												= static_cast<PCELL_DATA>(HvGetCell(Hive,Cell));
	if(!pcell)
		return CmpCheckRegistry2Debug.Status = 4099;

	pcell->u.KeyNode.WorkVar							= Index;

	for(Index = 0; Index < pcell->u.KeyNode.SubKeyCounts[0]; Index ++)
	{
		PVOID Temp										= HvGetCell(Hive,Cell);
		if(!Temp)
			return CmpCheckRegistry2Debug.Status = 4098;

		SubKey											= CmpFindSubKeyByNumber(Hive,Temp,Index);
		if(SubKey == HCELL_NIL)
			return 0;

		ParentCell										= Cell;
		Cell											= SubKey;
		goto NewKey;

ResumeKey:;
	}

	if(Cell == StartCell)
		return 0;

	pcell												= static_cast<PCELL_DATA>(HvGetCell(Hive,Cell));
	if(!pcell)
		return CmpCheckRegistry2Debug.Status = 4097;

	Index												= pcell->u.KeyNode.WorkVar;
	Cell												= ParentCell;
	pcell												= static_cast<PCELL_DATA>(HvGetCell(Hive,Cell));
	if(!pcell)
		return CmpCheckRegistry2Debug.Status = 4096;

	ParentCell											= pcell->u.KeyNode.Parent;

	goto ResumeKey;
}

//
// check registry
//
ULONG CmCheckRegistry(__in PCMHIVE CmHive,__in ULONG Flags)
{
	if(CmHive == CmpMasterHive)
		return 0;

	ULONG Ret											= 0;
	CmCheckRegistryDebug.Status							= 0;
	CmCheckRegistryDebug.Hive							= &CmHive->Hive;

	//
	// flags == 0x10004
	//
	if(Flags & 0x10000)
	{
		ULONG Storage									= 0;
		Ret												= HvCheckHive(&CmHive->Hive,&Storage);
		if(Ret)
		{
			CmCheckRegistryDebug.Status					= Ret;
			return Ret;
		}
	}

	BOOLEAN SecurityStatus								= FALSE;
	PRELEASE_CELL_ROUTINE SavedReleaseCell				= CmHive->Hive.ReleaseCellRoutine;
	CmHive->Hive.ReleaseCellRoutine						= 0;

	if(!CmpValidateHiveSecurityDescriptors(&CmHive->Hive,&SecurityStatus))
	{
		CmHive->Hive.ReleaseCellRoutine					= SavedReleaseCell;
		Ret												= 3040;
		CmCheckRegistryDebug.Status						= Ret;
		return Ret;
	}

	Ret													= CmpCheckRegistry2(&CmHive->Hive,Flags,CmHive->Hive.BaseBlock->RootCell,HCELL_NIL,SecurityStatus);
	CmHive->Hive.ReleaseCellRoutine						= SavedReleaseCell;

	return Ret;
}