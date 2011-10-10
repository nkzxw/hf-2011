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

#pragma once

#include "../../../../Com/SinkBase.h"
#include <exdisp.h>
#include <exdispid.h>
class CBrowserEvents: public CSinkBase
{
private:
    pInvokeCallBack InvokeCallBack;
    PVOID UserParam;
public:
    CBrowserEvents(IWebBrowser2* pBrowser);
    ~CBrowserEvents(void);
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember,
                                                REFIID riid,
                                                LCID lcid,
                                                WORD wFlags,
                                                DISPPARAMS* pdispparams,
                                                VARIANT* pvarResult,
                                                EXCEPINFO* pexcepinfo,
                                                UINT* puArgErr);

    HRESULT CBrowserEvents::SetCallBack(pInvokeCallBack InvokeCallBack,PVOID UserParam);
};
