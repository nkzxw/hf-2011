#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "../LinkList/SingleThreaded/LinkListSimpleSingleThreaded.h"

class CPeManifestAssemblyIdentity
{
public:
    CPeManifestAssemblyIdentity();
    virtual ~CPeManifestAssemblyIdentity();

    BOOL __fastcall Parse(TCHAR* const Content);

    const TCHAR* GetType();
    const TCHAR* GetName();
    const TCHAR* GetVersion();
    const TCHAR* GetProcessorArchitecture();
    const TCHAR* GetPublicKeyToken();
    const TCHAR* GetPublicLanguage();
    const TCHAR* GetDependencyFullPathWithBackSlash();
    
protected:
    FORCEINLINE void DeleteIfNotNull(TCHAR* psz)
    {
        if(psz)
            delete [] psz;
    }
    FORCEINLINE const TCHAR* GetSecureContent(TCHAR* const Content)
    {
        if (Content)
            return Content;
        else
            return _T("");
    }
    TCHAR* Type;
    TCHAR* Name;
    TCHAR* Version;
    TCHAR* Architecture;
    TCHAR* Token;
    TCHAR* Language;
    TCHAR DependencyFullPathWithBackSlash[MAX_PATH];

    TCHAR* __fastcall ExtractField(TCHAR* const StrIdentityContent,TCHAR* const FieldName);
};

class CPeManifestAssembly
{
public:
    CPeManifestAssembly();
    virtual ~CPeManifestAssembly();

    BOOL Parse(TCHAR* FileName);
    SIZE_T GetDependenciesCount();
    CPeManifestAssemblyIdentity* GetFirstDependency();
    CPeManifestAssemblyIdentity* GetNextDependency();

protected:
    CLinkListItem* pCurrentEnumerationItem;
    CLinkListSimpleSingleThreaded* pDependenciesList;// list of CPeManifestAssemblyIdentity*
    void FreeMemory();
    void AddDependency(CPeManifestAssemblyIdentity* pAssemblyIdentity);
    static BOOL CALLBACK EnumResNameProc(HMODULE hModule,LPCTSTR lpszType,LPTSTR lpszName,LONG_PTR lParam);
};

