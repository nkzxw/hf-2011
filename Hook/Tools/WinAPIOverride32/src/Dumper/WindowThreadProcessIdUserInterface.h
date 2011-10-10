#pragma once
#include "..\tools\gui\dialog\dialogbase.h"
#include "..\tools\gui\SelectWindow\SelectWindow.h"

class CWindowThreadProcessIdUserInterface :
    public CDialogBase
{
private:
    CSelectWindow* pSelectWindow;
public:
    CWindowThreadProcessIdUserInterface(void);
    virtual ~CWindowThreadProcessIdUserInterface(void);
    virtual void OnInit();
    virtual void OnClose();
    virtual void OnMouseDown(WPARAM wParam,LPARAM lParam);
    virtual void OnMouseUp(WPARAM wParam,LPARAM lParam);
    virtual void OnMouseMove(WPARAM wParam,LPARAM lParam);
};
