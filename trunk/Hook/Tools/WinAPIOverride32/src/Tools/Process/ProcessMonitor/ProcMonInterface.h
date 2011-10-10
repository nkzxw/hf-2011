/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//-----------------------------------------------------------------------------
// Object: provide a class helper for using ProcMon driver
//         It allows to:
//              - start/stop the driver checking if another application is using it
//              - start/stop monitoring
//              - setting callback for process creation or ends
//              - call callback in multiples thread. So you can do some blocking 
//                  operations in your callbacks, other events will occur
//-----------------------------------------------------------------------------


#pragma once

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <windows.h>
#include "ProcMonExport.h"

#include "../../APIError/APIError.h"
#include "../../Driver/Driver.h"

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define PROC_MON_SYS _T("ProcMon.sys")
#define PROC_MON_NAME _T("ProcMon")
#define DRIVER_NAME _T("\\\\.\\ProcMon")
#define MSG_DRIVER_ALREADY_RUNNING _T("ProcMon driver is already running.\r\nIf your are SURE there's no other application using it, click yes,else click no.\r\nDo you want to continue ?")
#define DRIVER_STOPPING_MAX_TIME 5000

typedef void (*PROC_MON_CALLBACK)(HANDLE hParentId,HANDLE hProcessId);

class CProcMonInterface;

typedef struct tagPROC_MON_THREAD_CALLBACK_PARAM
{
    CProcMonInterface* pProcMonInterface;
    PROCESS_CALLBACK_INFO ProcInfo;
}PROC_MON_THREAD_CALLBACK_PARAM,*PPROC_MON_THREAD_CALLBACK_PARAM;

//---------------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------------
DWORD WINAPI CProcMonInterfaceThreadProc(CProcMonInterface* ProcMonInterface);
DWORD WINAPI CProcMonInterfaceCallBacksThreadProc(PPROC_MON_THREAD_CALLBACK_PARAM Param);


//---------------------------------------------------------------------------
// Class
//---------------------------------------------------------------------------
class CProcMonInterface
{
private:
    PROC_MON_CALLBACK StartProcCallBack;
    PROC_MON_CALLBACK StopProcCallBack;
    HANDLE hEvtStartMonitoring;
    HANDLE hEvtStopMonitoring;
    HANDLE hEvtStopDriver;
    HANDLE hWatchingThread;
    BOOL bStarted;
    static DWORD WINAPI ThreadProc(CProcMonInterface* ProcMonInterface);
    static DWORD WINAPI CallBacksThreadProc(PPROC_MON_THREAD_CALLBACK_PARAM Param);
public:
    CProcMonInterface(void);
    ~CProcMonInterface(void);
    void SetProcessStartCallBack(PROC_MON_CALLBACK StartProcCallBack);
    void SetProcessStopCallBack(PROC_MON_CALLBACK StopProcCallBack);
    BOOL StartMonitoring();
    BOOL StopMonitoring();
    BOOL StartDriver();
    BOOL StopDriver();
};
