#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif

#include <windows.h>
//#include <objidl.h>
#include <objbase.h>
#pragma comment(lib,"ole32")

class CUnknownBase: public IUnknown
{
private:
    LONG RefCount; // reference count
public:
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppInterface );
};
