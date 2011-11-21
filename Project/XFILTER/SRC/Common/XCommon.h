#ifndef XCOMMON_H
#define XCOMMON_H

#include <Winnetwk.h>

#include "ColorStatic.h"
#include "guires.h"

#ifdef XF_COMMON_FUNCTION

//
// 2002/08/21 add
//
#define WINDOWS_VERSION_NONE		0
#define WINDOWS_VERSION_2000		1
#define WINDOWS_VERSION_XP			2
#define WINDOWS_VERSION_9X			3
#define WINDOWS_VERSION_NT			4
int GetWindowsVersion();


BOOL IsWin9x();
int	WINAPI GetBit(BYTE bit, int index, int count = 1);
int	WINAPI SetBit(BYTE* bit, int index, BOOL isTrue);
CString	WINAPI DIPToSIP(DWORD* pIP);

CString	WINAPI GetAppPath(
			BOOL IsDLL = FALSE, 
			HINSTANCE instance = NULL,  
			BOOL IsFullPathName = FALSE
			);

CString WINAPI GetPath(TCHAR *sFilename);
CString WINAPI GetName(TCHAR *sFilename, BOOL IsIncludeExt = FALSE);
SECURITY_ATTRIBUTES *GetSecurityAttributes();


#define DRIVER_NAME		_T("\\\\.\\XPACKET")
#define DRIVER_NAME_NT	_T("XPACKET")
extern HANDLE m_DriverHandle;

HANDLE XF_OpenDriver();
BOOL XF_AddSpiPort(WORD wPort);
BOOL XF_DeleteSpiPort(WORD wPort);
BOOL XF_ReadBufferPoint(PPACKET_BUFFER_POINT pBufferPoint);
BOOL XF_ReadDirectionPoint(PDIRECTION_POINT pDirectionPoint);
BOOL XF_AddNetBiosName(char* pName, int nLenth);
BOOL XF_SetXfilterHandle();
BOOL XF_SetFilterMode(BOOL bFilterMode);
PNAME_LIST XF_GetNameList();
DWORD XF_GetNameListEx(char* pBuffer, DWORD nSize);
DWORD XF_GetIpFromName(char* Name);
BOOL XF_GetNameFromIp(DWORD dwIp, char* Name);
BOOL GetNameFromIp(DWORD dwIp, char* pBuffer);
BOOL CheckGuid();

//
// 2002/08/21 add
// call xpacket.sys to refresh hook send
//
BOOL XF_RefreshHookSend();


HANDLE XF_CreateFileMapping(
	HANDLE hFile,                       
	LPSECURITY_ATTRIBUTES lpAttributes, 
	DWORD flProtect,                    
	DWORD dwMaximumSizeHigh,            
	DWORD dwMaximumSizeLow,             
	LPCTSTR lpName                      
);

LPVOID XF_MapViewOfFile(
	HANDLE hFileMappingObject,  
	DWORD dwDesiredAccess,      
	DWORD dwFileOffsetHigh,     
	DWORD dwFileOffsetLow,      
	SIZE_T dwNumberOfBytesToMap,
	DWORD bIsCreatedProcess = 0
);

BOOL XF_UnmapViewOfFile(  
	HANDLE hFileMappingObject,  
	DWORD dwIoControl, // 2002/08/20 add
	LPCVOID lpBaseAddress
);

#endif //XF_COMMON_FUNCTION



#ifdef XF_GUI_COMMON_FUNCTION
void WINAPI AddComboStrings(CComboBox* pCombo, TCHAR** pString, int nCount);
void WINAPI AddListStrings(CListBox* pList, TCHAR** pString, int nCount);
void WINAPI AddListHead(CListCtrl* pListCtrl, TCHAR** pString, int nCount, int* ppLenth = NULL);
void WINAPI AddTreeList(CTreeCtrl* pTreeCtrl, TCHAR** pString, int nCount);
int WINAPI TextToIndex(CString sText, TCHAR** pString, int nCount);
int WINAPI AddList(
			CListCtrl *pList, 
			const TCHAR** pString, 
			int nCount, 
			BOOL bIsSelect = FALSE, 
			BOOL bIsEdit = FALSE, 
			int iIndex = -1, 
			int iIcon = -1
			);

VOID
IcmpTypeToString(
	IN	BYTE		bIcmpType,
	IN	BYTE		bSubCode,
	OUT	PCHAR		sIcmpType
);

void AddApp(CListCtrl* pList, PPACKET_LOG pLog, int nMaxCount = 1000, BOOL IsSeleted = TRUE, BOOL IsShowDate = FALSE);
void AddNnb(CListCtrl* pList, PPACKET_LOG pLog, int nMaxCount = 1000, BOOL IsSeleted = TRUE, BOOL IsShowDate = FALSE);
void AddIcmp(CListCtrl* pList, PPACKET_LOG pLog, int nMaxCount = 1000, BOOL IsSeleted = TRUE, BOOL IsShowDate = FALSE);
void AddMonitor(CListCtrl *pList, LPCTSTR* pString, int nCount, int nMaxCount = 1000, BOOL IsSeleted = TRUE);

#endif// XF_GUI_COMMON_FUNCTION



#ifdef XF_DLL_COMMON_FUNCTION
#endif // XF_DLL_COMMON_FUNCTION

#endif