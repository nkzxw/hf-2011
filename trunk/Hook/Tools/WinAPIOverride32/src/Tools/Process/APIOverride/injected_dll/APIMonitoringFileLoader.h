/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
originaly based from APISpy32 v2.1 from Yariv Kaplan @ WWW.INTERNALS.COM

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
// Object: load monitoring files
//-----------------------------------------------------------------------------

#include "../SupportedParameters.h"

#pragma once
#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "struct.h"
#include "ApiInfo.h"
#include "APIOverrideKernel.h"


typedef struct tagMonitoringFileLineParserUserParam
{
    MONITORING_FILE_INFOS* pMonitoringFileInfos;
}MONITORING_FILE_LINE_PARSER_USER_PARAM,*PMONITORING_FILE_LINE_PARSER_USER_PARAM;

///////////////////////////////////////////////////////
//////////////          functions        //////////////
///////////////////////////////////////////////////////
BOOL LoadMonitoringFile(TCHAR* pszFileName);
BOOL UnloadMonitoringFile(TCHAR* pszFileName);
PBYTE StrByteArrayToByteArray(TCHAR* pc,DWORD* pdwSize);
BOOL MonitoringFileLineParser(TCHAR* pszFileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam);
CLinkList* __stdcall CreateParameterConditionalLogContentListIfDoesntExist(PARAMETER_INFOS* pParameter);
CLinkList* __stdcall CreateParameterConditionalBreakContentListIfDoesntExist(PARAMETER_INFOS* pParameter);
BOOL __stdcall ParseFunctionDescription(TCHAR* pszFunctionDescription,OUT TCHAR** ppszFunctionName, OUT TCHAR** ppszParameters, OUT TCHAR** ppszOptions,OUT tagCALLING_CONVENTION* pCallingConvention);
BOOL __stdcall ParseParameters(API_INFO *pAPIInfo,TCHAR* pszParameters,TCHAR* pszFileName,DWORD dwLineNumber);
BOOL __stdcall ParseOptions(API_INFO *pAPIInfo,TCHAR* pszOptions,BOOL bAlreadyHooked,TCHAR* pszFileName,DWORD dwLineNumber);

BOOL UnloadAllMonitoringFiles();
BOOL GetMonitoringFileName(MONITORING_FILE_INFOS* pMonitoringFileInfo,TCHAR* pszFileName);
BOOL GetMonitoringFileInfos(TCHAR* pszFileName,MONITORING_FILE_INFOS** ppMonitoringFileInfo);