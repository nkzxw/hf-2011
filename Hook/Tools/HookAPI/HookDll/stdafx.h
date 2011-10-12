// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <atlstr.h>
#define BUFSIZE 512
#define MAX_CHATCONTENTS_LEN 2048 
#define MAX_CHATNAME_LEN 256 
#define SF_USERNAME_LEN 64 

#pragma pack(push, 1)
typedef struct _QQCHAT_LOG
{
	WCHAR userName[SF_USERNAME_LEN];
	SYSTEMTIME dateTime;
	ULONG SrcQQNO;
	ULONG DestQQNO;
	ULONG groupNO;
	WCHAR msg[MAX_CHATCONTENTS_LEN + 1];
	WCHAR pSrcName[MAX_CHATNAME_LEN];
	WCHAR pDestName[MAX_CHATNAME_LEN];
	WCHAR GroupName[MAX_CHATNAME_LEN];
	BOOL bGroupMsg;
	USHORT direction;	

}QQCHAT_LOG, *PQQCHAT_LOG;
#pragma pack(pop)




// TODO: reference additional headers your program requires here
