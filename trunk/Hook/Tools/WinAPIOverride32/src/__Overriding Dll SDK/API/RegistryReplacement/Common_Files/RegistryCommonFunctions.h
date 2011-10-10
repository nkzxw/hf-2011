/*
Copyright (C) 2010 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <windows.h>
#include "tstring.h"
#include <vector>
#include "AnsiUnicodeConvert.h"
#include "XmlLite.h"
#include "TextFile.h"

#if (defined(UNICODE)||defined(_UNICODE))
    #define STRINGIFY(x) L#x
#else
    #define STRINGIFY(x) #x
#endif
#define TOSTRING(x) STRINGIFY(x)
// usage "#define MYSTR TOSTRING(123)" NOT "#define MYSTR (TOSTRING(123))"

#define RegistryReplacement_CONFIG_FILE_MAJOR_VERSION 0x1
#define RegistryReplacement_CONFIG_FILE_MINOR_VERSION 0x0

#define RegistryReplacement_STR_CONFIG_FILE_MAJOR_VERSION TOSTRING(RegistryReplacement_CONFIG_FILE_MAJOR_VERSION)
#define RegistryReplacement_STR_CONFIG_FILE_MINOR_VERSION TOSTRING(RegistryReplacement_CONFIG_FILE_MINOR_VERSION)


#define RegistryReplacement_LOCAL_MACHINE_REGISTRY _T("_LocalHost_")

#ifndef HKEY_CURRENT_USER_LOCAL_SETTINGS
    #define HKEY_CURRENT_USER_LOCAL_SETTINGS ((HKEY)0x80000007)
#endif
namespace EmulatedRegistry
{

BOOL DoesFileExists(TCHAR* const FullPath);
BOOL IsBaseKey(HKEY const hKey);
TCHAR* BaseKeyToString(HKEY const hKey);
HKEY StringToBaseKey(TCHAR* const BaseKeyName);

}