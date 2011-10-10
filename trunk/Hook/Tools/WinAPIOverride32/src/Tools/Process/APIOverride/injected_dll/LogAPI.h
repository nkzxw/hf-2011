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
// Object: manages the sending of logs to main application
//-----------------------------------------------------------------------------

#pragma once
#include <windows.h>
#include <malloc.h>

#include "APIOverrideKernel.h"
#include "struct.h"
#include "../../../LinkList/SingleThreaded/LinkListSingleThreaded.h"

class CLogAPI
{
private:
    typedef struct tagSEND_LOG_FIXED_SIZE_STRUCT
    {
        // NOTICE keep structure order

        DWORD dwCommande;
        DWORD dwFullMessageSize;// size of data without dwCommande and dwFullMessageSize sent to logging exe 
                                // mailslot already gives us amount of data receive, but it's allows us to do a check
        LOG_ENTRY_FIXED_SIZE LogEntry;
    }SEND_LOG_FIXED_SIZE_STRUCT,*PSEND_LOG_FIXED_SIZE_STRUCT;
public:
    static BOOL AddLogEntry(API_INFO* pAPIInfo,
                          LOG_INFOS* pLogInfo,
                          PBYTE ReturnValue,
                          double DoubleResult,
                          BOOL bFailure,
                          PBYTE ReturnAddress,
                          int iParametersType,
                          TCHAR* pszCallingModuleName,
                          PBYTE CallingModuleRelativeAddress,
                          PREGISTERS pRegistersBeforeCall,
                          PREGISTERS pRegistersAfterCall,
                          PBYTE CallerEbp,
                          DWORD ThreadId,
                          CLinkListSingleThreaded* pLinkListTlsData
                          );
};