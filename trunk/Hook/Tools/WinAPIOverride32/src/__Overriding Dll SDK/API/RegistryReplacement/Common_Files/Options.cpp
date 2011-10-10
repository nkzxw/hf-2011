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

#include "Options.h"

namespace EmulatedRegistry
{

COptions::COptions()
{
    this->bUnicodeSave = 
#if ( defined(UNICODE) || defined(_UNICODE) )
        1;
#else
        0;
#endif
    this-> MajorVersion=1;
    this-> MinorVersion=0;    
    this->EmulateSubProcesses = TRUE;
    this->AllowConnectionToUnspecifiedRemoteHosts = FALSE;
    this->SpyMode = FALSE;

    this->bHasChangedSinceLastSave = FALSE;
}
COptions::~COptions()
{

}
BOOL COptions::Load(TCHAR* KeyContent) 
{
    BOOL bRetValue = TRUE;
    TCHAR* pszCurrentMarkup = KeyContent;
    TCHAR* pszNextMarkup = NULL;
    SIZE_T Tmp = 0;


    // get version info
    if (CXmlLite::ReadXMLValue(pszCurrentMarkup,_T("MAJOR_VERSION"),&this->MajorVersion,&pszNextMarkup))
    {
        CXmlLite::ReadXMLValue(pszCurrentMarkup,_T("MINOR_VERSION"),&this->MinorVersion,&pszNextMarkup);
    }

    // even if file was saved with unicode file format, some xml editors (like m$ one) are happy to translate
    // file from unicode to ascii, and as our string are saved in hexa values it makes all string retrieval failed
    // add a field to specify file format
    CXmlLite::ReadXMLValue(pszCurrentMarkup,_T("UNICODE"),(DWORD*)&this->bUnicodeSave,&pszNextMarkup);        

    CXmlLite::ReadXMLValue(pszCurrentMarkup,_T("ALLOW_CONNECTION_TO_UNSPECIFIED_REMOTE_HOSTS_REGISTRIES"),&Tmp,&pszNextMarkup);
    this->AllowConnectionToUnspecifiedRemoteHosts = Tmp;

    CXmlLite::ReadXMLValue(pszCurrentMarkup,_T("EMULATE_SUB_PROCESSES"),&Tmp,&pszNextMarkup);
    this->EmulateSubProcesses = Tmp;

    if (CXmlLite::ReadXMLValue(pszCurrentMarkup,_T("SPY_MODE"),&Tmp,&pszNextMarkup))
        this->SpyMode = Tmp;
    else
        this->SpyMode = FALSE;

    this->bHasChangedSinceLastSave = FALSE;
    return bRetValue;
}    
BOOL COptions::Save(HANDLE hFile)
{
    // save options
    CTextFile::WriteText(hFile,_T("<MAJOR_VERSION>") RegistryReplacement_STR_CONFIG_FILE_MAJOR_VERSION _T("</MAJOR_VERSION>"));
    CTextFile::WriteText(hFile,_T("<MINOR_VERSION>") RegistryReplacement_STR_CONFIG_FILE_MINOR_VERSION _T("</MINOR_VERSION>"));
#if (defined(UNICODE)||defined(_UNICODE))
    CTextFile::WriteText(hFile,_T("<UNICODE>0x1</UNICODE>"));
#else
    CTextFile::WriteText(hFile,_T("<UNICODE>0x0</UNICODE>"));
#endif        

    CXmlLite::WriteXMLValue(hFile,_T("ALLOW_CONNECTION_TO_UNSPECIFIED_REMOTE_HOSTS_REGISTRIES"),this->AllowConnectionToUnspecifiedRemoteHosts);
    CXmlLite::WriteXMLValue(hFile,_T("EMULATE_SUB_PROCESSES"),this->EmulateSubProcesses);

    CXmlLite::WriteXMLValue(hFile,_T("SPY_MODE"),this->SpyMode);

    this->bHasChangedSinceLastSave = FALSE;
    return TRUE;
}
}