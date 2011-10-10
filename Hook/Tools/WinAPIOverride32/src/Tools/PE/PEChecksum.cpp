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
// Object: PE helper
//-----------------------------------------------------------------------------

#include "PEChecksum.h"


//-----------------------------------------------------------------------------
// Name: GetChecksum
// Object: return the current file checksum stored in NTHeader.OptionalHeader.CheckSum
//         and the real file checksum
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPEChecksum::GetChecksum(DWORD* pCurrentHeaderCheckSum,DWORD* pRealCheckSum)
{
    HANDLE hFile;
    HANDLE hFileMapping;
    LPVOID BaseAddress;

    if (IsBadWritePtr(pCurrentHeaderCheckSum,sizeof(DWORD))
        || IsBadWritePtr(pRealCheckSum,sizeof(DWORD)))
        return FALSE;

    // open file
    hFile = CreateFile(this->pcFilename, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        CAPIError::ShowLastError();
        return FALSE;
    }
    // create file mapping
    hFileMapping=CreateFileMapping(hFile,NULL,PAGE_READONLY,0,0,NULL);
    if (!hFileMapping)
    {
        CAPIError::ShowLastError();
        CloseHandle(hFile);
        return FALSE;
    }
    // map view of file
    BaseAddress=MapViewOfFile(hFileMapping,FILE_MAP_READ,0,0,0);

    if (BaseAddress==NULL)
    {
        CAPIError::ShowLastError();
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return FALSE;
    }

    // get checksum infos
    if (!CheckSumMappedFile(
                        BaseAddress,
                        GetFileSize(hFile,NULL), // FileLength
                        pCurrentHeaderCheckSum,  // HeaderSum
                        pRealCheckSum            // CheckSum
                        )
       )
    {
        CAPIError::ShowLastError();
        UnmapViewOfFile(BaseAddress);
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        return FALSE;
    }

    // unmap view of file
    UnmapViewOfFile(BaseAddress);
    // close file mapping
    CloseHandle(hFileMapping);
    // close file
    CloseHandle(hFile);
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: CorrectChecksum
// Object: comput and corredt the checksum in NTHeader.OptionalHeader.CheckSum
//         Warning this func make a parsing before writting checksum
//         --> DosHeader, NTHeader and pSectionHeaders information will match 
//             file content and modifications to these member will be lost
//             exept if you save them before calling this func
// Parameters :
//     in  : 
//     out :
//     return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CPEChecksum::CorrectChecksum()
{
    DWORD dwCurrentCheckSum;
    DWORD dwRealCheckSum;
    if (!this->Parse())
        return FALSE;
    if (!this->GetChecksum(&dwCurrentCheckSum,&dwRealCheckSum))
        return FALSE;
    this->NTHeader.OptionalHeader.CheckSum=dwRealCheckSum;

    return this->SaveIMAGE_NT_HEADERS();
}
