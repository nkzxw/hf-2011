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
// Object: winapioverride com options
//-----------------------------------------------------------------------------

#pragma once

enum tagCOMCLSIDFilterTypes
{
    COM_CLSID_FilterHookOnlySpecified,
    COM_CLSID_FilterDontHookSpecified
};

///////////////////////////////////////////
// exported HookCom dll structures
///////////////////////////////////////////
typedef struct tagHookComOptions  
{
    BOOL IDispatchAutoMonitoring; // true is hook auto install them from IDispatch parsing
    BOOL QueryMethodToHookForInterfaceParsedByIDispatch;// show dialog if no COM monitoring file is associated with interface
                                                        // allowing user to select methods to hook
    BOOL ReportCLSIDNotSupportingIDispatch;// TRUE to report CLSID not supporting IDispatch interface
    BOOL ReportIIDHavingNoMonitoringFileAssociated; // TRUE to report IID queried for which we haven't definition file
    BOOL ReportHookedCOMObject;// TRUE to report life of COM object (creation and destruction)
    BOOL ReportUseNameInsteadOfIDIfPossible; // TRUE to use of PRogId instead of CLSID for reports
    TCHAR pszConfigFileComObjectCreationHookedFunctions[MAX_PATH];// config file defining COM object creation hooked functions
    tagCOMCLSIDFilterTypes CLSIDFilterType;
    TCHAR pszOnlyHookedFileName[MAX_PATH];
    TCHAR pszNotHookedFileName[MAX_PATH];
    BOOL  bUseClsidFilter;
    // new fields must be added at the end for plug in compatibility
}HOOK_COM_OPTIONS,*PHOOK_COM_OPTIONS;