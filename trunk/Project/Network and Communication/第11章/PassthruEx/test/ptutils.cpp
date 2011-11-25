//////////////////////////////////////////////
// ptutils.cpp�ļ�

#include <windows.h>
#include <winioctl.h>
#include <ntddndis.h>
#include <stdio.h>
#include <tchar.h>

#include "../driver/IOCOMMON.h"
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


//////////////////////////////////////

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

