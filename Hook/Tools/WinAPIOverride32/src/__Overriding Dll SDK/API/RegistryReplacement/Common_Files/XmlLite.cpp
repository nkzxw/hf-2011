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
// Object: manages lite xml fields
//-----------------------------------------------------------------------------
#include "XmlLite.h"
#include <malloc.h>
#include <stdio.h>

namespace EmulatedRegistry
{

//-----------------------------------------------------------------------------
// Name: ReadXMLMarkupContent
// Object: read xml content. This func allow version soft read ascii saved files
//          or ansi version read unicode saved files
// Parameters :
//     in  : TCHAR* FullString : buffer supposed to contain markup
//           TCHAR* Markup : markup
//     out : TCHAR** ppszContent : content of the markup in the same encoding as FullString
//           SIZE_T* pContentLength : content length in TCHAR
//           TCHAR** pPointerAfterEndingMarkup : pointer after the markup
//     return : TRUE if Markup found, FALSE else
//-----------------------------------------------------------------------------
BOOL CXmlLite::ReadXMLMarkupContent(TCHAR* FullString,TCHAR* Markup,TCHAR** ppszContent,SIZE_T* pContentLength,TCHAR** pPointerAfterEndingMarkup)
{
    *ppszContent=0;
    *pContentLength=0;
    *pPointerAfterEndingMarkup=0;

    if (::IsBadReadPtr(FullString,sizeof(TCHAR*)))
        return FALSE;
    if (*FullString==0)
        return FALSE;

    size_t MarkupSize=_tcslen(Markup);
    TCHAR* pszStartTag=(TCHAR*)_alloca(sizeof(TCHAR)*(MarkupSize+3));
    TCHAR* pszEndTag=(TCHAR*)_alloca(sizeof(TCHAR)*(MarkupSize+4));
    TCHAR* pszStartTagPos;
    TCHAR* pszEndTagPos;
    
    // make xml start tag and end tag (Tag --> <Tag> and </Tag>)
    _stprintf(pszStartTag,_T("<%s>"),Markup);
    _stprintf(pszEndTag,_T("</%s>"),Markup);

    // search xml start tag and end tag
    pszStartTagPos=_tcsstr(FullString,pszStartTag);
    pszEndTagPos=_tcsstr(FullString,pszEndTag);
    // if none found
    if ((pszStartTagPos==0)||(pszEndTagPos==0))
        return FALSE;

    // check for sub items having same name by the way <a><b><a></a></b></a>
    TCHAR* pszStartTagPos2;
    TCHAR* pszEndTagPos2;
    pszStartTagPos2 = pszStartTagPos;

    while(pszStartTagPos2)
    {
        pszStartTagPos2=_tcsstr(pszStartTagPos2+1,pszStartTag);
        if (pszStartTagPos2)
        {
            if (pszStartTagPos2<pszEndTagPos)
            {
                pszEndTagPos2=_tcsstr(pszEndTagPos+1,pszEndTag);
                if(!pszEndTagPos2)
                    return FALSE;
                pszEndTagPos = pszEndTagPos2;
            }
        }
    }


    // if bad positions
    if (pszEndTagPos<pszStartTagPos)
        return FALSE;

    // get content of markup
    *ppszContent=pszStartTagPos+MarkupSize+2;
    *pContentLength=(SIZE_T)(pszEndTagPos-*ppszContent);
    *pPointerAfterEndingMarkup=pszEndTagPos+MarkupSize+3;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ReadXMLValue
// Object: read xml hexa value. 
// Parameters :
//     in  : TCHAR* FullString : buffer supposed to contain markup
//           TCHAR* Markup : markup
//     out : SIZE_T* pValue : value contained in the markup
//           TCHAR** pPointerAfterEndingMarkup : pointer after the markup
//     return : TRUE if Markup found, FALSE else
//-----------------------------------------------------------------------------
BOOL CXmlLite::ReadXMLValue(TCHAR* FullString,TCHAR* Markup,SIZE_T* pValue,TCHAR** pPointerAfterEndingMarkup)
{
    TCHAR* pszContent;
    SIZE_T ContentLength;
    *pValue=0;
    // read content
    if (!CXmlLite::ReadXMLMarkupContent(FullString,Markup,&pszContent,&ContentLength,pPointerAfterEndingMarkup))
        return FALSE;
    // if no content
    if (ContentLength==0)
        return FALSE;
    // check value retrieving
    if (_stscanf(pszContent,_T("0x%p"),pValue)!=1)
        return FALSE;
    return TRUE;

}

//-----------------------------------------------------------------------------
// Name: ReadXMLValue
// Object: read xml PBYTE buffer. 
// Parameters :
//     in  : TCHAR* FullString : buffer supposed to contain markup
//           TCHAR* Markup : markup
//     out : BYTE** pBuffer : retrieved buffer . Must be free by calling delete
//           SIZE_T* pBufferLengthInByte : length of the buffer
//           TCHAR** pPointerAfterEndingMarkup : pointer after the markup
//     return : TRUE if Markup found, FALSE else
//-----------------------------------------------------------------------------
BOOL CXmlLite::ReadXMLValue(TCHAR* FullString,TCHAR* Markup,BYTE** pBuffer,SIZE_T* pBufferLengthInByte,TCHAR** pPointerAfterEndingMarkup)
{
    TCHAR* pszContent;
    SIZE_T ContentLength;
    *pBuffer=0;
    *pBufferLengthInByte=0;

    // read content
    if (!CXmlLite::ReadXMLMarkupContent(FullString,Markup,&pszContent,&ContentLength,pPointerAfterEndingMarkup))
        return FALSE;
    if (ContentLength==0)
        return TRUE;

    // compute size of data
    *pBufferLengthInByte=ContentLength/2;
    *pBuffer=new BYTE[*pBufferLengthInByte];
    memset(*pBuffer, 0, *pBufferLengthInByte);
    // translate hex data to Byte
    BOOL RetValue = TRUE;
    SIZE_T Value;
    for (SIZE_T cnt=0;cnt<*pBufferLengthInByte;cnt++)
    {
        if (_stscanf(&pszContent[2*cnt],_T("%2X"),&Value)!=1)
            RetValue = FALSE;
        else
            (*pBuffer)[cnt] = Value;
    }

    return RetValue;
}

//-----------------------------------------------------------------------------
// Name: ReadXMLValue
// Object: read xml string. 
// Parameters :
//     in  : TCHAR* FullString : buffer supposed to contain markup
//           TCHAR* Markup : markup
//           BOOL bUnicodeFile : TRUE if file is in unicode
//     out : TCHAR** pszValue : retrieved string . Must be free by calling delete
//           TCHAR** pPointerAfterEndingMarkup : pointer after the markup
//     return : TRUE if Markup found, FALSE else
//-----------------------------------------------------------------------------
BOOL CXmlLite::ReadXMLValue(TCHAR* FullString,TCHAR* Markup,TCHAR** pszValue,BOOL bUnicodeFile,TCHAR** pPointerAfterEndingMarkup)
{
    BYTE* Buffer;
    SIZE_T BufferSize;
    *pszValue=0;
    // get content in byte buffer
    if (!CXmlLite::ReadXMLValue(FullString,Markup,&Buffer,&BufferSize,pPointerAfterEndingMarkup))
        return FALSE;

    // if empty field
    if  ( (BufferSize==0) || (!Buffer) )
    {
        // return an empty string
        *pszValue=new TCHAR[1];
        (*pszValue)[0]=0;
        return TRUE;
    }

    // we next have to convert buffer into ansi or unicode string

    // create string even BufferSize is null
#if (defined(UNICODE)||defined(_UNICODE))
    if (bUnicodeFile)
    {
        *pszValue=new TCHAR[BufferSize/2+1];// 1 wchar_t is 2 bytes
        memcpy(*pszValue,Buffer,BufferSize);
        (*pszValue)[BufferSize/2]=0;
    }
    else
    {
        *pszValue=new TCHAR[BufferSize+1];// 1 char is 1 byte
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)Buffer, BufferSize,*pszValue,(int)BufferSize);
        (*pszValue)[BufferSize]=0;
    }
#else
    if (bUnicodeFile)
    {
        *pszValue=new TCHAR[BufferSize/2+1];// 1 wchar_t is 2 byte
        WideCharToMultiByte(CP_ACP, 0,(LPCWSTR)Buffer, BufferSize/2, *pszValue, BufferSize/2, NULL, NULL);
        (*pszValue)[BufferSize/2]=0;
    }
    else
    {
        *pszValue=new TCHAR[BufferSize+1];// 1 char is 1 byte
        memcpy(*pszValue,Buffer,BufferSize);
        (*pszValue)[BufferSize]=0;
    }
#endif

    delete Buffer;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: WriteXMLValue
// Object: write dword. 
// Parameters :
//     in  : HANDLE hFile : file handle
//           TCHAR* Markup : xml markup
//           SIZE_T Value : value to write
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CXmlLite::WriteXMLValue(HANDLE hFile,TCHAR* Markup,SIZE_T Value)
{
    SIZE_T dwWrittenBytes;
    TCHAR psz[16];
    SIZE_T MarkupLength=(SIZE_T)_tcslen(Markup);
    ::WriteFile(hFile,_T("<"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
    ::WriteFile(hFile,Markup,MarkupLength*sizeof(TCHAR),&dwWrittenBytes,NULL);
    ::WriteFile(hFile,_T(">"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
    _stprintf(psz,_T("0x%p"),Value);
    ::WriteFile(hFile,psz,(SIZE_T)_tcslen(psz)*sizeof(TCHAR),&dwWrittenBytes,NULL);
    ::WriteFile(hFile,_T("</"),2*sizeof(TCHAR),&dwWrittenBytes,NULL);
    ::WriteFile(hFile,Markup,MarkupLength*sizeof(TCHAR),&dwWrittenBytes,NULL);
    ::WriteFile(hFile,_T(">"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
}

//-----------------------------------------------------------------------------
// Name: WriteXMLValue
// Object: write byte buffer. 
// Parameters :
//     in  : HANDLE hFile : file handle
//           TCHAR* Markup : xml markup
//           PBYTE Buffer : buffer to write
//           SIZE_T BufferLengthInByte : size of buffer
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CXmlLite::WriteXMLValue(HANDLE hFile,TCHAR* Markup,PBYTE Buffer,SIZE_T BufferLengthInByte)
{
    SIZE_T dwWrittenBytes;
    TCHAR psz[16];
    SIZE_T MarkupLength=(SIZE_T)_tcslen(Markup);
    ::WriteFile(hFile,_T("<"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
    ::WriteFile(hFile,Markup,MarkupLength*sizeof(TCHAR),&dwWrittenBytes,NULL);
    ::WriteFile(hFile,_T(">"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);

    for (SIZE_T cnt=0;cnt<BufferLengthInByte;cnt++)
    {
        _stprintf(psz,_T("%.2X"),Buffer[cnt]);
        ::WriteFile(hFile,psz,(SIZE_T)_tcslen(psz)*sizeof(TCHAR),&dwWrittenBytes,NULL);
    }

    ::WriteFile(hFile,_T("</"),2*sizeof(TCHAR),&dwWrittenBytes,NULL);
    ::WriteFile(hFile,Markup,MarkupLength*sizeof(TCHAR),&dwWrittenBytes,NULL);
    ::WriteFile(hFile,_T(">"),1*sizeof(TCHAR),&dwWrittenBytes,NULL);
}

//-----------------------------------------------------------------------------
// Name: WriteXMLValue
// Object: write string. 
// Parameters :
//     in  : HANDLE hFile : file handle
//           TCHAR* Markup : xml markup
//           TCHAR* Value : string to write
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CXmlLite::WriteXMLValue(HANDLE hFile,TCHAR* Markup,TCHAR* Value)
{
    if (Value==NULL)
    {
        TCHAR t=0;
        CXmlLite::WriteXMLValue(hFile,Markup,(BYTE*)&t,sizeof(TCHAR));
        return;
    }
    // convert to byte array to avoid a char to xml convertion at saving 
    //    and xml to char convertion at loading
    CXmlLite::WriteXMLValue(hFile,Markup,(BYTE*)Value,(SIZE_T)_tcslen(Value)*sizeof(TCHAR));
}

}