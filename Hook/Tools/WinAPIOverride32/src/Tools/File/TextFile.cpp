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
// Object: text file operation
//         use no C library, 
//         support ascii or little endian unicode files
//-----------------------------------------------------------------------------

#include "textfile.h"

//-----------------------------------------------------------------------------
// Name: CreateTextFile
// Object: create text file with read write attributes
// Parameters :
//     in : TCHAR* FullPath : path of file to be created
//     out : HANDLE* phFile : if successful return, handle to the created file
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTextFile::CreateTextFile(TCHAR* FullPath,OUT HANDLE* phFile)
{
    *phFile = ::CreateFile(FullPath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (*phFile==INVALID_HANDLE_VALUE)
        return FALSE;

#if (defined(UNICODE)||defined(_UNICODE))
    DWORD dwWrittenBytes;
    // in unicode mode, write the unicode little endian header (FFFE)
    BYTE pbUnicodeHeader[2]={0xFF,0xFE};
    ::WriteFile(*phFile,pbUnicodeHeader,2,&dwWrittenBytes,NULL);
#endif

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CreateTextFile
// Object: create text file with read write attributes
// Parameters :
//     in : TCHAR* FullPath : path of file to be created
//     out : HANDLE* phFile : if successful return, handle to the created file
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTextFile::CreateOrOpenForAppending(TCHAR* FullPath,OUT HANDLE* phFile)
{
    *phFile = ::CreateFile(FullPath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (*phFile==INVALID_HANDLE_VALUE)
    {
        if (CTextFile::CreateTextFile(FullPath,phFile))
            return FALSE;
    }

    ::SetFilePointer(*phFile,NULL,NULL,FILE_END);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: WriteText
// Object: write text at current position
// Parameters :
//     in : HANDLE hFile : File handle
//          TCHAR* Text : text to write
//     out  
//     return : result of WriteFile
//-----------------------------------------------------------------------------
BOOL CTextFile::WriteText(HANDLE hFile,TCHAR* Text)
{
    DWORD dwWrittenBytes;
    return WriteFile(hFile,Text,(DWORD)(_tcslen(Text)*sizeof(TCHAR)),&dwWrittenBytes,NULL);
}

//-----------------------------------------------------------------------------
// Name: WriteText
// Object: write text at current position
// Parameters :
//     in : HANDLE hFile : File handle
//          TCHAR* Text : text to write
//          SIZE_T LenInTCHAR : number of TCHAR to be written 
//     out  
//     return : result of WriteFile
//-----------------------------------------------------------------------------
BOOL CTextFile::WriteText(HANDLE hFile,TCHAR* Text,SIZE_T LenInTCHAR)
{
    DWORD dwWrittenBytes;
    return WriteFile(hFile,Text,(DWORD)(LenInTCHAR*sizeof(TCHAR)),&dwWrittenBytes,NULL);
}


//-----------------------------------------------------------------------------
// Name: ReportError
// Object: show an error message
// Parameters :
//     in : TCHAR* pszMsg
//     out  
//     return 
//-----------------------------------------------------------------------------
void CTextFile::ReportError(TCHAR* pszMsg)
{
#ifdef TOOLS_NO_MESSAGEBOX
    UNREFERENCED_PARAMETER(pszMsg);
#else
    // else use static linking
    MessageBox(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR);
#endif
}

//-----------------------------------------------------------------------------
// Name: Read
// Object: 
// Parameters :
//     in : TCHAR* FileName file name
//     out : TCHAR** ppszContent : content of file converted to current TCHAR definition
//           must be free with delete *ppszContent
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CTextFile::Read(TCHAR* FileName,TCHAR** ppszContent)
{
    BOOL b;
    return CTextFile::Read(FileName,ppszContent,&b);
}


//-----------------------------------------------------------------------------
// Name: Read
// Object: 
// Parameters :
//     in : TCHAR* FileName file name
//     out : TCHAR** ppszContent : content of file converted to current TCHAR definition
//           must be free with delete *ppszContent
//          BOOL* pbUnicodeFile : gives information about file encoding
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CTextFile::Read(TCHAR* FileName,TCHAR** ppszContent,BOOL* pbUnicodeFile)
{
    *pbUnicodeFile=FALSE;
    HANDLE hFile;
    TCHAR pszFileName[MAX_PATH];
    TCHAR pszMsg[2*MAX_PATH];
    DWORD dwFileSize;
    DWORD ContentSize=0;
    *ppszContent=0;

    // check empty file name
    if (*FileName==0)
    {
        CTextFile::ReportError(_T("Empty file name"));
        return FALSE;
    }

    // assume we get full path file name, else add the current exe path to pszFileName
    if (_tcschr(FileName,'\\')==0)
    {
        TCHAR* pszLastSep;
        // we don't get full path, so add current exe path
        GetModuleFileName(GetModuleHandle(NULL),pszFileName,MAX_PATH);
        pszLastSep=_tcsrchr(pszFileName,'\\');
        if (pszLastSep)
            *(pszLastSep+1)=0;// keep \ char

        _tcscat(pszFileName,FileName);
    }
    else
        // we get full path only copy file name
        _tcscpy(pszFileName,FileName);

    // open file
    hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        _sntprintf(pszMsg,2*MAX_PATH,_T("File %s not found"),pszFileName);
        CTextFile::ReportError(pszMsg);
        return FALSE;
    }

    // get file size
    dwFileSize = ::GetFileSize(hFile, NULL);
    if (dwFileSize == 0xFFFFFFFF)
    {
        _sntprintf(pszMsg,2*MAX_PATH,_T("Can't get file size for %s"),pszFileName);
        CTextFile::ReportError(pszMsg);
        CloseHandle(hFile);
        return FALSE;
    }

    if (dwFileSize==0)
    {
        // as we return true, assume to put an empty string
        *ppszContent=new TCHAR[1];
        (*ppszContent)[0]=0;
        CloseHandle(hFile);
        return TRUE;
    }

    // map view of file
    BYTE* pb=new BYTE[dwFileSize];
    BYTE* pbFileContent;
    DWORD dwRealyRead;
    if (!ReadFile(hFile,pb,dwFileSize,&dwRealyRead,NULL))
	{
		delete[] pb;
		CloseHandle(hFile);
		return FALSE;
	}

    pbFileContent=pb;
    // check if file is unicode or ansi file
    // unicode files begin with FFFE in little endian
    if (dwFileSize>=2)
    {
        // check FFFE (we have to cast pcFileOrig in PBYTE as we don't know if it's 8 or 16 byte pointer)
        if ((pb[0]==0xFF)
            &&(pb[1]==0xFE))
        {
            *pbUnicodeFile=TRUE;
            // point after Unicode header
            pbFileContent=&pb[2];
        }
    }
#if (defined(UNICODE)||defined(_UNICODE))
    if (*pbUnicodeFile)
    {
        // remove signature
        ContentSize=dwFileSize-2;
        // adjust file size : file size will now be in TCHAR count no more in bytes
        ContentSize/=2;

        *ppszContent=new TCHAR[ContentSize+1];
        memcpy(*ppszContent,pbFileContent,dwFileSize-2);
        // ends string
        (*ppszContent)[ContentSize]=0;
    }
    // if we work in unicode and the file is in ansi, convert it in unicode
    else
    {
        ContentSize=dwFileSize;
        // convert into unicode
        *ppszContent=new TCHAR[ContentSize+1];

        if (!MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pbFileContent, dwFileSize,*ppszContent,(int)ContentSize))
        {
            _sntprintf(pszMsg,2*MAX_PATH,_T("Error converting file %s in unicode"),pszFileName);
            CTextFile::ReportError(pszMsg);
            CloseHandle(hFile);
            delete[] pb;
            delete[] *ppszContent;
            *ppszContent=0;
            return FALSE;
        }
        (*ppszContent)[ContentSize]=0;
    }
#else
    // if we work in ansi and the file is in unicode, convert it in ansi
    if (*pbUnicodeFile)
    {
        // remove signature
        ContentSize=dwFileSize-2;
        // adjust file size : file size will now be in TCHAR count no more in bytes
        ContentSize/=2;

        // convert into ansi
        *ppszContent=new TCHAR[ContentSize+1];
        if (!WideCharToMultiByte(CP_ACP, 0,(LPCWSTR)pbFileContent, ContentSize, *ppszContent, ContentSize, NULL, NULL))
        {
            _sntprintf(pszMsg,2*MAX_PATH,_T("Error converting file %s in ansi"),pszFileName);
            CTextFile::ReportError(pszMsg);
            CloseHandle(hFile);
            delete[] pb;
            delete[] *ppszContent;
            *ppszContent=0;
            return FALSE;
        }
        (*ppszContent)[ContentSize]=0;
    }
    else
    {
        ContentSize=dwFileSize;
        *ppszContent=new TCHAR[ContentSize+1];
        memcpy(*ppszContent,pbFileContent,ContentSize);
        // ends string
        (*ppszContent)[ContentSize]=0;
    }
#endif

    CloseHandle(hFile);
    delete[] pb;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ParseLines
// Object: for each line of the file pszFileName, call the LineCallBack Callback
//          giving it 
//              - line content
//              - the current line number (1 based number)
//              - the provide user parameter
// Parameters :
//     in : TCHAR* pszFileName : file to parse
//          tagLineCallBack LineCallBack : callback called on each new line, 
//                                         must return TRUE to continue parsing, FALSE to stop it
//          LPVOID CallBackUserParam : user parameter translated as callback arg
//     out  
//     return : TRUE on success, False on error
//-----------------------------------------------------------------------------
BOOL CTextFile::ParseLines(TCHAR* pszFileName,tagLineCallBack LineCallBack,LPVOID CallBackUserParam)
{
    return CTextFile::ParseLines(pszFileName,NULL,LineCallBack,CallBackUserParam);
}



//-----------------------------------------------------------------------------
// Name: ParseLines
// Object: for each line of the file pszFileName, call the LineCallBack Callback
//          giving it 
//              - line content
//              - the current line number (1 based number)
//              - the provide user parameter
// Parameters :
//     in : TCHAR* pszFileName : file to parse
//          HANDLE hCancelEvent : a event that is set to stop parsing (can be null)
//          tagLineCallBack LineCallBack : callback called on each new line
//          LPVOID CallBackUserParam : user parameter translated as callback arg
//     out  
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CTextFile::ParseLines(TCHAR* pszFileName,HANDLE hCancelEvent,tagLineCallBack LineCallBack,LPVOID CallBackUserParam)
{
    DWORD dwCurrentLine=1;
    TCHAR* PreviousPos;
    TCHAR* NextPos;
    TCHAR* pszLine;
    DWORD dwLineSize;
    TCHAR* pszContent;
    DWORD MaxLineSize;
    BOOL bCanceled;

    if (IsBadCodePtr((FARPROC)LineCallBack))
        return FALSE;

    if (!CTextFile::Read(pszFileName,&pszContent))
        return FALSE;

    bCanceled=FALSE;
    MaxLineSize=512;
    pszLine=new TCHAR[MaxLineSize];

    PreviousPos=pszContent;
    NextPos=_tcsstr(pszContent,_T("\r\n"));
    while (NextPos)
    {
        // make a local buffer to avoid callback modifying content
        dwLineSize=(DWORD)(NextPos-PreviousPos);
        if (dwLineSize+1>MaxLineSize)
        {
            MaxLineSize=dwLineSize+1;
            delete[] pszLine;
            pszLine=new TCHAR[MaxLineSize];
        }
        if (!pszLine)// buffer allocation error
        {
            delete[] pszContent;
            return FALSE;
        }

        memcpy(pszLine,PreviousPos,dwLineSize*sizeof(TCHAR));
        pszLine[dwLineSize]=0;

        if (hCancelEvent)
        {
            // if event is signaled
            if (WaitForSingleObject(hCancelEvent,0)==WAIT_OBJECT_0)
            {
                bCanceled=TRUE;
                // stop parsing
                break;
            }
        }

        // call callback
        if (!LineCallBack(pszFileName,pszLine,dwCurrentLine,CallBackUserParam))
        {
            bCanceled=TRUE;
            // stop parsing
            break;
        }

        dwCurrentLine++;
        PreviousPos=NextPos+2;// PreviousPos and NextPos are already in TCHAR so no *sizeof(TCHAR) required
        NextPos=_tcsstr(PreviousPos,_T("\r\n"));
    }
    // call callback
    if (!bCanceled)
    {
        if (*PreviousPos!=0)
        {
            // make local copy
            dwLineSize=(DWORD)_tcslen(PreviousPos);
            if (dwLineSize+1>MaxLineSize)
            {
                MaxLineSize=dwLineSize+1;
                delete[] pszLine;
                pszLine=new TCHAR[MaxLineSize];
            }
            if (!pszLine)// buffer allocation error
            {
                delete[] pszContent;
                return FALSE;
            }

            memcpy(pszLine,PreviousPos,dwLineSize*sizeof(TCHAR));
            pszLine[dwLineSize]=0;
            LineCallBack(pszFileName,pszLine,dwCurrentLine,CallBackUserParam);
        }
    }
    delete[] pszLine;
    delete[] pszContent;

    return TRUE;
}