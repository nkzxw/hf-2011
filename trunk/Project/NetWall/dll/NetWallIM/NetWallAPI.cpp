/**
 * Copyright(c) 2003  NetWall Technology Co.,Ltd.
 *
 * Module Name  : $Id: NetWallAPI.cpp,v 1.0.0 2003/03/28 01:45:04 yjchen Exp $
 *
 * Abstract     : NDIS Intermediate Driver (NETWALL) Win32 API. 
 *
 * Author       : yjchen
 *
 * Environment  : Windows 2K
 *
 * Function List:
 *
 *   DllMain                    - The entry point for the DLL application.
 *   GetDriverAPIVersion        - Call the NETWALL driver to get its API version.
 *   GetDllAPIVersion           - Call the NetWall DLL to get its API version
 *   GetDriverDescriptionW      - Call the NetWall driver to get its description string(UNICODE).
 *   GetDriverDescriptionA      - Call the NetWall driver to get its description string(ANSI).
 *   EnumerateBindings          - Call the NetWall driver to enumerate binding information.
 *   OpenVirtualAdapterW        - Call the NetWall driver to open a adapter.
 *   OpenVirtualAdapterA        - Call the NetWall driver to open a adapter.
 *   OpenLowerAdapterW          - Call the NetWall driver to open a adapter.
 *   OpenLowerAdapterA          - Call the NetWall driver to open a adapter.
 *   OpenAdapter                - Helper Routine, Call the NetWall driver to open a adapter.
 *   SetRule                    - Set Rule to a Adapter
 *   ClearRule                  - Clear Rule from a adapter
 *   StartLogPrint              - Start LogPrint
 *   CloseLogPrint              - Close LogPrint
 *
 * Revision History:
 *
 *   28-Mar-2003	1.0.0		Initial version
 *   
 *   
 */
#include "stdafx.h"
#include "NetWallAPI.h"

/**
 * Routine Description:
 *
 *   Called when this DLL is accessed by a new process or thread
 *
 * Parameters:
 *
 *	 hModule                - Handle of this DLL
 *   ul_reason_for_call     - Reason for call
 *   lpReserved             - Not use
 *
 * Return Value:
 *
 *   The status of the operation.
 */
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


/*
 * This is the constructor of a class that has been exported.
 * see NetWallComm.h for the class definition
 */
CNetWallAPI::CNetWallAPI()
{ 
	return; 
}

CNetWallAPI::~CNetWallAPI()
{
    return;
}

/**
 * Routine Description:
 *
 *   Call the NETWALL driver to get its API version.
 *    
 * Parameters:
 *
 *   hIMHandle      - Device Handle
 *   pVersion       - Version number
 *
 * Return Value
 *
 *   TRUE   - If the function suceeds, the return value is nonzero.
 *   FALSE  - If the function fails, the return value is zero.To get extended error
 *            information, call GetLastError.
 */ 
BOOL CNetWallAPI::GetDriverAPIVersion(HANDLE hIMHandle, PULONG pVersion)
{
    BOOLEAN     bRc;
    ULONG       bytesReturned;

    //
    // Call Driver To Fetch Driver API Version
    //
    bRc = DeviceIoControl(hIMHandle, 
                         (DWORD)IOCTL_NETWALL_GET_API_VERSION, 
                         pVersion, 
                         sizeof(ULONG),
                         pVersion,
                         sizeof(ULONG),
                         &bytesReturned,
                         NULL);
    return(bRc);
}

/**
 * Routine Description:
 *
 *   Call the NetWall DLL to get its API version
 *
 * Parameters:
 *
 *   hIMHandle      - 
 *   pVersion       - 
 *
 * Return Value
 *
 *   TRUE   - If the function suceeds, the return value is nonzero.
 *   FALSE  - If the function fails, the return value is zero.To get extended error
 *            information, call GetLastError.
 */ 
BOOL CNetWallAPI::GetDllAPIVersion(PULONG pVersion)
{
   *pVersion = NETWALL_API_VERSION;

   return(TRUE);
}

/**
 * Routine Description:
 *
 *   Call the NetWall driver to get its description string.
 *
 * Parameters:
 *
 *   hIMHandle      - 
 *   pVersion       - 
 *
 * Return Value
 *
 *   TRUE   - If the function suceeds, the return value is nonzero.
 *   FALSE  - If the function fails, the return value is zero.To get extended error
 *            information, call GetLastError.
 */ 
BOOL CNetWallAPI::GetDriverDescriptionW(HANDLE     hIMHandle,
                                        LPWSTR     lpBuffer,
                                        LPDWORD    nSize)
{
    BOOLEAN     bRc;
    ULONG       bytesReturned = 0;

    //
    // Call Driver To Fetch Driver Description
    //
    bRc = DeviceIoControl(hIMHandle, 
                         (DWORD)IOCTL_NETWALL_GET_DRIVER_DESCRIPTION,
                         NULL, 
                         0,
                         lpBuffer,
                         *nSize,
                         &bytesReturned,
                         NULL 
                         );

    *nSize = bytesReturned;

    return(bRc);
}

/**
 * Routine Description:
 *
 *   Call the NetWall driver to get its description string.
 *
 * Parameters:
 *
 *   hIMHandle      - 
 *   pVersion       - 
 *
 * Return Value
 *
 *   TRUE   - If the function suceeds, the return value is nonzero.
 *   FALSE  - If the function fails, the return value is zero.To get extended error
 *            information, call GetLastError.
 */ 
BOOL CNetWallAPI::GetDriverDescriptionA(HANDLE     hIMHandle, 
                                        LPSTR      lpBuffer, 
                                        LPDWORD    nSize)
{
    BOOLEAN  bRc;
    ULONG    bytesReturned = 0, nWideSize;
    LPWSTR   pWideBuffer = NULL;

    //
    // Allocate A Wide Character Buffer Of Same Size as MBCS Buffer
    //
    nWideSize = sizeof(WCHAR) * (*nSize);

    pWideBuffer = (LPWSTR)new CHAR[nWideSize];

    assert(NULL != pWideBuffer);
    //pWideBuffer = (LPWSTR)malloc(nWideSize);

    if (NULL == pWideBuffer)
    {
        *nSize = 0;
        return(FALSE);
    }

    //
    // Get Wide Character Driver Description
    //
    bRc = GetDriverDescriptionW(hIMHandle, pWideBuffer, &nWideSize);

    if (bRc)
    {
        //
        // Convert From Wide Character To MBCS
        //
        *nSize = wcstombs(lpBuffer, pWideBuffer, nWideSize);
    }
    else
    {
        *nSize = 0;
    }

    //
    // Free The Wide Character Buffer
    //
    //free(pWideBuffer);
    delete [](PCHAR)pWideBuffer;

    return(bRc);
}

/**
 * Routine Description:
 *
 *   Call the NetWall driver to enumerate binding information.
 *
 * Parameters:
 *
 *   hIMHandle          - Handle of Adapter
 *   
 * Return Value
 *
 *   TRUE   - If the function suceeds, the return value is nonzero.
 *   FALSE  - If the function fails, the return value is zero.To get extended error
 *            information, call GetLastError.
 */ 
BOOL CNetWallAPI::EnumerateBindings(HANDLE hWDMHandle, PWCHAR pBuffer, PUINT pBufferSize)
{
    ULONG   bytesReturned = 0;
    
    assert(INVALID_HANDLE_VALUE != hWDMHandle);
    
    //
    // Sanity Checks
    //
    if (hWDMHandle == INVALID_HANDLE_VALUE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        
        return FALSE;
    }
        
    if (! DeviceIoControl(hWDMHandle,
                          IOCTL_NETWALL_ENUM_ADAPTERS,
                          NULL,
                          0,
                          pBuffer,
                          *pBufferSize,
                          &bytesReturned,
                          NULL))
    {
        *pBufferSize = bytesReturned;
        return FALSE;
    }

    return TRUE;
}

/**
 * Routine Description:
 *
 *   Call the NetWall driver to open a adapter.
 *
 * Parameters:
 *
 *   pszAdapterName     - Adapter's Name String
 *   bIsWideChar        - Whether is Wide Char or not
 *   bIsVirtualAdapter  - Whether is Virtual or not
 *
 * Return Value
 *
 *   HANDLE   - Handle of be opened adapter
 */ 
HANDLE CNetWallAPI::OpenAdapter(PVOID      pszAdapterName,
                                BOOLEAN    bIsWideChar,
                                BOOLEAN    bIsVirtualAdapter)
{
    HANDLE            hAdapt;
    BOOLEAN           bRc;
    ULONG             bytesReturned, nBufferLength;
    PIM_OPEN_ADAPTER  pOpenAdapt = NULL;
    PWSTR             pszNameBuffer;

    hAdapt = INVALID_HANDLE_VALUE;

    //
    // Open The Device Handle
    //
    //_asm int 3;
    hAdapt = CreateFile(NETWALL_WDM_DEVICE_FILENAME,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hAdapt == INVALID_HANDLE_VALUE)
    {
        return(INVALID_HANDLE_VALUE);
    }

    //
    // Allocate Structure For Passing Adapter Name To Driver
    //
    if (bIsWideChar)
    {
        nBufferLength = sizeof(IM_OPEN_ADAPTER)
                        + (wcslen((PWSTR)pszAdapterName) + 1) * sizeof(WCHAR);
    }
    else
    {
        nBufferLength = sizeof(IM_OPEN_ADAPTER)
                        + (strlen((PSTR)pszAdapterName) + 1) * sizeof(WCHAR);
    }

    pOpenAdapt = (PIM_OPEN_ADAPTER)new CHAR[nBufferLength];
    
    assert(NULL != pOpenAdapt);

    //pOpenAdapterPB = (PSIM_OPEN_ADAPTER )malloc( nBufferLength );

    if (pOpenAdapt == NULL)
    {
        CloseHandle(hAdapt);

        return(INVALID_HANDLE_VALUE);
    }

    //
    // Initialize Structure For Passing Adapter Name To Driver
    //
    pOpenAdapt->m_nOpenStatus = NDIS_STATUS_PENDING;
    pszNameBuffer = (PWSTR)&pOpenAdapt[1];

    if (bIsWideChar)
    {
        wcscpy(pszNameBuffer, (PWSTR)pszAdapterName);
    }
    else
    {
        MultiByteToWideChar(CP_ACP,        // ASCII Code Page
                            0,
                            (PSTR)pszAdapterName,
                            -1,
                            pszNameBuffer,
                            nBufferLength - sizeof(IM_OPEN_ADAPTER)
                            );
    }

    //
    // Call Driver To Make Open The Adapter Context
    //
    bRc = DeviceIoControl(hAdapt, 
                          bIsVirtualAdapter 
                            ? (DWORD)IOCTL_NETWALL_OPEN_VIRTUAL_ADAPTER
                            : (DWORD)IOCTL_NETWALL_OPEN_LOWER_ADAPTER, 
                          pOpenAdapt, 
                          nBufferLength,
                          pOpenAdapt, 
                          nBufferLength,
                          &bytesReturned,
                          NULL);

    //
    // Check Results
    //
	if (! bRc)
	{
        CloseHandle(hAdapt);

        delete [] (PCHAR)pOpenAdapt;
        return(INVALID_HANDLE_VALUE);
    }

    //
    // I/O Successful. Now Check NDIS Status
    //
    if (pOpenAdapt->m_nOpenStatus != NDIS_STATUS_SUCCESS)
    {
        SetLastError(pOpenAdapt->m_nOpenStatus);

        CloseHandle(hAdapt);

        delete [] (PCHAR)pOpenAdapt;
        return(INVALID_HANDLE_VALUE);
    }

    delete [] (PCHAR)pOpenAdapt;
    return(hAdapt);     // Success
}

/**
 * Routine Description:
 *
 *   Call the NetWall driver to open a adapter.
 *
 * Parameters:
 *
 *   pszAdapterName     - Virtual Adapter's Wide Char String
 *   
 * Return Value
 *
 *   HANDLE   - Handle of be opened adapter
 */ 
HANDLE CNetWallAPI::OpenVirtualAdapterW(PWSTR pszAdapterName)
{
    return OpenAdapter((PVOID)pszAdapterName, TRUE, TRUE);
}

/**
 * Routine Description:
 *
 *   Call the NetWall driver to open a adapter.
 *
 * Parameters:
 *
 *   pszAdapterName     - Virtual Adapter's Ansi Char String
 *   
 * Return Value
 *
 *   HANDLE   - Handle of be opened adapter
 */ 
HANDLE CNetWallAPI::OpenVirtualAdapterA(PSTR pszAdapterName)
{
    return OpenAdapter((PVOID)pszAdapterName, FALSE, TRUE);
}

/**
 * Routine Description:
 *
 *   Call the NetWall driver to open a adapter.
 *
 * Parameters:
 *
 *   pszAdapterName     - Lower Adapter's Wide Char String
 *   
 * Return Value
 *
 *   HANDLE   - Handle of be opened adapter
 */ 
HANDLE CNetWallAPI::OpenLowerAdapterW(PWSTR pszAdapterName)
{
    return OpenAdapter((PVOID)pszAdapterName, TRUE, FALSE);
}

/**
 * Routine Description:
 *
 *   Call the NetWall driver to open a adapter.
 *
 * Parameters:
 *
 *   pszAdapterName     - Lower Adapter's Ansi Char String
 *   
 * Return Value
 *
 *   HANDLE   - Handle of be opened adapter
 */ 
HANDLE CNetWallAPI::OpenLowerAdapterA(PSTR pszAdapterName)
{
    return OpenAdapter((PVOID)pszAdapterName, FALSE, FALSE);
}

/**
 * Routine Description:
 *
 *   Call the NetWall driver to Set Rule to a adapter.
 *
 * Parameters:
 *
 *   hWDMHandle     - Handle of be opened NetWall WDM
 *   pRuleItem      - Pointer to Rule Items
 *   dwRuleSize     - Size of Rule Items
 *   
 * Return Value
 *
 *   TRUE if set Success, otherwise FALSE
 */ 
BOOL CNetWallAPI::SetRule(HANDLE hWDMHandle, LPVOID pRuleItem, DWORD dwRuleSize)
{
    BOOL    bRet = FALSE;
    DWORD   bytesReturned;

    assert(INVALID_HANDLE_VALUE != hWDMHandle);
    assert(NULL != pRuleItem && 0 < dwRuleSize);
    
    bRet = DeviceIoControl(hWDMHandle, 
                           (DWORD)IOCTL_NETWALL_SET_FILTER, 
                           pRuleItem, 
                           dwRuleSize,
                           NULL,
                           0,
                           &bytesReturned,
                           NULL 
                          );
    return bRet;
}

/**
 * Routine Description:
 *
 *   Call the NetWall driver to Clear Rule From adapter.
 *
 * Parameters:
 *
 *   hWDMHandle     -  Handle of be opened NetWall WDM
 *   
 * Return Value
 *
 *   TRUE if clear Success, otherwise FALSE
 */ 
BOOL CNetWallAPI::ClearRule(HANDLE hWDMHandle)
{
    BOOL    bRet = FALSE;
    DWORD   bytesReturned;
    
    assert(INVALID_HANDLE_VALUE != hWDMHandle);

    bRet = DeviceIoControl(hWDMHandle, 
                           (DWORD)IOCTL_NETWALL_CLEAR_FILTER, 
                           NULL, 
                           0,
                           NULL,
                           0,
                           &bytesReturned,
                           NULL 
                          );
    return bRet;
}

/**
 * Routine Description:
 *
 *   Call the NetWall driver to open LogPrint.
 *
 * Parameters:
 *
 *   hWDMHandle     -  Handle of be opened NetWall WDM
 *   
 * Return Value
 *
 *   TRUE if open Success, otherwise FALSE
 */ 
BOOL CNetWallAPI::StartLogPrint(HANDLE hWDMHandle)
{
    BOOL    bRet = FALSE;
    DWORD   bytesReturned;
    
    assert(INVALID_HANDLE_VALUE != hWDMHandle);
    
    bRet = DeviceIoControl(hWDMHandle, 
                           (DWORD)IOCTL_NETWALL_OPEN_LOGPRINT, 
                           NULL, 
                           0,
                           NULL,
                           0,
                           &bytesReturned,
                           NULL 
                          );
    return bRet;
}

/**
 * Routine Description:
 *
 *   Call the NetWall driver to CLose LogPrint.
 *
 * Parameters:
 *
 *   hWDMHandle     -  Handle of be opened NetWall WDM
 *   
 * Return Value
 *
 *   TRUE if close Success, otherwise FALSE
 */ 
BOOL CNetWallAPI::CloseLogPrint(HANDLE hWDMHandle)
{
    BOOL    bRet = FALSE;
    DWORD   bytesReturned;
    
    assert(INVALID_HANDLE_VALUE != hWDMHandle);
    
    bRet = DeviceIoControl(hWDMHandle, 
                           (DWORD)IOCTL_NETWALL_CLOSE_LOGPRINT, 
                           NULL, 
                           0,
                           NULL,
                           0,
                           &bytesReturned,
                           NULL 
                          );
    return bRet;
}

/**
 * Routine Description:
 *
 *   Load LogFile into Process's Address Space.
 *
 * Parameters:
 *
 *   IN  lpszLogFileName    - Log File Name(Include Full Path)
 *   OUT lpMapAddr          - Start Address of Map File
 *   OUT dwLogFileSize      - Size of LogFile
 *   OUT hLogFile           - Handle of Log File
 *   OUT hMapFile           - Handle of Log Map File
 *   IN  lpszError          - Store Error Info
 *   
 * Return Value
 *
 *   TRUE if load Success, otherwise FALSE
 */ 
BOOL CNetWallAPI::LoadLogFile(LPCTSTR  lpszLogFileName, 
                             LPVOID * lpMapAddr, 
                             DWORD  * dwLogFileSize, 
                             HANDLE * hLogFile, 
                             HANDLE * hMapFile, 
                             LPTSTR   lpszError)
{    
    assert(lpszLogFileName);
    assert(lpszError);
    
    try
    {
        do 
        {        	                    
            // 1. 打开日志文件            
            *hLogFile = CreateFile(lpszLogFileName,
                                   GENERIC_READ, // | GENERIC_WRITE,
                                   0,
                                   NULL,
                                   OPEN_EXISTING, // Opens the file. The function fails if the file does not exist. 
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL
                                  );
        
            if (INVALID_HANDLE_VALUE == *hLogFile)
            {
                _stprintf(lpszError, _T("%s"), _T("打开日志文件失败!"));
                break;
            }

            // 2. 获得文件的大小            
            *dwLogFileSize = GetFileSize(*hLogFile, NULL);            
            if (INVALID_FILE_SIZE == *dwLogFileSize) 
            { 
                DWORD dwSizeHigh = 0;
                // Try to obtain hFile's huge size. 
                *dwLogFileSize = GetFileSize(*hLogFile, &dwSizeHigh); 
                
                // If we failed ... 
                if (*dwLogFileSize == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
                { 
                    _stprintf(lpszError, _T("获取日志文件的大小失败[0x08x]."), GetLastError());                      
                    break;                        
                } 
            }             

            // 5. 映射文件到系统内存                    
            *hMapFile = CreateFileMapping(*hLogFile,         // Current file handle. 
                                          NULL,              // Default security. 
                                          PAGE_READONLY,     // Read permission. 
                                          0,                 // Max. object size. 
                                          0,                 // Size of hFile. 
                                          _T("NetWallLogMap")// Name of mapping object.
                                         );      
        
            if (INVALID_HANDLE_VALUE != *hMapFile && ERROR_ALREADY_EXISTS == GetLastError()) 
            { 
                _stprintf(lpszError, _T("%s"), _T("日志文件已经映射到内存!"));
                break;
            } 
        
            if (INVALID_HANDLE_VALUE == *hMapFile) 
            { 
                _stprintf(lpszError, _T("%s"), _T("不能映射日志文件到系统内存!"));
                break;            
            } 
                    
            *lpMapAddr = MapViewOfFile(*hMapFile,       // Handle to mapping object. 
                                        FILE_MAP_READ,  // Read permission 
                                        0,              // Max. object size. 
                                        0,              // Size of hFile. 
                                        0               // Map entire file. 
                                       );                                
        
            if (NULL == *lpMapAddr) 
            { 
                _stprintf(lpszError, _T("%s"), _T("映射日志文件到系统内存失败!"));
                break;   
            } 

            return TRUE;

        } while (FALSE);

        if (INVALID_HANDLE_VALUE != *hMapFile) 
        { 
            CloseHandle(*hMapFile);  
            *hMapFile = INVALID_HANDLE_VALUE;
        } 
        
        if (INVALID_HANDLE_VALUE != *hLogFile)
        {
            CloseHandle(*hLogFile);   
            *hLogFile = INVALID_HANDLE_VALUE;
        }

        return FALSE;
    }
    catch (...)
    {
        _stprintf(lpszError, _T("%s"), _T("操作日志文件异常!"));
    }

    return FALSE;
}