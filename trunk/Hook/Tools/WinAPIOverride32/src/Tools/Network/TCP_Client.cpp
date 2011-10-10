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
// Object: manages tcp client connections
//-----------------------------------------------------------------------------

#pragma message (__FILE__ " Information : Include TCP_Client.h before including windows.h, or use _WINSOCKAPI_ preprocessor define\r\n")

#include "TCP_Client.h"


//-----------------------------------------------------------------------------
// Name: CTCP_Client
// Object: constructor
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
CTCP_Client::CTCP_Client(void)
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    this->Socket=NULL;
    this->bWSAStarted=FALSE;
    this->bShowMessageBoxForErrorMessages=TRUE;
    this->ErrorMessageCallBackUserParam=NULL;
    this->ErrorMessageCallBack=NULL;
    this->hevtCancelReceive=WSACreateEvent();
     
    wVersionRequested = MAKEWORD( 2, 2 );
     
    err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 )
    {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        this->ReportLastWSAError();
        return;
    }
     
    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions greater    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */
     
    if ( LOBYTE( wsaData.wVersion ) != 2 ||
            HIBYTE( wsaData.wVersion ) != 2 )
    {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        this->ReportLastWSAError();
        WSACleanup( );
        return; 
    }
     
    /* The WinSock DLL is acceptable. Proceed. */
    this->bWSAStarted=TRUE;
}

//-----------------------------------------------------------------------------
// Name: CTCP_Client
// Object: destructor
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
CTCP_Client::~CTCP_Client(void)
{

    if (this->bWSAStarted)
    {
        this->Close();
        WSACleanup();
    }

    WSACloseEvent(this->hevtCancelReceive);
}

//-----------------------------------------------------------------------------
// Name: ReportLastWSAError
// Object: call this func to report the last WSA error
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CTCP_Client::ReportLastWSAError()
{
    this->ReportWSAError(WSAGetLastError());
}
//-----------------------------------------------------------------------------
// Name: ReportWSAError
// Object: call this func to report WSA error specified by ErrorValue
// Parameters :
//     in  : int ErrorValue : WSA error value
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CTCP_Client::ReportWSAError(int ErrorValue)
{
    // if no error message callback defined and no message box wanted
    if ((!bShowMessageBoxForErrorMessages)
        && (!this->ErrorMessageCallBack))
        // return
        return;

    // format error message
    TCHAR pszMsg[MAX_PATH];

    DWORD dw=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    ErrorValue,
                    GetUserDefaultLangID(),//GetSystemDefaultLangID()
                    pszMsg,
                    MAX_PATH-1,
                    NULL);
    pszMsg[MAX_PATH-1]=0;
    //If the function succeeds, the return value is the number of TCHARs stored in the output buffer,
    //  excluding the terminating null character.
    //If the function fails, the return value is zero
    if(dw==0)
    {
        // FormatMessage failed
        _stprintf(pszMsg,_T("Error 0x%08X\r\n"),ErrorValue);
    }
    this->ReportError(pszMsg);
}

//-----------------------------------------------------------------------------
// Name: ReportError
// Object: call this func to report an error
// Parameters :
//     in  : TCHAR* ErrorMessage : error message to report
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CTCP_Client::ReportError(TCHAR* ErrorMessage)
{
    // if callback defined
    if (this->ErrorMessageCallBack)
        // call it
        this->ErrorMessageCallBack(ErrorMessage,this->ErrorMessageCallBackUserParam);

    // if error message boxes not wanted
    if (!bShowMessageBoxForErrorMessages)
        return;
    MessageBox(NULL,ErrorMessage,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
}

//-----------------------------------------------------------------------------
// Name: SetErrorMessageCallBack
// Object: Set the error message callback
// Parameters :
//     in  : tagErrorMessageCallBack CallBack : callback
//           LPVOID UserParam : user parameter provide in callback args
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CTCP_Client::SetErrorMessageCallBack(tagErrorMessageCallBack CallBack,LPVOID UserParam)
{
    // fill members
    this->ErrorMessageCallBack=CallBack;
    this->ErrorMessageCallBackUserParam=UserParam;
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: close the tcp connection
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CTCP_Client::Close()
{
    if (!this->bWSAStarted)
        return;
    if (!this->Socket)
        return;

    // Cancel receive if any
    this->CancelReceive();

    // disables both sends and receives (send FIN)
    shutdown(this->Socket,SD_BOTH);
    // close socket
    closesocket(this->Socket);

    this->Socket=NULL;
}

//-----------------------------------------------------------------------------
// Name: GetIp
// Object: convert a string ip or hostname to DWORD
// Parameters :
//     in  : char* Name : ip or hostname ("12.0.0.1" or "google.com")
//     out : DWORD* pIP : IP value
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CTCP_Client::GetIp(char* Name,DWORD* pIP)
{
    hostent*    ServerHostEnt;
    // try to convert if server_name is like a.b.c.d
    *pIP = inet_addr(Name);
    if (*pIP != INADDR_NONE)
        return TRUE;
    else // a name was given
    {
        // retrieve it's IP
        ServerHostEnt = gethostbyname(Name);
        if (ServerHostEnt == NULL)
        {
            this->ReportLastWSAError();
            return FALSE;
        }
        memcpy(pIP,ServerHostEnt->h_addr,ServerHostEnt->h_length);// ok for ipv4 only
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Connect
// Object: connect to specified host
// Parameters :
//     in  : char* IpOrName : ip or hostname ("12.0.0.1" or "google.com")
//           unsigned short Port : port
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CTCP_Client::Connect(char* IpOrName,unsigned short Port)
{
    if (!this->bWSAStarted)
        return FALSE;

    // close connection if not previously done
    if (this->Socket)
        this->Close();

    sockaddr_in ServerSockAddr={0};  
    DWORD       hostAddr;                 

    if (!this->GetIp(IpOrName,&hostAddr))
        return FALSE;
    memcpy(&ServerSockAddr.sin_addr,&hostAddr,sizeof(hostAddr));

    // fill port
    ServerSockAddr.sin_port = htons(Port);
    // Internet Family address
    ServerSockAddr.sin_family = AF_INET;

    // create an overlapped socket
    this->Socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if (this->Socket==INVALID_SOCKET)

    {
        this->Socket=NULL;
        this->ReportLastWSAError();
        return FALSE;
    }

    // connect to the remote host
    if(connect(this->Socket,(struct sockaddr *)&ServerSockAddr,sizeof(sockaddr_in))== SOCKET_ERROR)
    {
        this->ReportLastWSAError();
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Send
// Object: send data to remote host
// Parameters :
//     in  : BYTE* pData : data to send
//           DWORD DataSize : size of pData in bytes
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CTCP_Client::Send(CHAR* String)
{
    return this->Send((BYTE*)String,strlen(String));
}

//-----------------------------------------------------------------------------
// Name: Send
// Object: send data to remote host
// Parameters :
//     in  : BYTE* pData : data to send
//           DWORD DataSize : size of pData in bytes
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CTCP_Client::Send(BYTE* pData,SIZE_T DataSize)
{
    int RemainingSizeToSend;
    int SentDataSize;

    RemainingSizeToSend=DataSize;

    // while data not fully sent
    while ( RemainingSizeToSend > 0 )
    {
        SentDataSize = send(this->Socket, (const char*)pData, RemainingSizeToSend,0);
        if (SentDataSize == SOCKET_ERROR)// if error
        {
            this->ReportLastWSAError();
            return FALSE;
        }

        // reduce RemainingSizeToSend
        RemainingSizeToSend-= SentDataSize;

        // point to next byte to send
        pData+= SentDataSize;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CancelReceive
// Object: cancel current Receive or WaitForXX operation
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CTCP_Client::CancelReceive()
{
    WSASetEvent(this->hevtCancelReceive);
}

//-----------------------------------------------------------------------------
// Name: Receive
// Object: wait for data
// Parameters :
//     in  : DWORD MaxDataSize : max size of pData
//     out : BYTE* pData : buffer containing recieved data
//           DWORD* pDataSize : recieved datasize
//     return : 
//-----------------------------------------------------------------------------
BOOL CTCP_Client::Receive(BYTE* pData,SIZE_T MaxDataSize,SIZE_T* pDataSize)
{
    return this->Receive(pData,MaxDataSize,pDataSize,WSA_INFINITE);
}
//-----------------------------------------------------------------------------
// Name: Receive
// Object: wait for data during specified timeout
// Parameters :
//     in  : DWORD MaxDataSize : max size of pData
//           DWORD TimeOut : timeout in sec
//     out : BYTE* pData : buffer containing recieved data
//           DWORD* pDataSize : recieved datasize
//     return : 
//-----------------------------------------------------------------------------
BOOL CTCP_Client::Receive(BYTE* pData,SIZE_T MaxDataSize,SIZE_T* pDataSize,SIZE_T TimeOut)
{
    return this->Receive(pData,MaxDataSize,pDataSize,TimeOut,TRUE);
}

//-----------------------------------------------------------------------------
// Name: Receive
// Object: wait for data during specified timeout
// Parameters :
//     in  : DWORD MaxDataSize : max size of pData
//           DWORD TimeOut : timeout in sec
//           BOOL bResetCancelEvent : TRUE if CancelEvent must be reset, FALSE else
//     out : BYTE* pData : buffer containing recieved data
//           DWORD* pDataSize : recieved datasize
//     return : 
//-----------------------------------------------------------------------------
BOOL CTCP_Client::Receive(BYTE* pData,SIZE_T MaxDataSize,SIZE_T* pDataSize,SIZE_T TimeOut,BOOL bResetCancelEvent)
{
    int Ret;
    DWORD dwWaitRes;
    WSABUF WsaBuf;
    WSAEVENT RcvEvent;
    WSAEVENT ArrayEvents[2];
    DWORD Flags=0;
    WSAOVERLAPPED OverLapped={0};

    // reset the cancel event
    if (bResetCancelEvent)
        WSAResetEvent(this->hevtCancelReceive);

    *pDataSize=0;

    WsaBuf.buf=(char*)pData;
    WsaBuf.len=MaxDataSize;

    // convert timeout from seconds to ms
    if (TimeOut!=WSA_INFINITE)
        TimeOut=TimeOut*1000;

    // create overlapped event
    RcvEvent=WSACreateEvent();
    OverLapped.hEvent=RcvEvent;

    // check overlapped event
    if (RcvEvent==WSA_INVALID_EVENT)
    {
        this->ReportLastWSAError();
        return FALSE;
    }

    // make an overlapped receive
    if (WSARecv(this->Socket,&WsaBuf,1,pDataSize,&Flags,&OverLapped,NULL)==SOCKET_ERROR)
    {
        Ret=WSAGetLastError();
        // if IO is not pending
        if (Ret!=WSA_IO_PENDING)
        {
            this->ReportWSAError(Ret);
            WSACloseEvent(RcvEvent);
            return FALSE;
        }
    }

    ArrayEvents[0]=RcvEvent;
    ArrayEvents[1]=this->hevtCancelReceive;
    // wait for data arrival
    dwWaitRes=WSAWaitForMultipleEvents(2,ArrayEvents,FALSE,TimeOut,TRUE);
    if (dwWaitRes!=WSA_WAIT_EVENT_0)
    {
        // we have to cancel the blocking receive
        // as we are on the same thread we can use cancel io
        CancelIo((HANDLE)this->Socket);

        if (dwWaitRes==WSA_WAIT_FAILED)
            this->ReportLastWSAError();

        if (dwWaitRes==WSA_WAIT_TIMEOUT)
            this->ReportError(_T("Timeout"));

        // close created receive event
        WSACloseEvent(RcvEvent);
        return FALSE;
    }

    // retrieve data size
    WSAGetOverlappedResult(this->Socket,&OverLapped,pDataSize,FALSE,&Flags);

    // close created receive event
    WSACloseEvent(RcvEvent);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: WaitForSize
// Object: wait until required size is reached or specified timeout occurs
//         !! size of pData must be greater or equals to RequieredSize !!
// Parameters :
//     in  : DWORD RequieredSize : wanted size in bytes
//           DWORD TimeOut : timeout in sec
//     out : BYTE* pData : buffer containing recieved data (length must be >=RequieredSize)
//           DWORD* pDataSize : recieved datasize
//     return : 
//-----------------------------------------------------------------------------
BOOL CTCP_Client::WaitForSize(BYTE* pData,SIZE_T RequieredSize,SIZE_T* pDataSize,SIZE_T TimeOut)
{
    DWORD InitialTickCount;
    DWORD CurrentTickCount;
    DWORD RemainingSize;
    DWORD RemainingTime;
    DWORD ReceivedSize;

    // init received size to 0
    *pDataSize=0;
    // init tick count
    InitialTickCount=GetTickCount();
    CurrentTickCount=InitialTickCount;
    // init remaining size of pData
    RemainingSize=RequieredSize;

    // reset the cancel event
    WSAResetEvent(this->hevtCancelReceive);

    // while buffer length is not reached
    //       and TimeOut as not ended
    while( (*pDataSize<RequieredSize)
            && ((CurrentTickCount-InitialTickCount)<TimeOut*1000)
         )
    {
        // compute remaining time in ms
        RemainingTime=TimeOut*1000-(CurrentTickCount-InitialTickCount);

        // receive
        if (!this->Receive(pData,RemainingSize,&ReceivedSize,RemainingTime/1000,FALSE))// /1000 because time out is requiered in sec
            return FALSE;

        // update pointer position
        pData+=ReceivedSize;
        // update new maximum size
        RemainingSize-=ReceivedSize;
        // update full received size
        *pDataSize+=ReceivedSize;
        // update CurrentTickCount
        CurrentTickCount=GetTickCount();
    }

    // check while break
    if (*pDataSize==RequieredSize)
        return TRUE;

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: WaitForString
// Object: wait until required string is received or specified timeout occurs
// Parameters :
//     in  : char* WaitedString : string to wait for
//           DWORD MaxDataSize : pData max size in bytes
//           DWORD TimeOut : timeout
//     out : BYTE* pData : buffer containing recieved data 
//           DWORD* pDataSize : recieved datasize
//     return : 
//-----------------------------------------------------------------------------
BOOL CTCP_Client::WaitForString(char* WaitedString,BYTE* pData,SIZE_T MaxDataSize,SIZE_T* pDataSize,SIZE_T TimeOut)
{
    SIZE_T InitialTickCount;
    SIZE_T CurrentTickCount;
    SIZE_T RemainingSize;
    SIZE_T RemainingTime;
    SIZE_T ReceivedSize;
    char* BeginOfData;

    // init received size to 0
    *pDataSize=0;
    // set content of pData to 0 as we use strstr on it
    memset(pData,0,MaxDataSize);
    // init tick count
    InitialTickCount=GetTickCount();
    CurrentTickCount=InitialTickCount;
    // init remaining size of pData
    RemainingSize=MaxDataSize;

    // reset the cancel event
    WSAResetEvent(this->hevtCancelReceive);

    BeginOfData=(char*)pData;

    // while string is not found 
    //       and buffer is enough
    //       and TimeOut as not ended
    while( (!strstr(BeginOfData,WaitedString))
            && (*pDataSize<MaxDataSize)
            && ((CurrentTickCount-InitialTickCount)<TimeOut*1000)
         )
    {
        // compute remaining time in ms
        RemainingTime=TimeOut*1000-(CurrentTickCount-InitialTickCount);

        // receive
        if (!this->Receive(pData,RemainingSize,&ReceivedSize,RemainingTime/1000,FALSE))// /1000 because time out is required in sec
            return FALSE;

        // update pointer position
        pData+=ReceivedSize;
        // update new maximum size
        RemainingSize-=ReceivedSize;
        // update full received size
        *pDataSize+=ReceivedSize;
        // update CurrentTickCount
        CurrentTickCount=GetTickCount();
    }

    // check while break
    if (strstr(BeginOfData,WaitedString))
        return TRUE;

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: WaitForRegularExpression
// Object: wait until required regular expression match or specified timeout occurs
// Parameters :
//     in  : char* WaitedRegExp : reg exp to wait for
//           DWORD MaxDataSize : pData max size in bytes
//           DWORD TimeOut : timeout
//     out : BYTE* pData : buffer containing recieved data 
//           DWORD* pDataSize : recieved datasize
//     return : 
//-----------------------------------------------------------------------------
BOOL CTCP_Client::WaitForRegularExpression(char* WaitedRegExp,BYTE* pData,SIZE_T MaxDataSize,SIZE_T* pDataSize,SIZE_T TimeOut)
{
    SIZE_T InitialTickCount;
    SIZE_T CurrentTickCount;
    SIZE_T RemainingSize;
    SIZE_T RemainingTime;
    SIZE_T ReceivedSize;
    char* BeginOfData;
    CAtlRegExp<CAtlRECharTraitsA> RegExpr;
    CAtlREMatchContext<CAtlRECharTraitsA> MatchContext;

    // init received size to 0
    *pDataSize=0;
    // set content of pData to 0 as we use RegExpr on it
    memset(pData,0,MaxDataSize);
    // init tick count
    InitialTickCount=GetTickCount();
    CurrentTickCount=InitialTickCount;
    // init remaining size of pData
    RemainingSize=MaxDataSize;

    if (RegExpr.Parse(WaitedRegExp)!=REPARSE_ERROR_OK)
        return FALSE;

    // reset the cancel event
    WSAResetEvent(this->hevtCancelReceive);

    BeginOfData=(char*)pData;

    // while RegExpr is not found 
    //       and buffer is enough
    //       and TimeOut as not ended
    while( (!RegExpr.Match(BeginOfData,&MatchContext))
        && (*pDataSize<MaxDataSize)
        && ((CurrentTickCount-InitialTickCount)<TimeOut*1000)
        )
    {
        // compute remaining time in ms
        RemainingTime=TimeOut*1000-(CurrentTickCount-InitialTickCount);

        // receive
        if (!this->Receive(pData,RemainingSize,&ReceivedSize,RemainingTime/1000,FALSE))// /1000 because time out is requiered in sec
            return FALSE;

        // update pointer position
        pData+=ReceivedSize;
        // update new maximum size
        RemainingSize-=ReceivedSize;
        // update full received size
        *pDataSize+=ReceivedSize;
        // update CurrentTickCount
        CurrentTickCount=GetTickCount();
    }

    // check while break
    if (RegExpr.Match(BeginOfData,&MatchContext))
        return TRUE;

    return FALSE;
}