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
// Object: converting a dump file to an exe file
//-----------------------------------------------------------------------------

#include "dumptoexe.h"


BOOL CDumpToExe::GetFileNames(TCHAR* pcSrcDumpFilename, TCHAR* pcDestExeFilename)
{
    *pcSrcDumpFilename=0;
    *pcDestExeFilename=0;
    TCHAR* pc;

    OPENFILENAME ofn;
    // Open file dialog
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.lpstrFilter=_T("dmp\0*.dmp\0All\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt=_T("dmp");
    ofn.lpstrFile=pcSrcDumpFilename;
    ofn.nMaxFile=MAX_PATH;
    
    if (!GetOpenFileName(&ofn))
        return FALSE;

    // copy src name
    _tcscpy(pcDestExeFilename,pcSrcDumpFilename);
    // remove extension
    pc=_tcsrchr(pcDestExeFilename,'.');
    if (pc)
        *pc=0;

    // save file dialog
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.lpstrFilter=_T("exe dll\0*.exe;*.dll\0All\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt=_T("exe");
    ofn.lpstrFile=pcDestExeFilename;
    ofn.nMaxFile=MAX_PATH;
    
    if (!GetSaveFileName(&ofn))
        return FALSE;

    return TRUE;
}

BOOL CDumpToExe::RemoveUnusedMemory()
{
    TCHAR pcSrcDumpFilename[MAX_PATH];
    TCHAR pcDestExeFilename[MAX_PATH];
    if (!CDumpToExe::GetFileNames(pcSrcDumpFilename,pcDestExeFilename))
        return FALSE;
    return CDumpToExe::RemoveUnusedMemory(pcSrcDumpFilename,pcDestExeFilename);
}
BOOL CDumpToExe::KeepAllAndModifyPe()
{
    TCHAR pcSrcDumpFilename[MAX_PATH];
    TCHAR pcDestExeFilename[MAX_PATH];
    if (!CDumpToExe::GetFileNames(pcSrcDumpFilename,pcDestExeFilename))
        return FALSE;
    return CDumpToExe::KeepAllAndModifyPe(pcSrcDumpFilename,pcDestExeFilename);
}

// we just remove all virtual memory that contain no raw data
BOOL CDumpToExe::RemoveUnusedMemory(TCHAR* pcSrcDumpFilename, TCHAR* pcDestExeFilename)
{
    HANDLE hFile;
    SIZE_T Size;
    SIZE_T ReadSize=0;
    SIZE_T WrittenSize=0;
    SIZE_T MinRaw=(SIZE_T)(-1);
    SIZE_T ExeLenght=0;
    unsigned char* pBufferSrcFile;
    int cnt;
    CPE* pPE;
    unsigned char* pBuffer;
    unsigned char* pBufferPos;

    // open file
    hFile = CreateFile(pcSrcDumpFilename, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
                    
    if (hFile==INVALID_HANDLE_VALUE)
    {   
        CAPIError::ShowLastError();
        return FALSE;
    }
    // Try to obtain hFile's size 
    Size = GetFileSize (hFile, NULL) ; 
    // If we failed ... 
    if (Size == INVALID_FILE_SIZE) 
    {
        CAPIError::ShowLastError();
        CloseHandle(hFile);
        return FALSE;
    }
    pBufferSrcFile=new unsigned char[Size];
    if (!ReadFile(hFile,pBufferSrcFile,Size,&ReadSize,NULL))
    {
        CAPIError::ShowLastError();
        delete[] pBufferSrcFile;
        CloseHandle(hFile);
        return FALSE;
    }
    CloseHandle(hFile);

    // now all content of src file is in pBufferSrcFile

    // retrieve pe infos
    pPE=new CPE(pcSrcDumpFilename);
    pPE->Parse();

    // get min raw pos and max size (max (raw_pos+raw_size))
    MinRaw=pPE->NTHeader.OptionalHeader.SizeOfHeaders;
    for (cnt=0;cnt<pPE->NTHeader.FileHeader.NumberOfSections;cnt++)
        ExeLenght=max(pPE->pSectionHeaders[cnt].PointerToRawData+pPE->pSectionHeaders[cnt].SizeOfRawData,ExeLenght);

    pBuffer=new unsigned char[ExeLenght];

    // copy headers (we use first pointer to raw data)
    SIZE_T AvailableSize;
    AvailableSize = __min(Size,MinRaw); // assume dump contains a least MinRaw
    memcpy(pBuffer,pBufferSrcFile,AvailableSize);

    BOOL bDataMissInDump = FALSE;

    // copy headers from their virtual addr space
    for (cnt=0;cnt<pPE->NTHeader.FileHeader.NumberOfSections;cnt++)
    {
        // put these data at the end of the buffer
        pBufferPos=pBuffer+(pPE->pSectionHeaders[cnt].PointerToRawData);

        // assume dump contains data
        if (Size<pPE->pSectionHeaders[cnt].VirtualAddress)
        {
            bDataMissInDump =TRUE;
            continue;
        }
        
        // assume dump contains data
        // AvailableSize = __min(Size-pPE->pSectionHeaders[cnt].VirtualAddress,pPE->pSectionHeaders[cnt].SizeOfRawData); 
        if (Size<pPE->pSectionHeaders[cnt].VirtualAddress+pPE->pSectionHeaders[cnt].SizeOfRawData)
        {
            bDataMissInDump =TRUE;
            AvailableSize = Size-pPE->pSectionHeaders[cnt].VirtualAddress;
        }
        else
        {
            AvailableSize = pPE->pSectionHeaders[cnt].SizeOfRawData;
        }

        // take pointer from virtual address space with size of raw data
        memcpy(pBufferPos,&pBufferSrcFile[pPE->pSectionHeaders[cnt].VirtualAddress],AvailableSize);

    }

    delete[] pBufferSrcFile;
    delete pPE;


    // write file
    hFile = CreateFile(pcDestExeFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
                    
    if (hFile==INVALID_HANDLE_VALUE)
    {   
        CAPIError::ShowLastError();
        delete[] pBuffer;
        return FALSE;
    }

    if(!WriteFile(hFile,pBuffer,ExeLenght,&WrittenSize,NULL))
    {   
        CAPIError::ShowLastError();
        CloseHandle(hFile);
        delete[] pBuffer;
        return FALSE;
    }

    CloseHandle(hFile);
    delete[] pBuffer;
    MessageBox(NULL,_T("Convertion successfully completed"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
    return TRUE;
}

// we need to change pe raw addr to pe virtual addr
BOOL CDumpToExe::KeepAllAndModifyPe(TCHAR* pcSrcDumpFilename, TCHAR* pcDestExeFilename)
{
    if (!CopyFile(pcSrcDumpFilename,pcDestExeFilename,FALSE))
    {
        CAPIError::ShowLastError();
        return FALSE;
    }
    // retrieve pe infos
    CPE* pPE=new CPE(pcDestExeFilename);
    if (!pPE->Parse())
    {
        delete pPE;
        return FALSE;
    }
    // change raw address to virtual address
    for (int cnt=0;cnt<pPE->NTHeader.FileHeader.NumberOfSections;cnt++)
    {
        pPE->pSectionHeaders[cnt].PointerToRawData=pPE->pSectionHeaders[cnt].VirtualAddress;
    }
    if (!pPE->SavePIMAGE_SECTION_HEADER())
    {
        delete pPE;
        return FALSE;
    }
    delete pPE;
    MessageBox(NULL,_T("Convertion successfully completed"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
    return TRUE;
}