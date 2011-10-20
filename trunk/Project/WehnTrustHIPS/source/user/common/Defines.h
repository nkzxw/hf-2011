#ifndef _WEHNTRUST_COMMON_DEFINES_H
#define _WEHNTRUST_COMMON_DEFINES_H

////
//
// Defines
//
////

//
// Registry location information
//

//
// The system-wide location of WehnTrust settings.
//
#define WEHNTRUST_ROOT_KEY         HKEY_LOCAL_MACHINE
#define WEHNTRUST_BASE_KEY_ROOT    TEXT("System\\CurrentControlSet\\Services\\baserand")

//
// The system-wide location of WehnTrust configuration settings.
//
#define WEHNTRUST_SYSCONF_ROOT_KEY WEHNTRUST_ROOT_KEY
#define WEHNTRUST_SYSCONF_BASE_KEY WEHNTRUST_BASE_KEY_ROOT TEXT("\\Config")

//
// The user-specific WehnTrust configuration settings.
//
#define WEHNTRUST_USER_ROOT_KEY    HKEY_CURRENT_USER
#define WEHNTRUST_USER_BASE_KEY    TEXT("Software\\Wehnus\\WehnTrust")

#endif
