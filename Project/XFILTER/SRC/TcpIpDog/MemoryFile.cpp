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
// Author & Create Date: Tony Zhu, 2002/03/18
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
// Abstract:
//
//
// 简介：
//		控管规则比对模块
//
//
//
//

#include "stdafx.h"
#include "MemoryFile.h"

#pragma data_seg(".inidata")
	BYTE		m_gbWorkMode		= XF_PASS_ALL;
	BYTE		m_gbAclIsRefresh	= FALSE;
	DWORD		m_gdwUpdateVersion	= 0;
	DWORD		m_ghMemoryFile		= 0;
	DWORD		m_gdwMaxSize		= 0;
	DWORD		m_gdwBaseAddress	= 0;
	DWORD		m_gdwXfilterProcessId = 0;
#pragma data_seg()

#pragma bss_seg(".uinidata")
	char		m_XfilterPathName[MAX_PATH];
#pragma bss_seg()

//
// 保存XFILTER.EXE的进程ID和完整路径
//
void SetXfilterProcessId(DWORD dwProcessId, char* sXfilterName)
{
	m_gdwXfilterProcessId = dwProcessId;
	if(m_gdwXfilterProcessId != 0 && sXfilterName != NULL)
		strcpy(m_XfilterPathName, sXfilterName);
	else
		m_XfilterPathName[0] = 0;
}

//
// 保存总工作模式
//
void SetWorkMode(BYTE bWorkMode)
{
	m_gbWorkMode = bWorkMode;
}

//
// 保存控管规则内存映射文件句柄
//
void SetMemoryFile(DWORD hMemoryFile)
{
	m_ghMemoryFile = hMemoryFile;
}

//
// 保存控管规则内存映射文件的大小
//
void SetMaxSize(DWORD dwMaxSize)
{
	m_gdwMaxSize = dwMaxSize;
}

//
// 保存内存映射文件的基地址
//
void SetBaseAddress(DWORD dwBaseAddress)
{
	m_gdwBaseAddress = dwBaseAddress;
}

//
// 设置控管规则是否正在更新
//
void SetRefresh(BYTE bIsRefresh)
{
	m_gbAclIsRefresh = bIsRefresh;
}

//
// 设置控管规则更新计数
//
void SetUpdateVersion(DWORD dwUpdateVersion)
{
	m_gdwUpdateVersion = dwUpdateVersion;
}

//
// 控管规则更新计数加一
//
void RefenceUpdateVersion()
{
	m_gdwUpdateVersion ++;
}

//
// 返回当前的总工作模式
//
BYTE GetWorkMode()
{
	return m_gbWorkMode;
}

//
// 得到控管规则内存映射文件的句柄
//
HANDLE GetMemoryFile()
{
	return (HANDLE)m_ghMemoryFile;
}

//
// 得到控管规则是否处在更新状态
//
BYTE GetRefresh()
{
	return m_gbAclIsRefresh;
}

//
// 得到控管规则更新计数
//
DWORD GetUpdateVersion()
{
	return m_gdwUpdateVersion;
}

//==========================================================================
// 控管规则比对类
//

CMemoryFile::CMemoryFile()
{
	m_pMemoryFile		= NULL;
	m_pAclHeader		= NULL;
	m_dwUpdateVersion	= 0;
	m_dwOffset			= 0;

	//
	// 2002/08/20 add
	//
	m_hFile				= NULL;

	m_XfilterPathName[0] = 0;

	SetWindowsVersion();
}

CMemoryFile::~CMemoryFile()
{
	//OutputDebugString("~CMemoryFile, ");
	UnmapUserMemory();
}

//
// 2002/08/20 add
//
void CMemoryFile::UnmapUserMemory()
{
	//CString string;
	//string.Format("UnmapUserMemory, m_hFile:0x%08X, m_pMemoryFile:0x%08X\n\n"
	//	, m_hFile
	//	, m_pMemoryFile
	//	);
	//OutputDebugString(string);

	if(m_hFile && m_pMemoryFile)
		XF_UnmapViewOfFile(m_hFile, IOCTL_XPACKET_UNMAP_ACL_BUFFER, m_pMemoryFile);
}

//
// 使控管规则正在使用的计数器 + 1
//
BOOL CMemoryFile::Refence()
{
	if(!Open()) return FALSE;
	if(m_pAclHeader == NULL) return FALSE;
	while(!m_gbAclIsRefresh && m_pAclHeader->wPv != PV_UNLOCK)
		Sleep(PV_LOCK_WAIT_TIME);
	if(m_gbAclIsRefresh || m_pAclHeader == NULL) return FALSE;
	m_pAclHeader->wRefenceCount++;
	DP1("RefenceCount: %u\n", m_pAclHeader->wRefenceCount);
	return TRUE;
}

//
// 使控管规则正在使用的计数器 - 1
//
void CMemoryFile::Derefence()
{
	if(m_pAclHeader == NULL)
		return;
	m_pAclHeader->wRefenceCount--;
	DP1("DerefenceCount: %u\n", m_pAclHeader->wRefenceCount);
}

//
// 打开控管规则的内存映射文件
//
BOOL CMemoryFile::Open()
{
	while(m_gbAclIsRefresh)Sleep(100);
	if(m_ghMemoryFile == NULL)return FALSE;
	if(m_dwUpdateVersion >= m_gdwUpdateVersion && m_pMemoryFile != NULL) return TRUE;
	HANDLE hFile;
	hFile = XF_CreateFileMapping((HANDLE)INVALID_HANDLE_VALUE
		, GetSecurityAttributes()
		, PAGE_READWRITE
		, 0
		, m_gdwMaxSize
		, XFILTER_MEMORY_ACL_FILE);
	if(hFile == NULL || GetLastError() != ERROR_ALREADY_EXISTS) return FALSE;

	//
	// 2002/08/20 add
	//
	m_hFile = hFile;

	m_pMemoryFile = (char*)XF_MapViewOfFile(hFile, FILE_MAP_WRITE, 0, 0, 0, 0);
	if(m_pMemoryFile == NULL) return FALSE;
	m_dwUpdateVersion = m_gdwUpdateVersion;
	m_pAclHeader = (PXACL_HEADER)m_pMemoryFile;

	m_dwOffset = (DWORD)m_pMemoryFile - m_gdwBaseAddress;

	return TRUE;
}

//
// 由于映射的每一个用户地址空间的各不相同，所以需要通过偏移量
// 计算出正确的地址
//
DWORD CMemoryFile::Nat(PVOID pAddress)
{
	if(pAddress == NULL) return 0;
	return ((DWORD)pAddress + m_dwOffset);
}

//
// 根据控管规则对session记录进行合法性验证的总控函数
//
int CMemoryFile::CheckAcl(SESSION *session)
{
	static int iRet;
	__try
	{
		//
		// 在经过控管规则判断之前，首先通过常规性验证
		//
		iRet = GetAccessWithoutAcl(session);
		if(iRet != XF_FILTER) return iRet;
		//
		// 通过应用程序控管规则进行验证
		//
		iRet = CheckApp(session);
		iRet = GetAccessFromSecurity(iRet);
		if(iRet == XF_QUERY) 
		{
			//
			// 发出询问请求
			//
			iRet = Query(session, SESSION_STATUS_QUERY_APP);
			session->bIsQuery = TRUE;
			return iRet;
		}
		if(iRet != XF_PASS) return iRet;
		//
		// 对站点进行验证
		//
		iRet = CheckWeb(session);
		iRet = GetAccessFromSecurity(iRet);
		if(iRet == XF_QUERY)
		{
			//
			// 发出询问请求
			//
			iRet = Query(session, SESSION_STATUS_QUERY_WEB);
			session->bIsQuery = TRUE;
			return iRet;
		}
		if(iRet != XF_PASS) return iRet;

		return iRet;
	}
	__finally
	{
		session->bAction = iRet;
	}
	return XF_PASS;
}

//
// 根据安全等级判断是否需要放行。当需要询问而安全等级为低时默认放行
// 或者当需要询问安全等级为中时，连出默认放行。
//
int CMemoryFile::GetAccessFromSecurity(BYTE bAction)
{
	if(!Refence()) return XF_PASS;
	ODS("DLL: GetAccessFromSecurity");
	__try
	{
		if(bAction == XF_QUERY && m_pAclHeader->bSecurity == ACL_SECURITY_LOWER)
			return XF_PASS;
		return bAction;
	}
	__finally
	{
		Derefence();
	}
	return bAction;
}

//
// 2002/05/24 add
//
// 从总工作模式得到控管状态
//
int CMemoryFile::CheckWorkMode()
{
	if(m_pAclHeader->bWorkMode == XF_PASS_ALL)
		return XF_PASS;
	if(m_pAclHeader->bWorkMode == XF_DENY_ALL)
		return XF_DENY;
	if(m_pAclHeader->bWorkMode != XF_QUERY_ALL)
		return XF_UNKNOWN;

	return XF_FILTER;
}

//
// 应用程序规则比较函数
//
int CMemoryFile::CheckApp(SESSION *session)
{
	if(!Refence()) return XF_PASS;
	ODS("DLL: CheckApp");
	__try
	{
		//
		// 2002/05/24 add
		//
		int iRet = CheckWorkMode();
		if(iRet != XF_FILTER) return iRet;

		iRet = CheckSubWorkMode(m_pAclHeader->bAppSet, session);
		if(iRet != XF_QUERY) return iRet;

		PXACL pAcl = (PXACL)Nat(m_pAclHeader->pAcl);

		BYTE  bDirection;
		if(session->bDirection == ACL_DIRECTION_LISTEN)
			bDirection = ACL_DIRECTION_IN;
		else if(session->bDirection == ACL_DIRECTION_BROADCAST)
			bDirection = ACL_DIRECTION_OUT;
		else
			bDirection = session->bDirection;


		if(pAcl != NULL)
		{
			//
			// 循环比较应用程序规则
			//
			do
			{
				FindIp(session->dwRemoteIp, session);
				FindTime(session->tStartTime, session);
				if(pAcl->bDirection != ACL_DIRECTION_IN_OUT 
					&& pAcl->bDirection != bDirection) continue;
				if(pAcl->uiServicePort != ACL_SERVICE_PORT_ALL 
					&& pAcl->uiServicePort != session->wRemotePort) continue;
				if(pAcl->bRemoteNetType != ACL_NET_TYPE_ALL 
					&& pAcl->bRemoteNetType != session->bNetType) continue;
				if(pAcl->wLocalPort != ACL_SERVICE_PORT_ALL 
					&& pAcl->wLocalPort != session->wLocalPort) continue;
				if(pAcl->bAccessTimeType != ACL_SERVICE_PORT_ALL 
					&& pAcl->bAccessTimeType != session->bTimeType) continue;
				if(pAcl->bServiceType != ACL_SERVICE_TYPE_ALL 
					&& pAcl->bServiceType != session->bProtocol) continue;
				if(_tcscmp(pAcl->sApplication, _T("*")) != 0 
					&& (_tcslen(pAcl->sApplication) != _tcslen(session->sPathName) 
					|| _tcsnicmp(pAcl->sApplication, session->sPathName
						, _tcslen(session->sPathName)) != 0)
					) continue;

				return pAcl->bAction;

			}while((pAcl = (PXACL)Nat(pAcl->pNext)) != NULL);
		}

		iRet = CheckQueryEx(m_pAclHeader->bAppQueryEx);
		return iRet;
	}
	__finally
	{
		Derefence();
	}
	return XF_PASS;
}

//
// 网站规则比对函数
//
int CMemoryFile::CheckWeb(SESSION *session)
{
	if(!Refence()) return XF_PASS;
	ODS("DLL: CheckWeb");

	__try
	{
		//
		// 2002/05/24 add
		//
		int iRet = CheckWorkMode();
		if(iRet != XF_FILTER) return iRet;

		iRet = CheckSubWorkMode(m_pAclHeader->bWebSet, session);
		if(iRet != XF_QUERY) return iRet;

		if(session->bProtocol != ACL_SERVICE_TYPE_HTTP
			|| session->bDirection != ACL_DIRECTION_OUT
			|| session->sMemo[0] == 0)
			return XF_PASS;

		PXACL_WEB pWeb = (PXACL_WEB)Nat(m_pAclHeader->pWeb);
		while(pWeb != NULL)
		{
			if(FindString(session->sMemo, pWeb->sWeb))
				return pWeb->bAction;
			pWeb = (PXACL_WEB)Nat(pWeb->pNext);
		}

		iRet = CheckQueryEx(m_pAclHeader->bWebQueryEx);
		return iRet;
		
	}
	__finally
	{
		Derefence();
	}
	return XF_PASS;
}

//
// 保留函数
//
int CMemoryFile::CheckTorjan()
{
	return XF_PASS;
}

//
// 保留函数
//
int CMemoryFile::ProtectApp()
{
	return XF_PASS;
}

//
// 根据时间和星期得到自定义的时间类型
//
int CMemoryFile::FindTime(CTime time, PSESSION session)
{
	PXACL_TIME pTime = (PXACL_TIME)Nat(m_pAclHeader->pTime);
	if(pTime != NULL) pTime = (PXACL_TIME)Nat(pTime->pNext);
	int i = 0;

	__try
	{
		do
		{
			i++;
			if(GetBit(pTime->bWeekDay,time.GetDayOfWeek() - 1) != 1)
				continue;
			if(pTime->tStartTime == pTime->tEndTime)
				return i;

			CTime t = time.GetHour() * 3600 + time.GetMinute() * 60 + time.GetSecond();

			if(pTime->tStartTime < pTime->tEndTime)
			{
				if(t >= pTime->tStartTime && t <= pTime->tEndTime)
					return i;
			}
			else
			{
				if(t >= pTime->tStartTime || t <= pTime->tEndTime)
					return i;
			}
			
		} while((pTime = (PXACL_TIME)Nat(pTime->pNext)) != NULL);

		i = ACL_TIME_TYPE_ALL;
		return i;
	}
	__finally
	{
		session->bTimeType = i;
	}

	return ACL_TIME_TYPE_ALL;
}

//
// 根据Ip地址得到自定义的网络类型
//
int CMemoryFile::FindIp(DWORD Ip, PSESSION session)
{
	int iRet = ACL_NET_TYPE_ALL;
	__try
	{
		if(FindIpEx((PXACL_IP)Nat(m_pAclHeader->pIntranetIp), Ip))
		{
			iRet = ACL_NET_TYPE_INTRANET;
			return iRet;
		}
		if(FindIpEx((PXACL_IP)Nat(m_pAclHeader->pDistrustIp), Ip))
		{
			iRet = ACL_NET_TYPE_DISTRUST;
			return iRet;
		}
		if(FindIpEx((PXACL_IP)Nat(m_pAclHeader->pTrustIp), Ip))
		{
			iRet = ACL_NET_TYPE_TRUST;
			return iRet;
		}
		if(FindIpEx((PXACL_IP)Nat(m_pAclHeader->pCustomIp), Ip))
		{
			iRet = ACL_NET_TYPE_CUSTOM;
			return iRet;
		}

		iRet = ACL_NET_TYPE_ALL;
		return iRet;
	}
	__finally
	{
		session->bNetType = iRet;
	}
	return iRet;
}

//
// Ip 地址段比较函数
//
BOOL CMemoryFile::FindIpEx(PXACL_IP pIp, DWORD dwIp)
{
	while(pIp != NULL)
	{
		if(dwIp >= pIp->ulStartIP && dwIp <= pIp->ulEndIP) 
			return TRUE;
		pIp = (PXACL_IP)Nat(pIp->pNext);
	}
	return FALSE;
}

//
// 不使用控管规则判断封包是否允许放行，在使用控管规则判断之前一般
// 首先调用此函数进行判断，如果返回XF_FILTER则需要进一步使用控管规则
// 进行判断，如果是XF_PASS或者XF_DENY则直接进行放行或者拒绝。
// 
//
int	CMemoryFile::GetAccessWithoutAcl(SESSION *session)
{
	static int Action;
	Action = GetAccessFromWorkMode();
	if(Action != XF_FILTER)
		return Action;

	if(IsLocalIp(&session->dwRemoteIp) || IsSuperProcess(NULL)) 
		return XF_PASS;

	if(session->bDirection == ACL_DIRECTION_NOT_SET)
		return XF_PASS;

	if(session->bIsQuery)
		return session->bAction;

	return XF_FILTER;
}

//
// 判断是否是本地IP地址
// 返回值：
//		TRUE:	是本地IP
//		FALSE:	不是本地IP
//
BOOL CMemoryFile::IsLocalIp(DWORD *ip)
{
	BYTE IsLocalIP[4];
	memcpy(IsLocalIP, ip, sizeof(DWORD));

	if(*ip == 0 || IsLocalIP[3] == 127)
		return TRUE;
	return FALSE;
}

//
// 从总工作模式得到控管状态
//
int CMemoryFile::GetAccessFromWorkMode()
{
	if(m_gbWorkMode == XF_PASS_ALL)
		return XF_PASS;
	if(m_gbWorkMode == XF_DENY_ALL)
		return XF_DENY;
	if(m_gbWorkMode != XF_QUERY_ALL)
		return XF_UNKNOWN;

	return XF_FILTER;
}

//
// 得到当前Windows版本
//
void CMemoryFile::SetWindowsVersion()
{
	OSVERSIONINFO VerInfo;  
	VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&VerInfo);

	if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		m_bIsWin9x = TRUE;
	}
	else if(VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT &&
		(VerInfo.dwMajorVersion == 4 || VerInfo.dwMajorVersion == 5))
	{
		m_bIsWin9x = FALSE;
	}
}

//
//
// 2002/05/24 add
//
// 判断是否是XFILTER.EXE
//
BOOL CMemoryFile::IsXfilter()
{
	if(GetCurrentProcessId() == m_gdwXfilterProcessId)
		return TRUE;

	char CurrentModuleFileName[MAX_PATH];
	GetModuleFileName(NULL, CurrentModuleFileName, MAX_PATH);
	if(strcmp(CurrentModuleFileName, m_XfilterPathName) == 0)
		return TRUE;

	return FALSE;
}

//
// 判断是否是XFILTER.EXE或者是需要放行系统进程，这些进程默认放行
//
BOOL CMemoryFile::IsSuperProcess(LPCSTR sProcessName)
{
	if(GetCurrentProcessId() == m_gdwXfilterProcessId)
		return TRUE;

	char CurrentModuleFileName[MAX_PATH];
	GetModuleFileName(NULL, CurrentModuleFileName, MAX_PATH);
	if(strcmp(CurrentModuleFileName, m_XfilterPathName) == 0)
		return TRUE;

	sProcessName = CurrentModuleFileName;

	static TCHAR* SuperProcess9x[] = 
	{
		_T("%windir%\\system\\icsmgr.exe")
	};
	static TCHAR *SuperProcess2k[]	= 
	{
		"%windir%\\system32\\services.exe"
	};

	if(m_bIsWin9x)
	{
		return CompareString(
			SuperProcess9x, 
			sProcessName, 
			sizeof(SuperProcess9x)/sizeof(TCHAR*)
			);
	}
	else
	{
		return CompareString(
			SuperProcess2k, 
			sProcessName, 
			sizeof(SuperProcess2k)/sizeof(TCHAR*)
			);
	}

	return FALSE;
}

//
// 在字符串数组中查找字符串
//
BOOL CMemoryFile::FindString(TCHAR *pString, TCHAR *pSource)
{
	CString String = pString;
	if(String.Find(pSource) == -1)
		return FALSE;
	return TRUE;
}

//
// 比较字符串是否相等
//
BOOL CMemoryFile::CompareString(TCHAR **pString, LPCTSTR pSource, int nCount)
{
	TCHAR sPathName[MAX_PATH];

	for(int i = 0; i < nCount; i++)
	{
		ExpandEnvironmentStrings(pString[i], sPathName, MAX_PATH);
		if(_tcslen(pSource) != _tcslen(sPathName)) continue;
		if(_tcsnicmp(pSource, sPathName, _tcslen(pSource)) == 0)
		{
			ODS("IsSuperProcess.\n");
			return TRUE;
		}
	}
	return FALSE;
}

//
// 根据子工作模式判断是否放行。对于应用程序、网上邻居、ICMP分别都有
// 一个子工作模式，这个子工作模式优先与控管规则进行判断。
//
int CMemoryFile::CheckSubWorkMode(BYTE bWorkMode, SESSION *session)
{
	if(_tcscmp(m_pAclHeader->sSignature, ACL_HEADER_SIGNATURE) != 0)
		return XF_PASS;

	switch(bWorkMode)
	{
	case ACL_PASS_ALL:
		return XF_PASS;
	case ACL_DENY_ALL:
		return XF_DENY;
	case ACL_DENY_IN:
		if(session->bDirection == ACL_DIRECTION_IN)
			return XF_DENY;
		else
			return XF_PASS;
	case ACL_DENY_OUT:
		if(session->bDirection == ACL_DIRECTION_OUT)
			return XF_DENY;
		else
			return XF_PASS;
	case ACL_QUERY:
		return XF_QUERY;
	}
	return XF_PASS;
}

//
// 当没有找到符合的控管规则时按照控管规则的默认设置决定是放行、拒绝还是询问
//
int CMemoryFile::CheckQueryEx(BYTE bQueryEx)
{
	switch(bQueryEx)
	{
	case ACL_QUERY_PASS:
		return XF_PASS;
	case ACL_QUERY_DENY:
		return XF_DENY;
	case ACL_QUERY_QUERY:
		return XF_QUERY;
	}
	return XF_PASS;
}

//
// 发出询问请求，由XFILTER.EXE弹出询问对话框，并返回用户确认结果
//
int CMemoryFile::Query(SESSION *session, BYTE bStatus)
{
	ODS("Query");

	session->bStatus = bStatus;
	int bTimeout = 600;
	while(m_gbWorkMode == XF_QUERY_ALL && // 2002/05/24 add
		session->bStatus != SESSION_STATUS_FREE && bTimeout > 0)
	{
		Sleep(1000);
		bTimeout--;
	}

	if(bTimeout == 0) 
		return XF_DENY;
	//
	// 2002/05/24 add
	//
	else if(m_gbWorkMode == XF_PASS_ALL)
		return XF_PASS;
	else if(m_gbWorkMode == XF_DENY_ALL)
		return XF_DENY;

	return session->bAction;
}
#pragma comment( exestr, "B9D3B8FD2A6F676F71747B686B6E672B")
