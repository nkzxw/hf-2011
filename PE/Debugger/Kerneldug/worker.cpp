/*++

	This is the part of NGdbg kernel debugger

	worker.cpp

	This file contains initialization/clean routines for graphics debugger part,
	 main WR_ENTER_DEBUGGER routine and another routines, which
	 control user interface of the debugger.

--*/

#define NT_BUILD_ENVIRONMENT
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <winddi.h>
#include "mouse.h"

#define KERNEL_DEBUGGER_VERSION "0.1"

VOID
W32PrepareCall(
/*++

Routine Description

	This function prepares thread for calling WIN32

Arguments

	None

Return Value

	None

--*/
	);

VOID
W32ReleaseCall(
/*++

Routine Description

	This function unprepares thread from calling WIN32

Arguments

	None

Return Value

	None

--*/
	);

#include "gui.h"	
#include "symbols.h"

//
// Declarations
//

extern "C" VOID _cdecl DbgPrint (char*, ...);
#define KdPrint(X) DbgPrint X

extern SURFOBJ *pPrimarySurf;
extern PVOID pDrvCopyBits;

BOOL (APIENTRY *xxxDrvCopyBits)(
   OUT SURFOBJ *psoDst,
   IN SURFOBJ *psoSrc,
   IN CLIPOBJ *pco,
   IN XLATEOBJ *pxlo,
   IN RECTL *prclDst,
   IN POINTL *pptlSrc
   );

//
// We cannot include ntddk.h, because
// it conflicts with windows.h, which is necessary
//  for win32k routines :(
//
/*
namespace NT
{
extern "C"
{
#pragma warning(push)
#pragma warning(disable: 4005)
#include <ntddk.h>
#undef ExFreePool
#pragma warning(pop)
}
}
*/

extern "C"
{
UCHAR __fastcall KfRaiseIrql (UCHAR Irql);
VOID __fastcall KfLowerIrql (UCHAR Irql);
UCHAR __stdcall KeGetCurrentIrql ();
}

//
// Debugger global vars
//

SIZE MousePointerSize = {2,2};

extern LONG MouseX;
extern LONG MouseY;
extern LONG OldMouseX;
extern LONG OldMouseY;

//
// Parameters of main debugger 'window'
//

ULONG Width = 900;			// width
ULONG Height = 500;			// height
ULONG StartX = 100;			// x coordinate
ULONG StartY = 100;			// y coordinate
ULONG SpareX = 10;			// reserved vertical space
ULONG SpareY = 10;			// reserved horizontal space


//
// In the previous version i tried to use
// XLATEOBJ to translate 24bpp images to 32bpp.
// But win32k uses extended undocumented structure EXLATEOBJ.
// So, I have to make all bitmaps 32bpp to avoid any runtime 
// translation between pictures when we call DrvCopyBits
//

//XLATEOBJ XlateObj;


//
// Declare vars as 'extern "C"'.
// This will make them accessible from Disasm.c
//

extern "C" extern PVOID pNtBase;
//extern "C" extern PVOID pNtSymbols;
//extern "C" extern PMOD_SYM pNtSymbols;

//
// Base load address of nt kernel and
//  pointer to loaded symbols for it
//

PVOID pNtBase;
//PVOID pNtSymbols;		// pointer to mapped sym file
//ULONG_PTR iNtSymbols;	// ID of mapped sym file, it will be passed in EngUnmapFile
//PMDL SymMdl;			// MDL for symbols
//PMOD_SYM pNtSymbols;

//
// Some declarations..
//

PVOID FindImage (PWSTR);

#undef RegOpenKey
#undef RegQueryValue
HANDLE RegOpenKey (PWSTR KeyName, ACCESS_MASK DesiredAccess);
BOOLEAN RegQueryValue (HANDLE hKey, PWSTR ValueName, ULONG Type, PVOID Buffer, ULONG *Len);

extern "C"
BOOLEAN
FindBaseAndSize(
	IN PVOID SomePtr,
	OUT PVOID *BaseAddress OPTIONAL, 
	OUT ULONG *ImageSize OPTIONAL
	);

extern "C"
NTSTATUS 
ZwClose (
	HANDLE
	);

PVOID GetKernelAddress (PWSTR);

//
// Single entry in symbol file
//

typedef struct _SYMINFO
{
	ULONG NextEntryDelta;
	ULONG SymOffset;
	char SymName[1];
} SYMINFO, *PSYMINFO;

typedef struct SURFACE
{
	HBITMAP hBitmap;
	SURFOBJ* pSurface;
	PMDL pMdl;
} *PSURFACE;

VOID DeleteSurface (SURFACE* Surf)
{
	if (Surf->pMdl)
		UnlockMem (Surf->pMdl);
	if (Surf->pSurface)
		EngUnlockSurface (Surf->pSurface);
	if (Surf->hBitmap)
		EngDeleteSurface ((HSURF)Surf->hBitmap);
	ExFreePool (Surf);
}

SURFACE* CreateSurface (SIZE SurfaceSize, PVOID pvBits, ULONG iFormat)
{
	SURFACE *Surf = (SURFACE*) ExAllocatePoolWithTag (NonPagedPool, sizeof(SURFACE), ' kdD');
	BOOLEAN bOk = FALSE;
	if (Surf)
	{
		memset (Surf, 0, sizeof(SURFACE));

		Surf->hBitmap = EngCreateBitmap (
			SurfaceSize,
			SurfaceSize.cx * 4,
			iFormat,
			0,
			pvBits
			);

		if (Surf->hBitmap)
		{
			Surf->pSurface = EngLockSurface ((HSURF)Surf->hBitmap);
			if (Surf->pSurface)
			{
				if (pvBits == NULL)
				{
					Surf->pMdl = LockMem (Surf->pSurface->pvBits, Surf->pSurface->cjBits);
					if (Surf->pMdl)
						bOk = TRUE;
				}
				else
				{
					bOk = TRUE;
				}
			}
		}
	}

	if (Surf && !bOk)
		DeleteSurface (Surf);

	return Surf;
}

SURFACE *GDISurf;
SURFACE *BackupSurf;
SURFACE *MouseSurf;
SURFACE *MouseBackupSurf;

BOOL
MouStoreCursor(
	)
{
	RECTL Rect;
	POINTL Point;
	BOOL st;

	// Source point
	Point.x = 0;
	Point.y = 0;

	// Destination rectangle
	Rect.left = MouseX;
	Rect.right = MouseX + MousePointerSize.cx;
	Rect.top = MouseY;
	Rect.bottom = MouseY + MousePointerSize.cy;

	//
	// Draw cursor
	//

	//KdPrint(("Drawing cursor (%d,%d,%d,%d)\n", Rect));

	st = EngCopyBits (
		GDISurf->pSurface,
		MouseSurf->pSurface,
		NULL,
		NULL,
		&Rect,
		&Point
		);

	return st;
}

BOOL
MouRedrawCursor(
	)
{
	RECTL Rect;
	POINTL Point;
	BOOL st;

	// Source point
	Point.x = 0;
	Point.y = 0;

	// Destination rectangle
	Rect.left = OldMouseX;
	Rect.right = OldMouseX + MousePointerSize.cx - 1;
	Rect.top = OldMouseY;
	Rect.bottom = OldMouseY + MousePointerSize.cy - 1;

	//
	// Rectore image at old pointer
	//

	st = xxxDrvCopyBits (
		GDISurf->pSurface,
		MouseBackupSurf->pSurface,
		NULL,
		NULL,
		&Rect,
		&Point
		);

	// Source point
	Point.x = MouseX;
	Point.y = MouseY;

	// Destination rectangle
	Rect.left = 0;
	Rect.right = MousePointerSize.cx - 1;
	Rect.top = 0;
	Rect.bottom = MousePointerSize.cy - 1;

	//
	// Backup image
	//

	st = xxxDrvCopyBits (
		MouseBackupSurf->pSurface,
		GDISurf->pSurface,
		NULL,
		NULL,
		&Rect,
		&Point
		);

	st = MouStoreCursor ();

	return st;
}

VOID 
Worker(
	)

/*++

Routine Description

	Initialization routine for this part of the debugger.
	This function is called from DriverEntry in the context
	 of CSRSS process, so we can use Eng*** routines exported
	 by the win32k.sys

Arguments

	None

Return Value

	None, this function should always succeed initizaliation.

Environment

	This function is called at PASSIVE_LEVEL in the context of CSRSS process.

--*/

{
	SURFOBJ *Surf = pPrimarySurf;
	*(PVOID*)&xxxDrvCopyBits = pDrvCopyBits;

	KdPrint(("PrimarySurf=%X ..\n", Surf));
	KdPrint(("DrvCopyBits=%X ..\n", xxxDrvCopyBits));

	KdPrint(("Surf->iBitmapFormat = %X\n", Surf->iBitmapFormat));
	KdPrint(("Size X %d Y %d\n", Surf->sizlBitmap.cx, Surf->sizlBitmap.cy));

	//
	// Create surfaces
	//

	SIZEL Size;

	Size.cx = Width;
	Size.cy = Height;

	// GDI surface
	GDISurf = CreateSurface (Size, NULL, Surf->iBitmapFormat);
	KdPrint(("GDISurf %X\n", GDISurf));
	if (!GDISurf)
		return;

	// Backup surface
	BackupSurf = CreateSurface (Size, NULL, Surf->iBitmapFormat);
	KdPrint(("BackupSurf %X\n", BackupSurf));
	if (!BackupSurf)
		return;

	// Mouse pointer surface
	MouseSurf = CreateSurface (MousePointerSize, NULL, Surf->iBitmapFormat);
	KdPrint(("MouseSurf %X\n", MouseSurf));
	if (!MouseSurf)
		return;

	// Mouse backup surface
	MouseBackupSurf = CreateSurface (MousePointerSize, NULL, Surf->iBitmapFormat);
	KdPrint(("MouseBackupSurf %X\n", MouseBackupSurf));
	if (!MouseBackupSurf)
		return;

	//
	// Erase backup surface
	//

	RECTL Rect;
	Rect.left = 0;
	Rect.top = 0;
	Rect.right = Width;
	Rect.bottom = Height;

	BOOL s = EngEraseSurface (BackupSurf->pSurface, &Rect, RGB(0xFF,0xFF,0xFF));
	KdPrint(("EngEraseSurface (backup) %d\n", s));

	//
	// Erase drawing surface
	//

	s = EngEraseSurface (GDISurf->pSurface, &Rect, RGB(0xFF,0xFF,0xFF));
	KdPrint(("EngEraseSurface (main) %d\n", s));

	Rect.right = MousePointerSize.cx - 1;
	Rect.bottom = MousePointerSize.cy - 1;
	s = EngEraseSurface (MouseSurf->pSurface, &Rect, RGB(0,0,0));
	KdPrint(("EngEraseSurface (mouse) %d\n", s));

	//
	// Load active font
	//

	NTSTATUS Status = GuiLoadActiveFont ();
	if (!NT_SUCCESS(Status))
		return;

	GuiTextOut ("NGdbg kernel debugger v" KERNEL_DEBUGGER_VERSION "\n");

	//
	// Find NT base
	//

	if(!FindBaseAndSize (GetKernelAddress(L"DbgPrint"), &pNtBase, NULL))
	{
		KdPrint(("Could not get nt base\n"));
		return;
	}

	//
	// Initialize symbol engine and load symbols
	//

	SymInitialize ( TRUE );

	Status = SymLoadSymbolFile (L"nt", pNtBase);
	KdPrint(("nt symbols loaded with status %X\n", Status));

	Status = SymLoadSymbolFile (L"hal.dll", NULL);
	KdPrint(("hal.dll symbols loaded with status %X\n", Status));

	//
	// Initialization completed.
	//
}

typedef struct _EPROCESS EPROCESS, *PEPROCESS;

extern PEPROCESS CsrProcess;
extern ULONG Lock;

ULONG PsDirectoryTableBase = 0x18;

VOID 
EngFastAttachProcess (
	PEPROCESS Process, 
	PULONG Pdbr
	)

/*++

Routine Description

	Fast routine to attach to the specified process.
	Because KeStackAttachProcess makes many other critical things
	 that we cannot allow at raised IRQL we have to simply
	 swap CR3 (PDBR) register.
	This function SHOULD be called at raised IRQL (not less that DISPATCH_LEVEL)
	to avoid context switches from code with changed CR3.
	Each call to this routine should have corresponding EngFastDetachProcess call

Arguments

	Process

		EPROCESS to attach

	Pdbr

		Place where current CR3 value should be saved.
		This value should be passed to EngFastDetachProcess later

Return Value

	None

Environment

	This function MUST be called at raised IRQL (not less DISPATCH_LEVEL).
	
	For internal use by this module ONLY.

--*/

{
	ULONG NewCR3 = *(ULONG*)(&((UCHAR*)Process)[PsDirectoryTableBase]);
	__asm
	{
		mov eax, cr3
		mov ecx, [Pdbr]
		mov [ecx], eax
		mov eax, [NewCR3]
		mov cr3, eax
	}
}

VOID 
EngFastDetachProcess (
	ULONG Pdbr
	)

/*++

Routine Description

	This routine performs fast detach from the process, previously 
	attached by EngFastAttachProcess.
	It simply restores CR3 value

Arguments

	Pdbr

		Old CR3 value to restore previously obtained by EngFastAttachProcess

Return Value

	None

Environment

	This function MUST be called at raised IRQL (not less DISPATCH_LEVEL).
	
	For internal use by this module ONLY.

--*/

{
	__asm
	{
		mov eax, [Pdbr]
		mov cr3, eax
	}
}

VOID 
Cleanup(
	)

/*++

Routine Description

	This routine performs clean up for all resources
	 allocated by Worker() for this module.
	It is called from DriverUnload routine.

Arguments

	None

Return Value

	None

Environment

	This function is called at PASSIVE_LEVEL.
	DriverUnload attaches to CSRSS process before call Cleanup()

--*/

{
	KdPrint(( __FUNCTION__ " : unloading symbol tables\n"));
	SymFreeSymbolTables ();

	KdPrint(( __FUNCTION__ " : deleting surfaces\n"));
	DeleteSurface (MouseBackupSurf);
	DeleteSurface (MouseSurf);
	DeleteSurface (BackupSurf);
	DeleteSurface (GDISurf);

	KdPrint(( __FUNCTION__ " : unloading font\n"));
	GuiUnloadFont ();

	KdPrint(( __FUNCTION__ " : completed\n"));
}

UCHAR 
KbdGetKeyPolled(
	IN BOOLEAN AllowMouseCallback
	);

extern BOOLEAN BootDebuggingInitiated;

BOOLEAN
DisplayBuffer(
	)

/*++

Routine Description

	This function flushes drawing surface by copying it to the main 
	 screen surface.
    It uses DrvCopyBits of display driver to perform this copying.

Arguments

	None

Return Value

	BOOLEAN returned by DrvCopyBits

Environment

	This routine is usually called at raised IRQL

--*/

{
	if (BootDebuggingInitiated)
	{
		//
		// No need to refresh screen at boot time.
		// InbvDisplayString do it
		//

		return TRUE;
	}

	RECTL Rect;
	POINTL Point;
	BOOL st;

	// Write cursor to gdi surf
	st = MouStoreCursor ();

	Point.x = 0;
	Point.y = 0;
	Rect.left = StartX;
	Rect.top = StartY;
	Rect.right =  StartX + Width;
	Rect.bottom = StartY + Height;

	st = xxxDrvCopyBits(
			pPrimarySurf, 
			GDISurf->pSurface,
			NULL,
			NULL, //&XlateObj,		// no translation now
			&Rect,
			&Point
			);
	
	return st;
}

extern "C" extern void _cdecl _snprintf (char*, int, const char*, ...);
extern "C" VOID KeStallExecutionProcessor (ULONG);

UCHAR KeybdScanCodeToAsciiCode (UCHAR ScanCode);

VOID KeybProcessUserInputLocked (UCHAR ScanCode);

extern BOOLEAN WindowsNum;
extern BOOLEAN WindowsCaps;
extern BOOLEAN WindowsScroll;

extern BOOLEAN DbgNum;
extern BOOLEAN DbgCaps;

VOID
KbdSetLeds(
	BOOLEAN Num,
	BOOLEAN Caps,
	BOOLEAN Scroll
	);

LARGE_INTEGER PrevBlinkTickCount;


VOID 
PollIdle (
	)

/*++

Routine Description

	This routine is called when WR_ENTER_DEBUGGER have nothing
	 to do at raised IRQL when debugger is active.
    This routine can blink cursor on the screen and do another
	 things, which should be done periodically.

Arguments

	None

Return Value

	None

Environment

	This routine is called at raised IRQL from WR_ENTER_DEBUGGER

--*/

{
	// Now we have nothing to do
}


typedef 
VOID
(NTAPI
 *PDBG_CALLBACK)(
	BOOLEAN In,
	PVOID Argument,
	BOOLEAN DispatchException
	);

VOID
ProcessCommand(
	CHAR* Command
	);

BOOLEAN ExceptionShouldBeDispatched = FALSE;
BOOLEAN StopProcessingCommands = FALSE;

extern BOOLEAN DbgEnteredDebugger;

VOID DisasmAtAddress (PVOID Address, ULONG nCommands);


VOID
StateChangeCallbackRoutine(
	PMOUSE_STATE_CHANGE_PACKET StateChange
	)

/*++

Routine Description

	This is callback routine called from PS/2 mouse driver
	 when new packet from mouse is received.

Arguments

	StateChange

		State change packet with new mouse parameters.

Return Value

	None

Environment

	Called from mouse driver at raised IRQL

--*/

{
	KdPrint(("StateChangeCallbackRoutine, Changed: [ "));

	if (StateChange->Flags & MOUSE_STATE_LEFTBTN)
		KdPrint(("LEFT "));
	if (StateChange->Flags & MOUSE_STATE_MIDDLEBTN)
		KdPrint(("MIDDLE "));
	if (StateChange->Flags & MOUSE_STATE_RIGHTBTN)
		KdPrint(("RIGHT "));
	if (StateChange->Flags & MOUSE_STATE_MOVE_X)
		KdPrint(("X "));
	if (StateChange->Flags & MOUSE_STATE_MOVE_Y)
		KdPrint(("Y "));
	if (StateChange->Flags & MOUSE_STATE_MOVE_WHEEL)
		KdPrint(("WHEEL "));

	KdPrint(("] L %d M %d R %d X %d Y %d WH %d\n",
		StateChange->Left,
		StateChange->Middle,
		StateChange->Right,
		StateChange->X,
		StateChange->Y,
		StateChange->Wheel
		));

	//
	// Nothing to do now
	//

	// TODO: redraw cursor
	MouRedrawCursor ();
}

extern "C"
{
VOID
DbgFreezeProcessors(
	);

VOID
DbgThawProcessors(
	);
}

VOID 
WR_ENTER_DEBUGGER(
	BOOLEAN UserInitiated,
	PDBG_CALLBACK Callback,
	PVOID Argument
	)

/*++

Routine Description

	This function enters kernel debugger and starts to process
	 commands from keyboard.
	Debugger can be entered by Ctrl-Alt-Shift-F12 or when
	some exception (or another fault) occurrs in the system.
	This function is always called at raised IRQL, usually
	 DIRQL for keyboard (if debugger is initiated manually from keyboard)
	 or HIGH_LEVEL (from exception or bugcheck handler)

Arguments

	UserInitiated
	
		Specifies whenever debugger is initiated by user or not.

	Callback
	
		Callback which should be called before entering debugger
		 and immediately after exit from the debugger.
		dbgeng module uses this callback to handle exceptions

	Argument

		Argument to be passed to callback routine

Return Value

	None

Environment

	This function is always called (and MUST be called) at raied IRQL,
	 not less than keyboard's DIRQL.

-- */

{
	BOOL s;
	ULONG State;
	RECTL Rect;
	POINTL Point = {StartX, StartY};

	KdPrint(("WR_ENTER_DEBUGGER enter%s\n", BootDebuggingInitiated ? "(AT BOOT TIME)" : ""));

	// freeze processors
	DbgFreezeProcessors ();

	//
	// don't perform some operations at boot time
	//

	if (BootDebuggingInitiated == FALSE)
	{
		EngFastAttachProcess (CsrProcess, &State);
	}

	DbgEnteredDebugger = TRUE;
	UCHAR Irql = KeGetCurrentIrql();
	KfLowerIrql (1);
	KdPrint(("IRQL lowered!!\n"));
	//KeStallExecutionProcessor (3000000);
	//KdPrint(("ok\n"));

	if (BootDebuggingInitiated == FALSE)
	{
		KdPrint(("Surf->pvBits = %X\n", pPrimarySurf->pvBits));
		KdPrint(("pGDISurf(%X)->pvBits = %X\n", GDISurf->pSurface, GDISurf->pSurface->pvBits));

		KdPrint(("Backing up..\n"));

		Rect.left = 0;
		Rect.top = 0;
		Rect.right =  Width;
		Rect.bottom = Height;

		s = xxxDrvCopyBits (
			BackupSurf->pSurface,
			pPrimarySurf,
			NULL,
			NULL,
			&Rect,
			&Point
			);

		KdPrint(("Backed up with status %X\n", s));
	}

	//
	// Settings keyboard leds.
	//

	KbdSetLeds (DbgNum, DbgCaps, 1);

	//
	// We are within kernel debugger.
	// Callback the specified routine
	//

	if (!UserInitiated)
		Callback (TRUE, Argument, FALSE);
	else
		GuiTextOut ("User-break by Ctrl-Alt-Shift-F12\n");

	GuiTextOut ("> ");

	DisplayBuffer();

	/*
	//DEBUG
	{
		PVOID ptr = ExAllocatePoolWithTag (PagedPool, 0x10000, ' kdD');
		KdPrint(("ptr = %x\n", ptr));

		*(ULONG*)ptr = 0x11111111;

		KdPrint(("ok\n"));
		ExFreePool (ptr);
	}
	//DEBUG
	*/

	//
	// Directly wait on keyboard at VERY HIGH IRQL !!!
	// IRQL not is at DIRQL level for keyboard IRQ, but
	// we cannot setup a DPC or something else to wait
	// when IRQL will be lowered by system, alse we cannot
	// lower IRQL manually because system will become 
	// unstable. So we have to loop until the key is pressed.
	//

	StopProcessingCommands = FALSE;
	ExceptionShouldBeDispatched = FALSE;

	KdPrint(("Waiting for user input\n"));

	//
	// Poll until escape is pressed.
	//

	UCHAR Byte;

	char command[200];
	int iPos = 0;
	command[0] = 0;

	do
	{
		PollIdle ();

		Byte = KbdGetKeyPolled (TRUE);

		if (Byte == 0)
			continue;

		if (Byte == 1)
			break;

		KeybProcessUserInputLocked (Byte);

		// get ascii code
		UCHAR AsciiCode[2];
		AsciiCode[0] = KeybdScanCodeToAsciiCode (Byte);

		if (Byte == 0x0E)
		{	
			if (iPos != 0)
			{
				iPos --;
				GuiTextOut ("\b");
				DisplayBuffer();
			}
			// don't save this char
			AsciiCode[0] = 0;
		}

		if (AsciiCode[0] == 10)
		{
			command[iPos] = 0;
			AsciiCode[1] = 0;
			GuiTextOut ((PCHAR)AsciiCode);

			//KdPrint(("Endl, got string '%s'\n", command));
			ProcessCommand (command);

			if (StopProcessingCommands)
				break;

			GuiTextOut("> ");
			DisplayBuffer();

			iPos = 0;
			AsciiCode[0] = 0;
		}

		if (AsciiCode[0])
		{
			AsciiCode[1] = 0;
			GuiTextOut ((PCHAR)AsciiCode);
			DisplayBuffer();

			command[iPos++] = AsciiCode[0];
		}
	}
	while (TRUE);


	KdPrint(("Leaving debugger (BYTE %X StopProcessingCommands %X)\n", Byte, StopProcessingCommands));

	//
	// We are going to return from debugger
	//

	if (!UserInitiated)
	{
		Callback (FALSE, Argument, ExceptionShouldBeDispatched);
	}

	//
	// Resetting kbd leds
	//
	// Win LEDs vulues will be set by DPC routine
	//

//	KbdSetLeds (WindowsNum, WindowsCaps, WindowsScroll);

	if (BootDebuggingInitiated == FALSE)
	{
		//
		// Restore screen
		//

		Point.x = 0;
		Point.y = 0;
		Rect.left = StartX;
		Rect.top = StartY;
		Rect.right =  StartX + Width;
		Rect.bottom = StartX + Height;

		s = xxxDrvCopyBits(
				pPrimarySurf, 
				BackupSurf->pSurface, 
				NULL,
				NULL,
				&Rect,
				&Point
				);
	}

	KdPrint(("WR_ENTER_DEBUGGER exit\n"));

	DbgThawProcessors();

	KfRaiseIrql (Irql);
	KdPrint(("IRQL raised!!\n"));
	DbgEnteredDebugger = FALSE;

	if (BootDebuggingInitiated == FALSE)
	{
		EngFastDetachProcess (State);
	}
}

