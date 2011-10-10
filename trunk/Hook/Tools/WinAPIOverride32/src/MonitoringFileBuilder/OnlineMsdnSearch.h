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
// Object: search function description on MSDN web site
//-----------------------------------------------------------------------------

#pragma once

#include "../Tools/Network/TCP_Client.h"
#include "../Tools/Network/Proxy.h"

#include "defines.h"
#include "../Tools/linklist/linklist.h"
#include "../Tools/string/ansiunicodeconvert.h"
#include "../Tools/string/TrimString.h"
#include "../Tools/string/WildCharCompare.h"
#include "../Tools/File/TextFile.h"

#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#pragma warning (push)
#pragma warning(disable : 4018 )
#pragma warning(disable : 4389 )
#include <atlrx.h>// for regular expression
#pragma warning (pop)

#define SEARCHED_DEFINITION_CACHE_MAX_SIZE 4*MAX_PATH

#define PROXY_LIST_NAME             _T("proxy.txt")
#define SEARCH_SITE                 "www.google.com"
#define MSDN_SITE                   "msdn.microsoft.com"
#define MSDN_REQUIRED_COOKIES       "CodeSnippetContainerLang=Visual C++" // ; name2 = value2
#define HTTP_PORT                   80
#define MAX_HEADER_SIZE             0x4000 // 1024 few years ago 1024 was enough, but nowadays servers seems to be happy to send so much of data that 1024 is not enough for http header... Crazy word



#define MSDN_RESULT_BEGIN_LAST      "(</b>" // should be last
#define MSDN_RESULT_END_LAST        "</p>"
#define MSDN_RESULT_END_LAST2       "</P>"


#define SEARCH_QUERY                "/search?hl=en&q=%s+site%%3A%s&btnI=I%%27m+Feeling+Lucky"
#define MAX_SEARCH_QUERY_SIZE       2*MAX_PATH
#define USER_AGENT                  "Mozilla/5.0" // requiered by some web site (like the msdn web site :D)
#define HTTP_QUERY                  "GET %s HTTP/1.1\r\n"\
                                    "Host: %s\r\n"\
                                    "User-Agent: %s\r\n"\
                                    "Accept: text/xml,text/html,text/plain\r\n"\
                                    "Accept-Charset: ISO-8859-1\r\n"\
                                    "Connection: Close\r\n\r\n"

#define HTTP_QUERY_WITH_COOKIES     "GET %s HTTP/1.1\r\n"\
                                    "Host: %s\r\n"\
                                    "User-Agent: %s\r\n"\
                                    "Cookie: %s\r\n"\
                                    "Accept: text/xml,text/html,text/plain\r\n"\
                                    "Accept-Charset: ISO-8859-1\r\n"\
                                    "Connection: Close\r\n\r\n"

#define MAX_HTTP_QUERY_SIZE         6*MAX_PATH
#define MAX_RECEIVED_CONTENT_LENGTH 0xFFFF
#define RECEIVE_TIMEOUT             60 // time in seconds

#define PROXY_FILE_TYPE_GET         "GET"
#define PROXY_FILE_TYPE_CONNECT     "CONNECT"
#define PROXY_FILE_TYPE_SOCKS4      "SOCKS4"
#define PROXY_FILE_TYPE_SOCKS5      "SOCKS5"
#define MAX_PROXY_CONNECT_ERROR     5

class COnlineMSDNSearch
{
protected:

    typedef struct tagProxy
    {
        char IP[MAX_PATH];
        unsigned short Port;
        CProxy::ProxyType Type;
        DWORD ErrorCount;
    }PROXY,*PPROXY;
    CLinkListItem* pCurrentlyUsedProxyItem;
    BOOL bGooglePageRankingFiltersSucks;
    CTCP_Client* pTCP_Client;
    CProxy* pProxy;
    TCHAR ProxyListFileName[MAX_PATH];

    char pszLastMSDNSearchedFunc[MAX_PATH];
    char pszLastMSDNSearchedFuncReturn[MAX_PATH];
    char pszLastMSDNSearchedFuncDescritpion[SEARCHED_DEFINITION_CACHE_MAX_SIZE];
    BOOL bLastMSDNSearchSuccessful;

    LPVOID UserMessageInformationCallBackUserParam;
    tagUserMessageInformationCallBack UserMessageInformationCallBack;
    BOOL bCancel;

    void TCHAR_TypeAsciiUnicodeAdjustment(char* string,BOOL bUnicode);
    void SocketErrorMessageCallBack(TCHAR* ErrorMessage);
    static void SocketErrorMessageCallBackStatic(TCHAR* ErrorMessage,LPVOID UserParam);
    void ReportUserMessageInfo(TCHAR* Message,tagUserMessagesTypes MessageType);

    // proxy related members
    BOOL ParseProxyList();
    CLinkList* pProxyList;
    static BOOL ProxyListLineCallBackStatic(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam);
    void ProxyListLineCallBack(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber);


public:

    COnlineMSDNSearch();
    ~COnlineMSDNSearch();

    BOOL Search(char* pszLibName,char* pszUnDecoratedFuncName,TCHAR** ppszParameters,TCHAR** ppszReturn);
    void Cancel();
    BOOL SetProxyFile(TCHAR* FileName);
    void SetUserMessageInformationCallBack(tagUserMessageInformationCallBack CallBack,LPVOID UserParam);
    BOOL bInternetConnectionError;
};
