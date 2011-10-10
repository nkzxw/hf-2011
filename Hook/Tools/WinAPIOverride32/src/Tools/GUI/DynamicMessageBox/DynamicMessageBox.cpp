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
// Object: Allow to use messagebox without static linking with User32.dll
//         This dll will be loaded only at the first call of DynamicMessageBox
//-----------------------------------------------------------------------------

#include "DynamicMessageBox.h"

typedef int (__stdcall *tagMessageBox)(IN HWND hWnd,IN TCHAR* lpText,IN TCHAR* lpCaption,IN UINT uType);
HMODULE hModUser32=NULL;
tagMessageBox pMessageBox=NULL;

// avoid to load library user32.dll until not required
int DynamicMessageBox(IN HWND hWnd,IN TCHAR* lpText,IN TCHAR* lpCaption,IN UINT uType)
{
    // get proc address of DynamicMessageBox func
    if (!pMessageBox)
    {
        // if we don't have a handle to user32.dll try to get it
        if (!hModUser32)
        {
            hModUser32=GetModuleHandle(_T("user32.dll"));
            if (!hModUser32)// if user32.dll was not loaded
            {
                hModUser32=LoadLibrary(_T("user32.dll"));
                if(!hModUser32)
                    return -1;
            }
        }
#if (defined(UNICODE)||(_UNICODE))
        pMessageBox=(tagMessageBox)GetProcAddress(hModUser32,"MessageBoxW");
#else
        pMessageBox=(tagMessageBox)GetProcAddress(hModUser32,"MessageBoxA");
#endif
        if (!pMessageBox)
            return -1;
    }
    // now pMessageBox is ok
    return pMessageBox(hWnd,lpText,lpCaption,uType);
}