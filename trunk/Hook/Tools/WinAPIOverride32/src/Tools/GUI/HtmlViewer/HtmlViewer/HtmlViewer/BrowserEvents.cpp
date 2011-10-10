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

//-----------------------------------------------------------------------------
// Object: allow to display html content
//-----------------------------------------------------------------------------

#include "BrowserEvents.h"

CBrowserEvents::CBrowserEvents(IWebBrowser2* pBrowser):CSinkBase((IUnknown*)pBrowser,DIID_DWebBrowserEvents2)
{
    this->InvokeCallBack=0;
    this->UserParam=0;
}

CBrowserEvents::~CBrowserEvents(void)
{
}

HRESULT CBrowserEvents::SetCallBack(pInvokeCallBack InvokeCallBack,PVOID UserParam)
{
    if ((InvokeCallBack!=0) && IsBadCodePtr((FARPROC)InvokeCallBack))
    {
        this->InvokeCallBack=0;
        return E_INVALIDARG;
    }
    this->InvokeCallBack=InvokeCallBack;
    this->UserParam=UserParam;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBrowserEvents::Invoke(DISPID dispidMember,
                                                REFIID riid,
                                                LCID lcid,
                                                WORD wFlags,
                                                DISPPARAMS* pdispparams,
                                                VARIANT* pvarResult,
                                                EXCEPINFO* pexcepinfo,
                                                UINT* puArgErr)
{
/* not needed
    // try to get IWebBrowser2 object
    IDispatch* param1=NULL; 
    if (pdispparams->cArgs==1 && (pdispparams->rgvarg[0].vt==VT_DISPATCH))
        param1=pdispparams->rgvarg[0].pdispVal;

    IWebBrowser2* pObj=NULL;
    if (param1) 
        param1->QueryInterface(IID_IWebBrowser2, (void**)&pObj);
*/
    if (this->InvokeCallBack)
    {
        this->InvokeCallBack(
                            dispidMember,
                            riid,
                            lcid,
                            wFlags,
                            pdispparams,
                            pvarResult,
                            pexcepinfo,
                            puArgErr,
                            this->UserParam
                            );
    }

    return S_OK;
}