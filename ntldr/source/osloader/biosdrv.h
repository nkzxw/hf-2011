//********************************************************************
//	created:	13:8:2008   8:22
//	file:		biosdrv.h
//	author:		tiamo
//	purpose:	bios device
//********************************************************************

#pragma once

//
// ESC
//
#define ESC_KEY											0x011b

//
// backspace
//
#define BACKSPACE_KEY									0x0e08

//
// tab key
//
#define TAB_KEY											0x0f00

//
// F1
//
#define F1_KEY											0x3b00

//
// F2
//
#define F2_KEY											0x3c00

//
// F3
//
#define F3_KEY											0x3d00

//
// F4
//
#define F4_KEY											0x3e00

//
// F5
//
#define F5_KEY											0x3f00

//
// F6
//
#define F6_KEY											0x4000

//
// F7
//
#define F7_KEY											0x4100

//
// F8
//
#define F8_KEY											0x4200

//
// F9
//
#define F9_KEY											0x4300

//
// F10
//
#define F10_KEY											0x4400

//
// HOME
//
#define HOME_KEY										0x4700

//
// UP
//
#define UP_ARROW										0x4800

//
// LEFT
//
#define LEFT_ARROW										0x4b00

//
// RIGHT
//
#define RIGHT_ARROW										0x4d00

//
// END
//
#define END_KEY											0x4f00

//
// DOWN
//
#define DOWN_ARROW										0x5000

//
// insert
//
#define INSERT_KEY										0x5200

//
// DEL
//
#define DEL_KEY											0x5300

//
// F11
//
#define F11_KEY											0x5700

//
// F12
//
#define F12_KEY											0x5800

//
// null
//
#define ASCI_NUL										0x00

//
// bell
//
#define ASCI_BEL										0x07

//
// backspace
//
#define ASCI_BS											0x08

//
// horizontal tab
//
#define ASCI_HT											0x09

//
// line feed
//
#define ASCI_LF											0x0a

//
// 	vertical tab
//
#define ASCI_VT											0x0b

//
// form feed
//
#define ASCI_FF											0x0d

//
// return
//
#define ASCI_CR											0x0d

//
// esc
//
#define ASCI_ESC										0x1b

//
// input control sequence introducer.
//
#define ASCI_CSI_IN										0x9b
//
// escape-leftbracket
//
#define ASCI_CSI_OUT									"\033["

//
// disk sector size
//
#define SECTOR_SIZE										512

//
// sector shift
//
#define SECTOR_SHIFT									9

//
// open bios console
//
ARC_STATUS BiosConsoleOpen(__in PCHAR OpenPath,__in OPEN_MODE OpenMode,__out PULONG FileId);

//
// read console status
//
ARC_STATUS BiosConsoleReadStatus(__in ULONG FileId);

//
// read console
//
ARC_STATUS BiosConsoleRead(__in ULONG FileId,__out PUCHAR Buffer,__in ULONG Length,__out PULONG Count);

//
// write console
//
ARC_STATUS BiosConsoleWrite(__in ULONG FileId,__out PUCHAR Buffer,__in ULONG Length,__out PULONG Count);

//
// get key
//
ULONG BlGetKey();

//
// open partition
//
ARC_STATUS BiosPartitionOpen(__in PCHAR OpenPath,__in OPEN_MODE OpenMode,__out PULONG FileId);

//
// close partition
//
ARC_STATUS BiosPartitionClose(__in ULONG FileId);