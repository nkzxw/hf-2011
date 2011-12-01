#include "ntifs.h"
#include "create.h"
#include "readwrite.h"
#include "setquery.h"
#include "cleanclose.h"

#ifndef _SF_JT_
#define _SF_JT_

//
//  Macro to test if this is my device object
//

#define IS_MY_DEVICE_OBJECT(_devObj) \
    (((_devObj) != NULL) && \
     ((_devObj)->DriverObject == gSFilterDriverObject) && \
      ((_devObj)->DeviceExtension != NULL))

//
//  Macro to test if this is my control device object
//

#define IS_MY_CONTROL_DEVICE_OBJECT(_devObj) \
    (((_devObj) == gSFilterControlDeviceObject) ? \
            (ASSERT(((_devObj)->DriverObject == gSFilterDriverObject) && \
                    ((_devObj)->DeviceExtension == NULL)), TRUE) : \
            FALSE)

//
//  Buffer size for local names on the stack
//

#define MAX_DEVNAME_LENGTH		64

#define FILE_HEADER_SIZE		0x200	//在这个大小以下，用AES算法会出现问题
#define FILE_EXTENSION_SIZE		0x400

#define IRP_PAGE_SYN_NOCACHE	(IRP_PAGING_IO|IRP_SYNCHRONOUS_PAGING_IO|IRP_NOCACHE)

//
//  Device extension definition for our driver.  Note that the same extension
//  is used for the following types of device objects:
//      - File system device object we attach to
//      - Mounted volume device objects we attach to
//

typedef struct _SFILTER_DEVICE_EXTENSION {

    //
    //  Pointer to the file system device object we are attached to
    //

    PDEVICE_OBJECT AttachedToDeviceObject;

    //
    //  Pointer to the real (disk) device object that is associated with
    //  the file system device object we are attached to
    //

    PDEVICE_OBJECT StorageStackDeviceObject;

    //
    //  Name for this device.  If attached to a Volume Device Object it is the
    //  name of the physical disk drive.  If attached to a Control Device
    //  Object it is the name of the Control Device Object.
    //

    UNICODE_STRING DeviceName;

    //
    //  Buffer used to hold the above unicode strings
    //

    WCHAR DeviceNameBuffer[MAX_DEVNAME_LENGTH];

} SFILTER_DEVICE_EXTENSION, *PSFILTER_DEVICE_EXTENSION;

extern PDEVICE_OBJECT gSFilterControlDeviceObject;
extern PDRIVER_OBJECT gSFilterDriverObject;

#endif