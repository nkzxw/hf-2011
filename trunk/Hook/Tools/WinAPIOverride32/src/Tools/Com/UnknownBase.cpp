#include "UnknownBase.h"

ULONG STDMETHODCALLTYPE CUnknownBase::AddRef()
{
    InterlockedIncrement(&this->RefCount);
    return this->RefCount;
}

ULONG STDMETHODCALLTYPE CUnknownBase::Release()
{
    ULONG localRefCount;
    InterlockedDecrement(&this->RefCount);
    localRefCount=this->RefCount;
    if (this->RefCount==0)
        delete this;
    return localRefCount;
}

HRESULT STDMETHODCALLTYPE CUnknownBase::QueryInterface( REFIID riid, void **ppInterface )
{
    if(IsEqualIID(riid,IID_IUnknown))
        *ppInterface = static_cast<IUnknown*>(this);
    else
    {
        *ppInterface=NULL;
        return E_NOINTERFACE;
    }
    this->AddRef();
    return S_OK;
}