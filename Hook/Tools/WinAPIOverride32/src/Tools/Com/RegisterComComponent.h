#pragma once
#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CRegisterComComponent
{
public:
    static BOOL UnregisterComponent(TCHAR* ComponentPath);
    static BOOL RegisterComponent(TCHAR* ComponentPath);
    static BOOL RegisterComponentIfNeeded(TCHAR* ComponentPath,IN REFCLSID ClsidToCheck,OUT BOOL* pbComponentAlreadyRegistered);
};
