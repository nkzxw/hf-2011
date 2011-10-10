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
// Object: class helper for displaying a messagebox containing the win32 error message 
//         after an API failure
//         As this class is used in many other tools class, and if you don't want to 
//         report error to the user, just define the keyword "TOOLS_NO_MESSAGEBOX"
//         in preprocessor options
//-----------------------------------------------------------------------------


#pragma once
#include <windows.h>
#include <malloc.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CAPIError
{
private:
    static HWND hParentWindow;
public:
    static void SetParentWindowHandle(HWND ParentWindow)
    {
        CAPIError::hParentWindow = ParentWindow;
    }
    static BOOL ShowLastError();
    static BOOL ShowLastError(TCHAR* szPrefix);
    static BOOL ShowError(DWORD dwLastError);
    static BOOL ShowError(TCHAR* szPrefix,DWORD dwError);
};
