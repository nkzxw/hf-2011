#include <ntifs.h>
#include <windef.h>
#include "Bitmap.h"

static tBitmap bitmapMask[8] =
{
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
};

void * BitmapAlloc(
	int             poolType,
	unsigned long   length
	)
{

	if (0 == poolType)
	{
		return ExAllocatePoolWithTag(NonPagedPool, length , 'mbpD');
	}
	else if (1 == poolType)
	{
		return ExAllocatePoolWithTag(PagedPool, length , 'mbpD');
	}
	else
	{
		return NULL;
	}
}

void BitmapFree(DP_BITMAP* bitmap)
{

	DWORD i = 0;

	if (NULL != bitmap)
	{
		if (NULL != bitmap->Bitmap)
		{
			for (i = 0; i < bitmap->regionNumber; i++)
			{
				if (NULL != *(bitmap->Bitmap + i))
				{				
					ExFreePool(*(bitmap->Bitmap + i));
				}
			}
			ExFreePool(bitmap->Bitmap);
		}	
		ExFreePool(bitmap);
	}
}

void BitmapInitLock(void * lock){}

void BitmapLock(void * lock){}

void BitmapUnlock(void * lock){}

NTSTATUS BitmapInit(
	DP_BITMAP **     bitmap,
	unsigned long       sectorSize,
	unsigned long       byteSize,
	unsigned long       regionSize,
	unsigned long       regionNumber
	)
{
	int i = 0;
	DP_BITMAP * myBitmap = NULL;
	NTSTATUS status = STATUS_SUCCESS;


	if (NULL == bitmap || 0 == sectorSize ||
		0 == byteSize || 0 == regionSize  || 0 == regionNumber)
	{
		return STATUS_UNSUCCESSFUL;
	}
	__try
	{
	
		if (NULL == (myBitmap = (DP_BITMAP*)BitmapAlloc(0, sizeof(DP_BITMAP))))
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		memset(myBitmap, 0, sizeof(DP_BITMAP));

		myBitmap->sectorSize = sectorSize;
		myBitmap->byteSize = byteSize;
		myBitmap->regionSize = regionSize;
		myBitmap->regionNumber = regionNumber;
		myBitmap->regionReferSize = sectorSize * byteSize * regionSize;
		myBitmap->bitmapReferSize = (__int64)sectorSize * (__int64)byteSize * (__int64)regionSize * (__int64)regionNumber;

		if (NULL == (myBitmap->Bitmap = (tBitmap **)BitmapAlloc(0, sizeof(tBitmap*) * regionNumber)))
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		memset(myBitmap->Bitmap, 0, sizeof(tBitmap*) * regionNumber);
		* bitmap = myBitmap;
		status = STATUS_SUCCESS;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		status = STATUS_UNSUCCESSFUL;
	}
	if (!NT_SUCCESS(status))
	{
		if (NULL != myBitmap)
		{
			BitmapFree(myBitmap);
		}
		* bitmap = NULL;
	}
	return status;
}

NTSTATUS BitmapSet(
	DP_BITMAP *      bitmap,
	LARGE_INTEGER       offset,
	unsigned long       length
	)
{
	__int64 i = 0;
	unsigned long myRegion = 0, myRegionEnd = 0;
	unsigned long myRegionOffset = 0, myRegionOffsetEnd = 0;
	unsigned long myByteOffset = 0, myByteOffsetEnd = 0;
	unsigned long myBitPos = 0;
	NTSTATUS status = STATUS_SUCCESS;
	LARGE_INTEGER setBegin = { 0 }, setEnd = { 0 };

	__try
	{

		if (NULL == bitmap || offset.QuadPart < 0)
		{
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		if (0 != offset.QuadPart % bitmap->sectorSize || 0 != length % bitmap
			->sectorSize)
		{
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

	
		myRegion = (unsigned long)(offset.QuadPart / (__int64)bitmap->regionReferSize);
		myRegionEnd = (unsigned long)((offset.QuadPart + (__int64)length) / (__int64)bitmap->regionReferSize);
		for (i = myRegion; i <= myRegionEnd; ++i)
		{
			if (NULL == *(bitmap->Bitmap + i))
			{
				if (NULL == (*(bitmap->Bitmap + i) = (tBitmap*)BitmapAlloc(0, sizeof(tBitmap) * bitmap->regionSize)))
				{
					status = STATUS_INSUFFICIENT_RESOURCES;
					__leave;
				}
				else
				{
					memset(*(bitmap->Bitmap + i), 0, sizeof(tBitmap) * bitmap->regionSize);
				}
			}
		}


		for (i = offset.QuadPart; i < offset.QuadPart + (__int64)length; i += bitmap->sectorSize)
		{
			myRegion = (unsigned long)(i / (__int64)bitmap->regionReferSize);
			myRegionOffset = (unsigned long)(i % (__int64)bitmap->regionReferSize);
			myByteOffset = myRegionOffset / bitmap->byteSize / bitmap->sectorSize;
			myBitPos = (myRegionOffset / bitmap->sectorSize) % bitmap->byteSize;
			if (0 == myBitPos)
			{
				setBegin.QuadPart = i;
				break;
			}
			*(*(bitmap->Bitmap + myRegion) + myByteOffset) |= bitmapMask[myBitPos];
		}
		if (i >= offset.QuadPart + (__int64)length)
		{
			status = STATUS_SUCCESS;
			__leave;
		}

		for (i = offset.QuadPart + (__int64)length - bitmap->sectorSize; i >= offset.QuadPart; i -= bitmap->sectorSize)
		{
			myRegion = (unsigned long)(i / (__int64)bitmap->regionReferSize);
			myRegionOffset = (unsigned long)(i % (__int64)bitmap->regionReferSize);
			myByteOffset = myRegionOffset / bitmap->byteSize / bitmap->sectorSize;
			myBitPos = (myRegionOffset / bitmap->sectorSize) % bitmap->byteSize;
			if (7 == myBitPos)
			{
				setEnd.QuadPart = i;
				break;
			}
			*(*(bitmap->Bitmap + myRegion) + myByteOffset) |= bitmapMask[myBitPos];
		}

		if (i < offset.QuadPart || setEnd.QuadPart == setBegin.QuadPart)
		{
			status = STATUS_SUCCESS;
			__leave;
		}

		myRegionEnd = (unsigned long)(setEnd.QuadPart / (__int64)bitmap->regionReferSize);

		for (i = setBegin.QuadPart; i <= setEnd.QuadPart;)
		{
			myRegion = (unsigned long)(i / (__int64)bitmap->regionReferSize);
			myRegionOffset = (unsigned long)(i % (__int64)bitmap->regionReferSize);
			myByteOffset = myRegionOffset / bitmap->byteSize / bitmap->sectorSize;

			if (myRegion == myRegionEnd)
			{
				myRegionOffsetEnd = (unsigned long)(setEnd.QuadPart % (__int64)bitmap->regionReferSize);
				myByteOffsetEnd = myRegionOffsetEnd / bitmap->byteSize / bitmap->sectorSize;
				memset(*(bitmap->Bitmap + myRegion) + myByteOffset, 0xff, myByteOffsetEnd - myByteOffset + 1);
				break;
			}

			else
			{
				myRegionOffsetEnd = bitmap->regionReferSize;
				myByteOffsetEnd = myRegionOffsetEnd / bitmap->byteSize / bitmap->sectorSize;
				memset(*(bitmap->Bitmap + myRegion) + myByteOffset, 0xff, myByteOffsetEnd - myByteOffset);
				i += (myByteOffsetEnd - myByteOffset) * bitmap->byteSize * bitmap->sectorSize;
			}
		}
		status = STATUS_SUCCESS;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		status = STATUS_UNSUCCESSFUL;
	}

	if (!NT_SUCCESS(status))
	{
		
	}
	return status;
}

NTSTATUS BitmapGet(
	DP_BITMAP *    bitmap,
	LARGE_INTEGER     offset,
	unsigned long     length,
	void *            bufInOut,
	void *            bufIn
	)
{
	unsigned long i = 0;
	unsigned long myRegion = 0;
	unsigned long myRegionOffset = 0;
	unsigned long myByteOffset = 0;
	unsigned long myBitPos = 0;
	NTSTATUS status = STATUS_SUCCESS;

	__try
	{

		if (NULL == bitmap || offset.QuadPart < 0 || NULL == bufInOut || NULL == bufIn)
		{
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		if (0 != offset.QuadPart % bitmap->sectorSize || 0 != length % bitmap->sectorSize)
		{
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}


		for (i = 0; i < length; i += bitmap->sectorSize)
		{
			myRegion = (unsigned long)((offset.QuadPart + (__int64)i) / (__int64)bitmap->regionReferSize);

			myRegionOffset = (unsigned long)((offset.QuadPart + (__int64)i) % (__int64)bitmap->regionReferSize);

			myByteOffset = myRegionOffset / bitmap->byteSize / bitmap->sectorSize;

			myBitPos = (myRegionOffset / bitmap->sectorSize) % bitmap->byteSize;

			if (NULL != *(bitmap->Bitmap + myRegion) && (*(*(bitmap->Bitmap + myRegion) + myByteOffset) &bitmapMask[myBitPos]))
			{
				memcpy((tBitmap*)bufInOut + i, (tBitmap*)bufIn + i, bitmap->sectorSize);
			}
		}

		status = STATUS_SUCCESS;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		status = STATUS_UNSUCCESSFUL;
	}

	return status;
}

long BitmapTest(
	DP_BITMAP *      bitmap,
	LARGE_INTEGER       offset,
	unsigned long       length
	)
{
	char flag = 0;
	unsigned long i = 0;
	unsigned long myRegion = 0;
	unsigned long myRegionOffset = 0;
	unsigned long myByteOffset = 0;
	unsigned long myBitPos = 0;
	long ret = BITMAP_BIT_UNKNOW;

	__try
	{

		if (NULL == bitmap || offset.QuadPart < 0 || offset.QuadPart + length > bitmap->bitmapReferSize)
		{
			ret = BITMAP_BIT_UNKNOW;

			__leave;
		}

		for (i = 0; i < length; i += bitmap->sectorSize)
		{
			
			myRegion = (unsigned long)((offset.QuadPart + (__int64)i) / (__int64)bitmap->regionReferSize);

			myRegionOffset = (unsigned long)((offset.QuadPart + (__int64)i) % (__int64)bitmap->regionReferSize);

			myByteOffset = myRegionOffset / bitmap->byteSize / bitmap->sectorSize;

			myBitPos = (myRegionOffset / bitmap->sectorSize) % bitmap->byteSize;

			if (NULL != *(bitmap->Bitmap + myRegion) && (*(*(bitmap->Bitmap + myRegion) + myByteOffset) &bitmapMask[myBitPos]))
			{
				flag |= 0x2;
			}
			else
			{
				flag |= 0x1;
			}

			if (flag == 0x3)
			{
				break;
			}
		}

		if (0x2 == flag)
		{
			ret = BITMAP_RANGE_SET;
		}
		else if (0x01 == flag)
		{
			ret = BITMAP_RANGE_CLEAR;
		}
		else if (0x03 == flag)
		{
			ret = BITMAP_RANGE_BLEND;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = BITMAP_BIT_UNKNOW;
	}

	return ret;
}