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
// Object: parse c/c++ defines integer definitions (don't support float/double or string)
//-----------------------------------------------------------------------------

#include "UserDefine.h"
#include "ExpressionParser.h"

#include "../../File/TextFile.h"
#include "../../File/StdFileOperations.h"
#include "../../String/StringConverter.h"
#include "../../LinkList/SingleThreaded/LinkListSimpleSingleThreaded.h"
#include "../../File/TextFile.h"
#include "../../String/SecureTcscat.h"
#include "../../String/CodeParserHelper.h"
#include "../../String/TrimString.h"

#pragma intrinsic (memcpy,memset,memcmp)

CUserDefine::CUserDefine()
{
    this->pDefineMap = NULL;
}

CUserDefine::~CUserDefine()
{
    if (this->pDefineMap)
        delete this->pDefineMap;
}


//-----------------------------------------------------------------------------
// Name: StaticDefineGetValue
// Object: get define value (value associated to a string expression)
// Parameters :
//      IN TCHAR* const Expression : queried litteral for which we have to provide a value
//      OUT ULONG_PTR* pValue : litteral value
//      PBYTE StringToValueFunctionUserParam : our user param ie CUserDefine* object
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CUserDefine::StaticDefineGetValue(IN TCHAR* const Expression,OUT ULONG_PTR* pValue,PBYTE StringToValueFunctionUserParam)
{
    return ((CUserDefine*)StringToValueFunctionUserParam)->GetDefineValue(Expression,(PBYTE*)pValue);
}

//-----------------------------------------------------------------------------
// Name: AddDefineValue
// Object: add define value to our {literal,value} list
// Parameters :
//      TCHAR* Name : literal 
//      PBYTE Value : value
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CUserDefine::AddDefineValue(TCHAR* Name, PBYTE Value)
{
    // create list if not existing
    if (!this->pDefineMap)
        this->pDefineMap = new CLinkList(sizeof(DEFINE_MAP_CONTENT));

#ifdef _DEBUG
    TCHAR sz[MAX_PATH];
    _sntprintf(sz,MAX_PATH,_T("Define : %s = 0x%p\r\n"),Name,Value);
        OutputDebugString(sz);
#endif

    // add {literal,value} to our list, checking name max size
    DEFINE_MAP_CONTENT Content={0};
    Content.Value = Value;
    _tcsncpy(Content.Name,Name,CUserDefine_DEFINE_NAME_MAX_SIZE);
    Content.Name[CUserDefine_DEFINE_NAME_MAX_SIZE-1]=0;
    return (this->pDefineMap->AddItem(&Content) != NULL );
}


//-----------------------------------------------------------------------------
// Name: GetDefineValue
// Object: get matching value from a literal from our {literal,value} list
// Parameters :
//      TCHAR* Name : literal 
//      OUT PBYTE* pValue : value
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CUserDefine::GetDefineValue(TCHAR* Name, OUT PBYTE* pValue)
{
    // default output value
    *pValue = 0;

    if (!this->pDefineMap)
        return FALSE;

    // search value into map
    CLinkListItem* pItem;
    DEFINE_MAP_CONTENT* pContent;

    this->pDefineMap->Lock();
    // for each item of *ppLinkListSimple
    for (pItem=this->pDefineMap->Head;pItem;pItem=pItem->NextItem)
    {
        pContent = (DEFINE_MAP_CONTENT*)pItem->ItemData;

        // check literal name
        if (_tcscmp(pContent->Name ,Name)==0)
        {
            // on success return associated value
            *pValue = pContent->Value;
            this->pDefineMap->Unlock();
            return TRUE;
        }
    }
    this->pDefineMap->Unlock();

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: DefineToString
// Object: get matching literal from a value from our {literal,value} list
//         to output something like "VarName : Value = DEFINE_NAME"
// Parameters :
//      TCHAR* const VarName : name of the var or parameter (only for output pString presentation)
//      PBYTE const Value : value for which we are looking for it's associated define name(s)
//      IN OUT TCHAR** pString : allocated string representing "VarName : Value = DEFINE_NAME"  (MUST BE FREE with delete)
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CUserDefine::DefineToString(TCHAR* const VarName,PBYTE const Value,IN OUT TCHAR** pString)
{
    SIZE_T StringMaxSize = 512;
    // allocate string
    *pString = new TCHAR[StringMaxSize];
    (*pString)[0]=0;

    // put "VarName:" inside string
    CSecureTcscat::Secure_tcscat(pString,VarName,&StringMaxSize);
    CSecureTcscat::Secure_tcscat(pString,_T(":"),&StringMaxSize);

    // add define value to 
    TCHAR StrValue[64];
    _stprintf(StrValue,_T("0x%p"),Value);
    CSecureTcscat::Secure_tcscat(pString,StrValue,&StringMaxSize);

    if (!this->pDefineMap)
        return FALSE;

    // search value into map
    CLinkListItem* pItem;
    DEFINE_MAP_CONTENT* pContent;
    ULONG_PTR BinaryAndValue;

    CLinkListSimpleSingleThreaded PossibleItems; 

    this->pDefineMap->Lock();
    // for each item of *ppLinkListSimple
    for (pItem=this->pDefineMap->Head;pItem;pItem=pItem->NextItem)
    {
        pContent = (DEFINE_MAP_CONTENT*)pItem->ItemData;
        
        // if found : copy string from map
        if (pContent->Value == Value)
        {
            // add " = FOO" to output string
            CSecureTcscat::Secure_tcscat(pString,_T(" = "),&StringMaxSize);
            CSecureTcscat::Secure_tcscat(pString,pContent->Name,&StringMaxSize);
            // free possible items
            PossibleItems.RemoveAllItems();
            break;
        }
        else 
        {
            // look for an binary or-ed expression like "FOO1 | FOO2 | FOO3"
            // if item match a binary or
            BinaryAndValue = (ULONG_PTR)Value & (ULONG_PTR)pContent->Value;
            if (    ( BinaryAndValue )
                 && ( (ULONG_PTR)pContent->Value == BinaryAndValue )
               )
            {
                // add item to possible items
                PossibleItems.AddItem(pContent);
            }
        }
    }
    this->pDefineMap->Unlock();

    // at this point no matching value has been found
    // check for possible or-ed values

    // if possible items list is not empty
    if (PossibleItems.GetItemsCount()>0)
    {
        PBYTE KnownValue;

        // add " = " to output string
        CSecureTcscat::Secure_tcscat(pString,_T(" = "),&StringMaxSize);

        KnownValue = 0;
        for (pItem=PossibleItems.Head;pItem;pItem=pItem->NextItem)
        {
            // if not first item
            if (pItem!=PossibleItems.Head)
                // add " | " to output string
                CSecureTcscat::Secure_tcscat(pString,_T(" | "),&StringMaxSize);
            pContent = (DEFINE_MAP_CONTENT*)pItem->ItemData;
            // add value to known value
            KnownValue=(PBYTE)((ULONG_PTR)KnownValue | (ULONG_PTR)pContent->Value);
            // add "FOO" to output string
            CSecureTcscat::Secure_tcscat(pString,pContent->Name,&StringMaxSize);
        }

        // if we don't get all flags for the binary or expression
        if (Value != KnownValue )
        {
            // add " | RemainingValue" to output string
            _stprintf(StrValue,_T(" | 0x%p"),Value);
            CSecureTcscat::Secure_tcscat(pString,StrValue,&StringMaxSize);
        }
    }

    // return TRUE
    return ((*pString)[0] != 0);
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: parse a file containing c/c++ defines
// Parameters :
//      TCHAR* const DefinePath : directory containing define files
//      TCHAR* const UserDefinesRelativePath : relative file path from DefinePath
//      pfReportError const ReportError : error report function
//      PBYTE const ReportErrorUserParam : error report function user param
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CUserDefine::Parse(TCHAR* const DefinePath,TCHAR* const UserDefinesRelativePath,pfReportError const ReportError,PBYTE const ReportErrorUserParam)
{
    TCHAR FilePath[MAX_PATH];
    TCHAR RelativePath[2*MAX_PATH];
	if (!DefinePath)
		return FALSE;
	if (!UserDefinesRelativePath)
		return FALSE;
    _tcscpy(RelativePath,DefinePath);
    _tcscat(RelativePath,_T("\\"));
    _tcscat(RelativePath,UserDefinesRelativePath);

    // assume we have an absolute path (no more relative)
    CStdFileOperations::GetAbsolutePath(RelativePath,FilePath);

    this->ReportError=ReportError;
    this->ReportErrorUserParam=ReportErrorUserParam;
    return this->Parse(FilePath);
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: parse a file containing c/c++ defines
// Parameters :
//      TCHAR* const AbsoluteFilePath : absolute file path 
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CUserDefine::Parse(TCHAR* const AbsoluteFilePath)
{
    TCHAR* pFileContent;
    // read file
    if (!CTextFile::Read(AbsoluteFilePath,&pFileContent))
    {
// if TOOLS_NO_MESSAGEBOX is not defined, error is already reported by CTextFile, so avoid to report the same error twice
#ifdef TOOLS_NO_MESSAGEBOX
        if (this->ReportError)
        {
            TCHAR Msg[2*MAX_PATH];
            _sntprintf(Msg,2*MAX_PATH,_T("Error reading file %s :"),AbsoluteFilePath);
            Msg[2*MAX_PATH-1]=0;
            this->ReportError(Msg,this->ReportErrorUserParam);
        }
#endif
        return FALSE;
    }

    // remove \\\r\n \\\n (splitters for multiple lines definitions) and comments
    TCHAR* pExpressionBegin;
    TCHAR* pCurrentPosition;
    TCHAR* pSubExpressionEnd;
    TCHAR* pCommentBegin;
    TCHAR* pCommentEnd;

    pCurrentPosition = pFileContent;
    for (;;)
    {
        pCurrentPosition = _tcspbrk(pCurrentPosition,_T("/\\\""));// take care of string
        if (pCurrentPosition == NULL)
        {
            break;
        }
        // check for multiple line definition
        if (*pCurrentPosition == '\\')
        {
            TCHAR* pTmp = pCurrentPosition;
            pTmp++;
            if ( *(pTmp) == '\r' )
                pTmp++;
            if ( *(pTmp) == '\n' )
            {
                // remove '\' and line break
                pTmp++;
                memmove(pCurrentPosition,pTmp, (_tcslen(pTmp)+1) * sizeof(TCHAR) );
                pCurrentPosition = pTmp;
            }
        }
        // check for comments
        else if (*pCurrentPosition == '/')
        {
            pCommentBegin = pCurrentPosition;
            if ( *(pCurrentPosition+1) == '/' ) // single line comment
            {
                pSubExpressionEnd = pCurrentPosition + 2;
SingleLineComment:
                pSubExpressionEnd = _tcspbrk(pSubExpressionEnd,_T("\n\\"));
                if (pSubExpressionEnd == NULL)
                {
                    // unterminated comment
                    // remove comment from string
                    *pCommentBegin = 0;
                    break;
                }
                // take care of multiple line definition
                else if (*pSubExpressionEnd == '\\')
                {
                    if ( *(pSubExpressionEnd+1) == '\n' )
                    {
                        pSubExpressionEnd = pSubExpressionEnd +2 ;
                        goto SingleLineComment;
                    }
                    else if ( *(pSubExpressionEnd+1) == '\r' )
                    {
                        if ( *(pSubExpressionEnd+2) == '\n' )
                        {
                            pSubExpressionEnd = pSubExpressionEnd +3 ;
                            goto SingleLineComment;
                        }
                    }
                }
                else if (*pSubExpressionEnd == '\n')
                {
                    // update pCurrentPosition to find end of current type
                    pCurrentPosition = pSubExpressionEnd + 1;

                    // remove comment from string
                    pCommentEnd = pCurrentPosition;
                    memmove(pCommentBegin,pCommentEnd, (_tcslen(pCommentEnd)+1) * sizeof(TCHAR) );

                    // as comment as been remove, next item to be parsed is at pCommentBegin
                    pCurrentPosition = pCommentBegin;
                    continue;
                }
            }
            else if ( *(pCurrentPosition+1) == '*' ) // multiple lines comment
            {
                // look for "*/"
                pSubExpressionEnd = _tcsstr (pCurrentPosition+2 , _T("*/") );
                if (pSubExpressionEnd ==NULL)
                {
                    // unterminated comment
                    // remove comment from string
                    *pCommentBegin = 0;
                    break;
                }

                // update pCurrentPosition to find end of current type
                pCurrentPosition = pSubExpressionEnd + 2;

                // remove comment from string
                pCommentEnd = pCurrentPosition;
                memmove(pCommentBegin,pCommentEnd, (_tcslen(pCommentEnd)+1) * sizeof(TCHAR) );

                // as comment as been remove, next item to be parsed is at pCommentBegin
                pCurrentPosition = pCommentBegin;
                continue;
            }
            else // single '/' --> nothing to do
            {
                pCurrentPosition++;
            }
        }
        // check for string
        else if (*pCurrentPosition == '"')
        {
            // search end of string 
            pSubExpressionEnd = pCurrentPosition;
FindEndOfString:
            // look for '"'
            pSubExpressionEnd = _tcschr (pSubExpressionEnd+1 , '"' );
            if (pSubExpressionEnd ==NULL)
            {
                // unterminated string
                break;
            }
            // take care of '\"' in string expressions
            else if ( *(pSubExpressionEnd-1) == '\\' )
            {
                goto FindEndOfString;
            }

            // update pCurrentPosition to find end of current type
            pCurrentPosition = pSubExpressionEnd + 2;
            continue;
        }
    }

    pExpressionBegin = pFileContent;

    // parse
    for (;;pExpressionBegin = pCurrentPosition+1)
    {
        pCurrentPosition = _tcschr(pExpressionBegin,'\n');
        if (pCurrentPosition)
        {
            // end current line
            *pCurrentPosition = 0;
            if (*(pCurrentPosition-1)=='\r')
                *(pCurrentPosition-1) = 0;
        }

        if (!this->ParseLine(pExpressionBegin))
        {
            if (this->ReportError)
            {
                TCHAR Msg[MAX_PATH];
                _sntprintf(Msg,MAX_PATH,_T("Error parsing %s"),pExpressionBegin);
                Msg[MAX_PATH-1]=0;
                this->ReportError(Msg,this->ReportErrorUserParam);
            }
        }

        if (pCurrentPosition == NULL)
        {
            break;
        }
    }

    delete pFileContent;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ParseLine
// Object: parse a line containing c/c++ defines (line musn't contain comments or multiple lines definitions)
// Parameters :
//      TCHAR* Line : line content
// Return : TRUE on success
//----------------------------------------------------------------------------- 
BOOL CUserDefine::ParseLine(TCHAR* Line)
{
    TCHAR* pDefineName;
    TCHAR* pDefineValue;

    // search for #define
    pDefineName = _tcsstr(Line,_T("#define"));
    if (!pDefineName) // can be #undef or ...
        return TRUE;

    // point after #define
    pDefineName+=_tcslen(_T("#define"));
    pDefineName = CCodeParserHelper::FindNextNotSplitterChar(pDefineName);
    if (!*pDefineName)
        return FALSE;

    pDefineValue = CCodeParserHelper::FindNextNotAlphaNumOrUnderscore(pDefineName);
    if (!*pDefineName) // define with no value 
        return TRUE;
    // ends pDefineName
    *pDefineValue = 0;

    // point after first space
    pDefineValue++;

    // removes spaces to keep only value
    pDefineValue = CTrimString::TrimString(pDefineValue,FALSE);

    // if no value content
    if (*pDefineValue==0) // define with no value 
        return TRUE;

    // look for string
    if (_tcschr(pDefineValue,'"')) // we are not interested by string defines
        return TRUE;

    // look for char
    if (_tcschr(pDefineValue,'\'')) // we are not interested by char defines
        return TRUE;

    // use expression parser to parse expression
    ULONG_PTR Value;
    if (!CExpressionParser::ParseExpression(pDefineValue,&Value, CUserDefine::StaticDefineGetValue,(PBYTE)this,this->ReportError,this->ReportErrorUserParam))
        return FALSE;
    // in case of success, add {name,value} to our list
    this->AddDefineValue(pDefineName,(PBYTE)Value);
    return TRUE;
}
