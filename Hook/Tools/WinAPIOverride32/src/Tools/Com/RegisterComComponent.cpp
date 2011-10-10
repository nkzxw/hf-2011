#include "registercomcomponent.h"

// TCHAR* ComponentPath : dll or ocx path
BOOL CRegisterComComponent::UnregisterComponent(TCHAR* ComponentPath)
{
    HRESULT hr;
    HMODULE hModule;
    FARPROC pDllUnregisterServer;

    hModule=LoadLibrary(ComponentPath);
    if (!hModule)
        return FALSE;

    // get DllRegisterServer function pointer
    pDllUnregisterServer=GetProcAddress(hModule,"DllRegisterServer");

    if (!pDllUnregisterServer)
        return FALSE;

    // call DllRegisterServer
    hr=pDllUnregisterServer();

    // free library
    FreeLibrary(hModule);

    return (SUCCEEDED(hr));
}

// TCHAR* ComponentPath : dll or ocx path
BOOL CRegisterComComponent::RegisterComponent(TCHAR* ComponentPath)
{
    HRESULT hr;
    HMODULE hModule;
    FARPROC pDllRegisterServer;

    hModule=LoadLibrary(ComponentPath);
    if (!hModule)
        return FALSE;

    // get DllRegisterServer function pointer
    pDllRegisterServer=GetProcAddress(hModule,"DllRegisterServer");

    if (!pDllRegisterServer)
        return FALSE;

    // call DllRegisterServer
    hr=pDllRegisterServer();

    // free library
    FreeLibrary(hModule);

    return (SUCCEEDED(hr));
}

// IN TCHAR* ComponentPath : dll or ocx path
// IN REFCLSID pClsidToCheck : CLSID to check
// OUT BOOL* pbComponentAlreadyRegistered : FALSE if component wasn't registered, TRUE if component was already registered
BOOL CRegisterComComponent::RegisterComponentIfNeeded(IN TCHAR* ComponentPath,IN REFCLSID ClsidToCheck,OUT BOOL* pbComponentAlreadyRegistered)
{
    BOOL bRetValue;
    BOOL bComInitialized;
    IUnknown* pIUnknown = NULL;
    bComInitialized = SUCCEEDED(CoInitialize(NULL));
    HRESULT hr = CoCreateInstance(ClsidToCheck,
                                NULL, 
                                CLSCTX_ALL,
                                __uuidof(IUnknown),
                                (void **) &pIUnknown);
    if(FAILED(hr) || (pIUnknown==0))
    {
        *pbComponentAlreadyRegistered=FALSE;
        bRetValue = CRegisterComComponent::RegisterComponent(ComponentPath);
    }
    else
    {
        pIUnknown->Release();
        *pbComponentAlreadyRegistered=TRUE;
        bRetValue =  TRUE;
    }
    if (bComInitialized)
        CoUninitialize();

    return bRetValue;
}