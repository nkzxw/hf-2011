#pragma once

#include <Windows.h>
#include <TlHelp32.h>

namespace x86CrossCompatibility
{
#ifndef HANDLE64
#define HANDLE64 __int64
#endif

    typedef struct tagMODULEENTRY32W64
    {
        DWORD   dwSize;
        DWORD   th32ModuleID;       // This module
        DWORD   th32ProcessID;      // owning process
        DWORD   GlblcntUsage;       // Global usage count on the module
        DWORD   ProccntUsage;       // Module usage count in th32ProcessID's context
        __int64 modBaseAddr;        // Base address of module in th32ProcessID's context
        DWORD   modBaseSize;        // Size in bytes of module starting at modBaseAddr
        __int64 hModule;            // The hModule of this module in th32ProcessID's context
        WCHAR   szModule[MAX_MODULE_NAME32 + 1];
        WCHAR   szExePath[MAX_PATH];
    } MODULEENTRY32W64;
    typedef MODULEENTRY32W64 *  PMODULEENTRY32W64;
    typedef MODULEENTRY32W64 *  LPMODULEENTRY32W64;

    typedef struct tagMODULEENTRY32A64
    {
        DWORD   dwSize;
        DWORD   th32ModuleID;       // This module
        DWORD   th32ProcessID;      // owning process
        DWORD   GlblcntUsage;       // Global usage count on the module
        DWORD   ProccntUsage;       // Module usage count in th32ProcessID's context
        __int64 modBaseAddr;        // Base address of module in th32ProcessID's context
        DWORD   modBaseSize;        // Size in bytes of module starting at modBaseAddr
        __int64 hModule;            // The hModule of this module in th32ProcessID's context
        char    szModule[MAX_MODULE_NAME32 + 1];
        char    szExePath[MAX_PATH];
    } MODULEENTRY32A64;
    typedef MODULEENTRY32A64 *  PMODULEENTRY32A64;
    typedef MODULEENTRY32A64 *  LPMODULEENTRY32A64;

#ifdef UNICODE
#define MODULEENTRY3264 MODULEENTRY32W64
#define PMODULEENTRY3264 PMODULEENTRY32W64
#define LPMODULEENTRY3264 LPMODULEENTRY32W64
#else
#define MODULEENTRY3264 MODULEENTRY32A64
#define PMODULEENTRY3264 PMODULEENTRY32A64
#define LPMODULEENTRY3264 LPMODULEENTRY32A64
#endif  // !UNICODE


typedef enum _CommandId
{
    COMMAND_ID_Stopx86CrossCompatibilityProcess=0,
    COMMAND_ID_CreateToolhelp32Snapshot,
    COMMAND_ID_Module32FirstA,
    COMMAND_ID_Module32FirstW,
    COMMAND_ID_Module32NextA,
    COMMAND_ID_Module32NextW,
    COMMAND_ID_CloseHandle,
    COMMAND_ID_VirtualProtectEx,
    COMMAND_ID_VirtualQueryEx,
    COMMAND_ID_ReadProcessMemory,
    COMMAND_ID_WriteProcessMemory,
    COMMAND_ID_OpenProcess,
    COMMAND_ID_NtQueryInformationProcess,
    COMMAND_ID_SHGetFolderPathA,
    COMMAND_ID_SHGetFolderPathW
};

    typedef struct _BaseHeader
    {
        _CommandId CommandId;
    }BASE_HEADER;

    typedef struct CreateToolhelp32SnapshotQuery
    {
        BASE_HEADER Header;
        DWORD dwFlags;
        DWORD th32ProcessID;
    }CREATETOOLHELP32SNAPSHOT_QUERY;
    typedef struct CreateToolhelp32SnapshotReply
    {
        BASE_HEADER Header;
        HANDLE64 Handle;
    }CREATETOOLHELP32SNAPSHOT_REPLY;


    typedef struct Module32FirstQuery
    {
        BASE_HEADER Header;
        HANDLE64 hSnapshot;
    }MODULE32FIRST_QUERY;

    typedef struct Module32FirstAReply
    {
        BASE_HEADER Header;
        MODULEENTRY32A64 me;
        BOOL RetValue;
    }MODULE32FIRSTA_REPLY;

    typedef struct Module32FirstWReply
    {
        BASE_HEADER Header;
        MODULEENTRY32W64 me;
        BOOL RetValue;
    }MODULE32FIRST_REPLY;


    typedef struct Module32NextQuery
    {
        BASE_HEADER Header;
        HANDLE64 hSnapshot;
    }MODULE32NEXT_QUERY;
    typedef struct Module32NextAReply
    {
        BASE_HEADER Header;
        MODULEENTRY32A64 me;
        BOOL RetValue;
    }MODULE32NEXTA_REPLY;

    typedef struct Module32NextWReply
    {
        BASE_HEADER Header;
        MODULEENTRY32W64 me;
        BOOL RetValue;
    }MODULE32NEXTW_REPLY;

    typedef struct CloseHandleQuery
    {
        BASE_HEADER Header;
        HANDLE64 Handle;
    }CLOSEHANDLE_QUERY;
    typedef struct CloseHandleReply
    {
        BASE_HEADER Header;
        BOOL RetValue;
    }CLOSEHANDLE_REPLY;

    typedef struct VirtualProtectExQuery
    {
        BASE_HEADER Header;
		HANDLE64 hProcess;
		__int64 lpAddress;
		__int64 dwSize;
		DWORD flNewProtect;
    }VIRTUALPROTECTEX_QUERY;
    typedef struct VirtualProtectExReply
    {
        BASE_HEADER Header;
        DWORD OldProtect;
        BOOL RetValue;
    }VIRTUALPROTECTEX_REPLY;

    typedef struct VirtualQueryExQuery
    {
        BASE_HEADER Header;
        HANDLE64 hProcess;
        __int64 lpAddress;
    }VIRTUALQUERYEX_QUERY;
    typedef struct VirtualQueryExReply
    {
        BASE_HEADER Header;
        MEMORY_BASIC_INFORMATION64 lpBuffer;
        __int64 RetValue;
    }VIRTUALQUERYEX_REPLY;


    typedef struct ReadProcessMemoryQuery
    {
        BASE_HEADER Header;
		HANDLE64 hProcess;
		__int64 lpBaseAddress;
		__int64 nSize;
    }READPROCESSMEMORY_QUERY;
    typedef struct ReadProcessMemoryReply
    {
        BASE_HEADER Header;
        BOOL RetValue;
        __int64 NumberOfBytesRead;
        /* Buffer in case of success */
    }READPROCESSMEMORY_REPLY;

    typedef struct WriteProcessMemoryQuery
    {
        BASE_HEADER Header;
        HANDLE64 hProcess;
        __int64 lpBaseAddress;
        __int64 nSize;
        /* Buffer */
    }WRITEPROCESSMEMORY_QUERY;
    typedef struct WriteProcessMemoryReply
    {
        BASE_HEADER Header;
        BOOL RetValue;
        __int64 NumberOfBytesWritten;
    }WRITEPROCESSMEMORY_REPLY;

    typedef struct OpenProcessQuery
    {
        BASE_HEADER Header;
		DWORD dwDesiredAccess;
		BOOL bInheritHandle;
		DWORD dwProcessId;
    }OPENPROCESS_QUERY;
    typedef struct OpenProcessReply
    {
        BASE_HEADER Header;
        HANDLE64 RetValue;
    }OPENPROCESS_REPLY;

    typedef struct NtQueryInformationProcessQuery
    {
        BASE_HEADER Header;
        HANDLE64 ProcessHandle;
        __int64 ProcessInformationClass;
        ULONG ProcessInformationLength;
    }NTQUERYINFORMATIONPROCESS_QUERY;
    typedef struct NtQueryInformationProcessReply
    {
        BASE_HEADER Header;
        LONG RetValue;
        ULONG ReturnLength;
        /* Buffer in case of success */
    }NTQUERYINFORMATIONPROCESS_REPLY;

}
