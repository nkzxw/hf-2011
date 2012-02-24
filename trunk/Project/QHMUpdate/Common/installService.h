#ifndef _INSTALL_SERVER_H_
#define _INSTALL_SERVER_H_

#include<windows.h>   
#include <winsvc.h>   

SERVICE_STATUS_HANDLE hServiceStatus;
SERVICE_STATUS status;
char *g_serviceName = NULL;

typedef void (* LPFUNC_START) ();
typedef void (* LPFUNC_STOP)  ();

LPFUNC_START func_start;
LPFUNC_STOP func_stop;

void 
WINAPI 
ServiceStrl(
	DWORD dwOpcode
	)
{
    switch (dwOpcode)
    {
    case SERVICE_CONTROL_STOP:
		func_stop ();
		status.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(hServiceStatus, &status);
        break;
    case SERVICE_CONTROL_PAUSE:
        break;
    case SERVICE_CONTROL_CONTINUE:
        break;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        break;
    default:
        break;
    }
}

void 
WINAPI 
ServiceMain(
	)
{
    status.dwCurrentState = SERVICE_START_PENDING;
	status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	//注册服务控制
    hServiceStatus = RegisterServiceCtrlHandler(g_serviceName, ServiceStrl);
    if (hServiceStatus == NULL)
    {
        return;
    }
    SetServiceStatus(hServiceStatus, &status);

    status.dwWin32ExitCode = S_OK;
    status.dwCheckPoint = 0;
    status.dwWaitHint = 0;
	status.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(hServiceStatus, &status);

	func_start ();

	//模拟服务的运行，10后自动退出。应用时将主要任务放于此即可
	Sleep (1000);
    status.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(hServiceStatus, &status);
}

void Init()
{
    hServiceStatus = NULL;
    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState = SERVICE_STOPPED;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    status.dwWin32ExitCode = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint = 0;
    status.dwWaitHint = 0;
}

void 
StartServer(
	LPSTR lpServerName,
	LPFUNC_START pfunc_start,
	LPFUNC_STOP  pfunc_stop
	)   
{     
	Init();

	int len = strlen (lpServerName);
	g_serviceName = (char *)malloc (len + 1);
	strcpy (g_serviceName, lpServerName);

	func_start = pfunc_start;
	func_stop = pfunc_stop;
    
	SERVICE_TABLE_ENTRY st[] =
    {
        { lpServerName, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
        { NULL, NULL }
    };

	if (!::StartServiceCtrlDispatcher(st)){
		//TODO
	}
}  


/*
* 检查服务是否已经安装
*/
BOOL 
IsInstall(
	LPCSTR lpServiceName
	)
{
    BOOL bResult = FALSE;
    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM != NULL)
    {
        SC_HANDLE hService = ::OpenService(hSCM, lpServiceName, SERVICE_QUERY_CONFIG);
        if (hService != NULL)
        {
            bResult = TRUE;
            ::CloseServiceHandle(hService);
        }
        ::CloseServiceHandle(hSCM);
	}
    return bResult;
}

/*
*安装服务
*/
BOOL 
Install(
	LPCSTR lpServiceName,
	LPCSTR lpDisplayName
	)
{
	if (IsInstall(lpServiceName)) {
        return TRUE;
	}

	SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM == NULL){
        return FALSE;
    }

    TCHAR szFilePath[MAX_PATH];
    ::GetModuleFileName(NULL, szFilePath, MAX_PATH);

	//创建服务
    SC_HANDLE hService = ::CreateService(hSCM, 
									lpServiceName, 
									lpDisplayName,
									SERVICE_ALL_ACCESS, 
									SERVICE_WIN32_OWN_PROCESS,
									SERVICE_DEMAND_START, 
									SERVICE_ERROR_NORMAL,
									szFilePath, 
									NULL, NULL, "", NULL, NULL);
    if (NULL == hService)
    {
        ::CloseServiceHandle(hSCM);
        return FALSE;
    }

    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);

	return TRUE;
}

/*
* 卸载服务
*/
BOOL 
Uninstall(
	LPCSTR lpServiceName
	)
{
	if (!IsInstall(lpServiceName)) {
        return TRUE;
	}

    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM == NULL)
    {
        return FALSE;
    }

    SC_HANDLE hService = ::OpenService(hSCM, lpServiceName, SERVICE_STOP | DELETE);
    if (hService == NULL)
    {
        ::CloseServiceHandle(hSCM);
        return FALSE;
    }

	SERVICE_STATUS status;
    ::ControlService(hService, SERVICE_CONTROL_STOP, &status);

	//删除服务
    BOOL bDelete = ::DeleteService(hService);
    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);

	if (bDelete){
        return TRUE;
	}

	return FALSE;
}
   
#endif //_INSTALL_SERVER_H_