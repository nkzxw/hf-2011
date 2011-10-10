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
// Object: manages process internal calls
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "../InterProcessCommunication.h"
#include "APIOverrideKernel.h"
#include "../ApiOverrideFuncAndParams.h"
#include "../../MailSlot/MailSlotClient.h"
#include "../../../Exception/HardwareException.h"
#include "../../../Thread/ThreadContext.h"

class CProcessInternalCallRequest
{
private:
    #define PROCESSINTERNALCALLREQUESTINTHREAD_OPCODESIZE 16
    typedef struct tagProcessInternalCallRequestExParam
    {
        FARPROC pFunc;
        int NbParams;
        PSTRUCT_FUNC_PARAM pParams;
        REGISTERS* pRegisters;
        PBYTE* pRet;
        double* pFloatingResult;
        HANDLE hThread;
        HANDLE hEndEvent;
        BOOL bSuccess;
        tagCALLING_CONVENTION CallingConvention;
        PBYTE Eip;
        BYTE OriginalBytes[PROCESSINTERNALCALLREQUESTINTHREAD_OPCODESIZE];
    }PROCESS_INTERNAL_CALL_REQUEST_EX_PARAM;
    static void __stdcall ProcessInternalCallRequestInThread(PROCESS_INTERNAL_CALL_REQUEST_EX_PARAM* pProcessInternalCallRequestExParam);
    static BOOL ProcessInternalCallRequest(FARPROC pFunc,tagCALLING_CONVENTION CallingConvention,int NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE* pRet,double* pFloatingResult);
public:
    static BOOL ProcessInternalCallRequestFromMailSlot(PBYTE pBuffer,CMailSlotClient* pMailSlotClient);
    static BOOL __stdcall ProcessInternalCallRequestEx(FARPROC pFunc,tagCALLING_CONVENTION CallingConvention,int NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE* pRet,double* pFloatingResult,DWORD ThreadId,DWORD dwTimeOut);
};
