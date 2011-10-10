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

#include "ElementEvents.h"

CElementEvents::CElementEvents(IHTMLElement* pElement):CSinkBase((IUnknown*)pElement,DIID_HTMLElementEvents2)
{
    this->ElementEventsCallBack=NULL;
    this->ElementEventsCallBackUserParam=NULL;
    this->ElementEventsCallBackEx=NULL;
    this->ElementEventsCallBackExUserParam=NULL;
    this->bEnableContextMenu=FALSE;
    this->bEnableSelection=FALSE;
    this->pIDispatchDoc=NULL;
    this->pCmdTarget=NULL;
    this->hCursorArrow=::LoadCursor(NULL, IDC_ARROW);
    this->hCursorOld=this->hCursorArrow;
    pElement->get_document(&this->pIDispatchDoc);
    this->pIDispatchDoc->QueryInterface(IID_IOleCommandTarget, (void**)&this->pCmdTarget);

}

CElementEvents::~CElementEvents(void)
{
    if (this->pCmdTarget)
        this->pCmdTarget->Release();

    if (this->pIDispatchDoc)
        this->pIDispatchDoc->Release();
}

HRESULT STDMETHODCALLTYPE CElementEvents::Invoke(DISPID dispidMember,
                                                REFIID riid,
                                                LCID lcid,
                                                WORD wFlags,
                                                DISPPARAMS* pdispparams,
                                                VARIANT* pvarResult,
                                                EXCEPINFO* pexcepinfo,
                                                UINT* puArgErr)
{
    UNREFERENCED_PARAMETER(puArgErr);
    UNREFERENCED_PARAMETER(pexcepinfo);
    UNREFERENCED_PARAMETER(pvarResult);
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(lcid);
    UNREFERENCED_PARAMETER(wFlags);
    // try to get IHTMLEventObj
    IDispatch* param1=NULL; 
    if (pdispparams->cArgs==1 && (pdispparams->rgvarg[0].vt==VT_DISPATCH))
        param1=pdispparams->rgvarg[0].pdispVal;

    IHTMLEventObj* pEvtObj=NULL;
    if (param1) 
        param1->QueryInterface(IID_IHTMLEventObj, (void**)&pEvtObj);


    // see HTMLElementEvents2 Dispinterface for available values (include mshtmdid.h)

    // disable context menu by default (but for INPUT)
    switch (dispidMember)
    {
    case DISPID_HTMLELEMENTEVENTS2_ONCONTEXTMENU:
        if (this->bEnableContextMenu)
            break;
        // else
        {
            IHTMLElement* pHTMLElement;
            BSTR String;
            // get element source of event
            HRESULT hr=pEvtObj->get_srcElement(&pHTMLElement);
            if (SUCCEEDED(hr) && pHTMLElement)
            {
                // get element name
                pHTMLElement->get_tagName(&String);
                if (String==NULL)
                    String=SysAllocString(L"");

                // apply for all but input
                if (wcsicmp(String,L"INPUT")!=0)
                {
                    VARIANT v;
                    v.vt=VT_BOOL;
                    v.boolVal=VARIANT_FALSE;
                    pEvtObj->put_returnValue(v);
                }

                // Free memory and release objects
                SysFreeString(String);
                pHTMLElement->Release();
            }
        }
        break;

    // disable selection but for input
    case DISPID_HTMLELEMENTEVENTS2_ONSELECTSTART:
        if (this->bEnableSelection)
            break;
        // else
        {
            IHTMLElement* pHTMLElement;
            BSTR String;
            // get element source of event
            HRESULT hr=pEvtObj->get_srcElement(&pHTMLElement);
            if (SUCCEEDED(hr) && pHTMLElement)
            {
                // get element name
                pHTMLElement->get_tagName(&String);
                if (String==NULL)
                    String=SysAllocString(L"");

                // apply for all but input
                if (wcsicmp(String,L"INPUT")!=0)
                {
                    VARIANT v;
                    v.vt=VT_BOOL;
                    v.boolVal=VARIANT_FALSE;
                    pEvtObj->put_returnValue(v);
                }

                // Free memory and release objects
                SysFreeString(String);
                pHTMLElement->Release();
            }
        }
        break;
    // disable selection but for input
    case DISPID_HTMLELEMENTEVENTS2_ONMOUSEOVER:
        if (this->bEnableSelection || (this->pCmdTarget==NULL))
            break;
        // else
        {
            VARIANT var;
            var.vt=VT_BOOL;
            IHTMLElement* pHTMLElement;
            BSTR String;

            // get element source of event
            HRESULT hr=pEvtObj->get_srcElement(&pHTMLElement);
            if (SUCCEEDED(hr) && pHTMLElement)
            {
                // get element name
                pHTMLElement->get_tagName(&String);
                if (String==NULL)
                    String=SysAllocString(L"");

                // apply for all but input, link and img
                if ( (wcsicmp(String,L"INPUT")!=0)
                    && (wcsicmp(String,L"A")!=0)
                    && (wcsicmp(String,L"IMG")!=0)
                    )
                {
                    // stop cursor automatic mode
                    var.boolVal = VARIANT_TRUE;
                    this->pCmdTarget->Exec(&CGID_MSHTML, 
                                            IDM_OVERRIDE_CURSOR,
                                            OLECMDEXECOPT_DODEFAULT,
                                            &var,
                                            NULL);
                    // Set cursor to arrow
                    this->hCursorOld=::SetCursor(this->hCursorArrow);
                }

                // Free memory and release objects
                SysFreeString(String);
                pHTMLElement->Release();
            }
        }
        break;
    case DISPID_HTMLELEMENTEVENTS2_ONMOUSEOUT:
        if (this->bEnableSelection || (this->pCmdTarget==NULL))
            break;
        // else
        {
            VARIANT var;
            var.vt=VT_BOOL;
            IHTMLElement* pHTMLElement;
            BSTR String;

            // get element source of event
            HRESULT hr=pEvtObj->get_srcElement(&pHTMLElement);
            if (SUCCEEDED(hr) && pHTMLElement)
            {
                // get element name
                pHTMLElement->get_tagName(&String);
                if (String==NULL)
                    String=SysAllocString(L"");

                // apply for all but input, link and img
                if ( (wcsicmp(String,L"INPUT")!=0)
                    && (wcsicmp(String,L"A")!=0)
                    && (wcsicmp(String,L"IMG")!=0)
                    )
                {
                    // restore cursor automatic mode
                    var.boolVal = VARIANT_FALSE;
                    ::SetCursor(this->hCursorOld);
                    this->pCmdTarget->Exec(&CGID_MSHTML, 
                                            IDM_OVERRIDE_CURSOR, 
                                            OLECMDEXECOPT_DODEFAULT, 
                                            &var, 
                                            NULL);
                }
                // Free memory and release objects
                SysFreeString(String);
                pHTMLElement->Release();
            }
        }
        break;
    case DISPID_HTMLELEMENTEVENTS2_ONDRAGSTART:
        if (this->bEnableSelection)
            break;
        {
            VARIANT v;
            v.vt=VT_BOOL;
            v.boolVal=VARIANT_FALSE;
            pEvtObj->put_returnValue(v);
        }
        break;
    }


    // call callback
    if (this->ElementEventsCallBack)
    {
        IHTMLElement* pHTMLElement;
        BSTR String;
        // get element source of event
        HRESULT hr=pEvtObj->get_srcElement(&pHTMLElement);
        if (SUCCEEDED(hr) && pHTMLElement)
        {
            // get element id
            pHTMLElement->get_id(&String);
            if (String==NULL)
                String=SysAllocString(L"");

            // call callback
            this->ElementEventsCallBack(dispidMember,String,this->ElementEventsCallBackUserParam);

            // Free memory and release objects
            SysFreeString(String);
            pHTMLElement->Release();
        }
    }

    if (this->ElementEventsCallBackEx)
    {
        this->ElementEventsCallBackEx(dispidMember,pEvtObj,this->ElementEventsCallBackExUserParam);
    }

    // release IHTMLEventObj
    if (pEvtObj)
        pEvtObj->Release();

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: SetElementsEventsCallBack
// Object: set simple document events callback
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CElementEvents::SetElementEventsCallBack(pfElementEventsCallBack ElementEventsCallBack,LPVOID UserParam)
{
    if (IsBadCodePtr((FARPROC)ElementEventsCallBack) && (ElementEventsCallBack!=0))
        return FALSE;

    this->ElementEventsCallBack=ElementEventsCallBack;
    this->ElementEventsCallBackUserParam=UserParam;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetElementEventsCallBackEx
// Object: set extended document events callback
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CElementEvents::SetElementEventsCallBackEx(pfElementEventsCallBackEx ElementEventsCallBackEx,LPVOID UserParam)
{
    if (IsBadCodePtr((FARPROC)ElementEventsCallBackEx) && (ElementEventsCallBackEx!=0))
        return FALSE;

    this->ElementEventsCallBackEx=ElementEventsCallBackEx;
    this->ElementEventsCallBackExUserParam=UserParam;

    return TRUE;
}

BOOL CElementEvents::EnableContextMenu(BOOL Enable)
{
    this->bEnableContextMenu=Enable;
    return TRUE;
}
BOOL CElementEvents::EnableSelection(BOOL Enable)
{
    this->bEnableSelection=Enable;
    return TRUE;
}