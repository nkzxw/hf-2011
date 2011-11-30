#ifndef _LINK_H
#define _LINK_H

#include <ntddk.h>

BOOL Link_IsConnected () ;

NTSTATUS Link_QueryServer (LPVOID pRequestData, DWORD nRequestSize,
			   LPVOID pResponseData, LPDWORD pnResponseSize,
			   DWORD nMaxResponseSize) ;

NTSTATUS Link_CatchIrpDrv2App (PDEVICE_OBJECT pDeviceObject, PIRP pIRP) ;

NTSTATUS Link_CatchIrpApp2Drv (PDEVICE_OBJECT pDeviceObject, PIRP pIRP) ;

NTSTATUS Link_Init () ;

NTSTATUS Link_Uninit () ;

BOOL	Link_IsAppWaiting () ;

BOOL	Link_IsRequesting () ;


#endif
