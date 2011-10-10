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
// Object: manage windows clipboard
//-----------------------------------------------------------------------------

#include "clipboard.h"

//-----------------------------------------------------------------------------
// Name: CopyToClipboard
// Object: This function paste string inside clipboard
// Parameters :
//     in  : HWND hWindow : window handle
//           TCHAR* szData : data to copy
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CClipboard::CopyToClipboard(HWND hWindow,TCHAR* szData)
{
    LPTSTR  lptstrCopy; 
    HGLOBAL hglbCopy; 

    if (IsBadReadPtr(szData, sizeof(TCHAR)))
        return FALSE;

    if (!OpenClipboard(hWindow))
    {
        CAPIError::ShowLastError(CClipboard_ERROR_COPYING_DATA);
        return FALSE;
    }
    EmptyClipboard();

    // Allocate a global memory object for the text. 
    hglbCopy = GlobalAlloc(GMEM_MOVEABLE,(_tcslen(szData)+1)*sizeof(TCHAR)); 
    if (!hglbCopy) 
    {
        CloseClipboard();
        CAPIError::ShowLastError(CClipboard_ERROR_COPYING_DATA);
        return FALSE;
    }

    // Lock the handle and copy the text to the buffer. 
    lptstrCopy = (LPTSTR)GlobalLock(hglbCopy); 
    if (!lptstrCopy)
    {
        CloseClipboard();
        GlobalFree(hglbCopy);
        CAPIError::ShowLastError(CClipboard_ERROR_COPYING_DATA);
        return FALSE;
    }
    // copy data and ending null char
    memcpy(lptstrCopy, szData, (_tcslen(szData)+1)*sizeof(TCHAR));
    // unlock handle
    GlobalUnlock(hglbCopy); 

    // Place the handle on the clipboard.
    if (!SetClipboardData(
#if (defined(UNICODE)||defined(_UNICODE))
        CF_UNICODETEXT,
#else
        CF_TEXT,
#endif
        hglbCopy))
    {
        CAPIError::ShowLastError(CClipboard_ERROR_COPYING_DATA);
        CloseClipboard();
        GlobalFree(hglbCopy);
        return FALSE;
    }
    CloseClipboard();

    // Don't free memory with GlobalFree(hglbCopy); in case of SetClipboardData success
    // Windows do it itself at the next EmptyClipboard(); call

    return TRUE;
}
