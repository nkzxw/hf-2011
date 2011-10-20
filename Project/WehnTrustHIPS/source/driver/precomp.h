/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_DRIVER_PRECOMP_H
#define _WEHNTRUST_DRIVER_PRECOMP_H

#define DRIVER                  "baserand: "
#define DebugPrint(x)           \
				KdPrint((DRIVER));  \
				KdPrint(x);         \
				KdPrint(("\n"))

#define ALLOC_TAG               'wENU'

#define InsertEntryList(Before, After, Entry)            \
	((PLIST_ENTRY)Before)->Flink = ((PLIST_ENTRY)Entry);  \
	((PLIST_ENTRY)After)->Blink  = ((PLIST_ENTRY)Entry);  \
	((PLIST_ENTRY)Entry)->Blink  = ((PLIST_ENTRY)Before); \
	((PLIST_ENTRY)Entry)->Flink  = ((PLIST_ENTRY)After)  

#define Round16Page(Value)                       \
	(((Value) & 0xffff))                          \
		? (Value) + (0x10000 - ((Value) & 0xffff)) \
		: (Value)

#include <ntddk.h>
#include <ntimage.h>
#include <ntintsafe.h>

#include "../common/common.h"
#include "defines.h"
#include "layer.h"
#include "disasm.h"
#include "types.h"
#include "compat.h"
#include "custom.h"
#include "reg.h"
#include "rtle.h"
#include "rng.h"
#include "executive.h"
#include "ioctl.h"
#include "ldr.h"
#include "hooks.h"
#include "images.h"
#include "mapping.h"
#include "nrer.h"
#include "process.h"
#include "exemption.h"

#endif
