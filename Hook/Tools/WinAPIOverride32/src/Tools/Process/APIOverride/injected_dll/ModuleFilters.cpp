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

#include "modulefilters.h"

extern CMailSlotClient* pMailSlotClt;
extern DWORD dwCurrentProcessID;
extern CNET_Manager* pNetManager;

CModulesFilters::CModulesFilters(void)
{
    this->bLogOnlyBaseModule=FALSE;
    this->pLinkListLoggedModulesFilters=new CLinkList(MAX_PATH*sizeof(TCHAR));
    this->ModulesInfosArraySize=0;
    this->bInsideRefreshModuleList=FALSE;
    this->FilteringWay=FILTERING_WAY_NOT_SPECIFIED_MODULES;
    // update list now (we do it before any hook occurs)
    this->RefreshModuleList();
    this->ClearLoggedModulesListFilters();
}

CModulesFilters::~CModulesFilters(void)
{
    delete this->pLinkListLoggedModulesFilters;
}

//-----------------------------------------------------------------------------
// Name: GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress
// Object: For a given address, find associated module name and relative address inside the module name
// Parameters :
//     in : PBYTE pOriginAddress : origin address
//          BOOL TakeAnotherSnapshotIfNotFound : true to take another snapshot if module name has not been found
//          BOOL ContinueSearchingCallingModuleEvenIfShouldNotBeLogged : if FALSE, function stops has soon as it detects module 
//                                                  should not be logged, without retrieving name and relative address
//     out: HMODULE* pCallingModuleHandle : calling module handle
//          TCHAR* pszModuleName : caller module name (must be already allocated, not allocated inside func)
//                                 Buffer size should be >= MAX_PATH
//          PBYTE* pRelativeAddress : relative address from caller base address
//			BOOL*  pbShouldLog : TRUE if Caller Address respect module filters
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CModulesFilters::GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress(IN PBYTE pOriginAddress,
                                                                               OUT HMODULE* pCallingModuleHandle,
                                                                               OUT TCHAR* pszModuleName,
                                                                               OUT PBYTE* pRelativeAddress,
                                                                               OUT BOOL* pbShouldLog,
                                                                               BOOL TakeAnotherSnapshotIfNotFound,
                                                                               BOOL ContinueSearchingCallingModuleEvenIfShouldNotBeLogged)
{
    BOOL NetModule;
    BOOL bFound=FALSE;
    BOOL bpbShouldLogSet = FALSE;

    *pbShouldLog=TRUE;
    *pszModuleName=0;
    *pRelativeAddress=0;
    MODULES_INFOS* pModuleInfo;
    BOOL FindModuleAlreadyDone=FALSE;
    DWORD Cnt;
    DWORD Cnt2;
    
    // try to get .Net information
    NetModule=FALSE;
    if (pNetManager)
    {
        // pNetManager is responsible to check if .Net logging is activated and HookNet dll is loaded
        if (pNetManager->GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress(
                                pOriginAddress,
                                pCallingModuleHandle,
                                pszModuleName, // module name + function name
                                pRelativeAddress))// relative address from function start
        {
            NetModule=TRUE;
            // tip: change pOriginAddress to module handle to keep a common algorithm
            // for .Net modules and standard modules
            pOriginAddress=(PBYTE)(*pCallingModuleHandle);
        }
    }

FindModule:
    for (Cnt=0;Cnt<this->ModulesInfosArraySize;Cnt++)
    {
        pModuleInfo=&this->ModulesInfosArray[Cnt];

        // check if lpBaseOfDll<=pOriginAddress<=lpBaseOfDll+SizeOfImage
        if (((pModuleInfo->Infos.modBaseAddr)<=(pOriginAddress))
            &&((pModuleInfo->Infos.modBaseAddr+pModuleInfo->Infos.modBaseSize)>=pOriginAddress))
        {
            bFound=TRUE;

            if (!NetModule)
            {
                // fill module base address
                *pCallingModuleHandle=(HMODULE)pModuleInfo->Infos.modBaseAddr;
                // fill relative address
                *pRelativeAddress=(PBYTE)(pOriginAddress-pModuleInfo->Infos.modBaseAddr);

                // fill module name
                // like _tcscpy(pszModuleName,pModuleInfo->Infos.szExePath);
                // but without calling any api to avoid to be hooked
                Cnt2=0;
                while(pModuleInfo->Infos.szExePath[Cnt2]!=0)
                {
                    pszModuleName[Cnt2]=pModuleInfo->Infos.szExePath[Cnt2];
                    Cnt2++;
                }
                pszModuleName[Cnt2]=0;
            }

            if (!bpbShouldLogSet)
            {
                // fill log state
                *pbShouldLog=pModuleInfo->bLog;
            }

            // stop searching
            break;
        }
        if ((this->bLogOnlyBaseModule))
        {
            // base module is at index 0 so if we are here, that means caller don't belongs to base module
            bpbShouldLogSet = TRUE;
            *pbShouldLog=FALSE;

            if (!ContinueSearchingCallingModuleEvenIfShouldNotBeLogged)
                return TRUE;
        }
    }

    if ((!bFound) && TakeAnotherSnapshotIfNotFound)
    {
        // if not found refresh module list and try one more time
        if (!FindModuleAlreadyDone)
        {
            FindModuleAlreadyDone=TRUE;
            // if hooked call is not originated by RefreshModuleList() func
            if (!this->bInsideRefreshModuleList)
            {
                this->RefreshModuleList();
                goto FindModule;
            }
        }
    }

    return bFound;
}

//-----------------------------------------------------------------------------
// Name: RefreshModuleList
// Object: update the list of loaded modules by doing a new CreateToolhelp32Snapshot
//          and next retrieve informations for each loaded module
// Parameters :
//     in : 
//     out: 
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CModulesFilters::RefreshModuleList()
{
    // if hooked call is originated by RefreshModuleList() func
    if (this->bInsideRefreshModuleList)
        return FALSE;

    this->bInsideRefreshModuleList=TRUE;

    MODULEENTRY32 me32 = {0}; 
    DWORD Cnt;
    HANDLE hModuleSnap =CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,dwCurrentProcessID);

    if (hModuleSnap == INVALID_HANDLE_VALUE) 
    {
        this->bInsideRefreshModuleList=FALSE;
        return FALSE; 
    }

    // Fill the size of the structure before using it. 
    me32.dwSize = sizeof(MODULEENTRY32); 
 
    // Walk the module list of the process
    if (!Module32First(hModuleSnap, &me32))
    {
        this->bInsideRefreshModuleList=FALSE;
        CloseHandle(hModuleSnap);
        return FALSE; 
    }

    // lock list
    this->pLinkListLoggedModulesFilters->Lock();

    Cnt=0;
    do 
    { 
        memcpy(&this->ModulesInfosArray[Cnt].Infos,&me32,sizeof(MODULEENTRY32));
        // fill log flag
        this->ModulesInfosArray[Cnt].bLog=this->ShouldModuleBeLogged(me32.szExePath);
        Cnt++;
    } 
    while (Module32Next(hModuleSnap, &me32)&&(Cnt<MAX_MODULES_NUMBERS)); 

    this->ModulesInfosArraySize=Cnt;

    // unlock list
    this->pLinkListLoggedModulesFilters->Unlock();

    // clean up the snapshot object. 
    CloseHandle (hModuleSnap); 

    this->bInsideRefreshModuleList=FALSE;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: LogOnlyBaseModule
// Object: specify if only base module must be logged
// Parameters :
//     in : BOOL bOnlyBaseModule : TRUE to log only base module, FALSE to log all modules
//     out: 
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CModulesFilters::LogOnlyBaseModule(BOOL bOnlyBaseModule)
{
    this->bLogOnlyBaseModule=bOnlyBaseModule;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ShouldModuleBeLogged
// Object: query if a module should be logged LIST MUST BE LOCKED by Calling func
// Parameters :
//     in : TCHAR* pszModuleName : name of the module
//     out: 
// Return : TRUE if module must be logged, FALSE else
//-----------------------------------------------------------------------------
BOOL CModulesFilters::ShouldModuleBeLogged(TCHAR* pszModuleName)
{
    // check if module name is in list
    CLinkListItem* pItem;
    for(pItem=this->pLinkListLoggedModulesFilters->Head;pItem;pItem=pItem->NextItem)
    {
        // if module is in list
        if (CWildCharCompare::WildICmp((TCHAR*)pItem->ItemData,pszModuleName))
        {
            switch(this->FilteringWay)
            {
            default:
            case FILTERING_WAY_NOT_SPECIFIED_MODULES:
                // module should not be logged
                return FALSE;
            case FILTERING_WAY_ONLY_SPECIFIED_MODULES:
                // module should be logged
                return TRUE;
            }
        }
    }
    switch(this->FilteringWay)
    {
    default:
    case FILTERING_WAY_NOT_SPECIFIED_MODULES:
        // module should be logged
        return TRUE;
    case FILTERING_WAY_ONLY_SPECIFIED_MODULES:
        // module should not be logged
        return FALSE;
    }
}


//-----------------------------------------------------------------------------
// Name: AddLoggedModuleFilterToList
// Object: add module name to filter module list
// Parameters :
//     in : TCHAR* pszModuleName : name of the module
//          pLinkListLoggedModulesFilters must be locked before calling this func
//     out: 
// Return : 
//-----------------------------------------------------------------------------
void CModulesFilters::AddLoggedModuleFilterToList(TCHAR* pszModuleName)
{
    this->pLinkListLoggedModulesFilters->Lock();
    // check if module name is already in list
    CLinkListItem* pItem;
    for(pItem=this->pLinkListLoggedModulesFilters->Head;pItem;pItem=pItem->NextItem)
    {
        // if module is already marked as not logged one
        if (CWildCharCompare::WildICmp((TCHAR*)pItem->ItemData,pszModuleName))
        {
            // return;
            this->pLinkListLoggedModulesFilters->Unlock();
            return;
        }
    }
    // item is not in list, add it
    this->pLinkListLoggedModulesFilters->AddItem(pszModuleName,TRUE);

    this->pLinkListLoggedModulesFilters->Unlock();
}

//-----------------------------------------------------------------------------
// Name: RemoveLoggedModuleFilterFromList
// Object: remove module name from filter module list 
//          RemoveLoggedModuleFilterFromList must be locked before calling this func
// Parameters :
//     in : TCHAR* pszModuleName : name of the module
//     out: 
// Return : 
//-----------------------------------------------------------------------------
void CModulesFilters::RemoveLoggedModuleFilterFromList(TCHAR* pszModuleName)
{
    // check if module name is in list
    CLinkListItem* pItem;
    CLinkListItem* pNextItem;
    this->pLinkListLoggedModulesFilters->Lock();
    for(pItem=this->pLinkListLoggedModulesFilters->Head;pItem;pItem=pNextItem)
    {
        pNextItem=pItem->NextItem;
        // if found
        if (CWildCharCompare::WildICmp((TCHAR*)pItem->ItemData,pszModuleName))
            // remove from list;
            this->pLinkListLoggedModulesFilters->RemoveItem(pItem,TRUE);
    }
    this->pLinkListLoggedModulesFilters->Unlock();
}

//-----------------------------------------------------------------------------
// Name: ClearNotLoggedModulesList
// Object: empty the not logged modules list
// Parameters :
//     in : 
//     out: 
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CModulesFilters::ClearLoggedModulesListFilters()
{
    // update list
    this->pLinkListLoggedModulesFilters->RemoveAllItems();

    BOOL DefaultState=TRUE;
    switch(this->FilteringWay)
    {
    default:
    case FILTERING_WAY_NOT_SPECIFIED_MODULES:
        // module should be logged by default
        DefaultState=TRUE;
        break;
    case FILTERING_WAY_ONLY_SPECIFIED_MODULES:
        // module should not be logged  by default
        DefaultState=FALSE;
        break;
    }

    // apply settings 
    for (DWORD Cnt=0;Cnt<this->ModulesInfosArraySize;Cnt++)
        this->ModulesInfosArray[Cnt].bLog=DefaultState;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetModuleLogState
// Object: set the module log state
// Parameters :
//     in : TCHAR* pszModuleName : name of the module
//          BOOL bAddToList : add to not hooked or hooked only list.
//                            hook will depend of FilteringWay
//     out: 
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CModulesFilters::SetModuleLogState(TCHAR* pszModuleName,BOOL bAddToList)
{
    BOOL bLog;
    BOOL bHasJoker;
    // check if wild char compare
    bHasJoker=_tcschr(pszModuleName,'*')||_tcschr(pszModuleName,'?');

    // update pLinkListNotLoggedModules before any call of RefreshModuleList

    if (bAddToList)
        this->AddLoggedModuleFilterToList(pszModuleName);
    else
        this->RemoveLoggedModuleFilterFromList(pszModuleName);

    // apply settings 
    switch(this->FilteringWay)
    {
    default:
    case FILTERING_WAY_NOT_SPECIFIED_MODULES:
        // module should be logged
        bLog=!bAddToList;
        break;
    case FILTERING_WAY_ONLY_SPECIFIED_MODULES:
        // module should not be logged
        bLog=bAddToList;
        break;
    }

    // search throw the current module list
    for (DWORD Cnt=0;Cnt<this->ModulesInfosArraySize;Cnt++)
    {
        if (CWildCharCompare::WildICmp(pszModuleName,this->ModulesInfosArray[Cnt].Infos.szExePath))
        {
            // apply settings 
            this->ModulesInfosArray[Cnt].bLog=bLog;
            // if filter rules contains joker, multiples modules can match, so don't break at first match
            if (!bHasJoker)
                break;
        }
    }

    // if not found, it's state will be update as soon as a call will come from this module
    // let do it by GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress from APIHandler

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetFilteringWay
// Object: specify if pLinkListLoggedModulesFilters define an exclusion list or an inclusion list
// Parameters :
//     in : 
//     out: 
// Return : 
//-----------------------------------------------------------------------------
void CModulesFilters::SetFilteringWay(tagFilteringWay FilteringWay)
{
    // use a local var to avoid to change FilteringWay
    // and assume this->FilteringWay will never equal FILTERING_WAY_DONT_USE_LIST
    // even in a multi threaded application
    tagFilteringWay LocalFilteringWay = FilteringWay;

    // if new filtering way is the same as the old one
    if (this->FilteringWay==FilteringWay)
        // do nothing
        return;

    if (LocalFilteringWay == FILTERING_WAY_DONT_USE_LIST)
    {
        // no list == log all modules == empty exclusion list
        LocalFilteringWay = FILTERING_WAY_NOT_SPECIFIED_MODULES;
    }

    // update filtering way before calling ClearLoggedModulesListFilters
    this->FilteringWay=LocalFilteringWay;

    // clear old filters
    this->ClearLoggedModulesListFilters();

}

//-----------------------------------------------------------------------------
// Name: TransfertNotLoggedModuleList
// Object: allow injector part to retrieve current not logged module list
// Parameters :
//     in : 
//     out: 
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CModulesFilters::TransfertNotLoggedModuleList()
{
    // transmitted data
    //      (DWORD) Cmd
    //      (DWORD) NbNotLogged modules
    //      TCHAR[MAX_PATH]     (NbNotLogged modules times)
    BOOL bResult;
    DWORD dwCnt;
    DWORD dwNbModules;
    DWORD dw;
    PBYTE Buffer;
    DWORD dwBufferSize;

    // refresh module list to provide correct informations
    if (!this->bInsideRefreshModuleList)
        this->RefreshModuleList();
    else
        // wait a while for module list to be fully refreshed
        Sleep(MODULES_FILTERS_MAX_WAIT_IN_MS);

    // lock list
    this->pLinkListLoggedModulesFilters->Lock();

    // get number of modules
    dwNbModules=this->ModulesInfosArraySize;
    if (dwNbModules==0)
        return FALSE;

    // compute buffer size
    dwBufferSize=2*sizeof(DWORD)+MAX_PATH*sizeof(TCHAR)*dwNbModules;
    
    // allocate memory
    Buffer=new BYTE[dwBufferSize];

    // fill command value
    dw=CMD_NOT_LOGGED_MODULE_LIST_REPLY;
    memcpy(Buffer,&dw,sizeof(DWORD));

    // fill buffer modules names, assuming we are not doing buffer overflow (in case new module as been added since memory allocation)
    DWORD dwNotLoggedModulesCount=0;
    for(dwCnt=0;(dwCnt<dwNbModules) && (dwCnt<this->ModulesInfosArraySize);dwCnt++) 
    {
        if (!this->ModulesInfosArray[dwCnt].bLog)
        {
            memcpy(&Buffer[2*sizeof(DWORD)+dwNotLoggedModulesCount*MAX_PATH*sizeof(TCHAR)],this->ModulesInfosArray[dwCnt].Infos.szExePath,MAX_PATH*sizeof(TCHAR));
            dwNotLoggedModulesCount++;
        }
    }

    // unlock list
    this->pLinkListLoggedModulesFilters->Unlock();

    // copy number of modules (dwCnt can be < dwNbModules) (in case module as been removed since memory allocation)
    memcpy(&Buffer[sizeof(DWORD)],&dwNotLoggedModulesCount,sizeof(DWORD));

    // transmit struct to remote process (2*sizeof(DWORD)+(dwCnt+1)*MAX_PATH can be < dwBufferSize) (in case module as been removed since memory allocation)
    bResult=pMailSlotClt->Write(Buffer,2*sizeof(DWORD)+dwNotLoggedModulesCount*MAX_PATH*sizeof(TCHAR));

    delete[] Buffer;
    return bResult;
}
