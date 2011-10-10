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

#include "procmoninterface.h"

CProcMonInterface::CProcMonInterface(void)
{
    this->StartProcCallBack=NULL;
    this->StopProcCallBack=NULL;

    // auto reset event initial state non signaled
    this->hEvtStartMonitoring=CreateEvent(NULL,FALSE,FALSE,NULL);
    this->hEvtStopMonitoring=CreateEvent(NULL,FALSE,FALSE,NULL);
    this->hEvtStopDriver=CreateEvent(NULL,FALSE,FALSE,NULL);

    this->hWatchingThread=0;

    this->bStarted=FALSE;
}

CProcMonInterface::~CProcMonInterface(void)
{
    this->StopDriver();
    if (this->hEvtStartMonitoring)
        CloseHandle(this->hEvtStartMonitoring);
    if (this->hEvtStopMonitoring)
        CloseHandle(this->hEvtStopMonitoring);
    if (this->hEvtStopDriver)
        CloseHandle(this->hEvtStopDriver);
}

//-----------------------------------------------------------------------------
// Name: SetProcessStartCallBack
// Object: set process creation callback
//          WARNING : callback are called in different threads (there can be more than 1 thread executing your callback)
// Parameters :
//     in  : PROC_MON_CALLBACK StartProcCallBack : function pointer to the wanted callback
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CProcMonInterface::SetProcessStartCallBack(PROC_MON_CALLBACK StartProcCallBack)
{
    this->StartProcCallBack=StartProcCallBack;
}

//-----------------------------------------------------------------------------
// Name: SetProcessStopCallBack
// Object: set process end callback
//          WARNING : callback are called in different threads (there can be more than 1 thread executing your callback)
// Parameters :
//     in  : PROC_MON_CALLBACK StopProcCallBack : function pointer to the wanted callback
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CProcMonInterface::SetProcessStopCallBack(PROC_MON_CALLBACK StopProcCallBack)
{
    this->StopProcCallBack=StopProcCallBack;
}

//-----------------------------------------------------------------------------
// Name: StartDriver
// Object: Install and start procmon driver
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success, FALSE on error or if driver is already running
//-----------------------------------------------------------------------------
BOOL CProcMonInterface::StartDriver()
{
    BOOL bAlreadyLoaded=FALSE;
    BOOL bForceContinue=FALSE;
    BOOL bAlreadyInstalled=FALSE;
    DWORD dwCurrentState=0;
    CDriver Driver;

    TCHAR szDir[MAX_PATH];
    TCHAR* pos;

    // Get current application directory
    GetModuleFileName(GetModuleHandle(NULL),szDir,MAX_PATH);
    pos=_tcsrchr(szDir,'\\');
    if (pos)
        *(pos+1)=0;

    // put full path of ProcMon.sys in szDir
    _tcscat(szDir,PROC_MON_SYS);

    // install the driver querying if it is already installed
    if (!Driver.Install(szDir,SERVICE_DEMAND_START,&bAlreadyInstalled))
        return FALSE;
    
    // open the driver
    if (!Driver.Open(PROC_MON_NAME))
        return FALSE;

    // if driver is already installed, 2 things may append
    // - another soft is using it
    // - driver hasn't been uninstalled for unknown reason (software/computer crash, bad programming ...)
    if (bAlreadyInstalled)
    {
        // query the state of driver
        if (!Driver.GetState(&dwCurrentState))
            return FALSE;
        // another application is using it
        if ((dwCurrentState!=SERVICE_STOPPED)&&(dwCurrentState!=SERVICE_STOP_PENDING))
        {
#if (!defined(TOOLS_NO_MESSAGEBOX))
            if (MessageBox(NULL,MSG_DRIVER_ALREADY_RUNNING,_T("Warning"),MB_YESNO|MB_ICONWARNING|MB_TOPMOST)==IDYES)
                bForceContinue=TRUE;
            else
#endif
                return FALSE;
        }
    }

    // try to start the driver (still check if another app is using it)
    if (!Driver.Start(&bAlreadyLoaded))
        return FALSE;

    // if used by another application
    if (bAlreadyLoaded&&(!bForceContinue))
    {
#if (!defined(TOOLS_NO_MESSAGEBOX))
        if (MessageBox(NULL,MSG_DRIVER_ALREADY_RUNNING,_T("Warning"),MB_YESNO|MB_ICONWARNING|MB_TOPMOST)!=IDYES)
#endif
            return FALSE;
    }

    // reset stop events
    ResetEvent(this->hEvtStopDriver);
    ResetEvent(this->hEvtStopMonitoring);

    // create watching thread
    this->hWatchingThread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)CProcMonInterface::ThreadProc,this,0,NULL);
    if (!this->hWatchingThread)
        return FALSE;

    this->bStarted=TRUE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: StopDriver
// Object: stop and uninstall procmon driver
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CProcMonInterface::StopDriver()
{
    DWORD dwExitCode=0;

    if (!this->bStarted)
        return TRUE;

    // ask to stop the driver
    SetEvent(this->hEvtStopDriver);

    if (this->hWatchingThread)
    {
        GetExitCodeThread(this->hWatchingThread,&dwExitCode);
        if (dwExitCode==STILL_ACTIVE)
        {
            // wait for end of driver request treatment
            WaitForSingleObject(this->hWatchingThread,DRIVER_STOPPING_MAX_TIME);
        }
        CloseHandle(this->hWatchingThread);
        this->hWatchingThread=NULL;
    }


    // Make a block to avoid object creation when (this->bStarted==FALSE)
    // else if not sufficient rights and even driver is not started, an error message is reported
    {
        CDriver Driver;

        // open driver
        Driver.Open(PROC_MON_NAME);

        // stop it
        Driver.Stop();

        // uninstall it
        Driver.UnInstall();
    }

    this->bStarted=FALSE;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: StartMonitoring
// Object: start monitoring process creation and end
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CProcMonInterface::StartMonitoring()
{
    if (!this->bStarted)
        return FALSE;

    SetEvent(this->hEvtStartMonitoring);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: StopMonitoring
// Object: stop monitoring process creation and end
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CProcMonInterface::StopMonitoring()
{
    if (!this->bStarted)
        return FALSE;

    SetEvent(this->hEvtStopMonitoring);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ThreadProc
// Object: monitoring thread
// Parameters :
//     in  : CProcMonInterface* ProcMonInterface : object creating the thread
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
DWORD WINAPI CProcMonInterface::ThreadProc(CProcMonInterface* ProcMonInterface)
{
    BOOL        bRet;
    DWORD       dwRet;
    DWORD       dwLastError;
    HANDLE      hDriverFile;
    HANDLE      hThread;

    OVERLAPPED  ov={0};
    DWORD       dwBytesReturned;
    PROCESS_CALLBACK_INFO ProcessCallbackInfo;
    PPROC_MON_THREAD_CALLBACK_PARAM pThreadCallBackParam;
    HANDLE      pWaitStopHandles[3];
    HANDLE      pWaitStartHandles[2];

    BOOLEAN     bStopMonitoring;
    BOOLEAN     bStopDriver;

    // create a manual reset event
	ov.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL); 

    pWaitStopHandles[0]=ov.hEvent;
    pWaitStopHandles[1]=ProcMonInterface->hEvtStopDriver;
    pWaitStopHandles[2]=ProcMonInterface->hEvtStopMonitoring;

    pWaitStartHandles[0]=ProcMonInterface->hEvtStartMonitoring;
    pWaitStartHandles[1]=ProcMonInterface->hEvtStopDriver;


    // open driver
	hDriverFile = CreateFile(
		DRIVER_NAME,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		0,                     // Default security
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,  // Perform synchronous I/O
		0);                    // No template
 	if (hDriverFile==INVALID_HANDLE_VALUE)
    {
        CAPIError::ShowLastError();
        CloseHandle(ov.hEvent);
		return (DWORD)-1;
    }

    bStopDriver=FALSE;
    while(!bStopDriver)
    {
        bStopMonitoring=FALSE;

        // wait for StartMonitoring or StopDriver
        dwRet=WaitForMultipleObjects(2,pWaitStartHandles,FALSE,INFINITE);
        switch(dwRet)
        {
        case WAIT_OBJECT_0: // start monitoring event
	        // start monitoring
	        if(!DeviceIoControl(hDriverFile,IOCTL_START_MONITORING,NULL,0,NULL,0,&dwBytesReturned,NULL))
            {
                CAPIError::ShowLastError();
                bStopDriver=TRUE;
                bStopMonitoring=TRUE;
            }
            break;

        case WAIT_OBJECT_0+1: // StopDriver event
            bStopDriver=TRUE;
            bStopMonitoring=TRUE;
            break;

        default://error
            CAPIError::ShowLastError();
            bStopDriver=TRUE;
            bStopMonitoring=TRUE;
            break;
        }
        
        while(!bStopMonitoring)
        {

	        // Get the process info
	        bRet = DeviceIoControl(hDriverFile,IOCTL_GET_PROCINFO,0,0,
                                &ProcessCallbackInfo,sizeof(PROCESS_CALLBACK_INFO),&dwBytesReturned,&ov);
            if (!bRet)
            {
                dwLastError=GetLastError();
                if (dwLastError!=ERROR_IO_PENDING)
                {
                    CAPIError::ShowError(dwLastError);
                    // stop all
                    bStopDriver=TRUE;
                    bStopMonitoring=TRUE;
                    break;
                }
            }

            // wait IOCTL processing is completed, StopMonitoring or StopDriver query
            dwRet=WaitForMultipleObjects(3,pWaitStopHandles,FALSE,INFINITE);

            switch(dwRet)
            {
            case WAIT_OBJECT_0:// process creation or end
                if (!GetOverlappedResult(hDriverFile,&ov,&dwBytesReturned,TRUE))
                {
                    CAPIError::ShowLastError();
                    break;
                }
                // call callbacks in other threads

                // allocate memory
                pThreadCallBackParam=(PPROC_MON_THREAD_CALLBACK_PARAM)HeapAlloc(GetProcessHeap(), 0, sizeof(PROC_MON_THREAD_CALLBACK_PARAM));
                // if allocation is ok
                if (pThreadCallBackParam)
                {
                    // copy data
                    pThreadCallBackParam->pProcMonInterface=ProcMonInterface;
                    pThreadCallBackParam->ProcInfo=ProcessCallbackInfo;

                    // create thread
                    hThread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)CProcMonInterface::CallBacksThreadProc,pThreadCallBackParam,0,NULL);
                    if (hThread)
                    {
                        // try to speed thread to get callback sooner
                        SetThreadPriority(hThread,THREAD_PRIORITY_TIME_CRITICAL);
                        CloseHandle(hThread);
                    }
                }

                // reset ov event
                ResetEvent(ov.hEvent);
                break;

            case WAIT_OBJECT_0+1: // stop monitoring
                // query the cancel of current pending IOCTL_GET_PROCINFO
                // theoretically we don't need to call DeviceIoControl with IOCTL_CANCEL_IO as we are in the same thread
                // and can call CancelIo, but it seems to fail on multiple core processor (thx to Arno Garrels for reporting trouble) 
                // CancelIo(hDriverFile);
                DeviceIoControl(hDriverFile,IOCTL_CANCEL_IO,NULL,0,NULL,0,&dwBytesReturned,NULL);

                // stop monitoring
	            DeviceIoControl(hDriverFile,IOCTL_STOP_MONITORING,NULL,0,NULL,0,&dwBytesReturned,NULL);

                bStopMonitoring=TRUE;
                break;

            case WAIT_OBJECT_0+2: // stop
                bStopMonitoring=TRUE;
                bStopDriver=TRUE;
                break;

            default: // error -> stop
                bStopMonitoring=TRUE;
                bStopDriver=TRUE;
                CAPIError::ShowLastError();
                break;
            }
        }
    }

    // query the cancel of current pending IOCTL_GET_PROCINFO
    // theoretically we don't need to call DeviceIoControl with IOCTL_CANCEL_IO as we are in the same thread
    // and can call CancelIo, but it seems to fail on multiple core processor (thx to Arno Garrels for reporting trouble) 
    // CancelIo(hDriverFile);
    DeviceIoControl(hDriverFile,IOCTL_CANCEL_IO,NULL,0,NULL,0,&dwBytesReturned,NULL);

    // stop monitoring (for error case)
	DeviceIoControl(hDriverFile,IOCTL_STOP_MONITORING,NULL,0,NULL,0,&dwBytesReturned,NULL);

    // close driver
    CloseHandle(hDriverFile);

    CloseHandle(ov.hEvent);

    return 0;
}

//-----------------------------------------------------------------------------
// Name: CallBacksThreadProc
// Object: thread launched on each events
// Parameters :
//     in  : PPROC_MON_THREAD_CALLBACK_PARAM Param : call back parameters
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
DWORD WINAPI CProcMonInterface::CallBacksThreadProc(PPROC_MON_THREAD_CALLBACK_PARAM Param)
{
    PROC_MON_CALLBACK pfunc;
    // switch process creation flag
    if (Param->ProcInfo.bCreate)
        pfunc=Param->pProcMonInterface->StartProcCallBack;
    else
        pfunc=Param->pProcMonInterface->StopProcCallBack;

    // check call back pointer
    if (!IsBadCodePtr((FARPROC)pfunc))
        // call the callback
        pfunc(Param->ProcInfo.hParentId,Param->ProcInfo.hProcessId);

    // free allocated memory
    HeapFree(GetProcessHeap(),0,Param);

    return 0;
}