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
//
//  sorry this class is currently a great example of speed coding --> something we shouldn't do :D
//  hoping next versions will be cleaner
//  --> Fill free to improve it : it's really required :D
//-----------------------------------------------------------------------------
#include "OnlineMSDNSearch.h"

#ifndef _countof
    #define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

char MSDN_ResultBegin[][128]={
                            "<pre class=\"libCScode\"",
                            "<PRE CLASS=\"clsSyntax\"",
                            "<PRE class=\"syntax\"",
                            "<PRE class=syntax",
                            "<pre class=\"code\"",
                            "<P class=\"syntax\"",
                            "<div class=\"code\"",
                            "<pre class=\"syntax\"",
                            "<div id=\"CodeSnippetContainerCode0\"" // specific case MUST BE at last position
                        };
char MSDN_ResultEnd[][128]={ 
                        "</pre>",
                        "</PRE>",
                        "</PRE>",
                        "</PRE>",
                        "</pre>",
                        "</P>",
                        "</div>",
                        "</pre>",
                        "</pre>"
                        };


COnlineMSDNSearch::COnlineMSDNSearch()
{
    this->pProxy=new CProxy();
    // try to make code easily reading (else code readers will ask them why we always use a pProxy class instead of a pTCP_Client one)
    this->pTCP_Client=(CTCP_Client*)this->pProxy;
    this->pTCP_Client->SetErrorMessageCallBack(COnlineMSDNSearch::SocketErrorMessageCallBackStatic,this);
    this->pTCP_Client->bShowMessageBoxForErrorMessages=FALSE;

    this->bInternetConnectionError=FALSE;

    *this->pszLastMSDNSearchedFunc=0;
    *this->pszLastMSDNSearchedFuncDescritpion=0;
    *this->pszLastMSDNSearchedFuncReturn=0;
    this->bLastMSDNSearchSuccessful=FALSE;
    this->pProxyList=NULL;
    this->pCurrentlyUsedProxyItem=NULL;
    this->bGooglePageRankingFiltersSucks=FALSE;

    *this->ProxyListFileName=0;
    this->UserMessageInformationCallBackUserParam=0;
    this->UserMessageInformationCallBack=0;
}
COnlineMSDNSearch::~COnlineMSDNSearch(void)
{
    delete this->pProxy;
    if (this->pProxyList)
        delete this->pProxyList;
}

//-----------------------------------------------------------------------------
// Name: SocketErrorMessageCallBackStatic
// Object: callback for socket error
// Parameters :
//     in  : TCHAR* ErrorMessage : socket error message
//           LPVOID UserParam : associated CMonitoringFileBuilderobject
//     out :
//     return : 
//-----------------------------------------------------------------------------
void COnlineMSDNSearch::SocketErrorMessageCallBackStatic(TCHAR* ErrorMessage,LPVOID UserParam)
{
    ((COnlineMSDNSearch*)UserParam)->SocketErrorMessageCallBack(ErrorMessage);
}
void COnlineMSDNSearch::SocketErrorMessageCallBack(TCHAR* ErrorMessage)
{
    this->ReportUserMessageInfo(ErrorMessage,USER_MESSAGE_ERROR);
}

//-----------------------------------------------------------------------------
// Name: SetUserMessageInformationCallBack
// Object: set the error message callback
// Parameters :
//     in  : tagUserMessageInformationCallBack CallBack : callback called for each message information
//           LPVOID UserParam : user parameter
//     out :
//     return : 
//-----------------------------------------------------------------------------
void COnlineMSDNSearch::SetUserMessageInformationCallBack(tagUserMessageInformationCallBack CallBack,LPVOID UserParam)
{
    this->UserMessageInformationCallBackUserParam=UserParam;
    this->UserMessageInformationCallBack=CallBack;
}

//-----------------------------------------------------------------------------
// Name: ReportUserMessageInfo
// Object: report an error message
// Parameters :
//     in  : TCHAR* Message : message to report
//           CMonitoringFileBuilder::tagUserMessagesTypes MessageType : message type
//     out :
//     return : 
//-----------------------------------------------------------------------------
void COnlineMSDNSearch::ReportUserMessageInfo(TCHAR* Message,tagUserMessagesTypes MessageType)
{
    // if user message callback is defined
    if (this->UserMessageInformationCallBack)
        // call it
        this->UserMessageInformationCallBack(Message,MessageType,this->UserMessageInformationCallBackUserParam);
}

//-----------------------------------------------------------------------------
// Name: Cancel
// Object: Cancel current search
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void COnlineMSDNSearch::Cancel()
{
    this->bCancel=TRUE;
    this->pTCP_Client->CancelReceive();
}

//-----------------------------------------------------------------------------
// Name: Search
// Object: search function definition on online MDSN
// Parameters :
//     in  : char* pszLibName : library name
//           char* pszUnDecoratedFuncName : undecorated function name
//     out :
//           TCHAR** ppszReturn : NULL if not found.
//                                must be free with delete
//           TCHAR** ppszParameters : NULL if not found. contains beginning '(' and ending ')' and |Out option if pointer is found in func description
//                                    must be free with delete
//     return : TRUE if found, FALSE else
//-----------------------------------------------------------------------------
BOOL COnlineMSDNSearch::Search(char* pszLibName,char* pszUnDecoratedFuncName,TCHAR** ppszParameters,TCHAR** ppszReturn)
{
    UNREFERENCED_PARAMETER(pszLibName);
    // we don't use pszLibName yet, but we can check for it one day

    this->bCancel=FALSE;

    char* pszFuncName;
    BOOL bUnicodeVersion;
    SIZE_T FuncNameSize;
    char pszSearchQuery[MAX_SEARCH_QUERY_SIZE];
    char pszHttpQuery[MAX_HTTP_QUERY_SIZE];
    char pszHeader[MAX_HEADER_SIZE+1];
    char pszMSDNLink[MAX_PATH];
    char pszLastMoveMSDNLink[MAX_PATH];
    char pszLastMoveMSDNSite[MAX_PATH];
    char pszResultSiteAddress[MAX_PATH];
    char* pszSearched;
    char* pszContent;
    char* pszHtmlDescritption=NULL;
    char* psz;
    char* pc;
    char* pszDescription;
    char* pszDescriptionStart=NULL;
    char* pszAfterHeader;
    char* pszHtmlContent;
    TCHAR pszMsg[MAX_PATH];

    SIZE_T Size;
    SIZE_T ReceivedSize;
    BOOL bLastCharIsSpace;
    BOOL bIsSpace;
    BOOL bLineBreak;
    BOOL bHtmlTag;
    BOOL bOutParam;
    BOOL bFuncRetrievalError;
    CAtlRegExp<CAtlRECharTraitsA> RegExpr;
    CAtlREMatchContext<CAtlRECharTraitsA> MatchContext;
    const CAtlREMatchContext<CAtlRECharTraitsA>::RECHAR* pszMatchContextStart;
    const CAtlREMatchContext<CAtlRECharTraitsA>::RECHAR* pszMatchContextEnd;
    int iRes;
    CLinkListItem* pLastUsedProxyItem;
    PROXY* pProxyStruct=NULL;
    BOOL bProxyError;
    SIZE_T RemainingContentSize;
    SIZE_T ContentSize;

    *ppszReturn=0;
    *ppszParameters=0;

    // if an internet connection has failed,
    // don't try until a new parsing to avoid as many connect error messages as function
    if (this->bInternetConnectionError)
        return FALSE;

    // assume end of pszHeader
    pszHeader[MAX_HEADER_SIZE]=0;

    // make a local copy for not modifying pszUnDecoratedFuncName
    pszFuncName= strdup(pszUnDecoratedFuncName);

    // default : ascii version
    bUnicodeVersion=FALSE;

    // check func name to remove ansi / unicode terminator (A or W)
    FuncNameSize=strlen(pszFuncName);
    if (FuncNameSize>2)
    {
        if (((pszFuncName[FuncNameSize-1]=='A')||(pszFuncName[FuncNameSize-1]=='W'))
            // && isLowerCase(pszFuncName[FuncNameSize-2])
            && ((pszFuncName[FuncNameSize-2]>='a')&&(pszFuncName[FuncNameSize-2]<='z'))
            )
        {
            // fill bUnicodeVersion Flag
            bUnicodeVersion=(pszFuncName[FuncNameSize-1]=='W');

            // remove A or W
            pszFuncName[FuncNameSize-1]=0;
        }
    }

    // check if last retrieved func is not the same as the queried one
    // sample it's useless to retrieve definition of GetPrivateProfileIntA
    // and a second time for GetPrivateProfileIntW as we do search and parsing for 
    // generic GetPrivateProfileInt func 
    if (strcmp(this->pszLastMSDNSearchedFunc,pszFuncName)==0)
    {
        if (!this->bLastMSDNSearchSuccessful)
        {
            free(pszFuncName);
            return FALSE;
        }

        // TCHAR_TypeAsciiUnicodeAdjustment change string, but as we get the ansi and unicode
        // func, there can't be another func using this params
        this->TCHAR_TypeAsciiUnicodeAdjustment(this->pszLastMSDNSearchedFuncDescritpion,bUnicodeVersion);
        this->TCHAR_TypeAsciiUnicodeAdjustment(this->pszLastMSDNSearchedFuncReturn,bUnicodeVersion);

        // allocate and copy pszLastMSDNSearchedFuncDescritpion in ppszParameters
        // according to char encoding convention
        if (*this->pszLastMSDNSearchedFuncDescritpion)
        {
            Size=strlen(this->pszLastMSDNSearchedFuncDescritpion);
            *ppszParameters=new TCHAR[Size+1];
#if (defined(UNICODE)||defined(_UNICODE))
            MultiByteToWideChar(CP_ACP, 0, this->pszLastMSDNSearchedFuncDescritpion, Size,*ppszParameters, Size);
            (*ppszParameters)[Size]=0;
#else
            strcpy(*ppszParameters,this->pszLastMSDNSearchedFuncDescritpion);
#endif
        }
        // allocate and copy pszLastMSDNSearchedFuncReturn in ppszReturn
        // according to char encoding convention
        if (*this->pszLastMSDNSearchedFuncReturn)
        {
            Size=strlen(this->pszLastMSDNSearchedFuncReturn);
            *ppszReturn=new TCHAR[Size+1];
#if (defined(UNICODE)||defined(_UNICODE))
            MultiByteToWideChar(CP_ACP, 0, this->pszLastMSDNSearchedFuncReturn, Size,*ppszReturn, Size);
            (*ppszReturn)[Size]=0;
#else
            strcpy(*ppszReturn,this->pszLastMSDNSearchedFuncReturn);
#endif
        }

        this->ReportUserMessageInfo(_T("Function description retrieved using cache...\r\n"),USER_MESSAGE_INFORMATION);
        free(pszFuncName);
        return TRUE;
    }
    // initialize LastMSDNSearchXXX members
    this->bLastMSDNSearchSuccessful=FALSE;
    *this->pszLastMSDNSearchedFuncReturn=0;
    *this->pszLastMSDNSearchedFuncDescritpion=0;
    strcpy(this->pszLastMSDNSearchedFunc,pszFuncName);

    // show a user message
    this->ReportUserMessageInfo(_T("Searching on online MSDN...\r\n"),USER_MESSAGE_INFORMATION);

ConnectToSearchEngine:
    if (this->bGooglePageRankingFiltersSucks||(*this->ProxyListFileName!=0))
    {
        bProxyError=FALSE;
        do
        {
            // if non more proxy in list
            if (this->pProxyList->Head==NULL)
            {
                this->ReportUserMessageInfo(_T("No more proxy available... \r\n"),USER_MESSAGE_ERROR);
                free(pszFuncName);
                this->bInternetConnectionError=TRUE;
                return FALSE;
            }

            // get current proxy struct
            pProxyStruct=(PROXY*)this->pCurrentlyUsedProxyItem->ItemData;

#if (defined(UNICODE)||defined(_UNICODE))
            TCHAR* psz;
            CAnsiUnicodeConvert::AnsiToUnicode(pProxyStruct->IP,&psz);
            _stprintf(pszMsg,_T("Using proxy %s:%u\r\n"),psz,pProxyStruct->Port);
            free(psz);
#else
            _stprintf(pszMsg,_T("Using proxy %s:%u\r\n"),pProxyStruct->IP,pProxyStruct->Port);
#endif
            this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_INFORMATION);

            bProxyError=!this->pProxy->ConnectThroughProxy(pProxyStruct->IP,
                pProxyStruct->Port,
                pProxyStruct->Type,
                SEARCH_SITE,
                HTTP_PORT);
            // avoid to loop if user ask to stop
            if (this->bCancel)
            {
                // info "User cancel..." will be reported by calling func
                free(pszFuncName);
                return FALSE;
            }

            // store last used proxy
            pLastUsedProxyItem=this->pCurrentlyUsedProxyItem;

            // don't change proxy if proxy is requiered and 
            // google filter don't suck
            if (this->bGooglePageRankingFiltersSucks)
            {
                // get the next proxy
                if (this->pCurrentlyUsedProxyItem->NextItem==NULL)
                    this->pCurrentlyUsedProxyItem=this->pProxyList->Head;
                else
                    this->pCurrentlyUsedProxyItem=this->pCurrentlyUsedProxyItem->NextItem;

                if (bProxyError)
                {
                    // if error is upper than max allowed error
                    pProxyStruct->ErrorCount++;
                    if (pProxyStruct->ErrorCount>MAX_PROXY_CONNECT_ERROR)
                    {
                        // remove previous proxy from list
                        this->pProxyList->RemoveItemFromItemData(pLastUsedProxyItem);

                        // if we have removed all items then this->pProxyList->Head==NULL
                        // so we don't have to care with the next loop (see begin of loop)
                    }
                }
            }
        }
        while (bProxyError);

    }
    else
    {

        // if no proxy slow SEARCH_SITE connections to avoid to be banned 
        // (google seems to be 100 connections per minute, so sleeping 650 ms between each connection is ok)
        Sleep(650);

        // connect to search site
        if (!this->pTCP_Client->Connect(SEARCH_SITE,HTTP_PORT))
        {
            if (MessageBox(NULL,
                _T("Unable to connect to the search website\r\n")
                _T("this may result of an network error.\r\n")
                _T("Do you want to continue to use online MSDN ?"),
                _T("Error"),MB_YESNO|MB_ICONQUESTION|MB_TOPMOST)==IDNO)
                this->bInternetConnectionError=TRUE;
            free(pszFuncName);
            return FALSE;
        }
    }

    // make search query
    sprintf(pszSearchQuery,SEARCH_QUERY,pszFuncName,MSDN_SITE);

    // make http query
    // something like 
    //      GET /search?hl=en&q=MessageBox+site%3Amsdn.microsoft.com&btnI=I%27m+Feeling+Lucky HTTP/1.1
    //      Host: www.google.com
    //      Accept: text/html;text/plain;
    //      Accept-Charset: ISO-8859-1
    //      Connection: Close
    sprintf(pszHttpQuery,HTTP_QUERY,pszSearchQuery,SEARCH_SITE,USER_AGENT);

    // send http query
    if (!this->pTCP_Client->Send((BYTE*)pszHttpQuery,strlen(pszHttpQuery)))
    {
        free(pszFuncName);
        return FALSE;
    }

    // We should receive something like 
    //      HTTP/1.1 302 Found
    //      Location: http://msdn.microsoft.com/library/en-us/winui/winui/windowsuserinterface/windowing/dialogboxes/dialogboxreference/dialogboxfunctions/messagebox.asp?frame=true
    //      Cache-Control: private
    //      Set-Cookie: PREF=ID=7eb5:TM=112:LM=1160:S=6jtxeUH7; expires=Sun, 17-Jan-2038 19:14:07 GMT; path=/; domain=.google.com
    //      Content-Type: text/xml,text/html,text/plain
    //      Server: GWS/2.1
    //      Content-Length: 355
    //      Date: Tue, 10 Oct 2006 19:51:22 GMT
    //
    // Notice: you can see that google doesn't want to spy you : 31 YEARS FOR A COOKIE ! :D

    // wait the end of http header
    if (!this->pTCP_Client->WaitForString("\r\n\r\n",(BYTE*)pszHeader,MAX_HEADER_SIZE,&ReceivedSize,RECEIVE_TIMEOUT))
    {
        free(pszFuncName);
        return FALSE;
    }


    // try to speed up a little reg expr
    psz=strstr(pszHeader,"\r\n\r\n");
    if (!psz)
    {
        free(pszFuncName);
        this->ReportUserMessageInfo(_T("Http protocol error... Function description not found on MSDN\r\n"),USER_MESSAGE_ERROR);
        return FALSE;
    }

    *psz=0;
    // store after header data
    pszAfterHeader=psz+4;

    *pszLastMoveMSDNLink=0;
    *pszLastMoveMSDNSite=0;

    // try to parse reply and extract MSDN link
    // something like http://windowssdk.msdn.microsoft.com/en-us/library/system.windows.application.exit.aspx
    RegExpr.Parse("HTTP/([0-9]+)\\.([0-9]+)( +)30[0-9](.+)(\\n)Location( *):( *)http://{.*?(msdn[0-9]*).microsoft.com}{[^\r\n]+}(\r)*(.*)",FALSE);
    if (!RegExpr.Match(pszHeader,&MatchContext))
    {
        if (this->bGooglePageRankingFiltersSucks)
            this->ReportUserMessageInfo(_T("Proxy error or function description not found on MSDN\r\n"),USER_MESSAGE_ERROR);
        else
        {
            pszHtmlContent = NULL;
            // 2nd chance google may say stupid "Did you mean..." with links in page
            // find first link on function
            psz = strstr(pszAfterHeader,"http://msdn.microsoft.com/");
            if (!psz)// if header sent before data content, or data content not fully received (all google javascript shit takes a lot of place :D)
            {
                // grab content size from header /!\ content length not always present
                strupr( pszHeader );
                psz = strstr(pszHeader, "CONTENT-LENGTH");
                if (!psz)
                {
                    ContentSize = MAX_RECEIVED_CONTENT_LENGTH;
                    goto FunctionDescriptionNotFoundGetNextData;
                }
                pc = strstr(psz,"\r\n");
                if (pc)
                    *pc = 0;
                pc = strstr(psz,":");
                if (!pc)
                {
                    ContentSize = MAX_RECEIVED_CONTENT_LENGTH;
                    goto FunctionDescriptionNotFoundGetNextData;
                }
                pc++;
                for (;*pc;pc++)
                {
                    if (isdigit(*pc))
                        break;
                }
                ContentSize = atoi(pc);
                if (ContentSize ==0 )
                    goto FunctionDescriptionNotFound;
FunctionDescriptionNotFoundGetNextData:
                // get remaining data
                Size = strlen(pszAfterHeader);// current receive data size
                if (ContentSize > Size)
                {
                    // there is remaining data
                    // compute the remaining data size
                    RemainingContentSize = ContentSize - Size;// ContentSize-strlen(pszAfterHeader)
                    // allocate buffer for html content
                    pszHtmlContent = new char[ContentSize+1];
                    pszHtmlContent[ContentSize]=0;
                    // copy already received part
                    strcpy(pszHtmlContent ,pszAfterHeader);
                    // wait for remaining data
                    this->pTCP_Client->WaitForSize((BYTE*)&pszHtmlContent[Size],RemainingContentSize,&ReceivedSize,3);
                    // search in full html content
                    psz = strstr(pszHtmlContent,"http://msdn.microsoft.com/");
                }
            }

            // find next '"' (ending link content)
            if (psz)
                pc = strchr(psz,'"');
            else
                pc = NULL;

            if (pc)
            {
                // end psz. psz contains the new link
                *pc = 0;
                strcpy(pszResultSiteAddress,"msdn.microsoft.com");
                strcpy(pszMSDNLink, psz + strlen("http://msdn.microsoft.com"));// dont include last /
                if (pszHtmlContent)
                {
                    delete[] pszHtmlContent;
                    pszHtmlContent = NULL;
                }
                goto SiteConnection;
            }
            //else
FunctionDescriptionNotFound:
            this->ReportUserMessageInfo(_T("Function description not found on MSDN\r\n"),USER_MESSAGE_ERROR);

            if (pszHtmlContent)
            {
                delete[] pszHtmlContent;
                pszHtmlContent = NULL;
            }
        }
        free(pszFuncName);
        return FALSE;
    }
    // extract location from reply
    MatchContext.GetMatch(0, &pszMatchContextStart, &pszMatchContextEnd);

    memcpy(pszResultSiteAddress,pszMatchContextStart,pszMatchContextEnd-pszMatchContextStart);
    pszResultSiteAddress[pszMatchContextEnd-pszMatchContextStart]=0;

    MatchContext.GetMatch(1, &pszMatchContextStart, &pszMatchContextEnd);
    memcpy(pszMSDNLink,pszMatchContextStart,(pszMatchContextEnd-pszMatchContextStart)*sizeof(char));
    pszMSDNLink[pszMatchContextEnd-pszMatchContextStart]=0;

    // check if google ban you
    // host: www.google.com
    // content Google Error We're sorry but your query looks similar to automated requests from a computer virus or spyware application. To protect our users, we can't process your request right now
    if (strstr(pszResultSiteAddress,"www.google.com"))
    {
        // check if proxy list exists
        if (this->pProxyList)
        {
            pLastUsedProxyItem=this->pCurrentlyUsedProxyItem;
            // take next proxy
            if (this->pCurrentlyUsedProxyItem->NextItem)
                this->pCurrentlyUsedProxyItem=this->pCurrentlyUsedProxyItem->NextItem;
            else // next proxy is empty -> go back to head
                this->pCurrentlyUsedProxyItem=this->pProxyList->Head;

            // remove current proxy from list
            this->pProxyList->RemoveItemFromItemData(pLastUsedProxyItem);

            // reconnect to search engine
            goto ConnectToSearchEngine;
        }
        else // no proxy list
        {
            this->ReportUserMessageInfo(_T("Google temporary ban your IP to prevent abuse... \r\n"),USER_MESSAGE_WARNING);

UserChoice:
            // ask the user which action he wants to do
            iRes=MessageBox(NULL,
                _T("Google temporary ban your IP to prevent abuse... \r\n")
                _T("Select operation you want to do: \r\n")
                _T("- Manually disconnect and reconnect to the Internet to get a new IP (Click Retry when your IP has changed)\r\n")
                _T("- Use Proxy list defined in Proxy.txt file (Click continue)\r\n")
                _T("- Stop retrieving information from online MSDN (Click Cancel)"),
                _T("Warning"),
                MB_CANCELTRYCONTINUE|MB_ICONWARNING|MB_TOPMOST);
            // depending user choice
            switch(iRes)
            {
            case IDTRYAGAIN:
                // try again
                goto ConnectToSearchEngine;
            case IDCONTINUE:
                // parse proxy list
                if (!this->ParseProxyList())
                    // if proxy list parsing error ask the user again
                    goto UserChoice;
                // else
                // put flag to specify we have to use proxy
                this->bGooglePageRankingFiltersSucks=TRUE;
                // try to connect to google through proxy
                goto ConnectToSearchEngine;
            default:
                // user wants to stop retrieving information from online msdn
                this->bInternetConnectionError=TRUE;
                free(pszFuncName);
                return FALSE;
            }
        }
    }// end of google sucks



SiteConnection:
    // show a user message
    this->ReportUserMessageInfo(_T("Connecting to MSDN...\r\n"),USER_MESSAGE_INFORMATION);

    if (*this->ProxyListFileName!=0)
    {
        if(!this->pProxy->ConnectThroughProxy(pProxyStruct->IP,
            pProxyStruct->Port,
            pProxyStruct->Type,
            pszResultSiteAddress,
            HTTP_PORT))
        {
            free(pszFuncName);
            return FALSE;
        }
    }
    else
    {
        if (!this->pTCP_Client->Connect(pszResultSiteAddress,HTTP_PORT))
        {
            free(pszFuncName);
            return FALSE;
        }
    }

/* 
29/03/2010 : Following code was written during beta msdn and doesn't applies anymore
             online MSDN page.aspx redirect to page(VS.85).aspx so we must remove it to avoid infinite redirection

    // lots of msdn.xxxx/page(VS.80).aspx don't work anymore but msdn.xxxx/page.aspx work great
    // --> remove (VS.80)
    RegExpr.Parse("(.*)\\(VS.[0-9]+\\)\\.(.*))",FALSE);
    if (RegExpr.Match(pszMSDNLink,&MatchContext))
    {
        psz=strrchr(pszMSDNLink,'(');
        // end pszMSDNLink at (
        *psz=0;
        psz++;
        psz=strchr(psz,')');
        // point after )
        psz++;
        strcat(pszMSDNLink,psz);
    }
*/

    // make msdn http query
    //      GET /library/en-us/winui/winui/windowsuserinterface/windowing/dialogboxes/dialogboxreference/dialogboxfunctions/messagebox.asp?frame=true HTTP/1.1
    //      Host: msdn.microsoft.com
    //      User-Agent: Mozilla/5.0       <--- requiered else we get an msdn error
    //      Cookie: CodeSnippetContainerLang = C++
    //      Accept: text/xml,text/html,text/plain
    //      Accept-Charset: ISO-8859-1
    //      Connection: Close
    sprintf(pszHttpQuery,HTTP_QUERY_WITH_COOKIES,pszMSDNLink,pszResultSiteAddress,USER_AGENT,MSDN_REQUIRED_COOKIES);
    if (!this->pTCP_Client->Send((BYTE*)pszHttpQuery,strlen(pszHttpQuery)))
    {
        free(pszFuncName);
        return FALSE;
    }

    // wait the end of http header
    if (!this->pTCP_Client->WaitForString("\r\n\r\n",(BYTE*)pszHeader,MAX_HEADER_SIZE,&ReceivedSize,RECEIVE_TIMEOUT))
    {
        free(pszFuncName);
        return FALSE;
    }

    // try to speed up a little reg expr
    psz=strstr(pszHeader,"\r\n\r\n");
    if (!psz)
    {
        free(pszFuncName);
        this->ReportUserMessageInfo(_T("Http protocol error... Function description not found on MSDN\r\n"),USER_MESSAGE_ERROR);
        return FALSE;
    }
    *psz=0;

    // store after header data
    pszAfterHeader=psz+4;

    // try to parse reply and extract Content-Length link
    RegExpr.Parse("HTTP/([0-9]+)\\.([0-9]+)( +)20[0-9](.+)(\\n)Content-Length( *):( *){[0-9]+}(.*)",FALSE);
    if (!RegExpr.Match(pszHeader,&MatchContext))
    {

        // if page has moved
        RegExpr.Parse("HTTP/([0-9]+)\\.([0-9]+)( +)30[0-9](.+)(\\n)Location( *):( *){[^\r\n]+}(\r)*(.*)",FALSE);
        if (RegExpr.Match(pszHeader,&MatchContext))
        {
            // show a user message
            this->ReportUserMessageInfo(_T("MSDN page has moved. Going to new page...\r\n"),USER_MESSAGE_INFORMATION);

            // extract location from reply
            MatchContext.GetMatch(0, &pszMatchContextStart, &pszMatchContextEnd);
            memcpy(pszMSDNLink,pszMatchContextStart,pszMatchContextEnd-pszMatchContextStart);
            pszMSDNLink[pszMatchContextEnd-pszMatchContextStart]=0;

            // if there's a server address specified
            if (*pszMSDNLink!='/')
            {

                if (strnicmp(pszMSDNLink,"http://",strlen("http://"))==0)
                {
                    psz=pszMSDNLink+strlen("http://");
                    strcpy(pszMSDNLink,psz);
                }

                // try to split server/url
                psz=strchr(pszMSDNLink,'/');
                if (psz)
                {
                    strncpy(pszResultSiteAddress,pszMSDNLink,psz-pszMSDNLink);
                    pszResultSiteAddress[psz-pszMSDNLink]=0;
                    strcpy(pszMSDNLink,psz);
                }
            }

            // avoid looping
            if ((strcmp(pszLastMoveMSDNLink,pszMSDNLink)!=0)
                ||(strcmp(pszLastMoveMSDNSite,pszResultSiteAddress)!=0))
            {
                // store current pszMSDNLink and pszResultSiteAddress
                strcpy(pszLastMoveMSDNLink,pszMSDNLink);
                strcpy(pszLastMoveMSDNSite,pszResultSiteAddress);
                goto SiteConnection;
            }
        }
        free(pszFuncName);
        this->ReportUserMessageInfo(_T("Error parsing http header\r\n"),USER_MESSAGE_ERROR);
        return FALSE;
    }


    // extract Content-Length from reply
    MatchContext.GetMatch(0, &pszMatchContextStart, &pszMatchContextEnd);
    Size = (SIZE_T)atoi(pszMatchContextStart);

    pszContent=new char[Size+1];
    // assume that the string will be ended any number of char received
    memset(pszContent,0,Size+1);

    // copy first bytes received with header in pszContent
    strcpy(pszContent,pszAfterHeader);

    ReceivedSize=strlen(pszContent);
    // compute remaining size to receive
    Size-=ReceivedSize;

    this->ReportUserMessageInfo(_T("MSDN page found. Downloading data...\r\n"),USER_MESSAGE_INFORMATION);

    // if page is not fully downloaded
    if (Size>0)
    {
        // wait to receive full page content
        if (!this->pTCP_Client->WaitForSize((BYTE*)(&pszContent[ReceivedSize]),Size,&ReceivedSize,RECEIVE_TIMEOUT))
        {
            free(pszFuncName);
            delete[] pszContent;
            return FALSE;
        }
    }

    this->ReportUserMessageInfo(_T("MSDN page downloaded. Retreiving function description...\r\n"),USER_MESSAGE_INFORMATION);

    // try to retrieve function description
    //
    // Notice : REGULAR EXPRESSION ARE TOO SLOW
    //
    // content should contain something like
    // 1)
    //    <PRE CLASS="clsSyntax">
    //        int&nbsp;MessageBox(&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<BR></BR>&nbsp;&nbsp;&nbsp;&nbsp;HWND&nbsp;<I>hWnd</I>,
    //        &nbsp;&nbsp;&nbsp;&nbsp;LPCTSTR&nbsp;<I>lpText</I>,
    //        &nbsp;&nbsp;&nbsp;&nbsp;LPCTSTR&nbsp;<I>lpCaption</I>,
    //        &nbsp;&nbsp;&nbsp;&nbsp;UINT&nbsp;<I>uType</I>
    //        );
    //    </PRE>
    //
    // or 2)
    //    <PRE class="syntax"><B>char</B> <B>*strchr(</B>   /// or <PRE class="syntax" xml:space="preserve">
    //    <B>   const</B> <B>char</B> <B>*</B><I>string</I><B>,</B>
    //    <B>   int</B> <I>c</I> 
    //    <B>);</B>
    //    <B>wchar_t</B> <B>*wcschr(</B>
    //    <B>   const</B> <B>wchar_t</B> <B>*</B><I>string</I><B>,</B>
    //    <B>   wchar_t</B> <I>c</I> 
    //    <B>);</B>
    //    <B>unsigned</B> <B>char</B> <B>*_mbschr(</B>
    //    <B>   const</B> <B>unsigned</B> <B>char</B> <B>*</B><I>string</I><B>,</B>
    //    <B>   unsigned</B> <B>int</B> <I>c</I> 
    //    <B>);</B></PRE>
    // or 3)
    //    <PRE class=syntax>
    // or 4)
    //    <PRE class="Code">
    // or 5)
    //    <P class="syntax"
    // or 6)
    //
    // ... see .h
    //
    // <p>
    // <b>wchar_t</b> <b>*wcschr(</b> <b>const</b> <b>wchar_t</b> <b>*</b><i>string</i><b>,</b> <b>wint_t</b> <i>c</i> <b>);</b></p>

    bFuncRetrievalError=FALSE;

    BOOL StartingSyntaxFound=FALSE;

    for (DWORD Cnt=0;Cnt<_countof(MSDN_ResultBegin);Cnt++)
    {
        pszDescriptionStart=strstr(pszContent,MSDN_ResultBegin[Cnt]);
        if (pszDescriptionStart)
        {
            // specific case for last one (multiple tags)
            if (Cnt == (_countof(MSDN_ResultBegin)-1) )
            {
                // search for next <pre> tag
                pszDescriptionStart=strstr(pszDescriptionStart,"<pre>");
                if (!pszDescriptionStart)
                    break;
            }
            StartingSyntaxFound=TRUE;
            pszDescriptionStart=strchr(pszDescriptionStart,'>');
            if (pszDescriptionStart)
            {
                pszDescriptionStart++;
                psz=strstr(pszDescriptionStart,MSDN_ResultEnd[Cnt]);
            }
            else
                bFuncRetrievalError=TRUE;

            break;
        }
    }

    if (!StartingSyntaxFound)
    {

        // search Last
        // search funcname+"(</b>"
        pszSearched=new char[strlen(pszFuncName)+1+5];
        strcpy(pszSearched,pszFuncName);
        strcat(pszSearched,MSDN_RESULT_BEGIN_LAST);
        pszDescriptionStart=strstr(pszContent,pszSearched);
        if (pszDescriptionStart)
        {
            // search previous <p>
            psz=pszDescriptionStart;
            while(psz>pszContent)
            {
                if (*psz!='p')
                {
                    psz--;
                    continue;
                }
                if ((*(psz-1)=='<')&&(*(psz+1)=='>'))
                {
                    // point after <p>
                    psz+=2;
                    break;
                }
                psz--;
            }
            // check if found
            if (psz==pszContent)
                psz=NULL;
            pszDescriptionStart=psz;
            if (pszDescriptionStart)
            {
                psz=strstr(pszDescriptionStart,MSDN_RESULT_END_LAST);
                if (!psz)
                    psz=strstr(pszDescriptionStart,MSDN_RESULT_END_LAST2);
            }
            else
                bFuncRetrievalError=TRUE;
        }
        else
            bFuncRetrievalError=TRUE;
        delete[] pszSearched;// belong to last case
    }

    if (pszDescriptionStart && (!bFuncRetrievalError))
    {
        if (!psz)
            bFuncRetrievalError=TRUE;
        else
        {
            pszHtmlDescritption=new char[(psz-pszDescriptionStart)+1];
            memcpy(pszHtmlDescritption,pszDescriptionStart,(psz-pszDescriptionStart)*sizeof(char));
            pszHtmlDescritption[psz-pszDescriptionStart]=0;
        }
    }
    if (bFuncRetrievalError|| (pszHtmlDescritption==NULL))
    {
        free(pszFuncName);
        delete[] pszContent;
        this->ReportUserMessageInfo(_T("Error can't retrieve function description\r\n"),USER_MESSAGE_ERROR);
        return FALSE;
    }

    // pszContent is no more used
    delete[] pszContent;

    ////////////////////////////
    // get non html description
    ////////////////////////////

    Size=strlen(pszHtmlDescritption);
    pszDescription=new char[Size+1+4];// +1 for \0, +4 for option |Out
    memset(pszDescription,0,Size+1+4);

    // store pszDescription start address
    pszDescriptionStart=pszDescription;

    // replace &nbsp; and remove html markup
    bLastCharIsSpace=TRUE;

    psz=pszHtmlDescritption;
    while((*psz!=0)&&((SIZE_T)(pszDescription-pszDescriptionStart)<Size+4))
    {

        // useless stuff found in some description
        if (*psz==(char)0xc2)
        {
            psz++;
            continue;
        }

        bIsSpace=FALSE;
        bLineBreak=FALSE;
        bHtmlTag=FALSE;

        // if begin of html markup
        while ((*psz=='<')&&(*psz!=0))
        {
            // find end of markup
            while ((*psz!='>')&&(*psz!=0))
                psz++;

            if (*psz==0)
                break;

            psz++;
            bHtmlTag=TRUE;
        }
        if (bHtmlTag)
            continue;

        while((*psz=='&')&&(*psz!=0))
        {
            bIsSpace=TRUE;
            // find end of &xxx;
            while ((*psz!=';')&&(*psz!=0))
                psz++;

            if (*psz==0)
                break;

            continue;
        }
        if ((*psz==' ')||(*psz==(char)0xa0))// act like &nbsp; (0xa0 non breaking space)
        {
            psz++;
            bIsSpace=TRUE;
        }
        if (bIsSpace)
        {
            if (!bLastCharIsSpace)
            {
                // add space to pszDescription
                *pszDescription=' ';
                pszDescription++;
                bLastCharIsSpace=TRUE;
            }
            // continue to check for html tags
            continue;
        }

        // remove \r\n
        while ((*psz=='\r')||(*psz=='\n'))
        {
            bLineBreak=TRUE;
            psz++;
        }
        if (bLineBreak)
            // continue to check for html tags and space
            continue;

        // remove comments
        if (*psz=='/')
        {
            if (*(psz+1)=='/')
            {
                // copy nothing until \n \r\n, <br> or <BR>
                char* pszTmp1=strstr(psz,"\r\n");
                char* pszTmp2=strstr(psz,"<br>");
                char* pszTmp3=strstr(psz,"<BR>");
                char* pszTmp4=strstr(psz,"\n");
                if (!pszTmp1)
                    pszTmp1=(char*)0xFFFFFFFF;
                if ((pszTmp2<pszTmp1)&&(pszTmp2!=0))
                    pszTmp1=pszTmp2;
                if ((pszTmp3<pszTmp1)&&(pszTmp3!=0))
                    pszTmp1=pszTmp3;
                if ((pszTmp4<pszTmp1)&&(pszTmp4!=0))
                    pszTmp1=pszTmp4;
                if (pszTmp1!=(char*)0xFFFFFFFF)
                {
                    psz=pszTmp1;
                    continue;
                }
            }
        }

        bLastCharIsSpace=FALSE;
        // copy char to pszDescription
        *pszDescription=*psz;
        pszDescription++;

        // check for end of func definition
        if ((*psz==')')||(*psz==0))
            break;

        psz++;
    }

    // pszHtmlDescritption is useless from now
    delete[] pszHtmlDescritption;

    // restore pszDescription start address
    pszDescription=pszDescriptionStart;

    // if exported name is a property, not a function msdn can describe more than one property that means we can get something like 
    // extern int _daylight;extern long _timezone;extern char *_tzname[2];
    // or sometimes more than 1 definition is present

    // get function name position
    psz=strstr(pszDescription,pszFuncName);
    if (psz)
    {
        // if another definition is before func name
        pc=strrchr(psz,';');
        if (pc)
        {
            pc++;
            // remove definition
            memmove(pszDescription,pc,strlen(pc)+1);//+1 to add \0

            // refresh position of func name
            psz=strstr(pszDescription,pszFuncName);
        }
        // if another definition is after func name
        pc=strchr(pszDescription,';');
        if (pc)
            // end string
            *pc=0;
    }

    // get return (string before function name)
    if (psz)
    {
        *psz=0;
        pszDescription=psz+1;

        // store return in cache
        strcpy(this->pszLastMSDNSearchedFuncReturn,pszDescriptionStart);

        this->TCHAR_TypeAsciiUnicodeAdjustment(pszDescriptionStart,bUnicodeVersion);

        Size=strlen(pszDescriptionStart);
        // allocate memory for return
        *ppszReturn=new TCHAR[Size+1];
        // copy return type
#if (defined(UNICODE)||defined(_UNICODE))
        MultiByteToWideChar(CP_ACP, 0, pszDescriptionStart, Size,*ppszReturn, Size);
#else
        strcpy(*ppszReturn,pszDescriptionStart);
#endif
        // ends string
        (*ppszReturn)[Size]=0;
    }

    // remove function name
    psz=strchr(pszDescription,'(');
    if (!psz)
    {
        // free *ppszReturn if allocated
        if (*ppszReturn)
        {
            delete *ppszReturn;
            // reset *ppszReturn because a check is done by calling func
            *ppszReturn=NULL;
        }

        delete pszDescriptionStart;
        free(pszFuncName);

        // report error
        this->ReportUserMessageInfo(_T("Error can't retrieve function description\r\n"),USER_MESSAGE_ERROR);

        return FALSE;
    }
    // point on (
    pszDescription=psz;

    // replace (void) by ()
    if (strnicmp(pszDescription,"(void)",strlen("(void)"))==0)
        strcpy(pszDescription,"()");

    // check if a param is marked as OUT, or there's pointed value add |Out option to func description
    bOutParam=FALSE;
    if (strstr(pszDescription," OUT ")// search for OUT arg keyword
        || strstr(pszDescription,",OUT ")
        || strstr(pszDescription,"(OUT ")
        || strstr(pszDescription," __out ")// search for __out arg keyword
        || strstr(pszDescription,",__out ")
        || strstr(pszDescription,"(__out ")
        || strstr(pszDescription," __inout ")// search for __out arg keyword
        || strstr(pszDescription,",__inout ")
        || strstr(pszDescription,"(__inout ")
        )
    {
        bOutParam=TRUE;
    }
    if (!bOutParam) // search for '*'
    {
        char* pszArg;
        char* pszStar;
        char* pszComa;
        char* pszConst;
        char* pszConst2;

        pszArg=strchr(pszDescription,'(');
        while (pszArg)
        {
            // search '*'
            pszStar=strchr(pszArg,'*');
            if (!pszStar)
                break;

            // search previous delimiter ',' or '('
            psz=pszStar--;
            pszComa=NULL;
            while (psz>=pszArg)
            {
                if ((*psz==',')||(*psz=='('))
                {
                    pszComa=psz;
                    break;
                }
                psz--;
            }
            if (!pszComa)
            {
                // supposed out param by default
                bOutParam=TRUE;
                break;
            }
            // update current arg
            pszArg=pszComa;

            // search const keyword
            pszConst=strstr(pszArg,"(const ");
            if(!pszConst)
            {
                pszConst=strstr(pszArg," const ");
                pszConst2=strstr(pszArg,",const ");
                if ((pszConst2<pszConst)&& (pszConst2!=0))
                    pszConst=pszConst2;
            }

            if ((pszConst==0)||(pszConst>pszStar))
            {
                // not a const pointer
                bOutParam=TRUE;
                break;
            }

            // find next arg
            pszComa=strchr(pszStar,',');
            pszArg=pszComa;
        }
    }
    if (!bOutParam)
    {
        psz=pszDescription;
        while ((psz=strstr(psz,"TSTR "))&&(!bOutParam))// LPTSTR, PTSTR, LPCTSRT, PCTSTR
        {
            if (*(psz-1)!='C')// check that's not a constant string
                bOutParam=TRUE;
            psz+=5;
        }

        // begin of PXXX --> PWORD, PDWORD, PSTRUCT
        psz=pszDescription;
        while ((psz=strstr(psz," P"))&&(!bOutParam))
        {
            if (*(psz+2)!='C')// check that's not a constant string
            {
                // check that next char is upper case
                if ((*(psz+2)>='A')&&(*(psz+2)<='Z'))
                    bOutParam=TRUE;
            }
            psz+=2;
        }
        psz=pszDescription;
        while ((psz=strstr(psz,"(P"))&&(!bOutParam))
        {
            if (*(psz+2)!='C')// check that's not a constant string
                bOutParam=TRUE;
            psz+=2;
        }
        psz=pszDescription;
        while ((psz=strstr(psz,",P"))&&(!bOutParam))
        {
            if (*(psz+2)!='C')// check that's not a constant string
                bOutParam=TRUE;
            psz+=2;
        }
        // begin of LPXXX --> LPWORD, LPDWORD, LPSTRUCT
        psz=pszDescription;
        while ((psz=strstr(psz," LP"))&&(!bOutParam))
        {
            if (*(psz+3)!='C')// check that's not a constant string
                bOutParam=TRUE;
            psz+=3;
        }
        psz=pszDescription;
        while ((psz=strstr(psz,"(LP"))&&(!bOutParam))
        {
            if (*(psz+3)!='C')// check that's not a constant string
                bOutParam=TRUE;
            psz+=3;
        }
        psz=pszDescription;
        while ((psz=strstr(psz,",LP"))&&(!bOutParam))
        {
            if (*(psz+3)!='C')// check that's not a constant string
                bOutParam=TRUE;
            psz+=3;
        }
    }

    if (bOutParam)
        strcat(pszDescription,"|Out");

    //////////////////////////////////////////
    // Store func name and param in cache
    //////////////////////////////////////////
    if (strlen(pszDescription)>=SEARCHED_DEFINITION_CACHE_MAX_SIZE)
    {
        // we can't use cache for this func, and as returned type should have been changed
        // empty func name to remove last successful func info
        *this->pszLastMSDNSearchedFunc=0;
    }
    else
    {
        // store params
        strcpy(this->pszLastMSDNSearchedFuncDescritpion,pszDescription);
    }

    // pszFuncName is no more used
    free(pszFuncName);


    // adjust type name depending ASCII or Unicode version
    this->TCHAR_TypeAsciiUnicodeAdjustment(pszDescription,bUnicodeVersion);


    // copy pszDescription to *ppszParameters depending character encoding
    Size=strlen(pszDescription);
    *ppszParameters=new TCHAR[Size+1];

#if (defined(UNICODE)||defined(_UNICODE))
    MultiByteToWideChar(CP_ACP, 0, pszDescription, Size,*ppszParameters, Size);
    (*ppszParameters)[Size]=0;
#else
    strcpy(*ppszParameters,pszDescription);
#endif

    // pszDescription is no more use
    delete pszDescriptionStart;

    // report message info
    this->ReportUserMessageInfo(_T("Function description successfully retrieved\r\n"),USER_MESSAGE_INFORMATION);

    this->bLastMSDNSearchSuccessful=TRUE;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: TCHAR_TypeAsciiUnicodeAdjustment
// Object: adjust TCHAR types depending ASCII or Unicode version
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void COnlineMSDNSearch::TCHAR_TypeAsciiUnicodeAdjustment(char* string,BOOL bUnicode)
{
    char* psz;
    // if LPTSTR, PTSTR, PCTSRT, PCTSTR args
    psz=string;
    while((psz=strstr(psz,"TSTR "))!=0)
    {
        // psz points to T
        if (bUnicode)
        {
            // replace T by W
            *psz='W';
        }
        else
        {
            *psz='S';
            psz++;
            *psz='T';
            psz++;
            *psz='R';
            psz++;
            *psz=' ';// add a space
        }
    }

    // if TCHAR, TCHAR* args
    psz=string;
    while((psz=strstr(psz,"TCHAR"))!=0)
    {
        // psz points to P
        psz++;// make it points to T

        if (bUnicode)
        {
            // replace T by W
            *psz='W';
        }
        else
        {
            // replace T by space
            *psz=' ';
        }
    }
}

//-----------------------------------------------------------------------------
// Name: SetProxyFile
// Object: sets the proxy file
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL COnlineMSDNSearch::SetProxyFile(TCHAR* FileName)
{
    if (!FileName)
        return FALSE;
    _tcscpy(this->ProxyListFileName,FileName);
    return this->ParseProxyList();
}

//-----------------------------------------------------------------------------
// Name: ParseProxyList
// Object: parse proxy file list
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL COnlineMSDNSearch::ParseProxyList()
{
    // if already parsed
    if (this->pProxyList)
        return TRUE;

    // create a ClinkList of PROXY structs
    this->pProxyList=new CLinkList(sizeof(PROXY));

    // parse file
    if (!CTextFile::ParseLines(this->ProxyListFileName,ProxyListLineCallBackStatic,this))
    {
        delete (this->pProxyList);
        this->pProxyList=NULL;
        return FALSE;
    }

    // init this->pCurrentlyUsedProxyItem
    this->pCurrentlyUsedProxyItem=this->pProxyList->Head;

    // if proxy list is empty
    if (this->pProxyList->Head==NULL)
    {
        delete (this->pProxyList);
        this->pProxyList=NULL;

        // show user message
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("No proxy found in %s\r\n"),ProxyListFileName);
        this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_ERROR);

        return FALSE;
    }

    // proxy have been found
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ProxyListLineCallBackStatic
// Object: proxy file line read callback
// Parameters :
//     in  : TCHAR* Line
//           DWORD dwLineNumber
//           LPVOID UserParam : associated CMonitoringFileBuilder object
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL COnlineMSDNSearch::ProxyListLineCallBackStatic(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam)
{
    // reenter object model
    ((COnlineMSDNSearch*)UserParam)->ProxyListLineCallBack(FileName,Line,dwLineNumber);
    return TRUE;
}
void COnlineMSDNSearch::ProxyListLineCallBack(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber)
{
    // line is like ip:port:type
    // if type is not specified, default it to CProxy::PROXY_TYPE_HTTP_GET
    char* pszLine;
    char* psz;
    char* pszPort;
    PROXY Proxy;
    Proxy.ErrorCount=0;

    // if empty or comment line
    if ((*Line==';')||(*Line==0))
        return;

#if (defined(UNICODE)||defined(_UNICODE))
    // convert line to ascii
    CAnsiUnicodeConvert::UnicodeToAnsi(Line,&pszLine);
#else
    pszLine=Line;
#endif

    // find ip
    pszPort=strchr(pszLine,':');
    if (!pszPort)
    {
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("Syntax error in proxy list file %s\r\nat line %u: port not Found.\r\n"),FileName,dwLineNumber);
        this->ReportUserMessageInfo(pszMsg,USER_MESSAGE_ERROR);
        return;
    }
    *pszPort=0;
    pszPort++;
    strcpy(Proxy.IP,pszLine);

    psz=strchr(pszPort,':');
    if (!psz)
    {
        // get port number
        Proxy.Port=(unsigned short)atoi(pszPort);
        // set type to default
        Proxy.Type=CProxy::PROXY_TYPE_HTTP_GET;
    }
    else
    {
        // ends pszPort
        *psz=0;
        // point after :
        psz++;
        Proxy.Port=(unsigned short)atoi(pszPort);

        if (stricmp(psz,PROXY_FILE_TYPE_GET)==0)
        {
            Proxy.Type=CProxy::PROXY_TYPE_HTTP_GET;
        }
        else if (stricmp(psz,PROXY_FILE_TYPE_CONNECT)==0)
        {
            Proxy.Type=CProxy::PROXY_TYPE_HTTP_CONNECT;
        }
        else if (stricmp(psz,PROXY_FILE_TYPE_SOCKS4)==0)
        {
            Proxy.Type=CProxy::PROXY_TYPE_SOCKS4;
        }
        else if (stricmp(psz,PROXY_FILE_TYPE_SOCKS5)==0)
        {
            Proxy.Type=CProxy::PROXY_TYPE_SOCKS5;
        }
        else
            // default type
            Proxy.Type=CProxy::PROXY_TYPE_HTTP_GET;
    }
    // add item to list
    this->pProxyList->AddItem(&Proxy);

#if (defined(UNICODE)||defined(_UNICODE))
    // free allocated string
    free(pszLine);
#endif
}