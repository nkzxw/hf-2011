#include "StdAfx.h"
#include "DriverManager.h"
#include <windows.h>

CDriverManager::CDriverManager(void)
{
}

CDriverManager::~CDriverManager(void)
{
}

// ������������
BOOL CDriverManager::StartDriver(LPCTSTR lpDrivername)
{
	DWORD dwLastError = 0 ;
	BOOL bResult = FALSE ;
	SC_HANDLE hSCM = NULL , hCS = NULL ;
	m_strDriverName = lpDrivername ;
	
	__try{
		if ( m_strDriverName.IsEmpty() )
		{
			SetLastError(ERROR_INVALID_PARAMETER) ;
			return bResult ;
		}

		//���� m_lpFullPathName
		TCHAR dir[_MAX_PATH] = {0} ;
		::GetCurrentDirectory( _MAX_PATH , dir ) ;
		m_strFullPathName = dir ;
		m_strFullPathName += L"\\" ;
		m_strFullPathName += lpDrivername ;
		m_strFullPathName += L".sys" ;

		TRACE(m_strFullPathName) ;
		//�ļ��Ƿ����
		DWORD dwRet = ::GetFileAttributes(m_strFullPathName) ;
		if ( dwRet & FILE_ATTRIBUTE_DIRECTORY )
			return bResult ;

		//�� scm
		hSCM = OpenSCManager( NULL , NULL , SC_MANAGER_ALL_ACCESS ) ;
		if ( !hSCM )
			return bResult ;

		//��������
		hCS = ::CreateService( hSCM
			, m_strDriverName
			, m_strDriverName
			, SC_MANAGER_ALL_ACCESS
			, SERVICE_KERNEL_DRIVER 
			, SERVICE_AUTO_START
			, SERVICE_ERROR_IGNORE
			, m_strFullPathName
			, NULL
			, NULL
			, NULL
			, NULL
			, NULL ) ;
		if ( !hCS )
		{
			dwLastError = ::GetLastError() ;
			if ( dwLastError == ERROR_SERVICE_EXISTS )
			{
				hCS = OpenService( hSCM , m_strDriverName , SC_MANAGER_ALL_ACCESS ) ;
				if ( !hCS )
					return bResult ;
			}
		}

		//��������
		if ( !StartService( hCS , 0 , NULL ) )
			return bResult ;
		bResult = TRUE ;
		return bResult ;
	}
	__finally
	{
		if ( !bResult )
			dwLastError = GetLastError() ;

 		if ( hCS )
			CloseServiceHandle(hCS) ;
		if ( hSCM )
			CloseServiceHandle(hSCM) ;

		m_strDriverName.Empty() ;
		m_strFullPathName.Empty() ;

		if ( !bResult )
			SetLastError(dwLastError) ;
	}
}

BOOL CDriverManager::StopDriver(LPCTSTR lpDrivername)
{
	DWORD dwLastError = 0 ;
	BOOL bResult = FALSE ;
	SC_HANDLE hSCM = NULL , hCS = NULL ;
	SERVICE_STATUS srvsta = {0} ;
	m_strDriverName = lpDrivername ;

	__try{
		if ( m_strDriverName.IsEmpty() )
		{
			SetLastError(ERROR_INVALID_PARAMETER) ;
			return bResult ;
		}

		//��scm
		hSCM = OpenSCManager( NULL , NULL , SC_MANAGER_ALL_ACCESS ) ;
		if ( !hSCM )
			return bResult ;

		//�򿪷���
		hCS = OpenService( hSCM , m_strDriverName , SC_MANAGER_ALL_ACCESS ) ;
		if ( !hCS )
			return bResult ;

		//ֹͣ����
		ControlService( hCS , SERVICE_CONTROL_STOP , &srvsta) ;

		//ɾ������
		bResult = DeleteService( hCS ) ;

		return bResult ;
	}
	__finally{
		if ( !bResult )
			dwLastError = GetLastError() ;

 		if ( hCS )
			CloseServiceHandle( hCS ) ;
		if ( hSCM )
			CloseServiceHandle( hSCM ) ;
		
		m_strDriverName.Empty() ;

		if ( !bResult )
			SetLastError(dwLastError) ;
	}
}

BOOL CDriverManager::CallDriver( __in LPCTSTR lpDrivername, __in DWORD dwIoControlCode, __out LPVOID lpOutBuffer, __in DWORD nOutBufferSize, __out LPDWORD lpBytesReturned )
{
	BOOL bResult = FALSE ;

	/*���豸���������Զ��ķ������ӣ���Ӧ����IRP_MJ_CREATE*/  
	CString strDrv = "\\\\.\\" ;
	strDrv += lpDrivername ;

	HANDLE hDevice = CreateFile( strDrv ,GENERIC_READ | GENERIC_WRITE , 0 , NULL , CREATE_ALWAYS , FILE_ATTRIBUTE_NORMAL , NULL ) ;
	if ( INVALID_HANDLE_VALUE == hDevice )
		return bResult ;

	memset(lpOutBuffer, 0, nOutBufferSize);    

	/*�����豸����Ӧ����IRP_MJ_DEVICE_CONTROL*/  
	bResult = DeviceIoControl( hDevice, dwIoControlCode, NULL, 0, lpOutBuffer, nOutBufferSize, lpBytesReturned, NULL) ;   
	CloseHandle(hDevice);

	return bResult ;
}