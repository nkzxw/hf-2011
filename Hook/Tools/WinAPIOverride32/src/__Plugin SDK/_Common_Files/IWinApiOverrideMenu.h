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
// Object: winapioverride menu interface
//-----------------------------------------------------------------------------

#pragma once

class IWinApiOverrideMenu
{
public:
    virtual int STDMETHODCALLTYPE GetItemCount()=0;
    virtual UINT STDMETHODCALLTYPE AddMenuItem(TCHAR* Name,HICON hIcon)=0;
    virtual UINT STDMETHODCALLTYPE AddMenuItem(TCHAR* Name,HICON hIcon,UINT Index)=0; // returns item ID
    virtual UINT STDMETHODCALLTYPE AddSubMenuItem(TCHAR* SubMenuName,IWinApiOverrideMenu* SubMenu,HICON hIcon)=0;// returns item ID
    virtual UINT STDMETHODCALLTYPE AddSubMenuItem(TCHAR* SubMenuName,IWinApiOverrideMenu* SubMenu,HICON hIcon,UINT Index)=0;// returns item ID
    virtual UINT STDMETHODCALLTYPE AddSeparatorItem()=0;// returns item ID
    virtual UINT STDMETHODCALLTYPE AddSeparatorItem(UINT Index)=0;// returns item ID

    virtual void STDMETHODCALLTYPE SetCheckedState(UINT MenuID,BOOL bChecked)=0;
    virtual BOOL STDMETHODCALLTYPE IsChecked(UINT MenuID)=0;
    virtual void STDMETHODCALLTYPE SetEnabledState(UINT MenuID,BOOL bEnabled)=0;
    virtual BOOL STDMETHODCALLTYPE IsEnabled(UINT MenuID)=0;
    virtual BOOL STDMETHODCALLTYPE SetText(UINT MenuID,TCHAR* pszText)=0;
    virtual BOOL STDMETHODCALLTYPE SetIcon(UINT MenuID,HICON hIcon)=0;
    virtual void STDMETHODCALLTYPE Remove(UINT MenuID)=0;
};