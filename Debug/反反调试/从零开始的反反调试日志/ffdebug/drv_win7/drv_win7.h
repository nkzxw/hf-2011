///////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2010 - <company name here>
///
/// Original filename: drv_win7.h
/// Project          : drv_win7
/// Date of creation : <see drv_win7.cpp>
/// Author(s)        : <see drv_win7.cpp>
///
/// Purpose          : <see drv_win7.cpp>
///
/// Revisions:         <see drv_win7.cpp>
///
///////////////////////////////////////////////////////////////////////////////

// $Id$

#ifndef __DRVWIN7_H_VERSION__
#define __DRVWIN7_H_VERSION__ 100

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif


#include "drvcommon.h"
#include "drvversion.h"

#define DEVICE_NAME			"\\Device\\drv_win7"
#define SYMLINK_NAME		"\\DosDevices\\drv_win7"
PRESET_UNICODE_STRING(usDeviceName, DEVICE_NAME);
PRESET_UNICODE_STRING(usSymlinkName, SYMLINK_NAME);

#ifndef FILE_DEVICE_DRVWIN7
#define FILE_DEVICE_DRVWIN7 0x8000
#endif

// Values defined for "Method"
// METHOD_BUFFERED
// METHOD_IN_DIRECT
// METHOD_OUT_DIRECT
// METHOD_NEITHER
// 
// Values defined for "Access"
// FILE_ANY_ACCESS
// FILE_READ_ACCESS
// FILE_WRITE_ACCESS

const ULONG IOCTL_DRVWIN7_OPERATION = CTL_CODE(FILE_DEVICE_DRVWIN7, 0x01, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA);

#endif // __DRVWIN7_H_VERSION__
