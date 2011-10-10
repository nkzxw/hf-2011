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

#include "options.h"

COptions::COptions(TCHAR* FileName)
{
    this->FileName=_tcsdup(FileName);
    this->CommonConstructor();
}
COptions::COptions()
{
    this->FileName=NULL;
    this->CommonConstructor();
}
void COptions::CommonConstructor()
{
    this->SearchMatchCase=FALSE;
    this->SearchUnicode=TRUE;
    this->SearchAscii=TRUE;
    this->SearchHex=FALSE;
    *this->SearchContent=0;

    this->FilterMinAddress=0;
    this->FilterMaxAddress=0;
    this->FilterMinSize=0;
    this->FilterMaxSize=0;
    this->FilterMemoryFlags=COptions::FILTERMEMORYFLAGS_ALL;
    this->FilterApplyToCurrentEntries=TRUE;
}

COptions::~COptions(void)
{
    if (this->FileName)
        free(this->FileName);
}

// FALSE if option file don't exists
BOOL COptions::Load()
{

    if (!this->FileName)
        return FALSE;

    BOOL Ret=TRUE;

    // if file don't exist, constructor set values to 0 so just return a new object
    HANDLE hFile = CreateFile(this->FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        Ret=FALSE;
    CloseHandle(hFile);

    ////////////
    // Search UI
    ////////////
    GetPrivateProfileString( COPTIONS_SECTION_NAME_SEARCH_USER_INTERFACE,
                               COPTIONS_KEY_NAME_SEARCHCONTENT,
                               _T(""),
                               this->SearchContent,
                               MAX_PATH,
                               this->FileName
                               );

    this->SearchMatchCase=GetPrivateProfileInt( COPTIONS_SECTION_NAME_SEARCH_USER_INTERFACE,
                                                        COPTIONS_KEY_NAME_SEARCHMATCHCASE,
                                                        0,
                                                        this->FileName
                                                        );

    this->SearchUnicode=GetPrivateProfileInt( COPTIONS_SECTION_NAME_SEARCH_USER_INTERFACE,
                                                        COPTIONS_KEY_NAME_SEARCHUNICODE,
                                                        1,
                                                        this->FileName
                                                        );

    this->SearchAscii=GetPrivateProfileInt( COPTIONS_SECTION_NAME_SEARCH_USER_INTERFACE,
                                                        COPTIONS_KEY_NAME_SEARCHASCII,
                                                        1,
                                                        this->FileName
                                                        );

    this->SearchHex=GetPrivateProfileInt( COPTIONS_SECTION_NAME_SEARCH_USER_INTERFACE,
                                                        COPTIONS_KEY_NAME_SEARCHHEX,
                                                        1,
                                                        this->FileName
                                                        );

    ////////////
    // Filter UI
    ////////////
    this->FilterMemoryFlags=(COptions::FILTER_MEMORY_FLAGS)GetPrivateProfileInt( COPTIONS_SECTION_NAME_FILTER_USER_INTERFACE,
                                                        COPTIONS_KEY_NAME_FILTERMEMORYFLAGS,
                                                        COptions::FILTERMEMORYFLAGS_ALL,
                                                        this->FileName
                                                        );

    this->FilterMinAddress=GetPrivateProfileInt( COPTIONS_SECTION_NAME_FILTER_USER_INTERFACE,
                                                        COPTIONS_KEY_NAME_FILTERMINADDRESS,
                                                        0,
                                                        this->FileName
                                                        );
     
    this->FilterMaxAddress=GetPrivateProfileInt( COPTIONS_SECTION_NAME_FILTER_USER_INTERFACE,
                                                        COPTIONS_KEY_NAME_FILTERMAXADDRESS,
                                                        0,
                                                        this->FileName
                                                        );

    this->FilterMinSize=GetPrivateProfileInt( COPTIONS_SECTION_NAME_FILTER_USER_INTERFACE,
                                                        COPTIONS_KEY_NAME_FILTERMINSIZE,
                                                        0,
                                                        this->FileName
                                                        );
     
    this->FilterMaxSize=GetPrivateProfileInt( COPTIONS_SECTION_NAME_FILTER_USER_INTERFACE,
                                                        COPTIONS_KEY_NAME_FILTERMAXSIZE,
                                                        0,
                                                        this->FileName
                                                        );
           
              
              
          

    this->FilterApplyToCurrentEntries=GetPrivateProfileInt( COPTIONS_SECTION_NAME_FILTER_USER_INTERFACE,
                                                        COPTIONS_KEY_NAME_FILTERAPPLYTOCURRENTENTRIES,
                                                        1,
                                                        this->FileName
                                                        );




    return Ret;

}
BOOL COptions::Save()
{
    if (!this->FileName)
        return FALSE;

    TCHAR pszValue[MAX_PATH];

    ////////////
    // Search UI
    ////////////
    WritePrivateProfileString( COPTIONS_SECTION_NAME_SEARCH_USER_INTERFACE,
                               COPTIONS_KEY_NAME_SEARCHCONTENT,
                               this->SearchContent,
                               this->FileName
                               );

    _itot(this->SearchMatchCase,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_SEARCH_USER_INTERFACE,
                               COPTIONS_KEY_NAME_SEARCHMATCHCASE,
                               pszValue,
                               this->FileName
                               );

    _itot(this->SearchUnicode,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_SEARCH_USER_INTERFACE,
                               COPTIONS_KEY_NAME_SEARCHUNICODE,
                               pszValue,
                               this->FileName
                               );

    _itot(this->SearchAscii,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_SEARCH_USER_INTERFACE,
                               COPTIONS_KEY_NAME_SEARCHASCII,
                               pszValue,
                               this->FileName
                               );

    _itot(this->SearchHex,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_SEARCH_USER_INTERFACE,
                               COPTIONS_KEY_NAME_SEARCHHEX,
                               pszValue,
                               this->FileName
                               );


    ////////////
    // Filter UI
    ////////////
    _itot(this->FilterMemoryFlags,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_FILTER_USER_INTERFACE,
                               COPTIONS_KEY_NAME_FILTERMEMORYFLAGS,
                               pszValue,
                               this->FileName
                               );

    _itot(this->FilterMinAddress,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_FILTER_USER_INTERFACE,
                               COPTIONS_KEY_NAME_FILTERMINADDRESS,
                               pszValue,
                               this->FileName
                               );

    _itot(this->FilterMaxAddress,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_FILTER_USER_INTERFACE,
                               COPTIONS_KEY_NAME_FILTERMAXADDRESS,
                               pszValue,
                               this->FileName
                               );

    _itot(this->FilterMinSize,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_FILTER_USER_INTERFACE,
                               COPTIONS_KEY_NAME_FILTERMINSIZE,
                               pszValue,
                               this->FileName
                               );

    _itot(this->FilterMaxSize,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_FILTER_USER_INTERFACE,
                               COPTIONS_KEY_NAME_FILTERMAXSIZE,
                               pszValue,
                               this->FileName
                               );

    _itot(this->FilterApplyToCurrentEntries,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_FILTER_USER_INTERFACE,
                               COPTIONS_KEY_NAME_FILTERAPPLYTOCURRENTENTRIES,
                               pszValue,
                               this->FileName
                               );

    return TRUE;
}