//********************************************************************
//	created:	26:9:2008   18:09
//	file:		hiber.cpp
//	author:		tiamo
//	purpose:	hibernate
//********************************************************************

#include "stdafx.h"

#if _HIBERNATE_SUPPORT_

//
// link file
//
#define HIBER_SIGNATURE_LINK							'knil'

//
// restoring
//
#define HIBER_SIGNATURE_RESTORING						'ekaw'

//
// normal
//
#define HIBER_SIGNATURE_NORMAL							'rbih'

//
// break on wake
//
#define HIBER_SIGNATURE_BREAK_ON_WAKE					'pkrb'

//
// memory usage
//
enum
{
	//
	// free memory
	//
	HiberMemoryFree,

	//
	// insued memory
	//
	HiberMemoryInUsed,

	//
	// bad memory
	//
	HiberMemoryError,
};

//
// hiber page usage
//
enum
{
	//
	// page 0 = file header in page file
	//
	HiberPageFileImageHeader							= 0,

	//
	// page 1 = free mappings in page file
	//
	HiberPageFileFreeMappings							= 1,

	//
	// page 2 = process state in page file
	//
	HiberPageFileProcessorState							= 2,

	//
	// page 0 = temp page in hiber context
	//
	HiberPageMemoryTempPage0							= 0,

	//
	// page 1 = temp page in hiber context
	//
	HiberPageMemoryTempPage1							= 1,

	//
	// lznt1 workspace
	//
	HiberPageMemoryLZNT1Workspace						= 1,

	//
	// page 2 = remapped pfn
	//
	HiberPageMemoryRemappedMappings						= 2,

	//
	// page 2 = free pfn
	//
	HiberPageMemoryFreeMappings							= 2,

	//
	// page 3 = original pfn
	//
	HiberPageMemoryOriginalMappings						= 3,

	//
	// page 4 = processor state
	//
	HiberPageMemoryProcessorState						= 4,

	//
	// page 5 = pde
	//
	HiberPageMemoryPde									= 5,

	//
	// page 6 = hiber ptes
	//
	HiberPageMemoryPte									= 6,

	//
	// page 7 = wake dispatcher code
	//
	HiberPageMemoryWakeCode0							= 7,

	//
	// page 8 = wake dispatcher code
	//
	HiberPageMemoryWakeCode1							= 8,

	//
	// max wake code count
	//
	HiberPageMaxWakeCode								= 2,

	//
	// page 9 - page 25 = xpress decode page
	//
	HiberPageMemoryXpressWorkspace						= 9,
};

//
// hiber dispatch routine
//
typedef VOID (*PHIBER_DISPATCH_ROUTINE)();

//
// hiber file header
//
typedef struct _PO_MEMORY_IMAGE
{
	//
	// offset = 0000 signature
	//	hiber	= normal
	//	wake	= restoring
	//	link	= link to other file
	//
	ULONG												Signature;

	//
	// offset = 0004 version,must be zero
	//
	ULONG												Version;

	//
	// offset = 0008 header checksum,from Start to start + LenghSelf
	//
	ULONG												CheckSum;

	//
	// offset = 000c header length
	//
	ULONG												LengthSelf;

	//
	// offset = 0010 page frame number where the kernel's header is
	//
	ULONG												PageSelf;

	//
	// offset = 0014 page size,must be PAGE_SIZE
	//
	ULONG												PageSize;

	//
	// offset = 0018 image type
	//
	ULONG64												ImageType;

	//
	// offset = 0020 system time
	//
	LARGE_INTEGER										SystemTime;

	//
	// offset = 0028 interrupt time
	//
	ULONG64												InterruptTime;

	//
	// offset = 0030 feature flags
	//	4 = restore cr4 from kernel's processor state
	//
	ULONG												FeatureFlags;

	//
	// offset = 0034 hiber flags
	//	1 = reconnect APM
	//	2 = DEP enabled
	//
	ULONG												HiberFlags;

	//
	// offset = 0038 hiber ptes count must <= 32
	//
	ULONG												NoHiberPtes;

	//
	// offset = 003c kernel's hiber context virtual address
	//
	ULONG												HiberIdentityVa;

	//
	// offset = 0040 hiber pte
	//
	LARGE_INTEGER										HiberPte;

	//
	// offset = 0048 free pages count
	//
	ULONG												NoMappings;

	//
	// offset = 004c free map checksum
	//
	ULONG												FreeMapCheck;

	//
	// offset = 0050 wake checksum of HiberPageFileProcessorState page
	//
	ULONG												WakeCheck;

	//
	// offset = 0054 total pages
	//
	ULONG												TotalPages;

	//
	// offset = 0058 first compressed data table page
	//
	ULONG												FirstTablePage;

	//
	// offset = 005c last file page
	//
	ULONG												LastFilePage;
}PO_MEMORY_IMAGE,*PPO_MEMORY_IMAGE;

//
// range
//
typedef struct _PO_MEMORY_RANGE_ARRAY_RANGE
{
	//
	// offset = 0000 #page
	//
	ULONG												PageNo;

	//
	// offset = 0004 start page
	//
	ULONG												StartPage;

	//
	// offset = 0008 end page
	//
	ULONG												EndPage;

	//
	// offset = 000c checksum
	//
	ULONG												Checksum;
}PO_MEMORY_RANGE_ARRAY_RANGE,*PPO_MEMORY_RANGE_ARRAY_RANGE;

//
// link
//
typedef struct _PO_MEMORY_RANGE_ARRAY_LINK
{
	//
	// offset = 0000 next
	//
	union _PO_MEMORY_RANGE_ARRAY*						Next;

	//
	// offset = 0004 next table
	//
	ULONG												NextTable;

	//
	// offset = 0008 checksum
	//
	ULONG												CheckSum;

	//
	// offset = 000c entries
	//
	ULONG												EntryCount;
}PO_MEMORY_RANGE_ARRAY_LINK,*PPO_MEMORY_RANGE_ARRAY_LINK;

//
// memory range array
//
typedef union _PO_MEMORY_RANGE_ARRAY
{
	//
	// offset = 0000 range
	//
	PO_MEMORY_RANGE_ARRAY_RANGE							Range;

	//
	// offset = 0000 link
	//
	PO_MEMORY_RANGE_ARRAY_LINK							Link;
}PO_MEMORY_RANGE_ARRAY,*PPO_MEMORY_RANGE_ARRAY;

//
// xpress header,size = 0x20
//
typedef struct _HIBER_XPRESS_HEADER
{
	//
	// offset = 0000 signature
	//
	UCHAR												Signature[8];

	//
	// offset = 0008 size
	//
	UCHAR												Size[4];

	//
	// offset = 000c
	//
	UCHAR												OffsetC[2];

	//
	// offset = 000e checksum
	//
	UCHAR												CheckSum[2];

	//
	// offset = 0010
	//
	UCHAR												Reserved[16];
}HIBER_XPRESS_HEADER,*PHIBER_XPRESS_HEADER;

//
// read context
//
typedef struct _HIBER_COMPRESSED_READ_STATE
{
	//
	// offset = 0000 read position
	//
	PVOID												ReadPosition;

	//
	// offset = 0004 write position
	//
	PVOID												WritePosition;

	//
	// offset = 0008
	//
	PVOID												Offset8;

	//
	// offset = 000c
	//
	PVOID												EndOfWriteBuffer;

	//
	// offset = 0010 storage buffer
	//
	PVOID												StorageBuffer;

	//
	// offset = 0014
	//
	PVOID												StorageBufferEnd;

	//
	// offset = 0018 offset in page
	//
	ULONG												OffsetInPage;

	//
	// seek file
	//
	BOOLEAN												SeekFileBeforeRead;
}HIBER_COMPRESSED_READ_STATE,*PHIBER_COMPRESSED_READ_STATE;

//
// compression info
//
typedef struct _HIBER_DELAYED_XPRESS_INFO
{
	//
	// offset = 0000 output buffer
	//
	PVOID												OutputBuffer;

	//
	// offset = 0004 buffer
	//
	PVOID												Workspace;

	//
	// offset == 0008 output buffer size
	//
	ULONG												OutputBufferSize;

	//
	// offset = 000c xpress block size
	//
	ULONG												XpressBlockSize;

	//
	// offset = 0010 checksum in xpress header
	//
	ULONG												CheckSum;

	//
	// offset = 0014 final uncompressed size
	//
	ULONG												FinalOutputBytes;

	//
	// offset = 0018
	//
	ULONG												Offset18;

	//
	// offset = 001c is xpress compression method
	//
	BOOLEAN												IsXpressCompression;

	//
	// offset = 0020 delayed count
	//
	ULONG												DelayedCount;

	//
	// offset = 0024
	//
	ULONG												Offset24;

	//
	// offset = 0028 error flag
	//
	ULONG												ErrorFlags;

	//
	// offset = 002c delayed info
	//
	struct _DELAYED_PAGE
	{
		//
		// offset = 0000 mapped virtual address
		//
		PVOID											MappedAddress;

		//
		// offset = 0004 page frame number
		//
		ULONG											PageFrame;

		//
		// offset = 0008 entry count
		//
		ULONG											EntryCount;

		//
		// offset = 000c
		//
		ULONG											Offset0C;
	}DelayedPages[0x10];
}HIBER_DELAYED_XPRESS_STATE,*PHIBER_DELAYED_XPRESS_STATE;

#include <pshpack1.h>
//
// wake context
//
typedef struct _HIBER_WAKE_CONTEXT
{
	//
	// offset = 0000 first page
	//
	UCHAR												TempPage0[PAGE_SIZE];

	//
	// offset = 1000 2nd page
	//
	UCHAR												TempPage1[PAGE_SIZE];

	//
	// offset = 2000 remapped pfn
	//
	ULONG												RemappedPageFrame[PAGE_SIZE / sizeof(ULONG)];

	//
	// offset = 3000 original pfn
	//
	ULONG												OriginalPageFrame[PAGE_SIZE / sizeof(ULONG)];

	//
	// offset = 4000 processor state
	//
	KPROCESSOR_STATE									ProcessorState;

	//
	// offset = 4320 caller esp
	//
	ULONG												SavedEsp;

	//
	// offset = 4324 hiber ptes
	//
	PHARDWARE_PTE										HiberPtes;

	//
	// offset = 4328 pde page frame
	//
	ULONG												PdePageFrame;

	//
	// offset = 432c hiber ptes address under hiber page table
	//
	ULONG												HiberTransVa;

	//
	// offset = 4330 hiber context virtual address under both hiber's and kernel's page table
	//
	ULONG												HiberIdentityVa;

	//
	// offset = 4334 heap
	//
	ULONG												HiberHeap;

	//
	// offset = 4338 first index of remapped data page in new/old pfn
	//
	ULONG												FirstRemap;

	//
	// offset = 433c last index of remapped data page
	//
	ULONG												LastRemap;

	//
	// offset = 4340 flags
	//	4 = restore cr4 from kernel's processor state
	//
	ULONG												ImageFeatureFlags;

	//
	// offset = 4344 pae is enabled
	//
	ULONG												UsePae;

	//
	// offset = 4348 dep is enabled
	//
	ULONG												NoExecute;

	//
	// offset = 434c gdt
	//
	USHORT												GdtLimit;

	//
	// offset = 434e gdt base
	//
	ULONG												GdtBase;

	//
	// offset = 4352 hiber padding
	//
	USHORT												Reserved;

	//
	// offset = 4354 hiber stack buffer
	//
	UCHAR												StackBuffer[1024];

	//
	// offset = 4754 the remaining is heap space,up to page bound
	//
	UCHAR												HeapBuffer[2220];

	//
	// offset = 5000 pde page
	//
	HARDWARE_PTE										HiberPdePage[PAGE_SIZE / sizeof(HARDWARE_PTE)];

	//
	// offset = 6000 pte page
	//
	HARDWARE_PTE										HiberPtePage[PAGE_SIZE / sizeof(HARDWARE_PTE)];

	//
	// offset = 7000 wake code buffer
	//
	UCHAR												WakeCodeBuffer[HiberPageMaxWakeCode << PAGE_SHIFT];
}HIBER_WAKE_CONTEXT,*PHIBER_WAKE_CONTEXT;
#include <poppack.h>

//
// current screen
//
ULONG													HbCurrentScreen;

//
// break on wake
//
BOOLEAN													HiberBreakOnWake;

//
// io error
//
BOOLEAN													HiberIoError;

//
// abort
//
BOOLEAN													HiberAbort;

//
// hiber file id
//
ULONG													HiberFile;

//
// temp buffer
//
PVOID													HiberBuffer;

//
// temp buffer page frame
//
ULONG													HiberBufferPage;

//
// image feature flags
//
ULONG													HiberImageFeatureFlags;

//
// free mappings count
//
ULONG													HiberNoMappings;

//
// hiber virtual address under hiber/kernel's page table
//
ULONG													HiberIdentityVa;

//
// virtual address of hiber ptes under hiber/kernel's page table
//
ULONG													HiberTransVa;

//
// header page frame number
//
ULONG													HiberImagePageSelf;

//
// hiber va under osloader's page table
//
ULONG													HiberVa;

//
// hiber ptes
//
PHARDWARE_PTE											HiberPtes;

//
// current map index
//
ULONG													HiberCurrentMapIndex;

//
// first remap
//
ULONG													HiberFirstRemap;

//
// last remap
//
ULONG													HiberLastRemap;

//
// run out of remap flags
//
BOOLEAN													HiberOutOfRemap;

//
// page frame number database
//
ULONG													HiberPageFrames[32];

//
// no execute(DEP) is enabled
//
BOOLEAN													HiberNoExecute;

//
// hiber file name
//
CHAR													HiberFileName[] = "\\hiberfil.sys";

//
// print
//
VOID HbPrint(__in PCHAR Message)
{
	if(!Message)
		return;

	ULONG Count;
	ArcWrite(BlConsoleOutDeviceId,Message,strlen(Message),&Count);
}

//
// print message
//
VOID HbPrintMsg(__in ULONG MessageId)
{
	PCHAR Message										= BlFindMessage(MessageId);
	if(Message)
		HbPrint(Message);
}

//
// show hiber screen
//
VOID HbScreen(__in ULONG ScreenMessageId)
{
	HbCurrentScreen										= ScreenMessageId;
	BlSetInverseMode(FALSE);
	BlPositionCursor(1,1);
	BlClearToEndOfScreen();
	BlPositionCursor(1,3);
	HbPrintMsg(ScreenMessageId);
}

//
// get selection
//
ULONG HbSelection(__in ULONG StartX,__in ULONG StartY,__in PULONG MessageIdArray)
{
	//
	// count items
	//
	ULONG Count											= 0;
	while(MessageIdArray[Count])
		Count											+= 1;

	if(!Count)
		return 0;

	BOOLEAN RedrawMenu									= TRUE;
	ULONG CurrentSelection								= 0;
	while(1)
	{
		if(RedrawMenu)
		{
			//
			// redraw menu
			//
			for(ULONG i = 0; i < Count; i ++)
			{
				BlPositionCursor(StartX,StartY + i);
				BlSetInverseMode(CurrentSelection == i);
				HbPrintMsg(MessageIdArray[i]);
			}

			RedrawMenu									= FALSE;
		}

		//
		// read input
		//
		ULONG Key										= BlGetKey();
		RedrawMenu										= TRUE;
		switch(Key)
		{
		case UP_ARROW:
			CurrentSelection							= (CurrentSelection + Count - 1) % Count;
			break;

		case DOWN_ARROW:
			CurrentSelection							= (CurrentSelection + 1) % Count;
			break;

		case HOME_KEY:
			CurrentSelection							= 0;
			break;

		case END_KEY:
			CurrentSelection							= Count - 1;
			break;

		case F10_KEY:
			DbgBreakPoint();
			break;

		default:
			RedrawMenu									= FALSE;
			break;
		}

		if(Key == '\r' || Key == '\n')
			break;
	}

	BlSetInverseMode(FALSE);
	BlPositionCursor(1,2);
	BlClearToEndOfScreen();
	if(MessageIdArray[CurrentSelection] == BL_CONTINUE_DBG_BREAK_ON_WAKE)
		HiberBreakOnWake								= TRUE;

	return CurrentSelection;
}

//
// check for pause
//
VOID HbCheckForPause()
{
	ULONG Key											= BlGetKey();
	if(Key == ' ' || Key == F5_KEY || Key == F8_KEY)
	{
		HbScreen(BL_SYSTEM_RESTART_PAUSED);

		static ULONG Selections[]						= {BL_CONTINUE_WITH_RESTART,BL_DELETE_RESTORATION_DATA_AND_DISPLAY_BOOT_MENU,BL_CONTINUE_DBG_BREAK_ON_WAKE,0};
		ULONG Selection									= HbSelection(7,7,Selections);
		if(Selection == 1)
		{
			HiberIoError								= TRUE;
			HiberAbort									= TRUE;
		}
		else
		{
			BlSetInverseMode(FALSE);
			BlOutputStartupMsg(BL_RESUMING_WINDOWS);
			BlOutputTrailerMsg(BL_GOTO_ADVANCED_BOOT_F8);
		}
	}
}

//
// read one page
//
VOID HbReadPage(__in ULONG Offset,__in PVOID Buffer)
{
	LARGE_INTEGER LargeOffset;
	LargeOffset.QuadPart								= Offset << PAGE_SHIFT;

	if(ESUCCESS != BlSeek(HiberFile,&LargeOffset,SeekAbsolute))
		HiberIoError									= TRUE;

	ULONG Count;
	if(ESUCCESS != BlRead(HiberFile,Buffer,PAGE_SIZE,&Count))
		HiberIoError									= TRUE;
}

//
// allocate aligned heap buffer
//
PVOID HbAllocateAlignedHeap(__in ULONG Length)
{
	PVOID Buffer										= BlAllocateHeap(Length + PAGE_SIZE - 1);
	if(!Buffer)
		return 0;

	return reinterpret_cast<PVOID>(ROUND_TO_PAGES(Buffer));
}

//
// set image signature
//
ARC_STATUS HbSetImageSignature(__in ULONG Signature)
{
	LARGE_INTEGER Offset								= {0};
	ARC_STATUS Status									= BlSeek(HiberFile,&Offset,SeekAbsolute);
	if(Status != ESUCCESS)
		return Status;

	ULONG Count;
	return BlWrite(HiberFile,&Signature,sizeof(Signature),&Count);
}

//
// allocate contiguous ptes
//
VOID HbAllocatePtes(__in ULONG PageCount,__out PHARDWARE_PTE* Ptes,__out PULONG VirtualAddress)
{
	PHARDWARE_PTE Pte									= HalPT;
	ULONG Index											= 0;
	ULONG LeftCount										= 1024 - PageCount;
	while(PageCount)
	{
		ULONG FreeCount									= 0;
		PHARDWARE_PTE CurrentPte						= Pte;
		while(FreeCount < PageCount)
		{
			if(CurrentPte->Valid)
				break;

			FreeCount									+= 1;
			CurrentPte									+= 1;
		}

		if(FreeCount == PageCount)
			break;

		Index											+= 1;
		Pte												+= 1;

		if(Index > LeftCount)
		{
			BlPrint("NoMem");
			while(1);
		}
	}

	*Ptes												= HalPT + Index;
	*VirtualAddress										= HAL_PAGE_BASE + (Index << PAGE_SHIFT);
}

//
// set pte
//
VOID HbSetPte(__in ULONG VirtualAddress,__in PHARDWARE_PTE Ptes,__in ULONG Index,__in ULONG PageFrame)
{
	PHARDWARE_PTE Pte									= Ptes + Index;
	Pte->WriteThrough									= FALSE;
	Pte->CacheDisable									= FALSE;
	Pte->PageFrameNumber								= PageFrame;
	Pte->Valid											= TRUE;
	Pte->Write											= TRUE;

	__asm
	{
		mov		eax,[VirtualAddress]
		invlpg	[eax]
	}
}

//
// map pte
//
PVOID HbMapPte(__in ULONG Index,__in ULONG PageFrame)
{
	ULONG VirtualAddress								= HiberVa + (Index << PAGE_SHIFT);
	HbSetPte(VirtualAddress,HiberPtes,Index,PageFrame);
	return reinterpret_cast<PVOID>(VirtualAddress);
}

//
// get page disposition
//
ULONG HbPageDisposition(__in ULONG PageFrame)
{
	static PMEMORY_ALLOCATION_DESCRIPTOR HintDescriptor	= 0;
	PMEMORY_ALLOCATION_DESCRIPTOR Check					= HintDescriptor;

	if(!Check || PageFrame < Check->BasePage || PageFrame >= Check->BasePage + Check->PageCount)
	{
		PLIST_ENTRY NextEntry							= BlLoaderBlock->MemoryDescriptorListHead.Flink;
		while(NextEntry != &BlLoaderBlock->MemoryDescriptorListHead)
		{
			PMEMORY_ALLOCATION_DESCRIPTOR Current		= CONTAINING_RECORD(NextEntry,MEMORY_ALLOCATION_DESCRIPTOR,ListEntry);

			//
			// input page is in this range
			//
			if(Current && PageFrame >= Current->BasePage && PageFrame < Current->BasePage + Current->PageCount)
			{
				Check									= Current;
				break;
			}

			HintDescriptor								= Current;
			NextEntry									= NextEntry->Flink;
		}
	}

	//
	// bad memory
	//
	if(!Check || Check->MemoryType == LoaderBad)
		return HiberMemoryError;

	//
	// free memory
	//
	if(Check->MemoryType == LoaderFree || Check->MemoryType == LoaderReserved)
		return HiberMemoryFree;

	//
	// also free,because osloader will never use memory above 16MB
	//
	if(Check->MemoryType == LoaderFirmwareTemporary && PageFrame > (16 * 1024 * 1024 >> PAGE_SHIFT))
		return HiberMemoryFree;

	//
	// otherwise it is used
	//
	return HiberMemoryInUsed;
}

//
// next shared page
//
PVOID HbNextSharedPage(__in ULONG Index,__in ULONG OriginalPageFrame)
{
	PULONG RemappedPageFrameArray						= Add2Ptr(HiberVa,HiberPageMemoryRemappedMappings << PAGE_SHIFT,PULONG);
	PULONG FreePageFrameArray							= Add2Ptr(HiberVa,HiberPageMemoryFreeMappings << PAGE_SHIFT,PULONG);
	PULONG OriginalPageFrameArray						= Add2Ptr(HiberVa,HiberPageMemoryOriginalMappings << PAGE_SHIFT,PULONG);
	while(HiberCurrentMapIndex < HiberNoMappings)
	{
		//
		// free page frame array contains pages whose are not used by hibernated system
		//
		ULONG PageFrame									= FreePageFrameArray[HiberCurrentMapIndex];

		//
		// advance to next one
		//
		HiberCurrentMapIndex							+= 1;

		//
		// case on usage
		//
		switch(HbPageDisposition(PageFrame))
		{
			//
			// bad memory
			//
		case HiberMemoryError:
			HiberIoError								= TRUE;
			return HiberBuffer;
			break;

			//
			// this page is free,record it and map it in HiberPtes
			//
		case HiberMemoryFree:
			//
			// record in remapped pfn
			//
			RemappedPageFrameArray[HiberLastRemap]		= PageFrame;

			//
			// aslo record in original pfn
			//
			OriginalPageFrameArray[HiberLastRemap]		= OriginalPageFrame;

			//
			// advance it
			//
			HiberLastRemap								+= 1;

			//
			// and record in hiber's pfn database
			//
			HiberPageFrames[Index]						= PageFrame;

			//
			// map it and return a virtual address
			//
			return HbMapPte(Index,PageFrame);
			break;

		default:
			break;
		}
	}

	HiberOutOfRemap										= TRUE;

	return HiberBuffer;
}

//
// read next compressed page
//
BOOLEAN HbReadNextCompressedPages(__in ULONG ReadBytesCount,__in PHIBER_COMPRESSED_READ_STATE ReadState)
{
	//
	// check bytes count in current buffer
	//
	ULONG AlreadyInBuffer								= PtrOffset(ReadState->WritePosition,ReadState->ReadPosition,ULONG);
	if(AlreadyInBuffer >= ReadBytesCount)
		return TRUE;

	//
	// those bytes we need to read up to
	//
	ReadBytesCount										-= AlreadyInBuffer;

	//
	// and round up to page size
	//
	LONG RoundReadCount									= ROUND_TO_PAGES(ReadBytesCount);

	//
	// reset those pointers if current buffer is empty,otherwise move it
	//
	if(AlreadyInBuffer)
	{
		LONG Count										= PtrOffset(ReadState->StorageBuffer,ReadState->Offset8,LONG) & ~(PAGE_SIZE - 1);
		LONG Temp										= ROUND_TO_PAGES(AlreadyInBuffer);
		if(Count > Temp)
			Count										= Temp;

		//
		// round current length to 64 bytes
		//
		ULONG MoveLength								= ROUND_UP(AlreadyInBuffer,64);

		//
		// place final write position to a page bound
		//
		PUCHAR DstBuffer								= static_cast<PUCHAR>(ReadState->StorageBuffer) - MoveLength - Count + Temp;
		PUCHAR SrcBuffer								= static_cast<PUCHAR>(ReadState->WritePosition) - MoveLength;

		if(DstBuffer != SrcBuffer)
		{
			RtlMoveMemory(DstBuffer,SrcBuffer,MoveLength);
			LONG Diff									= PtrOffset(DstBuffer,SrcBuffer,LONG);
			ReadState->WritePosition					= Add2Ptr(ReadState->WritePosition,Diff,PVOID);
			ReadState->ReadPosition						= Add2Ptr(ReadState->ReadPosition,Diff,PVOID);
		}
	}
	else
	{
		ReadState->ReadPosition							= ReadState->StorageBuffer;
		ReadState->WritePosition						= ReadState->StorageBuffer;
	}

	//
	// wrap and left size check
	//
	LONG Offset											= reinterpret_cast<LONG>(ReadState->WritePosition) & 0xffff;
	LONG Count											= Offset - reinterpret_cast<LONG>(ReadState->WritePosition);
	LONG Count2											= PtrOffset(ReadState->EndOfWriteBuffer,ReadState->WritePosition,LONG);
	if(Count > Count2)
		Count											= Count2;

	if(Count > RoundReadCount)
		RoundReadCount									= Count;

	if(RoundReadCount > Count2)
		return FALSE;

	//
	// seek file first
	//
	ARC_STATUS Status									= ESUCCESS;
	if(ReadState->SeekFileBeforeRead)
	{
		LARGE_INTEGER Offset;
		Offset.QuadPart									= ReadState->OffsetInPage << PAGE_SHIFT;
		Status											= BlSeek(HiberFile,&Offset,SeekAbsolute);
		if(Status == ESUCCESS)
			ReadState->SeekFileBeforeRead				= FALSE;
	}

	//
	// read it from file
	//
	ULONG ActualReadCount								= 0;
	if(Status == ESUCCESS)
		Status											= BlRead(HiberFile,ReadState->WritePosition,static_cast<ULONG>(RoundReadCount),&ActualReadCount);

	//
	// seek/read error
	// length is not page size aligned
	// read length < wanted length
	//
	if(Status != ESUCCESS ||  ActualReadCount & (PAGE_SIZE - 1) || ActualReadCount < ReadBytesCount)
	{
		HiberIoError									= TRUE;
		return FALSE;
	}

	//
	// update write position and file offset
	//
	ReadState->WritePosition							= Add2Ptr(ReadState->WritePosition,ActualReadCount,PVOID);
	ReadState->OffsetInPage								+= (ActualReadCount >> PAGE_SHIFT);

	return TRUE;
}

//
// read compressed header
//
BOOLEAN HbReadNextCompressedBlockHeader(__in PHIBER_DELAYED_XPRESS_STATE DelayedState,__in PHIBER_COMPRESSED_READ_STATE ReadState)
{
	//
	// get xpress header
	//
	if(!HbReadNextCompressedPages(sizeof(HIBER_XPRESS_HEADER),ReadState))
		return FALSE;

	PHIBER_XPRESS_HEADER XpressHeader					= static_cast<PHIBER_XPRESS_HEADER>(ReadState->ReadPosition);

	//
	// check signature "=xpress"
	//
	static UCHAR Signature[8]							= {0x81,0x81,0x78,0x70,0x72,0x65,0x73,0x73};
	DelayedState->IsXpressCompression					= memcmp(&XpressHeader->Signature,Signature,sizeof(Signature)) == 0 ? TRUE : FALSE;
	if(!DelayedState->IsXpressCompression)
		return TRUE;

	//
	// calc block size
	//
	ULONG Size											=  (XpressHeader->Size[0] <<  0) + (XpressHeader->Size[1] <<  8);
	Size												+= (XpressHeader->Size[2] << 16) + (XpressHeader->Size[3] << 24);
	DelayedState->XpressBlockSize						=  (Size >> 10) + 1;
	DelayedState->FinalOutputBytes						=  ((Size & 0x3ff) + 1) << PAGE_SHIFT;

	//
	// save checksum and clear it to prepare checksum calc
	//
	DelayedState->CheckSum								= (XpressHeader->CheckSum[0] << 0) + (XpressHeader->CheckSum[1] << 8);
	XpressHeader->CheckSum[0]							= 0;
	XpressHeader->CheckSum[1]							= 0;

	//
	// verify it
	//
	ULONG MaxSize										= 16 << PAGE_SHIFT;
	if(DelayedState->FinalOutputBytes > MaxSize || DelayedState->XpressBlockSize > MaxSize || !DelayedState->XpressBlockSize || !DelayedState->FinalOutputBytes)
		return FALSE;

	return TRUE;
}

//
// read next compressed block
//
BOOLEAN HbReadNextCompressedBlock(__in PHIBER_DELAYED_XPRESS_STATE DelayedState,__in PHIBER_COMPRESSED_READ_STATE ReadState)
{
	//
	// get header
	//
	if(!HbReadNextCompressedBlockHeader(DelayedState,ReadState))
		return FALSE;

	//
	// is not xpress compressione
	//
	if(!DelayedState->IsXpressCompression)
		return FALSE;

	//
	// caller does not setup output info correctly,fix it
	//
	if(DelayedState->FinalOutputBytes != DelayedState->OutputBufferSize)
	{
		DelayedState->OutputBufferSize					= DelayedState->FinalOutputBytes;
		DelayedState->OutputBuffer						= DelayedState->Workspace;
	}

	//
	// read compressed pages
	//
	ULONG CompressedDataLength							= ROUND_UP(DelayedState->XpressBlockSize,8) + sizeof(HIBER_XPRESS_HEADER);
	if(!HbReadNextCompressedPages(CompressedDataLength,ReadState))
		return FALSE;

	//
	// update read position to indicate we have read the compressed data
	//
	PVOID CompressedData								= ReadState->ReadPosition;
	ReadState->ReadPosition								= Add2Ptr(ReadState->ReadPosition,CompressedDataLength,PVOID);

	//
	// checksum
	//
	if(DelayedState->CheckSum && ((DelayedState->CheckSum ^ tcpxsum(0,CompressedData,CompressedDataLength)) & 0xffff))
		return FALSE;

	//
	// did not compressed
	//
	if(DelayedState->FinalOutputBytes == DelayedState->XpressBlockSize)
	{
		DelayedState->OutputBuffer						= Add2Ptr(CompressedData,sizeof(HIBER_XPRESS_HEADER),PVOID);
		return TRUE;
	}

	//
	// decode it
	//
	PUCHAR InputData									= Add2Ptr(CompressedData,sizeof(HIBER_XPRESS_HEADER),PUCHAR);
	ULONG InputDataLength								= DelayedState->XpressBlockSize;
	PUCHAR OutputData									= static_cast<PUCHAR>(DelayedState->OutputBuffer);
	ULONG OutputDataLength								= DelayedState->OutputBufferSize;
	ULONG Ret											= XpressDecode(InputData,InputDataLength,OutputData,OutputDataLength);
	return Ret == OutputDataLength ? TRUE : FALSE;
}

//
// read delayed block
//
BOOLEAN HbReadDelayedBlock(__in BOOLEAN FlushDelayedBlocks,__in ULONG PageFrame,__in ULONG EntryCount,
						   __in PHIBER_DELAYED_XPRESS_STATE DelayedState,__in PHIBER_COMPRESSED_READ_STATE ReadState)
{
	BOOLEAN Ret											= FALSE;
	if(!FlushDelayedBlocks)
	{
		//
		// read header first
		//
		if(!DelayedState->DelayedCount)
		{
			Ret											= HbReadNextCompressedBlockHeader(DelayedState,ReadState);
			if(HiberIoError || !Ret || !DelayedState->IsXpressCompression)
				return FALSE;
		}

		//
		// save this one
		//
		HIBER_DELAYED_XPRESS_STATE::_DELAYED_PAGE* Info	= DelayedState->DelayedPages + DelayedState->DelayedCount;
		Info->PageFrame									= PageFrame;
		Info->EntryCount								= EntryCount;

		//
		// inc delayed count
		//
		DelayedState->DelayedCount						+= 1;

		//
		// we can delay it?
		//
		if(DelayedState->DelayedCount != ARRAYSIZE(DelayedState->DelayedPages) && (DelayedState->DelayedCount << PAGE_SHIFT) < DelayedState->FinalOutputBytes)
			return TRUE;
	}
	else if(!DelayedState->DelayedCount)
	{
		//
		// nothing to flush
		//
		return TRUE;
	}

	//
	// got here means we should process the whole delayed request
	//
	if((DelayedState->DelayedCount << PAGE_SHIFT) != DelayedState->FinalOutputBytes)
		return FALSE;

	//
	// map each page and check they are contiguous
	//
	BOOLEAN IsContiguous								= TRUE;
	for(ULONG i = 0; i < DelayedState->DelayedCount; i ++)
	{
		//
		// get disposition
		//
		ULONG PageFrame									= DelayedState->DelayedPages[i].PageFrame;
		ULONG Disposition								= HbPageDisposition(PageFrame);
		if(Disposition == HiberMemoryError)
			return FALSE;

		//
		// map it
		//
		ULONG Index										= i + HiberPageMemoryXpressWorkspace;
		PVOID MappedAddress								= Disposition == HiberMemoryFree ? HbMapPte(Index,PageFrame) : HbNextSharedPage(Index,PageFrame);
		DelayedState->DelayedPages[i].MappedAddress		= MappedAddress;

		//
		// it is contiguous with prev one?
		//
		if(i && Add2Ptr(DelayedState->DelayedPages[i - 1].MappedAddress,PAGE_SIZE,PVOID) != MappedAddress)
			IsContiguous								= FALSE;
	}

	//
	// use mapped address directly if they are contiguous
	//
	if(IsContiguous)
	{
		DelayedState->OutputBufferSize					= DelayedState->DelayedCount << PAGE_SHIFT;
		DelayedState->OutputBuffer						= DelayedState->DelayedPages[0].MappedAddress;
	}
	else
	{
		DelayedState->OutputBufferSize					= DelayedState->FinalOutputBytes;
		DelayedState->OutputBuffer						= DelayedState->Workspace;
	}

	//
	// read it
	//
	Ret													= HbReadNextCompressedBlock(DelayedState,ReadState);
	if(HiberIoError || !Ret)
		return FALSE;

	//
	// copy it
	//
	for(ULONG i = 0; i < DelayedState->DelayedCount; i ++)
	{
		if(DelayedState->DelayedPages[i].MappedAddress != DelayedState->OutputBuffer)
			RtlCopyMemory(DelayedState->DelayedPages[i].MappedAddress,DelayedState->OutputBuffer,PAGE_SIZE);

		DelayedState->OutputBuffer						= Add2Ptr(DelayedState->OutputBuffer,PAGE_SIZE,PVOID);
		DelayedState->OutputBufferSize					-= PAGE_SIZE;
	}

	DelayedState->DelayedCount							= 0;
	return TRUE;
}

//
// read lznt1 compressed chunk
//
BOOLEAN HbReadNextCompressedChunkLZNT1(__in PVOID UncompressedBuffer,__in PHIBER_COMPRESSED_READ_STATE ReadState)
{
	NTSTATUS Status										= STATUS_NO_MORE_ENTRIES;
	PUCHAR NextData										= 0;
	ULONG ChunkSize										= 0;
	PUCHAR ChunkBuffer									= 0;

	while(1)
	{
		//
		// try to describe chunk
		//
		NextData										= static_cast<PUCHAR>(ReadState->ReadPosition);
		PUCHAR EndBuffer								= static_cast<PUCHAR>(ReadState->WritePosition);
		Status											= RtlDescribeChunk(2/*COMPRESSION_FORMAT_LZNT1*/,&NextData,EndBuffer,&ChunkBuffer,&ChunkSize);
		if(Status != STATUS_NO_MORE_ENTRIES && Status != STATUS_BAD_COMPRESSION_BUFFER)
			break;

		//
		// read more data,and try again
		//
		ULONG Length									= PtrOffset(ReadState->StorageBufferEnd,ReadState->StorageBuffer,ULONG);
		if(Length > 64 * 1024)
			Length										= 64 * 1024;

		ULONG AlreadyInBufferSize						= ROUND_TO_PAGES(PtrOffset(ReadState->WritePosition,ReadState->ReadPosition,ULONG));
		if(AlreadyInBufferSize == Length)
			return FALSE;

		if(!HbReadNextCompressedPages(Length - AlreadyInBufferSize,ReadState))
			return FALSE;
	}

	//
	// error
	//
	if(Status != STATUS_SUCCESS)
		return FALSE;

	//
	// decompress it
	//
	PUCHAR InputData									= static_cast<PUCHAR>(ReadState->ReadPosition);
	ULONG InputDataLength								= PtrOffset(ReadState->WritePosition,ReadState->ReadPosition,ULONG);
	PUCHAR OutputData									= static_cast<PUCHAR>(UncompressedBuffer);
	Status												= RtlDecompressBuffer(2/*COMPRESSION_FORMAT_LZNT1*/,OutputData,PAGE_SIZE,InputData,InputDataLength,&ChunkSize);
	if(!NT_SUCCESS(Status) || ChunkSize != PAGE_SIZE)
		return FALSE;

	//
	// update read position
	//
	ReadState->ReadPosition								= NextData;
	return TRUE;
}

//
// read next lznt1 compressed page
//
BOOLEAN HbReadNextCompressedPageLZNT1(__in PVOID UncompressedBuffer,__in PHIBER_COMPRESSED_READ_STATE ReadState)
{
	for(ULONG i = 0; i < PAGE_SIZE; i += PAGE_SIZE,UncompressedBuffer = Add2Ptr(UncompressedBuffer,PAGE_SIZE,PVOID))
	{
		if(!HbReadNextCompressedChunkLZNT1(UncompressedBuffer,ReadState))
			return FALSE;
	}

	return TRUE;
}

//
// restore from file
//
ULONG HbRestoreFile(__out PULONG ErrorCode,__out PCHAR* LinkedRestorePath)
{
	*ErrorCode											= 0;
	ULONG ErrorMessageId								= 0;
	ARC_STATUS Status									= ESUCCESS;
	__try
	{
		//
		// allocate buffer page
		//
		HiberBufferPage									= 0;
		Status											= BlAllocateAlignedDescriptor(LoaderFirmwareTemporary,0,1,1,&HiberBufferPage);
		if(!HiberBufferPage)
			try_leave(*ErrorCode = 0x60f;ErrorMessageId = BL_SYSTEM_RESTART_MEMORY_ERROR);

		//
		// read file header
		//
		HiberBuffer										= MAKE_KERNEL_ADDRESS_PAGE(HiberBufferPage);
		PPO_MEMORY_IMAGE FileHeader						= static_cast<PPO_MEMORY_IMAGE>(HiberBuffer);
		HbReadPage(HiberPageFileImageHeader,HiberBuffer);

		//
		// check file type
		//
		if(FileHeader->Signature == HIBER_SIGNATURE_LINK)
		{
			//
			// link file,try to open the real one
			//
			ULONG DeviceId								= 0;
			Status										= ArcOpen(Add2Ptr(FileHeader,sizeof(FileHeader->Signature),PCHAR),ArcOpenReadWrite,&DeviceId);
			if(Status != ESUCCESS)
			{
				//
				// the real file may on a scsi device
				//
				if(LinkedRestorePath)
					*LinkedRestorePath					= Add2Ptr(FileHeader,sizeof(FileHeader->Signature),PCHAR);

				HbSetImageSignature(0);

				try_leave(ErrorMessageId = 0);
			}

			//
			// open hiber file
			//
			ULONG FileId								= 0;
			Status										= BlOpen(DeviceId,HiberFileName,ArcOpenReadWrite,&FileId);
			if(Status != ESUCCESS)
				try_leave(ErrorMessageId = 0;ArcCacheClose(DeviceId));

			//
			// close the old one
			//
			BlClose(HiberFile);
			HiberFile									= FileId;

			//
			// read the first page
			//
			HbReadPage(HiberPageFileImageHeader,HiberBuffer);
		}

		//
		// wake signature
		//
		if(FileHeader->Signature == HIBER_SIGNATURE_RESTORING)
		{
			HbScreen(BL_RESTART_FAILED_TRY_AGAIN);
			static ULONG Selections[]					= {BL_DELETE_RESTORATION_DATA_AND_DISPLAY_BOOT_MENU,BL_CONTINUE_WITH_RESTART,BL_CONTINUE_DBG_BREAK_ON_WAKE,0};
			ULONG Selection								= HbSelection(7,7,Selections);
			if(Selection == 0)
				try_leave(HiberAbort = TRUE;HbSetImageSignature(0);ErrorMessageId = 0);

			//
			// update as normal file
			//
			FileHeader->Signature						= HIBER_SIGNATURE_NORMAL;
		}

		//
		// hiber signature
		//
		if(FileHeader->Signature != HIBER_SIGNATURE_NORMAL)
			try_leave(ErrorMessageId = 0);

		//
		// check header length
		//
		if(FileHeader->LengthSelf > PAGE_SIZE)
			try_leave(ErrorMessageId = BL_SYSTEM_RESTART_IMAGE_FILE_CORRUPT;*ErrorCode = 0x666);

		//
		// allocate header buffer
		//
		ULONG Length									= FileHeader->LengthSelf;
		FileHeader										= static_cast<PPO_MEMORY_IMAGE>(BlAllocateHeap(Length));
		if(!FileHeader)
			try_leave(ErrorMessageId = BL_SYSTEM_RESTART_MEMORY_ERROR;*ErrorCode = 0x66e);

		//
		// copy from hiber buffer
		//
		RtlCopyMemory(FileHeader,HiberBuffer,Length);

		//
		// read flags
		//
		HiberImageFeatureFlags							= FileHeader->FeatureFlags;

		//
		// checksum
		//
		ULONG CheckSum									= FileHeader->CheckSum;
		FileHeader->CheckSum							= 0;
		if(CheckSum != tcpxsum(0,FileHeader,Length))
			try_leave(ErrorMessageId = BL_SYSTEM_RESTART_IMAGE_FILE_CORRUPT;*ErrorCode = 0x679);

		//
		// version must be zero
		//
		if(FileHeader->Version)
			try_leave(ErrorMessageId = BL_SYSTEM_RESTART_IMAGE_FILE_CONFIG_ERROR;*ErrorCode = 0x67a);

		//
		// 4KB page size
		//
		if(FileHeader->PageSize != PAGE_SIZE)
			try_leave(ErrorMessageId = BL_SYSTEM_RESTART_IMAGE_FILE_CONFIG_ERROR;*ErrorCode = 0x67b);

		//
		// pte counts must <= 32
		//
		if(FileHeader->NoHiberPtes > ARRAYSIZE(HiberPageFrames))
			try_leave(ErrorMessageId = BL_SYSTEM_RESTART_IMAGE_FILE_CONFIG_ERROR;*ErrorCode = 0x682);

		//
		// saving....
		//
		HiberNoMappings									= FileHeader->NoMappings;
		HiberIdentityVa									= FileHeader->HiberIdentityVa;
		HiberImagePageSelf								= FileHeader->PageSelf;

		//
		// allocate ptes
		//
		ULONG CurrentVa									= HiberVa;
		ULONG EndVa										= 0;
		while(!CurrentVa || (FileHeader->HiberIdentityVa >= CurrentVa && FileHeader->HiberIdentityVa <= EndVa))
		{
			HbAllocatePtes(ARRAYSIZE(HiberPageFrames),&HiberPtes,&HiberVa);
			CurrentVa									= HiberVa;
			EndVa										= CurrentVa + (ARRAYSIZE(HiberPageFrames) << PAGE_SHIFT);
		}

		//
		// read mappings page
		//
		HbReadPage(HiberPageFileFreeMappings,HiberBuffer);

		//
		// checksum mappings page
		//
		if(FileHeader->FreeMapCheck != tcpxsum(0,HiberBuffer,PAGE_SIZE))
			try_leave(ErrorMessageId = BL_SYSTEM_RESTART_IMAGE_FILE_CORRUPT;*ErrorCode = 0x6a3);

		//
		// allocate compression work buffer
		//
		PVOID CompressWorkBuffer						= HbAllocateAlignedHeap(0x10000 + sizeof(HIBER_DELAYED_XPRESS_STATE));
		if(!CompressWorkBuffer)
			try_leave(ErrorMessageId = BL_SYSTEM_RESTART_MEMORY_ERROR;*ErrorCode = 0x6b1);

		//
		// initialize compression info
		//
		PHIBER_DELAYED_XPRESS_STATE DelayedState		= Add2Ptr(CompressWorkBuffer,0x10000,PHIBER_DELAYED_XPRESS_STATE);
		DelayedState->OutputBufferSize					= 0;
		DelayedState->Workspace							= CompressWorkBuffer;

		//
		// allocate read buffer
		//
		ULONG BasePage									= 0;
		PVOID ReadWorkBuffer							= 0;
		Status											= BlAllocateAlignedDescriptor(LoaderFirmwareTemporary,0,0x14,0x10,&BasePage);
		if(Status == ESUCCESS)
			ReadWorkBuffer								= MAKE_KERNEL_ADDRESS_PAGE(BasePage);
		else
			ReadWorkBuffer								= HbAllocateAlignedHeap(0x14000);

		if(!ReadWorkBuffer)
			try_leave(ErrorMessageId = BL_SYSTEM_RESTART_MEMORY_ERROR;*ErrorCode = 0x6c5);

		//
		// intialize read state
		//
		HIBER_COMPRESSED_READ_STATE ReadState;
		ReadState.Offset8								= ReadWorkBuffer;
		ReadState.EndOfWriteBuffer						= Add2Ptr(ReadWorkBuffer,0x14000,PVOID);
		ReadState.StorageBuffer							= ReadWorkBuffer;
		ReadState.StorageBufferEnd						= Add2Ptr(ReadWorkBuffer,0x14000,PVOID);
		ReadState.OffsetInPage							= 0;
		ReadState.ReadPosition							= ReadWorkBuffer;
		ReadState.WritePosition							= ReadWorkBuffer;

		HbMapPte(HiberPageMemoryFreeMappings,HiberBufferPage);
		HbMapPte(HiberPageMemoryOriginalMappings,HiberBufferPage);

		//
		// remap free mappings to an unused page
		//
		PVOID FreeMappingsPage							= HbNextSharedPage(HiberPageMemoryFreeMappings,0);

		//
		// copy free mappings
		//
		RtlCopyMemory(FreeMappingsPage,HiberBuffer,PAGE_SIZE);

		//
		// remap new mappings to an unused page
		//
		HbNextSharedPage(HiberPageMemoryOriginalMappings,0);

		//
		// copy dispatcher
		//
		extern VOID WakeDispatcher();
		extern ULONG WakeDispatcherEnd();
		PVOID DispatcherCode							= &WakeDispatcher;
		ULONG DispatcherCodeLength						= PtrOffset(&WakeDispatcherEnd,&WakeDispatcher,ULONG);
		ULONG DispatcherCodePage						= 0;
		PHIBER_DISPATCH_ROUTINE DispatchRoutine			= 0;
		while(DispatcherCodeLength)
		{
			//
			// max dispather size is 2 pages
			//
			if(DispatcherCodePage >= HiberPageMaxWakeCode)
				try_leave(ErrorMessageId = BL_SYSTEM_RESTART_INTERNAL_ERROR2;*ErrorCode = 0x6fe);

			//
			// remap 7,8 page
			//
			PVOID Temp									= HbNextSharedPage(DispatcherCodePage + HiberPageMemoryWakeCode0,0);
			if(!DispatcherCodePage)
				DispatchRoutine							= reinterpret_cast<PHIBER_DISPATCH_ROUTINE>(Temp);

			ULONG CopyLength							= DispatcherCodeLength;
			if(CopyLength > PAGE_SIZE)
				CopyLength								= PAGE_SIZE;

			//
			// copy to new page
			//
			RtlCopyMemory(Temp,DispatcherCode,CopyLength);
			DispatcherCode								= Add2Ptr(DispatcherCode,CopyLength,PVOID);
			DispatcherCodeLength						-= CopyLength;
			DispatcherCodePage							+= 1;
		}

		//
		// remap processor state
		//
		PKPROCESSOR_STATE ProcessorState				= static_cast<PKPROCESSOR_STATE>(HbNextSharedPage(HiberPageMemoryProcessorState,0));

		//
		// read processor state
		//
		HbReadPage(HiberPageFileProcessorState,HiberBuffer);

		//
		// copy to new page
		//
		RtlCopyMemory(ProcessorState,HiberBuffer,PAGE_SIZE);

		//
		// checksum
		//
		if(tcpxsum(0,ProcessorState,PAGE_SIZE) != FileHeader->WakeCheck)
			try_leave(ErrorMessageId = BL_SYSTEM_RESTART_IMAGE_FILE_CORRUPT;*ErrorCode = 0x718);

		//
		// we are using PAE?
		//
		if(ProcessorState->SpecialRegisters.Cr4 & 0x20)
			BlUsePae									= TRUE;

		//
		// setup dispatcher
		//
		extern VOID HiberSetupForWakeDispatch();
		HiberSetupForWakeDispatch();

		//
		// start recording 'data' page remap
		//
		HiberFirstRemap									= HiberLastRemap;

		//
		// point to temp buffer
		//
		PPO_MEMORY_RANGE_ARRAY Array					= static_cast<PPO_MEMORY_RANGE_ARRAY>(HiberBuffer);

		//
		// get first compressed 'data' page
		//
		ULONG CurTablePage								= FileHeader->FirstTablePage;

		//
		// update screen
		//
		BlSetProgBarCharacteristics(BL_PROGRESS_BAR_FRONT_CHAR,BL_PROGRESS_BAR_BACK_CHAR);
		BlOutputStartupMsg(BL_RESUMING_WINDOWS);
		BlOutputTrailerMsg(BL_GOTO_ADVANCED_BOOT_F8);

		ULONG ProcessedPageCount						= 3;
		ULONG OldPercent								= 0;
		LONG CompressFormat								= -1;
		DelayedState->ErrorFlags						= FALSE;
		DelayedState->OutputBufferSize					= 0;
		DelayedState->DelayedCount						= 0;
		DelayedState->Offset24							= 0;

		//
		// process tabled 'data' page
		//
		while(CurTablePage)
		{
			//
			// setup read state
			//
			if( !ReadState.OffsetInPage					||
				CurTablePage > ReadState.OffsetInPage	||
				CurTablePage < ReadState.OffsetInPage - (PtrOffset(ReadState.WritePosition,ReadState.ReadPosition,ULONG) >> PAGE_SHIFT))
			{
				ReadState.OffsetInPage					= CurTablePage;
				ReadState.ReadPosition					= ReadState.StorageBuffer;
				ReadState.WritePosition					= ReadState.StorageBuffer;
				ReadState.SeekFileBeforeRead			= TRUE;
			}

			ReadState.ReadPosition						= Add2Ptr(ReadState.WritePosition,(CurTablePage << PAGE_SHIFT) - (ReadState.OffsetInPage << PAGE_SHIFT),PVOID);

			//
			// read memory range array
			//
			BOOLEAN Ret									= HbReadNextCompressedPages(PAGE_SIZE,&ReadState);
			if(HiberIoError)
				try_leave(ErrorMessageId = BL_SYSTEM_RESTART_READ_FAILURE;*ErrorCode = 0x76a);

			if(!Ret)
				try_leave(ErrorMessageId = BL_SYSTEM_RESTART_IMAGE_FILE_CORRUPT;*ErrorCode = 0x76b);

			//
			// copy to array buffer
			//
			RtlCopyMemory(Array,ReadState.ReadPosition,PAGE_SIZE);

			//
			// advance to compressed data
			//
			ReadState.ReadPosition						= Add2Ptr(ReadState.ReadPosition,PAGE_SIZE,PVOID);
			CheckSum									= Array->Link.CheckSum;

			//
			// if checksum is not zero,then check it
			//
			if(CheckSum)
			{
				Array->Link.CheckSum					= 0;
				if(CheckSum != tcpxsum(0,Array,PAGE_SIZE))
					try_leave(ErrorMessageId = BL_SYSTEM_RESTART_IMAGE_FILE_CORRUPT;*ErrorCode = 0x777);
			}

			//
			// check compress format
			//
			if(CompressFormat < 0)
			{
				Ret										= HbReadNextCompressedBlockHeader(DelayedState,&ReadState);
				if(HiberIoError)
					try_leave(ErrorMessageId = BL_SYSTEM_RESTART_READ_FAILURE;*ErrorCode = 0x77e);

				if(!Ret)
					try_leave(ErrorMessageId = BL_SYSTEM_RESTART_IMAGE_FILE_CORRUPT;*ErrorCode = 0x77f);

				CompressFormat							= DelayedState->IsXpressCompression;
			}

			//
			// process memory ranges
			//
			for(ULONG i = 1; i <= Array->Link.EntryCount; i ++)
			{
				PPO_MEMORY_RANGE_ARRAY_RANGE Range		= &Array->Range + i;
				ULONG Start								= Range->StartPage;
				ULONG End								= Range->EndPage;
				ULONG Checksum							= 0;

				while(Start < End)
				{
					if(CompressFormat)
					{
						//
						// xpress compression format
						//
						Ret								= HbReadDelayedBlock(FALSE,Start,Array->Link.EntryCount,DelayedState,&ReadState);
						if(HiberIoError)
							try_leave(ErrorMessageId = BL_SYSTEM_RESTART_READ_FAILURE;*ErrorCode = 0x7a7);

						if(!Ret)
							try_leave(ErrorMessageId = BL_SYSTEM_RESTART_IMAGE_FILE_CORRUPT;*ErrorCode = 0x7a8);
					}
					else
					{
						//
						// LZNT1 compression format
						//
						ULONG Disposition				= HbPageDisposition(Start);
						if(Disposition == 2)
							try_leave(ErrorMessageId = BL_SYSTEM_RESTART_IMAGE_FILE_CORRUPT;*ErrorCode = 0x794);

						ULONG Index						= HiberPageMemoryLZNT1Workspace;
						PVOID Temp						= Disposition == 0 ? HbMapPte(Index,Start) : HbNextSharedPage(Index,Start);
						Ret								= HbReadNextCompressedPageLZNT1(Temp,&ReadState);
						if(HiberIoError)
							try_leave(ErrorMessageId = BL_SYSTEM_RESTART_READ_FAILURE;*ErrorCode = 0x79d);

						if(!Ret)
							try_leave(ErrorMessageId = BL_SYSTEM_RESTART_IMAGE_FILE_CORRUPT;*ErrorCode = 0x79e);

						CheckSum						= tcpxsum(CheckSum,Temp,PAGE_SIZE);
					}

					ProcessedPageCount					+= 1;
					ULONG NewPercent					= ProcessedPageCount * 100 / FileHeader->TotalPages;
					if(OldPercent != NewPercent)
					{
						BlUpdateProgressBar(NewPercent);
						HbCheckForPause();
					}

					OldPercent							= NewPercent;
					Start								+= 1;
				}

				//
				// out of remap ?
				//
				if(HiberOutOfRemap)
					try_leave(ErrorMessageId = BL_SYSTEM_RESTART_INTERNAL_ERROR;*ErrorCode = 0x7b7);

				//
				// lznt1 checksum error ?
				//
				if(!CompressFormat && Range->Checksum != CheckSum)
					DelayedState->ErrorFlags			= TRUE;

				//
				// met some error
				//
				if(DelayedState->ErrorFlags && !HiberBreakOnWake)
				{
					HbScreen(BL_SYSTEM_RESTART_FAILED);
					HbPrintMsg(BL_SYSTEM_RESTART_IMAGE_FILE_CORRUPT);
					ULONG Selections[]					= {BL_DELETE_RESTORATION_DATA_AND_DISPLAY_BOOT_MENU,BL_CONTINUE_DBG_BREAK_ON_WAKE,0};
					ULONG Selection						= HbSelection(7,7,Selections);
					if(!Selection)
						try_leave(ErrorMessageId = 0;HiberAbort = TRUE;HbSetImageSignature(0));
				}
			}

			//
			// goto the next page
			//
			CurTablePage								= Array->Link.NextTable;
		}

		//
		// flush delayed blocks
		//
		if(CompressFormat > 0)
		{
			BOOLEAN Ret									= HbReadDelayedBlock(TRUE,0,0,DelayedState,&ReadState);
			if(HiberIoError)
				try_leave(ErrorMessageId = BL_SYSTEM_RESTART_READ_FAILURE;*ErrorCode = 0x7f8);

			if(!Ret)
				try_leave(ErrorMessageId = BL_SYSTEM_RESTART_IMAGE_FILE_CORRUPT;*ErrorCode = 0x7f9);

			if(DelayedState->ErrorFlags)
			{
				HbScreen(BL_SYSTEM_RESTART_FAILED);
				HbPrintMsg(BL_SYSTEM_RESTART_IMAGE_FILE_CORRUPT);
				ULONG Selections[]						= {BL_DELETE_RESTORATION_DATA_AND_DISPLAY_BOOT_MENU,BL_CONTINUE_DBG_BREAK_ON_WAKE,0};
				ULONG Selection							= HbSelection(7,7,Selections);
				if(!Selection)
					try_leave(ErrorMessageId = 0;HiberAbort = TRUE;HbSetImageSignature(0));
			}
		}

		//
		// set as waking
		//
		HbSetImageSignature(HIBER_SIGNATURE_RESTORING);

		//
		// enable DEP
		//
		if(FileHeader->HiberFlags & 2)
			HiberNoExecute								= TRUE;

		//
		// stop debugger
		//
		BdStopDebugger();

		//
		// reconnect APM
		//
		if(FileHeader->HiberFlags & 1)
			ExternalServicesTable->InitializeAPMBios();

		//
		// never return
		//
		DispatchRoutine();

		//
		// XXX : how about debugger ?
		//

		//
		// got here means we met an error
		//
		*ErrorCode										= 0x826;
		ErrorMessageId									= BL_SYSTEM_RESTART_INTERNAL_ERROR2;
	}
	__finally
	{

	}

	return ErrorMessageId;
}

//
// restore
//
ARC_STATUS BlHiberRestore(__in ULONG DeviceId,__out PCHAR* LinkedRestorePath)
{
	//
	// aborted
	//
	if(HiberAbort)
		return ESUCCESS;

	//
	// open hiber file
	//
	ARC_STATUS Status									= BlOpen(DeviceId,HiberFileName,ArcOpenReadWrite,&HiberFile);
	if(Status != ESUCCESS)
		return ESUCCESS;

	//
	// restore from file
	//
	BOOLEAN SavedOutputDots								= BlOutputDots;
	BlRestoring											= TRUE;
	BlOutputDots										= TRUE;
	ULONG ErrorCode										= 0;
	ULONG ErrorMessageId								= HbRestoreFile(&ErrorCode,LinkedRestorePath);
	BlOutputDots										= SavedOutputDots;

	//
	// failed
	//
	if(ErrorMessageId)
	{
		BlSetInverseMode(FALSE);
		if(!HiberAbort)
		{
			HbScreen(BL_SYSTEM_RESTART_FAILED);
			HbPrintMsg(ErrorMessageId);
			static ULONG Selections[]					= {BL_DELETE_RESTORATION_DATA_AND_DISPLAY_BOOT_MENU,0};
			HbSelection(7,7,Selections);
		}

		HbSetImageSignature(0);
	}

	//
	// done
	//
	BlRestoring											= FALSE;
	BlClose(HiberFile);
	return ErrorMessageId ? EAGAIN : ESUCCESS;
}

//
// setup x86 dispatch
//
VOID HiberSetupForWakeDispatchX86()
{
	PHARDWARE_PTE HiberPdePage							= static_cast<PHARDWARE_PTE>(HbNextSharedPage(HiberPageMemoryPde,0));
	PHARDWARE_PTE HiberPtePage							= static_cast<PHARDWARE_PTE>(HbNextSharedPage(HiberPageMemoryPte,0));

	RtlZeroMemory(HiberPdePage,PAGE_SIZE);
	RtlZeroMemory(HiberPtePage,PAGE_SIZE);

	//
	// map HiberVa to hiber wake context
	//
	PHARDWARE_PTE Pte									= HiberPdePage + (HiberVa >> 22);
	Pte->PageFrameNumber								= HiberPageFrames[HiberPageMemoryPte];
	Pte->Write											= TRUE;
	Pte->Valid											= TRUE;
	Pte													= HiberPtePage + ((HiberVa >> 12) & 0x3ff);
	RtlCopyMemory(Pte,HiberPtes,ARRAYSIZE(HiberPageFrames) * sizeof(HARDWARE_PTE));

	//
	// map HiberIdentityVa to hiber wake context
	//
	if((HiberVa >> 22) != (HiberIdentityVa >> 22))
	{
		HiberPtePage									= static_cast<PHARDWARE_PTE>(HbNextSharedPage(HiberPageMemoryPte,0));
		Pte												= HiberPdePage + (HiberIdentityVa >> 22);
		Pte->PageFrameNumber							= HiberPageFrames[HiberPageMemoryPte];
		Pte->Valid										= TRUE;
		Pte->Write										= TRUE;
	}

	Pte													= HiberPtePage + ((HiberIdentityVa >> 12) & 0x3ff);
	RtlCopyMemory(Pte,HiberPtes,ARRAYSIZE(HiberPageFrames) * sizeof(HARDWARE_PTE));

	//
	// compute VA of HiberPtes
	//	HiberIdentityVa + ((HiberIdentityVa >> 12) & 0x3ff) * 4 + (HiberPtePage - HiberVa);
	//
	HiberTransVa										= HiberIdentityVa - HiberVa + reinterpret_cast<ULONG>(Pte);
}

//
// map pae page
//
VOID HbMapPagePAE(__in PHARDWARE_PTE_PAE Pdpt,__in ULONG VirtualAddress,__in ULONG PhyPageFrame)
{
	PHARDWARE_PTE_PAE Pdpe								= Pdpt + (VirtualAddress >> 30);
	PHARDWARE_PTE_PAE Pde								= 0;
	PHARDWARE_PTE_PAE Pte								= 0;

	if(!Pdpe->Valid)
	{
		Pde												= static_cast<PHARDWARE_PTE_PAE>(HbNextSharedPage(HiberPageMemoryPde,0));
		RtlZeroMemory(Pde,PAGE_SIZE);

		Pdpe->Valid										= TRUE;
		Pdpe->PageFrameNumber							= HiberPageFrames[HiberPageMemoryPde];
	}
	else
	{
		Pde												= static_cast<PHARDWARE_PTE_PAE>(HbMapPte(HiberPageMemoryPde,Pdpe->PageFrameNumber));
	}

	Pde													+= (VirtualAddress >> 21) & 0x1ff;
	if(!Pde->Valid)
	{
		Pte												= static_cast<PHARDWARE_PTE_PAE>(HbNextSharedPage(HiberPageMemoryPte,0));
		RtlZeroMemory(Pte,PAGE_SIZE);
		Pde->Valid										= TRUE;
		Pde->Write										= TRUE;
		Pde->PageFrameNumber							= HiberPageFrames[HiberPageMemoryPte];
	}
	else
	{
		Pte												= static_cast<PHARDWARE_PTE_PAE>(HbMapPte(HiberPageMemoryPte,Pde->PageFrameNumber));
	}

	Pte													+= (VirtualAddress >> 12) & 0x1ff;
	Pte->Valid											= TRUE;
	Pte->Write											= TRUE;
	Pte->PageFrameNumber								= PhyPageFrame;
}

extern "C" VOID BlpEnablePAE(__in PHARDWARE_PTE_PAE Pdpt);
extern "C" ULONG BlpEnablePAEEnd();

//
// setup pae dispatch
//
VOID HiberSetupForWakeDispatchPAE()
{
	//
	// get PPDT
	//
	PHARDWARE_PTE_PAE Pdpt								= static_cast<PHARDWARE_PTE_PAE>(HbNextSharedPage(0,0));
	ULONG PdptPageFrame									= HiberPageFrames[0];
	RtlZeroMemory(Pdpt,PAGE_SIZE);

	//
	// map enable pae routine (from BlpEnablePAE to BlpEnablePAEEnd)
	//
	ULONG Temp											= reinterpret_cast<ULONG>(&BlpEnablePAE);
	HbMapPagePAE(Pdpt,Temp,Temp >> PAGE_SHIFT);
	Temp												= reinterpret_cast<ULONG>(&BlpEnablePAEEnd);
	HbMapPagePAE(Pdpt,Temp,Temp >> PAGE_SHIFT);

	//
	// map HiberVa to hiber wake context
	//
	for(ULONG i = 0; i < ARRAYSIZE(HiberPageFrames); i ++)
		HbMapPagePAE(Pdpt,HiberVa + (i << PAGE_SHIFT),HiberPtes[i].PageFrameNumber);

	//
	// map HiberIdentityVa to hiber wake context
	//
	for(ULONG i = 0; i < ARRAYSIZE(HiberPageFrames); i ++)
		HbMapPagePAE(Pdpt,HiberIdentityVa + (i << PAGE_SHIFT),HiberPtes[i].PageFrameNumber);

	//
	// save pde
	//
	HiberPageFrames[HiberPageMemoryPde]					= PdptPageFrame;

	//
	// compute VA of HiberPte
	//
	HiberTransVa										= HiberIdentityVa + ((HiberIdentityVa >> 12) & 0x1ff) * 8 + (HiberPageMemoryPte << PAGE_SHIFT);
}

//
// setup wake dispatcher
//
VOID HiberSetupForWakeDispatch()
{
	if(BlUsePae)
		HiberSetupForWakeDispatchPAE();
	else
		HiberSetupForWakeDispatchX86();
}

//
// HACK HACK HACK hope this will work
//
#pragma optimize("",off)
#pragma code_seg(push,".WAKE")

//
// dispatcher
//
VOID __declspec(naked) WakeDispatcher()
{
	__asm
	{
		push		ebp																			// save ebp
		push		ebx																			// save ebx
		push		esi																			// save esi
		push		edi																			// save edi
		mov			ebp,HiberVa																	// ebp = hiber context,HiberVa is the VA of Hiber context in osloader view
		mov			eax,HiberFirstRemap															// eax = first remap index to map data page (not include hiber meta page)
		mov			ecx,HiberLastRemap															// ecx = last remap index
		lea			edx,[ebp + HIBER_WAKE_CONTEXT.HeapBuffer]									// edx = hiber heap,we use the free space in prossesor state page as heap
		mov			esi,HiberPtes																// esi = hiber ptes used to map hiber meta page memory
		mov			[ebp + HIBER_WAKE_CONTEXT.FirstRemap],eax									// save frist remap index
		mov			[ebp + HIBER_WAKE_CONTEXT.LastRemap],ecx									// save last remap index
		mov			[ebp + HIBER_WAKE_CONTEXT.SavedEsp],esp										// save esp,we can restore esp to this and finally return to caller
		mov			[ebp + HIBER_WAKE_CONTEXT.HiberHeap],edx									// set heap pointer
		mov			[ebp + HIBER_WAKE_CONTEXT.HiberPtes],esi									// save ptes
		mov			eax,HiberPageFrames[HiberPageMemoryPde * type ULONG]						// eax = pde physical page frame number
		mov			ecx,HiberTransVa															// ecx = ptes VA under hiber's page table
		mov			edx,HiberIdentityVa															// edx = hiber VA under hiber and kernel's (the same) page table
		movzx		esi,BlUsePae																// esi = is using pae
		mov			[ebp + HIBER_WAKE_CONTEXT.PdePageFrame],eax									// save pde
		mov			[ebp + HIBER_WAKE_CONTEXT.HiberTransVa],ecx									// save ptes virtual address
		mov			[ebp + HIBER_WAKE_CONTEXT.HiberIdentityVa],edx								// save hiber virtual address
		mov			[ebp + HIBER_WAKE_CONTEXT.UsePae],esi										// save pae
		mov			eax,HiberImageFeatureFlags													// eax = feature flags,currently only 4(restore cr4) is used
		movzx		ecx,HiberNoExecute															// ecx = is DEP enabled
		mov			[ebp + HIBER_WAKE_CONTEXT.ImageFeatureFlags],eax							// save feature flags
		mov			[ebp + HIBER_WAKE_CONTEXT.NoExecute],ecx									// save DEP state
		sgdt		[ebp + HIBER_WAKE_CONTEXT.GdtLimit]											// get current gdt
		movzx		ecx,[ebp + HIBER_WAKE_CONTEXT.GdtLimit]										// ecx = gdt length - 1
		inc			ecx																			// ecx = gdt length
		push		ecx																			// save ecx in stack
		call		AllocateInHiberHeap															// allocate bytes(length = ecx) in hiber heap buffer
		pop			ecx																			// restore ecx = gdt length
		mov			edi,eax																		// edi = eax = new buffer address in hiber heap
		mov			esi,[ebp + HIBER_WAKE_CONTEXT.GdtBase]										// esi = old gdt address
		rep			movsd																		// copy to new buffer
		mov			[ebp + HIBER_WAKE_CONTEXT.GdtBase],eax										// set gdt base to new buffer
		lgdt		[ebp + HIBER_WAKE_CONTEXT.GdtLimit]											// and load the new gdt
		sub			eax,ebp																		// eax = gdt offset from hiber context
		add			eax,[ebp + HIBER_WAKE_CONTEXT.HiberIdentityVa]								// eax = gdt virtual address under hiber's page table
		mov			[ebp + HIBER_WAKE_CONTEXT.GdtBase],eax										// save it for later use
		mov			eax,[ebp + HIBER_WAKE_CONTEXT.ProcessorState.SpecialRegisters.Cr3]			// eax = kernel's cr3
		shr			eax,12																		// eax = kernel's PDE page frame number
		call		GetHiberPageFrame															// get remapped kernel's PDE page frame number in eax
		push		eax																			// eax = remapped kernel' PDE page frame number
		push		0																			// map in HiberPtes with index = 0
		call		MapInHiberPtesX86															// eax = mapped VA,we can use it to access kernel's PDE page

		//
		// setup kernel's identity map
		//
		cmp			[ebp + HIBER_WAKE_CONTEXT.UsePae],0											// is using PAE ?
		jnz			short SetupIdentityMapPAE													// jmp to PAE if kernel is using PAE

		mov			ecx,[ebp + HIBER_WAKE_CONTEXT.HiberIdentityVa]								// ecx = hiber context virtual address under kernel's page table
		shr			ecx,22																		// ecx = index in Page Directory Table
		mov			eax,[eax + ecx * 4]															// eax = content of Page Directory Table item
		shr			eax,12																		// eax = page frame number of the Page Table
		call		GetHiberPageFrame															// eax = remapped page frame number
		push		eax																			// eax = remapped page frame number of Page Table
		push		0																			// map index = 0
		call		MapInHiberPtesX86															// eax = mapped Page Table VA
		mov			ecx,[ebp + HIBER_WAKE_CONTEXT.HiberIdentityVa]								// ecx = hiber context VA under kernel's page table
		shr			ecx,12																		//
		and			ecx,0x3ff																	// ecx = Page Table index
		lea			edi,[eax + ecx * 4]															// edi = Page Table Entries which maps hiber meta data
		jmp			short CopyHiberPtes															// jmp to copy routine

SetupIdentityMapPAE:
		mov			ecx,[ebp + HIBER_WAKE_CONTEXT.ProcessorState.SpecialRegisters.Cr3]			// ecx = kernel's cr3
		and			ecx,0xfe0																	// eax is the base page of Page Directory Pointer Table,add offset here
		add			eax,ecx																		// eax = VA of Page Directory Pointer Table
		mov			ecx,[ebp + HIBER_WAKE_CONTEXT.HiberIdentityVa]								// ecx = hiber context virtual address under kernel's page table
		shr			ecx,30																		// ecx = index of Page Directory Pointer Table
		mov			eax,[eax + ecx * 8]															// eax = content of Page Directory Pointer Table
		shr			eax,12																		// eax = Page Directory Table's page frame number
		call		GetHiberPageFrame															// remap it
		push		eax																			// eax = remapped Page Directory Table's page frame number
		push		0																			// map index = 0
		call		MapInHiberPtesX86															// eax = Page Directory Table's VA
		mov			ecx,[ebp + HIBER_WAKE_CONTEXT.HiberIdentityVa]								// ecx = hiber context virtual address under kernel's page table
		shr			ecx,21																		//
		and			ecx,0x1ff																	// ecx = index of Page Directory Table 
		mov			eax,[eax + ecx * 8]															// eax = content of Page Directory Table
		shr			eax,12																		// eax = Page Table's page frame number
		call		GetHiberPageFrame															// remap it
		push		eax																			// eax = remapped Page Table's page frame number
		push		0																			// map index = 0
		call		MapInHiberPtesX86															// map it,eax = Page Table's VA
		mov			ecx,[ebp + HIBER_WAKE_CONTEXT.HiberIdentityVa]								// ecx = hiber context virtual address under kernel's page table
		shr			ecx,12																		//
		and			ecx,0x1ff																	// ecx = index of Page Table 
		lea			edi,[eax + ecx * 8]															// edi = Page Table Entries which maps hiber meta data

CopyHiberPtes:
		mov			esi,[ebp + HIBER_WAKE_CONTEXT.HiberPtes]									// esi = current hiber maps
		mov			ecx,32																		// ecx = max count
		xor			eax,eax																		// eax = 0
		cmp			[ebp + HIBER_WAKE_CONTEXT.UsePae],0											// kernel is using pae ?
		jnz			CopyHiberPtesPAE
		rep			movsd																		// kernel is not using pae,copy it directly
		jmp			short SetWakeBreak

CopyHiberPtesPAE:
		movsd																					// copy first UINT32
		stosd																					// zero out the next UINT32,hiber ptes should never be placed above 4GB
		loopne		CopyHiberPtesPAE															// loop copy

SetWakeBreak:
		cmp			byte ptr HiberBreakOnWake,0													// kernel should DbgBreak on wake ?
		jz			short SetPageDirectoryTable

		mov			eax,HiberImagePageSelf														// eax = kernel's page file header page frame number
		call		GetHiberPageFrame															// remap it
		push		eax																			// eax = kernel's page file header page frame number
		push		1																			// map index = 1
		call		MapInHiberPtesX86															// map it
		mov			dword ptr [eax],HIBER_SIGNATURE_BREAK_ON_WAKE								// change signature to brkp telling kernel to make a DbgBreak call

		//
		// after we set pde,we should use HiberIdentityVa to access hiber context
		//
SetPageDirectoryTable:
		mov			ebx,[ebp + HIBER_WAKE_CONTEXT.HiberIdentityVa]								// ebx = hiber context VA under hiber's page table
		mov			eax,[ebp + HIBER_WAKE_CONTEXT.PdePageFrame]									// eax = hiber's PDE page frame
		shl			eax,12																		// make cr3 content
		cmp			byte ptr BlUsePae,1															// kernel is using pae ?
		jnz			short SetPageDirectoryTableX86

		lea			esp,[ebp + HIBER_WAKE_CONTEXT.HeapBuffer]									// reset hiber stack
		push		eax																			// eax = Page Directory Pointer Table's physical address
		mov			eax,offset BlpEnablePAE														// call BlpEnablePAE to setup cr3,cr4
		call		eax																			// BlpEnablePAE has already been mapped in hiber's page table
		cmp			[ebp + HIBER_WAKE_CONTEXT.NoExecute],1										// DEP is enabled ?
		jnz			JmpToPhase2

		mov			ecx,0xc0000080																// msr index
		rdmsr																					// read msr
		or			eax,0x800																	// enable NX-bit/XD-bit(bit11)
		wrmsr																					// write msr
		jmp			short JmpToPhase2

SetPageDirectoryTableX86:
		mov			cr3,eax																		// setup cr3

JmpToPhase2:
		mov			edi,ebx																		// edi = ebx = hiber context under hiber's page table
		lea			ebx,[ebx + HIBER_WAKE_CONTEXT.WakeCodeBuffer]								// ebx = WakeDisptacher's virtual address under hiber's page table
		add			ebx,offset RunInNewPdePhase2
		sub			ebx,offset WakeDispatcher													// ebx = VA of RunInNewPdePhase2
		jmp			ebx																			// jmp to RunInNewPdePhase2

		//
		// now,we are using hiber's page table
		//
RunInNewPdePhase2:
		mov			ebp,edi																		// ebp = edi = hiber context under hiber's page table
		mov			eax,[ebp + HIBER_WAKE_CONTEXT.HiberTransVa]									// eax = VA of hiber ptes under hiber's page table
		mov			[ebp + HIBER_WAKE_CONTEXT.HiberPtes],eax									// fixup it
		lea			esp,[ebp + HIBER_WAKE_CONTEXT.HeapBuffer]									// setup stack
		lgdt		[ebp + HIBER_WAKE_CONTEXT.GdtLimit]											// reload gdt,base has already been set
		lea			ebx,[ebp + HIBER_WAKE_CONTEXT.WakeCodeBuffer]								// ebx =  WakeDisptacher's virtual address under hiber's page table
		sub			ebx,offset WakeDispatcher
		cmp			[ebp + HIBER_WAKE_CONTEXT.UsePae],0
		jnz			short SetupMapRoutinePAE
		add			ebx,offset MapInHiberPtesX86												// ebx =  MapInHiberPtesX86's virtual address under hiber's page table
		jmp			short CopyMappings

SetupMapRoutinePAE:
		add			ebx,offset MapInHiberPtesPAE												// ebx =  MapInHiberPtesPAE's virtual address under hiber's page table

		//
		// copy those "remapped" pages to the original position
		//
CopyMappings:
		mov			edx,[ebp + HIBER_WAKE_CONTEXT.FirstRemap]									// edx = first 'data' page remapped

CopyMappingsLoop:
		cmp			edx,[ebp + HIBER_WAKE_CONTEXT.LastRemap]									// all remapped 'data' page has been processed?
		jnb			short FlushTLB

		push		[ebp + edx * 4 + HIBER_WAKE_CONTEXT.RemappedPageFrame]						// remapped page frame number
		push		0																			// map index = 0
		call		ebx																			// map it
		mov			esi,eax																		// src address
		push		[ebp + edx * 4 + HIBER_WAKE_CONTEXT.OriginalPageFrame]						// which page it should be
		push		1																			// map index = 1
		call		ebx																			// map it
		mov			edi,eax																		// and this is dst address
		mov			ecx,PAGE_SIZE >> 2															// copy one page
		rep			movsd																		// copy it
		inc			edx																			// advance to the next remap
		jmp			short CopyMappingsLoop														// loop copy

FlushTLB:
		lea			esi,[ebp + HIBER_WAKE_CONTEXT.ProcessorState.SpecialRegisters]				// esi = special registers
		mov			eax,cr3																		// reload cr3 to flush TLB
		mov			cr3,eax
		mov			cr3,eax
		mov			eax,[esi + KSPECIAL_REGISTERS.Cr4]											// eax = cr4
		test		[ebp + HIBER_WAKE_CONTEXT.ImageFeatureFlags],4								// we should reload cr4 ?
		jz			short JmpToKernel

		_emit		0x0f																		// cr4 = eax
		_emit		0x22																		// mov cr4, eax
		_emit		0xe0

		//
		// jump to kernel using an "interrupt trap frame"
		//
JmpToKernel:
		mov			eax,[esi + KSPECIAL_REGISTERS.Cr3]											// eax = kernel's cr3
		mov			cr3,eax																		// set cr3 to kernel's cr3
		mov			eax,[esi + KSPECIAL_REGISTERS.Cr0]											// eax = kernel's cr0
		mov			cr0,eax																		// set cr0 to kernel's cr0
		mov			ecx,[esi + KSPECIAL_REGISTERS.Gdtr.Base]									// ecx = kernel's gdt base
		lgdt		[esi + KSPECIAL_REGISTERS.Gdtr.Limit]										// load kernel's gdt
		lidt		[esi + KSPECIAL_REGISTERS.Idtr.Limit]										// load kernel's idt
		lldt		[esi + KSPECIAL_REGISTERS.Ldtr]												// load kernel's ldt
		movzx		eax,[esi + KSPECIAL_REGISTERS.Tr]											// eax = kernel's tr selector
		and			byte ptr [ecx + eax + 5],0xfd												// clear busy flags
		ltr			ax																			// load kernel's tss
		mov			ds,word ptr [ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.SegDs]	// load kernel's ds
		mov			es,word ptr [ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.SegEs]	// load kernel's es
		mov			fs,word ptr [ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.SegFs]	// load kernel's fs
		mov			gs,word ptr [ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.SegGs]	// load kernel's gs
		mov			ss,word ptr [ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.SegSs]	// load kernel's ss
		mov			esp,[ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.Esp]				// switch to kernel's stack
		mov			ebx,[ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.Ebx]				// load kernel's ebx
		mov			ecx,[ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.Ecx]				// load kernel's ecx
		mov			edx,[ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.Edx]				// load kernel's edx
		mov			edi,[ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.Edi]				// load kernel's edi
		push		[ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.EFlags]				// push kernel's eflags
		movzx		eax,word ptr [ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.SegCs]	// eax = kernel's cs
		push		eax																			// push kernel's cs
		push		[ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.Eip]					// push kernel's eip
		push		[ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.Ebp]					// save kernel's ebp in kernel's stack
		push		[ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.Esi]					// save kernel's esi in kernel's stack
		push		[ebp + HIBER_WAKE_CONTEXT.ProcessorState.ContextFrame.Eax]					// save kernel's eax in kernel's stack
		lea			esi,[ebp + HIBER_WAKE_CONTEXT.ProcessorState.SpecialRegisters.KernelDr0]	// esi point to kernel DRx position
		lodsd																					// eax = KernelDr0
		mov			dr0,eax																		// set dr0
		lodsd																					// eax = KernelDr1
		mov			dr1,eax																		// set dr0
		lodsd																					// eax = KernelDr2
		mov			dr2,eax																		// set dr0
		lodsd																					// eax = KernelDr3
		mov			dr3,eax																		// set dr0
		lodsd																					// eax = KernelDr6
		mov			dr6,eax																		// set dr0
		lodsd																					// eax = KernelDr7
		mov			dr7,eax
		pop			eax																			// eax = kernel's eax
		pop			esi																			// esi = kernel's esi
		pop			ebp																			// ebp = kernel's ebp
		iretd																					// "interrupt return" to kernel

ReturnToOsLoader:
		mov			esp,[ebp + HIBER_WAKE_CONTEXT.SavedEsp]										// restore caller's esp
		pop			ebp																			// restore caller's ebp
		pop			ebx																			// restore caller's ebx
		pop			esi																			// restore caller's esi
		pop			edi																			// restore caller's edi
		retn																					// return to caller

		//
		// __fastcall
		//	input  ecx = count
		//	output eax =  buffer
		//
AllocateInHiberHeap:
		mov			eax,[ebp + HIBER_WAKE_CONTEXT.HiberHeap]									// eax = current heap buffer position
		mov			edx,eax																		// save in edx
		test		eax,0x1f																	// 32 bytes aligned ?
		jz			TryToAllocate																// already aligned

		and			eax,0xffffffe0																// align to the next 32 bytes bound
		add			eax,0x20

TryToAllocate:
		add			ecx,eax																		// 'allocate' space with length = ecx
		mov			[ebp + HIBER_WAKE_CONTEXT.HiberHeap],ecx									// update heap buffer position
		xor			ecx,edx																		// ecx = new,edx = old
		and			ecx,0xfffff000																// must be in the same page
		jnz			ReturnToOsLoader															// nz,buffer overflow
		retn

		//
		// input  eax = page frame
		// output eax = remapped frame
		//
GetHiberPageFrame:
		mov			edx,[ebp + HIBER_WAKE_CONTEXT.FirstRemap]									// edx = first remap
		dec			edx																			// balance the next inc

GetHiberPageFrameLoop:
		inc			edx																			// advance to next one
		cmp			edx,[ebp + HIBER_WAKE_CONTEXT.LastRemap]									// end of remap?
		jnb			GetHiberPageFrameReturn														// not found,return the input one(not remapped)

		cmp			eax,[ebp + edx * 4 + HIBER_WAKE_CONTEXT.OriginalPageFrame]					// check those remap record in old map
		jnz			GetHiberPageFrameLoop														// not the same,try the next one

		mov			eax,[ebp + edx * 4 + HIBER_WAKE_CONTEXT.RemappedPageFrame]					// found remap record,get remapped page frame number and return
GetHiberPageFrameReturn:
		retn

		//
		// PVOID __stdcall MapInHiberPtesX86(ULONG index,ULONG page)
		//
MapInHiberPtesX86:
		push		ecx																			// save ecx
		mov			eax,[esp + 8]																// eax = map index
		shl			eax,2																		// sizeof(HARDWARE_PTE) == 1 << 2 == 4
		add			eax,[ebp + HIBER_WAKE_CONTEXT.HiberPtes]									// eax = pte
		mov			ecx,[esp + 0xc]																// ecx = page frame number
		shl			ecx,12																		// shift to HARDWARE_PTE.PageFrameNumber
		or			ecx,0x23																	// Valid|Write|Accessed
		mov			[eax],ecx																	// set pte
		mov			eax,[esp + 8]																// eax = index
		shl			eax,12																		// eax = offset from hiber context
		add			eax,ebp																		// eax = mapped virtual address
		invlpg		[eax]																		// invalid TLB
		pop			ecx																			// restore ecx
		retn		8																			// __stdcall return

		//
		// PVOID __stdcall MapInHiberPtesPAE(ULONG index,ULONG page)
		//
MapInHiberPtesPAE:
		push		ecx																			// save ecx
		mov			eax,[esp + 8]																// eax = index
		shl			eax,3																		// sizeof(HARDWARE_PTE_PAE) == 1 << 3 == 8
		add			eax,[ebp + HIBER_WAKE_CONTEXT.HiberPtes]									// eax = pte
		mov			ecx,[esp + 0xc]																// ecx = page frame number
		shl			ecx,12																		// shift to HARDWARE_PTE_PAE.PageFrameNumber
		or			ecx,0x23																	// Valid|Write|Accessed
		mov			[eax],ecx																	// write LOW UINT32
		mov			[eax + 4],0																	// zero out HI UINT32,page is always under 4GB
		mov			eax,[esp + 8]																// eax = index
		shl			eax,12																		// eax = offset from hiber context
		add			eax,ebp																		// eax = mapped virtual address
		invlpg		[eax]																		// invalid TLB
		pop			ecx																			// restore ecx
		retn		8																			// __stdcall return
	}
}

ULONG WakeDispatcherEnd()
{
	return 0x12345678;
}

#pragma code_seg(pop)
#pragma optimize("",on)

#endif