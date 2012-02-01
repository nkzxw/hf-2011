
#ifndef __MARMenu
#define __MARMenu

#include <windows.h>
#include "SoftSnoop.h"
#include <Commctrl.h>
#include "resource.h"

void CreateMARDlg(HINSTANCE hInst,HANDLE hDlgOwner);
BOOL HandleRetValMod(char* szApi, void* pEax, DWORD *dwNewRetVal);

#endif