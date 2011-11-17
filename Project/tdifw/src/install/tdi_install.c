/* Copyright (c) 2002-2005 Vladislav Goncharov.
 *
 * Redistribution and use in source forms, with and without modification,
 * are permitted provided that this entire comment appears intact.
 *
 * Redistribution in binary form may occur without any restrictions.
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 */
 
// $Id: tdi_install.c,v 1.5 2002/12/05 13:03:05 dev Exp $

/*
 * Installer for tdifw_drv.sys driver (Win >= 2k)
 * Also sets loading order between tcpip.sys & netbt.sys
 */

#include <windows.h>
#include <stdio.h>

static void		install_driver(void);
static void		uninstall_driver(void);

int
main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "use: %s install|remove\n", argv[0]);
		return 0;
	}

	if (strcmp(argv[1], "install") == 0)
		install_driver();
	else if (strcmp(argv[1], "remove") == 0)
		uninstall_driver();
	else
		fprintf(stderr, "Invalid command line! \"%s\" is not \"install\" or \"remove\"\n", argv[1]);

	return 0;
}

#define DRIVER_NAME		"tdifw_drv"
#define DRIVER_BINARY	"system32\\drivers\\tdifw_drv.sys"
#define DRIVER_GROUP	"PNP_TDI"
#define DRIVER_DEPENDS	"tcpip\0\0"

#define swap_dword(a, b)	\
	do {					\
		DWORD c =(a);		\
		(a) = (b);			\
		(b) = c;			\
	} while(0)

void
install_driver(void)
{
	SC_HANDLE sch, tdifw, tcpip, netbt;
	DWORD tdifw_tag, tcpip_tag, netbt_tag, n, tags[100], type, i;
	char buf[2048];
	QUERY_SERVICE_CONFIG *cfg = (QUERY_SERVICE_CONFIG *)buf;
	HKEY reg_key;
	LONG status;
	int n_type;
	BOOL has_tcpip, has_netbt, has_tdifw;

	// install driver like instdrv does

    sch = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (sch == NULL) {
		fprintf(stderr, "OpenSCManager: %d\n", GetLastError());
		return;
	}

    tdifw = CreateService(sch,
                            DRIVER_NAME,
                            DRIVER_NAME,
                            SERVICE_ALL_ACCESS,
                            SERVICE_KERNEL_DRIVER,
                            SERVICE_SYSTEM_START,
                            SERVICE_ERROR_NORMAL,
                            DRIVER_BINARY,
                            DRIVER_GROUP,
                            &tdifw_tag,
                            DRIVER_DEPENDS,
                            NULL,
                            NULL
                            );
	if (tdifw == NULL) {
		fprintf(stderr, "CreateService: %d\n", GetLastError());
		return;
	}

	// get tag for tcpip

	tcpip = OpenService(sch, "tcpip", SERVICE_ALL_ACCESS);
	if (tcpip == NULL) {
		fprintf(stderr, "OpenService(tcpip): %d\n", GetLastError());
		return;
	}

	if (!QueryServiceConfig(tcpip, cfg, sizeof(buf), &n)) {
		fprintf(stderr, "QueryServiceConfig(tcpip): %d\n", GetLastError());
		return;
	}
	tcpip_tag = cfg->dwTagId;

	// get tag for netbt

	netbt = OpenService(sch, "netbt", SERVICE_ALL_ACCESS);
	if (netbt == NULL) {
		fprintf(stderr, "OpenService(netbt): %d\n", GetLastError());
		return;
	}

	if (!QueryServiceConfig(netbt, cfg, sizeof(buf), &n)) {
		fprintf(stderr, "QueryServiceConfig(tcpip): %d\n", GetLastError());
		return;
	}
	netbt_tag = cfg->dwTagId;

	// change tags for all drivers

	// 1. tcpip
	// 2. tdifw
	// 3. netbt

	// get registry key

	status = RegOpenKey(HKEY_LOCAL_MACHINE,
		"SYSTEM\\CurrentControlSet\\Control\\GroupOrderList", &reg_key);
	if (status != ERROR_SUCCESS) {
		fprintf(stderr, "RegOpenKey: %d\n", status);
		return;
	}

	n = sizeof(tags) - sizeof(DWORD) * 3;		// reserve space for 3 new items
	status = RegQueryValueEx(reg_key, DRIVER_GROUP, NULL, &type, (LPBYTE)tags, &n);
	if (status != ERROR_SUCCESS) {
		fprintf(stderr, "RegQueryValueEx: %d\n", status);
		return;
	}

	// find each num

	n /= sizeof(DWORD);

	// try to find tag for tdifw if not found add it
	has_tdifw = FALSE;
	for (i = 1; i < n; i++)
		if (tags[i] == tdifw_tag) {
			has_tdifw = TRUE;
			break;
		}

	if (!has_tdifw)
		tags[n++] = tdifw_tag;

	has_tcpip = has_netbt = FALSE;
	n_type = 1;

	for (i = 1; i < n; i++)
		if (tags[i] == tcpip_tag || tags[i] == netbt_tag || tags[i] == tdifw_tag) {

			switch (n_type) {
			case 1:
				// 1. tcpip
				tags[i] = tcpip_tag;
				has_tcpip = TRUE;
				break;
			case 2:
				// 2. tdifw
				tags[i] = tdifw_tag;
				break;
			case 3:
				// 3. netbt_tag
				tags[i] = netbt_tag;
				has_netbt = TRUE;
				break;
			}
			n_type++;

		}

	if (!has_tcpip) {
		printf("Warning! tcpip with tag %d not found! Trying to restore registry GroupOrderList key...\n", tcpip_tag);
		
		// insert at the begin of list
		memmove(&tags[1], &tags[2], (n - 1) * sizeof(DWORD));
		tags[1] = tcpip_tag;
		n++;
	}
	if (!has_netbt) {
		printf("Warning! netbt with tag %d not found! Trying to restore registry GroupOrderList key...\n", netbt_tag);

		// append to the end of list
		tags[n++] = netbt_tag;
	}

	// save number of entries
	tags[0] = n - 1;

	// that's all. save new order

	status = RegSetValueEx(reg_key, DRIVER_GROUP, 0, type, (LPBYTE)tags, n * sizeof(DWORD));
	if (status != ERROR_SUCCESS) {
		fprintf(stderr, "RegSetValueEx: %d\n", status);
		return;
	}

	// ok
	RegCloseKey(reg_key);
	CloseServiceHandle(tcpip);
	CloseServiceHandle(tdifw);
	CloseServiceHandle(netbt);
	CloseServiceHandle(sch);

	printf("success\n");
}

void
uninstall_driver(void)
{
	SC_HANDLE sch, service;
	DWORD tags[100], n, type, i, service_tag;
	char buf[2048];
	HKEY reg_key;
	QUERY_SERVICE_CONFIG *cfg = (QUERY_SERVICE_CONFIG *)buf;
	int status;

    sch = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (sch == NULL) {
		fprintf(stderr, "OpenSCManager: %d\n", GetLastError());
		return;
	}

	// get tag for driver

	service = OpenService(sch, DRIVER_NAME, SERVICE_ALL_ACCESS);
	if (service == NULL) {
		fprintf(stderr, "OpenService: %d\n", GetLastError());
		return;
	}

	if (!QueryServiceConfig(service, cfg, sizeof(buf), &n)) {
		fprintf(stderr, "QueryServiceConfig(tcpip): %d\n", GetLastError());
		return;
	}
	service_tag = cfg->dwTagId;

	// remove tag from registry

	status = RegOpenKey(HKEY_LOCAL_MACHINE,
		"SYSTEM\\CurrentControlSet\\Control\\GroupOrderList", &reg_key);
	if (status != ERROR_SUCCESS) {
		fprintf(stderr, "RegOpenKey: %d\n", status);
		return;
	}

	n = sizeof(tags);
	status = RegQueryValueEx(reg_key, DRIVER_GROUP, NULL, &type, (LPBYTE)tags, &n);
	if (status != ERROR_SUCCESS) {
		fprintf(stderr, "RegQueryValueEx: %d\n", status);
		return;
	}

	n /= sizeof(DWORD);

	for (i = 1; i < n; i++)
		if (tags[i] == service_tag) {
			memmove(&tags[i], &tags[i + 1], (n - i - 1) * sizeof(DWORD));
			n--;
			break;
		}

	tags[0] = n - 1;

	status = RegSetValueEx(reg_key, DRIVER_GROUP, 0, type, (LPBYTE)tags, n * sizeof(DWORD));
	if (status != ERROR_SUCCESS) {
		fprintf(stderr, "RegSetValueEx: %d\n", status);
		return;
	}

	RegCloseKey(reg_key);

	// remove service

	if (!DeleteService(service)) {
		if (GetLastError() != ERROR_SERVICE_MARKED_FOR_DELETE) {
			fprintf(stderr, "DeleteService: %d\n", GetLastError());
			return;
		}
		printf("Restart Windows to changes has effect\n");
	}

	CloseServiceHandle(service);
	CloseServiceHandle(sch);

	printf("success\n");
}
