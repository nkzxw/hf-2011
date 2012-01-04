/*

  TEST_KIDISPAT.h

  Author: Adly
  Last Updated: 2008-11-16

*/

#ifndef _TEST_KIDISPAT_H
#define _TEST_KIDISPAT_H 1

//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-0x7FFF(32767), and 0x8000(32768)-0xFFFF(65535)
// are reserved for use by customers.
//

#define FILE_DEVICE_TEST_KIDISPAT	0x8000

//
// Macro definition for defining IOCTL and FSCTL function control codes. Note
// that function codes 0-0x7FF(2047) are reserved for Microsoft Corporation,
// and 0x800(2048)-0xFFF(4095) are reserved for customers.
//

#define TEST_KIDISPAT_IOCTL_BASE	0x800

//
// The device driver IOCTLs
//

#define CTL_CODE_TEST_KIDISPAT(i) CTL_CODE(FILE_DEVICE_TEST_KIDISPAT, TEST_KIDISPAT_IOCTL_BASE+i, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_TEST_KIDISPAT_TEST0	CTL_CODE_TEST_KIDISPAT(0)
#define IOCTL_TEST_KIDISPAT_TEST1	CTL_CODE_TEST_KIDISPAT(1)

#define IOCTL_TEST_KIDISPAT_Set	 CTL_CODE_TEST_KIDISPAT(2)
//old to my
#define IOCTL_TEST_KIDISPAT_InlineHook	 CTL_CODE_TEST_KIDISPAT(3)
#define IOCTL_TEST_KIDISPAT_NewOsAddress	 CTL_CODE_TEST_KIDISPAT(4)
#define IOCTL_TEST_KIDISPAT_UNHOOK	 CTL_CODE_TEST_KIDISPAT(5)
#define IOCTL_TEST_KIDISPAT_PROTECT_INFO	 CTL_CODE_TEST_KIDISPAT(6)
//old to new
#define IOCTL_TEST_KIDISPAT_HOOK_OLD	 CTL_CODE_TEST_KIDISPAT(7)
// new to my
#define IOCTL_TEST_KIDISPAT_NEW2MY	 CTL_CODE_TEST_KIDISPAT(8)

#define IOCTL_TEST_KIDISPAT_HOOK_REPLACE	 CTL_CODE_TEST_KIDISPAT(9)



typedef	struct	__PROTECTINFO
{
	
		int					PidOrName;
		char			szNameBuffer[256];
		ULONG		PID;
}PROTECT_INFO,  *PPROTECT_INFO;

//
// Name that Win32 front end will use to open the TEST_KIDISPAT device
//

#define TEST_KIDISPAT_WIN32_DEVICE_NAME_A	"\\\\.\\GalaxyAPkidisp"
#define TEST_KIDISPAT_WIN32_DEVICE_NAME_W	L"\\\\.\\GalaxyAPkidisp"
#define TEST_KIDISPAT_DEVICE_NAME_W			L"\\Device\\GalaxyAPkidisp"
#define TEST_KIDISPAT_DOS_DEVICE_NAME_W		L"\\DosDevices\\GalaxyAPkidisp"


#endif




















