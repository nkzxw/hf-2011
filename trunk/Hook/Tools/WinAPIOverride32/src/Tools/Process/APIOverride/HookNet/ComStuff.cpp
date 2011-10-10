#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <strsafe.h>
#include <olectl.h>
#include "NetProfilerClassFactory.h"

// #define CLSID_NET_PROFILER_GUID "{52AE91FC-569A-496f-A268-74D62B866D73}" in HookNetExport
extern const GUID CLSID_NET_PROFILER = 
{ 0x52AE91FC, 0x569A, 0x496f, { 0xA2, 0x68, 0x74, 0xD6, 0x2B, 0x86, 0x6D, 0x73 } };
typedef struct{
    HKEY  hRootKey;
    LPTSTR szSubKey;        // TCHAR szSubKey[MAX_PATH];
    LPTSTR lpszValueName;
    LPTSTR szData;          // TCHAR szData[MAX_PATH];
}DOREGSTRUCT, *LPDOREGSTRUCT;

STDAPI RegisterServer(CLSID clsid, LPTSTR lpszTitle);
STDAPI UnregisterServer(CLSID clsid, LPTSTR lpszTitle);


LONG volatile ObjectsCount=0;
extern HINSTANCE DllhInstance;

#pragma comment(linker, "/EXPORT:DllGetClassObject=_DllGetClassObject@12,PRIVATE")
STDAPI DllGetClassObject(REFCLSID rclsid,REFIID riid,LPVOID FAR *ppv)                  
{    
    HRESULT hResult;

    if (!IsEqualCLSID(rclsid,CLSID_NET_PROFILER))
        return CLASS_E_CLASSNOTAVAILABLE;

    if (IsBadWritePtr(ppv,sizeof(CClassFactory*)))
        return E_INVALIDARG;

    CClassFactory* pClassFactory=new CClassFactory();

    if (IsEqualIID(riid,IID_IClassFactory))
    {
        hResult=pClassFactory->QueryInterface(riid,ppv);
        if (hResult!=S_OK)
        {
            delete pClassFactory;
            return hResult;
        }
    }
    else
    {
        // let pClassFactory do the job
        hResult=pClassFactory->CreateInstance(NULL,riid,ppv);
        if (hResult!=S_OK)
        {
            delete pClassFactory;
            return hResult;
        }
        delete pClassFactory;
    }

    return S_OK;   
}

#pragma comment(linker, "/EXPORT:DllCanUnloadNow=_DllCanUnloadNow@0,PRIVATE")
STDAPI DllCanUnloadNow(void)
{
    return (ObjectsCount ? S_FALSE : S_OK);
}

#pragma comment(linker, "/EXPORT:DllRegisterServer=_DllRegisterServer@0,PRIVATE")
STDAPI DllRegisterServer()
{
    DllUnregisterServer();

    // Register CLSID_NET_PROFILER object.
    if(FAILED(RegisterServer(CLSID_NET_PROFILER, TEXT("Hook Net"))))
        return SELFREG_E_CLASS;

    return S_OK;
}

STDAPI RegisterServer(CLSID clsid, LPTSTR lpszTitle)
{
    int      i;
    HKEY     hKey;
    LRESULT  lResult;
    DWORD    dwDisp;
    TCHAR    szSubKey[MAX_PATH];
    TCHAR    szCLSID[MAX_PATH];
    TCHAR    szModule[MAX_PATH];
    LPWSTR   pwsz;
    DWORD    retval;
    HRESULT  hr;
    size_t   length;

    // Get the CLSID in string form.
    StringFromIID(clsid, &pwsz);

    if(pwsz)
    {
#ifdef UNICODE
        hr = StringCchCopyW(szCLSID, MAX_PATH, pwsz);
        // TODO: Add error handling for hr here.
#else
        WideCharToMultiByte( CP_ACP,
            0,
            pwsz,
            -1,
            szCLSID,
            MAX_PATH * sizeof(TCHAR),
            NULL,
            NULL);
#endif

        // Free the string.
        LPMALLOC pMalloc;
        CoGetMalloc(1, &pMalloc);
        pMalloc->Free(pwsz);
        pMalloc->Release();
    }

    // Get this app's path and file name.
    retval = GetModuleFileName(DllhInstance, szModule, MAX_PATH);
    // TODO: Add error handling to check return value for success/failure
    //       before using szModule.

    DOREGSTRUCT ClsidEntries[ ] = {
        {HKEY_CLASSES_ROOT,TEXT("CLSID\\%38s"),NULL,lpszTitle},
        {HKEY_CLASSES_ROOT,TEXT("CLSID\\%38s\\InprocServer32"),NULL,szModule},
        {HKEY_CLASSES_ROOT,TEXT("CLSID\\%38s\\InprocServer32"),TEXT("ThreadingModel"),TEXT("Apartment")},
        {NULL,NULL,NULL,NULL}
    };

    //register the CLSID entries
    for(i = 0; ClsidEntries[i].hRootKey; i++)
    {
        //create the sub key string - for this case, insert the file extension
        hr = StringCchPrintf(szSubKey, 
            MAX_PATH, 
            ClsidEntries[i].szSubKey, 
            szCLSID);
        // TODO: Add error handling code here to check the hr return value.

        lResult = RegCreateKeyEx(ClsidEntries[i].hRootKey,
            szSubKey,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_WRITE,
            NULL,
            &hKey,
            &dwDisp);

        if(NOERROR == lResult)
        {
            TCHAR szData[MAX_PATH];

            // If necessary, create the value string.
            hr = StringCchPrintf(szData, 
                MAX_PATH, 
                ClsidEntries[i].szData, 
                szModule);
            // TODO: Add error handling code here to check the hr return value.

            hr = StringCchLength(szData, MAX_PATH, &length);
            // TODO: Add error handling code here to check the hr return value.

            lResult = RegSetValueEx(hKey,
                ClsidEntries[i].lpszValueName,
                0,
                REG_SZ,
                (LPBYTE)szData,
                (DWORD)((length + 1) * sizeof(TCHAR)));

            RegCloseKey(hKey);
        }
        else
            return E_FAIL;
    }

    return S_OK;
}

#pragma comment(linker, "/EXPORT:DllUnregisterServer=_DllUnregisterServer@0,PRIVATE")
STDAPI DllUnregisterServer()
{    
    // Register the desk band object.
    if(FAILED(UnregisterServer(CLSID_NET_PROFILER, TEXT("Hook Net"))))
        return SELFREG_E_CLASS;

    return S_OK;   
}

STDAPI UnregisterServer(CLSID clsid, LPTSTR lpszTitle)
{
    int      i;
    LRESULT  lResult;
    TCHAR    szSubKey[MAX_PATH];
    TCHAR    szCLSID[MAX_PATH];
    TCHAR    szModule[MAX_PATH];
    LPWSTR   pwsz;
    DWORD    retval;
    HRESULT  hr;

    // Get the CLSID in string form.
    StringFromIID(clsid, &pwsz);

    if(pwsz)
    {
#ifdef UNICODE
        hr = StringCchCopyW(szCLSID, MAX_PATH, pwsz);
        // TODO: Add error handling for hr here.
#else
        WideCharToMultiByte( CP_ACP,
            0,
            pwsz,
            -1,
            szCLSID,
            MAX_PATH * sizeof(TCHAR),
            NULL,
            NULL);
#endif

        // Free the string.
        LPMALLOC pMalloc;
        CoGetMalloc(1, &pMalloc);
        pMalloc->Free(pwsz);
        pMalloc->Release();
    }

    // Get this app's path and file name.
    retval = GetModuleFileName(DllhInstance, szModule, MAX_PATH);
    // TODO: Add error handling to check return value for success/failure
    //       before using szModule.

    DOREGSTRUCT ClsidEntries[ ] = {
        {HKEY_CLASSES_ROOT,TEXT("CLSID\\%38s"),NULL,lpszTitle},
        {HKEY_CLASSES_ROOT,TEXT("CLSID\\%38s\\InprocServer32"),NULL,szModule},
        {HKEY_CLASSES_ROOT,TEXT("CLSID\\%38s\\InprocServer32"),TEXT("ThreadingModel"),TEXT("Apartment")},
        {NULL,NULL,NULL,NULL}
    };

    //register the CLSID entries
    for(i = 0; ClsidEntries[i].hRootKey; i++)
    {
        //create the sub key string - for this case, insert the file extension
        hr = StringCchPrintf(szSubKey, 
            MAX_PATH, 
            ClsidEntries[i].szSubKey, 
            szCLSID);
        // TODO: Add error handling code here to check the hr return value.

        lResult = RegDeleteKey(ClsidEntries[i].hRootKey,ClsidEntries[i].szSubKey);
    }
    return S_OK;  
}