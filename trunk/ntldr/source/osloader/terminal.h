//********************************************************************
//	created:	11:8:2008   18:35
//	file:		terminal.h
//	author:		tiamo
//	purpose:	terminal
//********************************************************************

#pragma once

//
// redirection information
//
typedef struct _LOADER_REDIRECTION_INFORMATION
{
	//
	// valid
	//
	BOOLEAN												Valid;

	//
	// reserved
	//
	UCHAR												Reserved;

	//
	// stop bits
	//
	UCHAR												StopBits;

	//
	// parity
	//
	UCHAR												Parity;

	//
	// baudrate
	//
	ULONG												Baudrate;

	//
	// port number
	//
	ULONG												PortNum;

	//
	// port address
	//
	ULONG												PortAddress;

	//
	// pci device id
	//
	USHORT												PciDeviceId;

	//
	// pci vendor id
	//
	USHORT												PciVendorId;

	//
	// bus
	//
	UCHAR												BusNumber;

	//
	// device
	//
	UCHAR												DeviceNumber;

	//
	// function
	//
	UCHAR												FunctionNumber;

	//
	// reserved
	//
	UCHAR												Reserved2;

	//
	// flags
	//
	ULONG												Flags;

	//
	// guid
	//
	GUID												Guid;

	//
	// memory space
	//
	BOOLEAN												MemorySpace;

	//
	// terminal type
	//
	UCHAR												TerminalType;

	//
	// reserved
	//
	UCHAR												Reserved3;
}LOADER_REDIRECTION_INFORMATION,*PLOADER_REDIRECTION_INFORMATION;

//
// initialize serial console
//
VOID BlInitializeHeadlessPort();

//
// display mini-sac
//
BOOLEAN BlTerminalHandleLoaderFailure();

//
// initialize stdio
//
ARC_STATUS BlInitStdio(__in ULONG Argc,__in PCHAR Argv[]);

//
// clear screen
//
VOID BlClearScreen();

//
// clear to end of screen
//
VOID BlClearToEndOfScreen();

//
// clear to end of line
//
VOID BlClearToEndOfLine();

//
// set inverse mode
//
VOID BlSetInverseMode(__in ULONG Mode);

//
// set cursor position
//
VOID BlPositionCursor(__in ULONG x,__in ULONG y);

//
// terminal connected
//
extern BOOLEAN											BlTerminalConnected;

//
// terminal device id
//
extern ULONG											BlTerminalDeviceId;

//
// terminal delay
//
extern ULONG											BlTerminalDelay;

//
// screen width
//
extern ULONG											ScreenWidth;

//
// screen height
//
extern ULONG											ScreenHeight;

//
// redirection info
//
extern LOADER_REDIRECTION_INFORMATION					LoaderRedirectionInfo;