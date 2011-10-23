//********************************************************************
//	created:	17:8:2008   22:01
//	file:		blcache.h
//	author:		tiamo
//	purpose:	disk cache
//********************************************************************

#include "stdafx.h"

//
// cache range destructor
//
typedef VOID (*PDISK_CACHE_RANGE_DESCTRUCTOR)(__in struct _DISK_CACHE_RANGE* Range);

//
// merge range
//
typedef BOOLEAN (*PDISK_CACHE_RANGE_MERGE)(__in struct _DISK_CACHE_RANGE* Range1,__in struct _DISK_CACHE_RANGE* Range2);

//
// cached range
//
typedef struct _DISK_CACHE_RANGE
{
	//
	// in used list entry
	//
	LIST_ENTRY											InUsedListEntry;

	//
	// start
	//
	ULARGE_INTEGER										StartOffset;

	//
	// end
	//
	ULARGE_INTEGER										EndOffset;

	//
	// buffer
	//
	PVOID												Buffer;

	//
	// free list entry
	//
	LIST_ENTRY											FreeListEntry;
}DISK_CACHE_RANGE,*PDISK_CACHE_RANGE;

//
// cached range list
//
typedef struct _DISK_CACHE_RANGE_LIST
{
	//
	// ranges' list header
	//
	LIST_ENTRY											RangeListHead;

	//
	// ranges count
	//
	LONG												RangesCount;

	//
	// merge
	//
	PDISK_CACHE_RANGE_MERGE								MergeRange;


	//
	// destructor
	//
	PDISK_CACHE_RANGE_DESCTRUCTOR						RangeDesctructor;
}DISK_CACHE_RANGE_LIST,*PDISK_CACHE_RANGE_LIST;

//
// disk cache entry
//
typedef struct _DISK_CACHE_ENTRY
{
	//
	// in used
	//
	BOOLEAN												InUsed;

	//
	// device id
	//
	ULONG												CachedDeviceId;

	//
	// cached range list
	//
	DISK_CACHE_RANGE_LIST								CachedRangeList;
}DISK_CACHE_ENTRY,*PDISK_CACHE_ENTRY;

//
// disk cache info
//
typedef struct _DISK_CACHE_INFO
{
	//
	// cached device array
	//
	DISK_CACHE_ENTRY									CachedDevices[2];

	//
	// list head
	//
	LIST_ENTRY											LRUListHead;

	//
	// starting page
	//
	PVOID												CacheStartAddress;

	//
	// range list base page
	//
	PVOID												RangeListAddress;

	//
	// free range list head
	//
	LIST_ENTRY											FreeRangeListHead;

	//
	// intitialized
	//
	BOOLEAN												CacheInitialized;
}DISK_CACHE_INFO,*PDISK_CACHE_INFO;

//
// disk cache
//
DISK_CACHE_INFO											BlDiskCache;

//
// merge range list
//
BOOLEAN BlDiskCacheMergeRangeRoutine(__in struct _DISK_CACHE_RANGE* Range1,__in struct _DISK_CACHE_RANGE* Range2)
{
	return FALSE;
}

//
// allocate range entry
//
PDISK_CACHE_RANGE BlDiskCacheAllocateRangeEntry()
{
	if(IsListEmpty(&BlDiskCache.FreeRangeListHead))
		return 0;

	PLIST_ENTRY Entry									= RemoveHeadList(&BlDiskCache.FreeRangeListHead);
	return CONTAINING_RECORD(Entry,DISK_CACHE_RANGE,FreeListEntry);
}

//
// free range
//
VOID BlDiskCacheFreeRangeEntry(__in PDISK_CACHE_RANGE Range)
{
	//
	// insert into free list
	//
	InsertTailList(&BlDiskCache.FreeRangeListHead,&Range->FreeListEntry);
}

//
// free range
//
VOID BlDiskCacheFreeRangeRoutine(__in PDISK_CACHE_RANGE Range)
{
	Range->InUsedListEntry.Flink						= Range->FreeListEntry.Flink;
	Range->InUsedListEntry.Blink						= Range->FreeListEntry.Blink;

	BlDiskCacheFreeRangeEntry(Range);
}

//
// initialize
//
VOID BlRangeListInitialize(__out PDISK_CACHE_RANGE_LIST RangeList,__in PDISK_CACHE_RANGE_MERGE Merge,__in PDISK_CACHE_RANGE_DESCTRUCTOR Destructor)
{
	RangeList->MergeRange								= Merge;
	RangeList->RangeDesctructor							= Destructor;
	RangeList->RangesCount								= 0;
	InitializeListHead(&RangeList->RangeListHead);
}

//
// find overlapped range
//
BOOLEAN BlRangeListFindOverlaps(__in PDISK_CACHE_RANGE_LIST RangeList,__in PULARGE_INTEGER StartEnd,__out PDISK_CACHE_RANGE* RangesArray,__in ULONG Size,__out PULONG Count)
{
	*Count												= 0;

	//
	// range is empty,return
	//
	if(StartEnd[0].QuadPart == StartEnd[1].QuadPart)
		return RangesArray != 0;

	for(PLIST_ENTRY NextEntry = RangeList->RangeListHead.Flink; NextEntry != &RangeList->RangeListHead; NextEntry = NextEntry->Flink)
	{
		PDISK_CACHE_RANGE Range							= CONTAINING_RECORD(NextEntry,DISK_CACHE_RANGE,InUsedListEntry);

		//
		// current range is completely bebind end point
		//
		if(StartEnd[1].QuadPart <= Range->StartOffset.QuadPart)
			continue;

		//
		// current range is completely below start point
		//
		if(StartEnd[0].QuadPart >= Range->EndOffset.QuadPart)
			continue;

		//
		// add to ranges array
		//
		if(RangesArray && *Count < Size / sizeof(PDISK_CACHE_RANGE))
			RangesArray[*Count]							= Range;

		//
		// increase count
		//
		*Count											+= 1;
	}

	if(!RangesArray || *Count <= Size / sizeof(PDISK_CACHE_RANGE))
		return TRUE;

	return FALSE;
}

//
// find distinct ranges
//
BOOLEAN BlRangeListFindDistinctRanges(__in PDISK_CACHE_RANGE_LIST RangeList,__in PULARGE_INTEGER StartEnd,__out PULONGLONG RunsArray,__in ULONG Size,__out PULONG Count)
{
	*Count												= 0;

	//
	// range is empty,return
	//
	if(StartEnd[0].QuadPart == StartEnd[1].QuadPart)
		return RunsArray != 0;

	ULARGE_INTEGER Current								= StartEnd[0];
	ULARGE_INTEGER End									= StartEnd[1];
	PLIST_ENTRY NextEntry								= RangeList->RangeListHead.Flink;
	while(NextEntry != &RangeList->RangeListHead)
	{
		PDISK_CACHE_RANGE Range							= CONTAINING_RECORD(NextEntry,DISK_CACHE_RANGE,InUsedListEntry);

		if(Current.QuadPart >= End.QuadPart)
			break;

		if(End.QuadPart <= Range->StartOffset.QuadPart)
		{
			//
			// 41AB68
			//
			if(RunsArray && *Count < Size / (sizeof(ULARGE_INTEGER) * 2))
			{
				RunsArray[*Count * 2]					= Current.QuadPart;
				RunsArray[*Count * 2 + 1]				= End.QuadPart;
			}

			*Count										+= 1;

			Current.QuadPart							= End.QuadPart;
		}

		//
		// 41AB99
		//
		if(End.QuadPart > Range->StartOffset.QuadPart && Current.QuadPart < Range->EndOffset.QuadPart)
		{
			//
			// 41ABC6
			//
			ULONGLONG WriteEnd							= Current.QuadPart < Range->StartOffset.QuadPart ? Range->StartOffset.QuadPart : Current.QuadPart;

			//
			// 41ABE5
			//
			ULONGLONG Next								= End.QuadPart > Range->EndOffset.QuadPart ? Range->EndOffset.QuadPart : End.QuadPart;

			if(WriteEnd > StartEnd[0].QuadPart)
			{
				//
				// 41AC22
				//
				if(RunsArray && *Count < Size / (sizeof(ULARGE_INTEGER) * 2))
				{
					RunsArray[*Count * 2]				= Current.QuadPart;
					RunsArray[*Count * 2 + 1]			= WriteEnd;
				}

				*Count									+= 1;
			}

			Current.QuadPart							= Next;
		}

		NextEntry										= NextEntry->Flink;
	}

	if(Current.QuadPart < End.QuadPart)
	{
		//
		// add the last one
		//
		if(RunsArray && *Count < Size / (sizeof(ULARGE_INTEGER) * 2))
		{
			RunsArray[*Count * 2]						= Current.QuadPart;
			RunsArray[*Count * 2 + 1]					= End.QuadPart;
		}

		*Count											+= 1;
	}

	if(!RunsArray || *Count <= Size / (sizeof(ULARGE_INTEGER) * 2))
		return TRUE;

	return FALSE;
}

//
// merge entry
//
BOOLEAN BlRangeEntryMerge(__inout PDISK_CACHE_RANGE Range1,__in PDISK_CACHE_RANGE Range2,__in PDISK_CACHE_RANGE_MERGE MergeRoutine)
{
	DISK_CACHE_RANGE SavedRang1							= *Range1;
	if(MergeRoutine && !MergeRoutine(&SavedRang1,Range2))
		return FALSE;

	if(SavedRang1.StartOffset.QuadPart > Range2->StartOffset.QuadPart)
		SavedRang1.StartOffset.QuadPart					= Range2->StartOffset.QuadPart;

	if(SavedRang1.EndOffset.QuadPart < Range2->EndOffset.QuadPart)
		SavedRang1.EndOffset.QuadPart					= Range2->EndOffset.QuadPart;

	*Range1												= SavedRang1;

	return TRUE;
}

//
// merge range
//
BOOLEAN BlRangeListMergeRangeEntries(__in PDISK_CACHE_RANGE_LIST RangeList,__in PDISK_CACHE_RANGE Range1,__in PDISK_CACHE_RANGE Range2)
{
	//
	// merge entry
	//
	if(!BlRangeEntryMerge(Range1,Range2,RangeList->MergeRange))
		return FALSE;

	//
	// remove entry and free it
	//
	RemoveEntryList(&Range2->InUsedListEntry);

	if(RangeList->RangeDesctructor)
		RangeList->RangeDesctructor(Range2);

	return TRUE;
}

//
// add range
//
BOOLEAN BlRangeListAddRange(__in PDISK_CACHE_RANGE_LIST RangeList,__in PDISK_CACHE_RANGE Range)
{
	//
	// empty range
	//
	if(Range->StartOffset.QuadPart == Range->EndOffset.QuadPart)
		return TRUE;

	//
	// search insert point
	//
	PLIST_ENTRY NextEntry								= RangeList->RangeListHead.Flink;
	PDISK_CACHE_RANGE MergePrevRange					= 0;
	while(NextEntry != &RangeList->RangeListHead)
	{
		PDISK_CACHE_RANGE NextRange						= CONTAINING_RECORD(NextEntry,DISK_CACHE_RANGE,InUsedListEntry);

		//
		// new range is below current one
		//
		if(Range->EndOffset.QuadPart <= NextRange->StartOffset.QuadPart)
		{
			//
			// insert into list
			//
			InsertTailList(&NextRange->InUsedListEntry,&Range->InUsedListEntry);
			RangeList->RangesCount						+= 1;

			//
			// check we can merge with prev range
			//
			if(MergePrevRange && Range->StartOffset.QuadPart == MergePrevRange->EndOffset.QuadPart)
				BlRangeListMergeRangeEntries(RangeList,Range,MergePrevRange);

			//
			// merge current
			//
			if(Range->EndOffset.QuadPart == NextRange->StartOffset.QuadPart)
				BlRangeListMergeRangeEntries(RangeList,Range,NextRange);

			return TRUE;
		}

		//
		// current one is overlapped new range
		//
		if(Range->StartOffset.QuadPart < NextRange->EndOffset.QuadPart)
			return FALSE;

		MergePrevRange									= NextRange;
		NextEntry										= NextEntry->Flink;
	}

	//
	// insert into list
	//
	InsertTailList(&RangeList->RangeListHead,&Range->InUsedListEntry);
	RangeList->RangesCount								+= 1;

	//
	// check we can merge with prev range
	//
	if(MergePrevRange && Range->StartOffset.QuadPart == MergePrevRange->EndOffset.QuadPart)
		BlRangeListMergeRangeEntries(RangeList,Range,MergePrevRange);

	return TRUE;
}

//
// remove range
//
VOID BlRangeListRemoveRange(__in PDISK_CACHE_RANGE_LIST RangeList,__in PULARGE_INTEGER StartEnd)
{
	//
	// range is empty,return
	//
	if(StartEnd[0].QuadPart == StartEnd[1].QuadPart)
		return;

	PLIST_ENTRY NextEntry								= RangeList->RangeListHead.Flink;
	while(NextEntry != &RangeList->RangeListHead)
	{
		PDISK_CACHE_RANGE Range							= CONTAINING_RECORD(NextEntry,DISK_CACHE_RANGE,InUsedListEntry);

		//
		// goto the next one
		//
		NextEntry										= NextEntry->Flink;

		//
		// current range is completely bebind end point
		//
		if(StartEnd[1].QuadPart <= Range->StartOffset.QuadPart)
			continue;

		//
		// current range is completely below start point
		//
		if(StartEnd[0].QuadPart >= Range->EndOffset.QuadPart)
			continue;

		//
		// remove current range
		//
		RangeList->RangesCount							-= 1;
		RemoveEntryList(&Range->InUsedListEntry);

		//
		// call destructor
		//
		if(RangeList->RangeDesctructor)
			RangeList->RangeDesctructor(Range);
	}
}

//
// remove all ranges
//
VOID BlRangeListRemoveAllRanges(__in PDISK_CACHE_RANGE_LIST RangeList)
{
	PLIST_ENTRY NextEntry								= RangeList->RangeListHead.Flink;
	while(NextEntry != &RangeList->RangeListHead)
	{
		PDISK_CACHE_RANGE Range							= CONTAINING_RECORD(NextEntry,DISK_CACHE_RANGE,InUsedListEntry);

		//
		// goto the next one
		//
		NextEntry										= NextEntry->Flink;

		//
		// decrement count
		//
		RangeList->RangesCount							-= 1;

		//
		// remove it from the list
		//
		RemoveEntryList(&Range->InUsedListEntry);

		//
		// call destructor
		//
		if(RangeList->RangeDesctructor)
			RangeList->RangeDesctructor(Range);
	}
}

//
// find cache entry from device id
//
PDISK_CACHE_ENTRY BlDiskCacheFindCacheForDevice(__in ULONG DeviceId)
{
	//
	// cache has not been initialized
	//
	if(!BlDiskCache.CacheInitialized || !BlDiskCache.CacheStartAddress)
		return 0;

	//
	// search with device id
	//
	for(ULONG i = 0; i < ARRAYSIZE(BlDiskCache.CachedDevices); i ++)
	{
		if(BlDiskCache.CachedDevices[i].InUsed && BlDiskCache.CachedDevices[i].CachedDeviceId == DeviceId)
			return BlDiskCache.CachedDevices + i;
	}

	return 0;
}

//
// initialize disk cache
//
ARC_STATUS BlDiskCacheInitialize()
{
	//
	// already initialized
	//
	if(BlDiskCache.CacheInitialized)
		return ESUCCESS;

	//
	// save base and limit
	//
	ULONG SavedBase										= BlUsableBase;
	ULONG SavedLimit									= BlUsableLimit;

	//
	// setup with a new one
	//
	BlUsableLimit										= 0x1000;
	BlUsableBase										= 0x800;

	//
	// check if we are booting with /3GB
	//
	if(BlLoaderBlock->LoadOptions && (strstr(BlLoaderBlock->LoadOptions,"3GB") || strstr(BlLoaderBlock->LoadOptions,"3gb")))
		BlUsableBase									= 0;

	//
	// allocate cache buffer
	//
	ULONG BasePage;
	ARC_STATUS Status									= BlAllocateAlignedDescriptor(LoaderOsloaderHeap,0,0x201,0x10,&BasePage);
	if(Status == ESUCCESS)
	{
		//
		// save it
		//
		BlDiskCache.CacheStartAddress					= MAKE_KERNEL_ADDRESS_PAGE(BasePage);

		//
		// allocate range buffer
		//
		Status											= BlAllocateAlignedDescriptor(LoaderOsloaderHeap,0,1,1,&BasePage);
		if(Status == ESUCCESS)
		{
			//
			// save it
			//
			BlDiskCache.RangeListAddress				= MAKE_KERNEL_ADDRESS_PAGE(BasePage);

			//
			// initialize cache table
			//
			for(ULONG i = 0; i < ARRAYSIZE(BlDiskCache.CachedDevices); i ++)
				BlDiskCache.CachedDevices[i].InUsed		= FALSE;

			//
			// init list head
			//
			InitializeListHead(&BlDiskCache.FreeRangeListHead);
			InitializeListHead(&BlDiskCache.LRUListHead);

			//
			// for each range,it caches 8 pages
			//
			PDISK_CACHE_RANGE Range						= static_cast<PDISK_CACHE_RANGE>(BlDiskCache.RangeListAddress);
			for(ULONG i = 0; i < 0x200 / 0x8; i ++, Range ++)
			{
				Range->StartOffset.QuadPart				= 0;
				Range->EndOffset.QuadPart				= 0;

				//
				// setup buffer
				//
				Range->Buffer							= Add2Ptr(BlDiskCache.CacheStartAddress,(i * 8) << PAGE_SHIFT,PVOID);

				//
				// link to free list
				//
				InsertHeadList(&BlDiskCache.FreeRangeListHead,&Range->FreeListEntry);
			}

			//
			// initialized
			//
			BlDiskCache.CacheInitialized				= TRUE;
		}
	}

	//
	// restore saved values
	//
	BlUsableBase										= SavedBase;
	BlUsableLimit										= SavedLimit;

	if(Status == ESUCCESS)
		return ESUCCESS;

	//
	// free buffers if failed
	//
	if(BlDiskCache.CacheStartAddress)
		BlFreeDescriptor(GET_PHYSICAL_PAGE(BlDiskCache.CacheStartAddress));

	if(BlDiskCache.RangeListAddress)
		BlFreeDescriptor(GET_PHYSICAL_PAGE(BlDiskCache.RangeListAddress));

	return Status;
}

//
// starting caching on device
//
PVOID BlDiskCacheStartCachingOnDevice(__in ULONG DeviceId)
{
	//
	// not initialized
	//
	if(!BlDiskCache.CacheInitialized || !BlDiskCache.CacheStartAddress)
		return 0;

	//
	// already cached
	//
	PDISK_CACHE_ENTRY Entry								= BlDiskCacheFindCacheForDevice(DeviceId);
	if(Entry)
		return 0;

	//
	// find an empty entry
	//
	for(ULONG i = 0; i < ARRAYSIZE(BlDiskCache.CachedDevices); i ++)
	{
		if(BlDiskCache.CachedDevices[i].InUsed)
			continue;

		BlDiskCache.CachedDevices[i].InUsed				= TRUE;
		BlDiskCache.CachedDevices[i].CachedDeviceId		= DeviceId;
		BlRangeListInitialize(&BlDiskCache.CachedDevices->CachedRangeList,&BlDiskCacheMergeRangeRoutine,&BlDiskCacheFreeRangeRoutine);

		return BlDiskCache.CachedDevices + i;
	}

	return 0;
}

//
// stop cache on device
//
VOID BlDiskCacheStopCachingOnDevice(__in ULONG DeviceId)
{
	//
	// cache has not been initialized
	//
	if(!BlDiskCache.CacheInitialized || !BlDiskCache.CacheStartAddress)
		return;

	//
	// find cache entry
	//
	PDISK_CACHE_ENTRY Entry								= BlDiskCacheFindCacheForDevice(DeviceId);
	if(!Entry)
		return;

	//
	// remove cached ranges
	//
	BlRangeListRemoveAllRanges(&Entry->CachedRangeList);
}

//
// cache read
//
ARC_STATUS BlDiskCacheRead(__in ULONG DeviceId,__in PLARGE_INTEGER Offset,__in PVOID Buffer,__in ULONG Count,__in PULONG ReadCount,__in BOOLEAN SaveDataToCache)
{
	*ReadCount											= 0;
	BOOLEAN ReadDisk									= TRUE;
	ARC_STATUS Status									= ESUCCESS;
	PDISK_CACHE_RANGE NewRange							= 0;

	__try
	{
		PDISK_CACHE_ENTRY CacheEntry					= BlDiskCacheFindCacheForDevice(DeviceId);
		if(!CacheEntry)
			__leave;

		//
		// this device has a cache entry,try to use it
		//
		ULARGE_INTEGER ParamInfo[2];
		ParamInfo[0].QuadPart							= static_cast<ULONGLONG>(Offset->QuadPart);
		ParamInfo[1].QuadPart							= static_cast<ULONGLONG>(Offset->QuadPart + Count);

		//
		// get overlapped ranges
		//
		PDISK_CACHE_RANGE RangesArray[80]				= {0};
		ULONG RangesCount								= 0;
		if(!BlRangeListFindOverlaps(&CacheEntry->CachedRangeList,ParamInfo,RangesArray,sizeof(RangesArray),&RangesCount))
			__leave;

		for(ULONG i = 0; i < RangesCount; i ++)
		{
			PDISK_CACHE_RANGE Range						= RangesArray[i];

			//
			// remove it from list
			//
			RemoveEntryList(&Range->FreeListEntry);

			//
			// insert into LRU list
			//
			InsertHeadList(&BlDiskCache.LRUListHead,&Range->FreeListEntry);

			//
			// compute start and end offset
			//
			ULONGLONG StartOffset						= static_cast<ULONGLONG>(Offset->QuadPart);
			ULONGLONG EndOffset							= StartOffset + Count;

			if(StartOffset < Range->StartOffset.QuadPart)
				StartOffset								= Range->StartOffset.QuadPart;

			if(EndOffset > Range->EndOffset.QuadPart)
				EndOffset								= Range->EndOffset.QuadPart;

			//
			// copy content to user buffer
			//
			ULONG Length								= static_cast<ULONG>(EndOffset - StartOffset);
			PVOID SrcBuffer								= Add2Ptr(Range->Buffer,static_cast<ULONG>(StartOffset - Range->StartOffset.QuadPart),PVOID);
			PVOID DstBuffer								= Add2Ptr(Buffer,static_cast<ULONG>(StartOffset - Offset->QuadPart),PVOID);
			RtlCopyMemory(DstBuffer,SrcBuffer,Length);

			//
			// count length
			//
			*ReadCount									+= Length;
		}

		//
		// we've read all data needed ?
		//
		if(*ReadCount != Count)
		{
			//
			// get distinct ranges
			//
			ULONGLONG RunsArray[40]						= {0};
			ULONG RunsCount								= 0;
			if(!BlRangeListFindDistinctRanges(&CacheEntry->CachedRangeList,ParamInfo,RunsArray,sizeof(RunsArray),&RunsCount))
				__leave;

			for(ULONG i = 0; i < RunsCount; i ++)
			{
				ULONGLONG StartOffset					= RunsArray[i * 2];
				ULONGLONG EndOffset						= RunsArray[i * 2 + 1];

				if(SaveDataToCache)
				{
					ULONG HeadOffset					= static_cast<ULONG>(StartOffset) & 0x7fff;
					ULONG TailOffset					= static_cast<ULONG>(EndOffset) & 0x7fff;
					ULONGLONG RoundStartOffset			= StartOffset & 0xffffffffffff8000ULL;
					ULONGLONG RoundEndOffset			= EndOffset & 0xffffffffffff8000ULL;
					ULONGLONG CurrentOffset				= RoundStartOffset;
					PUCHAR DstBuffer					= Add2Ptr(Buffer,static_cast<ULONG>(StartOffset	- Offset->QuadPart),PUCHAR);
					PUCHAR EndOfDstBuffer				= Add2Ptr(Buffer,static_cast<ULONG>(EndOffset - Offset->QuadPart),PUCHAR);

					while(DstBuffer < EndOfDstBuffer)
					{
						//
						// allocate range
						//
						NewRange						= BlDiskCacheAllocateRangeEntry();
						if(!NewRange && !IsListEmpty(&BlDiskCache.LRUListHead))
						{
							//
							// free a used range
							//
							PDISK_CACHE_RANGE FreeRange	= CONTAINING_RECORD(BlDiskCache.LRUListHead.Blink,DISK_CACHE_RANGE,FreeListEntry);
							BlRangeListRemoveRange(&CacheEntry->CachedRangeList,&FreeRange->StartOffset);

							//
							// allocate again
							//
							NewRange					= BlDiskCacheAllocateRangeEntry();
						}

						//
						// unable to get a free range
						//
						if(!NewRange)
							__leave;

						//
						// setup it
						//
						NewRange->StartOffset.QuadPart	= CurrentOffset;
						NewRange->EndOffset.QuadPart	= CurrentOffset + 0x8000;

						//
						// read disk data
						//
						Status							= ArcSeek(DeviceId,reinterpret_cast<PLARGE_INTEGER>(&NewRange->StartOffset),SeekAbsolute);
						if(Status != ESUCCESS)
							__leave;

						ULONG ReadLength				= 0;
						Status							= ArcRead(DeviceId,NewRange->Buffer,0x8000,&ReadLength);
						if(Status != ESUCCESS || ReadLength != 0x8000)
							__leave;

						//
						// add it
						//
						if(!BlRangeListAddRange(&CacheEntry->CachedRangeList,NewRange))
							__leave;

						//
						// insert into LRU list
						//
						InsertHeadList(&BlDiskCache.LRUListHead,&NewRange->FreeListEntry);

						//
						// copy to user buffer
						//
						PVOID SrcBuffer					= NewRange->Buffer;
						if(CurrentOffset == RoundStartOffset)
						{
							ReadLength					-= HeadOffset;
							SrcBuffer					= Add2Ptr(SrcBuffer,HeadOffset,PVOID);
						}

						if(CurrentOffset == RoundEndOffset)
							ReadLength					-= (0x8000 - TailOffset);

						RtlCopyMemory(DstBuffer,SrcBuffer,ReadLength);

						DstBuffer						+= ReadLength;
						CurrentOffset					= NewRange->EndOffset.QuadPart;
						NewRange						= 0;
						*ReadCount						+= ReadLength;
					}
				}
				else
				{
					//
					// read data from disk directly
					//
					ULONG Length						= static_cast<ULONG>(EndOffset - StartOffset);
					LARGE_INTEGER SeekOffset;
					SeekOffset.QuadPart					= static_cast<LONGLONG>(StartOffset);
					PVOID DstBuffer						= Add2Ptr(Buffer,static_cast<ULONG>(StartOffset - Offset->QuadPart),PVOID);
					Status								= ArcSeek(DeviceId,&SeekOffset,SeekAbsolute);
					if(Status != ESUCCESS)
						__leave;

					ULONG ReadLength					= 0;
					Status								= ArcRead(DeviceId,DstBuffer,Length,&ReadLength);
					if(Status != ESUCCESS || ReadLength != Length)
						__leave;

					*ReadCount							+= ReadLength;
				}
			}
		}

		if(*ReadCount == Count)
		{
			//
			// seek to end position
			//
			LARGE_INTEGER EndPos;
			EndPos.QuadPart								= Offset->QuadPart + Count;
			Status										= ArcSeek(DeviceId,&EndPos,SeekAbsolute);
			ReadDisk									= Status == ESUCCESS ? FALSE : TRUE;
		}
	}
	__finally
	{
		//
		// free allocated range
		//
		if(NewRange)
			BlDiskCacheFreeRangeEntry(NewRange);

		//
		// get data from disk
		//
		if(ReadDisk)
		{
			*ReadCount									= 0;
			Status										= ArcSeek(DeviceId,Offset,SeekAbsolute);
			if(Status == ESUCCESS)
				Status									= ArcRead(DeviceId,Buffer,Count,ReadCount);
		}
	}

	return Status;
}

//
// cache write
//
ARC_STATUS BlDiskCacheWrite(__in ULONG DeviceId,__in PLARGE_INTEGER Offset,__in PVOID Buffer,__in ULONG Count,__in PULONG WritenCount)
{
	PDISK_CACHE_ENTRY Entry								= BlDiskCacheFindCacheForDevice(DeviceId);
	if(Entry)
	{
		ULARGE_INTEGER ParamInfo[2];
		ParamInfo[0].QuadPart							= static_cast<ULONGLONG>(Offset->QuadPart);
		ParamInfo[1].QuadPart							= static_cast<ULONGLONG>(Offset->QuadPart + Count);

		BlRangeListRemoveRange(&Entry->CachedRangeList,ParamInfo);
	}

	ARC_STATUS Status									= ArcSeek(DeviceId,Offset,SeekAbsolute);
	if(Status != ESUCCESS)
		return Status;

	return ArcWrite(DeviceId,Buffer,Count,WritenCount);
}

//
// uninitialize
//
VOID BlDiskCacheUninitialize()
{
	//
	// stop cache
	//
	for(ULONG i = 0; i < ARRAYSIZE(BlDiskCache.CachedDevices); i ++)
	{
		if(BlDiskCache.CachedDevices[i].InUsed)
			BlDiskCacheStopCachingOnDevice(BlDiskCache.CachedDevices[i].CachedDeviceId);
	}

	//
	// free memory
	//
	if(BlDiskCache.CacheStartAddress)
		BlFreeDescriptor(GET_PHYSICAL_PAGE(BlDiskCache.CacheStartAddress));

	if(BlDiskCache.RangeListAddress)
		BlFreeDescriptor(GET_PHYSICAL_PAGE(BlDiskCache.RangeListAddress));

	//
	// set as not initialized
	//
	BlDiskCache.CacheInitialized						= FALSE;
}