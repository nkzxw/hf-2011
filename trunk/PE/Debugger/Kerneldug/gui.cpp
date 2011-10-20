/*++

	This is the part of NGdbg kernel debugger

	gui.cpp

	Contains implementation of kernel-mode graphics GUI

--*/

#define NT_BUILD_ENVIRONMENT
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <winddi.h>
#include "gui.h"
#include "config.h"

NTSTATUS
W32GuiLoadBitmap(
	PWSTR BitmapFileName,
	PLOADED_BITMAP Bitmap
	)

/*++

Routine Description

	Load bitmap from the file.
	EngMapFile wrapper

Arguments

	BitmapFileName

		File name that contains bitmap


	Bitmap

		Pointer to the structure that will receive information about
		 loaded bitmap

 Return Value

	NTSTATUS of the operation

Environment

	Context of CSRSS process

--*/

{
	Bitmap->pBitmap = EngMapFile (BitmapFileName, 0, &Bitmap->iMappedFile);
	if (Bitmap->pBitmap == NULL)
		return STATUS_UNSUCCESSFUL;

	Bitmap->info = (PBITMAPINFOHEADER) (((PBITMAPFILEHEADER)Bitmap->hdr) + 1);
	Bitmap->pvBits = (PUCHAR)Bitmap->pBitmap + Bitmap->hdr->bfOffBits;

	return STATUS_SUCCESS;
}

VOID
W32GuiUnloadBitmap(
	PLOADED_BITMAP Bitmap
	)

/*++

Routine Description

	Unload image previously loaded by W32GuiLoadBitmap

Arguments

	Bitmap

		Pointer to the structure containing information about loaded bitmap

 Return Value

	NTSTATUS of the operation

Environment

	Context of CSRSS process

--*/

{
	EngUnmapFile (Bitmap->iMappedFile);
	memset (Bitmap, 0, sizeof(LOADED_BITMAP));
}

FONT_PARAMETERS ActiveFont;

extern "C" extern int _cdecl _snwprintf (wchar_t*, int, ...);

extern VOID _cdecl EngPrint (char*, ...);
#define KdPrint(X) EngPrint X

char FontString[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!?@#$%^&*()-=\\|/<>,._+[]";


NTSTATUS
W32GuiLoadActiveFont(
	)
{
	NTSTATUS Status;
	wchar_t font[512];
	ULONG Size;

	Size = sizeof(ULONG);
	Status = QueryRegistrySetting (
		L"Video\\Fonts", 
		L"Active",
		REG_DWORD,
		&ActiveFont.FontNumber,
		&Size
		);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("QueryRegitrySetting failed for Video\\Fonts\\Active (Status %X)\n", Status));
		return Status;
	}

	_snwprintf (font, sizeof(font)/2, L"Video\\Fonts\\%d", ActiveFont.FontNumber);

	KdPrint(("Active font: '%S'\n", font));

	Size = sizeof(ULONG);
	Status = QueryRegistrySetting (
		font,
		L"CharWidth",
		REG_DWORD,
		&ActiveFont.CharWidth,
		&Size
		);

	if (!NT_SUCCESS(Status) || ActiveFont.CharWidth == 0)
	{
		KdPrint(("QueryRegitrySetting failed for %S\\CharWidth (Status %X)\n", font, Status));
		return Status;
	}

	Size = sizeof(ULONG);
	Status = QueryRegistrySetting (
		font,
		L"CharHeight",
		REG_DWORD,
		&ActiveFont.CharHeight,
		&Size
		);

	if (!NT_SUCCESS(Status) || ActiveFont.CharHeight == 0)
	{
		KdPrint(("QueryRegitrySetting failed for %S\\CharHeight (Status %X)\n", font, Status));
		return Status;
	}

	KdPrint(("Font: char width %d px, char height %x px\n",
		ActiveFont.CharWidth, ActiveFont.CharHeight));

	UNICODE_STRING FileName;
	FileName.MaximumLength = 0x1000;
	FileName.Buffer = (PWSTR) ExAllocatePoolWithTag (PagedPool, FileName.MaximumLength, 'tnoF');
	if (FileName.Buffer == NULL)
	{
		KdPrint(("Not enough resources to alloc mem for font file name\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	Size = FileName.MaximumLength;
	Status = QueryRegistrySetting(
		font,
		L"FileName",
		REG_SZ,
		FileName.Buffer,
		&Size
		);

	if (!NT_SUCCESS(Status))
	{
		KdPrint(("QueryRegitrySetting failed for %S\\FileName (Status %X)\n", font, Status));
		ExFreePool (FileName.Buffer);
		return Status;
	}

	FileName.Buffer[Size/2] = 0;
	FileName.Length = (USHORT)(Size - 2);

	ActiveFont.FileName = FileName;

	KdPrint(("Font file name: %S\n", FileName.Buffer));

	Status = W32GuiLoadBitmap (FileName.Buffer, &ActiveFont.LoadedFont);
	if (!NT_SUCCESS(Status))
	{
		ExFreePool (FileName.Buffer);
		KdPrint(("W32GuiLoadBitmap failed for font file (Status %X)\n", Status));
		return Status;
	}

	if ((ActiveFont.CharWidth * (sizeof(FontString)-1) != ActiveFont.LoadedFont.info->biWidth) ||
		(ActiveFont.CharHeight != ActiveFont.LoadedFont.info->biHeight))
	{
		KdPrint(("Font file is corrupted\n"
			"Image width is %d should be %d\n"
			"Image height is %d should be %d\n"
			,
			ActiveFont.LoadedFont.info->biWidth,
			(ActiveFont.CharWidth * (sizeof(FontString)-1)),
			ActiveFont.LoadedFont.info->biHeight,
			ActiveFont.CharHeight
			));
		W32GuiUnloadBitmap (&ActiveFont.LoadedFont);
		ExFreePool (FileName.Buffer);
		return STATUS_INVALID_FILE_FOR_SECTION;
	}

	SIZEL sizel = { (sizeof(FontString)-1)*ActiveFont.CharWidth, ActiveFont.CharHeight };

	ActiveFont.hFontBitmap = EngCreateBitmap (
		sizel,
		sizel.cx * 4,
		BMF_32BPP,
		0,
		ActiveFont.LoadedFont.pvBits
		);

	if (!ActiveFont.hFontBitmap)
	{
		KdPrint(("EngCreateBitmap failed for font\n"));
		W32GuiUnloadBitmap (&ActiveFont.LoadedFont);
		ExFreePool (FileName.Buffer);
		return STATUS_UNSUCCESSFUL;
	}

	ActiveFont.pFontSurf = EngLockSurface ((HSURF)ActiveFont.hFontBitmap);

	if (!ActiveFont.pFontSurf)
	{
		KdPrint(("EngLockSurface failed for font surface\n"));
		EngDeleteSurface ((HSURF)ActiveFont.hFontBitmap);
		W32GuiUnloadBitmap (&ActiveFont.LoadedFont);
		ExFreePool (FileName.Buffer);
		return STATUS_UNSUCCESSFUL;
	}

	ActiveFont.pFontMdl = LockMem (ActiveFont.pFontSurf->pvBits, ActiveFont.pFontSurf->cjBits);

	if (!ActiveFont.pFontMdl)
	{
		KdPrint(("LockMem failed for font surface\n"));
		EngUnlockSurface (ActiveFont.pFontSurf);
		EngDeleteSurface ((HSURF)ActiveFont.hFontBitmap);
		W32GuiUnloadBitmap (&ActiveFont.LoadedFont);
		ExFreePool (FileName.Buffer);
		return STATUS_UNSUCCESSFUL;
	}

	KdPrint(("Font loaded successfully\n"));

	return Status;
}

VOID
W32GuiUnloadFont(
	)
/**
	Unload active font
*/
{
	KdPrint(("W32GuiUnloadFont enter\n"));

	UnlockMem (ActiveFont.pFontMdl);
	EngUnlockSurface (ActiveFont.pFontSurf);
	EngDeleteSurface ((HSURF)ActiveFont.hFontBitmap);
	W32GuiUnloadBitmap (&ActiveFont.LoadedFont);
	ExFreePool (ActiveFont.FileName.Buffer);

	KdPrint(("W32GuiUnloadFont exit\n"));
}

ULONG
W32GuiGetCharFontPosition(
	IN CHAR c
	)

/*++

Routine Description

	This routine get char position in font

Arguments

	c     character to test

Return Value

	Position in font string or -1 if character is not found

This function can be called at any IRQL

--*/

{
	PCHAR s = strchr( FontString, c );
	
	if( s )
		return (ULONG)s - (ULONG)FontString;
	else
		return -1;
}

BOOLEAN
DisplayBuffer(
	);

extern ULONG Width;
extern ULONG Height;
extern ULONG SpareX;
extern ULONG SpareY;

ULONG _x = 0, _y = 0;

typedef struct SURFACE
{
	HBITMAP hBitmap;
	SURFOBJ* pSurface;
	PMDL pMdl;
} *PSURFACE;

extern SURFACE *GDISurf;

BOOLEAN
W32GuiFillRegion(
	ULONG x,
	ULONG y,
	ULONG w,
	ULONG h,
	ULONG color
	)
{
	RECTL dst = {x, y, x+w, y+h};
	return EngEraseSurface (GDISurf->pSurface, &dst, color);
}

VOID
W32GuiScrollLines(
	IN ULONG nLines
	)

/*++

Routine Description

	This function scrolls specified number of text lines on the screen

Arguments

	nLines     Number of lines to scroll

Return Value

	None

This function can be called at any IRQL

--*/

{
	POINTL PointFrom = { SpareX, SpareY + ActiveFont.CharHeight * nLines };
	RECTL RectDest = {	SpareX,
						SpareY,
						Width - SpareX,
						_y };	// _y already contains SpareY

	if ( (ActiveFont.CharHeight * nLines + _y) > Height )
	{
		RectDest.bottom -= (ActiveFont.CharHeight * nLines + _y) - Height;
	}

//	KdPrint(("Scrolling lines: nLines %d, _y %d from %d %d %d %d to %d %d\n",
//		nLines, _y, PointFrom.x, PointFrom.y, PointFrom.x+(Width-SpareX*2), PointFrom.y+_y-SpareY));

	EngCopyBits (
		GDISurf->pSurface,
		GDISurf->pSurface,
		NULL,
		NULL,
		&RectDest,
		&PointFrom
		);
	
	RectDest.left = SpareX;
	RectDest.top = _y - ActiveFont.CharHeight * nLines; // _y already contains SpareY
	RectDest.right = Width - SpareX;
	RectDest.bottom = _y; // _y already contains SpareY

//	KdPrint(("Scrolling lines: erasing from %d %d %d %d\n",
//		RectDest.left, RectDest.top, RectDest.right, RectDest.bottom));

	EngEraseSurface (GDISurf->pSurface, &RectDest, RGB(0xFF,0xFF,0xFF));

	_y -= ActiveFont.CharHeight * nLines;
}



ULONG nScrollLines = 3;


VOID
W32GuiTextOut(
	IN PCHAR Text
	)

/*++

Routine Description

	This routine outputs ANSI string to the screen

Arguments

	Text     ASCIIZ ANSI string

Return Value

	None

--*/

{
	if (!_x && !_y)
	{
		_x = SpareX;
		_y = SpareY;
	}

	while( *Text )
	{
		BOOLEAN Backspace = FALSE;
		CHAR Char = *Text;

		if( _y + ActiveFont.CharHeight >= (Height-SpareY) )
		{
			//__asm int 3
			W32GuiScrollLines( nScrollLines );
			DisplayBuffer();
		}

		if( Char == '\b' )
		{
			// Go to the previous symbol
			if (_x == SpareX)
			{
				// Beginning of the output.
				// Cannot go back
				KdPrint(("Cannot use backspace - beginning of the string. Skipping\n"));
				Text ++;
				continue;
			}

			_x -= ActiveFont.CharWidth;
			Char = ' ';
			Backspace = TRUE;
		}

		if( Char == ' ' )
		{
			W32GuiFillRegion( _x, _y, ActiveFont.CharWidth, ActiveFont.CharHeight-1, RGB(0xFF,0xFF,0xFF) );
		}
		else if( Char != '\n' && Char != '\r' )
		{
			int position = W32GuiGetCharFontPosition( Char );
			if( position == -1 )
				position = W32GuiGetCharFontPosition( '?' );

			ULONG filex = position * ActiveFont.CharWidth;

			/*
			Status = W32GuiDisplayBitmapPart( 
				LoadedFont,
				LoadedFontBitmapSize,
				_x, _y,
				filex, 0,
				filex + ActiveFont.CharWidth, 0
				);
			*/

			RECTL RectDest =  {_x, _y, _x + ActiveFont.CharWidth, _y + ActiveFont.CharHeight};
			POINTL PointSrc = { filex, 0 };

			EngCopyBits (
				GDISurf->pSurface,
				ActiveFont.pFontSurf,
				NULL,
				NULL,
				&RectDest,
				&PointSrc
				);
		}
        
		if( Char == '\r' )
		{
			_x = SpareX;
		}
		else if( Char == '\n' )
		{
#if LINUX_LF_OUTPUT
			_x = SpareX;
#endif
			_y += ActiveFont.CharHeight;
		}
		else
		{
			if (!Backspace)
			{
				_x += ActiveFont.CharWidth;
			}
		}

		if( _x >= (Width-SpareX) )
		{
			_x = SpareX;
			_y += ActiveFont.CharHeight;
		}

		Text ++;
	}
}

CHAR TempPrintBuffer[1024];

extern "C" extern int _cdecl _vsnprintf (char*, int, const char*, va_list);

VOID
_cdecl
W32GuiPrintf(
	IN PCHAR Format,
	...
	)
/**
	Print formatted string to screen
*/
{
	va_list va;
	va_start (va, Format);

	_vsnprintf (TempPrintBuffer, sizeof(TempPrintBuffer)-1, Format, va);
	W32GuiTextOut (TempPrintBuffer);
}

/*
NTSTATUS
W32GuiLoadBitmap(
	PWSTR BitmapFileName,
	PLOADED_BITMAP Bitmap
	);

VOID
W32GuiUnloadBitmap(
	PLOADED_BITMAP Bitmap
	);

NTSTATUS
W32GuiLoadActiveFont(
	);

VOID
W32GuiUnloadFont(
	);

ULONG
W32GuiGetCharFontPosition(
	IN CHAR c
	);

VOID
W32GuiScrollLines(
	IN ULONG nLines
	);

VOID
W32GuiTextOut(
	IN PCHAR Text
	);

VOID
_cdecl
W32GuiPrintf(
	IN PCHAR Format,
	...
	);
*/

NTSTATUS
(*GuiLoadBitmap)(
	PWSTR BitmapFileName,
	PLOADED_BITMAP Bitmap
	) = &W32GuiLoadBitmap;

VOID
(*GuiUnloadBitmap)(
	PLOADED_BITMAP Bitmap
	) = &W32GuiUnloadBitmap;

NTSTATUS
(*GuiLoadActiveFont)(
	) = &W32GuiLoadActiveFont;

VOID
(*GuiUnloadFont)(
	) = &W32GuiUnloadFont;

ULONG
(*GuiGetCharFontPosition)(
	IN CHAR c
	) = &W32GuiGetCharFontPosition;

VOID
(*GuiScrollLines)(
	IN ULONG nLines
	) = &W32GuiScrollLines;

VOID
(*GuiTextOut)(
	IN PCHAR Text
	) = &W32GuiTextOut;

VOID
(_cdecl
*GuiPrintf)(
	IN PCHAR Format,
	...
	) = &W32GuiPrintf;
