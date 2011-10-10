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
// Object: File Searching helper
//-----------------------------------------------------------------------------

#include "FileSearch.h"


BOOL CFileSearch::IsDirectory(WIN32_FIND_DATA* pWin32FindData)
{
    return ((pWin32FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

//-----------------------------------------------------------------------------
// Name: Search
// Object: search for files and directories. file name can contain * and ?
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CFileSearch::Search(TCHAR* PathWithWildChar,BOOL SearchInSubDirectories,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam)
{
    return CFileSearch::Search(PathWithWildChar,SearchInSubDirectories,NULL,FileFoundCallBack,UserParam);
}

BOOL CFileSearch::Search(TCHAR* PathWithWildChar,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam)
{
    BOOL SearchCanceled;
    return CFileSearch::Search(PathWithWildChar,SearchInSubDirectories,hCancelEvent,FileFoundCallBack,UserParam,&SearchCanceled);
}

BOOL CFileSearch::Search(TCHAR* PathWithWildChar,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL* pSearchCanceled)
{
    TCHAR* pszFileFilter;
    // split search in directory + file filter
    pszFileFilter=_tcsrchr(PathWithWildChar,'\\');
    if (!pszFileFilter)
        return TRUE;
    // point after '\'
    pszFileFilter++;

    // get directory
    SIZE_T Size=pszFileFilter-PathWithWildChar;
    TCHAR szPath[MAX_PATH];
    memcpy(szPath,PathWithWildChar,Size*sizeof(TCHAR));
    szPath[Size]=0;

    return CFileSearch::SearchMultipleNames(szPath,&pszFileFilter,1,SearchInSubDirectories,hCancelEvent,FileFoundCallBack,UserParam,pSearchCanceled);
}

BOOL CFileSearch::SearchMultipleNames(TCHAR* Directory, TCHAR** FileNameWithWildCharArray,SIZE_T FileNameWithWildCharArraySize,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL* pSearchCanceled)
{
    HANDLE hFileHandle;
    WIN32_FIND_DATA FindFileData={0};
    *pSearchCanceled=FALSE;

    if (::IsBadCodePtr((FARPROC)FileFoundCallBack))
        return FALSE;

    // if cancel event is signaled
    if (::WaitForSingleObject(hCancelEvent,0)==WAIT_OBJECT_0)
        return TRUE;

    if (::IsBadReadPtr(Directory,sizeof(TCHAR)))
        return FALSE;

    if (::IsBadReadPtr(FileNameWithWildCharArray,sizeof(TCHAR*)*FileNameWithWildCharArraySize))
        return FALSE;

    SIZE_T Cnt;
    TCHAR FullPath[MAX_PATH];
    SIZE_T FullPathLen;
    _tcscpy(FullPath,Directory);
    FullPathLen = _tcslen(FullPath);
    // assume that directory ends with '\\'
    if (Directory[FullPathLen-1]!='\\')
    {
        _tcscat(FullPath,_T("\\"));
        FullPathLen++;
    }

    //////////////////////////////////////////////
    // 1) find files in current directory
    //////////////////////////////////////////////

    for (Cnt=0;Cnt<FileNameWithWildCharArraySize;Cnt++)
    {
        if (::IsBadReadPtr(FileNameWithWildCharArray[Cnt],sizeof(TCHAR)))
        {
#ifdef _DEBUG
            if (::IsDebuggerPresent())
                ::DebugBreak();
#endif
            continue;
        }

        // remove last filename if any
        FullPath[FullPathLen]=0;

        // forge new path
        _tcscat(FullPath,FileNameWithWildCharArray[Cnt]);

        // find first file
        hFileHandle=::FindFirstFile(FullPath,&FindFileData);
        if (hFileHandle!=INVALID_HANDLE_VALUE)// if no matching file found
        {
            do 
            {
                // call callback
                if (!FileFoundCallBack(Directory,&FindFileData,UserParam))
                {
                    *pSearchCanceled=TRUE;
                    ::FindClose(hFileHandle);
                    return TRUE;
                }

                // check cancel event
                if (hCancelEvent)
                {
                    // if event is signaled
                    if (::WaitForSingleObject(hCancelEvent,0)==WAIT_OBJECT_0)
                    {
                        *pSearchCanceled=TRUE;
                        ::FindClose(hFileHandle);
                        return TRUE;
                    }
                }

                // find next file
            } while(::FindNextFile(hFileHandle, &FindFileData));

            ::FindClose(hFileHandle);
        }
    }

    //////////////////////////////////////////////
    // 2) find sub directories
    //////////////////////////////////////////////

    // check if we should search in subdirectories
    if (!SearchInSubDirectories)
        return TRUE;

    // remove last filename if any
    FullPath[FullPathLen]=0;

    // SearchSubDirectoriesPattern="path\*"
    TCHAR SearchSubDirectoriesPattern[MAX_PATH];
    _tcscpy(SearchSubDirectoriesPattern,FullPath);
    _tcscat(SearchSubDirectoriesPattern,_T("*"));
    TCHAR SubDirectory[MAX_PATH];

    // find first file
    hFileHandle=::FindFirstFile(SearchSubDirectoriesPattern,&FindFileData);
    if (hFileHandle==INVALID_HANDLE_VALUE)// if no file found
        return TRUE;

    do 
    {
        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            if (     (_tcscmp(FindFileData.cFileName,_T("."))==0)
                || (_tcscmp(FindFileData.cFileName,_T(".."))==0)
                )
                continue;

            _tcscpy(SubDirectory,FullPath);
            _tcscat(SubDirectory,FindFileData.cFileName);
            _tcscat(SubDirectory,_T("\\"));
            CFileSearch::SearchMultipleNames(SubDirectory,FileNameWithWildCharArray,FileNameWithWildCharArraySize,SearchInSubDirectories,hCancelEvent,FileFoundCallBack,UserParam,pSearchCanceled);
            if (*pSearchCanceled)
            {
                ::FindClose(hFileHandle);
                return TRUE;
            }
        }

        // check cancel event
        if (hCancelEvent)
        {
            // if event is signaled
            if (::WaitForSingleObject(hCancelEvent,0)==WAIT_OBJECT_0)
            {
                *pSearchCanceled=TRUE;
                FindClose(hFileHandle);
                return TRUE;
            }
        }

        // find next file
    } while(::FindNextFile(hFileHandle, &FindFileData));

    ::FindClose(hFileHandle);

    return TRUE;
}