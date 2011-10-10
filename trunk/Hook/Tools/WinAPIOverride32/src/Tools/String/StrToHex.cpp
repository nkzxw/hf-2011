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
// Object: for converting string hex to byte or DWORD
//-----------------------------------------------------------------------------

#include "StrToHex.h"
#include <stdio.h> // required in ansi mode for sprintf


//-----------------------------------------------------------------------------
// Name: StrHexToDword
// Object: Convert first 4 bytes (8 TCHARS) of psz into DWORD
// Parameters :
//     in  : TCHAR* psz : string containing hex value
//     out :
//     return : DWORD : result of convertion
//-----------------------------------------------------------------------------
DWORD CStrToHex::StrHexToDword(TCHAR* psz)
{
    BYTE cntChar;
    TCHAR c;
    DWORD dwRet=0;
    if (!psz)
        return 0;
    c=*psz;
    cntChar=0;
    while((cntChar<8)&&(c!=0))
    {
        c=psz[cntChar];
        if (((c>=_T('0'))&&(c<=_T('9')))
            ||((c>=_T('A'))&&(c<=_T('F')))
            ||((c>=_T('a'))&&(c<=_T('f')))
           )
        {
            dwRet<<=4;
            dwRet+=CStrToHex::StrHexToByte(c);
            cntChar++;
        }
        else
            break;
    }
    return dwRet;
}

//-----------------------------------------------------------------------------
// Name: StrHexToByte
// Object: Convert first byte (2 TCHARS) of psz into BYTE
// Parameters :
//     in  : TCHAR* psz : string containing hexa value
//     out :
//     return : BYTE : result of convertion
//-----------------------------------------------------------------------------
BYTE CStrToHex::StrHexToByte(TCHAR* psz)
{
    BYTE cntChar;
    TCHAR c;
    BYTE Ret=0;
    if (!psz)
        return 0;
    c=*psz;
    cntChar=0;
    while((cntChar<2)&&(c!=0))
    {
        c=psz[cntChar];
        if (((c>=_T('0'))&&(c<=_T('9')))
            ||((c>=_T('A'))&&(c<=_T('F')))
            ||((c>=_T('a'))&&(c<=_T('f')))
           )
        {
            Ret<<=4;
            Ret=(BYTE)(Ret+CStrToHex::StrHexToByte(c));
            cntChar++;
        }
        else
            break;
    }
    return Ret;
}

//-----------------------------------------------------------------------------
// Name: StrHexToByte
// Object: Convert a single TCHAR c into BYTE
// Parameters :
//     in  : TCHAR c : convert c into byte
//     out :
//     return : DWORD : result of conversion
//-----------------------------------------------------------------------------
BYTE CStrToHex::StrHexToByte(TCHAR c)
{
    BYTE b=0;
    if ((c>=_T('0'))&&(c<=_T('9')))
    {
        b=(BYTE)(c-_T('0'));
    }
    else 
    if ((c>=_T('A'))&&(c<=_T('F')))
    {
        b=(BYTE)(c-_T('A')+10);
    }
    else 
    if ((c>=_T('a'))&&(c<=_T('f')))
    {
        b=(BYTE)(c-_T('a')+10);
    }
    return b;
}

//-----------------------------------------------------------------------------
// Name: StrByteArrayToByteArray
// Object: convert a string byte array to a byte array
// Parameters :
//      in: TCHAR* pc : pointer string to convert like "AF12", "AD-FF-CD" "AB fe"
//      out : SIZE_T* pSize size in byte of return array
// Return : NULL on error, pointer to byte array else (must be deleted with delete[] if not null)
//-----------------------------------------------------------------------------
PBYTE CStrToHex::StrHexArrayToByteArray(TCHAR* pc,SIZE_T* pSize)
{
    // remove non alpha num char
    DWORD PosInArray;
    DWORD PosInRemovedArray=0;
    BYTE* Buffer;
    DWORD Size=(DWORD)_tcslen(pc);
    for (PosInArray=0;PosInArray<Size;PosInArray++)
    {
        if (((pc[PosInArray]>='a')&&(pc[PosInArray]<='f'))
            ||((pc[PosInArray]>='A')&&(pc[PosInArray]<='F'))
            ||((pc[PosInArray]>='0')&&(pc[PosInArray]<='9'))
            )
        {
            pc[PosInRemovedArray]=pc[PosInArray];
            PosInRemovedArray++;
        }
    }
    // ends string
    pc[PosInRemovedArray]=0;

    // compute size of data
    *pSize=(DWORD)_tcslen(pc)/2;
    Buffer=(BYTE*)new BYTE[*pSize];
    if (!Buffer)
    {
        *pSize=0;
        return NULL;
    }
    // translate hex data to Byte
    for (PosInArray=0;PosInArray<*pSize;PosInArray++)
        Buffer[PosInArray]=CStrToHex::StrHexToByte(&pc[PosInArray*2]);

    return Buffer;
}

//-----------------------------------------------------------------------------
// Name: ByteArrayToStrHexArray
// Object: convert an array to string byte representation
// Parameters :
//      in: PBYTE Buffer : buffer 
//          SIZE_T BufferSize : buffer size in byte count
//          TCHAR* ByteSplitter : splitter used between 2 byte (can be NULL)
// Return : NULL on error, pointer to string (must be deleted with delete[] if not null)
//-----------------------------------------------------------------------------
TCHAR* CStrToHex::ByteArrayToStrHexArray(PBYTE Buffer,SIZE_T BufferSize, TCHAR* ByteSplitter)
{
    TCHAR* mByteSplitter;
    TCHAR* StrResult;

    if (!ByteSplitter)
        mByteSplitter = _T("");
    else
        mByteSplitter = ByteSplitter;

    SIZE_T SplitterLen = _tcslen(mByteSplitter);
    SIZE_T RequiredBufferSize = BufferSize*(2+SplitterLen)+1;// 1 byte needs 2 char + len(splitter) to be represented
    StrResult = new TCHAR[RequiredBufferSize];
    if (!StrResult)
        return NULL;
    *StrResult=0;
    TCHAR* pszIndex;

    pszIndex=StrResult;
    for (SIZE_T cnt=0;cnt<BufferSize;cnt++)
    {
        if (cnt)
        {
            _tcscpy(pszIndex,mByteSplitter);
            pszIndex+=SplitterLen;
        }
        _stprintf(pszIndex,_T("%.2X"),Buffer[cnt]);
        pszIndex+=2;
    }
    return StrResult;
}
