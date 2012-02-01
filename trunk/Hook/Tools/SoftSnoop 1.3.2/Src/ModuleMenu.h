
#ifndef __ModuleMenu
#   define __ModuleMenu

#   include <windows.h>
#   include <Commctrl.h>
#   include <Tlhelp32.h>
#   include "SoftSnoop.h"
#   include "resource.h"

    BOOL CreateModuleDlg(HINSTANCE hInst,HANDLE hDlgOwner, BOOL bShowModuleList);

#   endif