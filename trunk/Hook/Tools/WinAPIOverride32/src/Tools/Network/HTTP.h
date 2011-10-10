#pragma once

// use wininet function to avoid to manually grep proxy and other internet settings from registry (let the wininet do all by itself)

#include <windows.h>
#include <malloc.h>
#include <Wininet.h>
// required lib : Wininet.lib
#pragma comment (lib,"Wininet")
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#define CHTTP_HTTP_PROTOCOL_PREFIX _T("http://")
#define CHTTP_MAX_URL_SIZE 1024 // according to rfc max is 256

class CHTTP
{
public:
    CHTTP(void);
    ~CHTTP(void);

    static void ShowLastError();

    BOOL Get(CONST TCHAR* Url,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize);
    BOOL Get(CONST TCHAR* Url,CONST USHORT Port,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize);
    BOOL Post(CONST TCHAR* Url,CONST PBYTE Data, CONST SIZE_T DataSize,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize);
    BOOL Post(CONST TCHAR* Url,CONST USHORT Port,CONST PBYTE Data, CONST SIZE_T DataSize,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize);
    BOOL DoHttpRequest(CONST TCHAR* Url,CONST USHORT Port,CONST TCHAR* Headers,CONST TCHAR* Method,CONST PBYTE Data,CONST SIZE_T DataSize,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize);

};
