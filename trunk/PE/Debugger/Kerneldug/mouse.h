#ifndef _MOUSE_H_
#define _MOUSE_H_


// MOUSE_STATE_CHANGE_PACKET::Flags bits
#define MOUSE_STATE_LEFTBTN		0x01
#define MOUSE_STATE_RIGHTBTN	0x02
#define MOUSE_STATE_MIDDLEBTN	0x04
#define MOUSE_STATE_MOVE_X		0x08
#define MOUSE_STATE_MOVE_Y		0x10
#define MOUSE_STATE_MOVE_WHEEL	0x20
#define MOUSE_STATE_RESERVED1	0x40
#define MOUSE_STATE_RESERVED2	0x80

typedef struct MOUSE_STATE_CHANGE_PACKET
{
	UCHAR Flags;

	//
	// These fields always contains valid data.
	// Flags' bits contain information about only those fields
	// which values have been changed.
	//

	// buttons
	BOOLEAN Left;
	BOOLEAN Right;
	BOOLEAN Middle;

	// coordinates
	LONG X;
	LONG Y;
	LONG Wheel;
} *PMOUSE_STATE_CHANGE_PACKET;

VOID
MouseInitialize(
	VOID (*)(PMOUSE_STATE_CHANGE_PACKET)
	);

VOID
StateChangeCallbackRoutine(
	PMOUSE_STATE_CHANGE_PACKET StateChange
	);

#endif
