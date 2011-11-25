///////////////////////////////////////
// ptutils.cpp�ļ�

#include <windows.h>
#include <winioctl.h>
#include <ntddndis.h>
#include <stdio.h>
#include <tchar.h>

#include "IOCOMMON.h"
#include "ptutils.h"


HANDLE PtOpenControlDevice()
{
	// �򿪵����������������豸�ľ��
	HANDLE hFile = ::CreateFile(
		_T("\\\\.\\PassThru"),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	return hFile;
}

HANDLE PtOpenAdapter(PWSTR pszAdapterName)
{
	// �򿪿����豸������
	HANDLE hAdapter = PtOpenControlDevice();
	if(hAdapter == INVALID_HANDLE_VALUE)
		return INVALID_HANDLE_VALUE;

	
	// ȷ�����������Ƶĳ���
	int nBufferLength = wcslen((PWSTR)pszAdapterName) * sizeof(WCHAR);

	// ����IOCTL_PTUSERIO_OPEN_ADAPTER���ƴ��룬��������������
	DWORD dwBytesReturn;
	BOOL bOK = ::DeviceIoControl(hAdapter, IOCTL_PTUSERIO_OPEN_ADAPTER, 
					pszAdapterName, nBufferLength, NULL, 0, &dwBytesReturn, NULL);
	// �����
	if(!bOK)
	{
		::CloseHandle(hAdapter);
		return INVALID_HANDLE_VALUE;
	}
	return hAdapter;
}

BOOL PtAdapterRequest(HANDLE hAdapter, PPTUSERIO_OID_DATA pOidData, BOOL bQuery)
{
	if(hAdapter == INVALID_HANDLE_VALUE)
		return FALSE;
	// ����IOCTL
	DWORD dw;
	int bRet = ::DeviceIoControl(
		hAdapter, bQuery ? IOCTL_PTUSERIO_QUERY_OID : IOCTL_PTUSERIO_SET_OID,
		pOidData, sizeof(PTUSERIO_OID_DATA) -1 + pOidData->Length,
		pOidData, sizeof(PTUSERIO_OID_DATA) -1 + pOidData->Length, &dw, NULL);

	return bRet;
}

//

// ��ѯ����״̬
BOOL PtQueryStatistics(HANDLE hAdapter, PPassthruStatistics pStats)
{
	ULONG nStatsLen = sizeof(PassthruStatistics);
	BOOL bRet = ::DeviceIoControl(hAdapter, 
		IOCTL_PTUSERIO_QUERY_STATISTICS, NULL, 0, pStats, nStatsLen, &nStatsLen, NULL);

	return bRet;
}

// ����ͳ������
BOOL PtResetStatistics(HANDLE hAdapter)
{
	DWORD dwBytes;
	BOOL bRet = ::DeviceIoControl(hAdapter, 
		IOCTL_PTUSERIO_RESET_STATISTICS, NULL, 0, NULL, 0, &dwBytes, NULL);
	return bRet;
}

// �����������һ�����˹���
BOOL PtAddFilter(HANDLE hAdapter, PPassthruFilter pFilter)
{
	ULONG nFilterLen = sizeof(PassthruFilter);
	BOOL bRet = ::DeviceIoControl(hAdapter, IOCTL_PTUSERIO_ADD_FILTER, 
			pFilter, nFilterLen, NULL, 0, &nFilterLen, NULL);
	return bRet;
}

// ����������ϵĹ��˹���
BOOL PtClearFilter(HANDLE hAdapter)
{
	DWORD dwBytes;
	BOOL bRet = ::DeviceIoControl(hAdapter, 
		IOCTL_PTUSERIO_ADD_FILTER, NULL, 0, NULL, 0, &dwBytes, NULL);
	return bRet;
}

//////////////////////////////////////////////////////////

BOOL CIMAdapters::EnumAdapters(HANDLE hControlDevice)
{
	DWORD dwBufferLength = sizeof(m_buffer);
	BOOL bRet = ::DeviceIoControl(hControlDevice, IOCTL_PTUSERIO_ENUMERATE, 
		NULL, 0, m_buffer, dwBufferLength, &dwBufferLength, NULL);
	if(!bRet)
		return FALSE;
	
	// ��������������
	m_nAdapters = (ULONG)((ULONG*)m_buffer)[0];
	
	// �����m_buffer�л�ȡ���������ƺͷ�����������
	// ָ���豸����
	WCHAR *pwsz = (WCHAR *)((ULONG *)m_buffer + 1);
	int i = 0;
	m_pwszVirtualName[i] = pwsz;
	while(*(pwsz++) != NULL)
	{
		while(*(pwsz++) != NULL)
		{ ; }
		
		m_pwszAdapterName[i] = pwsz;
		
		while(*(pwsz++) != NULL)
		{ ; }
		
		if(++i >= MAX_ADAPTERS)
			break;	
		
		m_pwszVirtualName[i] = pwsz;
	}
	
	return TRUE;
}


BOOL IMClearRules()
{
	BOOL bRet = TRUE;
	HANDLE hControlDevice = PtOpenControlDevice();
	CIMAdapters adapters;
	if(!adapters.EnumAdapters(hControlDevice))
		return FALSE;

	HANDLE hAdapter;
	for(int i=0; i<adapters.m_nAdapters; i++)
	{
		hAdapter = PtOpenAdapter(adapters.m_pwszAdapterName[i]);
		if(hAdapter != INVALID_HANDLE_VALUE)
		{
			PtClearFilter(hAdapter);
			::CloseHandle(hAdapter);
		}
		else
		{
			bRet = FALSE;
			break;
		}
	}
	return bRet;
}

BOOL IMSetRules(PPassthruFilter pRules, int nRuleCount)
{
	BOOL bRet = TRUE;
	HANDLE hControlDevice = PtOpenControlDevice();
	CIMAdapters adapters;
	if(!adapters.EnumAdapters(hControlDevice))
		return FALSE;

	HANDLE hAdapter;
	int i, j;
	for(i=0; i<adapters.m_nAdapters; i++)
	{
		hAdapter = PtOpenAdapter(adapters.m_pwszAdapterName[i]);
		if(hAdapter == INVALID_HANDLE_VALUE)
		{
			bRet = FALSE;
			break;
		}
		else
		{
			for(j=0; i<nRuleCount; j++)
			{
				if(!PtAddFilter(hAdapter, &pRules[j]))
				{
					bRet = FALSE;
					break;
				}
			}
			::CloseHandle(hAdapter);
		}
	}
	::CloseHandle(hControlDevice);
	return bRet;
}