#include "windowthreadprocessiduserinterface.h"
#include "resource.h"
#include <stdio.h>

CWindowThreadProcessIdUserInterface::CWindowThreadProcessIdUserInterface(void)
{
}

CWindowThreadProcessIdUserInterface::~CWindowThreadProcessIdUserInterface(void)
{
}

void CWindowThreadProcessIdUserInterface::OnInit()
{
    this->pSelectWindow = new CSelectWindow();
    this->pSelectWindow->bSelectOnlyDialog = TRUE;
    this->pSelectWindow->bOwnerWindowSelectable = TRUE;
    this->pSelectWindow->Initialize(this->GetInstance(),this->GetControlHandle(),IDC_WINDOW_THREAD_PROCESS_ID_PICTURE_CONTROL,
                                    IDB_BLANK,IDB_CROSS,IDC_CURSOR_TARGET);
}
void CWindowThreadProcessIdUserInterface::OnClose()
{
    delete this->pSelectWindow;
}

void CWindowThreadProcessIdUserInterface::OnMouseDown(WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    this->pSelectWindow->MouseDown(lParam);
}
void CWindowThreadProcessIdUserInterface::OnMouseUp(WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    this->pSelectWindow->MouseUp();
}
void CWindowThreadProcessIdUserInterface::OnMouseMove(WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    BOOL bWindowChanged=FALSE;
    this->pSelectWindow->MouseMove(lParam,&bWindowChanged);
    if (bWindowChanged)
    {
        TCHAR Tmp[260];
        _stprintf(Tmp,_T("0x%.8X"),this->pSelectWindow->WindowProcessID);
        this->SetDlgItemText(IDC_EDIT_WINDOW_PROCESS_ID,Tmp);

        _stprintf(Tmp,_T("0x%.8X"),this->pSelectWindow->WindowThreadID);
        this->SetDlgItemText(IDC_EDIT_WINDOW_THREAD_ID,Tmp);
    }
}