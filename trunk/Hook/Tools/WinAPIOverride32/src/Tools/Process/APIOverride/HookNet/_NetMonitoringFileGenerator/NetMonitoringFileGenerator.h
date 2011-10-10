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
// Object: generate NET. monitoring files from .NET assemblies
//-----------------------------------------------------------------------------

#pragma once
// _WINSOCKAPI_ must be set in C/C++ preprocessor options
#include <windows.h>
#include <malloc.h>
#include <stdio.h>

#include <cor.h>
#include <CorError.h>

#include "../FunctionInfo.h"
#include "../../../../File/TextFile.h"

#define CNetMonitoringFileGenerator_ENUM_BUFFER_SIZE 100

class CNetMonitoringFileGenerator
{
private:
    IMetaDataImport* pIMetaDataImport;
    TCHAR szAssemblyName[MAX_PATH];
    WCHAR wAssemblyName[MAX_PATH];

    void ReportError(TCHAR* ErrorMsg,HRESULT hr);
    BOOL ParseMethods(mdTypeDef inTypeDef);
    BOOL ParseGlobalFunctions();
    BOOL GetClassName(mdTypeDef inTypeDef,WCHAR* pszClassName,DWORD pszClassNameMaxSize);
    BOOL ParseTypeDefInfo(mdTypeDef inTypeDef);
    BOOL ParseTypeDefs();
    
public:
    CLinkListSimple* pLinkListFunctions;// public for MonitoringFileBuilder

    CNetMonitoringFileGenerator(TCHAR* AssemblyFileName);
    ~CNetMonitoringFileGenerator(void);
    BOOL ParseAssembly();
    BOOL GenerateMonitoringFile(TCHAR* OutputFileName);
};
