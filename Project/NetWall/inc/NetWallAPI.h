
#ifndef __NETWALLAPI_H__
#define __NETWALLAPI_H__

#include "comm.h"

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the NETWALLDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// NETWALLDLL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef NETWALLDLL_EXPORTS
#define NETWALLDLL_API __declspec(dllexport)
#else
#define NETWALLDLL_API __declspec(dllimport)
#endif

// This class is exported from the NetWall Dll.dll
class NETWALLDLL_API CNetWallAPI {
private:
    //ADAPTERS_INFO   m_AdaptersInfo;

public:
	CNetWallAPI();
    virtual ~CNetWallAPI();
	
public:
    BOOL GetDriverAPIVersion(HANDLE hIMHandle, PULONG pVersion);
    BOOL GetDllAPIVersion(PULONG pVersion);

    BOOL GetDriverDescriptionW(HANDLE hIMHandle, LPWSTR lpBuffer, LPDWORD nSize);
    BOOL GetDriverDescriptionA(HANDLE hIMHandle, LPSTR lpBuffer, LPDWORD nSize);

#ifdef UNICODE
#define GetDriverDescription    GetDriverDescriptionW
#else
#define GetDriverDescription    GetDriverDescriptionA
#endif // !UNICODE

    BOOL EnumerateBindings(HANDLE hWDMHandle, PWCHAR pBuffer, PUINT pBufferSize);
    
    BOOL   SetRule(HANDLE hWDMHandle, LPVOID pRuleItem, DWORD dwRuleSize);
    BOOL   ClearRule(HANDLE hWDMHandle);

    HANDLE OpenVirtualAdapterW(PWSTR pszAdapterName);
    HANDLE OpenVirtualAdapterA(PSTR pszAdapterName);

    HANDLE OpenLowerAdapterW(PWSTR pszAdapterName);
    HANDLE OpenLowerAdapterA(PSTR pszAdapterName);

#ifdef UNICODE
#define OpenVirtualAdapter   OpenVirtualAdapterW
#define OpenLowerAdapter     OpenLowerAdapterW
#else
#define OpenVirtualAdapter   OpenVirtualAdapterA
#define OpenLowerAdapter     OpenLowerAdapterA
#endif // !UNICODE

private:
    HANDLE OpenAdapter(PVOID pszAdapterName, BOOLEAN bIsWideChar, BOOLEAN bIsVirtualAdapter);

    // LogPrint Routine
public:
    BOOL StartLogPrint(HANDLE hWDMHandle);
    BOOL CloseLogPrint(HANDLE hWDMHandle);

    BOOL LoadLogFile(LPCTSTR lpszLogFileName, LPVOID * lpMapAddr, DWORD * dwLogFileSize, HANDLE * hLogFile, HANDLE * hMapFile, LPTSTR lpszError);
};


#endif // __NETWALLAPI_H__