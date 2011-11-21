//-----------------------------------------------------------
/*
	���̣�		�Ѷ����˷���ǽ
	��ַ��		http://www.xfilt.com
	�����ʼ���	xstudio@xfilt.com
	��Ȩ���� (c) 2002 ���޻�(�Ѷ���ȫʵ����)

	��Ȩ����:
	---------------------------------------------------
		�����Գ���������Ȩ���ı�����δ����Ȩ������ʹ��
	���޸ı����ȫ���򲿷�Դ���롣�����Ը��ơ����û�ɢ
	���˳���򲿷ֳ�������������κ�ԽȨ��Ϊ�����⵽��
	���⳥�����µĴ�������������������̷�����׷�ߡ�
	
		��ͨ���Ϸ�;�������Դ������(�����ڱ���)��Ĭ��
	��Ȩ�����Ķ������롢���ԡ������ҽ����ڵ��Ե���Ҫ��
	�����޸ı����룬���޸ĺ�Ĵ���Ҳ����ֱ��ʹ�á�δ��
	��Ȩ������������Ʒ��ȫ���򲿷ִ�������������Ʒ��
	������ת�����ˣ����������κη�ʽ���ƻ򴫲���������
	�����κη�ʽ����ҵ��Ϊ��	

    ---------------------------------------------------	
*/
//-----------------------------------------------------------
//
// ���ò�������
//
//

#include "stdafx.h"
#include "xcommon.h"

#ifdef XF_COMMON_FUNCTION

//
// �жϵ�ǰ����ϵͳ��Win9x����NT
//
BOOL IsWin9x()
{
	OSVERSIONINFO VerInfo;  
	VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&VerInfo);

	if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		return TRUE;
	else if(VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT 
		) // && (VerInfo.dwMajorVersion == 4 || VerInfo.dwMajorVersion == 5))
		return FALSE;
	return FALSE;
}

//
// 2002/08/21 add
// �õ���ǰ����ϵͳ�汾
//
int GetWindowsVersion()
{
	OSVERSIONINFO VerInfo;  
	VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&VerInfo);

	if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		return WINDOWS_VERSION_9X;
	else if(VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		if(VerInfo.dwMajorVersion == 4)
			return WINDOWS_VERSION_NT;
		else if(VerInfo.dwMajorVersion == 5 && VerInfo.dwMinorVersion == 0)
			return WINDOWS_VERSION_2000;
		else 
			return WINDOWS_VERSION_XP;
	}
	return WINDOWS_VERSION_NONE;
}

//
// ����ֵIPת��Ϊ�ַ���IP
//
CString WINAPI DIPToSIP(DWORD* pIP)
{
	if(pIP == NULL)
		return _T("");

	CString	s;
	BYTE	*b = (BYTE*)pIP;
	s.Format(_T("%d.%d.%d.%d"),b[3],b[2],b[1],b[0]);

	return s;
}

/*---------------------------------------------------------------------------------------------
	index from 0 start, for example:
	index:				0 1 2 3 4 5 6 7
	Binary value:		0 0 0 0 0 0 0 0
*/
int	WINAPI GetBit(BYTE bit, int index, int count)
{
	bit <<= index;
	bit >>= (8 - count);

	return bit;
}

//
// �����ֽ��е�һλ
//
int WINAPI SetBit(BYTE* bit, int index, BOOL isTrue)
{
	BYTE bOr = 0xFF,bAnd = 0x00;

	bOr <<= index;
	bOr >>= 7;
	bOr <<= (7 - index);
	bAnd = ~bOr;

	if(isTrue)
		*bit = *bit | bOr;
	else
		*bit = *bit & bAnd;

	return 0;
}

//
// �õ�Ӧ�ó���·��
//
CString WINAPI GetAppPath(BOOL IsDLL, HINSTANCE instance, BOOL IsFullPathName) 
{
	TCHAR sFilename[_MAX_PATH];
	TCHAR sDrive[_MAX_DRIVE];
	TCHAR sDir[_MAX_DIR];
	TCHAR sFname[_MAX_FNAME];
	TCHAR sExt[_MAX_EXT];

	if(IsDLL)
		GetModuleFileName(instance, sFilename, _MAX_PATH);
	else
		GetModuleFileName(AfxGetInstanceHandle(), sFilename, _MAX_PATH);

	if(IsFullPathName)
		return sFilename;

	_tsplitpath(sFilename, sDrive, sDir, sFname, sExt);

	CString rVal(CString(sDrive) + CString(sDir));
	int nLen = rVal.GetLength();

	if (rVal.GetAt(nLen-1) != _T('\\'))
		rVal += _T("\\");

	return rVal;
}

//
// ������·���з����·��
//
CString WINAPI GetPath(TCHAR *sFilename) 
{
	TCHAR sDrive[_MAX_DRIVE];
	TCHAR sDir[_MAX_DIR];
	TCHAR sFname[_MAX_FNAME];
	TCHAR sExt[_MAX_EXT];

	_tsplitpath(sFilename, sDrive, sDir, sFname, sExt);

	CString rVal(CString(sDrive) + CString(sDir));
	int nLen = rVal.GetLength();

	if (rVal.GetAt(nLen-1) != _T('\\'))
		rVal += _T("\\");

	return rVal;
}  

//
// ������·���з�����ļ���
//
CString WINAPI GetName(TCHAR *sFilename, BOOL IsIncludeExt) 
{
	TCHAR sDrive[_MAX_DRIVE];
	TCHAR sDir[_MAX_DIR];
	TCHAR sFname[_MAX_FNAME];
	TCHAR sExt[_MAX_EXT];

	_tsplitpath(sFilename, sDrive, sDir, sFname, sExt);

	CString rVal;
	if(IsIncludeExt)
		rVal.Format(_T("%s%s"), sFname, sExt);
	else 
		rVal.Format(_T("%s"), sFname);

	return rVal;
}

//
// ����һ����ȫ�����ļ�����CreateFileMappingʱʹ��
//
SECURITY_ATTRIBUTES *GetSecurityAttributes()
{
	static BOOL bIsInitialize = FALSE;
	static SECURITY_ATTRIBUTES SecurityAttributes;
	static SECURITY_DESCRIPTOR SecurityDescriptor;

	OSVERSIONINFO VerInfo;  
	VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&VerInfo);
	if(VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		return NULL;

	if(bIsInitialize) return &SecurityAttributes;

	if(!InitializeSecurityDescriptor(&SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION)
		|| !SetSecurityDescriptorDacl(&SecurityDescriptor, TRUE, NULL, FALSE))
		return NULL;

	SecurityAttributes.nLength = sizeof(SecurityAttributes); 
	SecurityAttributes.lpSecurityDescriptor	= &SecurityDescriptor;
	SecurityAttributes.bInheritHandle = FALSE; 

	bIsInitialize = TRUE;

	return &SecurityAttributes;
}


HANDLE m_DriverHandle = NULL;

//
// ��XPACKET.VXD��XPACKET.SYS
//
HANDLE XF_OpenDriver()
{
	if(m_DriverHandle != NULL)
		return m_DriverHandle;

	m_DriverHandle = CreateFile(DRIVER_NAME,
          GENERIC_READ | GENERIC_WRITE,
          0,
          0,
          CREATE_NEW,
          FILE_ATTRIBUTE_NORMAL,
          0
          );

	if(m_DriverHandle == (HANDLE)0xFFFFFFFF)
		m_DriverHandle = NULL;

	return m_DriverHandle;
}

//
// ֪ͨ������������SPI���˵Ķ˿�
//
BOOL XF_AddSpiPort(WORD wPort)
{
	HANDLE hHandle = XF_OpenDriver();
	if(hHandle == NULL)
		return FALSE;

	DWORD dwByteCount = 0;
	BOOL result = DeviceIoControl(hHandle, 
					IOCTL_XPACKET_ADD_SPI_PORT,  
					&wPort,					
					sizeof(wPort),	
					NULL,				
					0,						
					&dwByteCount,					
					NULL					
					);
	return result;
}

//
// ֪ͨ��������ɾ��SPI���˵Ķ˿�
//
BOOL XF_DeleteSpiPort(WORD wPort)
{
	HANDLE hHandle = XF_OpenDriver();
	if(hHandle == NULL)
		return FALSE;

	DWORD dwByteCount = 0;
	BOOL result = DeviceIoControl(hHandle, 
					IOCTL_XPACKET_DELETE_SPI_PORT,  
					&wPort,	
					sizeof(wPort),	
					NULL,				
					0,						
					&dwByteCount,					
					NULL					
					);
	return result;
}

//
// ֪ͨ�������򱣴�XFILTER.EXE�Ľ��̾��
//
BOOL XF_SetXfilterHandle()
{
	HANDLE hHandle = XF_OpenDriver();
	if(hHandle == NULL)
		return FALSE;

	DWORD dwByteCount = 0;
	BOOL result = DeviceIoControl(hHandle, 
					IOCTL_XPACKET_SET_XFILTER_HANDLE,  
					NULL,	
					0,	
					NULL,				
					0,						
					&dwByteCount,					
					NULL					
					);
	return result;
}

//
// ������������Ĺ���ģʽ
//
BOOL XF_SetFilterMode(BOOL bFilterMode)
{
	HANDLE hHandle = XF_OpenDriver();
	if(hHandle == NULL)
		return FALSE;

	DWORD dwFilterMode = bFilterMode;
	DWORD dwByteCount = 0;
	BOOL result = DeviceIoControl(hHandle, 
					IOCTL_XPACKET_SET_FILTER_MODE,  
					&dwFilterMode,	
					sizeof(dwFilterMode),	
					NULL,				
					0,						
					&dwByteCount,					
					NULL					
					);
	return result;
}

//
// �����������ȡ�����������ַ
//
BOOL XF_ReadBufferPoint(PPACKET_BUFFER_POINT pBufferPoint)
{
	HANDLE hHandle = XF_OpenDriver();
	if(hHandle == NULL)
		return FALSE;

	DWORD dwByteCount = 0;
	BOOL result = DeviceIoControl(hHandle, 
					IOCTL_XPACKET_GET_BUFFER_POINT,  
					NULL,					
					0,	
					pBufferPoint,				
					sizeof(PACKET_BUFFER_POINT),						
					&dwByteCount,					
					NULL					
					);

#ifdef DEBUG
	char buf[256];
	sprintf(buf, "Result: %d, MaxCount:%d, pPacket:0x%08X, ReadIndex:%d, WriteIndex:%d\n"
		, result
		, pBufferPoint->MaxCount
		, pBufferPoint->pPacket 
		, pBufferPoint->ReadIndex
		, pBufferPoint->WriteIndex
		);
	OutputDebugString(buf);
#endif

	return result;
}

//
// �����������ȡ��ǰ���ߵ����ݻ�����
//
BOOL XF_ReadDirectionPoint(PDIRECTION_POINT pDirectionPoint)
{
	HANDLE hHandle = XF_OpenDriver();
	if(hHandle == NULL)
		return FALSE;

	DWORD dwByteCount = 0;
	BOOL result = DeviceIoControl(hHandle, 
					IOCTL_XPACKET_GET_DIRECTION_POINT,  
					NULL,					
					0,	
					pDirectionPoint,				
					sizeof(DIRECTION_POINT),						
					&dwByteCount,					
					NULL					
					);

#ifdef DEBUG
	char buf[256];
	sprintf(buf, "Result: %d, DirectionCount:%d, pDirection:0x%08X, pDelete:0x%08X\n"
		, result
		, pDirectionPoint->DirectionCount 
		, pDirectionPoint->pDirection
		, pDirectionPoint->pDelete 
		);
	OutputDebugString(buf);
#endif

	return result;
}

//
// ֪ͨ�����������������ھ����ּ�¼
//
BOOL XF_AddNetBiosName(char* pName, int nLenth)
{
	HANDLE hHandle = XF_OpenDriver();
	if(hHandle == NULL)
		return FALSE;

	DWORD dwByteCount = 0;
	BOOL result =  DeviceIoControl(hHandle, 
						IOCTL_XPACKET_ADD_NETBIOS_NAME,  
						pName,					
						nLenth,	
						NULL,				
						0,						
						&dwByteCount,					
						NULL					
						);

	return result;
}

PNAME_LIST	m_pFirstNameList = NULL;

//
// ����������õ������ھ����ּ�¼�ĵ�ַ
//
PNAME_LIST XF_GetNameList()
{
	HANDLE hHandle = XF_OpenDriver();
	if(hHandle == NULL)
		return FALSE;

	if(m_pFirstNameList != NULL)
		return m_pFirstNameList;

	DWORD dwByteCount = 0;
	BOOL result =  DeviceIoControl(hHandle, 
						IOCTL_XPACKET_GET_NETBIOS_NAME,  
						NULL,					
						0,	
						&m_pFirstNameList,				
						sizeof(m_pFirstNameList),						
						&dwByteCount,					
						NULL					
						);
	return m_pFirstNameList;
}

//
// ����������õ������ھ����ֵ����м�¼
//
DWORD XF_GetNameListEx(char* pBuffer, DWORD nSize)
{
	HANDLE hHandle = XF_OpenDriver();
	if(hHandle == NULL)
		return FALSE;

	DWORD dwByteCount = 0;
	BOOL result =  DeviceIoControl(hHandle, 
						IOCTL_XPACKET_GET_NETBIOS_NAME_LIST,  
						NULL,					
						0,	
						pBuffer,				
						nSize,						
						&dwByteCount,					
						NULL					
						);
	return dwByteCount;
}

//
// ������������������ھӵ����ֵõ�IP
//
DWORD XF_GetIpFromName(char* Name)
{
	HANDLE hHandle = XF_OpenDriver();
	if(hHandle == NULL)
		return FALSE;

	DWORD dwIp = 0;
	DWORD dwByteCount = 0;
	BOOL result =  DeviceIoControl(hHandle, 
						IOCTL_XPACKET_GET_IP_FROM_NAME,  
						Name,					
						strlen(Name) + 1,	
						&dwIp,				
						sizeof(dwIp),						
						&dwByteCount,					
						NULL					
						);
	return dwIp;
}

//
// �������������IP�õ������ھӵ�����
//
BOOL XF_GetNameFromIp(DWORD dwIp, char* Name)
{
	HANDLE hHandle = XF_OpenDriver();
	if(hHandle == NULL)
		return FALSE;

	DWORD dwByteCount = 0;
	BOOL result =  DeviceIoControl(hHandle, 
						IOCTL_XPACKET_GET_NAME_FROM_IP,  
						&dwIp,					
						sizeof(dwIp),	
						Name,				
						sizeof(Name),						
						&dwByteCount,					
						NULL					
						);
	return result;
}

//
// 2002/08/21 add
// call xpacket.sys to refresh hook send
//
BOOL XF_RefreshHookSend()
{
	HANDLE hHandle = XF_OpenDriver();
	if(hHandle == NULL)
		return FALSE;

	DWORD dwByteCount = 0;
	BOOL result =  DeviceIoControl(hHandle, 
						IOCTL_XPACKET_REFRESH_HOOK_SEND,  
						&dwByteCount,					
						sizeof(dwByteCount),	
						NULL,				
						0,						
						&dwByteCount,					
						NULL					
						);
	return result;
}

//
// �������ھӵ����ֵõ�IP
//
BOOL GetNameFromIp(DWORD dwIp, char* pBuffer)
{
	BOOL bIsFind = FALSE;
	if(IsWin9x())
	{
		PNAME_LIST pList = m_pFirstNameList;
		if(pList == NULL)
			pList = XF_GetNameList();

		while(pList != NULL)
		{
			if(dwIp == pList->Address)
			{
				strcpy(pBuffer, pList->Name);
				bIsFind = TRUE;
				break;
			}
			pList = pList->pNext;
		}
		if(!bIsFind)
		{
			BYTE *pIp = (BYTE*)&dwIp;
			sprintf(pBuffer, "%u.%u.%u.%u", pIp[3], pIp[2], pIp[1], pIp[0]);
		}
	}
	else
	{
		pBuffer[0] = 0;
		XF_GetNameFromIp(dwIp, pBuffer);
		if(pBuffer[0] == 0)
			bIsFind = FALSE;
		else 
			bIsFind = TRUE;
	}
	return bIsFind;
}

DWORD m_pVoid = NULL;

//
// ����һ���ڴ�ӳ���ļ�������ֱ�Ӵ�XPACKET.VXD����XPACKET.SYS�����ڴ�ռ�
//
HANDLE XF_CreateFileMapping(
	HANDLE hFile,                       
	LPSECURITY_ATTRIBUTES lpAttributes, 
	DWORD flProtect,                    
	DWORD dwMaximumSizeHigh,            
	DWORD dwMaximumSizeLow,             
	LPCTSTR lpName                      
)
{
	//return CreateFileMapping(hFile
	//	, lpAttributes
	//	, flProtect
	//	, dwMaximumSizeHigh
	//	, dwMaximumSizeLow
	//	, lpName
	//	);

	static HANDLE hHandle;
	static DWORD dwSize = 0;

	SetLastError(ERROR_ALREADY_EXISTS);

	if(hHandle != NULL && dwSize == dwMaximumSizeLow) 
		return hHandle;
	hHandle = XF_OpenDriver();

	if(hHandle == NULL)
		return hHandle;

	DWORD dwByteCount = 0;
	BOOL result = 
		DeviceIoControl(hHandle, 
						IOCTL_XPACKET_MALLOC_ACL_BUFFER,  
						&dwMaximumSizeLow,					
						sizeof(dwMaximumSizeLow),	// sizeof buffer
						NULL,				
						0,						
						&dwByteCount,					
						NULL					
						);

	if(result == FALSE)
	{
		//CloseHandle(hHandle);
		return NULL;
	}

	m_pVoid = NULL;
	dwSize = dwMaximumSizeLow;
	return hHandle;
}

//
// �õ��ڴ�ӳ���ļ��Ļ�������ַ������ֱ�Ӵ�XPACKET.VXD����XPACKET.SYS�õ�
// �����ڴ�ĵ�ַ
//
LPVOID XF_MapViewOfFile(
	HANDLE hFileMappingObject,  
	DWORD dwDesiredAccess,      
	DWORD dwFileOffsetHigh,     
	DWORD dwFileOffsetLow,      
	SIZE_T dwNumberOfBytesToMap,
	DWORD bIsCreatedProcess
)
{
	//return MapViewOfFile(hFileMappingObject
	//	, dwDesiredAccess
	//	, dwFileOffsetHigh
	//	, dwFileOffsetLow
	//	, dwNumberOfBytesToMap
	//	);

	if(m_pVoid != NULL) 
		return (LPVOID)m_pVoid;

	DWORD dwByteCount = 0;
	BOOL result = 
		DeviceIoControl(hFileMappingObject, 
						IOCTL_XPACKET_GET_ACL_BUFFER,  
						&bIsCreatedProcess,					
						sizeof(bIsCreatedProcess),
						&m_pVoid,				
						sizeof(m_pVoid),						
						&dwByteCount,					
						NULL					
						);

	if (FALSE == result)
		return NULL;

	return (LPVOID)m_pVoid;
}

//
// �ͷ��ڴ�ӳ���ļ�������ֱ�ӵ���XPACKET.VXD����XPACKET.SYS�ͷ��ڴ�ռ�
//
BOOL XF_UnmapViewOfFile(  
	HANDLE hFileMappingObject,
	DWORD dwIoControl, // 2002/08/20 add
	LPCVOID lpBaseAddress
)
{
	//return UnmapViewOfFile(lpBaseAddress);

	if(hFileMappingObject == NULL) return FALSE;
	static BOOL IsFree = FALSE;

	//
	// 2002/08/20 modify
	//
	if(IsFree && dwIoControl == IOCTL_XPACKET_FREE_ACL_BUFFER) return IsFree; 

	DWORD dwByteCount = 0;
	IsFree = DeviceIoControl(hFileMappingObject, 
					dwIoControl, // 2002/08/20 remove IOCTL_XPACKET_FREE_ACL_BUFFER,  
					&lpBaseAddress,			// 2002/08/20 modify		
					sizeof(lpBaseAddress),	// 2002/08/20 modify
					NULL,				
					0,						
					&dwByteCount,					
					NULL					
					);
	return IsFree;
}

//
// ���Ӧ�ó���DLL����������֮��ı�������Ƿ���ͬ
//
BOOL CheckGuid()
{
	BOOL bReturn;
	CHECK_GUID(bReturn);
	return bReturn;
}

#endif //XF_COMMON_FUNCTION

//================================================================================
// ���������غ���
//

#ifdef XF_GUI_COMMON_FUNCTION

//
// Ϊ ComboBox �����ַ���
//
void WINAPI AddComboStrings(CComboBox* pCombo, TCHAR** pString, int nCount)
{
	int nItemCount = pCombo->GetCount();

	int i = 0;
	for(i; i < nItemCount; i ++)
	{
		pCombo->DeleteString(0);
	}
	for(i = 0; i < nCount; i++)
	{
		pCombo->AddString(pString[i]);
	}
}

//
// Ϊ ListBox ���Ӽ�¼
//
void WINAPI AddListStrings(CListBox* pList, TCHAR** pString, int nCount)
{
	int nItemCount = pList->GetCount();

	int i = 0;
	for(i; i < nItemCount; i ++)
	{
		pList->DeleteString(0);
	}
	for(i = 0; i < nCount; i++)
	{
		pList->AddString(pString[i]);
	}
}

//
// Ϊ ListCtrl ���ӱ�ͷ
//
void WINAPI AddListHead(CListCtrl* pListCtrl, TCHAR** pString, int nCount, int* ppLenth)
{
//	pListCtrl->SetBkColor(PASSECK_DIALOG_BKCOLOR);
//	pListCtrl->SetTextColor(COLOR_TEXT_NORMAL);

	pListCtrl->SetExtendedStyle(LVS_EX_FLATSB | LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);

	int nColumnCount = pListCtrl->GetHeaderCtrl()->GetItemCount();

	int i = 0;
	for (i; i < nColumnCount; i++)
	{
	   pListCtrl->DeleteColumn(0);
	}
	for(i = 0; i < nCount; i++)
	{
		pListCtrl->InsertColumn(i, pString[i], LVCFMT_LEFT, ppLenth == NULL ? strlen(pString[i])*CHAR_WIDTH : ppLenth[i]);
	}
}

//
// Ϊ TreeCtrl ���Ӽ�¼
//
void WINAPI AddTreeList(CTreeCtrl* pTreeCtrl, TCHAR** pString, int nCount)
{
	pTreeCtrl->SetBkColor(COLOR_TREE_BK);
	pTreeCtrl->SetTextColor(COLOR_TREE_TEXT);

	pTreeCtrl->DeleteAllItems();

	for(int i = 0; i< nCount; i++)
	{
		pTreeCtrl->InsertItem(pString[i]);
	}
}

//
// �õ��ַ������ַ��������е�����
//
int WINAPI TextToIndex(CString sText, TCHAR** pString, int nCount)
{
	for(int i = 0; i < nCount; i++)
	{
		if(sText.Compare(pString[i]) == 0) return i;
	}
	return -1;
}


//
// Ϊ ListCtrl ���Ӽ�¼
//
int WINAPI AddList(CListCtrl *pList, const TCHAR** pString
	, int nCount, BOOL bIsSelect, BOOL bIsEdit, int iIndex, int iIcon)
{
	if(pString == NULL) return -1;

	if(!bIsEdit)
	{
		iIndex = pList->GetItemCount();
		pList->InsertItem(iIndex, pString[0], iIcon);
	}

	for(int i = 0; i < nCount; i++)
	{
		pList->SetItemText(iIndex, i, pString[i]);
	}
	if(bIsSelect && !bIsEdit)
	{
		pList->EnsureVisible(iIndex, TRUE);
		pList->SetItemState(iIndex,	LVIS_SELECTED,LVIS_SELECTED);
	}

	return iIndex;
}

//
// ����ICMP������ת��Ϊ���������ַ���
//
static BYTE m_IcmpConst[] = {0xCB,0xD3,0xD2,0xEA,0xCB,0xB6,0xC9,0xAC,0xB1,0xB3,0xB7,0xFC,0xB8,0xD2,0x21,0x29,0x44,0x2A,0xD4,0xD1,0xCC,0xFA,0xC9,0xA9,0xB1,0xE7};
VOID
IcmpTypeToString(
	IN	BYTE		bIcmpType,
	IN	BYTE		bSubCode,
	OUT	PCHAR		sIcmpType
)
{
	sIcmpType[0] = 0;
	switch(bIcmpType)
	{
	case ICMP_ECHOREPLY:
		strcat(sIcmpType, "ICMP_ECHOREPLY(echo reply)");
		break;
	case ICMP_UNREACH:
		strcat(sIcmpType, "ICMP_UNREACH");
		switch(bSubCode)
		{
		case ICMP_UNREACH_NET:
			strcat(sIcmpType, "[ICMP_UNREACH_NET](bad net)");
			break;
		case ICMP_UNREACH_HOST:
			strcat(sIcmpType, "[ICMP_UNREACH_HOST](bad host)");
			break;
		case ICMP_UNREACH_PROTOCOL:
			strcat(sIcmpType, "[ICMP_UNREACH_HOST](bad protocol)");
		case ICMP_UNREACH_PORT:
			strcat(sIcmpType, "[ICMP_UNREACH_PORT](bad port)");
			break;
		case ICMP_UNREACH_NEEDFRAG:
			strcat(sIcmpType, "[ICMP_UNREACH_NEEDFRAG](IP_DF caused drop)");
			break;
		case ICMP_UNREACH_SRCFAIL:
			strcat(sIcmpType, "[ICMP_UNREACH_SRCFAIL](src route failed)");
			break;
		case ICMP_UNREACH_NET_UNKNOWN:
			strcat(sIcmpType, "[ICMP_UNREACH_NET_UNKNOWN](unknown net)");
			break;
		case ICMP_UNREACH_HOST_UNKNOWN:
			strcat(sIcmpType, "[ICMP_UNREACH_HOST_UNKNOWN](unknown host)");
			break;
		case ICMP_UNREACH_ISOLATED:
			strcat(sIcmpType, "[ICMP_UNREACH_ISOLATED](src host isolated)");
			break;
		case ICMP_UNREACH_NET_PROHIB:
			strcat(sIcmpType, "[ICMP_UNREACH_NET_PROHIB](prohibited access)");
			break;
		case ICMP_UNREACH_HOST_PROHIB:
			strcat(sIcmpType, "[ICMP_UNREACH_HOST_PROHIB](ditto)");
			break;
		case ICMP_UNREACH_TOSNET:
			strcat(sIcmpType, "[ICMP_UNREACH_TOSNET](bad tos for net)");
			break;
		case ICMP_UNREACH_TOSHOST:
			strcat(sIcmpType, "[ICMP_UNREACH_TOSHOST](bad tos for host)");
			break;
		default:
			strcat(sIcmpType, "[OTHER](Unknow Sub Code)");
			break;
		}
		break;
	case ICMP_SOURCEQUENCH:
		strcat(sIcmpType, "ICMP_SOURCEQUENCH(packet lost, slow down)");
		break;
	case ICMP_REDIRECT:
		strcat(sIcmpType, "ICMP_REDIRECT(shorter route)");
		switch(bSubCode)
		{
		case ICMP_REDIRECT_NET:
			strcat(sIcmpType, "[ICMP_REDIRECT_NET](for network)");
			break;
		case ICMP_REDIRECT_HOST:
			strcat(sIcmpType, "[ICMP_REDIRECT_HOST](for host)");
			break;
		case ICMP_REDIRECT_TOSNET:
			strcat(sIcmpType, "[ICMP_REDIRECT_TOSNET](for tos and net)");
			break;
		case ICMP_REDIRECT_TOSHOST:
			strcat(sIcmpType, "[ICMP_REDIRECT_TOSHOST](for tos and host)");
			break;
		default:
			strcat(sIcmpType, "[OTHER](Unknow Sub Code)");
			break;
		}
		break;
	case ICMP_ECHO:
		strcat(sIcmpType, "ICMP_ECHO(echo service)");
		break;
	case ICMP_ROUTERADVERT:
		strcat(sIcmpType, "ICMP_ROUTERADVERT(router advertisement)");
		break;
	case ICMP_ROUTERSOLICIT:
		strcat(sIcmpType, "ICMP_ROUTERSOLICIT(router solicitation)");
		break;
	case ICMP_TIMXCEED:
		strcat(sIcmpType, "ICMP_TIMXCEED(time exceeded)");
		switch(bSubCode)
		{
		case ICMP_TIMXCEED_INTRANS:
			strcat(sIcmpType, "[ICMP_TIMXCEED_INTRANS](ttl==0 in transit)");
			break;
		case ICMP_TIMXCEED_REASS:
			strcat(sIcmpType, "[ICMP_TIMXCEED_REASS](ttl==0 in reass)");
			break;
		default:
			strcat(sIcmpType, "[OTHER](Unknow Sub Code)");
			break;
		}
		break;
	case ICMP_PARAMPROB:
		strcat(sIcmpType, "ICMP_PARAMPROB(ip header bad)");
		switch(bSubCode)
		{
		case ICMP_PARAMPROB_OPTABSENT:
			strcat(sIcmpType, "[ICMP_PARAMPROB_OPTABSENT](req. opt. absent)");
			break;
		default:
			strcat(sIcmpType, "[OTHER](Unknow Sub Code)");
			break;
		}
		break;
	case ICMP_TSTAMP:
		strcat(sIcmpType, "ICMP_TSTAMP(timestamp request)");
		break;
	case ICMP_TSTAMPREPLY:
		strcat(sIcmpType, "ICMP_TSTAMPREPLY(timestamp reply)");
		break;
	case ICMP_IREQ:
		strcat(sIcmpType, "ICMP_IREQ(information request)");
		break;
	case ICMP_IREQREPLY:
		strcat(sIcmpType, "ICMP_IREQREPLY(information reply)");
		break;
	case ICMP_MASKREQ:
		strcat(sIcmpType, "ICMP_MASKREQ(address mask request)");
		break;
	case ICMP_MASKREPLY:
		strcat(sIcmpType, "ICMP_MASKREPLY(address mask reply)");
		break;
	default:
		strcat(sIcmpType, "ICMP_OTHER_TYPE");
		break;
	}
}


//
// ΪӦ�ó�������б����Ӽ�¼
//
void AddApp(CListCtrl* pList, PPACKET_LOG pLog, int nMaxCount, BOOL IsSeleted, BOOL IsShowDate)
{
	CString sString[MONITOR_APP_HEADER_COUNT];

	sString[0] = ACL_QUERY_TEXT[pLog->bAction];
	sString[1] = GetName(pLog->sProcessName);
	sString[2].Format("%s", pLog->tEndTime.Format(IsShowDate ? "%Y-%m-%d %H:%M:%S" : "%H:%M:%S"));
	sString[3].Format("%s/%u", DIPToSIP(&pLog->dwRemoteIp), pLog->wRemotePort);
	sString[4].Format("%u/%u", pLog->dwSendData, pLog->dwRecvData);
	sString[5].Format("%s/%s", GUI_SERVICE_TYPE[pLog->bProtocol], GUI_DIRECTION[pLog->bDirection]);
	sString[6].Format("%s/%u", DIPToSIP(&pLog->dwLocalIp), pLog->wLocalPort);
	sString[7] = pLog->sMemo;

	AddMonitor(pList, (LPCTSTR*)sString, MONITOR_APP_HEADER_COUNT, nMaxCount, IsSeleted);
}

//
// Ϊ�����ھӼ����б����Ӽ�¼
//
void AddNnb(CListCtrl* pList, PPACKET_LOG pLog, int nMaxCount, BOOL IsSeleted, BOOL IsShowDate)
{
	CString sString[MONITOR_NNB_HEADER_COUNT];

	sString[0] = ACL_QUERY_TEXT[pLog->bAction];
	sString[1] = GetName(pLog->sProcessName);
	sString[2].Format("%s/%u", pLog->sLocalHost, pLog->wLocalPort);
	sString[3].Format("%s/%u", pLog->sRemoteHost, pLog->wRemotePort);
	sString[4].Format("%s/%u", SEND_OR_RECV[pLog->SendOrRecv], pLog->dwRecvData + pLog->dwSendData);
	sString[5].Format("%s/%s", GUI_SERVICE_TYPE[pLog->bProtocol], GUI_DIRECTION[pLog->bDirection]);
	sString[6].Format("%s", pLog->tStartTime.Format(IsShowDate ? "%Y-%m-%d %H:%M:%S" : "%H:%M:%S"));
	sString[7] = pLog->sMemo;

	AddMonitor(pList, (LPCTSTR*)sString, MONITOR_NNB_HEADER_COUNT, nMaxCount, IsSeleted);
}

//
// ΪICMP�����б����Ӽ�¼
//
void AddIcmp(CListCtrl* pList, PPACKET_LOG pLog, int nMaxCount, BOOL IsSeleted, BOOL IsShowDate)
{
	CString sString[MONITOR_ICMP_HEADER_COUNT];

	sString[0] = ACL_QUERY_TEXT[pLog->bAction];
	sString[1] = GetName(pLog->sProcessName);
	sString[2].Format("%s", pLog->tStartTime.Format(IsShowDate ? "%Y-%m-%d %H:%M:%S" : "%H:%M:%S"));
	sString[3].Format("%s/%u", SEND_OR_RECV[pLog->SendOrRecv], pLog->dwRecvData + pLog->dwSendData);
	sString[4].Format("%s", GUI_DIRECTION[pLog->bDirection]);
	sString[5].Format("%s -> %s; %s", DIPToSIP(&pLog->dwLocalIp), DIPToSIP(&pLog->dwRemoteIp), pLog->sMemo);

	AddMonitor(pList, (LPCTSTR*)sString, MONITOR_ICMP_HEADER_COUNT, nMaxCount, IsSeleted);
}

void AddMonitor(CListCtrl *pList, LPCTSTR* pString, int nCount, int nMaxCount, BOOL IsSeleted)
{
	if(nMaxCount!= -1 && pList->GetItemCount() > nMaxCount)
		pList->DeleteItem(0);
	AddList(pList, pString, nCount, IsSeleted, FALSE, -1);
}

#endif// XF_GUI_COMMON_FUNCTION


#ifdef XF_DLL_COMMON_FUNCTION
#endif // XF_DLL_COMMON_FUNCTION

#pragma comment( exestr, "B9D3B8FD2A7A65716F6F71702B")
