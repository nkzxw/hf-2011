#include "HasDebugInfos.h"


BOOL CHasDebugInfos::HasDebugInfos(TCHAR* FileName)
{
    BOOL bRet=FALSE;
    
    TCHAR DiaDllPath[MAX_PATH];
    IDiaDataSource* pDiaDataSource;
    IDiaSession* pDiaSession;
    IDiaSymbol* pDiaGlobalSymbol;
    pDiaGlobalSymbol=NULL;
    pDiaSession=NULL;
    pDiaDataSource=NULL;
    *DiaDllPath=0;
    BOOL bComInitialized=SUCCEEDED(CoInitialize(NULL));


    HRESULT hr;
    wchar_t wszFileName[MAX_PATH];
    wchar_t* wszExt;
    wchar_t* wszSearchPath = L"SRV**\\\\symbols\\symbols"; // Alternate path to search for debug data

    // Obtain Access To The Provider
    hr = CoCreateInstance(__uuidof(DiaSource),//CLSID_DiaSource, 
        NULL, 
        CLSCTX_INPROC_SERVER,
        __uuidof(IDiaDataSource),
        (void **) &pDiaDataSource);
    if(FAILED(hr) || (pDiaDataSource==0))
    {
        CStdFileOperations::GetAppPath(DiaDllPath,MAX_PATH);
        _tcscat(DiaDllPath,DIA_DLL_NAME);

        // assume com dia dll has been registered
        if (!CRegisterComComponent::RegisterComponent(DiaDllPath))
        {
            TCHAR psz[2*MAX_PATH];
            _stprintf(psz,_T("Can't find %s"),DIA_DLL_NAME);
            MessageBox(NULL,psz,_T("Error"),MB_OK|MB_TOPMOST|MB_ICONERROR);
            *DiaDllPath=0;
            goto CleanUp;
        }

        // try again after having registered dll
        hr = CoCreateInstance(__uuidof(DiaSource),//CLSID_DiaSource, 
            NULL, 
            CLSCTX_INPROC_SERVER,
            __uuidof(IDiaDataSource),
            (void **) &pDiaDataSource);
        if(FAILED(hr) || (pDiaDataSource==0))
            goto CleanUp;
    }
#if (defined(UNICODE)||defined(_UNICODE))
    _tcscpy(wszFileName,FileName);
#else
    CAnsiUnicodeConvert::AnsiToUnicode(FileName,wszFileName,MAX_PATH);
#endif
    wszExt=wcsrchr(wszFileName,'.');
    if (!wszExt)
        wszExt=wszFileName;

    // if a .pdb opening was queried
    if(!_wcsicmp(wszExt,L".pdb"))
    {
        // Open and prepare a program database (.pdb) file as a debug data source
        hr = pDiaDataSource->loadDataFromPdb(wszFileName);
        if(FAILED(hr))
            goto CleanUp;
    }
    else
    {
        CCallback callback; // Receives callbacks from the DIA symbol locating procedure,
        // thus enabling a user interface to report on the progress of 
        // the location attempt. The client application may optionally 
        // provide a reference to its own implementation of this 
        // virtual base class to the IDiaDataSource::loadDataForExe method.
        callback.AddRef();
        // Open and prepare the debug data associated with the .exe/.dll file
        hr = pDiaDataSource->loadDataForExe(wszFileName,wszSearchPath,&callback);
        if(FAILED(hr))
            goto CleanUp;
    }
    // Open a session for querying symbols
    hr = pDiaDataSource->openSession(&pDiaSession);
    if(FAILED(hr) || (pDiaSession==0))
        goto CleanUp;

    // Retrieve a reference to the global scope
    hr = pDiaSession->get_globalScope(&pDiaGlobalSymbol);
    if(FAILED(hr) || (pDiaGlobalSymbol==0))
        goto CleanUp;


    bRet=TRUE;

CleanUp:

    if(pDiaGlobalSymbol)
        pDiaGlobalSymbol->Release();

    if(pDiaSession)
        pDiaSession->Release();

    if (bComInitialized)
        CoUninitialize();
    // if com dia has been registered by this app, unregister it
    if (*DiaDllPath)
        CRegisterComComponent::UnregisterComponent(DiaDllPath);

    return bRet;
}
