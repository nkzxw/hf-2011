#ifndef _BITMAP_H_
#define _BITMAP_H_

#define BITMAP_ERR_OUTOFRANGE	-1
#define BITMAP_ERR_ALLOCMEMORY	-2
#define BITMAP_SUCCESS			0
#define BITMAP_BIT_SET			1
#define BITMAP_BIT_CLEAR		2
#define BITMAP_BIT_UNKNOW		3
#define BITMAP_RANGE_SET		4
#define BITMAP_RANGE_CLEAR		5
#define BITMAP_RANGE_BLEND		6
#define BITMAP_RANGE_SIZE		25600
#define BITMAP_RANGE_SIZE_SMALL 256
#define BITMAP_RANGE_SIZE_MAX	51684
#define BITMAP_RANGE_AMOUNT		16*1024

typedef unsigned char tBitmap;

#pragma pack(1)

typedef struct _DP_BITMAP_
{

    unsigned long sectorSize; 

    unsigned long byteSize; 

    unsigned long regionSize;

    unsigned long regionNumber;

    unsigned long regionReferSize;

    __int64 bitmapReferSize;

    tBitmap** Bitmap; 

    void* lockBitmap; 
} DP_BITMAP, * PDP_BITMAP;

#pragma pack()
NTSTATUS BitmapInit(
	DP_BITMAP **	  bitmap,
	unsigned long     sectorSize,
	unsigned long     byteSize,
	unsigned long     regionSize,
	unsigned long     regionNumber
	);

void BitmapFree(DP_BITMAP* bitmap);

NTSTATUS BitmapSet(
  DP_BITMAP *		bitmap,
  LARGE_INTEGER     offset,
  unsigned long     length
  );

NTSTATUS BitmapGet(
  DP_BITMAP *    bitmap,
  LARGE_INTEGER     offset,
  unsigned long     length,
  void *            bufInOut,
  void *            bufIn
  );

long BitmapTest(
  DP_BITMAP *    bitmap,
  LARGE_INTEGER     offset,
  unsigned long     length
  );
#endif
