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


#include "APIError.h"
#ifndef TOOLS_NO_MESSAGEBOX
    #include <stdio.h>
#endif

HWND CAPIError::hParentWindow = NULL;

//-----------------------------------------------------------------------------
// Name: ShowLastError
// Object: show last windows api error
// Parameters :
//     in  :
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CAPIError::ShowLastError(TCHAR* szPrefix)
{
#if (defined(TOOLS_NO_MESSAGEBOX))
    UNREFERENCED_PARAMETER(szPrefix);
    return TRUE;
#else
    TCHAR pcError[MAX_PATH];
    TCHAR* pszMsg;
    DWORD dwLastError=GetLastError();
    DWORD dw=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL,
                           dwLastError,
                           GetUserDefaultLangID(),//GetSystemDefaultLangID()
                           pcError,
                           MAX_PATH-1,
                           NULL);
    
    pszMsg=(TCHAR*)_alloca((_tcslen(szPrefix)+MAX_PATH+6)*sizeof(TCHAR)); // +3 for \r\n and \0
    _tcscpy(pszMsg,szPrefix);

    //If the function succeeds, the return value is the number of TCHARs stored in the output buffer,
    //  excluding the terminating null character.
    //If the function fails, the return value is zero
    if(dw==0)
    {
        // FormatMessage failed
        _stprintf(pszMsg,_T("%s Error 0x%08X\r\n"),szPrefix,dwLastError);
    }
    else
    {
        // FormatMessage is ok add error description to pszMsg
        _tcscat(pszMsg,_T("\r\n"));
        _tcscat(pszMsg,pcError);
    }
    return MessageBox(CAPIError::hParentWindow,pszMsg,_T("Error"),MB_OK|MB_ICONERROR);

#endif
}

//-----------------------------------------------------------------------------
// Name: ShowLastError
// Object: show last windows api error
// Parameters :
//     in  :
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CAPIError::ShowLastError()
{
#if (defined(TOOLS_NO_MESSAGEBOX))
    return TRUE;
#else
    TCHAR pcError[MAX_PATH];
    DWORD dwLastError=GetLastError();
    DWORD dw=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL,
                           dwLastError,
                           GetUserDefaultLangID(),//GetSystemDefaultLangID()
                           pcError,
                           MAX_PATH-1,
                           NULL);
    
    
    //If the function succeeds, the return value is the number of TCHARs stored in the output buffer,
    //  excluding the terminating null character.
    //If the function fails, the return value is zero
    if(dw==0)
    {
        // FormatMessage failed
        _stprintf(pcError,_T("Error 0x%08X"),dwLastError);
    }
    return MessageBox(CAPIError::hParentWindow,pcError,_T("Error"),MB_OK|MB_ICONERROR);
#endif
}
//-----------------------------------------------------------------------------
// Name: ShowError
// Object: show windows api error (avoid to remove last error value)
// Parameters :
//     in  : DWORD dwLastError
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CAPIError::ShowError(DWORD dwLastError)
{
#if (defined(TOOLS_NO_MESSAGEBOX))
    UNREFERENCED_PARAMETER(dwLastError);
    return TRUE;
#else
    TCHAR pcError[MAX_PATH];
    DWORD dw=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL,
                           dwLastError,
                           GetUserDefaultLangID(),//GetSystemDefaultLangID()
                           pcError,
                           MAX_PATH-1,
                           NULL);
    
    
    //If the function succeeds, the return value is the number of TCHARs stored in the output buffer,
    //  excluding the terminating null character.
    //If the function fails, the return value is zero
    if(dw==0)
    {
        // FormatMessage failed
        _stprintf(pcError,_T("Error 0x%08X"),dwLastError);
    }
        return MessageBox(CAPIError::hParentWindow,pcError,_T("Error"),MB_OK|MB_ICONERROR);
#endif
}

//-----------------------------------------------------------------------------
// Name: ShowLastError
// Object: show last windows api error
// Parameters :
//     in  :
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CAPIError::ShowError(TCHAR* szPrefix,DWORD dwError)
{
#if (defined(TOOLS_NO_MESSAGEBOX))
    UNREFERENCED_PARAMETER(szPrefix);
    UNREFERENCED_PARAMETER(dwError);
    return TRUE;
#else
    TCHAR pcError[MAX_PATH];
    TCHAR* pszMsg;
    DWORD dw=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL,
                           dwError,
                           GetUserDefaultLangID(),//GetSystemDefaultLangID()
                           pcError,
                           MAX_PATH-1,
                           NULL);
    
    pszMsg=(TCHAR*)_alloca((_tcslen(szPrefix)+MAX_PATH+6)*sizeof(TCHAR)); // +3 for \r\n and \0
    _tcscpy(pszMsg,szPrefix);

    //If the function succeeds, the return value is the number of TCHARs stored in the output buffer,
    //  excluding the terminating null character.
    //If the function fails, the return value is zero
    if(dw==0)
    {
        // FormatMessage failed
        _stprintf(pszMsg,_T("%s Error 0x%08X\r\n"),szPrefix,dwError);
    }
    else
    {
        // FormatMessage is ok add error description to pszMsg
        _tcscat(pszMsg,_T("\r\n"));
        _tcscat(pszMsg,pcError);
    }
    return MessageBox(CAPIError::hParentWindow,pszMsg,_T("Error"),MB_OK|MB_ICONERROR);

#endif
}