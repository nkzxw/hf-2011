/*++

	This is the part of NGdbg kernel debugger

	bootgui.cpp

	Contains implementation of GUI used at system startup

--*/

#include <ntifs.h>
#include "gui.h"
#include "bootvid.h"
#include <stdarg.h>

#define KdfPrint(X) do					\
	{									\
		KdPrint ((__FUNCTION__ " : "));	\
		KdPrint (X);					\
	}									\
	while (0);

//
// Stubs that should not be called...
//

NTSTATUS
BootGuiLoadBitmap(
	PWSTR BitmapFileName,
	PLOADED_BITMAP Bitmap
	)
{
	// Should not be called during boot-time
	ASSERT (FALSE);
	return STATUS_NOT_IMPLEMENTED;
}

VOID
BootGuiUnloadBitmap(
	PLOADED_BITMAP Bitmap
	)
{
	// Should not be called during boot-time
	ASSERT (FALSE);
}

NTSTATUS
BootGuiLoadActiveFont(
	)
{
	// Should not be called during boot-time
	ASSERT (FALSE);
	return STATUS_NOT_IMPLEMENTED;
}

VOID
BootGuiUnloadFont(
	)
{
	// Should not be called during boot-time
	ASSERT (FALSE);
}

ULONG
BootGuiGetCharFontPosition(
	IN CHAR c
	)
{
	// Should not be called during boot-time
	ASSERT (FALSE);
	return -1;
}

VOID
BootGuiScrollLines(
	IN ULONG nLines
	)
{
	// Should not be called during boot-time
	ASSERT (FALSE);
}


//
// Really defined functions
//

UCHAR PositionY = 0;

VOID
BootGuiTextOut(
	IN PCHAR Text
	)
{
	InbvDisplayString (Text);
}

extern CHAR TempPrintBuffer[1024];
extern "C" extern int _cdecl _vsnprintf (char*, int, const char*, va_list);

VOID
_cdecl
BootGuiPrintf(
	IN PCHAR Format,
	...
	)
{
	va_list va;
	va_start (va, Format);

	_vsnprintf (TempPrintBuffer, sizeof(TempPrintBuffer)-1, Format, va);
	InbvDisplayString (TempPrintBuffer);
}



UCHAR 
KbdGetKeyPolled(
	IN BOOLEAN AllowMouseCallback
	);


VOID
BootGuiInitialize(
	)
{
	KdfPrint (("Setting up GUI entry points\n"));

	// Setup GUI entry points.
	GuiLoadBitmap = &BootGuiLoadBitmap;
	GuiUnloadBitmap = &BootGuiUnloadBitmap;
	GuiLoadActiveFont = &BootGuiLoadActiveFont;
	GuiUnloadFont = &BootGuiUnloadFont;
	GuiGetCharFontPosition = &BootGuiGetCharFontPosition;
	GuiScrollLines = &BootGuiScrollLines;
	GuiTextOut = &BootGuiTextOut;
	GuiPrintf = &BootGuiPrintf;

	KdfPrint (("Resetting display\n"));

	if (!InbvIsBootDriverInstalled())
	{
		KdfPrint (("Bootvid is not installed!\n"));
		ASSERT (FALSE);
	}

	//
	// Borrowed from ntos kernel =)
	//


	// Reset display
	InbvAcquireDisplayOwnership ();
	InbvResetDisplay ();

	//
	// InbvDisplayString incorrecly scrolls lines, so
	// reserve 2 pixels from each side
	//
	ULONG x1 = 2, y1 = 2, x2 = 637, y2 = 477;

	UCHAR SpareColor = 4;	// blue
	UCHAR BackColor = 3;	// green
	UCHAR TextColor = 15;	// white

	// Make the screen blue
    InbvSolidColorFill(0,0,639,479,SpareColor);		// blue, 640x480
    InbvSolidColorFill(x1,y1,x2,y2,BackColor);		// green, (x1,y1)x(x2,y2)
    InbvSetTextColor (TextColor);					// white

	// Enable InbvDisplayString
    InbvInstallDisplayStringFilter((INBV_DISPLAY_STRING_FILTER)NULL);
    InbvEnableDisplayString(TRUE);

	// Set scroll region for InbvDisplayString
    InbvSetScrollRegion(x1,y1,x2,y2);	

	KdfPrint(("NGdbg boot-GUI loaded\n"));
	//InbvDisplayString ("NGdbg boot-GUI loaded\n");

	char *Str = "Press ESC to cancel loading NGdbg ... (%d seconds remaining)   \r";

	KIRQL Irql = KfRaiseIrql (HIGH_LEVEL);
	UCHAR Byte;

	ULARGE_INTEGER TickCountStart;
	ULARGE_INTEGER TickCount;
	ULARGE_INTEGER Difference;
	ULONG TickIncrement = KeQueryTimeIncrement ();
	ULONG Seconds = 10;

	KeQueryTickCount (&TickCountStart);

	do
	{
		KeQueryTickCount (&TickCount);
		
		Difference.QuadPart = (TickCount.QuadPart - TickCountStart.QuadPart) / TickIncrement;
		Difference.QuadPart /= 10000; // 100 ns -> 1 ms

		ULONG SecondsElapsed = (ULONG)(Difference.QuadPart/1000);
		ULONG SecondsRemaining = Seconds - SecondsElapsed;

		GuiPrintf (Str, SecondsRemaining);

		if (SecondsRemaining == 0)
			break;

		//
		// Get key
		//

		Byte = KbdGetKeyPolled(FALSE);

		if (Byte == 0)
			continue;

		if (Byte == 1)
			break;

	}
	while (TRUE);

	InbvDisplayString ("NGdbg kernel debugger : boot-gui loaded successfully                     \n");

	KfLowerIrql (Irql);
}
