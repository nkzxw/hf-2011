#include "http.h"

#include <stdio.h>
#define HTTP_AGENT_NAME _T("CppHttpAgent")


void CHTTP::ShowLastError()
{

    TCHAR pcError[MAX_PATH];
    *pcError=0;
    DWORD dwLastError=::GetLastError();
    if (dwLastError==0)
        return;
    DWORD dw=::FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
                           GetModuleHandle( _T("wininet.dll") ),
                           dwLastError,
                           NULL,//GetUserDefaultLangID(), wininet errors seems to be only in English
                           pcError,
                           MAX_PATH-1,
                           NULL);
    //If the function succeeds, the return value is the number of TCHARs stored in the output buffer,
    //  excluding the terminating null character.
    //If the function fails, the return value is zero
    if(dw==0)
    {
        // FormatMessage failed
        _stprintf(pcError,_T("Error 0x%08X\r\n"),dwLastError);
    }

    ::MessageBox(NULL,pcError,_T("Error"),MB_OK|MB_ICONERROR);
}


CHTTP::CHTTP(void)
{
}

CHTTP::~CHTTP(void)
{
}
BOOL CHTTP::Get(CONST TCHAR* Url,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize)
{
    return this->Get(Url,INTERNET_DEFAULT_HTTP_PORT,pReceivedData,pReceivedDataSize);
}
BOOL CHTTP::Get(CONST TCHAR* Url,CONST USHORT Port,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize)
{
    return this->DoHttpRequest(Url,Port,0,_T("GET"),0,0,pReceivedData,pReceivedDataSize);
}

BOOL CHTTP::Post(CONST TCHAR* Url,CONST PBYTE Data, CONST SIZE_T DataSize,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize)
{
    return this->Post(Url,INTERNET_DEFAULT_HTTP_PORT,Data,DataSize,pReceivedData,pReceivedDataSize);
}

//sample static TCHAR Data[] = _T("name=John&userid=123&other=P%26Q");
BOOL CHTTP:: Post(CONST TCHAR* URL,CONST USHORT Port,CONST PBYTE Data, CONST SIZE_T DataSize,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize)
{
    static TCHAR Headers[] = _T("Content-Type: application/x-www-form-urlencoded");
    /*
    Notice: POST different ways: 
    1) application/x-www-form-urlencoded
    ________________________
    Content-Type: application/x-www-form-urlencoded
    Content-Length: 21

    time=16h35&date=21/01
    ________________________


    2) multipart/form-data
    ________________________
    Content-Type: multipart/form-data; boundary=---------------------------2116772612019
    Content-Length: 550

    -----------------------------2116772612019
    Content-Disposition: form-data; name="time"

    16h35
    -----------------------------2116772612019
    Content-Disposition: form-data; name="date"

    21/01
    -----------------------------2116772612019--
    ________________________
    */
    return this->DoHttpRequest(URL,Port,Headers,_T("POST"),Data,DataSize,pReceivedData,pReceivedDataSize);
}
// Method : GET / POST /PUT
BOOL CHTTP:: DoHttpRequest(CONST TCHAR* Url,CONST USHORT Port,CONST TCHAR* Headers,CONST TCHAR* Method,CONST PBYTE Data,CONST SIZE_T DataSize,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize)
{
    static LPCTSTR Accept[]={_T("*/*"), NULL};
    BOOL bSuccess=FALSE;

    USHORT LocalServerPort=Port;

    //////////////////////////////
    // convert url into HostName + UrlPath
    //////////////////////////////
    TCHAR* LocalURL;
    TCHAR UrlPath[CHTTP_MAX_URL_SIZE];
    TCHAR HostName[CHTTP_MAX_URL_SIZE];
    TCHAR UserName[CHTTP_MAX_URL_SIZE];
    TCHAR PassWord[CHTTP_MAX_URL_SIZE];

    // assume "http://" prefix is present
    if (_tcsnicmp(Url,CHTTP_HTTP_PROTOCOL_PREFIX,_tcslen(CHTTP_HTTP_PROTOCOL_PREFIX))==0)
    {
        LocalURL=(TCHAR*)Url;
    }
    else
    {
        LocalURL=(TCHAR*)_alloca((_tcslen(CHTTP_HTTP_PROTOCOL_PREFIX)+_tcslen(Url)+1)*sizeof(TCHAR));
        _tcscpy(LocalURL,CHTTP_HTTP_PROTOCOL_PREFIX);
        _tcscat(LocalURL,Url);
    }

    URL_COMPONENTS uc;
    memset(&uc,0,sizeof(uc));

    *UrlPath=0;
    *HostName=0;
    *UserName=0;
    *PassWord=0;

    uc.dwStructSize = sizeof(uc);
    uc.dwUrlPathLength = CHTTP_MAX_URL_SIZE;
    uc.lpszUrlPath=UrlPath;
    uc.dwHostNameLength = CHTTP_MAX_URL_SIZE;
    uc.lpszHostName = HostName;
    uc.dwPasswordLength = CHTTP_MAX_URL_SIZE;
    uc.lpszPassword = PassWord;
    uc.dwUserNameLength = CHTTP_MAX_URL_SIZE;
    uc.lpszUserName = UserName;

    HINTERNET hSession=NULL;
    HINTERNET hConnect=NULL;
    HINTERNET hHttpFile=NULL;

    if (!::InternetCrackUrl(LocalURL,(DWORD)_tcslen(LocalURL),ICU_DECODE,&uc))
        return FALSE;

    // if a port is specified (if no port specified InternetCrackUrl set uc.nPort to INTERNET_DEFAULT_HTTP_PORT)
    if (uc.nPort!=INTERNET_DEFAULT_HTTP_PORT)
        LocalServerPort=uc.nPort;

    //////////////////////////////
    // end of convert url into HostName + UrlPath
    //////////////////////////////

    // Open Internet session.
    hSession = ::InternetOpen(  HTTP_AGENT_NAME,
                                INTERNET_OPEN_TYPE_PRECONFIG,
                                NULL, 
                                NULL,
                                0) ;
    if (!hSession)
        goto CleanUp;

    // Connect to www.microsoft.com.
    hConnect = ::InternetConnect(hSession,
                                HostName,
                                LocalServerPort,
                                UserName,
                                PassWord,
                                INTERNET_SERVICE_HTTP,
                                0,
                                0) ;
    if (!hConnect)
        goto CleanUp;

    // Request the file /MSDN/MSDNINFO/ from the server.
    hHttpFile = ::HttpOpenRequest(  hConnect,
                                    Method,
                                    UrlPath,
                                    NULL,
                                    NULL,
                                    Accept,
                                    INTERNET_FLAG_DONT_CACHE,
                                    0) ;
    if (!hHttpFile)
        goto CleanUp;

    // Send the request.
    BOOL bSendRequest;
    SIZE_T HeadersSize;
    if (Headers)
        HeadersSize=_tcslen(Headers);
    else
        HeadersSize=0;

    bSendRequest = ::HttpSendRequest(hHttpFile, Headers, (DWORD)HeadersSize, Data, (DWORD)DataSize);
    if (!bSendRequest)
        goto CleanUp;


    // get status code
    DWORD Status;
    DWORD StatusSize=sizeof(Status);
    if (!::HttpQueryInfo(hHttpFile,
                        HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, 
                        (LPVOID)&Status, 
                        &StatusSize,
                        NULL)
        )
        goto CleanUp;
        
    // assume status is ok
    if (Status!=HTTP_STATUS_OK)
        goto CleanUp;


    // Get the length of the file.            
    DWORD dwContentLen;
    DWORD dwLengthBufQuery = sizeof(dwContentLen);
    if (::HttpQueryInfo(hHttpFile,
                        HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, 
                        (LPVOID)&dwContentLen, 
                        &dwLengthBufQuery,
                        NULL) 
        )
    {
        *pReceivedDataSize = (ULONG)dwContentLen ;

        // Allocate a buffer for the file.   
        *pReceivedData = new BYTE[*pReceivedDataSize+1] ;

        // Read the file into the buffer. 
        DWORD dwBytesRead ;
        BOOL bRead = ::InternetReadFile(hHttpFile,
                                        *pReceivedData,
                                        *pReceivedDataSize,
                                        &dwBytesRead);
        // Put a zero on the end of the buffer.
        (*pReceivedData)[dwBytesRead] = 0 ;

        if (!bRead)
            goto CleanUp;
    }
    else
    {
        DWORD BufferSize=4096;
        DWORD RemainingBufferSize=BufferSize;
        DWORD UsedBufferSize=0;
        DWORD dwBytesRead=0;
        PBYTE TmpBuffer;
        *pReceivedData = new BYTE[BufferSize+1] ;
        do
        {
            
            RemainingBufferSize-=dwBytesRead;

            // get used buffer size
            UsedBufferSize=BufferSize-RemainingBufferSize;

            if (RemainingBufferSize<=1024)
            {
                // save current buffer
                TmpBuffer=*pReceivedData;

                // increase buffer size
                BufferSize*=2;
                *pReceivedData = new BYTE[BufferSize+1] ;

                // compute new remaining data size
                RemainingBufferSize=BufferSize-UsedBufferSize;

                // copy old data to new buffer
                memcpy(*pReceivedData,TmpBuffer,UsedBufferSize);

                // destroy old memory
                delete TmpBuffer;

                // update used buffer size
                UsedBufferSize=BufferSize-RemainingBufferSize;
            }
            ::InternetReadFile(hHttpFile, &((*pReceivedData)[UsedBufferSize]), RemainingBufferSize, &dwBytesRead);
            
        }while (dwBytesRead );

        // compute used data size
        *pReceivedDataSize=BufferSize-RemainingBufferSize;

        // Put a zero on the end of the buffer.
        (*pReceivedData)[*pReceivedDataSize] = 0;
    }



    bSuccess=TRUE;

CleanUp:
    DWORD LastError=0;
    if (!bSuccess)
        LastError = ::GetLastError();
    // Close all of the Internet handles.
    if (hHttpFile)
        ::InternetCloseHandle(hHttpFile); 
    if (hConnect)
        ::InternetCloseHandle(hConnect) ;
    if (hSession)
        ::InternetCloseHandle(hSession) ;

    if (!bSuccess)
        ::SetLastError(LastError);

    return bSuccess;
}