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
// Object: manages winapioverride options
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#define COPTIONS_SECTION_NAME_SEARCH_USER_INTERFACE         _T("SEARCH_UI")
#define COPTIONS_KEY_NAME_SEARCHMATCHCASE                   _T("SEARCHMATCHCASE")
#define COPTIONS_KEY_NAME_SEARCHUNICODE                     _T("SEARCHUNICODE")
#define COPTIONS_KEY_NAME_SEARCHASCII                       _T("SEARCHASCII")
#define COPTIONS_KEY_NAME_SEARCHHEX                         _T("SEARCHHEX")
#define COPTIONS_KEY_NAME_SEARCHCONTENT                     _T("SEARCHCONTENT")
#define COPTIONS_SECTION_NAME_FILTER_USER_INTERFACE         _T("FILTER_UI")
#define COPTIONS_KEY_NAME_FILTERMINADDRESS                  _T("FILTERMINADDRESS")
#define COPTIONS_KEY_NAME_FILTERMAXADDRESS                  _T("FILTERMAXADDRESS")
#define COPTIONS_KEY_NAME_FILTERMINSIZE                     _T("FILTERMINSIZE")
#define COPTIONS_KEY_NAME_FILTERMAXSIZE                     _T("FILTERMAXSIZE")
#define COPTIONS_KEY_NAME_FILTERMEMORYFLAGS                 _T("FILTERMEMORYFLAGS")
#define COPTIONS_KEY_NAME_FILTERAPPLYTOCURRENTENTRIES       _T("FILTERAPPLYTOCURRENTENTRIES")

class COptions
{
private:
    TCHAR* FileName;
    void CommonConstructor();
public:
    typedef enum FILTER_MEMORY_FLAGS
    {
        FILTERMEMORYFLAGS_ALL,
        FILTERMEMORYFLAGS_FREE,
        FILTERMEMORYFLAGS_MOVABLE,
        FILTERMEMORYFLAGS_FIXED
    };
    COptions();
    COptions(TCHAR* FileName);
    ~COptions(void);
    BOOL SearchMatchCase;
    BOOL SearchUnicode;
    BOOL SearchAscii;
    BOOL SearchHex;
    TCHAR SearchContent[MAX_PATH];

    DWORD FilterMinAddress;
    DWORD FilterMaxAddress;
    DWORD FilterMinSize;
    DWORD FilterMaxSize;
    FILTER_MEMORY_FLAGS FilterMemoryFlags;
    BOOL FilterApplyToCurrentEntries;

    BOOL Load();
    BOOL Save();

};
