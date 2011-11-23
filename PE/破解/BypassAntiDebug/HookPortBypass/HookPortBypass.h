/*

  HookPortBypass.h

  Author: Adly
  Last Updated: 2008-11-16

*/

#ifndef _HOOKPORTBYPASS_H
#define _HOOKPORTBYPASS_H 1

//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-0x7FFF(32767), and 0x8000(32768)-0xFFFF(65535)
// are reserved for use by customers.
//

#define FILE_DEVICE_HOOKPORTBYPASS	0x8000

//
// Macro definition for defining IOCTL and FSCTL function control codes. Note
// that function codes 0-0x7FF(2047) are reserved for Microsoft Corporation,
// and 0x800(2048)-0xFFF(4095) are reserved for customers.
//

#define HOOKPORTBYPASS_IOCTL_BASE	0x800

//
// The device driver IOCTLs
//

#define CTL_CODE_HOOKPORTBYPASS(i) CTL_CODE(FILE_DEVICE_HOOKPORTBYPASS, HOOKPORTBYPASS_IOCTL_BASE+i, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_HOOKPORTBYPASS_TEST0	CTL_CODE_HOOKPORTBYPASS(0)
#define IOCTL_HOOKPORTBYPASS_TEST1	CTL_CODE_HOOKPORTBYPASS(1)

#define IOCTL_HOOKPORTBYPASS_HOOKPORT	CTL_CODE_HOOKPORTBYPASS(2)
#define IOCTL_HOOKPORTBYPASS_HOOKINT1	CTL_CODE_HOOKPORTBYPASS(3)
#define IOCTL_HOOKPORTBYPASS_UNHOOKPORT	CTL_CODE_HOOKPORTBYPASS(4)
//
// Name that Win32 front end will use to open the HookPortBypass device
//

#define HOOKPORTBYPASS_WIN32_DEVICE_NAME_A	"\\\\.\\GalaxyAPhookPort"
#define HOOKPORTBYPASS_WIN32_DEVICE_NAME_W	L"\\\\.\\GalaxyAPhookPort"
#define HOOKPORTBYPASS_DEVICE_NAME_W			L"\\Device\\GalaxyAPhookPort"
#define HOOKPORTBYPASS_DOS_DEVICE_NAME_W		L"\\DosDevices\\GalaxyAPhookPort"


#endif