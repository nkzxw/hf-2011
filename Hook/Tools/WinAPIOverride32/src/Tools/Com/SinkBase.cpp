#include "SinkBase.h"


CSinkBase::CSinkBase(IUnknown* pIUnknown,IID riidConnectionPoint)
{
    // store var for connection
    this->pIUnknown=pIUnknown;
    if (this->pIUnknown)
        this->pIUnknown->AddRef();
    memcpy(&this->riidConnectionPoint,&riidConnectionPoint,sizeof(IID));

    this->Cookie=0;
    this->Connected=FALSE;
    this->pConnectionPointContainer = NULL;
    this->pConnectionPoint = NULL;
}
CSinkBase::~CSinkBase(void)
{
    if (this->pIUnknown)
        this->pIUnknown->Release();

    // if connected, disconnect
    if (this->Connected)
        this->DisconnectEvents();
}

ULONG STDMETHODCALLTYPE CSinkBase::AddRef()
{
    InterlockedIncrement(&this->RefCount);
    return this->RefCount;
}

ULONG STDMETHODCALLTYPE CSinkBase::Release()
{
    ULONG localRefCount;
    InterlockedDecrement(&this->RefCount);
    localRefCount=this->RefCount;
    if (this->RefCount==0)
        delete this;
    return localRefCount;
}

HRESULT STDMETHODCALLTYPE CSinkBase::QueryInterface( REFIID riid, void **ppInterface)
{
    if(IsEqualIID(riid,IID_IUnknown))
        *ppInterface = static_cast<IUnknown*>(this);
    else if(IsEqualIID(riid,IID_IDispatch))
        *ppInterface = static_cast<IDispatch*>(this);
    else
    {
        *ppInterface=NULL;
        return E_NOINTERFACE;
    }
    this->AddRef();
    return S_OK;

}

// http://support.microsoft.com/kb/181277/ :
// "A sink uses only one of the IDispatch methods, Invoke. The other three IDispatch methods, GetTypeInfoCount, GetTypeInfo, and GetIDsOfNames just returns E_NOTIMPL"
HRESULT STDMETHODCALLTYPE CSinkBase::GetTypeInfoCount(unsigned int FAR*  pctinfo)
{
    UNREFERENCED_PARAMETER(pctinfo);
    return E_NOTIMPL;
}
HRESULT STDMETHODCALLTYPE CSinkBase::GetTypeInfo(unsigned int  iTInfo,LCID  lcid,ITypeInfo FAR* FAR*  ppTInfo)
{
    UNREFERENCED_PARAMETER(iTInfo);
    UNREFERENCED_PARAMETER(lcid);
    UNREFERENCED_PARAMETER(ppTInfo);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSinkBase::GetIDsOfNames(  
                                        REFIID  riid,                  
                                        OLECHAR FAR* FAR*  rgszNames,  
                                        unsigned int  cNames,          
                                        LCID   lcid,                   
                                        DISPID FAR*  rgDispId          
                                        )
{
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(rgszNames);
    UNREFERENCED_PARAMETER(cNames);
    UNREFERENCED_PARAMETER(lcid);
    UNREFERENCED_PARAMETER(rgDispId);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSinkBase::Invoke(DISPID dispidMember,
                                    REFIID riid,
                                    LCID lcid,
                                    WORD wFlags,
                                    DISPPARAMS* pdispparams,
                                    VARIANT* pvarResult,
                                    EXCEPINFO* pexcepinfo,
                                    UINT* puArgErr)
{
    UNREFERENCED_PARAMETER(dispidMember);
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(lcid);
    UNREFERENCED_PARAMETER(wFlags);
    UNREFERENCED_PARAMETER(pdispparams);
    UNREFERENCED_PARAMETER(pvarResult);
    UNREFERENCED_PARAMETER(pexcepinfo);
    UNREFERENCED_PARAMETER(puArgErr);

    return S_OK;
}

HRESULT CSinkBase::ConnectEvents()
{
    HRESULT hr;

    if (this->pIUnknown==NULL)
        return E_FAIL;

    if (this->Connected)
        return S_OK;

    // Check that this is a connectable object.
    hr = this->pIUnknown->QueryInterface(IID_IConnectionPointContainer, (void**)&this->pConnectionPointContainer);

    if (FAILED(hr) || (this->pConnectionPointContainer==NULL))
        return hr;

    // Find the connection point.
    hr = this->pConnectionPointContainer->FindConnectionPoint(this->riidConnectionPoint, &this->pConnectionPoint);

    if (FAILED(hr) || (this->pConnectionPoint==NULL))
    {
        this->pConnectionPointContainer->Release();
        return hr;
    }

    // Advise the connection point.
    hr = pConnectionPoint->Advise(this, &this->Cookie);

    if (FAILED(hr))
    {
        this->pConnectionPointContainer->Release();
        this->pConnectionPoint->Release();
        return hr;
    }

    this->Connected=TRUE;

    return hr;
} 

HRESULT CSinkBase::DisconnectEvents()
{
    HRESULT hr;

    if (this->pIUnknown==NULL)
        return E_FAIL;

    if (!this->Connected)
        return S_OK;

    // Unadvise the connection point.
    hr = pConnectionPoint->Unadvise(Cookie);
    if (FAILED(hr))
        return hr;

    pConnectionPoint->Release();
    pConnectionPointContainer->Release();

    this->Connected=FALSE;

    return hr;
}
