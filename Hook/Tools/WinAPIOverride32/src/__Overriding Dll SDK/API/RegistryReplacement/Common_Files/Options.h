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

#include "RegistryCommonFunctions.h"
namespace EmulatedRegistry
{

class COptions
{
private:
    SIZE_T MajorVersion;
    SIZE_T MinorVersion;
    BOOL bUnicodeSave;
    BOOL AllowConnectionToUnspecifiedRemoteHosts;
    BOOL EmulateSubProcesses;// TRUE to emulate sub processes
    BOOL SpyMode;

    BOOL bHasChangedSinceLastSave;
public:


    FORCEINLINE SIZE_T GetMajorVersion() const { return this->MajorVersion; }
    FORCEINLINE void SetMajorVersion(SIZE_T MajorVersion) 
    {   
        if (this->MajorVersion == MajorVersion)
            return;
        this->MajorVersion = MajorVersion; 
        this->bHasChangedSinceLastSave = TRUE;
    }

    FORCEINLINE SIZE_T GetMinorVersion() const { return this->MinorVersion; }
    FORCEINLINE void SetMinorVersion(SIZE_T MinorVersion)
    {
        if (this->MinorVersion == MinorVersion)
            return;
        this->MinorVersion = MinorVersion;
        this->bHasChangedSinceLastSave = TRUE;
    }

    FORCEINLINE BOOL GetUnicodeSave() const { return this->bUnicodeSave; }
    FORCEINLINE void SetUnicodeSave(BOOL bUnicodeSave) 
    { 
        if (this->bUnicodeSave == bUnicodeSave)
            return;
        this->bUnicodeSave = bUnicodeSave;
        this->bHasChangedSinceLastSave = TRUE;
    }

    FORCEINLINE BOOL GetAllowConnectionToUnspecifiedRemoteHosts() const { return this->AllowConnectionToUnspecifiedRemoteHosts; }
    FORCEINLINE void SetAllowConnectionToUnspecifiedRemoteHosts(BOOL AllowConnectionToUnspecifiedRemoteHosts) 
    {
        if (this->AllowConnectionToUnspecifiedRemoteHosts == AllowConnectionToUnspecifiedRemoteHosts)
            return;
        this->AllowConnectionToUnspecifiedRemoteHosts = AllowConnectionToUnspecifiedRemoteHosts;
        this->bHasChangedSinceLastSave = TRUE;
    }

    FORCEINLINE BOOL GetEmulateSubProcesses() const { return this->EmulateSubProcesses; }
    FORCEINLINE void SetEmulateSubProcesses(BOOL EmulateSubProcesses) 
    {
        if (this->EmulateSubProcesses == EmulateSubProcesses)
            return;
        this->EmulateSubProcesses = EmulateSubProcesses;
        this->bHasChangedSinceLastSave = TRUE;
    }

    FORCEINLINE BOOL GetSpyMode() const { return this->SpyMode; }
    FORCEINLINE void SetSpyMode(BOOL SpyMode)
    {
        if (this->SpyMode == SpyMode)
            return;
        this->SpyMode = SpyMode;
        this->bHasChangedSinceLastSave = TRUE;
    }

    COptions();
    ~COptions();
    BOOL Load(TCHAR* KeyContent);
    BOOL Save(HANDLE hFile);
    FORCEINLINE BOOL HasChangedSinceLastSave() const {return this->bHasChangedSinceLastSave; }
};

}