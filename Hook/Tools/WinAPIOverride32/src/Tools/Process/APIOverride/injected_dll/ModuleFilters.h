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
// Object: manages the modules filters
//-----------------------------------------------------------------------------


#pragma once
#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <tlhelp32.h>
#include "defines.h"
#include "../../../linklist/linklist.h"
#include "../InterProcessCommunication.h"
#include "../../MailSlot/MailSlotClient.h"
#include "../../../String/WildCharCompare.h"
#include "NET_Manager.h"

#define MODULES_FILTERS_MAX_WAIT_IN_MS 2000
#define MAX_MODULES_NUMBERS 1024

class CModulesFilters
{
private:

    typedef struct tagMODULES_INFOS
    {
        MODULEENTRY32 Infos;
        BOOL        bLog;           // specify if module calls must be logged
    }MODULES_INFOS,*PMODULES_INFOS;

    CLinkList* pLinkListLoggedModulesFilters;// list containing the name and filter of a module
                                            // used only when refreshing modules list, else to earn speed,
                                            // logged state is stored into ModulesInfosArray
                                            //  calling RefreshModuleList will update ModulesInfosArray from pLinkListNotLoggedModules
    MODULES_INFOS ModulesInfosArray[MAX_MODULES_NUMBERS];// LET IT STATIC AND DON'T USE CLinkList
                                                         // else you'll get synchro troubles and deadlock depending hooked funcs
    DWORD ModulesInfosArraySize;
    void AddLoggedModuleFilterToList(TCHAR* pszModuleName);
    void RemoveLoggedModuleFilterFromList(TCHAR* pszModuleName);
    
    BOOL bLogOnlyBaseModule;
    BOOL RefreshModuleList();
    BOOL bInsideRefreshModuleList;
    tagFilteringWay FilteringWay;


public:

    CModulesFilters(void);
    ~CModulesFilters(void);
    BOOL SetModuleLogState(TCHAR* pszModuleName,BOOL bLog);
    BOOL ClearLoggedModulesListFilters();
    BOOL LogOnlyBaseModule(BOOL bOnlyBaseModule);
    BOOL TransfertNotLoggedModuleList();
    BOOL GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress(IN PBYTE pOriginAddress,
                                                                    OUT HMODULE* pCallingModuleHandle,
                                                                    OUT TCHAR* pszModuleName,
                                                                    OUT PBYTE* pRelativeAddress,
                                                                    OUT BOOL* pbShouldLog,
                                                                    BOOL TakeAnotherSnapshotIfNotFound,
                                                                    BOOL ContinueEvenIfShouldNotBeLogged);
    BOOL ShouldModuleBeLogged(TCHAR* pszModuleName);
    void SetFilteringWay(tagFilteringWay FilteringWay);
};
