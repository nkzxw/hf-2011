#ifndef _LINK_H
#define _LINK_H

#include <windows.h>

typedef DWORD (*LINKCALLBACK)(LPVOID,DWORD) ;

BOOL Link_Init (HANDLE hDriver, LINKCALLBACK pfnCallback) ;

BOOL Link_Uninit () ;

#endif
