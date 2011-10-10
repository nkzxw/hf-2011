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
// Object: Module Searching helper
//-----------------------------------------------------------------------------

#include "ModulesParser.h"

CModulesParser::CModulesParser(void)
{
}

CModulesParser::~CModulesParser(void)
{
}

BOOL CModulesParser::Parse(DWORD ProcessID,pfModuleFoundCallback ModuleFoundCallBack,PVOID UserParam)
{
    return CModulesParser::Parse(ProcessID,NULL,ModuleFoundCallBack,UserParam);
}

BOOL CModulesParser::Parse(DWORD ProcessID,HANDLE hCancelEvent,pfModuleFoundCallback ModuleFoundCallBack,PVOID UserParam)
{
    if (IsBadCodePtr((FARPROC)ModuleFoundCallBack))
        return FALSE;
    MODULEENTRY me = {0}; 
    HANDLE hSnap;

    // get modules informations
    hSnap =CreateToolhelpSnapshot(THCS_SNAPMODULE,ProcessID);
    if (hSnap == INVALID_HANDLE_VALUE) 
        return FALSE;

    // Fill the size of the structure before using it. 
    me.dwSize = sizeof(MODULEENTRY); 
 
    // Walk the module list of the process
    if (!ModuleFirst(hSnap, &me))
    {
        CloseHandle(hSnap);
        return FALSE;
    }
    do 
    { 
        // if cancel event
        if (hCancelEvent)
        {
            // if cancel event has raised
            if (WaitForSingleObject(hCancelEvent,0)!=WAIT_TIMEOUT)
            {
                // clean up the snapshot object. 
                CloseHandle (hSnap); 
                return TRUE;
            }
        }
        // call callback
        if (!ModuleFoundCallBack(&me,UserParam))
        {
            // if callback return false, user wants to stop parsing

            // clean up the snapshot object. 
            CloseHandle (hSnap); 
            return TRUE;
        }
    } 
    while (ModuleNext(hSnap, &me)); 
 
    // clean up the snapshot object. 
    CloseHandle (hSnap); 

    return TRUE;
}