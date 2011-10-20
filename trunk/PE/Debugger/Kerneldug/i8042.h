#ifndef _8042_H_
#define _8042_H_

/******************************************************************\
 *            8042 registers, commans, responses                  *
\******************************************************************/

//
// 8042 keyboard registers
//

#define KBD_STATUS_REGISTER		((PUCHAR)0x64)	// Status register, Read-Only
#define KBD_CONTROL_REGISTER	((PUCHAR)0x64)	// Control register, Write-Only
#define KBD_DATA_REGISTER		((PUCHAR)0x60)	// Data regiser, Read/Write

//
// 8042 contoller commands (port 64h)
//

#define I8042_READ_COMMAND_BYTE		0x20		// Read i8042 command byte
#define I8042_WRITE_COMMAND_BYTE	0x60		// Write i8042 command byte
#define I8042_DISABLE_MOUSE			0xA7		// Disable mouse
#define I8042_ENABLE_MOUSE			0xA8		// Enable mouse
#define I8042_AUXILIARY_DEVICE_TEST	0xA9		// Auxiliary device test
#define I8042_SELF_TEST				0xAA		// Self test of i8042
#define I8042_KEYBOARD_DEVICE_TEST	0xAB		// Keyboard test
#define I8042_READ_RAMCTRL			0xAC		// Read controller's RAM
#define I8042_DISABLE_KEYBOARD		0xAD		// Disable keyboard
#define I8042_ENABLE_KEYBOARD		0xAE		// Enable keyboard
#define I8042_READ_INPORT			0xC0		// Read i8042 input port
												// Bit description:
												//  bit  7	  0=KeyLock enabled
												//  bit  6	  videomode, 0=CGA, 1=MDA
												//  bit  5	  reserved
												//  bit  4    system RAM: 0 = more than 512K, 1=256K
												//  bits 3:2  reserved
												//  bit  1    data input of aux interface (MOUSE)
												//  bit  0    data input of main interface (KEYB)
#define I8042_READ_OUTPORT			0xD0		// Read i8042 output port
#define I8042_WRITE_OUTPORT			0xD1		// Write i8042 output port
												// Bit description:
												//  bit  7    serial keyboard data
												//  bit  6    keyboard interface synchronization
												//  bit  5    IRQ from aux. interface (IRQ12, MOUSE)
												//  bit  4    IRQ from main interface (IRQ1, KEYBRD)
												//  bit  3    aux interface (mouse) syncronization
												//  bit  2    serial auxiliary (mouse) data
												//  bit  1    Gate A20
												//  bit  0    processor reset
#define I8042_WRITE_OUTPUT_REGISTER	0xD2
#define I8042_WRITE_AUXOUT_REGISTER	0xD3
#define I8042_WRITE_TO_AUX_DEVICE	0xD4		// Write to auxiliary device
#define I8042_DISABLE_A20_CONTROL	0xDD		// Enable A20 line control
#define I8042_ENABLE_A20_CONTROL	0xDF		// Disable A20 line contol
#define I8042_RESET					0xFE

//
// 8042 controller command byte (read/write by commands 20h/60h)
//

#define I8042CCB_ENABLE_KEYBOARD_INTERRUPT	0x01
#define I8042CCB_ENABLE_MOUSE_INTERRUPT		0x02
#define I8042CCB_DISABLE_KEYBOARD_DEVICE	0x10
#define I8042CCB_DISABLE_MOUSE_DEVICE		0x20
#define I8402CCB_KEYBOARD_TRANSLATE_MODE	0x40

//
// 8042 conroller status regiser bits (port 64h)
//

#define I8042_OUTPUT_BUFFER_FULL	0x01
#define I8042_INPUT_BUFFER_FULL		0x02
#define I8042_AUXIN_BUFFER_FULL		0x02
#define I8042_RESET_OK				0x04
#define I8042_COMMAND_OR_DATA		0x08
#define I8042_KEY_LOCKED			0x10
#define I8042_AUXOUT_BUFFER_FULL	0x20
#define I8042_GENERAL_TIMEOUT		0x40
#define I8042_PARITY_ERROR			0x80

//
// 8042 responses
//

#define I8042RS_ACKNOWLEDGE			0xFA
#define I8042RS_RESEND				0xFE


/******************************************************************\
 *            keyboard registers, commans, responses              *
\******************************************************************/

//
// Keyboard commands
//

#define KBD_ENABLE_SCAN				0xAE		// Enable scan
#define KBD_DISABLE_SCAN			0xAD		// Disable scan
#define KBD_SET_INDICATORS			0xED		// Set/reset mode indicators (LEDs)
												// The following data byte:
												//  bit 0:  1=enable Scroll Lock
												//  bit 1:  1=enable Num Lock
												//  bit 2:  1=enable Caps Lock
#define KBD_ECHO					0xEE		// Send echo diag. command.
												// Response should be EE
#define KBD_SELECT_SCAN_CODE_SET	0xF0		// Select scan-code set
#define KBD_READ_ID_CODE			0xF2		// Read 2-byte identifier
#define KBD_SET_TYPENATIC_RATE		0xF3		// Set typematic rate/delay
												// The following data byte:
												//  bit  7    =0
												//  bits 6:5  delay, ms.: 00=250, 01=500, 10=750, 11=1000
												//  bits 4:0  =00-1F, repeat rate, Hz:
												//
												//  float kbd_repeat_rate[0x20] = {
												//    30.0, 26.7, 24.0, 21.8, 20.0, 18.5, 17.1, 16.0, 15.0, 13.3,
												//       0, 10.0,  9.2,    0,  8.0,  7.5,    0,  6.0,    0,  5.0,
												//       0,    0,  4.0,    0,    0,  3.0,    0,    0,    0,    0,
												//     2.0
												//    };
												//
#define KBD_ENABLE_KEYBOARD			0xF4		// Keyboard response should be ACK and keyboard will continue
												// to scan
#define KBD_RESET_DISABLE_KEYBOARD	0xF5		// Reset to initial state and disable scan
#define KBD_RESET_ENABLE_KEYBOARD	0xF6		// Reset to initial state and enable scan
#define KBD_SET_ALL_KEYS1			0xF7		// \ 
#define KBD_SET_ALL_KEYS2			0xF8		// | Set autorepeat of the whole keyboard
#define KBD_SET_ALL_KEYS3			0xF9		// /
#define KBD_SET_ALL_TYPEMATIC_MAKE_BREAK  0xFA	// Make break code
#define KBD_SET_KEY_TYPES1			0xFB		// \ 
#define KBD_SET_KEY_TYPES2			0xFC		// | Set autorepeat of keys
#define KBD_SET_KEY_TYPES3			0xFD		// /
#define KBD_RESEND					0xFE		// Resend last byte
#define KBD_RESET_AND_TEST			0xFF		// Reset and test keyboard

//
// Keyboard responses
//

#define KBD_KEY_DETECTION_ERROR_1	0x00
#define KBD_ECHO_RESPONSE			0xEE
#define KBD_BREAK_PREFIX			0xF0
#define KBD_COMPLETE_SUCCESS		0xAA
#define KBD_COMPLETE_FAILURE		0xFC
#define KBD_KEY_DETECTION_ERROR_2_3	0xFF
#define KBD_BREAK_CODE				0xF0
#define KBD_DEBUG_HOTKEY_ENH		0x37 // SysReq scan code for Enhanced Keyboard
#define KBD_DEBUG_HOTKEY_AT			0x54 // SysReq scan code for 84-key AT Keyboard

/******************************************************************\
 *            mouse registers, commans, responses                 *
\******************************************************************/

//
// Mouse commands (through 8042 data port)
//

#define MOUSE_SET_RESOLUTION		0xE8
#define MOUSE_SET_SAMPLING_RATE		0xF3
#define MOUSE_RESET					0xFF
#define MOUSE_ENABLE_TRANSMISSION	0xF4
#define MOUSE_SET_SCALING_1TO1		0xE6
#define MOUSE_READ_STATUS			0xE9
#define MOUSE_GET_DEVICE_ID			0xF2

//
// Mouse responses
//

#define MOUSE_COMPLETE				0xAA
#define MOUSE_ID_BYTE				0x00
#define MOUSE_WHEELMOUSE_ID_BYTE	0x03


/******************************************************************\
 *            8042 macroses for port access                       *
\******************************************************************/

#define I8X_GET_STATUS_BYTE()	READ_PORT_UCHAR((PUCHAR)0x64)

#define I8X_GET_DATA_BYTE()		READ_PORT_UCHAR((PUCHAR)0x60)

#define I8X_WRITE_COMMAND_BYTE(BYTE)	WRITE_PORT_UCHAR((PUCHAR)0x64, (BYTE))

#define I8X_WRITE_DATA_BYTE(BYTE)		WRITE_PORT_UCHAR((PUCHAR)0x60, (BYTE))


#define I8042_RESEND_ITERATIONS		200
#define I8042_POLLSTATUS_ITERATIONS	20
#define I8042_POLLING_ITERATIONS	1000
#define I8042_STALL_MICROSECONDS	50


// Was previously defined in NTDDK,
// we don't need that definition
#undef DEVICE_TYPE

typedef enum _DEVICE_TYPE {
	ControllerDevice,
	KeyboardDevice,
	MouseDevice,
	UndefinedDevice
} DEVICE_TYPE, *PDEVICE_TYPE;

typedef enum _PORT_TYPE {
	DataPort = 0,
	CommandPort
} PORT_TYPE, *PPORT_TYPE;

NTSTATUS
I8xGetBytePolled(
	IN CCHAR DeviceType,
	OUT PUCHAR Byte,
	IN BOOLEAN AllowMouseCallback
	);

NTSTATUS
I8xPutBytePolled(
	IN CCHAR PortType,
	IN CCHAR DeviceType,
	IN BOOLEAN WaitForAck,
	IN CCHAR Byte
	);

VOID
I8xGetByteAsynchronous(
	IN CCHAR DeviceType,
	OUT PUCHAR Byte
	);

#endif
