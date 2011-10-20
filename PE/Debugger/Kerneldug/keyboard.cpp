/*++

	This is the part of NGdbg kernel debugger

	keyboard.cpp

	High-level PS/2 keyboard driver for the kernel debugger

--*/

#include <ntifs.h>
#include "dbgeng.h"
#include "i8042.h"


VOID WR_ENTER_DEBUGGER (BOOLEAN UserInitiated, PDBG_CALLBACK Callback, PVOID Argument);
VOID ProcessMouseInput (UCHAR Byte);

//
// Scan-Code tables
//

char KeybdAsciiCodes[] =
{
	0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,
		'q','w','e','r','t','y','u','i','o','p','[',']',10,0,
		'a','s','d','f','g','h','j','k','l',';','\'', '`',0,
		'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,
		' ',0, 0,0,0,0,0,0,0,0,0,0, 0,0, '7','8','9','-','4','5',
		'6','+','1','2','3','0','.', 0,0
};

char KeybdAsciiCodesShifted[] =
{
	0,0,'!','@','#','$','%','^','&','*','(',')','_','+',0,0,
		'Q','W','E','R','T','Y','U','I','O','P','{','}',10,0,
		'A','S','D','F','G','H','J','K','L',':','"', '~',0,
		'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,
		' ',0, 0,0,0,0,0,0,0,0,0,0, 0,0, '7','8','9','-','4','5',
		'6','+','1','2','3','0','.', 0,0
};

char *KeybdScanToAsciiTables[] = { KeybdAsciiCodes, KeybdAsciiCodesShifted };



#if 0

UCHAR KbdGetKeyPolled()
{
	UCHAR ScanCode;
	UCHAR AsciiCode;
	UCHAR response;
	UCHAR Byte;

	//
	// Poll until we get back a controller status value that
	// indicates the output buffer is full
	//

	while (((response = KBD_GET_STATUS_BYTE()) & BUFFER_FULL) != I8042_OUTPUT_BUFFER_FULL)
	{
		if (response & I8042_AUXOUT_BUFFER_FULL)
		{
			//
			// There is something in the i8042 output buffer,
			// but it isn't from the device we want to get a byte from
			// Eat the byte and try again
			//

			Byte = KBD_GET_DATA_BYTE ();
			KdPrint(("KBDPOLL: Response %X ate %x\n", response, Byte));
		}
		else
		{
			//KdPrint(("KBDPOLL: Response %X Stalling\n", response));
			KeStallExecutionProcessor (200);
		}
	}

	//KdPrint(("KBDPOLL: Response %X, got\n", response));

	ScanCode = KBD_GET_DATA_BYTE();

	AsciiCode  = KeybdScanCodeToAsciiCode (ScanCode);

#if 0
	KdPrint (("KBDPOLL: exit with byte %02X ", ScanCode));
	if (AsciiCode)
		KdPrint(("(%c)", AsciiCode));
	KdPrint(("\n"));
#endif

	return ScanCode;
}

UCHAR KbdGetKeyAsychnorous()
{
	UCHAR response;
	UCHAR Byte;
	
	//
	// Poll until we get back a controller status value that indicates
	// the output buffer is full.
	//
	
	while (((response = KBD_GET_STATUS_BYTE()) & BUFFER_FULL) != I8042_OUTPUT_BUFFER_FULL)
	{
		if (response & I8042_AUXOUT_BUFFER_FULL)
		{
			//
			// There is something in the i8042 output buffer, but it
			// isn't from the device we want to get a byte from. Eat
			// the byte and try again.
			//

			Byte = KBD_GET_DATA_BYTE();
			KdPrint(("KBDASYNCH: ate %X\n", Byte));
		}
		else
		{
			// Try again
		}
	}

	Byte = KBD_GET_DATA_BYTE();

	//KdPrint(("KBDASYNC: exit with byte %X\n", Byte));

	return Byte;
}

#endif

UCHAR 
KbdGetKeyPolled(
	IN BOOLEAN AllowMouseCallback
	)

/*++

Routine Description

	Read scan-code in polled mode.
	Simply call 8042 driver

Arguments

	AllowMouseCallback

		Should 8042.sys call mouse callback if it detects byte from mousr

Return Value

	Scan-code or 0 if the timeout occured

Environment
	
	This function is called at raised IRQL

--*/

{
	NTSTATUS Status;
	UCHAR Byte;

	Status = I8xGetBytePolled (KeyboardDevice, &Byte, AllowMouseCallback);

	if (!NT_SUCCESS(Status))
		Byte = 0;

	return Byte;
}

extern KDPC HotkeyResetStateDpc;


VOID
PutScanCode(
	UCHAR ScanCode
	)

/*++

Routine Description

	This routine puts scan-code to 8042 output register
	and requests keyboard IRQ as if user pressed key manually.
	It is used to simulate some scan-codes.

Arguments

	ScanCode

		Scan code to put

Return Value

	None

Environment

	This function must be called at DISPATCH_LEVEL <= IRQL < keyboard DIRQL because
	IRQ request should occur

--*/

{
	//
	// Raise IRQL to dispatch if it is less than dispatch level
	//

	KIRQL Irql;
	Irql = KfRaiseIrql (DISPATCH_LEVEL);

	// Tell 8042 that we want to write to output register
	I8xPutBytePolled (CommandPort, 
		ControllerDevice, 
		FALSE, 
		(UCHAR)I8042_WRITE_OUTPUT_REGISTER);

	// Write value to 8042 output register
	I8xPutBytePolled (DataPort, 
		ControllerDevice, 
		FALSE, 
		ScanCode);

	//
	// Lower IRQL back.
	// Check build of NT bugchecks if KeLowerIrql tries to lower
	// irql greater than current value.
	// So, use this function only at DISPATCH_LEVEL
	//

	KfLowerIrql (Irql);
}



BOOLEAN WindowsNum;
BOOLEAN WindowsCaps;
BOOLEAN WindowsScroll;

BOOLEAN DbgNum;
BOOLEAN DbgCaps;

VOID
KbdSetLeds(
	BOOLEAN Num,
	BOOLEAN Caps,
	BOOLEAN Scroll
	)

/*++

Routine Description

	Set keyboard LEDs.
	This function is always called at high irql.

Arguments

	Num, Caps, Scroll - new values for keyboard indicators

Return Value

	None

Environment

	Raised IRQL

--*/

{
	UCHAR LedStatus = 0;
	if (Num)
		LedStatus |= 2;
	if (Caps)
		LedStatus |= 4;
	if (Scroll)
		LedStatus |= 1;

	I8xPutBytePolled (DataPort, KeyboardDevice, TRUE, (UCHAR) KBD_SET_INDICATORS);
	I8xPutBytePolled (DataPort, KeyboardDevice, TRUE, LedStatus);
}

VOID
  HotkeyResetStateDeferredRoutine(
    IN struct _KDPC  *Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    )

/*++

Routine Description

	CALLBACK

	DPC deferred routine.
	Put break codes for ctrl, alt shift and F12
	to let i8042 know that user have released them and
	 restore windows LED states

 Arguments

	Dpc

		Pointer to KPDC

	DeferredContext
	SystemArgument1
	SystemArgument2

		Always NULL

Return Value

	None

Environment

	This is DPC routine, so it executes at DISPATCH_LEVEL

--*/

{
	KdPrint(("Ctrl-Shift-Alt restore DPC7\n"));

	PutScanCode (0xAA); // Shift UP
	PutScanCode (0x9D); // Ctrl  UP
	PutScanCode (0xB8); // Alt   UP
	PutScanCode (0xD8); // F12   UP

	KbdSetLeds (WindowsNum, WindowsCaps, WindowsScroll);
}


VOID
ProcessLockKeys (
	UCHAR ScanCode,
	BOOLEAN UP
	)

/*++

Routine Description

	This function is called when kernel debugger is active to process scan-code
	Process num and caps lock. Scroll lock is always ON when kernel gui is active

Argument

	ScanCode

		Low 7 bits of scan code

	UP

		High 8th bit of scan code

Return Value

	None

Environment

	Raised IRQL, kernel debugger active

--*/

{
	switch (ScanCode)
	{
	case 0x45: // NUM
		if (!UP)
		{
			DbgNum = !DbgNum;
			KbdSetLeds (DbgNum, DbgCaps, 1);
			KdPrint(("dbg num lock %s\n", DbgNum ? "on" : "off"));
		}
		break;

	case 0x3A: // CAPS
		if (!UP)
		{
			DbgCaps = !DbgCaps;
			KbdSetLeds (DbgNum, DbgCaps, 1);
			KdPrint(("dbg caps lock %s\n", DbgCaps ? "on" : "off"));
		}
		break;
	}
}

// Shift, Ctrl and Alt states
BOOLEAN Shift = FALSE, Ctrl = FALSE, Alt = FALSE;


VOID
ProcessControlKeys (
	UCHAR ScanCode,
	BOOLEAN UP
	)

/*++

Routine Description

	This function is called at every key press.
	Action: Process control keys - shift, control, alt.

Arguments

	ScanCode

		Low 7 bits of scan code

	UP

		High 8th bit of scan code

Return Value

	None

Environment

	Raised IRQL


--*/

{
	switch (ScanCode)
	{
	case 42:
	case 54:
		Shift = !UP;
		break;

	case 0x1D:
		Ctrl = !UP;
		break;

	case 56:
		Alt = !UP;
		break;
	}
}


VOID
KeybProcessUserInputLocked (
	UCHAR ScanCode
	)

/*++

Routine Description

	Process user input when kernel debugger is active.

Arguments

	Full 8-bit scan code

Return Value

	None

Environment

	Raised IRQL, kernel debugger active

--*/

{
	BOOLEAN UP;

	UP = ScanCode >> 7;
	ScanCode &= 0x7F;

	ProcessControlKeys (ScanCode, UP);
	ProcessLockKeys (ScanCode, UP);
}

extern BOOLEAN BootDebuggingInitiated;

VOID 
ProcessScanCode (
	UCHAR ScanCode
	)

/*++

Routine Description

	Process key press.

Arguments

	ScanCode

		Full 8-bit scan code

Return Value

	None

Environment

	Raised IRQL, called from hook set by IOCTL_INTERNAL_I8042_HOOK_KEYBOARD

--*/

{
	BOOLEAN UP;

	UP = ScanCode >> 7;
	ScanCode &= 0x7F;

	ProcessControlKeys (ScanCode, UP);

	switch (ScanCode)
	{
	// Copy windows lock key states to global var.

	case 0x3A: // CAPS
		if (!UP)
		{
			WindowsCaps = !WindowsCaps;
			KdPrint(("win caps lock %s\n", WindowsCaps ? "on" : "off"));
		}
		break;

	case 0x45: // NUM
		if (!UP)
		{
			WindowsNum = !WindowsNum;
			KdPrint(("win num lock %s\n", WindowsNum ? "on" : "off"));
		}
		break;

	case 0x46: // SCROLL
		if (!UP)
		{
			WindowsScroll = !WindowsScroll;
			KdPrint(("win scrl lock %s\n", WindowsScroll ? "on" : "off"));
		}
		break;

	case 0x58:
		if (Ctrl && Alt && Shift)
		{
			KdPrint(("Enter debugger\n"));
			WR_ENTER_DEBUGGER (TRUE, NULL, NULL);
			KdPrint(("Exit debugger\n"));

			if (BootDebuggingInitiated == FALSE)
			{
				//
				// Put break codes for ctrl, alt and shift
				// to let i8042 know that user have released them.
				//

				KeInsertQueueDpc (
					&HotkeyResetStateDpc,
					NULL,
					NULL
					);
			}
		}
		break;
	}
}

//
// Convert scan code to ascii code
//

UCHAR KeybdScanCodeToAsciiCode (UCHAR ScanCode)
{
	BOOLEAN Shifted = Shift;
	if (DbgCaps)
	{
		Shifted = !Shifted;
	}

	if (ScanCode < sizeof(KeybdAsciiCodes))
        return KeybdScanToAsciiTables[Shifted][ScanCode];

	return 0;
}

