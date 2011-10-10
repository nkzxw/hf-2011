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
// Object: winapioverride logs interface
//-----------------------------------------------------------------------------

#pragma once

class IWinApiOverrideLogs
{
public:
    typedef BOOL (STDMETHODCALLTYPE *pfForEachLogListEntryCallBack)(LOG_LIST_ENTRY* pLogEntry,PVOID UserParam);
    virtual void STDMETHODCALLTYPE Clear(BOOL bWaitEndOfClearing)=0;
    virtual BOOL STDMETHODCALLTYPE Load(TCHAR* LogName,BOOL bWaitEndOfLoading)=0;
    virtual BOOL STDMETHODCALLTYPE Save(TCHAR* LogName,BOOL bWaitEndOfSaving)=0;
    virtual BOOL STDMETHODCALLTYPE DisplayDetailsForLog(LOG_LIST_ENTRY* pLogEntry)=0;
    virtual void STDMETHODCALLTYPE ForEach(pfForEachLogListEntryCallBack CallBack,PVOID UserParam)=0;
};