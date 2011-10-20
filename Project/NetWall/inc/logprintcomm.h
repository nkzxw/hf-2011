
/////////////////////////////////////////////////////////////////////////////
//	LogPrint device interface GUID

// {ED6026A8-6813-11d2-AE83-00C0DFE8C1F8}
DEFINE_GUID(LOGPRINT_GUID, 0xed6026a8, 0x6813, 0x11d2, 0xae, 0x83, 0x0, 0xc0, 0xdf, 0xe8, 0xc1, 0xf8);

//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//
#define FILE_DEVICE_LOGPRINT      0x00008600

//
// Version #
//
#define LOGPRINT_VERSION    0x01000000
//
// Commands that the GUI can send the device driver
//
#define IOCTL_LOGPRINT_GET_VERSION   (ULONG) CTL_CODE(FILE_DEVICE_LOGPRINT, 0x00, METHOD_BUFFERED, FILE_ANY_ACCESS)
