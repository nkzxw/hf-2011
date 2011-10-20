#ifndef _GUI_H_
#define _GUI_H_

#ifndef STATUS_INVALID_PARAMETER

typedef ULONG NTSTATUS;
#include <ntstatus.h>
#define NT_SUCCESS(X) ((X)>=0)

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool,
} POOL_TYPE;

extern "C"
{
PVOID
NTAPI
ExAllocatePoolWithTag (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

VOID
NTAPI
ExFreePool(
    IN PVOID P
    );
}

#else

#define PBITMAPINFOHEADER PVOID
#define PBITMAPFILEHEADER PVOID
#define HBITMAP PVOID
typedef struct _SURFOBJ SURFOBJ;

#endif

typedef struct LOADED_BITMAP
{
	PBITMAPINFOHEADER info;
	
	union
	{
		PVOID pBitmap;
		PBITMAPFILEHEADER hdr;
	};

	PVOID pvBits;
	ULONG_PTR iMappedFile;
} *PLOADED_BITMAP;


typedef struct _MDL *PMDL;
PMDL LockMem (PVOID Buffer, ULONG Size);
VOID UnlockMem (PMDL Mdl);

typedef struct FONT_PARAMETERS
{
	ULONG FontNumber;
	ULONG CharWidth;
	ULONG CharHeight;
	UNICODE_STRING FileName;

	LOADED_BITMAP LoadedFont;

	HBITMAP hFontBitmap;
	SURFOBJ *pFontSurf;
	PMDL pFontMdl;

} *PFONT_PARAMETERS;

extern "C"
{

extern
NTSTATUS
(*GuiLoadBitmap)(
	PWSTR BitmapFileName,
	PLOADED_BITMAP Bitmap
	);

extern
VOID
(*GuiUnloadBitmap)(
	PLOADED_BITMAP Bitmap
	);

extern
NTSTATUS
(*GuiLoadActiveFont)(
	);

extern
VOID
(*GuiUnloadFont)(
	);

extern
ULONG
(*GuiGetCharFontPosition)(
	IN CHAR c
	);

extern
VOID
(*GuiScrollLines)(
	IN ULONG nLines
	);

extern
VOID
(*GuiTextOut)(
	IN PCHAR Text
	);

extern
VOID
(_cdecl
*GuiPrintf)(
	IN PCHAR Format,
	...
	);

}

BOOLEAN
DisplayBuffer(
	);

#define LINUX_LF_OUTPUT 1


#endif
