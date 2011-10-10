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
#include <Mshtml.h>
#include <Mshtmdid.h>
#include <MsHtmHst.h> // CGID_MSHTML
#include <mshtmcid.h> // IDM_OVERRIDE_CURSOR

typedef void (__stdcall *pfElementEventsCallBack)(DISPID dispidMember,WCHAR* ElementId,LPVOID UserParam);
typedef void (__stdcall *pfElementEventsCallBackEx)(DISPID dispidMember,IHTMLEventObj* pEvtObj,LPVOID UserParam);

class CElementEvents: public CSinkBase
{
private:
    pfElementEventsCallBack ElementEventsCallBack;
    LPVOID ElementEventsCallBackUserParam;
    pfElementEventsCallBackEx ElementEventsCallBackEx;
    LPVOID ElementEventsCallBackExUserParam;
    BOOL bEnableContextMenu;
    BOOL bEnableSelection;
    HCURSOR hCursorOld;
    HCURSOR hCursorArrow;
    IOleCommandTarget* pCmdTarget;
    IDispatch* pIDispatchDoc;
public:
    CElementEvents(IHTMLElement* pElement);
    ~CElementEvents(void);
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember,
                                                REFIID riid,
                                                LCID lcid,
                                                WORD wFlags,
                                                DISPPARAMS* pdispparams,
                                                VARIANT* pvarResult,
                                                EXCEPINFO* pexcepinfo,
                                                UINT* puArgErr);
    BOOL SetElementEventsCallBack(pfElementEventsCallBack ElementEventsCallBack,LPVOID UserParam);
    BOOL SetElementEventsCallBackEx(pfElementEventsCallBackEx ElementEventsCallBackEx,LPVOID UserParam);
    BOOL EnableContextMenu(BOOL Enable);
    BOOL EnableSelection(BOOL Enable);
};
