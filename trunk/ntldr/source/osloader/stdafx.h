//********************************************************************
//	created:	10:8:2008   17:25
//	file:		stdafx.h
//	author:		tiamo
//	purpose:	stdafx
//********************************************************************

#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _X86_

#pragma warning(disable:4101)
#pragma warning(disable:4273)
#pragma comment(lib,"libcnt.lib")

#define _EFI_SUPPORT_									0
#define _NTFS_SUPPORT_									1
#define _SCSI_SUPPORT_									0
#define _FAT_SUPPORT_									0
#define _UDFS_SUPPORT_									0
#define _CDFS_SUPPORT_									0
#define _ETFS_SUPPORT_									0
#define _XIP_SUPPORT_									0
#define _RAMDISK_SUPPORT_								0
#define _NET_SUPPORT_									0
#define _HIBERNATE_SUPPORT_								0
#define _TCP_CHECKSUM_SUPPORT_							0
#define _XPRESS_SUPPORT_								0
#define _GRAPHICS_MODE_SUPPORT_							0

#if _HIBERNATE_SUPPORT_
	#undef _TCP_CHECKSUM_SUPPORT_
	#define _TCP_CHECKSUM_SUPPORT_						1

	#undef _XPRESS_SUPPORT_
	#define _XPRESS_SUPPORT_							1
#endif

#define MAJOR_VERSION_NT5								5
#define MINOR_VERSION_2000								0

#define MINOR_VERSION_XP								1
#define BUILD_NUMBER_XP									2600
#define SP2_NUMBER_XP									2180
#define WINDOWS_XP_WITH_DEP_FULL_VERSION				0x000500010a2807e0ULL

#define MINOR_VERSION_2003								2
#define BUILD_NUMBER_2003								3790
#define SP2_NUMBER_2003									3959
#define WINDOWS_2003_SP1_FULL_VERSION					0x000500020ece0467ULL

#define MAJOR_VERSION									MAJOR_VERSION_NT5
#define MINOR_VERSION									MINOR_VERSION_2003
#define BUILD_NUMBER									BUILD_NUMBER_2003
#define SERVICE_PACK_NUMBER								SP2_NUMBER_2003

extern "C"
{
	#include <ntddk.h>
}

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

//
// ARC status code
//
typedef ULONG											ARC_STATUS;

//
// try leave
//
#define try_leave(S)									{S;__leave;}

//
// add to pointer
//
#define Add2Ptr(P,L,T)									((T)((ULONG_PTR)(P) + (L)))

//
// ptr offset
//
#define PtrOffset(End,Start,T)							((T)((ULONG_PTR)(End) - (ULONG_PTR)(Start)))

//
// round up size
//
#define ROUND_UP(Input,Size)							(((ULONG_PTR)(Input) + (Size) - 1) & ~((Size) - 1))

//
// round up to sector
//
#define ROUND_TO_SECTORS(Size)							(((ULONG_PTR)(Size) + SECTOR_SIZE - 1) & ~(SECTOR_SIZE - 1))

//
// align buffer to data cache fill line
//
#define ALIGN_BUFFER(Buffer)							(PVOID)((((ULONG_PTR)(Buffer) + BlDcacheFillSize - 1)) & (~(BlDcacheFillSize - 1)))

#define CONVERT_TO_STRING1(a)							#a
#define CONVERT_TO_STRING2(a)							CONVERT_TO_STRING1(a)
#define PRAGMA_MSG(x)									message(__FILE__ ## "(" ## CONVERT_TO_STRING2(__LINE__) ##"):  <" __FUNCTION__ "> " ## x)

extern "C"
{
	#include "sumodule.h"
	#include "msg.h"
	#include "i386.h"
	#include "pcibus.h"
	#include "busdata.h"
	#include "runtime.h"
	#include "memory.h"
	#include "arcemul.h"
	#include "debug.h"
	#include "kdapi.h"
	#include "terminal.h"
	#include "acpi.h"
	#include "portio.h"
	#include "comport.h"
	#include "bootfont.h"
	#include "display.h"
	#include "blconfig.h"
	#include "peloader.h"
	#include "blres.h"
	#include "bllog.h"
	#include "arcdisk.h"
	#include "dskcache.h"
	#include "bootstat.h"
	#include "advboot.h"
	#include "loadprog.h"
	#include "registry.h"
	#include "cmboot.h"
	#include "blload.h"
	#include "selkrnl.h"
	#include "cfgmenu.h"
	#include "osloader.h"
	#include "netboot.h"

	#if _GRAPHICS_MODE_SUPPORT_
		#include "graphics.h"
	#endif

	#if _TCP_CHECKSUM_SUPPORT_
		#include "tcpxsum.h"
	#endif

	#if _XPRESS_SUPPORT_
		#include "xpress.h"
	#endif

	#if _HIBERNATE_SUPPORT_
		#include "hiber.h"
	#endif

	#if _RAMDISK_SUPPORT_
		#include "ramdisk.h"
	#endif

	#if _XIP_SUPPORT_
		#include "xip.h"
	#endif

	#if _NTFS_SUPPORT_
		#include "ntfsboot.h"
	#endif

	#if _FAT_SUPPORT_
		#include "fatboot.h"
	#endif

	#if _SCSI_SUPPORT_
		#include "scsiboot.h"
	#endif

	#if _CDFS_SUPPORT_
		#include "cdfsboot.h"
	#endif

	#if _UDFS_SUPPORT_
		#include "udfsboot.h"
	#endif

	#include "blio.h"
	#include "biosdrv.h"
}