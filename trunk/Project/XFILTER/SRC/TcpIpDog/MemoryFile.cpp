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
// Author & Create Date: Tony Zhu, 2002/03/18
//
// Project: XFILTER 2.0
//
// Copyright:	2002-2003 Passeck Technology.
//
// Abstract:
//
//
// ��飺
//		�عܹ���ȶ�ģ��
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
// ����XFILTER.EXE�Ľ���ID������·��
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
// �����ܹ���ģʽ
//
void SetWorkMode(BYTE bWorkMode)
{
	m_gbWorkMode = bWorkMode;
}

//
// ����عܹ����ڴ�ӳ���ļ����
//
void SetMemoryFile(DWORD hMemoryFile)
{
	m_ghMemoryFile = hMemoryFile;
}

//
// ����عܹ����ڴ�ӳ���ļ��Ĵ�С
//
void SetMaxSize(DWORD dwMaxSize)
{
	m_gdwMaxSize = dwMaxSize;
}

//
// �����ڴ�ӳ���ļ��Ļ���ַ
//
void SetBaseAddress(DWORD dwBaseAddress)
{
	m_gdwBaseAddress = dwBaseAddress;
}

//
// ���ÿعܹ����Ƿ����ڸ���
//
void SetRefresh(BYTE bIsRefresh)
{
	m_gbAclIsRefresh = bIsRefresh;
}

//
// ���ÿعܹ�����¼���
//
void SetUpdateVersion(DWORD dwUpdateVersion)
{
	m_gdwUpdateVersion = dwUpdateVersion;
}

//
// �عܹ�����¼�����һ
//
void RefenceUpdateVersion()
{
	m_gdwUpdateVersion ++;
}

//
// ���ص�ǰ���ܹ���ģʽ
//
BYTE GetWorkMode()
{
	return m_gbWorkMode;
}

//
// �õ��عܹ����ڴ�ӳ���ļ��ľ��
//
HANDLE GetMemoryFile()
{
	return (HANDLE)m_ghMemoryFile;
}

//
// �õ��عܹ����Ƿ��ڸ���״̬
//
BYTE GetRefresh()
{
	return m_gbAclIsRefresh;
}

//
// �õ��عܹ�����¼���
//
DWORD GetUpdateVersion()
{
	return m_gdwUpdateVersion;
}

//==========================================================================
// �عܹ���ȶ���
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
// ʹ�عܹ�������ʹ�õļ����� + 1
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
// ʹ�عܹ�������ʹ�õļ����� - 1
//
void CMemoryFile::Derefence()
{
	if(m_pAclHeader == NULL)
		return;
	m_pAclHeader->wRefenceCount--;
	DP1("DerefenceCount: %u\n", m_pAclHeader->wRefenceCount);
}

//
// �򿪿عܹ�����ڴ�ӳ���ļ�
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
// ����ӳ���ÿһ���û���ַ�ռ�ĸ�����ͬ��������Ҫͨ��ƫ����
// �������ȷ�ĵ�ַ
//
DWORD CMemoryFile::Nat(PVOID pAddress)
{
	if(pAddress == NULL) return 0;
	return ((DWORD)pAddress + m_dwOffset);
}

//
// ���ݿعܹ����session��¼���кϷ�����֤���ܿغ���
//
int CMemoryFile::CheckAcl(SESSION *session)
{
	static int iRet;
	__try
	{
		//
		// �ھ����عܹ����ж�֮ǰ������ͨ����������֤
		//
		iRet = GetAccessWithoutAcl(session);
		if(iRet != XF_FILTER) return iRet;
		//
		// ͨ��Ӧ�ó���عܹ��������֤
		//
		iRet = CheckApp(session);
		iRet = GetAccessFromSecurity(iRet);
		if(iRet == XF_QUERY) 
		{
			//
			// ����ѯ������
			//
			iRet = Query(session, SESSION_STATUS_QUERY_APP);
			session->bIsQuery = TRUE;
			return iRet;
		}
		if(iRet != XF_PASS) return iRet;
		//
		// ��վ�������֤
		//
		iRet = CheckWeb(session);
		iRet = GetAccessFromSecurity(iRet);
		if(iRet == XF_QUERY)
		{
			//
			// ����ѯ������
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
// ���ݰ�ȫ�ȼ��ж��Ƿ���Ҫ���С�����Ҫѯ�ʶ���ȫ�ȼ�Ϊ��ʱĬ�Ϸ���
// ���ߵ���Ҫѯ�ʰ�ȫ�ȼ�Ϊ��ʱ������Ĭ�Ϸ��С�
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
// ���ܹ���ģʽ�õ��ع�״̬
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
// Ӧ�ó������ȽϺ���
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
			// ѭ���Ƚ�Ӧ�ó������
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
// ��վ����ȶԺ���
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
// ��������
//
int CMemoryFile::CheckTorjan()
{
	return XF_PASS;
}

//
// ��������
//
int CMemoryFile::ProtectApp()
{
	return XF_PASS;
}

//
// ����ʱ������ڵõ��Զ����ʱ������
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
// ����Ip��ַ�õ��Զ������������
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
// Ip ��ַ�αȽϺ���
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
// ��ʹ�ÿعܹ����жϷ���Ƿ�������У���ʹ�ÿعܹ����ж�֮ǰһ��
// ���ȵ��ô˺��������жϣ��������XF_FILTER����Ҫ��һ��ʹ�ÿعܹ���
// �����жϣ������XF_PASS����XF_DENY��ֱ�ӽ��з��л��߾ܾ���
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
// �ж��Ƿ��Ǳ���IP��ַ
// ����ֵ��
//		TRUE:	�Ǳ���IP
//		FALSE:	���Ǳ���IP
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
// ���ܹ���ģʽ�õ��ع�״̬
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
// �õ���ǰWindows�汾
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
// �ж��Ƿ���XFILTER.EXE
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
// �ж��Ƿ���XFILTER.EXE��������Ҫ����ϵͳ���̣���Щ����Ĭ�Ϸ���
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
// ���ַ��������в����ַ���
//
BOOL CMemoryFile::FindString(TCHAR *pString, TCHAR *pSource)
{
	CString String = pString;
	if(String.Find(pSource) == -1)
		return FALSE;
	return TRUE;
}

//
// �Ƚ��ַ����Ƿ����
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
// �����ӹ���ģʽ�ж��Ƿ���С�����Ӧ�ó��������ھӡ�ICMP�ֱ���
// һ���ӹ���ģʽ������ӹ���ģʽ������عܹ�������жϡ�
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
// ��û���ҵ����ϵĿعܹ���ʱ���տعܹ����Ĭ�����þ����Ƿ��С��ܾ�����ѯ��
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
// ����ѯ��������XFILTER.EXE����ѯ�ʶԻ��򣬲������û�ȷ�Ͻ��
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
