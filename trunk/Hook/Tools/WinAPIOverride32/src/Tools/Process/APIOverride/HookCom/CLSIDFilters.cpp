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
// Object: manages CLSID filters
//-----------------------------------------------------------------------------

#include "clsidfilters.h"

extern HOOK_COM_INIT HookComInfos;

CCLSIDFilters::CCLSIDFilters(void)
{
    this->pLinkListCLSID=new CLinkList(sizeof(CLSID));
    this->bExclusionFile=TRUE;
}

CCLSIDFilters::~CCLSIDFilters(void)
{
    // check pLinkListCLSID
    if (this->pLinkListCLSID)
    {
        delete this->pLinkListCLSID;
        this->pLinkListCLSID=NULL;
    }
}

//-----------------------------------------------------------------------------
// Name: ClearFilters
// Object: clear clsid filters : all COM object will be monitored
// Parameters :
//     in  : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CCLSIDFilters::ClearFilters()
{
    // check pLinkListCLSID
    if (!this->pLinkListCLSID)
        return FALSE;
    // remove all items of list
    this->pLinkListCLSID->RemoveAllItems();
    // if we want all CLSID match we have to put bExclusionFile to TRUE
    this->bExclusionFile=TRUE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ParseFiltersFile
// Object: parse a CLSID filtering file (exclusion or inclusion)
// Parameters :
//     in  : TCHAR* pszFileName : filtering file name
//           BOOL bExclusionFile : TRUE if pszFileName is an exclusion file,
//                                 FALSE if pszFileName is an inclusion file
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CCLSIDFilters::ParseFiltersFile(TCHAR* pszFileName,BOOL bExclusionFile)
{
    // check pLinkListCLSID
    if (!this->pLinkListCLSID)
        return FALSE;

    // check if file pszFileName exists
    if (!CStdFileOperations::DoesFileExists(pszFileName))
    {
        TCHAR pszMsg[2*MAX_PATH];
        _stprintf(pszMsg,_T("Filtering file %s doesn't exist"),pszFileName);
        HookComInfos.DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }

    // clear filters
    this->ClearFilters();

    // get new type of filter file (exclusion or inclusion)
    this->bExclusionFile=bExclusionFile;

    // parse file lines
    CTextFile::ParseLines(pszFileName,HookComInfos.hevtFreeProcess,CCLSIDFilters::ParseLinesCallBack,this);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ParseLinesCallBack
// Object: call back for CLSID file parsing
// Parameters :
//     in  : 
//     return : TRUE to continue parsing, FALSE to stop it
//-----------------------------------------------------------------------------
BOOL CCLSIDFilters::ParseLinesCallBack(TCHAR* pszFileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam)
{
    CLSID Clsid;
    BOOL bCLSIDRetrievalSuccess;
    CCLSIDFilters* pCLSIDFilters=(CCLSIDFilters*)UserParam;

    // trim string buffer
    Line=CTrimString::TrimString(Line);

    // if empty or commented line
    if ((*Line==0)||(*Line==';')||(*Line=='!'))
        // continue parsing
        return TRUE;

    // check for CLSID or ProgId
    if (*Line=='{') 
        bCLSIDRetrievalSuccess=CGUIDStringConvert::CLSIDFromTchar(Line,&Clsid);
    else
        // if first char != { --> check for ProgId
        bCLSIDRetrievalSuccess=CGUIDStringConvert::CLSIDFromProgId(Line,&Clsid);

    // convert line content to CLSID
    if(!bCLSIDRetrievalSuccess)
    {
        TCHAR pszMsg[3*MAX_PATH];
        // parsing error. Ask if we should continue parsing
        _sntprintf(pszMsg,3*MAX_PATH,_T("Parsing Error in %s at line: %d\r\n%s\r\nContinue parsing ?"),pszFileName,dwLineNumber,Line);
        if (HookComInfos.DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_YESNO|MB_ICONERROR|MB_TOPMOST)==IDYES)
            // continue parsing
            return TRUE;
        else
            // stop parsing
            return FALSE;
    }

    // add CLSID to pLinkListCLSID
    pCLSIDFilters->pLinkListCLSID->AddItem(&Clsid);

    //continue parsing
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CheckFilters
// Object: check CLSID filters for specified CLSID
// Parameters :
//     in  : CLSID* pClsid : pointer to CLSID to check
//     return : TRUE if CLSID match filters
//-----------------------------------------------------------------------------
BOOL CCLSIDFilters::CheckFilters(CLSID* pClsid)
{
    // if Clsid not specified, always hook
    if (*pClsid==CLSID_NULL)
        return TRUE;

    // check pLinkListCLSID
    if (!this->pLinkListCLSID)
        return FALSE;

    // for each item of pLinkListCLSID
    CLinkListItem* pItem;
    CLSID* mpClsid;
    for(pItem=this->pLinkListCLSID->Head;pItem;pItem=pItem->NextItem)
    {
        // check pointer
        if (IsBadReadPtr(pItem->ItemData,sizeof(CLSID)))
            continue;

        // get associated data
        mpClsid=(CLSID*)pItem->ItemData;

        // check if CLSID are equal
        if (IsEqualCLSID(*pClsid,*mpClsid))
        {
            if (this->bExclusionFile)
            {
                // CLSID belongs to a "not hooked CLSID file"
                // so CLSID musn't be hooked
                return FALSE;
            }
            else
            {
                // CLSID belongs to a "hook only CLSID file"
                // so CLSID must be hooked
                return TRUE;
            }
        }
    }

    // CLSID has not been found in list
    if (this->bExclusionFile)
        return TRUE;// CLSID doesn't belongs to a "not hooked CLSID file" --> must be hooked
    else
        return FALSE;// CLSID doesn't belongs to a "hook only CLSID file" --> musn't be hooked
}