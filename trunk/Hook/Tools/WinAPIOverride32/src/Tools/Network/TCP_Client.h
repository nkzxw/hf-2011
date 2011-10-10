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

#pragma once

#pragma warning (push)
#pragma warning(disable : 4005)// to avoid  '_WINSOCKAPI_' macro redefinition warning
#include <winsock2.h>// must be include before windows.h or _WINSOCKAPI_ must be set as preprocessor to project
#pragma warning (pop)
#pragma comment (lib,"Ws2_32")

#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h> // requierd for atlrx
#pragma warning (pop)

#pragma warning (push)
#pragma warning(disable : 4018)
#pragma warning(disable : 4389 )
#include <atlrx.h>// for regular expression
#pragma warning (pop)

#include <windows.h>

#define PACKET_SIZE  1024

class CTCP_Client
{
public:
    typedef void (*tagErrorMessageCallBack)(TCHAR* ErrorMessage,LPVOID UserParam);

protected:
    SOCKET  Socket;
    BOOL    bWSAStarted;
    WSAEVENT hevtCancelReceive;

    LPVOID  ErrorMessageCallBackUserParam;
    tagErrorMessageCallBack ErrorMessageCallBack;

    void ReportLastWSAError();
    void ReportWSAError(int ErrorValue);
    void ReportError(TCHAR* ErrorMessage);
    BOOL Receive(BYTE* pData,SIZE_T MaxDataSize,SIZE_T* pDataSize,SIZE_T TimeOut,BOOL bResetCancelEvent);
    BOOL GetIp(char* Name,DWORD* pIP);
public:
    BOOL bShowMessageBoxForErrorMessages;

    CTCP_Client();
    ~CTCP_Client();

    BOOL Connect(char* IpOrName,unsigned short Port);
    void Close();
    BOOL Send(CHAR* String);
    BOOL Send(BYTE* pData,SIZE_T DataSize);
    BOOL Receive(BYTE* pData,SIZE_T MaxDataSize,SIZE_T* pDataSize);
    BOOL Receive(BYTE* pData,SIZE_T MaxDataSize,SIZE_T* pDataSize,SIZE_T TimeOut);
    BOOL WaitForSize(BYTE* pData,SIZE_T RequieredSize,SIZE_T* pDataSize,SIZE_T TimeOut);
    BOOL WaitForString(char* WaitedString,BYTE* pData,SIZE_T MaxDataSize,SIZE_T* pDataSize,SIZE_T TimeOut);
    BOOL WaitForRegularExpression(char* WaitedRegExp,BYTE* pData,SIZE_T MaxDataSize,SIZE_T* pDataSize,SIZE_T TimeOut);
    void CancelReceive();// cancel current Receive or WaitForXX operation

    void SetErrorMessageCallBack(tagErrorMessageCallBack CallBack,LPVOID UserParam);
};


