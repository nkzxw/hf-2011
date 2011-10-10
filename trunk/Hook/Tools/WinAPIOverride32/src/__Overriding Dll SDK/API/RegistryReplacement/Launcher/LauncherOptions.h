/*
Copyright (C) 2010 Jacquelin POTIER <jacquelin.potier@free.fr>
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

#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "../RegistryReplacementInjectedDll/RegReplaceOptions.h"

class CLauncherOptions
{
private:
    void ReportError(TCHAR* ErrorMessage);
public:
    typedef enum tagStartingWay
    {
        StartingWay_FROM_NAME,
        StartingWay_FROM_PROCESS_ID_AND_THREAD_ID_OF_SUSPENDED_PROCESS // if bHookSubProcesses is set and a subprocess start
    }STARTING_WAY;
    
    REGISTRY_REPLACEMENT_FILTERING_TYPE FilteringType;
    TCHAR FilteringTypeFileName[MAX_PATH]; // may relative

    TCHAR EmulatedRegistryConfigFile[MAX_PATH]; // may relative
    
    STARTING_WAY StartingWay; // specify if we have to take {ProcessId,ThreadId} or {TargetName,TargetCommandLine} fields into account
    DWORD ProcessId; // validity depends of StartingWay
    DWORD ThreadId;  // validity depends of StartingWay
    TCHAR TargetName[MAX_PATH]; // may relative
                                // validity depends of StartingWay
    TCHAR* TargetCommandLine;   // validity depends of StartingWay

    DWORD Flags;
    
    CLauncherOptions();
    ~CLauncherOptions();
    
    BOOL Load(TCHAR* ConfigFileName,BOOL bCalledFromCommandLine); // if bCalledFromCommandLine don't force StartingWay_FROM_NAME
    BOOL Save(TCHAR* ConfigFileName);
    
    BOOL CheckConsistency(BOOL bSpyMode);
};