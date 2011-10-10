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
// Object: extract resource from binaries
//-----------------------------------------------------------------------------

#include "BinResource.h"


//-----------------------------------------------------------------------------
// Name: ExtractBinResource
// Object: extract resource from binaries
// Parameters :
//     in : HMODULE hModule : module handle
//          TCHAR* strCustomResName : custom res name
//          int nResourceId : resource id
//          TCHAR* strOutputName : destination file name
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CBinResource::ExtractBinResource(HMODULE hModule, TCHAR* strCustomResName, int nResourceId, TCHAR* strOutputName )
{
    if (IsBadReadPtr(strCustomResName,sizeof(TCHAR)))
        return FALSE;
    if (IsBadReadPtr(strOutputName,sizeof(TCHAR)))
        return FALSE;

	HGLOBAL hResourceLoaded;
	HRSRC hRes;
	LPVOID lpResLock;
	DWORD dwSizeRes;

	// handle to resource
	hRes = FindResource( hModule, MAKEINTRESOURCE(nResourceId), strCustomResName );
    if (!hRes)
        return FALSE;
	
	// loads the specified resource into global memory
	hResourceLoaded = LoadResource( hModule, hRes ); 
    if (!hResourceLoaded)
        return FALSE;

	// get a pointer to the loaded resource
	lpResLock = LockResource( hResourceLoaded ); 
    if (!lpResLock)
        return FALSE;

	// get the size of the resource
	dwSizeRes = SizeofResource( hModule, hRes);


    CStdFileOperations::CreateDirectoryForFile(strOutputName);

    // create file
    HANDLE hFile = CreateFile(strOutputName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL,CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile==INVALID_HANDLE_VALUE)
        return FALSE;

    // write file content
    DWORD WrittenBytes=0;
    WriteFile(hFile,lpResLock, dwSizeRes,&WrittenBytes,NULL);

    // close file
	CloseHandle(hFile);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ExtractBinResource
// Object: extract resource from binaries
// Parameters :
//     in : TCHAR* ResLink : resource link with res protocol syntax res://sFile[/sType]/sID
//          TCHAR* strOutputName : destination file name
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CBinResource::ExtractBinResource(TCHAR* ResLink, TCHAR* strOutputName)
{
    if (IsBadReadPtr(ResLink,sizeof(TCHAR)))
        return FALSE;

    HMODULE hModule;
    TCHAR* ModuleName;
    TCHAR* strCustomResName;
    TCHAR* ResId;
    int nResourceId;
    TCHAR* psz;
    TCHAR* LocalResLink=(TCHAR*)_alloca((_tcslen(ResLink)+1)*sizeof(TCHAR));
    _tcscpy(LocalResLink,ResLink);

    //////////////////////////////////////////////
    // split res protocol into ModuleName,strCustomResName,ResId
    //////////////////////////////////////////////

    // res protocol syntax res://sFile[/sType]/sID
    psz=_tcsstr(LocalResLink,_T("res://"));
    if(psz)
        LocalResLink=psz+_tcslen(_T("res://"));

    ModuleName=LocalResLink;

    psz=_tcschr(LocalResLink,'/');
    if (!psz)
        return FALSE;

    *psz=0;
    strCustomResName=psz+1;

    psz=_tcschr(strCustomResName,'/');
    if (psz)
    {
        // /sType present
        *psz=0;
        ResId=psz+1;
    }
    else
    {
        // no /sType
        // point ResId on strCustomResName
        ResId=strCustomResName;
        // make strCustomResName point to an empty string
        strCustomResName--;
    }
    //////////////////////////////////////////////
    // End of split res protocol into ModuleName,strCustomResName,ResId
    //////////////////////////////////////////////

    hModule=GetModuleHandle(ModuleName);
    if (hModule==NULL)
        return FALSE;
    
    if (_stscanf(ResId,_T("%u"),&nResourceId)!=1)
        return FALSE;

    return CBinResource::ExtractBinResource(hModule, strCustomResName, nResourceId, strOutputName);
}