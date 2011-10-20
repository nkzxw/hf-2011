#ifndef WEHNTRUST_COMMON_COMMON_H
#define WEHNTRUST_COMMON_COMMON_H

#define _WIN32_DCOM 
#ifndef _WIN32_IE
#define _WIN32_IE 0x0500
#endif
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <commctrl.h>
#include <winioctl.h>
#include <tchar.h>
#include <objbase.h>

#define  USER_MODE
#include "../../common/common.h"
#include "libdasm.h"
#include "sha1.h"

#include "Defines.h"
#include "events.h"
#include "WehnServ.h"
#include "WehnServEventSubscriber.h"
#ifdef __cplusplus
#include "Config.h"
#include "DriverClient.h"
#include "Logger.h"
#include "Native.h"

#ifdef DBG
#define Log Logger::Log
#else
#define Log 
#endif
#endif

#endif
