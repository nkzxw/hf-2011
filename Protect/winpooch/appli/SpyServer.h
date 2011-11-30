/******************************************************************/
/*                                                                */
/*  Winpooch : Windows Watchdog                                   */
/*  Copyright (C) 2004-2005  Benoit Blanchon                      */
/*                                                                */
/*  This program is free software; you can redistribute it        */
/*  and/or modify it under the terms of the GNU General Public    */
/*  License as published by the Free Software Foundation; either  */
/*  version 2 of the License, or (at your option) any later       */
/*  version.                                                      */
/*                                                                */
/*  This program is distributed in the hope that it will be       */
/*  useful, but WITHOUT ANY WARRANTY; without even the implied    */
/*  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       */
/*  PURPOSE.  See the GNU General Public License for more         */
/*  details.                                                      */
/*                                                                */
/*  You should have received a copy of the GNU General Public     */
/*  License along with this program; if not, write to the Free    */
/*  Software Foundation, Inc.,                                    */
/*  675 Mass Ave, Cambridge, MA 02139, USA.                       */
/*                                                                */
/******************************************************************/


#ifndef _SPY_SERVER_H
#define _SPY_SERVER_H


#include "FilterSet.h"
#include "ProcList.h"	//<- for PROCADDR
#include "ScanResult.h"

// TO BE REMOVED
#define WM_ADDLOG (WM_USER+1)

#define WM_SPYNOTIFY (WM_USER+36)
#define SN_EVENTLOGGED		1
#define SN_FILTERCHANGED	2
#define SN_ALERT		3
#define SN_PROCESSCREATED	4
#define SN_PROCESSTERMINATED	5
#define SN_PIDCHANGED		6
#define SN_PROCESSCHANGED	7


BOOL	SpySrv_Init (HWND) ;

VOID	SpySrv_Uninit () ;

BOOL	SpySrv_InstallDriver (BOOL bPermanently) ;

BOOL	SpySrv_UninstallDriver () ;

VOID	SpySrv_ReadFilterFile () ;

VOID	SpySrv_WriteFilterFile () ;

BOOL	SpySrv_Start () ;

VOID	SpySrv_Stop () ;

HFILTERSET SpySrv_GetFilterSet () ;

VOID SpySrv_SetFilterSet (HFILTERSET hNewFilterSet) ;

BOOL	SpySrv_AddRuleForProgram (PFILTRULE pRule, LPCTSTR szPath) ;

SCANRESULT SpySrv_ScanFile (LPCTSTR szFilePath, BOOL bBackground) ;

BOOL	SpySrv_KillProcess (PROCADDR nProcessAddress, BOOL bKernelModeKill) ;

BOOL	SpySrv_IgnoreProcess (PROCADDR nProcessAddress, BOOL bIngore) ;

VOID	SpySrv_LockFilterSet () ;

VOID	SpySrv_UnlockFilterSet () ;

BOOL	SpySrv_SendFilterSetToDriver () ;

BOOL	SpySrv_SetScannerExePath (LPCWSTR szScannerExe) ;

BOOL	SpySrv_SetScanFilters (LPCWSTR * pszFilters, UINT nFilters) ;

typedef BOOL (*SYNCCACHECALLBACK)(VOID * pUserPtr, ULONG nIdentifier, LPCWSTR wszFilePath, SCANRESULT nResult, LARGE_INTEGER*pliScanTime) ;

BOOL	SpySrv_SyncScanCache (VOID * pUserPtr, SYNCCACHECALLBACK pfnCallback, LARGE_INTEGER* pliLastSyncTime,
			      ULONG * pnFirstIdentifier, ULONG * pnLastIdentifier) ;

BOOL	SpySrv_AddFileToCache (LPCWSTR szFilePath, SCANRESULT nResult, LARGE_INTEGER * pliScanTime) ;

#endif
