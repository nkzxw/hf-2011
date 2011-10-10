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
// Object: winapioverride plugin interface
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "IWinApiOverride.h"

#define IWinApiOverridePlugin_GetPluginObject_FUNCTION_NAME    "_GetPluginObject@0"
#define IWinApiOverridePlugin_GetPluginObject_FUNCTION_NAMEW   L"_GetPluginObject@0"
#if (defined(UNICODE)||defined(_UNICODE))
    #define IWinApiOverridePlugin_GetPluginObject_FUNCTION_NAMET   IWinApiOverridePlugin_GetPluginObject_FUNCTION_NAMEW
#else
    #define IWinApiOverridePlugin_GetPluginObject_FUNCTION_NAMET   IWinApiOverridePlugin_GetPluginObject_FUNCTION_NAME
#endif

#define IWinApiOverridePlugin_MAJOR_VERSION 1
#define IWinApiOverridePlugin_MINOR_VERSION 2

class IWinApiOverridePlugin
{
public:
    enum ENCODING_TYPE
    {
        ENCODING_TYPE_ASCII,
        ENCODING_TYPE_UNICODE 
    };
    SIZE_T virtual STDMETHODCALLTYPE GetPluginFrameworkVersion()
    {
        return ( (IWinApiOverridePlugin_MAJOR_VERSION<<16) | (IWinApiOverridePlugin_MINOR_VERSION) );
    }
    ENCODING_TYPE virtual STDMETHODCALLTYPE GetPluginEncoding()
    {
#if (defined(UNICODE) || defined(_UNICODE))
        return ENCODING_TYPE_UNICODE;
#else
        return ENCODING_TYPE_ASCII;
#endif
    }
    virtual void STDMETHODCALLTYPE Initialize(HWND hWndDialog,IWinApiOverride* pWinApiOverride)
    {
        UNREFERENCED_PARAMETER(hWndDialog);
        UNREFERENCED_PARAMETER(pWinApiOverride);
    }
    virtual void STDMETHODCALLTYPE Destroy(){}

    // should return TRUE if MenuItemID belongs to your plugin menu
    virtual BOOL STDMETHODCALLTYPE OnPluginMenuClick(UINT MenuItemID)
    {
        UNREFERENCED_PARAMETER(MenuItemID);
        return FALSE;
    }
    virtual void STDMETHODCALLTYPE OnProcessHooked(IApiOverride* pApiOverride)
    {
        UNREFERENCED_PARAMETER(pApiOverride);
    }
    virtual void STDMETHODCALLTYPE OnProcessUnhooked(IApiOverride* pApiOverride)
    {
        UNREFERENCED_PARAMETER(pApiOverride);
    }
    virtual void STDMETHODCALLTYPE OnProcessUnload(IApiOverride* pApiOverride)
    {
        UNREFERENCED_PARAMETER(pApiOverride);
    }

    virtual BOOL STDMETHODCALLTYPE OnOverridingDllQuery(IApiOverride* pApiOverride,PVOID MessageId,PBYTE pMsg,SIZE_T MsgSize)
    {


        // pApiOverride : item from which request comes
        // MessageId : NULL if no reply is required
        UNREFERENCED_PARAMETER(pApiOverride);
        UNREFERENCED_PARAMETER(MessageId);
        UNREFERENCED_PARAMETER(pMsg);
        UNREFERENCED_PARAMETER(MsgSize);
        // pApiOverride->SendReplyToOverridingDllQuery(MessageId,pMsg,MsgSize);
        return FALSE;
    }
};

typedef IWinApiOverridePlugin* (__stdcall *pfGetPluginObject)();