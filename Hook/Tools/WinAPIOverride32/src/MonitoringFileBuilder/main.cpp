#include "MonitoringFileBuilderUI.h"

#include <windows.h>
#include <commctrl.h>
#pragma comment (lib,"comctl32")

int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // to enable XP visual style
    InitCommonControls();

    CMonitoringFileBuilderUI::Show(hInstance,NULL);

    return 0;
}