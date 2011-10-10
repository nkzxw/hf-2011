#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "resource.h"
#include "../Tools/gui/Dialog/DialogBase.h"
#include "../Tools/gui/ListView/Listview.h"
#include "../Tools/Dll/DllStub.h"
#include "../Tools/File/StdFileOperations.h"

class CStubResolverGUI:public CDialogBase
{
public:
    CStubResolverGUI();
    ~CStubResolverGUI();
protected:
    CDllStub* pDllStub;
    CListview* pListView;

    virtual void OnInit();
    virtual void OnClose();
    virtual void OnCommand(WPARAM wParam,LPARAM lParam);
    virtual void OnNotify(WPARAM wParam,LPARAM lParam);
    virtual void OnDropFiles(WPARAM wParam,LPARAM lParam);

    void OnResolve();
    void OnBrowse(int EditId,TCHAR* Filter);
};