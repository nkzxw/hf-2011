//-----------------------------------------------------------
/*
	工程：		费尔个人防火墙
	网址：		http://www.xfilt.com
	电子邮件：	xstudio@xfilt.com
	版权所有 (c) 2002 朱艳辉(费尔安全实验室)

	版权声明:
	---------------------------------------------------
		本电脑程序受著作权法的保护。未经授权，不能使用
	和修改本软件全部或部分源代码。凡擅自复制、盗用或散
	布此程序或部分程序或者有其它任何越权行为，将遭到民
	事赔偿及刑事的处罚，并将依法以最高刑罚进行追诉。
	
		凡通过合法途径购买此源程序者(仅限于本人)，默认
	授权允许阅读、编译、调试。调试且仅限于调试的需要才
	可以修改本代码，且修改后的代码也不可直接使用。未经
	授权，不允许将本产品的全部或部分代码用于其它产品，
	不允许转阅他人，不允许以任何方式复制或传播，不允许
	用于任何方式的商业行为。	

    ---------------------------------------------------	
*/
//-----------------------------------------------------------
// Author & Create Date: Tony Zhu, 2002/03/23
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
// Abstract:
//		This module used to manage the ACL memory. First time, 
//		allocate full size of ACL file, and after, dynamic allocate 
//		page when add ACL's record.
//
//
//

#include "stdafx.h"
#include "../xfilter/xfilter.h"
#include "PacketMonitor.h"

DWORD m_dwPacketMonitorThreadId = NULL;
BOOL m_bIsMonitor = FALSE;
BOOL m_bInMonitor = FALSE;

DWORD			m_SessionCount = 0;
PSESSION		m_pSession = NULL;
PACKET_BUFFER_POINT m_BufferPoint;
DIRECTION_POINT		m_DirectionPoint;

BOOL WINAPI StartMonitor()
{
	memset(&m_BufferPoint, 0, sizeof(PACKET_BUFFER_POINT));
	memset(&m_DirectionPoint, 0, sizeof(DIRECTION_POINT));

	if(theApp.m_pDllIoControl == NULL
		|| !XF_ReadBufferPoint(&m_BufferPoint) 
		|| m_BufferPoint.pPacket == NULL
		|| !XF_ReadDirectionPoint(&m_DirectionPoint)
		|| m_DirectionPoint.pDelete == NULL
		|| m_DirectionPoint.pDirection == NULL
		|| !XF_SetXfilterHandle()
		) 
		return FALSE;

	ODS("XF_SetXfilterHandle True\n");

	XFILTER_IO_CONTROL ioControl;

	if(theApp.m_pDllIoControl(IO_CONTROL_GET_SESSION_COUNT, &ioControl) == XERR_SUCCESS)
		m_SessionCount = ioControl.DWord;
	else
		return FALSE;

	ODS("IO_CONTROL_GET_SESSION_COUNT True\n");

	if(theApp.m_pDllIoControl(IO_CONTROL_GET_SESSION_FILE_HANDLE, &ioControl) == XERR_SUCCESS)
	{
		HANDLE hMemFile = (HANDLE)ioControl.DWord;
		if(hMemFile == NULL) return FALSE;
		hMemFile = CreateFileMapping((HANDLE)INVALID_HANDLE_VALUE
			, GetSecurityAttributes()
			, PAGE_READWRITE
			, 0
			, SESSION_MEMORY_FILE_MAX_SIZE
			, SESSION_MEMORY_FILE_NAME);
		if(hMemFile == NULL || GetLastError() != ERROR_ALREADY_EXISTS) return FALSE;
		ODS("IO_CONTROL_GET_SESSION_FILE_HANDLE True\n");
		m_pSession = (PSESSION)MapViewOfFile(hMemFile, FILE_MAP_WRITE, 0, 0, 0);
		if(m_pSession == NULL) return FALSE;
		ODS("MapViewOfFile True\n");
	}
	else
		return FALSE;


	m_bIsMonitor = TRUE;
	if(m_dwPacketMonitorThreadId == NULL
		&& ::CreateThread(NULL, 0, MonitorPacket, 0, 0, &m_dwPacketMonitorThreadId) == NULL)
	{
		m_bIsMonitor = FALSE;
		ODS("MonitorPacket FALSE\n");
		return FALSE;
	}
	return TRUE;
}

BOOL WINAPI StopMonitor()
{
	if(m_bIsMonitor == FALSE || m_bInMonitor == FALSE) return TRUE;
	m_bIsMonitor = FALSE;
	while(m_bInMonitor)
		Sleep(50);
	return TRUE;
}

int GetOnePacket()
{
	static int iIndex;
	if(m_BufferPoint.pPacket == NULL 
		|| *m_BufferPoint.ReadIndex == *m_BufferPoint.WriteIndex) 
		return -1;

	iIndex = *m_BufferPoint.ReadIndex;
	*m_BufferPoint.ReadIndex = iIndex + 1;
	if(*m_BufferPoint.ReadIndex >= MAX_PACKET_BUFFER)
		*m_BufferPoint.ReadIndex = 0;

	return iIndex;
}

DWORD WINAPI MonitorPacket(LPVOID pVoid)
{
	int iCount = 0;
	m_bInMonitor = TRUE;

	//
	// 2002/08/21 for windows xp
	//
	CXInstall m_Install;
	BOOL bIsWin9x = IsWin9x();

	while(m_bIsMonitor)
	{
		//
		// 2002/08/21 for windows xp
		//
		m_Install.UpdateInstall();

		//
		// 2002/08/21 add for refresh hook send
		//
		if(!bIsWin9x)
			XF_RefreshHookSend();

		for(DWORD i = 0; i < m_SessionCount; i++)
		{
			if(m_pSession[i].s != 0)
			{
				if(m_pSession[i].bStatus >= SESSION_STATUS_QUERY_APP)
					theApp.m_pMainDlg.GetAclDlg()->PostMessage(ACL_WM_QUERY, MON_ADD_SPI_ONLINE, i);

				theApp.m_pMainDlg.GetMonitorDlg()->PostMessage(MON_WM_ADD_LIST, MON_ADD_SPI_ONLINE, i);
			}
			
			if(i < 100 && m_DirectionPoint.pDelete[i].Id != 0)
				theApp.m_pMainDlg.GetMonitorDlg()->PostMessage(MON_WM_ADD_LIST, MON_DEL_DRV_ONLINE, i);

			if(i < (DWORD)*m_DirectionPoint.DirectionCount)
				theApp.m_pMainDlg.GetMonitorDlg()->PostMessage(MON_WM_ADD_LIST, MON_ADD_DRV_ONLINE, i);
			
			iCount = GetOnePacket();
			if(iCount >= 0)
			{
				if(m_BufferPoint.pPacket[iCount].Action == XF_QUERY)
					theApp.m_pMainDlg.GetAclDlg()->PostMessage(ACL_WM_QUERY, MON_ADD_PACKET, iCount);

				theApp.m_pMainDlg.GetMonitorDlg()->PostMessage(MON_WM_ADD_LIST, MON_ADD_PACKET, iCount);
			}
		}

		iCount = 0;
		while(m_bIsMonitor && iCount++ < 10)Sleep(50);
	}
	m_bInMonitor = FALSE;
	return 0;
}

DWORD m_dwUpdateNnbThreadId = NULL;
BOOL m_bIsUpdateNnb = FALSE;
BOOL m_bInUpdateNnb = FALSE;

DWORD WINAPI UpdateNnbThread(LPVOID pVoid)
{
	int iCount = 0;
	m_bInUpdateNnb = TRUE;
	if(m_bIsUpdateNnb)
		EnumerateFunc(NULL, NULL, RESOURCEDISPLAYTYPE_SERVER);
	m_bInUpdateNnb = FALSE;
	return 0;
}

BOOL WINAPI StartUpdateNnb()
{
	m_bIsUpdateNnb = TRUE;
	if(m_dwUpdateNnbThreadId == NULL
		&& ::CreateThread(NULL, 0, UpdateNnbThread, 0, 0, &m_dwUpdateNnbThreadId) == NULL)
	{
		m_bIsUpdateNnb = FALSE;
		return FALSE;
	}
	return TRUE;
}

BOOL WINAPI StopUpdateNnb()
{
	if(m_bIsUpdateNnb == FALSE || m_bInUpdateNnb == FALSE) return TRUE;
	m_bIsUpdateNnb = FALSE;
	while(m_bInUpdateNnb)
		Sleep(50);
	return TRUE;
}


//
// Parameters:
//		lpnr			first call must be NULL.
//		pList			show enum string.
//		dwDisplayType	RESOURCEDISPLAYTYPE_DOMAIN
//						RESOURCEDISPLAYTYPE_SERVER
//						RESOURCEDISPLAYTYPE_SHARE
//						RESOURCEDISPLAYTYPE_GENERIC
//
BOOL WINAPI EnumerateFunc(LPNETRESOURCE lpnr, CListBox* pList, DWORD dwDisplayType)
{   
	DWORD dwResult, dwResultEnum;
	HANDLE hEnum;  
	DWORD cbBuffer = 16384;      
	DWORD cEntries = -1;         
	LPNETRESOURCE lpnrLocal;     
	DWORD i; 

	dwResult = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY, 0, lpnr, &hEnum);
	if (dwResult != NO_ERROR) return FALSE;
	lpnrLocal = (LPNETRESOURCE) GlobalAlloc(GPTR, cbBuffer);
	do  
	{
		ZeroMemory(lpnrLocal, cbBuffer);
		dwResultEnum = WNetEnumResource(hEnum, &cEntries, lpnrLocal, &cbBuffer);
		if (dwResultEnum == NO_ERROR)    
		{
			for(i = 0; i < cEntries; i++) 
			{
				if(lpnrLocal[i].dwDisplayType == dwDisplayType)
				{
					if(pList != NULL)
						pList->AddString(lpnrLocal[i].lpRemoteName + 2);
					else
						UpdateNnb(lpnrLocal[i].lpRemoteName + 2);
				}
				else
				{
					if(RESOURCEUSAGE_CONTAINER 
						== (lpnrLocal[i].dwUsage & RESOURCEUSAGE_CONTAINER))
						EnumerateFunc(&lpnrLocal[i], pList, dwDisplayType);
				}
			}
		}    
		else if (dwResultEnum != ERROR_NO_MORE_ITEMS)
		{
			break;
		}  
	} while(dwResultEnum != ERROR_NO_MORE_ITEMS);  

	GlobalFree((HGLOBAL)lpnrLocal);  
	dwResult = WNetCloseEnum(hEnum);
	if(dwResult != NO_ERROR)
		return FALSE;
	return TRUE;
}

void UpdateNnbAddress(LPTSTR sName, DWORD dwIp)
{
	PXACL_HEADER pHeader = theApp.m_AclFile.GetHeader();
	if(pHeader == NULL)
		return;
	PXACL_NNB pNnb = pHeader->pNnb;
	if(pNnb == NULL)
		return;

	while(pNnb != NULL)
	{
		if(strcmp(pNnb->sNnb, sName) == 0)
		{
			if(pNnb->dwIp != dwIp)
				pNnb->dwIp = dwIp;
			break;
		}
		pNnb = pNnb->pNext;
	}
}
//
// this function depend on winsock, so this project must initialize with WSAStartup()
//
BOOL WINAPI GetAddressByHost(LPTSTR sHost, BYTE* pIpBuffer)
{
	HOSTENT  *pHostEnt = NULL; 
	pHostEnt = gethostbyname(sHost);
	if(pHostEnt == NULL) return FALSE;
	if(memcmp(pIpBuffer, pHostEnt->h_addr_list[0], 4) != 0)
		memcpy(pIpBuffer, pHostEnt->h_addr_list[0], 4);
	DWORD dwIp = ntohl(*(DWORD*)pIpBuffer);
	UpdateNnbAddress(sHost, dwIp);
	return TRUE;
}

void UpdateNnb(LPTSTR pHost)
{
	static char buf[68];
	strcpy(buf + 4, pHost);
	GetAddressByHost(pHost, (BYTE*)buf);
	XF_AddNetBiosName(buf, sizeof(buf));
}


#pragma comment( exestr, "B9D3B8FD2A7263656D67766F71706B7671742B")
