//********************************************************************
//	created:	25:8:2008   0:06
//	file:		registry.h
//	author:		tiamo
//	purpose:	cm and hive support
//********************************************************************

#pragma once

//
// LOGICAL block size
//
#define HBLOCK_SIZE										0x1000

//
// LOGICAL sector size
//
#define HSECTOR_SIZE									0x200

//
// LOGICAL sectors / LOGICAL Block
//
#define HSECTOR_COUNT									8

//
// create hive
//
#define HINIT_CREATE									0

//
// memory hive
//
#define HINIT_MEMORY									1

//
// hive file
//
#define HINIT_FILE										2

//
// memory inplace file
//
#define HINIT_MEMORY_INPLACE							3

//
// flat
//
#define HINIT_FLAT										4

//
// mapped
//
#define HINIT_MAPPED									5

//
// volatile hive
//
#define HIVE_VOLATILE									1

//
// disable lazy flush
//
#define HIVE_NOLAZYFLUSH								2

//
// has been replaced
//
#define HIVE_HAS_BEEN_REPLACED							4

//
// base block signature
//
#define HBASE_BLOCK_SIGNATURE							0x66676572

//
// hive signature
//
#define HHIVE_SIGNATURE									0xbee0bee0

//
// log signature,"DIRT"
//
#define HLOG_DV_SIGNATURE								0x54524944

//
// bin signature
//
#define HBIN_SIGNATURE									0x6e696268

//
// this key (and all its children) is volatile.
//
#define KEY_VOLATILE									0x0001

//
// this key marks a bounary to another hive (sort of a link).
// the null value entry contains the hive and hive index of the root of the child hive.
//
#define KEY_HIVE_EXIT									0x0002

//
// this key is the root of a particular hive
//
#define KEY_HIVE_ENTRY									0x0004

//
// this key cannot be deleted, period.
//
#define KEY_NO_DELETE									0x0008

//
// this key is really a symbolic link.
//
#define KEY_SYM_LINK									0x0010

//
// the name for this key is stored in a compressed form.
//
#define KEY_COMP_NAME									0x0020

//
// there is no real key backing this, return the predefined handle. predefined handles are stashed in ValueList.Count.
//
#define KEY_PREDEF_HANDLE								0x0040

//
// the name for this value is stored in a compressed form
//
#define VALUE_COMP_NAME									0x01

//
// base hive
//
#define HFILE_TYPE_PRIMARY								0

//
// alternate (e.g. system.alt)
//
#define HFILE_TYPE_ALTERNATE							1

//
// log (security.log)
//
#define HFILE_TYPE_LOG									2

//
// target of savekey, etc.
//
#define HFILE_TYPE_EXTERNAL								3

//
// max type
//
#define HFILE_TYPE_MAX									4

//
// count
//
#define HTYPE_COUNT										2

//
// display size
//
#define HHIVE_FREE_DISPLAY_SIZE							24

//
// name size
//
#define HBASE_NAME_ALLOC								64

//
// directories
//
#define HDIRECTORY_SLOTS								1024

//
// tables
//
#define HTABLE_SLOTS									512

//
// type mask
//
#define HCELL_TYPE_MASK									0x80000000

//
// type shift
//
#define HCELL_TYPE_SHIFT								31

//
// table mask
//
#define HCELL_TABLE_MASK								0x7fe00000

//
// table shift
//
#define HCELL_TABLE_SHIFT								21

//
// block mask
//
#define HCELL_BLOCK_MASK								0x001ff000

//
// block shift
//
#define HCELL_BLOCK_SHIFT								12

//
// offset mask
//
#define HCELL_OFFSET_MASK								0x00000fff

//
// key signature "kn"
//
#define CM_KEY_NODE_SIGNATURE							0x6b6e

//
// value signature "kv"
//
#define CM_KEY_VALUE_SIGNATURE							0x6b76

//
// index root signature "ir"
//
#define CM_KEY_INDEX_ROOT								0x6972

//
// index leaf signature "il"
//
#define CM_KEY_INDEX_LEAF								0x696c

//
// fast leaf signature "fl"
//
#define CM_KEY_FAST_LEAF								0x666c

//
// hash leaf signature "hl"
//
#define CM_KEY_INDEX_HASH								0x686c

//
// special size
//
#define CM_KEY_VALUE_SPECIAL_SIZE						0x80000000

//
// cell index
//
typedef ULONG HCELL_INDEX,*PHCELL_INDEX;

//
// NULL
//
#define HCELL_NIL										((HCELL_INDEX)(-1))

//
// get cell
//
#define HvGetCell(Hive,Cell)							(((Hive)->GetCellRoutine)(Hive,Cell))

//
// release cell
//
#define HvReleaseCell(Hive,Cell)						(((Hive)->ReleaseCellRoutine) ? ((Hive)->ReleaseCellRoutine)(Hive,Cell) : FALSE)

//
// get cell type
//
#define HvGetCellType(Cell)								((ULONG)((Cell & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT))

//
// old cell format
//
#define USE_OLD_CELL(Hive)								(Hive->Version == 1)

//
// pad size
//
#define HCELL_PAD(Hive)									((Hive->Version >= 2) ? 8 : 16)

//
// allocate routine
//
typedef PVOID (*PALLOCATE_ROUTINE)(__in ULONG Length,__in BOOLEAN UseForIo,__in ULONG PoolTag);

//
// free routine
//
typedef VOID (*PFREE_ROUTINE)(__in PVOID MemoryBlock,__in ULONG GlobalQuotaSize);

//
// set size
//
typedef BOOLEAN (*PFILE_SET_SIZE_ROUTINE)(__in struct _HHIVE* Hive,__in ULONG FileType,__in ULONG FileSize);

//
// write
//
typedef BOOLEAN (*PFILE_WRITE_ROUTINE)(__in struct _HHIVE* Hive,__in ULONG FileType,__in PULONG FileOffset,__in PVOID DataBuffer,__in ULONG DataLength);

//
// read
//
typedef BOOLEAN (*PFILE_READ_ROUTINE)(struct _HHIVE* Hive,__in ULONG FileType,__in PULONG FileOffset,__in PVOID DataBuffer,__in ULONG DataLength);

//
// flush
//
typedef BOOLEAN (*PFILE_FLUSH_ROUTINE)(__in struct _HHIVE* Hive,__in ULONG FileType);

//
// get cell
//
typedef PVOID (*PGET_CELL_ROUTINE)(__in struct _HHIVE* Hive,__in HCELL_INDEX Cell);

//
// release cell
//
typedef VOID (*PRELEASE_CELL_ROUTINE)(__in struct _HHIVE* Hive,__in HCELL_INDEX Cell);

#include <pshpack4.h>
//
// base block
//
typedef struct _HBASE_BLOCK
{
	//
	// signature
	//
	ULONG												Signature;

	//
	// sequence1
	//
	ULONG												Sequence1;

	//
	// sequence2
	//
	ULONG												Sequence2;

	//
	// time stamp
	//
	LARGE_INTEGER										TimeStamp;

	//
	// major version
	//
	ULONG												Major;

	//
	// minor
	//
	ULONG												Minor;

	//
	// HFILE_TYPE_[PRIMARY|LOG]
	//
	ULONG												Type;

	//
	// format
	//
	ULONG												Format;

	//
	// root cell
	//
	HCELL_INDEX											RootCell;

	//
	// length exclude header
	//
	ULONG												Length;

	//
	// cluster,log file only
	//
	ULONG												Cluster;

	//
	// file name
	//
	UCHAR												FileName[HBASE_NAME_ALLOC];

	//
	// reserved
	//
	ULONG												Reserved1[99];

	//
	// checksum
	//
	ULONG												CheckSum;

	//
	// reserved
	//
	ULONG												Reserved2[894];

	//
	// offset ff8
	//
	ULONG												OffsetFF8;

	//
	// recovered from log
	//
	ULONG												RecoveredFromLog;
}HBASE_BLOCK,*PHBASE_BLOCK;

#include <poppack.h>

//
// entry
//
typedef struct _HMAP_ENTRY
{
	//
	// block memory address
	//
	ULONG												BlockAddress;

	//
	// bin address
	//
	ULONG												BinAddress;

	//
	// offset 8
	//
	ULONG												Offset8;

	//
	// mem alloc
	//
	ULONG												MemAlloc;
}HMAP_ENTRY,*PHMAP_ENTRY;

//
// table
//
typedef struct _HMAP_TABLE
{
	//
	// entries
	//
	HMAP_ENTRY											Table[HTABLE_SLOTS];
}HMAP_TABLE,*PHMAP_TABLE;

//
// directory
//
typedef struct _HMAP_DIRECTORY
{
	//
	// map tables
	//
	PHMAP_TABLE											Directory[HDIRECTORY_SLOTS];
}HMAP_DIRECTORY,*PHMAP_DIRECTORY;

//
// hive
//
typedef struct _HHIVE
{
	//
	// signature
	//
	ULONG												Signature;

	//
	// get cell routine
	//
	PGET_CELL_ROUTINE									GetCellRoutine;

	//
	// release cell routine
	//
	PRELEASE_CELL_ROUTINE								ReleaseCellRoutine;

	//
	// allocate routine
	//
	PALLOCATE_ROUTINE									Allocate;

	//
	// free routine
	//
	PFREE_ROUTINE										Free;

	//
	// set size routine
	//
	PFILE_SET_SIZE_ROUTINE								FileSetSize;

	//
	// write routine
	//
	PFILE_WRITE_ROUTINE									FileWrite;

	//
	// read routine
	//
	PFILE_READ_ROUTINE									FileRead;

	//
	// flush routine
	//
	PFILE_FLUSH_ROUTINE									FileFlush;

	//
	// base block
	//
	PHBASE_BLOCK										BaseBlock;

	//
	// dirty vector
	//
	RTL_BITMAP											DirtyVector;

	//
	// dirty count
	//
	ULONG												DirtyCount;

	//
	// allocated bytes for dirty vect
	//
	ULONG												DirtyAlloc;

	//
	// offset 38
	//
	ULONG												Offset38;

	//
	// cluster size
	//
	ULONG												Cluster;

	//
	// flat hive
	//
	BOOLEAN												Flat;

	//
	// read only hive
	//
	BOOLEAN												ReadOnly;

	//
	// log file
	//
	BOOLEAN												Log;

	//
	// alternate file
	//
	BOOLEAN												Alternate;

	//
	// flags
	//
	ULONG												HiveFlags;

	//
	// log size
	//
	ULONG												LogSize;

	//
	// refresh count
	//
	ULONG												RefreshCount;

	//
	// storage type count
	//
	ULONG												StorageTypeCount;

	//
	// version
	//
	ULONG												Version;

	//
	// dual
	//
	struct _DUAL
	{
		//
		// length
		//
		ULONG											Length;

		//
		// map
		//
		PHMAP_DIRECTORY									Map;

		//
		// small map table
		//
		PHMAP_TABLE										SmallDir;

		//
		// always == -1
		//
		LONG											Guard;

		//
		// free display
		//
		RTL_BITMAP										FreeDisplay[HHIVE_FREE_DISPLAY_SIZE];

		//
		// free summary
		//
		ULONG											FreeSummary;

		//
		// list of freed HBINs (FREE_HBIN)
		//
		LIST_ENTRY										FreeBins;
	}Storage[HTYPE_COUNT];
}HHIVE,*PHHIVE;

//
// cm hive
//
typedef struct _CMHIVE
{
	//
	// hive
	//
	HHIVE												Hive;

	//
	// file handles
	//
	HANDLE												FileHandles[HFILE_TYPE_MAX];

	//
	// notify list
	//
	LIST_ENTRY											NotifyList;

	//
	// number of KeyControlBlocks currently
	//
	ULONG												KcbCount;

	//
	// open on this hive.
	//
	LIST_ENTRY											HiveList;
}CMHIVE,*PCMHIVE;

//
// free bin
//
typedef struct _FREE_HBIN
{
	//
	// list entry
	//
	LIST_ENTRY											ListEntry;

	//
	// size
	//
	ULONG												Size;

	//
	// file offset
	//
	ULONG												FileOffset;

	//
	// flags
	//
	ULONG												Flags;
}FREE_HBIN,*PFREE_HBIN;

#include <pshpack4.h>

//
// child list
//
typedef struct _CHILD_LIST
{
	//
	// count
	//
	ULONG												Count;

	//
	// list
	//
	HCELL_INDEX											List;
}CHILD_LIST,*PCHILD_LIST;

//
// key reference
//
typedef struct _CM_KEY_REFERENCE
{
	//
	// hive
	//
	PHHIVE												KeyHive;

	//
	// cell
	//
	HCELL_INDEX											KeyCell;
}CM_KEY_REFERENCE,*PCM_KEY_REFERENCE;

//
// key node
//
typedef struct _CM_KEY_NODE
{
	//
	// 0x0000,signature
	//
	USHORT												Signature;

	//
	// 0x0002,flags
	//
	USHORT												Flags;

	//
	// 0x0004,last write time
	//
	LARGE_INTEGER										LastWriteTime;

	//
	// 0x000c,spare
	//
	ULONG												Spare;

	//
	// 0x0010,parent
	//
	HCELL_INDEX											Parent;

	//
	// 0x0014,sub key counts
	//
	ULONG												SubKeyCounts[HTYPE_COUNT];

	//
	// 0x001c,sub key lists
	//
	HCELL_INDEX											SubKeyLists[HTYPE_COUNT];

	//
	// 0x0024,value list
	//
	CHILD_LIST											ValueList;

	//
	// 0x002c,u1
	//
	union
	{
		struct
		{
			//
			// 0x002c,security
			//
			HCELL_INDEX									Security;

			//
			// 0x0030,class
			//
			HCELL_INDEX									Class;
		}s1;

		//
		// 0x002c,reference
		//
		CM_KEY_REFERENCE								ChildHiveReference;
	}u1;

	//
	// 0x0034,max name length
	//
	ULONG												MaxNameLen;

	//
	// 0x0038,max class length
	//
	ULONG												MaxClassLen;

	//
	// 0x003c,max value name length
	//
	ULONG												MaxValueNameLen;

	//
	// 0x0040,max value data length
	//
	ULONG												MaxValueDataLen;

	//
	// 0x0044,temp work
	//
	ULONG												WorkVar;

	//
	// 0x0048,name length
	//
	USHORT												NameLength;

	//
	// 0x004a,class length
	//
	USHORT												ClassLength;

	//
	// 0x004c,var-length name
	//
	WCHAR												Name[1];
}CM_KEY_NODE,*PCM_KEY_NODE;

//
// value node
//
typedef struct _CM_KEY_VALUE
{
	//
	// 0x0000,signature
	//
	USHORT												Signature;

	//
	// 0x0002,name length
	//
	USHORT												NameLength;

	//
	// 0x0004,data length
	//
	ULONG												DataLength;

	//
	// 0x0008,data
	//
	HCELL_INDEX											Data;

	//
	// 0x000c,type
	//
	ULONG												Type;

	//
	// 0x0010,flags
	//
	USHORT												Flags;

	//
	// 0x0012,spare
	//
	USHORT												Spare;

	//
	// 0x0014,var-length name
	//
	WCHAR												Name[1];
}CM_KEY_VALUE,*PCM_KEY_VALUE;

//
// key index
//
typedef struct _CM_KEY_INDEX
{
	//
	// signature
	//
	USHORT												Signature;

	//
	// count
	//
	USHORT												Count;

	//
	// list array
	//
	HCELL_INDEX											List[1];
}CM_KEY_INDEX,*PCM_KEY_INDEX;

//
// cm index
//
typedef struct _CM_INDEX
{
	//
	// cell
	//
	HCELL_INDEX											Cell;

	union
	{
		//
		// upcased first four chars of name
		//
		UCHAR											NameHint[4];

		//
		// hash value
		//
		ULONG											NameHash;
	};
}CM_INDEX,*PCM_INDEX;

//
// fast index
//
typedef struct _CM_KEY_FAST_INDEX
{
	//
	// signature
	//
	USHORT												Signature;

	//
	// count
	//
	USHORT												Count;

	//
	// list array
	//
	CM_INDEX											List[1];
}CM_KEY_FAST_INDEX,*PCM_KEY_FAST_INDEX;

//
// security data
//
typedef struct _CM_KEY_SECURITY
{
	//
	// signature
	//
	USHORT												Signature;

	//
	// reserved
	//
	USHORT												Reserved;

	//
	// next
	//
	HCELL_INDEX											Flink;

	//
	// prev
	//
	HCELL_INDEX											Blink;

	//
	// reference count
	//
	ULONG												ReferenceCount;

	//
	// length
	//
	ULONG												DescriptorLength;
	//
	// Variable length descriptor
	//
}CM_KEY_SECURITY,*PCM_KEY_SECURITY;

//
// big data
//
typedef struct _CM_KEY_BIG_DATA
{
	//
	// signature
	//
	USHORT												Signature;

	//
	// count
	//
	USHORT												Count;

	//
	// data list
	//
	HCELL_INDEX											DataList;
}CM_KEY_BIG_DATA,*PCM_KEY_BIG_DATA;

//
// cell data
//
typedef struct _CELL_DATA
{
	union _u
	{
		//
		// key node
		//
		CM_KEY_NODE										KeyNode;

		//
		// value node
		//
		CM_KEY_VALUE									KeyValue;

		//
		// key index
		//
		CM_KEY_INDEX									KeyIndex;

		//
		// key list
		//
		HCELL_INDEX										KeyList[1];

		//
		// key string
		//
		WCHAR											KeyString[1];

		//
		// big data
		//
		CM_KEY_BIG_DATA									KeyBigData;

		//
		// security
		//
		CM_KEY_SECURITY									KeySecurity;
	}u;
}CELL_DATA,*PCELL_DATA;

//
// cell
//
typedef struct _HCELL
{
	//
	// size
	//
	LONG												Size;

	//
	// two formats
	//
	union
	{
		//
		// old format
		//
		struct
		{
			//
			// last
			//
			ULONG										Last;

			//
			// data or next
			//
			union
			{
				//
				// user data
				//
				ULONG									UserData;

				//
				// offset of next element in freelist (not a FLink)
				//
				HCELL_INDEX								Next;
			}u;
		}OldCell;

		//
		// new format
		//
		struct
		{
			//
			// data or next
			//
			union
			{
				//
				// user data
				//
				ULONG									UserData;

				//
				// offset of next element in freelist (not a FLink)
				//
				HCELL_INDEX								Next;
			}u;
		}NewCell;
	}u;
}HCELL,*PHCELL;

//
// bin
//
typedef struct  _HBIN
{
	//
	// signature
	//
	ULONG												Signature;

	//
	// offset
	//
	ULONG												FileOffset;

	//
	// size
	//
	ULONG												Size;

	//
	// reserved
	//
	ULONG												Reserved1[2];

	//
	// timestamp
	//
	LARGE_INTEGER										TimeStamp;

	//
	// memory alloc
	//
	ULONG												MemAlloc;
}HBIN,*PHBIN;

#include <poppack.h>

//
// hardware profile
//
typedef struct _CM_HARDWARE_PROFILE
{
	//
	// name length,offset = 0
	//
	ULONG												NameLength;

	//
	// friendly name,offset = 4
	//
	PWSTR												FriendlyName;

	//
	// preference order,offset = 8
	//
	ULONG												PreferenceOrder;

	//
	// id,offset = 0c
	//
	ULONG												Id;

	//
	// dock flags,offset = 10
	//
	ULONG												DockState;

}CM_HARDWARE_PROFILE,*PCM_HARDWARE_PROFILE;

//
// profile list,sizeof = 0x1c
//
typedef struct _CM_HARDWARE_PROFILE_LIST
{
	//
	// max count,offset = 0
	//
	ULONG												MaxProfileCount;

	//
	// current count,offset = 4
	//
	ULONG												CurrentProfileCount;

	//
	// profile array,offset = 8
	//
	CM_HARDWARE_PROFILE									Profile[1];

}CM_HARDWARE_PROFILE_LIST,*PCM_HARDWARE_PROFILE_LIST;

//
// dock info
//
typedef struct _CM_HARDWARE_DOCK_INFO
{
	//
	// profile number
	//
	ULONG												ProfileNumber;

	//
	// dock state
	//
	ULONG												DockState;

	//
	// dock id
	//
	ULONG												DockID;

	//
	// serial number
	//
	ULONG												SerialNumber;
}CM_HARDWARE_DOCK_INFO,*PCM_HARDWARE_DOCK_INFO;

//
// dock info list
//
typedef struct _CM_HARDWARE_DOCK_INFO_LIST
{
	//
	// max count,offset = 0
	//
	ULONG												MaxCount;

	//
	// current count,offset = 4
	//
	ULONG												CurrentCount;

	//
	// info array
	//
	CM_HARDWARE_DOCK_INFO								DockInfo[1];
}CM_HARDWARE_DOCK_INFO_LIST,*PCM_HARDWARE_DOCK_INFO_LIST;

//
// check hive debug
//
typedef struct _CHECK_HIVE_DEBUG
{
	//
	// hive
	//
	PHHIVE												Hive;

	//
	// status
	//
	ULONG												Status;

	//
	// length
	//
	ULONG												Space;

	//
	// map point
	//
	HCELL_INDEX											MapPoint;

	//
	// bin point
	//
	PHBIN												BinPoint;
}CHECK_HIVE_DEBUG,*PCHECK_HIVE_DEBUG;

//
// check registry debug
//
typedef struct _CHECK_REGISTRY_DEBUG
{
	//
	// hive
	//
	PHHIVE												Hive;

	//
	// status
	//
	ULONG												Status;
}CHECK_REGISTRY_DEBUG,*PCHECK_REGISTRY_DEBUG;

//
// check bin debug
//
typedef struct _CHECK_BIN_DEBUG
{
	//
	// bin
	//
	PHBIN												Bin;

	//
	// status
	//
	ULONG												Status;

	//
	// cell
	//
	PHCELL												CellPoint;
}CHECK_BIN_DEBUG,*PCHECK_BIN_DEBUG;

//
// check key debug
//
typedef struct _CHECK_KEY_DEBUG
{
	//
	// hive
	//
	PHHIVE												Hive;

	//
	// status
	//
	ULONG												Status;

	//
	// key cell
	//
	HCELL_INDEX											Cell;

	//
	// error point
	//
	PCELL_DATA											CellPoint;

	//
	// root index
	//
	PVOID												RootPoint;

	//
	// error index
	//
	ULONG												Index;
}CHECK_KEY_DEBUG,*PCHECK_KEY_DEBUG;

typedef struct _CHECK_VALUE_LIST_DEBUG
{
	//
	// hive
	//
	PHHIVE												Hive;

	//
	// status
	//
	ULONG												Status;

	//
	// value list
	//
	PCELL_DATA											List;

	//
	// error index
	//
	ULONG												Index;

	//
	// cell
	//
	HCELL_INDEX											Cell;

	//
	// cell data
	//
	PCELL_DATA											CellPoint;

}CHECK_VALUE_LIST_DEBUG,*PCHECK_VALUE_LIST_DEBUG;

//
// initialize hive
//
NTSTATUS HvInitializeHive(__in PHHIVE Hive,__in ULONG OperationType,__in ULONG HiveFlags,__in ULONG FileTypes,__in_opt PVOID HiveData,
						  __in PALLOCATE_ROUTINE AllocateRoutine,__in PFREE_ROUTINE FreeRoutine,__in PFILE_SET_SIZE_ROUTINE FileSetSizeRoutine,
						  __in PFILE_WRITE_ROUTINE FileWriteRoutine,__in PFILE_READ_ROUTINE FileReadRoutine,__in PFILE_FLUSH_ROUTINE FileFlushRoutine,
						  __in ULONG Cluster,__in PUNICODE_STRING FileName);

//
// recover hive
//
BOOLEAN BlRecoverHive(__in PVOID DstBaseBlock,__in PVOID SrcBaseBlock);

//
// check registry
//
ULONG CmCheckRegistry(__in PCMHIVE CmHive,__in ULONG Flags);

//
// find key by name
//
HCELL_INDEX CmpFindSubKeyByName(__in PHHIVE Hive,__in PVOID Parent,__in PUNICODE_STRING SearchName);

//
// find key by number
//
HCELL_INDEX CmpFindSubKeyByNumber(__in PHHIVE Hive,__in PVOID Parent,__in ULONG SearchIndex);

//
// find value by name
//
HCELL_INDEX CmpFindValueByName(__in PHHIVE Hive,__in PVOID Parent,__in PUNICODE_STRING SearchName);

//
// value to data
//
PVOID CmpValueToData(__in PHHIVE Hive,__in PCM_KEY_VALUE Value,__out_opt PULONG DataLength);

//
// compressed name size
//
ULONG CmpCompressedNameSize(__in PWCHAR Name,__in ULONG Length);

//
// copy compressed name
//
VOID CmpCopyCompressedName(__in PWCHAR Destination,__in ULONG DestinationLength,__in PWCHAR Source,__in ULONG SourceLength);