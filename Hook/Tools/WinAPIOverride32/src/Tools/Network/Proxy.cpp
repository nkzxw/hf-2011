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

#pragma message (__FILE__ " Information : Include Proxy.h before including windows.h, or use _WINSOCKAPI_ preprocessor define")

#include "Proxy.h"

CProxy::CProxy()
{

}

CProxy::~CProxy(void)
{

}

//-----------------------------------------------------------------------------
// Name: ConnectThroughProxy
// Object: connect to TargetIP:TargetPort through proxy ProxyIP:ProxyPort
//           after ConnectThroughProxy call (if return is successful)
//           you can use classical send /receive request of the TCP_Client object
// Parameters :
//     in  : char* ProxyIP : ip of proxy
//           unsigned short ProxyPort : port of proxy
//           CProxy::ProxyType Type : proxy type
//           char* TargetIP : ip of client to connect
//           unsigned short TargetPort : port of client to connect
//     out :
//     return : TRUE on success, FALSE else
//-----------------------------------------------------------------------------
BOOL CProxy::ConnectThroughProxy(char* ProxyIP,unsigned short ProxyPort,CProxy::ProxyType Type,
                                 char* TargetIP,unsigned short TargetPort)
{
    switch (Type)
    {
    case CProxy::PROXY_TYPE_HTTP_GET:
        return this->ConnectThroughProxyGet(ProxyIP,ProxyPort);
    case CProxy::PROXY_TYPE_HTTP_CONNECT:
        return this->ConnectThroughProxyConnect(ProxyIP,ProxyPort,TargetIP,TargetPort);
    case CProxy::PROXY_TYPE_SOCKS4:
        return this->ConnectThroughProxySocks4(ProxyIP,ProxyPort,TargetIP,TargetPort);
    case CProxy::PROXY_TYPE_SOCKS5:
        return this->ConnectThroughProxySocks5(ProxyIP,ProxyPort,TargetIP,TargetPort);
    default:
        return FALSE;
    }
}

BOOL CProxy::ConnectThroughProxyGet(char* ProxyIP,unsigned short ProxyPort)
{
    if (!this->Connect(ProxyIP,ProxyPort))
        return FALSE;
    return TRUE;
}
BOOL CProxy::ConnectThroughProxyConnect(char* ProxyIP,unsigned short ProxyPort,char* TargetIP,unsigned short TargetPort)
{
    if (!this->Connect(ProxyIP,ProxyPort))
        return FALSE;

    char ConnectHeader[MAX_PATH];
    char Reply[CPROXY_PACKET_SIZE];
    DWORD dwDataSize=0;
    char* psz;
    CAtlRegExp<CAtlRECharTraitsA> RegExpr;
    CAtlREMatchContext<CAtlRECharTraitsA> MatchContext;


    sprintf(ConnectHeader,
        "CONNECT %s:%u HTTP/1.1\r\n"
        "Host: %s:%u\r\n\r\n",
        TargetIP,TargetPort,
        TargetIP,TargetPort
        );
    if (!this->Send(ConnectHeader))
        return FALSE;

    if (!this->WaitForString("\r\n\r\n",(BYTE*)Reply,CPROXY_PACKET_SIZE,&dwDataSize,CPROXY_RECEIVE_TIMEOUT))
        return FALSE;

    // speed up reg exp
    psz=strstr(Reply,"\r\n\r\n");
    if (psz)
        *psz=0;

    // check proxy reply
    RegExpr.Parse("HTTP/([0-9]+)\\.([0-9]+)( +)2[0-9](.+)(.*)",FALSE);
    if (!RegExpr.Match(Reply,&MatchContext))
    {
        this->ReportError(_T("Bad Proxy reply\r\n"));
        return FALSE;
    }
    return TRUE;
}
BOOL CProxy::ConnectThroughProxySocks4(char* ProxyIP,unsigned short ProxyPort,char* TargetIP,unsigned short TargetPort)
{
    if (!this->Connect(ProxyIP,ProxyPort))
        return FALSE;
    
    CProxy::SOCKS4_HEADER Socks4Header;

    // version number 4
    Socks4Header.Version=4;
    // command code 1 (connect)
    Socks4Header.Command=1;

    Socks4Header.Port=htons(TargetPort);
    if (!this->GetIp(TargetIP,&Socks4Header.Ip))
        return FALSE;

    Socks4Header.End=0;

    // send socks 4 connect command
    if(!this->Send((BYTE*)&Socks4Header,sizeof(CProxy::SOCKS4_HEADER)))
        return FALSE;

    // wait proxy reply
    BYTE Data[8];
    DWORD dwDataSize=0;

    if (!this->WaitForSize((BYTE*)&Data,8,&dwDataSize,CPROXY_RECEIVE_TIMEOUT))
        return FALSE;

    // vn should = 0
    if (Data[0]!=0)
    {
        this->ReportError(_T("Bad proxy handshake"));
        return FALSE;
    }

    // code should be 90
    if (Data[1]!=90)
    {
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("Proxy error code : %u"),Data[1]);
        this->ReportError(pszMsg);
        return FALSE;
    }

    // dst port
    // dst ip
    if ((memcmp(&Data[2],&Socks4Header.Port,2)!=0)
        ||(memcmp(&Data[4],&Socks4Header.Ip,4)!=0)
        )
    {
        this->ReportError(_T("Bad proxy handshake"));
        return FALSE;
    }

    return TRUE;
}
BOOL CProxy::ConnectThroughProxySocks5(char* ProxyIP,unsigned short ProxyPort,char* TargetIP,unsigned short TargetPort)
{
    if (!this->Connect(ProxyIP,ProxyPort))
        return FALSE;

    BYTE Data[10];
    DWORD dwDataSize=0;
    unsigned short us;

    // version number 5
    Data[0]=5;
    Data[1]=1;// number of methods
    Data[2]=0;// ask for NO AUTHENTICATION REQUIRED 
    if (!this->Send((BYTE*)&Data,3))
        return FALSE;

    // loop until full header is received or error occurs
    if (!this->WaitForSize((BYTE*)&Data,2,&dwDataSize,CPROXY_RECEIVE_TIMEOUT))
        return FALSE;

    // vn should = 5
    if (Data[0]!=5)
    {
        this->ReportError(_T("Bad proxy handshake"));
        return FALSE;
    }
    //X'00' NO AUTHENTICATION REQUIRED 
    //X'01' GSSAPI 
    //X'02' USERNAME/PASSWORD 
    //X'03' to X'7F' IANA ASSIGNED 
    //X'80' to X'FE' RESERVED FOR PRIVATE METHODS 
    //X'FF' NO ACCEPTABLE METHODS 
    if (Data[1]!=0)
    {
        this->ReportError(_T("No authentication not allowed"));
        return FALSE;
    }

    // No Authentication accepted
    Data[0]=5;// version
    Data[1]=1;// connect cmd
    Data[2]=0;// reserved
    Data[3]=1;// address type=ipv4
    // dest ip
    if (!this->GetIp(TargetIP,(DWORD*)&Data[4]))
        return FALSE;
    // dest port
    us=htons(TargetPort);
    memcpy(&Data[8],&us,2);

    if (!this->Send((BYTE*)&Data,10))
        return FALSE;

    // loop until full header is received or error occurs
    if (!this->WaitForSize((BYTE*)&Data,10,&dwDataSize,CPROXY_RECEIVE_TIMEOUT))
        return FALSE;

    // vn should = 5
    if (Data[0]!=5)
    {
        this->ReportError(_T("Bad proxy handshake"));
        return FALSE;
    }

    switch(Data[1])
    {
    case 0://successful
        return TRUE;
    case 1:
        this->ReportError(_T("General SOCKS server failure."));
        return FALSE;
    case 2:
        this->ReportError(_T("Connection not allowed by ruleset."));
        return FALSE;
    case 3:
        this->ReportError(_T("Network unreachable."));
        return FALSE;
    case 4:
        this->ReportError(_T("Host unreachable."));
        return FALSE;
    case 5:
        this->ReportError(_T("Connection refused."));
        return FALSE;
    case 6:
        this->ReportError(_T("TTL expired."));
        return FALSE;
    case 7:
        this->ReportError(_T("Command not supported."));
        return FALSE;
    case 8:
        this->ReportError(_T("Address type not supported."));
        return FALSE;
    default:
        this->ReportError(_T("Unknown error code."));
        return FALSE;
    }
}