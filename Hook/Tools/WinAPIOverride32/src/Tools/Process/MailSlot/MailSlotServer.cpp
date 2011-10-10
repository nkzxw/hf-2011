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
// Object: class helper for using a Mailslot server
//-----------------------------------------------------------------------------

#include "mailslotserver.h"

//-----------------------------------------------------------------------------
// Name: CMailSlotServer
// Object: Constructor
// Parameters :
//     in  : - TCHAR* pszMailSlotName : slot name
//                  \\.\mailslot\name Retrieves a client handle to a local mailslot. 
//                  \\computername\mailslot\name Retrieves a client handle to a remote mailslot. 
//                  \\domainname\mailslot\name Retrieves a client handle to all mailslots with the specified name in the specified domain. 
//                  \\*\mailslot\name Retrieves a client handle to all mailslots with the specified name in the system's primary domain. 
//           - DWORD dwMaxMessageSize : max message size
//           - pfnMailSlotCallback CallbackFunc : pointer to a callback function taking in parameter 
//                                 PVOID pData : pointer to receive data
//                                 DWORD dwDataSize : size of pData in bytes
//                                 PVOID pUserData : user data = what you want
//           - PVOID pUserData : user data given back at each callback can be used to store pointer of object
//                               as call back is not object oriented
//     out : 
//     return : 
//-----------------------------------------------------------------------------
CMailSlotServer::CMailSlotServer(TCHAR* pszMailSlotName,pfnMailSlotCallback CallbackFunc,PVOID pUserData)
{

    this->pMailSlotData=NULL;
    this->hOverlappedEvent=NULL;
    _tcsncpy(this->pszMailSlotName,pszMailSlotName,MAX_PATH);
    this->pszMailSlotName[MAX_PATH-1]=0;
    
    this->CallbackFunc=CallbackFunc;
    this->pUserData=pUserData;

    this->bStarted=FALSE;
    this->hMailslot=NULL;
    this->hThreadHandle=NULL;
    

    // create events
    this->hevtStopEvent=CreateEvent(NULL,TRUE,FALSE,NULL);
    this->hevtDataArrival=CreateEvent(NULL,FALSE,FALSE,NULL);
}

CMailSlotServer::~CMailSlotServer(void)
{
    // make sure thread is stopped
    this->Stop();
    if (this->hevtStopEvent)
    {
        CloseHandle(this->hevtStopEvent);
        this->hevtStopEvent=NULL;
    }

    // on application unload, thread are not always closed gracefully,
    // so free memory if not done
    if (this->pMailSlotData)
    {
        delete this->pMailSlotData;
        this->pMailSlotData=NULL;
    }
    if (this->hOverlappedEvent)
    {
        CloseHandle(this->hOverlappedEvent);
        this->hOverlappedEvent=NULL;
    }
    if (this->hevtDataArrival)
    {
        CloseHandle(this->hevtDataArrival);
        this->hevtDataArrival=NULL;
    }
}

//-----------------------------------------------------------------------------
// Name: GetServerThreadId
// Object: Get mailslot server thread ID
// Parameters :
//     in  :
//     out : DWORD* ServerThreadID : thread id of mailslot server
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CMailSlotServer::GetServerThreadId(OUT DWORD* pServerThreadID)
{
    if (IsBadWritePtr(pServerThreadID,sizeof(DWORD)))
        return FALSE;

    if (!this->bStarted)
    {
        *pServerThreadID=0;
        return FALSE;
    }

    *pServerThreadID=this->ServerThreadID;
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: Start
// Object: Start mailslot server to use it
// Parameters :
//     in  :
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CMailSlotServer::Start(void)
{
    return this->Start(FALSE);
}

//-----------------------------------------------------------------------------
// Name: Start
// Object: Start mailslot server to use it
// Parameters :
//     in  : BOOL AllowAccessToAllUsers : TRUE to allow all account to be available
//                                         to send messages to mailslot server
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CMailSlotServer::Start(BOOL AllowAccessToAllUsers)
{
    if (this->bStarted)
        this->Stop();

    // assume we have a callback
    if (IsBadCodePtr((FARPROC)this->CallbackFunc))
        return FALSE;

    // create mailslot

    // if we allow access to all user
    if (AllowAccessToAllUsers)
    {
        SECURITY_DESCRIPTOR sd={0};
        sd.Revision=SECURITY_DESCRIPTOR_REVISION;
        sd.Control=SE_DACL_PRESENT;
        sd.Dacl=NULL; // assume everyone access
        SECURITY_ATTRIBUTES SecAttr={0};
        SecAttr.bInheritHandle=FALSE;
        SecAttr.nLength=sizeof(SECURITY_ATTRIBUTES);
        SecAttr.lpSecurityDescriptor=&sd;
        this->hMailslot=CreateMailslot(this->pszMailSlotName,0,MAILSLOT_WAIT_FOREVER,&SecAttr);
    }
    else // use default security attributes
        this->hMailslot=CreateMailslot(this->pszMailSlotName,0,MAILSLOT_WAIT_FOREVER,0);

    if (this->hMailslot == INVALID_HANDLE_VALUE)
        return FALSE;

    // reset events
    ResetEvent(this->hevtStopEvent);

    // create a listening thread
    this->hThreadHandle=CreateThread(NULL,0,CMailSlotServer::ThreadListener,this,0,&this->ServerThreadID);
    if (!this->hThreadHandle)
        return FALSE;

    // set bStarted flag
    this->bStarted=TRUE;

    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: Stop
// Object: Stop mailslot server
// Parameters :
//     in  :
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CMailSlotServer::Stop(void)
{
    if (!this->bStarted)
        return TRUE;
    // send stop event
    SetEvent(this->hevtStopEvent);
    this->CallbackFunc=NULL;
    // wait for end of thread
    if (WaitForSingleObject(this->hThreadHandle,CMAILSLOTSERVER_CLOSINGSERVER_MAX_WAIT)==WAIT_TIMEOUT)
    {
        ::TerminateThread(this->hThreadHandle,0);
        if (this->hOverlappedEvent)
        {
            CloseHandle(this->hOverlappedEvent);
            this->hOverlappedEvent=NULL;
        }
        if (this->pMailSlotData)
        {
            delete this->pMailSlotData;
            this->pMailSlotData=NULL;
        }
    }

    //destroy mailslot
    if (this->hMailslot)
    {
        CloseHandle(this->hMailslot);
        this->hMailslot=NULL;
    }
    // close thread handle
    if (this->hThreadHandle)
    {
        CloseHandle(this->hThreadHandle);
        this->hThreadHandle=NULL;
    }
    // set bStarted flag
    this->bStarted=FALSE;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CMailSlotServer::ThreadListener
// Object: Get messages and call user call back. Stop on event hevtStopEvent
// Parameters :
//     in  : LPVOID lpParam : pointer to a CMailSlotServer object
//     out :
//     return : 0 on success, 0xFFFFFFFF on error
//-----------------------------------------------------------------------------
DWORD WINAPI CMailSlotServer::ThreadListener(LPVOID lpParam)
{
    DWORD dwResult;
    DWORD dwError;
    DWORD dwBytesRead=0;
    DWORD dwMsgSize=0;
    DWORD dwMessageCount;
    DWORD ReturnValue = 0;

    CMailSlotServer* pCMSS=(CMailSlotServer*)lpParam;
    DWORD dwMaxMessageSize=512;
    pCMSS->pMailSlotData=new BYTE[dwMaxMessageSize];
    OVERLAPPED Overlapped;
    memset(&Overlapped,0,sizeof(OVERLAPPED));
    pCMSS->hOverlappedEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
    Overlapped.hEvent=pCMSS->hOverlappedEvent;
    HANDLE ph[2]={pCMSS->hevtStopEvent,Overlapped.hEvent};
    for (;;)
    {
        // see CMailSlotClient : in the same write operation we write data size on first DWORD
        // and next data
        if (!ReadFile(pCMSS->hMailslot, pCMSS->pMailSlotData, dwMsgSize, &dwBytesRead, &Overlapped))
        {
            dwError=GetLastError();
            if (dwError==ERROR_INSUFFICIENT_BUFFER)
            {
                if(GetMailslotInfo(pCMSS->hMailslot, 0, &dwMsgSize, &dwMessageCount, NULL))
                {
                    // adjust our local buffer size
                    if (dwMaxMessageSize<dwMsgSize)
                    {
                        delete pCMSS->pMailSlotData;
                        pCMSS->pMailSlotData=new BYTE[dwMsgSize];
                        dwMaxMessageSize=dwMsgSize;
                    }
                    // restart the read operation with correct buffer size
                    continue;
                }
                else
                {
                    CAPIError::ShowError(dwError);
                    ReturnValue = dwError;
                    goto CleanUp;
                }
            }

            if (dwError!=ERROR_IO_PENDING)
            {
                CAPIError::ShowError(dwError);
                ReturnValue = dwError;
                goto CleanUp;
            }
        }
        // wait for overlapped event or stop event
        dwResult = WaitForMultipleObjects(2,ph,FALSE,INFINITE);
        switch (dwResult)
        {
        case WAIT_OBJECT_0:// stop event
            ReturnValue = 0;
            goto CleanUp;
        case WAIT_OBJECT_0+1:// read event
            if (dwMsgSize==0)
                break;
            SetEvent(pCMSS->hevtDataArrival);
            if (!IsBadCodePtr((FARPROC)pCMSS->CallbackFunc))
                pCMSS->CallbackFunc(pCMSS->pMailSlotData,dwBytesRead,pCMSS->pUserData);
            // gives a new fake buffer size to allow to catch ERROR_INSUFFICIENT_BUFFER error
            dwMsgSize=0;
            break;
        case WAIT_FAILED:
            ReturnValue = WAIT_FAILED;
            goto CleanUp;
        }
    }
CleanUp:
    HANDLE TmpHandle = pCMSS->hOverlappedEvent;
    pCMSS->hOverlappedEvent=NULL;
    CloseHandle(TmpHandle);

    PBYTE Tmp = pCMSS->pMailSlotData;
    pCMSS->pMailSlotData=NULL;
    delete Tmp;
    

    return ReturnValue;

}

//-----------------------------------------------------------------------------
// Name: WaitUntilNoMessageDuringSpecifiedTime
// Object: allow to wait the specified time without data reception, before destroying mailslot server
// Parameters :
//     in  : DWORD TimeInMs : time in ms during which no message should arrive
//           HANDLE hCancelEvent : cancel event to cancel waiting thread
//     out :
//     return : TRUE on success or server destruction or hCancelEvent
//              FALSE in case of failure
//-----------------------------------------------------------------------------
BOOL CMailSlotServer::WaitUntilNoMessageDuringSpecifiedTime(DWORD TimeInMs,HANDLE hCancelEvent)
{
    if (!this->bStarted)
        return FALSE;

    DWORD dwResult;
    DWORD NbEvents;
    HANDLE ph[3];
    if (hCancelEvent)
    {
        NbEvents=3;
        ph[0]=this->hevtStopEvent;
        ph[1]=this->hevtDataArrival;
        ph[2]=hCancelEvent;
    }
    else
    {
        NbEvents=2;
        ph[0]=this->hevtStopEvent;
        ph[1]=this->hevtDataArrival;
    }
    for(;;)
    {
        // wait for stop, cancel and data arrival event during specified timeout
        dwResult = WaitForMultipleObjects(NbEvents,ph,FALSE,TimeInMs);
        switch (dwResult)
        {
        case WAIT_OBJECT_0: // stop
            return TRUE;
        case WAIT_OBJECT_0+1: // data arrival --> continue infinite wait
            break;
        case WAIT_OBJECT_0+2: // cancel
            return TRUE;
        case WAIT_FAILED:
            return FALSE;
        case WAIT_TIMEOUT:
            return TRUE;
        }
    }
    return TRUE;
}