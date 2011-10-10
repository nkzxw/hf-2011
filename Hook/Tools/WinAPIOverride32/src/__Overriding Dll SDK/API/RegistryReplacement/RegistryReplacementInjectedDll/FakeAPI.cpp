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

// sorry for this quick coding file :(

#include <windows.h>
#include "../../../_Common_Files/GenericFakeAPI.h"
#include "../Common_Files/EmulatedRegistry.h"
#include "HardwareException.h"
#include "RegReplaceOptions.h"

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

#ifdef _DEBUG
#define M_SECURE_TRY try{CExceptionHardware::RegisterTry();
#define M_SECURE_CATCH }catch( CExceptionHardware e ){e;::DebugBreak();}catch (...){::DebugBreak();}

//#define M_SECURE_TRY
//#define M_SECURE_CATCH
#else
#define M_SECURE_TRY try{CExceptionHardware::RegisterTry();
#define M_SECURE_CATCH }catch( CExceptionHardware e ){}catch (...){}
#endif



using namespace EmulatedRegistry;

#ifndef __max 
        #define __max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#define LOADED_WITH_WINAPIOVERRIDE_FOR_DEBUG 0 // 1 to be load by winapioverride, 0 to be used by launcher
#define FORCE_SPY_MODE_FOR_DEBUG 0 // 1 to force gRegistry_bSpyMode at 1 at startup (used for winapioverride debug purpose)

#if (LOADED_WITH_WINAPIOVERRIDE_FOR_DEBUG ==1)
    // if we are spying registry with monitoring file, on seven as we only fake KernelBase.dll,
    // all monitored API of advapi32.dll that are not faked are called by apioverride.dll. So if apioverride.dll is filtered, calls won't be faked
    #pragma message (" TO SPY REGISTRY CALLS WITH MONITORING FILE, ENABLE ApiOverride.dll SPYING IN MODULES FILTERS\r\n")
#endif

HINSTANCE gDllhInstance =NULL;
CEmulatedRegistry* gpEmulatedRegistry = NULL;
#define CONFIG_FILE_EXTENSION _T(".RegReplace.xml")
HMODULE ghModuleKernel = 0;
HMODULE ghModuleAdvapi = 0;
#define LAUNCHER_EVENT_INJECTION_FINISHEDW L"Global\\InjectFinished"
#define LAUNCHER_NAMEA "Launcher.exe"
#define LAUNCHER_NAMEW L"Launcher.exe"



REGISTRY_REPLACEMENT_OPTIONS gLauncherOptions={0};
CHAR gLauncherFullPathA[MAX_PATH];    
WCHAR gLauncherFullPathW[MAX_PATH];

// You just need to edit this file to add new fake api 
// WARNING YOUR FAKE API MUST HAVE THE SAME PARAMETERS AND CALLING CONVENTION AS THE REAL ONE,
//                  ELSE YOU WILL GET STACK ERRORS

///////////////////////////////////////////////////////////////////////////////
// fake API prototype MUST HAVE THE SAME PARAMETERS 
// for the same calling convention see MSDN : 
// "Using a Microsoft modifier such as __cdecl on a data declaration is an outdated practice"
///////////////////////////////////////////////////////////////////////////////


// TODO : it will be better to catch CreateProcessInternal
BOOL WINAPI mCreateProcessA(LPCSTR lpApplicationName,
                            CHAR* lpCommandLine,
                            LPSECURITY_ATTRIBUTES lpProcessAttributes,
                            LPSECURITY_ATTRIBUTES lpThreadAttributes,
                            BOOL bInheritHandles,
                            DWORD dwCreationFlags,
                            LPVOID lpEnvironment,
                            LPCSTR lpCurrentDirectory,
                            LPSTARTUPINFOA lpStartupInfo,
                            LPPROCESS_INFORMATION lpProcessInformation);

BOOL WINAPI mCreateProcessW(LPCWSTR lpApplicationName,
                            WCHAR* lpCommandLine,
                            LPSECURITY_ATTRIBUTES lpProcessAttributes,
                            LPSECURITY_ATTRIBUTES lpThreadAttributes,
                            BOOL bInheritHandles,
                            DWORD dwCreationFlags,
                            LPVOID lpEnvironment,
                            LPCWSTR lpCurrentDirectory,
                            LPSTARTUPINFOW lpStartupInfo,
                            LPPROCESS_INFORMATION lpProcessInformation);


LONG WINAPI mAdvapiRegCloseKey(HKEY hKey);
LONG WINAPI mAdvapiRegConnectRegistryA(LPCSTR lpMachineName,HKEY hKey,PHKEY phkResult);
LONG WINAPI mAdvapiRegConnectRegistryW(LPCWSTR lpMachineName,HKEY hKey,PHKEY phkResult);
LONG WINAPI mAdvapiRegCopyTreeA(HKEY hKeySrc,LPCSTR lpSubKey,HKEY hKeyDest);
LONG WINAPI mAdvapiRegCopyTreeW(HKEY hKeySrc,LPCWSTR lpSubKey,HKEY hKeyDest);
LONG WINAPI mAdvapiRegCreateKeyA(HKEY hKey,LPCSTR lpSubKey,PHKEY phkResult);
LONG WINAPI mAdvapiRegCreateKeyW(HKEY hKey,LPCWSTR lpSubKey,PHKEY phkResult);
LONG WINAPI mAdvapiRegCreateKeyExA(HKEY hKey,LPCSTR lpSubKey,DWORD Reserved, LPSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition);
LONG WINAPI mAdvapiRegCreateKeyExW(HKEY hKey,LPCWSTR lpSubKey,DWORD Reserved, LPWSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition);
LONG WINAPI mAdvapiRegCreateKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,DWORD Reserved,LPSTR lpClass,DWORD dwOptions,REGSAM samDesired,const LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,HANDLE hTransaction,PVOID pExtendedParemeter);
LONG WINAPI mAdvapiRegCreateKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,DWORD Reserved,LPWSTR lpClass,DWORD dwOptions,REGSAM samDesired,const LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,HANDLE hTransaction,PVOID pExtendedParemeter);
LONG WINAPI mAdvapiRegDeleteKeyA(HKEY hKey,LPCSTR lpSubKey);
LONG WINAPI mAdvapiRegDeleteKeyW(HKEY hKey,LPCWSTR lpSubKey);
LONG WINAPI mAdvapiRegDeleteKeyExA(HKEY hKey,LPCSTR lpSubKey,REGSAM samDesired,DWORD Reserved);
LONG WINAPI mAdvapiRegDeleteKeyExW(HKEY hKey,LPCWSTR lpSubKey,REGSAM samDesired,DWORD Reserved);
LONG WINAPI mAdvapiRegDeleteKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,REGSAM samDesired,DWORD Reserved,HANDLE hTransaction,PVOID pExtendedParameter);
LONG WINAPI mAdvapiRegDeleteKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,REGSAM samDesired,DWORD Reserved,HANDLE hTransaction,PVOID pExtendedParameter);
LONG WINAPI mAdvapiRegDeleteKeyValueA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpValueName);
LONG WINAPI mAdvapiRegDeleteKeyValueW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpValueName);
LONG WINAPI mAdvapiRegDeleteTreeA(HKEY hKey,LPCSTR lpSubKey);
LONG WINAPI mAdvapiRegDeleteTreeW(HKEY hKey,LPCWSTR lpSubKey);
LONG WINAPI mAdvapiRegDeleteValueA(HKEY hKey,LPCSTR lpValueName);
LONG WINAPI mAdvapiRegDeleteValueW(HKEY hKey,LPCWSTR lpValueName);
// LONG WINAPI RegDisablePredefinedCache();// no need to be overrided
// LONG WINAPI RegDisablePredefinedCacheEx(void); // no need to be overrided
LONG WINAPI mAdvapiRegDisableReflectionKey(HKEY hBase);
LONG WINAPI mAdvapiRegEnableReflectionKey(HKEY hBase);
LONG WINAPI mAdvapiRegEnumKeyA(HKEY hKey,DWORD dwIndex,LPSTR lpName,DWORD cchName);
LONG WINAPI mAdvapiRegEnumKeyW(HKEY hKey,DWORD dwIndex,LPWSTR lpName,DWORD cchName);
LONG WINAPI mAdvapiRegEnumKeyExA(HKEY hKey,DWORD dwIndex,LPSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime);
LONG WINAPI mAdvapiRegEnumKeyExW(HKEY hKey,DWORD dwIndex,LPWSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPWSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime);
LONG WINAPI mAdvapiRegEnumValueA(HKEY hKey,DWORD dwIndex,LPSTR lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);
LONG WINAPI mAdvapiRegEnumValueW(HKEY hKey,DWORD dwIndex,LPWSTR lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);
LONG WINAPI mAdvapiRegFlushKey(HKEY hKey);
LONG WINAPI mAdvapiRegGetKeySecurity(HKEY hKey,SECURITY_INFORMATION SecurityInformation,PSECURITY_DESCRIPTOR pSecurityDescriptor,LPDWORD lpcbSecurityDescriptor);
LONG WINAPI mAdvapiRegGetValueA(HKEY hkey,LPCSTR lpSubKey,LPCSTR lpValue,DWORD dwFlags,LPDWORD pdwType,PVOID pvData,LPDWORD pcbData);
LONG WINAPI mAdvapiRegGetValueW(HKEY hkey,LPCWSTR lpSubKey,LPCWSTR lpValue,DWORD dwFlags,LPDWORD pdwType,PVOID pvData,LPDWORD pcbData);
LONG WINAPI mAdvapiRegLoadAppKeyA(LPCSTR lpFile,PHKEY phkResult,REGSAM samDesired,DWORD dwOptions,DWORD Reserved);
LONG WINAPI mAdvapiRegLoadAppKeyW(LPCWSTR lpFile,PHKEY phkResult,REGSAM samDesired,DWORD dwOptions,DWORD Reserved);
LONG WINAPI mAdvapiRegLoadKeyA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpFile);
LONG WINAPI mAdvapiRegLoadKeyW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpFile);
LONG WINAPI mAdvapiRegLoadMUIStringA(HKEY hKey,LPCSTR pszValue,LPSTR pszOutBuf,DWORD cbOutBuf,LPDWORD pcbData,DWORD Flags,LPCSTR pszDirectory);
LONG WINAPI mAdvapiRegLoadMUIStringW(HKEY hKey,LPCWSTR pszValue,LPWSTR pszOutBuf,DWORD cbOutBuf,LPDWORD pcbData,DWORD Flags,LPCWSTR pszDirectory);
LONG WINAPI mAdvapiRegNotifyChangeKeyValue(HKEY hKey,BOOL bWatchSubtree,DWORD dwNotifyFilter,HANDLE hEvent,BOOL fAsynchronous);
LONG WINAPI mAdvapiRegOpenCurrentUser(REGSAM samDesired,PHKEY phkResult);
LONG WINAPI mAdvapiRegOpenKeyA(HKEY hKey,LPCSTR lpSubKey,PHKEY phkResult);
LONG WINAPI mAdvapiRegOpenKeyW(HKEY hKey,LPCWSTR lpSubKey,PHKEY phkResult);
LONG WINAPI mAdvapiRegOpenKeyExA(HKEY hKey,LPCSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult);
LONG WINAPI mAdvapiRegOpenKeyExW(HKEY hKey,LPCWSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult);
LONG WINAPI mAdvapiRegOpenKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,HANDLE hTransaction,PVOID pExtendedParameter);
LONG WINAPI mAdvapiRegOpenKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,HANDLE hTransaction,PVOID pExtendedParameter);
LONG WINAPI mAdvapiRegOpenUserClassesRoot(HANDLE hToken,DWORD dwOptions,REGSAM samDesired,PHKEY phkResult);
LONG WINAPI mAdvapiRegOverridePredefKey(HKEY hKey,HKEY hNewHKey);
LONG WINAPI mAdvapiRegQueryInfoKeyA(HKEY hKey,LPSTR lpClass,LPDWORD lpcClass,LPDWORD lpReserved,LPDWORD lpcSubKeys,LPDWORD lpcMaxSubKeyLen,LPDWORD lpcMaxClassLen,LPDWORD lpcValues,LPDWORD lpcMaxValueNameLen,LPDWORD lpcMaxValueLen,LPDWORD lpcbSecurityDescriptor,PFILETIME lpftLastWriteTime);
LONG WINAPI mAdvapiRegQueryInfoKeyW(HKEY hKey,LPWSTR lpClass,LPDWORD lpcClass,LPDWORD lpReserved,LPDWORD lpcSubKeys,LPDWORD lpcMaxSubKeyLen,LPDWORD lpcMaxClassLen,LPDWORD lpcValues,LPDWORD lpcMaxValueNameLen,LPDWORD lpcMaxValueLen,LPDWORD lpcbSecurityDescriptor,PFILETIME lpftLastWriteTime);
LONG WINAPI mAdvapiRegQueryMultipleValuesA(HKEY hKey,PVALENTA val_list,DWORD num_vals,LPSTR lpValueBuf,LPDWORD ldwTotsize);
LONG WINAPI mAdvapiRegQueryMultipleValuesW(HKEY hKey,PVALENTW val_list,DWORD num_vals,LPWSTR lpValueBuf,LPDWORD ldwTotsize);
LONG WINAPI mAdvapiRegQueryReflectionKey(HKEY hBase,BOOL *bIsReflectionDisabled);
LONG WINAPI mAdvapiRegQueryValueA(HKEY hKey,LPCSTR lpSubKey,LPSTR lpValue,PLONG lpcbValue);
LONG WINAPI mAdvapiRegQueryValueW(HKEY hKey,LPCWSTR lpSubKey,LPWSTR lpValue,PLONG lpcbValue);
LONG WINAPI mAdvapiRegQueryValueExA(HKEY hKey,LPCSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);
LONG WINAPI mAdvapiRegQueryValueExW(HKEY hKey,LPCWSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);
LONG WINAPI mAdvapiRegReplaceKeyA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpNewFile,LPCSTR lpOldFile);
LONG WINAPI mAdvapiRegReplaceKeyW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpNewFile,LPCWSTR lpOldFile);
LONG WINAPI mAdvapiRegRestoreKeyA(HKEY hKey,LPCSTR lpFile,DWORD dwFlags);
LONG WINAPI mAdvapiRegRestoreKeyW(HKEY hKey,LPCWSTR lpFile,DWORD dwFlags);
LONG WINAPI mAdvapiRegSaveKeyA(HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes);
LONG WINAPI mAdvapiRegSaveKeyW(HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes);
LONG WINAPI mAdvapiRegSaveKeyExA(HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags);
LONG WINAPI mAdvapiRegSaveKeyExW(HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags);
LONG WINAPI mAdvapiRegSetKeySecurity(HKEY hKey,SECURITY_INFORMATION SecurityInformation,PSECURITY_DESCRIPTOR pSecurityDescriptor);
LONG WINAPI mAdvapiRegSetKeyValueA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpValueName,DWORD dwType,LPCVOID lpData,DWORD cbData);
LONG WINAPI mAdvapiRegSetKeyValueW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpValueName,DWORD dwType,LPCVOID lpData,DWORD cbData);
LONG WINAPI mAdvapiRegSetValueA(HKEY hKey,LPCSTR lpSubKey,DWORD dwType,LPCSTR lpData,DWORD cbData);
LONG WINAPI mAdvapiRegSetValueW(HKEY hKey,LPCWSTR lpSubKey,DWORD dwType,LPCWSTR lpData,DWORD cbData);
LONG WINAPI mAdvapiRegSetValueExA(HKEY hKey,LPCSTR lpValueName,DWORD Reserved,DWORD dwType,const BYTE* lpData,DWORD cbData);
LONG WINAPI mAdvapiRegSetValueExW(HKEY hKey,LPCWSTR lpValueName,DWORD Reserved,DWORD dwType,const BYTE* lpData,DWORD cbData);
LONG WINAPI mAdvapiRegUnLoadKeyA(HKEY hKey,LPCSTR lpSubKey);
LONG WINAPI mAdvapiRegUnLoadKeyW(HKEY hKey,LPCWSTR lpSubKey);

LONG WINAPI  mKernelRegCloseKey(HKEY hKey);
LONG WINAPI  mKernelRegConnectRegistryA(LPCSTR lpMachineName,HKEY hKey,PHKEY phkResult);
LONG WINAPI  mKernelRegConnectRegistryW(LPCWSTR lpMachineName,HKEY hKey,PHKEY phkResult);
LONG WINAPI  mKernelRegCopyTreeA(HKEY hKeySrc,LPCSTR lpSubKey,HKEY hKeyDest);
LONG WINAPI  mKernelRegCopyTreeW(HKEY hKeySrc,LPCWSTR lpSubKey,HKEY hKeyDest);
LONG WINAPI  mKernelRegCreateKeyA(HKEY hKey,LPCSTR lpSubKey,PHKEY phkResult);
LONG WINAPI  mKernelRegCreateKeyW(HKEY hKey,LPCWSTR lpSubKey,PHKEY phkResult);
LONG WINAPI  mKernelRegCreateKeyExA(HKEY hKey,LPCSTR lpSubKey,DWORD Reserved, LPSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition);
LONG WINAPI  mKernelRegCreateKeyExW(HKEY hKey,LPCWSTR lpSubKey,DWORD Reserved, LPWSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition);
LONG WINAPI  mKernelRegCreateKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,DWORD Reserved,LPSTR lpClass,DWORD dwOptions,REGSAM samDesired,const LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,HANDLE hTransaction,PVOID pExtendedParemeter);
LONG WINAPI  mKernelRegCreateKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,DWORD Reserved,LPWSTR lpClass,DWORD dwOptions,REGSAM samDesired,const LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,HANDLE hTransaction,PVOID pExtendedParemeter);
LONG WINAPI  mKernelRegDeleteKeyA(HKEY hKey,LPCSTR lpSubKey);
LONG WINAPI  mKernelRegDeleteKeyW(HKEY hKey,LPCWSTR lpSubKey);
LONG WINAPI  mKernelRegDeleteKeyExA(HKEY hKey,LPCSTR lpSubKey,REGSAM samDesired,DWORD Reserved);
LONG WINAPI  mKernelRegDeleteKeyExW(HKEY hKey,LPCWSTR lpSubKey,REGSAM samDesired,DWORD Reserved);
LONG WINAPI  mKernelRegDeleteKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,REGSAM samDesired,DWORD Reserved,HANDLE hTransaction,PVOID pExtendedParameter);
LONG WINAPI  mKernelRegDeleteKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,REGSAM samDesired,DWORD Reserved,HANDLE hTransaction,PVOID pExtendedParameter);
LONG WINAPI  mKernelRegDeleteKeyValueA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpValueName);
LONG WINAPI  mKernelRegDeleteKeyValueW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpValueName);
LONG WINAPI  mKernelRegDeleteTreeA(HKEY hKey,LPCSTR lpSubKey);
LONG WINAPI  mKernelRegDeleteTreeW(HKEY hKey,LPCWSTR lpSubKey);
LONG WINAPI  mKernelRegDeleteValueA(HKEY hKey,LPCSTR lpValueName);
LONG WINAPI  mKernelRegDeleteValueW(HKEY hKey,LPCWSTR lpValueName);
// LONG WINAPI RegDisablePredefinedCache();// no need to be overrided
// LONG WINAPI RegDisablePredefinedCacheEx(void); // no need to be overrided
LONG WINAPI  mKernelRegDisableReflectionKey(HKEY hBase);
LONG WINAPI  mKernelRegEnableReflectionKey(HKEY hBase);
LONG WINAPI  mKernelRegEnumKeyA(HKEY hKey,DWORD dwIndex,LPSTR lpName,DWORD cchName);
LONG WINAPI  mKernelRegEnumKeyW(HKEY hKey,DWORD dwIndex,LPWSTR lpName,DWORD cchName);
LONG WINAPI  mKernelRegEnumKeyExA(HKEY hKey,DWORD dwIndex,LPSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime);
LONG WINAPI  mKernelRegEnumKeyExW(HKEY hKey,DWORD dwIndex,LPWSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPWSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime);
LONG WINAPI  mKernelRegEnumValueA(HKEY hKey,DWORD dwIndex,LPSTR lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);
LONG WINAPI  mKernelRegEnumValueW(HKEY hKey,DWORD dwIndex,LPWSTR lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);
LONG WINAPI  mKernelRegFlushKey(HKEY hKey);
LONG WINAPI  mKernelRegGetKeySecurity(HKEY hKey,SECURITY_INFORMATION SecurityInformation,PSECURITY_DESCRIPTOR pSecurityDescriptor,LPDWORD lpcbSecurityDescriptor);
LONG WINAPI  mKernelRegGetValueA(HKEY hkey,LPCSTR lpSubKey,LPCSTR lpValue,DWORD dwFlags,LPDWORD pdwType,PVOID pvData,LPDWORD pcbData);
LONG WINAPI  mKernelRegGetValueW(HKEY hkey,LPCWSTR lpSubKey,LPCWSTR lpValue,DWORD dwFlags,LPDWORD pdwType,PVOID pvData,LPDWORD pcbData);
LONG WINAPI  mKernelRegLoadAppKeyA(LPCSTR lpFile,PHKEY phkResult,REGSAM samDesired,DWORD dwOptions,DWORD Reserved);
LONG WINAPI  mKernelRegLoadAppKeyW(LPCWSTR lpFile,PHKEY phkResult,REGSAM samDesired,DWORD dwOptions,DWORD Reserved);
LONG WINAPI  mKernelRegLoadKeyA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpFile);
LONG WINAPI  mKernelRegLoadKeyW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpFile);
LONG WINAPI  mKernelRegLoadMUIStringA(HKEY hKey,LPCSTR pszValue,LPSTR pszOutBuf,DWORD cbOutBuf,LPDWORD pcbData,DWORD Flags,LPCSTR pszDirectory);
LONG WINAPI  mKernelRegLoadMUIStringW(HKEY hKey,LPCWSTR pszValue,LPWSTR pszOutBuf,DWORD cbOutBuf,LPDWORD pcbData,DWORD Flags,LPCWSTR pszDirectory);
LONG WINAPI  mKernelRegNotifyChangeKeyValue(HKEY hKey,BOOL bWatchSubtree,DWORD dwNotifyFilter,HANDLE hEvent,BOOL fAsynchronous);
LONG WINAPI  mKernelRegOpenCurrentUser(REGSAM samDesired,PHKEY phkResult);
LONG WINAPI  mKernelRegOpenKeyA(HKEY hKey,LPCSTR lpSubKey,PHKEY phkResult);
LONG WINAPI  mKernelRegOpenKeyW(HKEY hKey,LPCWSTR lpSubKey,PHKEY phkResult);
LONG WINAPI  mKernelRegOpenKeyExA(HKEY hKey,LPCSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult);
LONG WINAPI  mKernelRegOpenKeyExW(HKEY hKey,LPCWSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult);
LONG WINAPI  mKernelRegOpenKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,HANDLE hTransaction,PVOID pExtendedParameter);
LONG WINAPI  mKernelRegOpenKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,HANDLE hTransaction,PVOID pExtendedParameter);
LONG WINAPI  mKernelRegOpenUserClassesRoot(HANDLE hToken,DWORD dwOptions,REGSAM samDesired,PHKEY phkResult);
LONG WINAPI  mKernelRegOverridePredefKey(HKEY hKey,HKEY hNewHKey);
LONG WINAPI  mKernelRegQueryInfoKeyA(HKEY hKey,LPSTR lpClass,LPDWORD lpcClass,LPDWORD lpReserved,LPDWORD lpcSubKeys,LPDWORD lpcMaxSubKeyLen,LPDWORD lpcMaxClassLen,LPDWORD lpcValues,LPDWORD lpcMaxValueNameLen,LPDWORD lpcMaxValueLen,LPDWORD lpcbSecurityDescriptor,PFILETIME lpftLastWriteTime);
LONG WINAPI  mKernelRegQueryInfoKeyW(HKEY hKey,LPWSTR lpClass,LPDWORD lpcClass,LPDWORD lpReserved,LPDWORD lpcSubKeys,LPDWORD lpcMaxSubKeyLen,LPDWORD lpcMaxClassLen,LPDWORD lpcValues,LPDWORD lpcMaxValueNameLen,LPDWORD lpcMaxValueLen,LPDWORD lpcbSecurityDescriptor,PFILETIME lpftLastWriteTime);
LONG WINAPI  mKernelRegQueryMultipleValuesA(HKEY hKey,PVALENTA val_list,DWORD num_vals,LPSTR lpValueBuf,LPDWORD ldwTotsize);
LONG WINAPI  mKernelRegQueryMultipleValuesW(HKEY hKey,PVALENTW val_list,DWORD num_vals,LPWSTR lpValueBuf,LPDWORD ldwTotsize);
LONG WINAPI  mKernelRegQueryReflectionKey(HKEY hBase,BOOL *bIsReflectionDisabled);
LONG WINAPI  mKernelRegQueryValueA(HKEY hKey,LPCSTR lpSubKey,LPSTR lpValue,PLONG lpcbValue);
LONG WINAPI  mKernelRegQueryValueW(HKEY hKey,LPCWSTR lpSubKey,LPWSTR lpValue,PLONG lpcbValue);
LONG WINAPI  mKernelRegQueryValueExA(HKEY hKey,LPCSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);
LONG WINAPI  mKernelRegQueryValueExW(HKEY hKey,LPCWSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);
LONG WINAPI  mKernelRegReplaceKeyA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpNewFile,LPCSTR lpOldFile);
LONG WINAPI  mKernelRegReplaceKeyW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpNewFile,LPCWSTR lpOldFile);
LONG WINAPI  mKernelRegRestoreKeyA(HKEY hKey,LPCSTR lpFile,DWORD dwFlags);
LONG WINAPI  mKernelRegRestoreKeyW(HKEY hKey,LPCWSTR lpFile,DWORD dwFlags);
LONG WINAPI  mKernelRegSaveKeyA(HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes);
LONG WINAPI  mKernelRegSaveKeyW(HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes);
LONG WINAPI  mKernelRegSaveKeyExA(HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags);
LONG WINAPI  mKernelRegSaveKeyExW(HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags);
LONG WINAPI  mKernelRegSetKeySecurity(HKEY hKey,SECURITY_INFORMATION SecurityInformation,PSECURITY_DESCRIPTOR pSecurityDescriptor);
LONG WINAPI  mKernelRegSetKeyValueA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpValueName,DWORD dwType,LPCVOID lpData,DWORD cbData);
LONG WINAPI  mKernelRegSetKeyValueW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpValueName,DWORD dwType,LPCVOID lpData,DWORD cbData);
LONG WINAPI  mKernelRegSetValueA(HKEY hKey,LPCSTR lpSubKey,DWORD dwType,LPCSTR lpData,DWORD cbData);
LONG WINAPI  mKernelRegSetValueW(HKEY hKey,LPCWSTR lpSubKey,DWORD dwType,LPCWSTR lpData,DWORD cbData);
LONG WINAPI  mKernelRegSetValueExA(HKEY hKey,LPCSTR lpValueName,DWORD Reserved,DWORD dwType,const BYTE* lpData,DWORD cbData);
LONG WINAPI  mKernelRegSetValueExW(HKEY hKey,LPCWSTR lpValueName,DWORD Reserved,DWORD dwType,const BYTE* lpData,DWORD cbData);
LONG WINAPI  mKernelRegUnLoadKeyA(HKEY hKey,LPCSTR lpSubKey);
LONG WINAPI  mKernelRegUnLoadKeyW(HKEY hKey,LPCWSTR lpSubKey);

typedef LONG (WINAPI *pfRegCloseKey)(HKEY hKey);
typedef LONG (WINAPI *pfRegConnectRegistryA)(LPCSTR lpMachineName,HKEY hKey,PHKEY phkResult);
typedef LONG (WINAPI *pfRegConnectRegistryW)(LPCWSTR lpMachineName,HKEY hKey,PHKEY phkResult);
typedef LONG (WINAPI *pfRegCopyTreeA)(HKEY hKeySrc,LPCSTR lpSubKey,HKEY hKeyDest);
typedef LONG (WINAPI *pfRegCopyTreeW)(HKEY hKeySrc,LPCWSTR lpSubKey,HKEY hKeyDest);
typedef LONG (WINAPI *pfRegCreateKeyA)(HKEY hKey,LPCSTR lpSubKey,PHKEY phkResult);
typedef LONG (WINAPI *pfRegCreateKeyW)(HKEY hKey,LPCWSTR lpSubKey,PHKEY phkResult);
typedef LONG (WINAPI *pfRegCreateKeyExA)(HKEY hKey,LPCSTR lpSubKey,DWORD Reserved, LPSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition);
typedef LONG (WINAPI *pfRegCreateKeyExW)(HKEY hKey,LPCWSTR lpSubKey,DWORD Reserved, LPWSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition);
typedef LONG (WINAPI *pfRegCreateKeyTransactedA)(HKEY hKey,LPCSTR lpSubKey,DWORD Reserved,LPSTR lpClass,DWORD dwOptions,REGSAM samDesired,const LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,HANDLE hTransaction,PVOID pExtendedParemeter);
typedef LONG (WINAPI *pfRegCreateKeyTransactedW)(HKEY hKey,LPCWSTR lpSubKey,DWORD Reserved,LPWSTR lpClass,DWORD dwOptions,REGSAM samDesired,const LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,HANDLE hTransaction,PVOID pExtendedParemeter);
typedef LONG (WINAPI *pfRegDeleteKeyA)(HKEY hKey,LPCSTR lpSubKey);
typedef LONG (WINAPI *pfRegDeleteKeyW)(HKEY hKey,LPCWSTR lpSubKey);
typedef LONG (WINAPI *pfRegDeleteKeyExA)(HKEY hKey,LPCSTR lpSubKey,REGSAM samDesired,DWORD Reserved);
typedef LONG (WINAPI *pfRegDeleteKeyExW)(HKEY hKey,LPCWSTR lpSubKey,REGSAM samDesired,DWORD Reserved);
typedef LONG (WINAPI *pfRegDeleteKeyTransactedA)(HKEY hKey,LPCSTR lpSubKey,REGSAM samDesired,DWORD Reserved,HANDLE hTransaction,PVOID pExtendedParameter);
typedef LONG (WINAPI *pfRegDeleteKeyTransactedW)(HKEY hKey,LPCWSTR lpSubKey,REGSAM samDesired,DWORD Reserved,HANDLE hTransaction,PVOID pExtendedParameter);
typedef LONG (WINAPI *pfRegDeleteKeyValueA)(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpValueName);
typedef LONG (WINAPI *pfRegDeleteKeyValueW)(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpValueName);
typedef LONG (WINAPI *pfRegDeleteTreeA)(HKEY hKey,LPCSTR lpSubKey);
typedef LONG (WINAPI *pfRegDeleteTreeW)(HKEY hKey,LPCWSTR lpSubKey);
typedef LONG (WINAPI *pfRegDeleteValueA)(HKEY hKey,LPCSTR lpValueName);
typedef LONG (WINAPI *pfRegDeleteValueW)(HKEY hKey,LPCWSTR lpValueName);
// typedef LONG (WINAPI *pfRegDisablePredefinedCache)();// no need to be overrided
// typedef LONG (WINAPI *pfRegDisablePredefinedCacheEx)(void); // no need to be overrided
typedef LONG (WINAPI *pfRegDisableReflectionKey)(HKEY hBase);
typedef LONG (WINAPI *pfRegEnableReflectionKey)(HKEY hBase);
typedef LONG (WINAPI *pfRegEnumKeyA)(HKEY hKey,DWORD dwIndex,LPSTR lpName,DWORD cchName);
typedef LONG (WINAPI *pfRegEnumKeyW)(HKEY hKey,DWORD dwIndex,LPWSTR lpName,DWORD cchName);
typedef LONG (WINAPI *pfRegEnumKeyExA)(HKEY hKey,DWORD dwIndex,LPSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime);
typedef LONG (WINAPI *pfRegEnumKeyExW)(HKEY hKey,DWORD dwIndex,LPWSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPWSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime);
typedef LONG (WINAPI *pfRegEnumValueA)(HKEY hKey,DWORD dwIndex,LPSTR lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);
typedef LONG (WINAPI *pfRegEnumValueW)(HKEY hKey,DWORD dwIndex,LPWSTR lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);
typedef LONG (WINAPI *pfRegFlushKey)(HKEY hKey);
typedef LONG (WINAPI *pfRegGetKeySecurity)(HKEY hKey,SECURITY_INFORMATION SecurityInformation,PSECURITY_DESCRIPTOR pSecurityDescriptor,LPDWORD lpcbSecurityDescriptor);
typedef LONG (WINAPI *pfRegGetValueA)(HKEY hkey,LPCSTR lpSubKey,LPCSTR lpValue,DWORD dwFlags,LPDWORD pdwType,PVOID pvData,LPDWORD pcbData);
typedef LONG (WINAPI *pfRegGetValueW)(HKEY hkey,LPCWSTR lpSubKey,LPCWSTR lpValue,DWORD dwFlags,LPDWORD pdwType,PVOID pvData,LPDWORD pcbData);
typedef LONG (WINAPI *pfRegLoadAppKeyA)(LPCSTR lpFile,PHKEY phkResult,REGSAM samDesired,DWORD dwOptions,DWORD Reserved);
typedef LONG (WINAPI *pfRegLoadAppKeyW)(LPCWSTR lpFile,PHKEY phkResult,REGSAM samDesired,DWORD dwOptions,DWORD Reserved);
typedef LONG (WINAPI *pfRegLoadKeyA)(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpFile);
typedef LONG (WINAPI *pfRegLoadKeyW)(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpFile);
typedef LONG (WINAPI *pfRegLoadMUIStringA)(HKEY hKey,LPCSTR pszValue,LPSTR pszOutBuf,DWORD cbOutBuf,LPDWORD pcbData,DWORD Flags,LPCSTR pszDirectory);
typedef LONG (WINAPI *pfRegLoadMUIStringW)(HKEY hKey,LPCWSTR pszValue,LPWSTR pszOutBuf,DWORD cbOutBuf,LPDWORD pcbData,DWORD Flags,LPCWSTR pszDirectory);
typedef LONG (WINAPI *pfRegNotifyChangeKeyValue)(HKEY hKey,BOOL bWatchSubtree,DWORD dwNotifyFilter,HANDLE hEvent,BOOL fAsynchronous);
typedef LONG (WINAPI *pfRegOpenCurrentUser)(REGSAM samDesired,PHKEY phkResult);
typedef LONG (WINAPI *pfRegOpenKeyA)(HKEY hKey,LPCSTR lpSubKey,PHKEY phkResult);
typedef LONG (WINAPI *pfRegOpenKeyW)(HKEY hKey,LPCWSTR lpSubKey,PHKEY phkResult);
typedef LONG (WINAPI *pfRegOpenKeyExA)(HKEY hKey,LPCSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult);
typedef LONG (WINAPI *pfRegOpenKeyExW)(HKEY hKey,LPCWSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult);
typedef LONG (WINAPI *pfRegOpenKeyTransactedA)(HKEY hKey,LPCSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,HANDLE hTransaction,PVOID pExtendedParameter);
typedef LONG (WINAPI *pfRegOpenKeyTransactedW)(HKEY hKey,LPCWSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,HANDLE hTransaction,PVOID pExtendedParameter);
typedef LONG (WINAPI *pfRegOpenUserClassesRoot)(HANDLE hToken,DWORD dwOptions,REGSAM samDesired,PHKEY phkResult);
typedef LONG (WINAPI *pfRegOverridePredefKey)(HKEY hKey,HKEY hNewHKey);
typedef LONG (WINAPI *pfRegQueryInfoKeyA)(HKEY hKey,LPSTR lpClass,LPDWORD lpcClass,LPDWORD lpReserved,LPDWORD lpcSubKeys,LPDWORD lpcMaxSubKeyLen,LPDWORD lpcMaxClassLen,LPDWORD lpcValues,LPDWORD lpcMaxValueNameLen,LPDWORD lpcMaxValueLen,LPDWORD lpcbSecurityDescriptor,PFILETIME lpftLastWriteTime);
typedef LONG (WINAPI *pfRegQueryInfoKeyW)(HKEY hKey,LPWSTR lpClass,LPDWORD lpcClass,LPDWORD lpReserved,LPDWORD lpcSubKeys,LPDWORD lpcMaxSubKeyLen,LPDWORD lpcMaxClassLen,LPDWORD lpcValues,LPDWORD lpcMaxValueNameLen,LPDWORD lpcMaxValueLen,LPDWORD lpcbSecurityDescriptor,PFILETIME lpftLastWriteTime);
typedef LONG (WINAPI *pfRegQueryMultipleValuesA)(HKEY hKey,PVALENTA val_list,DWORD num_vals,LPSTR lpValueBuf,LPDWORD ldwTotsize);
typedef LONG (WINAPI *pfRegQueryMultipleValuesW)(HKEY hKey,PVALENTW val_list,DWORD num_vals,LPWSTR lpValueBuf,LPDWORD ldwTotsize);
typedef LONG (WINAPI *pfRegQueryReflectionKey)(HKEY hBase,BOOL *bIsReflectionDisabled);
typedef LONG (WINAPI *pfRegQueryValueA)(HKEY hKey,LPCSTR lpSubKey,LPSTR lpValue,PLONG lpcbValue);
typedef LONG (WINAPI *pfRegQueryValueW)(HKEY hKey,LPCWSTR lpSubKey,LPWSTR lpValue,PLONG lpcbValue);
typedef LONG (WINAPI *pfRegQueryValueExA)(HKEY hKey,LPCSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);
typedef LONG (WINAPI *pfRegQueryValueExW)(HKEY hKey,LPCWSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);
typedef LONG (WINAPI *pfRegReplaceKeyA)(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpNewFile,LPCSTR lpOldFile);
typedef LONG (WINAPI *pfRegReplaceKeyW)(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpNewFile,LPCWSTR lpOldFile);
typedef LONG (WINAPI *pfRegRestoreKeyA)(HKEY hKey,LPCSTR lpFile,DWORD dwFlags);
typedef LONG (WINAPI *pfRegRestoreKeyW)(HKEY hKey,LPCWSTR lpFile,DWORD dwFlags);
typedef LONG (WINAPI *pfRegSaveKeyA)(HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef LONG (WINAPI *pfRegSaveKeyW)(HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes);
typedef LONG (WINAPI *pfRegSaveKeyExA)(HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags);
typedef LONG (WINAPI *pfRegSaveKeyExW)(HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags);
typedef LONG (WINAPI *pfRegSetKeySecurity)(HKEY hKey,SECURITY_INFORMATION SecurityInformation,PSECURITY_DESCRIPTOR pSecurityDescriptor);
typedef LONG (WINAPI *pfRegSetKeyValueA)(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpValueName,DWORD dwType,LPCVOID lpData,DWORD cbData);
typedef LONG (WINAPI *pfRegSetKeyValueW)(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpValueName,DWORD dwType,LPCVOID lpData,DWORD cbData);
typedef LONG (WINAPI *pfRegSetValueA)(HKEY hKey,LPCSTR lpSubKey,DWORD dwType,LPCSTR lpData,DWORD cbData);
typedef LONG (WINAPI *pfRegSetValueW)(HKEY hKey,LPCWSTR lpSubKey,DWORD dwType,LPCWSTR lpData,DWORD cbData);
typedef LONG (WINAPI *pfRegSetValueExA)(HKEY hKey,LPCSTR lpValueName,DWORD Reserved,DWORD dwType,const BYTE* lpData,DWORD cbData);
typedef LONG (WINAPI *pfRegSetValueExW)(HKEY hKey,LPCWSTR lpValueName,DWORD Reserved,DWORD dwType,const BYTE* lpData,DWORD cbData);
typedef LONG (WINAPI *pfRegUnLoadKeyA)(HKEY hKey,LPCSTR lpSubKey);
typedef LONG (WINAPI *pfRegUnLoadKeyW)(HKEY hKey,LPCWSTR lpSubKey);

LONG WINAPI mRegCloseKey(HKEY hKey,pfRegCloseKey pFunction);
LONG WINAPI mRegConnectRegistryA(LPCSTR lpMachineName,HKEY hKey,PHKEY phkResult,pfRegConnectRegistryA pFunction);
LONG WINAPI mRegConnectRegistryW(LPCWSTR lpMachineName,HKEY hKey,PHKEY phkResult,pfRegConnectRegistryW pFunction);
LONG WINAPI mRegCopyTreeA(HKEY hKeySrc,LPCSTR lpSubKey,HKEY hKeyDest,pfRegCopyTreeA pFunction);
LONG WINAPI mRegCopyTreeW(HKEY hKeySrc,LPCWSTR lpSubKey,HKEY hKeyDest,pfRegCopyTreeW pFunction);
LONG WINAPI mRegCreateKeyA(HKEY hKey,LPCSTR lpSubKey,PHKEY phkResult,pfRegCreateKeyA pFunction);
LONG WINAPI mRegCreateKeyW(HKEY hKey,LPCWSTR lpSubKey,PHKEY phkResult,pfRegCreateKeyW pFunction);
LONG WINAPI mRegCreateKeyExA(HKEY hKey,LPCSTR lpSubKey,DWORD Reserved, LPSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,pfRegCreateKeyExA pFunction);
LONG WINAPI mRegCreateKeyExW(HKEY hKey,LPCWSTR lpSubKey,DWORD Reserved, LPWSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,pfRegCreateKeyExW pFunction);
LONG WINAPI mRegCreateKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,DWORD Reserved,LPSTR lpClass,DWORD dwOptions,REGSAM samDesired,const LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,HANDLE hTransaction,PVOID pExtendedParemeter,pfRegCreateKeyTransactedA pFunction);
LONG WINAPI mRegCreateKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,DWORD Reserved,LPWSTR lpClass,DWORD dwOptions,REGSAM samDesired,const LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,HANDLE hTransaction,PVOID pExtendedParemeter,pfRegCreateKeyTransactedW pFunction);
LONG WINAPI mRegDeleteKeyA(HKEY hKey,LPCSTR lpSubKey,pfRegDeleteKeyA pFunction);
LONG WINAPI mRegDeleteKeyW(HKEY hKey,LPCWSTR lpSubKey,pfRegDeleteKeyW pFunction);
LONG WINAPI mRegDeleteKeyExA(HKEY hKey,LPCSTR lpSubKey,REGSAM samDesired,DWORD Reserved,pfRegDeleteKeyExA pFunction);
LONG WINAPI mRegDeleteKeyExW(HKEY hKey,LPCWSTR lpSubKey,REGSAM samDesired,DWORD Reserved,pfRegDeleteKeyExW pFunction);
LONG WINAPI mRegDeleteKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,REGSAM samDesired,DWORD Reserved,HANDLE hTransaction,PVOID pExtendedParameter,pfRegDeleteKeyTransactedA pFunction);
LONG WINAPI mRegDeleteKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,REGSAM samDesired,DWORD Reserved,HANDLE hTransaction,PVOID pExtendedParameter,pfRegDeleteKeyTransactedW pFunction);
LONG WINAPI mRegDeleteKeyValueA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpValueName,pfRegDeleteKeyValueA pFunction);
LONG WINAPI mRegDeleteKeyValueW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpValueName,pfRegDeleteKeyValueW pFunction);
LONG WINAPI mRegDeleteTreeA(HKEY hKey,LPCSTR lpSubKey,pfRegDeleteTreeA pFunction);
LONG WINAPI mRegDeleteTreeW(HKEY hKey,LPCWSTR lpSubKey,pfRegDeleteTreeW pFunction);
LONG WINAPI mRegDeleteValueA(HKEY hKey,LPCSTR lpValueName,pfRegDeleteValueA pFunction);
LONG WINAPI mRegDeleteValueW(HKEY hKey,LPCWSTR lpValueName,pfRegDeleteValueW pFunction);
// LONG WINAPI RegDisablePredefinedCache();// no need to be overrided
// LONG WINAPI RegDisablePredefinedCacheEx(); // no need to be overrided
LONG WINAPI mRegDisableReflectionKey(HKEY hBase,pfRegDisableReflectionKey pFunction);
LONG WINAPI mRegEnableReflectionKey(HKEY hBase,pfRegEnableReflectionKey pFunction);
LONG WINAPI mRegEnumKeyA(HKEY hKey,DWORD dwIndex,LPSTR lpName,DWORD cchName,pfRegEnumKeyA pFunction);
LONG WINAPI mRegEnumKeyW(HKEY hKey,DWORD dwIndex,LPWSTR lpName,DWORD cchName,pfRegEnumKeyW pFunction);
LONG WINAPI mRegEnumKeyExA(HKEY hKey,DWORD dwIndex,LPSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime,pfRegEnumKeyExA pFunction);
LONG WINAPI mRegEnumKeyExW(HKEY hKey,DWORD dwIndex,LPWSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPWSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime,pfRegEnumKeyExW pFunction);
LONG WINAPI mRegEnumValueA(HKEY hKey,DWORD dwIndex,LPSTR lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData,pfRegEnumValueA pFunction);
LONG WINAPI mRegEnumValueW(HKEY hKey,DWORD dwIndex,LPWSTR lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData,pfRegEnumValueW pFunction);
LONG WINAPI mRegFlushKey(HKEY hKey,pfRegFlushKey pFunction);
LONG WINAPI mRegGetKeySecurity(HKEY hKey,SECURITY_INFORMATION SecurityInformation,PSECURITY_DESCRIPTOR pSecurityDescriptor,LPDWORD lpcbSecurityDescriptor,pfRegGetKeySecurity pFunction);
LONG WINAPI mRegGetValueA(HKEY hkey,LPCSTR lpSubKey,LPCSTR lpValue,DWORD dwFlags,LPDWORD pdwType,PVOID pvData,LPDWORD pcbData,pfRegGetValueA pFunction);
LONG WINAPI mRegGetValueW(HKEY hkey,LPCWSTR lpSubKey,LPCWSTR lpValue,DWORD dwFlags,LPDWORD pdwType,PVOID pvData,LPDWORD pcbData,pfRegGetValueW pFunction);
LONG WINAPI mRegLoadAppKeyA(LPCSTR lpFile,PHKEY phkResult,REGSAM samDesired,DWORD dwOptions,DWORD Reserved,pfRegLoadAppKeyA pFunction);
LONG WINAPI mRegLoadAppKeyW(LPCWSTR lpFile,PHKEY phkResult,REGSAM samDesired,DWORD dwOptions,DWORD Reserved,pfRegLoadAppKeyW pFunction);
LONG WINAPI mRegLoadKeyA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpFile,pfRegLoadKeyA pFunction);
LONG WINAPI mRegLoadKeyW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpFile,pfRegLoadKeyW pFunction);
LONG WINAPI mRegLoadMUIStringA(HKEY hKey,LPCSTR pszValue,LPSTR pszOutBuf,DWORD cbOutBuf,LPDWORD pcbData,DWORD Flags,LPCSTR pszDirectory,pfRegLoadMUIStringA pFunction);
LONG WINAPI mRegLoadMUIStringW(HKEY hKey,LPCWSTR pszValue,LPWSTR pszOutBuf,DWORD cbOutBuf,LPDWORD pcbData,DWORD Flags,LPCWSTR pszDirectory,pfRegLoadMUIStringW pFunction);
LONG WINAPI mRegNotifyChangeKeyValue(HKEY hKey,BOOL bWatchSubtree,DWORD dwNotifyFilter,HANDLE hEvent,BOOL fAsynchronous,pfRegNotifyChangeKeyValue pFunction);
LONG WINAPI mRegOpenCurrentUser(REGSAM samDesired,PHKEY phkResult,pfRegOpenCurrentUser pFunction);
LONG WINAPI mRegOpenKeyA(HKEY hKey,LPCSTR lpSubKey,PHKEY phkResult,pfRegOpenKeyA pFunction);
LONG WINAPI mRegOpenKeyW(HKEY hKey,LPCWSTR lpSubKey,PHKEY phkResult,pfRegOpenKeyW pFunction);
LONG WINAPI mRegOpenKeyExA(HKEY hKey,LPCSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,pfRegOpenKeyExA pFunction);
LONG WINAPI mRegOpenKeyExW(HKEY hKey,LPCWSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,pfRegOpenKeyExW pFunction);
LONG WINAPI mRegOpenKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,HANDLE hTransaction,PVOID pExtendedParameter,pfRegOpenKeyTransactedA pFunction);
LONG WINAPI mRegOpenKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,HANDLE hTransaction,PVOID pExtendedParameter,pfRegOpenKeyTransactedW pFunction);
LONG WINAPI mRegOpenUserClassesRoot(HANDLE hToken,DWORD dwOptions,REGSAM samDesired,PHKEY phkResult,pfRegOpenUserClassesRoot pFunction);
LONG WINAPI mRegOverridePredefKey(HKEY hKey,HKEY hNewHKey,pfRegOverridePredefKey pFunction);
LONG WINAPI mRegQueryInfoKeyA(HKEY hKey,LPSTR lpClass,LPDWORD lpcClass,LPDWORD lpReserved,LPDWORD lpcSubKeys,LPDWORD lpcMaxSubKeyLen,LPDWORD lpcMaxClassLen,LPDWORD lpcValues,LPDWORD lpcMaxValueNameLen,LPDWORD lpcMaxValueLen,LPDWORD lpcbSecurityDescriptor,PFILETIME lpftLastWriteTime,pfRegQueryInfoKeyA pFunction);
LONG WINAPI mRegQueryInfoKeyW(HKEY hKey,LPWSTR lpClass,LPDWORD lpcClass,LPDWORD lpReserved,LPDWORD lpcSubKeys,LPDWORD lpcMaxSubKeyLen,LPDWORD lpcMaxClassLen,LPDWORD lpcValues,LPDWORD lpcMaxValueNameLen,LPDWORD lpcMaxValueLen,LPDWORD lpcbSecurityDescriptor,PFILETIME lpftLastWriteTime,pfRegQueryInfoKeyW pFunction);
LONG WINAPI mRegQueryMultipleValuesA(HKEY hKey,PVALENTA val_list,DWORD num_vals,LPSTR lpValueBuf,LPDWORD ldwTotsize,pfRegQueryMultipleValuesA pFunction,pfRegQueryValueExA pRegQueryValueExA);
LONG WINAPI mRegQueryMultipleValuesW(HKEY hKey,PVALENTW val_list,DWORD num_vals,LPWSTR lpValueBuf,LPDWORD ldwTotsize,pfRegQueryMultipleValuesW pFunction,pfRegQueryValueExW pRegQueryValueExW);
LONG WINAPI mRegQueryReflectionKey(HKEY hBase,BOOL *bIsReflectionDisabled,pfRegQueryReflectionKey pFunction);
LONG WINAPI mRegQueryValueA(HKEY hKey,LPCSTR lpSubKey,LPSTR lpValue,PLONG lpcbValue,pfRegQueryValueA pFunction);
LONG WINAPI mRegQueryValueW(HKEY hKey,LPCWSTR lpSubKey,LPWSTR lpValue,PLONG lpcbValue,pfRegQueryValueW pFunction);
LONG WINAPI mRegQueryValueExA(HKEY hKey,LPCSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData,pfRegQueryValueExA pFunction);
LONG WINAPI mRegQueryValueExW(HKEY hKey,LPCWSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData,pfRegQueryValueExW pFunction);
LONG WINAPI mRegReplaceKeyA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpNewFile,LPCSTR lpOldFile,pfRegReplaceKeyA pFunction);
LONG WINAPI mRegReplaceKeyW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpNewFile,LPCWSTR lpOldFile,pfRegReplaceKeyW pFunction);
LONG WINAPI mRegRestoreKeyA(HKEY hKey,LPCSTR lpFile,DWORD dwFlags,pfRegRestoreKeyA pFunction);
LONG WINAPI mRegRestoreKeyW(HKEY hKey,LPCWSTR lpFile,DWORD dwFlags,pfRegRestoreKeyW pFunction);
LONG WINAPI mRegSaveKeyA(HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,pfRegSaveKeyA pFunction);
LONG WINAPI mRegSaveKeyW(HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,pfRegSaveKeyW pFunction);
LONG WINAPI mRegSaveKeyExA(HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags,pfRegSaveKeyExA pFunction);
LONG WINAPI mRegSaveKeyExW(HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags,pfRegSaveKeyExW pFunction);
LONG WINAPI mRegSetKeySecurity(HKEY hKey,SECURITY_INFORMATION SecurityInformation,PSECURITY_DESCRIPTOR pSecurityDescriptor,pfRegSetKeySecurity pFunction);
LONG WINAPI mRegSetKeyValueA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpValueName,DWORD dwType,LPCVOID lpData,DWORD cbData,pfRegSetKeyValueA pFunction);
LONG WINAPI mRegSetKeyValueW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpValueName,DWORD dwType,LPCVOID lpData,DWORD cbData,pfRegSetKeyValueW pFunction);
LONG WINAPI mRegSetValueA(HKEY hKey,LPCSTR lpSubKey,DWORD dwType,LPCSTR lpData,DWORD cbData,pfRegSetValueA pFunction);
LONG WINAPI mRegSetValueW(HKEY hKey,LPCWSTR lpSubKey,DWORD dwType,LPCWSTR lpData,DWORD cbData,pfRegSetValueW pFunction);
LONG WINAPI mRegSetValueExA(HKEY hKey,LPCSTR lpValueName,DWORD Reserved,DWORD dwType,const BYTE* lpData,DWORD cbData,pfRegSetValueExA pFunction);
LONG WINAPI mRegSetValueExW(HKEY hKey,LPCWSTR lpValueName,DWORD Reserved,DWORD dwType,const BYTE* lpData,DWORD cbData,pfRegSetValueExW pFunction);
LONG WINAPI mRegUnLoadKeyA(HKEY hKey,LPCSTR lpSubKey,pfRegUnLoadKeyA pFunction);
LONG WINAPI mRegUnLoadKeyW(HKEY hKey,LPCWSTR lpSubKey,pfRegUnLoadKeyW pFunction);
///////////////////////////////////////////////////////////////////////////////
// fake API array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API pArrayFakeAPI_7[]=
{
// !!!!!!!!!!!!! WARNING !!!!!!!
// advapi functions are redirected to kernerl32 (jumped) so override only kernel32 functions not boths
    // library name ,function name, function handler, stack size (required to allocate enough stack space), Additional options (optional put to 0 if you don't know it's meaning see documentation for more informations)
    //                                                stack size= sum(StackSizeOf(ParameterType))
//    {_T("advapi32.dll"),_T("RegCloseKey"),(FARPROC)mAdvapiRegCloseKey,StackSizeOf(HKEY),0},
    {_T("advapi32.dll"),_T("RegConnectRegistryA"),(FARPROC)mAdvapiRegConnectRegistryA,StackSizeOf(LPCSTR)+StackSizeOf(HKEY)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegConnectRegistryW"),(FARPROC)mAdvapiRegConnectRegistryW,StackSizeOf(LPCWSTR)+StackSizeOf(HKEY)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegCopyTreeA"),(FARPROC)mAdvapiRegCopyTreeA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(HKEY),0},
    {_T("advapi32.dll"),_T("RegCopyTreeW"),(FARPROC)mAdvapiRegCopyTreeW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(HKEY),0},
    {_T("advapi32.dll"),_T("RegCreateKeyA"),(FARPROC)mAdvapiRegCreateKeyA,StackSizeOf(LPCSTR)+StackSizeOf(HKEY)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegCreateKeyW"),(FARPROC)mAdvapiRegCreateKeyW,StackSizeOf(LPCWSTR)+StackSizeOf(HKEY)+StackSizeOf(PHKEY),0},
//    {_T("advapi32.dll"),_T("RegCreateKeyExA"),(FARPROC)mAdvapiRegCreateKeyExA,StackSizeOf(LPCSTR)+StackSizeOf(HKEY)+StackSizeOf(PHKEY)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(LPDWORD),0},
//    {_T("advapi32.dll"),_T("RegCreateKeyExW"),(FARPROC)mAdvapiRegCreateKeyExW,StackSizeOf(LPCWSTR)+StackSizeOf(HKEY)+StackSizeOf(PHKEY)+StackSizeOf(DWORD)+StackSizeOf(LPWSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(LPDWORD),0},
    {_T("advapi32.dll"),_T("RegCreateKeyTransactedA"),(FARPROC)mAdvapiRegCreateKeyTransactedA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(PHKEY)+StackSizeOf(LPDWORD)+StackSizeOf(HANDLE)+StackSizeOf(PVOID),0},
    {_T("advapi32.dll"),_T("RegCreateKeyTransactedW"),(FARPROC)mAdvapiRegCreateKeyTransactedW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD)+StackSizeOf(LPWSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(PHKEY)+StackSizeOf(LPDWORD)+StackSizeOf(HANDLE)+StackSizeOf(PVOID),0},
    {_T("advapi32.dll"),_T("RegDeleteKeyA"),(FARPROC)mAdvapiRegDeleteKeyA,StackSizeOf(LPCSTR)+StackSizeOf(HKEY),0},
    {_T("advapi32.dll"),_T("RegDeleteKeyW"),(FARPROC)mAdvapiRegDeleteKeyW,StackSizeOf(LPCWSTR)+StackSizeOf(HKEY),0},
//    {_T("advapi32.dll"),_T("RegDeleteKeyExA"),(FARPROC)mAdvapiRegDeleteKeyExA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(REGSAM)+StackSizeOf(DWORD),0},
//    {_T("advapi32.dll"),_T("RegDeleteKeyExW"),(FARPROC)mAdvapiRegDeleteKeyExW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(REGSAM)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegDeleteKeyTransactedA"),(FARPROC)mAdvapiRegDeleteKeyTransactedA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(REGSAM)+StackSizeOf(DWORD)+StackSizeOf(HANDLE)+StackSizeOf(PVOID),0},
    {_T("advapi32.dll"),_T("RegDeleteKeyTransactedW"),(FARPROC)mAdvapiRegDeleteKeyTransactedW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(REGSAM)+StackSizeOf(DWORD)+StackSizeOf(HANDLE)+StackSizeOf(PVOID),0},
    {_T("advapi32.dll"),_T("RegDeleteKeyValueA"),(FARPROC)mAdvapiRegDeleteKeyValueA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPCSTR),0},
    {_T("advapi32.dll"),_T("RegDeleteKeyValueW"),(FARPROC)mAdvapiRegDeleteKeyValueW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPCWSTR),0},
//    {_T("advapi32.dll"),_T("RegDeleteTreeA"),(FARPROC)mAdvapiRegDeleteTreeA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR),0},
//    {_T("advapi32.dll"),_T("RegDeleteTreeW"),(FARPROC)mAdvapiRegDeleteTreeW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR),0},
//    {_T("advapi32.dll"),_T("RegDeleteValueA"),(FARPROC)mAdvapiRegDeleteValueA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR),0},
//    {_T("advapi32.dll"),_T("RegDeleteValueW"),(FARPROC)mAdvapiRegDeleteValueW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR),0},
    //    {_T("advapi32.dll"),_T("RegDisablePredefinedCache"),(FARPROC)mAdvapiRegDisablePredefinedCache,0,0},// no need to be override
    //    {_T("advapi32.dll"),_T("RegDisablePredefinedCacheEx"),(FARPROC)mAdvapiRegDisablePredefinedCacheEx,0,0},// no need to be override
    {_T("advapi32.dll"),_T("RegDisableReflectionKey"),(FARPROC)mAdvapiRegDisableReflectionKey,StackSizeOf(HKEY),0},
    // RegDisableReflectionKey and RegEnableReflectionKey have same entry point and just return 0 on seven
    // {_T("advapi32.dll"),_T("RegEnableReflectionKey"),(FARPROC)mAdvapiRegDeleteValueW,StackSizeOf(HKEY),0},
    {_T("advapi32.dll"),_T("RegEnumKeyA"),(FARPROC)mAdvapiRegEnumKeyA,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegEnumKeyW"),(FARPROC)mAdvapiRegEnumKeyW,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPWSTR)+StackSizeOf(DWORD),0},
//    {_T("advapi32.dll"),_T("RegEnumKeyExA"),(FARPROC)mAdvapiRegEnumKeyExA,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD)+StackSizeOf(PFILETIME),0},
//    {_T("advapi32.dll"),_T("RegEnumKeyExW"),(FARPROC)mAdvapiRegEnumKeyExW,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPWSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPWSTR)+StackSizeOf(LPDWORD)+StackSizeOf(PFILETIME),0},
//    {_T("advapi32.dll"),_T("RegEnumValueA"),(FARPROC)mAdvapiRegEnumValueA,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPBYTE)+StackSizeOf(LPDWORD),0},
//    {_T("advapi32.dll"),_T("RegEnumValueW"),(FARPROC)mAdvapiRegEnumValueW,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPWSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPBYTE)+StackSizeOf(LPDWORD),0},
//    {_T("advapi32.dll"),_T("RegFlushKey"),(FARPROC)mAdvapiRegFlushKey,StackSizeOf(HKEY),0},
//    {_T("advapi32.dll"),_T("RegGetKeySecurity"),(FARPROC)mAdvapiRegGetKeySecurity,StackSizeOf(HKEY)+StackSizeOf(SECURITY_INFORMATION)+StackSizeOf(PSECURITY_DESCRIPTOR)+StackSizeOf(LPDWORD),0},
//    {_T("advapi32.dll"),_T("RegGetValueA"),(FARPROC)mAdvapiRegGetValueA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD)+StackSizeOf(LPDWORD)+StackSizeOf(PVOID)+StackSizeOf(LPDWORD),0},
//    {_T("advapi32.dll"),_T("RegGetValueW"),(FARPROC)mAdvapiRegGetValueW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD)+StackSizeOf(LPDWORD)+StackSizeOf(PVOID)+StackSizeOf(LPDWORD),0},
//    {_T("advapi32.dll"),_T("RegLoadAppKeyA"),(FARPROC)mAdvapiRegLoadAppKeyA,StackSizeOf(LPCSTR)+StackSizeOf(PHKEY)+StackSizeOf(REGSAM)+StackSizeOf(DWORD)+StackSizeOf(DWORD),0},
//    {_T("advapi32.dll"),_T("RegLoadAppKeyW"),(FARPROC)mAdvapiRegLoadAppKeyW,StackSizeOf(LPCWSTR)+StackSizeOf(PHKEY)+StackSizeOf(REGSAM)+StackSizeOf(DWORD)+StackSizeOf(DWORD),0},
//    {_T("advapi32.dll"),_T("RegLoadKeyA"),(FARPROC)mAdvapiRegLoadKeyA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPCSTR),0},
//    {_T("advapi32.dll"),_T("RegLoadKeyW"),(FARPROC)mAdvapiRegLoadKeyW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPCWSTR),0},
//    {_T("advapi32.dll"),_T("RegLoadMUIStringA"),(FARPROC)mAdvapiRegLoadMUIStringA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPSTR)+StackSizeOf(DWORD)+StackSizeOf(LPDWORD)+StackSizeOf(DWORD)+StackSizeOf(LPCSTR),0},
//    {_T("advapi32.dll"),_T("RegLoadMUIStringW"),(FARPROC)mAdvapiRegLoadMUIStringW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPWSTR)+StackSizeOf(DWORD)+StackSizeOf(LPDWORD)+StackSizeOf(DWORD)+StackSizeOf(LPCWSTR),0},
    {_T("advapi32.dll"),_T("RegNotifyChangeKeyValue"),(FARPROC)mAdvapiRegNotifyChangeKeyValue,StackSizeOf(HKEY)+StackSizeOf(BOOL)+StackSizeOf(DWORD)+StackSizeOf(HANDLE)+StackSizeOf(BOOL),0},
//    {_T("advapi32.dll"),_T("RegOpenCurrentUser"),(FARPROC)mAdvapiRegOpenCurrentUser,StackSizeOf(REGSAM)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegOpenKeyA"),(FARPROC)mAdvapiRegOpenKeyA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegOpenKeyW"),(FARPROC)mAdvapiRegOpenKeyW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(PHKEY),0},
//    {_T("advapi32.dll"),_T("RegOpenKeyExA"),(FARPROC)mAdvapiRegOpenKeyExA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(PHKEY),0},
//    {_T("advapi32.dll"),_T("RegOpenKeyExW"),(FARPROC)mAdvapiRegOpenKeyExW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegOpenKeyTransactedA"),(FARPROC)mAdvapiRegOpenKeyTransactedA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(PHKEY)+StackSizeOf(HANDLE)+StackSizeOf(PVOID),0},
    {_T("advapi32.dll"),_T("RegOpenKeyTransactedW"),(FARPROC)mAdvapiRegOpenKeyTransactedW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(PHKEY)+StackSizeOf(HANDLE)+StackSizeOf(PVOID),0},
//    {_T("advapi32.dll"),_T("RegOpenUserClassesRoot"),(FARPROC)mAdvapiRegOpenUserClassesRoot,StackSizeOf(HANDLE)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegOverridePredefKey"),(FARPROC)mAdvapiRegOverridePredefKey,StackSizeOf(HKEY)+StackSizeOf(HKEY),0},
//    {_T("advapi32.dll"),_T("RegQueryInfoKeyA"),(FARPROC)mAdvapiRegQueryInfoKeyA,StackSizeOf(HKEY)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(PFILETIME),0},
//    {_T("advapi32.dll"),_T("RegQueryInfoKeyW"),(FARPROC)mAdvapiRegQueryInfoKeyW,StackSizeOf(HKEY)+StackSizeOf(LPWSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(PFILETIME),0},
    {_T("advapi32.dll"),_T("RegQueryMultipleValuesA"),(FARPROC)mAdvapiRegQueryMultipleValuesA,StackSizeOf(HKEY)+StackSizeOf(PVALENT)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD),0},
    {_T("advapi32.dll"),_T("RegQueryMultipleValuesW"),(FARPROC)mAdvapiRegQueryMultipleValuesW,StackSizeOf(HKEY)+StackSizeOf(PVALENT)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD),0},
    {_T("advapi32.dll"),_T("RegQueryReflectionKey"),(FARPROC)mAdvapiRegQueryReflectionKey,StackSizeOf(HKEY)+StackSizeOf(BOOL*),0},
    {_T("advapi32.dll"),_T("RegQueryValueA"),(FARPROC)mAdvapiRegQueryValueA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPSTR)+StackSizeOf(PLONG),0},
    {_T("advapi32.dll"),_T("RegQueryValueW"),(FARPROC)mAdvapiRegQueryValueW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPWSTR)+StackSizeOf(PLONG),0},
//    {_T("advapi32.dll"),_T("RegQueryValueExA"),(FARPROC)mAdvapiRegQueryValueExA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPBYTE)+StackSizeOf(LPDWORD),0},
//    {_T("advapi32.dll"),_T("RegQueryValueExW"),(FARPROC)mAdvapiRegQueryValueExW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPBYTE)+StackSizeOf(LPDWORD),0},
    {_T("advapi32.dll"),_T("RegReplaceKeyA"),(FARPROC)mAdvapiRegReplaceKeyA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPCSTR)+StackSizeOf(LPCSTR),0},
    {_T("advapi32.dll"),_T("RegReplaceKeyW"),(FARPROC)mAdvapiRegReplaceKeyW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPCWSTR)+StackSizeOf(LPCWSTR),0},
//    {_T("advapi32.dll"),_T("RegRestoreKeyA"),(FARPROC)mAdvapiRegRestoreKeyA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD),0},
//    {_T("advapi32.dll"),_T("RegRestoreKeyW"),(FARPROC)mAdvapiRegRestoreKeyW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegSaveKeyA"),(FARPROC)mAdvapiRegSaveKeyA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPSECURITY_ATTRIBUTES),0},
    {_T("advapi32.dll"),_T("RegSaveKeyW"),(FARPROC)mAdvapiRegSaveKeyW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPSECURITY_ATTRIBUTES),0},
//    {_T("advapi32.dll"),_T("RegSaveKeyExA"),(FARPROC)mAdvapiRegSaveKeyExA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(DWORD),0},
//    {_T("advapi32.dll"),_T("RegSaveKeyExW"),(FARPROC)mAdvapiRegSaveKeyExW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(DWORD),0},
//    {_T("advapi32.dll"),_T("RegSetKeySecurity"),(FARPROC)mAdvapiRegSetKeySecurity,StackSizeOf(HKEY)+StackSizeOf(SECURITY_INFORMATION)+StackSizeOf(PSECURITY_DESCRIPTOR),0},
    {_T("advapi32.dll"),_T("RegSetKeyValueA"),(FARPROC)mAdvapiRegSetKeyValueA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR) +StackSizeOf(LPCSTR) +StackSizeOf(DWORD)+StackSizeOf(LPCVOID)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegSetKeyValueW"),(FARPROC)mAdvapiRegSetKeyValueW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD)+StackSizeOf(LPCVOID)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegSetValueA"),(FARPROC)mAdvapiRegSetValueA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegSetValueW"),(FARPROC)mAdvapiRegSetValueW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD),0},
//    {_T("advapi32.dll"),_T("RegSetValueExA"),(FARPROC)mAdvapiRegSetValueExA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD)+StackSizeOf(LPCSTR)+StackSizeOf(BYTE*)+StackSizeOf(DWORD),0},
//    {_T("advapi32.dll"),_T("RegSetValueExW"),(FARPROC)mAdvapiRegSetValueExW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD)+StackSizeOf(LPCWSTR)+StackSizeOf(BYTE*)+StackSizeOf(DWORD),0},
//    {_T("advapi32.dll"),_T("RegUnLoadKeyA"),(FARPROC)mAdvapiRegUnLoadKeyA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR),0},
//    {_T("advapi32.dll"),_T("RegUnLoadKeyW"),(FARPROC)mAdvapiRegUnLoadKeyW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR),0},

    {_T("kernel32.dll"),_T("RegCloseKey"),(FARPROC)mKernelRegCloseKey,StackSizeOf(HKEY),0},
    {_T("kernel32.dll"),_T("RegCreateKeyExA"),(FARPROC)mKernelRegCreateKeyExA,StackSizeOf(LPCSTR)+StackSizeOf(HKEY)+StackSizeOf(PHKEY)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(LPDWORD),0},
    {_T("kernel32.dll"),_T("RegCreateKeyExW"),(FARPROC)mKernelRegCreateKeyExW,StackSizeOf(LPCWSTR)+StackSizeOf(HKEY)+StackSizeOf(PHKEY)+StackSizeOf(DWORD)+StackSizeOf(LPWSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(LPDWORD),0},
    {_T("kernel32.dll"),_T("RegDeleteKeyExA"),(FARPROC)mKernelRegDeleteKeyExA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(REGSAM)+StackSizeOf(DWORD),0},
    {_T("kernel32.dll"),_T("RegDeleteKeyExW"),(FARPROC)mKernelRegDeleteKeyExW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(REGSAM)+StackSizeOf(DWORD),0},
    {_T("kernel32.dll"),_T("RegDeleteTreeA"),(FARPROC)mKernelRegDeleteTreeA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR),0},
    {_T("kernel32.dll"),_T("RegDeleteTreeW"),(FARPROC)mKernelRegDeleteTreeW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR),0},
    {_T("kernel32.dll"),_T("RegDeleteValueA"),(FARPROC)mKernelRegDeleteValueA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR),0},
    {_T("kernel32.dll"),_T("RegDeleteValueW"),(FARPROC)mKernelRegDeleteValueW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR),0},
    //    {_T("kernel32.dll"),_T("RegDisablePredefinedCacheEx"),(FARPROC)mKernelRegDisablePredefinedCacheEx,0,0},// no need to be override
    {_T("kernel32.dll"),_T("RegEnumKeyExA"),(FARPROC)mKernelRegEnumKeyExA,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD)+StackSizeOf(PFILETIME),0},
    {_T("kernel32.dll"),_T("RegEnumKeyExW"),(FARPROC)mKernelRegEnumKeyExW,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPWSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPWSTR)+StackSizeOf(LPDWORD)+StackSizeOf(PFILETIME),0},
    {_T("kernel32.dll"),_T("RegEnumValueA"),(FARPROC)mKernelRegEnumValueA,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPBYTE)+StackSizeOf(LPDWORD),0},
    {_T("kernel32.dll"),_T("RegEnumValueW"),(FARPROC)mKernelRegEnumValueW,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPWSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPBYTE)+StackSizeOf(LPDWORD),0},
    {_T("kernel32.dll"),_T("RegFlushKey"),(FARPROC)mKernelRegFlushKey,StackSizeOf(HKEY),0},
    {_T("kernel32.dll"),_T("RegGetKeySecurity"),(FARPROC)mKernelRegGetKeySecurity,StackSizeOf(HKEY)+StackSizeOf(SECURITY_INFORMATION)+StackSizeOf(PSECURITY_DESCRIPTOR)+StackSizeOf(LPDWORD),0},
    {_T("kernel32.dll"),_T("RegGetValueA"),(FARPROC)mKernelRegGetValueA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD)+StackSizeOf(LPDWORD)+StackSizeOf(PVOID)+StackSizeOf(LPDWORD),0},
    {_T("kernel32.dll"),_T("RegGetValueW"),(FARPROC)mKernelRegGetValueW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD)+StackSizeOf(LPDWORD)+StackSizeOf(PVOID)+StackSizeOf(LPDWORD),0},
    {_T("kernel32.dll"),_T("RegLoadKeyA"),(FARPROC)mKernelRegLoadKeyA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPCSTR),0},
    {_T("kernel32.dll"),_T("RegLoadKeyW"),(FARPROC)mKernelRegLoadKeyW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPCWSTR),0},
    {_T("kernel32.dll"),_T("RegLoadMUIStringA"),(FARPROC)mKernelRegLoadMUIStringA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPSTR)+StackSizeOf(DWORD)+StackSizeOf(LPDWORD)+StackSizeOf(DWORD)+StackSizeOf(LPCSTR),0},
    {_T("kernel32.dll"),_T("RegLoadMUIStringW"),(FARPROC)mKernelRegLoadMUIStringW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPWSTR)+StackSizeOf(DWORD)+StackSizeOf(LPDWORD)+StackSizeOf(DWORD)+StackSizeOf(LPCWSTR),0},
    {_T("kernel32.dll"),_T("RegNotifyChangeKeyValue"),(FARPROC)mKernelRegNotifyChangeKeyValue,StackSizeOf(HKEY)+StackSizeOf(BOOL)+StackSizeOf(DWORD)+StackSizeOf(HANDLE)+StackSizeOf(BOOL),0},
    {_T("kernel32.dll"),_T("RegOpenCurrentUser"),(FARPROC)mKernelRegOpenCurrentUser,StackSizeOf(REGSAM)+StackSizeOf(PHKEY),0},
    {_T("kernel32.dll"),_T("RegOpenKeyExA"),(FARPROC)mKernelRegOpenKeyExA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(PHKEY),0},
    {_T("kernel32.dll"),_T("RegOpenKeyExW"),(FARPROC)mKernelRegOpenKeyExW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(PHKEY),0},
    {_T("kernel32.dll"),_T("RegOpenUserClassesRoot"),(FARPROC)mKernelRegOpenUserClassesRoot,StackSizeOf(HANDLE)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(PHKEY),0},
    {_T("kernel32.dll"),_T("RegQueryInfoKeyA"),(FARPROC)mKernelRegQueryInfoKeyA,StackSizeOf(HKEY)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(PFILETIME),0},
    {_T("kernel32.dll"),_T("RegQueryInfoKeyW"),(FARPROC)mKernelRegQueryInfoKeyW,StackSizeOf(HKEY)+StackSizeOf(LPWSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(PFILETIME),0},
    {_T("kernel32.dll"),_T("RegQueryValueExA"),(FARPROC)mKernelRegQueryValueExA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPBYTE)+StackSizeOf(LPDWORD),0},
    {_T("kernel32.dll"),_T("RegQueryValueExW"),(FARPROC)mKernelRegQueryValueExW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPBYTE)+StackSizeOf(LPDWORD),0},
    {_T("kernel32.dll"),_T("RegRestoreKeyA"),(FARPROC)mKernelRegRestoreKeyA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD),0},
    {_T("kernel32.dll"),_T("RegRestoreKeyW"),(FARPROC)mKernelRegRestoreKeyW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD),0},
    {_T("kernel32.dll"),_T("RegSaveKeyExA"),(FARPROC)mKernelRegSaveKeyExA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(DWORD),0},
    {_T("kernel32.dll"),_T("RegSaveKeyExW"),(FARPROC)mKernelRegSaveKeyExW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(DWORD),0},
    {_T("kernel32.dll"),_T("RegSetKeySecurity"),(FARPROC)mKernelRegSetKeySecurity,StackSizeOf(HKEY)+StackSizeOf(SECURITY_INFORMATION)+StackSizeOf(PSECURITY_DESCRIPTOR),0},
    {_T("kernel32.dll"),_T("RegSetValueExA"),(FARPROC)mKernelRegSetValueExA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD)+StackSizeOf(LPCSTR)+StackSizeOf(BYTE*)+StackSizeOf(DWORD),0},
    {_T("kernel32.dll"),_T("RegSetValueExW"),(FARPROC)mKernelRegSetValueExW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD)+StackSizeOf(LPCWSTR)+StackSizeOf(BYTE*)+StackSizeOf(DWORD),0},
    {_T("kernel32.dll"),_T("RegUnLoadKeyA"),(FARPROC)mKernelRegUnLoadKeyA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR),0},
    {_T("kernel32.dll"),_T("RegUnLoadKeyW"),(FARPROC)mKernelRegUnLoadKeyW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR),0},



    // create process overriding functions
    // note : use OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_DONT_CHECK_MODULES_FILTERS
    // to catch created processes by all modules
    {_T("kernel32.dll"),_T("CreateProcessA"),(FARPROC)mCreateProcessA,
        StackSizeOf(LPCSTR)
        +StackSizeOf(CHAR*)
        +StackSizeOf(LPSECURITY_ATTRIBUTES)
        +StackSizeOf(LPSECURITY_ATTRIBUTES)
        +StackSizeOf(BOOL)
        +StackSizeOf(DWORD)
        +StackSizeOf(LPVOID)
        +StackSizeOf(LPCSTR)
        +StackSizeOf(LPSTARTUPINFOA)
        +StackSizeOf(LPPROCESS_INFORMATION)
        ,OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_DONT_CHECK_MODULES_FILTERS
    },

    {_T("kernel32.dll"),_T("CreateProcessW"),(FARPROC)mCreateProcessW,
        StackSizeOf(LPCWSTR)
        +StackSizeOf(WCHAR*)
        +StackSizeOf(LPSECURITY_ATTRIBUTES)
        +StackSizeOf(LPSECURITY_ATTRIBUTES)
        +StackSizeOf(BOOL)
        +StackSizeOf(DWORD)
        +StackSizeOf(LPVOID)
        +StackSizeOf(LPCWSTR)
        +StackSizeOf(LPSTARTUPINFOW)
        +StackSizeOf(LPPROCESS_INFORMATION)
        ,OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_DONT_CHECK_MODULES_FILTERS
    },


    {_T(""),_T(""),NULL,0,0}// last element for ending loops
};


STRUCT_FAKE_API pArrayFakeAPI_XP[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), Additional options (optional put to 0 if you don't know it's meaning see documentation for more informations)
    //                                                stack size= sum(StackSizeOf(ParameterType))
    {_T("advapi32.dll"),_T("RegCloseKey"),(FARPROC)mAdvapiRegCloseKey,StackSizeOf(HKEY),0},
    {_T("advapi32.dll"),_T("RegConnectRegistryA"),(FARPROC)mAdvapiRegConnectRegistryA,StackSizeOf(LPCSTR)+StackSizeOf(HKEY)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegConnectRegistryW"),(FARPROC)mAdvapiRegConnectRegistryW,StackSizeOf(LPCWSTR)+StackSizeOf(HKEY)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegCreateKeyA"),(FARPROC)mAdvapiRegCreateKeyA,StackSizeOf(LPCSTR)+StackSizeOf(HKEY)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegCreateKeyW"),(FARPROC)mAdvapiRegCreateKeyW,StackSizeOf(LPCWSTR)+StackSizeOf(HKEY)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegCreateKeyExA"),(FARPROC)mAdvapiRegCreateKeyExA,StackSizeOf(LPCSTR)+StackSizeOf(HKEY)+StackSizeOf(PHKEY)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(LPDWORD),0},
    {_T("advapi32.dll"),_T("RegCreateKeyExW"),(FARPROC)mAdvapiRegCreateKeyExW,StackSizeOf(LPCWSTR)+StackSizeOf(HKEY)+StackSizeOf(PHKEY)+StackSizeOf(DWORD)+StackSizeOf(LPWSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(LPDWORD),0},
    {_T("advapi32.dll"),_T("RegDeleteKeyA"),(FARPROC)mAdvapiRegDeleteKeyA,StackSizeOf(LPCSTR)+StackSizeOf(HKEY),0},
    {_T("advapi32.dll"),_T("RegDeleteKeyW"),(FARPROC)mAdvapiRegDeleteKeyW,StackSizeOf(LPCWSTR)+StackSizeOf(HKEY),0},
    {_T("advapi32.dll"),_T("RegDeleteValueA"),(FARPROC)mAdvapiRegDeleteValueA,StackSizeOf(LPCSTR)+StackSizeOf(HKEY),0},
    {_T("advapi32.dll"),_T("RegDeleteValueW"),(FARPROC)mAdvapiRegDeleteValueW,StackSizeOf(LPCWSTR)+StackSizeOf(HKEY),0},
    //{_T("advapi32.dll"),_T("RegDisablePredefinedCache"),(FARPROC)mAdvapiRegDisablePredefinedCache,0,0}, // no need to be override
    {_T("advapi32.dll"),_T("RegEnumKeyA"),(FARPROC)mAdvapiRegEnumKeyA,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegEnumKeyW"),(FARPROC)mAdvapiRegEnumKeyW,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPWSTR)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegEnumKeyExA"),(FARPROC)mAdvapiRegEnumKeyExA,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD)+StackSizeOf(PFILETIME),0},
    {_T("advapi32.dll"),_T("RegEnumKeyExW"),(FARPROC)mAdvapiRegEnumKeyExW,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPWSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPWSTR)+StackSizeOf(LPDWORD)+StackSizeOf(PFILETIME),0},
    {_T("advapi32.dll"),_T("RegEnumValueA"),(FARPROC)mAdvapiRegEnumValueA,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPBYTE)+StackSizeOf(LPDWORD),0},
    {_T("advapi32.dll"),_T("RegEnumValueW"),(FARPROC)mAdvapiRegEnumValueW,StackSizeOf(HKEY)+StackSizeOf(DWORD)+StackSizeOf(LPWSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPBYTE)+StackSizeOf(LPDWORD),0},
    {_T("advapi32.dll"),_T("RegFlushKey"),(FARPROC)mAdvapiRegFlushKey,StackSizeOf(HKEY),0},
    {_T("advapi32.dll"),_T("RegLoadKeyA"),(FARPROC)mAdvapiRegLoadKeyA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPCSTR),0},
    {_T("advapi32.dll"),_T("RegLoadKeyW"),(FARPROC)mAdvapiRegLoadKeyW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPCWSTR),0},
    {_T("advapi32.dll"),_T("RegNotifyChangeKeyValue"),(FARPROC)mAdvapiRegNotifyChangeKeyValue,StackSizeOf(HKEY)+StackSizeOf(BOOL)+StackSizeOf(DWORD)+StackSizeOf(HANDLE)+StackSizeOf(BOOL),0},
    {_T("advapi32.dll"),_T("RegOpenCurrentUser"),(FARPROC)mAdvapiRegOpenCurrentUser,StackSizeOf(REGSAM)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegOpenKeyA"),(FARPROC)mAdvapiRegOpenKeyA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegOpenKeyW"),(FARPROC)mAdvapiRegOpenKeyW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegOpenKeyExA"),(FARPROC)mAdvapiRegOpenKeyExA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegOpenKeyExW"),(FARPROC)mAdvapiRegOpenKeyExW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegOpenUserClassesRoot"),(FARPROC)mAdvapiRegOpenUserClassesRoot,StackSizeOf(HANDLE)+StackSizeOf(DWORD)+StackSizeOf(REGSAM)+StackSizeOf(PHKEY),0},
    {_T("advapi32.dll"),_T("RegOverridePredefKey"),(FARPROC)mAdvapiRegOverridePredefKey,StackSizeOf(HKEY)+StackSizeOf(HKEY),0},
    {_T("advapi32.dll"),_T("RegQueryInfoKeyA"),(FARPROC)mAdvapiRegQueryInfoKeyA,StackSizeOf(HKEY)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(PFILETIME),0},
    {_T("advapi32.dll"),_T("RegQueryInfoKeyW"),(FARPROC)mAdvapiRegQueryInfoKeyW,StackSizeOf(HKEY)+StackSizeOf(LPWSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(PFILETIME),0},
    {_T("advapi32.dll"),_T("RegQueryMultipleValuesA"),(FARPROC)mAdvapiRegQueryMultipleValuesA,StackSizeOf(HKEY)+StackSizeOf(PVALENT)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD),0},
    {_T("advapi32.dll"),_T("RegQueryMultipleValuesW"),(FARPROC)mAdvapiRegQueryMultipleValuesW,StackSizeOf(HKEY)+StackSizeOf(PVALENT)+StackSizeOf(DWORD)+StackSizeOf(LPSTR)+StackSizeOf(LPDWORD),0},
    {_T("advapi32.dll"),_T("RegQueryValueA"),(FARPROC)mAdvapiRegQueryValueA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPSTR)+StackSizeOf(PLONG),0},
    {_T("advapi32.dll"),_T("RegQueryValueW"),(FARPROC)mAdvapiRegQueryValueW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPWSTR)+StackSizeOf(PLONG),0},
    {_T("advapi32.dll"),_T("RegQueryValueExA"),(FARPROC)mAdvapiRegQueryValueExA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPBYTE)+StackSizeOf(LPDWORD),0},
    {_T("advapi32.dll"),_T("RegQueryValueExW"),(FARPROC)mAdvapiRegQueryValueExW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPDWORD)+StackSizeOf(LPDWORD)+StackSizeOf(LPBYTE)+StackSizeOf(LPDWORD),0},
    {_T("advapi32.dll"),_T("RegReplaceKeyA"),(FARPROC)mAdvapiRegReplaceKeyA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPCSTR)+StackSizeOf(LPCSTR),0},
    {_T("advapi32.dll"),_T("RegReplaceKeyW"),(FARPROC)mAdvapiRegReplaceKeyW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPCWSTR)+StackSizeOf(LPCWSTR),0},
    {_T("advapi32.dll"),_T("RegRestoreKeyA"),(FARPROC)mAdvapiRegRestoreKeyA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegRestoreKeyW"),(FARPROC)mAdvapiRegRestoreKeyW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegSaveKeyA"),(FARPROC)mAdvapiRegSaveKeyA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPSECURITY_ATTRIBUTES),0},
    {_T("advapi32.dll"),_T("RegSaveKeyW"),(FARPROC)mAdvapiRegSaveKeyW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPSECURITY_ATTRIBUTES),0},
    {_T("advapi32.dll"),_T("RegSaveKeyExA"),(FARPROC)mAdvapiRegSaveKeyExA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegSaveKeyExW"),(FARPROC)mAdvapiRegSaveKeyExW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(LPSECURITY_ATTRIBUTES)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegSetValueA"),(FARPROC)mAdvapiRegSetValueA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegSetValueW"),(FARPROC)mAdvapiRegSetValueW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegSetValueExA"),(FARPROC)mAdvapiRegSetValueExA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR)+StackSizeOf(DWORD)+StackSizeOf(LPCSTR)+StackSizeOf(BYTE*)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegSetValueExW"),(FARPROC)mAdvapiRegSetValueExW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR)+StackSizeOf(DWORD)+StackSizeOf(LPCWSTR)+StackSizeOf(BYTE*)+StackSizeOf(DWORD),0},
    {_T("advapi32.dll"),_T("RegUnLoadKeyA"),(FARPROC)mAdvapiRegUnLoadKeyA,StackSizeOf(HKEY)+StackSizeOf(LPCSTR),0},
    {_T("advapi32.dll"),_T("RegUnLoadKeyW"),(FARPROC)mAdvapiRegUnLoadKeyW,StackSizeOf(HKEY)+StackSizeOf(LPCWSTR),0},

    // create process overriding functions
    // note : use OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_DONT_CHECK_MODULES_FILTERS
    // to catch created processes by all modules
    {_T("kernel32.dll"),_T("CreateProcessA"),(FARPROC)mCreateProcessA,
        StackSizeOf(LPCSTR)
        +StackSizeOf(CHAR*)
        +StackSizeOf(LPSECURITY_ATTRIBUTES)
        +StackSizeOf(LPSECURITY_ATTRIBUTES)
        +StackSizeOf(BOOL)
        +StackSizeOf(DWORD)
        +StackSizeOf(LPVOID)
        +StackSizeOf(LPCSTR)
        +StackSizeOf(LPSTARTUPINFOA)
        +StackSizeOf(LPPROCESS_INFORMATION)
        ,OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_DONT_CHECK_MODULES_FILTERS
    },

    {_T("kernel32.dll"),_T("CreateProcessW"),(FARPROC)mCreateProcessW,
        StackSizeOf(LPCWSTR)
        +StackSizeOf(WCHAR*)
        +StackSizeOf(LPSECURITY_ATTRIBUTES)
        +StackSizeOf(LPSECURITY_ATTRIBUTES)
        +StackSizeOf(BOOL)
        +StackSizeOf(DWORD)
        +StackSizeOf(LPVOID)
        +StackSizeOf(LPCWSTR)
        +StackSizeOf(LPSTARTUPINFOW)
        +StackSizeOf(LPPROCESS_INFORMATION)
        ,OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_DONT_CHECK_MODULES_FILTERS
    },

    {_T(""),_T(""),NULL,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
// Before API call array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API_WITH_USERPARAM pArrayBeforeAPICall[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning),userParam : a value that will be post back to you when your hook will be called
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T(""),_T(""),NULL,0,0,0}// last element for ending loops
};

///////////////////////////////////////////////////////////////////////////////
// After API call array. Redirection are defined here
///////////////////////////////////////////////////////////////////////////////
STRUCT_FAKE_API_WITH_USERPARAM pArrayAfterAPICall[]=
{
    // library name ,function name, function handler, stack size (required to allocate enough stack space), FirstBytesCanExecuteAnywhereSize (optional put to 0 if you don't know it's meaning),userParam : a value that will be post back to you when your hook will be called
    //                                                stack size= sum(StackSizeOf(ParameterType))           Same as monitoring file keyword (see monitoring file advanced syntax)
    {_T(""),_T(""),NULL,0,0,0}// last element for ending loops
};
///////////////////////////////////////////////////////////////////////////////
///////////////////////////// NEW API DEFINITION //////////////////////////////
/////////////////////// You don't need to export these functions //////////////
///////////////////////////////////////////////////////////////////////////////
CKeyReplace* GetKeyReplace(HKEY hKey,OUT BOOL* pKeyCatchedByRegistryEmulator,OUT BOOL* pMarkedForDeletion)
{
    *pKeyCatchedByRegistryEmulator = FALSE;
    *pMarkedForDeletion = FALSE;
    if (!hKey)
        return NULL;
    if (IsBaseKey(hKey))
    {
        if (!gpEmulatedRegistry->pKeyLocalHost)
        {
#ifdef _DEBUG
            ::DebugBreak();
#endif
            return NULL;
        }
        *pKeyCatchedByRegistryEmulator = TRUE;
        return gpEmulatedRegistry->pKeyLocalHost->GetOrAddSubKey(BaseKeyToString(hKey));
    }
    else
    {
        if (gpEmulatedRegistry->IsValidKey((CKeyReplace*)hKey))
        {
            *pKeyCatchedByRegistryEmulator = TRUE;
            *pMarkedForDeletion = ((CKeyReplace*)hKey)->IsKeyMarkedForDeletion();
            return (CKeyReplace*)hKey;
        }
        else
            return NULL;
    }
}


BOOL gRegistry_bSpyMode = FALSE;
void SetSpyMode()
{
    gRegistry_bSpyMode = TRUE;
    if (gpEmulatedRegistry)
        gpEmulatedRegistry->SetSpyMode();
}

BOOL SetConfigurationFile()
{
    // Load all values of pseudo registry
    BOOL bRetValue = gpEmulatedRegistry->Load(gLauncherOptions.EmulatedRegistryConfigFileAbsolutePath);
    if (bRetValue)
    {
        if (gpEmulatedRegistry->pOptions->GetSpyMode())
            SetSpyMode();
    }
#ifdef _DEBUG
    else // if (!bRetValue)
    {
        if (!DoesFileExists(gLauncherOptions.EmulatedRegistryConfigFileAbsolutePath))
        {
        ::DebugBreak();
        }
        else
        {
            ::DebugBreak();
        }
    }
#endif

    return bRetValue;
}

extern "C" __declspec(dllexport) BOOL __stdcall SetConfigurationA(REGISTRY_REPLACEMENT_OPTIONSA * pOptions)
{
#if (defined(UNICODE)||defined(_UNICODE))
    gLauncherOptions.Version = pOptions->Version;
    gLauncherOptions.Flags = pOptions->Flags;
    gLauncherOptions.FilteringType = pOptions->FilteringType;
    EmulatedRegistry::CAnsiUnicodeConvert::AnsiToUnicode(pOptions->EmulatedRegistryConfigFileAbsolutePath,gLauncherOptions.EmulatedRegistryConfigFileAbsolutePath,MAX_PATH);
    EmulatedRegistry::CAnsiUnicodeConvert::AnsiToUnicode(pOptions->FilteringTypeFileAbsolutePath,gLauncherOptions.FilteringTypeFileAbsolutePath,MAX_PATH);
    EmulatedRegistry::CAnsiUnicodeConvert::AnsiToUnicode(pOptions->OutputFileWhenSpyModeEnabledAbsolutePath,gLauncherOptions.OutputFileWhenSpyModeEnabledAbsolutePath,MAX_PATH);
#else
    memcpy(&gLauncherOptions,pOptions,sizeof(REGISTRY_REPLACEMENT_OPTIONS));
#endif

    return SetConfigurationFile();
}
extern "C" __declspec(dllexport) BOOL __stdcall SetConfigurationW(REGISTRY_REPLACEMENT_OPTIONSW * pOptions)
{
#if (defined(UNICODE)||defined(_UNICODE))
    memcpy(&gLauncherOptions,pOptions,sizeof(REGISTRY_REPLACEMENT_OPTIONS));
#else
    gLauncherOptions.Version = pOptions->Version;
    gLauncherOptions.Flags = pOptions->Flags;
    gLauncherOptions.FilteringType = pOptions->FilteringType;
    EmulatedRegistry::CAnsiUnicodeConvert::UnicodeToAnsi(pOptions->EmulatedRegistryConfigFileAbsolutePath,gLauncherOptions.EmulatedRegistryConfigFileAbsolutePath,MAX_PATH);
    EmulatedRegistry::CAnsiUnicodeConvert::UnicodeToAnsi(pOptions->FilteringTypeFileAbsolutePath,gLauncherOptions.FilteringTypeFileAbsolutePath,MAX_PATH);   
    EmulatedRegistry::CAnsiUnicodeConvert::UnicodeToAnsi(pOptions->OutputFileWhenSpyModeEnabledAbsolutePath,gLauncherOptions.OutputFileWhenSpyModeEnabledAbsolutePath,MAX_PATH);     
#endif

    return SetConfigurationFile();
}

BOOL Initialize()
{

    gpEmulatedRegistry = new CEmulatedRegistry();

    ghModuleKernel = ::GetModuleHandle(_T("kernel32.dll"));
    ghModuleAdvapi = ::GetModuleHandle(_T("advapi32.dll"));
    if (!ghModuleAdvapi)
        ghModuleAdvapi = LoadLibrary(_T("advapi32.dll"));

    if ( (ghModuleAdvapi==NULL) || (ghModuleKernel==NULL) )
        return FALSE;

    ::GetModuleFileNameA(gDllhInstance,gLauncherFullPathA,MAX_PATH);
    CHAR* pcA = strrchr(gLauncherFullPathA,'\\');
    if (pcA)
    {
        pcA++;
        *pcA = 0;
    }
    strcat(gLauncherFullPathA,LAUNCHER_NAMEA);

    ::GetModuleFileNameW(gDllhInstance,gLauncherFullPathW,MAX_PATH);
    WCHAR* pcW = wcsrchr(gLauncherFullPathW,'\\');
    if (pcW)
    {
        pcW++;
        *pcW = 0;
    }
    wcscat(gLauncherFullPathW,LAUNCHER_NAMEW);        
        
#if (LOADED_WITH_WINAPIOVERRIDE_FOR_DEBUG == 1)
    // else use SetConfigurationFile() to get configuration file
    gLauncherOptions.FilteringType = FilteringType_ONLY_BASE_MODULE;
    *gLauncherOptions.FilteringTypeFileAbsolutePath = 0;

    // forge config file name
    ::GetModuleFileName(::GetModuleHandle(0),gLauncherOptions.OutputFileWhenSpyModeEnabledAbsolutePath,MAX_PATH);
    gLauncherOptions.OutputFileWhenSpyModeEnabledAbsolutePath[MAX_PATH-1]=0;
    _tcscat(gLauncherOptions.OutputFileWhenSpyModeEnabledAbsolutePath,CONFIG_FILE_EXTENSION); // config file must be placed in application directory
    
    ::GetModuleFileName(::GetModuleHandle(0),gLauncherOptions.OutputFileWhenSpyModeEnabledAbsolutePath,MAX_PATH);
    gLauncherOptions.OutputFileWhenSpyModeEnabledAbsolutePath[MAX_PATH-1]=0;
    _tcscat(gLauncherOptions.EmulatedRegistryConfigFileAbsolutePath,_T("GenericRegistryForSpying.xml"));

    SetConfigurationFile();
    #if (FORCE_SPY_MODE_FOR_DEBUG == 1)
        SetSpyMode();
    #endif
#endif

#ifdef _DEBUG
        //// avoid static linking which is really bad for us
        //HMODULE hMod = ::LoadLibrary(_T("user32.dll"));
        //if (hMod)
        //{
        //    typedef int (WINAPI *pfMessageBoxW)(__in_opt HWND hWnd,__in_opt LPCWSTR lpText,__in_opt LPCWSTR lpCaption,__in UINT uType);
        //    pfMessageBoxW pMsgBox = (pfMessageBoxW)::GetProcAddress(hMod,"MessageBoxW");
        //    if (pMsgBox)
        //        pMsgBox(0,L"RegReplace.dll : ready for debug",L"Reg Relpace.dll",MB_OK|MB_ICONINFORMATION);
        //}
#endif

    return TRUE;
}

void Destroy()
{
    TCHAR SavingFileName[MAX_PATH];

    // if we want to save all accessed keys
    if (gRegistry_bSpyMode)
    {
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Warning Launcher use the same algorithm so if change occurs here it MUST be reported inside launcher.exe
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////
        _tcscpy(SavingFileName,gLauncherOptions.OutputFileWhenSpyModeEnabledAbsolutePath);
        
        // change file extension from ".xml" to ".spy.xml" (try to avoid users to use spy file as portable registry)
        if (_tcslen(SavingFileName)+_tcslen(_T(".spy"))<MAX_PATH)
        {
            TCHAR* psz = _tcsrchr(SavingFileName,'.');
            if (psz)
            {
                // point after '.'
                psz++;

                // store new extension
                _tcscpy(psz,_T("spy.xml"));
            }
            else // file with no extension
                _tcscat(SavingFileName,_T(".spy.xml"));
        }
    }
    else
        _tcscpy(SavingFileName,gLauncherOptions.EmulatedRegistryConfigFileAbsolutePath);


    if (!(gLauncherOptions.Flags & REGISTRY_REPLACEMENT_OPTIONS_FLAG_DON_T_SAVE_REGISTRY))
        // save registry into xml file
        gpEmulatedRegistry->Save(SavingFileName);

    // free memory
    delete gpEmulatedRegistry;
}
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, PVOID pvReserved)
{
    UNREFERENCED_PARAMETER(pvReserved);
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        gDllhInstance=hInstDLL;
        DisableThreadLibraryCalls(gDllhInstance);
        return Initialize();
    case DLL_PROCESS_DETACH:
        Destroy();
        return TRUE;
    }
    return TRUE;
}

LONG WINAPI mRegCloseKey(HKEY hKey,pfRegCloseKey pFunction)
{
    LONG RetValue = ERROR_SUCCESS;
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (pFunction)
        {
            M_SECURE_TRY
            return pFunction(hKey);
            M_SECURE_CATCH
            return ERROR_FUNCTION_FAILED;
        }
        else
            return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_INVALID_PARAMETER;
        
    if (pKey->hRealRegistryKeyHandleIfNotEmulated)
    {
        if (pKey->hRealRegistryKeyHandleIfNotEmulated)
        {
            if (pFunction)
            {
                RetValue = ERROR_FUNCTION_FAILED;
                M_SECURE_TRY
                RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated);
                M_SECURE_CATCH
            }
            else
                RetValue = ERROR_FUNCTION_FAILED;
        }
        pKey->hRealRegistryKeyHandleIfNotEmulated =0;
    }
    if (pKey->DecreaseOpenedHandleCount()==0)
    {
        if (pKey->IsKeyMarkedForDeletion())
            pKey->RemoveCurrentKey();
    }
    return RetValue;
}
LONG WINAPI mRegConnectRegistryT(TCHAR* lpMachineName,HKEY hKey,PHKEY phkResult,OUT CHostKey** ppHostKey)
{
    LONG RetValue;
    CKeyReplace* pWantedKey = NULL;
    CHostKey* pHostKey = NULL;
    
    if (!lpMachineName)
        return ERROR_INVALID_PARAMETER;
    if (!IsBaseKey(hKey))
        return ERROR_INVALID_PARAMETER;
    pHostKey = (CHostKey*) gpEmulatedRegistry->pRootKey->GetSubKey(lpMachineName);
    if (!pHostKey)
    {
        // if remote hosts must be declared
        if (!gpEmulatedRegistry->pOptions->GetAllowConnectionToUnspecifiedRemoteHosts())// specified remote hosts have already been created by the loading feature
            return ERROR_FUNCTION_FAILED;
        else
        {
            // notice : we are forced to handle all keys 
            pHostKey = new CHostKey();
            pHostKey->KeyName = lpMachineName;
            pHostKey->AllowConnectionToRemoteHost = TRUE;
            pHostKey->DisableWriteOperationsOnNotEmulatedKeys = FALSE;
            
            gpEmulatedRegistry->pRootKey->AddHost(pHostKey);
            pWantedKey = pHostKey->GetOrAddSubKey(BaseKeyToString(hKey));
        }
    }
    else
    {
        pWantedKey = pHostKey->GetOrAddSubKey(BaseKeyToString(hKey));
    }
    pWantedKey->IncreaseOpenedHandleCount();
    *phkResult = (HKEY)pWantedKey;
    *ppHostKey = pHostKey;
    RetValue = (pWantedKey==0) ?ERROR_FUNCTION_FAILED:ERROR_SUCCESS;
    return RetValue;
}
LONG WINAPI mRegConnectRegistryA(LPCSTR lpMachineName,HKEY hKey,PHKEY phkResult,pfRegConnectRegistryA pFunction)
{
    LONG RetValue;
    TCHAR* psz;
    CHostKey* pHostKey = NULL;
    CAnsiUnicodeConvert::AnsiToTchar(lpMachineName,&psz);
    RetValue = mRegConnectRegistryT(psz,hKey,phkResult,&pHostKey);
    if (psz)
        free(psz);

    CKeyReplace* pKey = (CKeyReplace*)*phkResult;
    if (pKey && pHostKey)
    {
        // even if base key is emulated, get an handle to real key (better for RegOpenKey and RegCreateKey call for not emulated subkeys)
        if (pHostKey->AllowConnectionToRemoteHost )
        {
            if (pFunction)
            {
                RetValue = ERROR_FUNCTION_FAILED;
                // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                M_SECURE_TRY
                RetValue = pFunction(lpMachineName,hKey,&pKey->hRealRegistryKeyHandleIfNotEmulated);
                M_SECURE_CATCH

                pHostKey->IsHostConnected = (RetValue == ERROR_SUCCESS);
            }
            else
                return ERROR_FUNCTION_FAILED;
        }
    }    
    
    return RetValue;
}
LONG WINAPI mRegConnectRegistryW(LPCWSTR lpMachineName,HKEY hKey,PHKEY phkResult,pfRegConnectRegistryW pFunction)
{
    LONG RetValue;
    TCHAR* psz;
    CHostKey* pHostKey = NULL;
    CAnsiUnicodeConvert::UnicodeToTchar(lpMachineName,&psz);
    RetValue = mRegConnectRegistryT(psz,hKey,phkResult,&pHostKey);
    if (psz)
        free(psz);

    if (!phkResult)
        return RetValue;        

    CKeyReplace* pKey = (CKeyReplace*)*phkResult;
    if (pKey && pHostKey)
    {
        // even if base key is emulated, get an handle to real key (better for RegOpenKey and RegCreateKey call for not emulated subkeys)
        if (pHostKey->AllowConnectionToRemoteHost)
        {
            if (pFunction)
            {
                RetValue = ERROR_FUNCTION_FAILED;
                // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                M_SECURE_TRY
                    RetValue = pFunction(lpMachineName,hKey,&pKey->hRealRegistryKeyHandleIfNotEmulated);
                M_SECURE_CATCH
                
                pHostKey->IsHostConnected = (RetValue == ERROR_SUCCESS);
            }
            else
                return ERROR_FUNCTION_FAILED;
        }
    }  
        
    return RetValue;
}
LONG WINAPI mRegCopyTreeA(HKEY hKeySrc,LPCSTR lpSubKey,HKEY hKeyDest,pfRegCopyTreeA pFunction)
{
    BOOL SrcEmulatedKeyHandle = FALSE;
    BOOL DstEmulatedKeyHandle = FALSE;
    BOOL SrcMarkedForDeletion;
    BOOL DstMarkedForDeletion;
    CKeyReplace* pSrcKey = GetKeyReplace(hKeySrc,&SrcEmulatedKeyHandle,&SrcMarkedForDeletion);
    CKeyReplace* pDstKey = GetKeyReplace(hKeyDest,&DstEmulatedKeyHandle,&DstMarkedForDeletion);
    if ( (!SrcEmulatedKeyHandle) && (!DstEmulatedKeyHandle) )
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
            return pFunction(hKeySrc, lpSubKey,hKeyDest);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (SrcMarkedForDeletion || DstMarkedForDeletion)
        return ERROR_FILE_NOT_FOUND;
    if ((!SrcEmulatedKeyHandle) || (!DstEmulatedKeyHandle))
    {
        if (SrcEmulatedKeyHandle) // we miss the dst key handle
        {
            // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
            if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
                return ERROR_SUCCESS;
            
            // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
            M_SECURE_TRY
                return pFunction(pSrcKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, hKeyDest);
            M_SECURE_CATCH
            return ERROR_FUNCTION_FAILED;
        }
        else // DstEmulatedKeyHandle == TRUE we miss the src key handle
        {
            if (pDstKey->Emulated)
                return ERROR_FUNCTION_FAILED;

            if (!pDstKey->IsWriteAccessAllowed())
                return ERROR_SUCCESS;
            
            // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
            M_SECURE_TRY
                return pFunction(hKeySrc, lpSubKey,pDstKey->hRealRegistryKeyHandleIfNotEmulated);
            M_SECURE_CATCH
            return ERROR_FUNCTION_FAILED;
        }
    }
    // at this point pSrcKey and pDstKey are valid

    if (pDstKey->Emulated)
    {        
        TCHAR* psz;
        CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,&psz);
        CKeyReplace* pSrcSubKey = pSrcKey->GetOrAddSubKeyWithFullPath(psz);
        free(psz);
        if (!pSrcSubKey)
            return ERROR_FUNCTION_FAILED;
        return (pSrcSubKey->CopyKeyContent(pDstKey) == TRUE);
    }
    // else --> !pDstKey->Emulated
    
    // if write access is disabled
    if (!pDstKey->IsWriteAccessAllowed())
        return ERROR_SUCCESS;    
    
    M_SECURE_TRY
        return pFunction(pSrcKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey,pDstKey->hRealRegistryKeyHandleIfNotEmulated);
    M_SECURE_CATCH
    return ERROR_FUNCTION_FAILED;
}
LONG WINAPI mRegCopyTreeW(HKEY hKeySrc,LPCWSTR lpSubKey,HKEY hKeyDest,pfRegCopyTreeW pFunction)
{
    BOOL SrcEmulatedKeyHandle = FALSE;
    BOOL DstEmulatedKeyHandle = FALSE;
    BOOL SrcMarkedForDeletion;
    BOOL DstMarkedForDeletion;
    CKeyReplace* pSrcKey = GetKeyReplace(hKeySrc,&SrcEmulatedKeyHandle,&SrcMarkedForDeletion);
    CKeyReplace* pDstKey = GetKeyReplace(hKeyDest,&DstEmulatedKeyHandle,&DstMarkedForDeletion);
    if ( (!SrcEmulatedKeyHandle) && (!DstEmulatedKeyHandle) )
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        
        M_SECURE_TRY
            return pFunction(hKeySrc, lpSubKey,hKeyDest);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (SrcMarkedForDeletion || DstMarkedForDeletion)
        return ERROR_FILE_NOT_FOUND;
    if ((!SrcEmulatedKeyHandle) || (!DstEmulatedKeyHandle))
    {
        if (SrcEmulatedKeyHandle) // we miss the dst key handle
        {
            // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
            if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
                return ERROR_SUCCESS;
            
            // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
            M_SECURE_TRY
                return pFunction(pSrcKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, hKeyDest);
            M_SECURE_CATCH
            return ERROR_FUNCTION_FAILED;
        }
        else // DstEmulatedKeyHandle == TRUE we miss the src key handle
        {
            if (pDstKey->Emulated)
                return ERROR_FUNCTION_FAILED;

            if (!pDstKey->IsWriteAccessAllowed())
                return ERROR_SUCCESS;
            
            // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
            M_SECURE_TRY
            return pFunction(hKeySrc, lpSubKey,pDstKey->hRealRegistryKeyHandleIfNotEmulated);
            M_SECURE_CATCH
            return ERROR_FUNCTION_FAILED;
        }
    }
    // at this point pSrcKey and pDstKey are valid

    if (pDstKey->Emulated)
    {        
        TCHAR* psz;
        CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,&psz);
        CKeyReplace* pSrcSubKey = pSrcKey->GetOrAddSubKeyWithFullPath(psz);
        free(psz);
        if (!pSrcSubKey)
            return ERROR_FUNCTION_FAILED;
        return (pSrcSubKey->CopyKeyContent(pDstKey) == TRUE);
    }
    // else --> !pDstKey->Emulated
    
    // if write access is disabled
    if (!pDstKey->IsWriteAccessAllowed())
        return ERROR_SUCCESS;    
    M_SECURE_TRY
    return pFunction(pSrcKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey,pDstKey->hRealRegistryKeyHandleIfNotEmulated);
    M_SECURE_CATCH
    return ERROR_FUNCTION_FAILED;
}

LONG WINAPI mRegCreateKeyExT(CKeyReplace* pOriginalKey,TCHAR* lpSubKey,DWORD Reserved, TCHAR* lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition)
{
    *phkResult = NULL;
    if (!lpSubKey)
        return ERROR_INVALID_PARAMETER;

    BOOL bKeyWasExisting = FALSE;
    CKeyReplace* pCreatedKey = pOriginalKey->GetOrAddSubKeyWithFullPath(lpSubKey,&bKeyWasExisting);
    pCreatedKey->IncreaseOpenedHandleCount();
    if (!bKeyWasExisting)
    {
        if (lpClass)
            pCreatedKey->Class = lpClass;

        //if (dwOptions)
        //    pCreatedKey->dwOptions = dwOptions;
        //if (samDesired)
        //    pCreatedKey->samDesired = samDesired;
        //if (lpSecurityAttributes)
        //    pCreatedKey->lpSecurityAttributes = lpSecurityAttributes;
    }

    if (lpdwDisposition)
    {
        if (bKeyWasExisting)
            *lpdwDisposition = REG_OPENED_EXISTING_KEY;
        else
            *lpdwDisposition = REG_CREATED_NEW_KEY;
    }

    *phkResult = (HKEY)pCreatedKey;
    if (*phkResult)
        return ERROR_SUCCESS;
    else
        return ERROR_FUNCTION_FAILED;
}

LONG WINAPI mRegCreateKeyExA(HKEY hKey,LPCSTR lpSubKey,DWORD Reserved, LPSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,pfRegCreateKeyExA pFunction)
{
    LONG RetValue;

    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction(hKey, lpSubKey, Reserved,  lpClass, dwOptions, samDesired, lpSecurityAttributes,phkResult, lpdwDisposition);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    TCHAR* psz;
    CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,&psz);
    TCHAR* psz2;
    CAnsiUnicodeConvert::AnsiToTchar(lpClass,&psz2);
    RetValue = mRegCreateKeyExT(pKey,psz,Reserved,psz2,dwOptions,samDesired,lpSecurityAttributes,phkResult,lpdwDisposition);
    if (psz)
        free(psz);
    if (psz2)
        free(psz2);

    if (!phkResult)
        return RetValue;

    CKeyReplace* pNewKey = (CKeyReplace*)*phkResult;
    if (pKey && pNewKey)
    {
        // if key is not emulated, we have to get real registry informations
        // if we are in spy mode only we have get real informations too
        if  ( (!pNewKey->Emulated) || gRegistry_bSpyMode )
        {
            if (pNewKey->IsWriteAccessAllowed())
            {
                if (!pFunction)
                    return ERROR_FUNCTION_FAILED;
                if (!pKey->hRealRegistryKeyHandleIfNotEmulated)
                {
                    // we have to get real hkey even if not emulated
                    
                    // get base key
                    HKEY hBaseKey = NULL;
                    CKeyReplace* pBaseKey = pKey->GetBaseKey();
                    if (pBaseKey)
                    {
                        hBaseKey = StringToBaseKey((TCHAR*)pBaseKey->KeyName.c_str());
                    }
                    
                    // get reference key
                    DWORD Disposition = 0;
                    std::tstring KeyNameRelativeToHost=_T("");
                    pKey->GetKeyPathWithoutBaseKeyName(KeyNameRelativeToHost);
                    // open or create key with same attributes
                    // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                    CHAR* pc;
                    CAnsiUnicodeConvert::TcharToAnsi(KeyNameRelativeToHost.c_str(),&pc);
                    M_SECURE_TRY
                    pFunction(hBaseKey, pc, 0,  0, dwOptions, samDesired, lpSecurityAttributes,&pKey->hRealRegistryKeyHandleIfNotEmulated, &Disposition);
                    M_SECURE_CATCH
                    free(pc);
                }
                RetValue = ERROR_FUNCTION_FAILED;
                // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                M_SECURE_TRY
                RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, Reserved,  lpClass, dwOptions, samDesired, lpSecurityAttributes,&pNewKey->hRealRegistryKeyHandleIfNotEmulated, lpdwDisposition);
                M_SECURE_CATCH
            }
        }
    }        
        
    return RetValue;
}
LONG WINAPI mRegCreateKeyExW(HKEY hKey,LPCWSTR lpSubKey,DWORD Reserved, LPWSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,pfRegCreateKeyExW pFunction)
{
    LONG RetValue;

    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
           
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction(hKey, lpSubKey, Reserved,  lpClass, dwOptions, samDesired, lpSecurityAttributes,phkResult, lpdwDisposition);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    TCHAR* psz;
    CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,&psz);
    TCHAR* psz2;
    CAnsiUnicodeConvert::UnicodeToTchar(lpClass,&psz2);
    RetValue = mRegCreateKeyExT(pKey,psz,Reserved,psz2,dwOptions,samDesired,lpSecurityAttributes,phkResult,lpdwDisposition);
    if (psz)
        free(psz);
    if (psz2)
        free(psz2);

    if (!phkResult)
        return RetValue;

    CKeyReplace* pNewKey = (CKeyReplace*)*phkResult;
    if (pKey && pNewKey)
    {
        // if key is not emulated, we have to get real registry informations
        // if we are in spy mode only we have get real informations too
        if  ( (!pNewKey->Emulated) || gRegistry_bSpyMode )
        {
            if (pNewKey->IsWriteAccessAllowed())
            {
                if (!pFunction)
                    return ERROR_FUNCTION_FAILED;
                    
                if (!pKey->hRealRegistryKeyHandleIfNotEmulated)
                {
                    // we have to get real hkey even if not emulated
                    
                    // get base key
                    HKEY hBaseKey = NULL;
                    CKeyReplace* pBaseKey = pKey->GetBaseKey();
                    if (pBaseKey)
                    {
                        hBaseKey = StringToBaseKey((TCHAR*)pBaseKey->KeyName.c_str());
                    }
                    
                    // get reference key
                    DWORD Disposition = 0;
                    std::tstring KeyNameRelativeToHost=_T("");
                    pKey->GetKeyPathWithoutBaseKeyName(KeyNameRelativeToHost);
                    // open or create key with same attributes
                    // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                    WCHAR* pc;
                    CAnsiUnicodeConvert::TcharToUnicode(KeyNameRelativeToHost.c_str(),&pc);
                    M_SECURE_TRY
                    pFunction(hBaseKey, pc, 0,  0, dwOptions, samDesired, lpSecurityAttributes,&pKey->hRealRegistryKeyHandleIfNotEmulated, &Disposition);
                    M_SECURE_CATCH
                    free(pc);
                }
                RetValue = ERROR_FUNCTION_FAILED;
                // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                M_SECURE_TRY
                RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, Reserved,  lpClass, dwOptions, samDesired, lpSecurityAttributes,&pNewKey->hRealRegistryKeyHandleIfNotEmulated, lpdwDisposition);
                M_SECURE_CATCH
            }
        }
    }           
     
    return RetValue;
}
LONG WINAPI mRegCreateKeyT(CKeyReplace* pKey,TCHAR* lpSubKey,PHKEY phkResult)
{
    return mRegCreateKeyExT(pKey,lpSubKey,0,0,0,0,0,phkResult,0);
}
LONG WINAPI mRegCreateKeyA(HKEY hKey,LPCSTR lpSubKey,PHKEY phkResult,pfRegCreateKeyA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;

        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,phkResult);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,&psz);
    RetValue = mRegCreateKeyT(pKey,psz,phkResult);
    if (psz)
        free(psz);

    if (!phkResult)
        return RetValue;
    CKeyReplace* pNewKey = (CKeyReplace*)*phkResult;
    if (pKey && pNewKey)
    {
        // if key is not emulated, we have to get real registry informations
        // if we are in spy mode only we have get real informations too
        if  ( (!pNewKey->Emulated) || gRegistry_bSpyMode )
        {
            if (pNewKey->IsWriteAccessAllowed())
            {
                if (!pFunction)
                    return ERROR_FUNCTION_FAILED;

                if (!pKey->hRealRegistryKeyHandleIfNotEmulated)
                {
                    // we have to get real hkey even if not emulated

                    // get base key
                    HKEY hBaseKey = NULL;
                    CKeyReplace* pBaseKey = pKey->GetBaseKey();
                    if (pBaseKey)
                    {
                        hBaseKey = StringToBaseKey((TCHAR*)pBaseKey->KeyName.c_str());
                    }

                    // get reference key
                    std::tstring KeyNameRelativeToHost=_T("");
                    pKey->GetKeyPathWithoutBaseKeyName(KeyNameRelativeToHost);
                    // open or create key with same attributes
                    // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                    CHAR* pc;
                    CAnsiUnicodeConvert::TcharToAnsi(KeyNameRelativeToHost.c_str(),&pc);
                    M_SECURE_TRY
                    pFunction(hBaseKey, pc, &pKey->hRealRegistryKeyHandleIfNotEmulated);
                    M_SECURE_CATCH
                    free(pc);
                }
                RetValue = ERROR_FUNCTION_FAILED;
                // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                M_SECURE_TRY
                RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, &pNewKey->hRealRegistryKeyHandleIfNotEmulated);
                M_SECURE_CATCH
            }
        }
    } 


    return RetValue;
}
LONG WINAPI mRegCreateKeyW(HKEY hKey,LPCWSTR lpSubKey,PHKEY phkResult,pfRegCreateKeyW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
                        
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,phkResult);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,&psz);
    RetValue = mRegCreateKeyT(pKey,psz,phkResult);
    if (psz)
        free(psz);
    if (!phkResult)
        return RetValue;
    CKeyReplace* pNewKey = (CKeyReplace*)*phkResult;
    if (pKey && pNewKey)
    {
        // if key is not emulated, we have to get real registry informations
        // if we are in spy mode only we have get real informations too
        if  ( (!pNewKey->Emulated) || gRegistry_bSpyMode )
        {
            if (pNewKey->IsWriteAccessAllowed())
            {
                if (!pFunction)
                    return ERROR_FUNCTION_FAILED;

                if (!pKey->hRealRegistryKeyHandleIfNotEmulated)
                {
                    // we have to get real hkey even if not emulated

                    // get base key
                    HKEY hBaseKey = NULL;
                    CKeyReplace* pBaseKey = pKey->GetBaseKey();
                    if (pBaseKey)
                    {
                        hBaseKey = StringToBaseKey((TCHAR*)pBaseKey->KeyName.c_str());
                    }

                    // get reference key
                    std::tstring KeyNameRelativeToHost=_T("");
                    pKey->GetKeyPathWithoutBaseKeyName(KeyNameRelativeToHost);
                    // open or create key with same attributes
                    // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                    WCHAR* pc;
                    CAnsiUnicodeConvert::TcharToUnicode(KeyNameRelativeToHost.c_str(),&pc);
                    M_SECURE_TRY
                    pFunction(hBaseKey, pc, &pKey->hRealRegistryKeyHandleIfNotEmulated);
                    M_SECURE_CATCH
                    free(pc);
                }
                RetValue = ERROR_FUNCTION_FAILED;
                // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                M_SECURE_TRY
                RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, &pNewKey->hRealRegistryKeyHandleIfNotEmulated);
                M_SECURE_CATCH
            }
        }
    } 

    return RetValue;
}


LONG WINAPI mRegCreateKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,DWORD Reserved,LPSTR lpClass,DWORD dwOptions,REGSAM samDesired,const LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,HANDLE hTransaction,PVOID pExtendedParemeter,pfRegCreateKeyTransactedA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;

        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,Reserved,lpClass,dwOptions,samDesired,lpSecurityAttributes,phkResult,lpdwDisposition,hTransaction,pExtendedParemeter);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,&psz);
    RetValue = mRegCreateKeyT(pKey,psz,phkResult);
    if (psz)
        free(psz);

    if (!phkResult)
        return RetValue;
    CKeyReplace* pNewKey = (CKeyReplace*)*phkResult;
    if (pKey && pNewKey)
    {
        // if key is not emulated, we have to get real registry informations
        // if we are in spy mode only we have get real informations too
        if  ( (!pNewKey->Emulated) || gRegistry_bSpyMode )
        {
            if (pNewKey->IsWriteAccessAllowed())
            {
                if (!pFunction)
                    return ERROR_FUNCTION_FAILED;

                if (!pKey->hRealRegistryKeyHandleIfNotEmulated)
                {
                    // we have to get real hkey even if not emulated

                    // get base key
                    HKEY hBaseKey = NULL;
                    CKeyReplace* pBaseKey = pKey->GetBaseKey();
                    if (pBaseKey)
                    {
                        hBaseKey = StringToBaseKey((TCHAR*)pBaseKey->KeyName.c_str());
                    }

                    // get reference key
                    std::tstring KeyNameRelativeToHost=_T("");
                    pKey->GetKeyPathWithoutBaseKeyName(KeyNameRelativeToHost);
                    // open or create key with same attributes
                    // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                    CHAR* pc;
                    CAnsiUnicodeConvert::TcharToAnsi(KeyNameRelativeToHost.c_str(),&pc);
                    RetValue = ERROR_FUNCTION_FAILED;
                    M_SECURE_TRY
                    RetValue =pFunction( hBaseKey,pc,Reserved,lpClass,dwOptions,samDesired,lpSecurityAttributes,&pKey->hRealRegistryKeyHandleIfNotEmulated,lpdwDisposition,hTransaction,pExtendedParemeter);
                    M_SECURE_CATCH
                    free(pc);
                    return RetValue;
                }
                RetValue = ERROR_FUNCTION_FAILED;
                // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                M_SECURE_TRY
                RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, Reserved,lpClass,dwOptions,samDesired,lpSecurityAttributes,&pNewKey->hRealRegistryKeyHandleIfNotEmulated,lpdwDisposition,hTransaction,pExtendedParemeter);
                M_SECURE_CATCH
            }
        }
    } 

    return RetValue;
}
LONG WINAPI mRegCreateKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,DWORD Reserved,LPWSTR lpClass,DWORD dwOptions,REGSAM samDesired,const LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,HANDLE hTransaction,PVOID pExtendedParemeter,pfRegCreateKeyTransactedW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
                        
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,Reserved,lpClass,dwOptions,samDesired,lpSecurityAttributes,phkResult,lpdwDisposition,hTransaction,pExtendedParemeter);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,&psz);
    RetValue = mRegCreateKeyT(pKey,psz,phkResult);
    if (psz)
        free(psz);
    if (!phkResult)
        return RetValue;
    CKeyReplace* pNewKey = (CKeyReplace*)*phkResult;
    if (pKey && pNewKey)
    {
        // if key is not emulated, we have to get real registry informations
        // if we are in spy mode only we have get real informations too
        if  ( (!pNewKey->Emulated) || gRegistry_bSpyMode )
        {
            if (pNewKey->IsWriteAccessAllowed())
            {
                if (!pFunction)
                    return ERROR_FUNCTION_FAILED;

                if (!pKey->hRealRegistryKeyHandleIfNotEmulated)
                {
                    // we have to get real hkey even if not emulated

                    // get base key
                    HKEY hBaseKey = NULL;
                    CKeyReplace* pBaseKey = pKey->GetBaseKey();
                    if (pBaseKey)
                    {
                        hBaseKey = StringToBaseKey((TCHAR*)pBaseKey->KeyName.c_str());
                    }

                    // get reference key
                    std::tstring KeyNameRelativeToHost=_T("");
                    pKey->GetKeyPathWithoutBaseKeyName(KeyNameRelativeToHost);
                    // open or create key with same attributes
                    // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                    WCHAR* pc;
                    CAnsiUnicodeConvert::TcharToUnicode(KeyNameRelativeToHost.c_str(),&pc);
                    RetValue = ERROR_FUNCTION_FAILED;
                    M_SECURE_TRY
                    RetValue = pFunction( hBaseKey,pc,Reserved,lpClass,dwOptions,samDesired,lpSecurityAttributes,&pKey->hRealRegistryKeyHandleIfNotEmulated,lpdwDisposition,hTransaction,pExtendedParemeter);
                    M_SECURE_CATCH
                    free(pc);
                    return RetValue;
                }

                RetValue = ERROR_FUNCTION_FAILED;
                // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                M_SECURE_TRY
                RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, Reserved,lpClass,dwOptions,samDesired,lpSecurityAttributes,&pNewKey->hRealRegistryKeyHandleIfNotEmulated,lpdwDisposition,hTransaction,pExtendedParemeter);
                M_SECURE_CATCH
            }
        }
    } 

    return RetValue;
}

LONG WINAPI mRegDeleteKeyT(CKeyReplace* pParentKey,TCHAR* lpSubKey,OUT HKEY* phRealKeyToDelete,BOOL bDeleteTree)
{
    *phRealKeyToDelete=NULL;
    if (!lpSubKey)
        return ERROR_INVALID_PARAMETER;
    if (!pParentKey)
        return ERROR_FUNCTION_FAILED;
    CKeyReplace* pCurrentKey = pParentKey; // if lpSubKey is empty
    if (*lpSubKey !=0 )// if lpSubKey is specified
        pCurrentKey = pParentKey->GetSubKeyWithFullPath(lpSubKey);
    if (!pCurrentKey)
        return ERROR_FUNCTION_FAILED;
    if (!pCurrentKey->Emulated)
    {
        if (pCurrentKey->IsWriteAccessAllowed())
            *phRealKeyToDelete = pParentKey->hRealRegistryKeyHandleIfNotEmulated;
    }

    // if tree deletion is not queried, check if key contains subkeys
    if (!bDeleteTree)
    {
        // msdn : A deleted key is not removed until the last handle to it has been closed.
        // The subkey to be deleted must not have subkeys. To delete a key and all its subkeys, you need to recursively enumerate the subkeys and delete them individually.
        if (pCurrentKey->GetSubKeysCount() > 0)
            return ERROR_FUNCTION_FAILED;
    }

    // we musn't removed key as key handle will still remain valid until RegCloseKey is called with it,
    // but key content must become inaccessible
    pCurrentKey->MarkKeyForDeletion(bDeleteTree);
    return ERROR_SUCCESS;
}

LONG WINAPI mRegDeleteKeyA(HKEY hKey,LPCSTR lpSubKey,pfRegDeleteKeyA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    HKEY hRealKeyToDelete;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,&psz);
    RetValue = mRegDeleteKeyT(pKey,psz,&hRealKeyToDelete,FALSE);
    if (psz)
        free(psz);
        
    if (hRealKeyToDelete)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        RetValue = ERROR_FUNCTION_FAILED;
        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKeyToDelete,lpSubKey);
        M_SECURE_CATCH
    }    
    return RetValue;
}
LONG WINAPI mRegDeleteKeyW(HKEY hKey,LPCWSTR lpSubKey,pfRegDeleteKeyW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
                
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    HKEY hRealKeyToDelete;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,&psz);
    RetValue = mRegDeleteKeyT(pKey,psz,&hRealKeyToDelete,FALSE);
    if (psz)
        free(psz);
        
    if (hRealKeyToDelete)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        RetValue = ERROR_FUNCTION_FAILED;
        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKeyToDelete,lpSubKey);
        M_SECURE_CATCH
    }    
    return RetValue;
}


LONG WINAPI mRegDeleteKeyExA(HKEY hKey,LPCSTR lpSubKey,REGSAM samDesired,DWORD Reserved,pfRegDeleteKeyExA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,samDesired,Reserved);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    HKEY hRealKeyToDelete;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,&psz);
    RetValue = mRegDeleteKeyT(pKey,psz,&hRealKeyToDelete,FALSE);
    if (psz)
        free(psz);
        
    if (hRealKeyToDelete)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        RetValue = ERROR_FUNCTION_FAILED;
        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKeyToDelete,lpSubKey,samDesired,Reserved);
        M_SECURE_CATCH
    }    
    return RetValue;
}
LONG WINAPI mRegDeleteKeyExW(HKEY hKey,LPCWSTR lpSubKey,REGSAM samDesired,DWORD Reserved,pfRegDeleteKeyExW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
                
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,samDesired,Reserved);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    HKEY hRealKeyToDelete;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,&psz);
    RetValue = mRegDeleteKeyT(pKey,psz,&hRealKeyToDelete,FALSE);
    if (psz)
        free(psz);
        
    if (hRealKeyToDelete)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        RetValue = ERROR_FUNCTION_FAILED;
        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKeyToDelete,lpSubKey,samDesired,Reserved);
        M_SECURE_CATCH
    }    
    return RetValue;
}
LONG WINAPI mRegDeleteKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,REGSAM samDesired,DWORD Reserved,HANDLE hTransaction,PVOID pExtendedParameter,pfRegDeleteKeyTransactedA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,samDesired,Reserved,hTransaction,pExtendedParameter);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    HKEY hRealKeyToDelete;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,&psz);
    RetValue = mRegDeleteKeyT(pKey,psz,&hRealKeyToDelete,FALSE);
    if (psz)
        free(psz);
        
    if (hRealKeyToDelete)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        RetValue = ERROR_FUNCTION_FAILED;
        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKeyToDelete,lpSubKey,samDesired,Reserved,hTransaction,pExtendedParameter);
        M_SECURE_CATCH
    }    
    return RetValue;
}
LONG WINAPI mRegDeleteKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,REGSAM samDesired,DWORD Reserved,HANDLE hTransaction,PVOID pExtendedParameter,pfRegDeleteKeyTransactedW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
                
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,samDesired,Reserved,hTransaction,pExtendedParameter);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    HKEY hRealKeyToDelete;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,&psz);
    RetValue = mRegDeleteKeyT(pKey,psz,&hRealKeyToDelete,FALSE);
    if (psz)
        free(psz);
        
    if (hRealKeyToDelete)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED;
        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKeyToDelete,lpSubKey,samDesired,Reserved,hTransaction,pExtendedParameter);
        M_SECURE_CATCH
    }    
    return RetValue;
}

LONG WINAPI mRegDeleteTreeA(HKEY hKey,LPCSTR lpSubKey,pfRegDeleteTreeA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    HKEY hRealKeyToDelete;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,&psz);
    RetValue = mRegDeleteKeyT(pKey,psz,&hRealKeyToDelete,TRUE);
    if (psz)
        free(psz);
        
    if (hRealKeyToDelete)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKeyToDelete,lpSubKey);
        M_SECURE_CATCH
    }    
    return RetValue;
}
LONG WINAPI mRegDeleteTreeW(HKEY hKey,LPCWSTR lpSubKey,pfRegDeleteTreeW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
                
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    HKEY hRealKeyToDelete;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,&psz);
    RetValue = mRegDeleteKeyT(pKey,psz,&hRealKeyToDelete,TRUE);
    if (psz)
        free(psz);
        
    if (hRealKeyToDelete)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED;
        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKeyToDelete,lpSubKey);
        M_SECURE_CATCH
    }    
    return RetValue;
}

LONG WINAPI mRegDeleteValueT(CKeyReplace* pCurrentKey,TCHAR* lpValueName,OUT HKEY* pRealKeyToDeleteValue)
{
    *pRealKeyToDeleteValue = NULL;
    if (!lpValueName)
        return ERROR_INVALID_PARAMETER;
    if (!pCurrentKey)
        return ERROR_FUNCTION_FAILED;
        
    if (!pCurrentKey->Emulated)
    {
        if (pCurrentKey->IsWriteAccessAllowed())
            *pRealKeyToDeleteValue = pCurrentKey->hRealRegistryKeyHandleIfNotEmulated;
    }        
        
    if (pCurrentKey->RemoveValue(lpValueName))
        return ERROR_SUCCESS;
    return ERROR_FUNCTION_FAILED;
}

LONG WINAPI mRegDeleteValueA(HKEY hKey,LPCSTR lpValueName,pfRegDeleteValueA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpValueName);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    HKEY hRealKeyToDeleteValue;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::AnsiToTchar(lpValueName,&psz);
    RetValue = mRegDeleteValueT(pKey,psz,&hRealKeyToDeleteValue);
    if (psz)
        free(psz);
        
    if (hRealKeyToDeleteValue)// hRealKey is set only if write operations are available on registry
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKeyToDeleteValue,lpValueName);
        M_SECURE_CATCH
    }    
    return RetValue;
}
LONG WINAPI mRegDeleteValueW(HKEY hKey,LPCWSTR lpValueName,pfRegDeleteValueW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpValueName);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    HKEY hRealKeyToDeleteValue;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::UnicodeToTchar(lpValueName,&psz);
    RetValue = mRegDeleteValueT(pKey,psz,&hRealKeyToDeleteValue);
    if (psz)
        free(psz);
        
    if (hRealKeyToDeleteValue)// hRealKey is set only if write operations are available on registry
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKeyToDeleteValue,lpValueName);
        M_SECURE_CATCH
    }
        
    return RetValue;
}

LONG WINAPI mRegDeleteKeyValueA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpValueName,pfRegDeleteKeyValueA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,lpValueName);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    BOOL bKeyWasExisting = FALSE;
    TCHAR* pszSubKey;
    CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,&pszSubKey);
    CKeyReplace* pChildKey = pKey->GetOrAddSubKeyWithFullPath(pszSubKey,&bKeyWasExisting);       
    free(pszSubKey);

    HKEY hRealKeyToDeleteValue;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::AnsiToTchar(lpValueName,&psz);
    RetValue = mRegDeleteValueT(pChildKey,psz,&hRealKeyToDeleteValue);
    if (psz)
        free(psz);
        
    hRealKeyToDeleteValue = NULL;
    if (!pChildKey->Emulated)
    {
        if (pChildKey->IsWriteAccessAllowed())
            hRealKeyToDeleteValue = pKey->hRealRegistryKeyHandleIfNotEmulated;
    }        
        
    if (hRealKeyToDeleteValue)// hRealKey is set only if write operations are available on registry
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKeyToDeleteValue,lpSubKey,lpValueName);
        M_SECURE_CATCH
    }   
    return RetValue;
}
LONG WINAPI mRegDeleteKeyValueW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpValueName,pfRegDeleteKeyValueW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,lpValueName);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    BOOL bKeyWasExisting = FALSE;
    TCHAR* pszSubKey;
    CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,&pszSubKey);
    CKeyReplace* pChildKey = pKey->GetOrAddSubKeyWithFullPath(pszSubKey,&bKeyWasExisting);       
    free(pszSubKey);

    HKEY hRealKeyToDeleteValue;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::UnicodeToTchar(lpValueName,&psz);
    RetValue = mRegDeleteValueT(pChildKey,psz,&hRealKeyToDeleteValue);
    if (psz)
        free(psz);
       
    hRealKeyToDeleteValue = NULL;
    if (!pChildKey->Emulated)
    {
        if (pChildKey->IsWriteAccessAllowed())
            hRealKeyToDeleteValue = pKey->hRealRegistryKeyHandleIfNotEmulated;
    }        
        
    if (hRealKeyToDeleteValue)// hRealKey is set only if write operations are available on registry
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKeyToDeleteValue,lpSubKey,lpValueName);
        M_SECURE_CATCH
    }
        
    return RetValue;
}

LONG WINAPI mRegSetValueExT(CKeyReplace* pCurrentKey,TCHAR* lpValueName,DWORD Reserved,DWORD dwType,const BYTE* lpData,DWORD cbData,OUT HKEY* pRealKey)
{
    *pRealKey = NULL;
    if (!lpValueName)
        return ERROR_INVALID_PARAMETER;

    if (!pCurrentKey)
        return ERROR_FUNCTION_FAILED;
        
    if (!pCurrentKey->Emulated)
    {
        if (pCurrentKey->IsWriteAccessAllowed())
            *pRealKey = pCurrentKey->hRealRegistryKeyHandleIfNotEmulated;
    }  
        
    if (pCurrentKey->SetValue(lpValueName,dwType,(PBYTE)lpData,cbData))
        return ERROR_SUCCESS;
    return ERROR_FUNCTION_FAILED;
}
LONG WINAPI mRegSetValueExA(HKEY hKey,LPCSTR lpValueName,DWORD Reserved,DWORD dwType,const BYTE* lpData,DWORD cbData,pfRegSetValueExA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction(hKey,lpValueName, Reserved, dwType, lpData, cbData);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    HKEY hRealKey;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::AnsiToTchar(lpValueName,&psz);
    RetValue = mRegSetValueExT(pKey,psz,Reserved,dwType,lpData,cbData,&hRealKey);
    if (psz)
        free(psz);
        
    if (hRealKey) // hRealKey is set only if write operations are available on registry
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKey,lpValueName, Reserved, dwType, lpData, cbData);
        M_SECURE_CATCH

        // in case of success, if we are in spying mode, put value in emulated registry
        if (gRegistry_bSpyMode)
        {
            CAnsiUnicodeConvert::AnsiToTchar(lpValueName,&psz);
            if (psz)
            {
                pKey->SetValue(psz,(SIZE_T)dwType,(PBYTE)lpData,(SIZE_T)cbData,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_WRITE);
                free(psz);
            }
        }
    }        
    return RetValue;
}
LONG WINAPI mRegSetValueExW(HKEY hKey,LPCWSTR lpValueName,DWORD Reserved,DWORD dwType,const BYTE* lpData,DWORD cbData,pfRegSetValueExW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction(hKey,lpValueName, Reserved, dwType, lpData, cbData);
        M_SECURE_CATCH

        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    HKEY hRealKey;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::UnicodeToTchar(lpValueName,&psz);
    RetValue = mRegSetValueExT(pKey,psz,Reserved,dwType,lpData,cbData,&hRealKey);
    if (psz)
        free(psz);
        
    if (hRealKey)// hRealKey is set only if write operations are available on registry
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKey,lpValueName, Reserved, dwType, lpData, cbData);
        M_SECURE_CATCH

        // in case of success, if we are in spying mode, put value in emulated registry
        if (gRegistry_bSpyMode)
        {
            CAnsiUnicodeConvert::UnicodeToTchar(lpValueName,&psz);
            if (psz)
            {
                pKey->SetValue(psz,(SIZE_T)dwType,(PBYTE)lpData,(SIZE_T)cbData,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_WRITE);
                free(psz);
            }
        }
    }    
    return RetValue;
}

LONG WINAPI mRegSetKeyValueA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpValueName,DWORD dwType,LPCVOID lpData,DWORD cbData,pfRegSetKeyValueA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    TCHAR* psz;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction(hKey,lpSubKey,lpValueName, dwType, lpData, cbData);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,&psz);
    CKeyReplace* pCurrentKey = pKey->GetOrAddSubKeyWithFullPath(psz);
    if (psz)
        free(psz);
    if (!pCurrentKey)
        return ERROR_FUNCTION_FAILED;       
    
    HKEY hRealKey;
    LONG RetValue;
    CAnsiUnicodeConvert::AnsiToTchar(lpValueName,&psz);
    RetValue = mRegSetValueExT(pCurrentKey,psz,0,dwType,(BYTE*)lpData,cbData,&hRealKey);
    if (psz)
        free(psz);
        
    if (hRealKey) // hRealKey is set only if write operations are available on registry
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKey,"",lpValueName, dwType, lpData, cbData);
        M_SECURE_CATCH

        // in case of success, if we are in spying mode, put value in emulated registry
        if (gRegistry_bSpyMode)
        {
            CAnsiUnicodeConvert::AnsiToTchar(lpValueName,&psz);
            if (psz)
            {
                pCurrentKey->SetValue(psz,(SIZE_T)dwType,(PBYTE)lpData,(SIZE_T)cbData,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_WRITE);
                free(psz);
            }
        }
    }        
    return RetValue;
}
LONG WINAPI mRegSetKeyValueW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpValueName,DWORD dwType,LPCVOID lpData,DWORD cbData,pfRegSetKeyValueW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    TCHAR* psz;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction(hKey,lpSubKey,lpValueName, dwType, lpData, cbData);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,&psz);
    CKeyReplace* pCurrentKey = pKey->GetOrAddSubKeyWithFullPath(psz);
    if (psz)
        free(psz);
    if (!pCurrentKey)
        return ERROR_FUNCTION_FAILED;    
    
    HKEY hRealKey;
    LONG RetValue;
    CAnsiUnicodeConvert::UnicodeToTchar(lpValueName,&psz);
    RetValue = mRegSetValueExT(pCurrentKey,psz,0,dwType,(BYTE*)lpData,cbData,&hRealKey);
    if (psz)
        free(psz);
        
    if (hRealKey)// hRealKey is set only if write operations are available on registry
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        // hRealKey already points to subkey
        M_SECURE_TRY
        RetValue = pFunction(hRealKey,L"",lpValueName, dwType, lpData, cbData);
        M_SECURE_CATCH

        // in case of success, if we are in spying mode, put value in emulated registry
        if (gRegistry_bSpyMode)
        {
            CAnsiUnicodeConvert::UnicodeToTchar(lpValueName,&psz);
            if (psz)
            {
                pCurrentKey->SetValue(psz,(SIZE_T)dwType,(PBYTE)lpData,(SIZE_T)cbData,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_WRITE);
                free(psz);
            }
        }
    }    
    return RetValue;
}

LONG WINAPI mRegSetValueT(CKeyReplace* pParentKey,TCHAR* lpSubKey,DWORD dwType,PBYTE lpData,DWORD cbData, CKeyReplace** ppCurrentKey,HKEY* phRealKey)
{
    *ppCurrentKey =NULL;
    if (!lpSubKey)
        lpSubKey=_T("");
    if (dwType != REG_SZ)
        return ERROR_INVALID_PARAMETER;
    if (!pParentKey)
        return ERROR_FUNCTION_FAILED;
    *ppCurrentKey = pParentKey->GetOrAddSubKeyWithFullPath(lpSubKey);
    if (!*ppCurrentKey)
        return ERROR_FUNCTION_FAILED;
    
    return mRegSetValueExT(*ppCurrentKey,_T(""),0,dwType,lpData,cbData,phRealKey);
}
LONG WINAPI mRegSetValueA(HKEY hKey,LPCSTR lpSubKey,DWORD dwType,LPCSTR lpData,DWORD cbData,pfRegSetValueA pFunction)
{
    HKEY hRealKey;
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pCurrentKey;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction(hKey,lpSubKey, dwType,lpData,cbData); 
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,&psz);
    RetValue = mRegSetValueT(pKey,psz,dwType,(PBYTE)lpData,cbData+sizeof(CHAR),&pCurrentKey,&hRealKey); // +sizeof(CHAR) : MSDN : size of string not including \0
    if (psz)
        free(psz);

    if (hRealKey)// hRealKey is set only if write operations are available on registry
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        // hRealKey already points to subkey
        M_SECURE_TRY
        RetValue = pFunction(hRealKey,"", dwType,lpData,cbData);
        M_SECURE_CATCH

        // in case of success, if we are in spying mode, put value in emulated registry
        if (gRegistry_bSpyMode && pCurrentKey)
        {
            pCurrentKey->SetValue(_T(""),(SIZE_T)dwType,(PBYTE)lpData,(SIZE_T)cbData,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_WRITE);
        }
    }   
    return RetValue;
}
LONG WINAPI mRegSetValueW(HKEY hKey,LPCWSTR lpSubKey,DWORD dwType,LPCWSTR lpData,DWORD cbData,pfRegSetValueW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pCurrentKey;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
            
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction(hKey,lpSubKey, dwType,lpData,cbData); 
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    LONG RetValue;
    HKEY hRealKey;
    TCHAR* psz;
    CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,&psz);
    RetValue = mRegSetValueT(pKey,psz,dwType,(PBYTE)lpData,cbData+sizeof(WCHAR),&pCurrentKey,&hRealKey);// +sizeof(WCHAR) : MSDN : size of string not including \0
    if (psz)
        free(psz);

    if (hRealKey)// hRealKey is set only if write operations are available on registry
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        // hRealKey already points to subkey
        M_SECURE_TRY
        RetValue = pFunction(hRealKey,L"", dwType,lpData,cbData);
        M_SECURE_CATCH

        // in case of success, if we are in spying mode, put value in emulated registry
        if (gRegistry_bSpyMode && pCurrentKey)
        {
            pCurrentKey->SetValue(_T(""),(SIZE_T)dwType,(PBYTE)lpData,(SIZE_T)cbData,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_WRITE);
        }
    }
    return RetValue;
}

LONG WINAPI mRegQueryValueExT(CKeyReplace* pCurrentKey,TCHAR* lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData)
{
    if (lpcbData==NULL)
        return ERROR_INVALID_PARAMETER;
    if (!pCurrentKey)
        return ERROR_FUNCTION_FAILED;
        
    CKeyValue* pValue = pCurrentKey->GetValue(lpValueName);
    if (!pValue)
        return ERROR_FILE_NOT_FOUND;

    if (lpType)
        *lpType = (DWORD)(pValue->Type);
    if (lpData == NULL) // MSDN : specific case to get content size
    {
        *lpcbData = (DWORD)(pValue->BufferSize);
        return ERROR_SUCCESS;
    }
    if (*lpcbData<pValue->BufferSize)
    {
        *lpcbData = (DWORD)(pValue->BufferSize);
        return ERROR_MORE_DATA;
    }
    if (::IsBadWritePtr(lpData,pValue->BufferSize))
        return ERROR_INVALID_PARAMETER;

    *lpcbData = (DWORD)(pValue->BufferSize);
    if ( (pValue->BufferSize>0) && pValue->Buffer)
        memcpy(lpData,pValue->Buffer,pValue->BufferSize);
    return ERROR_SUCCESS;
}
LONG WINAPI mRegQueryValueExA(HKEY hKey,LPCSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData,pfRegQueryValueExA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction(hKey,lpValueName, lpReserved, lpType, lpData, lpcbData); 
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }

    LONG RetValue;
    TCHAR* psz;

    if (!pKey->Emulated)
    {
        HKEY hRealKey;
        hRealKey = pKey->hRealRegistryKeyHandleIfNotEmulated;

        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKey,lpValueName, lpReserved, lpType, lpData, lpcbData);
        M_SECURE_CATCH

        // in case of success, if we are in spying mode, put value in emulated registry
        if (gRegistry_bSpyMode)
        {
            CAnsiUnicodeConvert::AnsiToTchar(lpValueName,&psz);
            if (psz)
            {
                SIZE_T Type = REG_NONE;
                SIZE_T cbData = 0;
                if (lpType)
                    Type = *lpType;
                if (lpcbData)
                    cbData = *lpcbData;
                pKey->SetValue(psz,Type,lpData,cbData,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_READ);
                free(psz);
            }
        }
        return RetValue;
    }

    // else : key emulated
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;

    CAnsiUnicodeConvert::AnsiToTchar(lpValueName,&psz);
    if (!psz)
        return ERROR_FUNCTION_FAILED;
    RetValue = mRegQueryValueExT(pKey,psz,lpReserved,lpType,lpData,lpcbData);
    free(psz);

    return RetValue;
}
LONG WINAPI mRegQueryValueExW(HKEY hKey,LPCWSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData,pfRegQueryValueExW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction(hKey,lpValueName, lpReserved, lpType, lpData, lpcbData); 
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }

    LONG RetValue;
    TCHAR* psz;
    if (!pKey->Emulated)
    {
        HKEY hRealKey;
        hRealKey = pKey->hRealRegistryKeyHandleIfNotEmulated;

        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(hRealKey,lpValueName, lpReserved, lpType, lpData, lpcbData);
        M_SECURE_CATCH

        // in case of success, if we are in spying mode, put value in emulated registry
        if (gRegistry_bSpyMode)
        {
            CAnsiUnicodeConvert::UnicodeToTchar(lpValueName,&psz);
            if (psz)
            {
                SIZE_T Type = REG_NONE;
                SIZE_T cbData = 0;
                if (lpType)
                    Type = *lpType;
                if (lpcbData)
                    cbData = *lpcbData;
                pKey->SetValue(psz,Type,lpData,cbData,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_READ);
                free(psz);
            }
        }

        return RetValue;
    }

    // else : key emulated
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    
    CAnsiUnicodeConvert::UnicodeToTchar(lpValueName,&psz);
    if (!psz)
        return ERROR_FUNCTION_FAILED;
    RetValue = mRegQueryValueExT(pKey,psz,lpReserved,lpType,lpData,lpcbData);
    free(psz);

    return RetValue;
}

LONG WINAPI mRegGetValueA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpValue,DWORD dwFlags,LPDWORD pdwType,PVOID pvData,LPDWORD pcbData,pfRegGetValueA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    TCHAR* psz;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction(hKey,lpSubKey,lpValue,dwFlags, pdwType, pvData, pcbData); 
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,&psz);
    CKeyReplace* pCurrentKey = pKey->GetOrAddSubKeyWithFullPath(psz);
    if (psz)
        free(psz);
    if (!pCurrentKey)
        return ERROR_FUNCTION_FAILED;       
    
    LONG RetValue;
        
    if (!pCurrentKey->Emulated)
    {
        HKEY hRealKey;
        hRealKey = pCurrentKey->hRealRegistryKeyHandleIfNotEmulated;

        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        // warning hRealKey is already the subkey
        M_SECURE_TRY
        RetValue = pFunction(hRealKey,"",lpValue,dwFlags, pdwType, pvData, pcbData); 
        M_SECURE_CATCH

        // in case of success, if we are in spying mode, put value in emulated registry
        if (gRegistry_bSpyMode)
        {
            CAnsiUnicodeConvert::AnsiToTchar(lpValue,&psz);
            if (psz)
            {
                SIZE_T Type = REG_NONE;
                SIZE_T cbData = 0;
                if (pdwType)
                    Type = *pdwType;
                if (pcbData)
                    cbData = *pcbData;
                pCurrentKey->SetValue(psz,Type,(PBYTE)pvData,cbData,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_READ);
                free(psz);
            }
        }
        return RetValue;
    }
    // else : emulated
    if (pCurrentKey->IsKeyMarkedForDeletion())
        return ERROR_KEY_DELETED;

    CAnsiUnicodeConvert::AnsiToTchar(lpValue,&psz);
    // warning psz can be null
    RetValue = mRegQueryValueExT(pCurrentKey,psz,0,pdwType,(LPBYTE)pvData,pcbData);
    if (psz)
        free(psz);

    return RetValue;
}
LONG WINAPI mRegGetValueW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpValue,DWORD dwFlags,LPDWORD pdwType,PVOID pvData,LPDWORD pcbData,pfRegGetValueW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    TCHAR* psz;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction(hKey,lpSubKey,lpValue, dwFlags, pdwType,pvData, pcbData); 
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,&psz);
    CKeyReplace* pCurrentKey = pKey->GetOrAddSubKeyWithFullPath(psz);
    if (psz)
        free(psz);
    if (!pCurrentKey)
        return ERROR_FUNCTION_FAILED;    
    
    LONG RetValue;

    if (!pCurrentKey->Emulated)
    {
        HKEY hRealKey;
        hRealKey = pCurrentKey->hRealRegistryKeyHandleIfNotEmulated;

        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        // warning hRealKey is already the subkey
        M_SECURE_TRY
        RetValue = pFunction(hRealKey,L"",lpValue, dwFlags, pdwType, pvData, pcbData);
        M_SECURE_CATCH

        // in case of success, if we are in spying mode, put value in emulated registry
        if (gRegistry_bSpyMode)
        {
            CAnsiUnicodeConvert::UnicodeToTchar(lpValue,&psz);
            if (psz)
            {
                SIZE_T Type = REG_NONE;
                SIZE_T cbData = 0;
                if (pdwType)
                    Type = *pdwType;
                if (pcbData)
                    cbData = *pcbData;
                pCurrentKey->SetValue(psz,Type,(PBYTE)pvData,cbData,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_READ);
                free(psz);
            }
        }
        return RetValue;
    }
    // else : emulated
    if (pCurrentKey->IsKeyMarkedForDeletion())
        return ERROR_KEY_DELETED;

    CAnsiUnicodeConvert::UnicodeToTchar(lpValue,&psz);
    // warning psz can be null
    RetValue = mRegQueryValueExT(pCurrentKey,psz,0,pdwType,(LPBYTE)pvData,pcbData);
    if (psz)
        free(psz);

    return RetValue;
}

LONG WINAPI mRegQueryValueA(HKEY hKey,LPCSTR lpSubKey,LPSTR lpValue,PLONG lpcbValue,pfRegQueryValueA pFunction)
{
    LONG RetValue;
    TCHAR* psz;
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pCurrentKey;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction(hKey,lpSubKey, lpValue,lpcbValue); 
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }

    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;

    CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,&psz);
    pCurrentKey = pKey->GetOrAddSubKeyWithFullPath(psz);
    if (psz)
        free(psz);
    if (!pCurrentKey)
        return ERROR_FUNCTION_FAILED;  

    if (!pCurrentKey->Emulated)
    {
        HKEY hRealKey;
        hRealKey = pCurrentKey->hRealRegistryKeyHandleIfNotEmulated;

        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        // hRealKey already points to subkey
        M_SECURE_TRY
        RetValue = pFunction(hRealKey,"", lpValue,lpcbValue);
        M_SECURE_CATCH

        // in case of success, if we are in spying mode, put value in emulated registry
        if (gRegistry_bSpyMode)
        {
            CAnsiUnicodeConvert::AnsiToTchar(lpValue,&psz);
            if (psz)
            {
                SIZE_T cbData = 0;
                if (lpcbValue)
                    cbData = *lpcbValue;
                pCurrentKey->SetValue(psz,(SIZE_T)REG_SZ,(PBYTE)psz,cbData,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_READ);
                free(psz);
            }
        }
        return RetValue;
    }  

    // else : emulated
    if (pCurrentKey->IsKeyMarkedForDeletion())
        return ERROR_KEY_DELETED;

    RetValue = mRegQueryValueExT(pCurrentKey,_T(""),NULL,NULL,(PBYTE)lpValue,(LPDWORD)lpcbValue);

    return RetValue;
}
LONG WINAPI mRegQueryValueW(HKEY hKey,LPCWSTR lpSubKey,LPWSTR lpValue,PLONG lpcbValue,pfRegQueryValueW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pCurrentKey;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction(hKey,lpSubKey, lpValue,lpcbValue); 
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;

    LONG RetValue;
    TCHAR* psz;
    CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,&psz);
    pCurrentKey = pKey->GetOrAddSubKeyWithFullPath(psz);
    if (psz)
        free(psz);

    if (!pCurrentKey)
        return ERROR_FUNCTION_FAILED; 

    if (!pCurrentKey->Emulated)
    {
        HKEY hRealKey;
        hRealKey = pCurrentKey->hRealRegistryKeyHandleIfNotEmulated;

        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        // hRealKey already points to subkey
        M_SECURE_TRY
        RetValue = pFunction(hRealKey,L"", lpValue,lpcbValue);
        M_SECURE_CATCH

        // in case of success, if we are in spying mode, put value in emulated registry
        if (gRegistry_bSpyMode)
        {
            CAnsiUnicodeConvert::UnicodeToTchar(lpValue,&psz);
            if (psz)
            {
                SIZE_T cbData = 0;
                if (lpcbValue)
                    cbData = *lpcbValue;
                pCurrentKey->SetValue(psz,(SIZE_T)REG_SZ,(PBYTE)psz,cbData,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_READ);
                free(psz);
            }
        }
        return RetValue;
    } 
    // else : emulated
    if (pCurrentKey->IsKeyMarkedForDeletion())
        return ERROR_KEY_DELETED;

    RetValue = mRegQueryValueExT(pCurrentKey,_T(""),NULL,NULL,(PBYTE)lpValue,(LPDWORD)lpcbValue);
    return RetValue;
}

LONG WINAPI mRegQueryMultipleValuesA(HKEY hKey,PVALENTA val_list,DWORD num_vals,LPSTR lpValueBuf,LPDWORD ldwTotsize,pfRegQueryMultipleValuesA pFunction,pfRegQueryValueExA pRegQueryValueExA)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    LONG RetValue = ERROR_FUNCTION_FAILED;
    PVALENTA pCurrentVal;
    DWORD Cnt;

    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction(hKey,val_list,num_vals,lpValueBuf,ldwTotsize);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;

    if (!pKey->Emulated)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, val_list, num_vals, lpValueBuf, ldwTotsize);
        M_SECURE_CATCH

        //The function fails if any of the specified values do not exist in the specified key. 
        //If the function succeeds, each element of the array contains the information for the specified value.
        if (gRegistry_bSpyMode)
        {
            TCHAR* psz;
            for (Cnt = 0; Cnt<num_vals; Cnt++)
            {
                pCurrentVal = &val_list[Cnt];

                CAnsiUnicodeConvert::AnsiToTchar(pCurrentVal->ve_valuename,&psz);
                if (psz)
                {
                    pKey->SetValue(psz,(SIZE_T)pCurrentVal->ve_type,(PBYTE)pCurrentVal->ve_valueptr,pCurrentVal->ve_valuelen,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_READ);
                    free(psz);
                }
            }
        }
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    if (ldwTotsize == NULL)
        return ERROR_INVALID_PARAMETER;

    DWORD RequiredSize = 0;

    // clean all pointers
    for (Cnt = 0; Cnt<num_vals; Cnt++)
    {
        pCurrentVal = &val_list[Cnt];
        pCurrentVal->ve_valueptr = NULL;
    }

    for (Cnt = 0; Cnt<num_vals; Cnt++)
    {
        pCurrentVal = &val_list[Cnt];

        mRegQueryValueExA(hKey,pCurrentVal->ve_valuename,NULL,&pCurrentVal->ve_type,0,&pCurrentVal->ve_valuelen,pRegQueryValueExA);
        RequiredSize+=pCurrentVal->ve_valuelen;
        if (lpValueBuf) // earn time if only for size retrieving
        {
            pCurrentVal->ve_valueptr =(DWORD_PTR) new BYTE[pCurrentVal->ve_valuelen];
            mRegQueryValueExA(hKey,pCurrentVal->ve_valuename,NULL,&pCurrentVal->ve_type,(PBYTE)pCurrentVal->ve_valueptr,&pCurrentVal->ve_valuelen,pRegQueryValueExA);
        }
    }

    if (*ldwTotsize < RequiredSize)
    {
        *ldwTotsize = RequiredSize;
        if (lpValueBuf)
        {
            RetValue = ERROR_MORE_DATA;
        }
        else
            RetValue = ERROR_SUCCESS;
        goto CleanUp;
    }

    if (::IsBadWritePtr(lpValueBuf,RequiredSize))
    {
        RetValue = ERROR_INVALID_PARAMETER;
        goto CleanUp;
    }

    DWORD Index;
    for (Cnt = 0,Index = 0; Cnt<num_vals; Cnt++)
    {
        pCurrentVal = &val_list[Cnt];

        // copy temporary buffer to output buffer
        memcpy(&lpValueBuf[Index],(PBYTE)pCurrentVal->ve_valueptr,pCurrentVal->ve_valuelen);

        // free temporary buffer
        delete[] (PBYTE)pCurrentVal->ve_valueptr;

        // update pointer value to point inside output buffer
        pCurrentVal->ve_valueptr = (DWORD_PTR)  ( (PBYTE)lpValueBuf + Index);

        // increase output buffer index
        Index+=pCurrentVal->ve_valuelen;
    }

    return ERROR_SUCCESS;

CleanUp:
    // clean all pointers
    for (Cnt = 0; Cnt<num_vals; Cnt++)
    {
        pCurrentVal = &val_list[Cnt];
        if ( pCurrentVal->ve_valueptr)
            delete[] (PBYTE)pCurrentVal->ve_valueptr;
        pCurrentVal->ve_valueptr = NULL;
    }
    return RetValue;
}
LONG WINAPI mRegQueryMultipleValuesW(HKEY hKey,PVALENTW val_list,DWORD num_vals,LPWSTR lpValueBuf,LPDWORD ldwTotsize,pfRegQueryMultipleValuesW pFunction,pfRegQueryValueExW pRegQueryValueExW)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    LONG RetValue;
    PVALENTW pCurrentVal;
    DWORD Cnt;

    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction( hKey, val_list, num_vals, lpValueBuf, ldwTotsize);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;

    if (!pKey->Emulated)
    {
        RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue =  pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, val_list, num_vals, lpValueBuf, ldwTotsize);
        M_SECURE_CATCH

        //The function fails if any of the specified values do not exist in the specified key. 
        //If the function succeeds, each element of the array contains the information for the specified value.
        if (gRegistry_bSpyMode)
        {
            TCHAR* psz;
            for (Cnt = 0; Cnt<num_vals; Cnt++)
            {
                pCurrentVal = &val_list[Cnt];

                CAnsiUnicodeConvert::UnicodeToTchar(pCurrentVal->ve_valuename,&psz);
                if (psz)
                {
                    pKey->SetValue(psz,(SIZE_T)pCurrentVal->ve_type,(PBYTE)pCurrentVal->ve_valueptr,pCurrentVal->ve_valuelen,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_READ);
                    free(psz);
                }
            }
        }

        return RetValue;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;     
    if (ldwTotsize == NULL)
        return ERROR_INVALID_PARAMETER;

    RetValue = ERROR_FUNCTION_FAILED;
    DWORD RequiredSize = 0;

    // clean all pointers
    for (Cnt = 0; Cnt<num_vals; Cnt++)
    {
        pCurrentVal = &val_list[Cnt];
        pCurrentVal->ve_valueptr = NULL;
    }

    for (Cnt = 0; Cnt<num_vals; Cnt++)
    {
        pCurrentVal = &val_list[Cnt];

        mRegQueryValueExW(hKey,pCurrentVal->ve_valuename,NULL,&pCurrentVal->ve_type,0,&pCurrentVal->ve_valuelen,pRegQueryValueExW);
        RequiredSize+=pCurrentVal->ve_valuelen;
        if (lpValueBuf) // earn time if only for size retrieving
        {
            pCurrentVal->ve_valueptr =(DWORD_PTR) new BYTE[pCurrentVal->ve_valuelen];
            mRegQueryValueExW(hKey,pCurrentVal->ve_valuename,NULL,&pCurrentVal->ve_type,(PBYTE)pCurrentVal->ve_valueptr,&pCurrentVal->ve_valuelen,pRegQueryValueExW);
        }
    }

    if (*ldwTotsize < RequiredSize)
    {
        *ldwTotsize = RequiredSize;
        if (lpValueBuf)
            RetValue = ERROR_MORE_DATA;
        else
            RetValue = ERROR_SUCCESS;
        goto CleanUp;
    }

    if (::IsBadWritePtr(lpValueBuf,RequiredSize))
    {
        RetValue = ERROR_INVALID_PARAMETER;
        goto CleanUp;
    }

    DWORD Index;
    for (Cnt = 0,Index = 0; Cnt<num_vals; Cnt++)
    {
        pCurrentVal = &val_list[Cnt];

        // copy temporary buffer to output buffer
        memcpy(&lpValueBuf[Index],(PBYTE)pCurrentVal->ve_valueptr,pCurrentVal->ve_valuelen);

        // free temporary buffer
        delete[] (PBYTE)pCurrentVal->ve_valueptr;

        // update pointer value to point inside output buffer
        pCurrentVal->ve_valueptr = (DWORD_PTR)  ( (PBYTE)lpValueBuf + Index);

        // increase output buffer index
        Index+=pCurrentVal->ve_valuelen;
    }

    return ERROR_SUCCESS;

CleanUp:
    // clean all pointers
    for (Cnt = 0; Cnt<num_vals; Cnt++)
    {
        pCurrentVal = &val_list[Cnt];
        if ( pCurrentVal->ve_valueptr)
            delete[] (PBYTE)pCurrentVal->ve_valueptr;
        pCurrentVal->ve_valueptr = NULL;
    }
    return RetValue;
}
LONG WINAPI mRegEnumKeyExT(CKeyReplace* pCurrentKey,DWORD dwIndex,TCHAR* lpName,LPDWORD lpcName,LPDWORD lpReserved,TCHAR* lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime)
{
    if (lpName)
        *lpName = 0;
    if (lpClass)
        *lpClass = 0;

    LONG RetValue = ERROR_SUCCESS;
    if (!pCurrentKey)
        return ERROR_FUNCTION_FAILED;
    CKeyReplace* pSubKey = pCurrentKey->GetSubKeyFromIndex(dwIndex);
    if (!pSubKey)
        return ERROR_NO_MORE_ITEMS;
    
    SIZE_T NameLen = pSubKey->KeyName.length();
    SIZE_T ClassLen = pSubKey->Class.length();
    if (lpName)
    {
        if (lpcName)
        {
            if (*lpcName>NameLen)
                _tcscpy(lpName,pSubKey->KeyName.c_str());
            else
                RetValue = ERROR_MORE_DATA;

            // fill lpcName (only after check)
            *lpcName = NameLen;
        }
    }
    else
    {
        if (lpcName)
        {
            *lpcName = NameLen;
        }
    }

    if (lpClass)
    {
        if (lpcClass)
        {
            if (*lpcClass>ClassLen)
                _tcscpy(lpClass,pSubKey->Class.c_str());
            else
                RetValue = ERROR_MORE_DATA;

            // fill lpcClass only after check
            *lpcClass = ClassLen;
        }
    }
    else
    {
        if (lpcClass)
        {
            *lpcClass = ClassLen;
        }
    }

    if (lpftLastWriteTime)
    {
        *lpftLastWriteTime = pSubKey->LastWriteTime;
    }
    return RetValue;
}
LONG WINAPI mRegEnumKeyExA(HKEY hKey,DWORD dwIndex,LPSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime,pfRegEnumKeyExA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction( hKey, dwIndex, lpName, lpcName, lpReserved, lpClass, lpcClass, lpftLastWriteTime);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (!pKey->Emulated)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        LONG RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, dwIndex, lpName, lpcName, lpReserved, lpClass, lpcClass, lpftLastWriteTime);
        M_SECURE_CATCH
        if (gRegistry_bSpyMode)
        {
            if (RetValue ==ERROR_SUCCESS)
            {
                TCHAR* psz;
                CAnsiUnicodeConvert::AnsiToTchar(lpName,&psz);
                if (psz)
                {
                    pKey->GetOrAddSubKey(psz);
                    free(psz);
                }
            }
        }
        return RetValue;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    LONG RetValue;
    TCHAR* pszName = NULL; 
    TCHAR* pszClass = NULL; 

    if (lpcName)
    {
        if (*lpcName)
        {
            pszName= new TCHAR[*lpcName];
            *pszName = 0;
        }
    }
    if (lpcClass)
    {
        if (*lpcClass) 
        {
            pszClass = new TCHAR[*lpcClass];
            *pszClass = 0;
        }
    }

    RetValue = mRegEnumKeyExT(pKey,dwIndex,pszName,lpcName,lpReserved,pszClass,lpcClass,lpftLastWriteTime);
    if (RetValue==ERROR_SUCCESS)
    {
        if (pszName)
            CAnsiUnicodeConvert::TcharToAnsi(pszName,lpName,*lpcName+1);
        if (pszClass)
            CAnsiUnicodeConvert::TcharToAnsi(pszClass,lpClass,*lpcClass+1);
    }
    if (pszName)
        delete[] pszName;
    if (pszClass)
        delete[] pszClass;
    return RetValue;
}
LONG WINAPI mRegEnumKeyExW(HKEY hKey,DWORD dwIndex,LPWSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPWSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime,pfRegEnumKeyExW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        M_SECURE_TRY
        return pFunction( hKey, dwIndex, lpName, lpcName, lpReserved, lpClass, lpcClass, lpftLastWriteTime);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (!pKey->Emulated)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        LONG RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, dwIndex, lpName, lpcName, lpReserved, lpClass, lpcClass, lpftLastWriteTime);
        M_SECURE_CATCH
        if (gRegistry_bSpyMode)
        {
            if (RetValue ==ERROR_SUCCESS)
            {
                TCHAR* psz;
                CAnsiUnicodeConvert::UnicodeToTchar(lpName,&psz);
                if (psz)
                {
                    pKey->GetOrAddSubKey(psz);
                    free(psz);
                }
            }
        }
        return RetValue;

    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;        
    LONG RetValue;
    TCHAR* pszName = NULL; 
    TCHAR* pszClass = NULL; 

    if (lpcName)
    {
        if (*lpcName)
        {
            pszName= new TCHAR[*lpcName];
            *pszName=0;
        }
    }
    if (lpcClass)
    {
        if (*lpcClass)
        {
            pszClass = new TCHAR[*lpcClass];
            *pszClass = 0;
        }
    }

    RetValue = mRegEnumKeyExT(pKey,dwIndex,pszName,lpcName,lpReserved,pszClass,lpcClass,lpftLastWriteTime);

    if (RetValue==ERROR_SUCCESS)
    {
        if (pszName)
            CAnsiUnicodeConvert::TcharToUnicode(pszName,lpName,*lpcName+1);
        if (pszClass)
            CAnsiUnicodeConvert::TcharToUnicode(pszClass,lpClass,*lpcClass+1);
    }
    if (pszName)
        delete[] pszName;
    if (pszClass)
        delete[] pszClass;

    return RetValue;
}
LONG WINAPI mRegEnumKeyA(HKEY hKey,DWORD dwIndex,LPSTR lpName,DWORD cchName,pfRegEnumKeyA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        M_SECURE_TRY
        return pFunction( hKey, dwIndex, lpName, cchName);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (!pKey->Emulated)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        LONG RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, dwIndex, lpName, cchName);
        M_SECURE_CATCH
        if (gRegistry_bSpyMode)
        {
            if (RetValue ==ERROR_SUCCESS)
            {
                TCHAR* psz;
                CAnsiUnicodeConvert::AnsiToTchar(lpName,&psz);
                if (psz)
                {
                    pKey->GetOrAddSubKey(psz);
                    free(psz);
                }
            }
        }
        return RetValue;

    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    LONG RetValue;
    TCHAR* pszName = NULL; 
    DWORD cName = cchName;

    if (cchName)
    {
        pszName= new TCHAR[cchName];
        *pszName = 0;
    }

    RetValue = mRegEnumKeyExT(pKey,dwIndex,pszName,&cName,0,0,0,0);
    if (RetValue==ERROR_SUCCESS)
    {
        if (pszName)
            CAnsiUnicodeConvert::TcharToAnsi(pszName,lpName,cName);
    }
    if (pszName)
        delete[] pszName;

    return RetValue;
}
LONG WINAPI mRegEnumKeyW(HKEY hKey,DWORD dwIndex,LPWSTR lpName,DWORD cchName,pfRegEnumKeyW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        M_SECURE_TRY
        return pFunction( hKey, dwIndex, lpName, cchName);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (!pKey->Emulated)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        LONG RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, dwIndex, lpName, cchName);
        M_SECURE_CATCH
        if (gRegistry_bSpyMode)
        {
            if (RetValue ==ERROR_SUCCESS)
            {
                TCHAR* psz;
                CAnsiUnicodeConvert::UnicodeToTchar(lpName,&psz);
                if (psz)
                {
                    pKey->GetOrAddSubKey(psz);
                    free(psz);
                }
            }
        }
        return RetValue;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    LONG RetValue;
    TCHAR* pszName = NULL; 
    DWORD cName = cchName;
    if (cchName)
    {
        pszName= new TCHAR[cchName];
        *pszName=0;
    }

    RetValue = mRegEnumKeyExT(pKey,dwIndex,pszName,&cName,0,0,0,0);

    if (RetValue==ERROR_SUCCESS)
    {
        if (pszName)
            CAnsiUnicodeConvert::TcharToUnicode(pszName,lpName,cName);
    }
    if (pszName)
        delete[] pszName;

        return RetValue;
}
LONG WINAPI mRegEnumValueT(CKeyReplace* pCurrentKey,DWORD dwIndex,TCHAR* lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData)
{
    if (lpValueName)
        *lpValueName = 0;
    if (lpcValueName)
        *lpcValueName = 0;
    if (lpType)
        *lpType = REG_SZ;
    //if (lpData)
    //    *lpData = 0;
    if (lpcbData)
        *lpcbData = 0;

    LONG RetValue = ERROR_SUCCESS;
    if (!pCurrentKey)
        return ERROR_FUNCTION_FAILED;
    CKeyValue* pValue = pCurrentKey->GetValueFromIndex(dwIndex);
    if (!pValue)
        return ERROR_NO_MORE_ITEMS;

    SIZE_T NameLen = pValue->Name.length();
    if (lpValueName)
    {
        if (lpcValueName)
        {
            if (*lpcValueName>NameLen)
                _tcscpy(lpValueName,pValue->Name.c_str());
            else
                RetValue = ERROR_MORE_DATA;

            // fill lpcValueName only after check
            *lpcValueName = NameLen;
        }
    }
    else
    {
        if (lpcValueName)
        {
            *lpcValueName = NameLen;
        }
    }
    if (lpType)
        *lpType = pValue->Type;

    if (lpcbData)
    {
        
        if (lpData)
        {
            if (*lpcbData<pValue->BufferSize)
                RetValue = ERROR_MORE_DATA;
            else
            {
                if (::IsBadWritePtr(lpData,pValue->BufferSize))
                    RetValue = ERROR_INVALID_PARAMETER;
                else
                {
                    memcpy(lpData,pValue->Buffer,pValue->BufferSize);
                }
            }
        }
        // set *lpcbData only now (after having checked it)
        *lpcbData = pValue->BufferSize;
    }
    return RetValue;
}
LONG WINAPI mRegEnumValueA(HKEY hKey,DWORD dwIndex,LPSTR lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData,pfRegEnumValueA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction(hKey,dwIndex,lpValueName,lpcValueName,lpReserved,lpType,lpData,lpcbData);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (!pKey->Emulated)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        LONG RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, dwIndex, lpValueName, lpcValueName, lpReserved, lpType, lpData, lpcbData);
        M_SECURE_CATCH
        if (gRegistry_bSpyMode)
        {
            TCHAR* psz;
            CAnsiUnicodeConvert::AnsiToTchar(lpValueName,&psz);
            if (psz)
            {
                SIZE_T Type = REG_NONE;
                SIZE_T cbData = 0;
                if (lpType)
                    Type = *lpType;
                if (lpcbData)
                    cbData = *lpcbData;
                pKey->SetValue(psz,Type,lpData,cbData,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_READ);
                free(psz);
            }
        }
        return RetValue;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    LONG RetValue;
    TCHAR* pszName = NULL; 

    if (lpcValueName)
    {
        if (*lpcValueName)
        {
            pszName= new TCHAR[*lpcValueName];
            *pszName=0;
        }
    }

    RetValue = mRegEnumValueT(pKey,dwIndex,pszName,lpcValueName,lpReserved,lpType,lpData,lpcbData);

    if (pszName)
    {
        CAnsiUnicodeConvert::TcharToAnsi(pszName,lpValueName,*lpcValueName+1);
        delete[] pszName;
    }
    return RetValue;
}
LONG WINAPI mRegEnumValueW(HKEY hKey,DWORD dwIndex,LPWSTR lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData,pfRegEnumValueW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction(hKey,dwIndex,lpValueName,lpcValueName,lpReserved,lpType,lpData,lpcbData);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (!pKey->Emulated)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        LONG RetValue = ERROR_FUNCTION_FAILED; // default RetValue in case of exception

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, dwIndex, lpValueName, lpcValueName, lpReserved, lpType, lpData, lpcbData);
        M_SECURE_CATCH
        if (gRegistry_bSpyMode)
        {
            TCHAR* psz;
            CAnsiUnicodeConvert::UnicodeToTchar(lpValueName,&psz);
            if (psz)
            {
                SIZE_T Type = REG_NONE;
                SIZE_T cbData = 0;
                if (lpType)
                    Type = *lpType;
                if (lpcbData)
                    cbData = *lpcbData;
                pKey->SetValue(psz,Type,lpData,cbData,RetValue !=ERROR_SUCCESS,CKeyReplace::AccessType_READ);
                free(psz);
            }
        }
        return RetValue;

    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    LONG RetValue;
    TCHAR* pszName = NULL; 

    if (lpcValueName)
    {
        if (*lpcValueName)
        {
            pszName= new TCHAR[*lpcValueName];
            *pszName=0;
        }
    }

    RetValue = mRegEnumValueT(pKey,dwIndex,pszName,lpcValueName,lpReserved,lpType,lpData,lpcbData);

    if (pszName)
    {
        CAnsiUnicodeConvert::TcharToUnicode(pszName,lpValueName,*lpcValueName+1);
        delete[] pszName;
    }
    return RetValue;
}

LONG WINAPI mRegOpenCurrentUser(REGSAM samDesired,PHKEY phkResult,pfRegOpenCurrentUser pFunction)
{
    if (!phkResult)
        return ERROR_INVALID_PARAMETER;
    *phkResult = 0;
    CKeyReplace* pKey = gpEmulatedRegistry->pKeyLocalHost->GetOrAddSubKey(BaseKeyToString(HKEY_CURRENT_USER));
    pKey->IncreaseOpenedHandleCount();
    if (!pKey->Emulated)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        M_SECURE_TRY
        pFunction(samDesired,&pKey->hRealRegistryKeyHandleIfNotEmulated);
        M_SECURE_CATCH
    }
    
    *phkResult = (HKEY) pKey;

    return ERROR_SUCCESS;
}
LONG WINAPI mRegOpenUserClassesRoot(HANDLE hToken,DWORD dwOptions,REGSAM samDesired,PHKEY phkResult,pfRegOpenUserClassesRoot pFunction)
{
    // MSDN : Applications running in the security context of the interactively logged-on user do not need to use RegOpenUserClassesRoot. These applications can call the RegOpenKeyEx function to retrieve a merged view of the HKEY_CLASSES_ROOT key for the interactive user.
    if (!phkResult)
        return ERROR_INVALID_PARAMETER;
    *phkResult = 0;
        
    CKeyReplace* pKey = gpEmulatedRegistry->pKeyLocalHost->GetOrAddSubKey(BaseKeyToString(HKEY_CLASSES_ROOT));
    pKey->IncreaseOpenedHandleCount();
    if (!pKey->Emulated)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        M_SECURE_TRY
        pFunction(hToken,dwOptions,samDesired,&pKey->hRealRegistryKeyHandleIfNotEmulated);
        M_SECURE_CATCH
    }
    
    *phkResult = (HKEY) pKey;
    return ERROR_SUCCESS;
}
LONG WINAPI mRegOpenKeyExT(CKeyReplace* pKey,TCHAR* lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult)
{
    BOOL bKeyWasExisting;
    // Unlike the RegCreateKeyEx function, the RegOpenKeyEx function does not create the specified key if the key does not exist in the registry
    if (!phkResult)
        return ERROR_INVALID_PARAMETER;
    *phkResult = 0;
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    // In theory we should do the following, but as we emulate the registry,
    // we must create key if they don't exists (by the way, users won't try to create HKLM\Software key)
    // CKeyReplace* OpenedKey = pParentKey->GetSubKeyWithFullPath(lpSubKey);
    CKeyReplace* OpenedKey = pKey->GetOrAddSubKeyWithFullPath(lpSubKey,&bKeyWasExisting);
    if (!OpenedKey)
        return ERROR_FUNCTION_FAILED;

    OpenedKey->IncreaseOpenedHandleCount();
    *phkResult = (HKEY)OpenedKey;
    return ERROR_SUCCESS;
}

LONG WINAPI mRegOpenKeyExA(HKEY hKey,LPCSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,pfRegOpenKeyExA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,ulOptions,samDesired,phkResult); 
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }


    LONG RetValue;
    if (!lpSubKey)
        return ERROR_INVALID_PARAMETER;
    SIZE_T SubKeySize = strlen(lpSubKey)+1;
    TCHAR* psz = new TCHAR[SubKeySize];
    CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,psz,SubKeySize);
    RetValue = mRegOpenKeyExT(pKey,psz,ulOptions,samDesired,phkResult);
    delete[] psz;
    if (!phkResult)
        return RetValue;
    CKeyReplace* pNewKey = (CKeyReplace*)*phkResult;
    if (pKey && pNewKey)
    {
        // if key is not emulated, we have to get real registry informations
        // if we are in spy mode only we have get real informations too
        if  ( (!pNewKey->Emulated) || gRegistry_bSpyMode )
        {
            if (!pFunction)
                return ERROR_FUNCTION_FAILED;

            if (!pKey->hRealRegistryKeyHandleIfNotEmulated)
            {
                // we have to get real hkey even if not emulated
                
                // get base key
                HKEY hBaseKey = NULL;
                CKeyReplace* pBaseKey = pKey->GetBaseKey();
                if (pBaseKey)
                {
                    hBaseKey = StringToBaseKey((TCHAR*)pBaseKey->KeyName.c_str());
                }
                
                // get reference key
                std::tstring KeyNameRelativeToHost=_T("");
                pKey->GetKeyPathWithoutBaseKeyName(KeyNameRelativeToHost);
                // open or create key with same attributes
                // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                CHAR* pc;
                CAnsiUnicodeConvert::TcharToAnsi(KeyNameRelativeToHost.c_str(),&pc);
                M_SECURE_TRY
                pFunction(hBaseKey, pc, ulOptions, samDesired,&pKey->hRealRegistryKeyHandleIfNotEmulated);
                M_SECURE_CATCH
                free(pc);
            }

            RetValue = ERROR_FUNCTION_FAILED;

            // we MUST call exactly the same api to allow winapioverride engine to call the real API 
            M_SECURE_TRY
            RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, ulOptions, samDesired,&pNewKey->hRealRegistryKeyHandleIfNotEmulated);
            M_SECURE_CATCH
        }
    }
    
    return RetValue;
}
LONG WINAPI mRegOpenKeyExW(HKEY hKey,LPCWSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,pfRegOpenKeyExW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,ulOptions,samDesired,phkResult); 
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    LONG RetValue;
    if (!lpSubKey)
        return ERROR_INVALID_PARAMETER;
    SIZE_T SubKeySize = wcslen(lpSubKey)+1;
    TCHAR* psz = new TCHAR[SubKeySize];
    CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,psz,SubKeySize);
    RetValue = mRegOpenKeyExT(pKey,psz,ulOptions,samDesired,phkResult);
    delete[] psz;
    if (!phkResult)
        return RetValue;
    CKeyReplace* pNewKey = (CKeyReplace*)*phkResult;
    if (pKey && pNewKey)
    {
        // if key is not emulated, we have to get real registry informations
        // if we are in spy mode only we have get real informations too
        if  ( (!pNewKey->Emulated) || gRegistry_bSpyMode )
        {
            if (!pFunction)
                return ERROR_FUNCTION_FAILED;

            if (!pKey->hRealRegistryKeyHandleIfNotEmulated)
            {
                // we have to get real hkey even if not emulated
                
                // get base key
                HKEY hBaseKey = NULL;
                CKeyReplace* pBaseKey = pKey->GetBaseKey();
                if (pBaseKey)
                {
                    hBaseKey = StringToBaseKey((TCHAR*)pBaseKey->KeyName.c_str());
                }
                
                // get reference key
                std::tstring KeyNameRelativeToHost=_T("");
                pKey->GetKeyPathWithoutBaseKeyName(KeyNameRelativeToHost);
                // open or create key with same attributes
                // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                WCHAR* pc;
                CAnsiUnicodeConvert::TcharToUnicode(KeyNameRelativeToHost.c_str(),&pc);
                M_SECURE_TRY
                pFunction(hBaseKey, pc, ulOptions, samDesired,&pKey->hRealRegistryKeyHandleIfNotEmulated);
                M_SECURE_CATCH
                free(pc);
            }
            RetValue = ERROR_FUNCTION_FAILED;
            // we MUST call exactly the same api to allow winapioverride engine to call the real API 
            M_SECURE_TRY
            RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, ulOptions, samDesired,&pNewKey->hRealRegistryKeyHandleIfNotEmulated);
            M_SECURE_CATCH
        }
    }
    
    return RetValue;
}
LONG WINAPI mRegOpenKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,HANDLE hTransaction,PVOID pExtendedParameter,pfRegOpenKeyTransactedA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,ulOptions,samDesired,phkResult,hTransaction,pExtendedParameter); 
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;

    LONG RetValue;
    if (!lpSubKey)
        return ERROR_INVALID_PARAMETER;
    SIZE_T SubKeySize = strlen(lpSubKey)+1;
    TCHAR* psz = new TCHAR[SubKeySize];
    CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,psz,SubKeySize);
    RetValue = mRegOpenKeyExT(pKey,psz,ulOptions,samDesired,phkResult);
    delete[] psz;
    if (!phkResult)
        return RetValue;
    CKeyReplace* pNewKey = (CKeyReplace*)*phkResult;
    if (pKey && pNewKey)
    {
        // if key is not emulated, we have to get real registry informations
        // if we are in spy mode only we have get real informations too
        if  ( (!pNewKey->Emulated) || gRegistry_bSpyMode )
        {
            if (!pFunction)
                return ERROR_FUNCTION_FAILED;

            if (!pKey->hRealRegistryKeyHandleIfNotEmulated)
            {
                // we have to get real hkey even if not emulated
                
                // get base key
                HKEY hBaseKey = NULL;
                CKeyReplace* pBaseKey = pKey->GetBaseKey();
                if (pBaseKey)
                {
                    hBaseKey = StringToBaseKey((TCHAR*)pBaseKey->KeyName.c_str());
                }
                
                // get reference key
                std::tstring KeyNameRelativeToHost=_T("");
                pKey->GetKeyPathWithoutBaseKeyName(KeyNameRelativeToHost);
                // open or create key with same attributes
                // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                CHAR* pc;
                CAnsiUnicodeConvert::TcharToAnsi(KeyNameRelativeToHost.c_str(),&pc);
                M_SECURE_TRY
                pFunction(hBaseKey, pc, ulOptions, samDesired,&pKey->hRealRegistryKeyHandleIfNotEmulated,hTransaction,pExtendedParameter);
                M_SECURE_CATCH
                free(pc);
            }

            RetValue = ERROR_FUNCTION_FAILED;

            // we MUST call exactly the same api to allow winapioverride engine to call the real API 
            M_SECURE_TRY
            RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, ulOptions, samDesired,&pNewKey->hRealRegistryKeyHandleIfNotEmulated,hTransaction,pExtendedParameter);
            M_SECURE_CATCH
        }
    }
    
    return RetValue;
}
LONG WINAPI mRegOpenKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,HANDLE hTransaction,PVOID pExtendedParameter,pfRegOpenKeyTransactedW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,ulOptions,samDesired,phkResult,hTransaction,pExtendedParameter); 
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    LONG RetValue;
    if (!lpSubKey)
        return ERROR_INVALID_PARAMETER;
    SIZE_T SubKeySize = wcslen(lpSubKey)+1;
    TCHAR* psz = new TCHAR[SubKeySize];
    CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,psz,SubKeySize);
    RetValue = mRegOpenKeyExT(pKey,psz,ulOptions,samDesired,phkResult);
    delete[] psz;
    
    CKeyReplace* pNewKey = (CKeyReplace*)*phkResult;
    if (pKey && pNewKey)
    {
        // if key is not emulated, we have to get real registry informations
        // if we are in spy mode only we have get real informations too
        if  ( (!pNewKey->Emulated) || gRegistry_bSpyMode )
        {
            if (!pFunction)
                return ERROR_FUNCTION_FAILED;

            if (!pKey->hRealRegistryKeyHandleIfNotEmulated)
            {
                // we have to get real hkey even if not emulated
                
                // get base key
                HKEY hBaseKey = NULL;
                CKeyReplace* pBaseKey = pKey->GetBaseKey();
                if (pBaseKey)
                {
                    hBaseKey = StringToBaseKey((TCHAR*)pBaseKey->KeyName.c_str());
                }
                
                // get reference key
                std::tstring KeyNameRelativeToHost=_T("");
                pKey->GetKeyPathWithoutBaseKeyName(KeyNameRelativeToHost);
                // open or create key with same attributes
                // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                WCHAR* pc;
                CAnsiUnicodeConvert::TcharToUnicode(KeyNameRelativeToHost.c_str(),&pc);
                M_SECURE_TRY
                pFunction(hBaseKey, pc, ulOptions, samDesired,&pKey->hRealRegistryKeyHandleIfNotEmulated,hTransaction,pExtendedParameter);
                M_SECURE_CATCH
                free(pc);
            }
            RetValue = ERROR_FUNCTION_FAILED;

            // we MUST call exactly the same api to allow winapioverride engine to call the real API 
            M_SECURE_TRY
            RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, ulOptions, samDesired,&pNewKey->hRealRegistryKeyHandleIfNotEmulated,hTransaction,pExtendedParameter);
            M_SECURE_CATCH
        }
    }
    
    return RetValue;
}

LONG WINAPI mRegOpenKeyA(HKEY hKey,LPCSTR lpSubKey,PHKEY phkResult,pfRegOpenKeyA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,phkResult); 
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;

    LONG RetValue;
    if (!lpSubKey)
        return ERROR_INVALID_PARAMETER;
    SIZE_T SubKeySize = strlen(lpSubKey)+1;
    TCHAR* psz = new TCHAR[SubKeySize];
    CAnsiUnicodeConvert::AnsiToTchar(lpSubKey,psz,SubKeySize);
    RetValue = mRegOpenKeyExT(pKey,psz,0,0,phkResult);
    delete[] psz;
    if (!phkResult)
        return RetValue;    
    CKeyReplace* pNewKey = (CKeyReplace*)*phkResult;
    if (pKey && pNewKey)
    {
        // if key is not emulated, we have to get real registry informations
        // if we are in spy mode only we have get real informations too
        if  ( (!pNewKey->Emulated) || gRegistry_bSpyMode )
        {
            if (!pFunction)
                return ERROR_FUNCTION_FAILED;

            if (!pKey->hRealRegistryKeyHandleIfNotEmulated)
            {
                // we have to get real hkey even if not emulated
                
                // get base key
                HKEY hBaseKey = NULL;
                CKeyReplace* pBaseKey = pKey->GetBaseKey();
                if (pBaseKey)
                {
                    hBaseKey = StringToBaseKey((TCHAR*)pBaseKey->KeyName.c_str());
                }
                
                // get reference key
                std::tstring KeyNameRelativeToHost=_T("");
                pKey->GetKeyPathWithoutBaseKeyName(KeyNameRelativeToHost);
                // open or create key with same attributes
                // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                CHAR* pc;
                CAnsiUnicodeConvert::TcharToAnsi(KeyNameRelativeToHost.c_str(),&pc);
                M_SECURE_TRY
                pFunction(hBaseKey, pc, &pKey->hRealRegistryKeyHandleIfNotEmulated);
                M_SECURE_CATCH
                free(pc);
            }

            RetValue = ERROR_FUNCTION_FAILED;

            // we MUST call exactly the same api to allow winapioverride engine to call the real API 
            M_SECURE_TRY
            RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated,lpSubKey,&pNewKey->hRealRegistryKeyHandleIfNotEmulated);
            M_SECURE_CATCH
        }
    }
    
    return RetValue;
}
LONG WINAPI mRegOpenKeyW(HKEY hKey,LPCWSTR lpSubKey,PHKEY phkResult,pfRegOpenKeyW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction( hKey,lpSubKey,phkResult); 
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;

    LONG RetValue;
    if (!lpSubKey)
        return ERROR_INVALID_PARAMETER;
    SIZE_T SubKeySize = wcslen(lpSubKey)+1;
    TCHAR* psz = new TCHAR[SubKeySize];
    CAnsiUnicodeConvert::UnicodeToTchar(lpSubKey,psz,SubKeySize);
    RetValue = mRegOpenKeyExT(pKey,psz,0,0,phkResult);
    delete[] psz;
    if (!phkResult)
        return RetValue;    
    CKeyReplace* pNewKey = (CKeyReplace*)*phkResult;
    if (pKey && pNewKey)
    {
        // if key is not emulated, we have to get real registry informations
        // if we are in spy mode only we have get real informations too
        if  ( (!pNewKey->Emulated) || gRegistry_bSpyMode )
        {
            if (!pFunction)
                return ERROR_FUNCTION_FAILED;

            // if we don't already get the real handle
            if (!pKey->hRealRegistryKeyHandleIfNotEmulated)
            {
                // we have to get real hkey even if not emulated
                
                // get base key
                HKEY hBaseKey = NULL;
                CKeyReplace* pBaseKey = pKey->GetBaseKey();
                if (pBaseKey)
                {
                    hBaseKey = StringToBaseKey((TCHAR*)pBaseKey->KeyName.c_str());
                }
                
                // get reference key
                std::tstring KeyNameRelativeToHost=_T("");
                pKey->GetKeyPathWithoutBaseKeyName(KeyNameRelativeToHost);
                // open or create key with same attributes
                // we MUST call exactly the same api to allow winapioverride engine to call the real API 
                WCHAR* pc;
                CAnsiUnicodeConvert::TcharToUnicode(KeyNameRelativeToHost.c_str(),&pc);
                M_SECURE_TRY
                pFunction(hBaseKey, pc, &pKey->hRealRegistryKeyHandleIfNotEmulated);
                M_SECURE_CATCH
                free(pc);
            }

            // we MUST call exactly the same api to allow winapioverride engine to call the real API 
            M_SECURE_TRY
            RetValue = pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey,&pNewKey->hRealRegistryKeyHandleIfNotEmulated);
            M_SECURE_CATCH
        }
    }
    
    return RetValue;
}

LONG WINAPI mRegQueryInfoKeyT(CKeyReplace* pKey,TCHAR* lpClass,LPDWORD lpcClass,LPDWORD lpReserved,LPDWORD lpcSubKeys,
                              LPDWORD lpcMaxSubKeyLen,LPDWORD lpcMaxClassLen,LPDWORD lpcValues,
                              LPDWORD lpcMaxValueNameLen,LPDWORD lpcMaxValueLen,LPDWORD lpcbSecurityDescriptor,PFILETIME lpftLastWriteTime)
{
    if (lpClass)
        *lpClass = 0;
    if (lpcClass)
        *lpcClass = 0;
    if (lpcSubKeys)
        *lpcSubKeys= 0;
    if (lpcMaxSubKeyLen)
        *lpcMaxSubKeyLen = 0;

    if (lpcMaxClassLen)
        *lpcMaxClassLen = 0;
    if (lpcValues)
        *lpcValues = 0;
    if (lpcMaxValueNameLen)
        *lpcMaxValueNameLen = 0;
    if (lpcMaxValueLen)
        *lpcMaxValueLen = 0;
    if (lpcbSecurityDescriptor)
        *lpcbSecurityDescriptor = 0;
    UNREFERENCED_PARAMETER(lpReserved);
    LONG RetValue = ERROR_SUCCESS;

    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (lpcClass)
    {
        if (*lpcClass<=pKey->Class.size())
        {
            RetValue = ERROR_MORE_DATA;
        }
        else if (lpClass)
        {
            _tcscpy(lpClass,pKey->Class.c_str());
        }
        *lpcClass=pKey->Class.size();
    }
    else
    {
        if (lpClass)
            RetValue = ERROR_INVALID_PARAMETER;
    }
    // lpcSubKeys : Pointer to a variable that receives the number of subkeys contained by the specified key. This parameter can be NULL
    if (lpcSubKeys)
        *lpcSubKeys = pKey->GetSubKeysCount();
    // lpcMaxSubKeyLen : Pointer to a variable that receives the size of the key's subkey with the longest name, in TCHARs. This parameter can be NULL
    // Windows NT/2000/XP:  The size does not include the terminating null character
    if (lpcMaxSubKeyLen)
        *lpcMaxSubKeyLen = pKey->GetMaxSubKeyLengh();

    // lpcMaxClassLen : Pointer to a variable that receives the size of the longest string specifying a subkey class, in TCHARs. The count returned does not include the terminating null character. This parameter can be NULL
    if (lpcMaxClassLen)
        *lpcMaxClassLen = pKey->GetMaxSubKeyClassLengh();

    if (lpcValues)
        *lpcValues = pKey->GetValuesCount();

    // lpcMaxValueNameLen : Pointer to a variable that receives the size of the key's longest value name, in TCHARs. The size does not include the terminating null character. This parameter can be NULL
    if (lpcMaxValueNameLen)
        *lpcMaxValueNameLen = pKey->GetMaxValueNameLengh();

    // Pointer to a variable that receives the size of the key's security descriptor, in bytes
    if (lpcbSecurityDescriptor)
// TODO : to implement
        *lpcbSecurityDescriptor = 0;

    if (lpftLastWriteTime)
        *lpftLastWriteTime = pKey->LastWriteTime;

    return RetValue;
}
LONG WINAPI mRegQueryInfoKeyA(HKEY hKey,LPSTR lpClass,LPDWORD lpcClass,LPDWORD lpReserved,LPDWORD lpcSubKeys,LPDWORD lpcMaxSubKeyLen,LPDWORD lpcMaxClassLen,LPDWORD lpcValues,LPDWORD lpcMaxValueNameLen,LPDWORD lpcMaxValueLen,LPDWORD lpcbSecurityDescriptor,PFILETIME lpftLastWriteTime,pfRegQueryInfoKeyA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction(hKey, lpClass, lpcClass, lpReserved, lpcSubKeys, lpcMaxSubKeyLen, lpcMaxClassLen, lpcValues, lpcMaxValueNameLen, lpcMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (!pKey->Emulated)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, lpClass, lpcClass, lpReserved, lpcSubKeys, lpcMaxSubKeyLen, lpcMaxClassLen, lpcValues, lpcMaxValueNameLen, lpcMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    LONG RetValue;
    TCHAR* pszName = NULL; 

    if (lpClass)
    {
        if (*lpcClass)
            pszName= new TCHAR[*lpcClass];
    }

    RetValue = mRegQueryInfoKeyT(pKey,pszName,lpcClass,lpReserved,lpcSubKeys,lpcMaxSubKeyLen,lpcMaxClassLen,lpcValues,lpcMaxValueNameLen,lpcMaxValueLen,lpcbSecurityDescriptor,lpftLastWriteTime);

    if (pszName)
    {
        CAnsiUnicodeConvert::TcharToAnsi(pszName,lpClass,*lpcClass+1);
        delete[] pszName;
    }
    return RetValue;
}
LONG WINAPI mRegQueryInfoKeyW(HKEY hKey,LPWSTR lpClass,LPDWORD lpcClass,LPDWORD lpReserved,LPDWORD lpcSubKeys,LPDWORD lpcMaxSubKeyLen,LPDWORD lpcMaxClassLen,LPDWORD lpcValues,LPDWORD lpcMaxValueNameLen,LPDWORD lpcMaxValueLen,LPDWORD lpcbSecurityDescriptor,PFILETIME lpftLastWriteTime,pfRegQueryInfoKeyW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        M_SECURE_TRY
        return pFunction(hKey, lpClass, lpcClass, lpReserved, lpcSubKeys, lpcMaxSubKeyLen, lpcMaxClassLen, lpcValues, lpcMaxValueNameLen, lpcMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (!pKey->Emulated)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated, lpClass, lpcClass, lpReserved, lpcSubKeys, lpcMaxSubKeyLen, lpcMaxClassLen, lpcValues, lpcMaxValueNameLen, lpcMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    LONG RetValue;
    TCHAR* pszName = NULL; 

    if (lpClass)
    {
        if (*lpcClass)
            pszName= new TCHAR[*lpcClass];
    }

    RetValue = mRegQueryInfoKeyT(pKey,pszName,lpcClass,lpReserved,lpcSubKeys,lpcMaxSubKeyLen,lpcMaxClassLen,lpcValues,lpcMaxValueNameLen,lpcMaxValueLen,lpcbSecurityDescriptor,lpftLastWriteTime);

    if (pszName)
    {
        CAnsiUnicodeConvert::TcharToUnicode(pszName,lpClass,*lpcClass+1);
        delete[] pszName;
    }
    return RetValue;
}
//LONG WINAPI mRegDisablePredefinedCache()// no need to be override
//{
//
//}
// LONG WINAPI RegDisablePredefinedCacheEx(); // no need to be overrided
LONG WINAPI mRegDisableReflectionKey(HKEY hBase,pfRegDisableReflectionKey pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hBase,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction(hBase);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;

// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegDisableReflectionKey not implemented\r\n"));

        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegEnableReflectionKey(HKEY hBase,pfRegEnableReflectionKey pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hBase,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction(hBase);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegEnableReflectionKey not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegQueryReflectionKey(HKEY hBase,BOOL *bIsReflectionDisabled,pfRegQueryReflectionKey pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hBase,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction(hBase,bIsReflectionDisabled);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegQueryReflectionKey not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated,bIsReflectionDisabled);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegNotifyChangeKeyValue(HKEY hKey,BOOL bWatchSubtree,DWORD dwNotifyFilter,HANDLE hEvent,BOOL fAsynchronous,pfRegNotifyChangeKeyValue pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction(hKey,bWatchSubtree, dwNotifyFilter, hEvent, fAsynchronous);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegNotifyChangeKeyValue not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated,bWatchSubtree, dwNotifyFilter, hEvent, fAsynchronous);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegLoadKeyA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpFile,pfRegLoadKeyA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
        M_SECURE_TRY
        return pFunction( hKey, lpSubKey, lpFile);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;

// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegLoadKeyA not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        if (!pKey->IsWriteAccessAllowed())
            return ERROR_SUCCESS;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction( pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, lpFile);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegLoadKeyW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpFile,pfRegLoadKeyW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
        M_SECURE_TRY
        return pFunction( hKey, lpSubKey, lpFile);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegLoadKeyW not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        if (!pKey->IsWriteAccessAllowed())
            return ERROR_SUCCESS;
        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction( pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, lpFile);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}

LONG WINAPI mRegLoadAppKeyA(LPCSTR lpFile,PHKEY phkResult,REGSAM samDesired,DWORD dwOptions,DWORD Reserved,pfRegLoadAppKeyA pFunction)
{


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegLoadAppKeyA not implemented\r\n"));


    if (!pFunction)
        return ERROR_FUNCTION_FAILED;
        
    // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
    if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
        return ERROR_SUCCESS;
    M_SECURE_TRY
    return pFunction( lpFile,phkResult,samDesired,dwOptions,Reserved);
    M_SECURE_CATCH
    return ERROR_FUNCTION_FAILED;
}
LONG WINAPI mRegLoadAppKeyW(LPCWSTR lpFile,PHKEY phkResult,REGSAM samDesired,DWORD dwOptions,DWORD Reserved,pfRegLoadAppKeyW pFunction)
{


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegLoadAppKeyW not implemented\r\n"));


    if (!pFunction)
        return ERROR_FUNCTION_FAILED;
        
    // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
    if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
        return ERROR_SUCCESS;
    M_SECURE_TRY
    return pFunction( lpFile,phkResult,samDesired,dwOptions,Reserved);
    M_SECURE_CATCH
    return ERROR_FUNCTION_FAILED;
}

LONG WINAPI mRegReplaceKeyA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpNewFile,LPCSTR lpOldFile,pfRegReplaceKeyA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction( hKey, lpSubKey, lpNewFile, lpOldFile);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegReplaceKeyA not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        if (!pKey->IsWriteAccessAllowed())
            return ERROR_SUCCESS;
        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction( pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, lpNewFile, lpOldFile);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegReplaceKeyW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpNewFile,LPCWSTR lpOldFile,pfRegReplaceKeyW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction( hKey, lpSubKey, lpNewFile, lpOldFile);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegReplaceKeyW not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        if (!pKey->IsWriteAccessAllowed())
            return ERROR_SUCCESS;
        
        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction( pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey, lpNewFile, lpOldFile);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegRestoreKeyA(HKEY hKey,LPCSTR lpFile,DWORD dwFlags,pfRegRestoreKeyA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction( hKey, lpFile, dwFlags);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegRestoreKeyA not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
    
        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction( pKey->hRealRegistryKeyHandleIfNotEmulated, lpFile, dwFlags);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegRestoreKeyW(HKEY hKey,LPCWSTR lpFile,DWORD dwFlags,pfRegRestoreKeyW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
    
        M_SECURE_TRY
        return pFunction( hKey, lpFile, dwFlags);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegRestoreKeyW not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction( pKey->hRealRegistryKeyHandleIfNotEmulated, lpFile, dwFlags);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegSaveKeyA(HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,pfRegSaveKeyA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
    
        M_SECURE_TRY
        return pFunction( hKey, lpFile, lpSecurityAttributes);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegSaveKeyA not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
    
        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction( pKey->hRealRegistryKeyHandleIfNotEmulated, lpFile, lpSecurityAttributes);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegSaveKeyW(HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,pfRegSaveKeyW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction( hKey, lpFile, lpSecurityAttributes);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegSaveKeyW not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction( pKey->hRealRegistryKeyHandleIfNotEmulated, lpFile, lpSecurityAttributes);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegSaveKeyExA(HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags,pfRegSaveKeyExA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction(hKey, lpFile, lpSecurityAttributes, Flags);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegSaveKeyExA not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction( pKey->hRealRegistryKeyHandleIfNotEmulated, lpFile, lpSecurityAttributes, Flags);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegSaveKeyExW(HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags,pfRegSaveKeyExW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction( hKey, lpFile, lpSecurityAttributes, Flags);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegSaveKeyExW not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction( pKey->hRealRegistryKeyHandleIfNotEmulated, lpFile, lpSecurityAttributes, Flags);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegOverridePredefKey(HKEY hKey,HKEY hNewKey,pfRegOverridePredefKey pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction( hKey, hNewKey);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;        
    CKeyReplace* pNewKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
    
        M_SECURE_TRY
        return pFunction( hKey, hNewKey);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pNewKey) 
        return ERROR_FUNCTION_FAILED;
    if (MarkedForDeletion)
        return ERROR_KEY_DELETED;
    if (pKey->Emulated || pNewKey->Emulated)
    {


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegOverridePredefKey not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction( pKey->hRealRegistryKeyHandleIfNotEmulated, pNewKey->hRealRegistryKeyHandleIfNotEmulated);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegUnLoadKeyA(HKEY hKey,LPCSTR lpSubKey,pfRegUnLoadKeyA pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction(hKey, lpSubKey);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;
// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegUnLoadKeyA not implemented\r\n"));

        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction( pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegUnLoadKeyW(HKEY hKey,LPCWSTR lpSubKey,pfRegUnLoadKeyW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction( hKey, lpSubKey);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegUnLoadKeyW not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction( pKey->hRealRegistryKeyHandleIfNotEmulated, lpSubKey);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}

LONG WINAPI mRegLoadMUIStringA(HKEY hKey,LPCSTR pszValue,LPSTR pszOutBuf,DWORD cbOutBuf,LPDWORD pcbData,DWORD Flags,LPCSTR pszDirectory,pfRegLoadMUIStringA pFunction)
{
    // MSDN
    //The ANSI version of this function returns ERROR_CALL_NOT_IMPLEMENTED.
    //Remarks
    //
    //The RegLoadMUIString function is supported only for Unicode. Although both Unicode (W) and ANSI (A) versions of this function are declared, the RegLoadMUIStringA function returns ERROR_CALL_NOT_IMPLEMENTED. Applications should explicitly call RegLoadMUIStringW or specify Unicode as the character set in platform invoke (PInvoke) calls. 
    return ERROR_CALL_NOT_IMPLEMENTED;
}
LONG WINAPI mRegLoadMUIStringW(HKEY hKey,LPCWSTR pszValue,LPWSTR pszOutBuf,DWORD cbOutBuf,LPDWORD pcbData,DWORD Flags,LPCWSTR pszDirectory,pfRegLoadMUIStringW pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
          
        // we don't know where hKey base key nor name --> impossible to create a CKeyReplace object from this handle
        M_SECURE_TRY
        return pFunction(hKey,pszValue,pszOutBuf,cbOutBuf,pcbData,Flags,pszDirectory);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegLoadMUIStringW not implemented\r\n"));


        return ERROR_SUCCESS;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction( pKey->hRealRegistryKeyHandleIfNotEmulated,pszValue,pszOutBuf,cbOutBuf,pcbData,Flags,pszDirectory);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}

LONG WINAPI mRegFlushKey(HKEY hKey,pfRegFlushKey pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction( hKey);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;
        // currently no interest due to our registry implementation
        return ERROR_SUCCESS;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction( pKey->hRealRegistryKeyHandleIfNotEmulated);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegGetKeySecurity(HKEY hKey,SECURITY_INFORMATION SecurityInformation,PSECURITY_DESCRIPTOR pSecurityDescriptor,LPDWORD lpcbSecurityDescriptor,pfRegGetKeySecurity pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
        M_SECURE_TRY
        return pFunction(hKey,SecurityInformation,pSecurityDescriptor,lpcbSecurityDescriptor);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegGetKeySecurity not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated,SecurityInformation,pSecurityDescriptor,lpcbSecurityDescriptor);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}
LONG WINAPI mRegSetKeySecurity(HKEY hKey,SECURITY_INFORMATION SecurityInformation,PSECURITY_DESCRIPTOR pSecurityDescriptor,pfRegSetKeySecurity pFunction)
{
    BOOL EmulatedKeyHandle = FALSE;
    BOOL MarkedForDeletion;
    CKeyReplace* pKey = GetKeyReplace(hKey,&EmulatedKeyHandle,&MarkedForDeletion);
    if (!EmulatedKeyHandle)
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;

        // check DisableWriteOperationsOnNotEmulatedKeys, key supposed to be local (as 99% of them are)
        if (gpEmulatedRegistry->pKeyLocalHost->DisableWriteOperationsOnNotEmulatedKeys)
            return ERROR_SUCCESS;
        M_SECURE_TRY
        return pFunction(hKey,SecurityInformation,pSecurityDescriptor);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
    if (!pKey)
        return ERROR_FUNCTION_FAILED;
    if (pKey->Emulated)
    {
        if (MarkedForDeletion)
            return ERROR_KEY_DELETED;


// TODO : to implement
#ifdef _DEBUG
        ::DebugBreak();
#endif
        OutputDebugString(_T("RegSetKeySecurity not implemented\r\n"));


        return ERROR_FUNCTION_FAILED;
    }
    else
    {
        if (!pFunction)
            return ERROR_FUNCTION_FAILED;
            
        if (!pKey->IsWriteAccessAllowed())
            return ERROR_SUCCESS;
        
        // we MUST call exactly the same api to allow winapioverride engine to call the real API 
        M_SECURE_TRY
        return pFunction(pKey->hRealRegistryKeyHandleIfNotEmulated,SecurityInformation,pSecurityDescriptor);
        M_SECURE_CATCH
        return ERROR_FUNCTION_FAILED;
    }
}

LONG WINAPI mAdvapiRegCloseKey(HKEY hKey)
{
    return mRegCloseKey(hKey,(pfRegCloseKey)::GetProcAddress(ghModuleAdvapi,"RegCloseKey"));
}
LONG WINAPI mAdvapiRegConnectRegistryA(LPCSTR lpMachineName,HKEY hKey,PHKEY phkResult)
{
    return mRegConnectRegistryA(lpMachineName,hKey,phkResult,(pfRegConnectRegistryA)::GetProcAddress(ghModuleAdvapi,"RegConnectRegistryA"));
}
LONG WINAPI mAdvapiRegConnectRegistryW(LPCWSTR lpMachineName,HKEY hKey,PHKEY phkResult)
{
    return mRegConnectRegistryW(lpMachineName,hKey,phkResult,(pfRegConnectRegistryW)::GetProcAddress(ghModuleAdvapi,"RegConnectRegistryW"));
}
LONG WINAPI mAdvapiRegCopyTreeA(HKEY hKeySrc,LPCSTR lpSubKey,HKEY hKeyDest)
{
    return mRegCopyTreeA(hKeySrc,lpSubKey,hKeyDest,(pfRegCopyTreeA)::GetProcAddress(ghModuleAdvapi,"RegCopyTreeA"));
}
LONG WINAPI mAdvapiRegCopyTreeW(HKEY hKeySrc,LPCWSTR lpSubKey,HKEY hKeyDest)
{
    return mRegCopyTreeW(hKeySrc,lpSubKey,hKeyDest,(pfRegCopyTreeW)::GetProcAddress(ghModuleAdvapi,"RegCopyTreeW"));
}
LONG WINAPI mAdvapiRegCreateKeyA(HKEY hKey,LPCSTR lpSubKey,PHKEY phkResult)
{
    return mRegCreateKeyA(hKey,lpSubKey,phkResult,(pfRegCreateKeyA)::GetProcAddress(ghModuleAdvapi,"RegCreateKeyA"));
}
LONG WINAPI mAdvapiRegCreateKeyW(HKEY hKey,LPCWSTR lpSubKey,PHKEY phkResult)
{
    return mRegCreateKeyW(hKey,lpSubKey,phkResult,(pfRegCreateKeyW)::GetProcAddress(ghModuleAdvapi,"RegCreateKeyW"));
}
LONG WINAPI mAdvapiRegCreateKeyExA(HKEY hKey,LPCSTR lpSubKey,DWORD Reserved, LPSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition)
{
    return mRegCreateKeyExA(hKey,lpSubKey,Reserved,lpClass, dwOptions,samDesired,lpSecurityAttributes,phkResult,lpdwDisposition,(pfRegCreateKeyExA)::GetProcAddress(ghModuleAdvapi,"RegCreateKeyExA"));
}
LONG WINAPI mAdvapiRegCreateKeyExW(HKEY hKey,LPCWSTR lpSubKey,DWORD Reserved, LPWSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition)
{
    return mRegCreateKeyExW(hKey,lpSubKey,Reserved,lpClass, dwOptions,samDesired,lpSecurityAttributes,phkResult,lpdwDisposition,(pfRegCreateKeyExW)::GetProcAddress(ghModuleAdvapi,"RegCreateKeyExW"));
}
LONG WINAPI mAdvapiRegCreateKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,DWORD Reserved,LPSTR lpClass,DWORD dwOptions,REGSAM samDesired,const LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,HANDLE hTransaction,PVOID pExtendedParemeter)
{
    return mRegCreateKeyTransactedA(hKey,lpSubKey,Reserved,lpClass, dwOptions,samDesired,lpSecurityAttributes,phkResult,lpdwDisposition, hTransaction,pExtendedParemeter,(pfRegCreateKeyTransactedA)::GetProcAddress(ghModuleAdvapi,"RegCreateKeyTransactedA"));
}
LONG WINAPI mAdvapiRegCreateKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,DWORD Reserved,LPWSTR lpClass,DWORD dwOptions,REGSAM samDesired,const LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,HANDLE hTransaction,PVOID pExtendedParemeter)
{
    return mRegCreateKeyTransactedW(hKey,lpSubKey,Reserved,lpClass, dwOptions,samDesired,lpSecurityAttributes,phkResult,lpdwDisposition,hTransaction,pExtendedParemeter,(pfRegCreateKeyTransactedW)::GetProcAddress(ghModuleAdvapi,"RegCreateKeyTransactedW"));
}
LONG WINAPI mAdvapiRegDeleteKeyA(HKEY hKey,LPCSTR lpSubKey)
{
    return mRegDeleteKeyA(hKey,lpSubKey,(pfRegDeleteKeyA)::GetProcAddress(ghModuleAdvapi,"RegDeleteKeyA"));
}
LONG WINAPI mAdvapiRegDeleteKeyW(HKEY hKey,LPCWSTR lpSubKey)
{
    return mRegDeleteKeyW(hKey,lpSubKey,(pfRegDeleteKeyW)::GetProcAddress(ghModuleAdvapi,"RegDeleteKeyW"));
}
LONG WINAPI mAdvapiRegDeleteKeyExA(HKEY hKey,LPCSTR lpSubKey,REGSAM samDesired,DWORD Reserved)
{
    return mRegDeleteKeyExA(hKey,lpSubKey, samDesired, Reserved,(pfRegDeleteKeyExA)::GetProcAddress(ghModuleAdvapi,"RegDeleteKeyExA"));
}
LONG WINAPI mAdvapiRegDeleteKeyExW(HKEY hKey,LPCWSTR lpSubKey,REGSAM samDesired,DWORD Reserved)
{
    return mRegDeleteKeyExW(hKey,lpSubKey,samDesired, Reserved,(pfRegDeleteKeyExW)::GetProcAddress(ghModuleAdvapi,"RegDeleteKeyExW"));
}
LONG WINAPI mAdvapiRegDeleteKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,REGSAM samDesired,DWORD Reserved,HANDLE hTransaction,PVOID pExtendedParameter)
{
    return mRegDeleteKeyTransactedA(hKey,lpSubKey, samDesired, Reserved,hTransaction,pExtendedParameter,(pfRegDeleteKeyTransactedA)::GetProcAddress(ghModuleAdvapi,"RegDeleteKeyTransactedA"));
}
LONG WINAPI mAdvapiRegDeleteKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,REGSAM samDesired,DWORD Reserved,HANDLE hTransaction,PVOID pExtendedParameter)
{
    return mRegDeleteKeyTransactedW(hKey,lpSubKey,samDesired, Reserved,hTransaction,pExtendedParameter,(pfRegDeleteKeyTransactedW)::GetProcAddress(ghModuleAdvapi,"RegDeleteKeyTransactedW"));
}
LONG WINAPI mAdvapiRegDeleteKeyValueA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpValueName)
{
    return mRegDeleteKeyValueA(hKey,lpSubKey,lpValueName,(pfRegDeleteKeyValueA)::GetProcAddress(ghModuleAdvapi,"RegDeleteKeyValueA"));
}
LONG WINAPI mAdvapiRegDeleteKeyValueW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpValueName)
{
    return mRegDeleteKeyValueW(hKey,lpSubKey,lpValueName,(pfRegDeleteKeyValueW)::GetProcAddress(ghModuleAdvapi,"RegDeleteKeyValueW"));
}
LONG WINAPI mAdvapiRegDeleteTreeA(HKEY hKey,LPCSTR lpSubKey)
{
    return mRegDeleteTreeA(hKey,lpSubKey,(pfRegDeleteTreeA)::GetProcAddress(ghModuleAdvapi,"RegDeleteTreeA"));
}
LONG WINAPI mAdvapiRegDeleteTreeW(HKEY hKey,LPCWSTR lpSubKey)
{
    return mRegDeleteTreeW(hKey,lpSubKey,(pfRegDeleteTreeW)::GetProcAddress(ghModuleAdvapi,"RegDeleteTreeW"));
}
LONG WINAPI mAdvapiRegDeleteValueA(HKEY hKey,LPCSTR lpValueName)
{
    return mRegDeleteValueA(hKey,lpValueName,(pfRegDeleteValueA)::GetProcAddress(ghModuleAdvapi,"RegDeleteValueA"));
}
LONG WINAPI mAdvapiRegDeleteValueW(HKEY hKey,LPCWSTR lpValueName)
{
    return mRegDeleteValueW(hKey,lpValueName,(pfRegDeleteValueW)::GetProcAddress(ghModuleAdvapi,"RegDeleteValueW"));
}
// LONG WINAPI RegDisablePredefinedCache();// no need to be overrided
// LONG WINAPI RegDisablePredefinedCacheEx(void); // no need to be overrided
LONG WINAPI mAdvapiRegDisableReflectionKey(HKEY hBase)
{
    return mRegDisableReflectionKey(hBase,(pfRegDisableReflectionKey)::GetProcAddress(ghModuleAdvapi,"RegDisableReflectionKey"));
}
LONG WINAPI mAdvapiRegEnableReflectionKey(HKEY hBase)
{
    return mRegEnableReflectionKey(hBase,(pfRegEnableReflectionKey)::GetProcAddress(ghModuleAdvapi,"RegEnableReflectionKey"));
}
LONG WINAPI mAdvapiRegEnumKeyA(HKEY hKey,DWORD dwIndex,LPSTR lpName,DWORD cchName)
{
    return mRegEnumKeyA(hKey,dwIndex,lpName,cchName,(pfRegEnumKeyA)::GetProcAddress(ghModuleAdvapi,"RegEnumKeyA"));
}
LONG WINAPI mAdvapiRegEnumKeyW(HKEY hKey,DWORD dwIndex,LPWSTR lpName,DWORD cchName)
{
    return mRegEnumKeyW(hKey,dwIndex,lpName,cchName,(pfRegEnumKeyW)::GetProcAddress(ghModuleAdvapi,"RegEnumKeyW"));
}
LONG WINAPI mAdvapiRegEnumKeyExA(HKEY hKey,DWORD dwIndex,LPSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime)
{
    return mRegEnumKeyExA(hKey,dwIndex,lpName,lpcName,lpReserved,lpClass,lpcClass,lpftLastWriteTime,(pfRegEnumKeyExA)::GetProcAddress(ghModuleAdvapi,"RegEnumKeyExA"));
}
LONG WINAPI mAdvapiRegEnumKeyExW(HKEY hKey,DWORD dwIndex,LPWSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPWSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime)
{
    return mRegEnumKeyExW(hKey,dwIndex,lpName,lpcName,lpReserved,lpClass,lpcClass,lpftLastWriteTime,(pfRegEnumKeyExW)::GetProcAddress(ghModuleAdvapi,"RegEnumKeyExW"));
}
LONG WINAPI mAdvapiRegEnumValueA(HKEY hKey,DWORD dwIndex,LPSTR lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData)
{
    return mRegEnumValueA(hKey,dwIndex,lpValueName,lpcValueName,lpReserved,lpType,lpData,lpcbData,(pfRegEnumValueA)::GetProcAddress(ghModuleAdvapi,"RegEnumValueA"));
}
LONG WINAPI mAdvapiRegEnumValueW(HKEY hKey,DWORD dwIndex,LPWSTR lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData)
{
    return mRegEnumValueW(hKey,dwIndex,lpValueName,lpcValueName,lpReserved,lpType,lpData,lpcbData,(pfRegEnumValueW)::GetProcAddress(ghModuleAdvapi,"RegEnumValueW"));
}
LONG WINAPI mAdvapiRegFlushKey(HKEY hKey)
{
    return mRegFlushKey(hKey,(pfRegFlushKey)::GetProcAddress(ghModuleAdvapi,"RegFlushKey"));
}
LONG WINAPI mAdvapiRegGetKeySecurity(HKEY hKey,SECURITY_INFORMATION SecurityInformation,PSECURITY_DESCRIPTOR pSecurityDescriptor,LPDWORD lpcbSecurityDescriptor)
{
    return mRegGetKeySecurity(hKey, SecurityInformation,pSecurityDescriptor,lpcbSecurityDescriptor,(pfRegGetKeySecurity)::GetProcAddress(ghModuleAdvapi,"RegGetKeySecurity"));
}
LONG WINAPI mAdvapiRegGetValueA(HKEY hkey,LPCSTR lpSubKey,LPCSTR lpValue,DWORD dwFlags,LPDWORD pdwType,PVOID pvData,LPDWORD pcbData)
{
    return mRegGetValueA(hkey,lpSubKey,lpValue,dwFlags,pdwType,pvData,pcbData,(pfRegGetValueA)::GetProcAddress(ghModuleAdvapi,"RegGetValueA"));
}
LONG WINAPI mAdvapiRegGetValueW(HKEY hkey,LPCWSTR lpSubKey,LPCWSTR lpValue,DWORD dwFlags,LPDWORD pdwType,PVOID pvData,LPDWORD pcbData)
{
    return mRegGetValueW(hkey,lpSubKey,lpValue,dwFlags,pdwType,pvData,pcbData,(pfRegGetValueW)::GetProcAddress(ghModuleAdvapi,"RegGetValueW"));
}
LONG WINAPI mAdvapiRegLoadAppKeyA(LPCSTR lpFile,PHKEY phkResult,REGSAM samDesired,DWORD dwOptions,DWORD Reserved)
{
    return mRegLoadAppKeyA(lpFile,phkResult,samDesired,dwOptions,Reserved,(pfRegLoadAppKeyA)::GetProcAddress(ghModuleAdvapi,"RegLoadAppKeyA"));
}
LONG WINAPI mAdvapiRegLoadAppKeyW(LPCWSTR lpFile,PHKEY phkResult,REGSAM samDesired,DWORD dwOptions,DWORD Reserved)
{
    return mRegLoadAppKeyW(lpFile,phkResult,samDesired,dwOptions,Reserved,(pfRegLoadAppKeyW)::GetProcAddress(ghModuleAdvapi,"RegLoadAppKeyW"));
}
LONG WINAPI mAdvapiRegLoadKeyA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpFile)
{
    return mRegLoadKeyA(hKey,lpSubKey,lpFile,(pfRegLoadKeyA)::GetProcAddress(ghModuleAdvapi,"RegLoadKeyA"));
}
LONG WINAPI mAdvapiRegLoadKeyW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpFile)
{
    return mRegLoadKeyW(hKey,lpSubKey,lpFile,(pfRegLoadKeyW)::GetProcAddress(ghModuleAdvapi,"RegLoadKeyW"));
}
LONG WINAPI mAdvapiRegLoadMUIStringA(HKEY hKey,LPCSTR pszValue,LPSTR pszOutBuf,DWORD cbOutBuf,LPDWORD pcbData,DWORD Flags,LPCSTR pszDirectory)
{
    return mRegLoadMUIStringA(hKey, pszValue,pszOutBuf,cbOutBuf,pcbData,Flags,pszDirectory,(pfRegLoadMUIStringA)::GetProcAddress(ghModuleAdvapi,"RegLoadMUIStringA"));
}
LONG WINAPI mAdvapiRegLoadMUIStringW(HKEY hKey,LPCWSTR pszValue,LPWSTR pszOutBuf,DWORD cbOutBuf,LPDWORD pcbData,DWORD Flags,LPCWSTR pszDirectory)
{
    return mRegLoadMUIStringW(hKey, pszValue,pszOutBuf,cbOutBuf,pcbData,Flags,pszDirectory,(pfRegLoadMUIStringW)::GetProcAddress(ghModuleAdvapi,"RegLoadMUIStringW"));
}
LONG WINAPI mAdvapiRegNotifyChangeKeyValue(HKEY hKey,BOOL bWatchSubtree,DWORD dwNotifyFilter,HANDLE hEvent,BOOL fAsynchronous)
{
    return mRegNotifyChangeKeyValue(hKey, bWatchSubtree,dwNotifyFilter,hEvent,fAsynchronous,(pfRegNotifyChangeKeyValue)::GetProcAddress(ghModuleAdvapi,"RegNotifyChangeKeyValue"));
}
LONG WINAPI mAdvapiRegOpenCurrentUser(REGSAM samDesired,PHKEY phkResult)
{
    return mRegOpenCurrentUser(samDesired, phkResult,(pfRegOpenCurrentUser)::GetProcAddress(ghModuleAdvapi,"RegOpenCurrentUser"));
}
LONG WINAPI mAdvapiRegOpenKeyA(HKEY hKey,LPCSTR lpSubKey,PHKEY phkResult)
{
    return mRegOpenKeyA(hKey, lpSubKey,phkResult,(pfRegOpenKeyA)::GetProcAddress(ghModuleAdvapi,"RegOpenKeyA"));
}
LONG WINAPI mAdvapiRegOpenKeyW(HKEY hKey,LPCWSTR lpSubKey,PHKEY phkResult)
{
    return mRegOpenKeyW(hKey, lpSubKey,phkResult,(pfRegOpenKeyW)::GetProcAddress(ghModuleAdvapi,"RegOpenKeyW"));
}
LONG WINAPI mAdvapiRegOpenKeyExA(HKEY hKey,LPCSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult)
{
    return mRegOpenKeyExA(hKey, lpSubKey,ulOptions,samDesired,phkResult,(pfRegOpenKeyExA)::GetProcAddress(ghModuleAdvapi,"RegOpenKeyExA"));
}
LONG WINAPI mAdvapiRegOpenKeyExW(HKEY hKey,LPCWSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult)
{
    return mRegOpenKeyExW(hKey, lpSubKey, ulOptions,samDesired,phkResult,(pfRegOpenKeyExW)::GetProcAddress(ghModuleAdvapi,"RegOpenKeyExW"));
}
LONG WINAPI mAdvapiRegOpenKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,HANDLE hTransaction,PVOID pExtendedParameter)
{
    return mRegOpenKeyTransactedA(hKey, lpSubKey,ulOptions,samDesired,phkResult,hTransaction,pExtendedParameter,(pfRegOpenKeyTransactedA)::GetProcAddress(ghModuleAdvapi,"RegOpenKeyTransactedA"));
}
LONG WINAPI mAdvapiRegOpenKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,HANDLE hTransaction,PVOID pExtendedParameter)
{
    return mRegOpenKeyTransactedW(hKey, lpSubKey, ulOptions,samDesired,phkResult,hTransaction,pExtendedParameter,(pfRegOpenKeyTransactedW)::GetProcAddress(ghModuleAdvapi,"RegOpenKeyTransactedW"));
}
LONG WINAPI mAdvapiRegOpenUserClassesRoot(HANDLE hToken,DWORD dwOptions,REGSAM samDesired,PHKEY phkResult)
{
    return mRegOpenUserClassesRoot(hToken,dwOptions,samDesired,phkResult,(pfRegOpenUserClassesRoot)::GetProcAddress(ghModuleAdvapi,"RegOpenUserClassesRoot"));
}
LONG WINAPI mAdvapiRegOverridePredefKey(HKEY hKey,HKEY hNewHKey)
{
    return mRegOverridePredefKey(hKey,hNewHKey,(pfRegOverridePredefKey)::GetProcAddress(ghModuleAdvapi,"RegOverridePredefKey"));
}
LONG WINAPI mAdvapiRegQueryInfoKeyA(HKEY hKey,LPSTR lpClass,LPDWORD lpcClass,LPDWORD lpReserved,LPDWORD lpcSubKeys,LPDWORD lpcMaxSubKeyLen,LPDWORD lpcMaxClassLen,LPDWORD lpcValues,LPDWORD lpcMaxValueNameLen,LPDWORD lpcMaxValueLen,LPDWORD lpcbSecurityDescriptor,PFILETIME lpftLastWriteTime)
{
    return mRegQueryInfoKeyA(hKey,lpClass,lpcClass,lpReserved,lpcSubKeys,lpcMaxSubKeyLen,lpcMaxClassLen,lpcValues,lpcMaxValueNameLen,lpcMaxValueLen,lpcbSecurityDescriptor,lpftLastWriteTime,(pfRegQueryInfoKeyA)::GetProcAddress(ghModuleAdvapi,"RegQueryInfoKeyA"));
}
LONG WINAPI mAdvapiRegQueryInfoKeyW(HKEY hKey,LPWSTR lpClass,LPDWORD lpcClass,LPDWORD lpReserved,LPDWORD lpcSubKeys,LPDWORD lpcMaxSubKeyLen,LPDWORD lpcMaxClassLen,LPDWORD lpcValues,LPDWORD lpcMaxValueNameLen,LPDWORD lpcMaxValueLen,LPDWORD lpcbSecurityDescriptor,PFILETIME lpftLastWriteTime)
{
    return mRegQueryInfoKeyW(hKey,lpClass,lpcClass,lpReserved,lpcSubKeys,lpcMaxSubKeyLen,lpcMaxClassLen,lpcValues,lpcMaxValueNameLen,lpcMaxValueLen,lpcbSecurityDescriptor,lpftLastWriteTime,(pfRegQueryInfoKeyW)::GetProcAddress(ghModuleAdvapi,"RegQueryInfoKeyW"));
}
LONG WINAPI mAdvapiRegQueryMultipleValuesA(HKEY hKey,PVALENTA val_list,DWORD num_vals,LPSTR lpValueBuf,LPDWORD ldwTotsize)
{
    return mRegQueryMultipleValuesA(hKey,val_list,num_vals,lpValueBuf,ldwTotsize,
        (pfRegQueryMultipleValuesA)::GetProcAddress(ghModuleAdvapi,"RegQueryMultipleValuesA"),
        (pfRegQueryValueExA)::GetProcAddress(ghModuleAdvapi,"RegQueryValueExA")
        );
}
LONG WINAPI mAdvapiRegQueryMultipleValuesW(HKEY hKey,PVALENTW val_list,DWORD num_vals,LPWSTR lpValueBuf,LPDWORD ldwTotsize)
{
    return mRegQueryMultipleValuesW(hKey,val_list,num_vals,lpValueBuf,ldwTotsize,
        (pfRegQueryMultipleValuesW)::GetProcAddress(ghModuleAdvapi,"RegQueryMultipleValuesW"),
        (pfRegQueryValueExW)::GetProcAddress(ghModuleAdvapi,"RegQueryValueExW")
        );
}
LONG WINAPI mAdvapiRegQueryReflectionKey(HKEY hBase,BOOL *bIsReflectionDisabled)
{
    return mRegQueryReflectionKey(hBase,bIsReflectionDisabled,(pfRegQueryReflectionKey)::GetProcAddress(ghModuleAdvapi,"RegQueryReflectionKey"));
}
LONG WINAPI mAdvapiRegQueryValueA(HKEY hKey,LPCSTR lpSubKey,LPSTR lpValue,PLONG lpcbValue)
{
    return mRegQueryValueA( hKey,lpSubKey,lpValue,lpcbValue,(pfRegQueryValueA)::GetProcAddress(ghModuleAdvapi,"RegQueryValueA"));
}
LONG WINAPI mAdvapiRegQueryValueW(HKEY hKey,LPCWSTR lpSubKey,LPWSTR lpValue,PLONG lpcbValue)
{
    return mRegQueryValueW( hKey,lpSubKey,lpValue,lpcbValue,(pfRegQueryValueW)::GetProcAddress(ghModuleAdvapi,"RegQueryValueW"));
}
LONG WINAPI mAdvapiRegQueryValueExA(HKEY hKey,LPCSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData)
{
    return mRegQueryValueExA( hKey, lpValueName,lpReserved,lpType,lpData,lpcbData,(pfRegQueryValueExA)::GetProcAddress(ghModuleAdvapi,"RegQueryValueExA"));
}
LONG WINAPI mAdvapiRegQueryValueExW(HKEY hKey,LPCWSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData)
{
    return mRegQueryValueExW( hKey, lpValueName,lpReserved,lpType,lpData,lpcbData,(pfRegQueryValueExW)::GetProcAddress(ghModuleAdvapi,"RegQueryValueExW"));
}
LONG WINAPI mAdvapiRegReplaceKeyA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpNewFile,LPCSTR lpOldFile)
{
    return mRegReplaceKeyA( hKey, lpSubKey,lpNewFile,lpOldFile,(pfRegReplaceKeyA)::GetProcAddress(ghModuleAdvapi,"RegReplaceKeyA"));
}
LONG WINAPI mAdvapiRegReplaceKeyW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpNewFile,LPCWSTR lpOldFile)
{
    return mRegReplaceKeyW( hKey, lpSubKey,lpNewFile,lpOldFile,(pfRegReplaceKeyW)::GetProcAddress(ghModuleAdvapi,"RegReplaceKeyW"));
}
LONG WINAPI mAdvapiRegRestoreKeyA(HKEY hKey,LPCSTR lpFile,DWORD dwFlags)
{
    return mRegRestoreKeyA( hKey, lpFile,dwFlags,(pfRegRestoreKeyA)::GetProcAddress(ghModuleAdvapi,"RegRestoreKeyA"));
}
LONG WINAPI mAdvapiRegRestoreKeyW(HKEY hKey,LPCWSTR lpFile,DWORD dwFlags)
{
    return mRegRestoreKeyW( hKey, lpFile,dwFlags,(pfRegRestoreKeyW)::GetProcAddress(ghModuleAdvapi,"RegRestoreKeyW"));
}
LONG WINAPI mAdvapiRegSaveKeyA(HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    return mRegSaveKeyA( hKey, lpFile,lpSecurityAttributes,(pfRegSaveKeyA)::GetProcAddress(ghModuleAdvapi,"RegSaveKeyA"));
}
LONG WINAPI mAdvapiRegSaveKeyW(HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    return mRegSaveKeyW( hKey, lpFile,lpSecurityAttributes,(pfRegSaveKeyW)::GetProcAddress(ghModuleAdvapi,"RegSaveKeyW"));
}
LONG WINAPI mAdvapiRegSaveKeyExA(HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags)
{
    return mRegSaveKeyExA( hKey, lpFile,lpSecurityAttributes,Flags,(pfRegSaveKeyExA)::GetProcAddress(ghModuleAdvapi,"RegSaveKeyExA"));
}
LONG WINAPI mAdvapiRegSaveKeyExW(HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags)
{
    return mRegSaveKeyExW( hKey, lpFile,lpSecurityAttributes,Flags,(pfRegSaveKeyExW)::GetProcAddress(ghModuleAdvapi,"RegSaveKeyExW"));
}
LONG WINAPI mAdvapiRegSetKeySecurity(HKEY hKey,SECURITY_INFORMATION SecurityInformation,PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    return mRegSetKeySecurity( hKey, SecurityInformation,pSecurityDescriptor,(pfRegSetKeySecurity)::GetProcAddress(ghModuleAdvapi,"RegSetKeySecurity"));
}
LONG WINAPI mAdvapiRegSetKeyValueA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpValueName,DWORD dwType,LPCVOID lpData,DWORD cbData)
{
    return mRegSetKeyValueA( hKey,lpSubKey,lpValueName,dwType,lpData,cbData,(pfRegSetKeyValueA)::GetProcAddress(ghModuleAdvapi,"RegSetKeyValueA"));
}
LONG WINAPI mAdvapiRegSetKeyValueW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpValueName,DWORD dwType,LPCVOID lpData,DWORD cbData)
{
    return mRegSetKeyValueW( hKey,lpSubKey,lpValueName,dwType,lpData,cbData,(pfRegSetKeyValueW)::GetProcAddress(ghModuleAdvapi,"RegSetKeyValueW"));
}
LONG WINAPI mAdvapiRegSetValueA(HKEY hKey,LPCSTR lpSubKey,DWORD dwType,LPCSTR lpData,DWORD cbData)
{
    return mRegSetValueA( hKey, lpSubKey,dwType,lpData,cbData,(pfRegSetValueA)::GetProcAddress(ghModuleAdvapi,"RegSetValueA"));
}
LONG WINAPI mAdvapiRegSetValueW(HKEY hKey,LPCWSTR lpSubKey,DWORD dwType,LPCWSTR lpData,DWORD cbData)
{
    return mRegSetValueW( hKey, lpSubKey,dwType,lpData,cbData,(pfRegSetValueW)::GetProcAddress(ghModuleAdvapi,"RegSetValueW"));
}
LONG WINAPI mAdvapiRegSetValueExA(HKEY hKey,LPCSTR lpValueName,DWORD Reserved,DWORD dwType,const BYTE* lpData,DWORD cbData)
{
    return mRegSetValueExA( hKey, lpValueName,Reserved,dwType,lpData,cbData,(pfRegSetValueExA)::GetProcAddress(ghModuleAdvapi,"RegSetValueExA"));
}
LONG WINAPI mAdvapiRegSetValueExW(HKEY hKey,LPCWSTR lpValueName,DWORD Reserved,DWORD dwType,const BYTE* lpData,DWORD cbData)
{
    return mRegSetValueExW( hKey, lpValueName,Reserved,dwType,lpData,cbData,(pfRegSetValueExW)::GetProcAddress(ghModuleAdvapi,"RegSetValueExW"));
}
LONG WINAPI mAdvapiRegUnLoadKeyA(HKEY hKey,LPCSTR lpSubKey)
{
    return mRegUnLoadKeyA(hKey,lpSubKey,(pfRegUnLoadKeyA)::GetProcAddress(ghModuleAdvapi,"RegUnLoadKeyA"));
}
LONG WINAPI mAdvapiRegUnLoadKeyW(HKEY hKey,LPCWSTR lpSubKey)
{
    return mRegUnLoadKeyW(hKey,lpSubKey,(pfRegUnLoadKeyW)::GetProcAddress(ghModuleAdvapi,"RegUnLoadKeyW"));
}




LONG WINAPI mKernelRegCloseKey(HKEY hKey)
{
    return mRegCloseKey(hKey,(pfRegCloseKey)::GetProcAddress(ghModuleKernel,"RegCloseKey"));
}
LONG WINAPI mKernelRegConnectRegistryA(LPCSTR lpMachineName,HKEY hKey,PHKEY phkResult)
{
    return mRegConnectRegistryA(lpMachineName,hKey,phkResult,(pfRegConnectRegistryA)::GetProcAddress(ghModuleKernel,"RegConnectRegistryA"));
}
LONG WINAPI mKernelRegConnectRegistryW(LPCWSTR lpMachineName,HKEY hKey,PHKEY phkResult)
{
    return mRegConnectRegistryW(lpMachineName,hKey,phkResult,(pfRegConnectRegistryW)::GetProcAddress(ghModuleKernel,"RegConnectRegistryW"));
}
LONG WINAPI mKernelRegCopyTreeA(HKEY hKeySrc,LPCSTR lpSubKey,HKEY hKeyDest)
{
    return mRegCopyTreeA(hKeySrc,lpSubKey,hKeyDest,(pfRegCopyTreeA)::GetProcAddress(ghModuleKernel,"RegCopyTreeA"));
}
LONG WINAPI mKernelRegCopyTreeW(HKEY hKeySrc,LPCWSTR lpSubKey,HKEY hKeyDest)
{
    return mRegCopyTreeW(hKeySrc,lpSubKey,hKeyDest,(pfRegCopyTreeW)::GetProcAddress(ghModuleKernel,"RegCopyTreeW"));
}
LONG WINAPI mKernelRegCreateKeyA(HKEY hKey,LPCSTR lpSubKey,PHKEY phkResult)
{
    return mRegCreateKeyA(hKey,lpSubKey,phkResult,(pfRegCreateKeyA)::GetProcAddress(ghModuleKernel,"RegCreateKeyA"));
}
LONG WINAPI mKernelRegCreateKeyW(HKEY hKey,LPCWSTR lpSubKey,PHKEY phkResult)
{
    return mRegCreateKeyW(hKey,lpSubKey,phkResult,(pfRegCreateKeyW)::GetProcAddress(ghModuleKernel,"RegCreateKeyW"));
}
LONG WINAPI mKernelRegCreateKeyExA(HKEY hKey,LPCSTR lpSubKey,DWORD Reserved, LPSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition)
{
    return mRegCreateKeyExA(hKey,lpSubKey,Reserved,lpClass, dwOptions,samDesired,lpSecurityAttributes,phkResult,lpdwDisposition,(pfRegCreateKeyExA)::GetProcAddress(ghModuleKernel,"RegCreateKeyExA"));
}
LONG WINAPI mKernelRegCreateKeyExW(HKEY hKey,LPCWSTR lpSubKey,DWORD Reserved, LPWSTR lpClass,DWORD dwOptions,REGSAM samDesired,LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition)
{
    return mRegCreateKeyExW(hKey,lpSubKey,Reserved,lpClass, dwOptions,samDesired,lpSecurityAttributes,phkResult,lpdwDisposition,(pfRegCreateKeyExW)::GetProcAddress(ghModuleKernel,"RegCreateKeyExW"));
}
LONG WINAPI mKernelRegCreateKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,DWORD Reserved,LPSTR lpClass,DWORD dwOptions,REGSAM samDesired,const LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,HANDLE hTransaction,PVOID pExtendedParemeter)
{
    return mRegCreateKeyTransactedA(hKey,lpSubKey,Reserved,lpClass, dwOptions,samDesired,lpSecurityAttributes,phkResult,lpdwDisposition, hTransaction,pExtendedParemeter,(pfRegCreateKeyTransactedA)::GetProcAddress(ghModuleKernel,"RegCreateKeyTransactedA"));
}
LONG WINAPI mKernelRegCreateKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,DWORD Reserved,LPWSTR lpClass,DWORD dwOptions,REGSAM samDesired,const LPSECURITY_ATTRIBUTES lpSecurityAttributes,PHKEY phkResult,LPDWORD lpdwDisposition,HANDLE hTransaction,PVOID pExtendedParemeter)
{
    return mRegCreateKeyTransactedW(hKey,lpSubKey,Reserved,lpClass, dwOptions,samDesired,lpSecurityAttributes,phkResult,lpdwDisposition,hTransaction,pExtendedParemeter,(pfRegCreateKeyTransactedW)::GetProcAddress(ghModuleKernel,"RegCreateKeyTransactedW"));
}
LONG WINAPI mKernelRegDeleteKeyA(HKEY hKey,LPCSTR lpSubKey)
{
    return mRegDeleteKeyA(hKey,lpSubKey,(pfRegDeleteKeyA)::GetProcAddress(ghModuleKernel,"RegDeleteKeyA"));
}
LONG WINAPI mKernelRegDeleteKeyW(HKEY hKey,LPCWSTR lpSubKey)
{
    return mRegDeleteKeyW(hKey,lpSubKey,(pfRegDeleteKeyW)::GetProcAddress(ghModuleKernel,"RegDeleteKeyW"));
}
LONG WINAPI mKernelRegDeleteKeyExA(HKEY hKey,LPCSTR lpSubKey,REGSAM samDesired,DWORD Reserved)
{
    return mRegDeleteKeyExA(hKey,lpSubKey, samDesired, Reserved,(pfRegDeleteKeyExA)::GetProcAddress(ghModuleKernel,"RegDeleteKeyExA"));
}
LONG WINAPI mKernelRegDeleteKeyExW(HKEY hKey,LPCWSTR lpSubKey,REGSAM samDesired,DWORD Reserved)
{
    return mRegDeleteKeyExW(hKey,lpSubKey,samDesired, Reserved,(pfRegDeleteKeyExW)::GetProcAddress(ghModuleKernel,"RegDeleteKeyExW"));
}
LONG WINAPI mKernelRegDeleteKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,REGSAM samDesired,DWORD Reserved,HANDLE hTransaction,PVOID pExtendedParameter)
{
    return mRegDeleteKeyTransactedA(hKey,lpSubKey, samDesired, Reserved,hTransaction,pExtendedParameter,(pfRegDeleteKeyTransactedA)::GetProcAddress(ghModuleKernel,"RegDeleteKeyTransactedA"));
}
LONG WINAPI mKernelRegDeleteKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,REGSAM samDesired,DWORD Reserved,HANDLE hTransaction,PVOID pExtendedParameter)
{
    return mRegDeleteKeyTransactedW(hKey,lpSubKey,samDesired, Reserved,hTransaction,pExtendedParameter,(pfRegDeleteKeyTransactedW)::GetProcAddress(ghModuleKernel,"RegDeleteKeyTransactedW"));
}
LONG WINAPI mKernelRegDeleteKeyValueA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpValueName)
{
    return mRegDeleteKeyValueA(hKey,lpSubKey,lpValueName,(pfRegDeleteKeyValueA)::GetProcAddress(ghModuleKernel,"RegDeleteKeyValueA"));
}
LONG WINAPI mKernelRegDeleteKeyValueW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpValueName)
{
    return mRegDeleteKeyValueW(hKey,lpSubKey,lpValueName,(pfRegDeleteKeyValueW)::GetProcAddress(ghModuleKernel,"RegDeleteKeyValueW"));
}
LONG WINAPI mKernelRegDeleteTreeA(HKEY hKey,LPCSTR lpSubKey)
{
    return mRegDeleteTreeA(hKey,lpSubKey,(pfRegDeleteTreeA)::GetProcAddress(ghModuleKernel,"RegDeleteTreeA"));
}
LONG WINAPI mKernelRegDeleteTreeW(HKEY hKey,LPCWSTR lpSubKey)
{
    return mRegDeleteTreeW(hKey,lpSubKey,(pfRegDeleteTreeW)::GetProcAddress(ghModuleKernel,"RegDeleteTreeW"));
}
LONG WINAPI mKernelRegDeleteValueA(HKEY hKey,LPCSTR lpValueName)
{
    return mRegDeleteValueA(hKey,lpValueName,(pfRegDeleteValueA)::GetProcAddress(ghModuleKernel,"RegDeleteValueA"));
}
LONG WINAPI mKernelRegDeleteValueW(HKEY hKey,LPCWSTR lpValueName)
{
    return mRegDeleteValueW(hKey,lpValueName,(pfRegDeleteValueW)::GetProcAddress(ghModuleKernel,"RegDeleteValueW"));
}
// LONG WINAPI RegDisablePredefinedCache();// no need to be overrided
// LONG WINAPI RegDisablePredefinedCacheEx(void); // no need to be overrided
LONG WINAPI mKernelRegDisableReflectionKey(HKEY hBase)
{
    return mRegDisableReflectionKey(hBase,(pfRegDisableReflectionKey)::GetProcAddress(ghModuleKernel,"RegDisableReflectionKey"));
}
LONG WINAPI mKernelRegEnableReflectionKey(HKEY hBase)
{
    return mRegEnableReflectionKey(hBase,(pfRegEnableReflectionKey)::GetProcAddress(ghModuleKernel,"RegEnableReflectionKey"));
}
LONG WINAPI mKernelRegEnumKeyA(HKEY hKey,DWORD dwIndex,LPSTR lpName,DWORD cchName)
{
    return mRegEnumKeyA(hKey,dwIndex,lpName,cchName,(pfRegEnumKeyA)::GetProcAddress(ghModuleKernel,"RegEnumKeyA"));
}
LONG WINAPI mKernelRegEnumKeyW(HKEY hKey,DWORD dwIndex,LPWSTR lpName,DWORD cchName)
{
    return mRegEnumKeyW(hKey,dwIndex,lpName,cchName,(pfRegEnumKeyW)::GetProcAddress(ghModuleKernel,"RegEnumKeyW"));
}
LONG WINAPI mKernelRegEnumKeyExA(HKEY hKey,DWORD dwIndex,LPSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime)
{
    return mRegEnumKeyExA(hKey,dwIndex,lpName,lpcName,lpReserved,lpClass,lpcClass,lpftLastWriteTime,(pfRegEnumKeyExA)::GetProcAddress(ghModuleKernel,"RegEnumKeyExA"));
}
LONG WINAPI mKernelRegEnumKeyExW(HKEY hKey,DWORD dwIndex,LPWSTR lpName,LPDWORD lpcName,LPDWORD lpReserved,LPWSTR lpClass,LPDWORD lpcClass,PFILETIME lpftLastWriteTime)
{
    return mRegEnumKeyExW(hKey,dwIndex,lpName,lpcName,lpReserved,lpClass,lpcClass,lpftLastWriteTime,(pfRegEnumKeyExW)::GetProcAddress(ghModuleKernel,"RegEnumKeyExW"));
}
LONG WINAPI mKernelRegEnumValueA(HKEY hKey,DWORD dwIndex,LPSTR lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData)
{
    return mRegEnumValueA(hKey,dwIndex,lpValueName,lpcValueName,lpReserved,lpType,lpData,lpcbData,(pfRegEnumValueA)::GetProcAddress(ghModuleKernel,"RegEnumValueA"));
}
LONG WINAPI mKernelRegEnumValueW(HKEY hKey,DWORD dwIndex,LPWSTR lpValueName,LPDWORD lpcValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData)
{
    return mRegEnumValueW(hKey,dwIndex,lpValueName,lpcValueName,lpReserved,lpType,lpData,lpcbData,(pfRegEnumValueW)::GetProcAddress(ghModuleKernel,"RegEnumValueW"));
}
LONG WINAPI mKernelRegFlushKey(HKEY hKey)
{
    return mRegFlushKey(hKey,(pfRegFlushKey)::GetProcAddress(ghModuleKernel,"RegFlushKey"));
}
LONG WINAPI mKernelRegGetKeySecurity(HKEY hKey,SECURITY_INFORMATION SecurityInformation,PSECURITY_DESCRIPTOR pSecurityDescriptor,LPDWORD lpcbSecurityDescriptor)
{
    return mRegGetKeySecurity(hKey, SecurityInformation,pSecurityDescriptor,lpcbSecurityDescriptor,(pfRegGetKeySecurity)::GetProcAddress(ghModuleKernel,"RegGetKeySecurity"));
}
LONG WINAPI mKernelRegGetValueA(HKEY hkey,LPCSTR lpSubKey,LPCSTR lpValue,DWORD dwFlags,LPDWORD pdwType,PVOID pvData,LPDWORD pcbData)
{
    return mRegGetValueA(hkey,lpSubKey,lpValue,dwFlags,pdwType,pvData,pcbData,(pfRegGetValueA)::GetProcAddress(ghModuleKernel,"RegGetValueA"));
}
LONG WINAPI mKernelRegGetValueW(HKEY hkey,LPCWSTR lpSubKey,LPCWSTR lpValue,DWORD dwFlags,LPDWORD pdwType,PVOID pvData,LPDWORD pcbData)
{
    return mRegGetValueW(hkey,lpSubKey,lpValue,dwFlags,pdwType,pvData,pcbData,(pfRegGetValueW)::GetProcAddress(ghModuleKernel,"RegGetValueW"));
}
LONG WINAPI mKernelRegLoadAppKeyA(LPCSTR lpFile,PHKEY phkResult,REGSAM samDesired,DWORD dwOptions,DWORD Reserved)
{
    return mRegLoadAppKeyA(lpFile,phkResult,samDesired,dwOptions,Reserved,(pfRegLoadAppKeyA)::GetProcAddress(ghModuleKernel,"RegLoadAppKeyA"));
}
LONG WINAPI mKernelRegLoadAppKeyW(LPCWSTR lpFile,PHKEY phkResult,REGSAM samDesired,DWORD dwOptions,DWORD Reserved)
{
    return mRegLoadAppKeyW(lpFile,phkResult,samDesired,dwOptions,Reserved,(pfRegLoadAppKeyW)::GetProcAddress(ghModuleKernel,"RegLoadAppKeyW"));
}
LONG WINAPI mKernelRegLoadKeyA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpFile)
{
    return mRegLoadKeyA(hKey,lpSubKey,lpFile,(pfRegLoadKeyA)::GetProcAddress(ghModuleKernel,"RegLoadKeyA"));
}
LONG WINAPI mKernelRegLoadKeyW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpFile)
{
    return mRegLoadKeyW(hKey,lpSubKey,lpFile,(pfRegLoadKeyW)::GetProcAddress(ghModuleKernel,"RegLoadKeyW"));
}
LONG WINAPI mKernelRegLoadMUIStringA(HKEY hKey,LPCSTR pszValue,LPSTR pszOutBuf,DWORD cbOutBuf,LPDWORD pcbData,DWORD Flags,LPCSTR pszDirectory)
{
    return mRegLoadMUIStringA(hKey, pszValue,pszOutBuf,cbOutBuf,pcbData,Flags,pszDirectory,(pfRegLoadMUIStringA)::GetProcAddress(ghModuleKernel,"RegLoadMUIStringA"));
}
LONG WINAPI mKernelRegLoadMUIStringW(HKEY hKey,LPCWSTR pszValue,LPWSTR pszOutBuf,DWORD cbOutBuf,LPDWORD pcbData,DWORD Flags,LPCWSTR pszDirectory)
{
    return mRegLoadMUIStringW(hKey, pszValue,pszOutBuf,cbOutBuf,pcbData,Flags,pszDirectory,(pfRegLoadMUIStringW)::GetProcAddress(ghModuleKernel,"RegLoadMUIStringW"));
}
LONG WINAPI mKernelRegNotifyChangeKeyValue(HKEY hKey,BOOL bWatchSubtree,DWORD dwNotifyFilter,HANDLE hEvent,BOOL fAsynchronous)
{
    return mRegNotifyChangeKeyValue(hKey, bWatchSubtree,dwNotifyFilter,hEvent,fAsynchronous,(pfRegNotifyChangeKeyValue)::GetProcAddress(ghModuleKernel,"RegNotifyChangeKeyValue"));
}
LONG WINAPI mKernelRegOpenCurrentUser(REGSAM samDesired,PHKEY phkResult)
{
    return mRegOpenCurrentUser(samDesired, phkResult,(pfRegOpenCurrentUser)::GetProcAddress(ghModuleKernel,"RegOpenCurrentUser"));
}
LONG WINAPI mKernelRegOpenKeyA(HKEY hKey,LPCSTR lpSubKey,PHKEY phkResult)
{
    return mRegOpenKeyA(hKey, lpSubKey,phkResult,(pfRegOpenKeyA)::GetProcAddress(ghModuleKernel,"RegOpenKeyA"));
}
LONG WINAPI mKernelRegOpenKeyW(HKEY hKey,LPCWSTR lpSubKey,PHKEY phkResult)
{
    return mRegOpenKeyW(hKey, lpSubKey,phkResult,(pfRegOpenKeyW)::GetProcAddress(ghModuleKernel,"RegOpenKeyW"));
}
LONG WINAPI mKernelRegOpenKeyExA(HKEY hKey,LPCSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult)
{
    return mRegOpenKeyExA(hKey, lpSubKey,ulOptions,samDesired,phkResult,(pfRegOpenKeyExA)::GetProcAddress(ghModuleKernel,"RegOpenKeyExA"));
}
LONG WINAPI mKernelRegOpenKeyExW(HKEY hKey,LPCWSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult)
{
    return mRegOpenKeyExW(hKey, lpSubKey, ulOptions,samDesired,phkResult,(pfRegOpenKeyExW)::GetProcAddress(ghModuleKernel,"RegOpenKeyExW"));
}
LONG WINAPI mKernelRegOpenKeyTransactedA(HKEY hKey,LPCSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,HANDLE hTransaction,PVOID pExtendedParameter)
{
    return mRegOpenKeyTransactedA(hKey, lpSubKey,ulOptions,samDesired,phkResult,hTransaction,pExtendedParameter,(pfRegOpenKeyTransactedA)::GetProcAddress(ghModuleKernel,"RegOpenKeyTransactedA"));
}
LONG WINAPI mKernelRegOpenKeyTransactedW(HKEY hKey,LPCWSTR lpSubKey,DWORD ulOptions,REGSAM samDesired,PHKEY phkResult,HANDLE hTransaction,PVOID pExtendedParameter)
{
    return mRegOpenKeyTransactedW(hKey, lpSubKey, ulOptions,samDesired,phkResult,hTransaction,pExtendedParameter,(pfRegOpenKeyTransactedW)::GetProcAddress(ghModuleKernel,"RegOpenKeyTransactedW"));
}
LONG WINAPI mKernelRegOpenUserClassesRoot(HANDLE hToken,DWORD dwOptions,REGSAM samDesired,PHKEY phkResult)
{
    return mRegOpenUserClassesRoot(hToken,dwOptions,samDesired,phkResult,(pfRegOpenUserClassesRoot)::GetProcAddress(ghModuleKernel,"RegOpenUserClassesRoot"));
}
LONG WINAPI mKernelRegOverridePredefKey(HKEY hKey,HKEY hNewHKey)
{
    return mRegOverridePredefKey(hKey,hNewHKey,(pfRegOverridePredefKey)::GetProcAddress(ghModuleKernel,"RegOverridePredefKey"));
}
LONG WINAPI mKernelRegQueryInfoKeyA(HKEY hKey,LPSTR lpClass,LPDWORD lpcClass,LPDWORD lpReserved,LPDWORD lpcSubKeys,LPDWORD lpcMaxSubKeyLen,LPDWORD lpcMaxClassLen,LPDWORD lpcValues,LPDWORD lpcMaxValueNameLen,LPDWORD lpcMaxValueLen,LPDWORD lpcbSecurityDescriptor,PFILETIME lpftLastWriteTime)
{
    return mRegQueryInfoKeyA(hKey,lpClass,lpcClass,lpReserved,lpcSubKeys,lpcMaxSubKeyLen,lpcMaxClassLen,lpcValues,lpcMaxValueNameLen,lpcMaxValueLen,lpcbSecurityDescriptor,lpftLastWriteTime,(pfRegQueryInfoKeyA)::GetProcAddress(ghModuleKernel,"RegQueryInfoKeyA"));
}
LONG WINAPI mKernelRegQueryInfoKeyW(HKEY hKey,LPWSTR lpClass,LPDWORD lpcClass,LPDWORD lpReserved,LPDWORD lpcSubKeys,LPDWORD lpcMaxSubKeyLen,LPDWORD lpcMaxClassLen,LPDWORD lpcValues,LPDWORD lpcMaxValueNameLen,LPDWORD lpcMaxValueLen,LPDWORD lpcbSecurityDescriptor,PFILETIME lpftLastWriteTime)
{
    return mRegQueryInfoKeyW(hKey,lpClass,lpcClass,lpReserved,lpcSubKeys,lpcMaxSubKeyLen,lpcMaxClassLen,lpcValues,lpcMaxValueNameLen,lpcMaxValueLen,lpcbSecurityDescriptor,lpftLastWriteTime,(pfRegQueryInfoKeyW)::GetProcAddress(ghModuleKernel,"RegQueryInfoKeyW"));
}
LONG WINAPI mKernelRegQueryMultipleValuesA(HKEY hKey,PVALENTA val_list,DWORD num_vals,LPSTR lpValueBuf,LPDWORD ldwTotsize)
{
    return mRegQueryMultipleValuesA(hKey,val_list,num_vals,lpValueBuf,ldwTotsize,
        (pfRegQueryMultipleValuesA)::GetProcAddress(ghModuleKernel,"RegQueryMultipleValuesA"),
        (pfRegQueryValueExA)::GetProcAddress(ghModuleKernel,"RegQueryValueExA")
        );
}
LONG WINAPI mKernelRegQueryMultipleValuesW(HKEY hKey,PVALENTW val_list,DWORD num_vals,LPWSTR lpValueBuf,LPDWORD ldwTotsize)
{
    return mRegQueryMultipleValuesW(hKey,val_list,num_vals,lpValueBuf,ldwTotsize,
        (pfRegQueryMultipleValuesW)::GetProcAddress(ghModuleKernel,"RegQueryMultipleValuesW"),
        (pfRegQueryValueExW)::GetProcAddress(ghModuleKernel,"RegQueryValueExW")
        );
}
LONG WINAPI mKernelRegQueryReflectionKey(HKEY hBase,BOOL *bIsReflectionDisabled)
{
    return mRegQueryReflectionKey(hBase,bIsReflectionDisabled,(pfRegQueryReflectionKey)::GetProcAddress(ghModuleKernel,"RegQueryReflectionKey"));
}
LONG WINAPI mKernelRegQueryValueA(HKEY hKey,LPCSTR lpSubKey,LPSTR lpValue,PLONG lpcbValue)
{
    return mRegQueryValueA( hKey,lpSubKey,lpValue,lpcbValue,(pfRegQueryValueA)::GetProcAddress(ghModuleKernel,"RegQueryValueA"));
}
LONG WINAPI mKernelRegQueryValueW(HKEY hKey,LPCWSTR lpSubKey,LPWSTR lpValue,PLONG lpcbValue)
{
    return mRegQueryValueW( hKey,lpSubKey,lpValue,lpcbValue,(pfRegQueryValueW)::GetProcAddress(ghModuleKernel,"RegQueryValueW"));
}
LONG WINAPI mKernelRegQueryValueExA(HKEY hKey,LPCSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData)
{
    return mRegQueryValueExA( hKey, lpValueName,lpReserved,lpType,lpData,lpcbData,(pfRegQueryValueExA)::GetProcAddress(ghModuleKernel,"RegQueryValueExA"));
}
LONG WINAPI mKernelRegQueryValueExW(HKEY hKey,LPCWSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData)
{
    return mRegQueryValueExW( hKey, lpValueName,lpReserved,lpType,lpData,lpcbData,(pfRegQueryValueExW)::GetProcAddress(ghModuleKernel,"RegQueryValueExW"));
}
LONG WINAPI mKernelRegReplaceKeyA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpNewFile,LPCSTR lpOldFile)
{
    return mRegReplaceKeyA( hKey, lpSubKey,lpNewFile,lpOldFile,(pfRegReplaceKeyA)::GetProcAddress(ghModuleKernel,"RegReplaceKeyA"));
}
LONG WINAPI mKernelRegReplaceKeyW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpNewFile,LPCWSTR lpOldFile)
{
    return mRegReplaceKeyW( hKey, lpSubKey,lpNewFile,lpOldFile,(pfRegReplaceKeyW)::GetProcAddress(ghModuleKernel,"RegReplaceKeyW"));
}
LONG WINAPI mKernelRegRestoreKeyA(HKEY hKey,LPCSTR lpFile,DWORD dwFlags)
{
    return mRegRestoreKeyA( hKey, lpFile,dwFlags,(pfRegRestoreKeyA)::GetProcAddress(ghModuleKernel,"RegRestoreKeyA"));
}
LONG WINAPI mKernelRegRestoreKeyW(HKEY hKey,LPCWSTR lpFile,DWORD dwFlags)
{
    return mRegRestoreKeyW( hKey, lpFile,dwFlags,(pfRegRestoreKeyW)::GetProcAddress(ghModuleKernel,"RegRestoreKeyW"));
}
LONG WINAPI mKernelRegSaveKeyA(HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    return mRegSaveKeyA( hKey, lpFile,lpSecurityAttributes,(pfRegSaveKeyA)::GetProcAddress(ghModuleKernel,"RegSaveKeyA"));
}
LONG WINAPI mKernelRegSaveKeyW(HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    return mRegSaveKeyW( hKey, lpFile,lpSecurityAttributes,(pfRegSaveKeyW)::GetProcAddress(ghModuleKernel,"RegSaveKeyW"));
}
LONG WINAPI mKernelRegSaveKeyExA(HKEY hKey,LPCSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags)
{
    return mRegSaveKeyExA( hKey, lpFile,lpSecurityAttributes,Flags,(pfRegSaveKeyExA)::GetProcAddress(ghModuleKernel,"RegSaveKeyExA"));
}
LONG WINAPI mKernelRegSaveKeyExW(HKEY hKey,LPCWSTR lpFile,LPSECURITY_ATTRIBUTES lpSecurityAttributes,DWORD Flags)
{
    return mRegSaveKeyExW( hKey, lpFile,lpSecurityAttributes,Flags,(pfRegSaveKeyExW)::GetProcAddress(ghModuleKernel,"RegSaveKeyExW"));
}
LONG WINAPI mKernelRegSetKeySecurity(HKEY hKey,SECURITY_INFORMATION SecurityInformation,PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    return mRegSetKeySecurity( hKey, SecurityInformation,pSecurityDescriptor,(pfRegSetKeySecurity)::GetProcAddress(ghModuleKernel,"RegSetKeySecurity"));
}
LONG WINAPI mKernelRegSetKeyValueA(HKEY hKey,LPCSTR lpSubKey,LPCSTR lpValueName,DWORD dwType,LPCVOID lpData,DWORD cbData)
{
    return mRegSetKeyValueA( hKey,lpSubKey,lpValueName,dwType,lpData,cbData,(pfRegSetKeyValueA)::GetProcAddress(ghModuleKernel,"RegSetKeyValueA"));
}
LONG WINAPI mKernelRegSetKeyValueW(HKEY hKey,LPCWSTR lpSubKey,LPCWSTR lpValueName,DWORD dwType,LPCVOID lpData,DWORD cbData)
{
    return mRegSetKeyValueW( hKey,lpSubKey,lpValueName,dwType,lpData,cbData,(pfRegSetKeyValueW)::GetProcAddress(ghModuleKernel,"RegSetKeyValueW"));
}
LONG WINAPI mKernelRegSetValueA(HKEY hKey,LPCSTR lpSubKey,DWORD dwType,LPCSTR lpData,DWORD cbData)
{
    return mRegSetValueA( hKey, lpSubKey,dwType,lpData,cbData,(pfRegSetValueA)::GetProcAddress(ghModuleKernel,"RegSetValueA"));
}
LONG WINAPI mKernelRegSetValueW(HKEY hKey,LPCWSTR lpSubKey,DWORD dwType,LPCWSTR lpData,DWORD cbData)
{
    return mRegSetValueW( hKey, lpSubKey,dwType,lpData,cbData,(pfRegSetValueW)::GetProcAddress(ghModuleKernel,"RegSetValueW"));
}
LONG WINAPI mKernelRegSetValueExA(HKEY hKey,LPCSTR lpValueName,DWORD Reserved,DWORD dwType,const BYTE* lpData,DWORD cbData)
{
    return mRegSetValueExA( hKey, lpValueName,Reserved,dwType,lpData,cbData,(pfRegSetValueExA)::GetProcAddress(ghModuleKernel,"RegSetValueExA"));
}
LONG WINAPI mKernelRegSetValueExW(HKEY hKey,LPCWSTR lpValueName,DWORD Reserved,DWORD dwType,const BYTE* lpData,DWORD cbData)
{
    return mRegSetValueExW( hKey, lpValueName,Reserved,dwType,lpData,cbData,(pfRegSetValueExW)::GetProcAddress(ghModuleKernel,"RegSetValueExW"));
}
LONG WINAPI mKernelRegUnLoadKeyA(HKEY hKey,LPCSTR lpSubKey)
{
    return mRegUnLoadKeyA(hKey,lpSubKey,(pfRegUnLoadKeyA)::GetProcAddress(ghModuleKernel,"RegUnLoadKeyA"));
}
LONG WINAPI mKernelRegUnLoadKeyW(HKEY hKey,LPCWSTR lpSubKey)
{
    return mRegUnLoadKeyW(hKey,lpSubKey,(pfRegUnLoadKeyW)::GetProcAddress(ghModuleKernel,"RegUnLoadKeyW"));
}


BOOL WINAPI mCreateProcessA(LPCSTR lpApplicationName,
                            CHAR* lpCommandLine,
                            LPSECURITY_ATTRIBUTES lpProcessAttributes,
                            LPSECURITY_ATTRIBUTES lpThreadAttributes,
                            BOOL bInheritHandles,
                            DWORD dwCreationFlags,
                            LPVOID lpEnvironment,
                            LPCSTR lpCurrentDirectory,
                            LPSTARTUPINFOA lpStartupInfo,
                            LPPROCESS_INFORMATION lpProcessInformation)
{
    // first of all check if we need to hook subprocesses or not
    BOOL bHookSubprocess = TRUE;
    if (!gpEmulatedRegistry)
        bHookSubprocess = FALSE;
    else
    {
        bHookSubprocess = gpEmulatedRegistry->pOptions->GetEmulateSubProcesses();
    }
    if (!bHookSubprocess)
    {
        // call the same api to call the original one
        return ::CreateProcessA(lpApplicationName,lpCommandLine,lpProcessAttributes,lpThreadAttributes,bInheritHandles,dwCreationFlags,lpEnvironment,lpCurrentDirectory,lpStartupInfo,lpProcessInformation);    
    }

    BOOL bOriginalySuspended = ( (dwCreationFlags & CREATE_SUSPENDED) !=0 );
    BOOL bRetValue;
    DWORD dwLastError;
    CHAR sz[MAX_PATH];
    
    // add the suspended option to flag
    dwCreationFlags|=CREATE_SUSPENDED;
    // call the same api to call the original one
    bRetValue = ::CreateProcessA(lpApplicationName,lpCommandLine,lpProcessAttributes,lpThreadAttributes,bInheritHandles,dwCreationFlags,lpEnvironment,lpCurrentDirectory,lpStartupInfo,lpProcessInformation);
    // on failure, do nothing
    if (!bRetValue)
        return bRetValue;
    dwLastError = ::GetLastError();
    
    CHAR CommandLine[4*MAX_PATH];
    sprintf(CommandLine," ProcessID=0x%x ThreadId=0x%x",lpProcessInformation->dwProcessId,lpProcessInformation->dwThreadId);
    switch(gLauncherOptions.FilteringType)
    {
        case FilteringType_ONLY_BASE_MODULE:
            strcat(CommandLine," OnlyBaseModule");
            break;
        case FilteringType_INCLUDE_ONLY_SPECIFIED:
            strcat(CommandLine," InclusionList=\"");
            CAnsiUnicodeConvert::TcharToAnsi(gLauncherOptions.FilteringTypeFileAbsolutePath,sz,MAX_PATH);
            strcat(CommandLine,sz);
            strcat(CommandLine,"\"");
            break;
        case FilteringType_INCLUDE_ALL_BUT_SPECIFIED:
            strcat(CommandLine," ExclusionList=\"");
            CAnsiUnicodeConvert::TcharToAnsi(gLauncherOptions.FilteringTypeFileAbsolutePath,sz,MAX_PATH);
            strcat(CommandLine,sz);
            strcat(CommandLine,"\"");
            break;
    }

    strcat(CommandLine," EmulatedRegistryFile=\"");
    CAnsiUnicodeConvert::TcharToAnsi(gLauncherOptions.EmulatedRegistryConfigFileAbsolutePath,sz,MAX_PATH);
    strcat(CommandLine,sz);
    strcat(CommandLine,"\"");

    // todo : to implement : currently a single registry can't be shared between different processes --> need a database like implementation
    // subprocesses can't save registry
    _snprintf(sz,MAX_PATH," Flags=0x%x",REGISTRY_REPLACEMENT_OPTIONS_FLAG_DON_T_SAVE_REGISTRY);
    strcat(CommandLine,sz);

    // save current registry if change occurs so subprocess will get the most up to date registry with current way (registry can't be shared between different processes)
    if (!gRegistry_bSpyMode)
    {
        if (!(gLauncherOptions.Flags & REGISTRY_REPLACEMENT_OPTIONS_FLAG_DON_T_SAVE_REGISTRY))
            // save registry into xml file
            gpEmulatedRegistry->Save(gLauncherOptions.EmulatedRegistryConfigFileAbsolutePath,TRUE);

    }

    STARTUPINFOA LauncherStartupInfo;
    PROCESS_INFORMATION LauncherProcessInformation;
    memset( &LauncherStartupInfo,0, sizeof(LauncherStartupInfo) );
    LauncherStartupInfo.cb = sizeof(LauncherStartupInfo);
    LauncherStartupInfo.dwFlags = CREATE_SUSPENDED; // create in a suspended way to get process id before process starts
    memset( &LauncherProcessInformation,0, sizeof(LauncherProcessInformation) );
    ::CreateProcessA(gLauncherFullPathA,CommandLine,0,0,0,0,0,0,&LauncherStartupInfo,&LauncherProcessInformation);

    // create synchronization event after creation (we need process id)
    TCHAR EventName[MAX_PATH];
    _sntprintf(EventName,MAX_PATH,_T("%s%u"),LAUNCHER_EVENT_INJECTION_FINISHEDW, lpProcessInformation->dwProcessId);
    HANDLE hEventLauncherJobFinished = ::CreateEvent(0,FALSE,FALSE,EventName);

    // from now as our event name is created using process id, resume launcher
    ::ResumeThread(LauncherProcessInformation.hThread);

    // wait launcher job or launcher exit/crash (avoid to deadlock current process in case of crash of launcher)
    HANDLE WaitingHandles[]={hEventLauncherJobFinished,LauncherProcessInformation.hProcess};
    DWORD dwRet = ::WaitForMultipleObjects(_countof(WaitingHandles),WaitingHandles,FALSE,INFINITE);
    ::CloseHandle(LauncherProcessInformation.hProcess);
    ::CloseHandle(LauncherProcessInformation.hThread);

    // if event is not the hEventLauncherJobFinished one
    if (dwRet!=WAIT_OBJECT_0)
    {
        // no specific action as sub process is resumed
    }

    // resume thread if needed
    if (!bOriginalySuspended)
        ::ResumeThread(lpProcessInformation->hThread);

    // restore original create process last error
    ::SetLastError(dwLastError);
    return bRetValue;
}
BOOL WINAPI mCreateProcessW(LPCWSTR lpApplicationName,
                            WCHAR* lpCommandLine,
                            LPSECURITY_ATTRIBUTES lpProcessAttributes,
                            LPSECURITY_ATTRIBUTES lpThreadAttributes,
                            BOOL bInheritHandles,
                            DWORD dwCreationFlags,
                            LPVOID lpEnvironment,
                            LPCWSTR lpCurrentDirectory,
                            LPSTARTUPINFOW lpStartupInfo,
                            LPPROCESS_INFORMATION lpProcessInformation)
{
    // first of all check if we need to hook subprocesses or not
    BOOL bHookSubprocess = TRUE;
    if (!gpEmulatedRegistry)
        bHookSubprocess = FALSE;
    else
    {
        bHookSubprocess = gpEmulatedRegistry->pOptions->GetEmulateSubProcesses();
    }
    if (!bHookSubprocess)
    {
        // call the same api to call the original one
        return ::CreateProcessW(lpApplicationName,lpCommandLine,lpProcessAttributes,lpThreadAttributes,bInheritHandles,dwCreationFlags,lpEnvironment,lpCurrentDirectory,lpStartupInfo,lpProcessInformation);    
    }
    
    BOOL bOriginalySuspended = ( (dwCreationFlags & CREATE_SUSPENDED) !=0 );
    BOOL bRetValue;
    DWORD dwLastError;
    WCHAR sz[MAX_PATH];
    
    // add the suspended option to flag
    dwCreationFlags|=CREATE_SUSPENDED;
    // call the same api to call the original one
    bRetValue = ::CreateProcessW(lpApplicationName,lpCommandLine,lpProcessAttributes,lpThreadAttributes,bInheritHandles,dwCreationFlags,lpEnvironment,lpCurrentDirectory,lpStartupInfo,lpProcessInformation);
    // on failure, do nothing
    if (!bRetValue)
        return bRetValue;
    dwLastError = ::GetLastError();
    
    WCHAR CommandLine[4*MAX_PATH];
    swprintf(CommandLine,L" ProcessID=0x%x ThreadId=0x%x",lpProcessInformation->dwProcessId,lpProcessInformation->dwThreadId);
    switch(gLauncherOptions.FilteringType)
    {
        case FilteringType_ONLY_BASE_MODULE:
            wcscat(CommandLine,L" OnlyBaseModule");
            break;
        case FilteringType_INCLUDE_ONLY_SPECIFIED:
            wcscat(CommandLine,L" InclusionList=\"");
            CAnsiUnicodeConvert::TcharToUnicode(gLauncherOptions.FilteringTypeFileAbsolutePath,sz,MAX_PATH);
            wcscat(CommandLine,sz);
            wcscat(CommandLine,L"\"");
            break;
        case FilteringType_INCLUDE_ALL_BUT_SPECIFIED:
            wcscat(CommandLine,L" ExclusionList=\"");
            CAnsiUnicodeConvert::TcharToUnicode(gLauncherOptions.FilteringTypeFileAbsolutePath,sz,MAX_PATH);
            wcscat(CommandLine,sz);
            wcscat(CommandLine,L"\"");
            break;
    }

    wcscat(CommandLine,L" EmulatedRegistryFile=\"");
    CAnsiUnicodeConvert::TcharToUnicode(gLauncherOptions.EmulatedRegistryConfigFileAbsolutePath,sz,MAX_PATH);
    wcscat(CommandLine,sz);
    wcscat(CommandLine,L"\"");

    // todo : to implement : currently a single registry can't be shared between different processes --> need a database like implementation
    // subprocesses can't save registry
    _snwprintf(sz,MAX_PATH,L" Flags=0x%x",REGISTRY_REPLACEMENT_OPTIONS_FLAG_DON_T_SAVE_REGISTRY);
    wcscat(CommandLine,sz);

    // save current registry if change occurs so subprocess will get the most up to date registry with current way (registry can't be shared between different processes)
    if (!gRegistry_bSpyMode)
    {
        if (!(gLauncherOptions.Flags & REGISTRY_REPLACEMENT_OPTIONS_FLAG_DON_T_SAVE_REGISTRY))
            // save registry into xml file
            gpEmulatedRegistry->Save(gLauncherOptions.EmulatedRegistryConfigFileAbsolutePath,TRUE);

    }

    STARTUPINFOW LauncherStartupInfo;
    PROCESS_INFORMATION LauncherProcessInformation;
    memset( &LauncherStartupInfo,0, sizeof(LauncherStartupInfo) );
    LauncherStartupInfo.cb = sizeof(LauncherStartupInfo);
    LauncherStartupInfo.dwFlags = CREATE_SUSPENDED; // create in a suspended way to get process id before process starts
    memset( &LauncherProcessInformation,0, sizeof(LauncherProcessInformation) );
    ::CreateProcessW(gLauncherFullPathW,CommandLine,0,0,0,0,0,0,&LauncherStartupInfo,&LauncherProcessInformation);

    // create synchronization event after creation (we need process id)
    TCHAR EventName[MAX_PATH];
    _sntprintf(EventName,MAX_PATH,_T("%s%u"),LAUNCHER_EVENT_INJECTION_FINISHEDW, lpProcessInformation->dwProcessId);
    HANDLE hEventLauncherJobFinished = ::CreateEvent(0,FALSE,FALSE,EventName);

    // from now as our event name is created using process id, resume launcher
    ::ResumeThread(LauncherProcessInformation.hThread);

    // wait launcher job or launcher exit/crash (avoid to deadlock current process in case of crash of launcher)
    HANDLE WaitingHandles[]={hEventLauncherJobFinished,LauncherProcessInformation.hProcess};
    DWORD dwRet = ::WaitForMultipleObjects(_countof(WaitingHandles),WaitingHandles,FALSE,INFINITE);
    ::CloseHandle(LauncherProcessInformation.hProcess);
    ::CloseHandle(LauncherProcessInformation.hThread);

    // if event is not the hEventLauncherJobFinished one
    if (dwRet!=WAIT_OBJECT_0)
    {
        // no specific action as sub process is resumed
    }

    // resume thread if needed
    if (!bOriginalySuspended)
        ::ResumeThread(lpProcessInformation->hThread);

    // restore original create process last error
    ::SetLastError(dwLastError);
    return bRetValue;
}