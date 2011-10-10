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
// Object: class helper to convert ansi to unicode, and unicode to ansi
//-----------------------------------------------------------------------------

#include "ansiunicodeconvert.h"
namespace EmulatedRegistry
{

//-----------------------------------------------------------------------------
// Name: AnsiToUnicode
// Object: AnsiToUnicode converts the ANSI string pszA to a Unicode string
// and returns the Unicode string through ppszW. Space for the
// converted string is allocated by AnsiToUnicode. Must be free using free(*ppszW)
// Parameters :
//     in  : LPCSTR pszA : string to convert
//     out : LPWSTR* ppszW : converted string
//     return : win32 error code
//-----------------------------------------------------------------------------
SIZE_T CAnsiUnicodeConvert::AnsiToUnicode(LPCSTR pszA, OUT LPWSTR* ppszW)
{

    ULONG cCharacters;
    SIZE_T dwError;

    if (ppszW==NULL)
        return ERROR_INVALID_PARAMETER;

    // If input is null then just return the same.
    if (NULL == pszA)
    {
        *ppszW = NULL;
        return ERROR_SUCCESS;
    }

    // Determine number of wide characters to be allocated for the
    // Unicode string.
    cCharacters =  (ULONG)strlen(pszA)+1;

    *ppszW = (LPWSTR) malloc(cCharacters*sizeof(WCHAR));
    if (NULL == *ppszW)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Convert to Unicode.
    if (0 == MultiByteToWideChar(CP_ACP, 0, pszA, cCharacters,
                  *ppszW, cCharacters))
    {
        dwError = GetLastError();
        free(*ppszW);
        *ppszW = NULL;
        return dwError;
    }

    return ERROR_SUCCESS;
}

//-----------------------------------------------------------------------------
// Name: AnsiToUnicode
// Object: AnsiToUnicode converts the ANSI string pszA to a Unicode string
//          and returns the Unicode string through pszW. 
//          No memory allocation is done
// Parameters :
//     in  : LPCSTR pszA : string to convert
//           SIZE_T pszWMaxSize : Max size in WCHAR count
//     out : LPWSTR pszW : converted string
//     return : win32 error code
//-----------------------------------------------------------------------------
SIZE_T CAnsiUnicodeConvert::AnsiToUnicode(LPCSTR pszA,OUT LPWSTR pszW,SIZE_T pszWMaxSize)
{
    SIZE_T RetValue;

    if (pszWMaxSize<1)
        return ERROR_NOT_ENOUGH_MEMORY;

    if (pszW==NULL)
        return ERROR_INVALID_PARAMETER;

    // initialize out string to an empty string
    *pszW=0;

    if (pszA==NULL)
        return ERROR_INVALID_PARAMETER;

    // convert ansi string to unicode
    RetValue=MultiByteToWideChar(CP_ACP, 0, 
        pszA,
        -1, 
        pszW, 
        pszWMaxSize);

    // assume string is ended by \0
    pszW[pszWMaxSize-1]=0;

    return RetValue;
}

//-----------------------------------------------------------------------------
// Name: UnicodeToAnsi
// Object: UnicodeToAnsi converts the Unicode string pszW to an ANSI string
// and returns the ANSI string through ppszA. Space for the
// converted string is allocated by UnicodeToAnsi. Must be free using free(*ppszA)
// Parameters :
//     in  : LPCWSTR pszW : string to convert
//     out : LPSTR* ppszA : converted string
//     return : win32 error code
//-----------------------------------------------------------------------------
SIZE_T CAnsiUnicodeConvert::UnicodeToAnsi(LPCWSTR pszW, OUT LPSTR* ppszA)
{

    ULONG cbAnsi, cCharacters;
    SIZE_T dwError;

    if (ppszA==NULL)
        return ERROR_INVALID_PARAMETER;

    // If input is null then just return the same.
    if (pszW == NULL)
    {
        *ppszA = NULL;
        return ERROR_SUCCESS;
    }

    cCharacters = (ULONG)wcslen(pszW)+1;
    // Determine number of bytes to be allocated for ANSI string. An
    // ANSI string can have at most 2 bytes per character (for Double
    // Byte Character Strings.)
    cbAnsi = cCharacters*2;

    *ppszA = (LPSTR) malloc(cbAnsi);
    if (NULL == *ppszA)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Convert to ANSI.
    if (0 == WideCharToMultiByte(CP_ACP, 0, pszW, cCharacters, *ppszA,
                  cbAnsi, NULL, NULL))
    {
        dwError = GetLastError();
        free(*ppszA);
        *ppszA = NULL;
        return dwError;
    }
    return ERROR_SUCCESS;
} 


//-----------------------------------------------------------------------------
// Name: UnicodeToAnsi
// Object: UnicodeToAnsi converts the Unicode string pszW to an ANSI string
//          and returns the ANSI string through pszA. 
//          No memory allocation is done
// Parameters :
//     in  : LPCWSTR pszW : string to convert
//           SIZE_T pszAMaxSize : Max size in CHAR count (buffer size including \0)
//     out : LPSTR pszA : converted string
//     return : win32 error code
//-----------------------------------------------------------------------------
SIZE_T CAnsiUnicodeConvert::UnicodeToAnsi(LPCWSTR pszW,OUT LPSTR pszA,SIZE_T pszAMaxSize)
{
    SIZE_T RetValue;

    if (pszAMaxSize<1)
        return ERROR_NOT_ENOUGH_MEMORY;

    if (pszA==NULL)
        return ERROR_INVALID_PARAMETER;

    // initialize out string to an empty string
    *pszA=0;

    if (pszW==NULL)
        return ERROR_INVALID_PARAMETER;

    // convert unicode string to ansi
    RetValue=WideCharToMultiByte(CP_ACP, 0, 
        pszW,
        -1, 
        pszA, 
        pszAMaxSize,
        NULL,
        NULL);

    // assume string is ended by \0
    pszA[pszAMaxSize-1]=0;

    return RetValue;
}

//-----------------------------------------------------------------------------
// Name: UnicodeToTchar
// Object: UnicodeToTchar converts the Unicode string pszW to an TCHAR string
//          and returns the TCHAR string through psz. 
//          No memory allocation is done
// Parameters :
//     in  : LPCWSTR pszW : string to convert
//           SIZE_T pszMaxSize : Max size in TCHAR count (buffer size including \0)
//     out : LPTSTR psz : converted string
//     return : win32 error code
//-----------------------------------------------------------------------------
SIZE_T CAnsiUnicodeConvert::UnicodeToTchar(LPCWSTR pszW,OUT LPTSTR psz,SIZE_T pszMaxSize)
{
#if (defined(UNICODE)||defined(_UNICODE))
    if (pszMaxSize<1)
        return ERROR_NOT_ENOUGH_MEMORY;

    if (psz==NULL)
        return ERROR_INVALID_PARAMETER;
    *psz=0;

    if (pszW==NULL)
        return ERROR_INVALID_PARAMETER;

    _tcsncpy(psz,pszW,pszMaxSize);
    psz[pszMaxSize-1]=0;
    return ERROR_SUCCESS;
#else
    return CAnsiUnicodeConvert::UnicodeToAnsi(pszW,psz,pszMaxSize);
#endif
}

//-----------------------------------------------------------------------------
// Name: UnicodeToTchar
// Object: UnicodeToTchar converts the Unicode string pszW to an TCHAR string
// and returns the TCHAR string through ppsz. Space for the
// converted string is allocated by UnicodeToTchar. Must be free using free(*ppsz)
// Parameters :
//     in  : LPCWSTR pszW : string to convert
//     out : LPTSTR* ppsz : converted string
//     return : win32 error code
//-----------------------------------------------------------------------------
SIZE_T CAnsiUnicodeConvert::UnicodeToTchar(LPCWSTR pszW, OUT LPTSTR* ppsz)
{
#if (defined(UNICODE)||defined(_UNICODE))
    if (ppsz==NULL)
        return ERROR_INVALID_PARAMETER;
    *ppsz = 0;
    if (pszW==NULL)
        return ERROR_INVALID_PARAMETER;
    *ppsz = _tcsdup(pszW);
    return ERROR_SUCCESS;
#else
    return CAnsiUnicodeConvert::UnicodeToAnsi(pszW,ppsz);
#endif
}

//-----------------------------------------------------------------------------
// Name: AnsiToTchar
// Object: AnsiToTchar converts the ANSI string pszA to a TCHAR string
//          and returns the TCHAR string through psz. 
//          No memory allocation is done
// Parameters :
//     in  : LPCSTR pszA : string to convert
//           SIZE_T pszMaxSize : Max size in TCHAR count
//     out : LPTSTR psz : converted string
//     return : win32 error code
//-----------------------------------------------------------------------------
SIZE_T CAnsiUnicodeConvert::AnsiToTchar(LPCSTR pszA,OUT LPTSTR psz,SIZE_T pszMaxSize)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return CAnsiUnicodeConvert::AnsiToUnicode(pszA,psz,pszMaxSize);
#else
    if (pszMaxSize<1)
        return ERROR_NOT_ENOUGH_MEMORY;

    if (psz==NULL)
        return ERROR_INVALID_PARAMETER;
    *psz=0;

    if (pszA==NULL)
        return ERROR_INVALID_PARAMETER;

    _tcsncpy(psz,pszA,pszMaxSize);
    psz[pszMaxSize-1]=0;
    return ERROR_SUCCESS;   
#endif
}

//-----------------------------------------------------------------------------
// Name: AnsiToTchar
// Object: AnsiToTchar converts the ANSI string pszA to a TCHAR string
// and returns the TCHAR string through ppsz. Space for the
// converted string is allocated by AnsiToTchar. Must be free using free(*ppsz)
// Parameters :
//     in  : LPCSTR pszA : string to convert
//     out : LPTSTR* ppszW : converted string
//     return : win32 error code
//-----------------------------------------------------------------------------
SIZE_T CAnsiUnicodeConvert::AnsiToTchar(LPCSTR pszA, OUT LPTSTR* ppsz)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return CAnsiUnicodeConvert::AnsiToUnicode(pszA,ppsz);
#else
    if (ppsz==NULL)
        return ERROR_INVALID_PARAMETER;
    *ppsz = 0;
    if (pszA==NULL)
        return ERROR_INVALID_PARAMETER;
    *ppsz = _tcsdup(pszA);
    return ERROR_SUCCESS;    
#endif
}

//-----------------------------------------------------------------------------
// Name: TcharToAnsi
// Object: TcharToAnsi converts the TCHAR string to an ANSI string pszA 
//          and returns the ANSI string through pszA. 
//          No memory allocation is done
// Parameters :
//     in  : LPCTSTR psz : string to convert
//           SIZE_T pszAMaxSize : Max size in CHAR count
//     out : LPSTR pszA : converted string
//     return : win32 error code
//-----------------------------------------------------------------------------
SIZE_T CAnsiUnicodeConvert::TcharToAnsi(LPCTSTR psz,OUT LPSTR pszA,SIZE_T pszAMaxSize)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return CAnsiUnicodeConvert::UnicodeToAnsi(psz,pszA,pszAMaxSize);
#else
    if (pszAMaxSize<1)
        return ERROR_NOT_ENOUGH_MEMORY;

    if (pszA==NULL)
        return ERROR_INVALID_PARAMETER;
    *pszA=0;

    if (psz==NULL)
        return ERROR_INVALID_PARAMETER;

    _tcsncpy(pszA,psz,pszAMaxSize);
    pszA[pszAMaxSize-1]=0;
    return ERROR_SUCCESS;   
#endif
}

//-----------------------------------------------------------------------------
// Name: TcharToUnicode
// Object: TcharToUnicode converts the TCHAR string to an ANSI string pszA 
//          and returns the Unicode string through pszW. 
//          No memory allocation is done
// Parameters :
//     in  : LPCTSTR psz : string to convert
//           SIZE_T pszAMaxSize : Max size in CHAR count
//     out : LPSTR pszA : converted string
//     return : win32 error code
//-----------------------------------------------------------------------------
SIZE_T CAnsiUnicodeConvert::TcharToUnicode(LPCTSTR psz,OUT LPWSTR pszW,SIZE_T pszWMaxSize)
{
#if (defined(UNICODE)||defined(_UNICODE))
    if (pszWMaxSize<1)
        return ERROR_NOT_ENOUGH_MEMORY;

    if (pszW==NULL)
        return ERROR_INVALID_PARAMETER;
    *pszW=0;

    if (psz==NULL)
        return ERROR_INVALID_PARAMETER;

    _tcsncpy(pszW,psz,pszWMaxSize);
    pszW[pszWMaxSize-1]=0;
    return ERROR_SUCCESS;     
#else
    return CAnsiUnicodeConvert::AnsiToUnicode(psz,pszW,pszWMaxSize);
#endif
}

//-----------------------------------------------------------------------------
// Name: TcharToAnsi
// Object: TcharToAnsi converts the LPCTSTR string psz to an ANSI string
// and returns the ANSI string through ppszA. Space for the
// converted string is allocated by TcharToAnsi. Must be free using free(*ppszA)
// Parameters :
//     in  : LPCTSTR psz : string to convert
//     out : LPSTR* ppszA : converted string
//     return : win32 error code
//-----------------------------------------------------------------------------
SIZE_T CAnsiUnicodeConvert::TcharToAnsi(LPCTSTR psz,OUT LPSTR* ppszA)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return CAnsiUnicodeConvert::UnicodeToAnsi(psz,ppszA);
#else
    if (ppszA==NULL)
        return ERROR_INVALID_PARAMETER;
    *ppszA = 0;
    if (psz==NULL)
        return ERROR_INVALID_PARAMETER;
    *ppszA = _tcsdup(psz);
    return ERROR_SUCCESS;   
#endif
}

//-----------------------------------------------------------------------------
// Name: TcharToUnicode
// Object: TcharToUnicode converts the LPCTSTR string psz to a Unicode string
// and returns the Unicode string through ppszW. Space for the
// converted string is allocated by TcharToUnicode. Must be free using free(*ppszW)
// Parameters :
//     in  : LPCTSTR psz : string to convert
//     out : LPWSTR* ppszW : converted string
//     return : win32 error code
//-----------------------------------------------------------------------------
SIZE_T CAnsiUnicodeConvert::TcharToUnicode(LPCTSTR psz,OUT LPWSTR* ppszW)
{
#if (defined(UNICODE)||defined(_UNICODE))
    if (ppszW==NULL)
        return ERROR_INVALID_PARAMETER;
    *ppszW = 0;
    if (psz==NULL)
        return ERROR_INVALID_PARAMETER;
    *ppszW = _tcsdup(psz);
    return ERROR_SUCCESS;   
#else
    return CAnsiUnicodeConvert::AnsiToUnicode(psz,ppszW);
#endif
}
}