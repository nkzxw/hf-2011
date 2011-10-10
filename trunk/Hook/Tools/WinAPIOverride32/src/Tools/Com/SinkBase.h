#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif

#include <windows.h>
#include <objbase.h>
#include <olectl.h>
#pragma comment(lib,"ole32")

typedef HRESULT (STDMETHODCALLTYPE *pInvokeCallBack)(DISPID dispidMember,
                                                   REFIID riid,
                                                   LCID lcid,
                                                   WORD wFlags,
                                                   DISPPARAMS* pdispparams,
                                                   VARIANT* pvarResult,
                                                   EXCEPINFO* pexcepinfo,
                                                   UINT* puArgErr,
                                                   PVOID UserParam
                                                   );

class CSinkBase: public IDispatch
{
public:
    CSinkBase(IUnknown* pIUnknown,IID riidConnectionPoint);
    ~CSinkBase(void);
private:
    LONG RefCount; // reference count
    IUnknown* pIUnknown;
    IID riidConnectionPoint;
    DWORD Cookie;
    BOOL Connected;
    IConnectionPointContainer* pConnectionPointContainer;
    IConnectionPoint* pConnectionPoint;
public:
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppInterface );
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(unsigned int FAR*  pctinfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(unsigned int  iTInfo,LCID  lcid,ITypeInfo FAR* FAR*  ppTInfo);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( REFIID  riid,                  
                                                    OLECHAR FAR* FAR*  rgszNames,  
                                                    unsigned int  cNames,          
                                                    LCID   lcid,                   
                                                    DISPID FAR*  rgDispId          
                                                    );
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember,
                                        REFIID riid,
                                        LCID lcid,
                                        WORD wFlags,
                                        DISPPARAMS* pdispparams,
                                        VARIANT* pvarResult,
                                        EXCEPINFO* pexcepinfo,
                                        UINT* puArgErr);
    HRESULT ConnectEvents();
    HRESULT DisconnectEvents();
};
