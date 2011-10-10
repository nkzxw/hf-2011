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

// use  _WINSOCKAPI_ as preprocessor info

#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#include <windows.h>
#include <winternl.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "InterProcessCommunication.h"
#include "../../LinkList/LinkListSimple.h"
#include "../../String/AnsiUnicodeConvert.h"

///////////////////////////////////
// include and def for struct only
///////////////////////////////////
#include <commctrl.h>
#include <shlobj.h>
#include <Tlhelp32.h>
#pragma warning (push)
#pragma warning(disable : 4005)// to avoid  '_WINSOCKAPI_' macro redefinition warning
#include <winsock2.h>
#pragma warning (pop)
#include <ws2tcpip.h>
#include <objidl.h>
#include <oaidl.h>
///////////////////////////////////
// End of include and def for struct only
///////////////////////////////////


#define REGISTER_BYTE_SIZE sizeof(PBYTE)
#define GetRegisterSize() (REGISTER_BYTE_SIZE)
#define GetPointerSize GetRegisterSize
// macro to know the stack sized used by a type or struct
#ifndef StackSizeOf
    #define StackSizeOf(Type) ( (sizeof(Type)<sizeof(PBYTE)) ? sizeof(PBYTE) : (sizeof(Type)) )
#endif

#ifndef StackSize
    #define StackSize(TypeSize) ( (TypeSize == 0) ? 0 : __max(TypeSize,REGISTER_BYTE_SIZE) )
#endif


// supported parameters enums
// keep enum order for incremental version support (previous versions saved files reloading)
enum tagSupportedParameters
{
    PARAM_BYTE=0,
    PARAM_UCHAR,
    PARAM_PBYTE,
    PARAM_WORD,
    PARAM_PWORD,
    PARAM_SHORT,
    PARAM_USHORT,
    PARAM_PSHORT,
    PARAM_WPARAM,
    PARAM_ULONG,
    PARAM_PULONG,
    PARAM_ULONG_PTR,
    PARAM_PULONG_PTR,
    PARAM_INT,
    PARAM_PINT,
    PARAM_UINT,
    PARAM_PUINT,
    PARAM_LONG,
    PARAM_PLONG,
    PARAM_LONG_PTR,
    PARAM_PLONG_PTR,
    PARAM_LPARAM,
    PARAM_DWORD,
    PARAM_PDWORD,
    PARAM_SIZE_T,
    PARAM_PSIZE_T,
    PARAM_PSTR,
    PARAM_PWSTR,
    PARAM_PVOID,
    PARAM_HANDLE,
    PARAM_BOOL,
    PARAM_PBOOL,
    PARAM_BOOLEAN,
    PARAM_PBOOLEAN,
    PARAM_CHAR,
    PARAM_PUCHAR,
    PARAM_WCHAR,
    PARAM_HINSTANCE,
    PARAM_HWND,
    PARAM_HMODULE,
    PARAM_PHMODULE,
    PARAM_HDC,
    PARAM_HMENU,
    PARAM_HIMAGELIST,
    PARAM_HDESK,
    PARAM_HBRUSH,
    PARAM_HRGN,
    PARAM_WNDPROC,
    PARAM_DLGPROC,
    PARAM_FARPROC,
    PARAM_LARGE_INTEGER,
    PARAM_PLARGE_INTEGER,
    PARAM_POINT,
    PARAM_PPOINT,
    PARAM_RECT,
    PARAM_PRECT,
    PARAM_PMSG,
    PARAM_PUNICODE_STRING,
    PARAM_PANSI_STRING,
    PARAM_FLOAT,
    PARAM_PFLOAT,
    PARAM_DOUBLE,
    PARAM_PDOUBLE,
    PARAM_INT64,
    PARAM_PINT64,
    PARAM_HKEY,
    PARAM_PHKEY,
    PARAM_HCRYPTPROV,
    PARAM_HCRYPTKEY,
    PARAM_HCRYPTHASH,
    PARAM_PHANDLE,
    PARAM_HPALETTE,
    PARAM_HCONV,
    PARAM_HSZ,
    PARAM_HDDEDATA,
    PARAM_SC_HANDLE,
    PARAM_HCERTSTORE,
    PARAM_HGLOBAL,
    PARAM_HFONT,
    PARAM_HMETAFILE,
    PARAM_HGDIOBJ,
    PARAM_HCOLORSPACE,
    PARAM_HBITMAP,
    PARAM_HICON,
    PARAM_PHICON,
    PARAM_HDPA,
    PARAM_HDSA,
    PARAM_SOCKET,
    PARAM_PSOCKET,
    PARAM_HOSTENT,
    PARAM_SOCKADDR_IN,
    PARAM_SOCKADDR,
    PARAM_PHOSTENT,
    PARAM_PSOCKADDR_IN,
    PARAM_PSOCKADDR,
    PARAM_PFILE,
    PARAM_PSID,
    PARAM_PPSID,
    PARAM_GUID,
    PARAM_PGUID,
    PARAM_PFNCALLBACK,
    PARAM_COLORREF,
    PARAM_PCOLORREF,
    PARAM_SECURITY_INFORMATION,
    PARAM_PSECURITY_DESCRIPTOR,
    PARAM_PPSECURITY_DESCRIPTOR,
    PARAM_SECURITY_ATTRIBUTES,
    PARAM_PSECURITY_ATTRIBUTES,
    PARAM_ACL,
    PARAM_PACL,
    PARAM_REGSAM,
    PARAM_PDLGTEMPLATE,
    PARAM_WNDCLASS,
    PARAM_PWNDCLASS,
    PARAM_WNDCLASSEX,
    PARAM_PWNDCLASSEX,
    PARAM_SIZE,
    PARAM_PSIZE,
    PARAM_ACCESS_MASK,
    PARAM_PACCESS_MASK,
    PARAM_TRACEHANDLE,
    PARAM_PTRACEHANDLE,
    PARAM_PSTARTUPINFO,
    PARAM_PSHELLEXECUTEINFO,
    PARAM_SYSTEMTIME,
    PARAM_PSYSTEMTIME,
    PARAM_FILETIME,
    PARAM_PFILETIME,
    PARAM_PSECHANDLE,
    PARAM_PCTXTHANDLE,
    PARAM_PCREDHANDLE,
    PARAM_LCID,
    PARAM_CRITICAL_SECTION,
    PARAM_PCRITICAL_SECTION,
    PARAM_MEMORY_BASIC_INFORMATION,
    PARAM_PMEMORY_BASIC_INFORMATION,
    PARAM_PDCB,
    PARAM_PPROCESSENTRY32A,
    PARAM_PPROCESSENTRY32W,
    PARAM_PMODULEENTRY32A,
    PARAM_PMODULEENTRY32W,
    PARAM_PWIN32_FIND_DATAA,
    PARAM_PWIN32_FIND_DATAW,
    PARAM_PHEAPENTRY32,
    PARAM_PTHREADENTRY32,
    PARAM_PPROCESS_HEAP_ENTRY,
    PARAM_PIO_STATUS_BLOCK,
    PARAM_PPRINTDLG,
    PARAM_PPRINTDLGEX,
    PARAM_PPAGESETUPDLG,
    PARAM_POPENFILENAME,
    PARAM_PCHOOSEFONT,
    PARAM_PFINDREPLACE,
    PARAM_NTSTATUS,
    PARAM_LSA_HANDLE,
    PARAM_PLSA_HANDLE,
    PARAM_POVERLAPPED,
    PARAM_PWSABUF,
    PARAM_PFD_SET,
    PARAM_PADDRINFO,
    PARAM_PWSADATA,
    PARAM_PWSAPROTOCOL_INFOA,
    PARAM_PWSAPROTOCOL_INFOW,
    PARAM_PTIMEVAL,
    PARAM_PCOMMTIMEOUTS,
    PARAM_PCOMMCONFIG,
    PARAM_PBROWSEINFO,
    PARAM_PSHFILEINFOA,
    PARAM_PSHFILEINFOW,
    PARAM_PNOTIFYICONDATAA,
    PARAM_PNOTIFYICONDATAW,
    PARAM_PPVOID,
    PARAM_IID,
    PARAM_CLSID,
    PARAM_FMTID,
    PARAM_PIID,
    PARAM_PCLSID,
    PARAM_PFMTID,
    PARAM_UNKNOWN, // don't need to be the last
    PARAM_MUTLI_QI,
    PARAM_PMUTLI_QI,
    PARAM_DECIMAL,
    PARAM_PDECIMAL,
    PARAM_BSTR,
    PARAM_VARIANT,
    PARAM_PVARIANT,
    PARAM_VARIANT_PARSED,// not a real type (on stack decoded parameter)
    PARAM_SAFEARRAY,
    PARAM_PSAFEARRAY,
    PARAM_SAFEARRAY_PARSED,// not a real type (on stack decoded parameter)
    PARAM_SAFEARRAYBOUND,
    PARAM_PSAFEARRAYBOUND,
    PARAM_EXCEPINFO,
    PARAM_PEXCEPINFO,
    PARAM_EXCEPINFO_PARSED,// not a real type (on stack decoded parameter)
    PARAM_POINTER,
    PARAM_PPOINTER,
    PARAM_DISPPARAMS,
    PARAM_PDISPPARAMS,
    PARAM_DISPPARAMS_PARSED,// not a real type (on stack decoded parameter)
    PARAM_NET_STRING,
    PARAM_VOID,
    PARAM_LOGFONTA,
    PARAM_PLOGFONTA,
    PARAM_LOGFONTW,
    PARAM_PLOGFONTW
};

// struct used to define types
typedef struct tagSupportedParametersStruct 
{
    // Waring: keep fields order for SupportedParametersArray initialization
    TCHAR* ParamName;
    DWORD ParamType;
    DWORD DataSize;
    DWORD PointedDataSize;
}SUPPORTED_PARAMETERS_STRUCT,*PSUPPORTED_PARAMETERS_STRUCT;

// macro to know the stack sized used by a type or struct
#ifndef StackSizeOf
    #define StackSizeOf(Type) ((sizeof(Type)<REGISTER_BYTE_SIZE)?REGISTER_BYTE_SIZE:(sizeof(Type)))
#endif 

#define DWORD_STRING_REPRESENTATION_MAX_SIZE 32
#define ULONG_PTR_STRING_REPRESENTATION_MAX_SIZE 64
#define WORD_STRING_REPRESENTATION_MAX_SIZE 32
#define BYTE_STRING_REPRESENTATION_MAX_SIZE 32
#define FILE_STRING_REPRESENTATION_MAX_SIZE 200
#define IO_STATUS_BLOCK_STRING_REPRESENTATION_MAX_SIZE 64
#define PRINTDLG_STRING_REPRESENTATION_MAX_SIZE 600
#define PRINTDLGEX_STRING_REPRESENTATION_MAX_SIZE 700
#define PAGESETUPDLG_STRING_REPRESENTATION_MAX_SIZE 700
#define CHOOSEFONT_STRING_REPRESENTATION_MAX_SIZE 700
#define OPENFILENAME_STRING_REPRESENTATION_MAX_SIZE 700
#define FINDREPLACE_STRING_REPRESENTATION_MAX_SIZE 700
#define BROWSEINFO_STRING_REPRESENTATION_MAX_SIZE 300
#define DCB_STRING_REPRESENTATION_MAX_SIZE 700
#define STARTUPINFO_STRING_REPRESENTATION_MAX_SIZE 600
#define SHELLEXECUTEINFO_STRING_REPRESENTATION_MAX_SIZE 600
#define LARGE_INTEGER_STRING_REPRESENTATION_MAX_SIZE 64
#define POINT_STRING_REPRESENTATION_MAX_SIZE 64
#define SIZE_STRING_REPRESENTATION_MAX_SIZE 64
#define RECT_STRING_REPRESENTATION_MAX_SIZE 100
#define MSG_STRING_REPRESENTATION_MAX_SIZE 200
#define TIMEVAL_STRING_REPRESENTATION_MAX_SIZE 64
#define HOSTENT_STRING_REPRESENTATION_MAX_SIZE 160
#define SOCKADDR_STRING_REPRESENTATION_MAX_SIZE 100
#define SOCKADDR_IN_STRING_REPRESENTATION_MAX_SIZE 100
#define GUID_STRING_REPRESENTATION_MAX_SIZE 120
#define SECURITY_ATTRIBUTES_STRING_REPRESENTATION_MAX_SIZE 80
#define ACL_STRING_REPRESENTATION_MAX_SIZE 120
#define MEMORY_BASIC_INFORMATION_STRING_REPRESENTATION_MAX_SIZE 180
#define OVERLAPPED_STRING_REPRESENTATION_MAX_SIZE 300
#define WSABUF_STRING_REPRESENTATION_MAX_SIZE 64
#define ADDRINFO_STRING_REPRESENTATION_MAX_SIZE 300
#define WSADATA_STRING_REPRESENTATION_MAX_SIZE 800
#define WSAPROTOCOL_INFOA_STRING_REPRESENTATION_MAX_SIZE 1500
#define WSAPROTOCOL_INFOW_STRING_REPRESENTATION_MAX_SIZE WSAPROTOCOL_INFOA_STRING_REPRESENTATION_MAX_SIZE
#define FD_SET_STRING_REPRESENTATION_MAX_SIZE (64+FD_SETSIZE*20)
#define CRITICAL_SECTION_STRING_REPRESENTATION_MAX_SIZE 180
#define HEAPENTRY32_STRING_REPRESENTATION_MAX_SIZE 300
#define THREADENTRY32_STRING_REPRESENTATION_MAX_SIZE 200
#define PROCESS_HEAP_ENTRY_STRING_REPRESENTATION_MAX_SIZE 300
#define SYSTEMTIME_STRING_REPRESENTATION_MAX_SIZE 200
#define FILETIME_STRING_REPRESENTATION_MAX_SIZE SYSTEMTIME_STRING_REPRESENTATION_MAX_SIZE
#define SECHANDLE_STRING_REPRESENTATION_MAX_SIZE 100
#define DLGTEMPLATE_STRING_REPRESENTATION_MAX_SIZE 200
#define WNDCLASS_STRING_REPRESENTATION_MAX_SIZE 300
#define WNDCLASSEX_STRING_REPRESENTATION_MAX_SIZE 300
#define DOUBLE_STRING_REPRESENTATION_MAX_SIZE 64
#define FLOAT_STRING_REPRESENTATION_MAX_SIZE 64
#define INT64_STRING_REPRESENTATION_MAX_SIZE 64
#define PROCESSENTRY32A_STRING_REPRESENTATION_MAX_SIZE (260+MAX_PATH)
#define PROCESSENTRY32W_STRING_REPRESENTATION_MAX_SIZE PROCESSENTRY32A_STRING_REPRESENTATION_MAX_SIZE
#define MODULEENTRY32A_STRING_REPRESENTATION_MAX_SIZE (260+MAX_PATH+MAX_MODULE_NAME32)
#define MODULEENTRY32W_STRING_REPRESENTATION_MAX_SIZE MODULEENTRY32A_STRING_REPRESENTATION_MAX_SIZE
#define WIN32_FIND_DATAA_STRING_REPRESENTATION_MAX_SIZE 1024
#define WIN32_FIND_DATAW_STRING_REPRESENTATION_MAX_SIZE WIN32_FIND_DATAA_STRING_REPRESENTATION_MAX_SIZE
#define COMMTIMEOUTS_STRING_REPRESENTATION_MAX_SIZE 400
#define COMMCONFIG_STRING_REPRESENTATION_MAX_SIZE (400+DCB_STRING_REPRESENTATION_MAX_SIZE)
#define SHFILEINFOA_STRING_REPRESENTATION_MAX_SIZE 400
#define SHFILEINFOW_STRING_REPRESENTATION_MAX_SIZE SHFILEINFOA_STRING_REPRESENTATION_MAX_SIZE
#define NOTIFYICONDATAA_STRING_REPRESENTATION_MAX_SIZE 400
#define NOTIFYICONDATAW_STRING_REPRESENTATION_MAX_SIZE NOTIFYICONDATAA_STRING_REPRESENTATION_MAX_SIZE
#define MULTI_QI_STRING_REPRESENTATION_MAX_SIZE 100
#define DECIMAL_STRING_REPRESENTATION_MAX_SIZE 100
#define SAFEARRAYBOUND_STRING_REPRESENTATION_MAX_SIZE 50
#define SAFEARRAY_STRING_REPRESENTATION_MAX_SIZE (150+SAFEARRAYBOUND_STRING_REPRESENTATION_MAX_SIZE)
#define VARIANT_STRING_REPRESENTATION_MAX_SIZE 1500
#define VARIANT_TYPE_STRING_MAX_SIZE 50
#define EXCEPINFO_STRING_REPRESENTATION_MAX_SIZE 400
#define DISPPARAMS_STRING_REPRESENTATION_MAX_SIZE 100
#define LOGFONTA_STRING_REPRESENTATION_MAX_SIZE 512
#define LOGFONTW_STRING_REPRESENTATION_MAX_SIZE LOGFONTA_STRING_REPRESENTATION_MAX_SIZE

class CSupportedParameters
{
public:
    typedef void (*pfReportError)(IN TCHAR* const ErrorMessage,IN PBYTE const UserParam);

private:
    typedef void (*tagParamParsingProc)(PVOID pData,OUT TCHAR* StringRepresentation);
    static void __fastcall GenericParameterParsing(PARAMETER_LOG_INFOS* const pParamLog,
                                                DWORD const SizeOfType,
                                                BOOL const IsPointer,
                                                tagParamParsingProc const pfnParsingFunc,
                                                DWORD const OneParamStringRepresentationSize,
                                                OUT TCHAR** ppszParam,
                                                TCHAR* const FieldSplitter,
                                                BOOL const DisplayTypeName,
                                                BOOL const DisplayPointerValue,
                                                BOOL const DisplayArrayLimits,
                                                DWORD const NbRequieredChars);
    static void __fastcall GenericParameterParsing(PARAMETER_LOG_INFOS* const pParamLog,
                                                DWORD const SizeOfType,
                                                BOOL const IsPointer,
                                                tagParamParsingProc const pfnParsingFunc,
                                                DWORD const OneParamStringRepresentationSize,
                                                OUT TCHAR** const ppszParam,
                                                DWORD const NbRequieredChars,
                                                BOOL const DisplayTypeName);
    static void ParseULONG_PTR(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseDWORD(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseWORD(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseBYTE(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseFILE(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseIO_STATUS_BLOCK(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParsePRINTDLG(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParsePRINTDLGEX(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParsePAGESETUPDLG(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseCHOOSEFONT(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseFINDREPLACE(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseDCB(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseCOMMTIMEOUTS(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseCOMMCONFIG(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseSTARTUPINFO(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseSHELLEXECUTEINFO(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseLARGE_INTEGER(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParsePOINT(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseLOGFONTA(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseLOGFONTW(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseSIZE(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseRECT(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseMSG(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseHOSTENT(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseSOCKADDR(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseSOCKADDR_IN(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseGUID(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseSECURITY_ATTRIBUTES(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseACL(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseMEMORY_BASIC_INFORMATION(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseCRITICAL_SECTION(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseHEAPENTRY32(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseTHREADENTRY32(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParsePROCESS_HEAP_ENTRY(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseSYSTEMTIME(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseFILETIME(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseSECHANDLE(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseDLGTEMPLATE(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseWNDCLASS(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseWNDCLASSEX(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseDOUBLE(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseFLOAT(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseINT64(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParsePROCESSENTRY32A(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParsePROCESSENTRY32W(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseMODULEENTRY32A(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseMODULEENTRY32W(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseWIN32_FIND_DATAA(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseWIN32_FIND_DATAW(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseOPENFILENAME(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseOVERLAPPED(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseWSABUF(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseFD_SET(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseADDRINFO(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseWSADATA(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseWSAPROTOCOL_INFOA(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseWSAPROTOCOL_INFOW(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseTIMEVAL(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseBROWSEINFO(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseSHFILEINFOA(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseSHFILEINFOW(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseNOTIFYICONDATAA(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseNOTIFYICONDATAW(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseMULTI_QI(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseSAFEARRAYBOUND(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseSAFEARRAY(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseDECIMAL(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseVARIANT(PVOID pData,OUT TCHAR* StringRepresentation);
    static BOOL GetVariantFromStack(HANDLE LogHeap,IN VARIANT* pVariant,DWORD PointedDataSize,IN OUT PBYTE pBuffer,OUT DWORD* pRequieredMemorySize);
    static void GetVariantFromStackDefaultParsing(HANDLE LogHeap,IN VARIANT* pVariant,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo,OUT DWORD* pRequieredMemorySize);
    static void GetVariantFromStackEx(HANDLE LogHeap,IN VARIANT* pVariant,DWORD PointedDataSize,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo,OUT DWORD* pRequieredMemorySize);
    static void ParseVARIANT_PARSED(PARAMETER_LOG_INFOS* pParamLog,OUT TCHAR** ppszParam,TCHAR* FieldSplitter,DWORD NbRequieredChars);
    static void ParseVARIANT_PARSED(IN PARAMETER_LOG_INFOS* pParamLog,IN OUT TCHAR* pszBuffer,IN TCHAR* FieldSplitter,IN DWORD NbRequieredChars,OUT DWORD* pNbNeededChars);
    static void GetSafeArrayFromStackDefaultParsing(HANDLE LogHeap,IN SAFEARRAY* pSafeArray,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo,DWORD* pRequieredMemorySize);
    static BOOL GetSafeArrayFromStack(HANDLE LogHeap,IN SAFEARRAY* pSafeArray,DWORD NbPointedItems,IN OUT PBYTE pBuffer,OUT DWORD* pRequieredMemorySize,OUT DWORD* pNbItems);
    static void GetSafeArrayFromStackEx(HANDLE LogHeap,IN SAFEARRAY* pSafeArray,DWORD PointedDataSize,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo,DWORD* pRequieredMemorySize);
    static void ParseSAFEARRAY_PARSED(PARAMETER_LOG_INFOS* pParamLog,OUT TCHAR** ppszParam,TCHAR* FieldSplitter,DWORD NbRequieredChars);
    static void ParseSAFEARRAY_PARSED(IN PARAMETER_LOG_INFOS* pParamLog,IN OUT TCHAR* pszBuffer,IN TCHAR* FieldSplitter,IN DWORD NbRequieredChars,OUT DWORD* pNbNeededChars);
    static void GetVARIANT_TYPEString(IN VARTYPE VarType,IN OUT TCHAR* psz);
    static void ParseEXCEPINFO(PVOID pData,OUT TCHAR* StringRepresentation);
    static void ParseEXCEPINFO_PARSED(PARAMETER_LOG_INFOS* pParamLog,OUT TCHAR** ppszParam,TCHAR* FieldSplitter,DWORD NbRequieredChars);
    static void ParseEXCEPINFO_PARSED(IN PARAMETER_LOG_INFOS* pParamLog,IN OUT TCHAR* pszBuffer,IN TCHAR* FieldSplitter,IN DWORD NbRequieredChars,OUT DWORD* pNbNeededChars);
    static void GetExcepinfoFromStackDefaultParsing(HANDLE LogHeap,IN EXCEPINFO* pExcepinfo,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo,DWORD* pRequieredMemorySize);
    static void GetExcepinfoFromStackEx(HANDLE LogHeap,IN EXCEPINFO* pExcepinfo,DWORD PointedDataSize,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo,DWORD* pRequieredMemorySize);
    static BOOL GetExcepinfoFromStack(HANDLE LogHeap,IN EXCEPINFO* pExcepinfo,DWORD PointedDataSize,IN OUT PBYTE pBuffer,OUT DWORD* pRequieredMemorySize,OUT DWORD* pNbItems);
    static void ParseDISPPARAMS_PARSED(IN PARAMETER_LOG_INFOS* pParamLog,IN OUT TCHAR* pszBuffer,IN TCHAR* FieldSplitter,IN DWORD NbRequieredChars,OUT DWORD* pNbNeededChars);
    static void ParseDISPPARAMS_PARSED(PARAMETER_LOG_INFOS* pParamLog,OUT TCHAR** ppszParam,TCHAR* FieldSplitter,DWORD NbRequieredChars);
    static void ParseDISPPARAMS(PVOID pData,OUT TCHAR* StringRepresentation);
    static void GetDispparamsFromStackDefaultParsing(HANDLE LogHeap,IN DISPPARAMS* pDispParam,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo);


    static PBYTE ReportErrorUserParam;
    static pfReportError ReportError;
    static TCHAR ApplicationPath[MAX_PATH];
    static TCHAR UserTypePath[MAX_PATH];
    static TCHAR DefinePath[MAX_PATH];
    static void GetPaths();
    static void DefineToString(TCHAR* const ModuleName,PARAMETER_LOG_INFOS* const pParamLog,TCHAR** pszParameter,DWORD const NbRequieredChars);
    static void UserDataTypeToString(TCHAR* const ModuleName,PARAMETER_LOG_INFOS* const pParamLog,TCHAR** pszParameter,DWORD const NbRequieredChars);
public:
    static int SecureStrlen(char* pc);
    static int SecureWstrlen(wchar_t* pc);
    static void GetSafeArrayFromStack(HANDLE LogHeap,IN SAFEARRAY* pSafeArray,DWORD PointedDataSize,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo);
    static void GetVariantFromStack(HANDLE LogHeap,IN VARIANT* pVariant,DWORD PointedDataSize,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo);
    static void GetExcepinfoFromStack(HANDLE LogHeap,IN EXCEPINFO* pExcepinfo,DWORD PointedDataSize,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo);
    static void GetDispparamsFromStack(HANDLE LogHeap,IN DISPPARAMS* pDispParam,DWORD PointedDataSize,IN BOOL IsPointedValue,IN OUT PARAMETER_LOG_INFOS* pParamLogInfo);
    static DWORD GetParamType(TCHAR* pszParameter);
    static TCHAR* GetParamName(DWORD ParamType);
    static DWORD GetParamPointedSize(DWORD ParamType);
    static DWORD GetParamStackSize(DWORD ParamType);
    static DWORD GetParamRealSize(DWORD ParamType);
    static SUPPORTED_PARAMETERS_STRUCT* GetParamInfos(DWORD ParamType);
    static SUPPORTED_PARAMETERS_STRUCT* GetParamInfos(TCHAR* pszParameter);
    static BOOL IsParamPassedByRegisterWithFastcall(DWORD ParamType);
    static void __fastcall ParameterToString(TCHAR* const ModuleName,PARAMETER_LOG_INFOS* const pParamLog, TCHAR** pszParameter,BOOL const bParseDefinesAndUserDataType);
    static void __fastcall ParameterToString(TCHAR* const ModuleName,PARAMETER_LOG_INFOS* const pParamLog, TCHAR** pszParameter,DWORD const NbRequieredChars,BOOL const bParseDefinesAndUserDataType);
    static void __fastcall ParameterToString(TCHAR* const ModuleName,PARAMETER_LOG_INFOS* const pParamLog, TCHAR** pszParameter,DWORD const NbRequieredChars,BOOL const bParseDefinesAndUserDataType,BOOL const bDisplayVarName);
    static void StrReplace(TCHAR* pszInputString,TCHAR* pszOutputString,TCHAR* pszOldStr,TCHAR* pszNewStr);
    static void FreeSplittedParameterFields(CLinkListSimple** ppLinkListSimple);
    static BOOL SplitParameterFields(TCHAR* pszParameter,DWORD ParamType,CLinkListSimple** ppLinkListSimple);
    static void SetErrorReport(pfReportError const ErrorReport,IN PBYTE const UserParam);

    static void ClearUserDataTypeCache();
};

typedef struct tagPROCESSENTRY32A
{
    DWORD   dwSize;
    DWORD   cntUsage;
    DWORD   th32ProcessID;          // this process
    ULONG_PTR th32DefaultHeapID;
    DWORD   th32ModuleID;           // associated exe
    DWORD   cntThreads;
    DWORD   th32ParentProcessID;    // this process's parent process
    LONG    pcPriClassBase;         // Base priority of process's threads
    DWORD   dwFlags;
    CHAR    szExeFile[MAX_PATH];    // Path
} PROCESSENTRY32A;

typedef struct tagMODULEENTRY32A
{
    DWORD   dwSize;
    DWORD   th32ModuleID;       // This module
    DWORD   th32ProcessID;      // owning process
    DWORD   GlblcntUsage;       // Global usage count on the module
    DWORD   ProccntUsage;       // Module usage count in th32ProcessID's context
    BYTE  * modBaseAddr;        // Base address of module in th32ProcessID's context
    DWORD   modBaseSize;        // Size in bytes of module starting at modBaseAddr
    HMODULE hModule;            // The hModule of this module in th32ProcessID's context
    char    szModule[MAX_MODULE_NAME32 + 1];
    char    szExePath[MAX_PATH];
} MODULEENTRY32A;

// define non standard type
#ifndef __SECHANDLE_DEFINED__
typedef struct _SecHandle
{
    ULONG_PTR dwLower ;
    ULONG_PTR dwUpper ;
} SecHandle, * PSecHandle ;

#define __SECHANDLE_DEFINED__
#endif // __SECHANDLE_DEFINED__
