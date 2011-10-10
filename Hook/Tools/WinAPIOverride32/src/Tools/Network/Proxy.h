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
// Object: manages proxy tcp connections
//-----------------------------------------------------------------------------

#pragma once

#include "TCP_Client.h"

#include <Winsock2.h>// must be include before windows.h
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
#include <stdio.h>

#define CPROXY_PACKET_SIZE  1024
#define CPROXY_RECEIVE_TIMEOUT             60 // time in seconds

class CProxy:public CTCP_Client
{
public:
    enum ProxyType
    {
        PROXY_TYPE_HTTP_CONNECT,
        PROXY_TYPE_HTTP_GET,
        PROXY_TYPE_SOCKS4,
        PROXY_TYPE_SOCKS5
    };

protected:

#pragma pack (push)
#pragma pack (1)
    // keep field order to send struct on network
    typedef struct tagSock4Header
    {
        BYTE Version;
        BYTE Command;
        unsigned short Port;// in network order
        unsigned long Ip;// in network order
        BYTE End;
    }SOCKS4_HEADER,*PSOCKS4_HEADER;

#pragma pack (pop)

    BOOL ConnectThroughProxyGet(char* ProxyIP,unsigned short ProxyPort);
    BOOL ConnectThroughProxyConnect(char* ProxyIP,unsigned short ProxyPort,char* TargetIP,unsigned short TargetPort);
    BOOL ConnectThroughProxySocks4(char* ProxyIP,unsigned short ProxyPort,char* TargetIP,unsigned short TargetPort);
    BOOL ConnectThroughProxySocks5(char* ProxyIP,unsigned short ProxyPort,char* TargetIP,unsigned short TargetPort);
public:
    CProxy();
    ~CProxy();

    BOOL ConnectThroughProxy(char* ProxyIP,unsigned short ProxyPort,CProxy::ProxyType Type,
                             char* TargetIP,unsigned short TargetPort);
};


