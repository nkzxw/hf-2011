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

#include "LauncherOptions.h"

#include "../../../../../Tools/File/StdFileOperations.h"
#include "../../../../../Tools/Ini/IniFile.h"
#include <stdio.h>

CLauncherOptions::CLauncherOptions()
{
    this->FilteringType = FilteringType_ONLY_BASE_MODULE;
    *this->FilteringTypeFileName=0;

    *this->EmulatedRegistryConfigFile=0;
    
    this->StartingWay= StartingWay_FROM_NAME;
    this->ProcessId=0;
    this->ThreadId=0;
    *this->TargetName=0;
    this->TargetCommandLine=0;

    this->Flags = 0;
}

CLauncherOptions::~CLauncherOptions()
{
    if (this->TargetCommandLine)
        delete[] this->TargetCommandLine;
}

void CLauncherOptions::ReportError(TCHAR* ErrorMessage)
{
    ::MessageBox(0,ErrorMessage,_T("Error"),MB_ICONERROR);
}

BOOL CLauncherOptions::CheckConsistency(BOOL bSpyMode)
{
    TCHAR Msg[2*MAX_PATH];
    BOOL bRetValue = TRUE;
    TCHAR FilteringNameAbsolutePath[MAX_PATH];
    CStdFileOperations::GetAbsolutePath(this->FilteringTypeFileName,FilteringNameAbsolutePath);
    TCHAR TargetNameAbsolutePath[MAX_PATH];
    CStdFileOperations::GetAbsolutePath(this->TargetName,TargetNameAbsolutePath);
    TCHAR EmulatedRegistryConfigFileAbsolutePath[MAX_PATH];
    CStdFileOperations::GetAbsolutePath(this->EmulatedRegistryConfigFile,EmulatedRegistryConfigFileAbsolutePath);
    
    // if starting from name
    if (this->StartingWay == StartingWay_FROM_NAME)
    {
        // assume name is consistent
        if (*this->TargetName == 0)
        {
            this->ReportError(_T("Empty target filename"));
            bRetValue = FALSE;
        }
        else if (!CStdFileOperations::DoesFileExists(TargetNameAbsolutePath))
        {
            _sntprintf(Msg,2*MAX_PATH,_T("Target filename %s doesn't exists"),TargetNameAbsolutePath);
            this->ReportError(Msg);
            bRetValue = FALSE;
        }
    }
    else if (this->StartingWay == StartingWay_FROM_PROCESS_ID_AND_THREAD_ID_OF_SUSPENDED_PROCESS)
    {
        if ( (this->ProcessId == 0)
            || (this->ThreadId == 0)
            )
            {
                _sntprintf(Msg,2*MAX_PATH,_T("Bad ProcessId %u or ThreadId %u"),this->ProcessId,this->ThreadId);
                this->ReportError(Msg);
                bRetValue = FALSE;
            }
            
    }
    
    if (this->FilteringType != FilteringType_ONLY_BASE_MODULE)
    {
        // assume filtering file name is consistent
        if (*this->FilteringTypeFileName == 0)
        {
            this->ReportError(_T("Empty filtering filename"));
            bRetValue = FALSE;
        }
        else if (!CStdFileOperations::DoesFileExists(FilteringNameAbsolutePath))
        {
            _sntprintf(Msg,2*MAX_PATH,_T("Filtering filename %s doesn't exists"),FilteringNameAbsolutePath);
            this->ReportError(Msg);
            bRetValue = FALSE;
        }
    }
    
    if (*this->EmulatedRegistryConfigFile == 0)
    {
        this->ReportError(_T("Empty emulated registry filename"));
        bRetValue = FALSE;
    }
    else if (!CStdFileOperations::DoesFileExists(this->EmulatedRegistryConfigFile))
    {
        if (!bSpyMode)
        {
            _sntprintf(Msg,2*MAX_PATH,_T("Emulated registry filename %s doesn't exists"),EmulatedRegistryConfigFileAbsolutePath);
            this->ReportError(Msg);
            bRetValue = FALSE; 
        }
   
    }
    
    
    return bRetValue;
}

BOOL CLauncherOptions::Load(TCHAR* ConfigFileName,BOOL bCalledFromCommandLine) // if bCalledFromCommandLine don't force StartingWay_FROM_NAME
{
    if (!CStdFileOperations::DoesFileExists(ConfigFileName))
        return FALSE;

    CIniFile IniFile(ConfigFileName);
    IniFile.SetCurrentSectionName(_T("Options"));
  
    if (!bCalledFromCommandLine)
        this->StartingWay = StartingWay_FROM_NAME;

    // if name not already set, or not called from command line
    if ( (*this->TargetName==0) || (!bCalledFromCommandLine) )
        IniFile.Get(_T("TargetName"),_T(""),this->TargetName,MAX_PATH);

    // if cmd line not already set, or not called from command line
    if ( (this->TargetCommandLine==0) || (!bCalledFromCommandLine) )
    {
        if (this->TargetCommandLine)
            delete[] this->TargetCommandLine;
        this->TargetCommandLine = new TCHAR[2048];
        IniFile.Get(_T("TargetCmdLine"),_T(""),this->TargetCommandLine,2048);
    }

    // if FilteringTypeFileName not already set, or not called from command line
    if ( (*this->FilteringTypeFileName==0) || (!bCalledFromCommandLine) )
        IniFile.Get(_T("FilteringTypeFileName"),_T(""),this->FilteringTypeFileName,MAX_PATH);

    // if FilteringTypeFileName not already set, or not called from command line
    if ( (*this->EmulatedRegistryConfigFile==0) || (!bCalledFromCommandLine) )
        IniFile.Get(_T("EmulatedRegistryConfigFile"),_T(""),this->EmulatedRegistryConfigFile,MAX_PATH);
        
    if (!bCalledFromCommandLine) 
        IniFile.Get(_T("FilteringType"),(int)FilteringType_ONLY_BASE_MODULE,(int*)&this->FilteringType);

    return TRUE;
}
BOOL CLauncherOptions::Save(TCHAR* ConfigFileName)
{
    CIniFile IniFile(ConfigFileName);
    IniFile.SetCurrentSectionName(_T("Options"));
    
    IniFile.Set(_T("TargetName"),this->TargetName);
    IniFile.Set(_T("TargetCmdLine"),this->TargetCommandLine);
    IniFile.Set(_T("FilteringTypeFileName"),this->FilteringTypeFileName);
    IniFile.Set(_T("EmulatedRegistryConfigFile"),this->EmulatedRegistryConfigFile);
    IniFile.Set(_T("FilteringType"),(int)this->FilteringType);

    return TRUE;
}