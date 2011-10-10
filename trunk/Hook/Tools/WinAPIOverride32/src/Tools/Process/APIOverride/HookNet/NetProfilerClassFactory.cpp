/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "netprofilerclassfactory.h"

CClassFactory::CClassFactory()
{
    this->RefCount=0;
}

CClassFactory::~CClassFactory()
{
}

ULONG STDMETHODCALLTYPE CClassFactory::AddRef()
{
    InterlockedIncrement(&this->RefCount);
    return this->RefCount;
}

ULONG STDMETHODCALLTYPE CClassFactory::Release()
{
    ULONG localRefCount;
    InterlockedDecrement(&this->RefCount);
    localRefCount=this->RefCount;
    if (this->RefCount==0)
        delete this;
    return localRefCount;
}

HRESULT STDMETHODCALLTYPE CClassFactory::QueryInterface(REFIID riid,void **ppInterface)
{    
    if (IsEqualIID(riid,IID_IUnknown))
        *ppInterface=static_cast<IUnknown*>(this);
    else if (IsEqualIID(riid,IID_IClassFactory))
        *ppInterface=static_cast<IClassFactory*>(this);
    else
    {
        *ppInterface=NULL;                                  
        return E_NOINTERFACE;
    }
    this->AddRef();
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CClassFactory::LockServer( BOOL fLock )
{
    UNREFERENCED_PARAMETER(fLock);
    return S_OK;
}
HRESULT STDMETHODCALLTYPE CClassFactory::CreateInstance( IUnknown *pUnkOuter,REFIID riid,void **ppInterface)
{
    if (IsBadWritePtr(ppInterface,sizeof(CNetProfiler*)))
        return E_INVALIDARG;

    // aggregation is not supported by these objects
    if (pUnkOuter!=NULL)
        return CLASS_E_NOAGGREGATION;

    CNetProfiler* pNetProfiler=new CNetProfiler();

    // let pNetProfiler do the job
    HRESULT hResult=pNetProfiler->QueryInterface(riid,ppInterface);
    if (hResult!=S_OK)
    {
        delete pNetProfiler;
        return hResult;
    }

    return S_OK;
}