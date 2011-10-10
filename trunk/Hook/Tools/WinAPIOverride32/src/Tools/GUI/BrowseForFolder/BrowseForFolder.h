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
// Object: Display Browse for folder dialog
//-----------------------------------------------------------------------------
#pragma once

#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <windows.h>
#include <shlobj.h>

class CBrowseForFolder
{
private:
    TCHAR szInitDir[MAX_PATH];
    BROWSEINFO bi;
    static INT CALLBACK BrowseCallbackProc(HWND hwnd, 
                                            UINT uMsg,
                                            LPARAM lp, 
                                            LPARAM pData);
public:
    TCHAR szSelectedPath[MAX_PATH];

    CBrowseForFolder(void);
    ~CBrowseForFolder(void);

    BOOL BrowseForFolder();
    BOOL BrowseForFolder(HWND Owner);
    BOOL BrowseForFolder(HWND Owner,TCHAR* szInitDir,TCHAR* szTitle,UINT ulFlags);
};
