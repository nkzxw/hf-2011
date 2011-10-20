/*++

	This is the part of NGdbg kernel debugger

	mouse.cpp

	High-level PS/2 mouse driver for the kernel debugger

--*/

#include <ntifs.h>
#include "dbgeng.h"
#include "i8042.h"
#include "gui.h"

typedef union MOUSE_PACKET
{
	struct
	{
		union
		{
			UCHAR Buttons;
			struct
			{
				UCHAR Left : 1;			// Left button down
				UCHAR Right : 1;		// Right button down
				UCHAR Middle : 1;		// Middle button down
				UCHAR Synch : 1;		// Synchronizattion bit, always 1
				UCHAR XSign : 1;		// XMovement is negative (LEFT)
				UCHAR YSign : 1;		// YMovement is negative (DOWN)
				UCHAR XOverflow : 1;	// XMovement overflow
				UCHAR YOverflow : 1;	// YMovement overflow
			} e1;
		} u1;
		UCHAR XMovement;
		UCHAR YMovement;
		UCHAR ZMovement;
	};
	UCHAR Raw[4];
} *PMOUSE_PACKET;

UCHAR 
MouGetBytePolled(
	)

/*++

Routine Description

	Read mouse byte in polled mode.
	Simply call 8042 driver

Arguments

	None

Return Value

	Mouse byte or 0 if the timeout occured

Environment
	
	This function is called at raised IRQL

--*/

{
	NTSTATUS Status;
	UCHAR Byte;

	Status = I8xGetBytePolled (MouseDevice, &Byte, FALSE);

	if (!NT_SUCCESS(Status))
		Byte = 0;

	return Byte;
}

#if DBG && 0
VOID
MouDumpPacket (
	PMOUSE_PACKET Packet
	)

/*++

Routine Description

	This function DbgPrints full mouse packet.
	In retail version it corresponds to empty macro

Arguments

	Packet

		Pointer to mouse packet read from 60h port of 8042 controller

Return Value

	None

Environment

	Raised IRQL

--*/

{
	KdPrint(("MOUSE PACKET [L %d M %d R %d Synch %d XS %d YS %d XV %d YV %d  X %d Y %d Z %d]\n",
		Packet->u1.e1.Left,
		Packet->u1.e1.Middle,
		Packet->u1.e1.Right,
		Packet->u1.e1.Synch,
		Packet->u1.e1.XSign,
		Packet->u1.e1.YSign,
		Packet->u1.e1.XOverflow,
		Packet->u1.e1.YOverflow,
		Packet->XMovement,
		Packet->YMovement,
		Packet->ZMovement
		));
}
#else
#define MouDumpPacket(PACKET) NOTHING;
#endif

BOOLEAN Left, Middle, Right;

MOUSE_PACKET Packet;
UCHAR BytesLoaded = 0;

LONG MouseX = 0;
LONG MouseY = 0;
LONG MouseWheel = 0;

LONG OldMouseX = 0;
LONG OldMouseY = 0;

BOOLEAN Resynch = FALSE;

#include "mouse.h"

VOID
(*MouseStateChangeCallback)(
	PMOUSE_STATE_CHANGE_PACKET
	);

VOID
ReportMouseStateChange(
	)
{
	MOUSE_STATE_CHANGE_PACKET StateChange = {0};

	//
	// Calculate new values for button states
	//  and currect X,Y,wheel coordinates.
	//

	CHAR ShiftX = (CHAR) Packet.XMovement;
	CHAR ShiftY = (CHAR) Packet.YMovement;
	CHAR ShiftZ = (CHAR) Packet.ZMovement;

	OldMouseX = MouseX;
	OldMouseY = MouseY;

	if (Packet.u1.e1.XSign)
	{
		MouseX += ShiftX;		// ShiftX already have correct sign
	}
	else
	{
		MouseX += (ULONG)ShiftX;		// ShiftX is negative
	}

	if (Packet.u1.e1.YSign)
	{
		MouseY += ShiftY;
	}
	else
	{
		// Up
		MouseY += (ULONG) ShiftY;
	}

	MouseWheel += ShiftZ; // with correct sign

	if (Left != Packet.u1.e1.Left)
	{
		Left = Packet.u1.e1.Left;
		StateChange.Flags |= MOUSE_STATE_LEFTBTN;
	}

	if (Right != Packet.u1.e1.Right)
	{
		Right = Packet.u1.e1.Right;
		StateChange.Flags |= MOUSE_STATE_RIGHTBTN;
	}

	if (Middle != Packet.u1.e1.Middle)
	{
		Middle = Packet.u1.e1.Middle;
		StateChange.Flags |= MOUSE_STATE_MIDDLEBTN;
	}

	if (ShiftX)
		StateChange.Flags |= MOUSE_STATE_MOVE_X;

	if (ShiftY)
		StateChange.Flags |= MOUSE_STATE_MOVE_Y;

	if (ShiftZ)
		StateChange.Flags |= MOUSE_STATE_MOVE_WHEEL;

	//
	// Fill out StateChange fields
	//

	StateChange.X = MouseX;
	StateChange.Y = MouseY;
	StateChange.Wheel = MouseWheel;

	StateChange.Left = Left;
	StateChange.Right = Right;
	StateChange.Middle = Middle;

	//
	// Packet OK.
	// Call our callback
	//

	if (MouseStateChangeCallback)
		MouseStateChangeCallback (&StateChange);
}

VOID
ProcessMouseInput (
	UCHAR Byte
	)

/*++

Routine Description
	
	This function is always called when kernel debugger is active to 
	 process data byte from mouse when i8042 driver detects mouse packet.
	NGdbg drives PS/2 mouse in polled mode because all its interface code
	 is executing at IRQL at least DIRQL for keyboard or higher.
	So it is not necessary to set up mouse ISR routine.

Arguments

	Byte

		Data byte from 60h port, which is detected as mouse byte by
		 i8042 driver.

Return Value

	None

Environment

	IRQL = Keyboard DIRQL or higher.
	Called from I8xGetBytePolled.

--*/

{
//	KdPrint (("Mouse input.. %d [%X]\n", BytesLoaded, Byte));

	if (Resynch)
	{
		BytesLoaded = 0;
		Packet.Raw[BytesLoaded] = Byte;
		if (!( Packet.u1.e1.Synch &&
			Packet.u1.e1.Left &&
			Packet.u1.e1.Right &&
			!Packet.u1.e1.Middle))
		{
			KdPrint(("Re-synching [%X]..\n", Byte));
			return;
		}

		++ BytesLoaded;

		KdPrint(("Resynchronized\n"));
		Resynch = FALSE;
		GuiTextOut ("Re-synchronized\n");
		DisplayBuffer ();
		return;
	}

	Packet.Raw[BytesLoaded] = Byte;
	++ BytesLoaded;
		
	if (BytesLoaded == 4)
	{
		BytesLoaded = 0;

//		KdPrint(("Got full mouse packet\n"));

		if (!Packet.u1.e1.Synch)
		{
			KdPrint(("Mouse packet corrupted, dump follows\n"));
			MouDumpPacket (&Packet);

			Resynch = TRUE;
			KdPrint(("Re-synchronization request, please, press left+right buttons\n"));
			GuiTextOut ("Re-synchronization request, please, press left+right buttons\n");
			DisplayBuffer ();

			return;
		}

		//
		// Got valid mouse p 8acket.
		//

		MouDumpPacket (&Packet);

		ReportMouseStateChange ();
	}
}

VOID
I8xSetupMouseCallack(
	VOID (*MouseCallback)(UCHAR)
	);

VOID
MouseInitialize(
	VOID (*StateChangeCallback)(PMOUSE_STATE_CHANGE_PACKET)
	)

/*++
	
Routine Description

	This routine initializes mouse support by calling 8042.sys!I8xSetupMouseCallback

Arguments

	StateChangeCallback
	
		State change callback which will be called when new packet from mouse
		 is received.

Return Value

	None

Environment

	This function is called during debugger initialization from DriverEntry or BootStartup

--*/

{
	I8xSetupMouseCallack (ProcessMouseInput);

	MouseStateChangeCallback = StateChangeCallback;
}