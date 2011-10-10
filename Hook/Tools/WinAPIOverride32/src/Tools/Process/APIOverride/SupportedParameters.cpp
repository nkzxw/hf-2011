/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
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

//-----------------------------------------------------------------------------
// Object: manages supported parameters 
//          common to winapioverride.exe and Apioverride.dll
//-----------------------------------------------------------------------------

#pragma message (__FILE__ " Information : Include SupportedParameters.h before including windows.h, or use _WINSOCKAPI_ preprocessor define\r\n")

#include "SupportedParameters.h"
#include "UserDefine.h"
#include "UserDataType.h"
#include "UserDataTypeVar.h"
#include "../../String/SecureTcscat.h"
#include "../../File/StdFileOperations.h"
#pragma intrinsic (memcpy,memset,memcmp)

// Known Parameter Table (used for parsing config file)
// order is not important, but to speed up, put more used parameter types in first positions
SUPPORTED_PARAMETERS_STRUCT SupportedParametersArray[] = {
    //  ParamName,ParamType,sizeof data (0 if param is a pointer),sizeof pointed data (0 if param is not a pointer)
    //  param name can be upper or lower case as an insensitive comparison is done
    
    // PARAM_POINTER and PPOINTER are PVOID and PVOID* like,
    // but PARAM_PVOID and PARAM_PPVOID are managed as Byte array pointer
    // instead PARAM_POINTER is managed as DWORD/INT64 and PARAM_PPOINTER is managed as DWORD*/INT64*
    {_T("PVOID"), PARAM_POINTER,sizeof(PBYTE),0},
    {_T("PCVOID"), PARAM_POINTER,sizeof(PBYTE),0},
    {_T("PVOID*"), PARAM_PPOINTER,0,sizeof(PBYTE)},

    {_T("PSTR"),   PARAM_PSTR,0,0}, // for string pointed value size is automatically found --> let i to 0
    {_T("PCSTR"),  PARAM_PSTR,0,0},
    {_T("LPSTR"),  PARAM_PSTR,0,0},
    {_T("LPCSTR"), PARAM_PSTR,0,0},
    {_T("CHAR*"),  PARAM_PSTR,0,0},
    {_T("SEC_CHAR*"),  PARAM_PSTR,0,0},
    {_T("PCHAR"),  PARAM_PSTR,0,0},
    {_T("LPCHAR"),  PARAM_PSTR,0,0},
    {_T("PWSTR"),  PARAM_PWSTR,0,0},
    {_T("PCWSTR"), PARAM_PWSTR,0,0},
    {_T("LPWSTR"), PARAM_PWSTR,0,0},
    {_T("LPCWSTR"),PARAM_PWSTR,0,0},
    {_T("LPCWSTR"),PARAM_PWSTR,0,0},
    {_T("WCHAR*"),PARAM_PWSTR,0,0},
    {_T("PWCHAR"),PARAM_PWSTR,0,0},
    {_T("LPWCHAR"),PARAM_PWSTR,0,0},
    {_T("PCWCH"),PARAM_PWSTR,0,0},
    {_T("wchar_t*"),PARAM_PWSTR,0,0},
    {_T("SEC_WCHAR*"),PARAM_PWSTR,0,0},
    {_T("BSTR"),  PARAM_BSTR,0,0}, // dissociate PARAM_PWSTR and PARAM_BSTR only for type representation
                                   // in COM method info displaying, and bstr generally needs SysAllocString
    {_T("OLECHAR*"),  PARAM_BSTR,0,0},
    {_T("LPOLESTR"),  PARAM_BSTR,0,0},
    

    {_T("VOID*"),  PARAM_PVOID,0,sizeof(PBYTE)},
    {_T("PVOID"),  PARAM_PVOID,0,sizeof(PBYTE)},
    {_T("LPVOID"), PARAM_PVOID,0,sizeof(PBYTE)},
    {_T("LPCVOID"),PARAM_PVOID,0,sizeof(PBYTE)},
    {_T("VOID**"),  PARAM_PPVOID,0,sizeof(PBYTE)},
    {_T("PVOID*"),  PARAM_PPVOID,0,sizeof(PBYTE)},
    {_T("LPVOID*"),  PARAM_PPVOID,0,sizeof(PBYTE)},
    {_T("LPCVOID*"),  PARAM_PPVOID,0,sizeof(PBYTE)},

    {_T("CHAR"),   PARAM_CHAR,sizeof(CHAR),0},
    {_T("SEC_CHAR"),   PARAM_CHAR,sizeof(CHAR),0},
    {_T("UCHAR"),  PARAM_UCHAR,sizeof(UCHAR),0},
    {_T("BOOLEAN"),PARAM_BOOLEAN,sizeof(BOOLEAN),0},
    {_T("BYTE"),   PARAM_BYTE,sizeof(BYTE),0},
    {_T("__int8"),   PARAM_BYTE,sizeof(BYTE),0},
    {_T("BYTE*"),  PARAM_PBYTE,0,sizeof(BYTE)},
    {_T("PBYTE"),  PARAM_PBYTE,0,sizeof(BYTE)},
    {_T("LPBYTE"),  PARAM_PBYTE,0,sizeof(BYTE)},
    {_T("BOOLEAN*"),PARAM_PBOOLEAN,0,sizeof(BYTE)},
    {_T("PBOOLEAN"),PARAM_PBOOLEAN,0,sizeof(BYTE)},
    {_T("LPBOOLEAN"),PARAM_PBOOLEAN,0,sizeof(BYTE)},
    {_T("UCHAR*"),  PARAM_PUCHAR,0,sizeof(BYTE)},
    {_T("PUCHAR"),  PARAM_PUCHAR,0,sizeof(BYTE)},
    {_T("LPUCHAR"),  PARAM_PUCHAR,0,sizeof(BYTE)},

    {_T("WCHAR"),  PARAM_WCHAR,sizeof(WCHAR),0},
    {_T("OLECHAR"),  PARAM_WCHAR,sizeof(WCHAR),0},
    {_T("SEC_WCHAR"),  PARAM_WCHAR,sizeof(wchar_t),0},
    {_T("wchar_t"),PARAM_WCHAR,sizeof(wchar_t),0},
    {_T("wctrans_t"),PARAM_WCHAR,sizeof(wchar_t),0},

    {_T("WORD"),   PARAM_WORD,sizeof(WORD),0},
    {_T("ATOM"),   PARAM_WORD,sizeof(WORD),0},
    {_T("LANGID"),   PARAM_WORD,sizeof(WORD),0},
    {_T("short"),  PARAM_SHORT,sizeof(short),0},
    {_T("SHORT"),  PARAM_SHORT,sizeof(short),0},
    {_T("__int16"),  PARAM_SHORT,sizeof(USHORT),0},
    {_T("USHORT"),  PARAM_USHORT,sizeof(USHORT),0},
    {_T("u_short"),PARAM_WORD,sizeof(u_short),0},
    {_T("wint_t"),PARAM_WORD,sizeof(u_short),0},
    {_T("wctype_t"),PARAM_WORD,sizeof(u_short),0},
    {_T("WPARAM"), PARAM_WPARAM,sizeof(WPARAM),0},
    {_T("short*"), PARAM_PSHORT,0,sizeof(short)},
    {_T("SHORT*"), PARAM_PSHORT,0,sizeof(short)},
    {_T("PSHORT"), PARAM_PSHORT,0,sizeof(short)},
    {_T("LPSHORT"), PARAM_PSHORT,0,sizeof(short)},
    {_T("USHORT*"), PARAM_PWORD,0,sizeof(WORD)},
    {_T("PUSHORT"), PARAM_PWORD,0,sizeof(WORD)},
    {_T("LPUSHORT"), PARAM_PWORD,0,sizeof(WORD)},
    {_T("WORD*"),  PARAM_PWORD,0,sizeof(WORD)},
    {_T("PWORD"),  PARAM_PWORD,0,sizeof(WORD)},
    {_T("LPWORD"), PARAM_PWORD,0,sizeof(WORD)},
    {_T("u_short*"), PARAM_PWORD,0,sizeof(u_short)},

    {_T("BOOL"),   PARAM_BOOL,sizeof(BOOL),0},
    {_T("BOOL*"),  PARAM_PBOOL,0,sizeof(BOOL)},
    {_T("PBOOL"),  PARAM_PBOOL,0,sizeof(BOOL)},
    {_T("LPBOOL"), PARAM_PBOOL,0,sizeof(BOOL)},

    {_T("SIZE_T"), PARAM_SIZE_T,sizeof(SIZE_T),0},
    {_T("_fsize_t"), PARAM_SIZE_T,sizeof(SIZE_T),0},
    {_T("SSIZE_T"), PARAM_SIZE_T,sizeof(SIZE_T),0},
    {_T("SIZE_T*"),PARAM_PSIZE_T,0,sizeof(SIZE_T)},
    {_T("PSIZE_T"),PARAM_PSIZE_T,0,sizeof(SIZE_T)},
    {_T("LPSIZE_T"),PARAM_PSIZE_T,0,sizeof(SIZE_T)},

    {_T("INT"),    PARAM_INT,sizeof(INT),0},
    {_T("__int32"),    PARAM_INT,sizeof(INT),0},
    {_T("INT*"),   PARAM_PINT,0,sizeof(INT)},
    {_T("PINT"),   PARAM_PINT,0,sizeof(INT)},
    {_T("LPINT"),  PARAM_PINT,0,sizeof(INT)},

    {_T("UINT"),   PARAM_UINT,sizeof(UINT),0},
    {_T("UINT*"),  PARAM_PUINT,0,sizeof(UINT)},
    {_T("PUINT"),  PARAM_PUINT,0,sizeof(UINT)},
    {_T("LPUINT"), PARAM_PUINT,0,sizeof(UINT)},

    {_T("LONG"),   PARAM_LONG,sizeof(LONG),0},
    {_T("LONG32"),   PARAM_LONG,sizeof(LONG),0},
    {_T("NTSTATUS"),PARAM_NTSTATUS,sizeof(LONG),0},
    {_T("LONG_PTR"),PARAM_LONG_PTR,sizeof(LONG_PTR),0},
    {_T("LRESULT"),PARAM_LONG_PTR,sizeof(LONG_PTR),0},
    {_T("LPARAM"), PARAM_LPARAM,sizeof(LPARAM),0},
    {_T("LONG*"),  PARAM_PLONG,0,sizeof(LONG)},
    {_T("PLONG"),  PARAM_PLONG,0,sizeof(LONG)},
    {_T("LPLONG"), PARAM_PLONG,0,sizeof(LONG)},
    {_T("LONG_PTR*"),PARAM_PLONG_PTR,0,sizeof(LONG_PTR)},
    {_T("PLONG_PTR"),PARAM_PLONG_PTR,0,sizeof(LONG_PTR)},
    {_T("LPLONG_PTR"),PARAM_PLONG_PTR,0,sizeof(LONG_PTR)},

    {_T("ULONG_PTR"), PARAM_ULONG_PTR,sizeof(ULONG_PTR),0},
    {_T("DWORD_PTR"), PARAM_ULONG_PTR,sizeof(ULONG_PTR),0},
    {_T("UINT_PTR"), PARAM_ULONG_PTR,sizeof(UINT_PTR),0},
    {_T("POINTER_32"), PARAM_ULONG_PTR,sizeof(UINT_PTR),0},
    {_T("SOCKET"), PARAM_SOCKET,sizeof(SOCKET),0},
    {_T("ULONG"),  PARAM_ULONG,sizeof(ULONG),0},
    {_T("u_long"),  PARAM_ULONG,sizeof(u_long),0},

    {_T("DWORD"),  PARAM_DWORD,sizeof(DWORD),0},
    {_T("DWORD32"),  PARAM_DWORD,sizeof(DWORD),0},
    {_T("LCTYPE"),  PARAM_DWORD,sizeof(DWORD),0},
    {_T("ULONG*"), PARAM_PULONG,0,sizeof(ULONG)},
    {_T("PULONG"), PARAM_PULONG,0,sizeof(ULONG)},
    {_T("LPULONG"), PARAM_PULONG,0,sizeof(ULONG)},
    {_T("SOCKET*"), PARAM_PSOCKET,0,sizeof(SOCKET)},
    {_T("u_long*"), PARAM_PULONG,0,sizeof(u_long)},
    {_T("DWORD*"), PARAM_PDWORD,0,sizeof(DWORD)},
    {_T("PDWORD"), PARAM_PDWORD,0,sizeof(DWORD)},
    {_T("LPDWORD"),PARAM_PDWORD,0,sizeof(DWORD)},
    {_T("PLCID"),PARAM_PDWORD,0,sizeof(DWORD)},
    {_T("ULONG_PTR*"), PARAM_PULONG_PTR,0,sizeof(ULONG_PTR)},
    {_T("PULONG_PTR"), PARAM_PULONG_PTR,0,sizeof(ULONG_PTR)},
    {_T("LPULONG_PTR"), PARAM_PULONG_PTR,0,sizeof(ULONG_PTR)},
    {_T("LSA_HANDLE"), PARAM_LSA_HANDLE,sizeof(ULONG_PTR),0},
    {_T("LSA_HANDLE*"), PARAM_PLSA_HANDLE,0,sizeof(ULONG_PTR)},
    {_T("PLSA_HANDLE"), PARAM_PLSA_HANDLE,0,sizeof(ULONG_PTR)},
    {_T("LPLSA_HANDLE"), PARAM_PLSA_HANDLE,0,sizeof(ULONG_PTR)},

    {_T("HKEY"), PARAM_HKEY,sizeof(HKEY),0},
    {_T("PHKEY"), PARAM_PHKEY,0,sizeof(HKEY)},
    {_T("HKEY*"), PARAM_PHKEY,0,sizeof(HKEY)},
    {_T("COLORREF"),  PARAM_COLORREF,sizeof(COLORREF),0},
    {_T("COLORREF*"),PARAM_PCOLORREF,0,sizeof(COLORREF)},
    {_T("LPCOLORREF"),PARAM_PCOLORREF,0,sizeof(COLORREF)},
    {_T("PFNCALLBACK"), PARAM_PFNCALLBACK,sizeof(PFNCALLBACK),0},
    {_T("LCID"), PARAM_LCID,sizeof(LCID),0},
      
    {_T("HANDLE"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HACCEL"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HCONVLIST"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HCURSOR"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HDDEDATA"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HDROP"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HDWP"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HENHMETAFILE"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HFILE"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HHOOK"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HIMAGELIST"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HIMC"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HKL"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HLOCAL"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HMONITOR"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HPEN"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HRSRC"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("HWINSTA"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("SC_LOCK"), PARAM_HANDLE,sizeof(HANDLE),0},
    {_T("SERVICE_STATUS_HANDLE"), PARAM_HANDLE,sizeof(HANDLE),0},

    {_T("HINSTANCE"), PARAM_HINSTANCE,sizeof(HINSTANCE),0},
    {_T("HWND"),   PARAM_HWND,sizeof(HWND),0},
    {_T("HMODULE"),PARAM_HMODULE,sizeof(HMODULE),0},
    {_T("HMODULE*"),PARAM_PHMODULE,0,sizeof(HMODULE)},
    {_T("PHMODULE"),PARAM_PHMODULE,0,sizeof(HMODULE)},
    {_T("LPHMODULE"),PARAM_PHMODULE,0,sizeof(HMODULE)},
    {_T("HANDLE*"),PARAM_PHANDLE,0,sizeof(HANDLE)},
    {_T("PHANDLE"),PARAM_PHANDLE,0,sizeof(HANDLE)},
    {_T("LPHANDLE"),PARAM_PHANDLE,0,sizeof(HANDLE)},
    {_T("HDESK"),PARAM_HDESK,sizeof(HDESK),0},
    {_T("HBRUSH"),PARAM_HBRUSH,sizeof(HBRUSH),0},
    {_T("HRGN"),PARAM_HRGN,sizeof(HRGN),0},
    {_T("HDPA"),PARAM_HDPA,sizeof(PVOID),0},
    {_T("HDSA"),PARAM_HDSA,sizeof(PVOID),0},
    {_T("HDC"),    PARAM_HDC,sizeof(HDC),0},
    {_T("HICON"),  PARAM_HICON,sizeof(HICON),0},
    {_T("HICON*"),  PARAM_PHICON,0,sizeof(HICON)},
    {_T("WNDPROC"),PARAM_WNDPROC,sizeof(WNDPROC),0},
    {_T("HMENU"),  PARAM_HMENU,sizeof(HMENU),0},
    {_T("HIMAGELIST"),  PARAM_HIMAGELIST,sizeof(PVOID),0},
    {_T("DLGPROC"),PARAM_DLGPROC,sizeof(DLGPROC),0},
    {_T("FARPROC"),PARAM_FARPROC,sizeof(FARPROC),0},
    {_T("LPWSAOVERLAPPED_COMPLETION_ROUTINE"),PARAM_FARPROC,sizeof(FARPROC),0},
    {_T("HPALETTE"), PARAM_HPALETTE,sizeof(HPALETTE),0},
    {_T("HFONT"), PARAM_HFONT,sizeof(HFONT),0},
    {_T("HMETAFILE"), PARAM_HMETAFILE,sizeof(HMETAFILE),0},
    {_T("HGDIOBJ"), PARAM_HGDIOBJ,sizeof(HGDIOBJ),0},
    {_T("HCOLORSPACE"), PARAM_HCOLORSPACE,sizeof(HCOLORSPACE),0},
    {_T("HBITMAP"), PARAM_HBITMAP,sizeof(HBITMAP),0},
    {_T("HCONV"), PARAM_HCONV,sizeof(HCONV),0},
    {_T("HSZ"), PARAM_HSZ,sizeof(HSZ),0},
    {_T("HDDEDATA"), PARAM_HDDEDATA,sizeof(HDDEDATA),0},
    {_T("SC_HANDLE"), PARAM_SC_HANDLE,sizeof(SC_HANDLE),0},
    {_T("HCERTSTORE"), PARAM_HCERTSTORE,sizeof(HCERTSTORE),0},
    {_T("HGLOBAL"), PARAM_HGLOBAL,sizeof(HGLOBAL),0},
    {_T("PSID"), PARAM_PSID,sizeof(PSID),0},
    {_T("PSID*"),PARAM_PPSID,0,sizeof(PSID)},
    {_T("PSECURITY_DESCRIPTOR"), PARAM_PSECURITY_DESCRIPTOR,sizeof(PSECURITY_DESCRIPTOR),0},
    {_T("PSECURITY_DESCRIPTOR*"),PARAM_PPSECURITY_DESCRIPTOR,0,sizeof(PSECURITY_DESCRIPTOR)},
    {_T("SECURITY_INFORMATION"), PARAM_SECURITY_INFORMATION,sizeof(SECURITY_INFORMATION),0},
    {_T("REGSAM"), PARAM_REGSAM,sizeof(REGSAM),0},


    {_T("WNDCLASS"), PARAM_WNDCLASS,sizeof(WNDCLASS),0},
    {_T("WNDCLASS*"),PARAM_PWNDCLASS,0,sizeof(WNDCLASS)},
    {_T("PWNDCLASS"),PARAM_PWNDCLASS,0,sizeof(WNDCLASS)},
    {_T("LPWNDCLASS"),PARAM_PWNDCLASS,0,sizeof(WNDCLASS)},

    {_T("WNDCLASSEX"), PARAM_WNDCLASSEX,sizeof(WNDCLASSEX),0},
    {_T("WNDCLASSEX*"),PARAM_PWNDCLASSEX,0,sizeof(WNDCLASSEX)},
    {_T("PWNDCLASSEX"),PARAM_PWNDCLASSEX,0,sizeof(WNDCLASSEX)},
    {_T("LPWNDCLASSEX"),PARAM_PWNDCLASSEX,0,sizeof(WNDCLASSEX)},

    {_T("POINT"), PARAM_POINT,sizeof(POINT),0},
    {_T("POINT*"),PARAM_PPOINT,0,sizeof(POINT)},
    {_T("PPOINT"),PARAM_PPOINT,0,sizeof(POINT)},
    {_T("LPPOINT"),PARAM_PPOINT,0,sizeof(POINT)},
    {_T("POINTL"), PARAM_POINT,sizeof(POINT),0},
    {_T("POINTL*"),PARAM_PPOINT,0,sizeof(POINT)},
    {_T("PPOINTL"),PARAM_PPOINT,0,sizeof(POINT)},

    {_T("SIZE"), PARAM_SIZE,sizeof(SIZE),0},
    {_T("SIZEL"), PARAM_SIZE,sizeof(SIZE),0},
    {_T("SIZE*"),PARAM_PSIZE,0,sizeof(SIZE)},
    {_T("PSIZE"),PARAM_PSIZE,0,sizeof(SIZE)},
    {_T("LPSIZE"),PARAM_PSIZE,0,sizeof(SIZE)},
    {_T("PSIZEL"),PARAM_PSIZE,0,sizeof(SIZE)},
    {_T("LPSIZEL"),PARAM_PSIZE,0,sizeof(SIZE)},

    {_T("RECT"), PARAM_RECT,sizeof(RECT),0},
    {_T("RECT*"),PARAM_PRECT,0,sizeof(RECT)},
    {_T("PRECT"),PARAM_PRECT,0,sizeof(RECT)},
    {_T("LPRECT"),PARAM_PRECT,0,sizeof(RECT)},
    {_T("LPCRECT"),PARAM_PRECT,0,sizeof(RECT)},
    {_T("RECTL"), PARAM_RECT,sizeof(RECT),0},
    {_T("RECTL*"),PARAM_PRECT,0,sizeof(RECT)},
    {_T("PRECTL"),PARAM_PRECT,0,sizeof(RECT)},
    {_T("LPRECTL"),PARAM_PRECT,0,sizeof(RECT)},

    {_T("float"), PARAM_FLOAT,sizeof(float),0},
    {_T("float*"), PARAM_PFLOAT,0,sizeof(float)},
    {_T("PFLOAT"), PARAM_PFLOAT,0,sizeof(float)},
    {_T("LPFLOAT"), PARAM_PFLOAT,0,sizeof(float)},

    {_T("double"), PARAM_DOUBLE,sizeof(double),0},
    {_T("double*"), PARAM_PDOUBLE,0,sizeof(double)},
    {_T("PDOUBLE"), PARAM_PDOUBLE,0,sizeof(double)},
    {_T("LPDOUBLE"), PARAM_PDOUBLE,0,sizeof(double)},

    {_T("INT64"), PARAM_INT64,sizeof(INT64),0},
    {_T("LONG64"), PARAM_INT64,sizeof(INT64),0},
    {_T("UINT64"), PARAM_INT64,sizeof(INT64),0},
    {_T("__int64"), PARAM_INT64,sizeof(__int64),0},
    {_T("DWORD64"), PARAM_INT64,sizeof(__int64),0},
    {_T("INT64*"), PARAM_PINT64,0,sizeof(INT64)},
    {_T("PINT64"), PARAM_PINT64,0,sizeof(INT64)},
    {_T("LPINT64"), PARAM_PINT64,0,sizeof(INT64)},
    {_T("__int64*"), PARAM_PINT64,0,sizeof(INT64)},
    {_T("UINT64*"), PARAM_PINT64,0,sizeof(INT64)},
    {_T("PUINT64"), PARAM_PINT64,0,sizeof(INT64)},
    {_T("LPUINT64"), PARAM_PINT64,0,sizeof(INT64)},
    {_T("LONGLONG"), PARAM_INT64,sizeof(ULONGLONG),0},
    {_T("LONGLONG*"), PARAM_PINT64,0,sizeof(ULONGLONG)},
    {_T("PLONGLONG"), PARAM_PINT64,0,sizeof(ULONGLONG)},
    {_T("ULONG64"), PARAM_INT64,sizeof(ULONG64),0},
    {_T("ULONG64*"), PARAM_PINT64,0,sizeof(ULONG64)},
    {_T("PULONG64"), PARAM_PINT64,0,sizeof(ULONG64)},
    {_T("LPULONG64"), PARAM_PINT64,0,sizeof(ULONG64)},
    {_T("DWORD64"), PARAM_INT64,sizeof(INT64),0},
    {_T("DWORD64*"), PARAM_PINT64,0,sizeof(INT64)},
    {_T("PDWORD64"), PARAM_PINT64,0,sizeof(INT64)},
    {_T("LPDWORD64"), PARAM_PINT64,0,sizeof(INT64)},
    {_T("ULONGLONG"), PARAM_INT64,sizeof(ULONGLONG),0},
    {_T("ULONGLONG*"), PARAM_PINT64,0,sizeof(ULONGLONG)},
    {_T("PULONGLONG"), PARAM_PINT64,0,sizeof(ULONGLONG)},
    {_T("DWORDLONG"), PARAM_INT64,sizeof(DWORDLONG),0},
    {_T("DWORDLONG*"), PARAM_PINT64,0,sizeof(DWORDLONG)},
    {_T("PDWORDLONG"), PARAM_PINT64,0,sizeof(DWORDLONG)},
    {_T("__ptr64"), PARAM_PINT64,sizeof(__int64),0},
    {_T("POINTER_64"), PARAM_PINT64,sizeof(__int64),0},

    {_T("MSG*"),PARAM_PMSG,0,sizeof(MSG)},
    {_T("PMSG"),PARAM_PMSG,0,sizeof(MSG)},
    {_T("LPMSG"),PARAM_PMSG,0,sizeof(MSG)},

    {_T("LARGE_INTEGER"), PARAM_LARGE_INTEGER,sizeof(LARGE_INTEGER),0},
    {_T("LARGE_INTEGER*"),PARAM_PLARGE_INTEGER,0,sizeof(LARGE_INTEGER)},
    {_T("PLARGE_INTEGER"),PARAM_PLARGE_INTEGER,0,sizeof(LARGE_INTEGER)},
    {_T("ULARGE_INTEGER"), PARAM_LARGE_INTEGER,sizeof(LARGE_INTEGER),0},
    {_T("ULARGE_INTEGER*"),PARAM_PLARGE_INTEGER,0,sizeof(LARGE_INTEGER)},
    {_T("PULARGE_INTEGER"),PARAM_PLARGE_INTEGER,0,sizeof(LARGE_INTEGER)},    

    {_T("GUID"), PARAM_GUID,sizeof(GUID),0},
    {_T("GUID*"),PARAM_PGUID,0,sizeof(GUID)},
    {_T("PGUID"),PARAM_PGUID,0,sizeof(GUID)},
    {_T("LPGUID"),PARAM_PGUID,0,sizeof(GUID)},
    {_T("REFGUID"),PARAM_PGUID,0,sizeof(GUID)},
    {_T("IID"), PARAM_IID,sizeof(GUID),0},
    {_T("IID*"),PARAM_PIID,0,sizeof(GUID)},
    {_T("PIID"),PARAM_PIID,0,sizeof(GUID)},
    {_T("LPIID"),PARAM_PIID,0,sizeof(GUID)},
    {_T("REFIID"),PARAM_PIID,0,sizeof(GUID)},
    {_T("CLSID"), PARAM_CLSID,sizeof(GUID),0},
    {_T("CLSID*"),PARAM_PCLSID,0,sizeof(GUID)},
    {_T("PCLSID"),PARAM_PCLSID,0,sizeof(GUID)},
    {_T("LPCLSID"),PARAM_PCLSID,0,sizeof(GUID)},
    {_T("REFCLSID"),PARAM_PCLSID,0,sizeof(GUID)},
    {_T("FMTID"), PARAM_FMTID,sizeof(GUID),0},
    {_T("FMTID*"),PARAM_PFMTID,0,sizeof(GUID)},
    {_T("PFMTID"),PARAM_PFMTID,0,sizeof(GUID)},
    {_T("LPFMTID"),PARAM_PFMTID,0,sizeof(GUID)},
    {_T("REFFMTID"),PARAM_PFMTID,0,sizeof(GUID)},

    {_T("CRITICAL_SECTION"), PARAM_CRITICAL_SECTION,sizeof(CRITICAL_SECTION),0},
    {_T("CRITICAL_SECTION*"), PARAM_PCRITICAL_SECTION,0,sizeof(CRITICAL_SECTION)},
    {_T("PCRITICAL_SECTION"), PARAM_PCRITICAL_SECTION,0,sizeof(CRITICAL_SECTION)},
    {_T("LPCRITICAL_SECTION"), PARAM_PCRITICAL_SECTION,0,sizeof(CRITICAL_SECTION)},

    {_T("SYSTEMTIME"),  PARAM_SYSTEMTIME,sizeof(SYSTEMTIME),0},
    {_T("SYSTEMTIME*"),PARAM_PSYSTEMTIME,0,sizeof(SYSTEMTIME)},
    {_T("PSYSTEMTIME"),PARAM_PSYSTEMTIME,0,sizeof(SYSTEMTIME)},
    {_T("LPSYSTEMTIME"),PARAM_PSYSTEMTIME,0,sizeof(SYSTEMTIME)},

    {_T("FILETIME"),  PARAM_FILETIME,sizeof(FILETIME),0},
    {_T("FILETIME*"),PARAM_PFILETIME,0,sizeof(FILETIME)},
    {_T("PFILETIME"),PARAM_PFILETIME,0,sizeof(FILETIME)},
    {_T("LPFILETIME"),PARAM_PFILETIME,0,sizeof(FILETIME)},

    {_T("ACCESS_MASK"),  PARAM_ACCESS_MASK,sizeof(ACCESS_MASK),0},
    {_T("ACCESS_MASK*"),PARAM_PACCESS_MASK,0,sizeof(ACCESS_MASK)},
    {_T("PACCESS_MASK"),PARAM_PACCESS_MASK,0,sizeof(ACCESS_MASK)},

    {_T("SECURITY_ATTRIBUTES"), PARAM_SECURITY_ATTRIBUTES,sizeof(SECURITY_ATTRIBUTES),0},
    {_T("SECURITY_ATTRIBUTES*"),PARAM_PSECURITY_ATTRIBUTES,0,sizeof(SECURITY_ATTRIBUTES)},
    {_T("PSECURITY_ATTRIBUTES"),PARAM_PSECURITY_ATTRIBUTES,0,sizeof(SECURITY_ATTRIBUTES)},
    {_T("LPSECURITY_ATTRIBUTES"),PARAM_PSECURITY_ATTRIBUTES,0,sizeof(SECURITY_ATTRIBUTES)},

    {_T("ACL"), PARAM_ACL,sizeof(ACL),0},
    {_T("ACL*"),PARAM_PACL,0,sizeof(ACL)},
    {_T("PACL"),PARAM_PACL,0,sizeof(ACL)},

    {_T("LPCDLGTEMPLATE"),PARAM_PDLGTEMPLATE,0,sizeof(DLGTEMPLATE)},

    {_T("sockaddr"), PARAM_SOCKADDR,sizeof(sockaddr),0},
    {_T("sockaddr*"), PARAM_PSOCKADDR,0,sizeof(sockaddr)},
    {_T("PSOCKADDR"), PARAM_PSOCKADDR,0,sizeof(sockaddr)},
    {_T("LPSOCKADDR"), PARAM_PSOCKADDR,0,sizeof(sockaddr)},
    {_T("sockaddr_in"), PARAM_SOCKADDR_IN,sizeof(sockaddr_in),0},
    {_T("sockaddr_in*"), PARAM_PSOCKADDR_IN,0,sizeof(sockaddr_in)},
    {_T("hostent"), PARAM_HOSTENT,sizeof(hostent),0},
    {_T("hostent*"), PARAM_PHOSTENT,0,sizeof(hostent)},

    {_T("timeval*"), PARAM_PTIMEVAL,0,sizeof(timeval)},
    
    {_T("FILE*"),PARAM_PFILE,0,sizeof(FILE)},
    {_T("LPSTARTUPINFO"),PARAM_PSTARTUPINFO,0,sizeof(STARTUPINFO)},
    {_T("LPSTARTUPINFOW"),PARAM_PSTARTUPINFO,0,sizeof(STARTUPINFO)},

    {_T("LPSHELLEXECUTEINFO"),PARAM_PSHELLEXECUTEINFO,0,sizeof(SHELLEXECUTEINFO)},
    {_T("LPSHELLEXECUTEINFOW"),PARAM_PSHELLEXECUTEINFO,0,sizeof(SHELLEXECUTEINFO)},

    {_T("HCRYPTPROV"), PARAM_HCRYPTPROV,sizeof(HCRYPTPROV),0},
    {_T("HCRYPTKEY"), PARAM_HCRYPTKEY,sizeof(HCRYPTKEY),0},
    {_T("HCRYPTHASH"), PARAM_HCRYPTHASH,sizeof(HCRYPTHASH),0},

    {_T("PUNICODE_STRING"),PARAM_PUNICODE_STRING,0,0},// as string case
    {_T("UNICODE_STRING*"),PARAM_PUNICODE_STRING,0,0},

    {_T("PANSI_STRING"),PARAM_PANSI_STRING,0,0},
    {_T("ANSI_STRING*"),PARAM_PANSI_STRING,0,0},

    {_T("PSecHandle"),PARAM_PSECHANDLE,0,sizeof(SecHandle)},
    {_T("PCtxtHandle"),PARAM_PCTXTHANDLE,0,sizeof(SecHandle)},
    {_T("PCredHandle"),PARAM_PCREDHANDLE,0,sizeof(SecHandle)},

    {_T("MEMORY_BASIC_INFORMATION"), PARAM_MEMORY_BASIC_INFORMATION,sizeof(MEMORY_BASIC_INFORMATION),0},
    {_T("MEMORY_BASIC_INFORMATION*"),PARAM_PMEMORY_BASIC_INFORMATION,0,sizeof(MEMORY_BASIC_INFORMATION)},
    {_T("PMEMORY_BASIC_INFORMATION"),PARAM_PMEMORY_BASIC_INFORMATION,0,sizeof(MEMORY_BASIC_INFORMATION)},

    {_T("PROCESSENTRY32*"),PARAM_PPROCESSENTRY32A,0,sizeof(PROCESSENTRY32A)},
    {_T("LPPROCESSENTRY32"),PARAM_PPROCESSENTRY32A,0,sizeof(PROCESSENTRY32A)},
    {_T("PPROCESSENTRY32"),PARAM_PPROCESSENTRY32A,0,sizeof(PROCESSENTRY32A)},
    {_T("PROCESSENTRY32W*"),PARAM_PPROCESSENTRY32W,0,sizeof(PROCESSENTRY32W)},
    {_T("LPPROCESSENTRY32W"),PARAM_PPROCESSENTRY32W,0,sizeof(PROCESSENTRY32W)},
    {_T("PPROCESSENTRY32W"),PARAM_PPROCESSENTRY32W,0,sizeof(PROCESSENTRY32W)},
    {_T("MODULEENTRY32*"),PARAM_PMODULEENTRY32A,0,sizeof(MODULEENTRY32A)},
    {_T("LPMODULEENTRY32"),PARAM_PMODULEENTRY32A,0,sizeof(MODULEENTRY32A)},
    {_T("PMODULEENTRY32"),PARAM_PMODULEENTRY32A,0,sizeof(MODULEENTRY32A)},
    {_T("MODULEENTRY32W*"),PARAM_PMODULEENTRY32W,0,sizeof(MODULEENTRY32W)},
    {_T("PMODULEENTRY32W"),PARAM_PMODULEENTRY32W,0,sizeof(MODULEENTRY32W)},
    {_T("LPMODULEENTRY32W"),PARAM_PMODULEENTRY32W,0,sizeof(MODULEENTRY32W)},
    {_T("HEAPENTRY32*"),PARAM_PHEAPENTRY32,0,sizeof(HEAPENTRY32)},
    {_T("PHEAPENTRY32"),PARAM_PHEAPENTRY32,0,sizeof(HEAPENTRY32)},
    {_T("LPHEAPENTRY32"),PARAM_PHEAPENTRY32,0,sizeof(HEAPENTRY32)},
    {_T("THREADENTRY32*"),PARAM_PTHREADENTRY32,0,sizeof(THREADENTRY32)},
    {_T("PTHREADENTRY32"),PARAM_PTHREADENTRY32,0,sizeof(THREADENTRY32)},
    {_T("LPTHREADENTRY32"),PARAM_PTHREADENTRY32,0,sizeof(THREADENTRY32)},
    {_T("PROCESS_HEAP_ENTRY*"),PARAM_PPROCESS_HEAP_ENTRY,0,sizeof(PROCESS_HEAP_ENTRY)},
    {_T("PPROCESS_HEAP_ENTRY"),PARAM_PPROCESS_HEAP_ENTRY,0,sizeof(PROCESS_HEAP_ENTRY)},
    {_T("LPPROCESS_HEAP_ENTRY"),PARAM_PPROCESS_HEAP_ENTRY,0,sizeof(PROCESS_HEAP_ENTRY)},

    {_T("WIN32_FIND_DATA*"),PARAM_PWIN32_FIND_DATAA,0,sizeof(WIN32_FIND_DATAA)},
    {_T("PWIN32_FIND_DATA"),PARAM_PWIN32_FIND_DATAA,0,sizeof(WIN32_FIND_DATAA)},
    {_T("LPWIN32_FIND_DATA"),PARAM_PWIN32_FIND_DATAA,0,sizeof(WIN32_FIND_DATAA)},
    {_T("WIN32_FIND_DATAW*"),PARAM_PWIN32_FIND_DATAW,0,sizeof(WIN32_FIND_DATAW)},
    {_T("PWIN32_FIND_DATAW"),PARAM_PWIN32_FIND_DATAW,0,sizeof(WIN32_FIND_DATAW)},
    {_T("LPWIN32_FIND_DATAW"),PARAM_PWIN32_FIND_DATAW,0,sizeof(WIN32_FIND_DATAW)},
    
    {_T("IO_STATUS_BLOCK*"),PARAM_PIO_STATUS_BLOCK,0,sizeof(IO_STATUS_BLOCK)},
    {_T("PIO_STATUS_BLOCK"),PARAM_PIO_STATUS_BLOCK,0,sizeof(IO_STATUS_BLOCK)},

    {_T("PRINTDLG*"),PARAM_PPRINTDLG,0,sizeof(PRINTDLG)},
    {_T("LPPRINTDLG"),PARAM_PPRINTDLG,0,sizeof(PRINTDLG)},
    {_T("LPPRINTDLGA"),PARAM_PPRINTDLG,0,sizeof(PRINTDLG)},
    {_T("LPPRINTDLGW"),PARAM_PPRINTDLG,0,sizeof(PRINTDLG)},
    {_T("PRINTDLGEX*"),PARAM_PPRINTDLGEX,0,sizeof(PRINTDLGEX)},
    {_T("LPPRINTDLGEX"),PARAM_PPRINTDLGEX,0,sizeof(PRINTDLGEX)},
    {_T("LPPRINTDLGEXA"),PARAM_PPRINTDLGEX,0,sizeof(PRINTDLGEX)},
    {_T("LPPRINTDLGEXW"),PARAM_PPRINTDLGEX,0,sizeof(PRINTDLGEX)},
    {_T("PAGESETUPDLG*"),PARAM_PPAGESETUPDLG,0,sizeof(PAGESETUPDLG)},
    {_T("LPPAGESETUPDLG"),PARAM_PPAGESETUPDLG,0,sizeof(PAGESETUPDLG)},
    {_T("LPPAGESETUPDLGA"),PARAM_PPAGESETUPDLG,0,sizeof(PAGESETUPDLG)},
    {_T("LPPAGESETUPDLGW"),PARAM_PPAGESETUPDLG,0,sizeof(PAGESETUPDLG)},
    {_T("OPENFILENAME*"),PARAM_POPENFILENAME,0,sizeof(OPENFILENAME)},
    {_T("LPOPENFILENAME"),PARAM_POPENFILENAME,0,sizeof(OPENFILENAME)},
    {_T("LPOPENFILENAMEA"),PARAM_POPENFILENAME,0,sizeof(OPENFILENAME)},
    {_T("LPOPENFILENAMEW"),PARAM_POPENFILENAME,0,sizeof(OPENFILENAME)},
    {_T("CHOOSEFONT*"),PARAM_PCHOOSEFONT,0,sizeof(CHOOSEFONT)},
    {_T("LPCHOOSEFONT"),PARAM_PCHOOSEFONT,0,sizeof(CHOOSEFONT)},
    {_T("LPCHOOSEFONTA"),PARAM_PCHOOSEFONT,0,sizeof(CHOOSEFONT)},
    {_T("LPCHOOSEFONTW"),PARAM_PCHOOSEFONT,0,sizeof(CHOOSEFONT)},
    {_T("FINDREPLACE*"),PARAM_PFINDREPLACE,0,sizeof(FINDREPLACE)},
    {_T("LPFINDREPLACE"),PARAM_PFINDREPLACE,0,sizeof(FINDREPLACE)},
    {_T("LPFINDREPLACEA"),PARAM_PFINDREPLACE,0,sizeof(FINDREPLACE)},
    {_T("LPFINDREPLACEW"),PARAM_PFINDREPLACE,0,sizeof(FINDREPLACE)},
    {_T("BROWSEINFO*"),PARAM_PBROWSEINFO,0,sizeof(BROWSEINFO)},
    {_T("PBROWSEINFO"),PARAM_PBROWSEINFO,0,sizeof(BROWSEINFO)},
    {_T("LPBROWSEINFO"),PARAM_PBROWSEINFO,0,sizeof(BROWSEINFO)},
    {_T("LPBROWSEINFOA"),PARAM_PBROWSEINFO,0,sizeof(BROWSEINFO)},
    {_T("LPBROWSEINFOW"),PARAM_PBROWSEINFO,0,sizeof(BROWSEINFO)},

    {_T("SHFILEINFOA*"),PARAM_PSHFILEINFOA,0,sizeof(SHFILEINFOA)},
    {_T("PSHFILEINFOA"),PARAM_PSHFILEINFOA,0,sizeof(SHFILEINFOA)},

    {_T("SHFILEINFOW*"),PARAM_PSHFILEINFOW,0,sizeof(SHFILEINFOW)},
    {_T("PSHFILEINFOW"),PARAM_PSHFILEINFOW,0,sizeof(SHFILEINFOW)},

    {_T("NOTIFYICONDATA*"),PARAM_PNOTIFYICONDATAA,0,6*4+64},//,sizeof(NOTIFYICONDATAA)},
    {_T("PNOTIFYICONDATA"),PARAM_PNOTIFYICONDATAA,0,6*4+64},//sizeof(NOTIFYICONDATAA)},
    {_T("NOTIFYICONDATAA*"),PARAM_PNOTIFYICONDATAA,0,6*4+64},//sizeof(NOTIFYICONDATAA)},
    {_T("PNOTIFYICONDATAA"),PARAM_PNOTIFYICONDATAA,0,6*4+64},//sizeof(NOTIFYICONDATAA)},

    {_T("NOTIFYICONDATAW*"),PARAM_PNOTIFYICONDATAW,0,6*4+64*2},//sizeof(NOTIFYICONDATAW)},
    {_T("PNOTIFYICONDATAW"),PARAM_PNOTIFYICONDATAW,0,6*4+64*2},//sizeof(NOTIFYICONDATAW)},


    {_T("fd_set*"), PARAM_PFD_SET,0,sizeof(fd_set)},

    {_T("WSABUF*"), PARAM_PWSABUF,0,sizeof(WSABUF)},
    {_T("PWSABUF"), PARAM_PWSABUF,0,sizeof(WSABUF)},
    {_T("LPWSABUF"), PARAM_PWSABUF,0,sizeof(WSABUF)},

    {_T("ADDRINFO*"), PARAM_PADDRINFO,0,sizeof(ADDRINFO)},
    {_T("PADDRINFO"), PARAM_PADDRINFO,0,sizeof(ADDRINFO)},
    {_T("LPADDRINFO"), PARAM_PADDRINFO,0,sizeof(ADDRINFO)},

    {_T("WSADATA*"), PARAM_PWSADATA,0,sizeof(WSADATA)},
    {_T("PWSADATA"), PARAM_PWSADATA,0,sizeof(WSADATA)},
    {_T("LPWSADATA"), PARAM_PWSADATA,0,sizeof(WSADATA)},

    {_T("LPWSAPROTOCOL_INFOA"), PARAM_PWSAPROTOCOL_INFOA,0,sizeof(WSAPROTOCOL_INFOA)},
    {_T("LPWSAPROTOCOL_INFOW"), PARAM_PWSAPROTOCOL_INFOW,0,sizeof(WSAPROTOCOL_INFOW)},

    {_T("OVERLAPPED*"), PARAM_POVERLAPPED,0,sizeof(OVERLAPPED)},
    {_T("POVERLAPPED"), PARAM_POVERLAPPED,0,sizeof(OVERLAPPED)},
    {_T("LPOVERLAPPED"), PARAM_POVERLAPPED,0,sizeof(OVERLAPPED)},
    {_T("WSAOVERLAPPED*"), PARAM_POVERLAPPED,0,sizeof(OVERLAPPED)},
    {_T("PWSAOVERLAPPED"), PARAM_POVERLAPPED,0,sizeof(OVERLAPPED)},
    {_T("LPWSAOVERLAPPED"), PARAM_POVERLAPPED,0,sizeof(OVERLAPPED)},

    {_T("TRACEHANDLE"), PARAM_TRACEHANDLE,sizeof(ULONG64),0},
    {_T("TRACEHANDLE*"), PARAM_PTRACEHANDLE,0,sizeof(ULONG64)},
    {_T("PTRACEHANDLE"), PARAM_PTRACEHANDLE,0,sizeof(ULONG64)},

    {_T("DCB*"), PARAM_PDCB,0,sizeof(DCB)},
    {_T("PDCB"), PARAM_PDCB,0,sizeof(DCB)},
    {_T("LPDCB"), PARAM_PDCB,0,sizeof(DCB)},
    {_T("COMMTIMEOUTS*"), PARAM_PCOMMTIMEOUTS,0,sizeof(COMMTIMEOUTS)},
    {_T("PCOMMTIMEOUTS"), PARAM_PCOMMTIMEOUTS,0,sizeof(COMMTIMEOUTS)},
    {_T("LPCOMMTIMEOUTS"), PARAM_PCOMMTIMEOUTS,0,sizeof(COMMTIMEOUTS)},
    {_T("COMMCONFIG*"), PARAM_PCOMMCONFIG,0,sizeof(COMMCONFIG)},
    {_T("PCOMMCONFIG"), PARAM_PCOMMCONFIG,0,sizeof(COMMCONFIG)},
    {_T("LPCOMMCONFIG"), PARAM_PCOMMCONFIG,0,sizeof(COMMCONFIG)},

    {_T("MUTLI_QI"), PARAM_MUTLI_QI,sizeof(MULTI_QI),0},
    {_T("MUTLI_QI*"), PARAM_PMUTLI_QI,0,sizeof(MULTI_QI)},
    {_T("LPMUTLI_QI"), PARAM_PMUTLI_QI,0,sizeof(MULTI_QI)},

    {_T("VARIANT"), PARAM_VARIANT,sizeof(VARIANT),0},
    {_T("VARIANTARG"), PARAM_VARIANT,sizeof(VARIANT),0},
    {_T("VARIANT*"), PARAM_PVARIANT,0,sizeof(VARIANT)},
    {_T("LPVARIANT"), PARAM_PVARIANT,0,sizeof(VARIANT)},
    {_T("VARIANTARG*"), PARAM_PVARIANT,0,sizeof(VARIANT)},
    {_T("LPVARIANTARG"), PARAM_PVARIANT,0,sizeof(VARIANT)},
    {_T("DECIMAL"), PARAM_DECIMAL,sizeof(DECIMAL),0},
    {_T("DECIMAL*"), PARAM_PDECIMAL,0,sizeof(DECIMAL)},
    {_T("LPDECIMAL"), PARAM_PDECIMAL,0,sizeof(DECIMAL)},
    {_T("SAFEARRAY"), PARAM_SAFEARRAY,sizeof(SAFEARRAY),0},
    {_T("SAFEARRAY*"), PARAM_PSAFEARRAY,0,sizeof(SAFEARRAY)},
    {_T("LPSAFEARRAY"), PARAM_PSAFEARRAY,0,sizeof(SAFEARRAY)},
    {_T("SAFEARRAYBOUND"), PARAM_SAFEARRAYBOUND,sizeof(SAFEARRAYBOUND),0},
    {_T("SAFEARRAYBOUND*"), PARAM_PSAFEARRAYBOUND,0,sizeof(SAFEARRAYBOUND)},
    {_T("LPSAFEARRAYBOUND"), PARAM_PSAFEARRAYBOUND,0,sizeof(SAFEARRAYBOUND)},
    {_T("DISPID"), PARAM_LONG,sizeof(LONG),0},
    {_T("DISPID*"), PARAM_PLONG,0,sizeof(LONG)},
    {_T("EXCEPINFO"), PARAM_EXCEPINFO,sizeof(EXCEPINFO),0},
    {_T("EXCEPINFO*"), PARAM_PEXCEPINFO,0,sizeof(EXCEPINFO)},
    {_T("LPEXCEPINFO"), PARAM_PEXCEPINFO,0,sizeof(EXCEPINFO)},

    {_T("DISPPARAMS"), PARAM_DISPPARAMS,sizeof(DISPPARAMS),0},
    {_T("DISPPARAMS*"), PARAM_PDISPPARAMS,0,sizeof(DISPPARAMS)},
    {_T("PDISPPARAMS"), PARAM_PDISPPARAMS,0,sizeof(DISPPARAMS)},
    {_T("LPDISPPARAMS"), PARAM_PDISPPARAMS,0,sizeof(DISPPARAMS)},

    {_T("NET_STRING"), PARAM_NET_STRING,0,0},
    {_T("VOID"),  PARAM_VOID,0,sizeof(PBYTE)},

    {_T("LOGFONTA"), PARAM_LOGFONTA,sizeof(LOGFONTA),0},
    {_T("LOGFONTA*"), PARAM_PLOGFONTA,0,sizeof(LOGFONTA)},
    {_T("PLOGFONTA"), PARAM_PLOGFONTA,0,sizeof(LOGFONTA)},
    {_T("LPLOGFONTA"), PARAM_PLOGFONTA,0,sizeof(LOGFONTA)},

    {_T("LOGFONTW"), PARAM_LOGFONTW,sizeof(LOGFONTW),0},
    {_T("LOGFONTW*"), PARAM_PLOGFONTW,0,sizeof(LOGFONTW)},
    {_T("PLOGFONTW"), PARAM_PLOGFONTW,0,sizeof(LOGFONTW)},
    {_T("LPLOGFONTW"), PARAM_PLOGFONTW,0,sizeof(LOGFONTW)},

    {_T("UNKNOWN"),PARAM_UNKNOWN,sizeof(PBYTE),0}// must be let as last param for loop seeking
};

//-----------------------------------------------------------------------------
// Name: IsParamPassedByRegisterWithFastcall
// Object: in case of fastcall calling convention, allow to know if parameter
//          is passed through register (ecx and edx in 32 bit)
// Parameters :
//     in : DWORD ParamType : parameter type
//     out : 
//     return : TRUE if param can be passed through registers
//-----------------------------------------------------------------------------
BOOL CSupportedParameters::IsParamPassedByRegisterWithFastcall(DWORD ParamType)
{
    // param can be passed throw registers if not float or its size is less than register size

    // check floating params
    if ((ParamType==PARAM_FLOAT)
#ifdef _WIN64
        ||(ParamType==PARAM_DOUBLE)
#endif
        )
        return FALSE;

    // check param size
    return (CSupportedParameters::GetParamStackSize(ParamType)<=sizeof(PBYTE));
}

//-----------------------------------------------------------------------------
// Name: GetParamType
// Object: get param type from SUPPORTED_PARAMETERS_STRUCT array (SupportedParametersArray) for specified param name
// Parameters :
//     in : TCHAR* pszParameter : parameter name
//     out : 
//     return : param type (param UNKNOWN type if not found)
//-----------------------------------------------------------------------------
DWORD CSupportedParameters::GetParamType(TCHAR* pszParameter)
{
    DWORD Index;

    for (Index = 0; SupportedParametersArray[Index].ParamType != PARAM_UNKNOWN; Index++)
    {
        if (_tcsicmp(pszParameter, SupportedParametersArray[Index].ParamName) == 0)
            return SupportedParametersArray[Index].ParamType;
    }
    // if not found return UNKNOWN info
    return SupportedParametersArray[Index].ParamType;
}

//-----------------------------------------------------------------------------
// Name: GetParamName
// Object: get param name from SUPPORTED_PARAMETERS_STRUCT array (SupportedParametersArray) for specified param type
// Parameters :
//     in : DWORD ParamType : parameter type
//     out : 
//     return : param name (param UNKNOWN name if not found)
//-----------------------------------------------------------------------------
TCHAR* CSupportedParameters::GetParamName(DWORD ParamType)
{
    DWORD Index;

    for (Index = 0; SupportedParametersArray[Index].ParamType != PARAM_UNKNOWN; Index++)
    {
        if (SupportedParametersArray[Index].ParamType == ParamType)
            return SupportedParametersArray[Index].ParamName;
    }
    // if not found return UNKNOWN info
    return SupportedParametersArray[Index].ParamName;
}

//-----------------------------------------------------------------------------
// Name: GetParamPointedSize
// Object: get size in bytes of pointer parameters from SUPPORTED_PARAMETERS_STRUCT array (SupportedParametersArray) for specified param type
// Parameters :
//     in : DWORD ParamType : parameter type
//     out : 
//     return :  - 0 if a) not a pointer parameter
//                      b) string parameter --> size is automatically found
//               - else return required size in Byte
//-----------------------------------------------------------------------------
DWORD CSupportedParameters::GetParamPointedSize(DWORD ParamType)
{
    DWORD Index;

    for (Index = 0; SupportedParametersArray[Index].ParamType != PARAM_UNKNOWN; Index++)
    {
        if (SupportedParametersArray[Index].ParamType == ParamType)
            return SupportedParametersArray[Index].PointedDataSize;
    }

    // if not found
    return 0;
}

//-----------------------------------------------------------------------------
// Name: GetParamStackSize
// Object: get size in bytes of parameters from SUPPORTED_PARAMETERS_STRUCT array (SupportedParametersArray) for specified param type
// Parameters :
//     in : DWORD ParamType : parameter type
//     out : 
//     return :  required size in Byte
//-----------------------------------------------------------------------------
DWORD CSupportedParameters::GetParamStackSize(DWORD ParamType)
{
    DWORD Index;

    for (Index = 0; SupportedParametersArray[Index].ParamType != PARAM_UNKNOWN; Index++)
    {
        if (SupportedParametersArray[Index].ParamType == ParamType)
        {
            SupportedParametersArray[Index].DataSize = StackSize(SupportedParametersArray[Index].DataSize);
        }
    }
    // return the PARAM_UNKNOWN size
    return StackSize(SupportedParametersArray[Index].DataSize);
}

//-----------------------------------------------------------------------------
// Name: GetParamRealSize
// Object: get size in bytes of parameters from SUPPORTED_PARAMETERS_STRUCT array (SupportedParametersArray) for specified param type
// Parameters :
//     in : DWORD ParamType : parameter type
//     out : 
//     return :  required size in Byte
//-----------------------------------------------------------------------------
DWORD CSupportedParameters::GetParamRealSize(DWORD ParamType)
{
    DWORD Index;

    for (Index = 0; SupportedParametersArray[Index].ParamType != PARAM_UNKNOWN; Index++)
    {
        if (SupportedParametersArray[Index].ParamType == ParamType)
            return SupportedParametersArray[Index].DataSize;
    }
    // return the PARAM_UNKNOWN size
    return SupportedParametersArray[Index].DataSize;
}

SUPPORTED_PARAMETERS_STRUCT* CSupportedParameters::GetParamInfos(DWORD ParamType)
{
    DWORD Index;
    for (Index = 0; SupportedParametersArray[Index].ParamType != PARAM_UNKNOWN; Index++)
    {
        if (SupportedParametersArray[Index].ParamType == ParamType)
            return &SupportedParametersArray[Index];
    }
    // return the PARAM_UNKNOWN infos
    return &SupportedParametersArray[Index];
}

SUPPORTED_PARAMETERS_STRUCT* CSupportedParameters::GetParamInfos(TCHAR* pszParameter)
{
    DWORD Index;

    for (Index = 0; SupportedParametersArray[Index].ParamType != PARAM_UNKNOWN; Index++)
    {
        if (_tcsicmp(pszParameter, SupportedParametersArray[Index].ParamName) == 0)
            return &SupportedParametersArray[Index];
    }
    // return the PARAM_UNKNOWN infos
    return &SupportedParametersArray[Index];
}

//-----------------------------------------------------------------------------
// Name: SecureStrlen
// Object: assume to not throw error as strlen can do for any string pointer (valid or not)
// Parameters :
//      in: char* pc : src string
//      out : 
// Return : -1 on error (bad string pointer) else size of string pointed by pc
//-----------------------------------------------------------------------------
int CSupportedParameters::SecureStrlen(char* pc)
{
    char* p=pc;
    for(;;)
    {
        // check pointer validity
        if (IsBadReadPtr(p,sizeof(char)))
            return -1;
        // check for end char
        if (*p==0)
            // end char found, return sizeof string
            return (int)(p-pc);
        p++;
    }
}
//-----------------------------------------------------------------------------
// Name: SecureWstrlen
// Object: assume to not throw error as strlen can do for any string pointer (valid or not)
// Parameters :
//      in: wchar_t* pc : src string
//      out : 
// Return : -1 on error (bad string pointer) else size of string pointed by pc
//-----------------------------------------------------------------------------
int CSupportedParameters::SecureWstrlen(wchar_t* pc)
{
    wchar_t* p=pc;
    for(;;)
    {
        // check pointer validity
        if (IsBadReadPtr(p,sizeof(wchar_t)))
            return -1;
        // check for end char
        if (*p==0)
            // end char found, return sizeof string
            return (int)(p-pc);
        p++;
    }
}

//-----------------------------------------------------------------------------
// Name: GetDispparamsFromStackDefaultParsing
// Object: in case of struct advanced parsing error, only log struct
// Parameters :
//     in : HANDLE LogHeap : currently used heap
//          IN DISPPARAMS* pDispParam : pointer to data
//          IN BOOL IsPointedValue : TRUE if function argument is a pointer to struct
//                                   FALSE if struct is passed on stack
//     inout: IN OUT PARAMETER_LOG_INFOS* pParamLogInfo : filled PARAMETER_LOG_INFOS
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSupportedParameters::GetDispparamsFromStackDefaultParsing(HANDLE LogHeap,IN DISPPARAMS* pDispParam,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo)
{
    // in case of failure
    if (IsPointedValue)
    {
        // return an empty struct (act like bad pointer)
        pParamLogInfo->dwType=PARAM_PDISPPARAMS;
        pParamLogInfo->Value=(PBYTE)pDispParam;
        pParamLogInfo->dwSizeOfData=0;

        if (IsBadReadPtr(pDispParam,sizeof(DISPPARAMS)))
        {
            pParamLogInfo->dwSizeOfPointedValue=0;
            pParamLogInfo->pbValue=0;
            return;
        }

        pParamLogInfo->dwSizeOfPointedValue=sizeof(DISPPARAMS);
    }
    else
    {
        // not a pointer value, copy SAFEARRAY struct
        pParamLogInfo->dwType=PARAM_DISPPARAMS;
        pParamLogInfo->Value=(PBYTE)pDispParam;
        pParamLogInfo->dwSizeOfData=sizeof(DISPPARAMS);
        pParamLogInfo->dwSizeOfPointedValue=0;
    }
    // allocate memory
    pParamLogInfo->pbValue=(PBYTE)HeapAlloc( LogHeap,HEAP_ZERO_MEMORY,sizeof(DISPPARAMS));

    // on memory allocation error
    if (!pParamLogInfo->pbValue)
    {
        // return a bad pointer
        pParamLogInfo->dwSizeOfData=0;
        pParamLogInfo->dwSizeOfPointedValue=0;
        return;
    }
    // else

    // copy DISPPARAMS struct
    memcpy(pParamLogInfo->pbValue,pDispParam,sizeof(DISPPARAMS));
}


//-----------------------------------------------------------------------------
// Name: GetDispparamsFromStack
// Object: do an advanced parsing of DISPPARAMS pointer
//      forge something like 
//      pParamLogInfo->type =PARAM_DISPPARAMS_PARSED
//            pParamLogInfo->pbValue=DISPPARAMS + nb_variant + pParamLogInfoWithData(Variant1) + pParamLogInfoWithData(Variant2)+ ... + pParamLogInfoWithData(VariantN)
//      
//      only one pDispParam is supported, not array to DISPPARAMS
//      only parse pDispParam->rgvarg values 
// Parameters :
//     in : HANDLE LogHeap : currently used heap
//          IN DISPPARAMS* pDispParam : pointer to data
//          DWORD PointedDataSize : size of pointed data in case of array of struct
//          IN BOOL IsPointedValue : TRUE if function argument is a pointer to struct
//                                   FALSE if struct is passed on stack
//     inout: IN OUT PARAMETER_LOG_INFOS* pParamLogInfo : filled PARAMETER_LOG_INFOS
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSupportedParameters::GetDispparamsFromStack(HANDLE LogHeap,IN DISPPARAMS* pDispParam,DWORD PointedDataSize,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo)
{
    UNREFERENCED_PARAMETER(PointedDataSize);
    DWORD RequieredMemorySize;
    PBYTE pBuffer;

    // if bad pointer
    if (IsBadReadPtr(pDispParam,sizeof(DISPPARAMS)))
    {
        // do default parsing
        CSupportedParameters::GetDispparamsFromStackDefaultParsing( LogHeap,pDispParam,IsPointedValue,pParamLogInfo);
        return;
    }
    // if no item or incompatible nbitem/readable memory
    if ((pDispParam->cArgs==0)||(IsBadReadPtr(pDispParam->rgvarg,pDispParam->cArgs*sizeof(VARIANT))))
    {
        // do default parsing
        CSupportedParameters::GetDispparamsFromStackDefaultParsing( LogHeap,pDispParam,IsPointedValue,pParamLogInfo);
        return;
    }
    // in case of advanced parameters parsing error
    if (!CSupportedParameters::GetVariantFromStack( LogHeap,pDispParam->rgvarg,pDispParam->cArgs,NULL,&RequieredMemorySize))
    {
        // do default parsing
        CSupportedParameters::GetDispparamsFromStackDefaultParsing( LogHeap,pDispParam,IsPointedValue,pParamLogInfo);
        return;
    }

    // forge pParamLogInfo->pbValue=DISPPARAMS + nb_variant + pParamLogInfoWithData(Variant1) + pParamLogInfoWithData(Variant2)+ ... + pParamLogInfoWithData(VariantN)
    RequieredMemorySize+=sizeof(DISPPARAMS);
    RequieredMemorySize+=sizeof(DWORD);
    if (IsPointedValue)
    {
        pParamLogInfo->dwType=PARAM_DISPPARAMS_PARSED;
        pParamLogInfo->Value=(PBYTE)pDispParam;
        pParamLogInfo->dwSizeOfData=0;
        pParamLogInfo->dwSizeOfPointedValue=RequieredMemorySize;
    }
    else
    {
        pParamLogInfo->dwType=PARAM_DISPPARAMS_PARSED;
        pParamLogInfo->Value=0;
        pParamLogInfo->dwSizeOfData=RequieredMemorySize;
        pParamLogInfo->dwSizeOfPointedValue=0;
    }

    // allocate memory
    pParamLogInfo->pbValue=(PBYTE)HeapAlloc( LogHeap,HEAP_ZERO_MEMORY,RequieredMemorySize);

    // on memory allocation error
    if (!pParamLogInfo->pbValue)
    {
        CSupportedParameters::GetDispparamsFromStackDefaultParsing( LogHeap,pDispParam,IsPointedValue,pParamLogInfo);
        return;
    }
    // else

    pBuffer=pParamLogInfo->pbValue;

    // copy DISPPARAMS struct
    memcpy(pBuffer,pDispParam,sizeof(DISPPARAMS));
    pBuffer+=sizeof(DISPPARAMS);

    // copy number of variant to act like VARIANT_PARSED
    memcpy(pBuffer,&pDispParam->cArgs,sizeof(DWORD));
    pBuffer+=sizeof(DWORD);

    // add VARIANT parsed data after DISPPARAMS struct
    if (!CSupportedParameters::GetVariantFromStack( LogHeap,pDispParam->rgvarg,pDispParam->cArgs,pBuffer,&RequieredMemorySize))
    {
        HeapFree(LogHeap,0,pParamLogInfo->pbValue);
        CSupportedParameters::GetDispparamsFromStackDefaultParsing( LogHeap,pDispParam,IsPointedValue,pParamLogInfo);
        return;
    }
}

//-----------------------------------------------------------------------------
// Name: GetExcepinfoFromStackDefaultParsing
// Object: in case of struct advanced parsing error, only log struct
// Parameters :
//     in : HANDLE LogHeap : currently used heap
//          IN EXCEPINFO* pExcepinfo : pointer to data
//          IN BOOL IsPointedValue : TRUE if function argument is a pointer to struct
//                                   FALSE if struct is passed on stack
//     inout: IN OUT PARAMETER_LOG_INFOS* pParamLogInfo : filled PARAMETER_LOG_INFOS
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSupportedParameters::GetExcepinfoFromStackDefaultParsing(HANDLE LogHeap,IN EXCEPINFO* pExcepinfo,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo,DWORD* pRequieredMemorySize)
{
    *pRequieredMemorySize=0;

    // in case of failure
    if (IsPointedValue)
    {
        // return an empty struct (act like bad pointer)
        pParamLogInfo->dwType=PARAM_PEXCEPINFO;
        pParamLogInfo->Value=(PBYTE)pExcepinfo;
        pParamLogInfo->dwSizeOfData=0;

        if (IsBadReadPtr(pExcepinfo,sizeof(EXCEPINFO)))
        {
            pParamLogInfo->dwSizeOfPointedValue=0;
            pParamLogInfo->pbValue=0;
            return;
        }
        
        pParamLogInfo->dwSizeOfPointedValue=sizeof(EXCEPINFO);
    }
    else
    {
        // not a pointer value, copy SAFEARRAY struct
        pParamLogInfo->dwType=PARAM_EXCEPINFO;
        pParamLogInfo->Value=(PBYTE)pExcepinfo;
        pParamLogInfo->dwSizeOfData=sizeof(EXCEPINFO);
        pParamLogInfo->dwSizeOfPointedValue=0;

    }

    // allocate memory
    pParamLogInfo->pbValue=(PBYTE)HeapAlloc( LogHeap,HEAP_ZERO_MEMORY,sizeof(EXCEPINFO));

    // on memory allocation error
    if (!pParamLogInfo->pbValue)
    {
        // return a bad pointer
        pParamLogInfo->dwSizeOfData=0;
        pParamLogInfo->dwSizeOfPointedValue=0;
        return;
    }
    // else

    // copy EXCEPINFO struct
    memcpy(pParamLogInfo->pbValue,pExcepinfo,sizeof(EXCEPINFO));

    *pRequieredMemorySize=sizeof(EXCEPINFO);

}
//-----------------------------------------------------------------------------
// Name: GetExcepinfoFromStack
// Object: do an advanced parsing of EXCEPINFO pointer
//          pExcepinfo must be a valid pointer (caller is responsible to check pointer with IsBadReadPtr)
//      forge  
//     NbEXCEPINFO
// for each EXCEPINFO
//     EXCEPINFO struct
//     DescriptionLen
//     Description
//     HelpFileLen
//     HelpFile
//     SourceLen
//     Source
// Parameters :
//     in : HANDLE LogHeap : currently used heap
//          IN EXCEPINFO* pExcepinfo : pointer to data
//          DWORD PointedDataSize : size of pointed data in case of array of struct
//          IN BOOL IsPointedValue : TRUE if function argument is a pointer to struct
//                                   FALSE if struct is passed on stack
//     inout: IN OUT PARAMETER_LOG_INFOS* pParamLogInfo : filled PARAMETER_LOG_INFOS
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSupportedParameters::GetExcepinfoFromStack(HANDLE LogHeap,IN EXCEPINFO* pExcepinfo,DWORD PointedDataSize,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo)
{
    DWORD RequieredMemorySize=0;
    CSupportedParameters::GetExcepinfoFromStackEx( LogHeap,pExcepinfo,PointedDataSize,IsPointedValue,pParamLogInfo,&RequieredMemorySize);
}
//-----------------------------------------------------------------------------
// Name: GetExcepinfoFromStackEx
// Object: do an advanced parsing of EXCEPINFO pointer
//          pExcepinfo must be a valid pointer (caller is responsible to check pointer with IsBadReadPtr)
//      forge
//     NbEXCEPINFO
// for each EXCEPINFO
//     EXCEPINFO struct
//     DescriptionLen
//     Description
//     HelpFileLen
//     HelpFile
//     SourceLen
//     Source
// Parameters :
//     in : HANDLE LogHeap : currently used heap
//          IN EXCEPINFO* pExcepinfo : pointer to data
//          DWORD PointedDataSize : size of pointed data in case of array of struct
//          IN BOOL IsPointedValue : TRUE if function argument is a pointer to struct
//                                   FALSE if struct is passed on stack
//     inout: IN OUT PARAMETER_LOG_INFOS* pParamLogInfo : filled PARAMETER_LOG_INFOS
//     out : DWORD* pRequieredMemorySize : memory size need to store advanced parsing
//     return : 
//-----------------------------------------------------------------------------
void CSupportedParameters::GetExcepinfoFromStackEx(HANDLE LogHeap,IN EXCEPINFO* pExcepinfo,DWORD PointedDataSize,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo,DWORD* pRequieredMemorySize)
{

    PBYTE pBuffer;
    DWORD RequieredMemorySize=0;
    DWORD NbItems=1;
    *pRequieredMemorySize=0;

    // get needed memory size
    if (!CSupportedParameters::GetExcepinfoFromStack( LogHeap,pExcepinfo,PointedDataSize,NULL,&RequieredMemorySize,&NbItems))
    {
        // do default parsing
        CSupportedParameters::GetExcepinfoFromStackDefaultParsing( LogHeap,pExcepinfo,IsPointedValue,pParamLogInfo,pRequieredMemorySize);
        return;
    }

    RequieredMemorySize+=sizeof(DWORD);
    *pRequieredMemorySize=RequieredMemorySize;

    // fill infos
    if (IsPointedValue)
    {
        pParamLogInfo->dwType=PARAM_EXCEPINFO_PARSED;
        pParamLogInfo->Value=(PBYTE)pExcepinfo;
        pParamLogInfo->dwSizeOfData=0;
        pParamLogInfo->dwSizeOfPointedValue=RequieredMemorySize;
    }
    else
    {
        pParamLogInfo->dwType=PARAM_EXCEPINFO_PARSED;
        pParamLogInfo->Value=0;
        pParamLogInfo->dwSizeOfData=RequieredMemorySize;
        pParamLogInfo->dwSizeOfPointedValue=0;
    }

    // allocate memory
    pBuffer=(PBYTE)HeapAlloc( LogHeap,HEAP_ZERO_MEMORY,RequieredMemorySize);

    if (pBuffer)
    {
        pParamLogInfo->pbValue=pBuffer;
        // copy number of EXCEPINFO items
        memcpy(pBuffer,&NbItems,sizeof(DWORD));

        // call again func
        if (!CSupportedParameters::GetExcepinfoFromStack( LogHeap,pExcepinfo,PointedDataSize,pBuffer+sizeof(DWORD),&RequieredMemorySize,&NbItems))
        {
            HeapFree(LogHeap,0,pBuffer);
            CSupportedParameters::GetExcepinfoFromStackDefaultParsing( LogHeap,pExcepinfo,IsPointedValue,pParamLogInfo,pRequieredMemorySize);
            *pRequieredMemorySize=sizeof(EXCEPINFO);
            return;
        }
        // warning at this point *pRequieredMemorySize=RequieredMemorySize+sizeof(DWORD)
        // that's why we use RequieredMemorySize local var
    }
    else
    {
        // do default parsing
        CSupportedParameters::GetExcepinfoFromStackDefaultParsing( LogHeap,pExcepinfo,IsPointedValue,pParamLogInfo,pRequieredMemorySize);
        return;
    }
}
//-----------------------------------------------------------------------------
// Name: GetExcepinfoFromStack
// Object: do an advanced parsing of EXCEPINFO pointer
//          pExcepinfo must be a valid pointer (caller is responsible to check pointer with IsBadReadPtr)
//      forge  
//     NbEXCEPINFO
// for each EXCEPINFO
//     EXCEPINFO struct
//     DescriptionLen
//     Description
//     HelpFileLen
//     HelpFile
//     SourceLen
//     Source
// Parameters :
//     in : HANDLE LogHeap : currently used heap
//          IN EXCEPINFO* pExcepinfo : pointer to data
//          DWORD PointedDataSize : size of pointed data in case of array of struct
//          IN BOOL IsPointedValue : TRUE if function argument is a pointer to struct
//                                   FALSE if struct is passed on stack
//     inout: IN OUT PBYTE pBuffer : buffer that will contain data (NULL to get only size)
//     out : OUT DWORD* pRequieredMemorySize : needed size
//           OUT DWORD* pNbItems : nb EXCEPINFO items
//     return : 
//-----------------------------------------------------------------------------
BOOL CSupportedParameters::GetExcepinfoFromStack(HANDLE LogHeap,IN EXCEPINFO* pExcepinfo,DWORD PointedDataSize,IN OUT PBYTE pBuffer,OUT DWORD* pRequieredMemorySize,OUT DWORD* pNbItems)
{
    UNREFERENCED_PARAMETER(LogHeap);
    *pRequieredMemorySize=0;
    *pNbItems=0;

    EXCEPINFO* pExcepinfo0=pExcepinfo;
    DWORD RequieredMemorySize=0;
    int DescriptionLen;
    int HelpFileLen;
    int SourceLen;
    DWORD RequieredMemorySizeOfCurrentExcepinfo=0;

    // for each EXCEPINFO struct
    // (until good memory pointer and according to pointed size)
    while ((PBYTE)pExcepinfo+sizeof(EXCEPINFO)<=(PBYTE)pExcepinfo0+PointedDataSize)
    {
        // if bad pointer
        if (IsBadReadPtr(pExcepinfo,sizeof(EXCEPINFO)))
            return FALSE;

        DescriptionLen=CSupportedParameters::SecureWstrlen(pExcepinfo->bstrDescription);
        HelpFileLen=CSupportedParameters::SecureWstrlen(pExcepinfo->bstrHelpFile);
        SourceLen=CSupportedParameters::SecureWstrlen(pExcepinfo->bstrSource);
        
        RequieredMemorySizeOfCurrentExcepinfo=sizeof(EXCEPINFO)+3*sizeof(int);
        if (DescriptionLen>0)
        {
            DescriptionLen=(DescriptionLen+1)*sizeof(WCHAR);
            RequieredMemorySizeOfCurrentExcepinfo+=DescriptionLen;
        }
        if (HelpFileLen>0)
        {
            HelpFileLen=(HelpFileLen+1)*sizeof(WCHAR);
            RequieredMemorySizeOfCurrentExcepinfo+=HelpFileLen;
        }
        if (SourceLen>0)
        {
            SourceLen=(SourceLen+1)*sizeof(WCHAR);
            RequieredMemorySizeOfCurrentExcepinfo+=SourceLen;
        }

        // compute necessary size
        RequieredMemorySize+=RequieredMemorySizeOfCurrentExcepinfo;

        // if buffer is defined, copy data into it
        if (pBuffer)
        {
            // pbuffer is like 
            // EXCEPINFO
            // DescriptionLen
            // Description
            // HelpFileLen
            // HelpFile
            // SourceLen
            // Source

            // copy struct
            memcpy(pBuffer,pExcepinfo,sizeof(EXCEPINFO));
            pBuffer+=sizeof(EXCEPINFO);

            // copy strings
            memcpy(pBuffer,&DescriptionLen,sizeof(int));
            pBuffer+=sizeof(int);
            if (DescriptionLen>0)
            {
                memcpy(pBuffer,pExcepinfo->bstrDescription,DescriptionLen);
                pBuffer+=DescriptionLen;
            }
            memcpy(pBuffer,&HelpFileLen,sizeof(int));
            pBuffer+=sizeof(int);
            if (HelpFileLen>0)
            {
                memcpy(pBuffer,pExcepinfo->bstrHelpFile,HelpFileLen);
                pBuffer+=HelpFileLen;
            }
            memcpy(pBuffer,&SourceLen,sizeof(int));
            pBuffer+=sizeof(int);
            if (SourceLen>0)
            {
                memcpy(pBuffer,pExcepinfo->bstrSource,SourceLen);
                pBuffer+=SourceLen;
            }
        }
        // get next EXCEPINFO pointer
        pExcepinfo=(EXCEPINFO*)(((PBYTE)pExcepinfo)+sizeof(EXCEPINFO));
        // increase number of items
        (*pNbItems)++;
    }
    *pRequieredMemorySize=RequieredMemorySize;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetSafeArrayFromStackDefaultParsing
// Object: in case of struct advanced parsing error, only log struct
// Parameters :
//     in : HANDLE LogHeap : currently used heap
//          IN SAFEARRAY* pSafeArray : pointer to data
//          IN BOOL IsPointedValue : TRUE if function argument is a pointer to struct
//                                   FALSE if struct is passed on stack
//     inout: IN OUT PARAMETER_LOG_INFOS* pParamLogInfo : filled PARAMETER_LOG_INFOS
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSupportedParameters::GetSafeArrayFromStackDefaultParsing(HANDLE LogHeap,IN SAFEARRAY* pSafeArray,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo,DWORD* pRequieredMemorySize)
{
    *pRequieredMemorySize=0;

    // in case of failure
    if (IsPointedValue)
    {
        // return an empty struct (act like bad pointer)
        pParamLogInfo->dwType=PARAM_PSAFEARRAY;
        pParamLogInfo->Value=(PBYTE)pSafeArray;
        pParamLogInfo->dwSizeOfData=0;


        if (IsBadReadPtr(pSafeArray,sizeof(SAFEARRAY)))
        {
            pParamLogInfo->dwSizeOfPointedValue=0;
            pParamLogInfo->pbValue=0;
            return;
        }
        pParamLogInfo->dwSizeOfPointedValue=sizeof(SAFEARRAY);
    }
    else
    {
        // not a pointer value, copy SAFEARRAY struct
        pParamLogInfo->dwType=PARAM_SAFEARRAY;
        pParamLogInfo->Value=(PBYTE)pSafeArray;
        pParamLogInfo->dwSizeOfData=sizeof(SAFEARRAY);
        pParamLogInfo->dwSizeOfPointedValue=0;

    }

    // allocate memory
    pParamLogInfo->pbValue=(PBYTE)HeapAlloc(LogHeap,HEAP_ZERO_MEMORY,sizeof(SAFEARRAY));

    // on memory allocation error
    if (!pParamLogInfo->pbValue)
    {
        // return a bad pointer
        pParamLogInfo->dwSizeOfData=0;
        pParamLogInfo->dwSizeOfPointedValue=0;
        return;
    }
    // else

    // copy SAFEARRAY struct
    memcpy(pParamLogInfo->pbValue,pSafeArray,sizeof(SAFEARRAY));

    *pRequieredMemorySize=sizeof(SAFEARRAY);
}


//-----------------------------------------------------------------------------
// Name: GetSafeArrayFromStack
// Object: do an advanced parsing of SAFEARRAY pointer
//          pSafeArray must be a valid pointer (caller is responsible to check pointer with IsBadReadPtr)
// forge
// pParamLogInfo->type =SAFEARRAY_PARSED
//       pParamLogInfo->pbValue=nb_SAFEARRAY + pParamLogInfoWithData(SAFEARRAY1) + pParamLogInfoWithData(SAFEARRAY2)+ ... + pParamLogInfoWithData(SAFEARRAYN)
// or 
// pParamLogInfo->type =SAFEARRAY
//       pParamLogInfo->pbValue=SAFEARRAY struct
// Parameters :
//     in : HANDLE LogHeap : currently used heap
//          IN SAFEARRAY* pSafeArray : pointer to data
//          DWORD PointedDataSize : size of pointed data in case of array of struct
//          IN BOOL IsPointedValue : TRUE if function argument is a pointer to struct
//                                   FALSE if struct is passed on stack
//     inout: IN OUT PARAMETER_LOG_INFOS* pParamLogInfo : filled PARAMETER_LOG_INFOS
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSupportedParameters::GetSafeArrayFromStack(HANDLE LogHeap,IN SAFEARRAY* pSafeArray,DWORD PointedDataSize,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo)
{
    DWORD RequieredMemorySize=0;
    CSupportedParameters::GetSafeArrayFromStackEx( LogHeap,pSafeArray,PointedDataSize,IsPointedValue,pParamLogInfo,&RequieredMemorySize);
}

//-----------------------------------------------------------------------------
// Name: GetSafeArrayFromStackEx
// Object: do an advanced parsing of SAFEARRAY pointer
//          pSafeArray must be a valid pointer (caller is responsible to check pointer with IsBadReadPtr)
// forge
// pParamLogInfo->type =SAFEARRAY_PARSED
//       pParamLogInfo->pbValue=nb_SAFEARRAY + pParamLogInfoWithData(SAFEARRAY1) + pParamLogInfoWithData(SAFEARRAY2)+ ... + pParamLogInfoWithData(SAFEARRAYN)
// or 
// pParamLogInfo->type =SAFEARRAY
//       pParamLogInfo->pbValue=SAFEARRAY struct
// Parameters :
//     in : HANDLE LogHeap : currently used heap
//          IN SAFEARRAY* pSafeArray : pointer to data
//          DWORD PointedDataSize : size of pointed data in case of array of struct
//          IN BOOL IsPointedValue : TRUE if function argument is a pointer to struct
//                                   FALSE if struct is passed on stack
//     inout: IN OUT PARAMETER_LOG_INFOS* pParamLogInfo : filled PARAMETER_LOG_INFOS
//     out : DWORD* pRequieredMemorySize : memory size need to store advanced parsing
//     return : 
//-----------------------------------------------------------------------------
void CSupportedParameters::GetSafeArrayFromStackEx(HANDLE LogHeap,IN SAFEARRAY* pSafeArray,DWORD PointedDataSize,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo,DWORD* pRequieredMemorySize)
{
    
    PBYTE pBuffer;
    DWORD RequieredMemorySize=0;
    DWORD NbItems=1;
    *pRequieredMemorySize=0;

    // get needed memory size
    if (!CSupportedParameters::GetSafeArrayFromStack( LogHeap,pSafeArray,PointedDataSize,NULL,&RequieredMemorySize,&NbItems))
    {
        // do default parsing
        CSupportedParameters::GetSafeArrayFromStackDefaultParsing( LogHeap,pSafeArray,IsPointedValue,pParamLogInfo,pRequieredMemorySize);
        return;
    }

    RequieredMemorySize+=sizeof(DWORD);
    *pRequieredMemorySize=RequieredMemorySize;

    // fill infos
    if (IsPointedValue)
    {
        pParamLogInfo->dwType=PARAM_SAFEARRAY_PARSED;
        pParamLogInfo->Value=(PBYTE)pSafeArray;
        pParamLogInfo->dwSizeOfData=0;
        pParamLogInfo->dwSizeOfPointedValue=RequieredMemorySize;
    }
    else
    {
        pParamLogInfo->dwType=PARAM_SAFEARRAY_PARSED;
        pParamLogInfo->Value=0;
        pParamLogInfo->dwSizeOfData=RequieredMemorySize;
        pParamLogInfo->dwSizeOfPointedValue=0;
    }

    // allocate memory
    pBuffer=(PBYTE)HeapAlloc(LogHeap,HEAP_ZERO_MEMORY,RequieredMemorySize);

    if (pBuffer)
    {
        pParamLogInfo->pbValue=pBuffer;
        // copy number of SAFEARRAY items
        memcpy(pBuffer,&NbItems,sizeof(DWORD));
        // call again func
        if (!CSupportedParameters::GetSafeArrayFromStack( LogHeap,pSafeArray,PointedDataSize,pBuffer+sizeof(DWORD),&RequieredMemorySize,&NbItems))
        {
            HeapFree(LogHeap,0,pBuffer);
            CSupportedParameters::GetSafeArrayFromStackDefaultParsing( LogHeap,pSafeArray,IsPointedValue,pParamLogInfo,pRequieredMemorySize);
            *pRequieredMemorySize=sizeof(SAFEARRAY);
            return;
        }
        // warning at this point *pRequieredMemorySize=RequieredMemorySize+sizeof(DWORD)
        // that's why we use RequieredMemorySize local var
    }
    else
    {
        // do default parsing
        CSupportedParameters::GetSafeArrayFromStackDefaultParsing(LogHeap,pSafeArray,IsPointedValue,pParamLogInfo,pRequieredMemorySize);
        return;
    }
}

//-----------------------------------------------------------------------------
// Name: GetSafeArrayFromStack
// Object: do an advanced parsing of SAFEARRAY pointer
//          pSafeArray must be a valid pointer (caller is responsible to check pointer with IsBadReadPtr)
// forge
// pParamLogInfo->type =SAFEARRAY_PARSED
//       pParamLogInfo->pbValue=nb_SAFEARRAY + pParamLogInfoWithData(SAFEARRAY1) + pParamLogInfoWithData(SAFEARRAY2)+ ... + pParamLogInfoWithData(SAFEARRAYN)
// Parameters :
//     in : HANDLE LogHeap : currently used heap
//          IN SAFEARRAY* pSafeArray : pointer to data
//          DWORD PointedDataSize : size of pointed data in case of array of struct
//          IN BOOL IsPointedValue : TRUE if function argument is a pointer to struct
//                                   FALSE if struct is passed on stack
//     inout: IN OUT PBYTE pBuffer : buffer that will contain data (NULL to get only size)
//     out : OUT DWORD* pRequieredMemorySize : needed size
//           OUT DWORD* pNbItems : nb SAFEARRAY items
//     return : 
//-----------------------------------------------------------------------------
BOOL CSupportedParameters::GetSafeArrayFromStack(HANDLE LogHeap,IN SAFEARRAY* pSafeArray,DWORD PointedDataSize,IN OUT PBYTE pBuffer,OUT DWORD* pRequieredMemorySize,OUT DWORD* pNbItems)
{
    UNREFERENCED_PARAMETER(LogHeap);
    *pRequieredMemorySize=0;
    *pNbItems=0;

    SAFEARRAY* pSafeArray0=pSafeArray;

    
    PARAMETER_LOG_INFOS* pSafeArrayParamLogInfo;
    PARAMETER_LOG_INFOS SafeArrayParamLogInfo;
    DWORD RequieredMemorySize=0;
    USHORT cntDim;

    DWORD PointedDataSizeOfCurrentArray=0;
    ULONG ArraySize;
    ULONG StructSize;
    
    // for each safearray
    while ((PBYTE)pSafeArray+sizeof(SAFEARRAY)<=(PBYTE)pSafeArray0+PointedDataSize)
    {
        // if bad pointer
        if (IsBadReadPtr(pSafeArray,sizeof(SAFEARRAY)))
            return FALSE;

        // take a pointer on a PARAMETER_LOG_INFOS struct
        if (pBuffer)
            pSafeArrayParamLogInfo=(PARAMETER_LOG_INFOS*)pBuffer;
        else
            pSafeArrayParamLogInfo=&SafeArrayParamLogInfo;

        // zero memory
        memset(pSafeArrayParamLogInfo,0,sizeof(PARAMETER_LOG_INFOS));

        if (pBuffer)// special action after memset(,0,)
            pSafeArrayParamLogInfo->pbValue=pBuffer+sizeof(PARAMETER_LOG_INFOS);

        // increase need memory size
        RequieredMemorySize+=sizeof(PARAMETER_LOG_INFOS);

        // fill type and dwSizeOfPointedValue
        pSafeArrayParamLogInfo->dwType=PARAM_SAFEARRAY_PARSED;
        pSafeArrayParamLogInfo->dwSizeOfPointedValue=0;

        // compute struct size containing all SAFEARRAYBOUND informations
        StructSize=sizeof(SAFEARRAY)+sizeof(SAFEARRAYBOUND)*(pSafeArray->cDims-1);

        if (IsBadReadPtr(pSafeArray,StructSize))
            return FALSE;

        // compute array size (array pointed by pvData)
        ArraySize=0;
        for (cntDim=0;cntDim<pSafeArray->cDims;cntDim++)
        {
            if (ArraySize==0)
                ArraySize=pSafeArray->rgsabound[cntDim].cElements;
            else
                ArraySize*=pSafeArray->rgsabound[cntDim].cElements;
        }
        ArraySize*=pSafeArray->cbElements;

        // compute full size
        PointedDataSizeOfCurrentArray=StructSize+ArraySize;

        if (IsBadReadPtr(pSafeArray->pvData,ArraySize))
            return FALSE;

        // compute necessary size
        RequieredMemorySize+=PointedDataSizeOfCurrentArray;

        // if buffer is defined, copy data into it
        if (pBuffer)
        {
            // store pointed data size
            pSafeArrayParamLogInfo->dwSizeOfData=PointedDataSizeOfCurrentArray;
            // copy struct infos
            memcpy(pBuffer,pSafeArray,StructSize);
            pBuffer+=StructSize;
            // copy array data
            memcpy(pBuffer,pSafeArray->pvData,ArraySize);
            pBuffer+=ArraySize;
        }
        // get next SAFEARRAY pointer
        pSafeArray=(SAFEARRAY*)(((PBYTE)pSafeArray)+StructSize);
        // increase number of items
        (*pNbItems)++;
    }
    *pRequieredMemorySize=RequieredMemorySize;
    return TRUE;
/*
Members
cDims 
Count of dimensions in this array. 

fFeatures 
Flags used by the SafeArray routines.

cbElements 
Size of an element of the array. Does not include size of pointed-to data. 

cLocks 
Number of times the array has been locked without corresponding unlock. 

handle 
Unused, but kept for compatibility. 

pvData; 
Void pointer to the data. 

rgsabound 
One bound for each dimension. 

Remarks
The array rgsabound is stored with the left-most dimension in rgsabound[0] and the right-most dimension in rgsabound[cDims – 1].
If an array was specified in a C-like syntax as a [2][5], it would have two elements in the rgsabound vector.
Element 0 has an lLbound of 0 and a cElements of 2. Element 1 has an lLbound of 0 and a cElements of 5.

*/
}
//-----------------------------------------------------------------------------
// Name: GetVariantFromStackDefaultParsing
// Object: in case of struct advanced parsing error, only log struct
// Parameters :
//     in : HANDLE LogHeap : currently used heap
//          IN VARIANT* pVariant : pointer to data
//          IN BOOL IsPointedValue : TRUE if function argument is a pointer to struct
//                                   FALSE if struct is passed on stack
//     inout: IN OUT PARAMETER_LOG_INFOS* pParamLogInfo : filled PARAMETER_LOG_INFOS
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSupportedParameters::GetVariantFromStackDefaultParsing(HANDLE LogHeap,IN VARIANT* pVariant,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo,DWORD* pRequieredMemorySize)
{
    *pRequieredMemorySize=0;
    // in case of failure
    if (IsPointedValue)
    {
        // return an empty struct (act like bad pointer)
        pParamLogInfo->dwType=PARAM_PVARIANT;
        pParamLogInfo->Value=(PBYTE)pVariant;
        pParamLogInfo->dwSizeOfData=0;

        if (IsBadReadPtr(pVariant,sizeof(VARIANT)))
        {
            pParamLogInfo->dwSizeOfPointedValue=0;
            pParamLogInfo->pbValue=0;
            return;
        }
        pParamLogInfo->dwSizeOfPointedValue=sizeof(VARIANT);
    }
    else
    {
        // not a pointer value, copy variant struct
        pParamLogInfo->dwType=PARAM_VARIANT;
        pParamLogInfo->Value=(PBYTE)pVariant;
        pParamLogInfo->dwSizeOfData=sizeof(VARIANT);
        pParamLogInfo->dwSizeOfPointedValue=0;

    }
    // allocate memory
    pParamLogInfo->pbValue=(PBYTE)HeapAlloc(LogHeap,HEAP_ZERO_MEMORY,sizeof(VARIANT));

    // on memory allocation error
    if (!pParamLogInfo->pbValue)
    {
        // return a bad pointer
        pParamLogInfo->dwSizeOfData=0;
        pParamLogInfo->dwSizeOfPointedValue=0;
        return;
    }
    // else

    // copy VARIANT struct
    memcpy(pParamLogInfo->pbValue,pVariant,sizeof(VARIANT));

    *pRequieredMemorySize=sizeof(VARIANT);
}
//-----------------------------------------------------------------------------
// Name: GetVariantFromStack
// Object: do an advanced parsing of VARIANT pointer
//          pVariant must be a valid pointer (caller is responsible to check pointer with IsBadReadPtr)
// forge
// pParamLogInfo->type =VARIANT_PARSED
//       pParamLogInfo->pbValue=nb_VARIANT + pParamLogInfoWithData(VARIANT1) + pParamLogInfoWithData(VARIANT2)+ ... + pParamLogInfoWithData(VARIANTN)
// or 
// pParamLogInfo->type =VARIANT
//       pParamLogInfo->pbValue=VARIANT struct
// Parameters :
//     in : HANDLE LogHeap : currently used heap
//          IN VARIANT* pVariant : pointer to data
//          DWORD PointedDataSize : size of pointed data in case of array of struct
//          IN BOOL IsPointedValue : TRUE if function argument is a pointer to struct
//                                   FALSE if struct is passed on stack
//     inout: IN OUT PARAMETER_LOG_INFOS* pParamLogInfo : filled PARAMETER_LOG_INFOS
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSupportedParameters::GetVariantFromStack(HANDLE LogHeap,IN VARIANT* pVariant,DWORD PointedDataSize,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo)
{
    DWORD RequieredMemorySize=0;
    CSupportedParameters::GetVariantFromStackEx(LogHeap,pVariant,PointedDataSize,IsPointedValue,pParamLogInfo,&RequieredMemorySize);
}
//-----------------------------------------------------------------------------
// Name: GetVariantFromStackEx
// Object: do an advanced parsing of VARIANT pointer
//          pVariant must be a valid pointer (caller is responsible to check pointer with IsBadReadPtr)
// forge
// pParamLogInfo->type =VARIANT_PARSED
//       pParamLogInfo->pbValue=nb_VARIANT + pParamLogInfoWithData(VARIANT1) + pParamLogInfoWithData(VARIANT2)+ ... + pParamLogInfoWithData(VARIANTN)
// or 
// pParamLogInfo->type =VARIANT
//       pParamLogInfo->pbValue=VARIANT struct
// Parameters :
//     in : HANDLE LogHeap : currently used heap
//          IN VARIANT* pVariant : pointer to data
//          DWORD PointedDataSize : size of pointed data in case of array of struct
//          IN BOOL IsPointedValue : TRUE if function argument is a pointer to struct
//                                   FALSE if struct is passed on stack
//     inout: IN OUT PARAMETER_LOG_INFOS* pParamLogInfo : filled PARAMETER_LOG_INFOS
//     out : DWORD* pRequieredMemorySize : memory size need to store advanced parsing
//     return : 
//-----------------------------------------------------------------------------
void CSupportedParameters::GetVariantFromStackEx(HANDLE LogHeap,IN VARIANT* pVariant,DWORD PointedDataSize,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo,OUT DWORD* pRequieredMemorySize)
{
    PBYTE pBuffer;
    *pRequieredMemorySize=0;
    DWORD RequieredMemorySize=0;
    DWORD NbPointedItems=PointedDataSize/sizeof(VARIANT);

    // get needed memory size
    if (!CSupportedParameters::GetVariantFromStack(LogHeap,pVariant,NbPointedItems,NULL,&RequieredMemorySize))
    {
        CSupportedParameters::GetVariantFromStackDefaultParsing(LogHeap,pVariant,IsPointedValue,pParamLogInfo,pRequieredMemorySize);
        return;
    }

    RequieredMemorySize+=sizeof(DWORD);
    *pRequieredMemorySize=RequieredMemorySize;

    // fill infos
    if (IsPointedValue)
    {
        pParamLogInfo->dwType=PARAM_VARIANT_PARSED;
        pParamLogInfo->Value=(PBYTE)pVariant;
        pParamLogInfo->dwSizeOfData=0;
        pParamLogInfo->dwSizeOfPointedValue=RequieredMemorySize;
    }
    else
    {
        pParamLogInfo->dwType=PARAM_VARIANT_PARSED;
        pParamLogInfo->Value=0;
        pParamLogInfo->dwSizeOfData=RequieredMemorySize;
        pParamLogInfo->dwSizeOfPointedValue=0;
    }

    // allocate memory
    pBuffer=(PBYTE)HeapAlloc(LogHeap,HEAP_ZERO_MEMORY,RequieredMemorySize);

    if (pBuffer)
    {
        pParamLogInfo->pbValue=pBuffer;
        // copy number of VARIANT items
        memcpy(pBuffer,&NbPointedItems,sizeof(DWORD));
        // call again func
        if (!CSupportedParameters::GetVariantFromStack(LogHeap,pVariant,NbPointedItems,pBuffer+sizeof(DWORD),&RequieredMemorySize))
        {
            HeapFree(LogHeap,0,pBuffer);
            CSupportedParameters::GetVariantFromStackDefaultParsing(LogHeap,pVariant,IsPointedValue,pParamLogInfo,pRequieredMemorySize);
            return;
        }
        // warning at this point *pRequieredMemorySize=RequieredMemorySize+sizeof(DWORD)
        // that's why we use RequieredMemorySize local var
    }
    else
    {
        CSupportedParameters::GetVariantFromStackDefaultParsing(LogHeap,pVariant,IsPointedValue,pParamLogInfo,pRequieredMemorySize);
        return;
    }
}
//-----------------------------------------------------------------------------
// Name: GetVariantFromStack
// Object: do an advanced parsing of VARIANT pointer
//          pVariant must be a valid pointer (caller is responsible to check pointer with IsBadReadPtr)
// forge
// pParamLogInfo->type =VARIANT_PARSED
//       pParamLogInfo->pbValue=nb_VARIANT + pParamLogInfoWithData(VARIANT1) + pParamLogInfoWithData(VARIANT2)+ ... + pParamLogInfoWithData(VARIANTN)
// Parameters :
//     in : HANDLE LogHeap : currently used heap
//          IN VARIANT* pVariant : pointer to data
//          DWORD PointedDataSize : size of pointed data in case of array of struct
//          IN BOOL IsPointedValue : TRUE if function argument is a pointer to struct
//                                   FALSE if struct is passed on stack
//     inout: IN OUT PBYTE pBuffer : buffer that will contain data (NULL to get only size)
//     out : OUT DWORD* pRequieredMemorySize : needed size
//           OUT DWORD* pNbItems : nb VARIANT items
//     return : 
//-----------------------------------------------------------------------------
BOOL CSupportedParameters::GetVariantFromStack(HANDLE LogHeap,IN VARIANT* pVariant,DWORD NbPointedItems,IN OUT PBYTE pBuffer,OUT DWORD* pRequieredMemorySize)
{
    *pRequieredMemorySize=0;

    // if bad pointer
    if (IsBadReadPtr(pVariant,sizeof(VARIANT)*NbPointedItems))
        return FALSE;

    PBYTE Pointer=NULL;
    DWORD DataSize=0;
    DWORD PointedDataSize=0;
    PARAMETER_LOG_INFOS* pVariantParamLogInfo;
    PARAMETER_LOG_INFOS VariantParamLogInfo;
    DWORD RequieredMemorySize=0;

    for (DWORD Cnt=0;Cnt<NbPointedItems;Cnt++)
    {
        // get next variant pointer
        if (Cnt)
            pVariant++;// pVariant= (VARIANT*)(((PBYTE)pVariant)+sizeof(VARIANT));
        VARTYPE VarType=pVariant->vt;
      
        // store variant type
        RequieredMemorySize+=sizeof(VARTYPE);
        if (pBuffer)
        {
            memcpy(pBuffer,&VarType,sizeof(VARTYPE));
            pBuffer+=sizeof(VARTYPE);
        }

        // get pointer to a PARAMETER_LOG_INFOS struct (simulated one VariantParamLogInfo, or output buffer pBuffer)
        if (pBuffer)
            pVariantParamLogInfo=(PARAMETER_LOG_INFOS*)pBuffer;
        else
            pVariantParamLogInfo=&VariantParamLogInfo;

        // zero memory
        memset(pVariantParamLogInfo,0,sizeof(PARAMETER_LOG_INFOS));

        // makes pBuffer point after PARAMETER_LOG_INFOS struct
        if (pBuffer)
            pBuffer+=sizeof(PARAMETER_LOG_INFOS);

        // increase memory size
        RequieredMemorySize+=sizeof(PARAMETER_LOG_INFOS);

        // here we have to interest us only to [V]
        /*
        // VARENUM usage key,
        // 
        // [V] - May appear in a VARIANT.
        // [T] - May appear in a TYPEDESC.
        // [P] - may appear in an OLE property set.
        // [S] - May appear in a Safe Array.
        // 
        // 
        VT_EMPTY            [V]   [P]         // Not specified.
        VT_NULL             [V]               // SQL-style Null.
        VT_I2               [V][T][P][S]      // 2-byte signed int.
        VT_I4               [V][T][P][S]      // 4-byte-signed int.
        VT_R4               [V][T][P][S]      // 4-byte real. 
        VT_R8               [V][T][P][S]      // 8-byte real.
        VT_CY               [V][T][P][S]      // Currency.
        VT_DATE             [V][T][P][S]      // Date.
        VT_BSTR             [V][T][P][S]      // Automation string.
        VT_DISPATCH         [V][T]   [S]      // IDispatch.Far*
        VT_ERROR            [V][T]   [S]      // Scodes.
        VT_BOOL             [V][T][P][S]      // Boolean; True=-1, False=0.
        VT_VARIANT          [V][T][P][S]      // VARIANT FAR*.
        VT_DECIMAL          [V][T]   [S]      // 16 byte fixed point.
        VT_RECORD           [V]   [P][S]      // User defined type
        VT_UNKNOWN          [V][T]   [S]      // IUnknown FAR*.
        VT_I1               [V][T]   [S]      // Char.
        VT_UI1              [V][T]   [S]      // Unsigned char.
        VT_UI2              [V][T]   [S]      // 2 byte unsigned int.
        VT_UI4              [V][T]   [S]      // 4 byte unsigned int. 
        VT_INT              [V][T]   [S]      // Signed machine int.
        VT_UINT             [V][T]   [S]      // Unsigned machine int.
        VT_VOID                [T]            // C-style void.
        VT_HRESULT             [T]                                    
        VT_PTR                 [T]            // Pointer type.
        VT_SAFEARRAY           [T]            // Use VT_ARRAY in VARIANT.
        VT_CARRAY              [T]            // C-style array.
        VT_USERDEFINED         [T]            // User-defined type.
        VT_LPSTR               [T][P]         // Null-terminated string.
        VT_LPWSTR              [T][P]         // Wide null-terminated string.
        VT_FILETIME               [P]         //FILETIME
        VT_BLOB                   [P]         //Length prefixed bytes
        VT_STREAM                 [P]         //Name of the stream follows
        VT_STORAGE                [P]         //Name of the storage follows
        VT_STREAMED_OBJECT        [P]         //Stream contains an object
        VT_STORED_OBJECT          [P]         //Storage contains an object
        VT_BLOB_OBJECT            [P]         //Blob contains an object
        VT_CF                     [P]         //Clipboard format
        VT_CLSID                  [P]         //A Class ID
        VT_VECTOR                 [P]         //simple counted array
        VT_ARRAY            [V]               // SAFEARRAY*.
        VT_BYREF            [V]
        VT_RESERVED

        VT_EMPTY No value was specified. If an optional argument to an Automation method is left blank, do not pass a VARIANT of type VT_EMPTY. Instead, pass a VARIANT of type VT_ERROR with a value of DISP_E_PARAMNOTFOUND. 
        VT_EMPTY | VT_BYREF Not valid. 
        VT_UI1 An unsigned 1-byte character is stored in bVal. 
        VT_UI1 | VT_BYREF A reference to an unsigned 1-byte character was passed. A pointer to the value is in pbVal. 
        VT_UI2 An unsigned 2-byte integer value is stored in uiVal. 
        VT_UI2 | VT_BYREF A reference to an unsigned 2-byte integer was passed. A pointer to the value is in puiVal. 
        VT_UI4 An unsigned 4-byte integer value is stored in ulVal. 
        VT_UI4 | VT_BYREF A reference to an unsigned 4-byte integer was passed. A pointer to the value is in pulVal. 
        VT_UI8 An unsigned 8-byte integer value is stored in ullVal. 
        VT_UI8 | VT_BYREF A reference to an unsigned 8-byte integer was passed. A pointer to the value is in pullVal. 
        VT_UINT An unsigned integer value is stored in uintVal. 
        VT_UINT | VT_BYREF A reference to an unsigned integer value was passed. A pointer to the value is in puintVal. 
        VT_INT An integer value is stored in intVal. 
        VT_INT | VT_BYREF A reference to an integer value was passed. A pointer to the value is in pintVal. 
        VT_I1 A 1-byte character value is stored in cVal. 
        VT_I1 | VT_BYREF A reference to a 1-byte character was passed. A pointer the value is in pcVal.  
        VT_I2 A 2-byte integer value is stored in iVal. 
        VT_I2 | VT_BYREF A reference to a 2-byte integer was passed. A pointer to the value is in piVal. 
        VT_I4 A 4-byte integer value is stored in lVal. 
        VT_I4 | VT_BYREF A reference to a 4-byte integer was passed. A pointer to the value is in plVal. 
        VT_I8 A 8-byte integer value is stored in llVal. 
        VT_I4 | VT_BYREF A reference to a 8-byte integer was passed. A pointer to the value is in pllVal. 
        VT_R4 An IEEE 4-byte real value is stored in fltVal. 
        VT_R4 | VT_BYREF A reference to an IEEE 4-byte real value was passed. A pointer to the value is in pfltVal. 
        VT_R8 An 8-byte IEEE real value is stored in dblVal. 
        VT_R8 | VT_BYREF A reference to an 8-byte IEEE real value was passed. A pointer to its value is in pdblVal. 
        VT_CY A currency value was specified. A currency number is stored as 64-bit (8-byte), two's complement integer, scaled by 10,000 to give a fixed-point number with 15 digits to the left of the decimal point and 4 digits to the right. The value is in cyVal. 
        VT_CY | VT_BYREF A reference to a currency value was passed. A pointer to the value is in pcyVal. 
        VT_BSTR A string was passed; it is stored in bstrVal. This pointer must be obtained and freed by the BSTR functions, which are described in Conversion and Manipulation Functions.  
        VT_BSTR | VT_BYREF A reference to a string was passed. A BSTR* that points to a BSTR is in pbstrVal. The referenced pointer must be obtained or freed by the BSTR functions. 
        VT_DECIMAL Decimal variables are stored as 96-bit (12-byte) unsigned integers scaled by a variable power of 10. VT_DECIMAL uses the entire 16 bytes of the Variant. 
        VT_DECIMAL | VT_BYREF A reference to a decimal value was passed. A pointer to the value is in pdecVal. 
        VT_NULL A propagating null value was specified. (This should not be confused with the null pointer.) The null value is used for tri-state logic, as with SQL. 
        VT_NULL | VT_BYREF Not valid. 
        VT_ERROR An SCODE was specified. The type of the error is specified in scodee. Generally, operations on error values should raise an exception or propagate the error to the return value, as appropriate. 
        VT_ERROR | VT_BYREF A reference to an SCODE was passed. A pointer to the value is in pscode. 
        VT_BOOL A 16 bit Boolean (True/False) value was specified. A value of 0xFFFF (all bits 1) indicates True; a value of 0 (all bits 0) indicates False. No other values are valid. 
        VT_BOOL | VT_BYREF A reference to a Boolean value. A pointer to the Boolean value is in pbool. 
        VT_DATE A value denoting a date and time was specified. Dates are represented as double-precision numbers, where midnight, January 1, 1900 is 2.0, January 2, 1900 is 3.0, and so on. The value is passed in date. 
        This is the same numbering system used by most spreadsheet programs, although some specify incorrectly that February 29, 1900 existed, and thus set January 1, 1900 to 1.0. The date can be converted to and from an MS-DOS representation using VariantTimeToDosDateTime, which is discussed in Conversion and Manipulation Functions. 

        VT_DATE | VT_BYREF A reference to a date was passed. A pointer to the value is in pdate. 
        VT_DISPATCH A pointer to an object was specified. The pointer is in pdispVal. This object is known only to implement IDispatch. The object can be queried as to whether it supports any other desired interface by calling QueryInterface on the object. Objects that do not implement IDispatch should be passed using VT_UNKNOWN. 
        VT_DISPATCH | VT_BYREF A pointer to a pointer to an object was specified. The pointer to the object is stored in the location referred to by ppdispVal. 
        VT_VARIANT Invalid. VARIANTARGs must be passed by reference. 
        VT_VARIANT | VT_BYREF A pointer to another VARIANTARG is passed in pvarVal. This referenced VARIANTARG, pvarVal, cannot be another VT_VARIANT|VT_BYREF. This value can be used to support languages that allow functions to change the types of variables passed by reference. 
        VT_UNKNOWN A pointer to an object that implements the IUnknown interface is passed in punkVal. 
        VT_UNKNOWN | VT_BYREF A pointer to the IUnknown interface is passed in ppunkVal. The pointer to the interface is stored in the location referred to by ppunkVal. 
        VT_ARRAY | <anything> An array of data type <anything> was passed. (VT_EMPTY and VT_NULL are invalid types to combine with VT_ARRAY.) The pointer in pparray points to an array descriptor, which describes the dimensions, size, and in-memory location of the array. The array descriptor is never accessed directly, but instead is read and modified using the functions described in Conversion and Manipulation Functions.  


        */

        
        BOOL Has_VT_BYREF_Type=((VarType & VT_BYREF)!=0);
        BOOL Has_VT_ARRAY_Type=((VarType & VT_ARRAY)!=0);

        if (Has_VT_BYREF_Type)
            VarType&=~VT_BYREF;

        if (Has_VT_ARRAY_Type)
            VarType&=~VT_ARRAY;

        // if variant is a pointer on a variant
        if (VarType==VT_VARIANT)
        {
            // if pointed value is a bad one
            if ((IsBadReadPtr(pVariant,sizeof(VARIANT))) || (!Has_VT_BYREF_Type))
                // set type to do a default parsing of variant type (struct saving)
                VarType=VT_NULL;
            else
            {
                DWORD mRequieredMemorySize=0;
                // pointer to a variant, try to get it's value
                // parse variant from stack (remember pBuffer==pVariantParamLogInfo)
                CSupportedParameters::GetVariantFromStackEx( LogHeap,pVariant->pvarVal,sizeof(VARIANT),TRUE,pVariantParamLogInfo,&mRequieredMemorySize);

                // copy data from pVariantParamLogInfo->pbValue into pBuffer after PARAMETER_LOG_INFOS struct,
                // and free pVariantParamLogInfo->pbValue previously allocated buffer
                if (pBuffer && pVariantParamLogInfo->pbValue)
                    // copy data
                    memcpy(pBuffer,pVariantParamLogInfo->pbValue,pVariantParamLogInfo->dwSizeOfPointedValue);

                if (pVariantParamLogInfo->pbValue) // even if !pBuffer
                    // free allocated memory
                    HeapFree(LogHeap,0, pVariantParamLogInfo->pbValue);

                if (pBuffer && pVariantParamLogInfo->pbValue)
                    // adjust pVariantParamLogInfo->pbValue
                    pVariantParamLogInfo->pbValue=pBuffer;

                // increase memory size
                RequieredMemorySize+=mRequieredMemorySize;
                if (pBuffer)
                    // move buffer pointer 
                    pBuffer+=mRequieredMemorySize;
                // parse next variant
                continue;
            }
        }

        // variant points to safe array
        if (Has_VT_ARRAY_Type)
        {
            SAFEARRAY* pSafeArray=NULL;
            DWORD mRequieredMemorySize=0;
            BOOL TryParsing=TRUE;

            // if byref
            if (Has_VT_BYREF_Type)
            {
                // assume pointer is ok
                if (IsBadReadPtr(pVariant->pparray,sizeof(SAFEARRAY*)))
                {
                    // don't try to parse bad pointer
                    TryParsing=FALSE;
                    // set type to do a default parsing of variant type (struct saving)
                    VarType=VT_NULL;
                }
                else
                    // store pointer value
                    pSafeArray=*pVariant->pparray;
            }
            else
                // store pointer value
                pSafeArray=pVariant->parray;

            if (IsBadReadPtr(pSafeArray,sizeof(SAFEARRAY)))
            {
                // don't try to parse bad pointer
                TryParsing=FALSE;
                // set type to do a default parsing of variant type (struct saving)
                VarType=VT_NULL;
            }

            // if safe array parsing should be tried
            if (TryParsing)
            {
                // parse safe array from stack (remember pBuffer==pVariantParamLogInfo)
                CSupportedParameters::GetSafeArrayFromStackEx( LogHeap,pSafeArray,sizeof(SAFEARRAY),TRUE,pVariantParamLogInfo,&mRequieredMemorySize);


                if (pBuffer && pVariantParamLogInfo->pbValue)
                    // copy data
                    memcpy(pBuffer,pVariantParamLogInfo->pbValue,pVariantParamLogInfo->dwSizeOfPointedValue);

                if (pVariantParamLogInfo->pbValue) // even if !pBuffer
                    // free allocated memory
                    HeapFree(LogHeap,0, pVariantParamLogInfo->pbValue);

                if (pBuffer && pVariantParamLogInfo->pbValue)
                    // adjust pVariantParamLogInfo->pbValue
                    pVariantParamLogInfo->pbValue=pBuffer;


                // increase memory size
                RequieredMemorySize+=mRequieredMemorySize;
                if (pBuffer)
                    // move buffer pointer 
                    pBuffer+=mRequieredMemorySize;
                // parse next variant
                continue;
            }
        }

        //////////////////////////////
        // basic types parsing
        //////////////////////////////

        switch(VarType)
        {
        case VT_R4:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PFLOAT;
                Pointer=(PBYTE)pVariant->pfltVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_FLOAT;
                Pointer=(PBYTE)&pVariant->fltVal;
            }
            break;
        case VT_R8:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PDOUBLE;
                Pointer=(PBYTE)pVariant->pdblVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_DOUBLE;
                Pointer=(PBYTE)&pVariant->dblVal;
            }
            break;
        case VT_CY:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PINT64;
                Pointer=(PBYTE)pVariant->pcyVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_INT64;
                Pointer=(PBYTE)&pVariant->cyVal;
            }
            break;
        case VT_DATE:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PDOUBLE;
                Pointer=(PBYTE)pVariant->pdate;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_DOUBLE;
                Pointer=(PBYTE)&pVariant->date;
            }
            break;
        case VT_BSTR:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_BSTR;
                if (IsBadReadPtr(pVariant->pbstrVal,sizeof(PBYTE)))
                {
                    pVariantParamLogInfo->dwType=PARAM_VARIANT;
                    Pointer=(PBYTE)pVariant;
                }
                else
                    Pointer=(PBYTE)*pVariant->pbstrVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_BSTR;
                Pointer=(PBYTE)pVariant->bstrVal;
            }
            break;
        case VT_ERROR:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PLONG;
                Pointer=(PBYTE)pVariant->pscode;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_LONG;
                Pointer=(PBYTE)&pVariant->scode;
            }
            break;
        case VT_BOOL:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PSHORT;
                Pointer=(PBYTE)pVariant->pboolVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_SHORT;
                Pointer=(PBYTE)&pVariant->boolVal;
            }
            break;
        case VT_DECIMAL:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PDECIMAL;
                Pointer=(PBYTE)pVariant->pdecVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_DECIMAL;
                Pointer=(PBYTE)&pVariant->decVal;
            }
            break;

        case VT_I1:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PBYTE;
                Pointer=(PBYTE)pVariant->pcVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_BYTE;
                Pointer=(PBYTE)&pVariant->cVal;
            }
            break;
        case VT_I2:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PSHORT;
                Pointer=(PBYTE)pVariant->piVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_SHORT;
                Pointer=(PBYTE)&pVariant->iVal;
            }
            break;
        case VT_I4:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PINT;
                Pointer=(PBYTE)pVariant->plVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_INT;
                Pointer=(PBYTE)&pVariant->lVal;
            }
            break;
        case VT_INT:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PINT;
                Pointer=(PBYTE)pVariant->pintVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_INT;
                Pointer=(PBYTE)&pVariant->intVal;
            }
            break;
        case VT_UI1:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PBYTE;
                Pointer=(PBYTE)pVariant->pbVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_BYTE;
                Pointer=(PBYTE)&pVariant->bVal;
            }
            break;
        case VT_UI2:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PWORD;
                Pointer=(PBYTE)pVariant->puiVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_WORD;
                Pointer=(PBYTE)&pVariant->uiVal;
            }
            break;
        case VT_UI4:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PUINT;
                Pointer=(PBYTE)pVariant->pulVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_UINT;
                Pointer=(PBYTE)&pVariant->ulVal;
            }
            break;
        case VT_UINT:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PUINT;
                Pointer=(PBYTE)pVariant->puintVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_UINT;
                Pointer=(PBYTE)&pVariant->uintVal;
            }
            break;

        case VT_I8:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PINT64;
                Pointer=(PBYTE)pVariant->pllVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_INT64;
                Pointer=(PBYTE)&pVariant->llVal;
            }
            break;
        case VT_UI8:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PINT64;
                Pointer=(PBYTE)pVariant->pullVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_INT64;
                Pointer=(PBYTE)&pVariant->ullVal;
            }
            break;

        case VT_DISPATCH:
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PPVOID;
                Pointer=(PBYTE)pVariant->ppdispVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_POINTER;
                Pointer=(PBYTE)&pVariant->pdispVal;
            }
            break;
        case VT_UNKNOWN :
            if (Has_VT_BYREF_Type)
            {
                pVariantParamLogInfo->dwType=PARAM_PPVOID;
                Pointer=(PBYTE)pVariant->ppunkVal;
            }
            else
            {
                pVariantParamLogInfo->dwType=PARAM_POINTER;
                Pointer=(PBYTE)&pVariant->punkVal;
            }
            break;

                break;
        case VT_RECORD: // we could use IRecordInfo but it will link project to ole dll
                        // that musn't be for apioverride.dll
        case VT_EMPTY:
        case VT_NULL:
        default:
            pVariantParamLogInfo->dwType=PARAM_VARIANT;
            Pointer=(PBYTE)pVariant;
            break;
        }

        if (pVariantParamLogInfo->dwType==PARAM_BSTR)
        {
            pVariantParamLogInfo->Value=Pointer;

            int iStringSize=CSupportedParameters::SecureWstrlen((LPWSTR)Pointer);
            if (iStringSize<0)
            {
                pVariantParamLogInfo->dwType=PARAM_VARIANT;
                Pointer=(PBYTE)pVariant;
            }
            else
            {
                PointedDataSize=(iStringSize+1)*sizeof(wchar_t);

                // compute necessary size
                RequieredMemorySize+=PointedDataSize;
                // if buffer is defined, copy data into it
                if (pBuffer)
                {
                    pVariantParamLogInfo->pbValue=pBuffer;
                    pVariantParamLogInfo->dwSizeOfPointedValue=PointedDataSize;
                    memcpy(pBuffer,(PVOID)Pointer,PointedDataSize);
                    pBuffer+=PointedDataSize;
                }
                continue;
            }
        }

        /////////////////////////////
        // generic values storing
        /////////////////////////////
        DataSize=CSupportedParameters::GetParamStackSize(pVariantParamLogInfo->dwType);
        PointedDataSize=CSupportedParameters::GetParamPointedSize(pVariantParamLogInfo->dwType);
        pVariantParamLogInfo->dwSizeOfData=DataSize;

        // if pointer or easy type
        if (pVariantParamLogInfo->dwSizeOfData<=sizeof(PBYTE))
        {
            // allocate and copy pointed data (if pointer)
            if (PointedDataSize)
            {
                pVariantParamLogInfo->Value=Pointer;
                
                if (!IsBadReadPtr((PVOID)Pointer, PointedDataSize))
                {
                    // compute necessary size
                    RequieredMemorySize+=PointedDataSize;
                    // if buffer is defined, copy data into it
                    if (pBuffer)
                    {
                        pVariantParamLogInfo->pbValue=pBuffer;
                        pVariantParamLogInfo->dwSizeOfPointedValue=PointedDataSize;
                        memcpy(pBuffer,(PVOID)Pointer,PointedDataSize);
                        pBuffer+=PointedDataSize;
                    }
                }
            }
            else
            {
                // copy parameter value
                pVariantParamLogInfo->Value=*((PBYTE*)Pointer);
            }
        }
        // struct passed on stack
        else
        {
            pVariantParamLogInfo->Value=0;

            PointedDataSize=DataSize;
            if (!IsBadReadPtr((PVOID)Pointer, PointedDataSize))
            {
                // compute necessary size
                RequieredMemorySize+=PointedDataSize;
                // if buffer is defined, copy data into it
                if (pBuffer)
                {
                    pVariantParamLogInfo->pbValue=pBuffer;
                    pVariantParamLogInfo->dwSizeOfData=PointedDataSize;
                    memcpy(pBuffer,(PVOID)Pointer,PointedDataSize);
                    pBuffer+=PointedDataSize;
                }
            }
        }

    }
    *pRequieredMemorySize=RequieredMemorySize;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ParameterToString
// Object: translate a parameter to it's string representation
// Parameters :
//      in: PARAMETER_LOG_INFOS* pParamLog : parameters log to transfert
//      out : TCHAR** pszParameter : Output string, MUST BE FREE BY "delete *pszParameter"
//                                   string contain the full parameter representation
// Return : 
//-----------------------------------------------------------------------------
void __fastcall CSupportedParameters::ParameterToString(TCHAR* const ModuleName,PARAMETER_LOG_INFOS* const pParamLog, TCHAR** pszParameter,BOOL const bParseDefinesAndUserDataType)
{
    CSupportedParameters::ParameterToString(ModuleName,pParamLog,pszParameter,0, bParseDefinesAndUserDataType);
}


//-----------------------------------------------------------------------------
// Name: ParameterToString
// Object: translate a parameter to it's string representation
//         use this function for better performance.
//         By the way if you want to parse a 2000 bytes array to only display 500 chars, 
//         it is useless to do a full parsing
// Parameters :
//      in: PARAMETER_LOG_INFOS* pParamLog : parameters log to transfert
//      out : TCHAR** pszParameter : Output string, MUST BE FREE BY "delete *pszParameter"
//                                   string may not contain full parameter representation
//            DWORD NbRequieredChars : required string size you need to use next, 
//                                     string size can be less or more than NbRequieredChars
// Return : 
//-----------------------------------------------------------------------------
void __fastcall CSupportedParameters::ParameterToString(TCHAR* const ModuleName,PARAMETER_LOG_INFOS* const pParamLog, TCHAR** pszParameter,DWORD const NbRequieredChars,BOOL const bParseDefinesAndUserDataType)
{
    return CSupportedParameters::ParameterToString(ModuleName,pParamLog,pszParameter,NbRequieredChars, bParseDefinesAndUserDataType,TRUE);
}
void __fastcall CSupportedParameters::ParameterToString(TCHAR* const ModuleName,PARAMETER_LOG_INFOS* const pParamLog, TCHAR** pszParameter,DWORD const NbRequieredChars,BOOL const bParseDefinesAndUserDataType,BOOL const bDisplayVarName)
{
    TCHAR pcHexaValue[9];
    DWORD dwCnt;
    DWORD dwStringIndex;
    DWORD dwType;
    DWORD NbMemberOfArray;
    TCHAR* pParameterName;
    TCHAR EmptyParamName;
    EmptyParamName=0;

    dwType=pParamLog->dwType & SIMPLE_TYPE_FLAG_MASK;
 
    // if a name has been provide for parameter
    if (*(pParamLog->pszParameterName)!=0)
        pParameterName=pParamLog->pszParameterName;
    else
        // get type name
        pParameterName=CSupportedParameters::GetParamName(dwType);

    if (bParseDefinesAndUserDataType)
    {
        if ( 
             (pParamLog->dwType & EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_DEFINE_VALUES_FILE)
             && pParamLog->pszDefineNamesFile
            )
        {
            return CSupportedParameters::DefineToString(ModuleName,pParamLog,pszParameter,NbRequieredChars);
        }

        if ( 
            (pParamLog->dwType & EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_USER_DATA_TYPE_FILE)
            && pParamLog->pszUserDataTypeName
            )
        {
            return CSupportedParameters::UserDataTypeToString(ModuleName,pParamLog,pszParameter,NbRequieredChars);
        }
    }

    switch(dwType)
    {

    // 1 byte unsigned value param
    case PARAM_UCHAR:
    case PARAM_BOOLEAN:
    case PARAM_BYTE:
    // 1 byte signed value param
    case PARAM_CHAR:
        *pszParameter=new TCHAR[128];
        if (bDisplayVarName)
            _stprintf(*pszParameter,_T("%s:0x%.2X") ,pParameterName,((BYTE)pParamLog->Value) & 0xFF);
        else
            _stprintf(*pszParameter,_T("0x%.2X") ,((BYTE)pParamLog->Value) & 0xFF);
        break;
    // 2 byte unsigned value param
    case PARAM_WORD:
    case PARAM_USHORT:
    case PARAM_WPARAM:
    case PARAM_WCHAR:
    // 2 byte signed value param
    case PARAM_SHORT:
        *pszParameter=new TCHAR[128];
        if (bDisplayVarName)
            _stprintf(*pszParameter,_T("%s:0x%.4X"),pParameterName,((SHORT)pParamLog->Value) & 0xFFFF);
        else
            _stprintf(*pszParameter,_T("0x%.4X"),((SHORT)pParamLog->Value) & 0xFFFF);
        break;

    // 4 byte unsigned value param
    case PARAM_ULONG:
    case PARAM_UINT:
    case PARAM_DWORD:
    case PARAM_REGSAM:
    case PARAM_ACCESS_MASK:
    case PARAM_LCID:
    case PARAM_COLORREF:
    case PARAM_SECURITY_INFORMATION:
    // 4 byte signed value param
    case PARAM_INT:
    case PARAM_BOOL:
    case PARAM_LONG:
    case PARAM_NTSTATUS:
        CSupportedParameters::GenericParameterParsing(pParamLog,
                                                      sizeof(DWORD),
                                                      FALSE,
                                                      CSupportedParameters::ParseDWORD,
                                                      ULONG_PTR_STRING_REPRESENTATION_MAX_SIZE,
                                                      pszParameter,
                                                      NbRequieredChars,
                                                      bDisplayVarName);
        break;

    // pointer values
    case PARAM_HANDLE:
    case PARAM_HINSTANCE:
    case PARAM_HWND:
    case PARAM_HMODULE:
    case PARAM_HDC:
    case PARAM_HMENU:
    case PARAM_HIMAGELIST:
    case PARAM_HDESK:
    case PARAM_HBRUSH:
    case PARAM_HRGN:
    case PARAM_HDPA:
    case PARAM_HDSA:
    case PARAM_WNDPROC:
    case PARAM_DLGPROC:
    case PARAM_FARPROC:
    case PARAM_SIZE_T:
    case PARAM_HKEY:
    case PARAM_HPALETTE:
    case PARAM_HFONT:
    case PARAM_HMETAFILE:
    case PARAM_HGDIOBJ:
    case PARAM_HCOLORSPACE:
    case PARAM_HBITMAP:
    case PARAM_HICON:
    case PARAM_HCONV:
    case PARAM_HSZ:
    case PARAM_HDDEDATA:
    case PARAM_SC_HANDLE:
    case PARAM_HCERTSTORE:
    case PARAM_HGLOBAL:
    case PARAM_ULONG_PTR: 
    case PARAM_HCRYPTPROV:
    case PARAM_HCRYPTKEY: 
    case PARAM_HCRYPTHASH:
    case PARAM_SOCKET:    
    case PARAM_PSID:
    case PARAM_PFNCALLBACK:
    case PARAM_PSECURITY_DESCRIPTOR:
    case PARAM_LSA_HANDLE:
    case PARAM_POINTER:
    case PARAM_VOID:
    // pointer signed value param
    case PARAM_LONG_PTR:
    case PARAM_LPARAM:
        CSupportedParameters::GenericParameterParsing(pParamLog,
                                                      sizeof(ULONG_PTR),
                                                      FALSE,
                                                      CSupportedParameters::ParseULONG_PTR,
                                                      ULONG_PTR_STRING_REPRESENTATION_MAX_SIZE,
                                                      pszParameter,
                                                      NbRequieredChars,
                                                      bDisplayVarName);
        break;

    // pointer to 4 byte data
    case PARAM_PBOOL:
    case PARAM_PINT:
    case PARAM_PLONG:
    case PARAM_PACCESS_MASK:
    case PARAM_PUINT:
    case PARAM_PULONG:
    case PARAM_PDWORD:
    case PARAM_PCOLORREF:
        CSupportedParameters::GenericParameterParsing(pParamLog,
                                                      sizeof(DWORD),
                                                      TRUE,
                                                      CSupportedParameters::ParseDWORD,
                                                      DWORD_STRING_REPRESENTATION_MAX_SIZE,
                                                      pszParameter,
                                                      NbRequieredChars,
                                                      bDisplayVarName);
        break;
    // as we put in hex representation, put signed and unsigned together
    case PARAM_PHANDLE:
    case PARAM_PHKEY:
    case PARAM_PHMODULE:
    case PARAM_PSIZE_T:
    case PARAM_PSOCKET:
    case PARAM_PPSID:
    case PARAM_PPSECURITY_DESCRIPTOR:
    case PARAM_PLONG_PTR:
    case PARAM_PULONG_PTR:
    case PARAM_PLSA_HANDLE:
    case PARAM_PHICON:
    case PARAM_PPOINTER:
    case PARAM_PPVOID:
        CSupportedParameters::GenericParameterParsing(pParamLog,
                                                      sizeof(ULONG_PTR),
                                                      TRUE,
                                                      CSupportedParameters::ParseULONG_PTR,
                                                      ULONG_PTR_STRING_REPRESENTATION_MAX_SIZE,
                                                      pszParameter,
                                                      NbRequieredChars,
                                                      bDisplayVarName);
        break;

    // pointer to 2 byte data
    case PARAM_PWORD:
    case PARAM_PSHORT:
        CSupportedParameters::GenericParameterParsing(pParamLog,
                                                        sizeof(WORD),
                                                        TRUE,
                                                        CSupportedParameters::ParseWORD,
                                                        WORD_STRING_REPRESENTATION_MAX_SIZE,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    // pointer to 1 byte data
    case PARAM_PBOOLEAN:
    case PARAM_PUCHAR:
    case PARAM_PBYTE:
    case PARAM_PVOID:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(BYTE),
                                                        TRUE,
                                                        CSupportedParameters::ParseBYTE,
                                                        BYTE_STRING_REPRESENTATION_MAX_SIZE,
                                                        pszParameter,
                                                        _T(" "),TRUE,TRUE,TRUE,
                                                        NbRequieredChars);
        break;
    case PARAM_PFILE:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(FILE),
                                                        TRUE,
                                                        CSupportedParameters::ParseFILE,
                                                        FILE_STRING_REPRESENTATION_MAX_SIZE,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PIO_STATUS_BLOCK:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(IO_STATUS_BLOCK),
                                                        TRUE,
                                                        CSupportedParameters::ParseIO_STATUS_BLOCK,
                                                        IO_STATUS_BLOCK_STRING_REPRESENTATION_MAX_SIZE,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PPRINTDLG:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(PRINTDLG),
                                                        TRUE,
                                                        CSupportedParameters::ParsePRINTDLG,
                                                        PRINTDLG_STRING_REPRESENTATION_MAX_SIZE,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PPRINTDLGEX:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(PRINTDLGEX),
                                                        TRUE,
                                                        CSupportedParameters::ParsePRINTDLGEX,
                                                        PRINTDLGEX_STRING_REPRESENTATION_MAX_SIZE,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PPAGESETUPDLG:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(PAGESETUPDLG),
                                                        TRUE,
                                                        CSupportedParameters::ParsePAGESETUPDLG,
                                                        PAGESETUPDLG_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_POPENFILENAME:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(OPENFILENAME),
                                                        TRUE,
                                                        CSupportedParameters::ParseOPENFILENAME,
                                                        OPENFILENAME_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PCHOOSEFONT:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(CHOOSEFONT),
                                                        TRUE,
                                                        CSupportedParameters::ParseCHOOSEFONT,
                                                        CHOOSEFONT_STRING_REPRESENTATION_MAX_SIZE,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PFINDREPLACE:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(FINDREPLACE),
                                                        TRUE,
                                                        CSupportedParameters::ParseFINDREPLACE,
                                                        FINDREPLACE_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
    
        break;

    case PARAM_PBROWSEINFO:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(BROWSEINFO),
                                                        TRUE,
                                                        CSupportedParameters::ParseBROWSEINFO,
                                                        BROWSEINFO_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
    
        break;

    case PARAM_PSHFILEINFOA:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(SHFILEINFOA),
                                                        TRUE,
                                                        CSupportedParameters::ParseSHFILEINFOA,
                                                        SHFILEINFOA_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
    
        break;

    case PARAM_PSHFILEINFOW:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(SHFILEINFOW),
                                                        TRUE,
                                                        CSupportedParameters::ParseSHFILEINFOW,
                                                        SHFILEINFOW_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
    
        break;

    case PARAM_PNOTIFYICONDATAA:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(NOTIFYICONDATA),
                                                        TRUE,
                                                        CSupportedParameters::ParseNOTIFYICONDATAA,
                                                        NOTIFYICONDATAA_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
    
        break;

    case PARAM_PNOTIFYICONDATAW:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(NOTIFYICONDATAW),
                                                        TRUE,
                                                        CSupportedParameters::ParseNOTIFYICONDATAW,
                                                        NOTIFYICONDATAW_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
    
        break;

    case PARAM_PDCB:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(DCB),
                                                        TRUE,
                                                        CSupportedParameters::ParseDCB,
                                                        DCB_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PCOMMTIMEOUTS:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(COMMTIMEOUTS),
                                                        TRUE,
                                                        CSupportedParameters::ParseCOMMTIMEOUTS,
                                                        COMMTIMEOUTS_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PCOMMCONFIG:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(COMMCONFIG),
                                                        TRUE,
                                                        CSupportedParameters::ParseCOMMCONFIG,
                                                        COMMCONFIG_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PSTARTUPINFO:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(STARTUPINFO),
                                                        TRUE,
                                                        CSupportedParameters::ParseSTARTUPINFO,
                                                        STARTUPINFO_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PSHELLEXECUTEINFO:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(SHELLEXECUTEINFO),
                                                        TRUE,
                                                        CSupportedParameters::ParseSHELLEXECUTEINFO,
                                                        SHELLEXECUTEINFO_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_LARGE_INTEGER:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(LARGE_INTEGER),
                                                        FALSE,
                                                        CSupportedParameters::ParseLARGE_INTEGER,
                                                        LARGE_INTEGER_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PLARGE_INTEGER:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(LARGE_INTEGER),
                                                        TRUE,
                                                        CSupportedParameters::ParseLARGE_INTEGER,
                                                        LARGE_INTEGER_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_POINT:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(POINT),
                                                        FALSE,
                                                        CSupportedParameters::ParsePOINT,
                                                        POINT_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PPOINT:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(POINT),
                                                        TRUE,
                                                        CSupportedParameters::ParsePOINT,
                                                        POINT_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;


    case PARAM_LOGFONTA:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(LOGFONT),
                                                        FALSE,
                                                        CSupportedParameters::ParseLOGFONTA,
                                                        LOGFONTA_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PLOGFONTA:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(LOGFONT),
                                                        TRUE,
                                                        CSupportedParameters::ParseLOGFONTA,
                                                        LOGFONTA_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_LOGFONTW:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(LOGFONT),
                                                        FALSE,
                                                        CSupportedParameters::ParseLOGFONTW,
                                                        LOGFONTW_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PLOGFONTW:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(LOGFONT),
                                                        TRUE,
                                                        CSupportedParameters::ParseLOGFONTW,
                                                        LOGFONTW_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_SIZE:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(SIZE),
                                                        FALSE,
                                                        CSupportedParameters::ParseSIZE,
                                                        SIZE_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PSIZE:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(SIZE),
                                                        TRUE,
                                                        CSupportedParameters::ParseSIZE,
                                                        SIZE_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);

        break;

    case PARAM_RECT:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(RECT),
                                                        FALSE,
                                                        CSupportedParameters::ParseRECT,
                                                        RECT_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PRECT:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(RECT),
                                                        TRUE,
                                                        CSupportedParameters::ParseRECT,
                                                        RECT_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PMSG:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(MSG),
                                                        TRUE,
                                                        CSupportedParameters::ParseMSG,
                                                        MSG_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PTIMEVAL:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(TIMEVAL),
                                                        TRUE,
                                                        CSupportedParameters::ParseTIMEVAL,
                                                        TIMEVAL_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_HOSTENT:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(hostent),
                                                        FALSE,
                                                        CSupportedParameters::ParseHOSTENT,
                                                        HOSTENT_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PHOSTENT:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(hostent),
                                                        TRUE,
                                                        CSupportedParameters::ParseHOSTENT,
                                                        HOSTENT_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_SOCKADDR:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(sockaddr),
                                                        FALSE,
                                                        CSupportedParameters::ParseSOCKADDR,
                                                        SOCKADDR_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PSOCKADDR:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(sockaddr),
                                                        TRUE,
                                                        CSupportedParameters::ParseSOCKADDR,
                                                        SOCKADDR_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_SOCKADDR_IN:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(sockaddr_in),
                                                        FALSE,
                                                        CSupportedParameters::ParseSOCKADDR_IN,
                                                        SOCKADDR_IN_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PSOCKADDR_IN:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(sockaddr_in),
                                                        TRUE,
                                                        CSupportedParameters::ParseSOCKADDR_IN,
                                                        SOCKADDR_IN_STRING_REPRESENTATION_MAX_SIZE,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;            
    case PARAM_GUID:
    case PARAM_IID:
    case PARAM_CLSID:
    case PARAM_FMTID:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(GUID),
                                                        FALSE,
                                                        CSupportedParameters::ParseGUID,
                                                        GUID_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PGUID:
    case PARAM_PIID:
    case PARAM_PCLSID:
    case PARAM_PFMTID:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(GUID),
                                                        TRUE,
                                                        CSupportedParameters::ParseGUID,
                                                        GUID_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break; 

    case PARAM_SECURITY_ATTRIBUTES:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(SECURITY_ATTRIBUTES),
                                                        FALSE,
                                                        CSupportedParameters::ParseSECURITY_ATTRIBUTES,
                                                        SECURITY_ATTRIBUTES_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PSECURITY_ATTRIBUTES:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(SECURITY_ATTRIBUTES),
                                                        TRUE,
                                                        CSupportedParameters::ParseSECURITY_ATTRIBUTES,
                                                        SECURITY_ATTRIBUTES_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_ACL:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(ACL),
                                                        FALSE,
                                                        CSupportedParameters::ParseACL,
                                                        ACL_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PACL:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(ACL),
                                                        TRUE,
                                                        CSupportedParameters::ParseACL,
                                                        ACL_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_MEMORY_BASIC_INFORMATION:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(MEMORY_BASIC_INFORMATION),
                                                        FALSE,
                                                        CSupportedParameters::ParseMEMORY_BASIC_INFORMATION,
                                                        MEMORY_BASIC_INFORMATION_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PMEMORY_BASIC_INFORMATION:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(MEMORY_BASIC_INFORMATION),
                                                        TRUE,
                                                        CSupportedParameters::ParseMEMORY_BASIC_INFORMATION,
                                                        MEMORY_BASIC_INFORMATION_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_POVERLAPPED:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(OVERLAPPED),
                                                        TRUE,
                                                        CSupportedParameters::ParseOVERLAPPED,
                                                        OVERLAPPED_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PWSABUF:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(WSABUF),
                                                        TRUE,
                                                        CSupportedParameters::ParseWSABUF,
                                                        WSABUF_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PADDRINFO:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(ADDRINFO),
                                                        TRUE,
                                                        CSupportedParameters::ParseADDRINFO,
                                                        ADDRINFO_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PWSADATA:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(WSADATA),
                                                        TRUE,
                                                        CSupportedParameters::ParseWSADATA,
                                                        WSADATA_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PWSAPROTOCOL_INFOA:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(WSAPROTOCOL_INFOA),
                                                        TRUE,
                                                        CSupportedParameters::ParseWSAPROTOCOL_INFOA,
                                                        WSAPROTOCOL_INFOA_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PWSAPROTOCOL_INFOW:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(WSAPROTOCOL_INFOW),
                                                        TRUE,
                                                        CSupportedParameters::ParseWSAPROTOCOL_INFOW,
                                                        WSAPROTOCOL_INFOW_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PFD_SET:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(fd_set),
                                                        TRUE,
                                                        CSupportedParameters::ParseFD_SET,
                                                        FD_SET_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_CRITICAL_SECTION:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(CRITICAL_SECTION),
                                                        FALSE,
                                                        CSupportedParameters::ParseCRITICAL_SECTION,
                                                        CRITICAL_SECTION_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PCRITICAL_SECTION:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(CRITICAL_SECTION),
                                                        TRUE,
                                                        CSupportedParameters::ParseCRITICAL_SECTION,
                                                        CRITICAL_SECTION_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PHEAPENTRY32:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(HEAPENTRY32),
                                                        TRUE,
                                                        CSupportedParameters::ParseHEAPENTRY32,
                                                        HEAPENTRY32_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PTHREADENTRY32:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(THREADENTRY32),
                                                        TRUE,
                                                        CSupportedParameters::ParseTHREADENTRY32,
                                                        THREADENTRY32_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PPROCESS_HEAP_ENTRY:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(PROCESS_HEAP_ENTRY),
                                                        TRUE,
                                                        CSupportedParameters::ParsePROCESS_HEAP_ENTRY,
                                                        PROCESS_HEAP_ENTRY_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_SYSTEMTIME:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(SYSTEMTIME),
                                                        FALSE,
                                                        CSupportedParameters::ParseSYSTEMTIME,
                                                        SYSTEMTIME_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PSYSTEMTIME:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(SYSTEMTIME),
                                                        TRUE,
                                                        CSupportedParameters::ParseSYSTEMTIME,
                                                        SYSTEMTIME_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_FILETIME:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(FILETIME),
                                                        FALSE,
                                                        CSupportedParameters::ParseFILETIME,
                                                        FILETIME_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
    case PARAM_PFILETIME:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(FILETIME),
                                                        TRUE,
                                                        CSupportedParameters::ParseFILETIME,
                                                        FILETIME_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PSECHANDLE:
    case PARAM_PCTXTHANDLE:
    case PARAM_PCREDHANDLE:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(SecHandle),
                                                        TRUE,
                                                        CSupportedParameters::ParseSECHANDLE,
                                                        SECHANDLE_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PDLGTEMPLATE:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(DLGTEMPLATE),
                                                        TRUE,
                                                        CSupportedParameters::ParseDLGTEMPLATE,
                                                        DLGTEMPLATE_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_WNDCLASS:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(WNDCLASS),
                                                        FALSE,
                                                        CSupportedParameters::ParseWNDCLASS,
                                                        WNDCLASS_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PWNDCLASS:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(WNDCLASS),
                                                        TRUE,
                                                        CSupportedParameters::ParseWNDCLASS,
                                                        WNDCLASS_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;


    case PARAM_WNDCLASSEX:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(WNDCLASSEX),
                                                        FALSE,
                                                        CSupportedParameters::ParseWNDCLASSEX,
                                                        WNDCLASSEX_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PWNDCLASSEX:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(WNDCLASSEX),
                                                        TRUE,
                                                        CSupportedParameters::ParseWNDCLASSEX,
                                                        WNDCLASSEX_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_DOUBLE:
        {
            *pszParameter=new TCHAR[128];
            if (IsBadReadPtr(pParamLog->pbValue,sizeof(double)))
            {
                if (bDisplayVarName)
                    _stprintf(*pszParameter,_T("%s:Internal Error"),pParameterName);
                else
                    _stprintf(*pszParameter,_T("Internal Error"));
            }
            else
            {
                if (bDisplayVarName)
                    _stprintf(*pszParameter, _T("%s:%.19g"),pParameterName, *((double*)pParamLog->pbValue));
                else
                    _stprintf(*pszParameter, _T("%.19g"),*((double*)pParamLog->pbValue));
            }
        }
        break;
    case PARAM_PDOUBLE:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(double),
                                                        TRUE,
                                                        CSupportedParameters::ParseDOUBLE,
                                                        DOUBLE_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_FLOAT:
        *pszParameter=new TCHAR[128];
        {
            // here we can't make a cast from pParamLog->dwValue to float, else data value in dwValue is converted to float
            // although pParamLog->dwValue already contains the float representation of the value
            float f;
            memcpy(&f,&pParamLog->Value,sizeof(float));
            if (bDisplayVarName)
                _stprintf(*pszParameter, _T("%s:%.19g"),pParameterName, f);
            else
                _stprintf(*pszParameter, _T("%.19g"),f);
        }
        break;

    case PARAM_PFLOAT:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(float),
                                                        TRUE,
                                                        CSupportedParameters::ParseFLOAT,
                                                        FLOAT_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
      break;

    
    case PARAM_INT64:
    case PARAM_TRACEHANDLE:
        *pszParameter=new TCHAR[128];
        if (pParamLog->dwSizeOfPointedValue>=0)// 32 bit
        {
            if (IsBadReadPtr(pParamLog->pbValue,sizeof(INT64)))
            {
                if (bDisplayVarName)
                    _stprintf(*pszParameter,_T("%s:Internal Error"),pParameterName);
                else
                    _stprintf(*pszParameter,_T("Internal Error"));
            }
            else
            {
                if (bDisplayVarName)
                    _stprintf(*pszParameter,_T("%s:0x%I64X"),pParameterName,*((INT64*)pParamLog->pbValue));
                else
                    _stprintf(*pszParameter,_T("0x%I64X"),*((INT64*)pParamLog->pbValue));
            }
        }
        else
        {
            if (bDisplayVarName)
                _stprintf(*pszParameter,_T("%s:0x%p"),pParameterName,pParamLog->Value);
            else
                _stprintf(*pszParameter,_T("0x%p"),pParamLog->Value);
        }
        break;

    case PARAM_PINT64:
    case PARAM_PTRACEHANDLE:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(INT64),
                                                        TRUE,
                                                        CSupportedParameters::ParseINT64,
                                                        INT64_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PPROCESSENTRY32A:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(PROCESSENTRY32A),
                                                        TRUE,
                                                        CSupportedParameters::ParsePROCESSENTRY32A,
                                                        PROCESSENTRY32A_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PPROCESSENTRY32W:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(PROCESSENTRY32W),
                                                        TRUE,
                                                        CSupportedParameters::ParsePROCESSENTRY32W,
                                                        PROCESSENTRY32W_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PMODULEENTRY32A:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(MODULEENTRY32A),
                                                        TRUE,
                                                        CSupportedParameters::ParseMODULEENTRY32A,
                                                        MODULEENTRY32A_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PMODULEENTRY32W:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(MODULEENTRY32W),
                                                        TRUE,
                                                        CSupportedParameters::ParseMODULEENTRY32W,
                                                        MODULEENTRY32W_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PWIN32_FIND_DATAA:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(WIN32_FIND_DATAA),
                                                        TRUE,
                                                        CSupportedParameters::ParseWIN32_FIND_DATAA,
                                                        WIN32_FIND_DATAA_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_PWIN32_FIND_DATAW:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(WIN32_FIND_DATAW),
                                                        TRUE,
                                                        CSupportedParameters::ParseWIN32_FIND_DATAW,
                                                        WIN32_FIND_DATAW_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_MUTLI_QI:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(MULTI_QI),
                                                        FALSE,
                                                        CSupportedParameters::ParseMULTI_QI,
                                                        MULTI_QI_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PMUTLI_QI:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(MULTI_QI),
                                                        TRUE,
                                                        CSupportedParameters::ParseMULTI_QI,
                                                        MULTI_QI_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_DECIMAL:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(DECIMAL),
                                                        FALSE,
                                                        CSupportedParameters::ParseDECIMAL,
                                                        DECIMAL_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PDECIMAL:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(DECIMAL),
                                                        TRUE,
                                                        CSupportedParameters::ParseDECIMAL,
                                                        DECIMAL_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;

    case PARAM_SAFEARRAY:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(SAFEARRAY),
                                                        FALSE,
                                                        CSupportedParameters::ParseSAFEARRAY,
                                                        SAFEARRAY_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PSAFEARRAY:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(SAFEARRAY),
                                                        TRUE,
                                                        CSupportedParameters::ParseSAFEARRAY,
                                                        SAFEARRAY_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_SAFEARRAY_PARSED:
        CSupportedParameters::ParseSAFEARRAY_PARSED(pParamLog,pszParameter,_T(","),NbRequieredChars);
        break;
    case PARAM_SAFEARRAYBOUND:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(SAFEARRAYBOUND),
                                                        FALSE,
                                                        CSupportedParameters::ParseSAFEARRAYBOUND,
                                                        SAFEARRAYBOUND_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PSAFEARRAYBOUND:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(SAFEARRAYBOUND),
                                                        TRUE,
                                                        CSupportedParameters::ParseSAFEARRAYBOUND,
                                                        SAFEARRAYBOUND_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_VARIANT:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(VARIANT),
                                                        FALSE,
                                                        CSupportedParameters::ParseVARIANT,
                                                        VARIANT_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PVARIANT:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(VARIANT),
                                                        TRUE,
                                                        CSupportedParameters::ParseVARIANT,
                                                        VARIANT_STRING_REPRESENTATION_MAX_SIZE ,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_VARIANT_PARSED:
        CSupportedParameters::ParseVARIANT_PARSED(pParamLog,pszParameter,_T(","),NbRequieredChars);
        break;

    case PARAM_EXCEPINFO:
         CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(EXCEPINFO),
                                                        FALSE,
                                                        CSupportedParameters::ParseEXCEPINFO,
                                                        EXCEPINFO_STRING_REPRESENTATION_MAX_SIZE,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PEXCEPINFO:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(EXCEPINFO),
                                                        TRUE,
                                                        CSupportedParameters::ParseEXCEPINFO,
                                                        EXCEPINFO_STRING_REPRESENTATION_MAX_SIZE,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_DISPPARAMS:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(DISPPARAMS),
                                                        FALSE,
                                                        CSupportedParameters::ParseDISPPARAMS,
                                                        DISPPARAMS_STRING_REPRESENTATION_MAX_SIZE,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_PDISPPARAMS:
        CSupportedParameters::GenericParameterParsing(
                                                        pParamLog,
                                                        sizeof(DISPPARAMS),
                                                        TRUE,
                                                        CSupportedParameters::ParseDISPPARAMS,
                                                        DISPPARAMS_STRING_REPRESENTATION_MAX_SIZE,
                                                        pszParameter,
                                                        NbRequieredChars,
                                                        bDisplayVarName);
        break;
    case PARAM_EXCEPINFO_PARSED:
        CSupportedParameters::ParseEXCEPINFO_PARSED(pParamLog,pszParameter,_T(","),NbRequieredChars);
        break;
    case PARAM_DISPPARAMS_PARSED:
        CSupportedParameters::ParseDISPPARAMS_PARSED(pParamLog,pszParameter,_T(","),NbRequieredChars);
        break;
    case PARAM_PANSI_STRING:
        {
            
            // PSTR:0x.8x[:"string"]
            // dwSizeOfPointedValue is size in byte so it's equal to number of chars for ansi string
            if (pParamLog->dwSizeOfPointedValue<sizeof(ANSI_STRING))
            {
                *pszParameter=new TCHAR[128+pParamLog->dwSizeOfPointedValue];
                if (bDisplayVarName)
                    _stprintf(*pszParameter,_T("%s:0x%p:"),pParameterName,pParamLog->Value);
                else
                    _stprintf(*pszParameter,_T("0x%p:"),pParamLog->Value);
                _tcscat(*pszParameter,_T("Bad Pointer"));
            }
            else
            {
                TCHAR* pszTmpParameter=new TCHAR[300+pParamLog->dwSizeOfPointedValue];
                if (bDisplayVarName)
                    _stprintf(pszTmpParameter,_T("%s:0x%p:{Length:%u,MaximumLength:%u,Buffer:0x%p:"),
                        pParameterName,
                        pParamLog->Value,
                        ((PANSI_STRING)pParamLog->pbValue)->Length,
                        ((PANSI_STRING)pParamLog->pbValue)->MaximumLength,
                        ((PANSI_STRING)pParamLog->pbValue)->Buffer
                        );
                else
                    _stprintf(pszTmpParameter,_T("0x%p:{Length:%u,MaximumLength:%u,Buffer:0x%p:"),
                        pParamLog->Value,
                        ((PANSI_STRING)pParamLog->pbValue)->Length,
                        ((PANSI_STRING)pParamLog->pbValue)->MaximumLength,
                        ((PANSI_STRING)pParamLog->pbValue)->Buffer
                        );

                _tcscat(pszTmpParameter,_T("\""));
#if (defined(UNICODE)||defined(_UNICODE))
                MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&pParamLog->pbValue[sizeof(ANSI_STRING)], pParamLog->dwSizeOfPointedValue-sizeof(ANSI_STRING),
                                &(pszTmpParameter[_tcslen(pszTmpParameter)]), pParamLog->dwSizeOfPointedValue-sizeof(ANSI_STRING));
#else
            
                _tcscat(pszTmpParameter,(TCHAR*)&pParamLog->pbValue[sizeof(ANSI_STRING)]);
#endif
                _tcscat(pszTmpParameter,_T("\""));
                _tcscat(pszTmpParameter,_T("}"));

                // replace parameters bad for presentation and dangerous for conversion (\r \n \t)
                *pszParameter=new TCHAR[(_tcslen(pszTmpParameter)+1)*2];
                TCHAR* Tmp=new TCHAR[(_tcslen(pszTmpParameter)+1)*2];
                _tcscpy(Tmp,pszTmpParameter);
                delete[] pszTmpParameter;
                
                CSupportedParameters::StrReplace(Tmp,*pszParameter,_T("\r"),_T("\\r"));
                _tcscpy(Tmp,*pszParameter);
                CSupportedParameters::StrReplace(Tmp,*pszParameter,_T("\n"),_T("\\n"));
                _tcscpy(Tmp,*pszParameter);
                CSupportedParameters::StrReplace(Tmp,*pszParameter,_T("\t"),_T("\\t"));
                delete[] Tmp;
            }
        }
        break;
    case PARAM_PSTR:
        {
            
            // PSTR:0x.8x[:"string"]
            // dwSizeOfPointedValue is size in byte so it's equal to number of chars for ansi string
            if (pParamLog->dwSizeOfPointedValue==0)
            {
                *pszParameter=new TCHAR[128+pParamLog->dwSizeOfPointedValue];
                if (bDisplayVarName)
                    _stprintf(*pszParameter,_T("%s:0x%p:"),pParameterName,pParamLog->Value);
                else
                    _stprintf(*pszParameter,_T("0x%p:"),pParamLog->Value);
                _tcscat(*pszParameter,_T("Bad Pointer"));
            }
            else
            {
                TCHAR* pszTmpParameter=new TCHAR[128+pParamLog->dwSizeOfPointedValue];
                if (bDisplayVarName)
                {
                    if (pParamLog->Value)
                        _stprintf(pszTmpParameter,_T("%s:0x%p:"),pParameterName,pParamLog->Value);
                    else // pParamLog->Value = 0 and pParamLog->dwSizeOfPointedValue!=0 is used for struct parsing
                        _stprintf(pszTmpParameter,_T("%s:"),pParameterName);
                }
                else
                {
                    if (pParamLog->Value)
                        _stprintf(pszTmpParameter,_T("0x%p:"),pParamLog->Value);
                    else // pParamLog->Value = 0 and pParamLog->dwSizeOfPointedValue!=0 is used for struct parsing
                        *pszTmpParameter=0;
                }
                _tcscat(pszTmpParameter,_T("\""));
#if (defined(UNICODE)||defined(_UNICODE))
                MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pParamLog->pbValue, pParamLog->dwSizeOfPointedValue,
                                &(pszTmpParameter[_tcslen(pszTmpParameter)]), pParamLog->dwSizeOfPointedValue);
#else
            
                _tcscat(pszTmpParameter,(TCHAR*)pParamLog->pbValue);
#endif
                _tcscat(pszTmpParameter,_T("\""));

                // replace parameters bad for presentation and dangerous for conversion (\r \n \t)
                *pszParameter=new TCHAR[(_tcslen(pszTmpParameter)+1)*2];
                TCHAR* Tmp=new TCHAR[(_tcslen(pszTmpParameter)+1)*2];
                _tcscpy(Tmp,pszTmpParameter);
                delete[] pszTmpParameter;
                
                CSupportedParameters::StrReplace(Tmp,*pszParameter,_T("\r"),_T("\\r"));
                _tcscpy(Tmp,*pszParameter);
                CSupportedParameters::StrReplace(Tmp,*pszParameter,_T("\n"),_T("\\n"));
                _tcscpy(Tmp,*pszParameter);
                CSupportedParameters::StrReplace(Tmp,*pszParameter,_T("\t"),_T("\\t"));
                delete[] Tmp;
            }
        }
        break;

    case PARAM_PUNICODE_STRING:
        {
            // PWSTR:0x.8x[:"string"]
            // dwSizeOfPointedValue is size in byte so it's equal to number of wchars*2 for unicode string

            if (pParamLog->dwSizeOfPointedValue==0)
            {
                *pszParameter=new TCHAR[128+pParamLog->dwSizeOfPointedValue/2];
                if (bDisplayVarName)
                    _stprintf(*pszParameter,_T("%s:0x%p:"),pParameterName,pParamLog->Value);
                else
                    _stprintf(*pszParameter,_T("0x%p:"),pParamLog->Value);
                _tcscat(*pszParameter,_T("Bad Pointer"));
            }
            else
            {
                TCHAR* pszTmpParameter=new TCHAR[300+pParamLog->dwSizeOfPointedValue/2];
                if (bDisplayVarName)
                    _stprintf(pszTmpParameter,_T("%s:0x%p:{Length:%u,MaximumLength:%u,Buffer:0x%p:"),
                        pParameterName,
                        pParamLog->Value,
                        ((PUNICODE_STRING)pParamLog->pbValue)->Length,
                        ((PUNICODE_STRING)pParamLog->pbValue)->MaximumLength,
                        ((PUNICODE_STRING)pParamLog->pbValue)->Buffer
                        );
                else
                    _stprintf(pszTmpParameter,_T("0x%p:{Length:%u,MaximumLength:%u,Buffer:0x%p:"),
                        pParamLog->Value,
                        ((PUNICODE_STRING)pParamLog->pbValue)->Length,
                        ((PUNICODE_STRING)pParamLog->pbValue)->MaximumLength,
                        ((PUNICODE_STRING)pParamLog->pbValue)->Buffer
                        );

                _tcscat(pszTmpParameter,_T("\""));
#if (defined(UNICODE)||defined(_UNICODE))
                _tcscat(pszTmpParameter,(TCHAR*)&pParamLog->pbValue[sizeof(UNICODE_STRING)]);
#else
                WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)&pParamLog->pbValue[sizeof(UNICODE_STRING)], pParamLog->dwSizeOfPointedValue-(sizeof(UNICODE_STRING))/2, 
                    &(pszTmpParameter[_tcslen(pszTmpParameter)]), (pParamLog->dwSizeOfPointedValue-sizeof(UNICODE_STRING))/2, NULL, NULL);   
                
#endif
                _tcscat(pszTmpParameter,_T("\""));
                _tcscat(pszTmpParameter,_T("}"));

                // replace parameters bad for presentation and dangerous for conversion (\r \n \t)
                *pszParameter=new TCHAR[(_tcslen(pszTmpParameter)+1)*2];
                TCHAR* Tmp=new TCHAR[(_tcslen(pszTmpParameter)+1)*2];
                _tcscpy(Tmp,pszTmpParameter);
                delete[] pszTmpParameter;
                
                CSupportedParameters::StrReplace(Tmp,*pszParameter,_T("\r"),_T("\\r"));
                _tcscpy(Tmp,*pszParameter);
                CSupportedParameters::StrReplace(Tmp,*pszParameter,_T("\n"),_T("\\n"));
                _tcscpy(Tmp,*pszParameter);
                CSupportedParameters::StrReplace(Tmp,*pszParameter,_T("\t"),_T("\\t"));
                delete[] Tmp;
            }
        }

        break;
    case PARAM_PWSTR:
    case PARAM_BSTR:
        {
            // PWSTR:0x.8x[:"string"]
            // dwSizeOfPointedValue is size in byte so it's equal to number of wchars*2 for unicode string

            if (pParamLog->dwSizeOfPointedValue==0)
            {
                *pszParameter=new TCHAR[128+pParamLog->dwSizeOfPointedValue/sizeof(WCHAR)];
                if (bDisplayVarName)
                    _stprintf(*pszParameter,_T("%s:0x%p:"),pParameterName,pParamLog->Value);
                else
                    _stprintf(*pszParameter,_T("0x%p:"),pParamLog->Value);
                _tcscat(*pszParameter,_T("Bad Pointer"));
            }
            else
            {
                TCHAR* pszTmpParameter=new TCHAR[128+pParamLog->dwSizeOfPointedValue/sizeof(WCHAR)];
                if (bDisplayVarName)
                {
                    if (pParamLog->Value)
                        _stprintf(pszTmpParameter,_T("%s:0x%p:"),pParameterName,pParamLog->Value);
                    else // pParamLog->Value = 0 and pParamLog->dwSizeOfPointedValue!=0 is used for struct parsing
                        _stprintf(pszTmpParameter,_T("%s:"),pParameterName);
                }
                else
                {
                    if (pParamLog->Value)
                        _stprintf(pszTmpParameter,_T("0x%p:"),pParamLog->Value);
                    else // pParamLog->Value = 0 and pParamLog->dwSizeOfPointedValue!=0 is used for struct parsing
                        *pszTmpParameter=0;
                }

                _tcscat(pszTmpParameter,_T("\""));
#if (defined(UNICODE)||defined(_UNICODE))
                _tcscat(pszTmpParameter,(TCHAR*)pParamLog->pbValue);
#else
                WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)pParamLog->pbValue, pParamLog->dwSizeOfPointedValue/sizeof(WCHAR), 
                    &(pszTmpParameter[_tcslen(pszTmpParameter)]), pParamLog->dwSizeOfPointedValue/sizeof(WCHAR), NULL, NULL);   
                
#endif
                _tcscat(pszTmpParameter,_T("\""));

                // replace parameters bad for presentation and dangerous for conversion (\r \n \t)
                *pszParameter=new TCHAR[(_tcslen(pszTmpParameter)+1)*2];
                TCHAR* Tmp=new TCHAR[(_tcslen(pszTmpParameter)+1)*2];
                _tcscpy(Tmp,pszTmpParameter);
                delete[] pszTmpParameter;
                
                CSupportedParameters::StrReplace(Tmp,*pszParameter,_T("\r"),_T("\\r"));
                _tcscpy(Tmp,*pszParameter);
                CSupportedParameters::StrReplace(Tmp,*pszParameter,_T("\n"),_T("\\n"));
                _tcscpy(Tmp,*pszParameter);
                CSupportedParameters::StrReplace(Tmp,*pszParameter,_T("\t"),_T("\\t"));
                delete[] Tmp;
            }
        }

        break;


    case PARAM_UNKNOWN:
        {
            BOOL bPointer;
            if (pParamLog->dwSizeOfPointedValue)
            {
                bPointer=TRUE;
                NbMemberOfArray=pParamLog->dwSizeOfPointedValue/sizeof(BYTE);
            }
            else
            {
                bPointer=FALSE;
                NbMemberOfArray=pParamLog->dwSizeOfData/sizeof(BYTE);
            }

            if ((!bPointer)&&(pParamLog->dwSizeOfData<=REGISTER_BYTE_SIZE))
            {
                // show a classical PBYTE representation
                *pszParameter=new TCHAR[128];
                if (bDisplayVarName)
                    _stprintf(*pszParameter,_T("%s:0x%p") ,pParameterName,pParamLog->Value);
                else
                    _stprintf(*pszParameter,_T("0x%p") ,pParamLog->Value);
            }
            else if ((bPointer)&&(NbMemberOfArray==0))
            {
                // show pointer value
                *pszParameter=new TCHAR[128];
                if (bDisplayVarName)
                    _stprintf(*pszParameter,_T("%s:0x%p:Bad Pointer"),pParameterName,pParamLog->Value);
                else
                    _stprintf(*pszParameter,_T("0x%p:Bad Pointer"),pParamLog->Value);
            }
            else if ((bPointer)&&(NbMemberOfArray==1))
            {
                *pszParameter=new TCHAR[128];
                if (bDisplayVarName)
                    _stprintf(*pszParameter,_T("%s:0x%p:0x%X") ,pParameterName,pParamLog->Value,*(BYTE*)pParamLog->pbValue);
                else
                    _stprintf(*pszParameter,_T("0x%p:0x%X") ,pParamLog->Value,*(BYTE*)pParamLog->pbValue);
            }
            else
            {
                // specific : don't add ",0x" but " " for better visibility of buffers
                *pszParameter=new TCHAR[128+NbMemberOfArray*6];//6 : 4 presentation char + 2 for max representation of DWORD value in hexa
                if (bPointer)
                {
                    if (bDisplayVarName)
                        _stprintf(*pszParameter,_T("%s:0x%p:"),pParameterName,pParamLog->Value);
                    else
                        _stprintf(*pszParameter,_T("0x%p:"),pParamLog->Value);
                }
                else // pParamLog->dwValue has no sens
                {
                    if (bDisplayVarName)
                        _stprintf(*pszParameter,_T("%s:"),pParameterName);
                }

                _tcscat(*pszParameter,_T("{"));
                dwStringIndex=0;// use &(*pszParameter)[dwStringIndex] instead of *pszParameter 
                                // to increase speed for big buffers
                for (dwCnt=0;dwCnt<NbMemberOfArray;dwCnt++)
                {
                    if (dwCnt!=0)
                        _tcscat(&(*pszParameter)[dwStringIndex++],_T(" "));
                    _stprintf(pcHexaValue,_T("%.2X"),*((BYTE*)(&pParamLog->pbValue[dwCnt*sizeof(BYTE)])));
                    _tcscat(&(*pszParameter)[dwStringIndex],pcHexaValue);
                    dwStringIndex+=2;
                }
                _tcscat(&(*pszParameter)[dwStringIndex++],_T("}"));
            }
        }
        break;
    default:// hex format
        *pszParameter=new TCHAR[128];
        if (bDisplayVarName)
            _stprintf(*pszParameter,_T("%s:0x%p") ,pParameterName,pParamLog->Value);
        else
            _stprintf(*pszParameter,_T("0x%p") ,pParamLog->Value);
        break;
    }
}

//-----------------------------------------------------------------------------
// Name: StrReplace
// Object: replace pszOldStr by pszNewStr in pszInputString and put the result in 
//         pszOutputString.
//         Warning pszOutputString must be large enough no check is done
// Parameters :
//     in  : TCHAR* pszInputString : string to translate
//           TCHAR* pszOldStr : string to replace
//           TCHAR* pszNewStr : replacing string
//     out : TCHAR* pszOutputString : result string
//     return : 
//-----------------------------------------------------------------------------
void CSupportedParameters::StrReplace(TCHAR* pszInputString,TCHAR* pszOutputString,TCHAR* pszOldStr,TCHAR* pszNewStr)
{
    TCHAR* pszPos;
    TCHAR* pszOldPos;
    int SearchedItemSize;

    if ((!pszInputString)||(!pszOutputString)||(!pszOldStr)||(!pszNewStr))
        return;

    *pszOutputString=0;

    pszOldPos=pszInputString;
    // get searched item size
    SearchedItemSize=(int)_tcslen(pszOldStr);
    // look for next string to replace
    pszPos=_tcsstr(pszInputString,pszOldStr);
    while(pszPos)
    {
        // copy unchanged data
        _tcsncat(pszOutputString,pszOldPos,pszPos-pszOldPos);
        // copy replace string
        _tcscat(pszOutputString,pszNewStr);
        // update old position
        pszOldPos=pszPos+SearchedItemSize;
        // look for next string to replace
        pszPos=_tcsstr(pszOldPos,pszOldStr);
    }
    // copy remaining data
    _tcscat(pszOutputString,pszOldPos);
}

//-----------------------------------------------------------------------------
// Name: SplitParameterFields
// Object: try to split a parameter into fields
// Parameters :
//     in  : TCHAR* pszParameter : parameter to split, can be delete after call
//           DWORD ParamType : parameter type
//     out : CLinkListSimple** ppLinkListSimple : a simple link list where ItemData is TCHAR* representing one fields
//                                                MUST BE FREE with CSupportedParameters::FreeSplittedParameterFields
//     return : FALSE if parameter is not splitted (no fields or error)
//                  in this case *ppLinkListSimple is NULL
//              TRUE if parameter is successfully splitted
//-----------------------------------------------------------------------------
BOOL CSupportedParameters::SplitParameterFields(TCHAR* pszParameter,DWORD ParamType,CLinkListSimple** ppLinkListSimple)
{
    TCHAR* pszFieldBegin;
    TCHAR* pszFieldEnd;
    TCHAR* pszNextSubField;
    TCHAR* pszSubFieldBegin=0;
    TCHAR* pszSubFieldEnd;
    CLinkListItem* pItem;
    DWORD SubFieldOrder;
    BOOL bSubField;
    TCHAR* psz;
    TCHAR* LocalpszParameter;

    *ppLinkListSimple=NULL;
    if (!pszParameter)
        return FALSE;

    // avoid parsing if not necessary (speed up a little)
    switch(ParamType)
    {
    // put here type that can't be array and that are not struct
    case PARAM_UNKNOWN:
    case PARAM_PSTR:
    case PARAM_PWSTR:
    case PARAM_BSTR:
    case PARAM_PVOID:
    case PARAM_PBYTE:
    case PARAM_PBOOLEAN:
    case PARAM_PUCHAR:
    case PARAM_UCHAR:
    case PARAM_BOOLEAN:
    case PARAM_BYTE:
    case PARAM_CHAR:
    case PARAM_WORD:
    case PARAM_USHORT:
    case PARAM_WPARAM:
    case PARAM_WCHAR:
    case PARAM_SHORT:
    case PARAM_HANDLE:
    case PARAM_HINSTANCE:
    case PARAM_HWND:
    case PARAM_HMODULE:
    case PARAM_HDC:
    case PARAM_HMENU:
    case PARAM_HIMAGELIST:
    case PARAM_HDESK:
    case PARAM_HBRUSH:
    case PARAM_HRGN:
    case PARAM_HDPA:
    case PARAM_HDSA:
    case PARAM_WNDPROC:
    case PARAM_DLGPROC:
    case PARAM_FARPROC:
    case PARAM_SIZE_T:
    case PARAM_ULONG:
    case PARAM_UINT:
    case PARAM_DWORD:
    case PARAM_HKEY:
    case PARAM_HPALETTE:
    case PARAM_HFONT:
    case PARAM_HMETAFILE:
    case PARAM_HGDIOBJ:
    case PARAM_HCOLORSPACE:
    case PARAM_HBITMAP:
    case PARAM_HICON:
    case PARAM_HCONV:
    case PARAM_HSZ:
    case PARAM_HDDEDATA:
    case PARAM_SC_HANDLE:
    case PARAM_HCERTSTORE:
    case PARAM_HGLOBAL:
    case PARAM_ULONG_PTR: 
    case PARAM_HCRYPTPROV:
    case PARAM_HCRYPTKEY: 
    case PARAM_HCRYPTHASH:
    case PARAM_SOCKET:    
    case PARAM_PSID:
    case PARAM_PFNCALLBACK:
    case PARAM_COLORREF:
    case PARAM_SECURITY_INFORMATION:
    case PARAM_PSECURITY_DESCRIPTOR:
    case PARAM_REGSAM:
    case PARAM_ACCESS_MASK:
    case PARAM_LCID:
    case PARAM_LONG_PTR:
    case PARAM_LPARAM:
    case PARAM_INT:
    case PARAM_BOOL:
    case PARAM_LONG:
    case PARAM_NTSTATUS:
    case PARAM_INT64:
    case PARAM_TRACEHANDLE:
    case PARAM_FLOAT:
    case PARAM_DOUBLE:
    case PARAM_LSA_HANDLE:
    case PARAM_VOID:
    case PARAM_POINTER:
        return FALSE;
    }

    // pszParameter can be like
    // Type:Value
    // Type:PointerValue:PointedData
    // Type:{Field1,....Fieldn}
    // Type:{Field1,{SubField1,...,SubFieldn},....Fieldn}

    // search for first field splitter
    pszFieldBegin=_tcschr(pszParameter,'{');
    // if none --> no fields
    if (!pszFieldBegin)
        return FALSE;

    // check for }
    pszFieldEnd=_tcsrchr(pszFieldBegin,'}');
    if (!pszFieldEnd)
        return FALSE;

    // check for a splitter (avoid variant string representation having guid to be considered as array)
    psz=_tcschr(pszFieldBegin,',');
    // if none --> no fields
    if (!psz)
        return FALSE;

    LocalpszParameter=_tcsdup(pszParameter);
    pszFieldEnd=pszFieldBegin-pszParameter+LocalpszParameter;
    pszFieldBegin=LocalpszParameter;

    // else create a CLinkListSimple to contain address of fields
    *ppLinkListSimple=new CLinkListSimple();

    *pszFieldEnd=0;
    pItem=(*ppLinkListSimple)->AddItem();
    pItem->ItemData=new TCHAR[_tcslen(pszFieldBegin)+1];//+1 for \0
    _tcscpy((TCHAR*)pItem->ItemData,pszFieldBegin);

    // point after Type:
    pszFieldBegin=pszFieldEnd+1;

    // remove last }
    pszFieldEnd=_tcsrchr(pszFieldBegin,'}');
    if (pszFieldEnd)
        *pszFieldEnd=0;

    bSubField=TRUE;
    for(;;)
    {
        // search next ','
        pszFieldEnd=_tcschr(pszFieldBegin,',');

        // if no more ',' the remaining data are the last field
        if (!pszFieldEnd)
        {
            // add field to list
            pItem=(*ppLinkListSimple)->AddItem();
            pItem->ItemData=new TCHAR[_tcslen(pszFieldBegin)+1];//+1 for \0
            _tcscpy((TCHAR*)pItem->ItemData,pszFieldBegin);

            // free memory
            free(LocalpszParameter);
            // return success status
            return TRUE;
        }

        // if there's still subfields
        if (bSubField)
        {
            // get next subfield position
            pszNextSubField=_tcschr(pszFieldBegin,'{');
            psz=pszNextSubField+1;
            if (pszNextSubField)
            {
                // if '{' is before ',' the subfield is inside current field
                if (pszNextSubField<pszFieldEnd)
                {
                    SubFieldOrder=1;

                    // while there's subfields included in current subfield
                    do
                    {
                        if (bSubField) // try to speed up
                            // find next sub field
                            pszSubFieldBegin=_tcschr(psz,'{');

                        pszSubFieldEnd=_tcschr(psz,'}');
                        if (!pszSubFieldEnd)
                        {
                            // parsing error: free ppLinkListSimple and return FALSE
                            CSupportedParameters::FreeSplittedParameterFields(ppLinkListSimple);
                            free(LocalpszParameter);
                            return FALSE;
                        }

                        if (!pszSubFieldBegin)
                            bSubField=FALSE;

                        // another subfield degree
                        if ((bSubField)&&(pszSubFieldBegin<pszSubFieldEnd))
                        {
                            SubFieldOrder++;
                            psz=pszSubFieldBegin+1;
                        }
                        // a subfield degre less
                        else
                        {
                            SubFieldOrder--;
                            psz=pszSubFieldEnd+1;
                        }

                    }
                    // end of first degree subfield
                    while(SubFieldOrder>0);

                    pszNextSubField=psz;

                    // if end of subfield is after pszFieldEnd (first ',')
                    if (pszNextSubField>pszFieldEnd)
                        // update pszFieldEnd
                        pszFieldEnd=pszNextSubField;
                }
            }
            // no more sub fields try to speed up
            else
                bSubField=FALSE;
        }
        *pszFieldEnd=0;
        pszFieldEnd++;

        // add field to list
        pItem=(*ppLinkListSimple)->AddItem();
        pItem->ItemData=new TCHAR[_tcslen(pszFieldBegin)+1];//+1 for \0
        _tcscpy((TCHAR*)pItem->ItemData,pszFieldBegin);

        // position pszFieldBegin to find next field
        pszFieldBegin=pszFieldEnd;
    }

    free(LocalpszParameter);
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: FreeSplittedParameterFields  
// Object: free CLinkListSimple** allocated by SplitParameterFields
// Parameters :
//     in  : CLinkListSimple** ppLinkListSimple : link list allocated by SplitParameterFields
//     return : FALSE if parameter is not splitted (no fields or error)
//                  in this case *ppLinkListSimple is NULL
//              TRUE if parameter is successfully splitted
//-----------------------------------------------------------------------------
void CSupportedParameters::FreeSplittedParameterFields(CLinkListSimple** ppLinkListSimple)
{
    if (*ppLinkListSimple==NULL)
        return;
    CLinkListItem* pItem;
    CLinkListItem* pNextItem;
    CLinkListSimple* pLinkListSimple;
    pLinkListSimple=(*ppLinkListSimple);

    *ppLinkListSimple=NULL;

    pLinkListSimple->Lock();
    // for each item of *ppLinkListSimple
    for (pItem=pLinkListSimple->Head;pItem;pItem=pNextItem)
    {
        // get next item before removing pItem from list
        pNextItem=pItem->NextItem;

        // delete field string
        delete pItem->ItemData;
        // remove item from list
        pLinkListSimple->RemoveItem(pItem,TRUE);
    }
    pLinkListSimple->Unlock();
    delete pLinkListSimple;
}

//-----------------------------------------------------------------------------
// Name: GenericParameterParsing
// Object: generic parsing type
// Parameters :
//     in : PARAMETER_LOG_INFOS* pParamLog : parameter log
//          DWORD SizeOfType : size of data type
//          BOOL IsPointer : TRUE if pointer (as only pointer to data are given to this func we have to know
//                                            if data is originaly pointer)
//                           ex : FALSE for DWORD, TRUE for DWORD*
//          tagParamParsingProc pfnParsingFunc : specific parsing function
//          DWORD OneParamStringRepresentationSize : size of string representation (depends of parsing function)
//     out : TCHAR** ppszParam : string parameter representation
//     return :  
//-----------------------------------------------------------------------------
void __fastcall CSupportedParameters::GenericParameterParsing(PARAMETER_LOG_INFOS* const pParamLog,
                                                            DWORD const SizeOfType,
                                                            BOOL const IsPointer,
                                                            tagParamParsingProc const pfnParsingFunc,
                                                            DWORD const OneParamStringRepresentationSize,
                                                            OUT TCHAR** ppszParam,
                                                            DWORD const NbRequieredChars,
                                                            BOOL const DisplayTypeName)
{
    CSupportedParameters::GenericParameterParsing(pParamLog,
                                                   SizeOfType,
                                                   IsPointer,
                                                   pfnParsingFunc,
                                                   OneParamStringRepresentationSize,
                                                   ppszParam,
                                                   _T(","),
                                                   DisplayTypeName,
                                                   TRUE,
                                                   TRUE,
                                                   NbRequieredChars
                                                   );
}

//-----------------------------------------------------------------------------
// Name: GenericParameterParsing
// Object: generic parsing type
// Parameters :
//     in : PARAMETER_LOG_INFOS* pParamLog : parameter log
//          DWORD SizeOfType : size of data type
//          BOOL IsPointer : TRUE if pointer (as only pointer to data are given to this func we have to know
//                                            if data is originaly pointer)
//                           ex : FALSE for DWORD, TRUE for DWORD*
//          tagParamParsingProc pfnParsingFunc : specific parsing function
//          DWORD OneParamStringRepresentationSize : size of string representation (depends of parsing function)
//          TCHAR* FieldSplitter : field separator
//          BOOL DisplayTypeName : TRUE to display type of name
//          BOOL DisplayPointerValue : TRUE to display pointer value
//          BOOL DisplayArrayLimits : TRUE to display { } at the begining and end of array
//          DWORD NbRequieredChars : 0 for full parsing, else number of requiered char :
//                                   by the way avoid a full parsing if next only 10 chars are used
//                                   If full required size for string representation is upper than
//                                   NbRequieredChars, string representation assume you will have at
//                                   least NbRequieredChars (there can be more)
//     out : TCHAR** ppszParam : string parameter representation
//     return :  
//-----------------------------------------------------------------------------
void __fastcall CSupportedParameters::GenericParameterParsing(PARAMETER_LOG_INFOS* const pParamLog,
                                                            DWORD const SizeOfType,
                                                            BOOL const IsPointer,
                                                            tagParamParsingProc const pfnParsingFunc,
                                                            DWORD const OneParamStringRepresentationSize,
                                                            OUT TCHAR** ppszParam,
                                                            TCHAR* const FieldSplitter,
                                                            BOOL const DisplayTypeName,
                                                            BOOL const DisplayPointerValue,
                                                            BOOL const DisplayArrayLimits,
                                                            DWORD const NbRequieredChars
                                                            )
{
    DWORD NbParameters;
    PVOID PointedData;
    PBYTE PointerValue=0;
    TCHAR* pParameterName;
    
    if (*(pParamLog->pszParameterName)!=0)
        pParameterName=pParamLog->pszParameterName;
    else
        pParameterName=CSupportedParameters::GetParamName(pParamLog->dwType);

    if (IsPointer)
    {
        NbParameters=pParamLog->dwSizeOfPointedValue/SizeOfType;
        PointedData=pParamLog->pbValue;
        PointerValue=pParamLog->Value;
    }
    else
    {
        NbParameters=pParamLog->dwSizeOfData/SizeOfType;
        if (pParamLog->dwSizeOfData>REGISTER_BYTE_SIZE)
            PointedData=pParamLog->pbValue;
        else
        {
            PointedData=(PVOID)&pParamLog->Value;
            // backward compatibility
            // item is not a pointer, it's size is less than REGISTER_BYTE_SIZE so
            // data is in pParamLog->Value. If pParamLog->dwSizeOfData == 0 that means there is/was (for logs reloading) a bug
            if (NbParameters==0)
                NbParameters = 1;
        }
    }

    if (NbParameters==0)
    {
        *ppszParam=new TCHAR[128];
        if (DisplayTypeName)
            _stprintf(*ppszParam,_T("%s:0x%p: Bad Pointer"),pParameterName,PointerValue);
        else
            _stprintf(*ppszParam,_T("0x%p: Bad Pointer"),PointerValue);
    }
    else
    {
        TCHAR* pszOneParam;
        PBYTE pCurrentParam;
        DWORD dwCnt;
        SIZE_T  StringIndex;
        BOOL  Reduced=FALSE;

        // adjust nb parameters to NbRequieredChars
        if (NbRequieredChars)
        {
            // compute the number of required params to fill the NbRequieredChars
            DWORD TmpNbParameters=(NbRequieredChars-128)/(OneParamStringRepresentationSize+_tcslen(FieldSplitter))+1;
            // if number of required params is less than the real one, adjust NbParameters
            if (TmpNbParameters<NbParameters)
            {
                NbParameters=TmpNbParameters;
                Reduced=TRUE;
            }
        }

        *ppszParam=new TCHAR[128+NbParameters*(OneParamStringRepresentationSize+_tcslen(FieldSplitter))];
        pszOneParam=new TCHAR[OneParamStringRepresentationSize];
        **ppszParam=0;
        *pszOneParam=0;
        
        if (IsPointer)
        {
            if (DisplayPointerValue)
            {
                if (DisplayTypeName)
                    _stprintf(*ppszParam,_T("%s: 0x%p: "),pParameterName,PointerValue);
                else
                    _stprintf(*ppszParam,_T("0x%p: "),PointerValue);
            }
            else if (DisplayTypeName)
                _stprintf(*ppszParam,_T("%s: "),pParameterName);
        }
        else if (DisplayTypeName)
            _stprintf(*ppszParam,_T("%s: "),pParameterName);

        pCurrentParam=(PBYTE)PointedData;

        // add { to mark the begin of array if wanted
        if (DisplayArrayLimits)
            if (NbParameters>1)
                _tcscat(*ppszParam,_T("{"));

        StringIndex=_tcslen(*ppszParam);// use &(*ppszParam)[StringIndex] instead of *ppszParam 
                                        // to increase speed for big buffers
        SIZE_T FieldSplitterLength=_tcslen(FieldSplitter);
        for (dwCnt=0;dwCnt<NbParameters;dwCnt++)
        {
            if (dwCnt!=0)
            {
                _tcscat(&(*ppszParam)[StringIndex],FieldSplitter);
                StringIndex+=FieldSplitterLength;
            }

            // call specific parsing func
            pfnParsingFunc(pCurrentParam,pszOneParam);

            // add to current one
            _tcscat(&(*ppszParam)[StringIndex],pszOneParam);
            StringIndex+=_tcslen(pszOneParam);

            // get next param
            pCurrentParam+=SizeOfType;
        }

        if (Reduced)
        {
            _tcscat(&(*ppszParam)[StringIndex],_T("..."));
            StringIndex+=3;
        }

        // add } to mark the end of the array if wanted
        if (DisplayArrayLimits)
        {
            if (NbParameters>1)
            {
                _tcscat(&(*ppszParam)[StringIndex],_T("}"));
                StringIndex++;
            }
        }

        delete[] pszOneParam;
    }
}
void CSupportedParameters::ParseDWORD(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,_T("0x%X") ,*(DWORD*)pData);
}

void CSupportedParameters::ParseULONG_PTR(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,_T("0x%p") ,*(PULONG_PTR*)pData);
}

void CSupportedParameters::ParseWORD(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,_T("0x%X") ,*(WORD*)pData);
}
void CSupportedParameters::ParseBYTE(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,_T("%.2X"),*(BYTE*)pData);
}
void CSupportedParameters::ParseFILE(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,_T("{ptr=0x%p,cnt=%u,base=0x%p,flag=0x%X,file=0x%X,charbuf=0x%X,bufsize=0x%X,tmpfname=0x%X}"),
                ((FILE*)pData)->_ptr,
                ((FILE*)pData)->_cnt,
                ((FILE*)pData)->_base,
                ((FILE*)pData)->_flag,
                ((FILE*)pData)->_file,
                ((FILE*)pData)->_charbuf,
                ((FILE*)pData)->_bufsiz,
                ((FILE*)pData)->_tmpfname);
}
void CSupportedParameters::ParseIO_STATUS_BLOCK(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,_T("{Status=0x%X,Information=0x%X}"),
                ((IO_STATUS_BLOCK*)pData)->Status,
                ((IO_STATUS_BLOCK*)pData)->Information);
}
void CSupportedParameters::ParsePRINTDLG(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
        _T("{lStructSize=%u,hwndOwner=0x%p,hDevMode=0x%p,hDevNames=0x%p,hDC=0x%p,Flags=0x%X,")
                _T("nFromPage=%u,nToPage=%u,nMinPage=%u,nMaxPage=%u,nCopies=%u,hInstance=0x%p,")
                _T("lCustData=0x%X,lpfnPrintHook=0x%p,lpfnSetupHook=0x%p,lpPrintTemplateName=0x%p,lpSetupTemplateName=0x%p,hPrintTemplate=0x%p,hSetupTemplate=0x%p}"),
                ((PRINTDLG*)pData)->lStructSize,
                ((PRINTDLG*)pData)->hwndOwner,
                ((PRINTDLG*)pData)->hDevMode,
                ((PRINTDLG*)pData)->hDevNames,
                ((PRINTDLG*)pData)->hDC,
                ((PRINTDLG*)pData)->Flags,
                ((PRINTDLG*)pData)->nFromPage,
                ((PRINTDLG*)pData)->nToPage,
                ((PRINTDLG*)pData)->nMinPage,
                ((PRINTDLG*)pData)->nMaxPage,
                ((PRINTDLG*)pData)->nCopies,
                ((PRINTDLG*)pData)->hInstance,
                ((PRINTDLG*)pData)->lCustData,
                ((PRINTDLG*)pData)->lpfnPrintHook,
                ((PRINTDLG*)pData)->lpfnSetupHook,
                ((PRINTDLG*)pData)->lpPrintTemplateName,
                ((PRINTDLG*)pData)->lpSetupTemplateName,
                ((PRINTDLG*)pData)->hPrintTemplate,
                ((PRINTDLG*)pData)->hSetupTemplate
                );
}
void CSupportedParameters::ParsePRINTDLGEX(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
                _T("{lStructSize=%u,hwndOwner=0x%p,hDevMode=0x%p,hDevNames=0x%p,hDC=0x%p,Flags=0x%X,")
                _T("Flags2=0x%X,ExclusionFlags=0x%X,nPageRanges=%u,nMaxPageRanges=%u,lpPageRanges=0x%p,")
                _T("nMinPage=%u,nMaxPage=%u,nCopies=%u,hInstance=0x%p,lpPrintTemplateName=0x%p,lpCallback=0x%p,nPropertyPages=%u,")
                _T("lphPropertyPages=0x%p,nStartPage=%u,dwResultAction=0x%X}"),
                ((PRINTDLGEX*)pData)->lStructSize,
                ((PRINTDLGEX*)pData)->hwndOwner,
                ((PRINTDLGEX*)pData)->hDevMode,
                ((PRINTDLGEX*)pData)->hDevNames,
                ((PRINTDLGEX*)pData)->hDC,
                ((PRINTDLGEX*)pData)->Flags,
                ((PRINTDLGEX*)pData)->Flags2,
                ((PRINTDLGEX*)pData)->ExclusionFlags,
                ((PRINTDLGEX*)pData)->nPageRanges,
                ((PRINTDLGEX*)pData)->nMaxPageRanges,
                ((PRINTDLGEX*)pData)->lpPageRanges,
                ((PRINTDLGEX*)pData)->nMinPage,
                ((PRINTDLGEX*)pData)->nMaxPage,
                ((PRINTDLGEX*)pData)->nCopies,
                ((PRINTDLGEX*)pData)->hInstance,
                ((PRINTDLGEX*)pData)->lpPrintTemplateName,
                ((PRINTDLGEX*)pData)->lpCallback,
                ((PRINTDLGEX*)pData)->nPropertyPages,
                ((PRINTDLGEX*)pData)->lphPropertyPages,
                ((PRINTDLGEX*)pData)->nStartPage,
                ((PRINTDLGEX*)pData)->dwResultAction
                );
}
void CSupportedParameters::ParsePAGESETUPDLG(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
                _T("{lStructSize=%u") 
                _T(",hwndOwner=0x%X") 
                _T(",hDevMode=0x%X") 
                _T(",hDevNames=0x%X") 
                _T(",Flags=0x%X,")
                _T("ptPaperSize={x=%d,y=%d},rtMinMargin={top=%d,bottom=%d,left=%d,right=%d},rtMargin={top=%d,bottom=%d,left=%d,right=%d},")
                _T("hInstance=0x%p") 
                _T(",lCustData=0x%X") 
                _T(",lpfnPageSetupHook=0x%p") 
                _T(",lpfnPagePaintHook=0x%p") 
                _T(",lpPageSetupTemplateName=0x%p") 
                _T(",hPageSetupTemplate=0x%p}"),
                ((PAGESETUPDLG*)pData)->lStructSize,
                (UINT_PTR)((PAGESETUPDLG*)pData)->hwndOwner,
                (UINT_PTR)((PAGESETUPDLG*)pData)->hDevMode,
                (UINT_PTR)((PAGESETUPDLG*)pData)->hDevNames,
                ((PAGESETUPDLG*)pData)->Flags,
                ((PAGESETUPDLG*)pData)->ptPaperSize.x,
                ((PAGESETUPDLG*)pData)->ptPaperSize.y,
                ((PAGESETUPDLG*)pData)->rtMinMargin.top,
                ((PAGESETUPDLG*)pData)->rtMinMargin.bottom,
                ((PAGESETUPDLG*)pData)->rtMinMargin.left,
                ((PAGESETUPDLG*)pData)->rtMinMargin.right,
                ((PAGESETUPDLG*)pData)->rtMargin.top,
                ((PAGESETUPDLG*)pData)->rtMargin.bottom,
                ((PAGESETUPDLG*)pData)->rtMargin.left,
                ((PAGESETUPDLG*)pData)->rtMargin.right,
                ((PAGESETUPDLG*)pData)->hInstance,
                ((PAGESETUPDLG*)pData)->lCustData,
                ((PAGESETUPDLG*)pData)->lpfnPageSetupHook,
                ((PAGESETUPDLG*)pData)->lpfnPagePaintHook,
                ((PAGESETUPDLG*)pData)->lpPageSetupTemplateName,
                ((PAGESETUPDLG*)pData)->hPageSetupTemplate
                );
}
void CSupportedParameters::ParseOPENFILENAME(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
                _T("{lStructSize=%u,hwndOwner=0x%p,hInstance=0x%p,lpstrFilter=0x%p,lpstrCustomFilter=0x%p,")
                _T("nMaxCustFilter=%u,nFilterIndex=%u,lpstrFile=0x%p,nMaxFile=%u,lpstrFileTitle=0x%p,nMaxFileTitle=%u,")
                _T("lpstrInitialDir=0x%p,lpstrTitle=0x%p,Flags=0x%X,nFileOffset=%u,nFileExtension=%u,lpstrDefExt=0x%p,lCustData=0x%X,")
                _T("lpfnHook=0x%p,lpTemplateName=0x%p}"),
                ((OPENFILENAME*)pData)->lStructSize,
                ((OPENFILENAME*)pData)->hwndOwner,
                ((OPENFILENAME*)pData)->hInstance,
                ((OPENFILENAME*)pData)->lpstrFilter,
                ((OPENFILENAME*)pData)->lpstrCustomFilter,
                ((OPENFILENAME*)pData)->nMaxCustFilter,
                ((OPENFILENAME*)pData)->nFilterIndex,
                ((OPENFILENAME*)pData)->lpstrFile,
                ((OPENFILENAME*)pData)->nMaxFile,
                ((OPENFILENAME*)pData)->lpstrFileTitle,
                ((OPENFILENAME*)pData)->nMaxFileTitle,
                ((OPENFILENAME*)pData)->lpstrInitialDir,
                ((OPENFILENAME*)pData)->lpstrTitle,
                ((OPENFILENAME*)pData)->Flags,
                ((OPENFILENAME*)pData)->nFileOffset,
                ((OPENFILENAME*)pData)->nFileExtension,
                ((OPENFILENAME*)pData)->lpstrDefExt,
                ((OPENFILENAME*)pData)->lCustData,
                ((OPENFILENAME*)pData)->lpfnHook,
                ((OPENFILENAME*)pData)->lpTemplateName
                );
}

void CSupportedParameters::ParseCHOOSEFONT(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
                _T("{lStructSize=%u,hwndOwner=0x%p,hInstance=0x%p,hDC=0x%p,lpLogFont=0x%p,")
                _T("iPointSize=%u,Flags=0x%X,rgbColors=0x%X,lCustData=0x%X,lpfnHook=0x%p,lpTemplateName=0x%p") 
                _T("lpszStyle=0x%p,nSizeMin=%u,nSizeMax=%u,ReservedAlign=0x%X}"),
                ((CHOOSEFONT*)pData)->lStructSize,
                ((CHOOSEFONT*)pData)->hwndOwner,
                ((CHOOSEFONT*)pData)->hInstance,
                ((CHOOSEFONT*)pData)->hDC,
                ((CHOOSEFONT*)pData)->lpLogFont,
                ((CHOOSEFONT*)pData)->iPointSize,
                ((CHOOSEFONT*)pData)->Flags,
                ((CHOOSEFONT*)pData)->rgbColors,
                ((CHOOSEFONT*)pData)->lCustData,
                ((CHOOSEFONT*)pData)->lpfnHook,
                ((CHOOSEFONT*)pData)->lpTemplateName,
                ((CHOOSEFONT*)pData)->lpszStyle,
                ((CHOOSEFONT*)pData)->nSizeMin,
                ((CHOOSEFONT*)pData)->nSizeMax,
                ((CHOOSEFONT*)pData)->___MISSING_ALIGNMENT__
                );
}

void CSupportedParameters::ParseFINDREPLACE(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
                _T("{lStructSize=%u,hwndOwner=0x%p,hInstance=0x%p,Flags=0x%X,")
                _T("lpstrFindWhat=0x%p,wFindWhatLen=%u,lpstrReplaceWith=0x%p,wReplaceWithLen=%u,")
                _T("lCustData=0x%X,lpfnHook=0x%p,lpTemplateName=0x%p}"),
                ((FINDREPLACE*)pData)->lStructSize,
                ((FINDREPLACE*)pData)->hwndOwner,
                ((FINDREPLACE*)pData)->hInstance,
                ((FINDREPLACE*)pData)->Flags,
                ((FINDREPLACE*)pData)->lpstrFindWhat,
                ((FINDREPLACE*)pData)->wFindWhatLen,
                ((FINDREPLACE*)pData)->lpstrReplaceWith,
                ((FINDREPLACE*)pData)->wReplaceWithLen,
                ((FINDREPLACE*)pData)->lCustData,
                ((FINDREPLACE*)pData)->lpfnHook,
                ((FINDREPLACE*)pData)->lpTemplateName
                );
}

void CSupportedParameters::ParseBROWSEINFO(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
                _T("{hwndOwner=0x%p,pidlRoot=0x%p,pszDisplayName=0x%p,")
                _T("lpszTitle=0x%p,ulFlags=%u,lpfn=0x%p,lParam=0x%X,iImage=%u}"),
                ((BROWSEINFO*)pData)->hwndOwner,
                ((BROWSEINFO*)pData)->pidlRoot,
                ((BROWSEINFO*)pData)->pszDisplayName,
                ((BROWSEINFO*)pData)->lpszTitle,
                ((BROWSEINFO*)pData)->ulFlags,
                ((BROWSEINFO*)pData)->lpfn,
                ((BROWSEINFO*)pData)->lParam,
                ((BROWSEINFO*)pData)->iImage
                );
}

void CSupportedParameters::ParseSHFILEINFOA(PVOID pData,OUT TCHAR* StringRepresentation)
{
#if (defined(UNICODE)||defined(_UNICODE))
    TCHAR        szDisplayName[MAX_PATH];
    TCHAR        szTypeName[80];
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)((SHFILEINFOA*)pData)->szDisplayName, MAX_PATH-1,szDisplayName,(int)MAX_PATH-1);
    szDisplayName[MAX_PATH-1]=0;
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)((SHFILEINFOA*)pData)->szTypeName, 80-1,szTypeName,(int)80-1);
    szTypeName[80-1]=0;
#endif
    _stprintf(StringRepresentation,
                _T("{hIcon=0x%p,iIcon=%u,dwAttributes=0x%X,")
                _T("szDisplayName=%s,szTypeName=%s}"),
                ((SHFILEINFOA*)pData)->hIcon,
                ((SHFILEINFOA*)pData)->iIcon,
                ((SHFILEINFOA*)pData)->dwAttributes,
#if (defined(UNICODE)||defined(_UNICODE))
                szDisplayName,
                szTypeName
#else
                ((SHFILEINFOA*)pData)->szDisplayName,
                ((SHFILEINFOA*)pData)->szTypeName
#endif
                );
}

void CSupportedParameters::ParseSHFILEINFOW(PVOID pData,OUT TCHAR* StringRepresentation)
{
#if (!defined(UNICODE)&&!defined(_UNICODE))
    TCHAR        szDisplayName[MAX_PATH];
    TCHAR        szTypeName[80];
    WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)((SHFILEINFOW*)pData)->szDisplayName, MAX_PATH-1, szDisplayName,MAX_PATH-1, NULL, NULL);
    szDisplayName[MAX_PATH-1]=0;
    WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)((SHFILEINFOW*)pData)->szTypeName, 80-1, szTypeName,(int)80-1, NULL, NULL);
    szTypeName[80-1]=0;
#endif
    _stprintf(StringRepresentation,
                _T("{hIcon=0x%p,iIcon=%u,dwAttributes=0x%X,")
                _T("szDisplayName=%s,szTypeName=%s}"),
                ((SHFILEINFOW*)pData)->hIcon,
                ((SHFILEINFOW*)pData)->iIcon,
                ((SHFILEINFOW*)pData)->dwAttributes,
#if (!defined(UNICODE)&&!defined(_UNICODE))
                szDisplayName,
                szTypeName
#else
                ((SHFILEINFOW*)pData)->szDisplayName,
                ((SHFILEINFOW*)pData)->szTypeName
#endif
                );
}

// we display only data for the smallest struct (_WIN32_IE < 0x0500) as we don't know with which version
// software as been compiled (we have to check cbSize and do lot of parsing)
void CSupportedParameters::ParseNOTIFYICONDATAA(PVOID pData,OUT TCHAR* StringRepresentation)
{
#if (defined(UNICODE)||defined(_UNICODE))
    TCHAR        szTip[64];
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)((NOTIFYICONDATAA*)pData)->szTip, 64-1,szTip,(int)64-1);
    szTip[64-1]=0;
#endif
    _stprintf(StringRepresentation,
                _T("{cbSize=%u,hWnd=0x%p,uID=0x%X,")
                _T("uFlags=0x%X,uCallbackMessage=0x%X,hIcon=0x%p,szTip=%s}"),
                ((NOTIFYICONDATAA*)pData)->cbSize,
                ((NOTIFYICONDATAA*)pData)->hWnd,
                ((NOTIFYICONDATAA*)pData)->uID,
                ((NOTIFYICONDATAA*)pData)->uFlags,
                ((NOTIFYICONDATAA*)pData)->uCallbackMessage,
                ((NOTIFYICONDATAA*)pData)->hIcon,
#if (defined(UNICODE)||defined(_UNICODE))
                szTip
#else
                ((NOTIFYICONDATAA*)pData)->szTip
#endif
                );
}

// we display only data for the smallest struct (_WIN32_IE < 0x0500) as we don't know with which version
// software as been compiled (we have to check cbSize and do lot of parsing)
void CSupportedParameters::ParseNOTIFYICONDATAW(PVOID pData,OUT TCHAR* StringRepresentation)
{
#if (!defined(UNICODE)&&!defined(_UNICODE))
    TCHAR        szTip[64];
    WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)((NOTIFYICONDATAW*)pData)->szTip, 64-1, szTip,(int)64-1, NULL, NULL);
    szTip[64-1]=0;
#endif
    _stprintf(StringRepresentation,
                _T("{cbSize=%u,hWnd=0x%p,uID=0x%X,")
                _T("uFlags=0x%X,uCallbackMessage=0x%X,hIcon=0x%p,szTip=%s}"),
                ((NOTIFYICONDATAW*)pData)->cbSize,
                ((NOTIFYICONDATAW*)pData)->hWnd,
                ((NOTIFYICONDATAW*)pData)->uID,
                ((NOTIFYICONDATAW*)pData)->uFlags,
                ((NOTIFYICONDATAW*)pData)->uCallbackMessage,
                ((NOTIFYICONDATAW*)pData)->hIcon,
#if (!defined(UNICODE)&&!defined(_UNICODE))
                szTip
#else
                ((NOTIFYICONDATAW*)pData)->szTip
#endif
                );
}

void CSupportedParameters::ParseDCB(PVOID pData,OUT TCHAR* StringRepresentation)
{
    TCHAR RtsCtrl[10];
    TCHAR DtrCtrl[10];
    TCHAR Parity[10];
    TCHAR StopBits[10];
    switch(((DCB*)pData)->fDtrControl)
    {
    case DTR_CONTROL_DISABLE:
        _tcscpy(DtrCtrl,_T("Disable"));
        break;
    case DTR_CONTROL_ENABLE:
        _tcscpy(DtrCtrl,_T("Enable"));
        break;
    case DTR_CONTROL_HANDSHAKE:
        _tcscpy(DtrCtrl,_T("Handshake"));
        break;
    default:
        _itot(((DCB*)pData)->fDtrControl,DtrCtrl,10);
        break;
    }
    switch(((DCB*)pData)->fRtsControl)
    {
    case RTS_CONTROL_DISABLE:
        _tcscpy(RtsCtrl,_T("Disable"));
        break;
    case RTS_CONTROL_ENABLE:
        _tcscpy(RtsCtrl,_T("Enable"));
        break;
    case RTS_CONTROL_HANDSHAKE:
        _tcscpy(RtsCtrl,_T("Handshake"));
        break;
    case RTS_CONTROL_TOGGLE:
        _tcscpy(RtsCtrl,_T("Toggle"));
        break;
    default:
        _itot(((DCB*)pData)->fRtsControl,RtsCtrl,10);
        break;
    }
    switch(((DCB*)pData)->Parity)
    {
    case NOPARITY:
        _tcscpy(Parity,_T("No parity"));
        break;
    case EVENPARITY:
        _tcscpy(Parity,_T("Even"));
        break;
    case MARKPARITY:
        _tcscpy(Parity,_T("Mark"));
        break;
    case ODDPARITY:
        _tcscpy(Parity,_T("Odd"));
        break;
    case SPACEPARITY:
        _tcscpy(Parity,_T("Space"));
        break;
    default:
        _itot(((DCB*)pData)->Parity,Parity,10);
        break;
    }

    switch(((DCB*)pData)->StopBits)
    {
    case ONESTOPBIT:
        _tcscpy(StopBits,_T("1"));
        break;
    case ONE5STOPBITS:
        _tcscpy(StopBits,_T("1.5"));
        break;
    case TWOSTOPBITS:
        _tcscpy(StopBits,_T("2"));
        break;
    default:
        _itot(((DCB*)pData)->StopBits,StopBits,10);
        break;
    }
    _stprintf(StringRepresentation,
                _T("{DCBlength=%u") 
                _T(",BaudRate=%u") 
                _T(",fBinary=%u") 
                _T(",fParity=%u") 
                _T(",fOutxCtsFlow=%u") 
                _T(",fOutxDsrFlow=%u") 
                _T(",fDtrControl=%s,") 
                _T("fDsrSensitivity=%u") 
                _T(",fTXContinueOnXoff=%u") 
                _T(",fOutX=%u") 
                _T(",fInX=%u") 
                _T(",fErrorChar=%u") 
                _T(",fNull=%u") 
                _T(",fRtsControl=%s,")
                _T("fAbortOnError=%u") 
                _T(",fDummy2=0x%X") 
                _T(",wReserved=0x%X") 
                _T(",XonLim=%u") 
                _T(",XoffLim=%u") 
                _T(",ByteSize=%u") 
                _T(",Parity=%s,")
                _T("StopBits=%s,XonChar=0x%.2X,XoffChar=0x%.2X,ErrorChar=0x%.2X,EofChar=0x%.2X,EvtChar=0x%.2X,wReserved1=0x%X}"),
                ((DCB*)pData)->DCBlength,
                ((DCB*)pData)->BaudRate,
                ((DCB*)pData)->fBinary,
                ((DCB*)pData)->fParity,
                ((DCB*)pData)->fOutxCtsFlow,
                ((DCB*)pData)->fOutxDsrFlow,
                DtrCtrl,
                ((DCB*)pData)->fDsrSensitivity,
                ((DCB*)pData)->fTXContinueOnXoff,
                ((DCB*)pData)->fOutX,
                ((DCB*)pData)->fInX,
                ((DCB*)pData)->fErrorChar,
                ((DCB*)pData)->fNull,
                RtsCtrl,
                ((DCB*)pData)->fAbortOnError,
                ((DCB*)pData)->fDummy2,
                ((DCB*)pData)->wReserved,
                ((DCB*)pData)->XonLim,
                ((DCB*)pData)->XoffLim,
                ((DCB*)pData)->ByteSize,
                Parity,
                StopBits,
                ((DCB*)pData)->XonChar,
                ((DCB*)pData)->XoffChar,
                ((DCB*)pData)->ErrorChar,
                ((DCB*)pData)->EofChar,
                ((DCB*)pData)->EvtChar,
                ((DCB*)pData)->wReserved1
                );
}

void CSupportedParameters::ParseCOMMTIMEOUTS(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
                _T("{ReadIntervalTimeout=%u,ReadTotalTimeoutMultiplier=%u,ReadTotalTimeoutConstant=%u,")
                _T("WriteTotalTimeoutMultiplier=%u,WriteTotalTimeoutConstant=%u}"),
                ((COMMTIMEOUTS*)pData)->ReadIntervalTimeout,
                ((COMMTIMEOUTS*)pData)->ReadTotalTimeoutMultiplier,
                ((COMMTIMEOUTS*)pData)->ReadTotalTimeoutConstant,
                ((COMMTIMEOUTS*)pData)->WriteTotalTimeoutMultiplier,
                ((COMMTIMEOUTS*)pData)->WriteTotalTimeoutConstant
                );
}

void CSupportedParameters::ParseCOMMCONFIG(PVOID pData,OUT TCHAR* StringRepresentation)
{

    TCHAR* StrDCB;

    PARAMETER_LOG_INFOS Log;
    *Log.pszParameterName=0;
    Log.dwSizeOfData=sizeof(DCB);
    Log.dwSizeOfPointedValue=0;
    Log.dwType=PARAM_PDCB;
    Log.Value=0;
    Log.pbValue=(PBYTE)&((COMMCONFIG*)pData)->dcb;

    CSupportedParameters::GenericParameterParsing(      &Log,
                                                        sizeof(DCB),
                                                        FALSE,
                                                        CSupportedParameters::ParseDCB,
                                                        DCB_STRING_REPRESENTATION_MAX_SIZE,
                                                        &StrDCB,_T(","),FALSE,FALSE,TRUE,0);


    _stprintf(StringRepresentation,
                _T("{dwSize=%u,wVersion=%u,wReserved=0x%X,DCB=%s,")
                _T("dwProviderSubType=0x%X,dwProviderOffset=0x%X,dwProviderSize=0x%X,wcProviderData=0x%p}"),
                ((COMMCONFIG*)pData)->dwSize,
                ((COMMCONFIG*)pData)->wVersion,
                ((COMMCONFIG*)pData)->wReserved,
                StrDCB,
                ((COMMCONFIG*)pData)->dwProviderSubType,
                ((COMMCONFIG*)pData)->dwProviderOffset,
                ((COMMCONFIG*)pData)->dwProviderSize,
                ((COMMCONFIG*)pData)->wcProviderData
                );

    delete StrDCB;
}
void CSupportedParameters::ParseSTARTUPINFO(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
                _T("{cb=0x%X,lpReserved=0x%p,lpDesktop=0x%p,lpTitle=0x%p,dwX=0x%X,dwY=0x%X,dwXSize=0x%X,")
                _T("dwYSize=0x%X,dwXCountChars=0x%X,dwYCountChars=0x%X,dwFillAttribute=0x%X,dwFlags=0x%X,wShowWindow=0x%X,cbReserved2=0x%X,")
                _T("lpReserved2=0x%p,hStdInput=0x%p,hStdOutput=0x%p,hStdError=0x%p}"),
                ((STARTUPINFO*)pData)->cb,
                ((STARTUPINFO*)pData)->lpReserved,
                ((STARTUPINFO*)pData)->lpDesktop,
                ((STARTUPINFO*)pData)->lpTitle,
                ((STARTUPINFO*)pData)->dwX,
                ((STARTUPINFO*)pData)->dwY,
                ((STARTUPINFO*)pData)->dwXSize,
                ((STARTUPINFO*)pData)->dwYSize,
                ((STARTUPINFO*)pData)->dwXCountChars,
                ((STARTUPINFO*)pData)->dwYCountChars,
                ((STARTUPINFO*)pData)->dwFillAttribute,
                ((STARTUPINFO*)pData)->dwFlags,
                ((STARTUPINFO*)pData)->wShowWindow,
                ((STARTUPINFO*)pData)->cbReserved2,
                ((STARTUPINFO*)pData)->lpReserved2,
                ((STARTUPINFO*)pData)->hStdInput,
                ((STARTUPINFO*)pData)->hStdOutput,
                ((STARTUPINFO*)pData)->hStdError
                );
}
void CSupportedParameters::ParseSHELLEXECUTEINFO(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
                _T("{cbSize=0x%X,fMask=0x%X,hwnd=0x%p,lpVerb=0x%p,lpFile=0x%p,lpParameters=0x%p,lpDirectory=0x%p,")
                _T("nShow=0x%X,hInstApp=0x%p,lpIDList=0x%p,lpClass=0x%p,hkeyClass=0x%p,dwHotKey=0x%X,hIcon/hMonitor=0x%p,")
                _T("hProcess=0x%p}"),
                ((SHELLEXECUTEINFO*)pData)->cbSize,
                ((SHELLEXECUTEINFO*)pData)->fMask,
                ((SHELLEXECUTEINFO*)pData)->hwnd,
                ((SHELLEXECUTEINFO*)pData)->lpVerb,
                ((SHELLEXECUTEINFO*)pData)->lpFile,
                ((SHELLEXECUTEINFO*)pData)->lpParameters,
                ((SHELLEXECUTEINFO*)pData)->lpDirectory,
                ((SHELLEXECUTEINFO*)pData)->nShow,
                ((SHELLEXECUTEINFO*)pData)->hInstApp,
                ((SHELLEXECUTEINFO*)pData)->lpIDList,
                ((SHELLEXECUTEINFO*)pData)->lpClass,
                ((SHELLEXECUTEINFO*)pData)->hkeyClass,
                ((SHELLEXECUTEINFO*)pData)->dwHotKey,
                ((SHELLEXECUTEINFO*)pData)->hIcon,
                ((SHELLEXECUTEINFO*)pData)->hProcess
                );
}
void CSupportedParameters::ParseLARGE_INTEGER(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
                _T("{0x%X,0x%X}"),
                ((LARGE_INTEGER*)pData)->HighPart,
                ((LARGE_INTEGER*)pData)->LowPart);
}

void CSupportedParameters::ParsePOINT(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
                _T("{x=%d,y=%d}"),
                ((POINT*)pData)->x,
                ((POINT*)pData)->y);
}

void CSupportedParameters::ParseLOGFONTA(PVOID pData,OUT TCHAR* StringRepresentation)
{
    TCHAR sz[LF_FACESIZE];
#if (defined(UNICODE)||defined(_UNICODE))
    CAnsiUnicodeConvert::AnsiToUnicode(((LOGFONTA*)pData)->lfFaceName,sz,LF_FACESIZE);
#else
    _tcsncpy(sz,((LOGFONTA*)pData)->lfFaceName,LF_FACESIZE);
#endif

    // assume lfFaceName is \0 ended
    sz[LF_FACESIZE-1]=0;

    _stprintf(StringRepresentation,
              _T("{lfHeight=%d,lfWidth=%d,lfEscapement=%d,lfOrientation=%d,lfWeight=%d,lfItalic=%d,")
              _T("lfUnderline=%d,lfStrikeOut=%d,lfCharSet=%d,lfOutPrecision=%d,lfQuality=%d,lfPitchAndFamily=%d,lfFaceName=%s}"),
              ((LOGFONTA*)pData)->lfHeight,
              ((LOGFONTA*)pData)->lfWidth,
              ((LOGFONTA*)pData)->lfEscapement,
              ((LOGFONTA*)pData)->lfOrientation,
              ((LOGFONTA*)pData)->lfWeight,
              ((LOGFONTA*)pData)->lfItalic,
              ((LOGFONTA*)pData)->lfUnderline,
              ((LOGFONTA*)pData)->lfStrikeOut,
              ((LOGFONTA*)pData)->lfCharSet,
              ((LOGFONTA*)pData)->lfOutPrecision,
              ((LOGFONTA*)pData)->lfQuality,
              ((LOGFONTA*)pData)->lfPitchAndFamily,
              sz
              );
}

void CSupportedParameters::ParseLOGFONTW(PVOID pData,OUT TCHAR* StringRepresentation)
{
    TCHAR sz[LF_FACESIZE];
#if (defined(UNICODE)||defined(_UNICODE))
    _tcsncpy(sz,((LOGFONTW*)pData)->lfFaceName,LF_FACESIZE);
#else
    CAnsiUnicodeConvert::UnicodeToAnsi(((LOGFONTW*)pData)->lfFaceName,sz,LF_FACESIZE);    
#endif
    // assume lfFaceName is \0 ended
    sz[LF_FACESIZE-1]=0;

    _stprintf(StringRepresentation,
        _T("{lfHeight=%d,lfWidth=%d,lfEscapement=%d,lfOrientation=%d,lfWeight=%d,lfItalic=%d,")
        _T("lfUnderline=%d,lfStrikeOut=%d,lfCharSet=%d,lfOutPrecision=%d,lfQuality=%d,lfPitchAndFamily=%d,lfFaceName=%s}"),
        ((LOGFONTW*)pData)->lfHeight,
        ((LOGFONTW*)pData)->lfWidth,
        ((LOGFONTW*)pData)->lfEscapement,
        ((LOGFONTW*)pData)->lfOrientation,
        ((LOGFONTW*)pData)->lfWeight,
        ((LOGFONTW*)pData)->lfItalic,
        ((LOGFONTW*)pData)->lfUnderline,
        ((LOGFONTW*)pData)->lfStrikeOut,
        ((LOGFONTW*)pData)->lfCharSet,
        ((LOGFONTW*)pData)->lfOutPrecision,
        ((LOGFONTW*)pData)->lfQuality,
        ((LOGFONTW*)pData)->lfPitchAndFamily,
        sz
        );
}
void CSupportedParameters::ParseSIZE(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
                _T("{cx=%d,cy=%d}"),
                ((SIZE*)pData)->cx,
                ((SIZE*)pData)->cy);
}

void CSupportedParameters::ParseRECT(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
                _T("{left=%d,right=%d,top=%d,bottom=%d}"),
                ((RECT*)pData)->left,
                ((RECT*)pData)->right,
                ((RECT*)pData)->top,
                ((RECT*)pData)->bottom
                );
}

void CSupportedParameters::ParseMSG(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
                _T("{hwnd=0x%p") 
                _T(",message=0x%X") 
                _T(",wParam=0x%X") 
                _T(",lParam=0x%X") 
                _T(",time=0x%X") 
                _T(",point{x=%d,y=%d}}"),
                (UINT_PTR)((MSG*)pData)->hwnd,
                ((MSG*)pData)->message,
                ((MSG*)pData)->wParam,
                ((MSG*)pData)->lParam,
                ((MSG*)pData)->time,
                ((MSG*)pData)->pt.x,
                ((MSG*)pData)->pt.y
                );
}

void CSupportedParameters::ParseTIMEVAL(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
                _T("{sec=%d,usec=%d}"),
                ((TIMEVAL*)pData)->tv_sec,
                ((TIMEVAL*)pData)->tv_usec);
}

void CSupportedParameters::ParseHOSTENT(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{h_name=0x%p,h_aliases=0x%p,h_addrtype=0x%X,h_length=0x%X,h_addr_list=0x%p}"),
            ((hostent*)pData)->h_name,
            ((hostent*)pData)->h_aliases,
            ((hostent*)pData)->h_addrtype,
            ((hostent*)pData)->h_length,
            ((hostent*)pData)->h_addr_list);
}
void CSupportedParameters::ParseSOCKADDR(PVOID pData,OUT TCHAR* StringRepresentation)
{
    TCHAR* psz;
    PARAMETER_LOG_INFOS Log;
    *Log.pszParameterName=0;
    Log.dwSizeOfData=0;
    Log.dwSizeOfPointedValue=14*sizeof(BYTE);
    Log.dwType=PARAM_PBYTE;
    Log.Value=(PBYTE)((sockaddr*)pData)->sa_data;
    Log.pbValue=(PBYTE)((sockaddr*)pData)->sa_data;

    CSupportedParameters::GenericParameterParsing(
                                                        &Log,
                                                        sizeof(BYTE),
                                                        TRUE,
                                                        CSupportedParameters::ParseBYTE,
                                                        BYTE_STRING_REPRESENTATION_MAX_SIZE ,
                                                        &psz,
                                                        _T(" "),FALSE,FALSE,FALSE,0);
    _stprintf(StringRepresentation,
            _T("{sa_family=0x%X,sa_data=%s}"),
            ((sockaddr*)pData)->sa_family,
            psz);
    delete psz;
}
void CSupportedParameters::ParseSOCKADDR_IN(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{sin_family=0x%X,sin_port=%u,sin_addr=0x%X}"),
            ((sockaddr_in*)pData)->sin_family,
            ((sockaddr_in*)pData)->sin_port,
            ((sockaddr_in*)pData)->sin_addr.S_un.S_addr
            );
}
void CSupportedParameters::ParseGUID(PVOID pData,OUT TCHAR* StringRepresentation)
{
    TCHAR* psz;
    TCHAR* psz2;
    PARAMETER_LOG_INFOS Log;
    *Log.pszParameterName=0;
    Log.dwSizeOfData=0;
    Log.dwSizeOfPointedValue=2*sizeof(BYTE);
    Log.dwType=PARAM_PBYTE;
    Log.Value=(PBYTE)(((GUID*)pData)->Data4);
    Log.pbValue=(PBYTE)(((GUID*)pData)->Data4);

    CSupportedParameters::GenericParameterParsing(
                                                        &Log,
                                                        sizeof(BYTE),
                                                        TRUE,
                                                        CSupportedParameters::ParseBYTE,
                                                        BYTE_STRING_REPRESENTATION_MAX_SIZE ,
                                                        &psz,_T("")
                                                        ,FALSE,FALSE,FALSE,0);

    Log.dwSizeOfData=0;
    Log.dwSizeOfPointedValue=6*sizeof(BYTE);
    Log.dwType=PARAM_PBYTE;
    Log.Value=(PBYTE)(&(((GUID*)pData)->Data4[2]));
    Log.pbValue=(PBYTE)(&(((GUID*)pData)->Data4[2]));

    CSupportedParameters::GenericParameterParsing(
                                                        &Log,
                                                        sizeof(BYTE),
                                                        TRUE,
                                                        CSupportedParameters::ParseBYTE,
                                                        BYTE_STRING_REPRESENTATION_MAX_SIZE ,
                                                        &psz2,_T("")
                                                        ,FALSE,FALSE,FALSE,0);
    _stprintf(StringRepresentation,
             _T("%.8X-%.4X-%.4X-%s-%s"),
            ((GUID*)pData)->Data1,
            ((GUID*)pData)->Data2,
            ((GUID*)pData)->Data3,
            psz,
            psz2);
    delete psz;
    delete psz2;
}

void CSupportedParameters::ParseSECURITY_ATTRIBUTES(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{nLength=%u,lpSecurityDescriptor=0x%p,bInheritHandle=0x%X}"),
            ((SECURITY_ATTRIBUTES*)pData)->nLength,
            ((SECURITY_ATTRIBUTES*)pData)->lpSecurityDescriptor,
            ((SECURITY_ATTRIBUTES*)pData)->bInheritHandle
            );
}
void CSupportedParameters::ParseACL(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{AclRevision=%u,Sbz1=0x%X,AclSize=%u,AceCount=%u,Sbz2=0x%X}"),
            ((ACL*)pData)->AclRevision,
            ((ACL*)pData)->Sbz1,
            ((ACL*)pData)->AclSize,
            ((ACL*)pData)->AceCount,
            ((ACL*)pData)->Sbz2
            );
}
void CSupportedParameters::ParseMEMORY_BASIC_INFORMATION(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{BaseAddress=0x%p,AllocationBase=0x%p,AllocationProtect=0x%X,RegionSize=%u,State=0x%X,Protect=0x%X,Type=0x%X}"),
            ((MEMORY_BASIC_INFORMATION*)pData)->BaseAddress,
            ((MEMORY_BASIC_INFORMATION*)pData)->AllocationBase,
            ((MEMORY_BASIC_INFORMATION*)pData)->AllocationProtect,
            ((MEMORY_BASIC_INFORMATION*)pData)->RegionSize,
            ((MEMORY_BASIC_INFORMATION*)pData)->State,
            ((MEMORY_BASIC_INFORMATION*)pData)->Protect,
            ((MEMORY_BASIC_INFORMATION*)pData)->Type);
}
void CSupportedParameters::ParseCRITICAL_SECTION(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{DebugInfo=0x%p,LockCount=%u,RecursionCount=%u,OwningThread=0x%p,LockSemaphore=0x%p,SpinCount=%u}"),
            ((CRITICAL_SECTION*)pData)->DebugInfo,
            ((CRITICAL_SECTION*)pData)->LockCount,
            ((CRITICAL_SECTION*)pData)->RecursionCount,
            ((CRITICAL_SECTION*)pData)->OwningThread,
            ((CRITICAL_SECTION*)pData)->LockSemaphore,
            ((CRITICAL_SECTION*)pData)->SpinCount
            );
}
void CSupportedParameters::ParseHEAPENTRY32(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{dwSize=%u,hHandle=0x%p,dwAddress=0x%X,dwBlockSize=0x%X,dwFlags=0x%X,dwLockCount=%u,")
                _T("dwResvd=0x%X,th32ProcessID=0x%X,th32HeapID=0x%X}"),
                ((HEAPENTRY32*)pData)->dwSize,
                ((HEAPENTRY32*)pData)->hHandle,
                ((HEAPENTRY32*)pData)->dwAddress,
                ((HEAPENTRY32*)pData)->dwBlockSize,
                ((HEAPENTRY32*)pData)->dwFlags,
                ((HEAPENTRY32*)pData)->dwLockCount,
                ((HEAPENTRY32*)pData)->dwResvd,
                ((HEAPENTRY32*)pData)->th32ProcessID,
                ((HEAPENTRY32*)pData)->th32HeapID
                );
}
void CSupportedParameters::ParseTHREADENTRY32(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{dwSize=%u,cntUsage=%u,th32ThreadID=0x%X,th32OwnerProcessID=0x%X,tpBasePri=%d,dwLockCount=%d,dwFlags=0x%X}"),
                ((THREADENTRY32*)pData)->dwSize,
                ((THREADENTRY32*)pData)->cntUsage,
                ((THREADENTRY32*)pData)->th32ThreadID,
                ((THREADENTRY32*)pData)->th32OwnerProcessID,
                ((THREADENTRY32*)pData)->tpBasePri,
                ((THREADENTRY32*)pData)->tpDeltaPri,
                ((THREADENTRY32*)pData)->dwFlags
                );
}
void CSupportedParameters::ParsePROCESS_HEAP_ENTRY(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{lpData=0x%p,cbData=%u,cbOverhead=%u,iRegionIndex=%u,wFlags=0x%X,")
                _T("hMem/dwCommittedSize=0x%X,dwUnCommittedSize=0x%X,lpFirstBlock=0x%p,lpLastBlock=0x%p}"),
                ((PROCESS_HEAP_ENTRY*)pData)->lpData,
                ((PROCESS_HEAP_ENTRY*)pData)->cbData,
                ((PROCESS_HEAP_ENTRY*)pData)->cbOverhead,
                ((PROCESS_HEAP_ENTRY*)pData)->iRegionIndex,
                ((PROCESS_HEAP_ENTRY*)pData)->wFlags,
                ((PROCESS_HEAP_ENTRY*)pData)->Region.dwCommittedSize,
                ((PROCESS_HEAP_ENTRY*)pData)->Region.dwUnCommittedSize,
                ((PROCESS_HEAP_ENTRY*)pData)->Region.lpFirstBlock,
                ((PROCESS_HEAP_ENTRY*)pData)->Region.lpLastBlock
                );
}
TCHAR DaysArray[7][10]={_T("Sunday"),_T("Monday"),_T("Tuesday"),_T("Wednesday"),_T("Thursday"),_T("Friday"),_T("Saturday")};
void CSupportedParameters::ParseSYSTEMTIME(PVOID pData,OUT TCHAR* StringRepresentation)
{
    TCHAR* pszDay=DaysArray[(((SYSTEMTIME*)pData)->wDayOfWeek)&0x6];

    _stprintf(StringRepresentation,
            _T("{%s %u/%u/%u %u:%.2u:%.2u:%.3u}"),
            pszDay,
            ((SYSTEMTIME*)pData)->wMonth,
            ((SYSTEMTIME*)pData)->wDay,
            ((SYSTEMTIME*)pData)->wYear,
            ((SYSTEMTIME*)pData)->wHour,
            ((SYSTEMTIME*)pData)->wMinute,
            ((SYSTEMTIME*)pData)->wSecond,
            ((SYSTEMTIME*)pData)->wMilliseconds
            );
}
void CSupportedParameters::ParseFILETIME(PVOID pData,OUT TCHAR* StringRepresentation)
{
    SYSTEMTIME St;
    if (FileTimeToSystemTime((FILETIME*)pData,&St))
        ParseSYSTEMTIME(&St,StringRepresentation);
    else
        _stprintf(StringRepresentation,
            _T("{dwHighDateTime=0x%X,dwLowDateTime=0x%X}"),
            ((FILETIME*)pData)->dwHighDateTime,
            ((FILETIME*)pData)->dwLowDateTime
            );
}
void CSupportedParameters::ParseSECHANDLE(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{dwUpper=0x%X,dwLower=0x%X}"),
                ((SecHandle*)pData)->dwUpper,
                ((SecHandle*)pData)->dwLower
                );
}
void CSupportedParameters::ParseDLGTEMPLATE(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{style=0x%X,dwExtendedStyle=0x%X,cdit=%u,x=%d,y=%d,cx=%d,cy=%d}"),
                ((DLGTEMPLATE*)pData)->style,
                ((DLGTEMPLATE*)pData)->dwExtendedStyle,
                ((DLGTEMPLATE*)pData)->cdit,
                ((DLGTEMPLATE*)pData)->x,
                ((DLGTEMPLATE*)pData)->y,
                ((DLGTEMPLATE*)pData)->cx,
                ((DLGTEMPLATE*)pData)->cy
                );
}
void CSupportedParameters::ParseWNDCLASS(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{style=0x%X,lpfnWndProc=0x%p,cbClsExtra=0x%X,cbWndExtra=0x%X,hInstance=0x%p,")
            _T("hIcon=0x%p,hCursor=0x%p,hbrBackground=0x%p,lpszMenuName=0x%p,lpszClassName=0x%p}"),
            ((WNDCLASS*)pData)->style,
            ((WNDCLASS*)pData)->lpfnWndProc,
            ((WNDCLASS*)pData)->cbClsExtra,
            ((WNDCLASS*)pData)->cbWndExtra,
            ((WNDCLASS*)pData)->hInstance,
            ((WNDCLASS*)pData)->hIcon,
            ((WNDCLASS*)pData)->hCursor,
            ((WNDCLASS*)pData)->hbrBackground,
            ((WNDCLASS*)pData)->lpszMenuName,
            ((WNDCLASS*)pData)->lpszClassName
            );
}
void CSupportedParameters::ParseWNDCLASSEX(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{cbSize=%u,style=0x%X,lpfnWndProc=0x%p,cbClsExtra=0x%X,cbWndExtra=0x%X,hInstance=0x%p,")
            _T("hIcon=0x%p,hCursor=0x%p,hbrBackground=0x%p,lpszMenuName=0x%p,lpszClassName=0x%p,hIconSm=0x%p}"),
            ((WNDCLASSEX*)pData)->cbSize,
            ((WNDCLASSEX*)pData)->style,
            ((WNDCLASSEX*)pData)->lpfnWndProc,
            ((WNDCLASSEX*)pData)->cbClsExtra,
            ((WNDCLASSEX*)pData)->cbWndExtra,
            ((WNDCLASSEX*)pData)->hInstance,
            ((WNDCLASSEX*)pData)->hIcon,
            ((WNDCLASSEX*)pData)->hCursor,
            ((WNDCLASSEX*)pData)->hbrBackground,
            ((WNDCLASSEX*)pData)->lpszMenuName,
            ((WNDCLASSEX*)pData)->lpszClassName,
            ((WNDCLASSEX*)pData)->hIconSm
            );
}

void CSupportedParameters::ParseOVERLAPPED(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{Internal=0x%p") 
            _T(",InternalHigh=0x%p") 
            _T(",Offset=0x%X,OffsetHigh=0x%X,hEvent=0x%p}"),
            ((OVERLAPPED*)pData)->Internal,
            ((OVERLAPPED*)pData)->InternalHigh,
            ((OVERLAPPED*)pData)->Offset,
            ((OVERLAPPED*)pData)->OffsetHigh,
            ((OVERLAPPED*)pData)->hEvent
            );
}

void CSupportedParameters::ParseWSABUF(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{len=0x%u,buf=0x%p}"),
            ((WSABUF*)pData)->len,
            ((WSABUF*)pData)->buf
            );
}


void CSupportedParameters::ParseADDRINFO(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{ai_flags=0x%X,ai_family=0x%X,ai_socktype=0x%X,ai_protocol=0x%X,")
            _T("ai_addrlen=%u,ai_canonname=0x%p,ai_addr=0x%p,ai_next=0x%p}"),
            ((ADDRINFO*)pData)->ai_flags,
            ((ADDRINFO*)pData)->ai_family,
            ((ADDRINFO*)pData)->ai_socktype,
            ((ADDRINFO*)pData)->ai_protocol,
            ((ADDRINFO*)pData)->ai_addrlen,
            ((ADDRINFO*)pData)->ai_canonname,
            ((ADDRINFO*)pData)->ai_addr,
            ((ADDRINFO*)pData)->ai_next
            );
}

void CSupportedParameters::ParseWSADATA(PVOID pData,OUT TCHAR* StringRepresentation)
{
#if (defined(UNICODE)||defined(_UNICODE))
    TCHAR szDescription[WSADESCRIPTION_LEN+1];
    TCHAR szSystemStatus[WSASYS_STATUS_LEN+1];

    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)((WSADATA*)pData)->szDescription, WSADESCRIPTION_LEN,szDescription,(int)WSADESCRIPTION_LEN);
    szDescription[WSADESCRIPTION_LEN]=0;
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)((WSADATA*)pData)->szSystemStatus, WSASYS_STATUS_LEN,szSystemStatus,(int)WSASYS_STATUS_LEN);
    szSystemStatus[WSASYS_STATUS_LEN]=0;
#endif
    _stprintf(StringRepresentation,
            _T("{wHighVersion=0x%X,wVersion=0x%X,iMaxSockets=%u,iMaxUdpDg=%u,")
            _T("lpVendorInfo=0x%p,szDescription=0x%p,szSystemStatus=0x%p}"),
            ((WSADATA*)pData)->wHighVersion,
            ((WSADATA*)pData)->wVersion,
            ((WSADATA*)pData)->iMaxSockets,
            ((WSADATA*)pData)->iMaxUdpDg,
            ((WSADATA*)pData)->lpVendorInfo,
#if (defined(UNICODE)||defined(_UNICODE))
            szDescription,
            szSystemStatus
#else
            ((WSADATA*)pData)->szDescription,
            ((WSADATA*)pData)->szSystemStatus
#endif
            );
}
void CSupportedParameters::ParseWSAPROTOCOL_INFOA(PVOID pData,OUT TCHAR* StringRepresentation)
{
    DWORD ChainLen=((WSAPROTOCOL_INFOA*)pData)->ProtocolChain.ChainLen;
    TCHAR* pszGUID;

    PARAMETER_LOG_INFOS Log;
    *Log.pszParameterName=0;
    Log.dwSizeOfData=sizeof(GUID);
    Log.dwSizeOfPointedValue=0;
    Log.dwType=PARAM_GUID;
    Log.Value=0;
    Log.pbValue=(PBYTE)&((WSAPROTOCOL_INFOA*)pData)->ProviderId;

    CSupportedParameters::GenericParameterParsing(
                                                        &Log,
                                                        sizeof(GUID),
                                                        FALSE,
                                                        CSupportedParameters::ParseGUID,
                                                        GUID_STRING_REPRESENTATION_MAX_SIZE,
                                                        &pszGUID,_T(","),FALSE,FALSE,FALSE,0);

    TCHAR pszChainEntries[80];
    *pszChainEntries=0;

    if (ChainLen>0)
    {
        TCHAR* psz;

        Log.dwSizeOfData=0;
        Log.dwSizeOfPointedValue=ChainLen*sizeof(DWORD);
        Log.dwType=PARAM_PDWORD;
        Log.Value=(PBYTE)&((WSAPROTOCOL_INFOA*)pData)->ProtocolChain.ChainEntries;
        Log.pbValue=(PBYTE)((WSAPROTOCOL_INFOA*)pData)->ProtocolChain.ChainEntries;
        
        CSupportedParameters::GenericParameterParsing(&Log,
                                                        sizeof(DWORD),
                                                        TRUE,
                                                        CSupportedParameters::ParseDWORD,
                                                        DWORD_STRING_REPRESENTATION_MAX_SIZE,
                                                        &psz,_T(","),FALSE,FALSE,FALSE,0);

        _tcscpy(pszChainEntries,_T(",ChainEntries="));
        _tcscat(pszChainEntries,psz);
        delete psz;
    }

#if (defined(UNICODE)||defined(_UNICODE))
    TCHAR szProtocol[WSAPROTOCOL_LEN+1];
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)((WSAPROTOCOL_INFOA*)pData)->szProtocol, WSAPROTOCOL_LEN,szProtocol,(int)WSAPROTOCOL_LEN);
    szProtocol[WSAPROTOCOL_LEN]=0;
#endif

    _stprintf(StringRepresentation,
            _T("{dwServiceFlags1=0x%X,dwServiceFlags2=0x%X,dwServiceFlags3=0x%X,dwServiceFlags4=0x%X,")
            _T("dwProviderFlags=0x%X,ProviderId=%s,dwCatalogEntryId=0x%X,ProtocolChain={ChainLen=%u%s},")
            _T("iVersion=0x%X,iAddressFamily=0x%X,iMaxSockAddr=0x%X,iMinSockAddr=0x%X,")
            _T("iSocketType=0x%X,iProtocol=0x%X,iProtocolMaxOffset=0x%X,iNetworkByteOrder=0x%X,")
            _T("iSecurityScheme=0x%X,dwMessageSize=0x%X,dwProviderReserved=0x%X,szProtocol=%s}"),
            ((WSAPROTOCOL_INFOA*)pData)->dwServiceFlags1,
            ((WSAPROTOCOL_INFOA*)pData)->dwServiceFlags2,
            ((WSAPROTOCOL_INFOA*)pData)->dwServiceFlags3,
            ((WSAPROTOCOL_INFOA*)pData)->dwServiceFlags4,
            ((WSAPROTOCOL_INFOA*)pData)->dwProviderFlags,
            pszGUID,
            ((WSAPROTOCOL_INFOA*)pData)->dwCatalogEntryId,
            ChainLen,
            pszChainEntries,
            ((WSAPROTOCOL_INFOA*)pData)->iVersion,
            ((WSAPROTOCOL_INFOA*)pData)->iAddressFamily,
            ((WSAPROTOCOL_INFOA*)pData)->iMaxSockAddr,
            ((WSAPROTOCOL_INFOA*)pData)->iMinSockAddr,
            ((WSAPROTOCOL_INFOA*)pData)->iSocketType,
            ((WSAPROTOCOL_INFOA*)pData)->iProtocol,
            ((WSAPROTOCOL_INFOA*)pData)->iProtocolMaxOffset,
            ((WSAPROTOCOL_INFOA*)pData)->iNetworkByteOrder,
            ((WSAPROTOCOL_INFOA*)pData)->iSecurityScheme,
            ((WSAPROTOCOL_INFOA*)pData)->dwMessageSize,
            ((WSAPROTOCOL_INFOA*)pData)->dwProviderReserved,
#if (defined(UNICODE)||defined(_UNICODE))
            szProtocol
#else
            ((WSAPROTOCOL_INFOA*)pData)->szProtocol
#endif
            );

    delete pszGUID;
}
void CSupportedParameters::ParseWSAPROTOCOL_INFOW(PVOID pData,OUT TCHAR* StringRepresentation)
{
    DWORD ChainLen=((WSAPROTOCOL_INFOW*)pData)->ProtocolChain.ChainLen;
    TCHAR* pszGUID;

    PARAMETER_LOG_INFOS Log;
    *Log.pszParameterName=0;
    Log.dwSizeOfData=sizeof(GUID);
    Log.dwSizeOfPointedValue=0;
    Log.dwType=PARAM_GUID;
    Log.Value=0;
    Log.pbValue=(PBYTE)&((WSAPROTOCOL_INFOW*)pData)->ProviderId;

    CSupportedParameters::GenericParameterParsing(
                                                        &Log,
                                                        sizeof(GUID),
                                                        FALSE,
                                                        CSupportedParameters::ParseGUID,
                                                        GUID_STRING_REPRESENTATION_MAX_SIZE,
                                                        &pszGUID,_T(","),FALSE,FALSE,FALSE,0);

    TCHAR pszChainEntries[80];
    *pszChainEntries=0;

    if (ChainLen>0)
    {
        TCHAR* psz;

        Log.dwSizeOfData=0;
        Log.dwSizeOfPointedValue=ChainLen*sizeof(DWORD);
        Log.dwType=PARAM_PDWORD;
        Log.Value=(PBYTE)&((WSAPROTOCOL_INFOW*)pData)->ProtocolChain.ChainEntries;
        Log.pbValue=(PBYTE)((WSAPROTOCOL_INFOW*)pData)->ProtocolChain.ChainEntries;

        CSupportedParameters::GenericParameterParsing(&Log,
            sizeof(DWORD),
            TRUE,
            CSupportedParameters::ParseDWORD,
            DWORD_STRING_REPRESENTATION_MAX_SIZE,
            &psz,_T(","),FALSE,FALSE,TRUE,0);

        _tcscpy(pszChainEntries,_T(",ChainEntries="));
        _tcscat(pszChainEntries,psz);
        delete psz;
    }

#if (!defined(UNICODE)&&!defined(_UNICODE))
    TCHAR szProtocol[WSAPROTOCOL_LEN+1];
    WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)((WSAPROTOCOL_INFOW*)pData)->szProtocol, WSAPROTOCOL_LEN, szProtocol,WSAPROTOCOL_LEN, NULL, NULL);
    szProtocol[WSAPROTOCOL_LEN]=0;
#endif

    _stprintf(StringRepresentation,
            _T("{dwServiceFlags1=0x%X,dwServiceFlags2=0x%X,dwServiceFlags3=0x%X,dwServiceFlags4=0x%X,")
            _T("dwProviderFlags=0x%X,ProviderId=%s,dwCatalogEntryId=0x%X,ProtocolChain={ChainLen=%u%s},")
            _T("iVersion=0x%X,iAddressFamily=0x%X,iMaxSockAddr=0x%X,iMinSockAddr=0x%X,")
            _T("iSocketType=0x%X,iProtocol=0x%X,iProtocolMaxOffset=0x%X,iNetworkByteOrder=0x%X,")
            _T("iSecurityScheme=0x%X,dwMessageSize=0x%X,dwProviderReserved=0x%X,szProtocol=%s}"),
            ((WSAPROTOCOL_INFOW*)pData)->dwServiceFlags1,
            ((WSAPROTOCOL_INFOW*)pData)->dwServiceFlags2,
            ((WSAPROTOCOL_INFOW*)pData)->dwServiceFlags3,
            ((WSAPROTOCOL_INFOW*)pData)->dwServiceFlags4,
            ((WSAPROTOCOL_INFOW*)pData)->dwProviderFlags,
            pszGUID,
            ((WSAPROTOCOL_INFOW*)pData)->dwCatalogEntryId,
            ChainLen,
            pszChainEntries,
            ((WSAPROTOCOL_INFOW*)pData)->iVersion,
            ((WSAPROTOCOL_INFOW*)pData)->iAddressFamily,
            ((WSAPROTOCOL_INFOW*)pData)->iMaxSockAddr,
            ((WSAPROTOCOL_INFOW*)pData)->iMinSockAddr,
            ((WSAPROTOCOL_INFOW*)pData)->iSocketType,
            ((WSAPROTOCOL_INFOW*)pData)->iProtocol,
            ((WSAPROTOCOL_INFOW*)pData)->iProtocolMaxOffset,
            ((WSAPROTOCOL_INFOW*)pData)->iNetworkByteOrder,
            ((WSAPROTOCOL_INFOW*)pData)->iSecurityScheme,
            ((WSAPROTOCOL_INFOW*)pData)->dwMessageSize,
            ((WSAPROTOCOL_INFOW*)pData)->dwProviderReserved,
#if (!defined(UNICODE)&&!defined(_UNICODE))
            szProtocol
#else
            ((WSAPROTOCOL_INFOW*)pData)->szProtocol
#endif
            );

    delete pszGUID;
}

void CSupportedParameters::ParseFD_SET(PVOID pData,OUT TCHAR* StringRepresentation)
{
    TCHAR SockValue[16];
    _stprintf(StringRepresentation,
            _T("{fd_count=%u,fd_array={"),
            ((fd_set*)pData)->fd_count
            );
    for(u_int cnt=0;(cnt<(((fd_set*)pData)->fd_count))&&(cnt<FD_SETSIZE);cnt++)
    {
        if (cnt)
            _tcscat(StringRepresentation,_T(", "));
        _stprintf(SockValue,_T("0x%p"),((fd_set*)pData)->fd_array[cnt]);
        _tcscat(StringRepresentation,SockValue);
    }
    _tcscat(StringRepresentation,_T("}}"));
}

void CSupportedParameters::ParseDOUBLE(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,_T("%.19g"), *((double*)pData));
}
void CSupportedParameters::ParseFLOAT(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,_T("%.19g"), *((float*)pData));
}
void CSupportedParameters::ParseINT64(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("0x%I64X"),
            *((INT64*)pData));
}
void CSupportedParameters::ParsePROCESSENTRY32A(PVOID pData,OUT TCHAR* StringRepresentation)
{
#if (defined(UNICODE)||defined(_UNICODE))
    TCHAR szExeFile[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)((PROCESSENTRY32A*)pData)->szExeFile, MAX_PATH-1,szExeFile,(int)MAX_PATH-1);
    szExeFile[MAX_PATH-1]=0;
#endif
    _stprintf(StringRepresentation,
                _T("{dwSize=%u") 
                _T(",cntUsage=%u") 
                _T(",th32ProcessID=0x%X") 
                _T(",th32DefaultHeapID=0x%p") 
                _T(",th32ModuleID=0x%X") 
                _T(",cntThreads=%u") 
                _T(",th32ParentProcessID=0x%X") 
                _T(",pcPriClassBase=%d,dwFlags=0x%X,szExeFile=%s}"),
                ((PROCESSENTRY32A*)pData)->dwSize,
                ((PROCESSENTRY32A*)pData)->cntUsage,
                ((PROCESSENTRY32A*)pData)->th32ProcessID,
                ((PROCESSENTRY32A*)pData)->th32DefaultHeapID,
                ((PROCESSENTRY32A*)pData)->th32ModuleID,
                ((PROCESSENTRY32A*)pData)->cntThreads,
                ((PROCESSENTRY32A*)pData)->th32ParentProcessID,
                ((PROCESSENTRY32A*)pData)->pcPriClassBase,
                ((PROCESSENTRY32A*)pData)->dwFlags,
#if (defined(UNICODE)||defined(_UNICODE))
                szExeFile
#else
                ((PROCESSENTRY32A*)pData)->szExeFile
#endif
                );
}

void CSupportedParameters::ParsePROCESSENTRY32W(PVOID pData,OUT TCHAR* StringRepresentation)
{
#if (!defined(UNICODE)&&!defined(_UNICODE))
    TCHAR szExeFile[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)((PROCESSENTRY32W*)pData)->szExeFile, MAX_PATH-1, szExeFile,MAX_PATH-1, NULL, NULL);
    szExeFile[MAX_PATH-1]=0;
#endif
    _stprintf(StringRepresentation,
                _T("{dwSize=%u") 
                _T(",cntUsage=%u") 
                _T(",th32ProcessID=0x%X") 
                _T(",th32DefaultHeapID=0x%p") 
                _T(",th32ModuleID=0x%X") 
                _T(",cntThreads=%u") 
                _T(",th32ParentProcessID=0x%X") 
                _T(",pcPriClassBase=%d,dwFlags=0x%X,szExeFile=%s}"),
                ((PROCESSENTRY32W*)pData)->dwSize,
                ((PROCESSENTRY32W*)pData)->cntUsage,
                ((PROCESSENTRY32W*)pData)->th32ProcessID,
                ((PROCESSENTRY32W*)pData)->th32DefaultHeapID,
                ((PROCESSENTRY32W*)pData)->th32ModuleID,
                ((PROCESSENTRY32W*)pData)->cntThreads,
                ((PROCESSENTRY32W*)pData)->th32ParentProcessID,
                ((PROCESSENTRY32W*)pData)->pcPriClassBase,
                ((PROCESSENTRY32W*)pData)->dwFlags,
#if (!defined(UNICODE)&&!defined(_UNICODE))
                szExeFile          
#else                              
                ((PROCESSENTRY32W*)pData)->szExeFile
#endif
                );
}
void CSupportedParameters::ParseMODULEENTRY32A(PVOID pData,OUT TCHAR* StringRepresentation)
{
#if (defined(UNICODE)||defined(_UNICODE))
    TCHAR szExePath[MAX_PATH];
    TCHAR szModule[MAX_MODULE_NAME32 + 1];
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)((MODULEENTRY32A*)pData)->szExePath, MAX_PATH-1,szExePath,(int)MAX_PATH-1);
    szExePath[MAX_PATH-1]=0;
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)((MODULEENTRY32A*)pData)->szModule, MAX_MODULE_NAME32,szModule,(int)MAX_MODULE_NAME32);
    szModule[MAX_MODULE_NAME32]=0;
#endif
    _stprintf(StringRepresentation,
                _T("{dwSize=%u") 
                _T(",th32ModuleID=0x%X") 
                _T(",th32ProcessID=0x%X") 
                _T(",GlblcntUsage=%X") 
                _T(",modBaseAddr=0x%p")
                _T(",modBaseSize=%u")
                _T(",hModule=0x%p") 
                _T(",szModule=%s,szExePath=%s}"),
                ((MODULEENTRY32A*)pData)->dwSize,
                ((MODULEENTRY32A*)pData)->th32ModuleID,
                ((MODULEENTRY32A*)pData)->th32ProcessID,
                ((MODULEENTRY32A*)pData)->GlblcntUsage,
                ((MODULEENTRY32A*)pData)->modBaseAddr,
                ((MODULEENTRY32A*)pData)->modBaseSize,
                ((MODULEENTRY32A*)pData)->hModule,
#if (defined(UNICODE)||defined(_UNICODE))
                szModule,
                szExePath
#else
                ((MODULEENTRY32A*)pData)->szModule,
                ((MODULEENTRY32A*)pData)->szExePath
#endif
                );
}
void CSupportedParameters::ParseMODULEENTRY32W(PVOID pData,OUT TCHAR* StringRepresentation)
{
#if (!defined(UNICODE)&&!defined(_UNICODE))
    TCHAR szExePath[MAX_PATH];
    TCHAR szModule[MAX_MODULE_NAME32 + 1];
    WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)((MODULEENTRY32W*)pData)->szExePath, MAX_PATH-1, szExePath,MAX_PATH-1, NULL, NULL);
    szExePath[MAX_PATH-1]=0;
    WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)((MODULEENTRY32W*)pData)->szModule, MAX_MODULE_NAME32, szModule,(int)MAX_MODULE_NAME32, NULL, NULL);
    szModule[MAX_MODULE_NAME32]=0;
#endif
    _stprintf(StringRepresentation,
                _T("{dwSize=%u") 
                _T(",th32ModuleID=0x%X") 
                _T(",th32ProcessID=0x%X") 
                _T(",GlblcntUsage=%X") 
                _T(",modBaseAddr=0x%p")
                _T(",modBaseSize=%u")
                _T(",hModule=0x%p") 
                _T(",szModule=%s,szExePath=%s}"),
                ((MODULEENTRY32W*)pData)->dwSize,
                ((MODULEENTRY32W*)pData)->th32ModuleID,
                ((MODULEENTRY32W*)pData)->th32ProcessID,
                ((MODULEENTRY32W*)pData)->GlblcntUsage,
                ((MODULEENTRY32W*)pData)->modBaseAddr,
                ((MODULEENTRY32W*)pData)->modBaseSize,
                ((MODULEENTRY32W*)pData)->hModule,
#if (!defined(UNICODE)&&!defined(_UNICODE))
                szModule,
                szExePath
#else
                ((MODULEENTRY32W*)pData)->szModule,
                ((MODULEENTRY32W*)pData)->szExePath
#endif
                );
}
void CSupportedParameters::ParseWIN32_FIND_DATAA(PVOID pData,OUT TCHAR* StringRepresentation)
{
#if (defined(UNICODE)||defined(_UNICODE))
    TCHAR cFileName[MAX_PATH];
    TCHAR cAlternateFileName[14];
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)((WIN32_FIND_DATAA*)pData)->cFileName, MAX_PATH-1,cFileName,(int)MAX_PATH-1);
    cFileName[MAX_PATH-1]=0;
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)((WIN32_FIND_DATAA*)pData)->cAlternateFileName, 14,cAlternateFileName,(int)14);
    cAlternateFileName[14-1]=0;
#endif

    TCHAR CreationTime[FILETIME_STRING_REPRESENTATION_MAX_SIZE];
    TCHAR LastAccessTime[FILETIME_STRING_REPRESENTATION_MAX_SIZE];
    TCHAR LastWriteTime[FILETIME_STRING_REPRESENTATION_MAX_SIZE];
    CSupportedParameters::ParseFILETIME(&((WIN32_FIND_DATAA*)pData)->ftCreationTime,CreationTime);
    CSupportedParameters::ParseFILETIME(&((WIN32_FIND_DATAA*)pData)->ftLastAccessTime,LastAccessTime);
    CSupportedParameters::ParseFILETIME(&((WIN32_FIND_DATAA*)pData)->ftLastWriteTime,LastWriteTime);

    _stprintf(StringRepresentation,
                _T("{dwSize=0x%X") 
                _T(",ftCreationTime=%s,")
                _T("ftLastAccessTime=%s,ftLastWriteTime=%s,")
                _T("nFileSizeHigh=%u")  
                _T(",nFileSizeLow=%u") 
                _T(",dwReserved0=0x%X") 
                _T(",dwReserved1=0x%X,")
                _T("cFileName=%s,cAlternateFileName=%s}"),
                ((WIN32_FIND_DATAA*)pData)->dwFileAttributes,
                CreationTime,
                LastAccessTime,
                LastWriteTime,
                ((WIN32_FIND_DATAA*)pData)->nFileSizeHigh,
                ((WIN32_FIND_DATAA*)pData)->nFileSizeLow,
                ((WIN32_FIND_DATAA*)pData)->dwReserved0,
                ((WIN32_FIND_DATAA*)pData)->dwReserved1,
#if (defined(UNICODE)||defined(_UNICODE))
                cFileName,
                cAlternateFileName
#else
                ((WIN32_FIND_DATAA*)pData)->cFileName,
                ((WIN32_FIND_DATAA*)pData)->cAlternateFileName
#endif
                );
}
void CSupportedParameters::ParseWIN32_FIND_DATAW(PVOID pData,OUT TCHAR* StringRepresentation)
{
#if (!defined(UNICODE)&&!defined(_UNICODE))
    TCHAR cFileName[MAX_PATH];
    TCHAR cAlternateFileName[14];
    WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)((WIN32_FIND_DATAW*)pData)->cFileName, MAX_PATH-1, cFileName,(int)MAX_PATH-1, NULL, NULL);
    cFileName[MAX_PATH-1]=0;
    WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)((WIN32_FIND_DATAW*)pData)->cAlternateFileName, 14, cAlternateFileName,(int)14, NULL, NULL);
    cAlternateFileName[14-1]=0;
#endif
    TCHAR CreationTime[FILETIME_STRING_REPRESENTATION_MAX_SIZE];
    TCHAR LastAccessTime[FILETIME_STRING_REPRESENTATION_MAX_SIZE];
    TCHAR LastWriteTime[FILETIME_STRING_REPRESENTATION_MAX_SIZE];
    CSupportedParameters::ParseFILETIME(&((WIN32_FIND_DATAW*)pData)->ftCreationTime,CreationTime);
    CSupportedParameters::ParseFILETIME(&((WIN32_FIND_DATAW*)pData)->ftLastAccessTime,LastAccessTime);
    CSupportedParameters::ParseFILETIME(&((WIN32_FIND_DATAW*)pData)->ftLastWriteTime,LastWriteTime);

    _stprintf(StringRepresentation,
                _T("{dwSize=0x%X") 
                _T(",ftCreationTime=%s,")
                _T("ftLastAccessTime=%s,ftLastWriteTime=%s,")
                _T("nFileSizeHigh=%u")  
                _T(",nFileSizeLow=%u") 
                _T(",dwReserved0=0x%X") 
                _T(",dwReserved1=0x%X,")
                _T("cFileName=%s,cAlternateFileName=%s}"),
                ((WIN32_FIND_DATAW*)pData)->dwFileAttributes,
                CreationTime,
                LastAccessTime,
                LastWriteTime,
                ((WIN32_FIND_DATAW*)pData)->nFileSizeHigh,
                ((WIN32_FIND_DATAW*)pData)->nFileSizeLow,
                ((WIN32_FIND_DATAW*)pData)->dwReserved0,
                ((WIN32_FIND_DATAW*)pData)->dwReserved1,
#if (!defined(UNICODE)&&!defined(_UNICODE))
                cFileName,
                cAlternateFileName
#else
                ((WIN32_FIND_DATAW*)pData)->cFileName,
                ((WIN32_FIND_DATAW*)pData)->cAlternateFileName
#endif
                );
}
void CSupportedParameters::ParseMULTI_QI(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{pIID=0x%p") 
            _T(",pInterface=0x%p") 
            _T(",Result=0x%X}"),
            ((MULTI_QI*)pData)->pIID,
            ((MULTI_QI*)pData)->pItf,
            ((MULTI_QI*)pData)->hr);
}


void CSupportedParameters::ParseDECIMAL(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{sign=%X") 
            _T(",scale=%X") 
            _T(",Hi32=0x%.8X") 
            _T(",Mid32=0x%.8X") 
            _T(",Lo32=0x%.8X") 
            _T("}"),
            ((DECIMAL*)pData)->sign,
            ((DECIMAL*)pData)->scale,
            ((DECIMAL*)pData)->Hi32,
            ((DECIMAL*)pData)->Mid32,
            ((DECIMAL*)pData)->Lo32);
}
void CSupportedParameters::ParseSAFEARRAYBOUND(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{cElements=%u") 
            _T(",lLbound=%d}"),
            ((SAFEARRAYBOUND*)pData)->cElements,
            ((SAFEARRAYBOUND*)pData)->lLbound);

}
void CSupportedParameters::ParseSAFEARRAY(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{cDims=%u") 
            _T(",fFeatures=0x%X") 
            _T(",cbElements=%u") 
            _T(",cLocks=%u") 
            _T(",pvData=0x%p"),
            ((SAFEARRAY*)pData)->cDims,
            ((SAFEARRAY*)pData)->fFeatures,
            ((SAFEARRAY*)pData)->cbElements,
            ((SAFEARRAY*)pData)->cLocks,
            ((SAFEARRAY*)pData)->pvData);
    if (((SAFEARRAY*)pData)->cDims>0)
    {
        // unfortunately as we only retrieve data for a size of sizeof(SAFEARRAY)
        // we only can access first SAFEARRAYBOUND struct data
        TCHAR psz[SAFEARRAYBOUND_STRING_REPRESENTATION_MAX_SIZE];
        CSupportedParameters::ParseSAFEARRAYBOUND(&(((SAFEARRAY*)pData)->rgsabound),psz);
        _tcscat(StringRepresentation,_T(",rgsabound[0]="));
        _tcscat(StringRepresentation,psz);
    }
    _tcscat(StringRepresentation,_T("}"));
}

// psz must be at least TCHAR[VARIANT_TYPE_STRING_MAX_SIZE]
void CSupportedParameters::GetVARIANT_TYPEString(IN VARTYPE VarType,IN OUT TCHAR* psz)
{
    *psz=0;
    BOOL Has_VT_BYREF_Type=((VarType & VT_BYREF)!=0);
    BOOL Has_VT_ARRAY_Type=((VarType & VT_ARRAY)!=0);

    if (Has_VT_ARRAY_Type)
        VarType&=~VT_ARRAY;

    if (Has_VT_BYREF_Type)
        VarType&=~VT_BYREF;

    // add space to split from parameter name
    _tcscat(psz,_T(" "));

    if (Has_VT_ARRAY_Type)
        _tcscat(psz,_T("Array of "));

    switch(VarType)
    {
    case VT_EMPTY:
        _tcscat(psz,_T("VT_EMPTY"));
        break;
    case VT_NULL:
        _tcscat(psz,_T("VT_NULL"));
        break;
    case VT_I2:
        _tcscat(psz,_T("VT_I2"));
        break;         
    case VT_I4:
        _tcscat(psz,_T("VT_I4"));
        break;   
    case VT_I8:
        _tcscat(psz,_T("VT_I8"));
        break; 
    case VT_R4:
        _tcscat(psz,_T("VT_R4"));
        break;         
    case VT_R8:
        _tcscat(psz,_T("VT_R8"));
        break;         
    case VT_CY:
        _tcscat(psz,_T("VT_CY"));
        break;         
    case VT_DATE:
        _tcscat(psz,_T("VT_DATE"));
        break;       
    case VT_BSTR:
        _tcscat(psz,_T("VT_BSTR"));
        break;       
    case VT_DISPATCH:
        _tcscat(psz,_T("VT_DISPATCH"));
        break;   
    case VT_ERROR:
        _tcscat(psz,_T("VT_ERROR"));
        break;      
    case VT_BOOL:
        _tcscat(psz,_T("VT_BOOL"));
        break;       
    case VT_VARIANT:
        _tcscat(psz,_T("VT_VARIANT"));
        break;    
    case VT_DECIMAL:
        _tcscat(psz,_T("VT_DECIMAL"));
        break;    
    case VT_RECORD:
        _tcscat(psz,_T("VT_RECORD"));
        break;     
    case VT_UNKNOWN:
        _tcscat(psz,_T("VT_UNKNOWN"));
        break;    
    case VT_I1:
        _tcscat(psz,_T("VT_I1"));
        break;        
    case VT_UI1:
        _tcscat(psz,_T("VT_UI1"));
        break;        
    case VT_UI2:
        _tcscat(psz,_T("VT_UI2"));
        break;        
    case VT_UI4:
        _tcscat(psz,_T("VT_UI4"));
        break;   
    case VT_UI8:
        _tcscat(psz,_T("VT_UI8"));
        break; 
    case VT_INT:
        _tcscat(psz,_T("VT_INT"));
        break;        
    case VT_UINT:
        _tcscat(psz,_T("VT_UINT"));
        break;       
    case VT_VOID:
        _tcscat(psz,_T("VT_VOID"));
        break;       
    case VT_HRESULT:
        _tcscat(psz,_T("VT_HRESULT"));
        break;    
    case VT_PTR:
        _tcscat(psz,_T("VT_PTR"));
        break;        
    case VT_SAFEARRAY:
        _tcscat(psz,_T("VT_SAFEARRAY"));
        break;  
    case VT_CARRAY:
        _tcscat(psz,_T("VT_CARRAY"));
        break;     
    case VT_USERDEFINED:
        _tcscat(psz,_T("VT_USERDEFINED"));
        break;
    case VT_LPSTR:
        _tcscat(psz,_T("VT_LPSTR"));
        break;      
    case VT_LPWSTR:
        _tcscat(psz,_T("VT_LPWSTR"));
        break;     
    case VT_FILETIME:
        _tcscat(psz,_T("VT_FILETIME"));
        break;   
    case VT_BLOB:
        _tcscat(psz,_T("VT_BLOB"));
        break;       
    case VT_STREAM:
        _tcscat(psz,_T("VT_STREAM"));
        break;     
    case VT_STORAGE:
        _tcscat(psz,_T("VT_STORAGE"));
        break;    
    case VT_STREAMED_OBJECT:
        _tcscat(psz,_T("VT_STREAMED_OBJECT"));
        break;
    case VT_STORED_OBJECT:
        _tcscat(psz,_T("VT_STORED_OBJECT"));
        break;  
    case VT_BLOB_OBJECT:
        _tcscat(psz,_T("VT_BLOB_OBJECT"));
        break;    
    case VT_CF:
        _tcscat(psz,_T("VT_CF"));
        break;             
    case VT_CLSID:
        _tcscat(psz,_T("VT_CLSID"));
        break;          
    case VT_VECTOR:
        _tcscat(psz,_T("VT_VECTOR"));
        break;         
    case VT_RESERVED:
        _tcscat(psz,_T("VT_RESERVED"));
        break;
    default:
        TCHAR val[10];
        _stprintf(val,_T("VT 0x%X"),VarType);
        _tcscat(psz,val);
        break;
    }

    if (Has_VT_BYREF_Type)
        _tcscat(psz,_T(" by ref"));
}

void CSupportedParameters::ParseVARIANT(PVOID pData,OUT TCHAR* StringRepresentation)
{
    // if we are here, that means variant parsing as failed
    // show full variant fields
    VARIANT* pVariant=(VARIANT*)pData;

    TCHAR pszVarType[VARIANT_TYPE_STRING_MAX_SIZE];
    CSupportedParameters::GetVARIANT_TYPEString(pVariant->vt,pszVarType);
    
    
    _stprintf(StringRepresentation,
            _T("{Vartype=%s") 
            _T(",llVal=0x%I64X")
            _T(",lVal=0x%.8X") 
            _T(",bVal=0x%X") 
            _T(",iVal=0x%.8X") 
            _T(",fltVal=%.19g")
            _T(",dblVal=%.19g") 
            _T(",boolVal=0x%X")
            _T(",scode=0x%.8X")
            _T(",cyVal=0x%I64X") 
            _T(",date=%.19g") 
            _T(",bstrVal=0x%p") 
            _T(",punkVal=0x%p") 
            _T(",pdispVal=0x%p") 
            _T(",parray=0x%p") 
            _T(",pbVal=0x%p") 
            _T(",piVal=0x%p") 
            _T(",plVal=0x%p") 
            _T(",pllVal=0x%p") 
            _T(",pfltVal=0x%p") 
            _T(",pdblVal=0x%p") 
            _T(",pboolVal=0x%p") 
            _T(",pscode=0x%p") 
            _T(",pcy=0x%p") 
            _T(",pdate=0x%p") 
            _T(",pbstrVal=0x%p") 
            _T(",ppunkVal=0x%p") 
            _T(",ppdispVal=0x%p") 
            _T(",pparray=0x%p") 
            _T(",pvarVal=0x%p") 
            _T(",byref=0x%p")
            _T(",cVal=0x%X") 
            _T(",uiVal=0x%X") 
            _T(",ulVal=0x%X") 
            _T(",ullVal=0x%I64X")
            _T(",intVal=0x%X")
            _T(",uintVal=0x%X")
            _T(",pdecVal=0x%p")
            _T(",pcVal=0x%p")
            _T(",puiVal=0x%p")
            _T(",pulVal=0x%p")
            _T(",pullVal=0x%p")
            _T(",pintVal=0x%p")
            _T(",puintVal=0x%p")
            _T(",Record {pvRecord=0x%p") 
            _T(",pRecInfo=0x%p")  
            _T("},wReserved1=0x%X") 
            _T(",wReserved2=0x%X") 
            _T(",wReserved3=0x%X") 
            _T("}"),
            pszVarType,
            pVariant->llVal,
            pVariant->lVal,
            pVariant->bVal,
            pVariant->iVal,
            pVariant->fltVal,
            pVariant->dblVal,
            pVariant->boolVal,
            pVariant->scode,
            pVariant->cyVal,
            pVariant->date,
            pVariant->bstrVal,
            pVariant->punkVal,
            pVariant->pdispVal,
            pVariant->parray,
            pVariant->pbVal,
            pVariant->piVal,
            pVariant->plVal,
            pVariant->pllVal,
            pVariant->pfltVal,
            pVariant->pdblVal,
            pVariant->pboolVal,
            pVariant->pscode,
            pVariant->pcyVal,
            pVariant->pdate,
            pVariant->pbstrVal,
            pVariant->ppunkVal,
            pVariant->ppdispVal,
            pVariant->pparray,
            pVariant->pvarVal,
            pVariant->byref,
            pVariant->cVal,
            pVariant->uiVal,
            pVariant->ulVal,
            pVariant->ullVal,
            pVariant->intVal,
            pVariant->uintVal,
            pVariant->pdecVal,
            pVariant->pcVal,
            pVariant->puiVal,
            pVariant->pulVal,
            pVariant->pullVal,
            pVariant->pintVal,
            pVariant->puintVal,
            pVariant->pvRecord,
            pVariant->pRecInfo,
            pVariant->wReserved1,
            pVariant->wReserved2,
            pVariant->wReserved3);
}

void CSupportedParameters::ParseVARIANT_PARSED(PARAMETER_LOG_INFOS* pParamLog,
                                               OUT TCHAR** ppszParam,
                                               TCHAR* FieldSplitter,
                                               DWORD NbRequieredChars)
{
    DWORD NbNeededChars=0;
    // get needed size
    CSupportedParameters::ParseVARIANT_PARSED(pParamLog,NULL,FieldSplitter,NbRequieredChars,&NbNeededChars);
    // allocate memory
    *ppszParam=new TCHAR[NbNeededChars+1];
    *ppszParam[0]=0;
    // get content
    CSupportedParameters::ParseVARIANT_PARSED(pParamLog,*ppszParam,FieldSplitter,NbRequieredChars,&NbNeededChars);

}
void CSupportedParameters::ParseVARIANT_PARSED(IN PARAMETER_LOG_INFOS* pParamLog,
                                               IN OUT TCHAR* pszBuffer,
                                               IN TCHAR* FieldSplitter,
                                               IN DWORD NbRequieredChars,
                                               OUT DWORD* pNbNeededChars)
{
    ////////////////////////////////////////////////////////////////
    // see GetVariantFromStack function for data members encoding
    ////////////////////////////////////////////////////////////////

    *pNbNeededChars=0;
    DWORD NbVariant;
    TCHAR* pszOut;
    PARAMETER_LOG_INFOS* pParamLogInfo;
    PBYTE DataBuffer;
    VARTYPE VarType;
    TCHAR pszVarType[VARIANT_TYPE_STRING_MAX_SIZE];
    // if type was a pointer on variant(s)
    if (pParamLog->Value)
    {
        (*pNbNeededChars)+=20+PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE;
        if (pszBuffer)// ParseVARIANT_PARSED is not called from GenericParameterParsing --> we have to add param name
            _stprintf(pszBuffer,_T("%s:0x%p:"),pParamLog->pszParameterName,pParamLog->Value);
    }
    else
    {
        (*pNbNeededChars)+=PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE;
        if (pszBuffer)// ParseVARIANT_PARSED is not called from GenericParameterParsing --> we have to add param name
            _tcscpy(pszBuffer,pParamLog->pszParameterName);
    }

    // check pointer validity
    if ((pParamLog->dwSizeOfData<sizeof(DWORD)) && (pParamLog->dwSizeOfPointedValue<sizeof(DWORD)))
    {
        (*pNbNeededChars)+=15;
        if (pszBuffer)
            _tcscat(pszBuffer,_T("Bad pointer"));
    }

    // get pointer on our encoded buffer
    DataBuffer=pParamLog->pbValue;

    // get number of VARIANT representations
    memcpy(&NbVariant,DataBuffer,sizeof(DWORD));
    DataBuffer+=sizeof(DWORD);

    // in case of array of variant add { presentation char
    if (NbVariant>1)
    {
        (*pNbNeededChars)+=1;
        if (pszBuffer)
            _tcscat(pszBuffer,_T("{"));
    }
    for (DWORD cnt=0;cnt<NbVariant;cnt++)
    {
        // if amount of needed char has been reached
        if (NbRequieredChars && (NbRequieredChars<*pNbNeededChars))
            break;

        // if not first element, add splitter
        if (cnt>0)
        {
            if (pszBuffer)
                _tcscat(pszBuffer,FieldSplitter);
            (*pNbNeededChars)+=(DWORD)_tcslen(FieldSplitter);
        }
        

        // get variant type
        memcpy(&VarType,DataBuffer,sizeof(VARTYPE));
        DataBuffer+=sizeof(VARTYPE);
        CSupportedParameters::GetVARIANT_TYPEString(VarType,pszVarType);

        // get param log infos
        pParamLogInfo=(PARAMETER_LOG_INFOS*)DataBuffer;
        DataBuffer+=sizeof(PARAMETER_LOG_INFOS);

        // force parameter name for representation
        _tcsncpy(pParamLogInfo->pszParameterName,pszVarType,PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE);
        pParamLogInfo->pszParameterName[PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1]=0;

        // update pParamLogInfo->pbValue to stored data position
        if (pParamLogInfo->pbValue)
            pParamLogInfo->pbValue=DataBuffer;

        // a parsed variant is composed of well known type, so get matching wellknown type representation
        // (notice : as type is wellknown, module name is useless)
        CSupportedParameters::ParameterToString(_T(""),pParamLogInfo,&pszOut,NbRequieredChars,FALSE);

        // add result to current buffer
        if (pszBuffer)
            _tcscat(pszBuffer,pszOut);

        // update number of required char
        (*pNbNeededChars)+=(DWORD)_tcslen(pszOut);

        // free memory allocated by ParameterToString
        delete pszOut;

        // make DataBuffer point to next VARIANT representation
        if (pParamLogInfo->dwSizeOfData>sizeof(PBYTE))
            DataBuffer+=pParamLogInfo->dwSizeOfData;
        else if(pParamLogInfo->dwSizeOfPointedValue)
            DataBuffer+=pParamLogInfo->dwSizeOfPointedValue;
    }
    // in case of array of variant add } presentation char
    if (NbVariant>1)
    {
        (*pNbNeededChars)+=1;
        if (pszBuffer)
            _tcscat(pszBuffer,_T("}"));
    }
}


void CSupportedParameters::ParseSAFEARRAY_PARSED(PARAMETER_LOG_INFOS* pParamLog,
                                               OUT TCHAR** ppszParam,
                                               TCHAR* FieldSplitter,
                                               DWORD NbRequieredChars)
{

    DWORD NbNeededChars=0;
    // get needed size
    CSupportedParameters::ParseSAFEARRAY_PARSED(pParamLog,NULL,FieldSplitter,NbRequieredChars,&NbNeededChars);
    // allocate memory
    *ppszParam=new TCHAR[NbNeededChars+1];
    *ppszParam[0]=0;
    // get content
    CSupportedParameters::ParseSAFEARRAY_PARSED(pParamLog,*ppszParam,FieldSplitter,NbRequieredChars,&NbNeededChars);

}
void CSupportedParameters::ParseSAFEARRAY_PARSED(IN PARAMETER_LOG_INFOS* pParamLog,
                                               IN OUT TCHAR* pszBuffer,
                                               IN TCHAR* FieldSplitter,
                                               IN DWORD NbRequieredChars,
                                               OUT DWORD* pNbNeededChars)
{
    ////////////////////////////////////////////////////////////////
    // see GetSafeArrayFromStack function for data members encoding
    ////////////////////////////////////////////////////////////////

    DWORD NbSafeArray;
    SAFEARRAY* pSafeArray;
    PBYTE DataBuffer;
    TCHAR pszStruct[SAFEARRAY_STRING_REPRESENTATION_MAX_SIZE];
    TCHAR pszArrayBound[SAFEARRAYBOUND_STRING_REPRESENTATION_MAX_SIZE];
    DWORD uscnt;
    DWORD ItemArrayPos;
    DWORD NbItems=0;
    TCHAR* psz;
    PARAMETER_LOG_INFOS Log;
    DWORD cnt;
    PBYTE pArray;
    DWORD StructSize;
    BOOL bError=FALSE;
    *pNbNeededChars=0;
    // if type was a pointer on safearray(s)
    if (pParamLog->Value)
    {
        (*pNbNeededChars)+=20+PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE;
        if (pszBuffer)// ParseSAFEARRAY_PARSED is not called from GenericParameterParsing --> we have to add param name
            _stprintf(pszBuffer,_T("%s:0x%p:"),pParamLog->pszParameterName,pParamLog->Value);
    }
    else
    {
        (*pNbNeededChars)+=PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE;
        if (pszBuffer)// ParseSAFEARRAY_PARSED is not called from GenericParameterParsing --> we have to add param name
            _tcscpy(pszBuffer,pParamLog->pszParameterName);
    }

    // check pointer validity
    if ((pParamLog->dwSizeOfData<sizeof(DWORD)) && (pParamLog->dwSizeOfPointedValue<sizeof(DWORD)))
    {
        (*pNbNeededChars)+=15;
        if (pszBuffer)
            _tcscat(pszBuffer,_T("Bad pointer"));
    }
    
    // get pointer on our encoded buffer
    DataBuffer=pParamLog->pbValue;

    // get number of SAFEARRAY representations
    memcpy(&NbSafeArray,DataBuffer,sizeof(DWORD));
    DataBuffer+=sizeof(DWORD);

    // in case of array of safearray add { presentation char
    if (NbSafeArray>1)
    {
        (*pNbNeededChars)+=1;
        if (pszBuffer)
            _tcscat(pszBuffer,_T("{"));
    }

    for (cnt=0;cnt<NbSafeArray;cnt++)
    {
        // if amount of needed char has been reached
        if (NbRequieredChars && (NbRequieredChars<*pNbNeededChars))
            break;

        // if not first SAFEARRAY representation add splitter
        if (cnt>0)
        {
            if (pszBuffer)
                _tcscat(pszBuffer,FieldSplitter);
            (*pNbNeededChars)+=(DWORD)_tcslen(FieldSplitter);
        }

        // get pointer to safearray
        pSafeArray=(SAFEARRAY*)DataBuffer;
        
        // parse struct
        _stprintf(pszStruct,
            _T("{cDims=%u,fFeatures=0x%X,cbElements=%u,cLocks=%u,pvData=0x%p"),
            pSafeArray->cDims,
            pSafeArray->fFeatures,
            pSafeArray->cbElements,
            pSafeArray->cLocks,
            pSafeArray->pvData);

        if (pszBuffer)
            _tcscat(pszBuffer,pszStruct);
        (*pNbNeededChars)+=(DWORD)_tcslen(pszStruct);


        StructSize=0;
        bError=FALSE;
        if (IsBadReadPtr(pSafeArray->rgsabound,pSafeArray->cDims*sizeof(SAFEARRAYBOUND)))
        {
            bError=TRUE;
        }
        else
        {

            // add ",rgsabound={" string before bounds representation
            if (pszBuffer)
                _tcscat(pszBuffer,_T(",rgsabound={"));
            (*pNbNeededChars)+=20;

            NbItems=0;
            if (pSafeArray->cDims>0)
            {
                // parse safe array bounds in reverse order as they are stored like this in memory
                for (uscnt=pSafeArray->cDims-1;;uscnt--)
                {
                    // if not first rgsabound, add splitter 
                    if (uscnt!=(USHORT)(pSafeArray->cDims-1))
                    {
                        if (pszBuffer)
                            _tcscat(pszBuffer,FieldSplitter);
                        (*pNbNeededChars)+=(DWORD)_tcslen(FieldSplitter);
                    }

                    // compute items number
                    if (NbItems==0)
                        NbItems=pSafeArray->rgsabound[uscnt].cElements;
                    else
                        NbItems*=pSafeArray->rgsabound[uscnt].cElements;
                    
                    // let ParseSAFEARRAYBOUND do the work
                    CSupportedParameters::ParseSAFEARRAYBOUND(&(pSafeArray->rgsabound[uscnt]),pszArrayBound);

                    // add result to pszBuffer
                    if (pszBuffer)
                        _tcscat(pszBuffer,pszArrayBound);

                    // update required memory size
                    (*pNbNeededChars)+=(DWORD)_tcslen(pszArrayBound);

                    if (uscnt==0)
                        break;
                }
            }
            // ends ",rgsabound={"
            if (pszBuffer)
                _tcscat(pszBuffer,_T("}"));
            (*pNbNeededChars)++;

            // parse array content
            StructSize=sizeof(SAFEARRAY)+sizeof(SAFEARRAYBOUND)*(pSafeArray->cDims-1);
            // pArray=((PBYTE)pSafeArray)+StructSize;
            pArray=DataBuffer+StructSize;

            if (IsBadReadPtr(pArray,NbItems*pSafeArray->cbElements))
            {
                bError=TRUE;
            }
            else
            {
                // add ",content={" string before array content representation
                if (pszBuffer)
                    _tcscat(pszBuffer,_T(",content={"));
                (*pNbNeededChars)+=20;

                ItemArrayPos=0;
                // for each item of the array
                for (uscnt=0;uscnt<NbItems;uscnt++)
                {
                    // if not first element, add splitter
                    if (uscnt>0)
                    {
                        if (pszBuffer)
                            _tcscat(pszBuffer,FieldSplitter);
                        (*pNbNeededChars)+=(DWORD)_tcslen(FieldSplitter);
                    }

                    // let GenericParameterParsing do all the work
                    Log.dwSizeOfData=0;
                    Log.dwSizeOfPointedValue=pSafeArray->cbElements;
                    Log.dwType=PARAM_PBYTE;
                    Log.Value=0;
                    ItemArrayPos=uscnt*pSafeArray->cbElements;
                    Log.pbValue=pArray+ItemArrayPos;
                    
                    CSupportedParameters::GenericParameterParsing(&Log,
                                                                    sizeof(BYTE),
                                                                    TRUE,
                                                                    CSupportedParameters::ParseBYTE,
                                                                    BYTE_STRING_REPRESENTATION_MAX_SIZE,
                                                                    &psz,_T(","),FALSE,FALSE,TRUE,0);

                    // add result to current buffer
                    if (pszBuffer)
                        _tcscat(pszBuffer,psz);

                    // update number of required char
                    (*pNbNeededChars)+=(DWORD)_tcslen(psz);

                    // free memory allocated by GenericParameterParsing
                    delete psz;
                }

                // ends ",content={"
                if (pszBuffer)
                    _tcscat(pszBuffer,_T("}"));
                (*pNbNeededChars)++;
            }
        }

        // close safe array struct
        if (pszBuffer)
            _tcscat(pszBuffer,_T("}"));
        (*pNbNeededChars)++;

        if (bError)// other fields may be crashed
            break;
        else
        {
            // make DataBuffer point to next SAFEARRAY representation
            DataBuffer+=StructSize+NbItems*pSafeArray->cbElements;
        }
    }
    // in case of array of safearray add } presentation char
    if (NbSafeArray>1)
    {
        (*pNbNeededChars)+=1;
        if (pszBuffer)
            _tcscat(pszBuffer,_T("}"));
    }
}


void CSupportedParameters::ParseEXCEPINFO(PVOID pData,OUT TCHAR* StringRepresentation)
{
    EXCEPINFO* pExcepinfo=(EXCEPINFO*)pData;
    _stprintf(StringRepresentation,
            _T("wCode=%u,wReserved=%u,dwHelpContext=%u,pvReserved=0x%p,pfnDeferredFillIn=0x%p,scode=0x%.8X")
            _T(",bstrDescription=0x%p")
            _T(",bstrHelpFile=0x%p")
            _T(",bstrSource=0x%p"),
            pExcepinfo->wCode,
            pExcepinfo->wReserved,
            pExcepinfo->dwHelpContext,
            pExcepinfo->pvReserved,
            pExcepinfo->pfnDeferredFillIn,
            pExcepinfo->scode,
            pExcepinfo->bstrDescription,
            pExcepinfo->bstrHelpFile,
            pExcepinfo->bstrSource
        );
}

void CSupportedParameters::ParseEXCEPINFO_PARSED(PARAMETER_LOG_INFOS* pParamLog,
                                               OUT TCHAR** ppszParam,
                                               TCHAR* FieldSplitter,
                                               DWORD NbRequieredChars)
{
    DWORD NbNeededChars=0;
    // get needed size
    CSupportedParameters::ParseEXCEPINFO_PARSED(pParamLog,NULL,FieldSplitter,NbRequieredChars,&NbNeededChars);
    // allocate memory
    *ppszParam=new TCHAR[NbNeededChars+1];
    *ppszParam[0]=0;
    // get content
    CSupportedParameters::ParseEXCEPINFO_PARSED(pParamLog,*ppszParam,FieldSplitter,NbRequieredChars,&NbNeededChars);

}
void CSupportedParameters::ParseEXCEPINFO_PARSED(IN PARAMETER_LOG_INFOS* pParamLog,
                                               IN OUT TCHAR* pszBuffer,
                                               IN TCHAR* FieldSplitter,
                                               IN DWORD NbRequieredChars,
                                               OUT DWORD* pNbNeededChars)
{
    ////////////////////////////////////////////////////////////////
    // see GetExcepinfoFromStack function for data members encoding
    ////////////////////////////////////////////////////////////////

    *pNbNeededChars=0;
    DWORD NbExcepinfo;
    PBYTE DataBuffer;
    TCHAR pszEXCEPINFO[EXCEPINFO_STRING_REPRESENTATION_MAX_SIZE];
    TCHAR psz[MAX_PATH];
    EXCEPINFO* pExcepinfo;
    int WStringLength;
    WCHAR* WString;

    // if type was a pointer
    if (pParamLog->Value)
    {
        (*pNbNeededChars)+=20+PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE;
        if (pszBuffer)// ParseVARIANT_PARSED is not called from GenericParameterParsing --> we have to add param name
            _stprintf(pszBuffer,_T("%s:0x%p:"),pParamLog->pszParameterName,pParamLog->Value);
    }
    else
    {
        (*pNbNeededChars)+=PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE;
        if (pszBuffer)// ParseVARIANT_PARSED is not called from GenericParameterParsing --> we have to add param name
            _tcscpy(pszBuffer,pParamLog->pszParameterName);
    }

    // check pointer validity
    if ((pParamLog->dwSizeOfData<sizeof(DWORD)) && (pParamLog->dwSizeOfPointedValue<sizeof(DWORD)))
    {
        (*pNbNeededChars)+=15;
        if (pszBuffer)
            _tcscat(pszBuffer,_T("Bad pointer"));
    }

    // get pointer on our encoded buffer
    DataBuffer=pParamLog->pbValue;

    // get number of EXCEPINFO representations
    memcpy(&NbExcepinfo,DataBuffer,sizeof(DWORD));
    DataBuffer+=sizeof(DWORD);

    // in case of array add { presentation char
    if (NbExcepinfo>1)
    {
        (*pNbNeededChars)+=1;
        if (pszBuffer)
            _tcscat(pszBuffer,_T("{"));
    }
    for (DWORD cnt=0;cnt<NbExcepinfo;cnt++)
    {
        // if amount of needed char has been reached
        if (NbRequieredChars && (NbRequieredChars<*pNbNeededChars))
            break;

        // if not first element, add splitter
        if (cnt>0)
        {
            if (pszBuffer)
                _tcscat(pszBuffer,FieldSplitter);
            (*pNbNeededChars)+=(DWORD)_tcslen(FieldSplitter);
        }


        pExcepinfo=(EXCEPINFO*)DataBuffer;
        DataBuffer+=sizeof(EXCEPINFO);
        (*pNbNeededChars)+=EXCEPINFO_STRING_REPRESENTATION_MAX_SIZE;
        if (pszBuffer)
        {
            _stprintf(pszEXCEPINFO,
                _T("{wCode=%u,wReserved=%u,dwHelpContext=%u,pvReserved=0x%p,pfnDeferredFillIn=0x%p,scode=0x%.8X"),
                pExcepinfo->wCode,
                pExcepinfo->wReserved,
                pExcepinfo->dwHelpContext,
                pExcepinfo->pvReserved,
                pExcepinfo->pfnDeferredFillIn,
                pExcepinfo->scode
                );
            _tcscat(pszBuffer,pszEXCEPINFO);
        }

        for (int cnt2=0;cnt2<3;cnt2++)
        {

            switch(cnt2)
            {
                case 0:
                    _stprintf(psz,_T(",bstrDescription=0x%p"),pExcepinfo->bstrDescription);
                break;
                case 1:
                    _stprintf(psz,_T(",bstrHelpFile=0x%p"),pExcepinfo->bstrHelpFile);
                    break;
                case 2:
                    _stprintf(psz,_T(",bstrSource=0x%p"),pExcepinfo->bstrSource);
                    break;
            }
            (*pNbNeededChars)+=50;
            if (pszBuffer)
                _tcscat(pszBuffer,psz);

            memcpy(&WStringLength,DataBuffer,sizeof(int));
            DataBuffer+=sizeof(int);

            if (pszBuffer)
                _tcscat(pszBuffer,_T(":"));
            (*pNbNeededChars)+=1;

            if (WStringLength>0)
            {
                WString=(WCHAR*)DataBuffer;
                DataBuffer+=WStringLength;
                if (pszBuffer)
                {
#if (defined(UNICODE)||defined(_UNICODE))
                    _tcscat(pszBuffer,WString);
#else
                    CHAR* AnsiStr;
                    CAnsiUnicodeConvert::UnicodeToAnsi(WString,&AnsiStr);
                    _tcscat(pszBuffer,AnsiStr);
                    free(AnsiStr);
#endif
                }
                (*pNbNeededChars)+=WStringLength/sizeof(WCHAR);
            }
            else
            {
                (*pNbNeededChars)+=20;
                if (pszBuffer)
                    _tcscat(pszBuffer,_T("Bad Pointer"));
            }
        }
        // add struct end char
        if (pszBuffer)
            _tcscat(pszBuffer,_T("}"));
        (*pNbNeededChars)+=1;
    }
    // in case of array add } presentation char
    if (NbExcepinfo>1)
    {
        (*pNbNeededChars)+=1;
        if (pszBuffer)
            _tcscat(pszBuffer,_T("}"));
    }
}


void CSupportedParameters::ParseDISPPARAMS(PVOID pData,OUT TCHAR* StringRepresentation)
{
    _stprintf(StringRepresentation,
            _T("{cArgs=%u") 
            _T(",rgvarg=0x%p") 
            _T(",cNamedArgs=%u") 
            _T(",rgdispidNamedArgs=0x%p") 
            _T("}"),
            ((DISPPARAMS*)pData)->cArgs,
            ((DISPPARAMS*)pData)->rgvarg,
            ((DISPPARAMS*)pData)->cNamedArgs,
            ((DISPPARAMS*)pData)->rgdispidNamedArgs);
}

void CSupportedParameters::ParseDISPPARAMS_PARSED(PARAMETER_LOG_INFOS* pParamLog,
                                             OUT TCHAR** ppszParam,
                                             TCHAR* FieldSplitter,
                                             DWORD NbRequieredChars)
{

    DWORD NbNeededChars=0;
    // get needed size
    CSupportedParameters::ParseDISPPARAMS_PARSED(pParamLog,NULL,FieldSplitter,NbRequieredChars,&NbNeededChars);
    // allocate memory
    *ppszParam=new TCHAR[NbNeededChars+1];
    *ppszParam[0]=0;
    // get content
    CSupportedParameters::ParseDISPPARAMS_PARSED(pParamLog,*ppszParam,FieldSplitter,NbRequieredChars,&NbNeededChars);

}
void CSupportedParameters::ParseDISPPARAMS_PARSED(IN PARAMETER_LOG_INFOS* pParamLog,IN OUT TCHAR* pszBuffer,IN TCHAR* FieldSplitter,IN DWORD NbRequieredChars,OUT DWORD* pNbNeededChars)
{
    // if wanted length is not enough to contain small representation
    if ((NbRequieredChars!=0)
        && (NbRequieredChars<=DISPPARAMS_STRING_REPRESENTATION_MAX_SIZE)
        )
    {
        // query only small representation
        *pNbNeededChars=DISPPARAMS_STRING_REPRESENTATION_MAX_SIZE;
        if (pszBuffer)
        {
            CSupportedParameters::ParseDISPPARAMS(pParamLog->pbValue,pszBuffer);
        }
        return;
    }

    // else query size for the variant array

    // use a forged PARAMETER_LOG_INFOS
    PARAMETER_LOG_INFOS ParamLogVariant;
    DISPPARAMS* pDispParams=(DISPPARAMS*)pParamLog->pbValue;
    memset(&ParamLogVariant,0,sizeof(PARAMETER_LOG_INFOS));

    _tcscpy(ParamLogVariant.pszParameterName,_T("rgvarg"));
    ParamLogVariant.Value=(PBYTE)pDispParams->rgvarg;
    ParamLogVariant.pbValue=pParamLog->pbValue+sizeof(DISPPARAMS);

    if (pParamLog->dwSizeOfPointedValue)
        ParamLogVariant.dwSizeOfPointedValue=pParamLog->dwSizeOfPointedValue;
    else
        ParamLogVariant.dwSizeOfPointedValue=pParamLog->dwSizeOfData;
    ParamLogVariant.dwSizeOfPointedValue-=sizeof(DISPPARAMS);

    CSupportedParameters::ParseVARIANT_PARSED(&ParamLogVariant,pszBuffer,FieldSplitter,NbRequieredChars,pNbNeededChars);
}


// static properties initialization
PBYTE CSupportedParameters::ReportErrorUserParam=NULL;
CSupportedParameters::pfReportError CSupportedParameters::ReportError=NULL;
TCHAR CSupportedParameters::ApplicationPath[MAX_PATH]=_T("");
TCHAR CSupportedParameters::UserTypePath[MAX_PATH]=_T("");
TCHAR CSupportedParameters::DefinePath[MAX_PATH]=_T("");

void CSupportedParameters::SetErrorReport(CSupportedParameters::pfReportError const ErrorReport,IN PBYTE const UserParam)
{
    CSupportedParameters::ReportError=ErrorReport;
    CSupportedParameters::ReportErrorUserParam=UserParam;
}

void CSupportedParameters::GetPaths()
{
    // supported parameters is included in apioverride.dll and winapioverride.exe
    // so if apioverride.dll is not present we are in the winapioverride.exe process
    HMODULE hModule = GetModuleHandle(API_OVERRIDE_DLL_NAME);
    if (hModule!=0)
        // apioverride.dll is loaded -> we can access the winapioverride directory
        CStdFileOperations::GetModulePath(hModule,CSupportedParameters::ApplicationPath,MAX_PATH);
    else
        // we are in the winapioverride.exe process -> we can access the winapioverride directory
        CStdFileOperations::GetAppPath(CSupportedParameters::ApplicationPath,MAX_PATH);

    _tcscpy(CSupportedParameters::UserTypePath,CSupportedParameters::ApplicationPath);
    _tcscat(CSupportedParameters::UserTypePath,APIOVERRIDE_USER_TYPES_PATH);

    _tcscpy(CSupportedParameters::DefinePath,CSupportedParameters::ApplicationPath);
    _tcscat(CSupportedParameters::DefinePath,APIOVERRIDE_DEFINES_PATH);
}

void CSupportedParameters::DefineToString(TCHAR* const ModuleName,PARAMETER_LOG_INFOS* const pParamLog,TCHAR** pszParameter,DWORD const NbRequieredChars)
{
    UNREFERENCED_PARAMETER(ModuleName);// ModuleName is not used. pParamLog->pszDefineNamesFile is the full relative path from the Define directory
    UNREFERENCED_PARAMETER(NbRequieredChars);

    if (*CSupportedParameters::ApplicationPath ==0)
        CSupportedParameters::GetPaths();

    CUserDefine ObjUserDefine;

    // parse define file if any
    ObjUserDefine.Parse(CSupportedParameters::DefinePath,pParamLog->pszDefineNamesFile,CSupportedParameters::ReportError,CSupportedParameters::ReportErrorUserParam);
    
    TCHAR* pParameterName;
    // if parameter has a specified name
    if (*(pParamLog->pszParameterName)!=0)
        // keep it
        pParameterName=pParamLog->pszParameterName;
    else
        // use the type name
        pParameterName=CSupportedParameters::GetParamName(pParamLog->dwType & SIMPLE_TYPE_FLAG_MASK);

    // do define resolution
    ObjUserDefine.DefineToString(pParameterName,pParamLog->Value,pszParameter);
}


void CSupportedParameters::ClearUserDataTypeCache()
{
    CUserDataType::ClearCache();
}

void CSupportedParameters::UserDataTypeToString(TCHAR* const ModuleName,PARAMETER_LOG_INFOS* const pParamLog,TCHAR** pszParameter,DWORD const NbRequieredChars)
{
    UNREFERENCED_PARAMETER(NbRequieredChars);

    if (*CSupportedParameters::ApplicationPath ==0)
        CSupportedParameters::GetPaths();

    TCHAR* pParameterName;
    // if parameter has a specified name
    if (*(pParamLog->pszParameterName)!=0)
        // keep it
        pParameterName=pParamLog->pszParameterName;
    else
        // use the type name
        pParameterName=pParamLog->pszUserDataTypeName;

    // module name is decorated in case of EXE_INTERNAL_XXX definitions 
    // ModuleName is like MyExe.exe|EXE_INTERNAL_XXX --> remove decoration |EXE_INTERNAL_XXX
    TCHAR LocalModuleName[2*MAX_PATH];
    TCHAR* psz= _tcschr(ModuleName,'|');
    if (psz)
    {
        // do string copy to avoid to modify original string
        _tcsncpy(LocalModuleName,ModuleName,2*MAX_PATH);
        LocalModuleName[2*MAX_PATH-1]=0;
        // look for '|' inside copy
        psz= _tcschr(LocalModuleName,'|');
        // remove '|' to keep only file name
        if (psz)
            *psz=0;
        psz = LocalModuleName;
    }
    else
        psz = ModuleName;
    // assume to keep only file name
    psz = CStdFileOperations::GetFileName(psz);


    SIZE_T NbPointedTime = 0;
    CUserDataType::DEFINITION_FOUND DefinitionFound;
    // parse user type file if any
    CUserDataType* pUserDataType = CUserDataType::Parse(CSupportedParameters::UserTypePath,psz,pParamLog->pszUserDataTypeName,&NbPointedTime,CSupportedParameters::ReportError,CSupportedParameters::ReportErrorUserParam,&DefinitionFound);
    // create a CUserDataTypeVar of specified type and with specified var name to do the parsing
    CUserDataTypeVar Var;
    Var.pUserDataType = pUserDataType;
    Var.NbPointedTimes = NbPointedTime;
    Var.SetName(pParameterName);
    // do user type parsing
    Var.ToString(pParamLog,pszParameter,NbRequieredChars);

    if (DefinitionFound == CUserDataType::DEFINITION_FOUND_NOT_FOUND)
    {
        if (CSupportedParameters::ReportError)
        {
            TCHAR Msg[MAX_PATH];
            _sntprintf(Msg,MAX_PATH,_T("Specified user type description file not found for type %s"),pParamLog->pszUserDataTypeName);
            Msg[MAX_PATH-1]=0;
            CSupportedParameters::ReportError(Msg,CSupportedParameters::ReportErrorUserParam);
        }
    }
}