/* Copyright (c) 2002-2005 Vladislav Goncharov.
 *
 * Redistribution and use in source forms, with and without modification,
 * are permitted provided that this entire comment appears intact.
 *
 * Redistribution in binary form may occur without any restrictions.
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 */
 
// $Id: main.c,v 1.3 2003/05/16 14:06:24 dev Exp $

/*
 * TdiFw helper service.
 * SCM & command line related stuff:
 */

#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>

#include "tdifw_svc.h"

#define CONFIG_SUBKEY	"SYSTEM\\CurrentControlSet\\Services\\tdifw"

BOOL g_console = TRUE;

static SERVICE_STATUS          ssStatus;       // current status of the service
static SERVICE_STATUS_HANDLE   sshStatusHandle;

static void		AddEventSource(const char *ident);

static void		install_service(const char *config);
static void		remove_service(void);

static VOID WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv);
static VOID WINAPI service_ctrl(DWORD dwCtrlCode);

static BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);

static BOOL add_config_info(HANDLE schService, const char *config);

int
main(int argc, char **argv)
{
    static SERVICE_TABLE_ENTRY dispatch_table[] = {
        {"tdifw", service_main},
        {NULL, NULL}
    };

	_LEAK_CHECK;

	if (argc >= 2) {
		const char *param = argv[1];
		
		if (strcmp(param, "install") == 0) {
			if (argc < 3) {
				fprintf(stderr, "Use: tdifw install <config>\n");
				return -1;
			}
		
			install_service(argv[2]);
		
		} else if (strcmp(param, "remove") == 0) {
			remove_service();
		} else if (strcmp(param, "debug") == 0) {

			if (argc < 3) {
				fprintf(stderr, "Use: tdifw debug <config>\n");
				return -1;
			}

			if (start(argv[2])) {
				printf("press enter to exit...\n");
				getchar();
				printf("exiting...\n");

				stop();
			}

		} else if (strcmp(param, "listen") == 0) {		// tdifw specific
			enum_listen();
		} else if (strcmp(param, "conn") == 0) {		// tdifw specific
			enum_connect();
		} else {
			fprintf(stderr, "Use: tdifw install|remove|debug|listen|conn\n");
		}
	} else {

		g_console = FALSE;

		// run as service
		if (!StartServiceCtrlDispatcher(dispatch_table))
			winerr("main: StartServiceCtrlDispatcher");

	}

	return 0;
}

void
install_service(const char *config)
{
	SC_HANDLE	schService;
	SC_HANDLE	schSCManager;

	CHAR szPath[MAX_PATH];

	AddEventSource("tdifw");

	if (GetModuleFileName(NULL, szPath, sizeof(szPath)) == 0) {
		winerr("install_service: GetModuleFileName");
		return;
	}

	schSCManager = OpenSCManager(
						NULL,					// machine (NULL == local)
						NULL,					// database (NULL == default)
						SC_MANAGER_ALL_ACCESS);	// access required

	if (schSCManager != NULL) {

		schService = CreateService(
			schSCManager,				// SCManager database
			"tdifw",				    // name of service
			"TDI-based open source personal firewall",	// name to display
			SERVICE_ALL_ACCESS, 		// desired access
			SERVICE_WIN32_OWN_PROCESS,	// service type
			SERVICE_AUTO_START,		    // start type
			SERVICE_ERROR_NORMAL,		// error control type
			szPath, 					// service's binary
			NULL,						// no load ordering group
			NULL,						// no tag identifier
			NULL,						// dependencies
			NULL,						// LocalSystem account
			NULL);						// no password

		if (schService != NULL) {
			printf("tdifw service has been installed\n");

			if (!add_config_info(schService, config))
				fprintf(stderr, "Can't store config info! Service will use defaults.\n");

			CloseServiceHandle(schService);
		} else
			winerr("install_service: CreateService");

		CloseServiceHandle(schSCManager);
	}
	else
		winerr("install_service: OpenSCManager");
}

void
remove_service(void)
{
	SC_HANDLE	schService;
	SC_HANDLE	schSCManager;

	schSCManager = OpenSCManager(
						NULL,					// machine (NULL == local)
						NULL,					// database (NULL == default)
						SC_MANAGER_ALL_ACCESS); // access required
	
	if (schSCManager != NULL) {
		schService = OpenService(schSCManager, "tdifw", SERVICE_ALL_ACCESS);

		if (schService != NULL) {

			// try to stop the service
			if (ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus)) {
				printf("stopping...");
				Sleep(1000);

				while(QueryServiceStatus( schService, &ssStatus)) {
					if (ssStatus.dwCurrentState == SERVICE_STOP_PENDING) {
						printf(".");
						Sleep( 1000 );
					}
					else
						break;
				}

				printf("\n");

				if (ssStatus.dwCurrentState == SERVICE_STOPPED)
					printf("stopped\n");
				else
					printf("failed to stop\n");
			}

			// now remove the service
			if (DeleteService(schService))
				printf("service has been removed\n");
			else
				winerr("install_service: DeleteService");

			CloseServiceHandle(schService);
		}
		else
			winerr("install_service: OpenService");

		CloseServiceHandle(schSCManager);
	}
	else
		winerr("install_service: OpenSCManager");
}

VOID WINAPI
service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{
	HKEY hkey = NULL;
	char *config = NULL;
	DWORD type, config_size, status;

	// register our service control handler:
	//
	sshStatusHandle = RegisterServiceCtrlHandler("tdifw", service_ctrl);
	if (sshStatusHandle == 0) {
		winerr("install_service: RegisterServiceCtrlHandler");
		goto cleanup;
	}

	// SERVICE_STATUS members that don't change in example
	//
	ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ssStatus.dwServiceSpecificExitCode = 0;

	// report the status to the service control manager.
	//
	if (!ReportStatusToSCMgr(
		SERVICE_START_PENDING, // service state
		NO_ERROR,			   // exit code
		3000))				   // wait hint
		goto cleanup;

	/* get config name from registry */

	if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, CONFIG_SUBKEY, 0, KEY_QUERY_VALUE,
		&hkey)) != ERROR_SUCCESS) {
		SetLastError(status);
		winerr("RegOpenKeyEx");
		goto cleanup;
	}

	if ((status = RegQueryValueEx(hkey, "config", 0, &type, NULL, &config_size)) != ERROR_SUCCESS) {
		SetLastError(status);
		winerr("RegOpenKeyEx");
		goto cleanup;
	}

	if (type != REG_SZ) {
		error("Invalid type for config value in registry");
		SetLastError(ERROR_INVALID_DATA);
		goto cleanup;
	}

	config = (char *)malloc(config_size);
	if (config == NULL) {
		liberr("malloc");
		goto cleanup;
	}

	if ((status = RegQueryValueEx(hkey, "config", 0, NULL, config, &config_size)) != ERROR_SUCCESS) {
		SetLastError(status);
		winerr("RegOpenKeyEx");
		goto cleanup;
	}

	if (start(config)) {

		// start success

		// report the status to the service control manager.
		//
		if (!ReportStatusToSCMgr(
			SERVICE_RUNNING,       // service state
			NO_ERROR,              // exit code
			0))                    // wait hint
			goto cleanup;

		wait();

		SetLastError(0);
	}
	
cleanup:

	// try to report the stopped status to the service control manager.
	//
	if (sshStatusHandle != 0)
		ReportStatusToSCMgr(SERVICE_STOPPED, GetLastError(), 0);

	if (hkey != NULL)
		RegCloseKey(hkey);
	free(config);
}

//
//  FUNCTION: ReportStatusToSCMgr()
//
//  PURPOSE: Sets the current status of the service and
//           reports it to the Service Control Manager
//
//  PARAMETERS:
//    dwCurrentState - the state of the service
//    dwWin32ExitCode - error code to report
//    dwWaitHint - worst case estimate to next checkpoint
//
//  RETURN VALUE:
//    TRUE  - success
//    FALSE - failure
//
//  COMMENTS:
//
BOOL
ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

    if (dwCurrentState == SERVICE_START_PENDING)
        ssStatus.dwControlsAccepted = 0;
    else
        ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    ssStatus.dwCurrentState = dwCurrentState;
    ssStatus.dwWin32ExitCode = dwWin32ExitCode;
    ssStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_RUNNING || dwCurrentState == SERVICE_STOPPED)
        ssStatus.dwCheckPoint = 0;
    else
        ssStatus.dwCheckPoint = dwCheckPoint++;

    // Report the status of the service to the service control manager.
    //

    if (!SetServiceStatus(sshStatusHandle, &ssStatus)) {
        winerr("install_service: SetServiceStatus");
		return FALSE;
	}

    return TRUE;
}

//
//  FUNCTION: service_ctrl
//
//  PURPOSE: This function is called by the SCM whenever
//           ControlService() is called on this service.
//
//  PARAMETERS:
//    dwCtrlCode - type of control requested
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID WINAPI
service_ctrl(DWORD dwCtrlCode)
{
	// Handle the requested control code.
	//
	switch(dwCtrlCode) {
		// Stop the service.
		//
		// SERVICE_STOP_PENDING should be reported before
		// setting the Stop Event - hServerStopEvent - in
		// ServiceStop().  This avoids a race condition
		// which may result in a 1053 - The Service did not respond...
		// error.
		case SERVICE_CONTROL_STOP:
			ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 0);
			
			// stop it!
			stop();
			
			return;

		// Update the service status.
		//
		case SERVICE_CONTROL_INTERROGATE:
			break;

		// invalid control code
		//
		default:
			break;

	}

	ReportStatusToSCMgr(ssStatus.dwCurrentState, NO_ERROR, 0);
}

/* Taken from MSDN. */
void
AddEventSource(const char *ident)
{
	HKEY hk; 
	DWORD dwData; 
	char szFilePath[_MAX_PATH];
	char key[_MAX_PATH];
	
	// Add your source name as a subkey under the Application 
	// key in the EventLog registry key. 
	_snprintf(key, sizeof(key), "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\%s", ident);

	if (RegCreateKey(HKEY_LOCAL_MACHINE, key, &hk)) {
		printf("Could not create the registry key."); 
		exit(-1);
	}
 
	// Set the name of the message file. 
	GetModuleFileName(NULL, szFilePath, sizeof(szFilePath));
	// Add the name to the EventMessageFile subkey. 
 
	if (RegSetValueEx(hk,			  // subkey handle 
			"EventMessageFile", 	  // value name 
			0,						  // must be zero 
			REG_EXPAND_SZ,			  // value type 
			(LPBYTE) szFilePath,		   // pointer to value data 
			strlen(szFilePath) + 1)) {		 // length of value data 
		printf("Could not set the event message file."); 
		exit(-1);
	}
 
	// Set the supported event types in the TypesSupported subkey. 
 
	dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | 
		EVENTLOG_INFORMATION_TYPE | EVENTLOG_AUDIT_SUCCESS | EVENTLOG_AUDIT_FAILURE; 
 
	if (RegSetValueEx(hk,	   // subkey handle 
			"TypesSupported",  // value name 
			0,				   // must be zero 
			REG_DWORD,		   // value type 
			(LPBYTE) &dwData,  // pointer to value data 
			sizeof(DWORD))){	// length of value data 
		printf("Could not set the supported types."); 
		exit(-1);
	}
 
	RegCloseKey(hk); 
}

BOOL
add_config_info(HANDLE schService, const char *config)
{
	BOOL result = FALSE;
	HKEY hkey = NULL;
	DWORD status;

	if ((status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, CONFIG_SUBKEY,
		0, KEY_SET_VALUE, &hkey)) != ERROR_SUCCESS) {
		SetLastError(status);
		winerr("RegOpenKeyEx");
		goto done;
	}

	if ((status = RegSetValueEx(hkey, "config", 0, REG_SZ, config,
		strlen(config) + 1)) != ERROR_SUCCESS) {
		SetLastError(status);
		winerr("RegSetValueEx");
		goto done;
	}

	result = TRUE;

done:
	if (hkey != NULL)
		RegCloseKey(hkey);
	return result;
}
