#include "StdAfx.h"
#include "DriverManager.h"
#include <windows.h>

CDriverManager::CDriverManager(void)
{
}

CDriverManager::~CDriverManager(void)
{
}

// 启动驱动程序
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

		//创建 m_lpFullPathName
		TCHAR dir[_MAX_PATH] = {0} ;
		::GetCurrentDirectory( _MAX_PATH , dir ) ;
		m_strFullPathName = dir ;
		m_strFullPathName += L"\\" ;
		m_strFullPathName += lpDrivername ;
		m_strFullPathName += L".sys" ;

		TRACE(m_strFullPathName) ;
		//文件是否存在
		DWORD dwRet = ::GetFileAttributes(m_strFullPathName) ;
		if ( dwRet & FILE_ATTRIBUTE_DIRECTORY )
			return bResult ;

		//打开 scm
		hSCM = OpenSCManager( NULL , NULL , SC_MANAGER_ALL_ACCESS ) ;
		if ( !hSCM )
			return bResult ;

		//创建服务
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

		//启动服务
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

		//打开scm
		hSCM = OpenSCManager( NULL , NULL , SC_MANAGER_ALL_ACCESS ) ;
		if ( !hSCM )
			return bResult ;

		//打开服务
		hCS = OpenService( hSCM , m_strDriverName , SC_MANAGER_ALL_ACCESS ) ;
		if ( !hCS )
			return bResult ;

		//停止驱动
		ControlService( hCS , SERVICE_CONTROL_STOP , &srvsta) ;

		//删除服务
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

	/*打开设备，用我们自定的符号链接，响应驱动IRP_MJ_CREATE*/  
	CString strDrv = "\\\\.\\" ;
	strDrv += lpDrivername ;

	HANDLE hDevice = CreateFile( strDrv ,GENERIC_READ | GENERIC_WRITE , 0 , NULL , CREATE_ALWAYS , FILE_ATTRIBUTE_NORMAL , NULL ) ;
	if ( INVALID_HANDLE_VALUE == hDevice )
		return bResult ;

	memset(lpOutBuffer, 0, nOutBufferSize);    

	/*控制设备，响应驱动IRP_MJ_DEVICE_CONTROL*/  
	bResult = DeviceIoControl( hDevice, dwIoControlCode, NULL, 0, lpOutBuffer, nOutBufferSize, lpBytesReturned, NULL) ;   
	CloseHandle(hDevice);

	return bResult ;
}